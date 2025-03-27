/********************************************************************************************************
 * @file    rsassa_pss.c
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


#ifdef SUPPORT_RSASSA_PSS

#include "lib/include/pke/rsa.h"
#include "lib/include/hash/hash.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"





/**
 * @brief       RSA PKCS#1_v2.2 EMSA-PSS-ENCODE
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt                 - salt
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg_digest           - Hash(message), message is to be signed, here Hash is msg_hash_alg.
 * @param[in]   msg_digest_bytes     - byte length of msg_digest or Hash(message)
 * @param[out]  em                   - big integer to be signed, big-endian.
 * @param[in]   em_bits              - bit length of em, should be bit length of RSA modulus n minus 1.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.if no salt prepared to input, please set salt to NULL, it will be generated inside.
      -# 3.salt_bytes should be in [0, em_bytes-digest_bytes-2], it is recommended to use default value, digest
           length of hash algorithm msg_hash_alg or mgf_hash_alg.
  @endverbatim
 */
unsigned int emsa_pss_encode_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, unsigned char *salt, unsigned int salt_bytes,
        unsigned char *msg_digest, unsigned int msg_digest_bytes, unsigned char *em, unsigned int em_bits)
{
    unsigned char *digest_p = NULL;
    unsigned char *salt_p   = NULL;
    unsigned int em_bytes = GET_BYTE_LEN(em_bits);
    unsigned int ret;

    HASH_CTX hash_ctx[1];

    if(0 == msg_digest_bytes)
    {
        return RSA_INPUT_INVALID;
    }
    else if(em_bytes < msg_digest_bytes + salt_bytes + 2)   //this is equal to em_bits < (msg_digest_bytes + salt_bytes)*8 + 9
    {
        return RSA_INPUT_INVALID;
    }
    else
    {;}

    //set salt
    salt_p = em + em_bytes - 1 - msg_digest_bytes - salt_bytes;
    if(NULL == salt)
    {
        if(salt_bytes)
        {
            ret = get_rand(salt_p, salt_bytes);
            if(TRNG_SUCCESS != ret)
            {
                goto END;
            }
            else
            {;}
        }
        else
        {;}
    }
    else
    {
        memcpy_(salt_p, salt, salt_bytes);
    }

    //get DB = PS||0x01||salt, PS||0x01 is Padding2.
    memset_(em, 0, salt_p - 1 - em);
    *(salt_p - 1) = 0x01;

    //set H(M') = H(padding1||mhash||salt)
    ret = hash_init(hash_ctx, msg_hash_alg);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    digest_p = salt_p + salt_bytes;
    memset_(digest_p, 0, 8);
    ret = hash_update(hash_ctx, digest_p, 8);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    ret = hash_update(hash_ctx, msg_digest, msg_digest_bytes);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    if(salt_bytes)
    {
        ret = hash_update(hash_ctx, salt_p, salt_bytes);
        if(ret != HASH_SUCCESS)
        {
            goto END;
        }
        else
        {;}
    }
    else
    {;}

    ret = hash_final(hash_ctx, digest_p);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    //get maskDB
    ret = rsa_pkcs1_mgf1_with_xor_in(mgf_hash_alg, digest_p, msg_digest_bytes, em, em, em_bytes - 1 - msg_digest_bytes);
    if(ret != RSA_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    //clear MSB of maskDB
    em_bits &= 7;
    if(em_bits)
    {
        em[0] &= (1<<(em_bits))-1;
    }
    else
    {;}

    //set last byte
    em[em_bytes-1] = 0xBC;

    ret = RSA_SUCCESS;

END:

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 EMSA-PSS-VERIFY
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg_digest           - Hash(message), message is to be verified, here Hash is msg_hash_alg.
 * @param[in]   msg_digest_bytes     - byte length of msg_digest or Hash(message)
 * @param[out]  em                   - big integer generated by RSA verified, big-endian.
 * @param[in]   em_bits              - bit length of em, should be bit length of RSA modulus n minus 1.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.salt_bytes should be in [0, em_bytes-digest_bytes-2], it is recommended to use default value, digest
           length of hash algorithm msg_hash_alg or mgf_hash_alg. if salt_bytes is not known, please set it to -1.
  @endverbatim
 */
unsigned int emsa_pss_verify_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, int32_t salt_bytes,
        unsigned char *msg_digest, unsigned int msg_digest_bytes, unsigned char *em, unsigned int em_bits)
{
#if 0
    unsigned char buf[RSA_MAX_WORD_LEN<<2];
#else
    unsigned char *buf = em;
#endif
    unsigned char hash_msg[HASH_DIGEST_MAX_WORD_LEN<<2];
    unsigned char *salt_p = NULL;
    unsigned int observed_salt_bytes = 0;
    unsigned int em_bytes = GET_BYTE_LEN(em_bits);
    unsigned int i, tmp, ret;

    HASH_CTX hash_ctx[1];

    if(0 == msg_digest_bytes)
    {
        return RSA_INPUT_INVALID;
    }
    else
    {;}

    if(salt_bytes >= 0)
    {
        if(em_bytes < msg_digest_bytes + salt_bytes + 2)   //this is equal to em_bits < (msg_digest_bytes + salt_bytes)*8 + 9
        {
            return RSA_INPUT_INVALID;
        }
        else
        {;}

        observed_salt_bytes = (unsigned int)salt_bytes;
    }
    else
    {
        if(em_bytes < msg_digest_bytes + 2)   //this is equal to em_bits < (msg_digest_bytes + 0)*8 + 9
        {
            return RSA_INPUT_INVALID;
        }
        else
        {;}
    }

    //check LSB bytes
    if(em[em_bytes-1] != 0xBC)
    {
        return RSA_INPUT_INVALID;
    }
    else
    {;}

    //first (8*em_bytes - em_bits) bits should be all 0
    tmp = em_bits & 7;
    if(tmp)
    {
        if(em[0] & (0xFF<<tmp))
        {
            return RSA_INPUT_INVALID;
        }
        else
        {;}
    }
    else
    {;}

    //recover DB
    ret = rsa_pkcs1_mgf1_with_xor_in(mgf_hash_alg, em + em_bytes - 1 - msg_digest_bytes, msg_digest_bytes, em, buf,
            em_bytes - 1 - msg_digest_bytes);
    if(ret != RSA_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    //clear first (8*em_bytes - em_bits) bits
    if(tmp)
    {
        buf[0] &= (1<<tmp)-1;
    }
    else
    {;}

    //check padding2
    if(salt_bytes >= 0)
    {
        tmp = em_bytes-1-msg_digest_bytes-1-salt_bytes;
        for(i=0; i<tmp; i++)
        {
            if(buf[i])
            {
                ret = RSA_INPUT_INVALID;
                goto END;
            }
            else
            {;}
        }

        if(buf[i] != 0x01)
        {
            ret = RSA_INPUT_INVALID;
            goto END;
        }
        else
        {;}

        salt_p = buf + tmp + 1;
    }
    else
    {
        tmp = em_bytes-1-msg_digest_bytes-1;
        for(i=0; i<tmp; i++)
        {
            if(buf[i])
            {
                break;
            }
            else
            {;}
        }

        if(0x01 == buf[i])
        {
            salt_p = buf + i + 1;
            observed_salt_bytes = tmp - i;  //(tmp+1)-(i+1)
        }
        else
        {
            ret = RSA_INPUT_INVALID;
            goto END;
        }
    }

    //get H(M')
    ret = hash_init(hash_ctx, msg_hash_alg);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    memset_(hash_msg, 0, 8);
    ret = hash_update(hash_ctx, hash_msg, 8);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    ret = hash_update(hash_ctx, msg_digest, msg_digest_bytes);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    if(observed_salt_bytes)
    {
        ret = hash_update(hash_ctx, salt_p, observed_salt_bytes);
        if(ret != HASH_SUCCESS)
        {
            goto END;
        }
        else
        {;}
    }
    else
    {;}

    ret = hash_final(hash_ctx, hash_msg);
    if(ret != HASH_SUCCESS)
    {
        goto END;
    }
    else
    {;}

    if(memcmp_(hash_msg, em + em_bytes - 1 - msg_digest_bytes, msg_digest_bytes))
    {
        ret = RSA_INPUT_INVALID;
        goto END;
    }
    else
    {;}

    ret = RSA_SUCCESS;

END:

#if 0
    memset_(buf, 0, sizeof(buf));
#endif
    memset_(hash_msg, 0, sizeof(hash_msg));

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 RSASSA-PSS-SIGN with message digest
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt                 - salt
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg_digest           - Hash(message), message is to be signed, here Hash is msg_hash_alg.
 * @param[in]   d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]   n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]   n_bits               - bit length of n
 * @param[out]  signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.if no salt prepared to input, please set salt to NULL, it will be generated inside.
      -# 3.salt_bytes should be in [0, em_bytes-digest_bytes-2], em_bytes is (em_bits+7)/8, em_bits is (n_bits-1).
           it is recommended to use default value, digest length of hash algorithm msg_hash_alg or mgf_hash_alg.
  @endverbatim
 */
unsigned int rsa_ssa_pss_sign_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, unsigned char *salt, unsigned int salt_bytes,
        unsigned char *msg_digest, unsigned char *d, unsigned char *n, unsigned int n_bits, unsigned char *signature)
{
    unsigned char em[RSA_MAX_WORD_LEN<<2];
    unsigned int msg_digest_bytes;
    unsigned int tmp, ret;

    //n can not be even
    tmp = GET_BYTE_LEN(n_bits);
    if(!(n[tmp-1] & 1))
    {
        return RSA_INPUT_INVALID;
    }
    else
    {;}

    msg_digest_bytes = hash_get_digest_word_len(msg_hash_alg)<<2;

#if 1  //to support such as RSA1025
    em[0] = 0;
    ret = emsa_pss_encode_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt, salt_bytes, msg_digest, msg_digest_bytes,
            (1==(n_bits&7))?em+1:em, n_bits-1);
#else
    ret = emsa_pss_encode_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt, salt_bytes, msg_digest, msg_digest_bytes,
            em, n_bits-1);
#endif
    if(RSA_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    //signature = em^d mod n
    ret = pke_modexp_U8(( unsigned char *)n, ( unsigned char *)d, ( unsigned char *)em, signature, n_bits, n_bits, 1);//print_buf_U8((U8 *)a, n_bytes, "a");
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    ret = RSA_SUCCESS;

END:

    memset_(em, 0, sizeof(em));

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 RSASSA-PSS-SIGN with message digest
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt                 - salt
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg                  - message to be signed
 * @param[in]   msg_digest           - byte length of message
 * @param[in]   d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]   n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]   n_bits               - bit length of n
 * @param[out]  signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.if no salt prepared to input, please set salt to NULL, it will be generated inside.
      -# 3.salt_bytes should be in [0, em_bytes-digest_bytes-2], em_bytes is (em_bits+7)/8, em_bits is (n_bits-1).
           it is recommended to use default value, digest length of hash algorithm msg_hash_alg or mgf_hash_alg.
  @endverbatim
 */
unsigned int rsa_ssa_pss_sign(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, unsigned char *salt, unsigned int salt_bytes, unsigned char *msg,
        unsigned int msg_bytes, unsigned char *d, unsigned char *n, unsigned int n_bits, unsigned char *signature)
{
    unsigned char msg_digest[HASH_DIGEST_MAX_WORD_LEN<<2];
    unsigned int ret;

    ret = hash(msg_hash_alg, msg, msg_bytes, msg_digest);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = rsa_ssa_pss_sign_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt, salt_bytes, msg_digest, d, n, n_bits, signature);

    memset_(msg_digest, 0, sizeof(msg_digest));

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 RSASSA-PSS-SIGN with message digest(private key is CRT style)
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt                 - salt
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg_digest           - Hash(message), message is to be signed, here Hash is msg_hash_alg.
 * @param[in]   d                    - RSA-CRT private key (p,q,dp,dq,u), every field is (n_bits/2+7)/8 bytes, big-endian.
 * @param[out]  signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.if no salt prepared to input, please set salt to NULL, it will be generated inside.
      -# 3.salt_bytes should be in [0, em_bytes-digest_bytes-2], em_bytes is (em_bits+7)/8, em_bits is (n_bits-1).
           it is recommended to use default value, digest length of hash algorithm msg_hash_alg or mgf_hash_alg.
  @endverbatim
 */
unsigned int rsa_ssa_pss_crt_sign_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, unsigned char *salt, unsigned int salt_bytes,
        unsigned char *msg_digest, RSA_CRT_PRIVATE_KEY *d, unsigned int n_bits, unsigned char *signature)
{
    unsigned int em_buf[RSA_MAX_WORD_LEN<<2];
    unsigned char *em = (unsigned char *)em_buf;
    unsigned int p[RSA_MAX_WORD_LEN>>1];
    unsigned int q[RSA_MAX_WORD_LEN>>1];
    unsigned int dp[RSA_MAX_WORD_LEN>>1];
    unsigned int dq[RSA_MAX_WORD_LEN>>1];
    unsigned int u[RSA_MAX_WORD_LEN>>1];
    unsigned int msg_digest_bytes;
    unsigned int tmp, ret;

    msg_digest_bytes = hash_get_digest_word_len(msg_hash_alg)<<2;

    //clear last word
    if(n_bits & 0x1F)
    {
        tmp = GET_WORD_LEN(n_bits) - 1;
        em_buf[tmp] = 0;
    }
    else
    {;}

#if 0  //to support such as RSA1025
    em[0] = 0;
    ret = emsa_pss_encode_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt, salt_bytes, msg_digest, msg_digest_bytes,
            (1==(n_bits&7))?em+1:em, n_bits-1);
#else
    ret = emsa_pss_encode_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt, salt_bytes, msg_digest, msg_digest_bytes,
            em, n_bits-1);
#endif
    if(RSA_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    reverse_byte_array(em, (unsigned char *)em, GET_BYTE_LEN(n_bits));

    //clear last word
    tmp = (n_bits>>1);
    if(tmp & 0x1F)
    {
        tmp = GET_WORD_LEN(tmp) - 1;
        p[tmp]  = 0;
        q[tmp]  = 0;
        dp[tmp] = 0;
        dq[tmp] = 0;
        u[tmp]  = 0;
    }
    else
    {;}

    tmp = GET_BYTE_LEN(tmp);
    reverse_byte_array(d->p,  (unsigned char *)p,  tmp);
    reverse_byte_array(d->q,  (unsigned char *)q,  tmp);
    reverse_byte_array(d->dp, (unsigned char *)dp, tmp);
    reverse_byte_array(d->dq, (unsigned char *)dq, tmp);
    reverse_byte_array(d->u,  (unsigned char *)u,  tmp);

    //signature = em^d mod n
    ret = RSA_CRTModExp((unsigned int *)em, p, q, dp, dq, u, (unsigned int *)em, n_bits);//print_buf_U8((U8 *)a, n_bytes, "a");
    if(PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {;}

    reverse_byte_array(em, (unsigned char *)signature, GET_BYTE_LEN(n_bits));

    ret = RSA_SUCCESS;

END:

    memset_(em, 0, sizeof(em));

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 RSASSA-PSS-SIGN with message(private key is CRT style)
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt                 - salt
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg                  - message to be signed
 * @param[in]   msg_digest           - byte length of message
 * @param[in]   d                    - RSA-CRT private key (p,q,dp,dq,u), every field is (n_bits/2+7)/8 bytes, big-endian.
 * @param[in]   n_bits               - bit length of n
 * @param[out]  signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.if no salt prepared to input, please set salt to NULL, it will be generated inside.
      -# 3.salt_bytes should be in [0, em_bytes-digest_bytes-2], em_bytes is (em_bits+7)/8, em_bits is (n_bits-1).
           it is recommended to use default value, digest length of hash algorithm msg_hash_alg or mgf_hash_alg.
  @endverbatim
 */
unsigned int rsa_ssa_pss_crt_sign(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, unsigned char *salt, unsigned int salt_bytes, unsigned char *msg,
        unsigned int msg_bytes, RSA_CRT_PRIVATE_KEY *d, unsigned int n_bits, unsigned char *signature)
{
    unsigned char msg_digest[HASH_DIGEST_MAX_WORD_LEN<<2];
    unsigned int ret;

    ret = hash(msg_hash_alg, msg, msg_bytes, msg_digest);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = rsa_ssa_pss_crt_sign_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt, salt_bytes, msg_digest, d, n_bits, signature);

    memset_(msg_digest, 0, sizeof(msg_digest));

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 RSASSA-PSS-VERIFY with message digest
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg_digest           - Hash(message), message is to be verified, here Hash is msg_hash_alg.
 * @param[in]   e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]   e_bits               - bit length of e
 * @param[in]   n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]   n_bits               - bit length of n
 * @param[out]  signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.salt_bytes should be in [0, em_bytes-digest_bytes-2], em_bytes is (em_bits+7)/8, em_bits is (n_bits-1).
           it is recommended to use default value, digest length of hash algorithm msg_hash_alg or mgf_hash_alg.
           if salt_bytes is not known, please set it to -1
  @endverbatim
 */
unsigned int rsa_ssa_pss_verify_by_msg_digest(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, int32_t salt_bytes,
        unsigned char *msg_digest, unsigned char *e, unsigned int e_bits, unsigned char *n, unsigned int n_bits, unsigned char *signature)
{
    unsigned char em[RSA_MAX_WORD_LEN<<2];
    unsigned int msg_digest_bytes;
    unsigned int tmp, ret;

    if((NULL == msg_digest) || (NULL == e) || (NULL == n) || (NULL == signature))
    {
        return RSA_BUFFER_NULL;
    }
    else if((e_bits < 2) || (e_bits > n_bits) || (n_bits > RSA_MAX_BIT_LEN) || (0 == n_bits))
    {
        return RSA_INPUT_INVALID;
    }
    else
    {;}

    //n can not be even
    tmp = GET_BYTE_LEN(n_bits);
    if(!(n[tmp-1] & 1))
    {
        return RSA_INPUT_INVALID;
    }
    else
    {;}

    ret = pke_modexp_U8(( unsigned char *)n, ( unsigned char *)e, ( unsigned char *)signature, em, n_bits, e_bits, 1);
    if(PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

#if 1  //to support such as RSA1025
    tmp = (n_bits&7);
    if(1 == tmp)
    {
        if(em[0])
        {
            return RSA_INPUT_INVALID;
        }
        else
        {;}
    }
    else
    {;}

    msg_digest_bytes = hash_get_digest_word_len(msg_hash_alg)<<2;
    ret = emsa_pss_verify_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt_bytes, msg_digest, msg_digest_bytes,
            (1==tmp)?em+1:em, n_bits-1);
#else
    ret = emsa_pss_verify_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt_bytes, msg_digest, msg_digest_bytes,
                em, n_bits-1);
#endif

    memset_(em, 0, sizeof(em));

    return ret;
}


/**
 * @brief       RSA PKCS#1_v2.2 RSASSA-PSS-VERIFY with message
 * @param[in]   msg_hash_alg         - specific hash algorithm for message or Hash(message)
 * @param[in]   mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]   salt_bytes           - byte length of salt
 * @param[in]   msg                  - message to be verified
 * @param[in]   msg_digest           - byte length of message
 * @param[in]   e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]   e_bits               - bit length of e
 * @param[in]   n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]   n_bits               - bit length of n
 * @param[in]   signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.it is recommended that msg_hash_alg and mgf_hash_alg are the same.
      -# 2.salt_bytes should be in [0, em_bytes-digest_bytes-2], em_bytes is (em_bits+7)/8, em_bits is (n_bits-1).
           it is recommended to use default value, digest length of hash algorithm msg_hash_alg or mgf_hash_alg.
           if salt_bytes is not known, please set it to -1
  @endverbatim
 */
unsigned int rsa_ssa_pss_verify(HASH_ALG msg_hash_alg, HASH_ALG mgf_hash_alg, int32_t salt_bytes, unsigned char *msg,
        unsigned int msg_bytes, unsigned char *e, unsigned int e_bits, unsigned char *n, unsigned int n_bits, unsigned char *signature)
{
    unsigned char msg_digest[HASH_DIGEST_MAX_WORD_LEN<<2];
    unsigned int ret;

    if(NULL == msg)
    {
        return RSA_BUFFER_NULL;
    }
    else
    {;}

    ret = hash(msg_hash_alg, msg, msg_bytes, msg_digest);
    if(HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {;}

    ret = rsa_ssa_pss_verify_by_msg_digest(msg_hash_alg, mgf_hash_alg, salt_bytes, msg_digest,
            e, e_bits, n, n_bits, signature);

    memset_(msg_digest, 0, sizeof(msg_digest));

    return ret;
}

#endif

