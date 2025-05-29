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

#if (INTER_TEST_MODE == TEST_RAS_SERVER)

#include "../intest_config.h"

#define __CHANNEL_SOUNDING_EN__  1

#define TTF_EN                   1
#define RAS_IOPTEST_ENABLE       1
#define CONSOLE_OUTPUT_VIA_DEBUG 1 // UI (condole) will be output via debug logs
#define CS_PROCEDURE_CMD_TRIG    1
#define UI_CONTROL_ENABLE        1

#define CONN_MAX_NUM_CONFIG      CONN_MAX_NUM_C0_P2

#define ACL_CENTRAL_MAX_NUM 0 // ACL central maximum number
#define ACL_PERIPHR_MAX_NUM 2 // ACL peripheral maximum number

#define GOOGLE_CS_REFL_ROLE_EN   0

///////////////////////// OS  Configuration ////////////////////////////////////////////////////
#define FREERTOS_ENABLE         0
#define OS_SEPARATE_STACK_SPACE 0 //Separate the task stack and interrupt stack space
#define configTOTAL_HEAP_SIZE   (64 * 1024)


///////////////////////// Feature Configuration////////////////////////////////////////////////
#define ACL_PERIPHR_SMP_ENABLE 1 //1 for smp,  0 no security
#define BLE_OTA_SERVER_ENABLE  1

#define BLE_APP_PM_ENABLE      0

#define BATT_CHECK_ENABLE      0


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

/////////////////////// Board Select Configuration ///////////////////////////////
#if (MCU_CORE_TYPE == MCU_CORE_B91)
    #define BOARD_SELECT BOARD_951X_EVK_C1T213A20
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
    #if(GOOGLE_CS_REFL_ROLE_EN)
        #define BOARD_SELECT BOARD_952X_EVK_C1T266A20
    #else
        #define BOARD_SELECT BOARD_952X_EVK_C1T266A20 //BOARD_952X_EVK_C1T266A102 //BOARD_952X_EVK_C1T266A20//BOARD_952X_EVK_C1T266A102
        #define TLKAPI_DEBUG_UART_RX_PIN    GPIO_FC_PD3
        #define TLKAPI_DEBUG_UART_TX_PIN    GPIO_FC_PD2
    #endif
#elif (MCU_CORE_TYPE == MCU_CORE_TL721X)
    #define BOARD_SELECT BOARD_721X_EVK_C1T315A20  //BOARD_721X_EVK_C1T315A102 --- tercel multi ant board for channel sounding
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
    #define BOARD_SELECT BOARD_321X_EVK_C1T331A20
#endif


#if(GOOGLE_CS_REFL_ROLE_EN)
    #define GPIO_WORK_VOLTAGE GPIO_VOLTAGE_1V8 //change
#else
    #define GPIO_WORK_VOLTAGE GPIO_VOLTAGE_3V3 //change
#endif


/* Flash 4line mode:
 *  enable the 4 line mode of flash, read and write.
 */
#define FLASH_4LINE_MODE_ENABLE 1

#define UI_LED_ENABLE           1
#define UI_KEYBOARD_ENABLE      0

///////////////////////// Channel sounding Configuration ///////////////////////////////
#define CS_PROCEDURE_EXCHANGE   1

#define APP_CS_CONFIG_NUM       1

#define RAS_STEP_FILTER         1
#define RAS_PERSISTENT_FILTER   1
#define RAS_OOB                 0
///////////////////////// DEBUG  Configuration /////////////////////////////////////////
#define DEBUG_CS_GPIO_ENABLE   1

#define DEBUG_GPIO_ENABLE      0

#define TLKAPI_DEBUG_ENABLE    1

#define APP_CS_LOG_EN          0
#define APP_LOG_EN             1
#define APP_CONTR_EVT_LOG_EN   1 //controller event
#define APP_CS_SUBEVENT_LOG_EN 0
#define APP_HOST_EVT_LOG_EN    1
#define APP_SMP_LOG_EN         1
#define APP_SIMPLE_SDP_LOG_EN  0
#define APP_KEY_LOG_EN         1

#define JTAG_DEBUG_DISABLE     1 //if use JTAG, change this


#define APP_CS_UI_UART         1
#define APP_CS_UI_USB_CDC      2


#define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UART
#define USB_CDC_ENABLE       0
#define APP_CS_UI_MODE APP_CS_UI_UART

// #if(GOOGLE_CS_REFL_ROLE_EN)
//     #define APP_CS_UI_MODE         TLKAPI_DEBUG_CHANNEL_GSUART
//     #define TLKAPI_DEBUG_GPIO_PIN GPIO_PC3
// #else
//     #define APP_CS_UI_MODE         TLKAPI_DEBUG_CHANNEL_UDB
// #endif

// #if (APP_CS_UI_MODE == APP_CS_UI_UART)
//     #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UDB
//     #define USB_CDC_ENABLE       0
// #elif (APP_CS_UI_MODE == APP_CS_UI_USB_CDC)
//     #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_GSUART
//     #define MODULE_USB_ENABLE    1
//     #define USB_CDC_ENABLE       1
//     #define ID_VENDOR            0x248a // for report
//     #define ID_PRODUCT_BASE      0x6102 //AUDIO_HOGP
// #endif


/////////////////// DEEP SAVE FLG //////////////////////////////////
#define USED_DEEP_ANA_REG PM_ANA_REG_POWER_ON_CLR_BUF1 //u8,can save 8 bit info when deep
#define LOW_BATT_FLG      BIT(0)                       //if 1: low battery
#define CONN_DEEP_FLG     BIT(1)                       //if 1: conn deep, 0: adv deep

#if FREERTOS_ENABLE
/////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
    #define APP_REAL_TIME_PRINTF         0
    #define traceAPP_LED_Task_Toggle()   //gpio_toggle(GPIO_PB5);
    #define traceAPP_BLE_Task_BEGIN()    //gpio_write(GPIO_PB6,1);
    #define traceAPP_BLE_Task_END()      //gpio_write(GPIO_PB6,0);
    #define traceAPP_KEY_Task_BEGIN()    //gpio_write(GPIO_PB7,1);
    #define traceAPP_KEY_Task_END()      //gpio_write(GPIO_PB7,0);
    #define traceAPP_BAT_Task_BEGIN()    //gpio_write(GPIO_CH04,1);
    #define traceAPP_BAT_Task_END()      //gpio_write(GPIO_CH04,0);

    #define traceAPP_MUTEX_Task_BEGIN()  //gpio_write(GPIO_CH05,1);
    #define traceAPP_MUTEX_Task_END()    //gpio_write(GPIO_CH05,0);

    #define tracePort_IrqHandler_BEGIN() //gpio_write(GPIO_CH06,1);
    #define tracePort_IrqHandler_END()   //gpio_write(GPIO_CH06,0);

#endif

#include "../common/default_config.h"

#endif //#if (INTER_TEST_MODE == TEST_RAS_SERVER)