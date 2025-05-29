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

#if (INTER_TEST_MODE == TEST_RAS_CLIENT)

#include "../intest_config.h"

#define __CHANNEL_SOUNDING_EN__  1
#define CONN_MAX_NUM_CONFIG      CONN_MAX_NUM_C4_P4

#define TTF_EN                   1
#define RAS_IOPTEST_ENABLE       1
#define CONSOLE_OUTPUT_VIA_DEBUG 1 // UI (condole) will be output via debug logs

#define ACL_CENTRAL_MAX_NUM 1 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM 0 // ACL peripheral maximum number

#define GOOGLE_CS_INIT_ROLE_EN        0

///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE        0 //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE        1 //1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE 0 //simple service discovery for ACL central

#define BATT_CHECK_ENABLE             0


/* Flash Protection:
 * 1. Flash protection is enabled by default in SDK. User must enable this function on their final mass production application.
 * 2. User should use "Unlock" command in Telink BDT tool for Flash access during development and debugging phase.
 * 3. Flash protection demonstration in SDK is a reference design based on sample code. Considering that user's final application may
 *    different from sample code, for example, user's final firmware size is bigger, or user have a different OTA design, or user need
 *    store more data in some other area of Flash, all these differences imply that Flash protection reference design in SDK can not
 *    be directly used on user's mass production application without any change. User should refer to sample code, understand the
 *    principles and methods, then change and implement a more appropriate mechanism according to their application if needed.
 */
#define APP_FLASH_PROTECTION_ENABLE 0


/////////////////////// Channel sounding Configuration ///////////////////////////////
#define CS_PROCEDURE_EXCHANGE       0
#define CS_PROCEDURE_CMD_TRIG       1

#define APP_CS_CONFIG_PER_ACL       1
#define APP_CS_CONFIG_NUM           (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM) * APP_CS_CONFIG_PER_ACL

//Ranging result filter
#define CS_DISTANCE_FILTER          1
#define CS_SW_FIXED_OFFSET          1
#define CS_TLK_ALGO2_EN             1

#define RAS_PERSISTENT_FILTER       1
#define RAS_OOB                     0
#define PRF_DBG_RAS_EN              1
#define RAS_LOGIC_MANUAL            0
#define RAS_PTS_LOST_SEGMENT_WORKAROUND 0
/////////////////////// Board Select Configuration ///////////////////////////////
#if (MCU_CORE_TYPE == MCU_CORE_B91)
    #define BOARD_SELECT BOARD_951X_EVK_C1T213A20
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    #define BOARD_SELECT BOARD_952X_EVK_C1T266A20 //BOARD_952X_EVK_C1T266A102 //BOARD_952X_EVK_C1T266A20//BOARD_952X_EVK_C1T266A102
    #define TLKAPI_DEBUG_UART_RX_PIN                 GPIO_FC_PD3
    #define TLKAPI_DEBUG_UART_TX_PIN                GPIO_FC_PD2
#elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
    #define BOARD_SELECT BOARD_721X_EVK_C1T315A20  //BOARD_721X_EVK_C1T315A102 --- tercel multi ant board for channel sounding
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
    #define BOARD_SELECT BOARD_321X_EVK_C1T331A20
#endif


#define GPIO_WORK_VOLTAGE GPIO_VOLTAGE_3V3

/* Flash 4line mode:
 *  enable the 4 line mode of flash, read and write.
 */
#define FLASH_4LINE_MODE_ENABLE 1
///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_KEYBOARD_ENABLE      0
#define UI_LED_ENABLE           1
#define UI_CONTROL_ENABLE       1
#define UI_BUTTON_ENABLE        0
///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_CS_GPIO_ENABLE     1

#define DEBUG_GPIO_ENABLE        0

#define TLKAPI_DEBUG_ENABLE      1

#define APP_CS_LOG_EN            0
#define APP_CS_SUBEVENT_LOG_EN   0
#define APP_LOG_EN               1
#define APP_CONTR_EVT_LOG_EN     1 //controller event
#define APP_HOST_EVT_LOG_EN      1
#define APP_SMP_LOG_EN           1
#define APP_SIMPLE_SDP_LOG_EN    0
#define APP_PAIR_LOG_EN          1
#define APP_KEY_LOG_EN           1

#define JTAG_DEBUG_DISABLE       1 //if use JTAG, change this


#define APP_CS_UI_UART           1
#define APP_CS_UI_USB_CDC        2
#define APP_CS_UI_MODE           APP_CS_UI_UART

#define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UART
#define USB_CDC_ENABLE       0
#define APP_CS_UI_MODE APP_CS_UI_UART

// #if (APP_CS_UI_MODE == APP_CS_UI_UART)
//     #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UDB
//     #define USB_CDC_ENABLE       0
// #elif (APP_CS_UI_MODE == APP_CS_UI_USB_CDC)
//     #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_GSUART
//     #define MODULE_USB_ENABLE    1
//     #define USB_CDC_ENABLE       1
//     #define ID_VENDOR            0x248a //for report
//     #define ID_PRODUCT_BASE      0x6102 //AUDIO_HOGP
// #endif

/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG PM_ANA_REG_POWER_ON_CLR_BUF1 //u8,can save 8 bit info when deep
#define LOW_BATT_FLG      BIT(0)                       //if 1: low battery
#define CONN_DEEP_FLG     BIT(1)                       //if 1: conn deep, 0: adv deep


#include "../common/default_config.h"

#endif //#if (INTER_TEST_MODE == TEST_RAS_CLIENT)