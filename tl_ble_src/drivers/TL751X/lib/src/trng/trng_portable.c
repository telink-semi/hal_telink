/********************************************************************************************************
 * @file    trng_portable.c
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
#include "lib/include/trng/trng_portable.h"
#include "compiler.h"
/**
 * @brief Initialize TRNG-related generic configurations.
 * @note        Only after calling this function can other TRNG related functions be called.
 *              Otherwise, other TRNG function settings will not take effect.
 * @return None.
 * @verbatim
      -# 1.The trng module is mounted on the AHB bus, and the internal configuration clock that controls the trng
           registers is derived from hclk, and the trng sample clock source is roclk.

   @endverbatim
 */
void trng_dig_en(void)
{
    trng_reset();
    trng_clk_en();
}

/**
 * @brief     This function initialize trng.
 * @return    none
 * @note
  @verbatim
      -# 1. The power consumption current of trng is about 1ma.
  @endverbatim
 */
void trng_init(void)
{
    /**
     *1: The default register configuration of the chip's random number module is set up
     *   according to the best possible randomness, so some of the initialization setup
     *   code is commented out to reduce code size and execution time.
     *2: Currently, trng has two entropy sources: RO and TERO, and we have confirmed with OSR
     *   that the randomness will be better if we choose RO as the entropy source.
     *3: The lower the sampling clock of the RO mode, the better the randomization performance,
     *   currently configurable values 1/4, 1/8, 1/16, 1/32 (the sampling clock frequency is the dividing factor of the input clock).
     *4: The random number performance of trng is better when all the entropy sources are turned on, currently there are four entropy
     *   sources (with more entropy sources turned on, the more power trng consumes).
     */

    trng_dig_en();
#if 0
    /***close ro rng***/
    trng_disable();
    /***set random number mode to true random number mode***/
    trng_set_mode(0);
    /***turn on the four-way ro entropy source***/
    trng_ro_entropy_config(0x0F);
    /***configure the 16-way entropy source subconfiguration for the serial number 0 entropy source***/
    trng_ro_sub_entropy_config(0, 0xFFFF);
    /***configure the 16-way entropy source subconfiguration for the serial number 1 entropy source***/
    trng_ro_sub_entropy_config(1, 0xFFFF);
    /***configure the 16-way entropy source subconfiguration for the serial number 2 entropy source***/
    trng_ro_sub_entropy_config(2, 0xFFFF);
    /***configure the 16-way entropy source subconfiguration for the serial number 3 entropy source***/
    trng_ro_sub_entropy_config(3, 0xFFFF);
    /***setting the ro rng sampling clock frequency***/
    trng_set_freq(3);
    /***enable ro rng***/
    trng_enable();
#endif
}

/**
 * @brief     This function performs to get one random number.
 * @return    the value of one random number.
 *  @verbatim
      -# 1. This interface function takes 30us to acquire data.
  @endverbatim
 */
_attribute_ram_code_sec_noinline_  unsigned int trng_rand(void)
{
    unsigned char i;
    unsigned char status;
    unsigned char trng_buf[4];
    /**
     *1: The get_rand() function updates the random number seed after every 32 bytes of data has been obtained.
     */
    for(i=0;i<6;i++)
    {
        status = get_rand(trng_buf, 4);
        if(status == TRNG_SUCCESS)
        {
            break;
        }
    }
    return *(unsigned int*)trng_buf;
}
