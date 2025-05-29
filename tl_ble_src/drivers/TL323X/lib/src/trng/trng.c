/********************************************************************************************************
 * @file    trng.c
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
#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"
#include "lib/include/trng/trng_basic.h"
#include "driver.h"

#ifdef TRNG_RO_ENTROPY
/**
 * @brief           get rand(for internal test)
 * @param[in]       rand                 - byte buffer rand
 * @param[in]       bytes                - byte length of rand
 * @return          TRNG_SUCCESS(success), other(error)
 */
unsigned int get_rand_internal(uint8_t *rand, unsigned int bytes)
{
    return get_rand_buffer(rand, bytes, get_rand_uint32);
}

/**
 * @brief           get rand with post processing
 * @param[in]       rand                 - byte buffer rand
 * @param[in]       bytes                - byte length of rand
 * @param[in]       get_rand_words       - function pointer to get some random words (at most 8 words)
 * @return          TRNG_SUCCESS(success), other(error)
*/
unsigned int get_rand_with_post_processing(uint8_t *rand, unsigned int bytes, GET_RAND_WORDS get_rand_words)
{
    MEM_VOLATILE unsigned int flag     = 0;
    volatile unsigned int     errorCnt = TRNG_ERROR_COUNTER_THRESHOLD;
    unsigned int              ret      = TRNG_ERROR;

    //with post-processing
    if (flag == rTRNG_MSEL) {
        trng_disable();
        trng_set_mode(1);
        trng_enable();
    } else {
        ;
    }

    while (0U != (errorCnt--)) {
        ret = get_rand_buffer(rand, bytes, get_rand_words);
        if ((TRNG_HT_ERROR == ret) || (TRNG_TIMEOUT_ERROR == ret)) {
            continue;
        } else {
            break;
        }
    }

    return ret;
}

/**
 * @brief           Get random data with fast speed (with entropy reducing, suitable for clearing temporary buffers).
 * @param[in]       rand                 - Byte buffer to store the random data.
 * @param[in]       bytes                - Byte length of the rand buffer.
 * @return          TRNG_SUCCESS on success, other error codes on failure.
 */
unsigned int get_rand_fast(uint8_t *rand, unsigned int bytes)
{
#ifdef CONFIG_TRNG_GENERATE_BY_HARDWARE
    return get_rand_with_post_processing(rand, bytes, get_rand_uint32_without_reseed);
#else
    //return get_rand(rand, bytes);
    volatile unsigned int i;

    for (i = 0; i < 2U; i++) {
        memset_(rand, 0, bytes);
    }

    return TRNG_SUCCESS;
#endif
}


#ifdef CONFIG_TRNG_GENERATE_BY_HARDWARE
/**
 * @brief           get rand(without entropy reducing)
 * @param[in]       rand                 - byte buffer rand
 * @param[in]       bytes                - byte length of rand
 * @return          TRNG_SUCCESS(success), other(error)
 */
unsigned int get_rand(uint8_t *rand, unsigned int bytes)
{
    return get_rand_with_post_processing(rand, bytes, get_rand_uint32_with_reseed);
}
#else
#endif

#endif
