/********************************************************************************************************
 * @file    pm.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "core.h"
#include "mspi.h"
#include "clock.h"
#include "flash.h"
#include "stimer.h"
#include "watchdog.h"

#if (!defined(MCU_CORE_TL751X_N22))

#if(PM_DEBUG)
volatile unsigned char debug_pm_info;
volatile unsigned int debug_ana_32k_tick;
volatile unsigned int debug_ana_tick_reset;
volatile unsigned char debug_ana_reg64_value[3];
volatile unsigned int debug_ana_32k_tick_cur[3];
volatile unsigned int debug_ana_32k_tick_set[3];
volatile unsigned char debug_wakeup_src_clr_err=0;
#endif

extern unsigned char pll_vco_itrim;
extern unsigned char g_power_config;

_attribute_aligned_(4) pm_status_info_s g_pm_status_info;
volatile unsigned char g_pm_reboot_event=0;
#define PM_IRQ_STATUS_MAX_CLR_TIMES                     3
//system timer clock source is constant 24M, never change
//NOTICE:We think that the external 32k crystal clk is very accurate, does not need to read through TIMER_32K_LAT_CAL.
//register, the conversion error(use 32k:64 cycle, count 24M sys tmr's ticks), at least the introduction of 64ppm.
#define CRYSTAL32768_TICK_PER_64CYCLE       46875

_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_calib;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_cur;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_cur;
_attribute_data_retention_sec_  unsigned char           g_pm_long_suspend;
_attribute_data_retention_sec_  unsigned char           g_pm_vbat_v;
_attribute_data_retention_sec_  unsigned char           g_pm_tick_update_en=1;
_attribute_data_retention_sec_  static unsigned char    g_pm_suspend_power_cfg=(FLD_PD_ZB_EN|FLD_PD_USB_EN|FLD_PD_AUDIO_EN|FLD_PD_NPE_EN|FLD_PD_DSP_EN|FLD_PD_WT_EN);
_attribute_data_retention_sec_  static unsigned char    g_pm_pad_filter_en=0x00;
_attribute_data_retention_sec_  unsigned int            g_pm_multi_addr=0;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_calib_repair=0;
_attribute_data_retention_sec_  unsigned int            g_pm_xtal_stable_loopnum = 10;
_attribute_data_retention_sec_  unsigned int            g_pm_suspend_delay_us = 200;
_attribute_data_retention_sec_  unsigned int            g_pm_interrupt_status = 0;
_attribute_data_retention_sec_  unsigned int            g_pm_interrupt_status1 = 0;
_attribute_data_retention_sec_  unsigned int            g_pm_interrupt_status2 = 0;
_attribute_data_retention_sec_
volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us = {
    .suspend_early_wakeup_time_us = 188 + 109 + 200 + 300,  //188(r_delay) + 109(3.5*(1/32k)) + 200(XTAL_delay) + 300(code)
    .deep_ret_early_wakeup_time_us = 188 + 109,             //188(r_delay) + 109(3.5*(1/32k))
    .deep_early_wakeup_time_us = 688 + 109 + 380,           //688(r_delay) + 109(3.5*(1/32k)) + 380(boot_rom)
    .sleep_min_time_us = 688 + 109 + 380 + 200,             //(the maximum value of suspend and deep) + 200. 200 means more margin, >32 is enough.
};
_attribute_data_retention_sec_
volatile pm_r_delay_cycle_s g_pm_r_delay_cycle = {
    .deep_r_delay_cycle = 4 + 8,        // 11 * (1/16k) = 687.5
    .suspend_ret_r_delay_cycle = 4,     // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
    .deep_xtal_delay_cycle = 2 + 8,     // 11 * (1/16k) = 687.5
    .suspend_ret_xtal_delay_cycle = 2,  // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
};

extern _attribute_ram_code_sec_ void flash_send_cmd(unsigned long addr, unsigned int cmd);

/**
 * @brief       This function configures pm wakeup time parameter.
 * @param[in]   param - deep/suspend/deep_retention r_delay time.(default value: suspend/deep_ret=3, deep=11)
 * @return      none.
 */
void pm_set_wakeup_time_param(pm_r_delay_cycle_s param)
{
    g_pm_r_delay_cycle.deep_r_delay_cycle = param.deep_r_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_r_delay_cycle = param.suspend_ret_r_delay_cycle;
    g_pm_r_delay_cycle.deep_xtal_delay_cycle = param.deep_xtal_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle = param.suspend_ret_xtal_delay_cycle;

    int deep_rx_delay_us = g_pm_r_delay_cycle.deep_r_delay_cycle *1000 /16;
    int suspend_ret_rx_delay_us = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle *1000 /16;
    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us = suspend_ret_rx_delay_us + 109 + 200 + 300;
    g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us = suspend_ret_rx_delay_us + 109;
    g_pm_early_wakeup_time_us.deep_early_wakeup_time_us = deep_rx_delay_us + 109 + 380;
    if(g_pm_early_wakeup_time_us.deep_early_wakeup_time_us < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us)
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + 200;
    }
    else
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + 200;
    }
}

/**
 * @brief       This function is used in applications where the crystal oscillator is relatively slow to start.
 *              When the start-up time is very slow, you can call this function to avoid restarting caused
 *              by insufficient crystal oscillator time (it is recommended to leave a certain margin when setting).
 * @param[in]   delay_us - The time wait for xtal stable and flash restore to the active working state in the ramcode
 *                          when wakeup from suspend sleep (default value: 200).
 * @param[in]   loopnum - The time for the crystal oscillator to stabilize is approximately: loopnum*40us(default value: loopnum=10).
 * @return      none.
 * @note        Those parameters will be lost after reboot or deep sleep, so it required to be reconfigured.
 */
void pm_set_xtal_stable_timer_param(unsigned int delay_us, unsigned int loopnum)
{
    g_pm_xtal_stable_loopnum = loopnum;
    g_pm_suspend_delay_us = delay_us;

    int suspend_ret_r_delay_us = (g_pm_r_delay_cycle.suspend_ret_r_delay_cycle + 1) *1000 /16;
    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us = suspend_ret_r_delay_us + delay_us + 200;
    if(g_pm_early_wakeup_time_us.deep_early_wakeup_time_us < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us)
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + 200;
    }
    else
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + 200;
    }
}

/**
 * @brief       This function serves to recover system timer.
 *              The code is placed in the ram code section, in order to shorten the time.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_  void pm_stimer_recover(void)
{
#if SYS_TIMER_AUTO_MODE
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_START, 0);
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(CLK_32K_RC == g_clk_32k_src)
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    }
    else
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_DONE, g_pm_tick_cur + 1);
    stimer_32k_tracking_enable();           //enable 32k cal

#else
    #error -- only for internal testing.
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(CLK_32K_RC == g_clk_32k_src)
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    }
    else
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }
    stimer_enable(STIMER_MANUAL_MODE, g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US);
    stimer_32k_tracking_enable();   //enable 32k cal
#endif
}

/**
 * @brief       This function configures a GPIO pin as the wakeup pin.
 * @param[in]   pin - the pin needs to be configured as wakeup pin.
 * @param[in]   pol - the wakeup polarity of the pad pin(0: low-level wakeup, 1: high-level wakeup).
 * @param[in]   en  - enable or disable the wakeup function for the pan pin(1: enable, 0: disable).
 * @return      none.
 */
void pm_set_gpio_wakeup (gpio_pin_e pin, pm_gpio_wakeup_level_e pol, int en)
{
    ///////////////////////////////////////////////////////////
    //        PA[7:0]        PB[7:0]        PC[7:0]        PD[7:0]        PE[7:0]        PF[7:0]        PG[7:0]        PH[6:0]
    // en:  ana_0x9e<7:0>  ana_0x9f<7:0>  ana_0xa0<7:0>  ana_0xa1<7:0>  ana_0xa2<7:0>  ana_0xa3<7:0>  ana_0xa4<7:0>  ana_0xa5<6:0>
    // pol: ana_0x94<7:0>  ana_0x95<7:0>  ana_0x96<7:0>  ana_0x97<7:0>  ana_0x98<7:0>  ana_0x99<7:0>  ana_0x9a<7:0>  ana_0x9b<6:0>
    unsigned char mask = pin & 0xff;
    unsigned char areg;
    unsigned char val;

    ////////////////////////// polarity ////////////////////////
    areg = ((pin>>8) + 0x94);
    val = analog_read_reg8(areg);
    if (pol) {
        val &= ~mask;
    }
    else {
        val |= mask;
    }
    analog_write_reg8 (areg, val);

    /////////////////////////// enable /////////////////////
    areg = ((pin>>8) + 0x9e);
    val = analog_read_reg8(areg);
    if (en) {
        val |= mask;
    }
    else {
        val &= ~mask;
    }
    analog_write_reg8 (areg, val);
}

/**
 * @brief       This function servers to wait bbpll clock lock.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_wait_bbpll_done(void)
{
    //The previous chip could adjust the voltage of pll ldo, this chip doesn't have that function
    analog_write_reg8(areg_0x105, analog_read_reg8(areg_0x105) | FLD_LOCK_DET_SIG_ENABLE);//lock detect enable signal
    analog_write_reg8(areg_0x106, analog_read_reg8(areg_0x106) | FLD_LOCK_DET_SIG_RESET);//lock detect normal signal

    if (g_chip_version == CHIP_VERSION_A0) {
        unsigned char trim_ok = 0;

        /*
        * The default vco ibias value may not be sufficient to support PLL stability.
        * In this case, it is necessary to try adjusting the trim value until a stable PLL signal is obtained.
        * Note: If the obtained gear value here is relatively marginal, it may still affect the stability of PLL 
        * in the case of large temperature changes in the future.
        * The Onca chip select 192MHz as PLL frequency, so the start point vco is 111(7).
        * (add by jilong.liu, confirmed by yangya 20240130)
        */
        for (unsigned char vco_trim = pll_vco_itrim; vco_trim > 0; vco_trim--) {
            core_cclk_delay_tick(sys_clk.cclk_hclk * 20);//20us, wait vco ibias trim stable
            for (unsigned char i = 0; i < 3; i++) {
                if (FLD_BBPLL_LOCK_DETECTOR == (analog_read_reg8(areg_0x108) & FLD_BBPLL_LOCK_DETECTOR)) {
                    trim_ok = 1;
                    break;
                }   
            }
            if (1 == trim_ok) {
                break;
            }
            analog_write_reg8(areg_0x104, (analog_read_reg8(areg_0x104) & 0x1f) | ((vco_trim - 1) << 5));
        }
        //TODO 
        /*
            The current temporary solution is while(0 == trim_ok) to make it easier to identify issues.
            If this solution is really adopted in the follow-up, will change it to system reboot.
        */
        while(0 == trim_ok);
    } else if (g_chip_version == CHIP_VERSION_A1) {
        while(FLD_BBPLL_LOCK_DETECTOR != (analog_read_reg8(areg_0x108) & FLD_BBPLL_LOCK_DETECTOR));
    }

    analog_write_reg8(areg_0x105, analog_read_reg8(areg_0x105) & (~FLD_LOCK_DET_SIG_ENABLE));//lock detect disable signal
    analog_write_reg8(areg_0x106, analog_read_reg8(areg_0x106) & (~FLD_LOCK_DET_SIG_RESET));//lock detect reset signal
}

/**
 * @brief       This function servers to wait bbpll to audio clock lock.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_wait_audio_pll_done(void)
{
    unsigned char ana_151 = analog_read_reg8(0x151);
    analog_write_reg8(0x151, ana_151 | 0x06);//lock detect enable signal, normal

    while(0 == (analog_read_reg8(0x151) & BIT(4)));//audio_bbpll_lockdet

    analog_write_reg8(0x151, analog_read_reg8(0x151) & 0xf9);//lock detect disable signal, reset
}

/**
 * @brief       This function servers to powen on bbpll to audio clock lock.
 * @return      none.
 */
void pm_audio_pll_power_on(void)
{
    unsigned char ana_09 = analog_read_reg8(0x09);
    analog_write_reg8(0x09, ana_09 & (~BIT(4)));//power on of bbpll to audio

    unsigned char ana_15e = analog_read_reg8(0x15e);
    analog_write_reg8(0x15e, ana_15e | 0x51);//enable dsm_fcal_rstn dsm_en, bbpll frequency coarse tuning enable

    delay_us(20);

    analog_write_reg8(0x15e, analog_read_reg8(0x15e) & (~BIT(6)));//bbpll frequency coarse tuning disable
}

/**
 * @brief       This function servers to powen down bbpll to audio clock lock.
 * @return      none.
 */
void pm_audio_pll_power_down(void)
{
    unsigned char ana_09 = analog_read_reg8(0x09);
    analog_write_reg8(0x09, ana_09 | BIT(4));//power down of bbpll to audio

    unsigned char ana_15e = analog_read_reg8(0x15e);
    analog_write_reg8(0x15e, ana_15e & 0xee);//disable dsm_fcal_rstn dsm_en
}

/**
 * @brief       This function is used to determine the stability of the crystal oscillator.
 *              To judge the stability of the crystal oscillator, xo_ready_ana is invalid, and use an alternative solution to judge.
 *              Alternative principle: Because the clock source of the stimer is the crystal oscillator,
 *              if the crystal oscillator does not vibrate, the tick value of the stimer does not increase or increases very slowly (when there is interference).
 *              So first use 24M RC to run the program and wait for a fixed time, calculate the number of ticks that the stimer should increase during this time,
 *              and then read the tick value actually increased by the stimer.
 *              When it reaches 50% of the calculated value, it proves that the crystal oscillator has started.
 *              If it is not reached for a long time, the system will reboot.
 * @param[in]   all_ramcode_en  - Whether all processing in this function is required to be ram code. If this parameter is set to 1, it requires that:
 *              before calling this function, you have done the disable BTB, disable interrupt, mspi_stop_xip and other operations as the corresponding function configured to 0.
 * @attention   This function can only be called with the 24M clock configuration
 * @return      none.
 */
_attribute_ram_code_sec_ void pm_wait_xtal_ready(unsigned char all_ramcode_en)
{
    //When adding this feature to each chip, it is important to note the following:
    //1.The percentage deviation of high and low temperatures for 24M RC, according to Eagle's test results, is:
    //2.Check the PM_ANA_REG_POWER_ON_CLR_BUF0 bit definition to ensure there are no conflicts, and add comments
    //3.Ensure that this function is called after CCLK switches to RC clock
    //4.Test how long this function will cost to make sure it can cover the poor quality crystal.
    volatile unsigned int j, t0;
    reg_system_ctrl |= FLD_SYSTEM_TIMER_EN;

    //The method to determine xtal stability here is to use the RC counting method. 
    //After the RC delay of 40us, check whether the stimer count value exceeds 20us (i.e. more than 50% of the rc count value) which is considered to have successfully started vibration. 
    //The setting of 10 times is to cover crystal oscillators with poor quality and slow onset as much as possible. 
    //Currently, no worse situation has been encountered in use, so the maximum value is set to 10 times.
    //(added by jilong.liu, confirmed by wenfeng.lou at 20240410)
    for(j = 0; j < g_pm_xtal_stable_loopnum; j++)
    {
        t0 = stimer_get_tick();
        //This delay time is about 40.38us under the calibrated 24M RC clock.
        //Note: Ensure that the current clock is 24M RC, otherwise the nop running time will not be 40us, resulting in a later calculation error and system reboot.
        //(add by weihua.zhang, confirmed by peng.sun 20230609)
        core_cclk_delay_tick(sys_clk.cclk_hclk * 40);//40us
        if(stimer_get_tick() - t0 > SYSTEM_TIMER_TICK_1US * 20)
        {
            break;
        }
    }

    if(j == g_pm_xtal_stable_loopnum)
    {
        //Use PM_ANA_REG_POWER_ON_CLR_BUF0[bit1] to check whether there has been a reset caused by the instability of the crystal oscillator.
        //If it is 1, it has occurred.
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0)|0x02);
        if(all_ramcode_en == 0x00)
        {
            sys_reboot();
        }
        else
        {
            sys_reset_all();
            while(1);
        }
    }
    reg_system_ctrl &= (~FLD_SYSTEM_TIMER_EN);
}

/**
 * @brief       This function serves to update wakeup status.
 * @return      none.
 */
void pm_update_status_info(void)
{
    unsigned char analog_35 = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);
    unsigned char analog_3a = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0);
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.is_pad_wakeup = (g_pm_status_info.wakeup_src & BIT(0));
    if(analog_35 & BIT(0)){
        if(analog_3a & BIT(0)){
            g_pm_status_info.mcu_status = MCU_STATUS_REBOOT_BACK;
            g_pm_reboot_event = SW_SYSTEM_REBOOT;
            if(wd_get_status()){
                g_pm_reboot_event = HW_TIMER_WATCHDOG_REBOOT;
            }
            analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, analog_35 & 0xfe);
        }else{
            g_pm_status_info.mcu_status = MCU_STATUS_POWER_ON;
            if(wd_32k_get_status()){
                g_pm_reboot_event = HW_32K_WATCHDOG_REBOOT;
                g_pm_status_info.mcu_status = MCU_STATUS_REBOOT_BACK;
            }
            analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0,analog_35 & 0xfe);
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_3a | BIT(0));
        }
    }else{
        if(pm_get_deep_retention_flag()){
            g_pm_status_info.mcu_status = MCU_STATUS_DEEPRET_BACK;
        }else{
            g_pm_status_info.mcu_status = MCU_STATUS_DEEP_BACK;
        }
    }
    analog_write_reg8(0x7f, (0x45 | g_pm_pad_filter_en));
}

/**
 * @brief       This function serves to get reboot status.
 * @return      reboot enum element of pm_reboot_event_e.
 * @note        -# if return HW_TIMER_WATCHDOG_REBOOT, need call wd_clear_status() to avoid affecting the next detection of the mcu status;
 *              -# if return HW_32K_WATCHDOG_REBOOT,need call wd_32k_clear_status() to avoid affecting the next detection of the mcu status;
 *              -# if return HW_VBUS_DETECT_REBOOT,need to write 1 and 0 for 0x64(bit7) to avoid affecting the next detection of the mcu status;
 *              -# the interface sys_init() must be called before this interface can be invoked;
 */
pm_reboot_event_e pm_get_reboot_event(void){

    return g_pm_reboot_event;
}

/**
 * @brief       this function serves to clear all irq status.
 * @return      Indicates whether clearing irq status was successful.
 */
_attribute_ram_code_sec_ unsigned char pm_clr_all_irq_status(void)
{
    unsigned char j, ana_reg64 = 0xff;
    for(j =0; j <PM_IRQ_STATUS_MAX_CLR_TIMES; j++)
    {
        pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
        ana_reg64 = pm_get_wakeup_src();
        if(ana_reg64 == 00){
            break;
        }else{
#if(PM_DEBUG)
            debug_wakeup_src_clr_err = 1;
            debug_ana_reg64_value[j] = ana_reg64;
            debug_ana_32k_tick_cur[j] = analog_read_reg32(0x60);
            debug_ana_32k_tick_set[j] = analog_read_reg32(0x65);
            /******************************************debug_pm_info**********************************************/
            debug_pm_info = 0x21;
#endif
        }
    }
    if(j == PM_IRQ_STATUS_MAX_CLR_TIMES)
    {
        return 0;
    }else{
        return 1;
    }
}

/**
 * @brief       this function serves to start sleep mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @return      none.
 */
_attribute_ram_code_sec_ static void  pm_sleep_start(pm_sleep_mode_e sleep_mode)
{
    pm_set_dig_module_power_switch(g_pm_suspend_power_cfg, PM_POWER_DOWN);

    flash_send_cmd(0, FLASH_WRITE_DEEP_CMD);
    reg_gpio_pi_ie = 0x00;      //MSPI ie disable
    reg_gpio_pj_ie = 0x00;

#if(PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x20;
#endif

    //The IRQ_PM_LVL interrupt is enabled to allow a program to continue execution after suspend wakes up.
    //If the wake source is deep or deep_retention, as long as all interrupt sources are turned off before sleep and plic_clr_all_request () is successfully executed,
    //it can enter the wfi state normally (if plic_clr_all_request () fails, Will wake up after entering wfi).
    //The pm_clr_all_irq_status () function needs to execute successfully. If the execution fails and IRQ_PM_LVL interrupt is enabled, plic_clr_all_request () will not succeed either.
    //Therefore, if either pm_clr_all_irq_status () or plic_clr_all_request () fails, wfi will be woken up and the logic will be changed to continue execution. Therefore, if the operation is deep, reboot will be performed
    //(add by bingyu.li, confirmed by jianzhi.chen 20230810)
    //The IRQ_PM_IRQ interrupt source cannot be used. For example, when the pad wake-up condition is always present,
    //the mcu cannot wake up from the stall because it is edge wake-up.The LDO is level awake and the LDO is powered on.
    //At this time, the mcu and ldo are out of sync.(add by weihua.zhang, confirmed by jianzhi.chen 20230907)
    plic_interrupt_enable(IRQ_PM_LVL);
    unsigned int clr_plic_request_result = 0;
    unsigned char clr_pm_irq_result = pm_clr_all_irq_status();
    if(clr_pm_irq_result==1){
        clr_plic_request_result = plic_clr_all_request();
    }

    //If the clearing fails, it indicates that the wake source is active. In this case, the deep mode will reboot,
    //and the other modes continue to run downward.(add by weihua.zhang, 20230907)
    if((clr_pm_irq_result == 0)|| (clr_plic_request_result == 0 ))
    {
#if(PM_DEBUG)
        while(1);
#endif
        if(sleep_mode == DEEPSLEEP_MODE)
        {
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0)|0x04);
            sys_reset_all();//Clear ana_0x64 for three times. If the status is not completely cleared for three times, reboot the mcu.
            while(1);
        }
    }
    else
    {
    //0x80 is to enter low power mode immediately. 0x81 is to wait for D25F to enter wfi mode before entering low power,this way is more secure.
    //Once in the WFI mode, memory transactions that are started before the execution of WFI are guaranteed to have been completed,
    //all transient states of memory handling are flushed and no new memory accesses will take place.
    //only suspend requires this process, after waking up to resume the scene.
    //(add by bingyu.li, confirmed by jianzhi.chen 20230810)
        write_reg8(0x14082f,0x81);  //stall mcu trig
        __asm__ __volatile__("wfi");
    }

    //This delay time is about 3.05us under the calibrated 24M RC clock.
    //After being triggered, the MCU needs to wait for a period of time before it actually goes to sleep,
    //during which time the MCU will continue to execute code. If the following code is executed
    //and some modules are awakened, the current will be larger than normal. About 20 empty instructions are fine,
    //but to be on the safe side, add 64 empty instructions.
    //The statement of the for loop may be optimized away by the compiler, resulting in a crash due to insufficient wait time,
    //so it cannot be used.(add by weihua.zhang, confirmed by sihui.wang 20230324)
    CLOCK_DLY_64_CYC;

    g_pm_status_info.wakeup_src = pm_get_wakeup_src();

    //Here we need to turn off the mask first, and then clear the plic. If the wake signal is always present and the interrupt mask is enabled,
    //the plic cannot be cleared. When exiting the sleep function, if the total interrupt is turned on, the interrupt handler function is entered.
    //If the interrupt handler is not defined, the program will run away.(changed by weihua.zhang, confirmed by jianzhi 20231101)
    plic_interrupt_disable(IRQ_PM_LVL);
    pm_clr_all_irq_status();  //clear all flag
    plic_clr_all_request();

    //Before sleeping,the MSPI has already been switched to 24M RC, so there is no need to wait for Xtal and PLL to stabilize.
    //Advance the flash wakeup to before the delay, because after the flash wakeup, It will take some time to restore to the active working state.
    //(usually set to 150us - a margin is left for different flash models)(add by bingyu.li, confirmed by kaixin.chen 20230616)
    //The flash two-wire system uses clk+cn+ two communication lines, and the flash four-wire system uses
    //clk+cn+ four communication lines. Before suspend sleep, the input of the six lines (PF0-PF5) used
    //by flash will be disabled. After suspend wakes up, the six lines will be set to input function.
    //(changed by weihua.zhang, confirmed by jianzhi 20201201)
    reg_gpio_pi_ie = 0xff;
    reg_gpio_pj_ie = 0x3f;          //MSPI(PI0-PJ5) ie enable
    flash_send_cmd(0, FLASH_WRITE_RELEASE_CMD);     //flash wakeup

    //wait for xtal stable and flash restore to the active working state.
    core_cclk_delay_tick(sys_clk.cclk_hclk * g_pm_suspend_delay_us);

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_xtal_ready(0x01);

    //When the crystal oscillator is stable, the PLL may also be unstable, so it is still necessary to wait for the PLL stable sign.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_bbpll_done();
}

/**
 * @brief       This function serves to switch external 32k pad to internal 32k rc.
 * @return      none.
 */
_attribute_ram_code_sec_ static inline void pm_switch_ext32kpad_to_int32krc(void)
{
    //switch 32k clk src: select internal 32k rc, if not do this, when deep+pad wakeup: there's no stable 32k clk(therefore, the pad wake-up time
    //is a bit long, the need for external 32k crystal vibration time) to count DCDC dly and XTAL dly. High temperatures even make it impossible
    //to vibrate, as the code for PWM excitation crystals has not yet been effectively executed. SO, we can switch 32k clk to the internal 32k rc.
    analog_write_reg8(0x4e, analog_read_reg8(0x4e) & (~BIT(7))); //32k select:[7]:0 sel 32k rc,1:32k pad
    analog_write_reg8(0x05, (analog_read_reg8(0x05) | 0x02) & 0xfe); //[0]:32k rc;[1]:32k xtal (1->power down,0->power up). Power up 32k rc.

    //deep + no tmr wakeup(we need  32k clk to count dcdc dly and xtal dly, but this case, ext 32k clk need close, here we use 32k rc instead)
    analog_write_reg8(areg_aon_0x2a, 0xea);
}

/**
 * @brief       This function serves to set the working mode of MCU based on 32k crystal,e.g. suspend mode, deep sleep mode, deep sleep with SRAM retention mode and shutdown mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @param[in]   wakeup_src          - wake up source select.
 * @param[in]   wakeup_tick_type    - tick type select. Use 32K tick count for long-time sleep or 24M tick count for short-time sleep.
 * @param[in]   wakeup_tick         - The tick value at the time of wake-up.
                                      If the wakeup_tick_type is PM_TICK_STIMER, then wakeup_tick is converted to 24M. The range of tick that can be set is approximately:
                                      current tick value + (18352~0xe0000000), and the corresponding sleep time is approximately: 2ms~234.88s.It cannot go to sleep normally when it exceeds this range.
                                      If the wakeup_tick_type is PM_TICK_32K, then wakeup_tick is converted to 32K. The range of tick that can be set is approximately:
                                      64~0xffffffff, and the corresponding sleep time is approximately: 2ms~37hours.It cannot go to sleep normally when it exceeds this range.
 * @note        There are two things to note when using LPC wake up:
 *              1.After the LPC is configured, you need to wait 100 seconds before you can go to sleep. After the LPC is opened, 1-2 32k tick is needed to calculate the result.
 *                Before this, the data in the result register is random. If you enter the sleep function at this time,
 *                you may not be able to sleep normally because the data in the result register is abnormal.
 *              2.When entering sleep, keep the input voltage and reference voltage difference must be greater than 30mV, otherwise can not enter sleep normally, crash occurs.
 * @return      indicate whether the cpu is wake up successful.
 */
_attribute_ram_code_sec_noinline_ int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick)
{
    /**
     * At present, the compensation value in the function is tested on the basis of the optimization level O2. If there are other optimization levels,
     * it needs to be retested and then see how the compensation time is handled.
     */
    ////////// disable IRQ //////////////////////////////////////////
    //If the time point of closing the total interrupt is later, the function may be interrupted by the interrupt,
    //cause the wake-up tick value to be calculated incorrectly, resulting in incorrect sleep time.
    //modify by weihua.zhang, confirmed by sihui.wang at 20220908.
    unsigned int r= core_interrupt_disable();

    ///////////////////       /////////////////////////////////
    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    if(timer_wakeup_enable && (wakeup_tick_type == PM_TICK_STIMER))
    {
        unsigned int span = (unsigned int)(wakeup_tick - stimer_get_tick ());
        if (span > 0xE0000000) //BIT(31)+BIT(30)+BIT(29)   7/8 cycle of 32bit, 178*7/8 = 156 S
        {
            core_restore_interrupt(r);
            return  pm_get_wakeup_src();
        }
        else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US)
        {
            unsigned int t = stimer_get_tick ();
            pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
            unsigned char st;
            do
            {
                st = pm_get_wakeup_src();
            }while ( ((unsigned int)stimer_get_tick () - t < span) && !st);

#if(PM_DEBUG)
            /******************************************debug_pm_info 1 **********************************************/
            debug_pm_info = 1;
#endif

            core_restore_interrupt(r);
            return st;
        }
    }

    //Turn off all interrupts immediately after entering the sleep function to prevent other interrupts, save the interrupt state before turning off, and restore it after waking up.
    //Turn on pm interrupt only before going to sleep,enable M-mode external interrupt first in this place.
    //modify by bingyu.li, confirmed by jianzhi.chen at 20230810.
    g_pm_interrupt_status = reg_irq_src0;
    g_pm_interrupt_status1 = reg_irq_src1;
    g_pm_interrupt_status2 = read_csr(NDS_MIE);
    reg_irq_src0 = 0;
    reg_irq_src1 = 0;
    core_mie_enable(FLD_MIE_MEIE);
    core_mie_disable(FLD_MIE_MSIE|FLD_MIE_MTIE);

    ///////////////////     get 32k calib      /////////////////////////////////
    if(CLK_32K_RC == g_clk_32k_src)
    {
        while(!read_reg32(0x140214));   //Wait for the 32k clock calibration to complete.

        g_pm_tick_32k_calib = read_reg32(0x140214);

#if(PM_DEBUG)
        analog_write_reg16(PM_ANA_REG_POWER_ON_CLR_BUF1, g_pm_tick_32k_calib);
        /******************************************debug_pm_info 2 **********************************************/
        debug_pm_info = 2;
#endif
    }
    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;

    ///////////////////    change clock    /////////////////////////////////
    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    mspi_stop_xip();

    clock_save_clock_config();
    clock_set_all_clock_to_default();

#if(PM_DEBUG)
    /******************************************debug_pm_info 3 **********************************************/
    debug_pm_info = 3;
#endif

    /////////////////// stop system timer /////////////////////////////////
#if SYS_TIMER_AUTO_MODE
    stimer_32k_tracking_disable();  //disable 32k track
    stimer_set_update_upon_nxt_32k_enable();        //system tick only update upon 32k posedge, must set before enable 32k read update!!!
    g_pm_tick_32k_cur = clock_get_32k_tick();
    g_pm_tick_cur = stimer_get_tick();
    stimer_set_update_upon_nxt_32k_disable();
    stimer_disable();   //disable system timer
#else
    #error -- Manual mode is only for internal testing, and 37 in the code may not be accurate
    stimer_32k_tracking_disable();  //disable 32k track
    g_pm_tick_cur = stimer_get_tick() + 37 * SYSTEM_TIMER_TICK_1US;  //cpu_get_32k_tick will cost 30~40 us
    stimer_disable();       //disable system timer
    g_pm_tick_32k_cur = clock_get_32k_tick ();
#endif

#if(PM_DEBUG)
    analog_write_reg32(PM_ANA_REG_WD_CLR_BUF1, g_pm_tick_32k_cur);
    /******************************************debug_pm_info 4 **********************************************/
    debug_pm_info = 4;
#endif

    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8(areg_aon_0x4b, wakeup_src);
    pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);               //clear all flag

    analog_write_reg8(0x020, 0xf0);
    analog_write_reg8(0x054, 0x42);

    unsigned int earlyWakeup_us;
    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)  //deep sleep with retention
    {
//      analog_write_reg8(0x78, 0x07);
        analog_write_reg8(0x7e, sleep_mode);        //sram retention
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(areg_aon_0x2b, 0x73);
        }
        else
        {
            analog_write_reg8(areg_aon_0x2b, 0x77);
        }
        analog_write_reg8(areg_aon_0x2c, 0xff);
        analog_write_reg8(areg_aon_0x2d, 0xbf);
        analog_write_reg8(areg_aon_0x2e, 0xf7);
        analog_write_reg8(areg_aon_0x06, analog_read_reg8(0x06) & (~FLD_DIG_RET_PD));

        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us;

#if(PM_DEBUG)
        /******************************************debug_pm_info 5 **********************************************/
        debug_pm_info = 5;
#endif

    }
    else if(sleep_mode == DEEPSLEEP_MODE)  //deepsleep no retention
    {
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(areg_aon_0x2b, 0xf3);
        }
        else
        {
            analog_write_reg8(areg_aon_0x2b, 0xf7);
        }
        analog_write_reg8(areg_aon_0x2c, 0xff);
        analog_write_reg8(areg_aon_0x2d, 0xff);
        analog_write_reg8(areg_aon_0x2e, 0xff);

        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;

#if(PM_DEBUG)
        /******************************************debug_pm_info 6 **********************************************/
        debug_pm_info = 6;
#endif

    }
    else  //suspend
    {
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(areg_aon_0x2b, 0xf3);
        }
        else
        {
            analog_write_reg8(areg_aon_0x2b, 0xf7);
        }
        analog_write_reg8(areg_aon_0x2c, 0xff);
        analog_write_reg8(areg_aon_0x2d, 0xbf);
        analog_write_reg8(areg_aon_0x2e, 0xf1);
        analog_write_reg8(areg_aon_0x08, analog_read_reg8(0x08) & ~(FLD_PD_LCLDO_DVDD1 | FLD_PD_LCLDO_DVDD2));

        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
    }

    /////////////////// auto power down /////////////////////////////////
    if(CLK_32K_RC == g_clk_32k_src)
    {
        if(((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_COMPARATOR))
        {
            analog_write_reg8(areg_aon_0x2a, 0xee);//disable auto power down 32KRC
        }
        else
        {
            analog_write_reg8(areg_aon_0x2a, 0xef);//enable auto power down 32KRC
        }

#if(PM_DEBUG)
        /******************************************debug_pm_info 7 **********************************************/
        debug_pm_info = 7;
#endif

    }
    else
    {
        if(sleep_mode == DEEPSLEEP_MODE && !timer_wakeup_enable) //if deep mode and no timer wakeup
        {
            pm_switch_ext32kpad_to_int32krc();
        }
        //suspend mode or deep retention mode or timer wake up source.
        //(we don't power down external 32k pad clock, even though three's no timer wakeup source in suspend or deep retention mode)
        else
        {
            analog_write_reg8(areg_aon_0x2a, 0xed);//if use timer wake up, auto pad 32k power down should be disabled
        }
    }

    if(sleep_mode == DEEPSLEEP_MODE)
    {
        analog_write_reg8(0x3d, g_pm_r_delay_cycle.deep_xtal_delay_cycle);
        analog_write_reg8(0x3e, g_pm_r_delay_cycle.deep_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }
    else
    {
        analog_write_reg8(0x3d, g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle);
        analog_write_reg8(0x3e, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 8 **********************************************/
    debug_pm_info = 8;
#endif

    //The variable pmcd.ref_tick is added, replacing the original variable g_pm_tick_cur. Because pmcd.ref_tick directly affects the value of
    //g_pm_long_suspend, g_pm_long_suspend can be assigned after pmcd.ref_tick is updated.changed by weihua,confirmed by biao.li.20201204.
    if(timer_wakeup_enable)
    {
        unsigned int tick_reset;
        unsigned int tick_wakeup_reset;
        if(wakeup_tick_type == PM_TICK_STIMER)
        {
            tick_wakeup_reset = (unsigned int)(wakeup_tick - (earlyWakeup_us * SYSTEM_TIMER_TICK_1US) - g_pm_tick_cur);
            if(CLK_32K_RC == g_clk_32k_src)
            {
                if(tick_wakeup_reset > 0x0fff0000)      // 24M: 11.18S
                {
                    tick_reset = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * g_track_32kcnt;
                    g_pm_long_suspend = 1;
                }
                else
                {
                    tick_reset = g_pm_tick_32k_cur + (tick_wakeup_reset * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
            }
            else
            {
                g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_64CYCLE;
                if(tick_wakeup_reset > 0x03ff0000)      // 24M: 2.79S
                {
                    tick_reset = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 64;
                    g_pm_long_suspend = 1;
                }
                else
                {
                    tick_reset = g_pm_tick_32k_cur + (tick_wakeup_reset * 64 + (CRYSTAL32768_TICK_PER_64CYCLE>>1)) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
            }
        }
        else
        {
            tick_reset = g_pm_tick_32k_cur + wakeup_tick - (earlyWakeup_us*4/125);  // 32k clk: /31.25
        }
        clock_set_32k_tick(tick_reset);

#if(PM_DEBUG)
        debug_ana_32k_tick = analog_read_reg32(0x65);
        if(tick_reset != debug_ana_32k_tick)
        {
            debug_ana_tick_reset = tick_reset;
            stimer_enable_in_manual_mode();
            stimer_32k_tracking_enable();   //enable 32k cal
//          gpio_set_high_level(GPIO_PE7);
            while(1);
        }
        /******************************************debug_pm_info 9 **********************************************/
        debug_pm_info = 9;
#endif

    }

    if(pm_get_wakeup_src()){

    }else{
        if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)
        {
            analog_write_reg8(0x7f, (0x40 | g_pm_pad_filter_en));
        }

        pm_sleep_start(sleep_mode);

        analog_write_reg8(0x7f, (0x41 | g_pm_pad_filter_en));

#if(PM_DEBUG)
        /******************************************debug_pm_info 10 **********************************************/
        debug_pm_info = 10;
#endif
    }

    if(sleep_mode == DEEPSLEEP_MODE){
        sys_reset_all();  //reboot
    }

#if SYS_TIMER_AUTO_MODE
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_START, 0);
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(CLK_32K_RC == g_clk_32k_src)
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    }
    else
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
#endif
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_DONE, g_pm_tick_cur + 1);
    stimer_32k_tracking_enable();           //enable 32k cal

#else
    #error -- only for internal testing.
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(CLK_32K_RC == g_clk_32k_src)
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    }
    else
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
#endif

    stimer_enable(STIMER_MANUAL_MODE, g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US);
    stimer_32k_tracking_enable();   //enable 32k cal
#endif

#if(PM_DEBUG)
    /******************************************debug_pm_info 12 **********************************************/
    debug_pm_info = 12;
#endif

    clock_restore_clock_config();

    mspi_set_xip_en();

#if(PM_DEBUG)
    /******************************************debug_pm_info 13 **********************************************/
    debug_pm_info = 13;
#endif

    if( (g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable )    //wakeup from timer only
    {
        if(wakeup_tick_type == PM_TICK_STIMER){
            while ((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30));
        }else{
            while ((unsigned int)(clock_get_32k_tick() - wakeup_tick - g_pm_tick_32k_cur + 1) > BIT(30));
        }
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 14 **********************************************/
    debug_pm_info = 14;
#endif

    //  DBG_CHN2_LOW;
    //Resume the interrupted state before sleep.Cannot be placed in the pm_sleep_start() interface to avoid failure to recover if this interface is not called.
    //changed by weihua, confirmed by jianzhi. 20231115
    reg_irq_src0 = g_pm_interrupt_status;
    reg_irq_src1 = g_pm_interrupt_status1;
    write_csr(NDS_MIE,g_pm_interrupt_status2);
    core_restore_interrupt(r);

    /**
     * Under normal circumstances, the wake up source cannot be zero. B85 had an exception that the wake up source was zero and never encountered it again.
     * STATUS_GPIO_ERR_NO_ENTER_PM indicates this case where the wake up source is zero. The name of this flag is not quite appropriate, but it has been used for a long time, so it is still used today.
     * added by bingyu.li, confirmed by sihui.wang at 20231018.
     */
    return ( g_pm_status_info.wakeup_src  ? (g_pm_status_info.wakeup_src | STATUS_ENTER_SUSPEND ) : STATUS_GPIO_ERR_NO_ENTER_PM );
}

_attribute_text_sec_ int pm_sleep_wakeup(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick)
{
    int status = 0;
    DISABLE_BTB;
    status = pm_sleep_wakeup_ram(sleep_mode, wakeup_src, wakeup_tick_type, wakeup_tick);
    ENABLE_BTB;
    return status;
}

/**
 * @brief       This function serves to set baseband/usb/npe power on/off before suspend sleep,If power
 *              on this module,the suspend current will increase;power down this module will save current,
 *              but you need to re-init this module after suspend wakeup.All module is power down default
 *              to save current.
 * @param[in]   value - whether to power on/off the baseband/usb/npe.
 * @param[in]   on_off - select power on or off, 0 - power off; other value - power on.
 * @return      none.
 */
void pm_set_suspend_power_cfg(pm_pd_module_e value, unsigned char on_off)
{
    if(0 == on_off){
        g_pm_suspend_power_cfg |= (value);
    }
    else{
        g_pm_suspend_power_cfg &= ~(value);
    }
}

/**
 * @brief       This function serves to switch digital module power.
 * @param[in]   module - digital module.
 * @param[in]   power_sel - power up or power down.
 * @return      none.
 */
void pm_set_dig_module_power_switch(pm_pd_module_e module, pm_power_sel_e power_sel)
{
    /*
    * After setting the power switch register of the digital module, it will take some time to take effect.
    * Check that the power switch is stable.
    * In short, the recommendation sequence of enable a module:
    * power up module -> wait power stable -> clock enable.
    * (added by jilong.liu, confirmed by junwen.jiang at 20240227)
    */
    if (power_sel == PM_POWER_UP) {
        analog_write_reg8(areg_aon_0x7d, (analog_read_reg8(areg_aon_0x7d) | FLD_PG_CLK_EN) & ~(module));
    } else if (power_sel == PM_POWER_DOWN) {
        analog_write_reg8(areg_aon_0x7d, (analog_read_reg8(areg_aon_0x7d) | FLD_PG_CLK_EN) | module);
    }

    /*
        Wait for power stable, for this chip(Onca), it will cost 13 * 1/24M times.
        The time required for different chips may vary so it is necessary to confirm with the chip colleagues when porting other chips.
    */
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk_hclk);//delay 1us

    while(analog_read_reg8(areg_aon_0x69) & FLD_PD_SM_BUSY);

    /*
        On the Onca platform, it was tested out that turning off power sequence clk (0x7d<7>) during sleep wake-up may cause issues.
        Colleagues in the chip department think that theoretically turning it off should not be a problem, 
        but turning it on will not have any impact on power consumption.
        Considering that previous chips were always power on this bit, so here will keep it power on too.
    */
}

#endif
