/********************************************************************************************************
 * @file    trng_portable.h
 *
 * @brief   This is the header file for B95
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#ifndef TRNG_PORTABLE_H
#define TRNG_PORTABLE_H
#include "driver.h"


#ifdef __cplusplus
extern "C" {
#endif

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
void trng_dig_en(void);

/**
 * @brief     Resets TRNG module,before using TRNG, it is needed to call trng_reset() to avoid affecting the use of TRNG.
 * @return    none
 */
static inline void trng_reset(void)
{
    reg_rst2 &= ~FLD_RST2_TRNG;
    reg_rst2 |= FLD_RST2_TRNG;
}

/**
 * @brief     Enable the clock of TRNG module.
 * @return    none
 */
static inline void trng_clk_en(void)
{
    reg_clk_en2 |= FLD_CLK2_TRNG_EN;
}

/**
 * @brief     This function initialize trng.
 * @return    none
 * @note
  @verbatim
      -# 1. The power consumption current of trng is about 1ma.
  @endverbatim
 */
void trng_init(void);

/**
 * @brief     This function performs to get one random number.
 * @return    the value of one random number.
 *  @verbatim
      -# 1. This interface function takes 30us to acquire data.
  @endverbatim
 */
_attribute_ram_code_sec_noinline_  unsigned int trng_rand(void);

#ifdef __cplusplus
}
#endif

#endif
