/********************************************************************************************************
 * @file    app_config.h
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

#define ULL_HID_DEVICE 0
#define ULL_HID_HOST 1

#define ULL_HID_DEMO_SLECT ULL_HID_DEVICE

#define APP_ULL_HID_LOG_EN                          1

#define APP_AUDIO_UI_UART                           1
#define APP_AUDIO_UI_USB_CDC                        2
#define APP_AUDIO_UI_IFACE                          APP_AUDIO_UI_UART

#if APP_AUDIO_UI_IFACE == APP_AUDIO_UI_UART
#define TLKAPI_DEBUG_ENABLE                         1
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_UDB
#else
#define TLKAPI_DEBUG_ENABLE                         0
#define MODULE_USB_ENABLE                           1
#define USB_CDC_ENABLE                              1
#define ID_VENDOR                                   0x248a          // for report
#define ID_PRODUCT_BASE                             0x6102          //AUDIO_HOGP
#endif

#if (ULL_HID_DEMO_SLECT == ULL_HID_DEVICE)
    #include "ull_hid_device/app_config.h"
#elif (ULL_HID_DEMO_SLECT == ULL_HID_HOST)
    #include "ull_hid_host/app_config.h"
#else
    #error "unknown demo"
#endif
