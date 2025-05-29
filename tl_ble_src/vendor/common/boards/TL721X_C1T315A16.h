/********************************************************************************************************
 * @file    TL721X_C1T315A20.h
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

#ifndef VENDOR_COMMON_BOARDS_TL721X_C1T315A16_H_
#define VENDOR_COMMON_BOARDS_TL721X_C1T315A16_H_

/**
 *  @brief  LED Configuration
 */
#if UI_LED_ENABLE
    /**
    *  @brief  Definition gpio for led
    */

    #define GPIO_LED_GREEN    GPIO_PF7
    #define GPIO_LED_RED      GPIO_PA0
    #define GPIO_LED_YELLOW   GPIO_PA1
    #define GPIO_LED_BLUE     GPIO_PA2
    #define GPIO_LED_WHITE    GPIO_PA3
    #define GPIO_LED_ORANGE   GPIO_PA4

    #define PF7_FUNC          AS_GPIO
    #define PA0_FUNC          AS_GPIO
    #define PA1_FUNC          AS_GPIO
    #define PA2_FUNC          AS_GPIO
    #define PA3_FUNC          AS_GPIO
    #define PA4_FUNC          AS_GPIO

    #define PF7_OUTPUT_ENABLE 1
    #define PA0_OUTPUT_ENABLE 1
    #define PA1_OUTPUT_ENABLE 1
    #define PA2_OUTPUT_ENABLE 1
    #define PA3_OUTPUT_ENABLE 1
    #define PA4_OUTPUT_ENABLE 1

    #define LED_ON_LEVEL      1 //gpio output high voltage to turn on led

#endif

#ifndef JTAG_DEBUG_DISABLE
    #define JTAG_DEBUG_DISABLE 1
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

#define TLKAPI_DEBUG_GPIO_PIN GPIO_PF0

#endif /* VENDOR_COMMON_BOARDS_TL721X_C1T315A20_H_ */
