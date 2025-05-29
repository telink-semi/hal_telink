/********************************************************************************************************
 * @file    app_ull_hid.h
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

#pragma once

#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)

    #define ULL_HID_LOG(fmt, ...) tlkapi_printf(APP_ULL_HID_LOG_EN, "[ULL-HID]" fmt "\n", ##__VA_ARGS__)

enum
{
    HID_MODE_NONE,
    HID_MODE_MOUSE_HYBRID,
    HID_MODE_MOUSE_GATT,
    HID_MODE_KEYBOARD_HYBRID,
    HID_MODE_KEYBOARD_GATT,
    HID_MODE_TEST_ISO,
};

typedef struct __attribute__((packed))
{
    u8 length;
    u8 sequenceNumber;
    u8 reportId;
    u8 data[2];
} ullhid_sdu_data_t;

extern ullhid_sdu_data_t sduData[8];
extern u8                reportIndex;

/**
 * @brief   initial Ultra Low Latency HID device.
 * @param   none.
 * @return  none.
 */
void app_initial_ull_hid_device(void);

/**
 * @brief       Ultra Low Latency HID device main loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_ull_hid_device_main_loop(void);

void app_ull_hid_acl_connect(u16 connHandle, u16 connInterval);
void app_ull_hid_acl_disconnect(u16 connHandle);
void app_ull_hid_cis_connect(u16 connHandle, u16 isoIntvl, u8 NSE, u16 pdu_s2m);
void app_ull_hid_cis_disconnect(u16 connHandle);
u16  app_ull_hid_get_acl_handle(void);
u16  app_ull_hid_get_cis_handle(void);

void blc_app_ull_ui_init(void);

#endif //INTER_TEST_MODE == TEST_ULL_HID_DEVICE
