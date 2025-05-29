/*! @file sm2_basic.c */
#include "lib/include/pke/pke_config.h"

#ifdef SUPPORT_SM2

#include "../../crypto_include/crypto_common/utility.h"
#include "../../crypto_include/pke/sm2.h"
#include "../../crypto_include/trng/trng.h"
#ifdef PKE_SEC
#include "../../crypto_include/crypto_common/utility_sec.h"
#endif

extern const unsigned char g_sm2_default_id[16];

#define SM2_DEFAULT_ID_BYTE_LEN (16u)
const unsigned char g_sm2_default_id[16] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38};

/**
 * @brief           get SM2 Z value = SM3(bitLenofID||ID||a||b||Gx||Gy||px||Py).
 * @param[in]       ID                   - User ID
 * @param[in]       byteLenofID          - byte length of ID, must be less than 2^13
 * @param[in]       pubKey               - public key(0x04 + x + y), 65 bytes, big-endian
 * @param[out]      Z                    - Z value, SM3 digest, 32 bytes.
 * @return          0:success     other:error
 * @note
 *        1.bit length of ID must be less than 2^16, thus byte length must be less than 2^13
 *        2.if ID is NULL, then replace it with sm2 default ID
 *        3.please make sure the pubKey is valid
 */
unsigned int sm2_getZ(const unsigned char *ID, unsigned int byteLenofID, const unsigned char pubKey[65], unsigned char Z[32])
{
    unsigned int tmp[SM2_WORD_LEN];
    unsigned int tmp2[SM2_WORD_LEN];
    hash_ctx_t ctx[1];
    unsigned int ret;
    unsigned char tmp_u8;

    if ((NULL == pubKey) || (NULL == Z))
    {
        return SM2_BUFFER_NULL;
    }
    else if (POINT_UNCOMPRESSED != pubKey[0])
    {
        return SM2_INPUT_INVALID;
    }
    else if (byteLenofID > SM2_MAX_ID_BYTE_LEN)
    {
        return SM2_INPUT_INVALID;
    }
    else if ((NULL == ID) || (0u == byteLenofID))
    {
        ID = g_sm2_default_id;
        byteLenofID = SM2_DEFAULT_ID_BYTE_LEN;
    }
    else
    {
        // handle other
    }

#ifdef PKE_BIG_ENDIAN
    reverse_word_array(&pubKey[1u], tmp, SM2_WORD_LEN);
    reverse_word_array(&pubKey[1u + SM2_BYTE_LEN], tmp2, SM2_WORD_LEN);
#else
    reverse_byte_array(&pubKey[1u], (unsigned char *)(tmp), SM2_BYTE_LEN);
    reverse_byte_array(&pubKey[1u + SM2_BYTE_LEN], (unsigned char *)(tmp2), SM2_BYTE_LEN);
#endif

    ret = eccp_pointverify(sm2_curve, tmp, tmp2);
    if (PKE_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_init(ctx, HASH_SM3);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    byteLenofID <<= 3;
    tmp_u8 = (unsigned char)((byteLenofID >> 8u) & 0xFFu);
    ret = hash_update(ctx, (unsigned char *)&tmp_u8, 1);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    tmp_u8 = (unsigned char)(byteLenofID & 0xFFu);
    ret = hash_update(ctx, (unsigned char *)&tmp_u8, 1);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    byteLenofID >>= 3;
    ret = hash_update(ctx, ID, byteLenofID);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_a, tmp, SM2_WORD_LEN);
#else
    reverse_byte_array((const unsigned char *)sm2_curve->eccp_a, (unsigned char *)tmp, SM2_BYTE_LEN);
#endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_b, tmp, SM2_WORD_LEN);
#else
    reverse_byte_array((const unsigned char *)sm2_curve->eccp_b, (unsigned char *)tmp, SM2_BYTE_LEN);
#endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_Gx, tmp, SM2_WORD_LEN);
#else
    reverse_byte_array((const unsigned char *)sm2_curve->eccp_Gx, (unsigned char *)tmp, SM2_BYTE_LEN);
#endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)sm2p256v1_Gy, tmp, SM2_WORD_LEN);
#else
    reverse_byte_array((const unsigned char *)sm2_curve->eccp_Gy, (unsigned char *)tmp, SM2_BYTE_LEN);
#endif

    ret = hash_update(ctx, (unsigned char *)tmp, SM2_BYTE_LEN);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx, &pubKey[1], SM2_BYTE_LEN << 1);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_final(ctx, Z);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

/**
 * @brief           get SM2 E value = SM3(Z||M) (one-off style)
 * @param[in]       M                    - Message
 * @param[in]       byteLen              - byte length of M
 * @param[in]       Z                    - Z value, 32 bytes
 * @param[out]      E                    - E value, 32 bytes
 * @return          0:success     other:error
 */
unsigned int sm2_getE(const unsigned char *M, unsigned int byteLen, const unsigned char Z[32], unsigned char E[32])
{
    hash_ctx_t ctx[1];
    unsigned int ret;

    if ((NULL == M) || (NULL == Z) || (NULL == E))
    {
        return SM2_BUFFER_NULL;
    }
    else if (0u == byteLen)
    {
        return SM2_INPUT_INVALID;
    }
    else
    {
        // handle other
    }

    ret = hash_init(ctx, HASH_SM3);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx, Z, 32u);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx, M, byteLen);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_final(ctx, E);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

#ifdef SM2_GETE_BY_STEPS
/**
 * @brief           Step 1 of getting SM2 E value (stepwise style) - initialization
 * @param[in]       ctx                  - Hash context structure pointer
 * @param[in]       Z                    - Z value, 32 bytes
 * @return          SM2_SUCCESS on success, other values indicate error
 */
unsigned int sm2_getE_init(hash_ctx_t *ctx, const unsigned char Z[32])
{
    unsigned int ret;

    ret = hash_init(ctx, HASH_SM3);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = hash_update(ctx, Z, 32u);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

/**
 * @brief           Step 2 of getting SM2 E value (stepwise style) - update message
 * @param[in]       ctx                  - Hash context structure pointer
 * @param[in]       msg                  - Message to be hashed
 * @param[in]       msg_len              - Byte length of the message
 * @return          SM2_SUCCESS on success, other values indicate error
 * @note            Ensure all parameters are valid and ctx is initialized
 */
unsigned int sm2_getE_update(hash_ctx_t *ctx, const unsigned char *msg, unsigned int msg_len)
{
    unsigned int ret;

    ret = hash_update(ctx, msg, msg_len);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}

/**
 * @brief           Step 3 of getting SM2 E value (stepwise style) - finalize and get digest
 * @param[in]       ctx                  - Hash context structure pointer (must be valid and initialized)
 * @param[out]      E                    - Output hash digest (SM2 E value)
 * @return          SM2_SUCCESS on success, other values indicate error
 * @note            Ensure the digest buffer E is sufficient (32 bytes)
 */
unsigned int sm2_getE_final(hash_ctx_t *ctx, unsigned char E[32])
{
    unsigned int ret;

    ret = hash_final(ctx, E);
    if (HASH_SUCCESS != ret)
    {
        goto END;
    }
    else
    {
    }

    ret = SM2_SUCCESS;

END:

    return ret;
}
#endif

/**
 * @brief           Generate SM2 public key from private key
 * @param[in]       priKey               - private key, 32 bytes, big-endian
 * @param[out]      pubKey               - public key(0x04 + x + y), 65 bytes, big-endian
 * @return          0:success     other:error
 */
unsigned int sm2_get_pubkey_from_prikey(const unsigned char priKey[32], unsigned char pubKey[65])
{
    unsigned int ret;

    if ((NULL == priKey) || (NULL == pubKey))
    {
        return SM2_BUFFER_NULL;
    }
    else
    {
    }

    ret = eccp_get_pubkey_from_prikey(sm2_curve, priKey, &pubKey[1]);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pubKey[0] = POINT_UNCOMPRESSED;

        return SM2_SUCCESS;
    }
}

/**
 * @brief           Generate SM2 random Key pair
 * @param[in]       priKey               - private key, 32 bytes, big-endian
 * @param[out]      pubKey               - public key(0x04 + x + y), 65 bytes, big-endian
 * @return          0:success     other:error
 */
unsigned int sm2_getkey(unsigned char priKey[32], unsigned char pubKey[65])
{
    unsigned int ret;

#if 1
    if ((NULL == priKey) || (NULL == pubKey))
    {
        return SM2_BUFFER_NULL;
    }
    else
    {
    }

    ret = eccp_getkey(sm2_curve, priKey, &pubKey[1]);
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        pubKey[0] = POINT_UNCOMPRESSED;

        return SM2_SUCCESS;
    }
#else

    unsigned int k[SM2_WORD_LEN], tmp[SM2_WORD_LEN << 1];

    if (NULL == priKey || NULL == pubKey)
    {
        return SM2_BUFFER_NULL;
    }
    else
    {
        ;
    }

SM2_GETKEY_LOOP:

    ret = get_rand((unsigned char *)k, SM2_BYTE_LEN);
    if (TRNG_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    // make sure priKey in [1, n-2]
    if (uint32_bignum_check_zero(k, SM2_WORD_LEN))
    {
        goto SM2_GETKEY_LOOP;
    }
    else if (uint32_big_num_cmp(k, SM2_WORD_LEN, (unsigned int *)sm2p256v1_n_1, SM2_WORD_LEN) >= 0)
    {
        goto SM2_GETKEY_LOOP;
    }
    else
    {
        ;
    }

#ifdef SM2_HIGH_SPEED
    ret = eccp_pointMul_base((eccp_curve_t *)sm2_curve, k, tmp, tmp + SM2_WORD_LEN);
#else
    ret = eccp_pointmul((eccp_curve_t *)sm2_curve, k, sm2_curve->eccp_Gx, sm2_curve->eccp_Gy, tmp, tmp + SM2_WORD_LEN);
#endif
    if (PKE_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    pubKey[0] = POINT_UNCOMPRESSED;
#ifdef PKE_BIG_ENDIAN
    reverse_word_array((unsigned char *)k, k, SM2_WORD_LEN);
    memcpy_(priKey, k, SM2_BYTE_LEN);
    reverse_word_array((unsigned char *)tmp, k, SM2_WORD_LEN);
    memcpy_(pubKey + 1, k, SM2_BYTE_LEN);
    reverse_word_array((unsigned char *)(tmp + SM2_WORD_LEN), k, SM2_WORD_LEN);
    memcpy_(pubKey + 1 + SM2_BYTE_LEN, k, SM2_BYTE_LEN);
#else
    reverse_byte_array((unsigned char *)k, priKey, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)tmp, pubKey + 1u, SM2_BYTE_LEN);
    reverse_byte_array((unsigned char *)(tmp + SM2_WORD_LEN), pubKey + 1u + SM2_BYTE_LEN, SM2_BYTE_LEN);
#endif
    return SM2_SUCCESS;
#endif
}

#endif
