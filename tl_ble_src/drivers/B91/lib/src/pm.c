/********************************************************************************************************
 * @file    pm.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/pm.h"
#include "gpio.h"
#include "compiler.h"
#include "core.h"
#include "mspi.h"
#include "clock.h"
#include "flash.h"
#include "stimer.h"

#define PM_DEBUG            0
#define SUSPEND_CODE_US     389 //the code run time for preparation entering and exiting suspend sleep
                                //116-147(while waiting time). Because B91 fluctuates greatly, we need to reserve more here.
#define SLEEP_MIN_CODE_US   120 //the minimum code run time before sleep, >68 is enough.

#if(PM_DEBUG)
volatile unsigned char debug_pm_info;
volatile unsigned int ana_32k_tick;
#endif


_attribute_aligned_(4) pm_status_info_s g_pm_status_info;

//system timer clock source is constant 16M, never change
//NOTICE:We think that the external 32k crystal clk is very accurate, does not need to read through TIMER_32K_LAT_CAL.
//register, the conversion error(use 32k:16 cycle, count 16M sys tmr's ticks), at least the introduction of 64ppm.
#define CRYSTAL32768_TICK_PER_32CYCLE       15625  // 7812.5 * 2

_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_calib;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_cur;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_cur;
_attribute_data_retention_sec_  unsigned char           g_pm_long_suspend;
_attribute_data_retention_sec_  unsigned char           g_pm_vbat_v;
_attribute_data_retention_sec_  unsigned char           g_pm_tick_update_en=1;
_attribute_data_retention_sec_  unsigned int            g_pm_xtal_stable_suspend_nopnum = 300;
_attribute_data_retention_sec_  unsigned int            g_pm_xtal_stable_loopnum = 10;
_attribute_data_retention_sec_  unsigned int            g_pm_suspend_delay_us = 200;
_attribute_data_retention_sec_  static unsigned char    g_pm_suspend_power_cfg=0x87;
_attribute_data_retention_sec_  unsigned int            g_pm_mspi_cfg=0;
_attribute_data_retention_sec_  pm_clock_drift_t        pmcd = {0,0,-30,0,0,0,0,0,0,0,0};
_attribute_data_retention_sec_
volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us = {
    .suspend_early_wakeup_time_us = 188 + 109 + 200 + SUSPEND_CODE_US,  //188(r_delay) + 109(3.5*(1/32k)) + 200(XTAL_delay) + SUSPEND_CODE_US(code)
    .deep_ret_early_wakeup_time_us = 188 + 109,                         //188(r_delay) + 109(3.5*(1/32k))
    .deep_early_wakeup_time_us = 688 + 259,                             //688(r_delay) + 109(3.5*(1/32k)) + 150(boot_rom)
    .sleep_min_time_us = 688 + 259 + SLEEP_MIN_CODE_US + 200,           //(the maximum value of suspend and deep) + SLEEP_MIN_CODE_US(code run time before sleep) + 200. 200 means more margin, >32 is enough.
};
_attribute_data_retention_sec_
volatile pm_early_wakeup_time_us_s g_pm_longsleep_early_wakeup_time_us = {
    .suspend_early_wakeup_time_us = 23,
    .deep_ret_early_wakeup_time_us = 10 ,
    .deep_early_wakeup_time_us = 32 ,
    .sleep_min_time_us = 25,

};
_attribute_data_retention_sec_
volatile pm_r_delay_cycle_s g_pm_r_delay_cycle = {

    .deep_r_delay_cycle = 3 + 8,    // 11 * (1/16k) = 687.5
    .suspend_ret_r_delay_cycle = 3, // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
};

/**
 * @brief       This function configures pm wakeup time parameter.
 * @param[in]   param - pm wakeup time parameter.
 * @return      none.
 */
void pm_set_wakeup_time_param(pm_r_delay_cycle_s param)
{
    g_pm_r_delay_cycle.deep_r_delay_cycle = param.deep_r_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_r_delay_cycle = param.suspend_ret_r_delay_cycle;

    int deep_r_delay_us = g_pm_r_delay_cycle.deep_r_delay_cycle *1000 /16;
    int suspend_ret_r_delay_us = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle *1000 /16;
    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us = suspend_ret_r_delay_us + 109 + 200 + SUSPEND_CODE_US;
    g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us = suspend_ret_r_delay_us + 109;
    g_pm_early_wakeup_time_us.deep_early_wakeup_time_us = deep_r_delay_us + 259;
    if(g_pm_early_wakeup_time_us.deep_early_wakeup_time_us < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us)
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    }
    else
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    }
}

/**
 * @brief       This function is used in applications where the crystal oscillator is relatively slow to start.
 *              When the start-up time is very slow, you can call this function to avoid restarting caused
 *              by insufficient crystal oscillator time (it is recommended to leave a certain margin when setting).
 * @param[in]   delay_us - This time setting is related to the parameter nopnum, which is about the execution time of the for loop
 *                          in the ramcode(default value: 200).
 * @param[in]   loopnum - The time for the crystal oscillator to stabilize is approximately: loopnum*40us(default value: loopnum=10).
 * @param[in]   nopnum - The number of for loops used to wait for the crystal oscillator to stabilize after suspend wakes up.
 *                       for(i = 0; i < nopnum; i++){ _asm_("tnop"); }(default value: Flash=250).
 * @return      none.
 */
void pm_set_xtal_stable_timer_param(unsigned int delay_us, unsigned int loopnum, unsigned int nopnum)
{
    g_pm_xtal_stable_suspend_nopnum = nopnum;
    g_pm_xtal_stable_loopnum = loopnum;
    g_pm_suspend_delay_us = delay_us;

    int suspend_ret_r_delay_us = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle *1000 /16;
    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us = suspend_ret_r_delay_us + 109 + delay_us + SUSPEND_CODE_US;
    if(g_pm_early_wakeup_time_us.deep_early_wakeup_time_us < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us)
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    }
    else
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    }
}

/**
 * @brief       This function serves to recover system timer.
 *              The code is placed in the ram code section, in order to shorten the time.
 * @return      none.
 */
void pm_stimer_recover(void)
{
#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x02; //sys tick 16M set upon next 32k posedge
    reg_system_ctrl     |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN) ;
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(g_pm_long_suspend){
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k + 1 - pmcd.ref_tick_32k) / 32 * g_pm_tick_32k_calib;
    }else{
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k + 1 - pmcd.ref_tick_32k) * g_pm_tick_32k_calib / 32;        // current clock
    }
    pmcd.rc32_wakeup = now_tick_32k + 1;
    pmcd.rc32 = now_tick_32k + 1 - pmcd.ref_tick_32k;
    reg_system_tick = g_pm_tick_cur + 1;
    //wait cmd set dly done upon next 32k posedge
    //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
    while((reg_system_st & BIT(7)) == 0);   //system timer set done status upon next 32k posedge
    REG_ADDR8(0x140218) = 0;//normal sys tick (16/sys) update
#else
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(g_pm_long_suspend){
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k - pmcd.ref_tick_32k) / 32 * g_pm_tick_32k_calib;
    }else{
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k - pmcd.ref_tick_32k) * g_pm_tick_32k_calib / 32;        // current clock
    }
    pmcd.rc32_wakeup = now_tick_32k;
    pmcd.rc32 = now_tick_32k - pmcd.ref_tick_32k;
    reg_system_tick = g_pm_tick_cur;
    reg_system_ctrl |= FLD_SYSTEM_32K_TRACK_EN | FLD_SYSTEM_TIMER_EN;
#endif
}

/**
 * @brief       This function configures a GPIO pin as the wakeup pin.
 * @param[in]   pin - the pins can be set to all GPIO except PE5 and GPIOF groups.
 * @param[in]   pol - the wakeup polarity of the pad pin(0: low-level wakeup, 1: high-level wakeup).
 * @param[in]   en  - enable or disable the wakeup function for the pan pin(1: enable, 0: disable).
 * @return      none.
 */
void pm_set_gpio_wakeup (gpio_pin_e pin, pm_gpio_wakeup_level_e pol, int en)
{
    ///////////////////////////////////////////////////////////
    //        PA[7:0]           PB[7:0]         PC[7:0]         PD[7:0]     PE[7:0]
    // en:  ana_0x41<7:0>    ana_0x42<7:0>  ana_0x43<7:0>  ana_0x44<7:0>  ana_0x45<7:0>
    // pol: ana_0x46<7:0>    ana_0x47<7:0>  ana_0x48<7:0>  ana_0x49<7:0>  ana_0x4a<7:0>
    unsigned char mask = pin & 0xff;
    unsigned char analog_reg;
    unsigned char val;

    ////////////////////////// polarity ////////////////////////
    analog_reg = ((pin>>8) + 0x41);
    val = analog_read_reg8(analog_reg);
    if (pol) {
        val &= ~mask;
    }
    else {
        val |= mask;
    }
    analog_write_reg8 (analog_reg, val);

    /////////////////////////// enable /////////////////////
    analog_reg = ((pin>>8) + 0x46);
    val = analog_read_reg8(analog_reg);
    if (en) {
        val |= mask;
    }
    else {
        val &= ~mask;
    }
    analog_write_reg8 (analog_reg, val);
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
_attribute_ram_code_sec_optimize_o2_ void pm_wait_xtal_ready(unsigned char all_ramcode_en)
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
        //Note: Ensure that the current clock is 24M RC, otherwise the nop running time will not be 40us, resulting in a later calculation error and system reboot.
        //(add by weihua.zhang, confirmed by peng.sun 20230609)
        core_cclk_delay_tick(sys_clk.cclk * 40);
        if(stimer_get_tick() - t0 > SYSTEM_TIMER_TICK_1US * 20)
        {
            break;
        }
    }

    if(j == g_pm_xtal_stable_loopnum)
    {
        //Use PM_ANA_REG_POWER_ON_CLR_BUF0[bit2] to check whether there has been a reset caused by the instability of the crystal oscillator.
        //If it is 1, it has occurred.
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | 0x04);
#if(PM_DEBUG)
        while(1);
#endif
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
 * @brief       this function servers to wait bbpll clock lock.
 * @return      none.
 */
void pm_wait_bbpll_done(void)
{
    unsigned char ana_81 = analog_read_reg8(0x81);
    analog_write_reg8(0x81, ana_81 | BIT(6));
    for(unsigned char j = 0; j < 3; j++)
    {
        core_cclk_delay_tick(sys_clk.cclk * 20);//20us
        if(BIT(5) == (analog_read_reg8(0x88) & BIT(5)))
        {
            analog_write_reg8(0x81, ana_81 & 0xbf);
            break;
        }
        else
        {
            if(j == 0){
                analog_write_reg8(0x01, 0x46);
            }
            else if(j == 1){
                analog_write_reg8(0x01, 0x43);
            }
            else{
                analog_write_reg8(0x81, ana_81 & 0xbf);
            }
        }
    }
}

/**
 * @brief       This function serves to update wakeup status.
 * @return      none.
 */
void pm_update_status_info(void)
{
    unsigned char analog_7f = analog_read_reg8(0x7f);
    unsigned char analog_38 = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);
    unsigned char analog_39 = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0);
    if(analog_38 & BIT(0)){
        if(analog_39 & BIT(0)){
            g_pm_status_info.mcu_status = MCU_STATUS_REBOOT_BACK;
            analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, analog_38 & 0xfe);
        }else{
            g_pm_status_info.mcu_status = MCU_STATUS_POWER_ON;
            analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0,analog_38 & 0xfe);
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_39 | BIT(0));
        }
    }else{
        if(!(analog_7f & 0x01)){
            g_pm_status_info.mcu_status = MCU_STATUS_DEEPRET_BACK;
        }else if(analog_39 & BIT(1)){
            g_pm_status_info.mcu_status = MCU_STATUS_REBOOT_DEEP_BACK;
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_39 & 0xfd);
        }else{
            g_pm_status_info.mcu_status = MCU_STATUS_DEEP_BACK;
        }
    }
    analog_write_reg8(0x7f, analog_7f | 0x01);
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.is_pad_wakeup = (g_pm_status_info.wakeup_src & BIT(3)) >> 3;
}

/**
 * @brief       this function serves to start sleep mode.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void  pm_sleep_start(void)
{

    //A0 chip can't disable baseband,A1 need disable baseband.See sys_init for specific instructions.(add by weihua.zhang, confirmed by junwen 20200925)
    if(0xff == g_chip_version){ //A0
        analog_write_reg8(0x7d, g_pm_suspend_power_cfg&0xfe);
    }else{
        analog_write_reg8(0x7d, g_pm_suspend_power_cfg);
    }
    mspi_stop_xip();
    mspi_fm_write_en();         /* write mode */
    mspi_low();
    mspi_write(0xb9);   //flash deep
    mspi_wait();
    mspi_high();
    write_reg8(0x140329, 0x00); //MSPI ie disable

    //This is 1.4V and 1.8V power supply during sleep. Do not power on during initialization, because after power on,
    //there will be two power supplies at the same time, which may cause abnormalities.add by weihua.zhang, confirmed by haitao 20210107
    analog_write_reg8(0x0b, analog_read_reg8(0x0b) & ~(BIT(0) | BIT(1)));   //<0>:pd_nvt_1p4,   power on native 1P4.
                                                                            //<1>:pd_nvt_1p8,   power on native 1P8.
    analog_write_reg8(0x81, analog_read_reg8(0x81) | BIT(7));

#if(PM_DEBUG)
    /******************************************debug_info**********************************************/
    debug_pm_info = 20;
#endif

    write_reg8(0x1401ef,0x80);  //trig pwdn

    //After being triggered, the MCU needs to wait for a period of time before it actually goes to sleep,
    //during which time the MCU will continue to execute code. If the following code is executed
    //and some modules are awakened, the current will be larger than normal. About 20 empty instructions are fine,
    //but to be on the safe side, add 64 empty instructions.
    for(volatile unsigned int i = 0; i < 64; i++){
        __asm__("nop");
    }

    analog_write_reg8(0x81, analog_read_reg8(0x81) & (~BIT(7)));

    analog_write_reg8(0x0b, analog_read_reg8(0x0b) | (BIT(0) | BIT(1)));    //<0>:pd_nvt_1p4,   power down native 1P4.
                                                                            //<1>:pd_nvt_1p8,   power down native 1P8.

    //must to set xo_quick_settle with manual and wait it stable.(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

    analog_write_reg8(0x7d, 0x80); // enable digital bb, usb, npe

    //Before sleeping,the MSPI has already been switched to 24M RC, so there is no need to wait for Xtal and PLL to stabilize.
    //Advance the flash wakeup to before the delay, because after the flash wakeup, It will take some time to restore to the active working state.
    //(usually set to 150us - a margin is left for different flash models)(add by bingyu.li, confirmed by kaixin.chen 20230728)
    //The flash two-wire system uses clk+cn+ two communication lines, and the flash four-wire system uses
    //clk+cn+ four communication lines. Before suspend sleep, the input of the six lines (PF0-PF5) used
    //by flash will be disabled. After suspend wakes up, the six lines will be set to input function.
    //(changed by weihua.zhang, confirmed by jianzhi 20201201)
    write_reg8(0x140329, 0x3f); //MSPI(PF0-PF5) ie enable
    mspi_low();
    mspi_write(0xab);   //flash wakeup
    mspi_wait();
    mspi_high();

    //When g_pm_xtal_stable_suspend_nopnum =300,This for loop is about 200us under the calibrated 24M RC clock.
    //wait for xtal stable and flash restore to the active working state.
    for(volatile unsigned int i = 0; i < g_pm_xtal_stable_suspend_nopnum; i++){
        __asm__("nop");
    }

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by jilong.liu, confirmed by wenfeng.lou 20240320)
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
    analog_write_reg8(0x05, 0x02); //[0]:32k rc;[1]:32k xtal (1->power down,0->power up). Power up 32k rc.

    //deep + no tmr wakeup(we need  32k clk to count dcdc dly and xtal dly, but this case, ext 32k clk need close, here we use 32k rc instead)
    analog_write_reg8(0x4c, 0xef);
}

/**
 * @brief       When 32k rc sleeps, the calibration function is initialized.
 * @return      none.
 */
void pm_32k_rc_offset_init(void)
{
    if(g_pm_vbat_v){
        pmcd.offset = 0;
    }else{
        pmcd.offset = -30;
    }
    pmcd.tc = 0;
    pmcd.ref_tick = 0;
    g_pm_tick_update_en = 0;
}

/**
 * @brief       Update the reference 32k tick value and 16M system clock value when needed.
 * @param[in]   tick_32k    - the reference 32k tick value.
 * @param[in]   tick        - the reference 16M system clock value.
 * @return      none.
 */
void pm_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick)
{
    pmcd.rc32_rt = tick_32k - pmcd.rc32_wakeup;
    if (pmcd.calib || !pmcd.ref_tick || g_pm_tick_update_en || ((tick_32k - pmcd.ref_tick_32k) & 0xfffffff) > 32 * 5000)
    {
        pmcd.calib = 0;
        pmcd.ref_tick_32k = tick_32k;
        pmcd.ref_tick = tick | 1;
    }
}

/**
 * @brief       Calculate the offset value based on the difference of 16M tick.
 * @param[in]   offset_tick - the 16M tick difference between the standard clock and the expected clock.
 * @return      none.
 */
void pm_cal_32k_rc_offset (int offset_tick)
{
    pmcd.offset_cur = offset_tick;
    int offset = offset_tick * (240 * 31) / pmcd.rc32;      //240ms / sleep_period
    if (offset > 0x100)
    {
        offset = 0x100;
    }
    else if (offset < -0x100)
    {
        offset = -0x100;
    }
    pmcd.calib = 1;
    pmcd.offset += (offset - pmcd.offset) >> 4;
    pmcd.offset_dc += (offset_tick - pmcd.offset_dc) >> 3;
    g_pm_tick_update_en = 0;
}

/**
 * @brief       32k rc calibration clock compensation.
 * @return      32k calibration value after compensation.
 */
unsigned int pm_get_32k_rc_calib (void)
{
    while(!read_reg32(0x140214));   //Wait for the 32k clock calibration to complete.

    int tc = read_reg32(0x140214);
    pmcd.s0 = tc;
    tc = tc << 4;
    if (!pmcd.tc)
    {
        pmcd.tc = tc;
    }
    else
    {
        pmcd.tc += (tc - pmcd.tc) >> (4 - pmcd.calib);
    }

    int offset = (pmcd.offset * (pmcd.tc >> 4)) >> 18;      //offset : tick per 256ms
    offset = (pmcd.tc >> 4) + offset;
    return (unsigned int)offset;
}

/**
 * @brief       This function serves to set the working mode of MCU based on 32k crystal,e.g. suspend mode, deep sleep mode, deep sleep with SRAM retention mode and shutdown mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @param[in]   wakeup_src          - wake up source select.
 * @param[in]   wakeup_tick_type    - tick type select. Use 32K tick count for long-term sleep and 16M tick count for short-term sleep.
 * @param[in]   wakeup_tick         - The tick value at the time of wake-up.
                                      If the wakeup_tick_type is PM_TICK_STIMER_16M, then wakeup_tick is converted to 16M. The range of tick that can be set is approximately:
                                      current tick value + (18352~0xe0000000), and the corresponding sleep time is approximately: 2ms~234.88s.It cannot go to sleep normally when it exceeds this range.
                                      If the wakeup_tick_type is PM_TICK_32K, then wakeup_tick is converted to 32K. The range of tick that can be set is approximately:
                                      64~0xffffffff, and the corresponding sleep time is approximately: 2ms~37hours.It cannot go to sleep normally when it exceeds this range.
 * @return      indicate whether the cpu is wake up successful.
 */
_attribute_ram_code_sec_noinline_ int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick)
{
    ////////// disable IRQ //////////////////////////////////////////
    //If the time point of closing the total interrupt is later, the function may be interrupted by the interrupt,
    //cause the wake-up tick value to be calculated incorrectly, resulting in incorrect sleep time.
    //modify by weihua.zhang, confirmed by sihui.wang at 20220908.
    unsigned int r= core_interrupt_disable();

    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    if(CLK_32K_RC == g_clk_32k_src){
        g_pm_tick_32k_calib = pm_get_32k_rc_calib()*2;

#if(PM_DEBUG)
        /******************************************debug_pm_info 1 **********************************************/
        debug_pm_info = 1;
        analog_write_reg16(0x3a, g_pm_tick_32k_calib>>1);
#endif
    }else{
        //NOTICE:We think that the external 32k crystal clk is very accurate
        g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_32CYCLE;

    }
    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;
    unsigned int span = (unsigned int)(wakeup_tick - ((wakeup_tick_type == PM_TICK_STIMER_16M)?stimer_get_tick ():0));
    if(timer_wakeup_enable && (wakeup_tick_type == PM_TICK_STIMER_16M)){
        if (span > 0xE0000000){ //BIT(31)+BIT(30)+BIT(29)   7/8 cycle of 32bit, 268.44*7/8 = 234.88 S
            core_restore_interrupt(r);
            return  analog_read_reg8 (0x64) & 0x1f;
        }
        else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US){ // 0 us base
            unsigned int t = stimer_get_tick ();
            analog_write_reg8 (0x64, 0x1f);
            unsigned char st;
            do {
                st = analog_read_reg8 (0x64) & 0x1f;
            } while ( ((unsigned int)stimer_get_tick () - t < span) && !st);

#if(PM_DEBUG)
            /******************************************debug_pm_info 2 **********************************************/
            debug_pm_info = 2;
#endif
            core_restore_interrupt(r);
            return st;
        }
    }

    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    unsigned char mspiclk_cclk_reg = read_reg8(0x1401e8);
    write_reg8(0x1401e8, mspiclk_cclk_reg & 0x0f);  //change mspiclk & cclk to 24M rc clock
    unsigned char div_reg = read_reg8(0x1401d8);
    write_reg8(0x1401d8, div_reg & 0xf8);           //change clock division

#if(PM_DEBUG)
    /******************************************debug_pm_info 3 **********************************************/
    debug_pm_info = 3;
#endif

#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x01;     //system tick only update upon 32k posedge, must set before enable 32k read update!!!
    BM_CLR(reg_system_ctrl,FLD_SYSTEM_32K_TRACK_EN);//disable 32k track
    g_pm_tick_32k_cur = clock_get_32k_tick();
    g_pm_tick_cur = stimer_get_tick();
    BM_SET(reg_system_st,FLD_SYSTEM_CMD_STOP);  //write 1, stop system timer when using auto mode
    REG_ADDR8(0x140218) = 0x00;
#else
    g_pm_tick_cur = stimer_get_tick() + 37 * SYSTEM_TIMER_TICK_1US;  //cpu_get_32k_tick will cost 30~40 us//15
    BM_CLR(reg_system_ctrl,FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);//disable 32k track and stimer
    g_pm_tick_32k_cur = clock_get_32k_tick ();
#endif

    pm_update_32k_rc_sleep_tick (g_pm_tick_32k_cur, g_pm_tick_cur);

#if(PM_DEBUG)
    /******************************************debug_pm_info 4 **********************************************/
    debug_pm_info = 4;
#endif

    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8(0x4b, wakeup_src);
    analog_write_reg8(0x64, 0x1f);              //clear all flag

    analog_write_reg8(0x7e, sleep_mode);//sram retention
    unsigned int earlyWakeup_us;

    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG) { //deep sleep with retention
        //0x00->0xd1
        //<0>pd_rc32k_auto=1 <4>power down power suspend ldo=1
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
        //0x140104 mspi_set_l: multiboot address offset option, 0:0k;  1:128k;  2:256k;  4:256k
        //0x140105 mspi_set_h: program space size = mspi_set_h*4k
        //0x140106 mspi_cmd_AHB: xip read command
        //0x140107 mspi_fm_AHB: [3:0] dummy_h  [5:4] dat_line_h  [6] addr_line_h  [7] cmd_line_h
        g_pm_mspi_cfg = read_reg32(0x140104);

        if(wakeup_tick_type == PM_TICK_STIMER_16M){
            earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us;
        }else{
            earlyWakeup_us = g_pm_longsleep_early_wakeup_time_us.deep_ret_early_wakeup_time_us;
        }

#if(PM_DEBUG)
        /******************************************debug_pm_info 5 **********************************************/
        debug_pm_info = 5;
#endif
    }
    else if(sleep_mode == DEEPSLEEP_MODE){  //deep sleep no retention
        //0x00->0xf9
        //<0>pd_rc32k_auto=1 <3>rst_xtal_quickstart_cnt=1 <4>power down power suspend ldo=1
        //<5>power down power retention ldo=1 <6>power down sequence enable=1 <7>enable isolation=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0xf8);//deep
        }
        else
        {
            analog_write_reg8(0x4d,0xf9);//deep
        }
        analog_write_reg8(0x7f, 0x01);
        if(wakeup_tick_type == PM_TICK_STIMER_16M){
            earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;
        }else{
            earlyWakeup_us = g_pm_longsleep_early_wakeup_time_us.deep_early_wakeup_time_us;
        }

#if(PM_DEBUG)
        /******************************************debug_pm_info 6 **********************************************/
        debug_pm_info = 6;
#endif
    }
    else{  //suspend
        //0x00->0x61
        //<0>pd_rc32k_auto=1 <5>power down power retention ldo=1 <6>power down sequence enable=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0x60);//suspend
        }
        else
        {
            analog_write_reg8(0x4d,0x61);//suspend
        }
        analog_write_reg8(0x7f, 0x01);

        if(wakeup_tick_type == PM_TICK_STIMER_16M){
            earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
        }else{
            earlyWakeup_us = g_pm_longsleep_early_wakeup_time_us.suspend_early_wakeup_time_us;
        }
    }
    unsigned int tick_wakeup_reset = wakeup_tick - ((wakeup_tick_type == PM_TICK_STIMER_16M)?(earlyWakeup_us * SYSTEM_TIMER_TICK_1US):earlyWakeup_us);
    if(CLK_32K_RC == g_clk_32k_src){
        //auto power down
        if((wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_MDEC) || (wakeup_src & PM_WAKEUP_COMPARATOR)){
            analog_write_reg8(0x4c,0xee);//disable auto power down 32KRC
        }
        else{
            analog_write_reg8(0x4c, 0xef);//en auto power down 32KRC
        }

#if(PM_DEBUG)
        /******************************************debug_pm_info 7 **********************************************/
        debug_pm_info = 7;
#endif
    }else{

        if(sleep_mode == DEEPSLEEP_MODE && !timer_wakeup_enable){ //if deep mode and no tmr wakeup
            pm_switch_ext32kpad_to_int32krc();
        }
        else{ //suspend mode or deep with retention mode or tmr wakeup (we don't power down ext 32k pad clk,even though three's no tmr wakeup src in suspend or deep with retention mode)
            analog_write_reg8(0x4c, 0xed);//if use tmr wakeup, auto pad 32k power down should be disabled
        }
    }

#if 1
    if(sleep_mode == DEEPSLEEP_MODE){
        analog_write_reg8 (0x40, g_pm_r_delay_cycle.deep_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }else{
        analog_write_reg8 (0x40, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 8 **********************************************/
    debug_pm_info = 8;
#endif

#else
    span = (PM_DCDC_DELAY_DURATION * (SYSTEM_TIMER_TICK_1US>>1) * 32 + tick_32k_halfCalib)/ g_pm_tick_32k_calib;
    unsigned char rst_cycle = 0xff - span;
    analog_write (0x1f, rst_cycle);
#endif
    unsigned int tick_reset;
    //The variable pmcd.ref_tick is added, replacing the original variable g_pm_tick_cur. Because pmcd.ref_tick directly affects the value of
    //g_pm_long_suspend, g_pm_long_suspend can be assigned after pmcd.ref_tick is updated.changed by weihua,confirmed by biao.li.20201204.
    if(timer_wakeup_enable && (wakeup_tick_type == PM_TICK_STIMER_16M)){
        if( (unsigned int)(tick_wakeup_reset - pmcd.ref_tick) > 0x07ff0000 ){ //CLK_32K_RC:BIT(28) = 0x10000000 16M:16S;CLK_32K_XTAL:BIT(28) = 0x08000000   16M:8.389S
            g_pm_long_suspend = 1;
        }
        else{
            g_pm_long_suspend = 0;
        }
    }
    if(wakeup_tick_type == PM_TICK_STIMER_16M){
        if(g_pm_long_suspend){
            tick_reset = pmcd.ref_tick_32k + (unsigned int)(tick_wakeup_reset - pmcd.ref_tick)/ g_pm_tick_32k_calib * 32;
        }
        else{
            tick_reset = pmcd.ref_tick_32k + ((unsigned int)(tick_wakeup_reset - pmcd.ref_tick) * 32 + tick_32k_halfCalib) / g_pm_tick_32k_calib;
        }
    }else{
        tick_reset = g_pm_tick_32k_cur + tick_wakeup_reset;
    }
    clock_set_32k_tick(tick_reset);

#if(PM_DEBUG)
    analog_write_reg32(0x3c, g_pm_tick_32k_cur);

    ana_32k_tick = analog_read_reg32(0x65);
    if(tick_reset != ana_32k_tick)
    {
        reg_system_ctrl |= FLD_SYSTEM_32K_TRACK_EN | FLD_SYSTEM_TIMER_EN ;
        flash_write_page(0x10000, 4, (unsigned char *)&ana_32k_tick);
        gpio_set_high_level(GPIO_PB7);
        while(1);
    }
    /******************************************debug_pm_info 9 **********************************************/
    debug_pm_info = 9;
#endif

    if(pm_get_wakeup_src()&0x1f){

    }
    else{
#if (WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
        if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)
            analog_write_reg8(0x7f, 0x00);
#endif
        pm_sleep_start();

#if (WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
        analog_write_reg8(0x7f, 0x01);
#endif

#if(PM_DEBUG)
        /******************************************debug_pm_info 10 **********************************************/
        debug_pm_info = 10;
#endif
    }
    if(sleep_mode == DEEPSLEEP_MODE){
       write_reg8 (0x1401ef, 0x20);  //reboot
    }
#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x02; //sys tick 16M set upon next 32k posedge
    reg_system_ctrl     |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN) ;
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(g_pm_long_suspend){
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k + 1 - pmcd.ref_tick_32k) / 32 * g_pm_tick_32k_calib;
    }else{
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k + 1 - pmcd.ref_tick_32k) * g_pm_tick_32k_calib / 32;        // current clock
    }
    pmcd.rc32_wakeup = now_tick_32k + 1;
    pmcd.rc32 = now_tick_32k + 1 - pmcd.ref_tick_32k;
    reg_system_tick = g_pm_tick_cur + 1;

#if(PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
#endif
    //But when cclk is set to 96M, pclk is the quarter frequency of cclk to 24M, but cclk will switch to 24M RC before the program enters sleep,
    //pclk is still the quarter frequency of cclk, so pclk becomes 6M, reading 32k The tick value slows down by about four times;
    //writing a 16M tick value and reading a 32k rising edge are likely to occur at the same time, causing the 32k rising edge flag to fail to be read.
    //modify by junhui.hu, confirmed by jianzhi at 20210923.
    //wait cmd set dly done upon next 32k posedge
    //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
    while((reg_system_st & BIT(7)) == 0);   //system timer set done status upon next 32k posedge
    REG_ADDR8(0x140218) = 0;                //normal sys tick (16/sys) update
#else
    #error -- Manual mode is only for internal testing, and 37 in the code may not be accurate
    unsigned int now_tick_32k = clock_get_32k_tick();
    if(g_pm_long_suspend){
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k - pmcd.ref_tick_32k) / 32 * g_pm_tick_32k_calib;
    }else{
        g_pm_tick_cur = pmcd.ref_tick + (unsigned int)(now_tick_32k - pmcd.ref_tick_32k) * g_pm_tick_32k_calib / 32;        // current clock
    }
    pmcd.rc32_wakeup = now_tick_32k;
    pmcd.rc32 = now_tick_32k - pmcd.ref_tick_32k;

#if(PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
#endif

    reg_system_tick = g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US;
    reg_system_ctrl |= FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN;    //enable 32k cal and stimer
#endif

#if(PM_DEBUG)
    /******************************************debug_pm_info 12 **********************************************/
    debug_pm_info = 12;
#endif

    write_reg8(0x1401d8, div_reg);
    write_reg8(0x1401e8, mspiclk_cclk_reg );//restore cclk and mspi clk

#if(PM_DEBUG)
    /******************************************debug_pm_info 13 **********************************************/
    debug_pm_info = 13;
#endif

    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    //  DBG_CHN2_HIGH;
    if( (g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable )    //wakeup from timer only
    {
        if(wakeup_tick_type == PM_TICK_STIMER_16M){
            while ((unsigned int)(stimer_get_tick () -  wakeup_tick) > BIT(30));
        }else{
            while ((unsigned int)(clock_get_32k_tick() -  wakeup_tick - g_pm_tick_32k_cur + 1) > BIT(30));
        }
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 14 **********************************************/
    debug_pm_info = 14;
#endif

    //  DBG_CHN2_LOW;
    core_restore_interrupt(r);
    /**
     * Under normal circumstances, the wake up source cannot be zero. B85 had an exception that the wake up source was zero and never encountered it again.
     * STATUS_GPIO_ERR_NO_ENTER_PM indicates this case where the wake up source is zero. The name of this flag is not quite appropriate, but it has been used for a long time, so it is still used today.
     * added by bingyu.li, confirmed by sihui.wang at 20231018.
     */
    return (g_pm_status_info.wakeup_src ? (g_pm_status_info.wakeup_src | STATUS_ENTER_SUSPEND) : STATUS_GPIO_ERR_NO_ENTER_PM );

}

/**
 * @brief       This function serves to set the working mode of MCU based on 32k crystal,e.g. suspend mode, deepsleep mode, deepsleep with SRAM retention mode and shutdown mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @param[in]   wakeup_src          - wake up source select.
 * @param[in]   wakeup_tick_type    - tick type select. Use 32K tick count for long-term sleep and 16M tick count for short-term sleep.
 * @param[in]   wakeup_tick         - The tick value at the time of wake-up.
                                      If the wakeup_tick_type is PM_TICK_STIMER_16M, then wakeup_tick is converted to 16M. The range of tick that can be set is approximately:
                                      current tick value + (18352~0xe0000000), and the corresponding sleep time is approximately: 2ms~234.88s.It cannot go to sleep normally when it exceeds this range.
                                      If the wakeup_tick_type is PM_TICK_32K, then wakeup_tick is converted to 32K. The range of tick that can be set is approximately:
                                      64~0xffffffff, and the corresponding sleep time is approximately: 2ms~37hours.It cannot go to sleep normally when it exceeds this range.
 * @return      indicate whether the cpu is wake up successful.
 */
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
 * @param[in]   value - weather to power on/off the baseband/usb/npe.
 * @param[in]   on_off - select power on or off, 0 - power off; other value - power on.
 * @return      none.
 */
void pm_set_suspend_power_cfg(pm_suspend_power_cfg_e value, unsigned char on_off)
{
    if(0 == on_off){
        g_pm_suspend_power_cfg |= (value);
    }
    else{
        g_pm_suspend_power_cfg &= ~(value);
    }
}

