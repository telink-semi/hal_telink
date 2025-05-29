/********************************************************************************************************
 * @file    pm.c
 *
 * @brief   This is the source file for TL321X
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
#include "watchdog.h"

/********************************************************************************************************
 *   The time a new chip needs to be tested and the time required to test when changing the code.
 *******************************************************************************************************/
//the code run time for preparation entering and exiting suspend sleep, 100-131(while waiting time).
//Open the macro definition PM_SUSPEND_WHILE_DEBUG and test this using TEST_SLEEP_TIME_ACCURACY in the test demo.
#define SUSPEND_CODE_US 354 //the code run time for preparation entering and exiting suspend sleep

//the minimum code run time before sleep
//Open the macro definition PM_MIN_CODE_DEBUG and test this using TEST_SLEEP_TIME_ACCURACY in the test demo.
#define SLEEP_MIN_CODE_US 95 //the minimum code run time before sleep

/********************************************************************************************************
 *                         The time required to test on a new chip.
 *******************************************************************************************************/
//Time before 32k tick value in sleep function.
//Open the macro definition PM_START_CODE_DEBUG and test this using TEST_SLEEP_TIME_ACCURACY in the test demo.
//In order to make this time compatible with more clocks, compatible with the most extreme cases, you need to do the following:
//1. Use a calibrated 24M rc clock.
//2. Call the PM function immediately after system initialization.
#define SLEEP_START_CODE_US 60 //30-60

/********************************************************************************************************
 *                         The time required to confirm on the new chip.
 *******************************************************************************************************/
//r_delay: 2*1/16k=125uS  3*1/16k=187.5uS  4*1/16k=250uS  11*1/16k=687.5uS
//The 16k clock is derived from the 32k clock frequency division.
//The 32k rc frequency is 32000. 32k pad frequency is 32768. Each tick varies by 0.73us, so 32k pad can use 32k rc data.
#define SUSPEND_RET_R_DELAY_US 188 //suspend or deep retention r_delay(us)
//deep r_delay(us). The minimum value can be the same as deep retention, because deep has a current pulse when it wakes up, so the value is set higher.
#define DEEP_R_DELAY_US       688

#define SUSPEND_XTAL_DELAY_US 200 //waits for XTAL to stabilize after suspend waking up.
#define BOOT_ROM_US           390 //BOOT ROM

#define HARDWARE_DELAY_US     109 //3.5*(1/32k), suspend/deep/deep retention hardware delay.

/********************************************************************************************************
 *                                 The usual fixed time.
 *******************************************************************************************************/
#define SLEEP_MIN_MARGIN_US 400 //400 means more margin, >32 is enough.

#if (PM_DEBUG)
volatile unsigned char      debug_pm_info;
volatile unsigned int       debug_ana_32k_tick;
volatile unsigned int       debug_sleep_32k_cur;
volatile unsigned int       debug_ana_tick_reset;
volatile unsigned int       debug_tick_32k_cur;
volatile unsigned char      debug_min_wakeup_src          = 0;
volatile unsigned char      debug_sleep_start_wakeup_src0 = 0;
volatile unsigned char      debug_sleep_start_wakeup_src1 = 0;
volatile unsigned char      debug_sleep_start_wakeup_src2 = 0;
volatile unsigned int       debug_min_stimer_tick         = 0;
volatile unsigned int       debug_sleep_start_cur_tick    = 0;
volatile unsigned int       debug_sleep_start_set_tick    = 0;
volatile unsigned int       debug_sleep_wakeup_return     = 0;
volatile unsigned long long debug_while_7d_tick_1;
volatile unsigned long long debug_while_7d_tick_2;
volatile unsigned long long debug_while_7d_tick_3;
volatile unsigned long long debug_min_code_tick_1;
volatile unsigned long long debug_min_code_tick_2;
volatile unsigned long long debug_min_code_tick_3;
volatile unsigned char      debug_ana_reg[128];
#endif

extern _attribute_data_retention_sec_ unsigned int g_pm_xtal_stable_loopnum;

_attribute_aligned_(4) pm_status_info_s g_pm_status_info;
volatile unsigned char g_pm_system_reboot_event = 0;
unsigned char          g_areg_aon_7f            = 0;

_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_calib;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_cur;
_attribute_data_retention_sec_ unsigned int                       g_pm_tick_32k_cur;
_attribute_data_retention_sec_ unsigned char                      g_pm_long_suspend;
_attribute_data_retention_sec_ unsigned char                      g_areg_aon_0a              = 0;
/**< BLE USED *****/
_attribute_data_retention_sec_ unsigned char                      g_pm_suspend_power_cfg=(FLD_PD_ZB_EN | FLD_PD_USB_EN | FLD_PD_AUDIO_EN);
_attribute_data_retention_sec_ unsigned char                      g_pm_pad_filter_en=0x00;
/**< BLE USED END */
_attribute_data_retention_sec_ unsigned int                       g_pm_multi_addr            = 0;
_attribute_data_retention_sec_ unsigned int                       g_pm_suspend_code_us       = SUSPEND_CODE_US;
_attribute_data_retention_sec_ unsigned int                       g_pm_min_code_us           = SLEEP_MIN_CODE_US;
_attribute_data_retention_sec_ unsigned int                       g_pm_suspend_xtal_delay_us = SUSPEND_XTAL_DELAY_US;
_attribute_data_retention_sec_ volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us  = {
     .suspend_early_wakeup_time_us  = SUSPEND_RET_R_DELAY_US + HARDWARE_DELAY_US + SUSPEND_XTAL_DELAY_US + SUSPEND_CODE_US,
     .deep_ret_early_wakeup_time_us = SUSPEND_RET_R_DELAY_US + HARDWARE_DELAY_US,
     .deep_early_wakeup_time_us     = DEEP_R_DELAY_US + HARDWARE_DELAY_US + BOOT_ROM_US,
     .sleep_min_time_us             = DEEP_R_DELAY_US + HARDWARE_DELAY_US + BOOT_ROM_US + SLEEP_START_CODE_US + SLEEP_MIN_CODE_US + SLEEP_MIN_MARGIN_US, //(the maximum value of suspend and deep) + margin.
};
_attribute_data_retention_sec_ volatile pm_r_delay_cycle_s g_pm_r_delay_cycle = {
    // 16K clock, 62.5us
    .deep_r_delay_cycle           = DEEP_R_DELAY_US / 62,
    .suspend_ret_r_delay_cycle    = SUSPEND_RET_R_DELAY_US / 62,
    .deep_xtal_delay_cycle        = DEEP_R_DELAY_US / 62,
    .suspend_ret_xtal_delay_cycle = SUSPEND_RET_R_DELAY_US / 62,
};

extern _attribute_ram_code_sec_noinline_ void flash_send_cmd(unsigned int cmd);

/**
 * @brief       This interface is used to update the value of the g_pm_early_wakeup_time_us structure.
 * @return      none.
 */
void pm_update_early_wakeup_time(void)
{
    int deep_r_delay_us        = g_pm_r_delay_cycle.deep_r_delay_cycle * 1000 / 16;
    int suspend_ret_r_delay_us = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle * 1000 / 16;

    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us  = suspend_ret_r_delay_us + HARDWARE_DELAY_US + g_pm_suspend_xtal_delay_us + g_pm_suspend_code_us;
    g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us = suspend_ret_r_delay_us + HARDWARE_DELAY_US;
    g_pm_early_wakeup_time_us.deep_early_wakeup_time_us     = deep_r_delay_us + HARDWARE_DELAY_US + BOOT_ROM_US;
    if ((g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + g_pm_min_code_us) < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us) {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + SLEEP_START_CODE_US + SLEEP_MIN_MARGIN_US;
    } else {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + SLEEP_START_CODE_US + g_pm_min_code_us + SLEEP_MIN_MARGIN_US;
    }
}

/**
 * @brief       This function configures pm wakeup time parameter.
 * @param[in]   param - deep/suspend/deep_retention r_delay time.(default value: suspend/deep_ret=3, deep=11)
 * @return      none.
 * @note        Those parameters will be lost after reboot or deep sleep, so it required to be reconfigured.
 */
void pm_set_wakeup_time_param(pm_r_delay_cycle_s param)
{
    g_pm_r_delay_cycle.deep_r_delay_cycle           = param.deep_r_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_r_delay_cycle    = param.suspend_ret_r_delay_cycle;
    g_pm_r_delay_cycle.deep_xtal_delay_cycle        = param.deep_xtal_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle = param.suspend_ret_xtal_delay_cycle;

    pm_update_early_wakeup_time();
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
    g_pm_xtal_stable_loopnum   = loopnum;
    g_pm_suspend_xtal_delay_us = delay_us;

    pm_update_early_wakeup_time();
}

/**
 * @brief       This function is used to obtain the cause of software reboot.
 * @return      reboot enum element of pm_poweron_clr_buf0_e.
 * @note        -# the interface pm_update_status_info() must be called before this interface can be invoked;
 */
pm_sw_reboot_reason_e pm_get_sw_reboot_event(void)
{
    return g_pm_system_reboot_event;
}

/**
 * @brief       this function serves to start sleep mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @return      none.
 */
/**< BLE USED *****/
_attribute_ram_code_sec_optimize_o2_noinline_ void pm_sleep_start(pm_sleep_mode_e sleep_mode)
/**< BLE USED END */
{
#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x20;
#endif

    /**
        Before powering up the zb, you need to make sure that the input function of PA2 is disable, otherwise it may cause the RF module to not have clock.
        In response to this question,The principle of suspend handling is:
        If the PA2 input function is enabled, it will not actively turn off the ZB (causing the pm_set_suspend_power_cfg function not to take effect).
        Because if you take the initiative to turn off the ZB, you need to recover after wakeup, and the scenario of Power on zb when PA2 input is enabled will occur,
        so in order not to trigger the situation, choose not to take the initiative to turn off the ZB, in this case,
        the pm_set_suspend_power_cfg function will not take effect, and the suspend power consumption will be higher, but it won't affect the function.
        The processing during suspend sleep is as follows:
        +--------------+-----------------------+--------- -------------------------------------+-----------------------------------------+
        | PA2 input    | User ZB configuration |    Handling of ZB during sleep                |   pm_set_suspend_power_cfg function     |
        +--------------+-----------------------+-----------------------------------------------+-----------------------------------------+
        |   enable     |       power on        |   Maintain user ZB configuration(power on)    |            Not effective                |
        |   enable     |       power down      |   Maintain user ZB configuration(power down)  |            Not effective                |
        |   disable    |       xx              |   determined by the pm_set_suspend_power_cfg  |            effective                    |
        +--------------+-----------------------+-----------------------------------------------+-----------------------------------------+
        Handling after suspend wakeup: restoring user ZB configuration
        See jira for details:BUT-53.(add by weihua.zhang, confirmed by peng.hao 20250101)
     */
    unsigned char areg_7d = 0;
    if(sleep_mode == SUSPEND_MODE)
    {
        areg_7d = analog_read_reg8(areg_aon_0x7d);
        if(reg_gpio_pa_ie&0x04)
        {
            g_pm_suspend_power_cfg &= ~FLD_PD_ZB_EN;
        }
        pm_set_dig_module_power_switch(g_pm_suspend_power_cfg, PM_POWER_DOWN);
    }else{
        pm_set_dig_module_power_switch((FLD_PD_ZB_EN | FLD_PD_USB_EN | FLD_PD_AUDIO_EN), PM_POWER_DOWN);
    }

    flash_send_cmd(FLASH_WRITE_DEEP_CMD);
    gpio_set_mspi_pin_ie_dis();

    //This is 1.2V and 2.0V power supply during sleep. Do not power on during initialization, because after power on,
    //there will be two power supplies at the same time, which may cause abnormalities.add by weihua.zhang, confirmed by haitao 20210107
    pm_enable_native_ldo();

#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x21;
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

#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x22;
#endif

    //If the clearing fails, it indicates that the wake source is active. In this case, the deep mode will reboot,
    //and the other modes continue to run downward.(add by weihua.zhang, 20230907)
    if ((clr_pm_irq_result == 0) || (clr_plic_request_result == 0)) {
#if (PM_DEBUG)
        while (1);
#endif
        if (sleep_mode == DEEPSLEEP_MODE) {
            pm_sys_reboot_with_reason(PM_CLR_PLIC_REQUEST_FAIL, 0x01);
            while (1);
        }
    } else {
#if (PM_DEBUG)
        debug_min_code_tick_3 = rdmcycle();
        debug_sleep_32k_cur = clock_get_32k_tick();
        if ((debug_ana_tick_reset - debug_sleep_32k_cur) > BIT(30)) {
            while (1);
        }
#endif

#if PM_MIN_CODE_DEBUG
        gpio_set_high_level(GPIO_PB5);
#endif

        pm_trigger_sleep();
    }

    //This delay time is about 3.05us under the calibrated 24M RC clock.
    //After being triggered, the MCU needs to wait for a period of time before it actually goes to sleep,
    //during which time the MCU will continue to execute code. If the following code is executed
    //and some modules are awakened, the current will be larger than normal. About 20 empty instructions are fine,
    //but to be on the safe side, add 64 empty instructions.
    //The statement of the for loop may be optimized away by the compiler, resulting in a crash due to insufficient wait time,
    //so it cannot be used.(add by weihua.zhang, confirmed by sihui.wang 20230324)
    CLOCK_DLY_64_CYC;

#if PM_SUSPEND_WHILE_DEBUG_2
    gpio_set_low_level(GPIO_PB5);
#endif

#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x23;
#endif

    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();

#if (PM_DEBUG)
    if (g_pm_status_info.wakeup_src & 0x70) {
        for (unsigned int i = 0; i < 128; i++) {
            debug_ana_reg[i] = analog_read_reg8(i);
        }
        while (1);
    }
#endif

    //Here we need to turn off the mask first, and then clear the plic. If the wake signal is always present and the interrupt mask is enabled,
    //the plic cannot be cleared. When exiting the sleep function, if the total interrupt is turned on, the interrupt handler function is entered.
    //If the interrupt handler is not defined, the program will run away.(changed by weihua.zhang, confirmed by jianzhi 20231101)
    plic_interrupt_disable(IRQ_PM_LVL);
    pm_clr_all_irq_status(); //clear all flag
    plic_clr_all_request();

#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x24;
#endif

    pm_disable_native_ldo();
    pm_disable_spd_ldo_enable_ret_ldo();
    if((areg_7d&FLD_PD_ZB_EN) == 0x00)
    {
        pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);
    }else{
        pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_DOWN);
    }

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

    //Before sleeping,the MSPI has already been switched to 24M RC, so there is no need to wait for Xtal and PLL to stabilize.
    //Advance the flash wakeup to before the delay, because after the flash wakeup, It will take some time to restore to the active working state.
    //(usually set to 150us - a margin is left for different flash models)(add by bingyu.li, confirmed by kaixin.chen 20230616)
    //The flash two-wire system uses clk+cn+ two communication lines, and the flash four-wire system uses
    //clk+cn+ four communication lines. Before suspend sleep, the input of the six lines (PF0-PF5) used
    //by flash will be disabled. After suspend wakes up, the six lines will be set to input function.
    //(changed by weihua.zhang, confirmed by jianzhi 20201201)
    gpio_set_mspi_pin_ie_en();
    flash_send_cmd(FLASH_WRITE_RELEASE_CMD); //flash wakeup

#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x25;
#endif

    //wait for xtal stable and flash restore to the active working state.
    core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * g_pm_suspend_xtal_delay_us));

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_xtal_ready(0x01);

    if (sys_clk_config.bbpll_is_used == 1) {
        pm_bbpll_power_up();
        g_bbpll_is_used = 1;
    }

#if (PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x26;
#endif
}

/**
 * @brief       This function serves to set the working mode of MCU based on 32k crystal,
 *              e.g. suspend mode, deep sleep mode, deep sleep with SRAM retention mode and shutdown mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @param[in]   wakeup_src          - wake up source select.
 * @param[in]   wakeup_tick_type    - tick type select. Use 32K tick count for long-time sleep or 24M tick count for short-time sleep.
 * @param[in]   wakeup_tick         - The tick value at the time of wake-up.
                                      If wakeup_tick_type is pm_tick_timer and
                                      if system timer is 24M, the scale range that can be set is about:
                                      The current tick value + (48000 ~ 0xe0000000) ranges from 2ms ~ 156.59 seconds.
                                      If the system timer is 16M, the scale range that can be set is about:
                                      The current tick value + (32000 ~ 0xe0000000) ranges from 2ms ~ 234.88 seconds.
                                      If the wakeup_tick_type is PM_TICK_32K, then wakeup_tick is converted to 32K.
                                      The range of tick that can be set is approximately: 64~0xffffffff,
                                      and the corresponding sleep time is approximately: 2ms~37hours.
                                      When it exceeds this range, it cannot sleep properly.
 * @note        -# There are two things to note when using LPC wake up:
 *                 1.After the LPC is configured, you need to wait 100 seconds before you can go to sleep.
 *                   After the LPC is opened, 1-2 32k tick is needed to calculate the result.
 *                   Before this, the data in the result register is random. If you enter the sleep function at this time,
 *                   you may not be able to sleep normally because the data in the result register is abnormal.
 *                 2.When entering sleep, keep the input voltage and reference voltage difference must be greater than 30mV,
 *                   otherwise can not enter sleep normally, crash occurs.
 *              -# Before calling this function, the input function of PA2 must be enabled.
 *                 If there is a scenario where the input function of PA2 must be enabled before calling this function, the processing in this function is as follows:
 *                 The FLD_PD_ZB_EN configuration of the function pm_set_suspend_power_cfg will not take effect and it will be handled in accordance with keeping the user FLD_PD_ZB_EN configuration.
 *                 If the user is power on FLD_PD_ZB_EN, the power consumption of suspend will be about 6uA larger in this usage scenario.
 *                 After suspend wakes up, it restores the user's FLD_PD_ZB_EN configuration((BUT-53))
 * @return      indicate whether the cpu is wake up successful.
 * @attention   Must ensure that all GPIOs cannot be floating status before going to sleep to prevent power leakage.
 */
_attribute_ram_code_sec_optimize_o2_noinline_ int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode, pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int wakeup_tick)
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

#if PM_START_CODE_DEBUG
    gpio_set_low_level(GPIO_PB5);
#endif

    ///////////////////       /////////////////////////////////
    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    if (timer_wakeup_enable) {
        if (wakeup_tick_type == PM_TICK_STIMER) {
            unsigned int span = (unsigned int)(wakeup_tick - stimer_get_tick());
            if (span > 0xE0000000) //BIT(31)+BIT(30)+BIT(29)   7/8 cycle of 32bit, 178*7/8 = 156 S
            {
                core_restore_interrupt(r);
                return pm_get_wakeup_src() | STATUS_EXCEED_MAX;
            } else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US) {
                unsigned int t = stimer_get_tick();
                pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
                unsigned char st;
                do {
                    st = pm_get_wakeup_src();
                } while (((unsigned int)stimer_get_tick() - t < span) && !(st & WAKEUP_STATUS_INUSE_ALL));

#if (PM_DEBUG)
                /******************************************debug_pm_info 1 **********************************************/
                debug_pm_info         = 1;
                debug_min_wakeup_src  = st;
                debug_min_stimer_tick = t;
#endif

                core_restore_interrupt(r);
                return st | STATUS_EXCEED_MIN;
            }
        } else {
            //What is the minimum time us, converted to how many 32k ticks.
            //When the minimum time is 2ms, the difference between /31.25 and /31 is 0.5 ticks, so 31 can also be used.
            if (wakeup_tick < g_pm_early_wakeup_time_us.sleep_min_time_us / 31) {
                unsigned int t = clock_get_32k_tick() - 1;
                pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
                unsigned char st;
                do {
                    st = pm_get_wakeup_src();
                } while (((unsigned int)clock_get_32k_tick() - t < wakeup_tick) && !(st & WAKEUP_STATUS_INUSE_ALL));

#if (PM_DEBUG)
                /******************************************debug_pm_info 1 **********************************************/
                debug_pm_info         = 1;
                debug_min_wakeup_src  = st;
                debug_min_stimer_tick = t;
#endif

                core_restore_interrupt(r);
                return st | STATUS_EXCEED_MIN;
            }
        }
    }

    //Turn off all interrupts immediately after entering the sleep function to prevent other interrupts, save the interrupt state before turning off, and restore it after waking up.
    //Turn on pm interrupt only before going to sleep,enable M-mode external interrupt first in this place.
    //modify by bingyu.li, confirmed by jianzhi.chen at 20230810.
    plic_irqs_preprocess_for_wfi(0, FLD_MIE_MEIE);

#if (PM_DEBUG)
    /******************************************debug_pm_info 2 **********************************************/
    debug_pm_info = 2;
#endif

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

    ///////////////////     get 32k calib      /////////////////////////////////
    if (CLK_32K_RC == g_clk_32k_src) {
        while (!stimer_get_tracking_32k_value()); //Wait for the 32k clock calibration to complete.

        g_pm_tick_32k_calib = stimer_get_tracking_32k_value();
    } else {
#if (STIMER_CLOCK == STIMER_CLOCK_16M)
        g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_32CYCLE;
#elif (STIMER_CLOCK == STIMER_CLOCK_24M)
        g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_64CYCLE;
#endif
    }
    unsigned int tick_32k_halfCalib = g_pm_tick_32k_calib >> 1;

#if (PM_DEBUG)
    analog_write_reg16(PM_ANA_REG_POWER_ON_CLR_BUF1, g_pm_tick_32k_calib);
    /******************************************debug_pm_info 4 **********************************************/
    debug_pm_info      = 4;
    debug_tick_32k_cur = analog_read_reg32(0x60);
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

#if PM_START_CODE_DEBUG
    gpio_set_high_level(GPIO_PB5);
#endif

#if PM_MIN_CODE_DEBUG
    gpio_set_low_level(GPIO_PB5);
#endif

#if (PM_DEBUG)
    /******************************************debug_pm_info 5 **********************************************/
    debug_pm_info         = 5;
    debug_min_code_tick_1 = rdmcycle();
#endif

    unsigned int earlyWakeup_us;
    if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) //deep sleep with retention
    {
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us;
    } else if (sleep_mode == DEEPSLEEP_MODE)   //deepsleep no retention
    {
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;
    } else                                     //suspend
    {
        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
    }

    //The variable pmcd.ref_tick is added, replacing the original variable g_pm_tick_cur. Because pmcd.ref_tick directly affects the value of
    //g_pm_long_suspend, g_pm_long_suspend can be assigned after pmcd.ref_tick is updated.changed by weihua,confirmed by biao.li.20201204.
    if (timer_wakeup_enable) {
        unsigned int tick_reset;
        unsigned int tick_wakeup_reset;

        if (wakeup_tick_type == PM_TICK_STIMER) {
            tick_wakeup_reset = (unsigned int)(wakeup_tick - (earlyWakeup_us * SYSTEM_TIMER_TICK_1US) - g_pm_tick_cur);
            if (CLK_32K_RC == g_clk_32k_src) {
                //0x0fff0000 is selected for tick_wakeup_reset * g_track_32kcnt do not exceed 32bit,
                //if more than, first divide and then multiply, if not more than, first multiply and then divide.
                if (tick_wakeup_reset > 0x0fff0000) // 24M: 11.18S, 16M: 16.77S
                {
                    tick_reset        = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * g_track_32kcnt;
                    g_pm_long_suspend = 1;
                } else {
                    tick_reset        = g_pm_tick_32k_cur + (tick_wakeup_reset * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
            } else {
#if (STIMER_CLOCK == STIMER_CLOCK_16M)
                if (tick_wakeup_reset > 0x07ff0000) // 16M: 8.38S
                {
                    tick_reset        = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 32;
                    g_pm_long_suspend = 1;
                } else {
                    tick_reset        = g_pm_tick_32k_cur + (tick_wakeup_reset * 32 + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
#elif (STIMER_CLOCK == STIMER_CLOCK_24M)
                if (tick_wakeup_reset > 0x03ff0000) // 24M: 2.79S
                {
                    tick_reset        = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 64;
                    g_pm_long_suspend = 1;
                } else {
                    tick_reset        = g_pm_tick_32k_cur + (tick_wakeup_reset * 64 + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
#endif
            }
        } else {
            tick_reset = g_pm_tick_32k_cur + wakeup_tick - (earlyWakeup_us * 4 / 125); // 32k clk: /31.25
        }
        //The clock_ct_32k_tick interface needs to avoid encountering rising edges of 32k as much as possible.
        //Otherwise, if the intermediate data generated during the clock_set_32k_tick process has the same value as 32k tick, it will cause the wake-up source to be set.
        //The interval time between the clock_get_32k_tick() and clock_set_32k_tick() interfaces should be as short as possible
        //to avoid the clock_set_32k_tick() interface encountering a rising edge of 32k.
        //add by weihua.zhang at 20240827
        clock_set_32k_tick(tick_reset);

#if (PM_DEBUG)
        analog_write_reg32(PM_ANA_REG_WD_CLR_BUF1, g_pm_tick_32k_cur);
        debug_ana_32k_tick   = analog_read_reg32(areg_aon_0x65);
        debug_ana_tick_reset = tick_reset;
        if (tick_reset != debug_ana_32k_tick) {
            stimer_enable_in_manual_mode();
            stimer_32k_tracking_enable(); //enable 32k cal
            gpio_set_high_level(GPIO_PE7);
            while (1);
        }
        /******************************************debug_pm_info 6 **********************************************/
        debug_pm_info         = 6;
        debug_min_code_tick_2 = rdmcycle();
#endif
    }

    /////////////////// set wakeup source /////////////////////////////////
    pm_set_wakeup_src(wakeup_src);

#if (PM_DEBUG)
    /******************************************debug_pm_info 7 **********************************************/
    debug_pm_info = 7;
#endif

    /////////////////// auto power down /////////////////////////////////
    unsigned short auto_power_down = FLD_PD_32K_RC | FLD_PD_32K_XTAL | FLD_PD_24M_XTAL | FLD_PD_DCDC | FLD_PD_VBUS_LDO | FLD_PD_ANA_BBPLL_TEMP_LDO | FLD_PD_VBUS_SW | FLD_PD_SEQUENCE_EN;
    unsigned char  sleep_ldo_en    = 0;


    /*
     * afe_0x7e<2:0> sram_ret default(000)
     * afe_0x7e<6:4> sram_slp default(000)
     * Analog 0x7e will remain value after sleep, the sram_slp is use to control the sram voltage not only in deep retention sleep mode but also take effect in suspend sleep mode.
     * If enter suspend sleep mode after enter deep retention sleep mode(setup not retention all sram), then the SRAM will also not retention all sram during suspend sleep mode.
     * So for tl321x it needs to be rewritten when enter suspend sleep mode.
     * (added by jilong.liu, confirmed by jianzhi.chen at 20241225)
     */
    analog_write_reg8(areg_aon_0x7e, sleep_mode);

    if (sleep_mode & DEEPSLEEP_RETENTION_FLAG)                                       //deep sleep with retention
    {
        g_pm_multi_addr = reg_mspi_xip_core_size | (reg_mspi_xip_core_offset << 16); //after retention, multiple address offset is lost, save it

        auto_power_down |= FLD_PD_ISOLATION;
        sleep_ldo_en = FLD_PD_DIG_RET_LDO;
    } else if (sleep_mode == DEEPSLEEP_MODE) //deepsleep no retention
    {
        auto_power_down |= FLD_PD_ISOLATION;
        sleep_ldo_en = 0;
    } else //suspend
    {
        sleep_ldo_en = FLD_PD_SPD_LDO;
    }

    if (!(wakeup_src & PM_WAKEUP_COMPARATOR)) {
        auto_power_down |= FLD_PD_LPC;
    }

    if (((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_COMPARATOR)) {
        if (CLK_32K_RC == g_clk_32k_src) {
            auto_power_down &= ~FLD_PD_32K_RC; //disable auto power down 32KRC
        } else {
            //suspend mode or deep retention mode or timer wake up source.
            //(we don't power down external 32k pad clock, even though three's no timer wake up source in suspend or deep retention mode)
            auto_power_down &= ~FLD_PD_32K_XTAL; //if use timer wake up, auto pad 32k power down should be disabled
        }
    } else {
        if (CLK_32K_XTAL == g_clk_32k_src) {
            if (sleep_mode == DEEPSLEEP_MODE) {
                //deep + no tmr wakeup(we need  32k clk to count dcdc dly and xtal dly, but this case, ext 32k clk need close, here we use 32k rc instead.
                //switch 32k clk src: select internal 32k rc, if not do this, when deep+pad wakeup: there's no stable 32k clk(therefore, the pad wake-up time
                //is a bit long, the need for external 32k crystal vibration time) to count DCDC dly and XTAL dly. High temperatures even make it impossible
                //to vibrate, as the code for PWM excitation crystals has not yet been effectively executed. SO, we can switch 32k clk to the internal 32k rc.
                clock_32k_init(CLK_32K_RC);
            } else {
                auto_power_down &= ~FLD_PD_32K_XTAL; //if use timer wake up, auto pad 32k power down should be disabled
            }
        }
    }

    //default:1, 0 Power up ; 1 power down;
    //areg_aon_0x06 need to restore after wake up
    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) | FLD_PD_BBPLL_LDO | FLD_PD_SPD_LDO | FLD_PD_DIG_RET_LDO) & (~sleep_ldo_en));
    sys_clk_config.bbpll_is_used = g_bbpll_is_used;
    g_bbpll_is_used              = 0;
    analog_write_reg16(areg_aon_0x4c, auto_power_down);

#if (PM_DEBUG)
    /******************************************debug_pm_info 8 **********************************************/
    debug_pm_info = 8;
#endif

    /////////////////// R DELAY AND XTAL DELAY /////////////////////////////////
    if (sleep_mode == DEEPSLEEP_MODE) {
        pm_set_delay_cycle(g_pm_r_delay_cycle.deep_xtal_delay_cycle, g_pm_r_delay_cycle.deep_r_delay_cycle);
    } else {
        pm_set_delay_cycle(g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);
    }

#if (PM_DEBUG)
    /******************************************debug_pm_info 9 **********************************************/
    debug_pm_info = 9;
#endif

    //Clear the wake source status after setting the wake tick.The wake tick value is set by bit shift.
    //This process will generate an intermediate value, which may be the same as the current 32k tick value.
    //If the value is the same, the state of the timer wake source will be set.
    //changed by weihua, confirmed by jianzhi. 20240711.
    pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL); //clear all flag

#if (PM_DEBUG)
    debug_sleep_start_wakeup_src1 = pm_get_wakeup_src();
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
#endif

    if (pm_get_wakeup_src() & WAKEUP_STATUS_INUSE_ALL) {
#if (PM_DEBUG)
        debug_sleep_start_wakeup_src2 = pm_get_wakeup_src();
        debug_sleep_start_cur_tick    = analog_read_reg32(0x60);
        debug_sleep_start_set_tick    = analog_read_reg32(0x65);
#endif

    } else {
        if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) {
            g_areg_aon_7f = (g_areg_aon_7f & (~FLD_BOOTFROMBROM)) | g_pm_pad_filter_en;
        } else {
            g_areg_aon_7f = (g_areg_aon_7f | FLD_BOOTFROMBROM | g_pm_pad_filter_en);
        }
        analog_write_reg8(areg_aon_0x7f, g_areg_aon_7f);

        pm_sleep_start(sleep_mode);
    }

#if (PM_DEBUG)
    /******************************************debug_pm_info 12 **********************************************/
    debug_pm_info = 12;
#endif

    if (sleep_mode == DEEPSLEEP_MODE) {
        sys_reset_all(); //reboot
    }

#if (PM_DEBUG)
    /******************************************debug_pm_info 13 **********************************************/
    debug_pm_info = 13;
#endif

    pm_stimer_recover();

#if (PM_DEBUG)
    /******************************************debug_pm_info 14 **********************************************/
    debug_pm_info = 14;
#endif

    clock_restore_clock_config();

    mspi_set_xip_en();

#if (PM_DEBUG)
    /******************************************debug_pm_info 15 **********************************************/
    debug_pm_info = 15;
#endif

#if PM_SUSPEND_WHILE_DEBUG_2
    gpio_set_high_level(GPIO_PB5);
#endif

#if PM_SUSPEND_WHILE_DEBUG
    gpio_set_low_level(GPIO_PB5);
#endif
    if ((g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable) //wakeup from timer only
    {
        if (wakeup_tick_type == PM_TICK_STIMER) {
            while ((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30));
        } else {
            while ((unsigned int)(clock_get_32k_tick() - wakeup_tick - g_pm_tick_32k_cur + 1) > BIT(30));
        }
    }

#if PM_SUSPEND_WHILE_DEBUG
    gpio_set_high_level(GPIO_PB5);
#endif

#if (PM_DEBUG)
    /******************************************debug_pm_info 16 **********************************************/
    debug_pm_info = 16;
#endif

    //  DBG_CHN2_LOW;
    //Resume the interrupted state before sleep.Cannot be placed in the pm_sleep_start() interface to avoid failure to recover if this interface is not called.
    //changed by weihua, confirmed by jianzhi. 20231115
    plic_irqs_postprocess_for_wfi();
    core_restore_interrupt(r);

#if (PM_DEBUG)
    /******************************************debug_pm_info 17 **********************************************/
    debug_pm_info = 17;
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
 * @brief       This function serves to switch digital module power.
 * @param[in]   module - digital module.
 * @param[in]   power_sel - power up or power down.
 * @return      none.
 * @note        Before calling this interface to open base band, you need to make sure that the input function of PA2 is turned off.(BUT-53)
 */
_attribute_ram_code_sec_optimize_o2_noinline_ void pm_set_dig_module_power_switch(pm_pd_module_e module, pm_power_sel_e power_sel)
{
    /*
    * This function will be called in PM, need to pay attention to the conditional judgment statement
    * where the code execution time is consistent.
    */

    /*
    * when doing digital module power switch should make sure the 24m rc is working or power switch won't take effect.
    * (add by jilong.liu, confirmed by jianzhi at 20240807)
    */
    pm_24mrc_power_up();

    /*
    * After setting the power switch register of the digital module in the second step, it will take some time to take effect.
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
     * tl321x don't have the pd_sm_busy bit(tl7518 and tl721x has 0x69<5> which can indicate power switch status).
     * And the 0x69<0:3> can only indicate the power up process, it's invalid when power down.
     * Wait for power stable, for this chip(tl321x), is a fixed value 5us.
     * (added by jilong.liu, confirmed by jianzhi.chen at 20240514)
     * The time required for different chips may vary so it is necessary to confirm with the chip colleagues when porting other chips.
     */
    core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * 5)); //delay 5us

    pm_24mrc_power_down_if_unused();
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
 * @brief       This function serves to set system power mode.
 * @param[in]   power_mode  - power mode(LDO/LDO_DCDC).
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
            analog_write_reg8(areg_aon_0x0c, (analog_read_reg8(areg_aon_0x0c) & 0x0f) | 0x60); // trim dcdc output to 1.2v
            /*
             * In A1 version, undering DCDC mode, some chips with low vdd_core voltage (around 0.95V, still within the theoretical voltage range for digital operation) 
             * will cause a crash when open RF module frequently by write analog 0x7d.
             * The specific reason is not yet clear, but raising the digital ldo voltage can temporarily avoid this problem.(ISSUE:BUT-41)
             * TODO: In A2 version, this need to be re-evaluate.(add by jilong.liu, confirmed by wenfeng.lou 20241111)
             */
            if (g_chip_version == CHIP_VERSION_A1) {
                analog_write_reg8(0x10, (analog_read_reg8(0x10) & 0x8f) | 0x70);                   // itrim_dig close all branch
                analog_write_reg8(areg_aon_0x0f, (analog_read_reg8(areg_aon_0x0f) & 0x0f) | 0xc0); // trim dig ldo to 1.05v
            }
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
        sys_reset_all();
        while (1);
    }
}
