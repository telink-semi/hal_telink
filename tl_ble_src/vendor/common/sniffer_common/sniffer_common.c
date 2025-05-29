/********************************************************************************************************
 * @file    sniffer_common.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "sniffer_common.h"


#ifndef APP_EXCEPTION_STUCK_EN
    #define APP_EXCEPTION_STUCK_EN 0
#endif


static void app_debug_trap(unsigned int err_code)
{
    write_dbg32(DBG_SRAM_ADDR, err_code);

    tlkapi_send_string_u32s(APP_LOG_EN, "[APP][MEM] memory operation length error", err_code);

#if (APP_EXCEPTION_STUCK_EN) //user determine if using while(1) code stuck to check the error
    #if (UI_LED_ENABLE)
        #ifdef GPIO_LED_RED
    gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif

        #ifdef GPIO_LED_WHITE
    gpio_write(GPIO_LED_WHITE, !LED_ON_LEVEL);
        #endif

        #ifdef GPIO_LED_BLUE
    gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
        #endif

        #ifdef GPIO_LED_GREEN
    gpio_write(GPIO_LED_GREEN, !LED_ON_LEVEL);
        #endif
    #endif

    while (1) {
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif
    }
#else
    //user can decide what to do
#endif
}

void blc_app_memory_set(void *pd, int val, unsigned int len, unsigned int dst_max_len, unsigned int error_code)
{
    if (len > dst_max_len) {
        tlkapi_send_string_u32s(APP_LOG_EN, "[APP][MEM] app_memory_set length error", len, dst_max_len);
        app_debug_trap(error_code);

        #if (APP_EXCEPTION_STUCK_EN == 0)
            len = dst_max_len; //user can decide if changing actual length to maximum length
        #endif
    }

    memset(pd, val, len);
}

void blc_app_memory_copy(void *pd, const void *ps, unsigned int len, unsigned int dst_max_len, unsigned int error_code)
{
    if (len > dst_max_len) {
        tlkapi_send_string_u32s(APP_LOG_EN, "[APP][MEM] app_memory_copy length error", len, dst_max_len);
        app_debug_trap(error_code);

        #if (APP_EXCEPTION_STUCK_EN == 0)
            len = dst_max_len; //user can decide if changing actual length to maximum length
        #endif
    }

    memcpy(pd, ps, len);
}

void app_start_reboot(void)
{
    volatile unsigned int app_delay_cnt = 0;

    while (1) {
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_debug_handler();

            #if (TLKAPI_DEBUG_CHANNEL == TLKAPI_DEBUG_CHANNEL_UART)
                extern void tlkapi_uart_irq_handler(void);
                tlkapi_uart_irq_handler();
            #endif
        #endif

        app_delay_cnt++;
        if(app_delay_cnt > BIT(19)){
            break;
        }
    }

    start_reboot();
}
