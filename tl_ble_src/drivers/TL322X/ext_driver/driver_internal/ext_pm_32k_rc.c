/********************************************************************************************************
 * @file    ext_pm_32k_rc.c
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
#include "lib/include/pm/pm.h"
#include "lib/include/pm/pm_internal.h"
#include "gpio.h"
#include "compiler.h"
#include "../../lib/include/core.h"
#include "../../lib/include/mspi.h"
#include "timer.h"
#include "lib/include/clock.h"
#include "flash.h"
#include "lib/include/stimer.h"

#include "compatibility_pack/cmpt.h"
#include "../ext_pm.h"
#include "ext_lib.h"



#ifndef OS_COMPILE_OPTIMIZE_EN
#define OS_COMPILE_OPTIMIZE_EN                                      0
#endif
extern  unsigned char           g_pm_pad_filter_en;

_attribute_data_retention_sec_ unsigned int  ext_BBTimerTick_BeforeSleep = 0;
_attribute_data_retention_sec_ unsigned int  ext_STimerTick_BeforeSleep = 0;

extern unsigned int  g_pm_multi_addr;
#if PM_32k_RC_CALIBRATION_ALGORITHM_EN

extern void pm_ble_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick);
extern unsigned int pm_ble_get_32k_rc_calib (void);

#endif


_attribute_data_retention_sec_  unsigned int        g_sleep_32k_rc_cnt;
_attribute_data_retention_sec_  unsigned int        g_sleep_stimer_tick;
/**
 * @brief      This function serves to set the working mode of MCU,e.g. suspend mode, deepsleep mode, deepsleep with SRAM retention mode and shutdown mode.
 * @param[in]  sleep_mode - sleep mode type select.
 * @param[in]  wakeup_src - wake up source select.
 * @param[in]  wakeup_tick - the time of short sleep, which means MCU can sleep for less than 5 minutes.
 * @return     indicate whether the cpu is wake up successful.
 */
_attribute_ram_code_sec_optimize_o2_noinline_ int cpu_sleep_wakeup_32k_rc_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
{
//    g_sleep_32k_rc_cnt = 0;
//    g_sleep_stimer_tick = 0;
//    /**
//     * At present, the compensation value in the function is tested on the basis of the optimization level O2. If there are other optimization levels,
//     * it needs to be retested and then see how the compensation time is handled.
//     */
//    /**
//     *  ==============================          -O2 compilation compatibility processing        ==============================
//     *
//     * The pm_sleep_wakeup interface will stop xip, so this function can not have a text section of code called, workaround:
//     * (1) for very short functions add _always_inline (2) for large functions, specify _attribute_ram_code_sec_noinline_.
//     */
//
//    ////////// disable IRQ //////////////////////////////////////////
//    //If the time point of closing the total interrupt is later, the function may be interrupted by the interrupt,
//    //cause the wake-up tick value to be calculated incorrectly, resulting in incorrect sleep time.
//    //modify by weihua.zhang, confirmed by sihui.wang at 20220908.
//    unsigned int r = core_interrupt_disable();
//
//#if PM_START_CODE_DEBUG
//    gpio_set_low_level(GPIO_PB5);
//#endif
//
//    ///////////////////       /////////////////////////////////
//    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
//    if(timer_wakeup_enable)
//    {
//        unsigned int span = (unsigned int)(wakeup_tick - stimer_get_tick());
//        if (span > 0xE0000000) //BIT(31)+BIT(30)+BIT(29)   7/8 cycle of 32bit, 178*7/8 = 156 S
//        {
//            core_restore_interrupt(r);
//            return  pm_get_wakeup_src() | STATUS_EXCEED_MAX;
//        }
//        else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US)
//        {
//            unsigned int t = stimer_get_tick();
//            pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
//            unsigned char st;
//            do
//            {
//                st = pm_get_wakeup_src();
//            }while ( ((unsigned int)stimer_get_tick () - t < span) && !(st & WAKEUP_STATUS_INUSE_ALL));
//
//    #if(PM_DEBUG)
//            /******************************************debug_pm_info 1 **********************************************/
//            debug_pm_info = 1;
//            debug_min_wakeup_src = st;
//            debug_min_stimer_tick = t;
//    #endif
//
//            core_restore_interrupt(r);
//            return st | STATUS_EXCEED_MIN;
//        }
//    }
//
//    //Turn off all interrupts immediately after entering the sleep function to prevent other interrupts, save the interrupt state before turning off, and restore it after waking up.
//    //Turn on pm interrupt only before going to sleep,enable M-mode external interrupt first in this place.
//    //modify by bingyu.li, confirmed by jianzhi.chen at 20230810.
//    plic_irqs_preprocess_for_wfi(0, FLD_MIE_MEIE);
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 2 **********************************************/
//    debug_pm_info = 2;
//#endif
//
//    ///////////////////    change clock    /////////////////////////////////
//    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
//    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
//    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
//    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
//    mspi_stop_xip();
//
//    clock_save_clock_config();
//    clock_set_all_clock_to_default();
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 3 **********************************************/
//    debug_pm_info = 3;
//#endif
//
//    ///////////////////     get 32k calib      /////////////////////////////////
//    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN//BLE ADD. by SunWei
//       g_pm_tick_32k_calib = pm_ble_get_32k_rc_calib ();
//    #else
//   ///////////////////     get 32k calib      ////////////////////////////////
//        while(!stimer_get_tracking_32k_value());   //Wait for the 32k clock calibration to complete.
//
//        g_pm_tick_32k_calib = stimer_get_tracking_32k_value();
//    #endif
//
//
//    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;
//
//#if(PM_DEBUG)
//    analog_write_reg16(PM_ANA_REG_POWER_ON_CLR_BUF1, g_pm_tick_32k_calib);
//    /******************************************debug_pm_info 4 **********************************************/
//    debug_pm_info = 4;
//#endif
//
//    /////////////////// stop system timer /////////////////////////////////
//#if SYS_TIMER_AUTO_MODE
//    stimer_32k_tracking_disable();  //disable 32k track
//    stimer_set_update_upon_nxt_32k_enable();        //system tick only update upon 32k posedge, must set before enable 32k read update!!!
//    g_pm_tick_32k_cur = clock_get_32k_tick();
//    g_pm_tick_cur = stimer_get_tick();
//
//    //TL721 BB time needs requires manual setup to save and restore.
//    ext_BBTimerTick_BeforeSleep  = rf_bb_timer_get_tick();
//    ext_STimerTick_BeforeSleep   = g_pm_tick_cur;
//
//    stimer_set_update_upon_nxt_32k_disable();
//    stimer_disable();   //disable system timer
//#else
//    #error -- Manual mode is only for internal testing, and 37 in the code may not be accurate
//    stimer_32k_tracking_disable();  //disable 32k track
//    g_pm_tick_cur = stimer_get_tick() + 37 * SYSTEM_TIMER_TICK_1US;  //cpu_get_32k_tick will cost 30~40 us
//    stimer_disable();       //disable system timer
//    g_pm_tick_32k_cur = clock_get_32k_tick ();
//#endif
//
//#if PM_START_CODE_DEBUG
//    gpio_set_high_level(GPIO_PB5);
//#endif
//
//#if PM_MIN_CODE_DEBUG
//    gpio_set_low_level(GPIO_PB5);
//#endif
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 5 **********************************************/
//    debug_pm_info = 5;
//#endif
//
//    unsigned int earlyWakeup_us;
//    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)  //deep sleep with retention
//    {
//        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us;
//    }
//    else if(sleep_mode == DEEPSLEEP_MODE)  //deepsleep no retention
//    {
//        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;
//    }
//    else  //suspend
//    {
//        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
//    }
//
//    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
//        pm_ble_update_32k_rc_sleep_tick (g_pm_tick_32k_cur, g_pm_tick_cur);//Todo:need check
//    #endif
//
//    //The variable pmcd.ref_tick is added, replacing the original variable g_pm_tick_cur. Because pmcd.ref_tick directly affects the value of
//    //g_pm_long_suspend, g_pm_long_suspend can be assigned after pmcd.ref_tick is updated.changed by weihua,confirmed by biao.li.20201204.
//    if(timer_wakeup_enable)
//    {
//        #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
//            unsigned int tick_reset;
//            unsigned int tick_wakeup_reset;
//
//            tick_wakeup_reset = wakeup_tick - earlyWakeup_us * SYSTEM_TIMER_TICK_1US;
//            if( (unsigned int)(tick_wakeup_reset - pmbcd.ref_tick) > 0x0fff0000){      // 24M: 11.18S, 16M: 16.77S
//                g_pm_long_suspend = 1;
//            }
//            else{
//                g_pm_long_suspend = 0;
//            }
//            #if 1  //SiHui fix
//            if(g_pm_long_suspend){
//                tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - g_pm_tick_cur)/ g_pm_tick_32k_calib * g_track_32kcnt;
//            }
//            else{
//                tick_reset = g_pm_tick_32k_cur + ((unsigned int)(tick_wakeup_reset - g_pm_tick_cur) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
//            }
//            #endif
//        #else
//            unsigned int tick_reset;
//            unsigned int tick_wakeup_reset;
//
//            tick_wakeup_reset = (unsigned int)(wakeup_tick - (earlyWakeup_us * SYSTEM_TIMER_TICK_1US) - g_pm_tick_cur);
//            //0x0fff0000 is selected for tick_wakeup_reset * g_track_32kcnt do not exceed 32bit,
//            //if more than, first divide and then multiply, if not more than, first multiply and then divide.
//            if(tick_wakeup_reset > 0x0fff0000)      // 24M: 11.18S, 16M: 16.77S
//            {
//                tick_reset = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * g_track_32kcnt;
//                g_pm_long_suspend = 1;
//            }
//            else
//            {
//                tick_reset = g_pm_tick_32k_cur + (tick_wakeup_reset * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
//                g_pm_long_suspend = 0;
//            }
//       #endif
//
//        //The clock_ct_32k_tick interface needs to avoid encountering rising edges of 32k as much as possible.
//        //Otherwise, if the intermediate data generated during the clock_set_32k_tick process has the same value as 32k tick, it will cause the wake-up source to be set.
//        //The interval time between the clock_get_32k_tick() and clock_set_32k_tick() interfaces should be as short as possible
//        //to avoid the clock_set_32k_tick() interface encountering a rising edge of 32k.
//        //add by weihua.zhang at 20240827
//        clock_set_32k_tick(tick_reset);
//
//#if(PM_DEBUG)
//        analog_write_reg32(PM_ANA_REG_WD_CLR_BUF1, g_pm_tick_32k_cur);
//        debug_ana_32k_tick = analog_read_reg32(areg_aon_0x65);
//        if(tick_reset != debug_ana_32k_tick)
//        {
//            debug_ana_tick_reset = tick_reset;
//            stimer_enable_in_manual_mode();
//            stimer_32k_tracking_enable();   //enable 32k cal
//            gpio_set_high_level(GPIO_PE7);
//            while(1);
//        }
//        /******************************************debug_pm_info 6 **********************************************/
//        debug_pm_info = 6;
//#endif
//
//    }
//
//    /////////////////// set sleep ldo  /////////////////////////////////
//    //Must switch to dig ldo to power the three modules before sleep.(modify by weihua.zhang, confirmed by wenfeng.lou at 20240318)
//    /*
//    * Before enter sleep should make sure system power supply all from one LDO.
//    * So if power configuration using CORE_0P7V_SRAM_0P8V_BB_0P7V need to switch to CORE_0P8V_SRAM_0P8V_BB_0P8V.
//    * (add by jilong.liu, confirmed by wenfeng and junwen at 20240808)
//    */
//    unsigned char pm_power_save = pm_get_dig_ldo_voltage();
//    if(pm_power_save == DIG_LDO_TRIM_0P700V){
//        pm_power_supply_config(CORE_0P8V_SRAM_0P8V_BB_0P8V);
//    }
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 7 **********************************************/
//    debug_pm_info = 7;
//#endif
//
//    /////////////////// set wakeup source /////////////////////////////////
//    pm_set_wakeup_src(wakeup_src);
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 8 **********************************************/
//    debug_pm_info = 8;
//#endif
//
//    /////////////////// auto power down /////////////////////////////////
//    unsigned short auto_power_down = FLD_PD_32K_RC|FLD_PD_32K_XTAL|FLD_PD_24M_XTAL|FLD_PD_DCDC|FLD_PD_VBUS_LDO|FLD_PD_ANA_BBPLL_TEMP_LDO\
//                                    |FLD_PD_DCORE_SRAM_LDO|FLD_PD_VBUS_SW|FLD_PD_SEQUENCE_EN;
//    unsigned char sleep_ldo_en = 0;
//
//    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)  //deep sleep with retention
//    {
//        g_pm_multi_addr = reg_mspi_xip_core_size | (reg_mspi_xip_core_offset << 16);//after retention, multiple address offset is lost, save it
//
//        analog_write_reg8(areg_aon_0x7e, sleep_mode);
//
//        auto_power_down |= FLD_PD_ISOLATION;
//        sleep_ldo_en = FLD_PD_DIG_RET_LDO;
//    }
//    else if(sleep_mode == DEEPSLEEP_MODE)  //deepsleep no retention
//    {
//        auto_power_down |= FLD_PD_ISOLATION;
//        sleep_ldo_en = 0;
//    }
//    else  //suspend
//    {
//        sleep_ldo_en = FLD_PD_SPD_LDO;
//    }
//
//    if(!(wakeup_src & PM_WAKEUP_COMPARATOR)){
//        auto_power_down |= FLD_PD_LPC;
//    }
//
//    if(((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_COMPARATOR))
//    {
//        auto_power_down &= ~FLD_PD_32K_RC;  //disable auto power down 32KRC
//    }
//
//    //default:1, 0 Power up ; 1 power down;
//    //areg_aon_0x06 need to restore after wake up
//    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) | FLD_PD_BBPLL_LDO | FLD_PD_SPD_LDO | FLD_PD_DIG_RET_LDO) & (~sleep_ldo_en));
//    sys_clk_config.bbpll_is_used = g_bbpll_is_used;
//    g_bbpll_is_used = 0;
//    analog_write_reg16(areg_aon_0x4c, auto_power_down);
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 9 **********************************************/
//    debug_pm_info = 9;
//#endif
//
//    /////////////////// R DELAY AND XTAL DELAY /////////////////////////////////
//    if(sleep_mode == DEEPSLEEP_MODE)
//    {
//        pm_set_delay_cycle(g_pm_r_delay_cycle.deep_xtal_delay_cycle, g_pm_r_delay_cycle.deep_r_delay_cycle);
//    }
//    else
//    {
//        pm_set_delay_cycle(g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);
//    }
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 10 **********************************************/
//    debug_pm_info = 10;
//#endif
//
//    //Clear the wake source status after setting the wake tick.The wake tick value is set by bit shift.
//    //This process will generate an intermediate value, which may be the same as the current 32k tick value.
//    //If the value is the same, the state of the timer wake source will be set.
//    //changed by weihua, confirmed by jianzhi. 20240711.
//    pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);               //clear all flag
//
//#if(PM_DEBUG)
//    debug_sleep_start_wakeup_src1 = pm_get_wakeup_src();
//    /******************************************debug_pm_info 11 **********************************************/
//    debug_pm_info = 11;
//#endif
//
//    if(pm_get_wakeup_src() & WAKEUP_STATUS_INUSE_ALL){
//
//#if(PM_DEBUG)
//        debug_sleep_start_wakeup_src2 = pm_get_wakeup_src();
//        debug_sleep_start_cur_tick = analog_read_reg32(0x60);
//        debug_sleep_start_set_tick = analog_read_reg32(0x65);
//#endif
//
//    }else{
//
//        if(sleep_mode & DEEPSLEEP_RETENTION_FLAG){
//            g_areg_aon_7f = (g_areg_aon_7f & (~FLD_BOOTFROMBROM)) | g_pm_pad_filter_en;
//        }else{
//            g_areg_aon_7f = (g_areg_aon_7f | FLD_BOOTFROMBROM | g_pm_pad_filter_en);
//        }
//        analog_write_reg8(areg_aon_0x7f, g_areg_aon_7f);
//
//        extern void pm_sleep_start(pm_sleep_mode_e sleep_mode);
//        pm_sleep_start(sleep_mode);
//    }
//
//#if(PM_DEBUG)
//        /******************************************debug_pm_info 12 **********************************************/
//    debug_pm_info = 12;
//#endif
//
//    if(sleep_mode == DEEPSLEEP_MODE){
//        sys_reset_all();  //reboot
//    }
//
//    if(pm_power_save == DIG_LDO_TRIM_0P700V){
//        pm_power_supply_config(CORE_0P7V_SRAM_0P8V_BB_0P7V);
//    }
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 13 **********************************************/
//    debug_pm_info = 13;
//#endif
//
////    pm_stimer_recover();
//
//    extern void pm_tim_recover_32k_rc(void);
//    pm_tim_recover_32k_rc();
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 14 **********************************************/
//    debug_pm_info = 14;
//#endif
//
//    clock_restore_clock_config();
//
//    mspi_set_xip_en();
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 15 **********************************************/
//    debug_pm_info = 15;
//#endif
//
//#if PM_SUSPEND_WHILE_DEBUG_2
//    gpio_set_high_level(GPIO_PB5);
//#endif
//
//#if PM_SUSPEND_WHILE_DEBUG
//    gpio_set_low_level(GPIO_PB5);
//#endif
//    if((g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable)  //wakeup from timer only
//    {
//          while((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30));
//    }
//
//#if PM_SUSPEND_WHILE_DEBUG
//    gpio_set_high_level(GPIO_PB5);
//#endif
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 16 **********************************************/
//    debug_pm_info = 16;
//#endif
//
//    //  DBG_CHN2_LOW;
//    //Resume the interrupted state before sleep.Cannot be placed in the pm_sleep_start() interface to avoid failure to recover if this interface is not called.
//    //changed by weihua, confirmed by jianzhi. 20231115
//    plic_irqs_postprocess_for_wfi();
//    core_restore_interrupt(r);
//
//#if(PM_DEBUG)
//    /******************************************debug_pm_info 17 **********************************************/
//    debug_pm_info = 17;
//#endif
//
//    /**
//     * Under normal circumstances, the wake up source cannot be zero. B85 had an exception that the wake up source was zero and never encountered it again.
//     * STATUS_GPIO_ERR_NO_ENTER_PM indicates this case where the wake up source is zero. The name of this flag is not quite appropriate, but it has been used for a long time, so it is still used today.
//     * added by bingyu.li, confirmed by sihui.wang at 20231018.
//     */
    return STATUS_GPIO_ERR_NO_ENTER_PM; //todo:xh
    //return (g_pm_status_info.wakeup_src ? (g_pm_status_info.wakeup_src | STATUS_ENTER_SUSPEND) : STATUS_GPIO_ERR_NO_ENTER_PM);
}

_attribute_text_sec_ _attribute_no_inline_ int cpu_sleep_wakeup_32k_rc(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
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
#if  1  //debug
    status = cpu_sleep_wakeup_32k_rc_ram(sleep_mode, wakeup_src, wakeup_tick);
#else  //debug
    extern  int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick);
    status = pm_sleep_wakeup_ram(sleep_mode, wakeup_src, PM_TICK_STIMER, wakeup_tick);
#endif
    ENABLE_BTB;
    return status;
}


_attribute_ram_code_sec_optimize_o2_noinline_ void pm_tim_recover_32k_rc(void)
{
//    unsigned int wakeup_tick;
//    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_START, 0);
//    unsigned int now_tick_32k = clock_get_32k_tick();
//    {
//    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
//        if(g_pm_long_suspend){
//            wakeup_tick = pmbcd.ref_tick  + (unsigned int)(now_tick_32k - pmbcd.ref_tick_32k) / g_track_32kcnt * g_pm_tick_32k_calib;
//        }
//        else{
//            wakeup_tick = pmbcd.ref_tick  + (unsigned int)(now_tick_32k - pmbcd.ref_tick_32k) * g_pm_tick_32k_calib / g_track_32kcnt;      // current clock
//        }
//    #else
//        if(g_pm_long_suspend){
//             wakeup_tick = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 16 * g_pm_tick_32k_calib;
//        }
//        else{
//            wakeup_tick = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / 16;     // current clock
//        }
//    #endif
//    }
//    g_sleep_32k_rc_cnt = now_tick_32k - g_pm_tick_32k_cur;
//    g_sleep_stimer_tick = wakeup_tick - g_pm_tick_cur;
//
//    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
//        pmbcd.rc32_wakeup = now_tick_32k;
//        pmbcd.rc32 = now_tick_32k - pmbcd.ref_tick_32k;
//    #endif
//    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_DONE, wakeup_tick + 1);
//
//    stimer_32k_tracking_enable();           //enable 32k cal
//    //return wakeup_tick;

}

_attribute_text_sec_ _attribute_no_inline_ int cpu_long_sleep_wakeup_32k_rc(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
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
#if  0  //debug
    status = cpu_sleep_wakeup_32k_rc_ram(sleep_mode, wakeup_src, wakeup_tick);
#else  //debug
    extern  int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick);
    status = pm_sleep_wakeup_ram(sleep_mode, wakeup_src, PM_TICK_STIMER, wakeup_tick);
#endif
    ENABLE_BTB;
    return status;
}
#if 0
int cpu_long_sleep_wakeup_32k_rc(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
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
    status = cpu_sleep_wakeup_32k_rc_ram(sleep_mode, wakeup_src, wakeup_tick);
#else  //debug
    extern  int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick);
    status = pm_sleep_wakeup_ram(sleep_mode, wakeup_src, PM_TICK_STIMER, wakeup_tick);
#endif
    ENABLE_BTB;
    return status;

    int sys_tick0 = clock_time();
    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
        g_pm_tick_32k_calib = pm_ble_get_32k_rc_calib ();
    #else
        while(!read_reg32(0x140214));
        g_pm_tick_32k_calib = read_reg32(0x140214);
    #endif
    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;
    unsigned int span = (unsigned int)(wakeup_tick);
    if(timer_wakeup_enable){
        if (span < ((g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib)){
            analog_write_reg8 (0x64, 0x1d);         //(clear all status 0x1f) but clear the timer wake_up status(bit[1])
                                                //before read 32k tick may cause the tick number error get from 32K.
            unsigned char st;
            do {
                st = analog_read_reg8 (0x64) & 0x1d;   //clear the timer wake_up status(bit[1]) before read 32k tick may
                                                  //cause the tick number error get from 32K.
            } while ( ((((unsigned int)clock_time () - sys_tick0 + tick_32k_halfCalib )/g_pm_tick_32k_calib) < span) && !st);
            return st;
        }
    }
    g_pm_long_suspend = 1;
    ////////// disable IRQ //////////////////////////////////////////
    unsigned int r= core_interrupt_disable();

    if(func_before_suspend){
        if (!func_before_suspend())
        {
            core_restore_interrupt(r);
            return WAKEUP_STATUS_PAD;
        }
    }

    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    unsigned char cclk_reg = read_reg8(0x1401e8);
    write_reg8(0x1401e8, cclk_reg & 0x8f );//change cclk to 24M rc clock
    unsigned char div_reg = read_reg8(0x1401d8);
    write_reg8(0x1401d8, div_reg & 0xf8);//change clock division

#if SYS_TIMER_AUTO_MODE
    BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer

    REG_ADDR8(0x140218) = 0x01;//sys tick only update upon 32k posedge,must set before enable 32k read update!!!

    BM_CLR(reg_system_ctrl,FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal

    //g_pm_tick_32k_cur = clock_get_digital_32k_tick();
    g_pm_tick_32k_cur=clock_get_32k_tick();
    reg_system_st = FLD_SYSTEM_CMD_STOP;//stop stimer
    g_pm_tick_cur = clock_time ();
#else
    g_pm_tick_cur = clock_time () + 37 * SYSTEM_TIMER_TICK_1US;  //clock_get_32k_tick will cost 30~40 us

    BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer
    BM_CLR(reg_system_ctrl,FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal and stimer

    //g_pm_tick_32k_cur = clock_get_digital_32k_tick();
    g_pm_tick_32k_cur = clock_get_32k_tick();
#endif
    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
        pmbcd.calib = 1;
        pm_ble_update_32k_rc_sleep_tick (g_pm_tick_32k_cur, g_pm_tick_cur);
    #endif
    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8 (0x4b, wakeup_src);
    analog_write_reg8 (0x64, 0x1f);             //clear all flag


    analog_write_reg8(0x7e, sleep_mode);//sram retention

    unsigned int earlyWakeup_us;
#if !SYS_TIMER_AUTO_MODE
    unsigned int tick_adjust_us = 0;
#endif
    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG) { //deepsleep with retention
        //0x00->0xd1
        //<0>pd_rc32k_auto=1 <4>pwdn power suspend ldo=1
        //<6>power down sequence enable=1 <7>enable isolation=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0xd0);//retention
        }
        else
        {
            analog_write_reg8(0x4d,0xd1);//retention
        }
        #if (!WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
            analog_write_reg8(0x7f, 0x00);
        #endif

        g_pm_mspi_cfg = read_reg32(0x23FFFF20);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*((wakeup_tick - g_pm_tick_cur)/160000))*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = ((g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib);
    }
    else if(sleep_mode == DEEPSLEEP_MODE){  //deepsleep no retention
        //0x00->0xf9
        //<0>pd_rc32k_auto=1 <3>rst_xtal_quickstart_cnt=1 <4>pwdn power suspend ldo=1
        //<5>pwdn power retention ldo=1 <6>power down sequence enable=1 <7>enable isolation=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0xf8);//deep
        }
        else
        {
            analog_write_reg8(0x4d,0xf9);//deep
        }
        analog_write_reg8(0x7f, 0x01);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*(wakeup_tick - g_pm_tick_cur)/160000)*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = ((g_pm_early_wakeup_time_us.deep_early_wakeup_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib);
    }
    else{  //suspend
        //0x00->0x61
        //<0>pd_rc32k_auto=1 <5>pwdn power retention ldo=1 <6>power down sequence enable=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0x60);//suspend
        }
        else
        {
            analog_write_reg8(0x4d,0x61);//suspend
        }
        analog_write_reg8(0x7f, 0x01);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*(wakeup_tick - g_pm_tick_cur)/160000)*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = ((g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib);
    }

    unsigned int tick_wakeup_reset = wakeup_tick - earlyWakeup_us;
    //auto power down
    if((wakeup_src & PM_WAKEUP_TIMER)/*|| (wakeup_src & PM_WAKEUP_MDEC)*/|| (wakeup_src & PM_WAKEUP_COMPARATOR) ){
        analog_write_reg8(0x4c,0xee);
    }
    else{
        analog_write_reg8(0x4c, 0xef);
    }

    //set DCDC delay duration
#if 1 //2 * 1/16k = 125 uS
    if(sleep_mode == DEEPSLEEP_MODE){
        analog_write_reg8 (0x40, g_pm_r_delay_cycle.deep_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }else{
        analog_write_reg8 (0x40, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }
#else
    span = (PM_DCDC_DELAY_DURATION * (SYSTEM_TIMER_TICK_1US>>1) * 16 + tick_32k_halfCalib)/ g_pm_tick_32k_calib;
    unsigned char rst_cycle = 0xff - span;
    analog_write (0x1f, rst_cycle);
#endif
    unsigned int tick_reset;
#if PM_32k_RC_CALIBRATION_ALGORITHM_EN

    if(g_pm_long_suspend){
        tick_reset = pmbcd.ref_tick_32k + (unsigned int)(tick_wakeup_reset - ((pmbcd.ref_tick - sys_tick0)/ g_pm_tick_32k_calib * 16));
    }
    else{
        tick_reset = pmbcd.ref_tick_32k + (unsigned int)(tick_wakeup_reset - (((pmbcd.ref_tick - sys_tick0) * 16 + tick_32k_halfCalib) / g_pm_tick_32k_calib));
    }

#else
#if !SYS_TIMER_AUTO_MODE
    if(g_pm_long_suspend){
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - ((tick_adjust_us + (g_pm_tick_cur -sys_tick0))/ g_pm_tick_32k_calib * 16));
    }
    else{
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - (((tick_adjust_us + (g_pm_tick_cur - sys_tick0)) * 16 + tick_32k_halfCalib) / g_pm_tick_32k_calib));
    }
#else
    if(g_pm_long_suspend){
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset  - ((g_pm_tick_cur - sys_tick0)/ g_pm_tick_32k_calib * 16));
    }
    else{
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - (((g_pm_tick_cur - sys_tick0) * 16 + tick_32k_halfCalib) / g_pm_tick_32k_calib));
    }
//  tick_reset = g_pm_tick_32k_cur + ((unsigned int)(wakeup_tick - g_pm_tick_cur) * 16 + tick_32k_halfCalib - tick_32k_halfCalib ) / g_pm_tick_32k_calib  - 1 - 3*2 - 2;
#endif
#endif

    clock_set_32k_tick(tick_reset);

    if(analog_read_reg8(0x64)&0x1f){

    }
    else{
        #if (WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
            if(sleep_mode & DEEPSLEEP_RETENTION_FLAG) { //deepsleep with retention
                analog_write_reg8(0x7f, 0x00);
            }
        #endif
        DBG_CHN0_LOW;
        pm_sleep_start(sleep_mode);
        DBG_CHN0_HIGH;
        #if (WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
            analog_write_reg8(0x7f, 0x01);
        #endif
    }
    if(sleep_mode == DEEPSLEEP_MODE){
       write_reg8 (0x1401ef, 0x20);  //reboot
    }
    reg_system_irq_mask |= BIT(0);
#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x02;//sys tick 16M set upon next 32k posedge
    reg_system_ctrl     |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN) ;

    //unsigned int now_tick_32k = clock_get_digital_32k_tick() + 1;
    unsigned int now_tick_32k = clock_get_32k_tick() + 1;
    if(g_pm_long_suspend){
        g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 16 * g_pm_tick_32k_calib;
    }
    else{
        g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / 16;       // current clock
    }
#if PM_32k_RC_CALIBRATION_ALGORITHM_EN
    pmbcd.rc32_wakeup = now_tick_32k;
    pmbcd.rc32 = now_tick_32k - pmbcd.ref_tick_32k;
#endif
    reg_system_tick = g_pm_tick_cur + 1;
    //But when cclk is set to 96M, pclk is the quarter frequency of cclk to 24M, but cclk will switch to 24M RC before the program enters sleep,
    //pclk is still the quarter frequency of cclk, so pclk becomes 6M, reading 32k The tick value slows down by about four times;
    //writing a 16M tick value and reading a 32k rising edge are likely to occur at the same time, causing the 32k rising edge flag to fail to be read.
    //modify by junhui.hu, confirmed by jianzhi at 20210923.
    //wait cmd set dly done upon next 32k posedge
    //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
    while((reg_system_st & BIT(7)) == 0);
    REG_ADDR8(0x140218) = 0;//normal sys tick (16/sys) update

#else
    //unsigned int now_tick_32k = clock_get_digital_32k_tick();
    unsigned int now_tick_32k = clock_get_32k_tick();
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 16 * g_pm_tick_32k_calib;
        }
        else{
            g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / 16;       // current clock
        }
    }

    reg_system_tick = g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US;
    reg_system_ctrl |= FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN;    //enable 32k cal and stimer
#endif

    unsigned char anareg64 = analog_read_reg8(0x64);
    write_reg8(0x1401d8, div_reg);
    write_reg8(0x1401e8, cclk_reg );//restore cclk


    core_restore_interrupt(r);
    return (anareg64 ? (anareg64 | STATUS_ENTER_SUSPEND) : STATUS_GPIO_ERR_NO_ENTER_PM );


}
#endif
