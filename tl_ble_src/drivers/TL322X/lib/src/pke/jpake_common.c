/********************************************************************************************************
 * @file    jpake_common.c
 *
 * @brief   This is the source file for tl322x
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
#include "lib/include/hash/hash.h"
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"

/**
 * @brief       J-PAKE hash item (byte) length
 * @param[in]   ctx                  - HASH_CTX struct pointer
 * @param[in]   msg_bytes            - message or item byte length
 * @return      0:success     other:error
 * @note
  @verbatim
      -# 1.ctx must be initialized
  @endverbatim
 */
unsigned int jpake_hash_length(HASH_CTX *ctx, unsigned int msg_bytes)
{
    reverse_byte_array((unsigned char *)&msg_bytes, (unsigned char *)&msg_bytes, 4);

    return hash_update(ctx, (unsigned char *)&msg_bytes, 4);
}

/**
 * @brief       J-PAKE hash string
 * @param[in]   ctx                  - HASH_CTX struct pointer
 * @param[in]   s                    - byte string
 * @param[in]   byteLen              - byte length of s
 * @return      0:success     other:error
 */
unsigned int jpake_hash_string(HASH_CTX *ctx, unsigned char *s, unsigned int byteLen)
{
    unsigned int ret;

    ret = jpake_hash_length(ctx, byteLen);
    if (HASH_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    return hash_update(ctx, (unsigned char *)s, byteLen);
}

/**
 * @brief       J-PAKE get rand xa less than q
 * @param[in]   q                        - big number q
 * @param[out]  xa                       - random big number less than q
 * @param[in]   wordLen                  - word length of q and xa
 * @param[in]   remainder_bits           - real bit length of q mod 32.
 * @param[in]   could_be_zero            - could xa be zero, 0(xa can not be zero), other(xa can be zero).
 * @return      0:success     other:error
 */
unsigned int jpake_get_rand_xa_less_than_q(unsigned int *q, unsigned int *xa, unsigned int wordLen, unsigned int remainder_bits, unsigned int could_be_zero)
{
    unsigned int ret;

GET_XA_LOOP:

    ret = get_rand((unsigned char *)xa, wordLen << 2);
    if (TRNG_SUCCESS != ret) {
        return ret;
    } else {
        ;
    }

    //make sure xa has the same bit length as q
    if (remainder_bits) {
        xa[wordLen - 1] &= (1 << (remainder_bits)) - 1;
    } else {
        ;
    }

    if (uint32_BigNumCmp(xa, wordLen, q, wordLen) >= 0) {
        goto GET_XA_LOOP;
    } else {
        ;
    }

    if (!could_be_zero) {
        if (uint32_BigNum_Check_Zero(xa, wordLen)) {
            goto GET_XA_LOOP;
        } else {
            ;
        }
    } else {
        ;
    }

    return 0;
}
