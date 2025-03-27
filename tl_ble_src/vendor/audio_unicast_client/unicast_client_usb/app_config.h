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

#include "../unicast_client_config.h"

#if (UNICAST_CLIENT_SELECT == UNICAST_CLIENT_USB)

#if (MCU_CORE_TYPE == MCU_CORE_B91)
#define TLSR9518EVK               1
#define TLSR9518DONGLE            2

#define HARDWARE_TYPE             TLSR9518EVK
#endif

#define APP_SCENE_TWS                               0
#define APP_SCENE_HEADSET                           1

#define APP_AUDIO_SCENE                             APP_SCENE_TWS

#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
#define ACL_CENTRAL_MAX_NUM                         2 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number
#define LC3_DECODE_CHANNEL_COUNT                    2
#define LC3_ENCODE_CHANNEL_COUNT                    2
#define APP_AUDIO_ASRC_EN                           0
#define HEAP_MEM_SIZE_CFG                           6000
#define MIC_CHANNEL_COUNT                           2
#define ID_PRODUCT_BASE                             0x6001
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
#define ACL_CENTRAL_MAX_NUM                         1 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number
#define LC3_DECODE_CHANNEL_COUNT                    1
#define LC3_ENCODE_CHANNEL_COUNT                    2
#define APP_AUDIO_ASRC_EN                           0
#define HEAP_MEM_SIZE_CFG                           5000
#define MIC_CHANNEL_COUNT                           1
#define ID_PRODUCT_BASE                             0x6002
#endif


//usb define start
#define VCD_EN                  0
#define MODULE_USB_ENABLE       1
#define ID_VENDOR               0x248a
#define STRING_VENDOR           L"Telink"
#define STRING_PRODUCT          L"Telink LEA Dongle"
#define STRING_SERIAL           L"TLSR9"

#define MIC_RESOLUTION_BIT      16
#define MIC_SAMPLE_RATE         16000
#define MIC_ENOCDER_ENABLE      0

#define SPK_RESOLUTION_BIT      16
#define SPK_SAMPLE_RATE         48000
#define SPK_CHANNEL_COUNT       2

#define PA5_FUNC                AS_USB_DM
#define PA6_FUNC                AS_USB_DP
#define PA5_INPUT_ENABLE        1
#define PA6_INPUT_ENABLE        1

#define USB_SPEAKER_ENABLE      1
#define USB_MIC_ENABLE          1

#define USB_RESUME_HOST         1
//usb define end


#define APP_LOG_EN                                  1
#define TLKAPI_DEBUG_ENABLE                         1
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_UART
#define BLC_PM_DEEP_RETENTION_MODE_EN                    0
#define TLKAPI_DEBUG_FIFO_NUM                            16

///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE                      1   //1 for smp,  0 no security
#define ACL_CENTRAL_SMP_ENABLE                      1   //1 for smp,  0 no security

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define UI_LED_ENABLE                               1
#define UI_KEYBOARD_ENABLE                          1

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////

#define DEBUG_GPIO_ENABLE                           0
#define JTAG_DEBUG_DISABLE                          1

#define TLKAPI_DEBUG_FIFO_SIZE                      160
#define TLKAPI_DEBUG_FIFO_NUM                       32

/**
 *  @brief  GPIO definition for keyboard
 */
#if (UI_KEYBOARD_ENABLE)
    //---------------  KeyMatrix PC2/PC0/PC3/PC1 -----------------------------
    #define MATRIX_ROW_PULL                 PM_PIN_PULLDOWN_100K
    #define MATRIX_COL_PULL                 PM_PIN_PULLUP_10K

    #define KB_LINE_HIGH_VALID              0   //drive pin output 0 when scan key, scan pin read 0 is valid

    #define BTN_PAIR                        0x01
    #define BTN_UNPAIR                      0x02

    #define CR_VOL_UP                       0xf0  ////
    #define CR_VOL_DN                       0xf1

    /**
     *  @brief  Normal keyboard map
     */
    #define KB_MAP_NORMAL                   {   {CR_VOL_DN,     BTN_PAIR},   \
                                                {CR_VOL_UP,     BTN_UNPAIR}, }

    //////////////////// KEY CONFIG (EVK board) ///////////////////////////
    #if (MCU_CORE_TYPE == MCU_CORE_B91)

        #if(HARDWARE_TYPE == TLSR9518DONGLE)
        #define SW2_GPIO                        GPIO_PB2
        #define SW7_GPIO                        GPIO_PB3

        //drive pin as gpio
        #define PB2_FUNC                        AS_GPIO
        #define PB3_FUNC                        AS_GPIO

        //drive pin need 10k pullup
        #define PULL_WAKEUP_SRC_PB2             PM_PIN_PULLUP_10K
        #define PULL_WAKEUP_SRC_PB3             PM_PIN_PULLUP_10K

        //drive pin open input to read gpio wakeup level
        #define PB2_INPUT_ENABLE                1
        #define PB3_INPUT_ENABLE                1

        #define USER_DEBUG_ENABLE               0 // APP USE
        #elif(HARDWARE_TYPE == TLSR9518EVK)
        #define KB_DRIVE_PINS                   {GPIO_PC2, GPIO_PC0}
        #define KB_SCAN_PINS                    {GPIO_PC3, GPIO_PC1}

        //drive pin as gpio
        #define PC2_FUNC                        AS_GPIO
        #define PC0_FUNC                        AS_GPIO

        //drive pin need 100K pulldown
        #define PULL_WAKEUP_SRC_PC2             MATRIX_ROW_PULL
        #define PULL_WAKEUP_SRC_PC0             MATRIX_ROW_PULL

        //drive pin open input to read gpio wakeup level
        #define PC2_INPUT_ENABLE                1
        #define PC0_INPUT_ENABLE                1

        //scan pin as gpio
        #define PC3_FUNC                        AS_GPIO
        #define PC1_FUNC                        AS_GPIO

        //scan  pin need 10K pullup
        #define PULL_WAKEUP_SRC_PC3             MATRIX_COL_PULL
        #define PULL_WAKEUP_SRC_PC1             MATRIX_COL_PULL

        //scan pin open input to read gpio level
        #define PC3_INPUT_ENABLE                1
        #define PC1_INPUT_ENABLE                1

        #define USER_DEBUG_ENABLE               1// APP USE
        #endif//#if(HARDWARE_TYPE == ...)
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
        #define KB_DRIVE_PINS                   {GPIO_PD6, GPIO_PF6}
        #define KB_SCAN_PINS                    {GPIO_PD7, GPIO_PD2}

        //scan pin as gpio
        #define PD6_FUNC                        AS_GPIO
        #define PF6_FUNC                        AS_GPIO

        //scan  pin need 10K pullup
        #define PULL_WAKEUP_SRC_PD6             MATRIX_ROW_PULL
        #define PULL_WAKEUP_SRC_PF6             MATRIX_ROW_PULL

        //scan pin open input to read gpio level
        #define PD6_INPUT_ENABLE                1
        #define PF6_INPUT_ENABLE                1

        //drive pin as gpio
        #define PD7_FUNC                        AS_GPIO
        #define PD2_FUNC                        AS_GPIO

        //drive pin need 100K pulldown
        #define PULL_WAKEUP_SRC_PD7             MATRIX_COL_PULL
        #define PULL_WAKEUP_SRC_PD2             MATRIX_COL_PULL

        //drive pin open input to read gpio wakeup level
        #define PD7_INPUT_ENABLE                1
        #define PD2_INPUT_ENABLE                1

    #endif
#endif


/**
 *  @brief  GPIO definition for LED
 */
#if UI_LED_ENABLE
    #define LED_ON_LEVEL                        1       //gpio output high voltage to turn on led
    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        #if(HARDWARE_TYPE == TLSR9518DONGLE)
        #define GPIO_LED_GREEN                      GPIO_PB0
        #define GPIO_LED_RED                        GPIO_PB4
        #define GPIO_LED_YELLOW                     GPIO_PB5
        #define GPIO_LED_BLUE                       GPIO_PB7


        #define PB0_FUNC                            AS_GPIO
        #define PB4_FUNC                            AS_GPIO
        #define PB5_FUNC                            AS_GPIO
        #define PB7_FUNC                            AS_GPIO

        #define PB0_OUTPUT_ENABLE                   1
        #define PB4_OUTPUT_ENABLE                   1
        #define PB5_OUTPUT_ENABLE                   1
        #define PB7_OUTPUT_ENABLE                   1
        #elif(HARDWARE_TYPE == TLSR9518EVK)
        #define GPIO_LED_BLUE                       GPIO_PB4
        #define GPIO_LED_GREEN                      GPIO_PB5
        #define GPIO_LED_WHITE                      GPIO_PB6
        #define GPIO_LED_RED                        GPIO_PB7

        #define PB4_FUNC                            AS_GPIO
        #define PB5_FUNC                            AS_GPIO
        #define PB6_FUNC                            AS_GPIO
        #define PB7_FUNC                            AS_GPIO

        #define PB4_OUTPUT_ENABLE                   1
        #define PB5_OUTPUT_ENABLE                   1
        #define PB6_OUTPUT_ENABLE                   1
        #define PB7_OUTPUT_ENABLE                   1
        #endif

    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
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
    #endif
#endif





/**
 *  @brief  GPIO definition for JTAG
 */
#if (JTAG_DEBUG_DISABLE)
    //JTAG will cost some power
    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        #define PE4_FUNC            AS_GPIO
        #define PE5_FUNC            AS_GPIO
        #define PE6_FUNC            AS_GPIO
        #define PE7_FUNC            AS_GPIO

        #define PE4_INPUT_ENABLE    0
        #define PE5_INPUT_ENABLE    0
        #define PE6_INPUT_ENABLE    0
        #define PE7_INPUT_ENABLE    0

        #define PULL_WAKEUP_SRC_PE4 0
        #define PULL_WAKEUP_SRC_PE5 0
        #define PULL_WAKEUP_SRC_PE6 0
        #define PULL_WAKEUP_SRC_PE7 0
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
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
#endif


/**
 *  @brief  GPIO definition for debug_io
 */
#if (DEBUG_GPIO_ENABLE)
    #if (MCU_CORE_TYPE == MCU_CORE_B91)
        #define GPIO_CHN0                           GPIO_PE1
        #define GPIO_CHN1                           GPIO_PE2
        #define GPIO_CHN2                           GPIO_PA0
        #define GPIO_CHN3                           GPIO_PA4
        #define GPIO_CHN4                           GPIO_PA3
        #define GPIO_CHN5                           GPIO_PB0
        #define GPIO_CHN6                           GPIO_PB2
        #define GPIO_CHN7                           GPIO_PE0

        #define GPIO_CHN8                           GPIO_PA2
        #define GPIO_CHN9                           GPIO_PA1
        #define GPIO_CHN10                          GPIO_PB1
        #define GPIO_CHN11                          GPIO_PB3
        #define GPIO_CHN12                          GPIO_PC7
        #define GPIO_CHN13                          GPIO_PC6
        #define GPIO_CHN14                          GPIO_PC5
        #define GPIO_CHN15                          GPIO_PC4


        #define PE1_OUTPUT_ENABLE                   1
        #define PE2_OUTPUT_ENABLE                   1
        #define PA0_OUTPUT_ENABLE                   1
        #define PA4_OUTPUT_ENABLE                   1
        #define PA3_OUTPUT_ENABLE                   1
        #define PB0_OUTPUT_ENABLE                   1
        #define PB2_OUTPUT_ENABLE                   1
        #define PE0_OUTPUT_ENABLE                   1

        #define PA2_OUTPUT_ENABLE                   1
        #define PA1_OUTPUT_ENABLE                   1
        #define PB1_OUTPUT_ENABLE                   1
        #define PB3_OUTPUT_ENABLE                   1
        #define PC7_OUTPUT_ENABLE                   1
        #define PC6_OUTPUT_ENABLE                   1
        #define PC5_OUTPUT_ENABLE                   1
        #define PC4_OUTPUT_ENABLE                   1
    #elif (MCU_CORE_TYPE == MCU_CORE_B92)
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
    #endif
#endif  //end of DEBUG_GPIO_ENABLE


#if (USER_DEBUG_ENABLE)

#define DBG_GPIO_CHN_0                          GPIO_PE1
#define DBG_GPIO_CHN_1                          GPIO_PE2
#define DBG_GPIO_CHN_2                          GPIO_PA0
#define DBG_GPIO_CHN_3                          GPIO_PA4
#define DBG_GPIO_CHN_4                          GPIO_PA3
#define DBG_GPIO_CHN_5                          GPIO_PB0
#define DBG_GPIO_CHN_6                          GPIO_PB2
#define DBG_GPIO_CHN_7                          GPIO_PE0

#define DBG_GPIO_CHN_8                          GPIO_PA2
#define DBG_GPIO_CHN_9                          GPIO_PA1
#define DBG_GPIO_CHN_10                         GPIO_PB1
#define DBG_GPIO_CHN_11                         GPIO_PB3
#define DBG_GPIO_CHN_12                         GPIO_PC7
#define DBG_GPIO_CHN_13                         GPIO_PC6
#define DBG_GPIO_CHN_14                         GPIO_PC5
#define DBG_GPIO_CHN_15                         GPIO_PC4

#define PE1_OUTPUT_ENABLE                   1
#define PE2_OUTPUT_ENABLE                   1
#define PA0_OUTPUT_ENABLE                   1
#define PA4_OUTPUT_ENABLE                   1
#define PA3_OUTPUT_ENABLE                   1
#define PB0_OUTPUT_ENABLE                   1
#define PB2_OUTPUT_ENABLE                   1
#define PE0_OUTPUT_ENABLE                   1

#define PA2_OUTPUT_ENABLE                   1
#define PA1_OUTPUT_ENABLE                   1
#define PB1_OUTPUT_ENABLE                   1
#define PB3_OUTPUT_ENABLE                   1
#define PC7_OUTPUT_ENABLE                   1
#define PC6_OUTPUT_ENABLE                   1
#define PC5_OUTPUT_ENABLE                   1
#define PC4_OUTPUT_ENABLE                   1


//main loop
#define APP_DBG_CHN_0_LOW       gpio_write(DBG_GPIO_CHN_0, 0)
#define APP_DBG_CHN_0_HIGH      gpio_write(DBG_GPIO_CHN_0, 1)

//tx encode
#define APP_DBG_CHN_1_LOW       gpio_write(DBG_GPIO_CHN_1, 0)
#define APP_DBG_CHN_1_HIGH      gpio_write(DBG_GPIO_CHN_1, 1)

//tx send
#define APP_DBG_CHN_2_LOW       gpio_write(DBG_GPIO_CHN_2, 0)
#define APP_DBG_CHN_2_HIGH      gpio_write(DBG_GPIO_CHN_2, 1)

//rx
#define APP_DBG_CHN_3_LOW       gpio_write(DBG_GPIO_CHN_3, 0)
#define APP_DBG_CHN_3_HIGH      gpio_write(DBG_GPIO_CHN_3, 1)

//render point
#define APP_DBG_CHN_4_LOW       gpio_write(DBG_GPIO_CHN_4, 0)
#define APP_DBG_CHN_4_HIGH      gpio_write(DBG_GPIO_CHN_4, 1)

//usb spk read
#define APP_DBG_CHN_5_LOW       gpio_write(DBG_GPIO_CHN_5, 0)
#define APP_DBG_CHN_5_HIGH      gpio_write(DBG_GPIO_CHN_5, 1)

//usb mic send
#define APP_DBG_CHN_6_LOW       gpio_write(DBG_GPIO_CHN_6, 0)
#define APP_DBG_CHN_6_HIGH      gpio_write(DBG_GPIO_CHN_6, 1)

//usb mic data read error
#define APP_DBG_CHN_7_LOW       gpio_write(DBG_GPIO_CHN_7, 0)
#define APP_DBG_CHN_7_HIGH      gpio_write(DBG_GPIO_CHN_7, 1)

//usb mic tick exceed
#define APP_DBG_CHN_8_LOW       gpio_write(DBG_GPIO_CHN_8, 0)
#define APP_DBG_CHN_8_HIGH      gpio_write(DBG_GPIO_CHN_8, 1)

//usb irq
#define APP_DBG_CHN_9_LOW       gpio_write(DBG_GPIO_CHN_9, 0)
#define APP_DBG_CHN_9_HIGH      gpio_write(DBG_GPIO_CHN_9, 1)

//list start
#define APP_DBG_CHN_10_LOW      gpio_write(DBG_GPIO_CHN_10, 0)
#define APP_DBG_CHN_10_HIGH     gpio_write(DBG_GPIO_CHN_10, 1)

//list continue
#define APP_DBG_CHN_11_LOW      gpio_write(DBG_GPIO_CHN_11, 0)
#define APP_DBG_CHN_11_HIGH     gpio_write(DBG_GPIO_CHN_11, 1)

//node find
#define APP_DBG_CHN_12_LOW      gpio_write(DBG_GPIO_CHN_12, 0)
#define APP_DBG_CHN_12_HIGH     gpio_write(DBG_GPIO_CHN_12, 1)

//sT,return
#define APP_DBG_CHN_13_LOW      gpio_write(DBG_GPIO_CHN_13, 0)
#define APP_DBG_CHN_13_HIGH     gpio_write(DBG_GPIO_CHN_13, 1)

//plc
#define APP_DBG_CHN_14_LOW      gpio_write(DBG_GPIO_CHN_14, 0)
#define APP_DBG_CHN_14_HIGH     gpio_write(DBG_GPIO_CHN_14, 1)

//normal receive
#define APP_DBG_CHN_15_LOW      gpio_write(DBG_GPIO_CHN_15, 0)
#define APP_DBG_CHN_15_HIGH     gpio_write(DBG_GPIO_CHN_15, 1)



#endif



#ifndef APP_DBG_CHN_0_LOW
#define APP_DBG_CHN_0_LOW
#endif
#ifndef APP_DBG_CHN_0_HIGH
#define APP_DBG_CHN_0_HIGH
#endif

#ifndef APP_DBG_CHN_1_LOW
#define APP_DBG_CHN_1_LOW
#endif
#ifndef APP_DBG_CHN_1_HIGH
#define APP_DBG_CHN_1_HIGH
#endif


#ifndef APP_DBG_CHN_2_LOW
#define APP_DBG_CHN_2_LOW
#endif
#ifndef APP_DBG_CHN_2_HIGH
#define APP_DBG_CHN_2_HIGH
#endif

#ifndef APP_DBG_CHN_3_LOW
#define APP_DBG_CHN_3_LOW
#endif
#ifndef APP_DBG_CHN_3_HIGH
#define APP_DBG_CHN_3_HIGH
#endif

#ifndef APP_DBG_CHN_4_LOW
#define APP_DBG_CHN_4_LOW
#endif
#ifndef APP_DBG_CHN_4_HIGH
#define APP_DBG_CHN_4_HIGH
#endif

#ifndef APP_DBG_CHN_5_LOW
#define APP_DBG_CHN_5_LOW
#endif
#ifndef APP_DBG_CHN_5_HIGH
#define APP_DBG_CHN_5_HIGH
#endif

#ifndef APP_DBG_CHN_6_LOW
#define APP_DBG_CHN_6_LOW
#endif
#ifndef APP_DBG_CHN_6_HIGH
#define APP_DBG_CHN_6_HIGH
#endif

#ifndef APP_DBG_CHN_7_LOW
#define APP_DBG_CHN_7_LOW
#endif
#ifndef APP_DBG_CHN_7_HIGH
#define APP_DBG_CHN_7_HIGH
#endif

#ifndef APP_DBG_CHN_8_LOW
#define APP_DBG_CHN_8_LOW
#endif
#ifndef APP_DBG_CHN_8_HIGH
#define APP_DBG_CHN_8_HIGH
#endif

#ifndef APP_DBG_CHN_9_LOW
#define APP_DBG_CHN_9_LOW
#endif
#ifndef APP_DBG_CHN_9_HIGH
#define APP_DBG_CHN_9_HIGH
#endif

#ifndef APP_DBG_CHN_10_LOW
#define APP_DBG_CHN_10_LOW
#endif
#ifndef APP_DBG_CHN_10_HIGH
#define APP_DBG_CHN_10_HIGH
#endif

#ifndef APP_DBG_CHN_11_LOW
#define APP_DBG_CHN_11_LOW
#endif
#ifndef APP_DBG_CHN_11_HIGH
#define APP_DBG_CHN_11_HIGH
#endif

#ifndef APP_DBG_CHN_12_LOW
#define APP_DBG_CHN_12_LOW
#endif
#ifndef APP_DBG_CHN_12_HIGH
#define APP_DBG_CHN_12_HIGH
#endif

#ifndef APP_DBG_CHN_13_LOW
#define APP_DBG_CHN_13_LOW
#endif
#ifndef APP_DBG_CHN_13_HIGH
#define APP_DBG_CHN_13_HIGH
#endif

#ifndef APP_DBG_CHN_14_LOW
#define APP_DBG_CHN_14_LOW
#endif
#ifndef APP_DBG_CHN_14_HIGH
#define APP_DBG_CHN_14_HIGH
#endif

#ifndef APP_DBG_CHN_15_LOW
#define APP_DBG_CHN_15_LOW
#endif
#ifndef APP_DBG_CHN_15_HIGH
#define APP_DBG_CHN_15_HIGH
#endif





#include "../common/default_config.h"

#endif
