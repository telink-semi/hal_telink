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

#if (MONITOR_ROLE_SELECT == OBSERVER)
    #define ACL_CENTRAL_MAX_NUM 4 // ACL central maximum number
    #define ACL_PERIPHR_MAX_NUM 0 // ACL peripheral maximum number

    ///////////////////////// Feature Configuration////////////////////////////////////////////////
    #define BLE_APP_PM_ENABLE 0

    #define BATT_CHECK_ENABLE 0


    /* Flash Protection:
 * 1. Flash protection is enabled by default in SDK. User must enable this function on their final mass production application.
 * 2. User should use "Unlock" command in Telink BDT tool for Flash access during development and debugging phase.
 * 3. Flash protection demonstration in SDK is a reference design based on sample code. Considering that user's final application may
 *    different from sample code, for example, user's final firmware size is bigger, or user have a different OTA design, or user need
 *    store more data in some other area of Flash, all these differences imply that Flash protection reference design in SDK can not
 *    be directly used on user's mass production application without any change. User should refer to sample code, understand the
 *    principles and methods, then change and implement a more appropriate mechanism according to their application if needed.
 */
    #define APP_FLASH_PROTECTION_ENABLE 1

    #define APP_EMI_TEST_ENABLE         0

    /*! OS settings */
    #define FREERTOS_ENABLE         0
    #define OS_SUP_EN               0
    #define OS_SEPARATE_STACK_SPACE 0 //Separate the task stack and interrupt stack space
    ///////////////////////// UI Configuration ////////////////////////////////////////////////////
    #define UI_LED_ENABLE      1
    #define UI_KEYBOARD_ENABLE 1


    ///////////////////////// Hardware board selection ////////////////////////////////////////////////
    #define BOARD_9528A_EVK_C1T266A20_V1_3 1
    #define BOARD_9223A_EVK_C1T289A67_V1_0 2
    #define BOARD_9223B_EVK_C1T325A67_V1_0 3
    #define BOARD_9223B_EVK_C1T325A20_V1_0 4
    #define BOARD_SELECT                   BOARD_9223B_EVK_C1T325A20_V1_0


    ///////////////////////// Sniffer sub node Configuration ////////////////////////////////////////////////
    #define LOCAL_SNIFFER_INDEX_0           0
    #define LOCAL_SNIFFER_INDEX_1           1
    #define LOCAL_SNIFFER_INDEX_2           2
    #define LOCAL_SNIFFER_INDEX_3           3
    #define LOCAL_SNIFFER_INDEX_4           4
    #define LOCAL_SNIFFER_INDEX_5           5
    #define APP_SNIFFER_INDEX               LOCAL_SNIFFER_INDEX_0

    #define APP_LE_LEGACY_SCAN_EN           1
    #define APP_LE_LEGACY_ADV_EN            0
    #define DEBUG_SNIFFER_REPORT_INSTANT_EN 0

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
    #endif
    #if (APP_TRANSPORT_UART_ENABLE && APP_TRANSPORT_CANFD_ENABLE)
        #error "can not use more than two transmission modes at the same time !!!"
    #endif


    ///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
    #define DEBUG_GPIO_ENABLE    0
    #define JTAG_DEBUG_DISABLE   1 //if use JTAG, change this

    #define TLKAPI_DEBUG_ENABLE  1
    #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_GSUART
    #if (BOARD_SELECT == BOARD_9528A_EVK_C1T266A20_V1_3)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PD4
    #elif (BOARD_SELECT == BOARD_9223A_EVK_C1T289A67_V1_0)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PB1
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A67_V1_0)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PA6
    #elif (BOARD_SELECT == BOARD_9223B_EVK_C1T325A20_V1_0)
        #define TLKAPI_DEBUG_GPIO_PIN GPIO_PA6
    #endif

    #define APP_LOG_EN            1
    #define APP_CONTR_EVT_LOG_EN  1 //controller event
    #define APP_HOST_EVT_LOG_EN   1
    #define APP_KEY_LOG_EN        1
    #define APP_FLASH_INIT_LOG_EN 1
    #define APP_FLASH_PROT_LOG_EN 1
    #define APP_CAN_LOG_EN        1
    #define APP_SNIF_LOG_EN       1


    ///////////////////// UART variables ///////////////////////////////////////////
    #define UART0_MODULE          0 //UART0
    #define UART_MODULE_SEL       UART0_MODULE
    #define UART_MODULE_BAUDRATE  500000
    #define UART_BAUD_RATE        UART_MODULE_BAUDRATE
    #define UART_DMA_CHANNEL_RX   DMA5
    #define UART_DMA_CHANNEL_TX   DMA6
    #define UART_TX_WAIT_MAX_BYTE 100
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
    #endif
    #define SPP_RXFIFO_SIZE 64
    #define SPP_RXFIFO_NUM  16
    #define SPP_TXFIFO_SIZE 48
    #define SPP_TXFIFO_NUM  8
    #if (UART_TX_WAIT_MAX_BYTE < (SPP_TXFIFO_SIZE + 10))
        #error "UART_TX_WAIT_MAX_BYTE < (SPP_TXFIFO_SIZE + 10) !!!"
    #endif


    /**
 *  @brief  Definition gpio for keyboard
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


    #if FREERTOS_ENABLE
    /////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
        #define APP_REAL_TIME_PRINTF         1


        #define traceAPP_LED_Task_Toggle()   //gpio_toggle(GPIO_CH01);
        #define traceAPP_BLE_Task_BEGIN()    //gpio_write(GPIO_CH02,1);
        #define traceAPP_BLE_Task_END()      //gpio_write(GPIO_CH02,0);
        #define traceAPP_KEY_Task_BEGIN()    //gpio_write(GPIO_CH03,1);
        #define traceAPP_KEY_Task_END()      //gpio_write(GPIO_CH03,0);
        #define traceAPP_BAT_Task_BEGIN()    //gpio_write(GPIO_CH04,1);
        #define traceAPP_BAT_Task_END()      //gpio_write(GPIO_CH04,0);

        #define traceAPP_MUTEX_Task_BEGIN()  //gpio_write(GPIO_CH05,1);
        #define traceAPP_MUTEX_Task_END()    //gpio_write(GPIO_CH05,0);

        #define tracePort_IrqHandler_BEGIN() //gpio_write(GPIO_CH06,1);
        #define tracePort_IrqHandler_END()   //gpio_write(GPIO_CH06,0);

    #endif

    #include "../common/default_config.h"
#endif
