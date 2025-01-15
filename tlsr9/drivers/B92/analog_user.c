/********************************************************************************************************
 * @file    analog_user.c
 *
 * @brief   This is the source file for TLSR9528
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "analog_user.h"
#include "analog.h"

/**
 * @brief      This function serves to analog register read by byte.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_optimize_o2_ unsigned char user_analog_read_reg8(unsigned char addr)
{
    return analog_read_reg8(addr);
}

/**
 * @brief      This function serves to analog register write by byte.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_optimize_o2_ int user_analog_write_reg8(unsigned char addr, unsigned char data)
{
    if (addr == analog_reg_59 || addr == analog_reg_60)
    {
        analog_write_reg8(addr, data);
        return 0;
    }
    return -1;
}


_attribute_data_retention_sec_ analog_read_f analog_read = user_analog_read_reg8;
_attribute_data_retention_sec_ analog_write_f analog_write = user_analog_write_reg8;
