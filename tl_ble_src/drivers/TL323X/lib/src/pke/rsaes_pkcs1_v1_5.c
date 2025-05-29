/*! @file rsaes_pkcs1_v1_5.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_RSAES_PKCS1_V1_5

#include "../../crypto_include/crypto_common/utility.h"
#include "../../crypto_include/pke/rsa.h"
#include "../../crypto_include/pke/rsa_u8.h"
#include "../../crypto_include/trng/trng.h"

/**
 * @brief           check RSAES PKCS#1-v1.5 parameter
 * @param[in]       modulus              - modulus
 * @param[in]       exponent             - exponent
 * @param[in]       n_bits               - bit length of n
 * @param[in]       exp_bits             - bit length of exponent
 * @param[in]       msg                  - message to be encode or decode
 * @param[in]       cipher               - cipher to encrypt or decrypt
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. modulus must be odd
 *        2. please make sure exp_bits <= n_bits <= OPERAND_MAX_BIT_LEN
 */
FLAG_STATIC unsigned int rsa_es_pkcs1_v1_5_param_check(const unsigned char *modulus, unsigned int n_bits, const void *exponent, unsigned int exp_bits, const unsigned char *msg,
                                                       const unsigned char *cipher)
{
    unsigned int tmp, ret = RSA_SUCCESS;

    tmp = get_byte_len(n_bits);

    if ((NULL == exponent) || (NULL == modulus) || (NULL == msg) || (NULL == cipher))
    {
        ret = RSA_INPUT_INVALID;
    }
    else if ((n_bits > RSA_MAX_BIT_LEN) || (exp_bits > n_bits) || (n_bits < RSA_MIN_BIT_LEN))
    {
        ret = RSA_INPUT_INVALID;
    }
    else if (((unsigned char)0) == (modulus[tmp - 1U] & 1U)) // n can not be even
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    return ret;
}

/**
 * @brief           get nonzero octets
 * @param[out]      ps                   - padding octets
 * @param[in]       ps_bytes             - byte length of ps
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. (No specific caution provided)
 */
FLAG_STATIC unsigned int eme_pkcs1_v1_5_encode_gen_ps(unsigned char *ps, unsigned int ps_bytes)
{
    unsigned int i, ret;

    ret = get_rand(ps, ps_bytes);
    if (TRNG_SUCCESS == ret)
    {
        ret = RSA_SUCCESS;

        // make sure PS is nonzero octets
        for (i = 0U; i < ps_bytes; i++)
        {
            while (0U == ps[i])
            {
                ret = get_rand(&ps[i], 1U);
                if (TRNG_SUCCESS != ret)
                {
                    ret = RSA_ERROR;
                    break;
                }
                else
                {
                }
            }

            if (RSA_ERROR == ret)
            {
                break;
            }
            else
            {
            }
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           EME-PKCS1-v1_5 encoding
 * @param[in]       msg                  - message to be encoded
 * @param[in]       msg_len              - byte length of message
 * @param[in]       ps                   - padding octets (set to NULL if not provided)
 * @param[out]      em                   - big integer to be encrypted, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. if you do not have ps, please set the parameter to be NULL; it will be
 *           generated inside. otherwise, please make sure ps is nonzero octets and it
 *           occupies em_bytes-3-msg_len.
 *        2. em_bytes is (em_bits+7)/8
 */
unsigned int eme_pkcs1_v1_5_encode(const unsigned char *msg, unsigned int msg_len, const unsigned char *ps, unsigned char *em, unsigned int em_bytes)
{
    unsigned int ret = RSA_SUCCESS;
    unsigned int i, ps_bytes;

    if (((NULL == msg) && (0U != msg_len)) || (NULL == em))
    {
        ret = RSA_INPUT_INVALID;
    }
    else if ((msg_len + 0x0BU) > em_bytes) // ps occupy at lease 8 bytes
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    // 0x00 || 0x02 || PS
    if (RSA_SUCCESS == ret)
    {
        ps_bytes = em_bytes - 3U - msg_len;

        em[0] = 0x00U;
        em[1] = 0x02;

        if (NULL != ps)
        {
            // ps from outside
            for (i = 0; i < ps_bytes; i++)
            {
                if (ps[i] == (unsigned char)0x00) // PS is nonzero octets
                {
                    ret = RSA_INPUT_INVALID;
                    break;
                }
                else
                {
                }
            }

            if (RSA_SUCCESS == ret)
            {
                memcpy_(&em[2], ps, ps_bytes);
            }
            else
            {
            }
        }
        else
        {
            // ps generate inside
            ret = eme_pkcs1_v1_5_encode_gen_ps(&em[2], ps_bytes);
            if (TRNG_SUCCESS == ret)
            {
                ret = RSA_SUCCESS;
            }
            else
            {
            }
        }
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        // em = 0x00 || 0x02 || PS || 0x00 || msg
        em[2U + ps_bytes] = 0x00U;
        memcpy_(&em[3U + ps_bytes], msg, msg_len);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           EME-PKCS1-v1_5 decoding
 * @param[out]      msg                  - message to be decoded
 * @param[out]      msg_len              - byte length of message
 * @param[in]       em                   - big integer to be parsed, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. em_bytes is (em_bits+7)/8
 */
unsigned int eme_pkcs1_v1_5_decode(unsigned char *msg, unsigned int *msg_len, const unsigned char *em, unsigned int em_bytes)
{
    unsigned int ret = RSA_SUCCESS, i;

    if ((NULL == msg) || (NULL == msg_len) || (NULL == em))
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        // check eme first and second byte
        if ((0x00U != em[0]) || (0x02U != em[1]))
        {
            ret = RSA_INPUT_INVALID;
        }
        else
        {
        }
    }

    if (RSA_SUCCESS == ret)
    {
        for (i = 2; i < em_bytes; i++)
        {
            if (em[i] == 0x00U)
            {
                break;
            }
            else
            {
            }
        }
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        // check PS bytes >= 8
        if ((em_bytes == i) || (i < 10U))
        {
            ret = RSA_INPUT_INVALID;
        }
        else
        {
            *msg_len = em_bytes - (i + 1U);
            memcpy_(msg, &em[i + 1U], *msg_len);
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES_PKCS1_V1_5 ENCRYPT with message
 * @param[in]       msg                  - message to be encrypted
 * @param[in]       msg_len              - byte length of message
 * @param[in]       ps                   - padding octets (set to NULL if not provided)
 * @param[in]       e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]       e_bits               - bit length of e
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. if you do not have ps, please set the parameter to be NULL; it will be
 *           generated inside. otherwise, please make sure ps is nonzero octets and it
 *           occupies em_bytes-3-msg_len.
 *        2. em_bytes is (em_bits+7)/8
 */
unsigned int rsa_es_pkcs1_v1_5_enc(const unsigned char *msg, unsigned int msg_len, const unsigned char *ps, const unsigned char *e, unsigned int e_bits, const unsigned char *n,
                                   unsigned int n_bits, unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_pkcs1_v1_5_param_check(n, n_bits, e, e_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = eme_pkcs1_v1_5_encode(msg, msg_len, ps, em, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, e, em, cipher, n_bits, e_bits, 1);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES_PKCS1_V1_5 DECRYPT with message
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of message
 * @param[in]       d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 */
unsigned int rsa_es_pkcs1_v1_5_dec(unsigned char *msg, unsigned int *msg_len, const unsigned char *d, const unsigned char *n, unsigned int n_bits, const unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_pkcs1_v1_5_param_check(n, n_bits, d, n_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, d, cipher, em, n_bits, n_bits, 1U);
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_pkcs1_v1_5_decode(msg, msg_len, em, get_byte_len(n_bits));
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSAES_PKCS1_V1_5 DECRYPT with message (private key is CRT style)
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of message
 * @param[in]       d                    - RSA-CRT private key (p,q,dp,dq,u), every field is (n_bits/2+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 */
unsigned int rsa_es_pkcs1_v1_5_crt_dec(unsigned char *msg, unsigned int *msg_len, const rsa_crt_private_key_t *d, unsigned int n_bits, const unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int tmp, ret = RSA_SUCCESS;

    // clear last word
    if (0U != (n_bits & 0x1FU))
    {
        tmp = get_word_len(n_bits) - 1U;
        em[tmp] = 0U;
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = RSA_CRTModExp_U8(cipher, d->p, d->q, d->dp, d->dq, d->u, em, n_bits);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        ret = eme_pkcs1_v1_5_decode(msg, msg_len, (unsigned char *)em, get_byte_len(n_bits));
    }
    else
    {
    }

    memset_(em, 0, sizeof(em));

    return ret;
}

#endif
