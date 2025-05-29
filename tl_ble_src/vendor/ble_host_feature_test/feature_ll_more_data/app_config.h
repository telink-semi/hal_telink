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

#if (FEATURE_TEST_MODE == TEST_LL_MD)


    #define ACL_CENTRAL_MAX_NUM 1  // ACL central maximum number
    #define ACL_PERIPHR_MAX_NUM 1  // ACL peripheral maximum number

    #define LL_ACL_CEN_EN       1 // todo:xh
    #define LL_ACL_PER_EN       1 // todo:xh


/////////////////////// Board Select Configuration ///////////////////////////////
#if (MCU_CORE_TYPE == MCU_CORE_B91)
    #define BOARD_SELECT BOARD_951X_EVK_C1T213A20
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    #define BOARD_SELECT BOARD_952X_EVK_C1T266A20
#elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
    #define BOARD_SELECT BOARD_721X_EVK_C1T315A20
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
    #define BOARD_SELECT BOARD_321X_EVK_C1T331A20 //BOARD_321X_EVK_C1T335A20
#elif (MCU_CORE_TYPE == MCU_CORE_TL322X)
    #define BOARD_SELECT BOARD_322X_EVK_C1T371A20
#endif

    ///////////////////////// Feature Configuration////////////////////////////////////////////////
    #define ACL_PERIPHR_SMP_ENABLE                         1 //1 for smp,  0 no security
    #define ACL_CENTRAL_SMP_ENABLE                         1 //1 for smp,  0 no security
    #define ACL_CENTRAL_SIMPLE_SDP_ENABLE                  1 //simple service discovery for ACL central

    #define BLE_APP_PM_ENABLE                              0


    #define APP_DEFAULT_BUFFER_ACL_OCTETS_MTU_SIZE_MINIMUM 1
    #define APP_DEFAULT_HID_BATTERY_OTA_ATTRIBUTE_TABLE    1


    ///////////////////////// UI Configuration ////////////////////////////////////////////////////
    #define UI_LED_ENABLE      1
    #define UI_KEYBOARD_ENABLE 1

    ///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
    #define DEBUG_GPIO_ENABLE     0

    #define TLKAPI_DEBUG_ENABLE   1
    #define TLKAPI_DEBUG_CHANNEL  TLKAPI_DEBUG_CHANNEL_GSUART

    #define APP_LOG_EN            1
    #define APP_CONTR_EVT_LOG_EN  1 //controller event
    #define APP_HOST_EVT_LOG_EN   1
    #define APP_SMP_LOG_EN        0
    #define APP_SIMPLE_SDP_LOG_EN 0
    #define APP_PAIR_LOG_EN       1
    #define APP_KEY_LOG_EN        1

    #define JTAG_DEBUG_DISABLE    1 //if use JTAG, change this

    #define APP_FLASH_PROTECTION_ENABLE 0
    #define FLASH_4LINE_MODE_ENABLE 0
    #define N22_FW_DOWNLOAD_FLASH_ADDR  0x20080000


//#define TLKAPI_DEBUG_FIFO_SIZE  320
#define TLKAPI_DEBUG_FIFO_NUM   128


#define MAILBOX_D25F          1
#define TLK_MESSAGE_D25F      1
#define MAILBOX_N22           0
#define TLK_MESSAGE_N22       0

//TODO: just for function test, need to remove later.
#define TLK_STK_BLE_ENABLE              1

#define TLKDBG_CFG_UDB_LOG_ENABLE       0
#define TLKDBG_CFG_HPU_LOG_ENABLE       0
#define TLK_DEV_LED_ENABLE              0
#define TLK_DEV_KEY_ENABLE              0
#define TLK_DEBUG_ENABLE                0
#define TLK_CFG_USB_ENABLE              0
#define TLK_USB_UDB_ENABLE              0
#define TLK_CFG_SYSTEM_ENABLE           0
#define DUAL_CORE_MODE_ENABLED          1
#define SVC_DEFAULT_KEYBOARD_ENABLE     1


#define HCI_UART                        0
#define HCI_USB                         1
#define HCI_SHAREMEMORY                 2
#define HCI_INTERFACE                   HCI_SHAREMEMORY
#define MAILBOX_FALSH_WR_NOTIFY         1


#if (HCI_INTERFACE==HCI_UART)
    #define UART_MODULE_SEL             3
    #define UART_MODULE_IRQ             IRQ_UART3
    #define UART_MODULE_BAUDRATE        1000000
    #define UART_MODULE_TX_PIN          GPIO_FC_PC1
    #define UART_MODULE_RX_PIN          GPIO_FC_PC0

    #define UART_DMA_CHANNEL_RX         DMA6
    #define UART_DMA_CHANNEL_TX         DMA12

    #define SPP_RXFIFO_SIZE             (1028)
    #define SPP_RXFIFO_NUM              32
#endif


    #include "../../common/default_config.h"

#endif //end of (FEATURE_TEST_MODE == ...)
