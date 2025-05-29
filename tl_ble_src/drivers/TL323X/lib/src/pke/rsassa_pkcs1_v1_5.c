/*! @file rsassa_pkcs1_v1_5.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_RSASSA_PKCS1_V1_5

#include "../../crypto_include/crypto_common/utility.h"
#include "../../crypto_include/hash_hmac/hash.h"
#include "../../crypto_include/pke/rsa.h"
#include "../../crypto_include/pke/rsa_u8.h"
#include "../../crypto_include/trng/trng.h"

#if 0
const unsigned char MD2_prefix[]       = {0x30,0x20,0x30,0x0C,0x06,0x08,0x2A,0x86,0x48,0x86,0xF7,0x0D,0x02,0x02,0x05,0x00,0x04,0x10,};
#endif

#ifdef SUPPORT_HASH_MD5
extern const unsigned char MD5_prefix[18];
const unsigned char MD5_prefix[18] = {0x30, 0x20, 0x30, 0x0C, 0x06, 0x08, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05, 0x05, 0x00, 0x04, 0x10};
#endif

#ifdef SUPPORT_HASH_SHA1
extern const unsigned char SHA_1_prefix[15];
const unsigned char SHA_1_prefix[15] = {0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2B, 0x0E, 0x03, 0x02, 0x1A, 0x05, 0x00, 0x04, 0x14};
#endif

#ifdef SUPPORT_HASH_SHA224
extern const unsigned char SHA_224_prefix[19];
const unsigned char SHA_224_prefix[19] = {0x30, 0x2D, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04, 0x05, 0x00, 0x04, 0x1C};
#endif

#ifdef SUPPORT_HASH_SHA256
extern const unsigned char SHA_256_prefix[19];
const unsigned char SHA_256_prefix[19] = {0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05, 0x00, 0x04, 0x20};
#endif

#ifdef SUPPORT_HASH_SHA384
extern const unsigned char SHA_384_prefix[19];
const unsigned char SHA_384_prefix[19] = {0x30, 0x41, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02, 0x05, 0x00, 0x04, 0x30};
#endif

#ifdef SUPPORT_HASH_SHA512
extern const unsigned char SHA_512_prefix[19];
const unsigned char SHA_512_prefix[19] = {0x30, 0x51, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03, 0x05, 0x00, 0x04, 0x40};
#endif

#ifdef SUPPORT_HASH_SHA512_224
extern const unsigned char SHA_512_224_prefix[19];
const unsigned char SHA_512_224_prefix[19] = {0x30, 0x2D, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x05, 0x05, 0x00, 0x04, 0x1C};
#endif

#ifdef SUPPORT_HASH_SHA512_256
extern const unsigned char SHA_512_256_prefix[19];
const unsigned char SHA_512_256_prefix[19] = {0x30, 0x31, 0x30, 0x0D, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x06, 0x05, 0x00, 0x04, 0x20};
#endif

/**
 * @brief           check RSASSA-PKCS1-v1.5 parameter
 * @param[in]       modulus              - modulus
 * @param[in]       exponent             - exponent
 * @param[in]       n_bits               - bit length of n
 * @param[in]       exp_bits             - bit length of exponent
 * @param[in]       msg                  - message or digest of message to be encoded or decoded
 * @param[in]       cipher               - cipher to enc or dec
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. modulus must be odd
 *        2. please make sure exp_bits <= n_bits <= OPERAND_MAX_BIT_LEN
 */
FLAG_STATIC unsigned int rsa_ssa_pkcs1_v1_5_param_check(const unsigned char *modulus, unsigned int n_bits, const void *exponent, unsigned int exp_bits, const unsigned char *msg,
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
 * @brief           get EMSA-PKCS1-v1_5 the DER encoding T of the digest info value
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[out]      prefix_bytes         - byte length of digestInfo value
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. please make sure hash alg is valid
 */
FLAG_STATIC const unsigned char *get_pkcs1_v1_5_t_prefix(hash_alg_e alg, unsigned int *prefix_bytes)
{
    const unsigned char *prefix;

    switch (alg)
    {
#ifdef SUPPORT_HASH_MD5
    case HASH_MD5:
        prefix = MD5_prefix;
        *prefix_bytes = sizeof(MD5_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA1
    case HASH_SHA1:
        prefix = SHA_1_prefix;
        *prefix_bytes = sizeof(SHA_1_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA224
    case HASH_SHA224:
        prefix = SHA_224_prefix;
        *prefix_bytes = sizeof(SHA_224_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA256
    case HASH_SHA256:
        prefix = SHA_256_prefix;
        *prefix_bytes = sizeof(SHA_256_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA384
    case HASH_SHA384:
        prefix = SHA_384_prefix;
        *prefix_bytes = sizeof(SHA_384_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA512
    case HASH_SHA512:
        prefix = SHA_512_prefix;
        *prefix_bytes = sizeof(SHA_512_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA512_224
    case HASH_SHA512_224:
        prefix = SHA_512_224_prefix;
        *prefix_bytes = sizeof(SHA_512_224_prefix);
        break;
#endif

#ifdef SUPPORT_HASH_SHA512_256
    case HASH_SHA512_256:
        prefix = SHA_512_256_prefix;
        *prefix_bytes = sizeof(SHA_512_256_prefix);
        break;
#endif

    default:
        prefix = NULL;
        *prefix_bytes = 0;
        break;
    }

    return prefix;
}

/**
 * @brief           RSA PKCS#1_v2.2 EMSA-PKCS1-v1_5 encoding with message digest
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg_digest           - Hash(message), message is to be signed, here Hash is msg_hash_alg.
 * @param[out]      em                   - big integer to be signed, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n minus 1.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. please make sure hash alg is valid
 */
unsigned int emsa_pkcs_v1_5_encode_by_msg_digest(hash_alg_e alg, const unsigned char *msg_digest, unsigned char *em, unsigned int em_bytes)
{
    const unsigned char *prefix;
    unsigned int prefix_bytes;
    unsigned int ps_bytes;
    unsigned int ret = RSA_SUCCESS;
    unsigned int digest_bytes = (unsigned int)hash_get_digest_word_len(alg) << 2;

    if ((0U == digest_bytes) || (NULL == msg_digest) || (NULL == em))
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        prefix = get_pkcs1_v1_5_t_prefix(alg, &prefix_bytes);
        if (em_bytes < ((prefix_bytes + digest_bytes) + 11U))
        {
            ret = RSA_INPUT_INVALID;
        }
        else
        {
        }
    }

    if (RSA_SUCCESS == ret)
    {
        ps_bytes = em_bytes - 3U - prefix_bytes - digest_bytes;

        // EM = 0x00 || 0x01 || PS || 0x00 || hashAlgID || hash(M)
        em[0] = 0x00;
        em[1] = 0x01;
        memset_(&em[2U], 0xFF, ps_bytes);
        em[2U + ps_bytes] = 0x00;
        memcpy_(&em[3U + ps_bytes], prefix, prefix_bytes);
        memcpy_(&em[em_bytes - digest_bytes], msg_digest, digest_bytes);
        if (HASH_SUCCESS == ret)
        {
            ret = RSA_SUCCESS;
        }
        else
        {
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 EMSA-PKCS1-v1_5 encoding with message
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg                  - message to be encoded
 * @param[in]       msg_len              - byte length of message
 * @param[out]      em                   - big integer to be signed, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n minus 1.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. please make sure hash alg is valid
 */
unsigned int emsa_pkcs_v1_5_encode(hash_alg_e alg, const unsigned char *msg, unsigned int msg_len, unsigned char *em, unsigned int em_bytes)
{
    const unsigned char *prefix;
    unsigned int prefix_bytes;
    unsigned int ps_bytes;
    unsigned int ret = RSA_SUCCESS;
    unsigned int digest_bytes = (unsigned int)hash_get_digest_word_len(alg) << 2;

    if (0U == digest_bytes)
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        prefix = get_pkcs1_v1_5_t_prefix(alg, &prefix_bytes);
        if (em_bytes < ((prefix_bytes + digest_bytes) + 11U))
        {
            ret = RSA_INPUT_INVALID;
        }
        else
        {
        }
    }

    if (RSA_SUCCESS == ret)
    {
        ps_bytes = em_bytes - 3U - prefix_bytes - digest_bytes;

        em[0] = 0x00;
        em[1] = 0x01;
        memset_(&em[2U], 0xFF, ps_bytes);
        em[2U + ps_bytes] = 0x00;
        memcpy_(&em[3U + ps_bytes], prefix, prefix_bytes);
        ret = hash(alg, msg, msg_len, &em[em_bytes - digest_bytes]);
        if (HASH_SUCCESS == ret)
        {
            ret = RSA_SUCCESS;
        }
        else
        {
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSASSA-PKCS1-v1_5 with message digest
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg_digest           - Hash(message), message is to be verified, here Hash is msg_hash_alg.
 * @param[in]       d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 */
unsigned int rsa_ssa_pkcs1_v1_5_sign_by_msg_digest(hash_alg_e alg, const unsigned char *msg_digest, const unsigned char *d, const unsigned char *n, unsigned int n_bits,
                                                   unsigned char *signature)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_ssa_pkcs1_v1_5_param_check(n, n_bits, d, n_bits, msg_digest, signature);
    if (RSA_SUCCESS == ret)
    {
        ret = emsa_pkcs_v1_5_encode_by_msg_digest(alg, msg_digest, (unsigned char *)em, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, d, (unsigned char *)em, signature, n_bits, n_bits, 1);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSASSA-PKCS1-v1_5 with message digest
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg_digest           - Hash(message), message is to be verified, here Hash is msg_hash_alg.
 * @param[in]       d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 */
unsigned int rsa_ssa_pkcs1_v1_5_crt_sign_by_msg_digest(hash_alg_e alg, const unsigned char *msg_digest, const rsa_crt_private_key_t *d, const unsigned char *n, unsigned int n_bits,
                                                       unsigned char *signature)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_ssa_pkcs1_v1_5_param_check(n, n_bits, d, n_bits, msg_digest, signature);
    if (RSA_SUCCESS == ret)
    {
        ret = emsa_pkcs_v1_5_encode_by_msg_digest(alg, msg_digest, (unsigned char *)em, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = RSA_CRTModExp_U8(em, d->p, d->q, d->dp, d->dq, d->u, signature, n_bits);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSASSA-PKCS1-v1_5 with message
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg                  - message to be signed
 * @param[in]       msg_len              - byte length of message
 * @param[in]       d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *           - msg_len should in [0, em_bytes-t_bytes-11], em_bytes is
 *           (em_bits+7)/8, t_bytes=hash_bytes+hashAlgID_bytes, hashAlgID_bytes refer to
 *           get_pkcs1_v1_5_t_prefix().
 */
unsigned int rsa_ssa_pkcs1_v1_5_sign(hash_alg_e alg, const unsigned char *msg, unsigned int msg_len, const unsigned char *d, const unsigned char *n, unsigned int n_bits,
                                     unsigned char *signature)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_ssa_pkcs1_v1_5_param_check(n, n_bits, d, n_bits, msg, signature);
    if (RSA_SUCCESS == ret)
    {
        ret = emsa_pkcs_v1_5_encode(alg, msg, msg_len, em, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, d, em, signature, n_bits, n_bits, 1);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSASSA-PKCS1-v1_5 with message (private key is CRT style)
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg                  - message to be signed
 * @param[in]       msg_len              - byte length of message
 * @param[in]       d                    - RSA-CRT private key (p,q,dp,dq,u), every field is (n_bits/2+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. msg_len should in [0, em_bytes-t_bytes-11], em_bytes is
 *           (em_bits+7)/8, t_bytes=hash_bytes+hashAlgID_bytes, hashAlgID_bytes refer to
 *           get_pkcs1_v1_5_t_prefix().
 */
unsigned int rsa_ssa_pkcs1_v1_5_crt_sign(hash_alg_e alg, const unsigned char *msg, unsigned int msg_len, const rsa_crt_private_key_t *d, const unsigned char *n,
                                         unsigned int n_bits, unsigned char *signature)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_ssa_pkcs1_v1_5_param_check(n, n_bits, d, n_bits, msg, signature);
    if (RSA_SUCCESS == ret)
    {
        ret = emsa_pkcs_v1_5_encode(alg, msg, msg_len, (unsigned char *)em, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = RSA_CRTModExp_U8(em, d->p, d->q, d->dp, d->dq, d->u, signature, n_bits);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSASSA-PKCS1-v1_5 with message
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg                  - message to be verified
 * @param[in]       msg_len              - byte length of message
 * @param[in]       e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]       e_bits               - bit length of e
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. msg_len should in [0, em_bytes-t_bytes-11], em_bytes is
 *           (em_bits+7)/8, t_bytes=hash_bytes+hashAlgID_bytes, hashAlgID_bytes refer to
 *           get_pkcs1_v1_5_t_prefix().
 */
unsigned int rsa_ssa_pkcs1_v1_5_verify(hash_alg_e alg, const unsigned char *msg, unsigned int msg_len, const unsigned char *e, unsigned int e_bits, const unsigned char *n,
                                       unsigned int n_bits, const unsigned char *signature)
{
    unsigned char em1[RSA_MAX_BYTE_LEN];
    unsigned char em2[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_ssa_pkcs1_v1_5_param_check(n, n_bits, e, e_bits, msg, signature);
    if (RSA_SUCCESS == ret)
    {
        ret = emsa_pkcs_v1_5_encode(alg, msg, msg_len, (unsigned char *)em1, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, e, signature, (unsigned char *)em2, n_bits, e_bits, 1);
        if (0U != memcmp_(em1, em2, get_byte_len(n_bits)))
        {
            ret = RSA_ERROR;
        }
        else
        {
            ret = RSA_SUCCESS;
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSASSA-PKCS1-v1_5 with message digest
 * @param[in]       alg                  - specific hash algorithm for message or Hash(message)
 * @param[in]       msg_digest           - Hash(message), message is to be signed, here Hash is msg_hash_alg.
 * @param[in]       e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]       e_bits               - bit length of e
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       signature            - RSA signature, (n_bits+7)/8 bytes, big-endian.
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. msg_len should in [0, em_bytes-t_bytes-11], em_bytes is
 *           (em_bits+7)/8, t_bytes=hash_bytes+hashAlgID_bytes, hashAlgID_bytes refer to
 *           get_pkcs1_v1_5_t_prefix().
 */
unsigned int rsa_ssa_pkcs1_v1_5_verify_by_msg_digest(hash_alg_e alg, const unsigned char *msg_digest, const unsigned char *e, unsigned int e_bits, const unsigned char *n,
                                                     unsigned int n_bits, const unsigned char *signature)
{
    unsigned char em1[RSA_MAX_BYTE_LEN];
    unsigned char em2[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_ssa_pkcs1_v1_5_param_check(n, n_bits, e, e_bits, msg_digest, signature);
    if (RSA_SUCCESS == ret)
    {
        ret = emsa_pkcs_v1_5_encode_by_msg_digest(alg, msg_digest, (unsigned char *)em1, get_byte_len(n_bits));
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, e, signature, (unsigned char *)em2, n_bits, e_bits, 1);
        if (0U != memcmp_(em1, em2, get_byte_len(n_bits)))
        {
            ret = RSA_ERROR;
        }
        else
        {
            ret = RSA_SUCCESS;
        }
    }
    else
    {
    }

    return ret;
}
#endif
