/********************************************************************************************************
 * @file    trng_basic.c
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
#include "lib/include/trng/trng_basic.h"
#include "lib/include/crypto_common/utility.h"

/**
 * @brief   This function serves to get trng IP version.
 * @return  trng IP version(hardware version)
 */
unsigned int trng_get_version(void)
{
    return rTRNG_VERSION;
}

/**
 * @brief   This function serves to get trng driver version.
 * @return  trng driver version(software version)
 */
unsigned int trng_get_driver_version(void)
{
    //the meaning of the version(for example, if the return value is 0x23080301)
    //the first 3 bytes:  23.08.03 ---- date
    //the last byte:      01       ---- first version on the day
    return (0x23U << 24U) | (0x08U << 16U) | (0x03U << 8U) | 0x01U;
}

/**
 * @brief   TRNG global interruption enable.
 * @return  none
 */
void trng_global_int_enable(void)
{
    MEM_VOLATILE unsigned int flag = (((unsigned int)1U) << TRNG_GLOBAL_INT_OFFSET);

    rTRNG_CR |= flag;
}

/**
 * @brief   TRNG global interruption disable
 * @return  none
 */
void trng_global_int_disable(void)
{
    MEM_VOLATILE unsigned int mask = ~(((unsigned int)1U) << TRNG_GLOBAL_INT_OFFSET);

    rTRNG_CR &= mask;
}

/**
 * @brief       TRNG empty-read interruption enable.
 * @return      none
 * @note
  @verbatim
      -# 1. works when global interruption is enabled.
  @endverbatim
 */
void trng_empty_read_int_enable(void)
{
    MEM_VOLATILE unsigned int flag = (((unsigned int)1U) << TRNG_READ_EMPTY_INT_OFFSET);

    rTRNG_CR |= flag;
}

/**
 * @brief       TRNG global interruption disable
 * @return      none
 */
void trng_empty_read_int_disable(void)
{
    MEM_VOLATILE unsigned int mask = ~(((unsigned int)1U) << TRNG_READ_EMPTY_INT_OFFSET);

    rTRNG_CR &= mask;
}

/**
 * @brief       TRNG data interruption enable.
 * @return      none
 * @note
  @verbatim
      -# 1. works when global interruption is enabled.
  @endverbatim
 */
void trng_data_int_enable(void)
{
    MEM_VOLATILE unsigned int flag = (((unsigned int)1U) << TRNG_DATA_INT_OFFSET);

    rTRNG_CR |= flag;
}

/**
 * @brief       TRNG data interruption disable
 * @return      none
 */
void trng_data_int_disable(void)
{
    MEM_VOLATILE unsigned int mask = ~(((unsigned int)1U) << TRNG_DATA_INT_OFFSET);

    rTRNG_CR &= mask;
}

/**
 * @brief       TRNG enable
 * @return      none
 */
void trng_enable(void)
{
    MEM_VOLATILE unsigned int flag = 1U;

    rTRNG_CR |= flag;
}

/**
 * @brief       TRNG enable
 * @return      none
 */
void trng_disable(void)
{
    MEM_VOLATILE unsigned int mask = ~((unsigned int)1U);

    rTRNG_CR &= mask;

    //sleep for a while until the entropy is stable before enabling it.
    uint32_sleep(TRNG_DELAY_COUNTER, 0);
}


#ifdef TRNG_RO_ENTROPY
/**
 * @brief: check if TRNG self test ready.
 * parameters:
 * return: 1(ready), 0(not ready)
 * caution:
 *     1. this works while i_skip_startup is 0. if i_skip_startup is 1, no 
 *        need to check this.
 */
unsigned int trng_if_self_test_ready(void)
{
    MEM_VOLATILE unsigned int flag = (((unsigned int)1U) << TRNG_SELF_TEST_READY_OFFSET);

    if (0 != (rTRNG_SR & flag)) {
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief       set RO entropy config
 * @param[in]   cfg       - RO entropy config, only the low 4 bits are valid, every bit,indicates one RO entropy, the MSB is RO 4, and LSB is RO 1.
 * @return      TRNG_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. only the low 4 bits of cfg are valid.
      -# 2. if the low 4 bits of cig is 0, that means to disable all RO entropy.
      -# 3. Turning on 4 ro entropy sources will have better randomization performance,
            but as the number of entropy sources turned on increases, the power consumption
            of the trng module will also increase..
  @endverbatim
 */
unsigned int trng_ro_entropy_config(unsigned char cfg)
{
    MEM_VOLATILE unsigned int mask = ~(0x0000000FU);

    if (cfg > 15U) {
        return TRNG_INVALID_INPUT;
    } else {
        ;
    }

    rRO_CLK_EN = (rRO_CLK_EN & mask) | ((unsigned int)cfg);

    return TRNG_SUCCESS;
}

/**
 * @brief       set sub RO entropy config
 * @param[in]   sn        - RO entropy source series number, must be in [1,4].
 * @param[in]   cfg       - the config value of RO sn.
 * @return      TRNG_SUCCESS(success), other(error)
 */
unsigned int trng_ro_sub_entropy_config(unsigned char sn, unsigned short cfg)
{
    MEM_VOLATILE unsigned int mask_high = ~0xFFFF0000U;
    MEM_VOLATILE unsigned int mask_low  = ~0x0000FFFFU;
    unsigned int              ret       = TRNG_SUCCESS;

    switch (sn) {
    case 1:
        rRO_SRC_EN1 = (rRO_SRC_EN1 & mask_high) | (((unsigned int)cfg) << 16);
        break;

    case 2:
        rRO_SRC_EN1 = (rRO_SRC_EN1 & mask_low) | ((unsigned int)cfg);
        break;

    case 3:
        rRO_SRC_EN2 = (rRO_SRC_EN2 & mask_high) | (((unsigned int)cfg) << 16);
        break;

    case 4:
        rRO_SRC_EN2 = (rRO_SRC_EN2 & mask_low) | ((unsigned int)cfg);
        break;

    default:
        ret = TRNG_INVALID_INPUT;
        break;
    }

    return ret;
}

/**
 * @brief       set TRNG mode
 * @param[in]   with_post_processing       - 0:no,  other:yes
 * @return      none
 * @note
  @verbatim
      -# 1. True random mode when set to 0, pseudo-random mode when set to a non-zero value.
  @endverbatim
 */
void trng_set_mode(unsigned char with_post_processing)
{
    MEM_VOLATILE unsigned int mask       = ~((unsigned int)1U);
    MEM_VOLATILE unsigned int flag       = 1U;
    MEM_VOLATILE unsigned int clear_flag = 0x00000007U;

    if ((unsigned char)0 != with_post_processing) {
        rTRNG_MSEL |= flag;
    } else {
        rTRNG_MSEL &= mask;
    }

    rTRNG_SR |= clear_flag; //write 1 to clear
}

/**
 * @brief       reseed TRNG(works when DRBG is enabled)
 * @return      none
 * @note
  @verbatim
      -# 1. used for DRBG
  @endverbatim
 */
void trng_reseed(void)
{
    MEM_VOLATILE unsigned int flag       = 1U;
    MEM_VOLATILE unsigned int clear_flag = 0x00000007U;

    rTRNG_RESEED |= flag;

    rTRNG_SR |= clear_flag; //write 1 to clear
}

/**
 * @brief       TRNG set frequency
 * @param[in]   freq       frequency config, must be in [0,3], and
 *                                  0: 1/4 of input frequency, the lower the sample frequency, the better the randomization performance,
 *                                  1: 1/8 ...,
 *                                  2: 1/16 ...,
 *                                  3: 1/32 ...,
 * @return      TRNG_SUCCESS(success), other(error)
 */
unsigned int trng_set_freq(unsigned char freq)
{
    MEM_VOLATILE unsigned int mask = ~(((unsigned int)0x00000003U) << TRNG_FREQ_OFFSET);

    if (freq > 3U) {
        return TRNG_INVALID_INPUT;
    } else {
        ;
    }

    rRO_CLK_EN = (rRO_CLK_EN & mask) | (((unsigned int)freq) << TRNG_FREQ_OFFSET);

    return TRNG_SUCCESS;
}

/**
 * @brief       get some rand words
 * @param[in]   a       random words
 * @param[in]   words   word number of output, must be in [1, 8]
 * @return      TRNG_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. used for DRBG
  @endverbatim
 */
unsigned int get_rand_uint32(unsigned int *a, unsigned int words)
{
    MEM_VOLATILE unsigned int DT_ready_flag = 2U;
    MEM_VOLATILE unsigned int HT_error_flag = 1U;
    unsigned int              i;

    while (0U == (rTRNG_SR & DT_ready_flag)) {
        if (0U != (rTRNG_SR & HT_error_flag)) {
            return TRNG_HT_ERROR;
        } else {
            ;
        }
    }

    for (i = 0; i < words; i++) {
        *(a++) = rTRNG_DR;     //printf("\r\n %08x", *(a-1));
    }

    rTRNG_SR |= DT_ready_flag; //clear

    //if now HT error
    if (0U != (rTRNG_SR & HT_error_flag)) {
        return TRNG_HT_ERROR;
    } else {
        ;
    }

    return TRNG_SUCCESS;
}

/**
 * @brief: get some rand words(with post-processing, but without reseed)
 * parameters:
 *     a -------------------------- output, random words
 *     words ---------------------- input, word number of output, must be in [1, 8]
 * return: TRNG_SUCCESS(success), other(error)
 * caution:
 *     1. please make sure the two parameters are valid
 */
unsigned int get_rand_uint32_without_reseed(unsigned int *a, unsigned int words)
{
    MEM_VOLATILE unsigned int DT_ready_flag = 2U;
    MEM_VOLATILE unsigned int HT_error_flag = 1U;
    MEM_VOLATILE unsigned int clear_flag    = 7U;
    volatile unsigned int     cnt           = 0;
    unsigned int              i;

    while (0U == (rTRNG_SR & DT_ready_flag)) {
        if (0U != (rTRNG_SR & HT_error_flag)) {
            trng_disable();
            rTRNG_SR |= clear_flag; //clear (alarm) status
            trng_enable();

            return TRNG_HT_ERROR;
        } else {
            cnt++;
            if (cnt > TRNG_TIMEOUT_COUNTER_THRESHOLD) {
                return TRNG_TIMEOUT_ERROR;
            } else {
                ;
            }
        }
    }

    for (i = 0; i < words; i++) {
        *(a++) = rTRNG_DR;     //printf("\r\n %08x", *(a-1));
    }

    rTRNG_SR |= DT_ready_flag; //clear

    //if now HT error
    if (0U != (rTRNG_SR & HT_error_flag)) {
        trng_disable();
        rTRNG_SR |= clear_flag; //clear (alarm) status
        trng_enable();

        return TRNG_HT_ERROR;
    } else {
        ;
    }

    return TRNG_SUCCESS;
}

/**
 * @brief       get some rand words(with reseed)
 * @param[in]   a       random words
 * @param[in]   words   word number of output, must be in [1, 8]
 * @return      TRNG_SUCCESS(success), other(error)
* @note
  @verbatim
      -# 1. please make sure the two parameters are valid
  @endverbatim
 */
unsigned int get_rand_uint32_with_reseed(unsigned int *a, unsigned int words)
{
    unsigned int ret = get_rand_uint32_without_reseed(a, words);

    if (TRNG_SUCCESS == ret) {
        trng_reseed(); //for next generation.
    } else {
        ;
    };

    return ret;
}

/**
 * @brief       get rand buffer(internal basis interface)
 * @param[in]   rand                byte buffer rand
 * @param[in]   bytes               byte length of rand
 * @param[in]   get_rand_words      function pointer to get some random words(at most 8 words)
 * @return      TRNG_SUCCESS(success), other(error)
 */
unsigned int get_rand_buffer(unsigned char *rand, unsigned int bytes, GET_RAND_WORDS get_rand_words)
{
    MEM_VOLATILE unsigned int enable_flag     = 1U;
    MEM_VOLATILE unsigned int ro_entropy_mask = (0x0000000FU);
    unsigned int              i;
    unsigned int              tmp, tmp_len, rng_data;
    unsigned int              count, ret;
    unsigned char            *a = rand;

    //check input parameters
    if (NULL == rand || NULL == get_rand_words) {
        return TRNG_BUFFER_NULL;
    } else if (0U == bytes) {
        return TRNG_SUCCESS;
    } else {
        //handle other;
    }

    //make sure trng and ro are enabled
    if (0U == (rTRNG_CR & enable_flag)) {
        return TRNG_INVALID_CONFIG;
    } else if (0U == (rRO_CLK_EN & ro_entropy_mask)) {
        return TRNG_INVALID_CONFIG;
    } else {
        //handle other;
    }

    tmp_len = bytes;

    tmp = ((unsigned int)a) & 3U;
    if (0U != tmp) {
        i = 4U - tmp;

        ret = get_rand_words(&rng_data, 1);
        if (TRNG_SUCCESS != ret) {
            goto END;
        } else {
            if (tmp_len > i) {
                memcpy_(a, (unsigned char *)(&rng_data), i);
                a = &a[i];
                tmp_len -= i;
            } else {
                memcpy_(a, (unsigned char *)(&rng_data), tmp_len);
                goto END;
            }
        }
    } else {
        ;
    }

    tmp = tmp_len / 4U;
    while (0U != tmp) {
        if (tmp > 8U) {
            count = 8U;
        } else {
            count = tmp;
        }

        ret = get_rand_words((unsigned int *)a, count);
        if (TRNG_SUCCESS != ret) {
            goto END;
        } else {
            a = &(a[count << 2]);
            tmp -= count;
        }
    }

    tmp_len = tmp_len & 3U;
    if (0U != tmp_len) {
        ret = get_rand_words(&rng_data, 1);
        if (TRNG_SUCCESS != ret) {
            goto END;
        } else {
            memcpy_(a, (unsigned char *)(&rng_data), tmp_len);
        }
    }

    ret = TRNG_SUCCESS;

END:

    if (TRNG_SUCCESS != ret) {
        memset_(rand, 0, bytes);
    } else {
        ;
    }

    #ifdef TRNG_POKER_TEST
    if (TRNG_SUCCESS == ret) {
        poker_test(rand, bytes);
    }
    #endif

    return ret;
}

#endif
