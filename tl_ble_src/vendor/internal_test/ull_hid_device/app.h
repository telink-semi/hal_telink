/********************************************************************************************************
 * @file    app.h
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
#ifndef VENDOR_APP_H_
#define VENDOR_APP_H_

#include "app_config.h"

#if (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)

    #define DEFAULT_MODE 0
    #define HYBRID_MODE  1

typedef struct
{
    u8 reportID;
    u8 reportType;

    struct
    { //additional_info
        u8 powerSavingCfm : 1;
        u8 repetition     : 1;
    };

    u16 cisSduInterval;
    u8  maxPduSize;
    u8  retryCount;
    u8  sequenceNumber;
    u8  recvAckSeqNum;

    u8 recvSequenceNumber;
} app_ullhid_param_t;

extern app_ullhid_param_t ullhidParam;

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
void user_init_normal(void);


/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
void main_loop(void);

#endif //INTER_TEST_MODE == TEST_ULL_HID_DEVICE

#endif
