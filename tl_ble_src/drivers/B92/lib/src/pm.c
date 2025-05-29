/********************************************************************************************************
 * @file    pm.c
 *
 * @brief   This is the source file for B92
 *
 * @author  Driver Group
 * @date    2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "audio.h"
#include "watchdog.h"

#define PM_DEBUG 0
//the code run time for preparation entering and exiting suspend sleep, 70-101(while waiting time)
#define SUSPEND_CODE_US    498
#define SLEEP_MIN_CODE_US  342 //the minimum code run time before sleep
#define PM_XTAL_ONCE_DEBUG 0

#if (PM_DEBUG)
volatile unsigned char                           debug_pm_info;
unsigned int                                     ana_32k_tick;
volatile unsigned char                           ana_reg64_value[3];
volatile unsigned int                            ana_32k_tick_cur[3];
volatile unsigned int                            ana_32k_tick_set[3];
volatile unsigned char                           wakeup_src_clr_err = 0;
extern _attribute_ram_code_sec_optimize_o2_ void flash_mspi_write_ram(flash_command_e cmd, unsigned long addr, unsigned char *data, unsigned long data_len);
#endif

extern unsigned char rc_24m_power;
extern unsigned char bbpll_power;

_attribute_aligned_(4) pm_status_info_s g_pm_status_info;
unsigned char g_areg_aon_7f = 0;

#define PM_IRQ_STATUS_MAX_CLR_TIMES 3
//system timer clock source is constant 16M, never change
//NOTICE:We think that the external 32k crystal clk is very accurate, does not need to read through TIMER_32K_LAT_CAL.
//register, the conversion error(use 32k:64 cycle, count 24M sys tmr's ticks), at least the introduction of 64ppm.
#define CRYSTAL32768_TICK_PER_64CYCLE 46875

_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_calib;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_cur;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_cur;
_attribute_data_retention_sec_ unsigned char                      g_pm_long_suspend;
_attribute_data_retention_sec_ unsigned char                      g_pm_vbat_v;
_attribute_data_retention_sec_ unsigned char                      g_pm_tick_update_en             = 1;
_attribute_data_retention_sec_ static unsigned char               g_pm_suspend_power_cfg          = 0x87;
_attribute_data_retention_sec_ unsigned char                      g_pm_pad_filter_en              = 0x20; //BLE SDK use: no static
_attribute_data_retention_sec_ unsigned int                       g_pm_mspi_cfg                   = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_calib_repair      = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_xtal_stable_suspend_nopnum = 250;
_attribute_data_retention_sec_ unsigned int                       g_pm_xtal_stable_loopnum        = 10;
_attribute_data_retention_sec_ unsigned int                       g_pm_suspend_delay_us           = 200;
_attribute_data_retention_sec_ unsigned int                       g_pm_interrupt_status           = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_interrupt_status1          = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_interrupt_status2          = 0;
_attribute_data_retention_sec_ volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us       = {
          .suspend_early_wakeup_time_us  = 188 + 109 + 200 + SUSPEND_CODE_US,         //188(r_delay) + 109(3.5*(1/32k)) + 200(XTAL_delay) + SUSPEND_CODE_US(code)
          .deep_ret_early_wakeup_time_us = 188 + 109,                                 //188(r_delay) + 109(3.5*(1/32k))
          .deep_early_wakeup_time_us     = 688 + 109 + 380,                           //688(r_delay) + 109(3.5*(1/32k)) + 380(boot_rom)
          .sleep_min_time_us             = 688 + 109 + 380 + SLEEP_MIN_CODE_US + 200, //688 + 109 + 380(the maximum value of suspend and deep)
                                                                                //+ SLEEP_MIN_CODE_US(code run time before sleep) + 200(margin)
};
_attribute_data_retention_sec_ volatile pm_r_delay_cycle_s g_pm_r_delay_cycle = {
    .deep_r_delay_cycle           = 3 + 8,                                            // 11 * (1/16k) = 687.5
    .suspend_ret_r_delay_cycle    = 3,                                                // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
    .deep_xtal_delay_cycle        = 3 + 8,                                            // 11 * (1/16k) = 687.5
    .suspend_ret_xtal_delay_cycle = 3,                                                // 2 * 1/16k = 125 uS, 3 * 1/16k = 187.5 uS  4*1/16k = 250 uS
};

extern _attribute_ram_code_sec_optimize_o2_ void flash_send_cmd(unsigned char cmd);

#define reg_mspi_cipher_ctrl REG_ADDR8(0x23FFFF00 + 0x85)
#define reg_mspi_cipher_key  REG_ADDR32(0x23FFFF00 + 0x80)
#define reg_mspi_data_nonce  REG_ADDR8(0x23FFFF00 + 0x84)
#define reg_debug_key(i)     REG_ADDR32(0x140040 + i * (4))


_attribute_data_retention_sec_ static unsigned char pm_retention_register_storage_data0    = 0x00;
_attribute_data_retention_sec_ static unsigned char pm_retention_register_storage_buff1[5] = {0x00}; //[0:3]->reg_mspi_cipher_key;[4]->reg_mspi_data_nonce
_attribute_data_retention_sec_ static unsigned int  pm_retention_register_storage_buff2[4] = {0x00}; //[0]->reg_debug_key(0) ...

/**
 * @brief       after retention, digital register will be lost, save some register values that need to be used.
 *              in fact, the function of the interface is to save cipher_ctrl and flash/debug encryption keys, which are indirectly named pm_retention_register_save for security purposes.
 * @param[in]   none.
 * @return      none.
 */
_attribute_ram_code_sec_optimize_o2_ void pm_retention_register_save(void)
{
    //flash cipher encryption keys
    *((unsigned int *)pm_retention_register_storage_buff1) = reg_mspi_cipher_key;
    pm_retention_register_storage_buff1[4]                 = reg_mspi_data_nonce;
    //debug cipher encryption keys
    for (unsigned char i = 0; i < 4; i++) {
        pm_retention_register_storage_buff2[i] = reg_debug_key(i);
    }
    //cipher_ctrl
    pm_retention_register_storage_data0 = reg_mspi_cipher_ctrl;
}

/**
 * @brief       after retention, digital register will be lost, restore some register values that need to be used.
 *              in fact, the function of the interface is to recover cipher_ctrl and flash/debug encryption keys, which are indirectly named pm_retention_register_recover for security purposes.
 * @param[in]   none.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_retention_register_recover(void)
{
    //flash cipher encryption keys
    reg_mspi_cipher_key = *((unsigned int *)pm_retention_register_storage_buff1);
    reg_mspi_data_nonce = pm_retention_register_storage_buff1[4];
    //debug cipher encryption keys
    for (unsigned char i = 0; i < 4; i++) {
        reg_debug_key(i) = pm_retention_register_storage_buff2[i];
    }
    //cipher_ctrl
    reg_mspi_cipher_ctrl = pm_retention_register_storage_data0;

    //clear storage_buff
    pm_retention_register_storage_data0                    = 0;
    *((unsigned int *)pm_retention_register_storage_buff1) = 0;
    pm_retention_register_storage_buff1[4]                 = 0;
    for (unsigned char i = 0; i < 4; i++) {
        pm_retention_register_storage_buff2[i] = 0;
    }
}

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
    g_pm_early_wakeup_time_us.deep_early_wakeup_time_us     = deep_r_delay_us + 109 + 380;
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
    g_pm_xtal_stable_loopnum        = loopnum;
    g_pm_suspend_delay_us           = delay_us;

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
    REG_ADDR8(0x140218) = 0x02; //sys tick 16M set upon next 32k posedge
    reg_system_ctrl |= (FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);
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
    reg_system_tick = g_pm_tick_cur + 1;
    //wait cmd set dly done upon next 32k posedge
    //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
    while ((reg_system_st & BIT(7)) == 0);                    //system timer set done status upon next 32k posedge
    REG_ADDR8(0x140218) = 0; //normal sys tick (16/sys) update
#else
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
    reg_system_tick = g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US;
    reg_system_ctrl |= (FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN); //enable 32k cal and system timer
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
    //        PA[7:0]           PB[7:0]         PC[7:0]         PD[7:0]     PE[7:0]         PF[7:0]
    // pol: ana_0x3f<7:0>    ana_0x40<7:0>  ana_0x41<7:0>  ana_0x42<7:0>  ana_0x43<7:0>  ana_0x44<7:0>
    // en:  ana_0x45<7:0>    ana_0x46<7:0>  ana_0x47<7:0>  ana_0x48<7:0>  ana_0x49<7:0>  ana_0x4a<7:0>
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
 * @brief       this function servers to wait bbpll clock lock.
 * @return      none.
 */
_attribute_ram_code_sec_optimize_o2_ void pm_wait_bbpll_done(void)
{
    unsigned char ana_81 = analog_read_reg8(0x81);
    analog_write_reg8(0x81, ana_81 | BIT(6));
    unsigned char j = 0;
    for (; j < 3; j++) {
        core_cclk_delay_tick(sys_clk.cclk * 20); //This delay time is about 20.38us under the calibrated 24M RC clock.
        if (BIT(5) == (analog_read_reg8(0x88) & BIT(5))) {
            analog_write_reg8(0x81, ana_81 & 0xbf);
            break;
        } else {
            if (j == 0) {
                //The high temperature test found that setting the bbpll ldo in the default voltage gear will cause the chip to crash.
                //After debug, it was found that when the bbpll ldo is switched, the clock will not be locked under certain voltages.
                //Therefore, what is done here Detect whether the bbpll clock is locked during processing,and if not, switch to a voltage gear.
                //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
                analog_write_reg8(0x01, 0x46);
            } else if (j == 1) {
                analog_write_reg8(0x01, 0x43);
            } else {
                //The reason why the PLL is stable does not require a reboot:
                //If there is a problem with the PLL, even if it is restarted, the PLL will not be good.
                //This is different from the crystal oscillator. After rebooting, the crystal oscillator still has a chance to oscillate.
                //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
                analog_write_reg8(0x81, ana_81 & 0xbf);
            }
        }
    }
    if (j == 3) {
        drv_timeout_handler(DRV_API_ERROR_TIMEOUT_PLL_DONE);
    }
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
    for (j = 0; j < g_pm_xtal_stable_loopnum; j++) {
        t0 = stimer_get_tick();
        //Note: Ensure that the current clock is 24M RC, otherwise the nop running time will not be 40us, resulting in a later calculation error and system reboot.
        //(add by weihua.zhang, confirmed by peng.sun 20230609)
        core_cclk_delay_tick(sys_clk.cclk * 40);
        if (stimer_get_tick() - t0 > SYSTEM_TIMER_TICK_1US * 20) {
            break;
        }
    }

#if PM_XTAL_ONCE_DEBUG
    if (j > 0) {
        while (1);
    }
#endif
    if (j == g_pm_xtal_stable_loopnum) {
        //Use PM_ANA_REG_POWER_ON_CLR_BUF0[bit1] to check whether there has been a reset caused by the instability of the crystal oscillator.
        //If it is 1, it has occurred.
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | 0x02);
#if (PM_DEBUG)
        while (1);
#endif
        if (all_ramcode_en == 0x00) {
            sys_reboot();
        } else {
            write_reg8(0x1401ef, 0x20);
            while (1);
        }
    }
    reg_system_ctrl &= (~FLD_SYSTEM_TIMER_EN);
}

/**
 * @brief       This function serves to update wakeup status.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_update_status_info(unsigned char clr_en) //BLE SDK use: to put into RAM
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
_attribute_ram_code_sec_optimize_o2_ unsigned char pm_clr_all_irq_status(void)
{
    unsigned char j, ana_reg64 = 0xff;
    for (j = 0; j < PM_IRQ_STATUS_MAX_CLR_TIMES; j++) {
        analog_write_reg8(0x64, 0xff);
        ana_reg64 = pm_get_wakeup_src() & 0x7f; //clear bit7(watch_dog), it is not a src of wakeup.
        if (ana_reg64 == 00) {
            break;
        } else {
#if (PM_DEBUG)
            wakeup_src_clr_err  = 1;
            ana_reg64_value[j]  = ana_reg64;
            ana_32k_tick_cur[j] = analog_read_reg32(0x60);
            ana_32k_tick_set[j] = analog_read_reg32(0x65);
            /******************************************debug_pm_info**********************************************/
            debug_pm_info = 0x21;
#endif
        }
    }
    if (j == PM_IRQ_STATUS_MAX_CLR_TIMES) {
        return 0;
    } else {
        return 1;
    }
}

/**
 * @brief       this function serves to start sleep mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @return      none.
 */
_attribute_ram_code_sec_optimize_o2_ void  pm_sleep_start(pm_sleep_mode_e sleep_mode) //BLE SDK use: no static
{
    audio_power_down();
    analog_write_reg8(0x7d, g_pm_suspend_power_cfg);
    flash_send_cmd(0xb9);
    write_reg8(0x140331, 0x00); //MSPI ie disable
    //This is 1.2V and 2.0V power supply during sleep. Do not power on during initialization, because after power on,
    //there will be two power supplies at the same time, which may cause abnormalities.add by weihua.zhang, confirmed by haitao 20210107
    analog_write_reg8(0x0b, analog_read_reg8(0x0b) & ~(BIT(0) | BIT(1))); //<0>:pd_nvt_1p2,   power on native 1P2 dcdc.
                                                                          //<1>:pd_nvt_2p0,   power on native 2P0 dcdc.
    analog_write_reg8(0x02, analog_read_reg8(0x02) | BIT(3));             //<3>:LDO_flash_en_bypass, LDO_flash's bypass mode enable
    analog_write_reg8(0x81, analog_read_reg8(0x81) | BIT(7));             //<7>:vco_pd,   power down signal of vco

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
    unsigned int  clr_plic_request_result = 0;
    unsigned char clr_pm_irq_result       = pm_clr_all_irq_status();
    if (clr_pm_irq_result == 1) {
        clr_plic_request_result = plic_clr_all_request();
    }

    //If the clearing fails, it indicates that the wake source is active. In this case, the deep mode will reboot,
    //and the other modes continue to run downward.(add by weihua.zhang, 20230907)
    if ((clr_pm_irq_result == 0) || (clr_plic_request_result == 0)) {
#if (PM_DEBUG)
        while (1);
#endif
        if (sleep_mode == DEEPSLEEP_MODE) {
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | 0x04);
            write_reg8(0x1401ef, 0x20); //Clear ana_0x64 for three times. If the status is not completely cleared for three times, reboot the mcu.
            while (1);
        }
    } else {
        //0x80 is to enter low power mode immediately. 0x81 is to wait for D25F to enter wfi mode before entering low power,this way is more secure.
        //Once in the WFI mode, memory transactions that are started before the execution of WFI are guaranteed to have been completed,
        //all transient states of memory handling are flushed and no new memory accesses will take place.
        //only suspend requires this process, after waking up to resume the scene.
        //(add by bingyu.li, confirmed by jianzhi.chen 20230810)
        write_reg8(0x1401ef, 0x81); //stall mcu trig
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
    pm_clr_all_irq_status();                                         //clear all flag
    plic_clr_all_request();

    analog_write_reg8(0x06, (analog_read_reg8(0x06) & 0xfe) | 0x40); //<0>:pd_bbpll_ldo, default:1,->0 Power up bbpll LDO.
                                                                     //<6>:spd_ldo_pd,   default:0,->1 Power down spd ldo.
    analog_write_reg8(0x81, analog_read_reg8(0x81) & (~BIT(7)));     //<7>:vco_pd,   power on signal of vco

    //In order to avoid the hidden danger of two voltages supplying power at the same time,
    //the operation of turning off the native voltage should be placed in the first place as far as possible.
    //(added by weihua.zhang,confired by haitao 20231123)
    analog_write_reg8(0x02, analog_read_reg8(0x02) & (~BIT(3)));         //<3>:LDO_flash_en_bypass, LDO_flash's bypass mode disable
    analog_write_reg8(0x0b, analog_read_reg8(0x0b) | (BIT(0) | BIT(1))); //<0>:pd_nvt_1p2,   power down native 1P2 dcdc.
                                                                         //<1>:pd_nvt_2p0,   power down native 2P0 dcdc.

    //must to set xo_quick_settle with manual and wait it stable.(added by bingyu.li,confired by wenfeng 20231123)
    crystal_manual_settle();

    analog_write_reg8(0x7d, 0x84); //<0>:pg_zb_en,     default:1,->0 power on baseband.
                                   //<1>:pg_usb_en,    default:1,->0 power on usb.
                                   //<2>:pg_audio_en,  default:1,    can not power on here

    //Before sleeping,the MSPI has already been switched to 24M RC, so there is no need to wait for Xtal and PLL to stabilize.
    //Advance the flash wakeup to before the delay, because after the flash wakeup, It will take some time to restore to the active working state.
    //(usually set to 150us - a margin is left for different flash models)(add by bingyu.li, confirmed by kaixin.chen 20230616)
    //The flash two-wire system uses clk+cn+ two communication lines, and the flash four-wire system uses
    //clk+cn+ four communication lines. Before suspend sleep, the input of the six lines (PF0-PF5) used
    //by flash will be disabled. After suspend wakes up, the six lines will be set to input function.
    //(changed by weihua.zhang, confirmed by jianzhi 20201201)
    write_reg8(0x140331, 0x3f); //MSPI(PF0-PF5) ie enable
    flash_send_cmd(0xab);       //flash wakeup

    //When g_pm_xtal_stable_suspend_nopnum =250,This for loop is about 190us under the calibrated 24M RC clock.
    //wait for xtal stable and flash restore to the active working state.
    for (volatile unsigned int i = 0; i < g_pm_xtal_stable_suspend_nopnum; i++) {
        _ASM_NOP_;
    }

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
_attribute_ram_code_sec_optimize_o2_ void pm_switch_ext32kpad_to_int32krc(void) //BLE SDK use: no static inline 
{
    //switch 32k clk src: select internal 32k rc, if not do this, when deep+pad wakeup: there's no stable 32k clk(therefore, the pad wake-up time
    //is a bit long, the need for external 32k crystal vibration time) to count DCDC dly and XTAL dly. High temperatures even make it impossible
    //to vibrate, as the code for PWM excitation crystals has not yet been effectively executed. SO, we can switch 32k clk to the internal 32k rc.
    analog_write_reg8(0x4e, analog_read_reg8(0x4e) & (~BIT(7))); //32k select:[7]:0 sel 32k rc,1:32k pad
    analog_write_reg8(0x05, 0x02);                               //[0]:32k rc;[1]:32k xtal (1->power down,0->power up). Power up 32k rc.

    //deep + no tmr wakeup(we need  32k clk to count dcdc dly and xtal dly, but this case, ext 32k clk need close, here we use 32k rc instead)
    analog_write_reg8(0x4c, 0xef);
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
 * @return      indicate whether the cpu is wake up successful.
 */
_attribute_ram_code_sec_optimize_o2_ int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode, pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int wakeup_tick)
{
    /**
     *  ==============================          -Os compilation compatibility processing        ==============================
     *
     * The pm_sleep_wakeup interface will stop xip, so this function can not have a text section of code called, workaround: (1) for very short functions add _always_inline (2) for large functions, specify ram code.
     *  Because we have been using O2 to calculate the compensation time, and considering that if the compiler optimization level is not the same, it will affect the code logic, resulting in incorrect compensation value in the pm_sleep_wakeup function.
     *  To sum up, we add _attribute_ram_code_sec_optimize_o2_
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
            return pm_get_wakeup_src() & 0x7f;
        } else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US) {
            unsigned int t = stimer_get_tick();
            analog_write_reg8(0x64, 0xff);
            unsigned char st;
            do {
                st = pm_get_wakeup_src() & 0x7f;
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
        while (!read_reg32(0x140214)); //Wait for the 32k clock calibration to complete.

        //Note:
        //A2 chip:The time accuracy of suspend, deep and deepret sleep is about 2000ppm, the test 32k rc clock accuracy is also about 2000ppm.Ya.yang thinks it is due to interference from the pll ldo and 24M rc.
        //Considering that the 32krc clock will become inaccurate after entering sleep, thus affecting the accuracy of sleep time, it is necessary to compensate 32krc before entering sleep.
        //Discuss compensation methods with designer ya.yang as follows.According to the results of ATE big data test, the deviation of 32k rc clock is obtained, the interference of 24m rc
        //and pll is compensated, and the compensated value is written into the 32k calibration value.
        //A3 chip 32k rc clock is accurate.
        //modify by bingyu.li, confirmed by ya.yang at 20230707.
        if ((bbpll_power == 0) && (rc_24m_power == 0)) {
            g_pm_tick_32k_calib_repair = 0x00000019;
        } else if ((bbpll_power == 1) && (rc_24m_power == 0)) {
            g_pm_tick_32k_calib_repair = 0x0000000d;
        } else if ((bbpll_power == 0) && (rc_24m_power == 1)) {
            g_pm_tick_32k_calib_repair = 0x0000000e;
        }

        if (g_chip_version == 0x11) {
            g_pm_tick_32k_calib = read_reg32(0x140214) - g_pm_tick_32k_calib_repair;
        } else {
            g_pm_tick_32k_calib = read_reg32(0x140214);
        }

#if (PM_DEBUG)
        analog_write_reg16(PM_ANA_REG_POWER_ON_CLR_BUF1, g_pm_tick_32k_calib);
        /******************************************debug_pm_info 2 **********************************************/
        debug_pm_info = 2;
#endif
    }
    unsigned int tick_32k_halfCalib = g_pm_tick_32k_calib >> 1;

    ///////////////////    change clock    /////////////////////////////////
    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    mspi_stop_xip();
    unsigned char cclk_reg = read_reg8(0x1401e8);
    write_reg8(0x1401e8, cclk_reg & 0x8f);             //change cclk to 24M rc clock
    unsigned char div_reg = read_reg8(0x1401d8);
    write_reg8(0x1401d8, div_reg & 0xf8);              //change clock division to 1:1:1
    unsigned char mspiclk_reg = read_reg8(0x1401c0);
    write_reg8(0x1401c0, (mspiclk_reg & 0x80) | 0x01); //change mspiclk to 24M rc clock

#if (PM_DEBUG)
    /******************************************debug_pm_info 3 **********************************************/
    debug_pm_info = 3;
#endif

    /////////////////// stop system timer /////////////////////////////////
#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x01;                       //system tick only update upon 32k posedge, must set before enable 32k read update!!!
    BM_CLR(reg_system_ctrl, FLD_SYSTEM_32K_TRACK_EN); //disable 32k track
    g_pm_tick_32k_cur = clock_get_32k_tick();
    g_pm_tick_cur     = stimer_get_tick();
    BM_SET(reg_system_st, FLD_SYSTEM_CMD_STOP);       //write 1, stop system timer when using auto mode
    REG_ADDR8(0x140218) = 0x00;
#else
    #error-- Manual mode is only for internal testing, and 37 in the code may not be accurate
    g_pm_tick_cur = stimer_get_tick() + 37 * SYSTEM_TIMER_TICK_1US;                                 //cpu_get_32k_tick will cost 30~40 us, stimer = 24M
    BM_CLR(reg_system_ctrl, FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN); //disable 32k track and stimer
    g_pm_tick_32k_cur = clock_get_32k_tick();
#endif

#if (PM_DEBUG)
    analog_write_reg32(PM_ANA_REG_WD_CLR_BUF1, g_pm_tick_32k_cur);
    /******************************************debug_pm_info 4 **********************************************/
    debug_pm_info = 4;
#endif

    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8(0x4b, wakeup_src);
    analog_write_reg8(0x64, 0xff);             //clear all flag
    analog_write_reg8(0x7e, sleep_mode);       //sram retention

    unsigned int earlyWakeup_us;
    if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) //deep sleep with retention
    {
        pm_retention_register_save();          //after retention, cipher_ctrl and flash/debug cipher encryption keys is lost, save it
        //0x4d:
        //1:auto power down:<0>lpc <1>dcore/sram LDO <2>UVLO ib <3>vbus switch <4>flash LDO
        //<6>power down sequence enable <7>enable isolation
        if ( (wakeup_src & PM_WAKEUP_COMPARATOR) || (wakeup_src & PM_WAKEUP_CTB)) {
            analog_write_reg8(0x4d, 0xfe);                               //retention
        } else {
            analog_write_reg8(0x4d, 0xff);                               //retention
        }
        analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0));        //<7-5>:ldo_main_trim,  default:100,->111 digital LDO output voltage trim: 1.15V
        analog_write_reg8(0x09, (analog_read_reg8(0x09) & 0x3f));        //<6>:pd_sw_dcore,  default:1,->0 power up the main dig ldo to dcore.
                                                                         //<7>:pd_sw_sram,   default:1,->0 power up the main dig ldo to sram.
        analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xf0) & 0x7f); //<4>:pd_ldo_dcore, default:0,->1 Power down of digital core ldo.
                                                                         //<5>:pd_ldo_sram,  default:0,->1 Power down of sram ldo.
                                                                         //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
                                                                         //<7>:dig_ret_pd,   default:1,->0 Power on retention ldo.
        //0xA3FFFF20 mspi_set_l: multiboot address offset option, 0:0k;  1:128k;  2:256k;  4:512k
        //0xA3FFFF21 mspi_set_h: program space size = (mspi_set_h+1)*4k
        g_pm_mspi_cfg = read_reg32(0x23FFFF20);

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
        if ( (wakeup_src & PM_WAKEUP_COMPARATOR) || (wakeup_src & PM_WAKEUP_CTB)) {
            analog_write_reg8(0x4d, 0xfe);                        //deep
        } else {
            analog_write_reg8(0x4d, 0xff);                        //deep
        }
        analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0)); //<7-5>:ldo_main_trim,  default:100,->111 digital LDO output voltage trim: 1.15V
        analog_write_reg8(0x09, (analog_read_reg8(0x09) & 0x3f)); //<6>:pd_sw_dcore,  default:1,->0 power up the main dig ldo to dcore.
                                                                  //<7>:pd_sw_sram,   default:1,->0 power up the main dig ldo to sram.
        analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xf0)); //<4>:pd_ldo_dcore, default:0,->1 Power down of digital core ldo.
                                                                  //<5>:pd_ldo_sram,  default:0,->1 Power down of sram ldo.
                                                                  //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
                                                                  //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;

#if (PM_DEBUG)
        /******************************************debug_pm_info 6 **********************************************/
        debug_pm_info = 6;
#endif

    } else { //suspend
        //0x4d:
        //1:auto power down:<0>lpc <1>dcore/sram LDO <2>UVLO ib <3>vbus switch <4>flash LDO
        //<6>power down sequence enable <7>enable isolation
        if ( (wakeup_src & PM_WAKEUP_COMPARATOR) || (wakeup_src & PM_WAKEUP_CTB)) {
            analog_write_reg8(0x4d, 0x7e);                               //suspend
        } else {
            analog_write_reg8(0x4d, 0x7f);                               //suspend
        }
        analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0));        //<7-5>:ldo_main_trim,  default:100,->111 digital LDO output voltage trim: 1.15V
        analog_write_reg8(0x09, (analog_read_reg8(0x09) & 0x3f));        //<6>:pd_sw_dcore,  default:1,->0 power up the main dig ldo to dcore.
                                                                         //<7>:pd_sw_sram,   default:1,->0 power up the main dig ldo to sram.
        analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xf0) & 0xbf); //<4>:pd_ldo_dcore, default:0,->1 Power down of digital core ldo.
                                                                         //<5>:pd_ldo_sram,  default:0,->1 Power down of sram ldo.
                                                                         //<6>:spd_ldo_pd,   default:1,->0 Power up spd ldo.
                                                                         //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
    }

    /////////////////// auto power down /////////////////////////////////
    //0x4c:<0>32KRC <1>32K xtal <2>4M rcosc <3>24M xtal <4>logic <5>dcdc <6>vbus LDO <7>ana/BBPLL/temp_sensor LDO
    if (CLK_32K_RC == g_clk_32k_src) {
        if (((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_CTB) || (wakeup_src & PM_WAKEUP_COMPARATOR)) {
            analog_write_reg8(0x4c, 0xee); //disable auto power down 32KRC
        } else {
            analog_write_reg8(0x4c, 0xef); //enable auto power down 32KRC
        }

#if (PM_DEBUG)
        /******************************************debug_pm_info 7 **********************************************/
        debug_pm_info = 7;
#endif

    } else {
        if (sleep_mode == DEEPSLEEP_MODE && !timer_wakeup_enable) //if deep mode and no timer wakeup
        {
            pm_switch_ext32kpad_to_int32krc();
        }
        //suspend mode or deep retention mode or timer wakeup source.
        //(we don't power down external 32k pad clock, even though three's no timer wakeup source in suspend or deep retention mode)
        else {
            analog_write_reg8(0x4c, 0xed); //if use timer wakeup, auto pad 32k power down should be disabled
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
                g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_64CYCLE;
                if (tick_wakeup_reset > 0x03ff0000) // 24M: 2.79S
                {
                    tick_reset        = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 64;
                    g_pm_long_suspend = 1;
                } else {
                    tick_reset        = g_pm_tick_32k_cur + (tick_wakeup_reset * 64 + (CRYSTAL32768_TICK_PER_64CYCLE >> 1)) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
            }
        } else {
            tick_reset = g_pm_tick_32k_cur + wakeup_tick - (earlyWakeup_us * 4 / 125); // 32k clk: /31.25
        }
        clock_set_32k_tick(tick_reset);

#if (PM_DEBUG)
        ana_32k_tick = analog_read_reg32(0x65);
        if (tick_reset != ana_32k_tick) {
            reg_system_ctrl |= FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN;
            //          flash_write_page(0x10000, 4, (unsigned char *)&ana_32k_tick);
            //          flash_write(0x10000, 4, (unsigned char *)&ana_32k_tick, FLASH_WRITE_CMD);
            flash_mspi_write_ram(FLASH_WRITE_CMD, 0x10000, (unsigned char *)&ana_32k_tick, 4);
            gpio_set_high_level(GPIO_PE7);
            while (1);
        }
        /******************************************debug_pm_info 9 **********************************************/
        debug_pm_info = 9;
#endif
    }

    if (pm_get_wakeup_src() & 0x1f) {
    } else {
        if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) {
            g_areg_aon_7f = (g_areg_aon_7f & 0xfe) | g_pm_pad_filter_en;
        } else {
            g_areg_aon_7f = (g_areg_aon_7f | 0x01 | g_pm_pad_filter_en);
        }
        analog_write_reg8(0x7f, g_areg_aon_7f);

        pm_sleep_start(sleep_mode);

#if (PM_DEBUG)
        /******************************************debug_pm_info 10 **********************************************/
        debug_pm_info = 10;
#endif
    }

    if (sleep_mode == DEEPSLEEP_MODE) {
        write_reg8(0x1401ef, 0x20);                                  //reboot
    }

    analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xc0) & 0xcf); //<4>:pd_ldo_dcore, default:1,->0 Power up of digital core ldo.
                                                                     //<5>:pd_ldo_sram,  default:1,->0 Power up of sram ldo.
                                                                     //<6>:spd_ldo_pd,   default:0,->1 Power down spd ldo.
                                                                     //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
    analog_write_reg8(0x09, (analog_read_reg8(0x09) | 0xc0));        //<6>:pd_sw_dcore,  default:0,->1 power down the main dig ldo to dcore.
                                                                     //<7>:pd_sw_sram,   default:0,->1 power down the main dig ldo to sram.
    analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0) & 0x9f); //<7-5>:ldo_main_trim,  default:111,->100 digital LDO output voltage trim: 1.0V

#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x02;                                      //sys tick 16M set upon next 32k posedge
    reg_system_ctrl |= (FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);
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
    reg_system_tick = g_pm_tick_cur + 1; // current clock

    #if (PM_DEBUG)
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
    #endif
    //wait cmd set dly done upon next 32k posedge
    //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
    while ((reg_system_st & BIT(7)) == 0);                    //system timer set done status upon next 32k posedge
    REG_ADDR8(0x140218) = 0; //normal sys tick (16/sys) update
#else
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

    reg_system_tick = g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US;
    reg_system_ctrl |= (FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN); //enable 32k cal and stimer
#endif

#if (PM_DEBUG)
    /******************************************debug_pm_info 12 **********************************************/
    debug_pm_info = 12;
#endif

    write_reg8(0x1401d8, div_reg);     //restore div
    write_reg8(0x1401e8, cclk_reg);    //restore cclk
    write_reg8(0x1401c0, mspiclk_reg); //restore mspiclk
    mspi_set_xip_en();

#if (PM_DEBUG)
    /******************************************debug_pm_info 13 **********************************************/
    debug_pm_info = 13;
#endif

    if ((g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable) //wakeup from timer only
    {
        if (wakeup_tick_type == PM_TICK_STIMER) {
            while ((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30));
        } else {
            while ((unsigned int)(clock_get_32k_tick() - wakeup_tick - g_pm_tick_32k_cur + 1) > BIT(30));
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
void pm_set_suspend_power_cfg(pm_suspend_power_cfg_e value, unsigned char on_off)
{
    if (0 == on_off) {
        g_pm_suspend_power_cfg |= (value);
    } else {
        g_pm_suspend_power_cfg &= ~(value);
    }
}

/**
 * @brief       This function servers to set 3V3 LDO output voltage in active mode.
 * @param[in]   voltage - vddo3 setting gear, can be set from 0 to 7.
 * @return      none.
 */
void pm_set_active_vddo3(pm_vddo3_voltage_e voltage)
{
    //There are three LDOs connected together inside the chip, which affect the output voltage of VDDO3.
    //Under different operating modes of the chip, different LDOs will be turned on, as follows: LDO for Active, LCLDO for SUSPEND and AOLDO for DEEP/DEEP_RET.
    //Among them, LCLDO and AOLDO have weak driving capabilities, while LDO has strong driving capabilities.
    //Currently, users of this interface only care about the voltage of VDDO3 in active mode so the interface only provides parameters for modifying LDO.
    analog_write_reg8(0x19, (analog_read_reg8(0x19) & 0x8f) | (voltage << 4));
}

/**
 * @brief       When an error occurs, such as the crystal does not vibrate properly, the corresponding recording and reset operations are performed.
 * @param[in]   reboot_reason  - The bit to be configured in the power on buffer.
 * @param[in]   all_ramcode_en  - Whether all processing in this function is required to be ram code.
 * @return      none.
 */
_attribute_ram_code_sec_optimize_o2_noinline_ void pm_sys_reboot_with_reason(pm_sw_reboot_reason_e reboot_reason, unsigned char all_ramcode_en)
{
    pm_set_reboot_reason(reboot_reason);

#if (PM_DEBUG)
    while (1);
#endif

    if (all_ramcode_en == 0x00) {
        DISABLE_BTB;
        sys_reboot_ram();
        ENABLE_BTB;
    } else {
        reg_pwdn_en = 0x20;
        while (1);
    }
}
