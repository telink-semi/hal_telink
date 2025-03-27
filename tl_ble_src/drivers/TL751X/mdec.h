/********************************************************************************************************
 * @file    mdec.h
 *
 * @brief   This is the header file for TL751X
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
#pragma once

#include "analog.h"
#include "reg_include/mdec_reg.h"

/**
 * @brief       This function servers to reset the MDEC module.When the system is wakeup by MDEC, you should
 *              to reset the MDEC module to clear the flag bit of MDEC wakeup.
 * @return      none.
 */
static inline void mdec_reset(void)
{
    analog_write_reg8(mdec_rst_addr,analog_read_reg8(mdec_rst_addr) | FLD_MDEC_RST);
    analog_write_reg8(mdec_rst_addr,analog_read_reg8(mdec_rst_addr) & (~FLD_MDEC_RST));
}

/**
 * @brief       This function is used to initialize the MDEC module,include clock setting and input IO select.
 * @param[in]   pin - mdec pin.
 *                    In order to distinguish which pin the data is input from,only one input pin can be selected one time.
 * @return      none.
 */
void mdec_init(mdec_pin_e pin);

/**
 * @brief       This function is used to read the receive data of MDEC module's IO.
 * @param[out]  dat     - The array to store date.
 * @return      1 decode success,  0 decode failure.
 */
unsigned char mdec_read_dat(unsigned char *dat);



