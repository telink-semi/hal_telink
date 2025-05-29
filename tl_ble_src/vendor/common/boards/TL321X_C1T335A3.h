/********************************************************************************************************
 * @file    TL321X_C1T335A20.h
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

#ifndef VENDOR_COMMON_BOARDS_TL321X_C1T335A3_H_
#define VENDOR_COMMON_BOARDS_TL321X_C1T335A3_H_



/**
 *  @brief  Keyboard Configuration
 */
#if (UI_KEYBOARD_ENABLE)
    #undef UI_KEYBOARD_ENABLE
    #define UI_KEYBOARD_ENABLE                  0
#endif

/**
 *  @brief  LED Configuration
 */
#if UI_LED_ENABLE
    /**
     *  @brief  Definition gpio for led
     */
    #define GPIO_LED_RED                        GPIO_PD0
    #define GPIO_LED_GREEN                      GPIO_PB0
    #define GPIO_LED_WHITE                      GPIO_PB3
    #define GPIO_LED_BLUE                       GPIO_PB1

    #define PD0_FUNC                            AS_GPIO
    #define PB0_FUNC                            AS_GPIO
    #define PB1_FUNC                            AS_GPIO
    #define PB3_FUNC                            AS_GPIO

    #define PD0_OUTPUT_ENABLE                   1
    #define PB0_OUTPUT_ENABLE                   1
    #define PB1_OUTPUT_ENABLE                   1
    #define PB3_OUTPUT_ENABLE                   1

    #define LED_ON_LEVEL                        1       //gpio output high voltage to turn on led

#endif

#ifndef  JTAG_DEBUG_DISABLE
#define  JTAG_DEBUG_DISABLE   1
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
 *  @brief  Battery_check Configuration
 */
#if (BATT_CHECK_ENABLE)
    #define VBAT_CHANNEL_EN                     0

    #if VBAT_CHANNEL_EN
        /**     The battery voltage sample range is 1.8~3.5V    **/
    #else
        /**     if the battery voltage > 3.6V, should take some external voltage divider    **/
        #define ADC_INPUT_PIN_CHN               ADC_GPIO_PB0
    #endif
#endif

/**
 *  @brief  GPIO definition for debug_io
 */
#if (DEBUG_GPIO_ENABLE)
    #undef DEBUG_GPIO_ENABLE
    #define DEBUG_GPIO_ENABLE                   0
#endif  //end of DEBUG_GPIO_ENABLE

#define TLKAPI_DEBUG_GPIO_PIN                   GPIO_PA1

#endif /* VENDOR_COMMON_BOARDS_TL321X_C1T331A3_H_ */
