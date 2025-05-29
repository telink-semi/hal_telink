/********************************************************************************************************
 * @file    ed25519.c
 *
 * @brief   This is the source file for TL323X
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
#include <stdio.h>
#include "string.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/pke/pke.h"
#include "lib/include/hash/hash.h"
#include "lib/include/pke/ed25519.h"
#include "lib/include/trng/trng.h"
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_C25519

//"SigEd25519 no Ed25519 collisions"
static const unsigned char Ed25519_sign_string[] = {0x53, 0x69, 0x67, 0x45, 0x64, 0x32, 0x35, 0x35, 0x31, 0x39, 0x20, 0x6e, 0x6f, 0x20, 0x45, 0x64,
                                                    0x32, 0x35, 0x35, 0x31, 0x39, 0x20, 0x63, 0x6f, 0x6c, 0x6c, 0x69, 0x73, 0x69, 0x6f, 0x6e, 0x73};

/**
 * @brief           Decode X25519 scalar for point multiplication
 * @param[in]       k                    - scalar
 * @param[out]      out                  - big scalar in little-endian format
 * @param[in]       bytes                - Byte length of k and out
 */
void x25519_decode_scalar(const unsigned char *k, unsigned char *out, unsigned int bytes)
{
    (void)bytes;

    if (k != out)
    {
        memcpy_(out, k, C25519_BYTE_LEN);
    }
    else
    {
    }

// actually this is internal interface,the caller ensures out is not NULL
#ifdef SUPPORT_STATIC_ANALYSIS
    if (NULL != out)
    {
#endif
        out[0] &= (unsigned char)0xF8;                    // clear lowest 3 bits
        out[C25519_BYTE_LEN - 1u] &= (unsigned char)0x7F; // clear highest 1 bit
        out[C25519_BYTE_LEN - 1u] |= (unsigned char)0x40; // set second highest bit as 1
#ifdef SUPPORT_STATIC_ANALYSIS
    }
    else
    {
    }
#endif
}

/**
 * @brief           edwards25519 curve point mul(random point), Q=[k]P, secure version
 * @param[in]       curve                - edwards25519 curve struct pointer
 * @param[in]       k                    - scalar, it could be 0 here
 * @param[in]       px                   - x coordinate of point P
 * @param[in]       Py                   - y coordinate of point P
 * @param[out]      qx                   - x coordinate of point Q
 * @param[out]      qy                   - y coordinate of point Q
 * @return          0:success     other:error
 * @note
 *        1.please make sure point P is on the curve
 *        2.even if the point P is valid, the output may be neutral point (0, 1), it is valid
 *        3.please make sure the curve is edwards25519
 *        4.k could be zero here.
 */
static unsigned int ed25519_pointMul_s_internal(const edward_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx,
                                                unsigned int *qy)
{
#if 1
    unsigned int ret;

    if (0u != uint32_bignum_check_zero(k, 8u))
    {
        uint32_clear_8_words(qx);
        pke_set_operand_uint32_value_256bits(qy, 1u);

        ret = PKE_SUCCESS;
    }
    else
    {
        ret = ed25519_pointMul_internal(curve, k, px, Py, qx, qy);
    }

    return ret;
#else
    unsigned int ret;
    unsigned int p_wlen = get_word_len(curve->p_bitLen);
    unsigned int n_wlen = get_word_len(curve->n_bitLen);

    if (0u != uint32_bignum_check_zero(k, n_wlen))
    {
        uint32_clear(qx, p_wlen);
        pke_set_operand_uint32_value_256bits(qy, 1u);

        ret = PKE_SUCCESS;
    }
    else
    {
        ret = ed25519_pointMul_internal(curve, k, px, Py, qx, qy);
    }

    return ret;
#endif
}

/**
 * @brief           Edwards25519 curve point multiplication (secure version), Q=[k]P
 * @param[in]       curve                - Edwards25519 curve struct pointer
 * @param[in]       k                    - Scalar (can be zero)
 * @param[in]       px                   - x coordinate of point P
 * @param[in]       Py                   - y coordinate of point P
 * @param[out]      qx                   - x coordinate of point Q
 * @param[out]      qy                   - y coordinate of point Q
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. Ensure point P is on the curve
 *        2. may be neutral point (0, 1) even if P is valid
 *        3. Curve must be edwards25519
 *        4. k can be zero
 */
static unsigned int ed25519_pointMul_s(const edward_curve_t *curve, const unsigned int *k, const unsigned int *px, const unsigned int *Py, unsigned int *qx, unsigned int *qy)
{
    unsigned int ret;

#if 1
    ret = pke_set_modulus_and_pre_monts(ed25519->p, ed25519->p_h, ed25519->p_n0, ed25519->p_bitLen);
#else
    ret = pke_load_modulus_and_pre_monts_256bits(ed25519->p, ed25519->p_h);
#endif
    if (PKE_SUCCESS == ret)
    {
        ret = ed25519_pointMul_s_internal(curve, k, px, Py, qx, qy);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Get Ed25519 public key from private key
 * @param[in]       prikey               - Private key, 32 bytes, little-endian
 * @param[out]      pubkey               - Public key, 32 bytes, little-endian
 * @return          EdDSA_SUCCESS on success, other values indicate error
 */
unsigned int ed25519_get_pubkey_from_prikey(const unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int h[16];
    unsigned int ret;

    if ((NULL == prikey) || (NULL == pubkey))
    {
        return EdDSA_POINTER_NULL;
    }
    else
    {
    }

    ret = hash(HASH_SHA512, prikey, 32, (unsigned char *)h);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // decode to get the scalar
    x25519_decode_scalar((unsigned char *)h, (unsigned char *)h, Ed25519_BYTE_LEN);

    ret = ed25519_pointMul_s(ed25519, h, ed25519->Gx, ed25519->Gy, h, &h[8]);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // encode pubkey
    memcpy_(pubkey, (unsigned char *)(&h[8]), Ed25519_BYTE_LEN);
    if (0u != (h[0] & 1u))
    {
        pubkey[Ed25519_BYTE_LEN - 1u] |= (unsigned char)0x80;
    }
    else
    {
    }

    return EdDSA_SUCCESS;
}

/**
 * @brief           generate Ed25519 random key pair
 * @param[out]      prikey               - private key, 32 bytes, little-endian
 * @param[out]      pubkey               - public key, 32 bytes, little-endian
 * @return          0:success     other:error
 * @note
 */
unsigned int ed25519_getkey(unsigned char prikey[32], unsigned char pubkey[32])
{
    unsigned int ret;

    if ((NULL == prikey) || (NULL == pubkey))
    {
        return EdDSA_POINTER_NULL;
    }
    else
    {
    }

    ret = get_rand(prikey, Ed25519_BYTE_LEN);
    if (TRNG_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        return ed25519_get_pubkey_from_prikey(prikey, pubkey);
    }
}

/**
 * @brief           Check parameters for Ed25519 sign/verify operations
 * @param[in]       mode                 - Ed25519 signature mode
 * @param[in]       prikey_pubkey        - Pointer to private key (for signing) or public key (for verifying), 32 bytes, little-endian
 * @param[in]       ctx                  - Pointer to context data, 0-255 bytes
 * @param[in]       ctx_bytes            - Pointer to byte length of ctx
 * @param[in]       M                    - Pointer to the message, requirements depend on mode
 * @param[in]       msg_len              - Pointer to byte length of M, requirements depend on mode
 * @param[in]       RS                   - Pointer to signature
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. For signing, prikey_pubkey is pointer to private key; for verifying, it's pointer to public key
 *        2. If mode is not Ed25519_PH_WITH_PH_M, M can be NULL (no need to check M and msg_len)
 *        3. For Ed25519_PH_WITH_PH_M, M is SHA512 digest (64 bytes) and msg_len is not used
 *        4. For Ed25519_DEFAULT, ctx is not used
 *        5. For Ed25519_CTX, ctx cannot be empty (ctx_bytes must be 1-255)
 *        6. For Ed25519_PH or Ed25519_PH_WITH_PH_M, ctx_bytes can be 0-255 (default 0, ctx can be empty)
 */
static unsigned int ed25519_check_input(ed25519_mode_e mode, const unsigned char *prikey_pubkey, const unsigned char *ctx, unsigned char *ctx_bytes, const unsigned char *M,
                                        unsigned int *msg_len, const unsigned char *RS)
{
    unsigned int ret;

    if (mode > Ed25519_PH_WITH_PH_M)
    {
        ret = EdDSA_INVALID_INPUT;
    }
    else if ((NULL == prikey_pubkey) || (NULL == RS))
    {
        ret = EdDSA_POINTER_NULL;
    }
    else
    {
        ret = PKE_SUCCESS;

        // if mode is not Ed25519_PH_WITH_PH_M, M could be empty,
        // so M could be NUll, msg_len could be 0, no need to check them
        if (Ed25519_PH_WITH_PH_M != mode)
        {
            if (NULL == M)
            {
                *msg_len = 0u;
            }
            else
            {
            }
        }
        else
        {
            if (NULL == M)
            {
                ret = EdDSA_INVALID_INPUT;
            }
            else
            {
                *msg_len = 64u; // SHA512 digest
            }
        }
    }

    if (PKE_SUCCESS == ret)
    {
        if ((Ed25519_CTX == mode) && ((NULL == ctx) || (((unsigned char)0) == ctx_bytes[0]))) // in this case ctx can not be empty
        {
            ret = EdDSA_INVALID_INPUT;
        }
        else if (((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode)) && (NULL == ctx)) // in this case ctx could be empty
        {
            *ctx_bytes = 0;
        }
        else
        {
            // Ed25519_DEFAULT mode, ctx is useless
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 sign step 1 (internal API)
 * @param[in]       mode                 - Ed25519 signature mode
 * @param[out]      phflag               - Flag indicating PH mode (1 for Ed25519_PH/Ed25519_PH_WITH_PH_M, 0 for Ed25519_CTX)
 * @param[in]       prikey               - Private key, 32 bytes, little-endian
 * @param[in]       M                    - Message, requirements depend on mode
 * @param[in]       msg_len              - Byte length of M, requirements depend on mode
 * @param[out]      digest               - SHA512(prikey) containing secret scalar s and prefix
 * @param[out]      PH_M                 - SHA512(M) for Ed25519_PH mode, unused otherwise
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. If mode is not Ed25519_PH_WITH_PH_M, M can be NULL (no need to check M and
 *           msg_len)
 *        2. For Ed25519_PH_WITH_PH_M, M is SHA512 digest (64 bytes) and msg_len is
 *           not used
 */
static unsigned int ed25519_sign_internal_step_1(ed25519_mode_e mode, unsigned char *phflag, const unsigned char *prikey, const unsigned char *M, unsigned int msg_len,
                                                 unsigned char *digest, unsigned char *PH_M)
{
    unsigned int ret;

    // get private scalar s and prefix
    ret = hash(HASH_SHA512, prikey, Ed25519_BYTE_LEN, digest);
    if (HASH_SUCCESS == ret)
    {
        // decode to get the scalar s
        x25519_ed25519_decode_scalar((unsigned char *)digest, (unsigned char *)digest);

        // set flag F
        if (Ed25519_CTX == mode)
        {
            *phflag = 0;
        }
        else if ((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode))
        {
            *phflag = 1;
        }
        else
        {
            // Ed25519_DEFAULT mode, phflag is useless
        }

        // PH_M
        if (Ed25519_PH == mode)
        {
            ret = hash(HASH_SHA512, M, msg_len, (unsigned char *)PH_M);
        }
        else
        {
        }
    }
    else
    {
    }

    if (HASH_SUCCESS == ret)
    {
        ret = PKE_SUCCESS;
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 sign step 2 (internal API)
 * @param[in]       mode                 - Ed25519 signature mode
 * @param[in]       phflag               - PH mode flag (1 for Ed25519_PH/Ed25519_PH_WITH_PH_M, 0 for Ed25519_CTX)
 * @param[in]       ctx                  - Context data, 0-255 bytes
 * @param[in]       ctx_bytes            - Byte length of ctx
 * @param[in]       prefix               - Second half of SHA512(prikey), 32 bytes, little-endian
 * @param[in]       M                    - Message, requirements depend on mode
 * @param[in]       msg_len              - Byte length of M, requirements depend on mode
 * @param[in]       PH_M                 - SHA512(M) for Ed25519_PH mode
 * @param[out]      k                    - hash value: SHA512(dom2(F, C) || prefix || PH(M))
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. If mode is not Ed25519_PH_WITH_PH_M, M can be NULL (no need to check M and msg_len)
 *        2. For Ed25519_PH_WITH_PH_M, M is SHA512 digest (64 bytes) and msg_len is not used
 */
static unsigned int ed25519_sign_internal_step_2(ed25519_mode_e mode, unsigned char phflag, const unsigned char *ctx, unsigned char ctx_bytes, const unsigned char *prefix,
                                                 const unsigned char *M, unsigned int msg_len, const unsigned char *PH_M, unsigned char *k)
{
    unsigned int ret;
    hash_ctx_t sha512_ctx_t[1];
    unsigned char buf[2];

    // get k := SHA512(dom2(F, C) || prefix || PH(M))
    ret = hash_init(sha512_ctx_t, HASH_SHA512);
    if ((HASH_SUCCESS == ret) && (Ed25519_DEFAULT != mode))
    {
        // dom2(phflag, ctx)
        ret = hash_update(sha512_ctx_t, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        if (HASH_SUCCESS == ret)
        {
            buf[0] = phflag;
            buf[1] = ctx_bytes;
            ret = hash_update(sha512_ctx_t, buf, 2u);
        }
        else
        {
        }

        if (HASH_SUCCESS == ret)
        {
            ret = hash_update(sha512_ctx_t, ctx, ctx_bytes);
        }
        else
        {
        }
    }
    else
    {
    }

    // prefix
    if (HASH_SUCCESS == ret)
    {
        ret = hash_update(sha512_ctx_t, prefix, Ed25519_BYTE_LEN);
    }
    else
    {
    }

    // PH(M)
    if (HASH_SUCCESS == ret)
    {
        if (Ed25519_PH == mode)
        {
            ret = hash_update(sha512_ctx_t, PH_M, 64u);
        }
        else
        {
            ret = hash_update(sha512_ctx_t, M, msg_len);
        }
    }
    else
    {
    }

    if (HASH_SUCCESS == ret)
    {
        ret = hash_final(sha512_ctx_t, (unsigned char *)k);
    }
    else
    {
    }

    if (HASH_SUCCESS == ret)
    {
        ret = PKE_SUCCESS;
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Compute k modulo n
 * @param[in]       k                    - value, 16 words, little-endian
 * @param[out]      out                  - value, k mod n, 8 words, little-endian
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note            k and out cannot point to the same buffer
 */
static unsigned int ed25519_k_mod_n(unsigned int *k, unsigned int *out)
{
    unsigned int ret;

// out = k mod n
#if defined(PKE_LP)
    ret = pke_mod(&(k[Ed25519_WORD_LEN - 1u]), Ed25519_WORD_LEN + 1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, out);
#else
    ret = pke_mod(&(k[Ed25519_WORD_LEN - 1u]), Ed25519_WORD_LEN + 1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, out);
#endif
    if (PKE_SUCCESS == ret)
    {
        uint32_copy_8_words(&(k[Ed25519_WORD_LEN - 1u]), out);
#if defined(PKE_LP)
        ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, out);
#else
        ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, out);
#endif
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 sign step 3 (internal API)
 * @param[in]       k                    - hash value: SHA512(dom2(F, C) || prefix || PH(M))
 * @param[out]      r                    - middle value
 * @param[out]      R                    - Encoded point [r]G, first half of the signature
 * @return          PKE_SUCCESS on success, other values indicate error
 */
static unsigned int ed25519_sign_internal_step_3(unsigned int *k, unsigned int *r, unsigned char *R)
{
    unsigned int ret;

    // r = k mod n
    ret = ed25519_k_mod_n(k, r);
    if (PKE_SUCCESS == ret)
    {
        ret = ed25519_pointMul_s(ed25519, r, ed25519->Gx, ed25519->Gy, k, &k[Ed25519_WORD_LEN]);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        memcpy_(R, (unsigned char *)(&k[Ed25519_WORD_LEN]), Ed25519_BYTE_LEN);
        if (0u != (k[0] & 1u))
        {
            R[Ed25519_BYTE_LEN - 1u] |= (unsigned char)0x80;
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
 * @brief           Hash update R||A (part of Ed25519 sign step 4, internal API)
 * @param[in]       sha512_ctx_t         - Initialized hash context structure
 * @param[in]       R                    - First half of signature, 32 bytes
 * @param[in]       pubkey               - Public key, 32 bytes
 * @param[in]       s                    - Secret scalar s
 * @param[in]       k                    - Temporary buffer to store public key point [s]G if pubkey is NULL
 * @return          HASH_SUCCESS on success, other values indicate error
 * @note            If pubkey is NULL, the public key will be generated from scalar s
 */
static unsigned int ed25519_sign_internal_step_4_hash_update_R_and_pubkey(hash_ctx_t *sha512_ctx_t, const unsigned char *R, const unsigned char *pubkey, const unsigned int *s,
                                                                          unsigned int *k)
{
    unsigned int ret;

    ret = hash_update(sha512_ctx_t, R, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS == ret)
    {
        if (NULL == pubkey)
        {
            ret = ed25519_pointMul_s_internal(ed25519, s, ed25519->Gx, ed25519->Gy, k, &k[Ed25519_WORD_LEN]);
            if (PKE_SUCCESS == ret)
            {
                if (0u != (k[0] & 1u))
                {
                    k[(Ed25519_WORD_LEN << 1) - 1u] |= 0x80000000u;
                }
                else
                {
                }

                ret = hash_update(sha512_ctx_t, (unsigned char *)(&k[Ed25519_WORD_LEN]), Ed25519_BYTE_LEN);
            }
            else
            {
            }
        }
        else
        {
            ret = hash_update(sha512_ctx_t, pubkey, Ed25519_BYTE_LEN);
        }
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 sign step 4 (internal API)
 * @param[in]       mode                 - Ed25519 signature mode
 * @param[in]       phflag               - PH mode flag (1 for Ed25519_PH/Ed25519_PH_WITH_PH_M, 0 for Ed25519_CTX)
 * @param[in]       ctx                  - Context data, 0-255 bytes
 * @param[in]       ctx_bytes            - Byte length of ctx
 * @param[in]       R                    - First half of signature, 32 bytes
 * @param[in]       s                    - Secret scalar s
 * @param[in]       pubkey               - Public key, 32 bytes
 * @param[in]       M                    - Message, requirements depend on mode
 * @param[in]       msg_len              - Byte length of M, requirements depend on mode
 * @param[in]       PH_M                 - SHA512(M) for Ed25519_PH mode
 * @param[out]      k                    - hash value: SHA512(dom2(F, C) || R || A || PH(M))
 * @return          PKE_SUCCESS on success, other values indicate error
 * @note
 *        1. If mode is not Ed25519_PH_WITH_PH_M, M can be NULL (no need to check M and
 *           msg_len)
 *        2. For Ed25519_PH_WITH_PH_M, M is SHA512 digest (64 bytes) and msg_len is
 *           not used
 */
static unsigned int ed25519_sign_internal_step_4(ed25519_mode_e mode, unsigned char phflag, const unsigned char *ctx, unsigned char ctx_bytes, const unsigned char *R,
                                                 const unsigned int *s, const unsigned char *pubkey, const unsigned char *M, unsigned int msg_len, const unsigned char *PH_M,
                                                 unsigned int *k)
{
    unsigned int ret;
    hash_ctx_t sha512_ctx_t[1];
    unsigned char buf[2];

    // get k := SHA512(dom2(F, C) || R || A || PH(M))
    ret = hash_init(sha512_ctx_t, HASH_SHA512);
    if ((HASH_SUCCESS == ret) && (Ed25519_DEFAULT != mode))
    {
// dom2(phflag, ctx)
#if 1
        ret = hash_update(sha512_ctx_t, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        if (HASH_SUCCESS == ret)
        {
            buf[0] = phflag;
            buf[1] = ctx_bytes;
            ret = hash_update(sha512_ctx_t, buf, 2u);
        }
        else
        {
        }

        if (HASH_SUCCESS == ret)
        {
            ret = hash_update(sha512_ctx_t, ctx, ctx_bytes);
        }
        else
        {
        }
#else
        memcpy_(sha512_ctx_t->hash_buffer, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        sha512_ctx_t->hash_buffer[sizeof(Ed25519_sign_string)] = phflag;
        sha512_ctx_t->hash_buffer[1u + sizeof(Ed25519_sign_string)] = ctx_bytes;
        sha512_ctx_t->total[0] = 2u + sizeof(Ed25519_sign_string);
#endif
    }
    else
    {
    }

    // R and pubkey(A)
    if (HASH_SUCCESS == ret)
    {
        ret = ed25519_sign_internal_step_4_hash_update_R_and_pubkey(sha512_ctx_t, R, pubkey, s, k);
    }
    else
    {
    }

    // PH(M)
    if (HASH_SUCCESS == ret)
    {
        if (Ed25519_PH == mode)
        {
            ret = hash_update(sha512_ctx_t, PH_M, 64u);
        }
        else
        {
            ret = hash_update(sha512_ctx_t, M, msg_len);
        }
    }
    else
    {
    }

    if (HASH_SUCCESS == ret)
    {
        ret = hash_final(sha512_ctx_t, (unsigned char *)k);
    }
    else
    {
    }

    if (HASH_SUCCESS == ret)
    {
        ret = PKE_SUCCESS;
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 sign step 5 (internal API)
 * @param[in]       r                    - value: r = SHA512(dom2(F, C) || prefix || PH(M)) mod n, 8 words, little-endian
 * @param[in]       k                    - value: k = SHA512(dom2(F, C) || R || A || PH(M)), 16 words, little-endian
 * @param[in]       secret_s             - Secret scalar s, first half of SHA512(prikey)
 * @param[in]       tmp                  - Temporary buffer, 8 words
 * @param[out]      S                    - value: S = (r + k * s) mod n, second half of signature, 32 bytes, little-endian
 * @return          PKE_SUCCESS on success, other values indicate error
 */
static unsigned int ed25519_sign_internal_step_5(const unsigned int *r, unsigned int *k, const unsigned int *secret_s, unsigned int *tmp, unsigned char *S)
{
    unsigned int ret;

// tmp = k mod n
#if defined(PKE_LP)
    ret = pke_mod(&(k[Ed25519_WORD_LEN - 1u]), Ed25519_WORD_LEN + 1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, tmp);
#else
    ret = pke_mod(&(k[Ed25519_WORD_LEN - 1u]), Ed25519_WORD_LEN + 1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, tmp);
#endif
    if (PKE_SUCCESS == ret)
    {
        uint32_copy_8_words(&k[Ed25519_WORD_LEN - 1u], tmp);
#if defined(PKE_LP)
        ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, tmp);
#else
        ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, tmp);
#endif
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
// k = s mod n
#if defined(PKE_LP)
        ret = pke_mod(secret_s, Ed25519_WORD_LEN, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, k);
#else
        ret = pke_mod(secret_s, Ed25519_WORD_LEN, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, k);
#endif
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
// k = k*s
#if 0
        ret = pke_modmul(ed25519->n, tmp, k, k, Ed25519_WORD_LEN);
#else
        ret = pke_load_modulus_and_pre_monts_256bits(ed25519->n, ed25519->n_h, ed25519->n_n0);
        if (PKE_SUCCESS == ret)
        {
            ret = pke_mod_add_sub_mul_256bits_internal(tmp, k, k, MICROCODE_MODMUL);
        }
        else
        {
        }
#endif
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
// k = (r+k*s)mod n
#if 0
        ret = pke_modadd(ed25519->n, k, r, k, Ed25519_WORD_LEN);
#else
        ret = pke_mod_add_sub_mul_256bits_internal(k, r, k, MICROCODE_MODADD);
#endif
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        memcpy_(S, (unsigned char *)k, Ed25519_BYTE_LEN);
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 sign function
 * @param[in]       mode                 - Ed25519 signature mode
 * @param[in]       prikey               - Private key, 32 bytes, little-endian
 * @param[in]       pubkey               - Public key, 32 bytes, little-endian (NULL if not available)
 * @param[in]       ctx                  - Context data, 0-255 bytes
 * @param[in]       ctx_len              - Byte length of ctx
 * @param[in]       M                    - Message, requirements depend on mode
 * @param[in]       m_len                - Byte length of M, requirements depend on mode
 * @param[out]      RS                   - signature
 * @return          EdDSA_SUCCESS on success, other values indicate error
 * @note
 *        1. If pubkey is NULL, it will be generated internally
 *        2. If mode is not Ed25519_PH_WITH_PH_M, M can be NULL (no need to check M and m_len)
 *        3. For Ed25519_PH_WITH_PH_M, M is SHA512 digest (64 bytes) and m_len is not used
 *        4. For Ed25519_DEFAULT, ctx is not involved
 *        5. For Ed25519_CTX, ctx cannot be empty (length 1-255)
 *        6. For Ed25519_PH/Ed25519_PH_WITH_PH_M, ctx length is 0-255 (default 0, can be empty)
 */
unsigned int ed25519_sign(ed25519_mode_e mode, const unsigned char prikey[32], const unsigned char pubkey[32], const unsigned char *ctx, unsigned char ctx_len,
                          const unsigned char *M, unsigned int m_len, unsigned char RS[64])
{
    unsigned int ret;
    unsigned int h[16];
    const unsigned int *s = h;
    const unsigned char *prefix = (unsigned char *)(&h[Ed25519_WORD_LEN]);

    unsigned int *r = &h[Ed25519_WORD_LEN];
    unsigned int k[Ed25519_WORD_LEN << 1];
    unsigned int PH_M[Ed25519_WORD_LEN << 1];

    unsigned int msg_len = m_len;
    unsigned char ctx_bytes = ctx_len;
    unsigned char phflag = 0;

    ret = ed25519_check_input(mode, prikey, ctx, &ctx_bytes, M, &msg_len, RS);
    if (PKE_SUCCESS == ret)
    {
        // get phflag, h=s||prefix=SHA512(prikey), PH_M=SHA512(M) for Ed25519_PH
        // mode
        ret = ed25519_sign_internal_step_1(mode, &phflag, prikey, M, msg_len, (unsigned char *)h, (unsigned char *)PH_M);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        // get k := SHA512(dom2(F, C) || prefix || PH(M))
        ret = ed25519_sign_internal_step_2(mode, phflag, ctx, ctx_bytes, prefix, M, msg_len, (unsigned char *)PH_M, (unsigned char *)k);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        // get r := k mod n, R := [r]B
        ret = ed25519_sign_internal_step_3(k, r, RS);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        // get k := SHA512(dom2(F, C) || R || A || PH(M))
        ret = ed25519_sign_internal_step_4(mode, phflag, ctx, ctx_bytes, RS, s, pubkey, M, msg_len, (unsigned char *)PH_M, k);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        // get S = (r + k * s) mod n
        ret = ed25519_sign_internal_step_5(r, k, s, PH_M, &RS[Ed25519_BYTE_LEN]);
    }
    else
    {
    }

    if (PKE_SUCCESS == ret)
    {
        ret = EdDSA_SUCCESS;
    }
    else
    {
    }

    return ret;
}

/**
 * @brief           Ed25519 verify function
 * @param[in]       mode                 - Ed25519 signature mode
 * @param[in]       pubkey               - Public key, 32 bytes, little-endian
 * @param[in]       ctx                  - Context data, 0-255 bytes
 * @param[in]       ctx_len              - Byte length of ctx
 * @param[in]       M                    - Message, requirements depend on mode
 * @param[in]       m_len                - Byte length of M (can be 0 if M is empty)
 * @param[in]       RS                   - signature
 * @return          EdDSA_SUCCESS on success, other values indicate error
 * @note
 *        1. If mode is not Ed25519_PH_WITH_PH_M, M can be NULL (no need to check M and m_len)
 *        2. For Ed25519_PH_WITH_PH_M, M is SHA512 value (64 bytes, big-endian) and m_len is not used
 *        3. For Ed25519_DEFAULT, ctx is not involved
 *        4. For Ed25519_CTX, ctx cannot be empty (length 1-255)
 *        5. For Ed25519_PH/Ed25519_PH_WITH_PH_M, ctx length is 0-255 (default 0, can be empty)
 */
unsigned int ed25519_verify(ed25519_mode_e mode, const unsigned char pubkey[32], const unsigned char *ctx, unsigned char ctx_len, const unsigned char *M, unsigned int m_len,
                            const unsigned char RS[64])
{
    unsigned int k[Ed25519_WORD_LEN << 1];
    unsigned int S[Ed25519_WORD_LEN];
    unsigned int PH_M[Ed25519_WORD_LEN << 1];

    unsigned int pub_x[Ed25519_WORD_LEN], *pub_y = S;
    unsigned int *x = PH_M, *y = &PH_M[Ed25519_WORD_LEN];

    hash_ctx_t sha512_ctx_t[1];
    unsigned int ret;
    unsigned char phflag;
    unsigned char ctx_bytelen = ctx_len;
    unsigned int m_bytelen = m_len;

    if (mode > Ed25519_PH_WITH_PH_M)
    {
        return EdDSA_INVALID_INPUT;
    }
    else if ((NULL == pubkey) || (NULL == RS))
    {
        return EdDSA_POINTER_NULL;
    }
    else
    {
        // handle other
    }

    // if mode is not Ed25519_PH_WITH_PH_M, M could be empty,
    // so M could be NUll, m_bytelen could be 0, no need to check them
    if (Ed25519_PH_WITH_PH_M != mode)
    {
        if (NULL == M)
        {
            m_bytelen = 0u;
        }
        else
        {
        }
    }
    else
    {
        if (NULL == M)
        {
            return EdDSA_INVALID_INPUT;
        }
        else
        {
            m_bytelen = 64u;
        }
    }

    if (Ed25519_CTX == mode) // in this case ctx can not be empty
    {
        if ((NULL == ctx) || (((unsigned char)0) == ctx_bytelen))
        {
            return EdDSA_INVALID_INPUT;
        }
        else
        {
        }
    }
    else if ((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode)) // in this case ctx could be empty
    {
        if (NULL == ctx)
        {
            ctx_bytelen = 0;
        }
        else
        {
        }
    }
    else // Ed25519_DEFAULT mode, ctx is useless
    {
    }

    // get S (S should be less than order of the base point)
    memcpy_((unsigned char *)S, &RS[Ed25519_BYTE_LEN], Ed25519_BYTE_LEN);
    if (uint32_big_num_cmp(S, Ed25519_WORD_LEN, ed25519->n, Ed25519_WORD_LEN) >= 0)
    {
        return EdDSA_INVALID_INPUT;
    }
    else
    {
    }

    /************************* set flag F **************************/
    if (Ed25519_CTX == mode)
    {
        phflag = 0;
    }
    else if ((Ed25519_PH == mode) || (Ed25519_PH_WITH_PH_M == mode))
    {
        phflag = 1;
    }
    else
    {
        // handle other
    }

    // PH_M
    if (Ed25519_PH == mode)
    {
        ret = hash(HASH_SHA512, M, m_bytelen, (unsigned char *)PH_M);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }
    else
    {
    }

    /******* get k = SHA-512(dom2(F, C) || R || A || PH(M)) ********/
    ret = hash_init(sha512_ctx_t, HASH_SHA512);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // dom2(phflag, ctx)
    if (Ed25519_DEFAULT != mode)
    {
        ret = hash_update(sha512_ctx_t, Ed25519_sign_string, sizeof(Ed25519_sign_string));
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = hash_update(sha512_ctx_t, (unsigned char *)&phflag, 1);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = hash_update(sha512_ctx_t, (unsigned char *)&ctx_bytelen, 1);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }

        ret = hash_update(sha512_ctx_t, ctx, ctx_bytelen);
        if (HASH_SUCCESS != ret)
        {
            return ret;
        }
        else
        {
        }
    }
    else
    {
    }

    // R
    ret = hash_update(sha512_ctx_t, RS, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // pubkey(A)
    ret = hash_update(sha512_ctx_t, pubkey, Ed25519_BYTE_LEN);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // PH(M)
    if (Ed25519_PH == mode)
    {
        ret = hash_update(sha512_ctx_t, (unsigned char *)PH_M, 64);
    }
    else
    {
        ret = hash_update(sha512_ctx_t, M, m_bytelen);
    }
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = hash_final(sha512_ctx_t, (unsigned char *)k);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

// k = k mod n
#if defined(PKE_LP)
    ret = pke_mod(&(k[Ed25519_WORD_LEN - 1u]), Ed25519_WORD_LEN + 1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, x);
#else
    ret = pke_mod(&(k[Ed25519_WORD_LEN - 1u]), Ed25519_WORD_LEN + 1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, x);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    uint32_copy(&(k[Ed25519_WORD_LEN - 1u]), x, Ed25519_WORD_LEN);
#if defined(PKE_LP)
    ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1u, ed25519->n, ed25519->n_h, ed25519->n_n0, Ed25519_WORD_LEN, x);
#else
    ret = pke_mod(k, (Ed25519_WORD_LEN << 1) - 1u, ed25519->n, ed25519->n_h, Ed25519_WORD_LEN, x);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        uint32_copy(k, x, Ed25519_WORD_LEN);
    }

    // get [S]B
    ret = ed25519_pointMul_s(ed25519, S, ed25519->Gx, ed25519->Gy, x, y);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get [k]A'
    ret = ed25519_decode_point(pubkey, (unsigned char *)pub_x, (unsigned char *)pub_y);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    ret = ed25519_pointMul_s(ed25519, k, pub_x, pub_y, pub_x, pub_y);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // get R
    ret = ed25519_decode_point(RS, (unsigned char *)k, (unsigned char *)(&k[Ed25519_WORD_LEN]));
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // R + [k]A
    ret = ed25519_pointAdd(ed25519, k, &k[Ed25519_WORD_LEN], pub_x, pub_y, k, &k[Ed25519_WORD_LEN]);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
    }

    // check whether [S]B = R + [k]A
    if ((int32_t)0 != uint32_big_num_cmp(k, Ed25519_WORD_LEN, x, Ed25519_WORD_LEN))
    {
        return EdDSA_VERIFY_FAIL;
    }
    else if ((int32_t)0 != uint32_big_num_cmp(&k[Ed25519_WORD_LEN], Ed25519_WORD_LEN, y, Ed25519_WORD_LEN))
    {
        return EdDSA_VERIFY_FAIL;
    }
    else
    {
        return EdDSA_SUCCESS;
    }
}
#endif