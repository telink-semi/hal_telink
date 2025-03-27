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
#include "../ull_hid_config.h"
#if (ULL_HID_DEMO_SLECT == ULL_HID_HOST)
#define ACL_CENTRAL_MAX_NUM                         1 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_CENTRAL_SMP_ENABLE                      1   //1 for smp,  0 no security
#define ACL_CENTRAL_CUSTOM_PAIR_ENABLE              0


///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE                               1
#define UI_KEYBOARD_ENABLE                          0
#define CIS_ADD_CIE                                 0  //todo
#define APPLICATION_DONGLE                          0 //usb application used

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////

#define APP_LOG_EN                                  0



#include "../common/default_config.h"



#endif    //INTER_TEST_MODE == TEST_ULL_HID_HOST_CUSTOMER
