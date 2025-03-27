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



#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_CS_ACL_CENTRAL)
#define ACL_CENTRAL_MAX_NUM                         4 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM                         0 // ACL peripheral maximum number


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_CENTRAL_SMP_ENABLE                      1   //1 for smp,  0 no security
#define ACL_CENTRAL_SIMPLE_SDP_ENABLE               1   //simple service discovery for ACL central
#define APPLICATION_DONGLE                          0

#define BATT_CHECK_ENABLE                           0

///////////////////////// UI Configuration ////////////////////////////////////////////////////
#define HW_C1T213A20_V1_0_80_EVB                    1
#define HW_C1T213A3_V1_1_80_DONGLE                  2
#define HARDWARE_BOARD_SELECT                       HW_C1T213A20_V1_0_80_EVB//HW_C1T213A20_V1_0_80_EVB


#if(HARDWARE_BOARD_SELECT == HW_C1T213A20_V1_0_80_EVB)
    #define UI_KEYBOARD_ENABLE                      1
    #define UI_BUTTON_ENABLE                        0
#elif(HARDWARE_BOARD_SELECT == HW_C1T213A3_V1_1_80_DONGLE)
    #define UI_KEYBOARD_ENABLE                      0
    #define UI_BUTTON_ENABLE                        1
#endif
#define UI_LED_ENABLE                               1


#define FAST_SETTLE         0

///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
#if (HARDWARE_BOARD_SELECT == HW_C1T213A20_V1_0_80_EVB)
    #define DEBUG_GPIO_ENABLE                       1
#elif(HARDWARE_BOARD_SELECT == HW_C1T213A3_V1_1_80_DONGLE)
    #define DEBUG_GPIO_ENABLE                       0
#endif



#define TLKAPI_DEBUG_ENABLE                         1
#define TLKAPI_DEBUG_CHANNEL                        TLKAPI_DEBUG_CHANNEL_UDB

#define APP_LOG_EN                                  1
#define APP_CONTR_EVT_LOG_EN                        1   //controller event
#define APP_HOST_EVT_LOG_EN                         1
#define APP_SMP_LOG_EN                              0
#define APP_SIMPLE_SDP_LOG_EN                       0
#define APP_PAIR_LOG_EN                             1
#define APP_KEY_LOG_EN                              1



#define CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN       1
#define LL_FEATURE_ENABLE_CHANNEL_SOUNDING          1





#if(APPLICATION_DONGLE)
    #define ID_VENDOR               0x248a          // for report
    #define ID_PRODUCT_BASE         0x880c          //AUDIO_HOGP
    #define STRING_VENDOR           L"Telink"
    #define STRING_PRODUCT          L"BLE Master Dongle"
    #define STRING_SERIAL           L"TLSR9518"

    //////////////////// Audio /////////////////////////////////////
    #define MIC_RESOLUTION_BIT      16
    #define MIC_SAMPLE_RATE         16000
    #define MIC_CHANNEL_COUNT       1
    #define MIC_ENCODER_ENABLE      0

    #define SPK_RESOLUTION_BIT      16
    #define SPK_SAMPLE_RATE         48000
    #define SPK_CHANNEL_COUNT       2

    #define PA5_FUNC                AS_USB_DM
    #define PA6_FUNC                AS_USB_DP
    #define PA5_INPUT_ENABLE        1
    #define PA6_INPUT_ENABLE        1

    #define USB_PRINTER_ENABLE      1
    #define USB_SPEAKER_ENABLE      0

    #define AUDIO_HOGP              0

    #define USB_MIC_ENABLE          1
    #define USB_MOUSE_ENABLE        1
    #define USB_KEYBOARD_ENABLE     1
    #define USB_SOMATIC_ENABLE      0   //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
    #define USB_CUSTOM_HID_REPORT   1
#endif




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
#endif


#if (UI_BUTTON_ENABLE)
    //---------------  Button ----------------------------------
    #define SW1_GPIO                GPIO_PB2
    #define SW2_GPIO                GPIO_PB3
    #define PB2_FUNC                AS_GPIO
    #define PB3_FUNC                AS_GPIO
    #define PB2_INPUT_ENABLE        1
    #define PB3_INPUT_ENABLE        1
    #define PULL_WAKEUP_SRC_PB2     PM_PIN_PULLUP_10K
    #define PULL_WAKEUP_SRC_PB3     PM_PIN_PULLUP_10K
#endif


#if UI_LED_ENABLE
    #if(HARDWARE_BOARD_SELECT == HW_C1T213A20_V1_0_80_EVB)
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
    #elif(HARDWARE_BOARD_SELECT == HW_C1T213A3_V1_1_80_DONGLE)
        #define GPIO_LED_GREEN      GPIO_PB0
        #define GPIO_LED_WHITE      GPIO_PB1
        #define GPIO_LED_RED        GPIO_PB4
        #define GPIO_LED_YELLOW     GPIO_PB5
        #define GPIO_LED_BLUE       GPIO_PB7
        #define GPIO_LED_ORANGE     GPIO_PE3


        #define PB7_FUNC                AS_GPIO
        #define PB0_FUNC                AS_GPIO
        #define PB1_FUNC                AS_GPIO
        #define PB4_FUNC                AS_GPIO
        #define PB5_FUNC                AS_GPIO
        #define PE3_FUNC                AS_GPIO

        #define PB7_OUTPUT_ENABLE       1
        #define PB0_OUTPUT_ENABLE       1
        #define PB1_OUTPUT_ENABLE       1
        #define PB4_OUTPUT_ENABLE       1
        #define PB5_OUTPUT_ENABLE       1
        #define PE3_OUTPUT_ENABLE       1
    #endif

    #define LED_ON_LEVEL                1       //gpio output high voltage to turn on led
#endif



/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG                   PM_ANA_REG_POWER_ON_CLR_BUF1 //u8,can save 8 bit info when deep
#define LOW_BATT_FLG                        BIT(0) //if 1: low battery
#define CONN_DEEP_FLG                       BIT(1) //if 1: conn deep, 0: adv deep



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


#if(DEBUG_GPIO_ENABLE )
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





#include "../common/default_config.h"
#endif
