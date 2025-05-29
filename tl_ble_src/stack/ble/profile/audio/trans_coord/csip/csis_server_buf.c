/********************************************************************************************************
 * @file    csis_server_buf.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#define CSISS_DEFAULT_LOCK_TIMEOUT 60                                                                                               //60s
#define CSISS_DEFAULT_PLAIN_SIRK   {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0f, 0x10} //must 16byte

const blc_csiss_regParam_t defaultCsipSetMemberParam =
    {
        .setSize       = 2,
        .setRank       = 1,
        .lockedTimeout = CSISS_DEFAULT_LOCK_TIMEOUT,
        .SIRK_type     = 1,
        .SIRK          = CSISS_DEFAULT_PLAIN_SIRK,
};
