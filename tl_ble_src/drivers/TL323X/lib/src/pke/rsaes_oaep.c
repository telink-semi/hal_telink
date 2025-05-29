/*! @file rsaes_oaep.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_RSAES_OAEP

#include "../../crypto_include/crypto_common/utility.h"
#include "../../crypto_include/hash_hmac/hash.h"
#include "../../crypto_include/pke/rsa_u8.h"
#include "../../crypto_include/trng/trng.h"

/**
 * @brief           check rsaes oaep parameter
 * @param[in]       modulus              - modulus
 * @param[in]       exponent             - exponent
 * @param[in]       n_bits               - bit length of modulus
 * @param[in]       exp_bits             - bit length of exponent
 * @param[in]       msg                  - message to be encode or decode
 * @param[in]       cipher               - cipher to encrypt or decrypt
 * @return          PKE_SUCCESS(success), other(error)
 * @note
 *        1. modulus must be odd
 *        2. please make sure exp_bits <= n_bits <= OPERAND_MAX_BIT_LEN
 */
FLAG_STATIC unsigned int rsa_es_oaep_param_check(const unsigned char *modulus, unsigned int n_bits, const void *exponent, unsigned int exp_bits, const unsigned char *msg,
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
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP encode with label digest
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label_digest         - label digest of label_hash_alg
 * @param[in]       seed                 - Seed, occupy digest bytes of label_hash_alg (can be NULL to generate inside)
 * @param[in]       msg                  - message to be encode
 * @param[in]       msg_len              - byte length of message
 * @param[out]      em                   - big integer to be encrypt, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 *        2. if no seed is prepared to input, please set seed to NULL, it will be generated inside.
 *        3. msg_len should be in [0, em_bytes-2*hLen-2], where em_bytes is
 *           (n_bits+7)/8 and hLen is digest length of hash algorithm label_hash_alg. It
 *           is recommended to use the default value, which is the digest length of the
 *           hash algorithm label_hash_alg or mgf_hash_alg.
 */
unsigned int eme_oaep_encode_by_label_digest(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label_digest, const unsigned char *seed,
                                             const unsigned char *msg, unsigned int msg_len, unsigned char *em, unsigned int em_bytes)
{
    unsigned int ret = RSA_SUCCESS;
    unsigned int label_digest_bytes = (unsigned int)hash_get_digest_word_len(label_hash_alg) << 2; // also byte length of seed
    unsigned char *seed_p = &em[1];
    unsigned char *db;

    if ((NULL == label_digest) || (NULL == em) || (0U == label_digest_bytes))
    {
        ret = RSA_INPUT_INVALID;
    }
    else if (em_bytes < ((label_digest_bytes * 2U) + 2U + msg_len))
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    if (RSA_SUCCESS == ret)
    {
        // set first bytes of EM
        em[0] = 0x00;

        // set seed
        if (NULL == seed)
        {
            ret = get_rand(seed_p, label_digest_bytes);
            if (TRNG_SUCCESS == ret)
            {
                ret = RSA_SUCCESS;
            }
            else
            {
            }
        }
        else
        {
            memcpy_(seed_p, seed, label_digest_bytes);
        }
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        // get lhash is hash(L)
        db = &em[1U + label_digest_bytes];
        memcpy_(db, label_digest, label_digest_bytes);

        // set (PS||0x01), DB = lHash||PS||0x01||M, PS is all zero octets
        memset_(&db[label_digest_bytes], 0U, em_bytes - msg_len - (2U * label_digest_bytes) - 2U);
        em[em_bytes - msg_len - 1u] = 0x01;
        memcpy_(&em[em_bytes - msg_len], msg, msg_len);

        // get maskedDB, EM = 0x00||maskedSeed||maskedDB
        ret = rsa_pkcs1_mgf1_with_xor_in(mgf_hash_alg, seed_p, label_digest_bytes, db, db, em_bytes - 1U - label_digest_bytes);
    }
    else
    {
    }

    if (ret == RSA_SUCCESS)
    {
        // get maskedSeed, EM = 0x00||maskedSeed||maskedDB
        ret = rsa_pkcs1_mgf1_with_xor_in(mgf_hash_alg, db, em_bytes - 1U - label_digest_bytes, seed_p, seed_p, label_digest_bytes);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP encode
 * @param[in]       label_hash_alg       - Specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - Specific hash algorithm for MGF1
 * @param[in]       label                - Label (can be NULL if no label is provided)
 * @param[in]       label_bytes          - Byte length of label (0 if label is NULL)
 * @param[in]       seed                 - Seed, occupy digest bytes of label_hash_alg (can be NULL to generate inside)
 * @param[in]       msg                  - Message to be encoded
 * @param[in]       msg_len              - Byte length of message
 * @param[out]      em                   - big integer to be encrypt, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n
 * @return          RSA_SUCCESS on success, other values indicate error
 * @note
 *        1. It is recommended that label_hash_alg and mgf_hash_alg are the same
 *        2. If no label is prepared to input, please set label to NULL
 *        3. If no seed is prepared to input, please set seed to NULL, it will be generated inside
 *        4. msg_len should be in [1, em_bytes-2*hLen-2], where em_bytes is (n_bits+7)/8 and hLen is
 *           digest length of hash algorithm msg_hash_alg. It is recommended to use the default value,
 *           which is the digest length of the hash algorithm msg_hash_alg or mgf_hash_alg
 */
unsigned int eme_oaep_encode(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label, unsigned int label_bytes, const unsigned char *seed,
                             const unsigned char *msg, unsigned int msg_len, unsigned char *em, unsigned int em_bytes)
{
    unsigned int ret;
    unsigned char label_digest[HASH_DIGEST_MAX_WORD_LEN << 2];

    // get lhash is hash(L)
    ret = hash(label_hash_alg, label, label_bytes, label_digest);
    if (HASH_SUCCESS == ret)
    {
        ret = RSA_SUCCESS;
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_encode_by_label_digest(label_hash_alg, mgf_hash_alg, label_digest, seed, msg, msg_len, em, em_bytes);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP decode with label digest
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label_digest         - label digest of label_hash_alg
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of the message pointer
 * @param[in]       em                   - big integer to be parsed, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 */
unsigned int eme_oaep_decode_by_label_digest(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label_digest, unsigned char *msg, unsigned int *msg_len,
                                             const unsigned char *em, unsigned int em_bytes)
{
    unsigned int ret = RSA_SUCCESS, i;
    unsigned int label_digest_bytes = (unsigned int)hash_get_digest_word_len(label_hash_alg) << 2; // also byte length of seed
    unsigned char seed[HASH_DIGEST_MAX_WORD_LEN << 2];
    unsigned char db[RSA_MAX_WORD_LEN << 2];

    if ((NULL == label_digest) || (NULL == msg) || (NULL == msg_len) || ((em[0] != (unsigned char)0x00)))
    {
        ret = RSA_INPUT_INVALID;
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        // get Seed = MGF1 xor MFG1(maskedDB, hLen)
        ret = rsa_pkcs1_mgf1_with_xor_in(mgf_hash_alg, &em[1U + label_digest_bytes], em_bytes - 1U - label_digest_bytes, NULL, seed, label_digest_bytes);
    }
    else
    {
    }

    if (ret == RSA_SUCCESS)
    {
        // get DB = maskedDB xor MGF1(seed, k-hLen-1)
        uint8_xor(seed, &em[1], seed, label_digest_bytes);
        ret = rsa_pkcs1_mgf1_with_xor_in(mgf_hash_alg, seed, label_digest_bytes, &em[1U + label_digest_bytes], db, em_bytes - 1U - label_digest_bytes);
    }
    else
    {
    }

    if (ret == RSA_SUCCESS)
    {
        // check lHash, DB = lHash||PS||0x01||M
        if (0U != memcmp_(label_digest, db, label_digest_bytes))
        {
            ret = RSA_INPUT_INVALID;
        }
        else
        {
        }
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        // get the location of 0x01, DB = lHash||PS||0x01||M
        for (i = label_digest_bytes; i < (em_bytes - label_digest_bytes - 1U); i++)
        {
            if (db[i] != (unsigned char)0x00)
            {
                break;
            }
            else
            {
            }
        }

        // check the location of 0x01
        if (((em_bytes - label_digest_bytes - 1U) == i) || (db[i] != (unsigned char)0x01))
        {
            ret = RSA_INPUT_INVALID;
        }
        else
        {
            // get the msg and msg bytes
            *msg_len = em_bytes - label_digest_bytes - 1U - (i + 1U);
            memcpy_(msg, &db[i + 1U], *msg_len);
        }
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP decode
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label                - label
 * @param[in]       label_bytes          - byte length of label
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of the message pointer
 * @param[in]       em                   - big integer to be parsed, big-endian.
 * @param[in]       em_bytes             - byte length of em, should be bit length of RSA modulus n
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 */
unsigned int eme_oaep_decode(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label, unsigned int label_bytes, unsigned char *msg, unsigned int *msg_len,
                             const unsigned char *em, unsigned int em_bytes)
{
    unsigned int ret = RSA_SUCCESS;
    unsigned char label_digest[HASH_DIGEST_MAX_WORD_LEN << 2];

    // check lHash, DB = lHash||PS||0x01||M
    ret = hash(label_hash_alg, label, label_bytes, label_digest);
    if (HASH_SUCCESS == ret)
    {
        ret = RSA_SUCCESS;
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_decode_by_label_digest(label_hash_alg, mgf_hash_alg, label_digest, msg, msg_len, em, em_bytes);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP-ENCRYPT with label digest
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label_digest         - label digest of label_hash_alg
 * @param[in]       seed                 - Seed, occupy digest bytes of label_hash_alg (can be NULL to generate inside)
 * @param[in]       msg                  - message to be encrypt
 * @param[in]       msg_len              - byte length of message
 * @param[in]       e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]       e_bits               - bit length of e
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 *        2. if no seed is prepared to input, please set seed to NULL, it will be
 *           generated inside.
 *        3. msg_len should be in [1, em_bytes-2*hLen-2], where em_bytes is
 *           (n_bits+7)/8 and hLen is digest length of hash algorithm label_hash_alg. It
 *           is recommended to use the default value, which is the digest length of the
 *           hash algorithm label_hash_alg or mgf_hash_alg.
 */
unsigned int rsa_es_oaep_enc_by_label_digest(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label_digest, const unsigned char *seed,
                                             const unsigned char *msg, unsigned int msg_len, const unsigned char *e, unsigned int e_bits, const unsigned char *n,
                                             unsigned int n_bits, unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_oaep_param_check(n, n_bits, e, e_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_encode_by_label_digest(label_hash_alg, mgf_hash_alg, label_digest, seed, msg, msg_len, em, get_byte_len(n_bits));
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
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP-ENCRYPT with message
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label                - label (can be NULL if no label is provided)
 * @param[in]       label_bytes          - byte length of label (0 if label is NULL)
 * @param[in]       seed                 - Seed, occupy digest bytes of label_hash_alg (can be NULL to generate inside)
 * @param[in]       msg                  - message to be encrypt
 * @param[in]       msg_len              - byte length of message
 * @param[in]       e                    - RSA public key e, (e_bits+7)/8 bytes, big-endian.
 * @param[in]       e_bits               - bit length of e
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[out]      cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 *        2. if no label is prepared to input, please set label to NULL.
 *        3. if no seed is prepared to input, please set seed to NULL, it will be
 *           generated inside.
 *        4. msg_len should be in [0, em_bytes-2*hLen-2], where em_bytes is
 *           (n_bits+7)/8 and hLen is digest length of hash algorithm label_hash_alg. It
 *           is recommended to use the default value, which is the digest length of the
 *           hash algorithm label_hash_alg or mgf_hash_alg.
 */
unsigned int rsa_es_oaep_enc(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label, unsigned int label_bytes, const unsigned char *seed,
                             const unsigned char *msg, unsigned int msg_len, const unsigned char *e, unsigned int e_bits, const unsigned char *n, unsigned int n_bits,
                             unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_oaep_param_check(n, n_bits, e, e_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_encode(label_hash_alg, mgf_hash_alg, label, label_bytes, seed, msg, msg_len, em, get_byte_len(n_bits));
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
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP-DECRYPT with message
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label                - label
 * @param[in]       label_bytes          - byte length of label
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of the message pointer
 * @param[in]       d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 */
unsigned int rsa_es_oaep_dec(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label, unsigned int label_bytes, unsigned char *msg, unsigned int *msg_len,
                             const unsigned char *d, const unsigned char *n, unsigned int n_bits, const unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_oaep_param_check(n, n_bits, d, n_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, d, cipher, em, n_bits, n_bits, 1);
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_decode(label_hash_alg, mgf_hash_alg, label, label_bytes, msg, msg_len, em, get_byte_len(n_bits));
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP-DECRYPT with label digest
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label_digest         - label digest of label_hash_alg
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of the message pointer
 * @param[in]       d                    - RSA private key d, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 */
unsigned int rsa_es_oaep_dec_by_label_digest(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label_digest, unsigned char *msg, unsigned int *msg_len,
                                             const unsigned char *d, const unsigned char *n, unsigned int n_bits, const unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_oaep_param_check(n, n_bits, d, n_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = pke_modexp_u8(n, d, cipher, em, n_bits, n_bits, 1);
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_decode_by_label_digest(label_hash_alg, mgf_hash_alg, label_digest, msg, msg_len, em, get_byte_len(n_bits));
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP-DECRYPT with label digest (private key is CRT style)
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label_digest         - label digest of label_hash_alg
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of the message pointer
 * @param[in]       d                    - RSA-CRT private key (p, q, dp, dq, u), every field is (n_bits/2+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 */
unsigned int rsa_es_oaep_crt_dec_by_label_digest(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label_digest, unsigned char *msg, unsigned int *msg_len,
                                                 const rsa_crt_private_key_t *d, const unsigned char *n, unsigned int n_bits, const unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret;

    ret = rsa_es_oaep_param_check(n, n_bits, d, n_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = RSA_CRTModExp_U8(cipher, d->p, d->q, d->dp, d->dq, d->u, em, n_bits);
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_decode_by_label_digest(label_hash_alg, mgf_hash_alg, label_digest, msg, msg_len, em, get_byte_len(n_bits));
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           RSA PKCS#1_v2.2 RSAES-OAEP-DECRYPT with message (private key is CRT style)
 * @param[in]       label_hash_alg       - specific hash algorithm for message or Hash(label)
 * @param[in]       mgf_hash_alg         - specific hash algorithm for MGF1
 * @param[in]       label                - label
 * @param[in]       label_bytes          - byte length of label
 * @param[out]      msg                  - message to be decrypted
 * @param[out]      msg_len              - byte length of the message pointer
 * @param[in]       d                    - RSA-CRT private key (p, q, dp, dq, u), every field is (n_bits/2+7)/8 bytes, big-endian.
 * @param[in]       n                    - RSA modulus n, (n_bits+7)/8 bytes, big-endian.
 * @param[in]       n_bits               - bit length of n
 * @param[in]       cipher               - RSA cipher, (n_bits+7)/8 bytes, big-endian.
 * @return          RSA_SUCCESS(success), other(error)
 * @note
 *        1. it is recommended that label_hash_alg and mgf_hash_alg are the same.
 */
unsigned int rsa_es_oaep_crt_dec(hash_alg_e label_hash_alg, hash_alg_e mgf_hash_alg, const unsigned char *label, unsigned int label_bytes, unsigned char *msg,
                                 unsigned int *msg_len, const rsa_crt_private_key_t *d, const unsigned char *n, unsigned int n_bits, const unsigned char *cipher)
{
    unsigned char em[RSA_MAX_BYTE_LEN];
    unsigned int ret = RSA_SUCCESS;

    ret = rsa_es_oaep_param_check(n, n_bits, d, n_bits, msg, cipher);
    if (RSA_SUCCESS == ret)
    {
        ret = RSA_CRTModExp_U8(cipher, d->p, d->q, d->dp, d->dq, d->u, em, n_bits);
    }
    else
    {
    }

    if (RSA_SUCCESS == ret)
    {
        ret = eme_oaep_decode(label_hash_alg, mgf_hash_alg, label, label_bytes, msg, msg_len, em, get_byte_len(n_bits));
    }
    else
    {
    }

    return ret;
}
#endif
