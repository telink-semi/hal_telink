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
#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)


    #pragma once


    #define ACL_CENTRAL_MAX_NUM 4 // ACL central maximum number
    #define ACL_PERIPHR_MAX_NUM 0 // ACL peripheral maximum number

    ///////////////////////// Feature Configuration////////////////////////////////////////////////
    #define ACL_PERIPHR_SMP_ENABLE               0 //1 for smp,  0 no security
    #define ACL_CENTRAL_SMP_ENABLE               1 //1 for smp,  0 no security
    #define LL_FEATURE_PRIVATE_BIS_SYNC_RECEIVER 0 //Private BIS bcst feature,use extAdv to bcst BIGInfo
    #define PRIVATE_EXT_FILTER_SPECIFIC_SID      8 //bit0~bit4 valid
/* LE Audio demo used */

    #define BIG_BCST_NUMBER         1
    #define BIS_NUM_IN_PER_BIG_BCST 2

    ///////////////////////// UI Configuration ////////////////////////////////////////////////////
    #define UI_LED_ENABLE      1
    #define UI_KEYBOARD_ENABLE 0

    ///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
    #define SDK_RELEASE_EN       1
    #define JTAG_DEBUG_DISABLE   1

    #define APP_LOG_EN           1
    #define TLKAPI_DEBUG_ENABLE  1
    #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UART
    #define VCD_EN               0
    #define DUMP_STR_EN          1
    #define FAST_SETTLE          1

    #define APP_LOG_EN           1
    #define APP_CONTR_EVT_LOG_EN 1 //controller event
    #define APP_PRF_EVT_LOG_EN   1

    ///////////////////////// ! OS settings////////////////////////////////////////////////
    #define OS_SUP_EN       0
    #define FREERTOS_ENABLE 0

    #if (SDK_RELEASE_EN)
        #define DEBUG_GPIO_ENABLE                     1
        #define CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN 0
    #else
        #define DEBUG_GPIO_ENABLE                     1
        #define CENTRAL_CONNECT_PERIPHR_MAC_FILTER_EN 1
    #endif

    #define BIS_CENTRAL_ACL_CENTRAL_TIMING_STAGGERED 1

///////////////////////// Audio  Configuration /////////////////////////////////
/* Audio sample rate select */

    #define LC3_ENCODE_CHANNEL_COUNT 2
    //broadcast source extend advertising parameter
    #define DEFAULT_DEV_NAME       "Telink-BIS-SOURCE"
    #define DEFAULT_BROADCAST_NAME "Broad-source"
    #define DEFAULT_BROADCAST_ID   0x010305

    #define APP_AUDIO_UI_UART      1
    #define APP_AUDIO_UI_USB_CDC   2
    #define APP_AUDIO_UI_IFACE     APP_AUDIO_UI_USB_CDC

    #define APPLICATION_DONGLE     1
    #define MODULE_USB_ENABLE      1

    #define DESC_IAD_ENABLE        1

    #define ID_VENDOR              0x248a // for report
    #define ID_PRODUCT_BASE        0x6101 //AUDIO_HOGP
    #define STRING_VENDOR          L"Telink"
    #define STRING_PRODUCT         L"Telink Broadcast SOURCE"
    #define STRING_SERIAL          L"TLSR9x"

    #define MIC_RESOLUTION_BIT     16
    #define MIC_SAMPLE_RATE        48000
    #define MIC_CHANNEL_COUNT      1
    #define MIC_ENCODER_ENABLE     0

    #define SPK_RESOLUTION_BIT     16
    #define SPK_SAMPLE_RATE        48000
    #define SPK_CHANNEL_COUNT      2

    #define PA5_FUNC               AS_USB_DM
    #define PA6_FUNC               AS_USB_DP
    #define PA5_INPUT_ENABLE       1
    #define PA6_INPUT_ENABLE       1

    #define USB_USE_VENDOR_DESC    1

    #define USB_PRINTER_ENABLE     0
    #define USB_SPEAKER_ENABLE     1
    #define USB_CDC_ENABLE         1
    #define AUDIO_HOGP             0

    #define USB_MIC_ENABLE         0
    #define USB_MOUSE_ENABLE       0
    #define USB_KEYBOARD_ENABLE    0
    #define USB_SOMATIC_ENABLE     0 //  when USB_SOMATIC_ENABLE, USB_EDP_PRINTER_OUT disable
    #define USB_CUSTOM_HID_REPORT  0


    /**
 *  @brief  GPIO definition for LED
 */
    #if UI_LED_ENABLE
        #define LED_ON_LEVEL 1 //gpio output high voltage to turn on led
        #if (MCU_CORE_TYPE == MCU_CORE_B91)
            #define GPIO_LED_BLUE     GPIO_PB4
            #define GPIO_LED_GREEN    GPIO_PB5
            #define GPIO_LED_WHITE    GPIO_PB6
            #define GPIO_LED_RED      GPIO_PB7

            #define PB4_FUNC          AS_GPIO
            #define PB5_FUNC          AS_GPIO
            #define PB6_FUNC          AS_GPIO
            #define PB7_FUNC          AS_GPIO

            #define PB4_OUTPUT_ENABLE 1
            #define PB5_OUTPUT_ENABLE 1
            #define PB6_OUTPUT_ENABLE 1
            #define PB7_OUTPUT_ENABLE 1
        #elif (MCU_CORE_TYPE == MCU_CORE_B92)
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
            #define GPIO_CHN0         GPIO_PE1
            #define GPIO_CHN1         GPIO_PE2
            #define GPIO_CHN2         GPIO_PA0
            #define GPIO_CHN3         GPIO_PA4
            #define GPIO_CHN4         GPIO_PA3
            #define GPIO_CHN5         GPIO_PB0
            #define GPIO_CHN6         GPIO_PB2
            #define GPIO_CHN7         GPIO_PE0

            #define GPIO_CHN8         GPIO_PA2
            #define GPIO_CHN9         GPIO_PA1
            #define GPIO_CHN10        GPIO_PB1
            #define GPIO_CHN11        GPIO_PB3
            #define GPIO_CHN12        GPIO_PC7
            #define GPIO_CHN13        GPIO_PC6
            #define GPIO_CHN14        GPIO_PC5
            #define GPIO_CHN15        GPIO_PC4


            #define PE1_OUTPUT_ENABLE 1
            #define PE2_OUTPUT_ENABLE 1
            #define PA0_OUTPUT_ENABLE 1
            #define PA4_OUTPUT_ENABLE 1
            #define PA3_OUTPUT_ENABLE 1
            #define PB0_OUTPUT_ENABLE 1
            #define PB2_OUTPUT_ENABLE 1
            #define PE0_OUTPUT_ENABLE 1

            #define PA2_OUTPUT_ENABLE 1
            #define PA1_OUTPUT_ENABLE 1
            #define PB1_OUTPUT_ENABLE 1
            #define PB3_OUTPUT_ENABLE 1
            #define PC7_OUTPUT_ENABLE 1
            #define PC6_OUTPUT_ENABLE 1
            #define PC5_OUTPUT_ENABLE 1
            #define PC4_OUTPUT_ENABLE 1
        #elif (MCU_CORE_TYPE == MCU_CORE_B92)
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
        #endif
    #endif //end of DEBUG_GPIO_ENABLE


    #include "../common/default_config.h"

#endif //SOURCE_VERSION == SOURCE_WITH_ASSISTANT
