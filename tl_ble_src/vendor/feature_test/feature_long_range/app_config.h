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

#include "../feature_config.h"

#if (FEATURE_TEST_MODE == TEST_LONG_RANGE)

#define I2C_CLK_SPEED       400000

#if MCU_CORE_TYPE==MCU_CORE_TL321X
#define I2C_GPIO_SDA_PIN   GPIO_FC_PE4
#define I2C_GPIO_SCL_PIN   GPIO_FC_PE5
#elif MCU_CORE_TYPE==MCU_CORE_TL721X
#define I2C_GPIO_SDA_PIN   GPIO_FC_PE6
#define I2C_GPIO_SCL_PIN   GPIO_FC_PE7
#else
#error "other MCU is not supported now."
#endif

#define TEST_CHANNEL_NUM                22      /*!< use secondary channel to avoid interference */
#define TEST_PACKET_SUM                 1000
#define TEST_PACKET_PER_TIMES           250
#define TEST_ADV_RESTART_TIMES          (TEST_PACKET_SUM / TEST_PACKET_PER_TIMES)
#define TEST_RF_POWER                   RF_POWER_INDEX_P10p56dBm

#define TEST_ROLE_CENTRAL   0
#define TEST_ROLE_PERIPHR   1
#define TEST_ROLE           TEST_ROLE_PERIPHR

#if TEST_ROLE ==  TEST_ROLE_PERIPHR
    #define ACL_PERIPHR_MAX_NUM                         1 // ACL peripheral maximum number
    #define ACL_CENTRAL_MAX_NUM                         0 // ACL central maximum number

    #define APP_EXT_ADV_SETS_NUMBER         1   //user set value
    #define APP_EXT_ADV_DATA_LENGTH         1024 //2048//1664//1024   //user set value
    #define APP_EXT_SCANRSP_DATA_LENGTH     1024 //2048//1664//1024   //user set value

#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
    #define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number
    #define ACL_CENTRAL_MAX_NUM                         1 // ACL central maximum number
#endif
///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE                      0   //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE                      0   //1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE               1   //simple service discovery for ACL central


#define BLE_APP_PM_ENABLE                           0



#define APP_DEFAULT_BUFFER_ACL_OCTETS_MTU_SIZE_MINIMUM      1
#define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE         1


///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE                               1
#define UI_KEYBOARD_ENABLE                          1

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE                           0

#define TLKAPI_DEBUG_ENABLE                         1
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_GSUART

#define APP_LOG_EN                                  1
#define APP_CONTR_EVT_LOG_EN                        1   //controller event
#define APP_HOST_EVT_LOG_EN                         1
#define APP_SMP_LOG_EN                              0
#define APP_SIMPLE_SDP_LOG_EN                       0
#define APP_PAIR_LOG_EN                             1
#define APP_KEY_LOG_EN                              1

#define JTAG_DEBUG_DISABLE                          1  //if use JTAG, change this


#include "../../common/default_config.h"

#endif //end of (FEATURE_TEST_MODE == ...)
