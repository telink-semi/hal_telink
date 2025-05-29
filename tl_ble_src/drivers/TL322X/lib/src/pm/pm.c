/********************************************************************************************************
 * @file    pm.c
 *
 * @brief   This is the source file for tl322x
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
#include "lib/include/swire.h"
#if !defined(MCU_CORE_TL322X_N22)
    #define PM_DEBUG          0
    #define SUSPEND_CODE_US   365 //the code run time for preparation entering and exiting suspend sleep
    #define SLEEP_MIN_CODE_US 220 //the minimum code run time before sleep

    #if (PM_DEBUG)
volatile unsigned char debug_pm_info;
volatile unsigned int  debug_ana_32k_tick;
volatile unsigned int  debug_ana_tick_reset;
volatile unsigned char debug_ana_reg64_value[3];
volatile unsigned int  debug_ana_32k_tick_cur[3];
volatile unsigned int  debug_ana_32k_tick_set[3];
volatile unsigned char debug_wakeup_src_clr_err = 0;
volatile unsigned char debug_info_arr[16]       = {0};
    #endif

    #define SRAM_INTF_CFG_0_1V1   0x43
    #define SRAM_INTF_CFG_1_1V1   0x04
    #define SRAM_INTF_CFG_0_1V0   0x42
    #define SRAM_INTF_CFG_1_1V0   0x05

    unsigned char g_pm_voltage_1v_mode     = 0x0a;
    unsigned char g_pm_voltage_1v1_mode    = 0x1a;

    typedef struct {
        unsigned int dcache;
        unsigned int icache;
    } cache_status_t;

_attribute_aligned_(4) pm_status_info_s g_pm_status_info;
volatile unsigned char g_pm_system_reboot_event = 0;
unsigned char          g_areg_aon_7f            = 0;

    #define PM_IRQ_STATUS_MAX_CLR_TIMES 3
    //system timer clock source is constant 24M, never change
    //NOTICE:We think that the external 32k crystal clk is very accurate, does not need to read through TIMER_32K_LAT_CAL.
    //register, the conversion error(use 32k:64 cycle, count 24M sys tmr's ticks), at least the introduction of 64ppm.
    #define CRYSTAL32768_TICK_PER_64CYCLE 46875

_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_calib;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_cur;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_cur;
_attribute_data_retention_sec_ unsigned char                      g_pm_long_suspend;
_attribute_data_retention_sec_ unsigned char                      g_pm_vbat_v;
_attribute_data_retention_sec_ unsigned char                      g_pm_tick_update_en        = 1;
_attribute_data_retention_sec_ static unsigned char               g_pm_suspend_power_cfg     = (FLD_PD_ZB_EN | FLD_PD_USB_EN | FLD_PD_AUDIO_EN);
_attribute_data_retention_sec_ static unsigned char               g_pm_pad_filter_en         = 0x00;
_attribute_data_retention_sec_ unsigned int                       g_pm_multi_addr            = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_calib_repair = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_xtal_stable_loopnum   = 10;
_attribute_data_retention_sec_ unsigned int                       g_pm_suspend_delay_us      = 200;
_attribute_data_retention_sec_ unsigned int                       g_pm_interrupt_status      = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_interrupt_status1     = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_interrupt_status2     = 0;
_attribute_data_retention_sec_ unsigned char                      g_areg_aon_0a              = 0;
_attribute_data_retention_sec_ volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us  = {
     .suspend_early_wakeup_time_us  = 188 + 109 + 200 + SUSPEND_CODE_US,         //188(r_delay) + 109(3.5*(1/32k)) + 200(XTAL_delay) + 300(code)
     .deep_ret_early_wakeup_time_us = 188 + 109,                                 //188(r_delay) + 109(3.5*(1/32k))
     .deep_early_wakeup_time_us     = 688 + 109 + 390,                           //688(r_delay) + 109(3.5*(1/32k)) + 390(boot_rom)
     .sleep_min_time_us             = 688 + 109 + 390 + SLEEP_MIN_CODE_US + 200, //(the maximum value of suspend and deep) + 200. 200 means more margin, >32 is enough.
};
_attribute_data_retention_sec_ volatile pm_r_delay_cycle_s g_pm_r_delay_cycle = {
    .deep_r_delay_cycle           = 3 + 8,                                       // 11 * (1/16k) = 687.5
    .suspend_ret_r_delay_cycle    = 3,                                           // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
    .deep_xtal_delay_cycle        = 3 + 8,                                       // 11 * (1/16k) = 687.5
    .suspend_ret_xtal_delay_cycle = 3,                                           // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
};

//extern _attribute_ram_code_sec_ void flash_send_cmd(unsigned int cmd);

/**
 * @brief       This function configures pm wakeup time parameter.
 * @param[in]   param - deep/suspend/deep_retention r_delay time.(default value: suspend/deep_ret=3, deep=11)
 * @return      none.
 */
void pm_set_wakeup_time_param(pm_r_delay_cycle_s param)
{
    g_pm_r_delay_cycle.deep_r_delay_cycle           = param.deep_r_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_r_delay_cycle    = param.suspend_ret_r_delay_cycle;
    g_pm_r_delay_cycle.deep_xtal_delay_cycle        = param.deep_xtal_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle = param.suspend_ret_xtal_delay_cycle;

    int deep_r_delay_us                                     = g_pm_r_delay_cycle.deep_r_delay_cycle * 1000 / 16;
    int suspend_ret_r_delay_us                              = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle * 1000 / 16;
    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us  = suspend_ret_r_delay_us + 109 + 200 + SUSPEND_CODE_US;
    g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us = suspend_ret_r_delay_us + 109;
    g_pm_early_wakeup_time_us.deep_early_wakeup_time_us     = deep_r_delay_us + 109 + 390;
    if (g_pm_early_wakeup_time_us.deep_early_wakeup_time_us < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us) {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    } else {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
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
    g_pm_suspend_delay_us    = delay_us;

    int suspend_ret_r_delay_us                             = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle * 1000 / 16;
    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us = suspend_ret_r_delay_us + 109 + delay_us + SUSPEND_CODE_US;
    if (g_pm_early_wakeup_time_us.deep_early_wakeup_time_us < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us) {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    } else {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + SLEEP_MIN_CODE_US + 200;
    }
}

/**
 * @brief       This function serves to recover system timer.
 *              The code is placed in the ram code section, in order to shorten the time.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_stimer_recover(void)
{
    #if SYS_TIMER_AUTO_MODE
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_START, 0);
    unsigned int now_tick_32k = clock_get_32k_tick();
    if (CLK_32K_RC == g_clk_32k_src) {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    } else {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_DONE, g_pm_tick_cur + 1);
    stimer_32k_tracking_enable(); //enable 32k cal

    #else
        #error-- only for internal testing.
    unsigned int now_tick_32k = clock_get_32k_tick();
    if (CLK_32K_RC == g_clk_32k_src) {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    } else {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }
    stimer_enable(STIMER_MANUAL_MODE, g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US);
    stimer_32k_tracking_enable(); //enable 32k cal
    #endif
}

/**
 * @brief       This function configures a GPIO pin as the wakeup pin.
 * @param[in]   pin - the pins can be set to all GPIO except PB0/PC5 and GPIOG groups.
 * @param[in]   pol - the wakeup polarity of the pad pin(0: low-level wakeup, 1: high-level wakeup).
 * @param[in]   en  - enable or disable the wakeup function for the pan pin(1: enable, 0: disable).
 * @return      none.
 */
void pm_set_gpio_wakeup(gpio_pin_e pin, pm_gpio_wakeup_level_e pol, int en)
{
    ///////////////////////////////////////////////////////////
    //        PA[7:0]           PB[7:0]         PC[7:0]         PD[7:0]     PE[7:0]
    // pol: ana_0x3f<7:0>    ana_0x40<7:0>  ana_0x41<7:0>  ana_0x42<7:0>  ana_0x43<7:0>
    // en:  ana_0x45<7:0>    ana_0x46<7:0>  ana_0x47<7:0>  ana_0x48<7:0>  ana_0x49<7:0>
    unsigned char mask = pin & 0xff;
    unsigned char analog_reg;
    unsigned char val;

    ////////////////////////// polarity ////////////////////////
    analog_reg = ((pin >> 8) + 0x3f);
    val        = analog_read_reg8(analog_reg);
    if (pol) {
        val &= ~mask;
    } else {
        val |= mask;
    }
    analog_write_reg8(analog_reg, val);

    /////////////////////////// enable /////////////////////
    analog_reg = ((pin >> 8) + 0x45);
    val        = analog_read_reg8(analog_reg);
    if (en) {
        val |= mask;
    } else {
        val &= ~mask;
    }
    analog_write_reg8(analog_reg, val);
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
_attribute_ram_code_sec_noinline_ void pm_wait_xtal_ready(unsigned char all_ramcode_en)
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
    for (j = 0; j < g_pm_xtal_stable_loopnum; j++) {
        t0 = stimer_get_tick();
        //This for loop is about 40us under the calibrated 24M RC clock.
        //Note: Ensure that the current clock is 24M RC, otherwise the nop running time will not be 40us, resulting in a later calculation error and system reboot.
        //(add by weihua.zhang, confirmed by peng.sun 20230609)
        core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * 40));
        if (stimer_get_tick() - t0 > SYSTEM_TIMER_TICK_1US * 20) {
            break;
        }
    }

    if (j == g_pm_xtal_stable_loopnum) {
        //Use PM_ANA_REG_POWER_ON_CLR_BUF0[bit1] to check whether there has been a reset caused by the instability of the crystal oscillator.
        //If it is 1, it has occurred.
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | 0x02);
    #if (PM_DEBUG)
        debug_info_arr[0] = 1;
        while (1)
            ;
    #endif
        if (all_ramcode_en == 0x00) {
            sys_reboot();
        } else {
            sys_reset_all();
            while (1)
                ;
        }
    }
    reg_system_ctrl &= (~FLD_SYSTEM_TIMER_EN);
}

/**
 * @brief       This function serves to update wakeup status.
 * @param[in]   clr_en  - Whether to set the value of the status register to a fixed value.
 *                        If the interface is called twice, the first time it is not modified, clr_en=0;
 *                        if the interface is called once, it is modified, clr_en=1.
 * @return      none.
 * @note        After calling this interface, it is necessary to clear the flag of the timer watchdog or the 32k watchdog.
 *              Otherwise, if the flag remains set, it may affect the next judgment.
 *              After calling this interface, other states are set to fixed values.
 *              Therefore, this interface cannot be called twice,
 *              and if it is called twice, the state will be fixed to one state, not the correct state.
 */
_attribute_ram_code_sec_noinline_ void pm_update_status_info(unsigned char clr_en)
{
    if (g_pm_status_info.mcu_status == MCU_DEEPRET_BACK) {
        return;
    }

    unsigned char wd_clr0      = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);
    unsigned char poweron_clr0 = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0);

    if (wd_clr0 & POWERON_FLAG) {
        if (poweron_clr0 & REBOOT_FLAG) {
            g_pm_status_info.mcu_status = MCU_SW_REBOOT_BACK;
            if (wd_get_status()) {
                g_pm_status_info.mcu_status = MCU_HW_REBOOT_TIMER_WATCHDOG;
            } else {
                g_pm_system_reboot_event = (poweron_clr0 >> 1);
            }
            if (clr_en == 1) {
                analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, wd_clr0 & (~POWERON_FLAG));
            }
        } else {
            g_pm_status_info.mcu_status = MCU_POWER_ON;
            if (wd_32k_get_status()) {
                g_pm_status_info.mcu_status = MCU_HW_REBOOT_32K_WATCHDOG;
            }
            if (clr_en == 1) {
                analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, wd_clr0 & (~POWERON_FLAG));
                analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, REBOOT_FLAG);
            }
        }
    } else {
        g_pm_status_info.mcu_status = MCU_DEEP_BACK;
    }
}

/**
 * @brief       this function serves to clear all irq status.
 * @return      Indicates whether clearing irq status was successful.
 */
_attribute_ram_code_sec_noinline_ unsigned char pm_clr_all_irq_status(void)
{
    unsigned char j, ana_reg64 = 0xff;
    for (j = 0; j < PM_IRQ_STATUS_MAX_CLR_TIMES; j++) {
//        pm_clr_irq_status(WAKEUP_STATUS_ALL);
        ana_reg64 = pm_get_wakeup_src();
        if (ana_reg64 == 0x00) {
            break;
        } else {
    #if (PM_DEBUG)
            debug_wakeup_src_clr_err  = 1;
            debug_ana_reg64_value[j]  = ana_reg64;
            debug_ana_32k_tick_cur[j] = analog_read_reg32(0x60);
            debug_ana_32k_tick_set[j] = analog_read_reg32(0x65);
            /******************************************debug_pm_info**********************************************/
            debug_info_arr[1] = 1;
            debug_pm_info     = 0x21;
    #endif
        }
    }
    if (j == PM_IRQ_STATUS_MAX_CLR_TIMES) {
        return 0;
    } else {
        return 1;
    }
}

volatile unsigned int  clr_plic_request_result = 0;
volatile unsigned char clr_pm_irq_result       = 0;

/**
 * @brief       this function serves to start sleep mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @return      none.
 */
_attribute_ram_code_sec_ static void pm_sleep_start(pm_sleep_mode_e sleep_mode)
{
    pm_set_dig_module_power_switch(g_pm_suspend_power_cfg, PM_POWER_DOWN);

    //    flash_send_cmd(FLASH_WRITE_DEEP_CMD);
    reg_gpio_pf_ie = 0x00; //MSPI ie disable

    //This is 1.2V and 2.0V power supply during sleep. Do not power on during initialization, because after power on,
    //there will be two power supplies at the same time, which may cause abnormalities.add by weihua.zhang, confirmed by haitao 20210107
//    analog_write_reg8(areg_aon_0x0b, (analog_read_reg8(areg_aon_0x0b) & ~(FLD_PD_NVT_0P94 | FLD_PD_NVT_1P8)));
    //<0>:pd_nvt_0p94,  default:1,->0 power on native 0P94 dcdc.
    //<1>:pd_nvt_2p0,   default:1,->0 power on native 2P0 dcdc.

    #if (PM_DEBUG)
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
    // unsigned int clr_plic_request_result = 0;
    // unsigned char
    clr_pm_irq_result = pm_clr_all_irq_status();
    if (clr_pm_irq_result == 1) {
        clr_plic_request_result = plic_clr_all_request();
    #if (PM_DEBUG)
        debug_info_arr[2] = 1;
    #endif
    }

    #if (PM_DEBUG)
    debug_pm_info = 0x22;
    #endif

    //If the clearing fails, it indicates that the wake source is active. In this case, the deep mode will reboot,
    //and the other modes continue to run downward.(add by weihua.zhang, 20230907)
    if ((clr_pm_irq_result == 0) || (clr_plic_request_result == 0)) {
    #if (PM_DEBUG)
        debug_info_arr[3] = 1;
        while (1)
            ;
    #endif
        if (sleep_mode == DEEPSLEEP_MODE) {
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | 0x04);
            sys_reset_all(); //Clear ana_0x64 for three times. If the status is not completely cleared for three times, reboot the mcu.
            while (1)
                ;
        }
    } else {
        //0x80 is to enter low power mode immediately. 0x81 is to wait for D25F to enter wfi mode before entering low power,this way is more secure.
        //Once in the WFI mode, memory transactions that are started before the execution of WFI are guaranteed to have been completed,
        //all transient states of memory handling are flushed and no new memory accesses will take place.
        //only suspend requires this process, after waking up to resume the scene.
        //(add by bingyu.li, confirmed by jianzhi.chen 20230810)
        write_reg8(0x14082f, 0x81); //stall mcu trig
        __asm__ __volatile__("wfi");
    }

    #if (PM_DEBUG)
    debug_pm_info = 0x23;
    #endif

    //This delay time is about 3.05us under the calibrated 24M RC clock.
    //After being triggered, the MCU needs to wait for a period of time before it actually goes to sleep,
    //during which time the MCU will continue to execute code. If the following code is executed
    //and some modules are awakened, the current will be larger than normal. About 20 empty instructions are fine,
    //but to be on the safe side, add 64 empty instructions.
    //The statement of the for loop may be optimized away by the compiler, resulting in a crash due to insufficient wait time,
    //so it cannot be used.(add by weihua.zhang, confirmed by sihui.wang 20230324)
    CLOCK_DLY_64_CYC;

    g_pm_status_info.wakeup_src = pm_get_wakeup_src();

    #if (PM_DEBUG)
    debug_pm_info = 0x24;
    #endif

    //Here we need to turn off the mask first, and then clear the plic. If the wake signal is always present and the interrupt mask is enabled,
    //the plic cannot be cleared. When exiting the sleep function, if the total interrupt is turned on, the interrupt handler function is entered.
    //If the interrupt handler is not defined, the program will run away.(changed by weihua.zhang, confirmed by jianzhi 20231101)
    plic_interrupt_disable(IRQ_PM_LVL);
    pm_clr_all_irq_status(); //clear all flag
    plic_clr_all_request();

    #if (PM_DEBUG)
    debug_pm_info = 0x25;
    #endif

    analog_write_reg8(areg_aon_0x0b, (analog_read_reg8(areg_aon_0x0b) | (FLD_PD_NVT_1P25 | FLD_PD_NVT_1P8)));
    //<0>:pd_nvt_0p94,  power down native 0P94 dcdc.
    //<1>:pd_nvt_2p0,   power down native 2P0 dcdc.
    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) & 0x3f) | FLD_PD_SPD_LDO);
    //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
    //<7>:dig_ret_pd,   default:1,->0 Power on retention ldo.
    #if (PM_DEBUG)
    debug_pm_info = 0x26;
    #endif

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

    #if (PM_DEBUG)
    debug_pm_info = 0x27;
    #endif

    //Before sleeping,the MSPI has already been switched to 24M RC, so there is no need to wait for Xtal and PLL to stabilize.
    //Advance the flash wakeup to before the delay, because after the flash wakeup, It will take some time to restore to the active working state.
    //(usually set to 150us - a margin is left for different flash models)(add by bingyu.li, confirmed by kaixin.chen 20230616)
    //The flash two-wire system uses clk+cn+ two communication lines, and the flash four-wire system uses
    //clk+cn+ four communication lines. Before suspend sleep, the input of the six lines (PF0-PF5) used
    //by flash will be disabled. After suspend wakes up, the six lines will be set to input function.
    //(changed by weihua.zhang, confirmed by jianzhi 20201201)
    reg_gpio_pf_ie = 0x3f; //MSPI(PF0-PF5) ie enable
    //    flash_send_cmd(FLASH_WRITE_RELEASE_CMD);        //flash wakeup

    #if (PM_DEBUG)
    debug_pm_info = 0x28;
    #endif
    //wait for xtal stable and flash restore to the active working state.
    core_cclk_delay_tick(sys_clk.cclk * g_pm_suspend_delay_us);

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_xtal_ready(0x01);

    //When the crystal oscillator is stable, the PLL may also be unstable, so it is still necessary to wait for the PLL stable sign.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_bbpll_done();

    #if (PM_DEBUG)
    debug_pm_info = 0x29;
    #endif
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
_attribute_ram_code_sec_noinline_ int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode, pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int wakeup_tick)
{
    /**
     * At present, the compensation value in the function is tested on the basis of the optimization level O2. If there are other optimization levels,
     * it needs to be retested and then see how the compensation time is handled.
     */

    ////////// disable IRQ //////////////////////////////////////////
    //If the time point of closing the total interrupt is later, the function may be interrupted by the interrupt,
    //cause the wake-up tick value to be calculated incorrectly, resulting in incorrect sleep time.
    //modify by weihua.zhang, confirmed by sihui.wang at 20220908.
    unsigned int r = core_interrupt_disable();

    ///////////////////       /////////////////////////////////
    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    if (timer_wakeup_enable && (wakeup_tick_type == PM_TICK_STIMER)) {
        unsigned int span = (unsigned int)(wakeup_tick - stimer_get_tick());
        if (span > 0xE0000000) //BIT(31)+BIT(30)+BIT(29)   7/8 cycle of 32bit, 178*7/8 = 156 S
        {
            core_restore_interrupt(r);
            return pm_get_wakeup_src();
        } else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US) {
            unsigned int t = stimer_get_tick();
//            pm_clr_irq_status(WAKEUP_STATUS_ALL);
            unsigned char st;
            do {
                st = pm_get_wakeup_src();
            } while (((unsigned int)stimer_get_tick() - t < span) && !st);

    #if (PM_DEBUG)
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
    g_pm_interrupt_status  = reg_irq_src0;
    g_pm_interrupt_status1 = reg_irq_src1;
    g_pm_interrupt_status2 = read_csr(NDS_MIE);
    reg_irq_src0           = 0;
    reg_irq_src1           = 0;
    core_mie_enable(FLD_MIE_MEIE);
    core_mie_disable(FLD_MIE_MSIE | FLD_MIE_MTIE);

    ///////////////////     get 32k calib      /////////////////////////////////
    if (CLK_32K_RC == g_clk_32k_src) {
        while (!read_reg32(0x140214))
            ; //Wait for the 32k clock calibration to complete.

        g_pm_tick_32k_calib = read_reg32(0x140214);

    #if (PM_DEBUG)
        analog_write_reg16(PM_ANA_REG_POWER_ON_CLR_BUF1, g_pm_tick_32k_calib);
        /******************************************debug_pm_info 2 **********************************************/
        debug_pm_info = 2;
    #endif
    } else {
        g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_64CYCLE;
    }
    unsigned int tick_32k_halfCalib = g_pm_tick_32k_calib >> 1;

    ///////////////////    change clock    /////////////////////////////////
    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    mspi_stop_xip();

    clock_save_clock_config();
    clock_set_all_clock_to_default();

    #if (PM_DEBUG)
    /******************************************debug_pm_info 3 **********************************************/
    debug_pm_info = 3;
    #endif

    /////////////////// stop system timer /////////////////////////////////
    #if SYS_TIMER_AUTO_MODE
    stimer_32k_tracking_disable();           //disable 32k track
    stimer_set_update_upon_nxt_32k_enable(); //system tick only update upon 32k posedge, must set before enable 32k read update!!!
    g_pm_tick_32k_cur = clock_get_32k_tick();
    g_pm_tick_cur     = stimer_get_tick();
    stimer_set_update_upon_nxt_32k_disable();
    stimer_disable(); //disable system timer
    #else
        #error-- Manual mode is only for internal testing, and 37 in the code may not be accurate
    stimer_32k_tracking_disable();                                  //disable 32k track
    g_pm_tick_cur = stimer_get_tick() + 37 * SYSTEM_TIMER_TICK_1US; //cpu_get_32k_tick will cost 30~40 us
    stimer_disable();                                               //disable system timer
    g_pm_tick_32k_cur = clock_get_32k_tick();
    #endif

    #if (PM_DEBUG)
    analog_write_reg32(PM_ANA_REG_WD_CLR_BUF1, g_pm_tick_32k_cur);
    /******************************************debug_pm_info 4 **********************************************/
    debug_pm_info = 4;
    #endif

    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8(0x4b, wakeup_src);

    unsigned int earlyWakeup_us;
    if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) //deep sleep with retention
    {
        //        g_pm_multi_addr = reg_mspi_xip_core_size|(reg_mspi_xip_core_offset<<16);//after retention, multiple address offset is lost, save it
        analog_write_reg8(0x7e, sleep_mode); //sram retention
        //0x4d:
        //1:auto power down:<0>lpc <1>dcore/sram LDO <2>UVLO ib <3>vbus switch <4>flash LDO
        //<6>power down sequence enable <7>enable isolation
        if (wakeup_src & PM_WAKEUP_COMPARATOR) {
            analog_write_reg8(0x4d, 0xc8); //retention
        } else {
            analog_write_reg8(0x4d, 0xc9); //retention
        }
        analog_write_reg8(0x06, (analog_read_reg8(0x06) & 0x3f) | FLD_PD_SPD_LDO);
        //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
        //<7>:dig_ret_pd,   default:1,->0 Power on retention ldo.
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us;

    #if (PM_DEBUG)
        /******************************************debug_pm_info 5 **********************************************/
        debug_pm_info = 5;
    #endif

    } else if (sleep_mode == DEEPSLEEP_MODE) //deepsleep no retention
    {
        //0x4d:
        //1:auto power down:<0>lpc <1>dcore/sram LDO <2>UVLO ib <3>vbus switch <4>flash LDO
        //<6>power down sequence enable <7>enable isolation
        if (wakeup_src & PM_WAKEUP_COMPARATOR) {
            analog_write_reg8(0x4d, 0xc8); //deep
        } else {
            analog_write_reg8(0x4d, 0xc9); //deep
        }
        analog_write_reg8(0x06, analog_read_reg8(0x06) | FLD_PD_SPD_LDO | FLD_PD_DIG_RET_LDO);
        //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
        //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;

    #if (PM_DEBUG)
        /******************************************debug_pm_info 6 **********************************************/
        debug_pm_info = 6;
    #endif

    } else //suspend
    {
        //0x4d:
        //1:auto power down:<0>lpc <1>dcore/sram LDO <2>UVLO ib <3>vbus switch <4>flash LDO
        //<6>power down sequence enable <7>enable isolation
        if (wakeup_src & PM_WAKEUP_COMPARATOR) {
            analog_write_reg8(0x4d, 0x48); //suspend
        } else {
            analog_write_reg8(0x4d, 0x49); //suspend
        }
        analog_write_reg8(0x06, (analog_read_reg8(0x06) & 0x3f) | FLD_PD_DIG_RET_LDO);
        //<6>:spd_ldo_pd,   default:1,->0 Power up spd ldo.
        //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;

    #if (PM_DEBUG)
        /******************************************debug_pm_info 7 **********************************************/
        debug_pm_info = 7;
    #endif
    }

    /////////////////// auto power down /////////////////////////////////
    //0x4c:<0>32KRC <1>32K xtal <2>4M rcosc <3>24M xtal <4>logic <5>dcdc <6>vbus LDO <7>ana/BBPLL/temp_sensor LDO
    if (CLK_32K_RC == g_clk_32k_src) {
        if (((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_COMPARATOR)) {
            analog_write_reg8(0x4c, 0xea); //disable auto power down 32KRC
        } else {
            analog_write_reg8(0x4c, 0xeb); //enable auto power down 32KRC
        }
    } else {
        //suspend mode or deep retention mode or timer wake up source.
        //(we don't power down external 32k pad clock, even though three's no timer wake up source in suspend or deep retention mode)
        if ((sleep_mode == SUSPEND_MODE) || (sleep_mode & DEEPSLEEP_RETENTION_FLAG) || ((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_COMPARATOR)) {
            analog_write_reg8(0x4c, 0xe9); //if use timer wake up, auto pad 32k power down should be disabled
        }
        //if deep mode and no timer wakeup
        else {
            analog_write_reg8(0x4c, 0xeb); //enable auto power down pad 32k
        }
    }

    if (sleep_mode == DEEPSLEEP_MODE) {
        analog_write_reg8(0x3d, g_pm_r_delay_cycle.deep_xtal_delay_cycle);
        analog_write_reg8(0x3e, g_pm_r_delay_cycle.deep_r_delay_cycle);        //(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    } else {
        analog_write_reg8(0x3d, g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle);
        analog_write_reg8(0x3e, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle); //(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }

    #if (PM_DEBUG)
    /******************************************debug_pm_info 8 **********************************************/
    debug_pm_info = 8;
    #endif

    //The variable pmcd.ref_tick is added, replacing the original variable g_pm_tick_cur. Because pmcd.ref_tick directly affects the value of
    //g_pm_long_suspend, g_pm_long_suspend can be assigned after pmcd.ref_tick is updated.changed by weihua,confirmed by biao.li.20201204.
    if (timer_wakeup_enable) {
        unsigned int tick_reset;
        unsigned int tick_wakeup_reset;
        if (wakeup_tick_type == PM_TICK_STIMER) {
            tick_wakeup_reset = (unsigned int)(wakeup_tick - (earlyWakeup_us * SYSTEM_TIMER_TICK_1US) - g_pm_tick_cur);
            if (CLK_32K_RC == g_clk_32k_src) {
                if (tick_wakeup_reset > 0x0fff0000) // 24M: 11.18S
                {
                    tick_reset        = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * g_track_32kcnt;
                    g_pm_long_suspend = 1;
                } else {
                    tick_reset        = g_pm_tick_32k_cur + (tick_wakeup_reset * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
            } else {
                if (tick_wakeup_reset > 0x03ff0000) // 24M: 2.79S
                {
                    tick_reset        = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 64;
                    g_pm_long_suspend = 1;
                } else {
                    tick_reset        = g_pm_tick_32k_cur + (tick_wakeup_reset * 64 + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
            }
        } else {
            tick_reset = g_pm_tick_32k_cur + wakeup_tick - (earlyWakeup_us * 4 / 125); // 32k clk: /31.25
        }
        clock_set_32k_tick(tick_reset);

    #if (PM_DEBUG)
        debug_ana_32k_tick = analog_read_reg32(0x65);
        if (tick_reset != debug_ana_32k_tick) {
            debug_ana_tick_reset = tick_reset;
            stimer_enable_in_manual_mode();
            stimer_32k_tracking_enable(); //enable 32k cal
            gpio_set_high_level(GPIO_PE7);
            while (1)
                ;
        }
        /******************************************debug_pm_info 9 **********************************************/
        debug_pm_info = 9;
    #endif
    }

    //Clear the wake source status after setting the wake tick.The wake tick value is set by bit shift.
    //This process will generate an intermediate value, which may be the same as the current 32k tick value.
    //If the value is the same, the state of the timer wake source will be set.
    //changed by weihua, confirmed by jianzhi. 20240711.
//    pm_clr_irq_status(WAKEUP_STATUS_ALL); //clear all flag

    if (pm_get_wakeup_src()) {
    } else {
        if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) {
            analog_write_reg8(0x7f, (0x40 | g_pm_pad_filter_en));
        }

        pm_sleep_start(sleep_mode);

        analog_write_reg8(0x7f, (0x41 | g_pm_pad_filter_en));

    #if (PM_DEBUG)
        /******************************************debug_pm_info 10 **********************************************/
        debug_pm_info = 10;
    #endif
    }

    if (sleep_mode == DEEPSLEEP_MODE) {
        sys_reset_all(); //reboot
    }

    #if SYS_TIMER_AUTO_MODE
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_START, 0);
    unsigned int now_tick_32k = clock_get_32k_tick();
    if (CLK_32K_RC == g_clk_32k_src) {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    } else {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k + 1 - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }

        #if (PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
        #endif
    stimer_enable(STIMER_AUTO_MODE_W_AND_NXT_32K_DONE, g_pm_tick_cur + 1);
    stimer_32k_tracking_enable(); //enable 32k cal

    #else
        #error-- only for internal testing.
    unsigned int now_tick_32k = clock_get_32k_tick();
    if (CLK_32K_RC == g_clk_32k_src) {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;
        }
    } else {
        if (g_pm_long_suspend) {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / 64 * CRYSTAL32768_TICK_PER_64CYCLE;
        } else {
            g_pm_tick_cur = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * CRYSTAL32768_TICK_PER_64CYCLE / 64;
        }
    }

        #if (PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
        #endif

    stimer_enable(STIMER_MANUAL_MODE, g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US);
    stimer_32k_tracking_enable(); //enable 32k cal
    #endif

    #if (PM_DEBUG)
    /******************************************debug_pm_info 12 **********************************************/
    debug_pm_info = 12;
    #endif

    clock_restore_clock_config();

    mspi_set_xip_en();

    #if (PM_DEBUG)
    /******************************************debug_pm_info 13 **********************************************/
    debug_pm_info = 13;
    #endif

    if ((g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable) //wakeup from timer only
    {
        if (wakeup_tick_type == PM_TICK_STIMER) {
            while ((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30))
                ;
        } else {
            while ((unsigned int)(clock_get_32k_tick() - wakeup_tick - g_pm_tick_32k_cur + 1) > BIT(30))
                ;
        }
    }

    #if (PM_DEBUG)
    /******************************************debug_pm_info 14 **********************************************/
    debug_pm_info = 14;
    #endif

    //  DBG_CHN2_LOW;
    //Resume the interrupted state before sleep.Cannot be placed in the pm_sleep_start() interface to avoid failure to recover if this interface is not called.
    //changed by weihua, confirmed by jianzhi. 20231115
    reg_irq_src0 = g_pm_interrupt_status;
    reg_irq_src1 = g_pm_interrupt_status1;
    write_csr(NDS_MIE, g_pm_interrupt_status2);
    core_restore_interrupt(r);

    #if (PM_DEBUG)
    /******************************************debug_pm_info 15 **********************************************/
    debug_pm_info = 15;
    #endif
    /**
     * Under normal circumstances, the wake up source cannot be zero. B85 had an exception that the wake up source was zero and never encountered it again.
     * STATUS_GPIO_ERR_NO_ENTER_PM indicates this case where the wake up source is zero. The name of this flag is not quite appropriate, but it has been used for a long time, so it is still used today.
     * added by bingyu.li, confirmed by sihui.wang at 20231018.
     */
    return (g_pm_status_info.wakeup_src ? (g_pm_status_info.wakeup_src | STATUS_ENTER_SUSPEND) : STATUS_GPIO_ERR_NO_ENTER_PM);
}

_attribute_text_sec_ int pm_sleep_wakeup(pm_sleep_mode_e sleep_mode, pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int wakeup_tick)
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
    if (0 == on_off) {
        g_pm_suspend_power_cfg |= (value);
    } else {
        g_pm_suspend_power_cfg &= ~(value);
    }
}

/**
 * @brief       This function serves to switch digital module power.
 * @param[in]   module - digital module.
 * @param[in]   power_sel - power up or power down.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_set_dig_module_power_switch(pm_pd_module_e module, pm_power_sel_e power_sel)
{
    /**
     * The temporary workaround is:
     * Before setting analog register 0x7d, switch the clock to 24M RC, then restore it after the setting is complete. Otherwise, at high frequencies, the system may crash.
     * Next, we will evaluate the power consumption of the module controlled by 0x7d. If the power consumption is low,
     * we can enable it in sys_init to reduce the risk of multiple operations.
     *
     * Current Observations (Root Cause Not Yet Confirmed)
     * When writing ana_0x7d (USB power on), the system frequently crashes.Testing found that vdd_core voltage drops to ~800mV.
     * After replacing board capacitors (as suggested by Wenfeng), the core voltage drop improved, and crashes were reduced.
     * Even at 1.1V core voltage, crashes still occur occasionally.
     * Using PLL_192M_CCLK_192M_HCLK_D25F_N22_96M_PCLK_96M_MSPI_48M causes more crashes than PLL_192M_CCLK_96M_HCLK_D25F_N22_48M_PCLK_48M_MSPI_48M.
     * Analysis suggests that 0x7d operations are unstable at high frequencies.
     * Current Status:
     * The root cause is still under investigation.
     * The temporary workaround (24M RC switch) is currently in use.
     * (added by kaixin.chen, confirmed by wenfeng at 20250514)
     */
    clock_save_clock_config();
    clock_set_all_clock_to_default();
    /*
    * After setting the power switch register of the digital module in the second step, it will take some time to take effect.
    * Check that the power switch is stable.In short, the recommendation sequence of enable a module:
    * power up module -> wait power stable -> clock enable.
    * (added by jilong.liu, confirmed by junwen.jiang at 20240227)
    */
    if (power_sel == PM_POWER_UP) {
        analog_write_reg8(areg_aon_0x7d, (analog_read_reg8(areg_aon_0x7d) | FLD_PG_CLK_EN) & ~(module));
    } else if (power_sel == PM_POWER_DOWN) {
        analog_write_reg8(areg_aon_0x7d, (analog_read_reg8(areg_aon_0x7d) | FLD_PG_CLK_EN) | module);
    }

    /*
     * Buteo don't have the pd_sm_busy bit(Onca and Tercel has 0x69<5> which can indicate power switch status).
     * And the 0x69<0:3> can only indicate the power up process, it's invalid when power down.
     * Wait for power stable, for this chip(Buteo), is a fixed value 5us.
     * (added by jilong.liu, confirmed by jianzhi.chen at 20240514)
     * The time required for different chips may vary so it is necessary to confirm with the chip colleagues when porting other chips.
     */
    core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * 5)); //delay 5us
    clock_restore_clock_config();
}

/**
 * @brief       This function serves to test different voltages from pd3.
 * @param[in]   mux_sel - select different voltages from pd3.
 * @return      none.
 */
void pm_set_probe_vol_to_pc3(pm_vol_mux_sel_e mux_sel)
{
    /**
     * Probe voltage to GPIO_PC3.
     * Configurations to other values are used internally by the chip designer as debug signals.
     * added by jilong.liu, confirmed by yangya at 20240530.
     */
    analog_write_reg8(0x11, (analog_read_reg8(0x11) & 0xf0) | mux_sel);
    /**
    * afe3V-reg17<4:0>      value           note
    * ---------------------------------------------------------------------------
    *                       4'b0010         vdd_bb
    *                       4'b0011         vdd_efuse
    *                       4'b0100         vdd_core
    *                       4'b0101         vdd_retram
    */
    gpio_function_en(GPIO_PC3);
    gpio_output_dis(GPIO_PC3);
    gpio_input_en(GPIO_PC3);
}

/********************************************************************************************************
 *                                          internal
 *******************************************************************************************************/

/********************************************************************************************************
 *              This is just for internal debug purpose, users are prohibited from calling.
 *******************************************************************************************************/

/**
 * @brief       This function serves to trim dig ldo voltage.
 * @param[in]   dig_ldo_trim - dig ldo trim voltage
 * @return      none
 */
void pm_set_dig_ldo_voltage(pm_dig_ldo_trim_e dig_ldo_trim)
{
    analog_write_reg8(areg_aon_0x0f, (analog_read_reg8(areg_aon_0x0f) & 0x0f) | (dig_ldo_trim << 4));
}

/**
 * @brief       This function serves to trim deep retention LDO voltage
 * @param[in]   ret_ldo_trim - deep retention LDO trim voltage
 * @return      none
 */
void pm_set_ret_ldo_voltage(pm_ret_ldo_trim_e ret_ldo_trim)
{
    analog_write_reg8(areg_aon_0x0f, (analog_read_reg8(areg_aon_0x0f) & 0xf8) | ret_ldo_trim);
}

/**
 * @brief       This function serves to trim ldo 1.25v out.     
 * @param[in]   trim_1p25vldo - ldo trim 1.25v out
 */
void pm_set_1p25v(pm_trim_1p25v_e trim_1p25vldo)
{
    analog_write_reg8(areg_aon_0x09, (analog_read_reg8(areg_aon_0x09) & 0xf0) | trim_1p25vldo);
}

/**
 * @brief       This function serves to set system power mode.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC).
 * @return      none.
 * @note        pd_dcdc_ldo_sw<1:0>, dcdc & bypass ldo status bits:
                    dcdc_1p25   dcdc_1p8     ldo_1p25    ldo_1p8
                00:     N           N           Y           Y
                01:     Y           N           N           Y
                10:     Y           N           N           N
                11:     Y           Y           N           N
 */
_attribute_ram_code_sec_noinline_ void pm_set_power_mode(power_mode_e power_mode)
{
    if (g_pm_status_info.mcu_status != MCU_STATUS_DEEPRET_BACK) {

        g_areg_aon_0a = analog_read_reg8(areg_aon_0x0a);
    }

    if ((g_areg_aon_0a & 0x03) != power_mode) {
        //The power-on process of a DCDC requires a 24M rc clock.(add by weihua.zhang, confirmed by wenfeng.lou 20240903)
        if (LDO_1P25_LDO_1P8 != power_mode) {

            pm_24mrc_power_up();
        }

        g_areg_aon_0a = (g_areg_aon_0a & 0xfc) | power_mode;
        analog_write_reg8(areg_aon_0x0a, g_areg_aon_0a);

        if (LDO_1P25_LDO_1P8 != power_mode) {
            if (!g_24m_rc_is_used) {
                core_cclk_delay_tick((unsigned long long)(25 * sys_clk.cclk));
            }
            pm_24mrc_power_down_if_unused();
        }
    }
}


/**
 * @brief       This function is used to write to the analog registers when configuring the dig ldo voltage.
 * @return      none
 */
static _attribute_text_sec_optimize_o2_ void pm_set_dig_ldo_write_ana_0x27(void)
{
     reg_ana_addr    = areg_aon_0x27;
     reg_ana_len     = 1;
     reg_ana_data(0) = reg_boot_idcode(0);
     while (!(reg_ana_buf_cnt & FLD_ANA_TX_BUFCNT))
         ;
     reg_ana_ctrl = (FLD_ANA_CYC | FLD_ANA_RW | ((areg_aon_0x27 & 0x00000300) >> 8));
     while (reg_ana_ctrl & FLD_ANA_BUSY) {
     }
     reg_ana_ctrl = 0x00;
}

/**
 * @brief       This function is used to write to the analog registers when configuring the dig ldo voltage.
 * @return      none
 */
static _attribute_text_sec_optimize_o2_ void pm_set_dig_ldo_write_1p25_vol(void)
{
     reg_ana_addr    = reg_boot_idcode(2);
     reg_ana_len     = 1;
     reg_ana_data(0) = reg_boot_idcode(3);
     while (!(reg_ana_buf_cnt & FLD_ANA_TX_BUFCNT))
         ;
     reg_ana_ctrl = (FLD_ANA_CYC | FLD_ANA_RW | ((0x28 & 0x00000300) >> 8));
     while (reg_ana_ctrl & FLD_ANA_BUSY) {
     }
     reg_ana_ctrl = 0x00;
}

/**
 * @brief       this function serves to set dig ldo 1V0 mode.
 * @return      none
 */
static _attribute_text_sec_optimize_o2_ void pm_set_dig_ldo_vol_1v0mode(void)
{
    //firstly Change frequency
    reg_sram_intf_cfg_0 = SRAM_INTF_CFG_0_1V0;
    reg_sram_intf_cfg_1 = SRAM_INTF_CFG_1_1V0;
    pm_set_dig_ldo_write_1p25_vol();
    pm_set_dig_ldo_write_ana_0x27();
}

/**
 * @brief       this function serves to set dig ldo 1V1 mode.
 * @return      none
 */
static _attribute_text_sec_optimize_o2_ void pm_set_dig_ldo_vol_1v1mode(void)
{
    pm_set_dig_ldo_write_ana_0x27();
    reg_sram_intf_cfg_0 = SRAM_INTF_CFG_0_1V1;
    reg_sram_intf_cfg_1 = SRAM_INTF_CFG_1_1V1;
    //lastly Change frequency
}

/**
 * @brief       this function serves to save the cache register before adjust the dig-ldo voltage.
 * @param[in]   swire      - flag    1: swire off    0:swire on.
 * @return      none
 */
static inline void pm_set_dig_ldo_restore_sws_cfg(unsigned char swire)
{
    if (swire) {
        swire_slave_en();
    }
}

/**
 * @brief       this function serves to save the cache register before adjust the dig-ldo voltage.
 * @param[in]   vol         - select digldo mode.
 * @return      the check result   0: success  1: fail
 */
static inline unsigned char pm_check_dig_ldo_config_correct(pm_dig_vol_mode_e vol)
{
    unsigned char vol_val;
    unsigned char sram_cfg0_val;
    unsigned char sram_cfg1_val;

    vol_val = analog_read_reg8(areg_aon_0x27);

    sram_cfg0_val = read_reg8(SRAM_INTF_CFG_0);
    sram_cfg1_val = read_reg8(SRAM_INTF_CFG_1);

    if (vol == DIG_VOL_1V1_MODE) {
        if ((vol_val != g_pm_voltage_1v1_mode) || (sram_cfg0_val != SRAM_INTF_CFG_0_1V1) || (sram_cfg1_val != SRAM_INTF_CFG_1_1V1)) {
            return 1;
        }
    } else if (vol == DIG_VOL_1V_MODE) {
        if ((vol_val != g_pm_voltage_1v_mode) || (sram_cfg0_val != SRAM_INTF_CFG_0_1V0) || (sram_cfg1_val != SRAM_INTF_CFG_1_1V0))
        {
            return 1;
        }
    }

    return 0;
}


/**
 * @brief       this function serves to save the cache register before adjust the dig-ldo voltage.
 * @return      none
 */
static inline cache_status_t cache_save_and_disable(void)
{
    unsigned int mcache_ctl = read_csr(NDS_MCACHE_CTL);
    cache_status_t status = {0};

    status.dcache = (mcache_ctl & 0x02) != 0;
    status.icache = (mcache_ctl & 0x01) != 0;
    fence_iorw;
    if (status.dcache) clear_csr(NDS_MCACHE_CTL,BIT(1));
    if (status.icache) clear_csr(NDS_MCACHE_CTL,BIT(0));

    return status;
}

/**
 * @brief       this function serves to restore the cache register after adjust the dig-ldo voltage.
 * @param       status - select i-cache or d-cache.
 * @return      none
 */
static inline void pm_set_dig_ldo_restore_cache_cfg(cache_status_t status)
{
    if (status.dcache) set_csr(NDS_MCACHE_CTL,BIT(1));
    if (status.icache) set_csr(NDS_MCACHE_CTL,BIT(0));
}

/**
 * @brief       this function serves to wait all dma chn complete.
 * @param       wait_timeout_us - timeout.
 * @return      DRV_API_TIMEOUT : wait timeout;
 *              DRV_API_SUCCESS : all dma complete status is successful;
 */
static inline drv_api_status_e dma_wait_for_all_chn_to_complete(unsigned int timeout_us)
{
    unsigned char      dma_done_flag = 1;
    unsigned long long start         = rdmcycle();
    while (dma_done_flag) {
        if (core_cclk_time_exceed(start, timeout_us)) {
            return DRV_API_TIMEOUT;
        }
        unsigned char i = 0;
        for (i = 0; i < DMA_CNT; i++) {
            if (dma_chn_is_complete(i)) {
                break;
            }
        }
        if (i == DMA_CNT) {
            dma_done_flag = 0;
        }
    }
    return DRV_API_SUCCESS;
}

/**
 * @brief       This function serves to set dig ldo mode.
 * @param[in]   power_mode  - power mode(LDO/LDO_DCDC, DCDC mode not support)
 * @param[in]   vol         - dig ldo mode.
 * @param[in]   dma_timeout_us - wait dma all chn complete timeout.
 * @return      DRV_API_SUCCESS - successful;
 *              DRV_API_INVALID_PARAM - select the not support mode;
 *              DRV_API_TIMEOUT - wait for dma all chn idle timeout to exit;
 * @note        1.If the voltage goes up, after calling the interface first, then adjust the frequency;
 *                If the voltage goes down, adjust the frequency first, then call the interface;
 *              2.When adjusting this voltage, no access ram operation is allowed, so it will wait for dma idle in this interface,
 *                modifying dma_timeout_us won't work if there are dma chains working all the time, and needs to be turned off by the upper layers themselves depending on the situation.
 *              3.When adjusting this voltage, no access ram operation is allowed, disable swire.
 *              4.If the check configuration fails, reboot.
 */
_attribute_ram_code_sec_noinline_ drv_api_status_e pm_set_dig_ldo(pm_dig_vol_mode_e vol, unsigned int dma_timeout_us)
{
    /* ema mode only support LDO_1P25_LDO_1P8/DCDC_1P25_LDO_1P8 mode.*/
    if (DCDC_1P25_DCDC_1P8 == (g_areg_aon_0a & 0x03)) {
        return DRV_API_INVALID_PARAM;
    }

    /* Disable interrupts globally in the system. */
    unsigned int r = core_interrupt_disable();

    /* waiting for dma to finish. */
    if (dma_wait_for_all_chn_to_complete(dma_timeout_us)) {
        /* interrupt recovery */
        core_restore_interrupt(r);
        return DRV_API_TIMEOUT;
    }

    /* Turn off swire and save */
    unsigned char swire = 0;
    if (swire_slave_is_init()) {
        swire = 1;
        swire_slave_dis();
    }

    /**
     * ========      adjust the dig ldo source vol 1.25/1.35      ========
     *                                 ldo            ldo_dcdc
     *               1V0            0x28<4>=0         0x0c<4>=0
     *               1V1            0x28<4>=1         0x0c<4>=1
     */
    unsigned char reg_addr_1p25 = ((g_areg_aon_0a & 0x03) == LDO_1P25_LDO_1P8) ? 0x28 : ((g_areg_aon_0a & 0x03) == DCDC_1P25_LDO_1P8) ? 0x0c : 0xFF;
    if (reg_addr_1p25 != 0xFF) {
        unsigned char val_1p25 = analog_read_reg8(reg_addr_1p25);

        if (vol == DIG_VOL_1V_MODE) {
            val_1p25 &= (~BIT(4));
            reg_boot_idcode(0) = g_pm_voltage_1v_mode;
            reg_boot_idcode(2) = reg_addr_1p25;
            reg_boot_idcode(3) = val_1p25;
        } else if (vol == DIG_VOL_1V1_MODE) {
            val_1p25 |= BIT(4);
            reg_boot_idcode(0) = g_pm_voltage_1v1_mode;
            analog_write_reg8(reg_addr_1p25, val_1p25);
        }
    }

    /* Turn off cache and save */
    cache_status_t saved_status = cache_save_and_disable();

    DISABLE_BTB;

    /* adjust dig ldo voltage. */
    if(vol == DIG_VOL_1V_MODE){
        pm_set_dig_ldo_vol_1v0mode();
    }else if(vol == DIG_VOL_1V1_MODE){
        pm_set_dig_ldo_vol_1v1mode();
    }

    ENABLE_BTB;

    /* check the data */
    if (pm_check_dig_ldo_config_correct(vol)) {
        sys_reboot();
    }

    /* swire recovery, cache recovery */
    pm_set_dig_ldo_restore_sws_cfg(swire);
    pm_set_dig_ldo_restore_cache_cfg(saved_status);

    /* interrupt recovery */
    core_restore_interrupt(r);

    return DRV_API_SUCCESS;
}
#endif
