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
#include "../node_config.h"

#if (MAIN_NODE_ROLE_SELECT == MAIN_NODE_CS_PERIPHERAL_CENTRAL)

    ///////////////////////// ACL Configuration ////////////////////////////////////
    #define ACL_CENTRAL_MAX_NUM 1 // ACL central maximum number
    #define ACL_PERIPHR_MAX_NUM 1 // ACL peripheral maximum number

    #define MY_RF_POWER_INDEX   RF_POWER_INDEX_P9p15dBm

    ///////////////////////// Hardware board selection /////////////////////////////
    #define HARDWARE_BOARD_SELECT               0xFF // configure this macro to be invalid
    #define BOARD_9528A_EVK_C1T266A20_V1_3      1
    #define BOARD_9223A_EVK_C1T289A67_V1_0      2
    #define BOARD_9223B_EVK_C1T325A67_V1_0      3
    #define BOARD_9223B_EVK_C1T325A20_V1_0      4
    #define BOARD_9223B_DUAL_ANTENNA_C1T325A67  5 //C1T325A67_V2_0(PA not included), C1T325A67_V2_1(Include PA)
    #define BOARD_9223B_DUAL_ANTENNA_C1T325A102 6

//    #define BOARD_SELECT                        BOARD_9528A_EVK_C1T266A20_V1_3
//    #define BOARD_SELECT                        BOARD_9223B_EVK_C1T325A20_V1_0
    #define BOARD_SELECT                        BOARD_9223B_DUAL_ANTENNA_C1T325A102

    ///////////////////////// Feature Configuration /////////////////////////////////
    #define ACL_PERIPHR_SMP_ENABLE                     1 //1 for smp,  0 no security
    #define ACL_CENTRAL_SMP_ENABLE                     1 //1 for smp,  0 no security
    #define ACL_CENTRAL_SIMPLE_SDP_ENABLE              0 //simple service discovery for ACL central

    #define BLE_APP_PM_ENABLE                          1

    #define BATT_CHECK_ENABLE                          0

    #define APP_PHY_SWITCHING_ENABLE                   1
    #define APP_CENTRAL_CONNECT_PERIPHR_NAME_FILTER_EN 0
    #define APP_SMP_LG_CAPABILITY_DISPLAY_ONLY_EN      0
    #define APP_BLE_EXT_ADV_ENABLE                     0
    #define APP_BLE_RESOLVING_LIST_ENABLE              0

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

    ///////////////////////// UI Configuration /////////////////////////////////////
    #define UI_LED_ENABLE 1
    #if ((BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102))
        #define UI_KEYBOARD_ENABLE 0
    #else
        #define UI_KEYBOARD_ENABLE 1
    #endif

    ///////////////////////// Sniffer main node Configuration //////////////////////
    #define LOCAL_SNIFFER_INDEX_0   0
    #define LOCAL_SNIFFER_INDEX_1   1
    #define LOCAL_SNIFFER_INDEX_2   2
    #define LOCAL_SNIFFER_INDEX_3   3
    #define LOCAL_SNIFFER_INDEX_4   4
    #define LOCAL_SNIFFER_INDEX_5   5
    #define CHECK_SNIFFER_INDEX_MAX (LOCAL_SNIFFER_INDEX_3 + 1)

    #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
        #define APP_TRANSPORT_UART_ENABLE  1
        #define APP_TRANSPORT_CANFD_ENABLE 0
    #elif (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
        #define APP_TRANSPORT_UART_ENABLE  0
        #define APP_TRANSPORT_CANFD_ENABLE 1
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A67_V1_0)
        #define APP_TRANSPORT_UART_ENABLE  0
        #define APP_TRANSPORT_CANFD_ENABLE 1
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
        #define APP_TRANSPORT_UART_ENABLE  0
        #define APP_TRANSPORT_LIN_ENABLE   0
        #define APP_TRANSPORT_CANFD_ENABLE 1
        #define APP_CAN_PM_ENABLE          0
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
        #define APP_TRANSPORT_UART_ENABLE  1
        #define APP_TRANSPORT_CANFD_ENABLE 0
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
        #define APP_TRANSPORT_UART_ENABLE  0
        #define APP_TRANSPORT_LIN_ENABLE   0
        #define APP_TRANSPORT_CANFD_ENABLE 1
        #define APP_CAN_PM_ENABLE          0
    #endif
    #if ((APP_TRANSPORT_UART_ENABLE && APP_TRANSPORT_CANFD_ENABLE) || (APP_TRANSPORT_UART_ENABLE && APP_TRANSPORT_LIN_ENABLE) || (APP_TRANSPORT_LIN_ENABLE && APP_TRANSPORT_CANFD_ENABLE) )
        #error "can not use more than two transmission modes at the same time !!!"
    #endif

    ///////////////////////// Channel Sounding Configuration ////////////////////////////////////
    #define LL_CS_SNIFFER_MODE_ENABLE   1
    #define CS_DISTANCE_CALC_SUB_NODE_EN    1

    #define RAS_PROCEDURE_COUNT         5

    #define CS_USE_TX_POWER_LEVEL       RF_POWER_P7p00dBm

    #define CS_PROCEDURE_EXCHANGE       1
    #define CS_PROCEDURE_CMD_TRIG       0

    #define APP_CS_CONFIG_PER_ACL       1
    #define APP_CS_CONFIG_NUM           APP_CS_CONFIG_PER_ACL//(ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM) * APP_CS_CONFIG_PER_ACL

    #define APP_CS_THREE_POINT_POSITIONING_TAG_EN 0

    #define CS_TLK_ALGO2_EN             1
    #define CS_DISTANCE_TYPE_SUPPORT_MAX 3

    #define CS_DISTANCE_FILTER          1
    #if (CS_DISTANCE_FILTER)
        #define KALMAN_FILTER_ENABLE    1
        #define MEDIAN_FILTER_ENABLE    0
    #endif

    #define APP_CS_DISTANCE_TIMEOUT_SECONDS 15

    #define UI_CONTROL_ENABLE           1

    #if ((BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102) || (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0))
        #define ANTENNA_SWITCHING_AUTO_EN 1
    #else
        #define ANTENNA_SWITCHING_AUTO_EN 0
    #endif

    #define APP_CS_UI_UART              1
    #define APP_CS_UI_USB_CDC           2
    #define APP_CS_UI_MODE              APP_CS_UI_USB_CDC
//    #define APP_CS_UI_MODE              APP_CS_UI_UART
    #if (APP_CS_UI_MODE == APP_CS_UI_UART)
        #define USB_CDC_ENABLE          0
    #elif (APP_CS_UI_MODE == APP_CS_UI_USB_CDC)
        #define MODULE_USB_ENABLE       1
        #define USB_CDC_ENABLE          1
        #define ID_VENDOR               0x248a //for report
        #define ID_PRODUCT_BASE         0x6102 //AUDIO_HOGP
    #endif

    ///////////////////////// software_PA Configuration /////////////////////////////////
    #if (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
        #define PA_TXEN_PIN GPIO_PA3
        #define PA_RXEN_PIN GPIO_PA4
    #endif

    ///////////////////////// DEBUG  Configuration /////////////////////////////////
    #define BLT_ERR_PROCESS        ERR_LOG_ON_SRAM //release SDK remove!!!
//    #define BLT_ERR_PROCESS        ERR_TRIGGER_CODE_STUCK //release SDK remove!!!
    #define DBG_CS_DATA_EN         1                      //release SDK remove!!!
    #define DBG_CS_STEP_DATA_EN    0                      //release SDK remove!!!
    #define DEBUG_GPIO_ENABLE      0
    #define DEBUG_CS_GPIO_ENABLE   1
    #define DEBUG_SNIF_GPIO_ENABLE 0

    #if ((BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3) || (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102))
        #define DEBUG_RF_BASEBAND_GPIO_EN 1
    #endif
    #define PRF_DBG_RAS_EN                        0
    #define CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN 0
    #define PERIPHR_CONNECT_CENTRAL_MAC_FILTER_EN 0
    #define JTAG_DEBUG_DISABLE                    1 //if use JTAG, change this

    #define TLKAPI_DEBUG_CHANNEL_UDB            1 //USB Dump
    #define TLKAPI_DEBUG_CHANNEL_GSUART         2 //GPIO simulate UART
    #define TLKAPI_DEBUG_CHANNEL_UART           3 //hardware UART

    #define TLKAPI_DEBUG_FIFO_SIZE              288
    #define TLKAPI_DEBUG_FIFO_NUM               64

    #define TLKAPI_DEBUG_ENABLE                 1
    #if ((BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102))
        #define TLKAPI_DEBUG_CHANNEL            TLKAPI_DEBUG_CHANNEL_UART
//        #define TLKAPI_DEBUG_CHANNEL            TLKAPI_DEBUG_CHANNEL_UDB
        #define TLKAPI_DEBUG_UART_BAUDRATE      3000000
    #else
        #define TLKAPI_DEBUG_CHANNEL            TLKAPI_DEBUG_CHANNEL_GSUART
//        #define TLKAPI_DEBUG_CHANNEL            TLKAPI_DEBUG_CHANNEL_UDB
        #define TLKAPI_DEBUG_GSUART_BAUDRATE    1000000
    #endif
    #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PD4
    #elif (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PB1
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A67_V1_0)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PA6
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
        #define TLKAPI_DEBUG_GPIO_PIN    GPIO_PE6
        #define TLKAPI_DEBUG_UART_TX_PIN GPIO_PE6
        #define TLKAPI_DEBUG_UART_RX_PIN GPIO_PD6
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PC4
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
        #define TLKAPI_DEBUG_GPIO_PIN    GPIO_PE6
        #define TLKAPI_DEBUG_UART_TX_PIN GPIO_PE6
        #define TLKAPI_DEBUG_UART_RX_PIN GPIO_PD6
    #endif

    #if ((TLKAPI_DEBUG_CHANNEL == TLKAPI_DEBUG_CHANNEL_UDB) && (APP_CS_UI_MODE == APP_CS_UI_USB_CDC))
        #error "TLKAPI_DEBUG_CHANNEL_UDB and APP_CS_UI_USB_CDC cannot be used at the same time"
    #endif

    #define APP_LOG_EN            1
    #define APP_KEY_LOG_EN        1
    #define APP_PAIR_LOG_EN       1
    #define APP_CONTR_EVT_LOG_EN  1 //controller event
    #define APP_HOST_EVT_LOG_EN   1
    #define APP_SMP_LOG_EN        1
    #define APP_SMP_SC_EN         1
    #define APP_MITM_EN           0
    #define APP_OTA_LOG_EN        1
    #define APP_FLASH_INIT_LOG_EN 1
    #define APP_FLASH_PROT_LOG_EN 1
    #define APP_BATT_CHECK_LOG_EN 1
    #define APP_SIMPLE_SDP_LOG_EN 0
    #define APP_CAN_LOG_EN        1
    #define APP_SNIF_LOG_EN       1
    #define APP_RSSI_LOG_EN       1
    #define APP_CS_LOG_EN         1

    ///////////////////////// Parse Char Baud Rate and Pin Configuration /////////////////////////////////
    #define PARSE_CHAR_UART_BAUDRATE 3000000
    #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
        #define PARSE_CHAR_UART_TX_PIN GPIO_FC_PD5
        #define PARSE_CHAR_UART_RX_PIN GPIO_FC_PD3
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
        #define PARSE_CHAR_UART_TX_PIN GPIO_FC_PB1
        #define PARSE_CHAR_UART_RX_PIN GPIO_FC_PB0
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
        #define PARSE_CHAR_UART_TX_PIN GPIO_FC_PB1
        #define PARSE_CHAR_UART_RX_PIN GPIO_FC_PB0
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
        #if (TLKAPI_DEBUG_CHANNEL == TLKAPI_DEBUG_CHANNEL_UDB)
            #define PARSE_CHAR_UART_TX_PIN GPIO_FC_PE6
            #define PARSE_CHAR_UART_RX_PIN GPIO_FC_PB1
        #else
            #define PARSE_CHAR_UART_TX_PIN GPIO_FC_PB1
            #define PARSE_CHAR_UART_RX_PIN GPIO_FC_PB0
        #endif
    #endif

    ///////////////////// UART variables ///////////////////////////////////////////
    #define UART0_MODULE          0       //UART0
    #define UART_MODULE_SEL       UART0_MODULE
    #define UART_MODULE_BAUDRATE  2000000 //500000
    #define UART_BAUD_RATE        UART_MODULE_BAUDRATE
    #define UART_DMA_CHANNEL_RX   DMA5
    #define UART_DMA_CHANNEL_TX   DMA6
    #define UART_TX_WAIT_MAX_BYTE (SPP_TXFIFO_SIZE + 10)
    #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
        #define UART_MODULE_TX_PIN GPIO_FC_PC7
        #define UART_MODULE_RX_PIN GPIO_FC_PC6
    #elif (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
        #define UART_MODULE_TX_PIN GPIO_FC_PB1
        #define UART_MODULE_RX_PIN GPIO_FC_PB0
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A67_V1_0)
        #define UART_MODULE_TX_PIN GPIO_FC_PB1
        #define UART_MODULE_RX_PIN GPIO_FC_PB0
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
        #define UART_MODULE_TX_PIN GPIO_FC_PB1
        #define UART_MODULE_RX_PIN GPIO_FC_PB0
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
        #define UART_MODULE_TX_PIN GPIO_FC_PA2
        #define UART_MODULE_RX_PIN GPIO_FC_PA1
    #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
        #define UART_MODULE_TX_PIN GPIO_FC_PB1
        #define UART_MODULE_RX_PIN GPIO_FC_PB0
    #endif
    #define SPP_RXFIFO_SIZE      (64+16)
    #define SPP_RXFIFO_NUM       128
    #define SPP_TXFIFO_SIZE      64
    #define SPP_TXFIFO_NUM       128
    #define SPP_SLIPT_PACKET_LEN 50

    #if (UART_TX_WAIT_MAX_BYTE < (SPP_TXFIFO_SIZE + 10))
        #error "UART_TX_WAIT_MAX_BYTE < (SPP_TXFIFO_SIZE + 10) !!!"
    #endif


    /**
 *  @brief  GPIO definition for keyboard
 */
    #if (UI_KEYBOARD_ENABLE)
        #define MATRIX_ROW_PULL    PM_PIN_PULLDOWN_100K
        #define MATRIX_COL_PULL    PM_PIN_PULLUP_10K

        #define KB_LINE_HIGH_VALID 0 //drive pin output 0 when scan key, scan pin read 0 is valid

        #define BTN_PAIR           0x01
        #define BTN_UNPAIR         0x02

        #define CR_VOL_UP          0xf0 ////
        #define CR_VOL_DN          0xf1

        /**
     *  @brief  Normal keyboard map
     */
        #define KB_MAP_NORMAL {      \
            {CR_VOL_DN, BTN_PAIR  },   \
            {CR_VOL_UP, BTN_UNPAIR}, \
        }

        //////////////////// KEY CONFIG (EVK board) ///////////////////////////
        #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
            /* Description
         +--------------------------------------------------+
         |  Button  |   SW2   |   SW3   |   SW4   |   SW5   |
         +----------+---------+---------+---------+---------+
         | Function |   Vol+  |  Unpair |   Vol-  |   Pair  |
         +----------+---------+---------+---------+---------+
         */
            #define KB_DRIVE_PINS {GPIO_PD6, GPIO_PF6}
            #define KB_SCAN_PINS  {GPIO_PD7, GPIO_PD2}

            //scan pin as gpio
            #define PD6_FUNC AS_GPIO
            #define PF6_FUNC AS_GPIO

            //scan  pin need 10K pullup
            #define PULL_WAKEUP_SRC_PD6 MATRIX_ROW_PULL
            #define PULL_WAKEUP_SRC_PF6 MATRIX_ROW_PULL

            //scan pin open input to read gpio level
            #define PD6_INPUT_ENABLE 1
            #define PF6_INPUT_ENABLE 1

            //drive pin as gpio
            #define PD7_FUNC AS_GPIO
            #define PD2_FUNC AS_GPIO

            //drive pin need 100K pulldown
            #define PULL_WAKEUP_SRC_PD7 MATRIX_COL_PULL
            #define PULL_WAKEUP_SRC_PD2 MATRIX_COL_PULL

            //drive pin open input to read gpio wakeup level
            #define PD7_INPUT_ENABLE 1
            #define PD2_INPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
            /* Description
         +--------------------------------------------------+
         |  Button  |   SW2   |   SW3   |   SW5   |   SW6   |
         +----------+---------+---------+---------+---------+
         | Function |   Vol+  |   Vol-  |  Unpair |   Pair  |
         +----------+---------+---------+---------+---------+
         */
            #define KB_DRIVE_PINS       {GPIO_PE6, GPIO_PE7}
            #define KB_SCAN_PINS        {GPIO_PE5, GPIO_PE4}

            //drive pin as gpio
            #define PE6_FUNC            AS_GPIO
            #define PE7_FUNC            AS_GPIO

            //drive pin need 100K pulldown
            #define PULL_WAKEUP_SRC_PE6 MATRIX_ROW_PULL
            #define PULL_WAKEUP_SRC_PE7 MATRIX_ROW_PULL

            //drive pin open input to read gpio wakeup level
            #define PE6_INPUT_ENABLE    1
            #define PE7_INPUT_ENABLE    1

            //scan pin as gpio
            #define PE5_FUNC            AS_GPIO
            #define PE4_FUNC            AS_GPIO

            //scan  pin need 10K pullup
            #define PULL_WAKEUP_SRC_PE5 MATRIX_COL_PULL
            #define PULL_WAKEUP_SRC_PE4 MATRIX_COL_PULL

            //scan pin open input to read gpio level
            #define PE5_INPUT_ENABLE    1
            #define PE4_INPUT_ENABLE    1
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A67_V1_0)
            /* Description
         +--------------------------------------------------+
         |  Button  |   SW2   |   SW3   |   SW5   |   SW6   |
         +----------+---------+---------+---------+---------+
         | Function |   Vol+  |   Vol-  |  Unpair |   Pair  |
         +----------+---------+---------+---------+---------+
         */
            #define KB_DRIVE_PINS       {GPIO_PE6, GPIO_PA0}
            #define KB_SCAN_PINS        {GPIO_PE5, GPIO_PE4}

            //drive pin as gpio
            #define PE6_FUNC            AS_GPIO
            #define PA0_FUNC            AS_GPIO

            //drive pin need 100K pulldown
            #define PULL_WAKEUP_SRC_PE6 MATRIX_ROW_PULL
            #define PULL_WAKEUP_SRC_PA0 MATRIX_ROW_PULL

            //drive pin open input to read gpio wakeup level
            #define PE6_INPUT_ENABLE    1
            #define PA0_INPUT_ENABLE    1

            //scan pin as gpio
            #define PE5_FUNC            AS_GPIO
            #define PE4_FUNC            AS_GPIO

            //scan  pin need 10K pullup
            #define PULL_WAKEUP_SRC_PE5 MATRIX_COL_PULL
            #define PULL_WAKEUP_SRC_PE4 MATRIX_COL_PULL

            //scan pin open input to read gpio level
            #define PE5_INPUT_ENABLE    1
            #define PE4_INPUT_ENABLE    1
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
            /* Description
         +--------------------------------------------------+
         |  Button  |   SW2   |   SW3   |   SW4   |   SW5   |
         +----------+---------+---------+---------+---------+
         | Function |   Vol+  |  Unpair |   Vol-  |   Pair  |
         +----------+---------+---------+---------+---------+
         */
            #define KB_DRIVE_PINS       {GPIO_PE5, GPIO_PE4}
            #define KB_SCAN_PINS        {GPIO_PA1, GPIO_PA0}

            //drive pin as gpio
            #define PE5_FUNC            AS_GPIO
            #define PE4_FUNC            AS_GPIO

            //drive pin need 100K pulldown
            #define PULL_WAKEUP_SRC_PE5 MATRIX_ROW_PULL
            #define PULL_WAKEUP_SRC_PE4 MATRIX_ROW_PULL

            //drive pin open input to read gpio wakeup level
            #define PE5_INPUT_ENABLE    1
            #define PE4_INPUT_ENABLE    1

            //scan pin as gpio
            #define PA1_FUNC            AS_GPIO
            #define PA0_FUNC            AS_GPIO

            //scan  pin need 10K pullup
            #define PULL_WAKEUP_SRC_PA1 MATRIX_COL_PULL
            #define PULL_WAKEUP_SRC_PA0 MATRIX_COL_PULL

            //scan pin open input to read gpio level
            #define PA1_INPUT_ENABLE    1
            #define PA0_INPUT_ENABLE    1
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
            /* Description
         +--------------------------------------------------+
         |  Button  |   SW2   |   SW4   |   SW3   |   SW5   |
         +----------+---------+---------+---------+---------+
         | Function |   Vol+  |  Unpair |   Vol-  |   Pair  |
         +----------+---------+---------+---------+---------+
         */
            #define KB_DRIVE_PINS       {GPIO_PE6, GPIO_PA0}
            #define KB_SCAN_PINS        {GPIO_PE5, GPIO_PE4}

            //scan pin as gpio
            #define PE6_FUNC            AS_GPIO
            #define PA0_FUNC            AS_GPIO

            //scan  pin need 10K pullup
            #define PULL_WAKEUP_SRC_PE6 MATRIX_ROW_PULL
            #define PULL_WAKEUP_SRC_PA0 MATRIX_ROW_PULL

            //scan pin open input to read gpio level
            #define PE6_INPUT_ENABLE    1
            #define PA0_INPUT_ENABLE    1

            //drive pin as gpio
            #define PE5_FUNC            AS_GPIO
            #define PE4_FUNC            AS_GPIO

            //drive pin need 100K pulldown
            #define PULL_WAKEUP_SRC_PE5 MATRIX_COL_PULL
            #define PULL_WAKEUP_SRC_PE4 MATRIX_COL_PULL

            //drive pin open input to read gpio wakeup level
            #define PE5_INPUT_ENABLE    1
            #define PE4_INPUT_ENABLE    1
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
            /* Description
         +--------------------------------------------------+
         |  Button  |   SW1   |   SW2   |   SW3   |   SW4   |
         +----------+---------+---------+---------+---------+
         | Function |   Vol+  |  Unpair |   Vol-  |   Pair  |
         +----------+---------+---------+---------+---------+
         */
            #define KB_DRIVE_PINS       {GPIO_PE6, GPIO_PA0}
            #define KB_SCAN_PINS        {GPIO_PE5, GPIO_PE4}

            #define KB_DRIVE_PINS       {GPIO_PE2, GPIO_PE3}
            #define KB_SCAN_PINS        {GPIO_PE1, GPIO_PE0}

            //scan pin as gpio
            #define PE2_FUNC            AS_GPIO
            #define PE3_FUNC            AS_GPIO

            //scan  pin need 10K pullup
            #define PULL_WAKEUP_SRC_PE2 MATRIX_ROW_PULL
            #define PULL_WAKEUP_SRC_PE3 MATRIX_ROW_PULL

            //scan pin open input to read gpio level
            #define PE2_INPUT_ENABLE    1
            #define PE3_INPUT_ENABLE    1

            //drive pin as gpio
            #define PE1_FUNC            AS_GPIO
            #define PE2_FUNC            AS_GPIO

            //drive pin need 100K pulldown
            #define PULL_WAKEUP_SRC_PE1 MATRIX_COL_PULL
            #define PULL_WAKEUP_SRC_PE2 MATRIX_COL_PULL

            //drive pin open input to read gpio wakeup level
            #define PE1_INPUT_ENABLE    1
            #define PE2_INPUT_ENABLE    1
        #endif
    #endif


    /**
 *  @brief  GPIO definition for LED
 */
    #if UI_LED_ENABLE
        #define LED_ON_LEVEL 1 //gpio output high voltage to turn on led

        #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
            #define GPIO_LED_BLUE     GPIO_PD0
            #define GPIO_LED_GREEN    GPIO_PD1
            #define GPIO_LED_WHITE    GPIO_PE6
            #define GPIO_LED_RED      GPIO_PE7

            #define PD0_FUNC          AS_GPIO
            #define PD1_FUNC          AS_GPIO
            #define PE6_FUNC          AS_GPIO
            #define PE7_FUNC          AS_GPIO

            #define PD0_OUTPUT_ENABLE 1
            #define PD1_OUTPUT_ENABLE 1
            #define PE6_OUTPUT_ENABLE 1
            #define PE7_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
            #define GPIO_LED_BLUE     GPIO_PD0
            #define GPIO_LED_GREEN    GPIO_PD2
            #define GPIO_LED_WHITE    GPIO_PC4
            #define GPIO_LED_RED      GPIO_PD1

            #define PD0_FUNC          AS_GPIO
            #define PD2_FUNC          AS_GPIO
            #define PC4_FUNC          AS_GPIO
            #define PD1_FUNC          AS_GPIO

            #define PD0_OUTPUT_ENABLE 1
            #define PD2_OUTPUT_ENABLE 1
            #define PD1_OUTPUT_ENABLE 1
            #define PC4_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A67_V1_0)
            #define GPIO_LED_BLUE     GPIO_PD0
            //#define GPIO_LED_GREEN                    GPIO_PD2
            #define GPIO_LED_WHITE    GPIO_PC4
            #define GPIO_LED_RED      GPIO_PD1

            #define PD0_FUNC          AS_GPIO
            //#define PD2_FUNC                          AS_GPIO
            #define PC4_FUNC          AS_GPIO
            #define PD1_FUNC          AS_GPIO

            #define PD0_OUTPUT_ENABLE 1
            //#define   PD2_OUTPUT_ENABLE                   1
            #define PD1_OUTPUT_ENABLE 1
            #define PC4_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
            #define GPIO_LED_BLUE     GPIO_PD0
            //#define GPIO_LED_GREEN                      GPIO_PD2
            #define GPIO_LED_WHITE    GPIO_PD1
            #define GPIO_LED_RED      GPIO_PC4

            #define PD0_FUNC          AS_GPIO
            //#define PD2_FUNC                          AS_GPIO
            #define PD1_FUNC          AS_GPIO
            #define PC4_FUNC          AS_GPIO

            #define PD0_OUTPUT_ENABLE 1
            //#define   PD2_OUTPUT_ENABLE                   1
            #define PD1_OUTPUT_ENABLE 1
            #define PC4_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
            #define GPIO_LED_BLUE     GPIO_PE2
            #define GPIO_LED_GREEN    GPIO_PE0
            #define GPIO_LED_WHITE    GPIO_PE1
            #define GPIO_LED_RED      GPIO_PE3

            #define PE2_FUNC          AS_GPIO
            #define PE0_FUNC          AS_GPIO
            #define PE1_FUNC          AS_GPIO
            #define PE3_FUNC          AS_GPIO

            #define PE2_OUTPUT_ENABLE 1
            #define PE0_OUTPUT_ENABLE 1
            #define PE1_OUTPUT_ENABLE 1
            #define PE3_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
            #define GPIO_LED_BLUE     GPIO_PA0
            //#define GPIO_LED_GREEN                      GPIO_PE0
            #define GPIO_LED_WHITE    GPIO_PE5
            #define GPIO_LED_RED      GPIO_PE4

            #define PA0_FUNC          AS_GPIO
            //#define PE0_FUNC                            AS_GPIO
            #define PE5_FUNC          AS_GPIO
            #define PE4_FUNC          AS_GPIO

            #define PA0_OUTPUT_ENABLE 1
            //#define PE0_OUTPUT_ENABLE                   1
            #define PE5_OUTPUT_ENABLE 1
            #define PE4_OUTPUT_ENABLE 1
        #endif
    #endif


    /////////////////// DEEP SAVE FLG //////////////////////////////////
    #define USED_DEEP_ANA_REG PM_ANA_REG_POWER_ON_CLR_BUF1 //u8,can save 8 bit info when deep
    #define LOW_BATT_FLG      BIT(0)                       //if 1: low battery
    #define CONN_DEEP_FLG     BIT(1)                       //if 1: conn deep, 0: adv deep


    #if (BATT_CHECK_ENABLE)
        #define VBAT_CHANNEL_EN 0

        #if VBAT_CHANNEL_EN
        /**     The battery voltage sample range is 1.8~3.5V    **/
        #else
            /**     if the battery voltage > 3.6V, should take some external voltage divider    **/
            #define GPIO_BAT_DETECT   GPIO_PB1
            #define PB1_FUNC          AS_GPIO
            #define PB1_INPUT_ENABLE  0
            #define PB1_DATA_OUT      0
            #define ADC_INPUT_PIN_CHN ADC_GPIO_PB1
        #endif
    #endif


    /**
 *  @brief  GPIO definition for JTAG
 */
    #if (JTAG_DEBUG_DISABLE)
        //JTAG will cost some power
        #define PC4_FUNC            AS_GPIO
        #define PC5_FUNC            AS_GPIO
        #define PC6_FUNC            AS_GPIO
        #define PC7_FUNC            AS_GPIO

        #define PC4_INPUT_ENABLE    0
        #define PC5_INPUT_ENABLE    0
        #define PC6_INPUT_ENABLE    0
        #define PC7_INPUT_ENABLE    0

        #define PULL_WAKEUP_SRC_PC4 0
        #define PULL_WAKEUP_SRC_PC5 0
        #define PULL_WAKEUP_SRC_PC6 0
        #define PULL_WAKEUP_SRC_PC7 0
    #endif


    /**
 *  @brief  GPIO definition for debug_io
 */
    #if (DEBUG_GPIO_ENABLE)
        #define GPIO_CHN0         GPIO_PA1
        #define GPIO_CHN1         GPIO_PA2
        #define GPIO_CHN2         GPIO_PA3
        #define GPIO_CHN3         GPIO_PA4
        #define GPIO_CHN4         GPIO_PB1
        #define GPIO_CHN5         GPIO_PB2
        #define GPIO_CHN6         GPIO_PB3
        #define GPIO_CHN7         GPIO_PB4

        #define GPIO_CHN8         GPIO_PB5
        #define GPIO_CHN9         GPIO_PB6
        #define GPIO_CHN10        GPIO_PB7
        #define GPIO_CHN11        GPIO_PC0
        #define GPIO_CHN12        GPIO_PE0
        #define GPIO_CHN13        GPIO_PE1
        #define GPIO_CHN14        GPIO_PE2
        #define GPIO_CHN15        GPIO_PE3


        #define PA1_OUTPUT_ENABLE 1
        #define PA2_OUTPUT_ENABLE 1
        #define PA3_OUTPUT_ENABLE 1
        #define PA4_OUTPUT_ENABLE 1
        #define PB1_OUTPUT_ENABLE 1
        #define PB2_OUTPUT_ENABLE 1
        #define PB3_OUTPUT_ENABLE 1
        #define PB4_OUTPUT_ENABLE 1

        #define PB5_OUTPUT_ENABLE 1
        #define PB6_OUTPUT_ENABLE 1
        #define PB7_OUTPUT_ENABLE 1
        #define PC0_OUTPUT_ENABLE 1
        #define PE0_OUTPUT_ENABLE 1
        #define PE1_OUTPUT_ENABLE 1
        #define PE2_OUTPUT_ENABLE 1
        #define PE3_OUTPUT_ENABLE 1
    #endif //end of DEBUG_GPIO_ENABLE

    #if (DEBUG_CS_GPIO_ENABLE)
        #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
            #define GPIO_CHN0         GPIO_PA3
            #define GPIO_CHN1         GPIO_PA4
            #define GPIO_CHN2         GPIO_PB0
            #define GPIO_CHN3         GPIO_PB1
            #define GPIO_CHN4         GPIO_PB2
            #define GPIO_CHN5         GPIO_PB3
            #define GPIO_CHN6         GPIO_PB4
            #define GPIO_CHN7         GPIO_PB5

            #define GPIO_CHN8         GPIO_PB6
            #define GPIO_CHN9         GPIO_PB7
            #define GPIO_CHN10        GPIO_PC0
            #define GPIO_CHN11        GPIO_PC1
            #define GPIO_CHN12        GPIO_PC2
            #define GPIO_CHN13        GPIO_PC3
            #define GPIO_CHN14        GPIO_PC4
            #define GPIO_CHN15        GPIO_PC5

            #define PA3_OUTPUT_ENABLE 1
            #define PA4_OUTPUT_ENABLE 1
            #define PB0_OUTPUT_ENABLE 1
            #define PB2_OUTPUT_ENABLE 1
            #define PB3_OUTPUT_ENABLE 1
            #define PB4_OUTPUT_ENABLE 1
            #define PB5_OUTPUT_ENABLE 1

            #define PB6_OUTPUT_ENABLE 1
            #define PB7_OUTPUT_ENABLE 1
            #define PC0_OUTPUT_ENABLE 1
            #define PC1_OUTPUT_ENABLE 1
            #define PC2_OUTPUT_ENABLE 1
            #define PC3_OUTPUT_ENABLE 1
            #define PC4_OUTPUT_ENABLE 1
            #define PC5_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
            #define GPIO_CHN7         GPIO_PE4

            #define PE4_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
            #define GPIO_CHN7         GPIO_PB2

            #define PB2_OUTPUT_ENABLE 1
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
            #define GPIO_CHN7         GPIO_PB0

            #define PB0_OUTPUT_ENABLE 1
        #endif
    #endif //end of DEBUG_CS_GPIO_ENABLE


    /**
 *  @brief  GPIO definition for antenna switching
 */
    #ifdef ANTENNA_SWITCHING_AUTO_EN
        #if ((BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67) || (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102) || (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0))
            #if (1)
                /* N_AP=4 */
                #define NUM_ANT_SUPPORT       0x02
                #define MAX_ANT_PATHS_SUPPORT 0X04
            #else
                /* N_AP=2 */
                #define NUM_ANT_SUPPORT       0x01
                #define MAX_ANT_PATHS_SUPPORT 0X02
            #endif
        #else
            #define NUM_ANT_SUPPORT       0x01
            #define MAX_ANT_PATHS_SUPPORT 0X02
        #endif

        #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
            #if (ANTENNA_SWITCHING_AUTO_EN == 1)
                #define ANTENNA_SWITCHING_SEL_0_PIN GPIO_PE4
                #define ANTENNA_SWITCHING_SEL_1_PIN GPIO_PE5
                #define ANTENNA_SWITCHING_SEL_2_PIN GPIO_PF6
                #define ANTENNA_SWITCHING_CTRL_BASE 0x11111111
            #elif (ANTENNA_SWITCHING_AUTO_EN == 0)
                #define PE4_FUNC          AS_GPIO
                #define PE5_FUNC          AS_GPIO
                #define PF6_FUNC          AS_GPIO
                #define PE4_OUTPUT_ENABLE 1
                #define PE5_OUTPUT_ENABLE 1
                #define PF6_OUTPUT_ENABLE 1
                #define PE4_DATA_OUT      1
                #define PE5_DATA_OUT      0
                #define PF6_DATA_OUT      0
            #endif
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
            #if (ANTENNA_SWITCHING_AUTO_EN == 1)
                #define ANTENNA_SWITCHING_SEL_0_PIN GPIO_PD3
                #define ANTENNA_SWITCHING_SEL_1_PIN GPIO_PD4
                #define ANTENNA_SWITCHING_SEL_2_PIN GPIO_PD5
                #define ANTENNA_SWITCHING_CTRL_BASE 0x11111111
            #elif (ANTENNA_SWITCHING_AUTO_EN == 0)
                #define PD3_FUNC          AS_GPIO
                #define PD4_FUNC          AS_GPIO
                #define PD5_FUNC          AS_GPIO
                #define PD3_OUTPUT_ENABLE 1
                #define PD4_OUTPUT_ENABLE 1
                #define PD5_OUTPUT_ENABLE 1
                #define PD3_DATA_OUT      1
                #define PD4_DATA_OUT      0
                #define PD5_DATA_OUT      0
            #endif
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A67)
            #if (ANTENNA_SWITCHING_AUTO_EN == 1)
                #define ANTENNA_SWITCHING_SEL_0_PIN GPIO_PD0
                #define ANTENNA_SWITCHING_SEL_1_PIN GPIO_PD1
                #define ANTENNA_SWITCHING_SEL_2_PIN GPIO_PD3
                #define ANTENNA_SWITCHING_CTRL_BASE 0x11111111
            #elif (ANTENNA_SWITCHING_AUTO_EN == 0)
                #define PD0_FUNC          AS_GPIO
                #define PD1_FUNC          AS_GPIO
                #define PD3_FUNC          AS_GPIO
                #define PD0_OUTPUT_ENABLE 1
                #define PD1_OUTPUT_ENABLE 1
                #define PD3_OUTPUT_ENABLE 1
                #define PD0_DATA_OUT      1
                #define PD1_DATA_OUT      0
                #define PD3_DATA_OUT      0
            #endif
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
            #if (ANTENNA_SWITCHING_AUTO_EN == 1)
                #define ANTENNA_SWITCHING_SEL_0_PIN GPIO_PD0
                #define ANTENNA_SWITCHING_SEL_1_PIN GPIO_PD1
                #define ANTENNA_SWITCHING_SEL_2_PIN GPIO_PD3
                #define ANTENNA_SWITCHING_CTRL_BASE 0x22222222
            #elif (ANTENNA_SWITCHING_AUTO_EN == 0)
                #define PD0_FUNC          AS_GPIO
                #define PD1_FUNC          AS_GPIO
                #define PD3_FUNC          AS_GPIO
                #define PD0_OUTPUT_ENABLE 1
                #define PD1_OUTPUT_ENABLE 1
                #define PD3_OUTPUT_ENABLE 1
                #define PD0_DATA_OUT      0
                #define PD1_DATA_OUT      1
                #define PD3_DATA_OUT      0
            #endif
        #else
            #if ANTENNA_SWITCHING_AUTO_EN
                #error "Hardware not configure antenna switching, need set ANTENNA_SWITCHING_AUTO_EN 0 !!!"
            #endif
        #endif
    #endif

    #if DEBUG_SNIF_GPIO_ENABLE
        #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
            #define PA1_OUTPUT_ENABLE     1
            #define PA2_OUTPUT_ENABLE     1
            #define PA3_OUTPUT_ENABLE     1
            #define PA4_OUTPUT_ENABLE     1
            #define PB1_OUTPUT_ENABLE     1
            #define PB2_OUTPUT_ENABLE     1
            #define PB3_OUTPUT_ENABLE     1
            #define PB4_OUTPUT_ENABLE     1

            #define PB5_OUTPUT_ENABLE     1
            #define PB6_OUTPUT_ENABLE     1
            #define PB7_OUTPUT_ENABLE     1
            #define PC0_OUTPUT_ENABLE     1
            #define PE0_OUTPUT_ENABLE     1
            #define PE1_OUTPUT_ENABLE     1
            #define PE2_OUTPUT_ENABLE     1
            #define PE3_OUTPUT_ENABLE     1

            #define DBG_SNIF_CHN0         GPIO_PA1 /* rf_irq_Handler */
            #define DBG_SNIF_CHN1         GPIO_PA2 /* stimer_irq_handler */
            #define DBG_SNIF_CHN2         GPIO_PA3 /* uart_tx_done_irq */
            #define DBG_SNIF_CHN3         GPIO_PA4 /* uart_rx_done_irq */
            #define DBG_SNIF_CHN4         GPIO_PB1 /* uart_loop_tx */
            #define DBG_SNIF_CHN5         GPIO_PB2 /* hci_subevent */
            #define DBG_SNIF_CHN6         GPIO_PB3 /* parse_subevent */
            #define DBG_SNIF_CHN7         GPIO_PB4 /* hci_subevent_continue */

            #define DBG_SNIF_CHN8         GPIO_PB5 /* parse_subevent_continue */
            #define DBG_SNIF_CHN9         GPIO_PB6 /* app_cs_procedure_data */
            #define DBG_SNIF_CHN10        GPIO_PB7 /* snif_main_node_rx_data_process */
            #define DBG_SNIF_CHN11        GPIO_PC0
            #define DBG_SNIF_CHN12        GPIO_PE0
            #define DBG_SNIF_CHN13        GPIO_PE1
            #define DBG_SNIF_CHN14        GPIO_PE2
            #define DBG_SNIF_CHN15        GPIO_PE3

            #define DBG_SNIF_CHN0_LOW     gpio_write(DBG_SNIF_CHN0, 0)
            #define DBG_SNIF_CHN0_HIGH    gpio_write(DBG_SNIF_CHN0, 1)
            #define DBG_SNIF_CHN0_TOGGLE  gpio_toggle(DBG_SNIF_CHN0)

            #define DBG_SNIF_CHN1_LOW     gpio_write(DBG_SNIF_CHN1, 0)
            #define DBG_SNIF_CHN1_HIGH    gpio_write(DBG_SNIF_CHN1, 1)
            #define DBG_SNIF_CHN1_TOGGLE  gpio_toggle(DBG_SNIF_CHN1)

            #define DBG_SNIF_CHN2_LOW     gpio_write(DBG_SNIF_CHN2, 0)
            #define DBG_SNIF_CHN2_HIGH    gpio_write(DBG_SNIF_CHN2, 1)
            #define DBG_SNIF_CHN2_TOGGLE  gpio_toggle(DBG_SNIF_CHN2)

            #define DBG_SNIF_CHN3_LOW     gpio_write(DBG_SNIF_CHN3, 0)
            #define DBG_SNIF_CHN3_HIGH    gpio_write(DBG_SNIF_CHN3, 1)
            #define DBG_SNIF_CHN3_TOGGLE  gpio_toggle(DBG_SNIF_CHN3)

            #define DBG_SNIF_CHN4_LOW     gpio_write(DBG_SNIF_CHN4, 0)
            #define DBG_SNIF_CHN4_HIGH    gpio_write(DBG_SNIF_CHN4, 1)
            #define DBG_SNIF_CHN4_TOGGLE  gpio_toggle(DBG_SNIF_CHN4)

            #define DBG_SNIF_CHN5_LOW     gpio_write(DBG_SNIF_CHN5, 0)
            #define DBG_SNIF_CHN5_HIGH    gpio_write(DBG_SNIF_CHN5, 1)
            #define DBG_SNIF_CHN5_TOGGLE  gpio_toggle(DBG_SNIF_CHN5)

            #define DBG_SNIF_CHN6_LOW     gpio_write(DBG_SNIF_CHN6, 0)
            #define DBG_SNIF_CHN6_HIGH    gpio_write(DBG_SNIF_CHN6, 1)
            #define DBG_SNIF_CHN6_TOGGLE  gpio_toggle(DBG_SNIF_CHN6)

            #define DBG_SNIF_CHN7_LOW     gpio_write(DBG_SNIF_CHN7, 0)
            #define DBG_SNIF_CHN7_HIGH    gpio_write(DBG_SNIF_CHN7, 1)
            #define DBG_SNIF_CHN7_TOGGLE  gpio_toggle(DBG_SNIF_CHN7)

            #define DBG_SNIF_CHN8_LOW     gpio_write(DBG_SNIF_CHN8, 0)
            #define DBG_SNIF_CHN8_HIGH    gpio_write(DBG_SNIF_CHN8, 1)
            #define DBG_SNIF_CHN8_TOGGLE  gpio_toggle(DBG_SNIF_CHN8)

            #define DBG_SNIF_CHN9_LOW     gpio_write(DBG_SNIF_CHN9, 0)
            #define DBG_SNIF_CHN9_HIGH    gpio_write(DBG_SNIF_CHN9, 1)
            #define DBG_SNIF_CHN9_TOGGLE  gpio_toggle(DBG_SNIF_CHN9)

            #define DBG_SNIF_CHN10_LOW    gpio_write(DBG_SNIF_CHN10, 0)
            #define DBG_SNIF_CHN10_HIGH   gpio_write(DBG_SNIF_CHN10, 1)
            #define DBG_SNIF_CHN10_TOGGLE gpio_toggle(DBG_SNIF_CHN10)

            #define DBG_SNIF_CHN11_LOW    gpio_write(DBG_SNIF_CHN11, 0)
            #define DBG_SNIF_CHN11_HIGH   gpio_write(DBG_SNIF_CHN11, 1)
            #define DBG_SNIF_CHN11_TOGGLE gpio_toggle(DBG_SNIF_CHN11)

            #define DBG_SNIF_CHN12_LOW    gpio_write(DBG_SNIF_CHN12, 0)
            #define DBG_SNIF_CHN12_HIGH   gpio_write(DBG_SNIF_CHN12, 1)
            #define DBG_SNIF_CHN12_TOGGLE gpio_toggle(DBG_SNIF_CHN12)

            #define DBG_SNIF_CHN13_LOW    gpio_write(DBG_SNIF_CHN13, 0)
            #define DBG_SNIF_CHN13_HIGH   gpio_write(DBG_SNIF_CHN13, 1)
            #define DBG_SNIF_CHN13_TOGGLE gpio_toggle(DBG_SNIF_CHN13)

            #define DBG_SNIF_CHN14_LOW    gpio_write(DBG_SNIF_CHN14, 0)
            #define DBG_SNIF_CHN14_HIGH   gpio_write(DBG_SNIF_CHN14, 1)
            #define DBG_SNIF_CHN14_TOGGLE gpio_toggle(DBG_SNIF_CHN14)

            #define DBG_SNIF_CHN15_LOW    gpio_write(DBG_SNIF_CHN15, 0)
            #define DBG_SNIF_CHN15_HIGH   gpio_write(DBG_SNIF_CHN15, 1)
            #define DBG_SNIF_CHN15_TOGGLE gpio_toggle(DBG_SNIF_CHN15)
        #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
            #define PA5_OUTPUT_ENABLE 1
            #define PA2_OUTPUT_ENABLE 1
            #define PE6_OUTPUT_ENABLE 1
            #define PA1_OUTPUT_ENABLE 1
            #define PA0_OUTPUT_ENABLE 1
            #define PE4_OUTPUT_ENABLE 1

            #define DBG_SNIF_CHN6     GPIO_PA5 /* parse_subevent */
            #define DBG_SNIF_CHN8     GPIO_PA2 /* parse_subevent continue */
            #define DBG_SNIF_CHN10    GPIO_PE6 /* data parse */
            #define DBG_SNIF_CHN11    GPIO_PA1
            #define DBG_SNIF_CHN12    GPIO_PA0
            #define DBG_SNIF_CHN15    GPIO_PE4 /* CAN Interrupt */

            #define DBG_SNIF_CHN0_LOW
            #define DBG_SNIF_CHN0_HIGH
            #define DBG_SNIF_CHN0_TOGGLE

            #define DBG_SNIF_CHN1_LOW
            #define DBG_SNIF_CHN1_HIGH
            #define DBG_SNIF_CHN1_TOGGLE

            #define DBG_SNIF_CHN2_LOW
            #define DBG_SNIF_CHN2_HIGH
            #define DBG_SNIF_CHN2_TOGGLE

            #define DBG_SNIF_CHN3_LOW
            #define DBG_SNIF_CHN3_HIGH
            #define DBG_SNIF_CHN3_TOGGLE

            #define DBG_SNIF_CHN4_LOW
            #define DBG_SNIF_CHN4_HIGH
            #define DBG_SNIF_CHN4_TOGGLE

            #define DBG_SNIF_CHN5_LOW
            #define DBG_SNIF_CHN5_HIGH
            #define DBG_SNIF_CHN5_TOGGLE

            #define DBG_SNIF_CHN6_LOW    gpio_write(DBG_SNIF_CHN6, 0)
            #define DBG_SNIF_CHN6_HIGH   gpio_write(DBG_SNIF_CHN6, 1)
            #define DBG_SNIF_CHN6_TOGGLE gpio_toggle(DBG_SNIF_CHN6)

            #define DBG_SNIF_CHN7_LOW
            #define DBG_SNIF_CHN7_HIGH
            #define DBG_SNIF_CHN7_TOGGLE

            #define DBG_SNIF_CHN8_LOW    gpio_write(DBG_SNIF_CHN8, 0)
            #define DBG_SNIF_CHN8_HIGH   gpio_write(DBG_SNIF_CHN8, 1)
            #define DBG_SNIF_CHN8_TOGGLE gpio_toggle(DBG_SNIF_CHN8)

            #define DBG_SNIF_CHN9_LOW
            #define DBG_SNIF_CHN9_HIGH
            #define DBG_SNIF_CHN9_TOGGLE

            #define DBG_SNIF_CHN10_LOW    gpio_write(DBG_SNIF_CHN10, 0)
            #define DBG_SNIF_CHN10_HIGH   gpio_write(DBG_SNIF_CHN10, 1)
            #define DBG_SNIF_CHN10_TOGGLE gpio_toggle(DBG_SNIF_CHN10)

            #define DBG_SNIF_CHN11_LOW    gpio_write(DBG_SNIF_CHN11, 0)
            #define DBG_SNIF_CHN11_HIGH   gpio_write(DBG_SNIF_CHN11, 1)
            #define DBG_SNIF_CHN11_TOGGLE gpio_write(DBG_SNIF_CHN11)

            #define DBG_SNIF_CHN12_LOW    gpio_write(DBG_SNIF_CHN12, 0)
            #define DBG_SNIF_CHN12_HIGH   gpio_write(DBG_SNIF_CHN12, 1)
            #define DBG_SNIF_CHN12_TOGGLE gpio_write(DBG_SNIF_CHN12)

            #define DBG_SNIF_CHN13_LOW
            #define DBG_SNIF_CHN13_HIGH
            #define DBG_SNIF_CHN13_TOGGLE

            #define DBG_SNIF_CHN14_LOW
            #define DBG_SNIF_CHN14_HIGH
            #define DBG_SNIF_CHN14_TOGGLE

            #define DBG_SNIF_CHN15_LOW    gpio_write(DBG_SNIF_CHN15, 0)
            #define DBG_SNIF_CHN15_HIGH   gpio_write(DBG_SNIF_CHN15, 1)
            #define DBG_SNIF_CHN15_TOGGLE gpio_toggle(DBG_SNIF_CHN15)
        #elif (BOARD_SELECT == BOARD_9223B_DUAL_ANTENNA_C1T325A102)
            #define PA5_OUTPUT_ENABLE 1
            #define PA2_OUTPUT_ENABLE 1
            #define PE6_OUTPUT_ENABLE 1
            #define PA1_OUTPUT_ENABLE 1
            #define PA0_OUTPUT_ENABLE 1
            #define PE4_OUTPUT_ENABLE 1

            #define DBG_SNIF_CHN6     GPIO_PA6 /* parse_subevent */
            #define DBG_SNIF_CHN8     GPIO_PA5 /* parse_subevent continue */
            #define DBG_SNIF_CHN10    GPIO_PA1 /* data parse */
            #define DBG_SNIF_CHN15    GPIO_PA2 /* CAN Interrupt */

            #define DBG_SNIF_CHN0_LOW
            #define DBG_SNIF_CHN0_HIGH
            #define DBG_SNIF_CHN0_TOGGLE

            #define DBG_SNIF_CHN1_LOW
            #define DBG_SNIF_CHN1_HIGH
            #define DBG_SNIF_CHN1_TOGGLE

            #define DBG_SNIF_CHN2_LOW
            #define DBG_SNIF_CHN2_HIGH
            #define DBG_SNIF_CHN2_TOGGLE

            #define DBG_SNIF_CHN3_LOW
            #define DBG_SNIF_CHN3_HIGH
            #define DBG_SNIF_CHN3_TOGGLE

            #define DBG_SNIF_CHN4_LOW
            #define DBG_SNIF_CHN4_HIGH
            #define DBG_SNIF_CHN4_TOGGLE

            #define DBG_SNIF_CHN5_LOW
            #define DBG_SNIF_CHN5_HIGH
            #define DBG_SNIF_CHN5_TOGGLE

            #define DBG_SNIF_CHN6_LOW    gpio_write(DBG_SNIF_CHN6, 0)
            #define DBG_SNIF_CHN6_HIGH   gpio_write(DBG_SNIF_CHN6, 1)
            #define DBG_SNIF_CHN6_TOGGLE gpio_toggle(DBG_SNIF_CHN6)

            #define DBG_SNIF_CHN7_LOW
            #define DBG_SNIF_CHN7_HIGH
            #define DBG_SNIF_CHN7_TOGGLE

            #define DBG_SNIF_CHN8_LOW    gpio_write(DBG_SNIF_CHN8, 0)
            #define DBG_SNIF_CHN8_HIGH   gpio_write(DBG_SNIF_CHN8, 1)
            #define DBG_SNIF_CHN8_TOGGLE gpio_toggle(DBG_SNIF_CHN8)

            #define DBG_SNIF_CHN9_LOW
            #define DBG_SNIF_CHN9_HIGH
            #define DBG_SNIF_CHN9_TOGGLE

            #define DBG_SNIF_CHN10_LOW    gpio_write(DBG_SNIF_CHN10, 0)
            #define DBG_SNIF_CHN10_HIGH   gpio_write(DBG_SNIF_CHN10, 1)
            #define DBG_SNIF_CHN10_TOGGLE gpio_toggle(DBG_SNIF_CHN10)

            #define DBG_SNIF_CHN11_LOW
            #define DBG_SNIF_CHN11_HIGH
            #define DBG_SNIF_CHN11_TOGGLE

            #define DBG_SNIF_CHN12_LOW
            #define DBG_SNIF_CHN12_HIGH
            #define DBG_SNIF_CHN12_TOGGLE

            #define DBG_SNIF_CHN13_LOW
            #define DBG_SNIF_CHN13_HIGH
            #define DBG_SNIF_CHN13_TOGGLE

            #define DBG_SNIF_CHN14_LOW
            #define DBG_SNIF_CHN14_HIGH
            #define DBG_SNIF_CHN14_TOGGLE

            #define DBG_SNIF_CHN15_LOW    gpio_write(DBG_SNIF_CHN15, 0)
            #define DBG_SNIF_CHN15_HIGH   gpio_write(DBG_SNIF_CHN15, 1)
            #define DBG_SNIF_CHN15_TOGGLE gpio_toggle(DBG_SNIF_CHN15)
        #endif
    #else
        #define DBG_SNIF_CHN0_LOW
        #define DBG_SNIF_CHN0_HIGH
        #define DBG_SNIF_CHN0_TOGGLE

        #define DBG_SNIF_CHN1_LOW
        #define DBG_SNIF_CHN1_HIGH
        #define DBG_SNIF_CHN1_TOGGLE

        #define DBG_SNIF_CHN2_LOW
        #define DBG_SNIF_CHN2_HIGH
        #define DBG_SNIF_CHN2_TOGGLE

        #define DBG_SNIF_CHN3_LOW
        #define DBG_SNIF_CHN3_HIGH
        #define DBG_SNIF_CHN3_TOGGLE

        #define DBG_SNIF_CHN4_LOW
        #define DBG_SNIF_CHN4_HIGH
        #define DBG_SNIF_CHN4_TOGGLE

        #define DBG_SNIF_CHN5_LOW
        #define DBG_SNIF_CHN5_HIGH
        #define DBG_SNIF_CHN5_TOGGLE

        #define DBG_SNIF_CHN6_LOW
        #define DBG_SNIF_CHN6_HIGH
        #define DBG_SNIF_CHN6_TOGGLE

        #define DBG_SNIF_CHN7_LOW
        #define DBG_SNIF_CHN7_HIGH
        #define DBG_SNIF_CHN7_TOGGLE

        #define DBG_SNIF_CHN8_LOW
        #define DBG_SNIF_CHN8_HIGH
        #define DBG_SNIF_CHN8_TOGGLE

        #define DBG_SNIF_CHN9_LOW
        #define DBG_SNIF_CHN9_HIGH
        #define DBG_SNIF_CHN9_TOGGLE

        #define DBG_SNIF_CHN10_LOW
        #define DBG_SNIF_CHN10_HIGH
        #define DBG_SNIF_CHN10_TOGGLE

        #define DBG_SNIF_CHN11_LOW
        #define DBG_SNIF_CHN11_HIGH
        #define DBG_SNIF_CHN11_TOGGLE

        #define DBG_SNIF_CHN12_LOW
        #define DBG_SNIF_CHN12_HIGH
        #define DBG_SNIF_CHN12_TOGGLE

        #define DBG_SNIF_CHN13_LOW
        #define DBG_SNIF_CHN13_HIGH
        #define DBG_SNIF_CHN13_TOGGLE

        #define DBG_SNIF_CHN14_LOW
        #define DBG_SNIF_CHN14_HIGH
        #define DBG_SNIF_CHN14_TOGGLE

        #define DBG_SNIF_CHN15_LOW
        #define DBG_SNIF_CHN15_HIGH
        #define DBG_SNIF_CHN15_TOGGLE
    #endif

    #include "../common/default_config.h"
#endif
