/********************************************************************************************************
 * @file    app_llmic.c
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

#include "stack/ble/controller/ll/llmic/llmic.h"

#include "app.h"
#include "app_llmic.h"

#if (INTER_TEST_MODE == TEST_LIVE_MIC_PROJECT)

volatile u8 app_llmic_count5ms = 0;
volatile u8 app_llmic_state;
volatile u8 task_llmic_task_en = 0;
volatile u8 app_llmic_taskTick = 0;

int global_abandon_enbale;
int last_abandon_enbale;
int cur_abandon_enbale;

u32 stick_abandon_start;
u32 stick_abandon_end;

_attribute_ram_code_sec_ void timer0_irq_handler(void)
{
    //delay_us(20);
    if (timer_get_irq_status(TMR_STA_TMR0)) {
        u32            cur_tick        = clock_time();
        signal_fifo_t *app_llmic_param = blc_ll_get_llmic_param();
        gpio_toggle(LL_MIC_DEBUG_IO_400US);
        DBG_SIHUI_CHN4_TOGGLE;

        timer_clr_irq_status(TMR_STA_TMR0); //clear irq status
        app_llmic_count5ms++;
        //      u32 tick = timer1_get_tick() + 385*sys_clk.pclk;
        //      timer_set_cap_tick(TIMER0,tick); //384us *13 = 5ms
        if (app_llmic_count5ms >= 12) {
            gpio_toggle(LL_MIC_DEBUG_IO_5MS);
            DBG_SIHUI_CHN5_TOGGLE;
            app_llmic_count5ms = 0;
        }

        if (app_llmic_param->ble_new_notify) { //new signal from BLE stack
            DBG_SIHUI_CHN9_TOGGLE;
            if (app_llmic_param->ble_signal == BLE_SIGL_TSYNC) {
                gpio_toggle(LL_MIC_DEBUG_IO_TSYNC);
                app_llmic_param->ble_new_notify = 0;
                DBG_SIHUI_CHN11_TOGGLE;
                app_llmic_param->llmic_signal = LLMIC_SIGL_COMPROMISE;
                global_abandon_enbale         = 1;
                stick_abandon_start           = app_llmic_param->stick_task_begin;
                stick_abandon_end             = app_llmic_param->stick_task_end;
            }
            /* next task tick is  */
            else if (tick1_exceed_tick2(cur_tick + 200 * SYSTEM_TIMER_TICK_1US, app_llmic_param->stick_task_begin)) {
                app_llmic_param->ble_new_notify = 0;
                gpio_toggle(LL_MIC_DEBUG_IO_ABANDON_BLE);
                DBG_SIHUI_CHN10_TOGGLE;
                app_llmic_param->llmic_signal = LLMIC_SIGL_REJECT;
                global_abandon_enbale         = 0;
            } else if (tick1_exceed_tick2(cur_tick + 600 * SYSTEM_TIMER_TICK_1US, app_llmic_param->stick_task_begin)) {
                app_llmic_param->ble_new_notify = 0;
                DBG_SIHUI_CHN11_TOGGLE;
                gpio_toggle(LL_MIC_DEBUG_IO_ABANDON_LLMIC);
                app_llmic_param->llmic_signal = LLMIC_SIGL_COMPROMISE;
                global_abandon_enbale         = 1;
                stick_abandon_start           = app_llmic_param->stick_task_begin;
                stick_abandon_end             = app_llmic_param->stick_task_end;
            } else {
                DBG_SIHUI_CHN12_TOGGLE;
                //future, process in next timer1 IRQ
            }
        }


        if (global_abandon_enbale &&
            tick1_exceed_tick2(cur_tick + 400 * SYSTEM_TIMER_TICK_1US, stick_abandon_end)) {
            global_abandon_enbale = 0;
            gpio_set_low_level(LL_MIC_DEBUG_IO_EXTERNAL);
            cur_abandon_enbale = 1;
            DBG_SIHUI_CHN6_LOW;
        } else if (global_abandon_enbale) {
            cur_abandon_enbale = 0;
            DBG_SIHUI_CHN6_HIGH;
            gpio_set_high_level(LL_MIC_DEBUG_IO_EXTERNAL);
        }


        //          if(last_abandon_enbale && !cur_abandon_enbale){
        //              global_abandon_enbale = 0;
        //          }

        //          last_abandon_enbale = cur_abandon_enbale;
    }
}
PLIC_ISR_REGISTER(timer0_irq_handler, IRQ_TIMER0)

_attribute_ram_code_sec_ void app_llmic_BLEtashFinsihCb(void)
{
    stick_abandon_end = clock_time();
    //<50us
}

void app_llmic_init(void)
{
    #ifdef LL_MIC_DEBUG_IO_400US
    gpio_function_en(LL_MIC_DEBUG_IO_400US);
    gpio_output_en(LL_MIC_DEBUG_IO_400US);
    #endif

    #ifdef LL_MIC_DEBUG_IO_5MS
    gpio_function_en(LL_MIC_DEBUG_IO_5MS);
    gpio_output_en(LL_MIC_DEBUG_IO_5MS);
    #endif

    #ifdef LL_MIC_DEBUG_IO_EXTERNAL
    gpio_function_en(LL_MIC_DEBUG_IO_EXTERNAL);
    gpio_output_en(LL_MIC_DEBUG_IO_EXTERNAL);
    #endif

    #ifdef LL_MIC_DEBUG_IO_TSYNC
    gpio_function_en(LL_MIC_DEBUG_IO_TSYNC);
    gpio_output_en(LL_MIC_DEBUG_IO_TSYNC);
    #endif

    #ifdef LL_MIC_DEBUG_IO_ABANDON_BLE
    gpio_function_en(LL_MIC_DEBUG_IO_ABANDON_BLE);
    gpio_output_en(LL_MIC_DEBUG_IO_ABANDON_BLE);
    #endif

    #ifdef LL_MIC_DEBUG_IO_ABANDON_LLMIC
    gpio_function_en(LL_MIC_DEBUG_IO_ABANDON_LLMIC);
    gpio_output_en(LL_MIC_DEBUG_IO_ABANDON_LLMIC);
    #endif


    plic_interrupt_enable(IRQ_TIMER0);
    timer_set_init_tick(TIMER0, 0);
    timer_set_cap_tick(TIMER0, (5000 * sys_clk.pclk) / 12);
    timer_set_mode(TIMER0, TIMER_MODE_SYSCLK);
    timer_start(TIMER0);
    task_llmic_task_en = 1;
    app_llmic_count5ms = 0;
    app_llmic_state    = 0;

    blc_ll_registerTelinkControllerFinishCallback(app_llmic_BLEtashFinsihCb);
    blc_ll_set_llmic_enable(1);
    plic_set_priority(IRQ_TIMER0, 2);
}

void app_llmic_mainloop(void)
{
}

#endif //end of TEST_LIVE_MIC_PROJECT
