/********************************************************************************************************
 * @file    flash_common.h
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
#ifndef __FLASH_COMMON_H__
#define __FLASH_COMMON_H__


/**
 * @brief 		This function is used to enable the four-wire function of flash.
 * @param[in]   device_num	- the number of slave device.
 * @param[in]  	flash_mid	- the mid of flash.
 * @return 		1: success, 0: error, 2: parameter error, 3: mid is not supported.
 */
unsigned char flash_4line_en_with_device_num(mspi_slave_device_num_e device_num, unsigned int flash_mid);

/**
 * @brief 		This function is used to disable the four-wire function of flash.
 * @param[in]   device_num	- the number of slave device.
 * @param[in]  	flash_mid	- the mid of flash.
 * @return 		1: success, 0: error, 2: parameter error, 3: mid is not supported.
 */
unsigned char flash_4line_dis_with_device_num(mspi_slave_device_num_e device_num, unsigned int flash_mid);

#endif

