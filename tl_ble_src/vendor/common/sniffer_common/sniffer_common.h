/********************************************************************************************************
 * @file    sniffer_common.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef VENDOR_COMMON_SNF_COMMON_H_
#define VENDOR_COMMON_SNF_COMMON_H_


/**
 * @brief      This function is used to set memory buffer's value (like memset)
 * @param[in]  pd:      Start address of destination buffer
 * @param[in]  val:     set value
 * @param[in]  len:     set value length
 * @param[in]  dst_max_len: destination buffer's max length
 * @param[in]  error_code: default error code used if set value length greater than destination buffer's max length
 * @return     none:
 */
void blc_app_memory_set(void *pd, int val, unsigned int len, unsigned int dst_max_len, unsigned int error_code);


/**
 * @brief      This function is used to copy memory buffer's value (like memcpy)
 * @param[in]  pd:      Start address of destination buffer
 * @param[in]  ps:      Start address of the source buffer
 * @param[in]  len:     copy data length
 * @param[in]  dst_max_len: destination buffer's max length
 * @param[in]  error_code: default error code used if copy data length greater than destination buffer's max length
 * @return     none:
 */
void blc_app_memory_copy(void *pd, const void *ps, unsigned int len, unsigned int dst_max_len, unsigned int error_code);

/**
 * @brief      This function is used to user operate reboot at the application layer
 * @return     none:
 */
void app_start_reboot(void);

#endif /* VENDOR_COMMON_SNF_COMMON_H_ */
