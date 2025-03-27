/********************************************************************************************************
 * @file    pm.c
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
#include "watchdog.h"
#include "string.h"
#include "lib/include/swire.h"

/********************************************************************************************************
 *   The time a new chip needs to be tested and the time required to test when changing the code.
 *******************************************************************************************************/
//the code run time for preparation entering and exiting suspend sleep, 100-131(while waiting time).
//Open the macro definition PM_SUSPEND_WHILE_DEBUG and test this using TEST_SLEEP_TIME_ACCURACY in the test demo.
#define SUSPEND_O2_CODE_US          383 //Use the O2 optimization option
#define SUSPEND_OS_CODE_US          632 //Use the OS optimization option

//the minimum code run time before sleep
//Open the macro definition PM_MIN_CODE_DEBUG and test this using TEST_SLEEP_TIME_ACCURACY in the test demo.
#define SLEEP_O2_MIN_CODE_US        134 //Use the O2 optimization option
#define SLEEP_OS_MIN_CODE_US        362 //Use the OS optimization option

/********************************************************************************************************
 *                         The time required to test on a new chip.
 *******************************************************************************************************/
//Time before 32k tick value in sleep function.
//Open the macro definition PM_START_CODE_DEBUG and test this using TEST_SLEEP_TIME_ACCURACY in the test demo.
//In order to make this time compatible with more clocks, compatible with the most extreme cases, you need to do the following:
//1. Use a calibrated 24M rc clock.
//2. Call the PM function immediately after system initialization.
#define SLEEP_START_CODE_US         60 //30-60

/********************************************************************************************************
 *                         The time required to confirm on the new chip.
 *******************************************************************************************************/
//r_delay: 2*1/16k=125uS  3*1/16k=187.5uS  4*1/16k=250uS  11*1/16k=687.5uS
//The 16k clock is derived from the 32k clock frequency division.
//The 32k rc frequency is 32000. 32k pad frequency is 32768. Each tick varies by 0.73us, so 32k pad can use 32k rc data.
#define SUSPEND_RET_R_DELAY_US      188 //suspend or deep retention r_delay(us)
#define DEEP_R_DELAY_US             688 //deep r_delay(us).
                                        //The minimum value can be the same as deep retention,
                                        //because deep has a current pulse when it wakes up, so the value is set higher.

#define SUSPEND_XTAL_DELAY_US       200 //waits for XTAL to stabilize after suspend waking up.
#define BOOT_ROM_US                 390 //BOOT ROM

#define HARDWARE_DELAY_US           109 //3.5*(1/32k), suspend/deep/deep retention hardware delay.

/********************************************************************************************************
 *                                 The usual fixed time.
 *******************************************************************************************************/
#define SLEEP_MIN_MARGIN_US         400 //400 means more margin, >32 is enough.

#if(PM_DEBUG)
volatile unsigned char debug_pm_info;
volatile unsigned int debug_ana_32k_tick;
volatile unsigned int debug_sleep_32k_cur;
volatile unsigned int debug_ana_tick_reset;
volatile unsigned int debug_tick_32k_cur;
volatile unsigned char debug_min_wakeup_src=0;
volatile unsigned char debug_sleep_start_wakeup_src0=0;
volatile unsigned char debug_sleep_start_wakeup_src1=0;
volatile unsigned char debug_sleep_start_wakeup_src2=0;
volatile unsigned int debug_min_stimer_tick=0;
volatile unsigned int debug_sleep_start_cur_tick=0;
volatile unsigned int debug_sleep_start_set_tick=0;
volatile unsigned int debug_sleep_wakeup_return=0;
volatile unsigned long long debug_while_7d_tick_1;
volatile unsigned long long debug_while_7d_tick_2;
volatile unsigned long long debug_while_7d_tick_3;
volatile unsigned long long debug_min_code_tick_1;
volatile unsigned long long debug_min_code_tick_2;
volatile unsigned long long debug_min_code_tick_3;
volatile unsigned char debug_ana_reg[128];
#endif

extern _attribute_data_retention_sec_ unsigned int g_pm_xtal_stable_loopnum;

_attribute_aligned_(4) pm_status_info_s g_pm_status_info;
volatile unsigned char g_pm_system_reboot_event = 0;
unsigned char g_areg_aon_7f = 0;

_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_calib;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_cur;
_attribute_data_retention_sec_  unsigned int            g_pm_tick_32k_cur;
_attribute_data_retention_sec_  unsigned char           g_pm_long_suspend;
_attribute_data_retention_sec_  unsigned char           g_areg_aon_0a=0;
_attribute_data_retention_sec_  static unsigned char    g_pm_suspend_power_cfg=(FLD_PD_ZB_EN | FLD_PD_USB_EN | FLD_PD_AUDIO_EN);
_attribute_data_retention_sec_  unsigned char           g_pm_pad_filter_en=0x00;//FLD_PAD_FILTER_EN //BLE used and not use static
_attribute_data_retention_sec_  unsigned int            g_pm_multi_addr=0;
_attribute_data_retention_sec_  unsigned int            g_pm_suspend_code_us = SUSPEND_O2_CODE_US;
_attribute_data_retention_sec_  unsigned int            g_pm_min_code_us = SLEEP_O2_MIN_CODE_US;
_attribute_data_retention_sec_  unsigned int            g_pm_suspend_xtal_delay_us = SUSPEND_XTAL_DELAY_US;
_attribute_data_retention_sec_  unsigned int            g_pm_mspi_cfg=0;
_attribute_data_retention_sec_
volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us = {
    .suspend_early_wakeup_time_us = SUSPEND_RET_R_DELAY_US + HARDWARE_DELAY_US + SUSPEND_XTAL_DELAY_US + SUSPEND_O2_CODE_US,
    .deep_ret_early_wakeup_time_us = SUSPEND_RET_R_DELAY_US + HARDWARE_DELAY_US,
    .deep_early_wakeup_time_us = DEEP_R_DELAY_US + HARDWARE_DELAY_US + BOOT_ROM_US,
    .sleep_min_time_us = DEEP_R_DELAY_US + HARDWARE_DELAY_US + BOOT_ROM_US + SLEEP_START_CODE_US + SLEEP_O2_MIN_CODE_US + SLEEP_MIN_MARGIN_US, //(the maximum value of suspend and deep) + margin.
};
_attribute_data_retention_sec_
volatile pm_r_delay_cycle_s g_pm_r_delay_cycle = {  // 16K clock, 62.5us
    .deep_r_delay_cycle = DEEP_R_DELAY_US / 62,
    .suspend_ret_r_delay_cycle = SUSPEND_RET_R_DELAY_US / 62,
    .deep_xtal_delay_cycle = DEEP_R_DELAY_US / 62,
    .suspend_ret_xtal_delay_cycle = SUSPEND_RET_R_DELAY_US / 62,
};

extern _attribute_ram_code_com_sec_optimize_o2_noinline_ void flash_send_cmd(unsigned long addr, unsigned int cmd);

__attribute__((section(".ram_code_retention"))) __attribute__((noinline)) 
void pm_retention_register_recover(void)
{
}


/**
 * @brief       This interface is used to update the value of the g_pm_early_wakeup_time_us structure.
 * @return      none.
 */
void pm_update_early_wakeup_time(void)
{
    int deep_r_delay_us = g_pm_r_delay_cycle.deep_r_delay_cycle * 1000 / 16;
    int suspend_ret_r_delay_us = g_pm_r_delay_cycle.suspend_ret_r_delay_cycle * 1000 / 16;

    g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us = suspend_ret_r_delay_us + HARDWARE_DELAY_US + g_pm_suspend_xtal_delay_us + g_pm_suspend_code_us;
    g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us = suspend_ret_r_delay_us + HARDWARE_DELAY_US;
    g_pm_early_wakeup_time_us.deep_early_wakeup_time_us = deep_r_delay_us + HARDWARE_DELAY_US + BOOT_ROM_US;
    if((g_pm_early_wakeup_time_us.deep_early_wakeup_time_us + g_pm_min_code_us) < g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us)
    {
        g_pm_early_wakeup_time_us.sleep_min_time_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us + SLEEP_START_CODE_US + SLEEP_MIN_MARGIN_US;
    }
    else
    {
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
    g_pm_r_delay_cycle.deep_r_delay_cycle = param.deep_r_delay_cycle;
    g_pm_r_delay_cycle.suspend_ret_r_delay_cycle = param.suspend_ret_r_delay_cycle;
    g_pm_r_delay_cycle.deep_xtal_delay_cycle = param.deep_xtal_delay_cycle;
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
    g_pm_xtal_stable_loopnum = loopnum;
    g_pm_suspend_xtal_delay_us = delay_us;

    pm_update_early_wakeup_time();
}

/**
 * @brief       This function is used to configure data about code runtime updates when os optimization options are used.
 * @param[in]   optimization - Currently selected optimization options. O2 or Os.
 * @return      none.
 * @note        Those parameters will be lost after reboot or deep sleep, so it required to be reconfigured.
 */
void pm_set_cfg_for_os_compile_opt(pm_optimize_sel_e optimization)
{
    if(optimization == PM_OPTIMIZE_O2)
    {
        g_pm_suspend_code_us = SUSPEND_O2_CODE_US;
        g_pm_min_code_us = SLEEP_O2_MIN_CODE_US;
    }
    else
    {
        g_pm_suspend_code_us = SUSPEND_OS_CODE_US;
        g_pm_min_code_us = SLEEP_OS_MIN_CODE_US;
    }

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
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_sleep_start(pm_sleep_mode_e sleep_mode) //BLE SDK use, not use static
{
#if(PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x20;
#endif

    pm_set_dig_module_power_switch(g_pm_suspend_power_cfg, PM_POWER_DOWN);

    //only tercel need, not a public config, jira:TER-43
    //This bit controls the capacitor switch used during the crystal initiation process, which is used during r_dly,
    //so it needs to be turned on before sleep.
    analog_write_reg8(0x8b, analog_read_reg8(0x8b) & 0xf7);

    flash_send_cmd(0, FLASH_WRITE_DEEP_CMD);
    gpio_set_mspi_pin_ie_dis();

    //This is 1.2V and 2.0V power supply during sleep. Do not power on during initialization, because after power on,
    //there will be two power supplies at the same time, which may cause abnormalities.add by weihua.zhang, confirmed by haitao 20210107
    pm_enable_native_ldo();

#if(PM_DEBUG)
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
    unsigned char pm_priority = reg_irq_src_priority(IRQ_PM_LVL);
    plic_set_priority(IRQ_PM_LVL,IRQ_PRI_LEV1);
    unsigned char pm_threshold = reg_irq_threshold;
    plic_set_threshold(IRQ_PRI_NUM0);
    unsigned int clr_plic_request_result = 0;
    unsigned char clr_pm_irq_result = pm_clr_all_irq_status();
    if(clr_pm_irq_result == 1){
        clr_plic_request_result = plic_clr_all_request();
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x22;
#endif

    //If the clearing fails, it indicates that the wake source is active. In this case, the deep mode will reboot,
    //and the other modes continue to run downward.(add by weihua.zhang, 20230907)
    if((clr_pm_irq_result == 0) || (clr_plic_request_result == 0))
    {
#if(PM_DEBUG)
        while(1);
#endif
        if(sleep_mode == DEEPSLEEP_MODE)
        {
            pm_sys_reboot_with_reason(PM_CLR_PLIC_REQUEST_FAIL, 0x01);
            while(1);
        }
    }
    else
    {
#if(PM_DEBUG)
        debug_min_code_tick_3 = rdmcycle();
        debug_sleep_32k_cur = analog_read_reg32(areg_aon_0x60);
        if((debug_ana_tick_reset - debug_sleep_32k_cur) > BIT(30)){
            while(1);
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

#if(PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x23;
#endif

    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();
    g_pm_status_info.wakeup_src = pm_get_wakeup_src();

#if(PM_DEBUG)
    if(g_pm_status_info.wakeup_src&0x70){
        for(unsigned int i=0; i<128; i++){
            debug_ana_reg[i] = analog_read_reg8(i);
        }
        while(1);
    }
#endif

    //Here we need to turn off the mask first, and then clear the plic. If the wake signal is always present and the interrupt mask is enabled,
    //the plic cannot be cleared. When exiting the sleep function, if the total interrupt is turned on, the interrupt handler function is entered.
    //If the interrupt handler is not defined, the program will run away.(changed by weihua.zhang, confirmed by jianzhi 20231101)
    plic_interrupt_disable(IRQ_PM_LVL);
    plic_set_priority(IRQ_PM_LVL,pm_priority);
    plic_set_threshold(pm_threshold);
    pm_clr_all_irq_status();  //clear all flag
    plic_clr_all_request();

#if(PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x24;
#endif

    pm_disable_native_ldo();
    pm_disable_spd_ldo();

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

#if PM_XTAL_READY_TIME
    gpio_function_en(GPIO_PB4);
    gpio_output_en(GPIO_PB4);
    gpio_input_dis(GPIO_PB4);
    gpio_set_high_level(GPIO_PB4);
#endif

    //Before sleeping,the MSPI has already been switched to 24M RC, so there is no need to wait for Xtal and PLL to stabilize.
    //Advance the flash wakeup to before the delay, because after the flash wakeup, It will take some time to restore to the active working state.
    //(usually set to 150us - a margin is left for different flash models)(add by bingyu.li, confirmed by kaixin.chen 20230616)
    //The flash two-wire system uses clk+cn+ two communication lines, and the flash four-wire system uses
    //clk+cn+ four communication lines. Before suspend sleep, the input of the six lines (PF0-PF5) used
    //by flash will be disabled. After suspend wakes up, the six lines will be set to input function.
    //(changed by weihua.zhang, confirmed by jianzhi 20201201)
    gpio_set_mspi_pin_ie_en();
    flash_send_cmd(0, FLASH_WRITE_RELEASE_CMD);     //flash wakeup

#if(PM_DEBUG)
    /******************************************debug_pm_info**********************************************/
    debug_pm_info = 0x25;
#endif

    //wait for xtal stable and flash restore to the active working state.
    core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * g_pm_suspend_xtal_delay_us));

#if PM_XTAL_READY_TIME
    gpio_set_low_level(GPIO_PB4);
#endif

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_xtal_ready(0x01);

    if(sys_clk_config.bbpll_is_used == 1)
    {
        pm_bbpll_power_up();
        g_bbpll_is_used = 1;
    }

#if(PM_DEBUG)
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
 * @note        There are two things to note when using LPC wake up:
 *              1.After the LPC is configured, you need to wait 100 seconds before you can go to sleep.
 *                After the LPC is opened, 1-2 32k tick is needed to calculate the result.
 *                Before this, the data in the result register is random. If you enter the sleep function at this time,
 *                you may not be able to sleep normally because the data in the result register is abnormal.
 *              2.When entering sleep, keep the input voltage and reference voltage difference must be greater than 30mV,
 *                otherwise can not enter sleep normally, crash occurs.
 * @return      indicate whether the cpu is wake up successful.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ int pm_sleep_wakeup_ram(pm_sleep_mode_e sleep_mode, pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int wakeup_tick)
{
    /**
     * At present, the compensation value in the function is tested on the basis of the optimization level O2. If there are other optimization levels,
     * it needs to be retested and then see how the compensation time is handled.
     */
    /**
     *  ==============================          -O2 compilation compatibility processing        ==============================
     *
     * The pm_sleep_wakeup interface will stop xip, so this function can not have a text section of code called, workaround:
     * (1) for very short functions add _always_inline (2) for large functions, specify _attribute_ram_code_com_sec_noinline_.
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
    if(timer_wakeup_enable)
    {
        if(wakeup_tick_type == PM_TICK_STIMER)
        {
            unsigned int span = (unsigned int)(wakeup_tick - stimer_get_tick());
            if (span > 0xE0000000) //BIT(31)+BIT(30)+BIT(29)   7/8 cycle of 32bit, 178*7/8 = 156 S
            {
                core_restore_interrupt(r);
                return  pm_get_wakeup_src() | STATUS_EXCEED_MAX;
            }
            else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US)
            {
                unsigned int t = stimer_get_tick();
                pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
                unsigned char st;
                do
                {
                    st = pm_get_wakeup_src();
                }while ( ((unsigned int)stimer_get_tick () - t < span) && !(st & WAKEUP_STATUS_INUSE_ALL));

#if(PM_DEBUG)
                /******************************************debug_pm_info 1 **********************************************/
                debug_pm_info = 1;
                debug_min_wakeup_src = st;
                debug_min_stimer_tick = t;
#endif

                core_restore_interrupt(r);
                return st | STATUS_EXCEED_MIN;
            }
        }
        else
        {
            //What is the minimum time us, converted to how many 32k ticks.
            //When the minimum time is 2ms, the difference between /31.25 and /31 is 0.5 ticks, so 31 can also be used.
            if (wakeup_tick < g_pm_early_wakeup_time_us.sleep_min_time_us / 31)
            {
                unsigned int t = clock_get_32k_tick () - 1;
                pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);
                unsigned char st;
                do
                {
                    st = pm_get_wakeup_src();
                }while ( ((unsigned int)clock_get_32k_tick () - t < wakeup_tick) && !(st & WAKEUP_STATUS_INUSE_ALL));

#if(PM_DEBUG)
                /******************************************debug_pm_info 1 **********************************************/
                debug_pm_info = 1;
                debug_min_wakeup_src = st;
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

#if(PM_DEBUG)
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

#if(PM_DEBUG)
    /******************************************debug_pm_info 3 **********************************************/
    debug_pm_info = 3;
#endif

    ///////////////////     get 32k calib      /////////////////////////////////
    if(CLK_32K_RC == g_clk_32k_src)
    {
        while(!stimer_get_tracking_32k_value());   //Wait for the 32k clock calibration to complete.

        g_pm_tick_32k_calib = stimer_get_tracking_32k_value();
    }
    else
    {
#if(STIMER_CLOCK == STIMER_CLOCK_16M)
        g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_32CYCLE;
#elif(STIMER_CLOCK == STIMER_CLOCK_24M)
        g_pm_tick_32k_calib = CRYSTAL32768_TICK_PER_64CYCLE;
#endif
    }
    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;

#if(PM_DEBUG)
    analog_write_reg16(PM_ANA_REG_POWER_ON_CLR_BUF1, g_pm_tick_32k_calib);
    /******************************************debug_pm_info 4 **********************************************/
    debug_pm_info = 4;
    debug_tick_32k_cur = analog_read_reg32(areg_aon_0x60);
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

#if PM_START_CODE_DEBUG
    gpio_set_high_level(GPIO_PB5);
#endif

#if PM_MIN_CODE_DEBUG
    gpio_set_low_level(GPIO_PB5);
#endif

#if(PM_DEBUG)
    /******************************************debug_pm_info 5 **********************************************/
    debug_pm_info = 5;
    debug_min_code_tick_1 = rdmcycle();
#endif

    unsigned int earlyWakeup_us;
    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)  //deep sleep with retention
    {
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us;
    }
    else if(sleep_mode == DEEPSLEEP_MODE)  //deepsleep no retention
    {
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;
    }
    else  //suspend
    {
        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
    }

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
                //0x0fff0000 is selected for tick_wakeup_reset * g_track_32kcnt do not exceed 32bit,
                //if more than, first divide and then multiply, if not more than, first multiply and then divide.
                if(tick_wakeup_reset > 0x0fff0000)      // 24M: 11.18S, 16M: 16.77S
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
#if(STIMER_CLOCK == STIMER_CLOCK_16M)
                if(tick_wakeup_reset > 0x07ff0000)      // 16M: 8.38S
                {
                    tick_reset = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 32;
                    g_pm_long_suspend = 1;
                }
                else
                {
                    tick_reset = g_pm_tick_32k_cur + (tick_wakeup_reset * 32 + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
#elif(STIMER_CLOCK == STIMER_CLOCK_24M)
                if(tick_wakeup_reset > 0x03ff0000)      // 24M: 2.79S
                {
                    tick_reset = g_pm_tick_32k_cur + tick_wakeup_reset / g_pm_tick_32k_calib * 64;
                    g_pm_long_suspend = 1;
                }
                else
                {
                    tick_reset = g_pm_tick_32k_cur + (tick_wakeup_reset * 64 + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                    g_pm_long_suspend = 0;
                }
#endif
            }
        }
        else
        {
            tick_reset = g_pm_tick_32k_cur + wakeup_tick - (earlyWakeup_us * 4 / 125);  // 32k clk: /31.25
        }
        //The clock_ct_32k_tick interface needs to avoid encountering rising edges of 32k as much as possible.
        //Otherwise, if the intermediate data generated during the clock_set_32k_tick process has the same value as 32k tick, it will cause the wake-up source to be set.
        //The interval time between the clock_get_32k_tick() and clock_set_32k_tick() interfaces should be as short as possible 
        //to avoid the clock_set_32k_tick() interface encountering a rising edge of 32k.
        //add by weihua.zhang at 20240827
        clock_set_32k_tick(tick_reset);

#if(PM_DEBUG)
        analog_write_reg32(PM_ANA_REG_WD_CLR_BUF1, g_pm_tick_32k_cur);
        debug_ana_32k_tick = analog_read_reg32(areg_aon_0x65);
        debug_ana_tick_reset = tick_reset;
        if(tick_reset != debug_ana_32k_tick)
        {
            stimer_enable_in_manual_mode();
            stimer_32k_tracking_enable();   //enable 32k cal
            gpio_set_high_level(GPIO_PE7);
            while(1);
        }
        /******************************************debug_pm_info 6 **********************************************/
        debug_pm_info = 6;
        debug_min_code_tick_2 = rdmcycle();
#endif

    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 7 **********************************************/
    debug_pm_info = 7;
#endif

    /////////////////// set wakeup source /////////////////////////////////
    pm_set_wakeup_src(wakeup_src);

#if(PM_DEBUG)
    /******************************************debug_pm_info 8 **********************************************/
    debug_pm_info = 8;
#endif

    /////////////////// auto power down /////////////////////////////////
    unsigned short auto_power_down = FLD_PD_32K_RC|FLD_PD_32K_XTAL|FLD_PD_24M_XTAL|FLD_PD_DCDC|FLD_PD_VBUS_LDO|FLD_PD_ANA_BBPLL_TEMP_LDO\
                                    |FLD_PD_DCORE_SRAM_LDO|FLD_PD_VBUS_SW|FLD_PD_SEQUENCE_EN;
    unsigned char sleep_ldo_en = 0;

    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG)  //deep sleep with retention
    {
        g_pm_multi_addr = reg_mspi_xip_core_size | (reg_mspi_xip_core_offset << 16);//after retention, multiple address offset is lost, save it

        /*
        * afe_0x7e<3:0> sram_ret default(0000)
        * afe_0x7e<7:4> rsvd
        * For Tercel only deep retention need to set sleep_mode, this can save time for enter deep/suspend sleep mode.
        */
        analog_write_reg8(areg_aon_0x7e, sleep_mode);

        auto_power_down |= FLD_PD_ISOLATION;
        sleep_ldo_en = FLD_PD_DIG_RET_LDO;
    }
    else if(sleep_mode == DEEPSLEEP_MODE)  //deepsleep no retention
    {
        auto_power_down |= FLD_PD_ISOLATION;
        sleep_ldo_en = 0;
    }
    else  //suspend
    {
        sleep_ldo_en = FLD_PD_SPD_LDO;
    }

    if(!(wakeup_src & PM_WAKEUP_COMPARATOR)){
        auto_power_down |= FLD_PD_LPC;
    }

    if(((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER) || (wakeup_src & PM_WAKEUP_COMPARATOR))
    {
        if(CLK_32K_RC == g_clk_32k_src){
            auto_power_down &= ~FLD_PD_32K_RC;  //disable auto power down 32KRC
        }else{
            //suspend mode or deep retention mode or timer wake up source.
            //(we don't power down external 32k pad clock, even though three's no timer wake up source in suspend or deep retention mode)
            auto_power_down &= ~FLD_PD_32K_XTAL;  //if use timer wake up, auto pad 32k power down should be disabled
        }
    }else{
        if(CLK_32K_XTAL == g_clk_32k_src){
            if(sleep_mode == DEEPSLEEP_MODE){
                //deep + no tmr wakeup(we need  32k clk to count dcdc dly and xtal dly, but this case, ext 32k clk need close, here we use 32k rc instead.
                //switch 32k clk src: select internal 32k rc, if not do this, when deep+pad wakeup: there's no stable 32k clk(therefore, the pad wake-up time
                //is a bit long, the need for external 32k crystal vibration time) to count DCDC dly and XTAL dly. High temperatures even make it impossible
                //to vibrate, as the code for PWM excitation crystals has not yet been effectively executed. SO, we can switch 32k clk to the internal 32k rc.
                clock_32k_init(CLK_32K_RC);
            }else{
                auto_power_down &= ~FLD_PD_32K_XTAL;  //if use timer wake up, auto pad 32k power down should be disabled
            }
        }
    }

    //default:1, 0 Power up ; 1 power down;
    //areg_aon_0x06 need to restore after wake up
    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) | FLD_PD_BBPLL_LDO | FLD_PD_SPD_LDO | FLD_PD_DIG_RET_LDO) & (~sleep_ldo_en));
    sys_clk_config.bbpll_is_used = g_bbpll_is_used;
    g_bbpll_is_used = 0;
    analog_write_reg16(areg_aon_0x4c, auto_power_down);

#if(PM_DEBUG)
    /******************************************debug_pm_info 9 **********************************************/
    debug_pm_info = 9;
#endif

    /////////////////// R DELAY AND XTAL DELAY /////////////////////////////////
    if(sleep_mode == DEEPSLEEP_MODE)
    {
        pm_set_delay_cycle(g_pm_r_delay_cycle.deep_xtal_delay_cycle, g_pm_r_delay_cycle.deep_r_delay_cycle);
    }
    else
    {
        pm_set_delay_cycle(g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 10 **********************************************/
    debug_pm_info = 10;
#endif

    //Clear the wake source status after setting the wake tick.The wake tick value is set by bit shift.
    //This process will generate an intermediate value, which may be the same as the current 32k tick value.
    //If the value is the same, the state of the timer wake source will be set.
    //changed by weihua, confirmed by jianzhi. 20240711.
    pm_clr_irq_status(FLD_WAKEUP_STATUS_ALL);               //clear all flag

#if(PM_DEBUG)
    debug_sleep_start_wakeup_src1 = pm_get_wakeup_src();
    /******************************************debug_pm_info 11 **********************************************/
    debug_pm_info = 11;
#endif

    if(pm_get_wakeup_src() & WAKEUP_STATUS_INUSE_ALL){

#if(PM_DEBUG)
        debug_sleep_start_wakeup_src2 = pm_get_wakeup_src();
        debug_sleep_start_cur_tick = analog_read_reg32(0x60);
        debug_sleep_start_set_tick = analog_read_reg32(0x65);
#endif

    }else{

        if(sleep_mode & DEEPSLEEP_RETENTION_FLAG){
            g_areg_aon_7f = (g_areg_aon_7f & (~FLD_BOOTFROMBROM)) | g_pm_pad_filter_en;
        }else{
            g_areg_aon_7f = (g_areg_aon_7f | FLD_BOOTFROMBROM | g_pm_pad_filter_en);
        }
        analog_write_reg8(areg_aon_0x7f, g_areg_aon_7f);

        pm_sleep_start(sleep_mode);
    }

#if(PM_DEBUG)
        /******************************************debug_pm_info 12 **********************************************/
    debug_pm_info = 12;
#endif

    if(sleep_mode == DEEPSLEEP_MODE){
        sys_reset_all();  //reboot
    }

#if(PM_DEBUG)
    /******************************************debug_pm_info 13 **********************************************/
    debug_pm_info = 13;
#endif

    pm_stimer_recover();

#if(PM_DEBUG)
    /******************************************debug_pm_info 14 **********************************************/
    debug_pm_info = 14;
#endif

    clock_restore_clock_config();

    mspi_set_xip_en();

#if(PM_DEBUG)
    /******************************************debug_pm_info 15 **********************************************/
    debug_pm_info = 15;
#endif

#if PM_SUSPEND_WHILE_DEBUG_2
    gpio_set_high_level(GPIO_PB5);
#endif

#if PM_SUSPEND_WHILE_DEBUG
    gpio_set_low_level(GPIO_PB5);
#endif
    if((g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable)  //wakeup from timer only
    {
        if(wakeup_tick_type == PM_TICK_STIMER){
            while((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30));
        }else{
            while((unsigned int)(clock_get_32k_tick() - wakeup_tick - g_pm_tick_32k_cur + 1) > BIT(30));
        }
    }

#if PM_SUSPEND_WHILE_DEBUG
    gpio_set_high_level(GPIO_PB5);
#endif

#if(PM_DEBUG)
    /******************************************debug_pm_info 16 **********************************************/
    debug_pm_info = 16;
#endif

    //  DBG_CHN2_LOW;
    //Resume the interrupted state before sleep.Cannot be placed in the pm_sleep_start() interface to avoid failure to recover if this interface is not called.
    //changed by weihua, confirmed by jianzhi. 20231115
    plic_irqs_postprocess_for_wfi();
    core_restore_interrupt(r);

#if(PM_DEBUG)
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
    if(0 == on_off){
        g_pm_suspend_power_cfg |= (value);
    }
    else{
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
    //        PA[7:0]           PB[7:0]         PC[7:0]         PD[7:0]     PE[7:0]         PF[7:0]
    // pol: ana_0x3f<7:0>    ana_0x40<7:0>  ana_0x41<7:0>  ana_0x42<7:0>  ana_0x43<7:0>  ana_0x44<7:0>
    // en:  ana_0x45<7:0>    ana_0x46<7:0>  ana_0x47<7:0>  ana_0x48<7:0>  ana_0x49<7:0>  ana_0x4a<7:0>
    unsigned char mask = pin & 0xff;
    unsigned char analog_reg;
    unsigned char val;

    ////////////////////////// polarity ////////////////////////
    analog_reg = ((pin >> 8) + 0x3f);
    val = analog_read_reg8(analog_reg);
    if (pol) {
        val &= ~mask;
    }
    else {
        val |= mask;
    }
    analog_write_reg8(analog_reg, val);

    /////////////////////////// enable /////////////////////
    analog_reg = ((pin >> 8) + 0x45);
    val = analog_read_reg8(analog_reg);
    if (en) {
        val |= mask;
    }
    else {
        val &= ~mask;
    }
    analog_write_reg8(analog_reg, val);
}

/**
 * @brief       This function serves to switch digital module power.
 * @param[in]   module - digital module.
 * @param[in]   power_sel - power up or power down.
 * @return      none.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_set_dig_module_power_switch(pm_pd_module_e module, pm_power_sel_e power_sel)
{
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
     * Wait for power stable, for this chip(Tercel), it will cost 5 * 1/24M times.
     * The time required for different chips may vary so it is necessary to confirm with the chip colleagues when porting other chips.
     */
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk);//delay 1us

#if(PM_DEBUG)
    debug_while_7d_tick_1 = rdmcycle();
#endif

    while(analog_read_reg8(areg_aon_0x69) & FLD_PD_SM_BUSY){}

#if(PM_DEBUG)
    debug_while_7d_tick_2 = rdmcycle();
    CLOCK_DLY_64_CYC; //0x47tick(3-2) -> 0x160(2-1)
    debug_while_7d_tick_3 = rdmcycle();
#endif

    /*
        On the Onca platform, it was tested out that turning off power sequence clk (0x7d<7>) during sleep wake-up may cause issues.
        Colleagues in the chip department think that theoretically turning it off should not be a problem,
        but turning it on will not have any impact on power consumption.
        Considering that previous chips were always power on this bit, so here will keep it power on too.
    */

    pm_24mrc_power_down_if_unused();
}

/**
 * @brief   active mode CORE/SRAM output trim definition
 * @note    The voltage values of the following gears are all theoretical values, and there may be deviations between the actual and theoretical values.
 *          The CORE_0P7V_SRAM_0P8V_BB_0P7V_CONFIG is not opened to user after evaluate. 
 *          As we know, reducing voltage can reduce power consumption in some extent, but for this gear, the benefits may not be significant. 
 *          At this gear, it will takes nearly 150us more time to enter sleep mode, which is unacceptable in most scenarios.
 */
typedef enum {
    // CORE_0P7V_SRAM_0P8V_BB_0P7V_CONFIG = ((0x06 << 8) | (0x04 << 4)),/**< multi ldo mode  0.94V-LDO/DCDC 0.7V_CORE 0.8V_SRAM 0.7V BB*/
    CORE_0P8V_SRAM_0P8V_BB_0P8V_CONFIG = ((0x0a << 8) | (0x04 << 4)),/**< dig ldo mode  0.94V-LDO/DCDC 0.8V_CORE 0.8V_SRAM 0.8V BB*/
    CORE_0P9V_SRAM_0P9V_BB_0P9V_CONFIG = ((0x0e << 8) | (0x06 << 4)),/**< dig ldo mode  1.05V-LDO/DCDC 0.9V_CORE 0.9V_SRAM 0.9V BB*/
}pm_core_sram_bb_voltage_e;

#define ALG0X0E_DEFAULT_CONFIG      0x44
#define ALG0X0F_DEFAULT_CONFIG      0xa4

#define AUDIO_EMA_DMA_COMMON_CFG(i) \
    .dma_chain_ctl =  M2M_DMA_CFG|FLD_DMA_CHANNEL_ENABLE|FLD_DMA_CHANNEL_TC_MASK,\
    .dma_chain_dst_addr = AUDIO_SRAM_EMA_ADDR,\
    .dma_chain_data_len = AUDIO_SRAM_EMA_DATA_LEN,\
    .dma_chain_src_addr = (unsigned int) dvdd_config[i].audio_sram_ema

#define USB_EMA_DMA_COMMON_CFG(i) \
    .dma_chain_ctl =  M2M_DMA_CFG|FLD_DMA_CHANNEL_ENABLE|FLD_DMA_CHANNEL_TC_MASK,\
    .dma_chain_dst_addr = USB_SRAM_EMA_ADDR,\
    .dma_chain_data_len = USB_SRAM_EMA_DATA_LEN,\
    .dma_chain_src_addr = (unsigned int) dvdd_config[i].usb_sram_ema

#define D25F_EMA_DMA_COMMON_CFG(i) \
    .dma_chain_ctl =  M2M_DMA_CFG|FLD_DMA_CHANNEL_ENABLE|FLD_DMA_CHANNEL_TC_MASK,\
    .dma_chain_dst_addr = D25F_SRAM_EMA_ADDR,\
    .dma_chain_data_len = D25F_SRAM_EMA_DATA_LEN,\
    .dma_chain_src_addr = (unsigned int) dvdd_config[i].d25f_encrypt_decrypt_sram_ema

#define DELAY_DMA_COMMON_CFG \
    .dma_chain_ctl =  ANALOG_FIXED_ADDR_TX_DMA_CFG|FLD_DMA_CHANNEL_ENABLE|FLD_DMA_CHANNEL_TC_MASK,\
    .dma_chain_src_addr = (unsigned int)&useless_data,\
    .dma_chain_dst_addr = ANALOG_DATA_REG_ADDR,\
    .dma_chain_data_len = sizeof(useless_data)*4

#define VOL_DMA_COMMON_CFG(i) \
    .dma_chain_ctl =  ANALOG_INC_ADDR_TX_DMA_CFG|FLD_DMA_CHANNEL_ENABLE|FLD_DMA_CHANNEL_TC_MASK,       \
    .dma_chain_dst_addr = ANALOG_DATA_REG_ADDR,\
    .dma_chain_data_len = VOL_DATA_LEN/4,\
    .dma_chain_src_addr = (unsigned int)dvdd_config[i].vol

#define CORE_SRAM_BB_MAX_CONFIG_NUM              2
#define CHANIN_NODE_CNT                          6
#define VOL_DATA_LEN                             4                        //Must be a multiple of four

typedef struct {
      unsigned char vol[VOL_DATA_LEN] __attribute__((aligned(4)));
      unsigned char audio_sram_ema[AUDIO_SRAM_EMA_DATA_LEN];
      unsigned char usb_sram_ema[USB_SRAM_EMA_DATA_LEN];
      unsigned char d25f_encrypt_decrypt_sram_ema[D25F_SRAM_EMA_DATA_LEN];
}dvdd_config_t;

const dvdd_config_t dvdd_config[CORE_SRAM_BB_MAX_CONFIG_NUM] __attribute__((section(".flash_data")))=
{
    //CORE_0P8V_SRAM_0P8V_BB_0P8V
    {
       .vol = {0x0e, ((ALG0X0E_DEFAULT_CONFIG & 0x88) | (CORE_0P8V_SRAM_0P8V_BB_0P8V_CONFIG & 0xff) | SPD_LDO_TRIM_0P75V),
                0x0f, (ALG0X0F_DEFAULT_CONFIG & 0x08) | ((CORE_0P8V_SRAM_0P8V_BB_0P8V_CONFIG & 0xff00) >> 4) | RET_LDO_TRIM_0P75V},
        .audio_sram_ema = {0x04, 0x18},
        .usb_sram_ema = {0x04, 0x18},
       .d25f_encrypt_decrypt_sram_ema = {0x02, 0x19, 0x04, 0x18},
    },

    //CORE_0P9V_SRAM_0P9V_BB_0P9V
    {
       .vol = {0x0e, ((ALG0X0E_DEFAULT_CONFIG & 0x88) | (CORE_0P9V_SRAM_0P9V_BB_0P9V_CONFIG & 0xff) | SPD_LDO_TRIM_0P75V),
                0x0f, (ALG0X0F_DEFAULT_CONFIG & 0x08) | ((CORE_0P9V_SRAM_0P9V_BB_0P9V_CONFIG & 0xff00) >> 4) | RET_LDO_TRIM_0P75V},
        .audio_sram_ema = {0x0b, 0x08},
        .usb_sram_ema = {0x0b, 0x08},
       .d25f_encrypt_decrypt_sram_ema = {0x02, 0x09, 0x0b, 0x08},
    },
};

const unsigned char useless_data[4]__attribute__((section(".flash_data"))) __attribute__((aligned(4)))={0xff, 0xff, 0xff, 0xff};

const dma_chain_config_t dvdd_set_to_0p8_dma_chain[CHANIN_NODE_CNT] __attribute__((section(".flash_data")))= \
{
    {
        DELAY_DMA_COMMON_CFG,
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p8_dma_chain + 1),
    },
    {
        AUDIO_EMA_DMA_COMMON_CFG(CORE_0P8V_SRAM_0P8V_BB_0P8V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p8_dma_chain + 2),
    },
    {
        USB_EMA_DMA_COMMON_CFG(CORE_0P8V_SRAM_0P8V_BB_0P8V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p8_dma_chain + 3),
    },
    {
        D25F_EMA_DMA_COMMON_CFG(CORE_0P8V_SRAM_0P8V_BB_0P8V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p8_dma_chain + 4),
    },
    {
        VOL_DMA_COMMON_CFG(CORE_0P8V_SRAM_0P8V_BB_0P8V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p8_dma_chain + 5),
    },
    {
        DELAY_DMA_COMMON_CFG,
        .dma_chain_llp_ptr = 0,
    },
};

const dma_chain_config_t dvdd_set_to_0p9_dma_chain[CHANIN_NODE_CNT]__attribute__((section(".flash_data")))= \
{
    {
        DELAY_DMA_COMMON_CFG,
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p9_dma_chain + 1),
    },
    {
        VOL_DMA_COMMON_CFG(CORE_0P9V_SRAM_0P9V_BB_0P9V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p9_dma_chain + 2),
    },
    {
        AUDIO_EMA_DMA_COMMON_CFG(CORE_0P9V_SRAM_0P9V_BB_0P9V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p9_dma_chain + 3),
    },
    {
        USB_EMA_DMA_COMMON_CFG(CORE_0P9V_SRAM_0P9V_BB_0P9V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p9_dma_chain + 4),
    },
    {
        D25F_EMA_DMA_COMMON_CFG(CORE_0P9V_SRAM_0P9V_BB_0P9V),
        .dma_chain_llp_ptr = (unsigned int)(dvdd_set_to_0p9_dma_chain + 5),
    },
    {
        DELAY_DMA_COMMON_CFG,
        .dma_chain_llp_ptr = 0,
    },
};

static void pm_set_dvdd_restore_cfg(dma_chn_e chn, unsigned char swire, unsigned char dma_mask){
    if(swire){
        swire_slave_en();
    }
    if(!dma_mask){
        dma_clr_irq_mask(chn, TC_MASK);
    }
}

static unsigned char pm_set_dvdd_is_correct(const dma_chain_config_t * dma_chain_p, unsigned char node_cnt){
    unsigned char i=0, j=0;
    unsigned char ref, rcv;
    //1.in preparation to enter wfi, there may be access ram, the first node waits to enter wfi by configuring useless data.
    //2.when the dvdd voltage is set up, it takes time to stabilize, by the last node sending useless data as a delay.
    for(i = 1; i < node_cnt - 1; i++){
         if(dma_chain_p[i].dma_chain_dst_addr == (ANALOG_DATA_REG_ADDR)){
            for(j = 0; j < (dma_chain_p[i].dma_chain_data_len) * 2; j++)
            {
                unsigned int ana_addr = 0;
                ana_addr = *((unsigned char*)dma_chain_p[i].dma_chain_src_addr + (2 * j + 0));
                rcv = analog_read_reg8(ana_addr);
                ref = *((unsigned char*)dma_chain_p[i].dma_chain_src_addr + (2 * j + 1));
                if(rcv != ref) {
                   return 1;
                }
            }
        }else{
            for(j = 0; j < dma_chain_p[i].dma_chain_data_len; j++){
                rcv = read_reg8(dma_chain_p[i].dma_chain_dst_addr + j);
                ref = *((unsigned char*)dma_chain_p[i].dma_chain_src_addr + j);
                if(rcv != ref) {
                    if(dma_chain_p[i].dma_chain_dst_addr == (D25F_SRAM_EMA_ADDR)){
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

unsigned int g_dvdd_vol = CORE_0P8V_SRAM_0P8V_BB_0P8V;

/**
 * @brief       This function serves to set dvdd
 * @param[in]   vol      - CORE_0P8V_SRAM_0P8V_BB_0P8V /CORE_0P9V_SRAM_0P9V_BB_0P9V.
 *                       - the 0.8v/0.9v confirms which of the pm_core_sram_bb_voltage_e enumeration is configured, and then assigns the value to the macro CORE_0P8V_SRAM_0P8V_BB_0P8V_CONFG/CORE_0P9V_SRAM_0P9V_BB_0P9V_CONFG.
 * @param[in]   chn      - dma channel.
 * @param[in]   dma_timeout_us - wait dma all chn complete timeout.
 * @return      DRV_API_SUCCESS - successful;
 *              DRV_API_INVALID_PARAM - equal to the current voltage configuration or dvdd1_dvdd2_vol error;
 *              DRV_API_FAILURE - core error(need contains all the cores used);
 *              DRV_API_TIMEOUT - wait for dma all chn idle timeout to exit;
 *              DRV_API_OTHER_ERROR - clear all interrupt requests failed;
 * @note        1.If the voltage goes up, after calling the interface first, then adjust the frequency;
 *                If the voltage goes down, adjust the frequency first, then call the interface;
 *              2.When adjusting this voltage, no access ram operation is allowed, so it will wait for dma idle in this interface,
 *                modifying dma_timeout_us won't work if there are dma chains working all the time, and needs to be turned off by the upper layers themselves depending on the situation.
 *              3.When adjusting this voltage, the mcu will be stalled because the ram cannot be operated, use the dma method to modify the dvdd configuration and wake up the d25f with this dma interrupt,
 *                so will turns off the general interrupt and clears all interrupt requests.
 *              4.When adjusting this voltage, no access ram operation is allowed, disable swire.
 *              5.If the check configuration fails, reboot.
 */
drv_api_status_e pm_set_dvdd(pm_power_cfg_e vol, dma_chn_e chn, unsigned int dma_timeout_us){

    if(g_dvdd_vol == vol){
        return DRV_API_INVALID_PARAM;
    }

    pm_24mrc_power_up();

    /* 1. adjust 1p8v_0.94v voltage. */
    /*
    * In the A0 version of this chip, it was found that the actual output voltage was lower than the set value,
    * and there were significant differences between different chips.(ISSUE:TER-31)
    * In the A1 version of this chip, the issue was fixed so that the following trim code were commented out.
    * (added by jilong.liu at 20240226, updated at 20240517)
    */

    /*
    * The core voltage comes from AVDD 0.94V and DVDD 0.94V, while AVDD and DVDD come from dcdc_ldo0p94.
    * When increase the core voltage to 0.9V from 0.7/0.8v, it is necessary to ensure that the dcdc_ldo0p94 
    * is set to 1.1 (the highest gear) before increase the core voltage.
    */
    if (vol == CORE_0P9V_SRAM_0P9V_BB_0P9V) {
        pm_set_vdd0p94(CAL_0P94V_TO_1P05V);
    }

    //turn off the interrupt source and save,the reason why it is placed at the front of the interface:
    //To prevent the interrupt status flag bit from going up before the interrupt source is turned off, which will cause the plic_clr_all_request()
    //interface to clear the interrupt and cause an exception to be returned.
    plic_irqs_preprocess_for_wfi(1, FLD_MIE_MEIE);

    //waiting for dma to finish.
    if(dma_wait_for_all_chn_to_complete(dma_timeout_us)){
        plic_irqs_postprocess_for_wfi();
        return  DRV_API_TIMEOUT;
    }

    //Open dma interrupt source and clear interrupt.
    plic_interrupt_enable(IRQ_DMA);
    unsigned int clr_plic_request_result = 0;
    unsigned char clr_dma_irq_result = dma_clr_all_irq_status();
    clr_plic_request_result = plic_clr_all_request();
    if((clr_plic_request_result == 0 ) && (clr_dma_irq_result == DRV_API_FAILURE))
    {
        plic_irqs_postprocess_for_wfi();
        return DRV_API_OTHER_ERROR;
    }
    //Turn off swire and save
    unsigned char swire = 0;
    if(swire_slave_is_init()){
        swire = 1;
        swire_slave_dis();
    }
    //Initialize dma configuration, dma enable, dma mask save.
    const dma_chain_config_t *dvdd_dma_chain = NULL;
    if(vol == CORE_0P9V_SRAM_0P9V_BB_0P9V){
        dvdd_dma_chain = dvdd_set_to_0p9_dma_chain;
    } else if(vol == CORE_0P8V_SRAM_0P8V_BB_0P8V){
        dvdd_dma_chain= dvdd_set_to_0p8_dma_chain;
    }
    unsigned char dma_mask = dma_is_irq_mask(chn, TC_MASK);

    /* 2. adjust dig ldo and sram voltage. */
    dma_write_reg(chn, (const dma_chain_config_t *)dvdd_dma_chain, CHANIN_NODE_CNT);
    //wfi
    core_entry_wfi_mode();
    dma_write_reg_is_complete();
    //check
    if(pm_set_dvdd_is_correct(( const dma_chain_config_t *)dvdd_dma_chain, CHANIN_NODE_CNT)){
        protected_sys_reboot();
    }
    //swire recovery, dma tc mask recovery
    pm_set_dvdd_restore_cfg(chn, swire, dma_mask);
    //clear flag bit, interrupt source recovery, Total interrupt recovery, core clk recovery
    dma_clr_tc_irq_status(BIT(chn));
    plic_irqs_postprocess_for_wfi();

    /*
    * The core voltage comes from AVDD 0.94V and DVDD 0.94V, while AVDD and DVDD come from dcdc_ldo0p94.
    * When decrease the core voltage to 0.8V from 0.9v, it is necessary to ensure that the dcdc_ldo0p94 
    * is set to 0.94 (the default gear) after decrease the core voltage.
    */
    if (vol == CORE_0P8V_SRAM_0P8V_BB_0P8V) {
        pm_set_vdd0p94(CAL_0P94V_TO_0P95V);
    }

    g_dvdd_vol = vol;

    pm_24mrc_power_down_if_unused();

    return DRV_API_SUCCESS;
}

/**
 * @brief       This function serves to test different voltages from pd3.
 * @param[in]   mux_sel - select different voltages from pd3.
 * @return      none.
 */
void pm_set_probe_vol_to_pd3(pm_vol_mux_sel_e mux_sel)
{
    /**
     * afe3V-reg17<6:4>:test_mux_sel_second =1,test voltage from GPIO_PD3.
     * Configurations to other values are used internally by the chip designer as debug signals.
     */
    analog_write_reg8(0x11, (analog_read_reg8(0x11) & 0x88) | 0x10 | mux_sel);
   /**
    * afe3V-reg17<2:0>      value           note
    * ---------------------------------------------------------------------------
    *                       3'b000          N/A
    *                       3'b001          vdd_bb
    *                       3'b010          vdd_bbpll
    *                       3'b011          vdd_core
    *                       3'b100          vdd_retram
    *                       3'b101          vdd_iso
    *                       3'b110          vdd_ram
    *                       3'b111          used internally as a debug signal.
    */
    gpio_function_en(GPIO_PD3);
    gpio_output_dis(GPIO_PD3);
    gpio_input_en(GPIO_PD3);
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
_attribute_ram_code_com_sec_noinline_ void pm_update_status_info(unsigned char clr_en)
{
    if(g_pm_status_info.mcu_status == MCU_DEEPRET_BACK){
        return;
    }

    unsigned char wd_clr0 = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);
    unsigned char poweron_clr0 = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0);

    if(wd_clr0 & POWERON_FLAG){
        if(poweron_clr0 & REBOOT_FLAG){
            g_pm_status_info.mcu_status = MCU_SW_REBOOT_BACK;
            if(wd_get_status()){
                g_pm_status_info.mcu_status = MCU_HW_REBOOT_TIMER_WATCHDOG;
            }else{
                g_pm_system_reboot_event = (poweron_clr0>>1);
            }
            if(clr_en == 1){
                analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, wd_clr0 & (~POWERON_FLAG));
            }
        }else{
            g_pm_status_info.mcu_status = MCU_POWER_ON;
            if(wd_32k_get_status()){
                g_pm_status_info.mcu_status = MCU_HW_REBOOT_32K_WATCHDOG;
            }
            if(clr_en == 1){
                analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, wd_clr0 & (~POWERON_FLAG));
                analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, REBOOT_FLAG);
            }
        }
    }else{
        g_pm_status_info.mcu_status = MCU_DEEP_BACK;
    }
}

/**
 * @brief       This function serves to set system power mode.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC).
 * @return      none.
 * @note        pd_dcdc_ldo_sw<1:0>, dcdc & bypass ldo status bits:
                    dcdc_0p94   dcdc_1p8     ldo_0p94    ldo_1p8
                00:     N           N           Y           Y
                01:     Y           N           N           Y
                10:     Y           N           N           N
                11:     Y           Y           N           N
 */
_attribute_ram_code_com_sec_noinline_ void pm_set_power_mode(power_mode_e power_mode)
{
    if(g_pm_status_info.mcu_status != MCU_STATUS_DEEPRET_BACK)
    {
        /* 
         * In A1 version, this bit need to set 0 before turning on DCDC mode. Otherwise, the DCDC mode cannot be used.
         * In A2 version, the default value of this bit is 1, but added a reverser internally so not need to set.
         * (add by jilong.liu, confirmed by wenfeng.lou 20241129)
         */
        if(g_chip_version == CHIP_VERSION_A1)
        {
            /*
             *                      poweron_dft:    0x83 -> 0x82.
             *      bit                     note
             * ---------------------------------------------------------------------------
             * <0>:dcdc_cal_twohigh_en,     default:1,->0 disable calibrate the logic bug when two EA output is high
             * This will reduce power consumption and has little impact to operate early or late.
             * (add by weihua.zhang, confirmed by wenfeng.lou 20240816)
             */
            analog_write_reg8(areg_aon_0x02, analog_read_reg8(areg_aon_0x02) & (~FLD_DCDC_CAL_TWOHIGH_EN));
        }

        g_areg_aon_0a = analog_read_reg8(areg_aon_0x0a);
    }

    if((g_areg_aon_0a & 0x03) != power_mode)
    {
        //The power-on process of a DCDC requires a 24M rc clock.(add by weihua.zhang, confirmed by wenfeng.lou 20240903)
        if(LDO_0P94_LDO_1P8 != power_mode){
            pm_24mrc_power_up();

            /*
            * DCDC voltage increase one level from 0.94/1.8 to 0.963/1.83.
            * (added by jilong liu, confirmed by wenfeng.lou at 20241129)
            */
            analog_write_reg8(areg_aon_0x02, analog_read_reg8(areg_aon_0x02) | FLD_DCDC_VCOMP_VOS_PN);//calibrate the comparator offset voltage enable
            analog_write_reg8(areg_aon_0x01, 0x84);//adjust offset to optimize efficiency
        }

        g_areg_aon_0a = (g_areg_aon_0a & 0xfc) | power_mode;
        analog_write_reg8(areg_aon_0x0a, g_areg_aon_0a);

        if(LDO_0P94_LDO_1P8 != power_mode){
            if(!g_24m_rc_is_used){
                core_cclk_delay_tick((unsigned long long)(25 * sys_clk.cclk));
            }
            pm_24mrc_power_down_if_unused();
        }
    }
}

/**
 * @brief      This function serves to update vdd0p94 and vddo1p8 current value from global variable.
 * @return     pm_cal_0p94v_e - the vdd 0.94 current voltage level.
 * @note       This function must call after sys_init, otherwise the return value may be incorrect.
 */
_attribute_ram_code_com_sec_noinline_ pm_cal_0p94v_e pm_get_vdd0p94_level(void) //BLE SDK use
{
    return g_pm_vdd0p94_level;
}

/**
 * @brief       This function serves to trim dcdc/ldo 0.94v.
 * @param[in]   value - the voltage to be set.
 * @return      none
 */
_attribute_ram_code_com_sec_noinline_ void pm_set_vdd0p94(pm_cal_0p94v_e value)
{
    if (value == CAL_0P94V_TO_0P95V) {
        pm_set_vddf_vdd0p94(TRIM_VDDF_TO_1P80V, g_pm_cal_vdd0p94_info.ldo_0p95v, TRIM_VDDF_TO_1P80V, g_pm_cal_vdd0p94_info.dcdc_0p95v);
    } else if (value == CAL_0P94V_TO_1P05V) {
        pm_set_vddf_vdd0p94(TRIM_VDDF_TO_1P80V, g_pm_cal_vdd0p94_info.ldo_1p05v, TRIM_VDDF_TO_1P80V, g_pm_cal_vdd0p94_info.dcdc_1p05v);
    }

    g_pm_vdd0p94_level = value;
}

/**
 * @brief       When an error occurs, such as the crystal does not vibrate properly, the corresponding recording and reset operations are performed.
 * @param[in]   reboot_reason  - The bit to be configured in the power on buffer.
 * @param[in]   all_ramcode_en  - Whether all processing in this function is required to be ram code.
 * @return      none.
 */
_attribute_ram_code_com_sec_optimize_o2_noinline_ void pm_sys_reboot_with_reason(pm_sw_reboot_reason_e reboot_reason, unsigned char all_ramcode_en)
{
    pm_set_reboot_reason(reboot_reason);

#if(PM_DEBUG)
    while(1);
#endif

    if(all_ramcode_en == 0x00)
    {
        DISABLE_BTB;
        sys_reboot_ram();
        ENABLE_BTB;
    }
    else
    {
        sys_reset_all();
        while(1);
    }
}
