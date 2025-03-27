/********************************************************************************************************
 * @file    ext_pm_32k_xtal.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "compiler.h"
#include "compatibility_pack/cmpt.h"
#include "driver.h"
#include "../driver_ext.h"

_attribute_ram_code_com_ int cpu_sleep_wakeup_32k_xtal_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
{
  //todo
   (void)sleep_mode;
   (void)wakeup_src;
   (void)wakeup_tick;

   return 0;
}

_attribute_text_sec_ _attribute_no_inline_ int cpu_sleep_wakeup_32k_xtal(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
{
    //todo
    if(func_before_suspend){
        if (!func_before_suspend())
        {
            return WAKEUP_STATUS_PAD;
        }
    }
    int status = 0;
    DISABLE_BTB;
#if 0 //debug
    status = cpu_sleep_wakeup_32k_xtal_ram(sleep_mode, wakeup_src, wakeup_tick);
#else  //debug
    extern  int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick);
    status = pm_sleep_wakeup_ram(sleep_mode, wakeup_src, PM_TICK_STIMER, wakeup_tick);
#endif
    ENABLE_BTB;
    return status;
}

_attribute_ram_code_com_ unsigned int pm_tim_recover_32k_xtal(unsigned int now_tick_32k)
{
//todo
    (void)now_tick_32k;

    return 0;
}



//_attribute_data_retention_ static unsigned int tick_check32kPad = 0;  // TODO


_attribute_no_inline_ void check_32k_clk_stable(void)
{
    //todo
//    if(clock_time_exceed(tick_check32kPad, 10000)){ //every 10ms, check if 32k pad clk is stable
//        tick_check32kPad = clock_time ();
//
//        unsigned int last_32k_tick;
//        unsigned int curr_32k_tick;
//
//        //Check if 32k pad vibration and basically works stably
//        last_32k_tick = clock_get_32k_tick ();
//        delay_us(50); //for 32k tick accumulator, tick period: 30.5us, if stable: delta tick > 0
//        curr_32k_tick = clock_get_32k_tick();
//
//        if(last_32k_tick != curr_32k_tick){
//            blt_miscParam.pm_enter_en = 1;//allow enter pm
//            return;
//        }
//    }
//    else{
//        return;
//    }
//
//    // T > 2s , 32k pad clk still unstable: reboot MCU
//    if(!blt_miscParam.pm_enter_en && clock_time_exceed(0, 2000000)){
//        analog_write_reg8(SYS_DEEP_ANA_REG, analog_read_reg8(SYS_DEEP_ANA_REG) & (~SYS_NEED_REINIT_EXT32K)); //clr
//        start_reboot(); //reboot the MCU
//    }
}











