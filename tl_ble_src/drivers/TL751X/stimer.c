/********************************************************************************************************
 * @file    stimer.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "stimer.h"

unsigned int g_track_32kcnt=16;

/**
 * @brief     This function performs to set delay time by us.
 * @param[in] microsec - need to delay.
 * @return    none
 */
_attribute_ram_code_sec_noinline_ void delay_us(unsigned int microsec)
{
    unsigned long t = stimer_get_tick();
    while(!clock_time_exceed(t, microsec)){
    }
}

/**
 * @brief     This function performs to set delay time by ms.
 * @param[in] millisec - need to delay.
 * @return    none
 */
_attribute_ram_code_sec_noinline_ void delay_ms(unsigned int millisec)
{
    unsigned long t = stimer_get_tick();
    while(!clock_time_exceed(t, millisec*1000)){
    }
}

/**
 * @brief       This function is used to start the system timer.
 * @param[in]   mode    - starting mode.
 * @param[in]   tick    - The initial value of the tick at startup.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void stimer_enable(stimer_enable_mode_e mode, unsigned int tick)
{
    if(STIMER_MANUAL_MODE == mode)
    {
        stimer_set_manual_enable_mode();
        stimer_set_tick(tick);
        stimer_enable_in_manual_mode();
    }
    else if(STIMER_AUTO_MODE_W_TRIG == mode)
    {
        stimer_set_auto_enable_mode();
        stimer_set_tick(tick);
    }
    else if(STIMER_AUTO_MODE_W_AND_NXT_32K_START == mode)
    {
        stimer_set_run_upon_nxt_32k_enable();   //system tick set upon next 32k posedge.
        stimer_set_auto_enable_mode();
    }
    else if(STIMER_AUTO_MODE_W_AND_NXT_32K_DONE == mode)
    {
        stimer_set_tick(tick);
        //wait command set delay done upon next 32k posedge.
        //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
        stimer_wait_write_done();               //system timer set done status upon next 32k posedge
        stimer_set_run_upon_nxt_32k_disable();  //normal system tick update
    }
}

/**
 * @brief       This function is used to stop the system timer.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void stimer_disable(void)
{
    if(stimer_get_enable_mode())    //auto mode
    {
        if(reg_system_ctrl & FLD_SYSTEM_TIMER_EN)   //abnormal condition
        {
            stimer_set_manual_enable_mode();
            reg_system_ctrl &= ~(FLD_SYSTEM_TIMER_EN);
        }
        else
        {
            reg_system_st = FLD_SYSTEM_CMD_STOP;    //Note: Write the corresponding bit directly here.
            stimer_set_manual_enable_mode();
        }
    }
    else    //manual mode enable
    {
        reg_system_ctrl &= ~(FLD_SYSTEM_TIMER_EN);
    }
}
