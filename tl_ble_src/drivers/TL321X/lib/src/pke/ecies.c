/********************************************************************************************************
 * @file    ecies.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/

#include "lib/include/pke/pke_config.h"


#ifdef SUPPORT_ECIES

#include "lib/include/pke/ecies.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"
//#include "lib/include/hash/hmac.h"



//#define DEBUG_ECIES


/**
 * @brief       Elliptic Curve Integrated Encryption Scheme (ECIES) core interface
 * @param[in]   ecies_ctx           - types of ecies structure
 * @param[in]   curve               - ecc curve struct pointer, please make sure it is valid
 * @param[in]   msg                 - original message, plaintext.
 * @param[in]   msg_bytes           - byte length of msg.
 * @param[in]   sender_tmp_pri_key  - sender's ephemeral private key, big-endian.
 * @param[in]   receiver_pub_key    - reveiver's public key, big-endian.
 * @param[in]   conversion_form     - curve point representation.
 * @param[in]   kdf_ctx             - key derivation function structure.
 * @param[in]   mac_ctx             - message authentication code structure.
 * @param[in]   enc_ctx             - symmetric encryption scheme structure.
 * @param[out]  cipher              - encryption result, ciphertext.
 * @param[out]  cipher_bytes        - byte length of cipher.
 * @return      ECIES_SUCCESS(success)     other:error
 * @note
   @verbatim
      -# 1.the result ciphertext consists of three parts. the 1st part is a point, the 2nd part is
           internal ciphertext, the 3rd part is mac.
   @endverbatim
 */
unsigned int ecies_encrypt(ECIES_STD *ecies_ctx,  eccp_curve_t *curve,  unsigned char *msg, unsigned int msg_bytes,
        unsigned char *sender_tmp_pri_key, unsigned char *receiver_pub_key, EC_POINT_FORM point_form,
        E_KDF_BASE *kdf_ctx, E_MAC_BASE *mac_ctx, E_ENC_BASE *enc_ctx, unsigned char *cipher,
        unsigned int *cipher_bytes)
{
    unsigned char enc_key[ECIES_BLOCK_ENC_KEY_MAX_BYTE_LEN];
    unsigned int k[ECCP_MAX_WORD_LEN];
    unsigned int tmp[2 * ECCP_MAX_WORD_LEN];
    unsigned int pWordLen;
    unsigned int pByteLen;
    unsigned int nByteLen;
    unsigned int nWordLen;
    unsigned int r_bytes;
    unsigned int ret;
    (void)msg;
    (void)msg_bytes;

    pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);

    //1. get r, 1st part of out
    k[nWordLen - 1] = 0;
    if(sender_tmp_pri_key)
    {
        //transfer to U32 big number.
        reverse_byte_array((unsigned char *)sender_tmp_pri_key, (unsigned char *)k, nByteLen);

        //make sure k in [1, n-1]
        ret = uint32_integer_check(k, curve->eccp_n, nWordLen, ECIES_ZERO_ALL, ECIES_INTEGER_TOO_BIG,
                ECIES_SUCCESS);
        if(ECIES_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        ret = eccp_get_pubkey_from_prikey(curve, (unsigned char *)sender_tmp_pri_key, (unsigned char *)tmp);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}
    }
    else
    {
        ret = eccp_getkey(curve, (unsigned char *)k, (unsigned char *)tmp);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        //transfer to U32 big number.
        reverse_byte_array((unsigned char *)k, (unsigned char *)k, nByteLen);
    }

    ret = ecies_ctx->point_compress(curve, (unsigned char *)tmp, ((unsigned char *)tmp) + pByteLen, point_form, 
            cipher, &r_bytes);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    //print_buf_U8(out,r_len, "point-compress-big-endian");
#endif

    //2. get ([k]pub_key).x
    tmp[pWordLen - 1] = 0;
    tmp[pWordLen + pWordLen - 1] = 0;
    reverse_byte_array(receiver_pub_key, (unsigned char *)tmp, pByteLen);
    reverse_byte_array(receiver_pub_key + pByteLen, (unsigned char *)(tmp+pWordLen), pByteLen);

    ret = eccp_pointVerify(curve, tmp, tmp+pWordLen);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = eccp_pointMul(curve, k, tmp, tmp+pWordLen, k, NULL);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    reverse_byte_array((unsigned char *)k, (unsigned char *)k, pByteLen);

#ifdef DEBUG_ECIES
    print_buf_U8((unsigned char *)k, pByteLen,  "kP.x");
#endif

    //set enc key and output buffer, this must be done before KDF action.
    enc_ctx->output = cipher + r_bytes;
    if(XOR_ENC == enc_ctx->enc_type)
    {
        enc_ctx->key = enc_ctx->output;
    }
    else
    {
        enc_ctx->key = enc_key;
    }

    //3. get k_enc and k_mac from KDF.
    kdf_ctx->input       =  (unsigned char *)k;
    kdf_ctx->input_bytes = pByteLen;
    if(ENC_MAC_ORDER == ecies_ctx->enc_mac_key_order)
    {
        kdf_ctx->out1       = enc_ctx->key;
        kdf_ctx->out1_bytes = enc_ctx->key_bytes;
        kdf_ctx->out2       = mac_ctx->key;
        kdf_ctx->out2_bytes = mac_ctx->key_bytes;
    }
    else
    {
        kdf_ctx->out1       = mac_ctx->key;
        kdf_ctx->out1_bytes = mac_ctx->key_bytes;
        kdf_ctx->out2       = enc_ctx->key;
        kdf_ctx->out2_bytes = enc_ctx->key_bytes;
    }

#ifdef DEBUG_ECIES
    print_buf_U8(kdf_ctx->input, pByteLen, "kdf - input");
#endif

    ret = kdf_ctx->kdf_fun_imp(kdf_ctx);
    if(ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    print_buf_U8(kdf_ctx->out1,kdf_ctx->out1_bytes, "kdf-enc");
    print_buf_U8(kdf_ctx->out2,kdf_ctx->out2_bytes, "kdf-mac");
#endif

    //4. c = Enc(k_enc, msg)
#ifdef DEBUG_ECIES
    //print_buf_U8( kdf_ctx->out,enc_ctx->key_len, "enc-key");
#endif

    ret = enc_ctx->enc_fun_imp(enc_ctx);
    if(ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    print_buf_U8(enc_ctx->output,enc_ctx->output_bytes, "cipher");
    //print_buf_U8(out,r_len, "out after enc");
#endif

    //5. get d = mac(k_mac, c)
    //set mac msg, msg_bytes and mac buffer, this must be done before MAC action.
    mac_ctx->msg       = enc_ctx->output;
    mac_ctx->msg_bytes = enc_ctx->output_bytes;
    mac_ctx->mac       = cipher + r_bytes + enc_ctx->output_bytes;

#ifdef DEBUG_ECIES
    print_buf_U8(mac_ctx->key,mac_ctx->key_bytes, "mac_key");
    print_buf_U8(mac_ctx->msg,mac_ctx->msg_bytes, "mac input msg");
    print_buf_U8(mac_ctx->appendix,mac_ctx->appendix_bytes, "mac input appendix");
#endif

    ret = mac_ctx->mac_imp(mac_ctx);
    if(ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    print_buf_U8(mac_ctx->mac, mac_ctx->mac_bytes, "mac value");
    //print_buf_U8(out,r_len, "out after mac");
#endif

    //6. out = r || c || d
    *cipher_bytes = r_bytes + enc_ctx->output_bytes + mac_ctx->mac_bytes;

    return ECIES_SUCCESS;
}


/**
 * @brief       Elliptic Curve Integrated Encryption Scheme (ECIES) Decrypt core interface
 * @param[in]   ecies_ctx           - types of ecies structure
 * @param[in]   curve               - ecc curve struct pointer, please make sure it is valid
 * @param[in]   cipher              - ciphertext
 * @param[in]   cipher_bytes        - byte length of ciphertext
 * @param[in]   receiver_pri_key    - receiver's private key, big-endian.
 * @param[in]   kdf_ctx             - key derivation function structure.
 * @param[in]   mac_ctx             - message authentication code structure.
 * @param[in]   dec_ctx             - symmetric encryption scheme structure.
 * @param[out]  msg                 - decryption result, plaintext.
 * @param[out]  msg_bytes           - byte length of msg.
 * @return      ECIES_SUCCESS(success)     other:error
 * @note
   @verbatim
      -# 1.the input ciphertext consists of three parts. the 1st part is a point, the 2nd part is
 *        internal ciphertext, the 3rd part is mac.
   @endverbatim
 */
unsigned int ecies_decrypt(ECIES_STD *ecies_ctx,  eccp_curve_t *curve, unsigned char *cipher,
        unsigned int cipher_bytes, unsigned char *receiver_pri_key, E_KDF_BASE *kdf_ctx, E_MAC_BASE *mac_ctx,
        E_ENC_BASE *dec_ctx, unsigned char *msg, unsigned int *msg_bytes)
{
    unsigned char enc_key[ECIES_BLOCK_ENC_KEY_MAX_BYTE_LEN];
    unsigned char mac_buf[ECIES_MAC_MAX_BYTE_LEN];
    unsigned int rx[ECCP_MAX_WORD_LEN];
    unsigned int ry[ECCP_MAX_WORD_LEN];
    unsigned int k[ECCP_MAX_WORD_LEN];
    unsigned int nWordLen;
    unsigned int nByteLen;
    unsigned int pWordLen;
    unsigned int pByteLen;
    unsigned int ret;

    pWordLen = GET_WORD_LEN(curve->eccp_p_bitLen);
    pByteLen = GET_BYTE_LEN(curve->eccp_p_bitLen);
    nWordLen = GET_WORD_LEN(curve->eccp_n_bitLen);
    nByteLen = GET_BYTE_LEN(curve->eccp_n_bitLen);

/*    //get msg length
    ecies_ctx->get_point_len_from_ciphertext(curve, cipher, cipher_bytes, &point_bytes);

   if(dec_ctx->enc_type == XOR_ENC)
   {
       *msg_len = dec_ctx->key_len;
   }
   else
   {
        mac_ctx->msg_len = 0;
        ret = mac_ctx->mac_imp(mac_ctx);
        if(PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {;}

        *msg_len = cipher_len - point_len - mac_ctx->out_len;
   }*/

    //set enc key and output buffer, this must be done before KDF action.
    dec_ctx->output = msg;
    if(XOR_ENC == dec_ctx->enc_type)
    {
        dec_ctx->key = dec_ctx->output;
    }
    else
    {
        dec_ctx->key = enc_key;
    }

    //set mac ctx
    mac_ctx->msg       = dec_ctx->input;
    mac_ctx->msg_bytes = dec_ctx->input_bytes;
    mac_ctx->mac       = mac_buf;

#ifdef DEBUG_ECIES
    //printf("msg-len : %d\n\r", *msg_len);
    //print_buf_U8(cipher, cipher_len, "cipher-input");
#endif

    //1. get [pri_key]r.x
    rx[pWordLen - 1] = 0;
    ry[pWordLen - 1] = 0;
    ret = ecies_ctx->point_decompress(curve, cipher, rx, ry);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    //check point r
    ret = eccp_pointVerify(curve, rx, ry);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    k[nWordLen - 1] = 0;
    reverse_byte_array(receiver_pri_key, (unsigned char *)k, nByteLen);

    //make sure pri_key in [1, n-1]
    ret = uint32_integer_check(k, curve->eccp_n, nWordLen, ECIES_ZERO_ALL, ECIES_INTEGER_TOO_BIG,
            ECIES_SUCCESS);
    if(ECIES_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    print_BN_buf_U32(rx,pWordLen, "rx");
    print_BN_buf_U32(ry,pWordLen, "ry");
#endif

    ret = eccp_pointMul(curve, k, rx, ry, k, NULL);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}
    reverse_byte_array((unsigned char *)k, (unsigned char *)k, pByteLen);

#ifdef DEBUG_ECIES
    //print_buf_U8(s, pByteLen, "s-dec");
#endif

    //2. get k_enc and k_mac from KDF.
    kdf_ctx->input       =  (unsigned char *)k;
    kdf_ctx->input_bytes = pByteLen;
    if(ENC_MAC_ORDER == ecies_ctx->enc_mac_key_order)
    {
        kdf_ctx->out1       = dec_ctx->key;
        kdf_ctx->out1_bytes = dec_ctx->key_bytes;
        kdf_ctx->out2       = mac_ctx->key;
        kdf_ctx->out2_bytes = mac_ctx->key_bytes;
    }
    else
    {
        kdf_ctx->out1       = mac_ctx->key;
        kdf_ctx->out1_bytes = mac_ctx->key_bytes;
        kdf_ctx->out2       = dec_ctx->key;
        kdf_ctx->out2_bytes = dec_ctx->key_bytes;
    }

    ret = kdf_ctx->kdf_fun_imp(kdf_ctx);
    if(ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    //printf("kdf-outlen: %d\n\r", kdf_ctx->out_bytes);
    //print_buf_U8(kdf_ctx->out,kdf_ctx->out_bytes, "kdf-dec");
#endif

    //3. d ?= mac(k_mac, c)
    ret = mac_ctx->mac_imp(mac_ctx);
    if(ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    //print_buf_U8(mac_ctx->out,mac_ctx->out_len, "mac-dec -value");
    //print_buf_U8(cipher + cipher_len - mac_ctx->out_len,mac_ctx->out_len, "mac-dec-compare");
#endif

    if(memcmp_(cipher + cipher_bytes - mac_ctx->mac_bytes, mac_ctx->mac, mac_ctx->mac_bytes))
    {
        return ECIES_ERROR;
    }
    else
    {;}

    //4. msg = dec(k_enc,c)
#ifdef DEBUG_ECIES
    //print_buf_U8(dec_ctx->key,dec_ctx->key_len, "dec-key");
    //print_buf_U8(dec_ctx->msg,dec_ctx->msg_len, "dec-cip");
#endif

    ret = dec_ctx->dec_fun_imp(dec_ctx);
    if(ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {;}

#ifdef DEBUG_ECIES
    //print_buf_U8(dec_ctx->out,dec_ctx->msg_len, "dec-out");
#endif

    *msg_bytes = dec_ctx->output_bytes;// because may be padding

    return ECIES_SUCCESS;
}


/**
 * @brief       Elliptic Curve Integrated Encryption Scheme (ECIES) ANSI-X963-2001
 * @param[in]   curve                 - ecc curve struct pointer, please make sure it is valid
 * @param[in]   msg                   - original message, plaintext.
 * @param[in]   msg_bytes             - byte length of msg.
 * @param[in]   shared_info1          - optional, shared information, for KDF.
 * @param[in]   shared_info1_bytes    - byte length of shared_info1.
 * @param[in]   shared_info2          - optional, shared information, for MAC.
 * @param[in]   shared_info2_bytes    - byte length of shared_info2.
 * @param[in]   sender_tmp_pri_key    - sender's ephemeral private key, big-endian.
                                        if you do not have this, please set this parameter to be NULL,
                                        it will be generated inside.
 * @param[in]   receiver_pub_key      - reveiver's public key, big-endian.
 * @param[in]   conversion_form       - curve point representation.
 * @param[in]   kdf_hash_alg          - specific hash algorithm for KDF.
 * @param[in]   mac_hash_alg          - specific hash algorithm for MAC.
 * @param[in]   mac_k_bytes           - key length of the MAC.
 * @param[out]  cipher                - encryption result, ciphertext.
 * @param[out]  cipher_bytes          - byte length of cipher.
 * @return      ECIES_SUCCESS(success)     other:error
 * @note
   @verbatim
      -# 1. if no shared_info1 needs to be provided, set the shared_info1 to NULL and the shared_info1_len to 0
      -# 2. if no shared_info2 needs to be provided, set the shared_info2 to NULL and the shared_info2_len to 0
      -# 3. if you do not have local_tmp_pri_key, please set the parameter to be NULL, it will be generated inside.
            this is recommended.
      -# 4. the result ciphertext consists of three parts. the 1st part is a point, the 2nd part is
            internal ciphertext, the 3rd part is mac.
   @endverbatim
 */
unsigned int ansi_x963_2001_ecies_encrypt( eccp_curve_t *curve,  unsigned char *msg, unsigned int msg_bytes,
         unsigned char *shared_info1, unsigned int shared_info1_bytes,  unsigned char *shared_info2,
        unsigned int shared_info2_bytes, unsigned char *sender_tmp_pri_key, unsigned char *receiver_pub_key,
        EC_POINT_FORM point_form, HASH_ALG kdf_hash_alg, HASH_ALG mac_hash_alg,
        unsigned int mac_k_bytes, unsigned char *cipher, unsigned int *cipher_bytes)
{
    unsigned char hmac_key[ECIES_MAC_KEY_MAX_BYTE_LEN];
    E_KDF_ANSI_X963_2001_CTX kdf_ctx;
    E_XOR_ENC_CTX xor_ctx;
    E_HMAC_CTX hmac_ctx;
    ECIES_STD ecies_ctx;

    if((NULL == curve)||(NULL == msg)||(NULL == receiver_pub_key)||(NULL == cipher)||(NULL == cipher_bytes))
    {
        return ECIES_POINTOR_NULL;
    }
    else if((0 == msg_bytes))
    {
        return ECIES_INVALID_INPUT;
    }
    else if((NULL == shared_info1)&&(0 != shared_info1_bytes))
    {
        return ECIES_INVALID_INPUT;
    }
    else if((NULL == shared_info2)&&(0 != shared_info2_bytes))
    {
        return ECIES_INVALID_INPUT;
    }
    else if(HASH_SUCCESS != check_hash_alg(kdf_hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if(HASH_SUCCESS != check_hash_alg(mac_hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if((mac_k_bytes < ECIES_MAC_KEY_ANSI_X963_MIN_BYTE_LEN) || (mac_k_bytes > ECIES_MAC_KEY_MAX_BYTE_LEN))
    {
        return ECIES_INVALID_INPUT;
    }
    else
    {;}

    ecies_ansi_x963_ctx_init(&ecies_ctx, ENC_MAC_ORDER);

    ansi_x963_2001_kdf_init(&kdf_ctx, shared_info1, shared_info1_bytes, kdf_hash_alg);

    e_xor_enc_init(&xor_ctx, msg, msg_bytes);

    e_hmac_init(&hmac_ctx, hmac_key, mac_k_bytes, shared_info2, shared_info2_bytes, mac_hash_alg);

    return ecies_encrypt(&ecies_ctx, curve, msg, msg_bytes, sender_tmp_pri_key, receiver_pub_key,
        point_form, (E_KDF_BASE *)&kdf_ctx, (E_MAC_BASE *)&hmac_ctx, (E_ENC_BASE *)&xor_ctx,
        cipher, cipher_bytes);
}


/**
 * @brief       Elliptic Curve Integrated Encryption Scheme (ECIES) Decrypt ANSI-X963-2001
 * @param[in]   curve                 - ecc curve struct pointer, please make sure it is valid
 * @param[in]   cipher                - original message, plaintext.
 * @param[in]   cipher_len            - byte length of msg.
 * @param[in]   receiver_pri_key      - optional, shared information, for KDF.
 * @param[in]   shared_info1          - byte length of shared_info1.
 * @param[in]   shared_info1_bytes    - optional, shared information, for MAC.
 * @param[in]   shared_info2          - byte length of shared_info2.
 * @param[in]   shared_info2_bytes    - sender's ephemeral private key, big-endian.
 * @param[in]   kdf_hash_alg          - reveiver's public key, big-endian.
 * @param[in]   mac_hash_alg          - curve point representation.
 * @param[in]   mac_k_bytes           - specific hash algorithm for KDF.
 * @param[out]  msg                   - decryption result.
 * @param[out]  msg_bytes             - byte length of msg.
 * @return      ECIES_SUCCESS(success)     other:error
 * @note
   @verbatim
      -# 1. if no shared_info1 needs to be provided, set the shared_info1 to NULL and the shared_info1_len to 0
      -# 2. if no shared_info2 needs to be provided, set the shared_info2 to NULL and the shared_info2_len to 0
      -# 3. the input cipher consists of three parts. the 1st part is a point, the 2nd part is
            internal ciphertext, the 3rd part is mac.
   @endverbatim
 */
unsigned int ansi_x963_2001_ecies_decrypt( eccp_curve_t *curve, unsigned char *cipher, unsigned int cipher_bytes,
        unsigned char *receiver_pri_key,  unsigned char *shared_info1, unsigned int shared_info1_bytes,
         unsigned char *shared_info2, unsigned int shared_info2_bytes, HASH_ALG kdf_hash_alg,
        HASH_ALG mac_hash_alg, unsigned int mac_k_bytes, unsigned char *msg, unsigned int *msg_bytes)
{
    unsigned char hmac_key[ECIES_MAC_KEY_MAX_BYTE_LEN];
    unsigned int point_bytes;
    unsigned int mac_bytes;
    E_KDF_ANSI_X963_2001_CTX kdf_ctx;
    E_XOR_ENC_CTX xor_ctx;
    E_HMAC_CTX hmac_ctx;
    ECIES_STD ecies_ctx;
    unsigned int ret;

    if((NULL == curve)||(NULL == cipher)||(NULL == receiver_pri_key)||(NULL == msg)||(NULL == msg_bytes))
    {
        return ECIES_POINTOR_NULL;
    }
    else if((NULL == shared_info1)&&(0 != shared_info1_bytes))
    {
        return ECIES_INVALID_INPUT;
    }
    else if((NULL == shared_info2)&&(0 != shared_info2_bytes))
    {
        return ECIES_INVALID_INPUT;
    }
    else if(HASH_SUCCESS != check_hash_alg(kdf_hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if(HASH_SUCCESS != check_hash_alg(mac_hash_alg))
    {
        return HASH_INPUT_INVALID;
    }
    else if((mac_k_bytes < ECIES_MAC_KEY_ANSI_X963_MIN_BYTE_LEN) || (mac_k_bytes > ECIES_MAC_KEY_MAX_BYTE_LEN))
    {
        return ECIES_INVALID_INPUT;
    }
    else
    {;}

    ecies_ansi_x963_ctx_init(&ecies_ctx, ENC_MAC_ORDER);

    e_hmac_init(&hmac_ctx, hmac_key, mac_k_bytes, shared_info2, shared_info2_bytes, mac_hash_alg);

    //Because XOR encryption requires the length of the message,
    //it is necessary to obtain the length of the mac
    mac_bytes = hmac_ctx.base.mac_bytes;

    ret = ecies_ctx.get_point_len_from_ciphertext(curve, cipher, cipher_bytes, &point_bytes);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    if(cipher_bytes <= point_bytes + mac_bytes)
    {
        return ECIES_INVALID_INPUT;
    }
    else
    {;}

    *msg_bytes = cipher_bytes - point_bytes - mac_bytes;

    e_xor_enc_init(&xor_ctx, cipher+point_bytes, *msg_bytes);

    ansi_x963_2001_kdf_init(&kdf_ctx, shared_info1, shared_info1_bytes, kdf_hash_alg);

    return ecies_decrypt(&ecies_ctx, curve, cipher, cipher_bytes, receiver_pri_key,
        (E_KDF_BASE *)&kdf_ctx, (E_MAC_BASE *)&hmac_ctx, (E_ENC_BASE *)&xor_ctx, msg, msg_bytes);
}

#endif

