/********************************************************************************************************
 * @file    trng.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "driver.h"

typedef unsigned int GET_RAND_WORDS(unsigned int *a, unsigned int words);


/**
 * @brief       TRNG global interruption enable
 * @return      none
 */
void trng_global_int_enable(void)
{
    TRNG_CR |= (1<<TRNG_GLOBAL_INT_OFFSET);
}

/**
 * @brief       TRNG global interruption disable
 * @return      none
 */
void trng_global_int_disable(void)
{
    TRNG_CR &= ~(1<<TRNG_GLOBAL_INT_OFFSET);
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
    TRNG_CR |= (1<<TRNG_READ_EMPTY_INT_OFFSET);
}

/**
 * @brief       TRNG global interruption disable
 * @return      none
 */
void trng_empty_read_int_disable(void)
{
    TRNG_CR &= ~(1<<TRNG_READ_EMPTY_INT_OFFSET);
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
    TRNG_CR |= (1<<TRNG_DATA_INT_OFFSET);
}

/**
 * @brief       TRNG data interruption disable
 * @return      none
 */
void trng_data_int_disable(void)
{
    TRNG_CR &= ~(1<<TRNG_DATA_INT_OFFSET);
}

/**
 * @brief       TRNG enable
 * @return      none
 */
void trng_enable(void)
{
    volatile unsigned int i;

    TRNG_CR |= 1;

    //sleep for a while
    i=0xFFF;
    while(i--)
    {;}
}

/**
 * @brief       TRNG enable
 * @return      none
 */
void trng_disable(void)
{
    TRNG_CR &= (~1);
}

/**
 * @brief       set RO entropy config
 * @param[in]   cfg       - RO entropy config, only the low 4 bits are valid, every bit,indicates one RO entropy, the MSB is RO 0, and LSB is RO 3.
 * @return      TRNG_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. only the low 4 bits of cfg are valid.
      -# 2. if the low 4 bits of cig is 0, that means to disable all RO entropy.
  @endverbatim
 */
unsigned int trng_ro_entropy_config(unsigned char cfg)
{
    if(cfg > 15)
    {
        return TRNG_INVALID_INPUT;
    }
    else
    {;}

    TRNG_CR &= ~(0x0F<<TRNG_RO_ENTROPY_OFFSET);
    TRNG_CR |=  (((unsigned int)(cfg&0x0F))<<TRNG_RO_ENTROPY_OFFSET);

    return TRNG_SUCCESS;
}

/**
 * @brief       set sub RO entropy config
 * @param[in]   sn       - RO entropy source series number, must be in [0,3].
 * @param[in]   cfg       - the config value of RO sn.
 * @return      TRNG_SUCCESS(success), other(error)
 */
unsigned int trng_ro_sub_entropy_config(unsigned char sn, unsigned short cfg)
{
    switch(sn)
    {
        case 0:
        RO_SRC_EN1 &= ~0xFFFF0000;
        RO_SRC_EN1 |= ((unsigned int)cfg)<<16;
        break;

        case 1:
        RO_SRC_EN1 &= ~0x0000FFFF;
        RO_SRC_EN1 |= (unsigned int)cfg;
        break;

        case 2:
        RO_SRC_EN2 &= ~0xFFFF0000;
        RO_SRC_EN2 |= ((unsigned int)cfg)<<16;
        break;

        case 3:
        RO_SRC_EN2 &= ~0x0000FFFF;
        RO_SRC_EN2 |= (unsigned int)cfg;
        break;

        default:
            return TRNG_INVALID_INPUT;
    }

    return TRNG_SUCCESS;
}

/**
 * @brief       set sub RO entropy config
 * @param[in]   with_post_processing       - 0:no,  other:yes
 * @return      none
 * @note
  @verbatim
      -# 1. True random mode when set to 0, pseudo-random mode when set to a non-zero value.
  @endverbatim
 */
void trng_set_mode(unsigned char with_post_processing)
{
    if(with_post_processing)
    {
        TRNG_MSEL |= 1;
    }
    else
    {
        TRNG_MSEL &= (~1);
    }

    TRNG_SR |= 0x07; //write 1 to clear
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
    TRNG_RESEED |= 0x01;

    TRNG_SR |= 0x07; //write 1 to clear
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
    if(freq > 3)
    {
        return TRNG_INVALID_INPUT;
    }
    else
    {;}

    SCLK_FREQ = freq;

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
    unsigned int i;

    while(0 == (TRNG_SR & 2))
    {
        if(TRNG_SR & 1)
        {
            return TRNG_HT_ERROR;
        }
        else
        {;}
    }

    for(i=0; i<words; i++)
    {
        *(a++) = TRNG_DR;  //printf("\r\n %08x", *(a-1));
    }

    TRNG_SR |= 2;  //clear

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
    unsigned int i;

    trng_reseed();

    while(0 == (TRNG_SR & 2))
    {
        if(TRNG_SR & 1)
        {
            return TRNG_HT_ERROR;
        }
        else
        {;}
    }

    for(i=0; i<words; i++)
    {
        *(a++) = TRNG_DR;  //printf("\r\n %08x", *(a-1));
    }

    TRNG_SR |= 2;  //clear

    return TRNG_SUCCESS;
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
    unsigned int i;
    unsigned int tmp, tmp_len, rng_data;
    unsigned int count, ret;
    unsigned char *a = rand;

    //check input parameters
    if(NULL == rand || NULL == get_rand_words)
    {
        return TRNG_BUFFER_NULL;
    }
    else if(0 == bytes)
    {
        return TRNG_SUCCESS;
    }
    else
    {;}

    //make sure trng and ro are enabled
    if(0 == (TRNG_CR & 1))
    {
        return TRNG_INVALID_CONFIG;
    }
    else if(0 == (TRNG_CR & (0x0F<<TRNG_RO_ENTROPY_OFFSET)))
    {
        return TRNG_INVALID_CONFIG;
    }
    else
    {;}

    tmp_len = bytes;

    tmp = ((unsigned int)a) & 3;
    if(tmp)
    {
        i = 4-tmp;

        ret = get_rand_words(&rng_data, 1);
        if(TRNG_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            if(tmp_len > i)
            {
                memcpy_(a, &rng_data, i);
                a += i;
                tmp_len -= i;
            }
            else
            {
                memcpy_(a, &rng_data, tmp_len);
                goto END;
            }
        }
    }
    else
    {;}

    tmp = tmp_len/4;
    while(tmp)
    {
        if(tmp>8)
        {
            count = 8;
        }
        else
        {
            count = tmp;
        }

        ret = get_rand_words((unsigned int *)a, count);
        if(TRNG_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            a += count<<2;
            tmp -= count;
        }
    }

    tmp_len = tmp_len & 3;
    if(tmp_len)
    {
        ret = get_rand_words(&rng_data, 1);
        if(TRNG_SUCCESS != ret)
        {
            goto END;
        }
        else
        {
            memcpy_(a, &rng_data, tmp_len);
        }
    }

    ret = TRNG_SUCCESS;

END:

    if(TRNG_SUCCESS != ret)
    {
        memset_(rand, 0, bytes);
    }
    else
    {;}

#ifdef TRNG_POKER_TEST
    if(TRNG_SUCCESS == ret)
    {
        poker_test(rand, bytes);
    }
#endif

    return ret;
}

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
    //with post-processing
    if(0 == TRNG_MSEL)
    {
        trng_disable();
        trng_set_mode(1);
        trng_enable();
    }
    else
    {;}

    return get_rand_buffer(rand, bytes, get_rand_uint32_with_reseed);
}


/*********************************** TREO ************************************/
/**
 * @brief       TERO RNG enable
 * @return      none
 */
void tero_enable(void)
{
    volatile unsigned int i;

    TERO_CR |= 1;

    //sleep for a while
    i=0xFFF;
    while(i--)
    {;}
}

/**
 * @brief       TERO RNG disable
 * @return      none
 */
void tero_disable(void)
{
    TERO_CR &= ~1;
}

/**
 * @brief       TERO RNG set the system cycle threshold of the TERO counter kept
 * @param[in]   threshold_value                 threshold value
 * @return      none
 */
unsigned int tero_set_stop_threshold(unsigned char threshold_value)
{
    if(0 == threshold_value)
    {
        return TRNG_INVALID_INPUT;
    }
    else
    {;}

    TERO_CR &= ~(0xFF<<TRNG_TERO_THRESHOLD_OFFSET);
    TERO_CR |= (((unsigned int)threshold_value)<<TRNG_TERO_THRESHOLD_OFFSET);

    return TRNG_SUCCESS;
}

/**
 * @brief       set TERO entropy config
 * @param[in]   cfg       random words
 * @return      TRNG_SUCCESS(success), other(error)
 * @note
  @verbatim
      -# 1. please make sure the two parameters are valid
  @endverbatim
 */
unsigned int tero_entropy_config(unsigned char cfg)
{
    if(cfg > 15)
    {
        return TRNG_INVALID_INPUT;
    }
    else
    {;}

    TERO_CR &= ~(0x0F<<TRNG_TERO_ENTROPY_OFFSET);
    TERO_CR |=  (((unsigned int)(cfg&0x0F))<<TRNG_TERO_ENTROPY_OFFSET);

    return TRNG_SUCCESS;
}

/**
 * @brief       TERO RNG set output as rng
 * @return      none
 */
void tero_set_output_rng(void)
{
    TERO_CR &= ~(1<<1);
}

/**
 * @brief       TERO RNG set output as oscillation times
 * @return      none
 */
void tero_set_output_osc_times(void)
{
    TERO_CR |= (1<<1);
}

/**
 * @brief       select TREO 1&2 or TERO 3&4 when output is oscillation times
 * @param[in]   cfg       random words
 * @return      none
 */
void tero_set_osc_sel(unsigned char cfg)
{
    if(0 == cfg)
    {
        TERO_CR &= ~(1<<2);
    }
    else
    {
        TERO_CR |= (1<<2);
    }
}

/**
 * @brief       set lower limit of oscillation times
 * @param[in]   value       lower limit value
 * @return      none
 */
void tero_set_osc_times_lower_limit(unsigned short value)
{
    TERO_THOLD &= ~(0xFF<<16);
    TERO_THOLD |= ((unsigned int)value)<<16;
}

/**
 * @brief       set upper limit of oscillation times
 * @param[in]   value       lower limit value
 * @return      none
 */
void tero_set_osc_times_upper_limit(unsigned short value)
{
    TERO_THOLD &= ~(0xFF);
    TERO_THOLD |= ((unsigned int)value);
}

/**
 * @brief       get tero rand
 * @param[in]   a       byte buffer a
 * @param[in]   bytes   byte length of rand
 * @return      none
 */
unsigned int get_tero_rand(unsigned char *a, unsigned int bytes)
{
    unsigned int i;
    unsigned int tmp, rng_data;
//  unsigned int tmp_len;

    //check input parameters
    if(NULL == a)
    {
        return TRNG_BUFFER_NULL;
    }
    else if(0 == bytes)
    {
        return TRNG_SUCCESS;
    }
    else
    {;}

    //make sure tero config is valid
    if(0 == (TERO_CR & 1))
    {
        return TRNG_INVALID_CONFIG;
    }
    else if(TERO_CR & (1<<1))
    {
        return TRNG_INVALID_CONFIG;
    }
    else if(0 == (TERO_CR & (0x0F<<TRNG_TERO_ENTROPY_OFFSET)))
    {
        return TRNG_INVALID_CONFIG;
    }
    else
    {;}

//  tmp_len = bytes;

    tmp = ((unsigned int)a) & 3;
    if(tmp)
    {
        i = 4-tmp;

        while(0 == (TERO_SR & 1))
        {;}
        rng_data = TERO_DR;//printf("\r\n %08x", rng_data);
        if(bytes > i)
        {
            memcpy_(a, &rng_data, i);
            a += i;
            bytes -= i;
        }
        else
        {
            memcpy_(a, &rng_data, bytes);
            return TRNG_SUCCESS;
        }
    }

    tmp = bytes/4;
    while(tmp)
    {
        while(0 == (TERO_SR & 1))
        {;}
        *((unsigned int *)a) = TERO_DR;//printf("\r\n %08x", *((unsigned int *)a));

        a += 4;
        tmp--;
    }

    bytes = bytes & 3;
    if(bytes)
    {
        while(0 == (TERO_SR & 1))
        {;}
        rng_data = TERO_DR;//printf("\r\n %08x", rng_data);
        memcpy_(a, &rng_data, bytes);
    }

#ifdef TRNG_POKER_TEST
    poker_test(a-tmp_len, tmp_len);
#endif

    return TRNG_SUCCESS;
}
