/********************************************************************************************************
 * @file    TL751X_C1T318A20.h
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
#ifndef VENDOR_COMMON_BOARDS_TL751X_C1T318A20_H_
#define VENDOR_COMMON_BOARDS_TL751X_C1T318A20_H_

#undef BLC_PM_EN
#define BLC_PM_EN                       0

#undef BLC_PM_DEEP_RETENTION_MODE_EN
#define BLC_PM_DEEP_RETENTION_MODE_EN           0

#undef BLE_APP_PM_ENABLE
#define BLE_APP_PM_ENABLE                       0

#undef PM_DEEPSLEEP_RETENTION_ENABLE
#define PM_DEEPSLEEP_RETENTION_ENABLE           0

#undef BATT_CHECK_ENABLE
#define BATT_CHECK_ENABLE                       0

#undef APP_FLASH_PROTECTION_ENABLE
#define APP_FLASH_PROTECTION_ENABLE             0

#undef UI_KEYBOARD_ENABLE
#define UI_KEYBOARD_ENABLE                      0

#undef TLKAPI_DEBUG_CHANNEL
#define TLKAPI_DEBUG_CHANNEL                    TLKAPI_DEBUG_CHANNEL_UART

#undef APP_REAL_TIME_PRINTF
#define APP_REAL_TIME_PRINTF                    0

#undef BLE_OTA_SERVER_ENABLE
#define BLE_OTA_SERVER_ENABLE                   0

// must
#define PI3_INPUT_ENABLE            1
#define PI4_INPUT_ENABLE            1
#define PI5_INPUT_ENABLE            1
#define PI6_INPUT_ENABLE            1
#define PI7_INPUT_ENABLE            1
#define PI6_OUTPUT_ENABLE           0
#define PI7_OUTPUT_ENABLE           0

#define ONCA_CHIP_A0         0
#define ONCA_CHIP_A1         1

#define ONCA_CHIP_VESION     ONCA_CHIP_A1

/**
 *  @brief  Keyboard Configuration
 */
#if (UI_KEYBOARD_ENABLE)
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



/**
 *  @brief  LED Configuration
 */
#if UI_LED_ENABLE
    /**
     *  @brief  Definition gpio for led
     */
    #define GPIO_LED_BLUE                       GPIO_PA5
    #define GPIO_LED_GREEN                      GPIO_PA6
    #define GPIO_LED_WHITE                      GPIO_PA6
    #define GPIO_LED_RED                        GPIO_PA6

    #define PA5_FUNC                            AS_GPIO
    #define PA6_FUNC                            AS_GPIO

    #define PA5_OUTPUT_ENABLE                   1
    #define PA6_OUTPUT_ENABLE                   1

    #define LED_ON_LEVEL                        1       //gpio output high voltage to turn on led

#endif

/**
 *  @brief  GPIO definition for JTAG
 */
#if (JTAG_DEBUG_DISABLE)
    //JTAG will cost some power
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

#endif

/**
 *  @brief  GPIO definition for debug_io
 */
#if (DEBUG_GPIO_ENABLE)
        #define GPIO_CHN0                           GPIO_PI6
        #define GPIO_CHN1                           GPIO_PI7
        #define GPIO_CHN2                           GPIO_PJ5
        #define GPIO_CHN3                           GPIO_PJ4
        #define GPIO_CHN4                           GPIO_PJ3
        #define GPIO_CHN5                           GPIO_PJ2
        #define GPIO_CHN6                           GPIO_PJ1
        #define GPIO_CHN7                           GPIO_PJ0

        #define GPIO_CHN8                           GPIO_PH6
        #define GPIO_CHN9                           GPIO_PH5
        #define GPIO_CHN10                          GPIO_PH4
        #define GPIO_CHN11                          GPIO_PH3
        #define GPIO_CHN12                          GPIO_PH2
        #define GPIO_CHN13                          GPIO_PH1
        #define GPIO_CHN14                          GPIO_PH0
        #define GPIO_CHN15                          GPIO_PF7
#endif  //end of DEBUG_GPIO_ENABLE

/**
 *  @brief  Battery_check Configuration
 */
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


#endif /* VENDOR_COMMON_BOARDS_TL751X_C1T318A20_H_ */
