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

#include "config.h"
#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_HDT_RECIPIENT)
#define ACL_CENTRAL_MAX_NUM    0 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM    1 // ACL peripheral maximum number

///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE 1 //1 for smp,  0 no security



/////////////////////// Board Select Configuration ///////////////////////////////
#if (MCU_CORE_TYPE == MCU_CORE_TL721X)
/*
     * BOARD_721X_EVK_C1T315A20     single antenna
     * BOARD_721X_EVK_C1T315A102    multi-antenna
     * */
#define BOARD_SELECT          BOARD_721X_EVK_C1T315A102
#else
#error "MCU type not support!!!"
#endif


#define GPIO_WORK_VOLTAGE GPIO_VOLTAGE_3V3 //change


/* Flash 4line mode:
 *  enable the 4 line mode of flash, read and write.
 */
#define FLASH_4LINE_MODE_ENABLE 1

#define UI_LED_ENABLE 1

#define UI_KEYBOARD_ENABLE 0

/////////////////////// Higher Data Throughput Configuration ///////////////////////////////
#define BLUETOOTH_VER                                                           BLUETOOTH_VER_6_X
#define LL_FEATURE_SUPPORT_HIGHER_DATA_THROUGHPUT                               1
#define LL_FEATURE_SUPPORT_CHANNEL_CLASSIFICATION                               0
#define LL_FEATURE_SUPPORT_CONNECTION_SUBRATING_HOST                            0
#define LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER       0
#define LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER          0
#define LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR                           0
#define LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_INITIATOR                           0
#define LL_FEATURE_SUPPORT_CS_TEST                                              0
#define LL_FEATURE_SUPPORT_MONITORING_ADVERTISERS                               0
#define OS_SUP_EN               0
#define OS_SUP_LONG_SLEEP       0

///////////////////////// DEBUG  Configuration /////////////////////////////////////////
#define DEBUG_HDT_GPIO_ENABLE 0 // clean this when release sdk

#define DEBUG_GPIO_ENABLE    0

#define TLKAPI_DEBUG_ENABLE  1

#define APP_LOG_EN           1
#define APP_CONTR_EVT_LOG_EN 1 //controller event
#define APP_HOST_EVT_LOG_EN   1
#define APP_SMP_LOG_EN        0
#define APP_SIMPLE_SDP_LOG_EN 0
#define APP_KEY_LOG_EN        1

#define JTAG_DEBUG_DISABLE    1 //if use JTAG, change this

#define APP_HDT_UI_UART        1
#define APP_HDT_UI_USB_CDC     2

#define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UDB

#include "../common/default_config.h"
#endif
