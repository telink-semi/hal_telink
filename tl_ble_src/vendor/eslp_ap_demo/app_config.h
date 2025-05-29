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

// need delete when release.
#define         BLUETOOTH_VER                       BLUETOOTH_VER_5_4
#define         CONN_MAX_NUM_CONFIG                 CONN_MAX_NUM_C1_P0
#define         LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER 1

//release use
#if 1
#if (MCU_CORE_TYPE == MCU_CORE_TL321X) || (MCU_CORE_TYPE == MCU_CORE_B91)
#define EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP 0
#else
#define EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP 1
#endif
#define TLKAPI_USE_INTERNAL_SPECIAL_UART_TOOL 0
#define SDK_RELEASE_CHECK_EN 0
#define PM_DEEPSLEEP_RETENTION_ENABLE 0
#define BLC_PM_DEEP_RETENTION_MODE_EN 1
#define FIX_AUX_CONN_SLOT_IDX_CAL 1
#define VCD_EN 0
#define BLE_STACK_MCU_STALL_EN  0
#endif

#define HW_EVK                                      1
#define HW_DONGLE                                   2

#define HARDWARE_BOARD_SELECT                       HW_DONGLE

#define ACL_CENTRAL_MAX_NUM                         1 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number

#define ESLP_AP_MAX_GROUPS                          4
#if (MCU_CORE_TYPE == MCU_CORE_TL321X) || (MCU_CORE_TYPE == MCU_CORE_B91)
#define ESLP_AP_ESL_RECORDS                         64
#else
#define ESLP_AP_ESL_RECORDS                         1024
#endif

#define APP_PARSE_CHAR_UART                         1
#define APP_PARSE_CHAR_USB_CDC                      2

#if (MCU_CORE_TYPE == MCU_CORE_B91)
    #define BOARD_SELECT                                BOARD_951X_EVK_C1T213A20
    #define APP_PARSE_CHAR_IFACE                        APP_PARSE_CHAR_UART
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    #define BOARD_SELECT                                BOARD_952X_EVK_C1T266A20
    #define APP_PARSE_CHAR_IFACE                        APP_PARSE_CHAR_UART
#elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
    #define BOARD_SELECT                                BOARD_721X_EVK_C1T315A20
    #define APP_PARSE_CHAR_IFACE                        APP_PARSE_CHAR_UART
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
    #define BOARD_SELECT                                BOARD_321X_EVK_C1T331A20
    #define APP_PARSE_CHAR_IFACE                        APP_PARSE_CHAR_UART
#else
    #error "Please set HARDWARE_BOARD_SELECT in app_config.h"
#endif

#define BLE_OTA_CLIENT_ENABLE                       1
#if (BLE_OTA_CLIENT_ENABLE)
    #define OTA_LEGACY_PROTOCOL                             0  //0: OTA extended protocol; 1: OTA legacy protocol
    #define OTA_CLIENT_SUPPORT_BIG_PDU_ENABLE               0
    #define OTA_CLIENT_SEND_SECURE_BOOT_SIGNATURE_ENABLE    0
    #define OTA_SECURE_BOOT_DESCRIPTOR_SIZE                 0x2000
#endif

#if (EXPAND_MASTER_BONDING_DEVICE_MAX_NUM_FOR_PAWR_AP)
#define FLASH_SMP_PAIRING_MAX_SIZE                      (0x1E000)
#define FLASH_SDP_ATT_MAX_SIZE                          (0x71000)

#define FLASH_ADR_SMP_PAIRING_2M_FLASH                  (0x1B4000)
#define FLASH_AUD_ATT_ADDRESS_2M_FLASH                  (0xD2000)

#define FLASH_ADR_SMP_PAIRING_4M_FLASH                  (0x3B4000)
#define FLASH_AUD_ATT_ADDRESS_4M_FLASH                  (0x2D2000)
#endif

#if(APP_PARSE_CHAR_IFACE == APP_PARSE_CHAR_UART)
    #define HCI_UART_EXT_DRIVER_EN                      1
#endif  //#if(APP_PARSE_CHAR_IFACE == APP_PARSE_CHAR_UART)


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE                      0   //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE                      1   //1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE               0   //simple service discovery for ACL central

#define BLE_APP_PM_ENABLE                           0

///////////////////////// ! OS settings////////////////////////////////////////////////
#if (MCU_CORE_TYPE == MCU_CORE_TL321X) || (MCU_CORE_TYPE == MCU_CORE_B91)
#define FREERTOS_ENABLE                             0
#else
#define FREERTOS_ENABLE                             1
#endif
#define MODULE_USB_ENABLE                           1
#define USB_CDC_ENABLE                              1

#define APP_DEFAULT_BUFFER_ACL_OCTETS_MTU_SIZE_MINIMUM      0
#define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE         1

#define FIX_AUX_CONN_SLOT_IDX_CAL                   1 //fix aux_conn_req sslot_idx_next bug TODO: remove latter

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE                               1
#define UI_KEYBOARD_ENABLE                          0

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE                           0

#define TLKAPI_DEBUG_ENABLE                         1
#if (HARDWARE_BOARD_SELECT == HW_DONGLE)
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_GSUART
#else
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_GSUART
#endif

#define APP_LOG_EN                                  1
#define APP_CONTR_EVT_LOG_EN                        1   //controller event
#define APP_HOST_EVT_LOG_EN                         1
#define APP_SMP_LOG_EN                              0
#define APP_SIMPLE_SDP_LOG_EN                       0
#define APP_PAIR_LOG_EN                             1
#define APP_KEY_LOG_EN                              1


#define JTAG_DEBUG_DISABLE                          1

#if (MCU_CORE_TYPE == MCU_CORE_B91)
    #define PARSE_CHAR_UART_TX_PIN         UART1_TX_PC6
    #define PARSE_CHAR_UART_RX_PIN         UART1_RX_PC7
    #define PARSE_CHAR_UART_BAUDRATE       1000000
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    #define PARSE_CHAR_UART_TX_PIN         GPIO_PC6
    #define PARSE_CHAR_UART_RX_PIN         GPIO_PC7
    #define PARSE_CHAR_UART_BAUDRATE       1000000
#elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
    #define PARSE_CHAR_UART_TX_PIN         GPIO_PB6
    #define PARSE_CHAR_UART_RX_PIN         GPIO_PB5
    #define PARSE_CHAR_UART_BAUDRATE       1000000
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
    #define PARSE_CHAR_UART_TX_PIN         GPIO_PC6
    #define PARSE_CHAR_UART_RX_PIN         GPIO_PC7
    #define PARSE_CHAR_UART_BAUDRATE       1000000
#endif

#define TLKAPI_DEBUG_GPIO_PIN GPIO_PA0
///////////////////////// OS settings /////////////////////////////////////////////////////////
#define OS_SEPARATE_STACK_SPACE                     1   //Separate the task stack and interrupt stack space
#define configTOTAL_HEAP_SIZE                       (16*1024)
#define configISR_PLIC_STACK_SIZE                   640

#if FREERTOS_ENABLE
    #define traceAPP_LED_Task_Toggle()  //gpio_toggle(GPIO_CH01);
    #define traceAPP_BLE_Task_BEGIN()   //gpio_write(GPIO_CH02,1);
    #define traceAPP_BLE_Task_END()     //gpio_write(GPIO_CH02,0);
    #define traceAPP_KEY_Task_BEGIN()   //gpio_write(GPIO_CH03,1);
    #define traceAPP_KEY_Task_END()     //gpio_write(GPIO_CH03,0);
    #define traceAPP_BAT_Task_BEGIN()   //gpio_write(GPIO_CH04,1);
    #define traceAPP_BAT_Task_END()     //gpio_write(GPIO_CH04,0);

    #define traceAPP_MUTEX_Task_BEGIN() //gpio_write(GPIO_CH05,1);
    #define traceAPP_MUTEX_Task_END()   //gpio_write(GPIO_CH05,0);

    #define tracePort_IrqHandler_BEGIN() //gpio_write(GPIO_CH06,1);
    #define tracePort_IrqHandler_END()   //gpio_write(GPIO_CH06,0);

#endif

#include "../common/default_config.h"
