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
#if (INTER_TEST_MODE == TEST_ONCA)

    #define ACL_CENTRAL_MAX_NUM 0 // ACL central maximum number
    #define ACL_PERIPHR_MAX_NUM 2 // ACL peripheral maximum number

    ///////////////////////// Feature Configuration////////////////////////////////////////////////
    #define ACL_PERIPHR_SMP_ENABLE        1 //1 for smp,  0 no security
    #define BLE_OTA_SERVER_ENABLE         1

    #define BLE_APP_PM_ENABLE             0
    #define PM_DEEPSLEEP_RETENTION_ENABLE 0

    #define BATT_CHECK_ENABLE             0

    #define BLMS_PM_ENABLE                0
    ///////////////////////// ! OS settings////////////////////////////////////////////////
    #define OS_SUP_EN           1

    #define BOARD_SELECT        BOARD_953X_EVK_C1T318A20

    #define APP_EMI_TEST_ENABLE 0

    ///////////////////////// OS settings /////////////////////////////////////////////////////////
    #define FREERTOS_ENABLE           0
    #define OS_SEPARATE_STACK_SPACE   0 //Separate the task stack and interrupt stack space
    #define configTOTAL_HEAP_SIZE     (16 * 1024)
    #define configISR_PLIC_STACK_SIZE 640

    ///////////////////////// UI Configuration ////////////////////////////////////////////////////
    #define UI_LED_ENABLE               0
    #define UI_KEYBOARD_ENABLE          0

    #define SVC_DEFAULT_KEYBOARD_ENABLE 1

    ///////////////////////// DEBUG  Configuration ////////////////////////////////////////////////
    #define DEBUG_GPIO_ENABLE    1

    #define DUMP_STR_EN          1
    #define TLKAPI_DEBUG_ENABLE  1
    #define TLKAPI_DEBUG_CHANNEL TLKAPI_DEBUG_CHANNEL_UART

    #define APP_LOG_EN           1
    #define APP_CONTR_EVT_LOG_EN 1 //controller event
    #define APP_HOST_EVT_LOG_EN  1
    #define APP_KEY_LOG_EN       1

    #define JTAG_DEBUG_DISABLE   1 //if use JTAG, change this

    #define DEFAULT_DEV_NAME     "onca-test-demo"


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


    #if FREERTOS_ENABLE
    /////////////////////////////////////// PRINT DEBUG INFO ///////////////////////////////////////
        #define APP_REAL_TIME_PRINTF         0


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
