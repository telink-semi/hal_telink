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
#include "../ull_hid_config.h"
#if (ULL_HID_DEMO_SLECT == ULL_HID_HOST)

    #define DEFAULT_MODE  0
    #define HYBRID_MODE   1

    #define CIS_MODE_TEST 0
    #define CIS_MODE_ULL  1

typedef struct
{
    int reportInterval;
    u8  nse;
    u8  reportID;
    u8  reportType;

    struct
    { //additional_info
        u8 powerSavingCfm : 1;
        u8 repetition     : 1;
    };

    u8 cisSduM2S;
    u8 cisSduS2M;

    u16 cisSduInterval;
    u8  maxPduSize;
    u8  retryCount;
    u8  sequenceNumber;
    u8  recvAckSeqNum;
    u8  recvSequenceNumber;
} app_ullhid_param_t;

extern app_ullhid_param_t ullhidParam;

/**
 * @brief        user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]    none
 * @return      none
 */
void user_init_normal(void);


/**
 * @brief        user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]    none
 * @return      none
 */
void user_init_deepRetn(void);


/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
void main_loop(void);


/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback(u32 h, u8 *p, int n);


/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback(u32 h, u8 *para, int n);


/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt);


#endif //INTER_TEST_MODE == TEST_ULL_HID_HOST_CUSTOMER

#endif
