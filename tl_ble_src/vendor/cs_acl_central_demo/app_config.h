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


#define   DBG_CS_LOG                                1

#define ACL_CENTRAL_MAX_NUM                         1 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE                      1   //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE                      1   //1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE               0   //simple service discovery for ACL central


#define BATT_CHECK_ENABLE                           0
#define APP_FLASH_PROTECTION_ENABLE                 0
#ifndef CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN
#define CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN       0
#endif

#define CS_PROCEDURE_EXCHANGE                       1
#define CS_PROCEDURE_CMD_TRIG                       1


///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define BOARD_B92_EVK_C1T266A20                     1
#define BOARD_B92_EVB_C1T297A20                     2
#define HARDWARE_BOARD_SELECT                       BOARD_B92_EVB_C1T297A20

#if  (HARDWARE_BOARD_SELECT == BOARD_B92_EVB_C1T297A20)
#define GPIO_WORK_VOLTAGE                           GPIO_VOLTAGE_1V8 //change
#else
#define GPIO_WORK_VOLTAGE                           GPIO_VOLTAGE_3V3 //change
#endif

#define UI_KEYBOARD_ENABLE                          0
#define UI_BUTTON_ENABLE                            0
#define UI_LED_ENABLE                               1
#define UI_CONTROL_ENABLE                           1


///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#define DEBUG_GPIO_ENABLE                           0

#define CS_ACCURACY_DEBUG_ENABLE                    1

#define TLKAPI_DEBUG_ENABLE                         1
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_UDB

#define APP_LOG_EN                                  0
#define APP_CONTR_EVT_LOG_EN                        1   //controller event
#define APP_PAIR_LOG_EN                             1

#define JTAG_DEBUG_DISABLE                          1  //if use JTAG, change this

#define TLKAPI_DEBUG_FIFO_NUM                       128
#define TLKAPI_DEBUG_FIFO_SIZE                      288
/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                   PM_ANA_REG_POWER_ON_CLR_BUF1 //u8,can save 8 bit info when deep
#define LOW_BATT_FLG                        BIT(0) //if 1: low battery
#define CONN_DEEP_FLG                       BIT(1) //if 1: conn deep, 0: adv deep


/**
 *  @brief  GPIO definition for LED
 */
#if (UI_LED_ENABLE && HARDWARE_BOARD_SELECT)
    #define LED_ON_LEVEL                        1       //gpio output high voltage to turn on led
    #if (HARDWARE_BOARD_SELECT == BOARD_B92_EVK_C1T266A20)
        #define GPIO_LED_BLUE                       GPIO_PD0
        #define GPIO_LED_GREEN                      GPIO_PD1
        #define GPIO_LED_WHITE                      GPIO_PE6
        #define GPIO_LED_RED                        GPIO_PE7

        #define PD0_FUNC                            AS_GPIO
        #define PD1_FUNC                            AS_GPIO
        #define PE6_FUNC                            AS_GPIO
        #define PE7_FUNC                            AS_GPIO

        #define PD0_OUTPUT_ENABLE                   1
        #define PD1_OUTPUT_ENABLE                   1
        #define PE6_OUTPUT_ENABLE                   1
        #define PE7_OUTPUT_ENABLE                   1
    #elif (HARDWARE_BOARD_SELECT == BOARD_B92_EVB_C1T297A20)
        #define GPIO_LED_BLUE                       GPIO_PE6
        #define GPIO_LED_GREEN                      GPIO_PE7
        #define GPIO_LED_WHITE                      GPIO_PF0
        #define GPIO_LED_RED                        GPIO_PF1

        #define PE6_FUNC                            AS_GPIO
        #define PE7_FUNC                            AS_GPIO
        #define PF0_FUNC                            AS_GPIO
        #define PF1_FUNC                            AS_GPIO

        #define PE6_OUTPUT_ENABLE                   1
        #define PE7_OUTPUT_ENABLE                   1
        #define PF0_OUTPUT_ENABLE                   1
        #define PF1_OUTPUT_ENABLE                   1
    #endif
#endif


#if (BATT_CHECK_ENABLE)
#define VBAT_CHANNEL_EN                     0

#if VBAT_CHANNEL_EN
    /**     The battery voltage sample range is 1.8~3.5V    **/
#else
    /**     if the battery voltage > 3.6V, should take some external voltage divider    **/
    #define GPIO_BAT_DETECT             GPIO_PB1
    #define PB1_FUNC                        AS_GPIO
    #define PB1_INPUT_ENABLE                0
    #define PB1_DATA_OUT                    0
    #define ADC_INPUT_PIN_CHN               ADC_GPIO_PB1
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
        #define GPIO_CHN0                           GPIO_PA1
        #define GPIO_CHN1                           GPIO_PA2
        #define GPIO_CHN2                           GPIO_PA3
        #define GPIO_CHN3                           GPIO_PA4
        #define GPIO_CHN4                           GPIO_PB1
        #define GPIO_CHN5                           GPIO_PB2
        #define GPIO_CHN6                           GPIO_PB3
        #define GPIO_CHN7                           GPIO_PB4

        #define GPIO_CHN8                           GPIO_PB5
        #define GPIO_CHN9                           GPIO_PB6
        #define GPIO_CHN10                          GPIO_PB7
        #define GPIO_CHN11                          GPIO_PC0
        #define GPIO_CHN12                          GPIO_PE0
        #define GPIO_CHN13                          GPIO_PE1
        #define GPIO_CHN14                          GPIO_PE2
        #define GPIO_CHN15                          GPIO_PE3


        #define PA1_OUTPUT_ENABLE                   1
        #define PA2_OUTPUT_ENABLE                   1
        #define PA3_OUTPUT_ENABLE                   1
        #define PA4_OUTPUT_ENABLE                   1
        #define PB1_OUTPUT_ENABLE                   1
        #define PB2_OUTPUT_ENABLE                   1
        #define PB3_OUTPUT_ENABLE                   1
        #define PB4_OUTPUT_ENABLE                   1

        #define PB5_OUTPUT_ENABLE                   1
        #define PB6_OUTPUT_ENABLE                   1
        #define PB7_OUTPUT_ENABLE                   1
        #define PC0_OUTPUT_ENABLE                   1
        #define PE0_OUTPUT_ENABLE                   1
        #define PE1_OUTPUT_ENABLE                   1
        #define PE2_OUTPUT_ENABLE                   1
        #define PE3_OUTPUT_ENABLE                   1
#endif  //end of DEBUG_GPIO_ENABLE



#include "../common/default_config.h"
