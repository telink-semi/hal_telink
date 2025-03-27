/********************************************************************************************************
 * @file    pm_internal.c
 *
 * @brief   This is the source file for TL721X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/core.h"
#include "lib/include/mspi.h"
#include "lib/include/clock.h"
#include "flash.h"
#include "lib/include/stimer.h"
#include "audio.h"
#include "watchdog.h"


#define PM_IRQ_STATUS_MAX_CLR_TIMES     3


extern unsigned char pll_vco_itrim;
extern volatile unsigned char g_pm_system_reboot_event;

_attribute_data_retention_sec_ unsigned int g_pm_xtal_stable_loopnum = 10;
_attribute_data_retention_sec_ pm_cal_vdd0p94_t g_pm_cal_vdd0p94_info = {TRIM_VDD0P94_TO_0P963V, TRIM_VDD0P94_TO_0P963V, TRIM_VDD0P94_TO_1P078V, TRIM_VDD0P94_TO_1P078V};
_attribute_data_retention_sec_ unsigned char g_pm_cal_vddo1p8_info = TRIM_VDDO1P8_TO_1P832V;
_attribute_data_retention_sec_ pm_cal_0p94v_e g_pm_vdd0p94_level = CAL_0P94V_TO_0P95V;

#if(PM_DEBUG)
volatile unsigned char debug_ana_reg64_value[3];
volatile unsigned int debug_ana_32k_tick_cur[3];
volatile unsigned int debug_ana_32k_tick_set[3];
volatile unsigned char debug_wakeup_src_clr_err=0;
volatile unsigned char debug_xtal_num=0;
#endif

/**
 * @brief       this function servers to power up BBPLL.
 *              [DRIV-1966]The power consumption of PLL is 260uA in LDO mode and 100uA in DCDC mode.
 * @return      none.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_bbpll_power_up(void)
{
    analog_write_reg8(areg_aon_0x06, analog_read_reg8(areg_aon_0x06) & (~FLD_PD_BBPLL_LDO));//power up pll

    /*
        trim bit for VCO ibias: from default 101 to 100
        To some extent, it can solve the problem of PLL lock loss. Issue(TER-111)
        The final plan needs further testing to determine.
        (add by jilong.liu, confirmed by yangya 20241223)
    */
    analog_write_reg8(areg_0x84, (analog_read_reg8(areg_0x84) & 0x1f) | 0x80);//solve pll issue

    pm_wait_bbpll_done();
}

/**
 * @brief       this function servers to wait BBPLL clock lock.
 * @return      none.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_wait_bbpll_done(void)
{
    analog_write_reg8(areg_0x85, analog_read_reg8(areg_0x85) | FLD_LOCK_DET_SIG_ENABLE);
    analog_write_reg8(areg_0x86, analog_read_reg8(areg_0x86) | FLD_LOCK_DET_SIG_RESET);

    /*
        Tercel A0 version PLL has a issue(TER-9) that pll cannot get ready signal so here need a vco trim process.
        A1 version has fixed this issue.
    */
    if (g_chip_version == CHIP_VERSION_A0) {
        unsigned char trim_ok = 0;

        /*
        * The default vco ibias value may not be sufficient to support PLL stability.
        * In this case, it is necessary to try adjusting the trim value until a stable PLL signal is obtained.
        * Note: If the obtained gear value here is relatively marginal, it may still affect the stability of PLL
        * in the case of large temperature changes in the future.
        * The Tercel chip select 240MHz as PLL frequency, so the start point vco is 101(5).
        * (add by jilong.liu, confirmed by yangya 20240130)
        */
        for (unsigned char vco_trim = pll_vco_itrim; vco_trim > 0; vco_trim--) {
            core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * 20));//20us, wait vco ibias trim stable
            for (unsigned char i = 0; i < 3; i++) {
                if (FLD_BBPLL_LOCK_DETECTOR == (analog_read_reg8(areg_0x88) & FLD_BBPLL_LOCK_DETECTOR)) {
                    trim_ok = 1;
                    break;
                }
            }
            if (1 == trim_ok) {
                break;
            }
            analog_write_reg8(areg_0x84, (analog_read_reg8(areg_0x84) & 0x1f) | ((vco_trim - 1) << 5));
        }
        //TODO
        /*
            The current temporary solution is while(0 == trim_ok) to make it easier to identify issues.
            If this solution is really adopted in the follow-up, will change it to system reboot.
        */
        while(0 == trim_ok);
    } else {
        unsigned char pll_ok = 0;
        unsigned long long start = rdmcycle();
        /*
        * The standard for judging the stability of PLL is that the ready flag bit read three times in a row is 1.
        * (add by jilong.liu, confirmed by wenfeng.lou 20241203)
        */
        while (pll_ok < 3) {
            if (FLD_BBPLL_LOCK_DETECTOR == (analog_read_reg8(areg_0x88) & FLD_BBPLL_LOCK_DETECTOR)) {
                pll_ok++;
            } else {
                pll_ok = 0;
            }
            //increase timeout to 10ms in case.
            if(core_cclk_time_exceed(start, 1000 * 10))
            {
                drv_timeout_handler(DRV_API_ERROR_TIMEOUT_PLL_DONE);
            }
        }
    }

    analog_write_reg8(areg_0x85, analog_read_reg8(areg_0x85) & (~FLD_LOCK_DET_SIG_ENABLE));
    analog_write_reg8(areg_0x86, analog_read_reg8(areg_0x86) & (~FLD_LOCK_DET_SIG_RESET));
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
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_wait_xtal_ready(unsigned char all_ramcode_en)
{
    //When adding this feature to each chip, it is important to note the following:
    //1.The percentage deviation of high and low temperatures for 24M RC, according to Eagle's test results, is:
    //2.Check the PM_ANA_REG_POWER_ON_CLR_BUF0 bit definition to ensure there are no conflicts, and add comments
    //3.Ensure that this function is called after CCLK switches to RC clock
    //4.Test how long this function will cost to make sure it can cover the poor quality crystal.
    volatile unsigned int j, t0;

#if PM_XTAL_READY_DEBUG
    gpio_function_en(GPIO_PB4);
    gpio_output_en(GPIO_PB4);
    gpio_input_dis(GPIO_PB4);
    gpio_set_high_level(GPIO_PB4);
#endif

    //The timer works abnormally at low temperatures,
    //resulting in frequency bias because the capacitor is not fully added,
    //and the crystal is not connected to the system timer.
    //bit1 configuration capacitors are added on.
    //bit3 configuration crystals are connected to system timer.(added by weihua, confirmed by wenfeng. 20241026)
    analog_write_reg8(0x8b, (analog_read_reg8(0x8b) & (~FLD_XO_DYN_CAP_ANA)) | FLD_XO_CNT_OFF_ANA);

    reg_system_ctrl |= FLD_SYSTEM_TIMER_EN;

    //The method to determine xtal stability here is to use the RC counting method. 
    //After the RC delay of 40us, check whether the stimer count value exceeds 20us(i.e. more than 50% of the rc count value)
    //which is considered to have successfully started vibration.
    //The setting of 10 times is to cover crystal oscillators with poor quality and slow onset as much as possible. 
    //Currently, no worse situation has been encountered in use, so the maximum value is set to 10 times.
    //(added by jilong.liu, confirmed by wenfeng.lou at 20240410)
    for(j = 0; j < g_pm_xtal_stable_loopnum; j++)
    {
#if PM_XTAL_READY_DEBUG
        gpio_toggle(GPIO_PB4);
#endif
        t0 = stimer_get_tick();
        //This for loop is about 40us under the calibrated 24M RC clock.
        //Note: Ensure that the current clock is 24M RC, otherwise the nop running time will not be 40us, resulting in a later calculation error and system reboot.
        //(add by weihua.zhang, confirmed by peng.sun 20230609)
        core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * 40));
        if(stimer_get_tick() - t0 > SYSTEM_TIMER_TICK_1US * 20)
        {
            break;
        }
    }

#if PM_XTAL_ONCE_DEBUG
    if(j > 0)
    {
        while(1){}
    }
#endif
#if(PM_DEBUG)
    debug_xtal_num = j;
#endif

    if(j == g_pm_xtal_stable_loopnum)
    {
        //Use pm_get_sw_reboot_event() to check whether there has been a reset caused by the instability of the crystal oscillator.
        //If it is 1, it has occurred.
        pm_sys_reboot_with_reason(XTAL_UNSTABLE, all_ramcode_en);
    }
    reg_system_ctrl &= (~FLD_SYSTEM_TIMER_EN);
}

/**
 * @brief       this function serves to clear all irq status.
 * @return      Indicates whether clearing irq status was successful.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ unsigned char pm_clr_all_irq_status(void)
{
    unsigned char j, ana_reg64 = 0xff;
    for(j = 0; j < PM_IRQ_STATUS_MAX_CLR_TIMES; j++)
    {
        pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
        ana_reg64 = pm_get_wakeup_src();
        if((ana_reg64 & WAKEUP_STATUS_INUSE_ALL) == 0x00){
            break;
        }else{
#if(PM_DEBUG)
            debug_wakeup_src_clr_err = 1;
            debug_ana_reg64_value[j] = ana_reg64;
            debug_ana_32k_tick_cur[j] = analog_read_reg32(0x60);
            debug_ana_32k_tick_set[j] = analog_read_reg32(0x65);
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
 * @brief       This function serves to recover system timer.
 *              The code is placed in the ram code section, in order to shorten the time.
 * @return      none.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_stimer_recover(void)
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
#if(STIMER_CLOCK == STIMER_CLOCK_16M)
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / 32 * CRYSTAL32768_TICK_PER_32CYCLE;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_32CYCLE / 32;
        }
#elif(STIMER_CLOCK == STIMER_CLOCK_24M)
        if(g_pm_long_suspend){
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        }else{
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
#endif
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
 * @brief      This function serves to get vdd0p94 and vddo1p8 current value from analog register.
 * @return     none.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_update_vdd0p94_level(void)
{
    unsigned char vdd0p94_ldo_value = analog_read_reg8(0x09) & 0x0f;
    unsigned char vdd0p94_dcdc_value = (analog_read_reg8(0x0c) & 0xf0) >> 4;

    if ((vdd0p94_ldo_value == g_pm_cal_vdd0p94_info.ldo_0p95v) && (vdd0p94_dcdc_value == g_pm_cal_vdd0p94_info.dcdc_0p95v)) {
        g_pm_vdd0p94_level = CAL_0P94V_TO_0P95V;
    }
    if ((vdd0p94_ldo_value == g_pm_cal_vdd0p94_info.ldo_1p05v) && (vdd0p94_dcdc_value == g_pm_cal_vdd0p94_info.dcdc_1p05v)) {
        g_pm_vdd0p94_level = CAL_0P94V_TO_1P05V;
    }
}

/**
 * @brief      This function serves to update vdd0p94 and vddo1p8 calibration value.
 * @param[in]  vdd0p94_value - calibration value for vdd0p94(calibration value is 4bytes)
 * @param[in]  vddo1p8_value - calibration value for vddo1p8(calibration value is 1bytes)
 * @return     none
 */
void pm_update_vdd0p94_vddo1p8_cal_value(unsigned char *vdd0p94_value, unsigned char vddo1p8_value)
{
    g_pm_cal_vdd0p94_info.dcdc_0p95v = vdd0p94_value[0];
    g_pm_cal_vdd0p94_info.ldo_0p95v = vdd0p94_value[1];
    g_pm_cal_vdd0p94_info.dcdc_1p05v = vdd0p94_value[2];
    g_pm_cal_vdd0p94_info.ldo_1p05v = vdd0p94_value[3];

    g_pm_cal_vddo1p8_info = vddo1p8_value;
}