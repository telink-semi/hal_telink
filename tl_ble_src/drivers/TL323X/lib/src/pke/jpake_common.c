/*! @file jpake_common.c */
#include "lib/include/crypto_common/utility.h"
#include "lib/include/hash/hash.h"
#include "lib/include/trng/trng.h"
#include "lib/include/trng/trng_basic.h"
/**
 * @brief           J-PAKE hash item (byte) length
 * @param[in]       ctx                  - hash_ctx_t struct pointer
 * @param[in]       msg_len              - message or item byte length
 * @return          0:success     other:error
 * @note
 *        1.ctx must be initialized
 */
unsigned int jpake_hash_length(hash_ctx_t *ctx, unsigned int msg_len)
{
    reverse_byte_array((unsigned char *)&msg_len, (unsigned char *)&msg_len, 4);

    return hash_update(ctx, (const unsigned char *)&msg_len, 4);
}

/**
 * @brief           J-PAKE hash string
 * @param[in]       ctx                  - hash_ctx_t struct pointer
 * @param[in]       s                    - byte string
 * @param[in]       byteLen              - byte length of s
 * @return          0:success     other:error
 */
unsigned int jpake_hash_string(hash_ctx_t *ctx, unsigned char *s, unsigned int byteLen)
{
    unsigned int ret;

    ret = jpake_hash_length(ctx, byteLen);
    if (HASH_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    return hash_update(ctx, (const unsigned char *)s, byteLen);
}

/**
 * @brief           J-PAKE get rand xa less than q
 * @param[in]       q                    - big number q
 * @param[out]      xa                   - random big
 *                                         number less than q
 * @param[in]       wlen                 - word length of q and
 *                                         xa
 * @param[in]       remainder_bits       - real bit length of q mod 32.
 * @param[in]       could_be_zero        - could xa be zero, 0(xa can
 *                                         not be zero), other(xa can be zero).
 * @return          0:success     other:error
 */
unsigned int jpake_get_rand_xa_less_than_q(unsigned int *q, unsigned int *xa, unsigned int wlen, unsigned int remainder_bits, unsigned int could_be_zero)
{
    unsigned int ret;

GET_XA_LOOP:

    ret = get_rand((unsigned char *)xa, wlen << 2);
    if (TRNG_SUCCESS != ret)
    {
        return ret;
    }
    else
    {
        ;
    }

    // make sure xa has the same bit length as q
    if (remainder_bits)
    {
        xa[wlen - 1] &= (1 << (remainder_bits)) - 1;
    }
    else
    {
        ;
    }

    if (uint32_big_num_cmp(xa, wlen, q, wlen) >= 0)
    {
        goto GET_XA_LOOP;
    }
    else
    {
        ;
    }

    if (!could_be_zero)
    {
        if (uint32_bignum_check_zero(xa, wlen))
        {
            goto GET_XA_LOOP;
        }
        else
        {
            ;
        }
    }
    else
    {
        ;
    }

    return 0;
}
