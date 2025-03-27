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

#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)

#define ACL_CENTRAL_MAX_NUM                         0 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         1 // ACL peripheral maximum number


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE                      1   //1 for smp,  0 no security
#define BLE_APP_PM_ENABLE                           0

#define ADV_USE_EXT_MODE                            1  //1: ext_adv;  0: leg_adv

#define FAST_SETTLE                                 1

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE                               1
#define UI_KEYBOARD_ENABLE                          1

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE                           0
#define DEBUG_SIHUI_GPIO_ENABLE                     0


#define APP_LOG_EN                                  1
#define APP_CONTR_EVT_LOG_EN                        1   //controller event
#define APP_HOST_EVT_LOG_EN                         1
#define APP_KEY_LOG_EN                              1




#define CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN       1  //remove when release SDK

#define VCD_EN                                      0
#define VCD_DEFINE_SELECT                           VCD_DEFINE_CIS_PER


#if (1)  //SiHui use, remove when release SDK
#define DEBUG_SIHUI_GPIO_ENABLE                     0
#define IUT_HCI_LOG_EN                              1
#define BLC_LL_LOG_EN                               0
#define CIS_FLOW_LOG_EN                             1
#define LL_CTRL_LOG_EN                              1
#define DBG_CIS_1ST_AP_TIMING_EN                    1
#define DBG_CIS_PARAM                               1
#define DBG_CIS_CENTRAL_PARAM                       1
#define DBG_CIS_TX_DATA                             0
#define DBG_CIS_RX_DATA                             0
#define DBG_SET_CIG_PARAMS                          1
#endif

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


#define SVC_DEFAULT_KEYBOARD_ENABLE                 0
#define SVC_DEFAULT_MOUSE_ENABLE                    0
#define SVC_DEFAULT_KEYBOARD_MOUSE_ENABEL           1


#include "../common/default_config.h"

#endif  //INTER_TEST_MODE == TEST_ULL_HID_DEVICE
