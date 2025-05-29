/********************************************************************************************************
 * @file    trng.c
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
//#include <stdio.h>

#include "lib/include/crypto_common/utility.h"
#include "lib/include/trng/trng.h"
#include "lib/include/trng/trng_basic.h"


#ifdef TRNG_RO_ENTROPY
/**
 * @brief       get rand(for internal test)
 * @param[in]   rand                byte buffer rand
 * @param[in]   bytes               byte length of rand
 * @return      TRNG_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. Directly from the entropy source to take the random number,
            depending on the current configuration, the power-up default true random number mode,
            if you get the true random number and the current mode is configured as a post-processing, you can use trng_disable, trng_set_mode, trng_enable interface function to set to the true random mode, and then call the function,
            if you get the pseudo-random please use get_rand.
  @endverbatim
 */
unsigned int get_rand_internal(unsigned char *rand, unsigned int bytes)
{
    return get_rand_buffer(rand, bytes, get_rand_uint32);
}

/**
 * @brief       get rand with post processing
 * @param[in]   rand                byte buffer rand
 * @param[in]   bytes               byte length of rand
 * @param[in]   get_rand_words      function pointer to get some random words(at most 8 words)
 * @return      TRNG_SUCCESS(success), other(error)
 */
unsigned int get_rand_with_post_processing(unsigned char *rand, unsigned int bytes, GET_RAND_WORDS get_rand_words)
{
    MEM_VOLATILE unsigned int flag     = 0;
    volatile unsigned int     errorCnt = TRNG_ERROR_COUNTER_THRESHOLD;
    unsigned int              ret      = TRNG_ERROR;

    //with post-processing
    if (flag == rTRNG_MSEL) {
        trng_disable();
        trng_set_mode(1);
        trng_enable();
        trng_reseed(); //Add an interface to update the random number seed here to prevent the same random number from being obtained after the first power-on.
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
 * @brief       get rand with fast speed(with entropy reducing, for such as clearing tmp buffer)
 * @param[in]   rand                byte buffer rand
 * @param[in]   bytes               byte length of rand
 * @return      TRNG_SUCCESS(success), other(error)
 */
unsigned int get_rand_fast(unsigned char *rand, unsigned int bytes)
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
 * @brief       get rand(without entropy reducing)
 * @param[in]   rand                byte buffer rand
 * @param[in]   bytes               byte length of rand
 * @return      TRNG_SUCCESS(success), other(error)
 * * @note
  @verbatim
      -# 1. This interface uses pseudo-random mode by default, through post-processing, the randomness will be better and easier to pass trng-related authentication
      -# 2. After the call the mode configuration is changed to post-processing mode.
  @endverbatim
 */
unsigned int get_rand(unsigned char *rand, unsigned int bytes)
{
    return get_rand_with_post_processing(rand, bytes, get_rand_uint32_with_reseed);
}
    #else

extern unsigned char SM3_Hash(unsigned char *message, unsigned int byteLen, unsigned char digest[32]);

static unsigned int  seed = 0x23ba78de;
static unsigned char sm3_buf[32];
static unsigned char buf_index = 0;
//trng config flag
static unsigned int trng_cfg_flag = 0;

unsigned int get_rand_register(void)
{
    static unsigned int i = 0;
    unsigned char       buf[32];
    unsigned int        tmp = 0;

    if (0U == trng_cfg_flag) {
        (void)SM3_Hash((unsigned char *)&seed, 4, sm3_buf);

        trng_cfg_flag = 1;
    } else {
        ;
    }

    if (buf_index < 28U) {
        tmp = *((unsigned int *)(sm3_buf + buf_index));
        buf_index += 4U;
    } else if (buf_index == 28U) {
        tmp = *((unsigned int *)(sm3_buf + 28));
        memcpy_(buf, sm3_buf, 32);
        i++;
        *((unsigned int *)(buf + 16)) += 1U;
        (void)SM3_Hash(buf, 32, sm3_buf);
        buf_index = 0;
    } else {
        //handle other;
    }

    return tmp;
}

unsigned int get_rand(unsigned char *rand, unsigned int byteLen)
{
    //unsigned char *rand_bak = rand;
    //unsigned int len_bak = byteLen;
    unsigned int  word_len, result;
    unsigned char left_len = (unsigned char)((unsigned int)rand & 0x3U);

    // if the data addr is not aligned by word
    if ((unsigned char)0 != left_len) {
        // wait the data is ready
        result = get_rand_register();

        if (byteLen > (4U - (unsigned int)left_len)) {
            memcpy_(rand, (unsigned char *)(&result), 4U - (unsigned int)left_len);
            byteLen -= (4U - (unsigned int)left_len);
            rand = &(rand[(4U - left_len)]);
        } else {
            memcpy_(rand, (unsigned char *)(&result), byteLen);
            //trng_disable();
            //print_buf_U8(rand_bak, len_bak, "rand");
            return 0; //TRNG_SUCCESS;
        }
    }

    word_len = byteLen >> 2;
    left_len = (unsigned char)(byteLen & 0x3U);

    // obtain the data by word
    while (0U != word_len--) {
        *((unsigned int *)rand) = get_rand_register();
        rand                    = &(rand[4]);
    }

    // if the byteLen is not aligned by word
    if ((unsigned char)0 != left_len) {
        result = get_rand_register();
        memcpy_(rand, (unsigned char *)(&result), left_len);
    }

    //print_buf_U8(rand_bak, len_bak, "rand");
    return 0; //TRNG_SUCCESS;
}
    #endif

#endif
