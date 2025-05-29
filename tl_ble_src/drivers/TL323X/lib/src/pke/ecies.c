/*! @file ecies.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_ECIES

#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/ecies.h"
#include "lib/include/trng/trng.h"
// #include "../../crypto_include/hash_hmac/hmac.h"

#if 0
#define DEBUG_ECIES
#endif

/**
 * @brief           Elliptic Curve Integrated Encryption Scheme (ECIES) core interface
 * @param[in]       ecies_ctx            - types of ecies structure
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid
 * @param[in]       msg                  - original message, plaintext.
 * @param[in]       msg_len              - byte length of msg.
 * @param[in]       sender_tmp_pri_key   - sender's ephemeral private key, big-endian.
 * @param[in]       receiver_pub_key     - receiver's public key, big-endian.
 * @param[in]       point_form           - curve point representation.
 * @param[in]       kdf_ctx              - key derivation function structure.
 * @param[in]       mac_ctx              - message authentication code structure.
 * @param[in]       enc_ctx              - symmetric encryption scheme structure.
 * @param[out]      cipher               - encryption result, ciphertext.
 * @param[out]      cipher_len           - byte length of cipher.
 * @return          ECIES_SUCCESS(success)     other:error
 * @note
 *        1.the result ciphertext consists of three parts. the 1st part is a point, the 2nd part is internal ciphertext, the 3rd part is mac.
 */
static unsigned int ecies_encrypt(ecies_std_t *ecies_ctx, const eccp_curve_t *curve, const unsigned char *msg, unsigned int msg_len, unsigned char *sender_tmp_pri_key,
                                  unsigned char *receiver_pub_key, ec_point_form_e point_form, kdf_base_t *kdf_ctx, mac_base_t *mac_ctx, enc_base_t *enc_ctx, unsigned char *cipher,
                                  unsigned int *cipher_len)
{
    unsigned char enc_key[ECIES_BLOCK_ENC_K_MAX_BYTE_LEN];
    unsigned int k[ECCP_MAX_WORD_LEN];
    unsigned int tmp[ECCP_MAX_WORD_LEN << 1];
    unsigned int p_wlen;
    unsigned int p_len;
    unsigned int n_len;
    unsigned int n_wlen;
    unsigned int r_bytes;
    unsigned int ret;

    (void)msg;
    (void)msg_len;

    p_wlen = get_word_len(curve->eccp_p_bitLen);
    p_len = get_byte_len(curve->eccp_p_bitLen);
    n_wlen = get_word_len(curve->eccp_n_bitLen);
    n_len = get_byte_len(curve->eccp_n_bitLen);

    // 1. get r, 1st part of out
    k[n_wlen - 1u] = 0u;
    if (NULL != sender_tmp_pri_key)
    {
        // transfer to unsigned int big number.
        reverse_byte_array((unsigned char *)sender_tmp_pri_key, (unsigned char *)k, n_len);

        // make sure k in [1, n-1]
        ret = uint32_integer_check(k, curve->eccp_n, n_wlen, ECIES_ZERO_ALL, ECIES_INTEGER_TOO_BIG, ECIES_SUCCESS);
        if (ECIES_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = eccp_get_pubkey_from_prikey(curve, (unsigned char *)sender_tmp_pri_key, (unsigned char *)tmp);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }
    else
    {
        ret = eccp_getkey(curve, (unsigned char *)k, (unsigned char *)tmp);
        if (PKE_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        // transfer to unsigned int big number.
        reverse_byte_array((unsigned char *)k, (unsigned char *)k, n_len);
    }

    ret = ecies_ctx->point_compress(curve, (unsigned char *)tmp, &(((unsigned char *)tmp)[p_len]), point_form, cipher, &r_bytes);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
    print_buf_u8(out, r_len, "point-compress-big-endian");
#endif

    // 2. get ([k]pub_key).x
    tmp[p_wlen - 1u] = 0u;
    tmp[p_wlen + p_wlen - 1u] = 0u;
    reverse_byte_array(receiver_pub_key, (unsigned char *)tmp, p_len);
    reverse_byte_array(&receiver_pub_key[p_len], (unsigned char *)(&tmp[p_wlen]), p_len);

    ret = eccp_pointverify(curve, tmp, &tmp[p_wlen]);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = eccp_pointmul(curve, k, tmp, &tmp[p_wlen], k, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    reverse_byte_array((unsigned char *)k, (unsigned char *)k, p_len);

#ifdef DEBUG_ECIES
    print_buf_u8((unsigned char *)k, p_len, "kP.x");
#endif

    // set enc_t key and output buffer, this must be done before KDF action.
    enc_ctx->output = &cipher[r_bytes];
    if (XOR_ENC == enc_ctx->enc_type_e)
    {
        enc_ctx->key = enc_ctx->output;
    }
    else
    {
        enc_ctx->key = enc_key;
    }

    // 3. get k_enc and k_mac from KDF.
    kdf_ctx->input = (unsigned char *)k;
    kdf_ctx->input_bytes = p_len;
    if (ENC_MAC_ORDER == ecies_ctx->enc_mac_key_order)
    {
        kdf_ctx->out1 = enc_ctx->key;
        kdf_ctx->out1_bytes = enc_ctx->key_bytes;
        kdf_ctx->out2 = mac_ctx->key;
        kdf_ctx->out2_bytes = mac_ctx->key_bytes;
    }
    else
    {
        kdf_ctx->out1 = mac_ctx->key;
        kdf_ctx->out1_bytes = mac_ctx->key_bytes;
        kdf_ctx->out2 = enc_ctx->key;
        kdf_ctx->out2_bytes = enc_ctx->key_bytes;
    }

#ifdef DEBUG_ECIES
    print_buf_u8(kdf_ctx->input, p_len, "kdf - input");
#endif

    ret = kdf_ctx->kdf_fun_imp(kdf_ctx);
    if (ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
    print_buf_u8(kdf_ctx->out1, kdf_ctx->out1_bytes, "kdf-enc_t");
    print_buf_u8(kdf_ctx->out2, kdf_ctx->out2_bytes, "kdf-mac");
#endif

// 4. c = enc_t(k_enc, msg)
#ifdef DEBUG_ECIES
    print_buf_u8(kdf_ctx->out, enc_ctx->key_len, "enc_t-key");
#endif

    ret = enc_ctx->enc_fun_imp(enc_ctx);
    if (ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
    print_buf_u8(enc_ctx->output, enc_ctx->output_bytes, "cipher");
    print_buf_u8(out, r_len, "out after enc_t");
#endif

    // 5. get d = mac(k_mac, c)
    // set mac msg, msg_len and mac buffer, this must be done before MAC action.
    mac_ctx->msg = enc_ctx->output;
    mac_ctx->msg_len = enc_ctx->output_bytes;
    mac_ctx->mac = &cipher[r_bytes + enc_ctx->output_bytes];

#ifdef DEBUG_ECIES
    print_buf_u8(mac_ctx->key, mac_ctx->key_bytes, "mac_key");
    print_buf_u8(mac_ctx->msg, mac_ctx->msg_len, "mac input msg");
    print_buf_u8(mac_ctx->appendix, mac_ctx->appendix_bytes, "mac input appendix");
#endif

    ret = mac_ctx->mac_imp(mac_ctx);
    if (ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
    print_buf_u8(mac_ctx->mac, mac_ctx->mac_bytes, "mac value");
    print_buf_u8(out, r_len, "out after mac");
#endif

    // 6. out = r || c || d
    *cipher_len = r_bytes + enc_ctx->output_bytes + mac_ctx->mac_bytes;

    return ECIES_SUCCESS;
}

/**
 * @brief           Elliptic Curve Integrated Encryption Scheme (ECIES) Decrypt core interface
 * @param[in]       ecies_ctx            - types of ecies structure
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid
 * @param[in]       cipher               - ciphertext
 * @param[in]       cipher_len           - byte length of ciphertext
 * @param[in]       receiver_pri_key     - receiver's private key, big-endian.
 * @param[in]       kdf_ctx              - key derivation function structure.
 * @param[in]       mac_ctx              - message authentication code structure.
 * @param[in]       dec_ctx              - symmetric encryption scheme structure.
 * @param[out]      msg                  - decryption result, plaintext.
 * @param[out]      msg_len              - byte length of msg.
 * @return          ECIES_SUCCESS(success)     other:error
 * @note
 *        1.the input ciphertext consists of three parts. the 1st part is a point, the 2nd part is
 *           internal ciphertext, the 3rd part is mac.
 */
static unsigned int ecies_decrypt(ecies_std_t *ecies_ctx, const eccp_curve_t *curve, unsigned char *cipher, unsigned int cipher_len, unsigned char *receiver_pri_key,
                                  kdf_base_t *kdf_ctx, mac_base_t *mac_ctx, enc_base_t *dec_ctx, unsigned char *msg, unsigned int *msg_len)
{
    unsigned char enc_key[ECIES_BLOCK_ENC_K_MAX_BYTE_LEN];
    unsigned char mac_buf[ECIES_MAC_MAX_BYTE_LEN];
    unsigned int rx[ECCP_MAX_WORD_LEN];
    unsigned int ry[ECCP_MAX_WORD_LEN];
    unsigned int k[ECCP_MAX_WORD_LEN];
    unsigned int n_wlen;
    unsigned int n_len;
    unsigned int p_wlen;
    unsigned int p_len;
    unsigned int ret;

    p_wlen = get_word_len(curve->eccp_p_bitLen);
    p_len = get_byte_len(curve->eccp_p_bitLen);
    n_wlen = get_word_len(curve->eccp_n_bitLen);
    n_len = get_byte_len(curve->eccp_n_bitLen);

#if 0
    //get msg length
    ecies_ctx->get_ec_point_len(curve, cipher, cipher_len, &point_bytes);

   if(dec_ctx->enc_type_e == XOR_ENC)
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
        {}

        *msg_len = cipher_len - point_len - mac_ctx->out_len;
   }
#endif

    // set enc_t key and output buffer, this must be done before KDF action.
    dec_ctx->output = msg;
    if (XOR_ENC == dec_ctx->enc_type_e)
    {
        dec_ctx->key = dec_ctx->output;
    }
    else
    {
        dec_ctx->key = enc_key;
    }

    // set mac ctx
    mac_ctx->msg = dec_ctx->input;
    mac_ctx->msg_len = dec_ctx->input_bytes;
    mac_ctx->mac = mac_buf;

#ifdef DEBUG_ECIES
    printf("msg-len : %d\n\r", *msg_len);
    print_buf_u8(cipher, cipher_len, "cipher-input");
#endif

    // 1. get [pri_key]r.x
    rx[p_wlen - 1u] = 0u;
    ry[p_wlen - 1u] = 0u;
    ret = ecies_ctx->point_decompress(curve, cipher, rx, ry);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // check point r
    ret = eccp_pointverify(curve, rx, ry);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    k[n_wlen - 1u] = 0u;
    reverse_byte_array(receiver_pri_key, (unsigned char *)k, n_len);

    // make sure pri_key in [1, n-1]
    ret = uint32_integer_check(k, curve->eccp_n, n_wlen, ECIES_ZERO_ALL, ECIES_INTEGER_TOO_BIG, ECIES_SUCCESS);
    if (ECIES_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
    print_bn_buf_u32(rx, p_wlen, "rx");
    print_bn_buf_u32(ry, p_wlen, "ry");
#endif

    ret = eccp_pointmul(curve, k, rx, ry, k, NULL);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }
    reverse_byte_array((unsigned char *)k, (unsigned char *)k, p_len);

#ifdef DEBUG_ECIES
    print_buf_u8(s, p_len, "s-dec");
#endif

    // 2. get k_enc and k_mac from KDF.
    kdf_ctx->input = (unsigned char *)k;
    kdf_ctx->input_bytes = p_len;
    if (ENC_MAC_ORDER == ecies_ctx->enc_mac_key_order)
    {
        kdf_ctx->out1 = dec_ctx->key;
        kdf_ctx->out1_bytes = dec_ctx->key_bytes;
        kdf_ctx->out2 = mac_ctx->key;
        kdf_ctx->out2_bytes = mac_ctx->key_bytes;
    }
    else
    {
        kdf_ctx->out1 = mac_ctx->key;
        kdf_ctx->out1_bytes = mac_ctx->key_bytes;
        kdf_ctx->out2 = dec_ctx->key;
        kdf_ctx->out2_bytes = dec_ctx->key_bytes;
    }

    ret = kdf_ctx->kdf_fun_imp(kdf_ctx);
    if (ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
// printf("kdf-outlen: %d\n\r", kdf_ctx->out_bytes);
// print_buf_u8(kdf_ctx->out,kdf_ctx->out_bytes, "kdf-dec");
#endif

    // 3. d ?= mac(k_mac, c)
    ret = mac_ctx->mac_imp(mac_ctx);
    if (ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
// print_buf_u8(mac_ctx->out,mac_ctx->out_len, "mac-dec -value");
// print_buf_u8(cipher + cipher_len - mac_ctx->out_len,mac_ctx->out_len,
// "mac-dec-compare");
#endif

    if (memcmp_(&cipher[cipher_len - mac_ctx->mac_bytes], mac_ctx->mac, mac_ctx->mac_bytes))
    {
        return ECIES_ERROR;
    }
    else
    {
    }

// 4. msg = dec(k_enc,c)
#ifdef DEBUG_ECIES
// print_buf_u8(dec_ctx->key,dec_ctx->key_len, "dec-key");
// print_buf_u8(dec_ctx->msg,dec_ctx->msg_len, "dec-cip");
#endif

    ret = dec_ctx->dec_fun_imp(dec_ctx);
    if (ret != ECIES_SUCCESS)
    {
        return ret;
    }
    else
    {
    }

#ifdef DEBUG_ECIES
// print_buf_u8(dec_ctx->out,dec_ctx->msg_len, "dec-out");
#endif

    *msg_len = dec_ctx->output_bytes; // because may be padding

    return ECIES_SUCCESS;
}

/**
 * @brief           Elliptic Curve Integrated Encryption Scheme (ECIES) ANSI-X963-2001
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid
 * @param[in]       msg                  - original message, plaintext.
 * @param[in]       msg_len              - byte length of msg.
 * @param[in]       shared_info1         - optional, shared information, for KDF.
 * @param[in]       shared_info1_len     - byte length of shared_info1.
 * @param[in]       shared_info2         - optional, shared information, for MAC.
 * @param[in]       shared_info2_len     - byte length of shared_info2.
 * @param[in]       sender_tmp_pri_key   - sender's ephemeral private key, big-endian. if you do not have this, please set this parameter to be NULL, it
 *                                         will be generated inside.
 * @param[in]       receiver_pub_key     - receiver's public key, big-endian.
 * @param[in]       point_form           - curve point representation.
 * @param[in]       kdf_hash_alg         - specific hash algorithm for KDF.
 * @param[in]       mac_hash_alg         - specific hash algorithm for MAC.
 * @param[in]       mac_k_len            - key length of the MAC.
 * @param[out]      cipher               - encryption result, ciphertext.
 * @param[out]      cipher_len           - byte length of cipher.
 * @return          ECIES_SUCCESS(success)     other:error
 * @note
 *        1. if no shared_info1 needs to be provided, set the shared_info1 to
 *           NULL and the shared_info1_len to 0
 *        2. if no shared_info2 needs to be provided, set the shared_info2 to
 *           NULL and the shared_info2_len to 0
 *        3. if you do not have local_tmp_pri_key, please set the parameter to be
 *           NULL, it will be generated inside. this is recommended.
 *        4. the result ciphertext consists of three parts. the 1st part is a
 *           point, the 2nd part is internal ciphertext, the 3rd part is mac.
 */
unsigned int ansi_x963_2001_ecies_encrypt(const eccp_curve_t *curve, const unsigned char *msg, unsigned int msg_len, const unsigned char *shared_info1,
                                          unsigned int shared_info1_len, const unsigned char *shared_info2, unsigned int shared_info2_len, unsigned char *sender_tmp_pri_key,
                                          unsigned char *receiver_pub_key, ec_point_form_e point_form, hash_alg_e kdf_hash_alg, hash_alg_e mac_hash_alg, unsigned int mac_k_len,
                                          unsigned char *cipher, unsigned int *cipher_len)
{
    unsigned int ret;
    unsigned char hmac_key[ECIES_MAC_K_MAX_BYTE_LEN];
    kdf_ansi_x963_2001_ctx_t kdf_ctx;
    xor_enc_ctx_t xor_ctx;
    ecies_hmac_ctx_t hmac_ctx;
    ecies_std_t ecies_ctx;

    if ((NULL == curve) || (NULL == msg) || (NULL == receiver_pub_key) || (NULL == cipher) || (NULL == cipher_len))
    {
        ret = ECIES_POINTER_NULL;
    }
    else if ((0u == msg_len))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else if ((NULL == shared_info1) && (0u != shared_info1_len))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else if ((NULL == shared_info2) && (0u != shared_info2_len))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else if (HASH_SUCCESS != check_hash_alg(kdf_hash_alg))
    {
        ret = HASH_INPUT_INVALID;
    }
    else if (HASH_SUCCESS != check_hash_alg(mac_hash_alg))
    {
        ret = HASH_INPUT_INVALID;
    }
    else if ((mac_k_len < ECIES_MAC_K_ANSI_X963_MIN_BYTE_LEN) || (mac_k_len > ECIES_MAC_K_MAX_BYTE_LEN))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else
    {
        ecies_ansi_x963_ctx_init(&ecies_ctx, ENC_MAC_ORDER);

        ansi_x963_2001_kdf_init(&kdf_ctx, shared_info1, shared_info1_len, kdf_hash_alg);

        xor_enc_init(&xor_ctx, msg, msg_len);

        ecies_hmac_init(&hmac_ctx, hmac_key, mac_k_len, shared_info2, shared_info2_len, mac_hash_alg);

        ret = ecies_encrypt(&ecies_ctx, curve, msg, msg_len, sender_tmp_pri_key, receiver_pub_key, point_form, (kdf_base_t *)&kdf_ctx, (mac_base_t *)&hmac_ctx,
                            (enc_base_t *)&xor_ctx, cipher, cipher_len);
    }

    return ret;
}

/**
 * @brief           Elliptic Curve Integrated Encryption Scheme (ECIES) Decrypt ANSI-X963-2001
 * @param[in]       curve                - ecc curve struct pointer, please make sure it is valid
 * @param[in]       cipher               - original message, cipher.
 * @param[in]       cipher_len           - byte length of cipher.
 * @param[in]       receiver_pri_key     - optional, shared information, for KDF.
 * @param[in]       shared_info1         - byte length of shared_info1.
 * @param[in]       shared_info1_len     - optional, shared information, for MAC.
 * @param[in]       shared_info2         - byte length of shared_info2.
 * @param[in]       shared_info2_len     - byte length of shared_info2.
 * @param[in]       kdf_hash_alg         - specific hash algorithm for KDF.
 * @param[in]       mac_hash_alg         - specific hash algorithm for KDF.
 * @param[in]       mac_k_len            - specific key length for MAC.
 * @param[out]      msg                  - decryption result.
 * @param[out]      msg_len              - byte length of msg.
 * @return          ECIES_SUCCESS(success)     other:error
 * @note
 *        1. if no shared_info1 needs to be provided, set the shared_info1 to
 *           NULL and the shared_info1_len to 0
 *        2. if no shared_info2 needs to be provided, set the shared_info2 to
 *           NULL and the shared_info2_len to 0
 *        3. the input cipher consists of three parts. the 1st part is a point,
 *           the 2nd part is internal ciphertext, the 3rd part is mac.
 */
unsigned int ansi_x963_2001_ecies_decrypt(const eccp_curve_t *curve, unsigned char *cipher, unsigned int cipher_len, unsigned char *receiver_pri_key,
                                          const unsigned char *shared_info1, unsigned int shared_info1_len, const unsigned char *shared_info2, unsigned int shared_info2_len,
                                          hash_alg_e kdf_hash_alg, hash_alg_e mac_hash_alg, unsigned int mac_k_len, unsigned char *msg, unsigned int *msg_len)
{
    unsigned int ret;
    unsigned char hmac_key[ECIES_MAC_K_MAX_BYTE_LEN];
    unsigned int point_bytes;
    unsigned int mac_bytes;
    kdf_ansi_x963_2001_ctx_t kdf_ctx;
    xor_enc_ctx_t xor_ctx;
    ecies_hmac_ctx_t hmac_ctx;
    ecies_std_t ecies_ctx;

    if ((NULL == curve) || (NULL == cipher) || (NULL == receiver_pri_key) || (NULL == msg) || (NULL == msg_len))
    {
        ret = ECIES_POINTER_NULL;
    }
    else if ((NULL == shared_info1) && (0u != shared_info1_len))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else if ((NULL == shared_info2) && (0u != shared_info2_len))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else if (HASH_SUCCESS != check_hash_alg(kdf_hash_alg))
    {
        ret = HASH_INPUT_INVALID;
    }
    else if (HASH_SUCCESS != check_hash_alg(mac_hash_alg))
    {
        ret = HASH_INPUT_INVALID;
    }
    else if ((mac_k_len < ECIES_MAC_K_ANSI_X963_MIN_BYTE_LEN) || (mac_k_len > ECIES_MAC_K_MAX_BYTE_LEN))
    {
        ret = ECIES_INVALID_INPUT;
    }
    else
    {
        ecies_ansi_x963_ctx_init(&ecies_ctx, ENC_MAC_ORDER);

        ecies_hmac_init(&hmac_ctx, hmac_key, mac_k_len, shared_info2, shared_info2_len, mac_hash_alg);

        // Because XOR encryption requires the length of the message,
        // it is necessary to obtain the length of the mac
        mac_bytes = hmac_ctx.base.mac_bytes;

        ret = ecies_ctx.get_ec_point_len(curve, cipher, cipher_len, &point_bytes);
        if (PKE_SUCCESS == ret)
        {
            if (cipher_len <= (point_bytes + mac_bytes))
            {
                ret = ECIES_INVALID_INPUT;
            }
            else
            {
                *msg_len = cipher_len - point_bytes - mac_bytes;

                xor_enc_init(&xor_ctx, &cipher[point_bytes], *msg_len);

                ansi_x963_2001_kdf_init(&kdf_ctx, shared_info1, shared_info1_len, kdf_hash_alg);

                ret = ecies_decrypt(&ecies_ctx, curve, cipher, cipher_len, receiver_pri_key, (kdf_base_t *)&kdf_ctx, (mac_base_t *)&hmac_ctx, (enc_base_t *)&xor_ctx, msg, msg_len);
            }
        }
        else
        {
        }
    }

    return ret;
}

#endif
