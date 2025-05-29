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


// need delete when release.
#define BLUETOOTH_VER            BLUETOOTH_VER_5_4
#define CONN_MAX_NUM_CONFIG      CONN_MAX_NUM_C0_P1
#define ESL_RAM_OPTIMIZATION     1
#define ESL_CURRENT_OPTIMIZATION 1
#define USE_DRIVER_PM            1
//release use
#if 1
    #define TLKAPI_USE_INTERNAL_SPECIAL_UART_TOOL 0
    #define SDK_RELEASE_CHECK_EN                  0
    #define PM_DEEPSLEEP_RETENTION_ENABLE         1
    #define BLC_PM_DEEP_RETENTION_MODE_EN         1
    #define FIX_AUX_CONN_SLOT_IDX_CAL             1
    #define VCD_EN                                0
    #define BLE_STACK_MCU_STALL_EN                1
    #define LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER 1
#endif

#define ACL_CENTRAL_MAX_NUM               0            // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM               1            // ACL peripheral maximum number
#define LEGACY_ADV_SEND                   1
#define BLE_OTA_SERVER_ENABLE             1
#define BLT_SOFTWARE_TIMER_ENABLE 1
#define HW_EVK                            1
#define HW_C1T233A78                      2            // B91
#define HW_C1T335A78                      3            // TL321X

#define HARDWARE_BOARD_SELECT             HW_C1T335A78 //HW_C1T233A78
#define BOARD_SELECT                      BOARD_321X_EVK_C1T335A78

#define APP_IMAGE_STORAGE_FLASH                     1
#if (APP_IMAGE_STORAGE_FLASH)
#define OTS_SERVER_MAX_OBJECTS_NUM                  0x21
#define APP_IMAGE_STORAGE_MAX_IMAGES                0x20
#else
#define OTS_SERVER_MAX_OBJECTS_NUM                  0x02
#define APP_IMAGE_STORAGE_MAX_IMAGES                0x01
#endif
#define OTS_SERVER_MAX_OBJECT_NAME_LENGTH           8
#define APP_IMAGE_STORAGE_MAX_IMAGE_SIZE            0x1280
#define APP_VENDOR_IMAGE                            1
#if (HARDWARE_BOARD_SELECT != HW_EVK)
    #define APP_EPD_DISPLAY 1
#else
    #define APP_EPD_DISPLAY 0
#endif

#define ESLS_DISPLAYS_SUPPORTED 1
#define ESLS_SENSORS_SUPPORTED  1
#define ESLS_LEDS_SUPPORTED     2

///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE        1 //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE        0 //1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE 1 //simple service discovery for ACL central

#define BLE_APP_PM_ENABLE             1
#define PM_DEEPSLEEP_RETENTION_ENABLE 1
///////////////////////// ! OS settings////////////////////////////////////////////////
#define FREERTOS_ENABLE                                0


#define APP_DEFAULT_BUFFER_ACL_OCTETS_MTU_SIZE_MINIMUM 0
#define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE    1


///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE 1
#if (HARDWARE_BOARD_SELECT != HW_EVK)
    #define UI_KEYBOARD_ENABLE 0
#else
    #define UI_KEYBOARD_ENABLE 0
#endif

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE 0

#if (HARDWARE_BOARD_SELECT != HW_EVK)
    #define TLKAPI_DEBUG_ENABLE 0
#else
    #define TLKAPI_DEBUG_ENABLE 1
#endif
#define TLKAPI_DEBUG_CHANNEL  TLKAPI_DEBUG_CHANNEL_GSUART

#define APP_LOG_EN            1
#define APP_PAWR_EVT_LOG_EN   1 //controller event
#define APP_HOST_EVT_LOG_EN   1
#define APP_SMP_LOG_EN        0
#define APP_SIMPLE_SDP_LOG_EN 0
#define APP_PAIR_LOG_EN       1
#define APP_KEY_LOG_EN        1


#define JTAG_DEBUG_DISABLE    1


#include "../common/default_config.h"
