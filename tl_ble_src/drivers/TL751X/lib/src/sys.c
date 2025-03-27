/********************************************************************************************************
 * @file    sys.c
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
#include "lib/include/sys.h"
#include "core.h"
#include "compiler.h"
#include "analog.h"
#include "gpio.h"
#include "mspi.h"
#include "stimer.h"


unsigned int g_chip_version=0;

extern void pm_update_status_info(void);
extern void clock_baseband_pll_config(sys_bbpll_clk_e clk);

/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void sys_reboot_ram(void)
{
    core_interrupt_disable();   //It must be inlined to ensure that the text segment cannot be entered.
    mspi_stop_xip();
    sys_reset_all();
    while(1);
}

/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_text_sec_ void sys_reboot(void)
{
    //To prevent reboot when the mcu is prefetching, the flash receives an incorrect command, resulting in an exception of the flash.
    DISABLE_BTB;
    sys_reboot_ram();
    ENABLE_BTB;
}

/**
 * @brief       This function serves to initialize system.
 * @param[in]   vbat_v  - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 *                      - Please refer to vbat_type_e for specific usage precautions.
 * @return      none
 * @note        For crystal oscillator with slow start-up or poor quality, after calling this function, 
 *              a reboot will be triggered(whether a reboot has occurred can be judged by using PM_ANA_REG_POWER_ON_CLR_BUF0[bit1]).
 *              For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface 
 *              to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *              When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 */
void sys_init(vbat_type_e vbat_v)
{
#if 0
    /**
     * Reset function will be cleared by set "1",which is different from the previous configuration.
     * This setting turns off the TRNG and NPE modules in order to test power consumption.The current
     * decrease about 3mA when those two modules be turn off.changed by zhiwei,confirmed by kaixin.20200828.
     */
    reg_rst      =  0xffffffff;
    reg_clk_en   =  0xffffffff;
    reg_rst_1    =  0xffffffff;
    reg_clk_en_1 =  0xffffffff;
#else
    /**
     * Reset function will be cleared by set "1".
     * Turn off the following modules compared to the default values: sspi, brom.
     * Turn on the following modules compared to the default values: stimer, dma, algm, dma1.
     * Overall, the enabled modules here includes: dbgen, swires, stimer, dma, algm, machinetime, mcu, lm, dma1, trace, mspi, hclk, mailbox,
     * uart0, uart1, uart2, uart3, pwm, timer.
     * among them, the uart, pwm, and timer modules do not have appropriate positions to enable them in the module interface, so they are also enabled here,
     * which increases the power consumption by 0.07mA.
     */
    reg_rst      =  0x93390ed4;             //reset_0_dft:  0x96388080 -> 0x93380e80 -> 0x93390ed4
    reg_clk_en   =  0x13312ef4;             //clken_0_dft:  0x96388080 -> 0x1630a0a0 -> 0x13312ef4

    reg_rst_1    =  0x00000e00;             //reset_1_dft:  0x00000004 -> 0x00000800 -> 0x00000e00
    reg_clk_en_1 =  0x00000e40;             //clken_1_dft:  0x00000044 -> 0x00000840 -> 0x00000e40
#endif
    /*
     * 1. Before calling the function pm_wait_xtal_ready, you need to ensure that the cclk is set to 24M,
     * otherwise a reboot may occur (for example, the following use case: when the flash is running a cclk>24M program,
     * without powering down the chip to load a ram program, at this time the nop timing judgment in the pm_wait_xtal_ready function is incorrect,
     * which may lead to a reboot)(add by weihua.zhang, confirmed by kaixin 20230609)
     * 2. Before calling the function crystal_manual_settle, you need to ensure that the cclk is set to 24M,
     * cause the crystal_manual_settle will power down xtal first then power it up.(add by jilong.liu at 20240513)
     */
    clock_set_all_clock_to_default();

    /**
     * All other chips will manually switch crystal on and off during power-on initialization to prevent crash.
     * TL751X is not added because TL751X does not have this function.
     * (add by jilong.liu, confirmed by wenfeng.lou 20240320)
     */

    /*
     * This is the register for the XO block configuration, but the design of the big and small endian is reversed (where the bit<23:16> chip is reversed and needs to be switched to bit<16:23>).
     * Therefore, the register that originally needed to be set to 0x06 now needs to be set to 0x60.
     * This will solve the problem of the crystal oscillator not starting to vibrate when adjusting the internal capacitance of the crystal oscillator to a certain value.
     * (added by jilong.liu, confirmed by xuqiang.zhang at 20240301)
     */
    analog_write_reg8(0x10c, 0x60);

    /* Trimming of the XO frequency through the load capacitance of 6pF. */
    analog_write_reg8(0x10d, 0x5d);

    //A0: 0x00, A1: 0x10
    g_chip_version = read_reg8(0x14083d);
#if 1
    sys_set_vbat_type(vbat_v);//set VBAT voltage type
#endif

    pm_update_status_info();

    pm_wait_xtal_ready(0x00);//41.19us
    /**
      -# The correct order to switch pll clock is power down pll->clock_baseband_pll_config()->power on pll->pm_wait_bbpll_done().
      -# Currently for the A0 chip, if power down will introduce some other problems,so A0 processing for only open 192M to the user to use,
         and then the later chips will be handled in the normal order if the problem is fixed.
      -# The chip's default pll clock is 192M, but the corresponding vco_itrim is set to a value corresponding to 240M,
         Here clock_baseband_pll_config is called to ensure that the value of vco_itrim is correct
         (add by kaixin.chen, confirmed by yangya 20231218)
    */
    clock_baseband_pll_config(PLL_CLK_192M);

    pm_wait_bbpll_done();//25.41us

    if(g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK)
    {
//      pm_stimer_recover();
    }else{
#if SYS_TIMER_AUTO_MODE
        stimer_enable(STIMER_AUTO_MODE_W_TRIG, 0x01);
        stimer_32k_tracking_enable();   //enable 32k cal
#else
        stimer_enable(STIMER_MANUAL_MODE, 0x01);
        stimer_32k_tracking_enable();   //enable 32k cal
#endif
    }

    extern void cpu_wakeup_no_deepretn_back_init(void);
    cpu_wakeup_no_deepretn_back_init();
}

/**
 * @brief       This function serves to set vbat type. 
 * @param[in]   vbat_v  - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 *                      - Please refer to vbat_type_e for specific usage precautions.
 * @return      none
 */
void sys_set_vbat_type(vbat_type_e vbat_v)
{
    /**
        Turn on VBAT LDO bypass when VBAT is below 3.3V and turn off 3.3V LDO can prevent extra current. (add by jilong.liu, confirmed by lingyu 20231226)
        According to our company's usage, switching when VBAT is below 3.6V is also the same. 
        Therefore, when VBAT is less than 3.6V, bypass is turned on and VBAT LDO is turned off.
     */
    if (vbat_v == VBAT_MAX_VALUE_GREATER_THAN_3V6) {
        analog_write_reg8(0x05, analog_read_reg8(0x05) & 0xbf);//power up VBAT LDO
        analog_write_reg8(0x06, analog_read_reg8(0x06) | 0x08);//power down VBAT LDO bypass
    }else if (vbat_v == VBAT_MAX_VALUE_LESS_THAN_3V6) {
        analog_write_reg8(0x06, analog_read_reg8(0x06) & 0xf7);//power on VBAT LDO bypass
        analog_write_reg8(0x05, analog_read_reg8(0x05) | 0x40);//power down VBAT LDO
    }
}

/**
 * @brief       This function serves to initialize dsp core system.
 * @return      none
 * @note        Only after calling this function can other DSP related functions be called. 
 *              Otherwise, other DSP function settings will not take effect.
 */
void sys_dsp_init(void)
{
    pm_set_dig_module_power_switch(FLD_PD_DSP_EN, PM_POWER_UP);

    reg_rst4 |= FLD_RST4_DSP;
    reg_clk_en4 |= FLD_CLK4_DSP_EN;
    reg_dsp_ctrl0 |= FLD_DSP_CTRL0_STALL;

    /*
        After set up sc_rst register, it need times for dsp system reset and clk to get ready.
        The rst_dsp_sync_n_o will cost 3 cycles clk_dsp_div + 1 cycle clk_dsp to release.
        The most time-consuming case: 
            D25F clock source select 24M RC, so the clk_dsp_div is 1/24M
            DSP clock divide 16 times from 24M RC, so the clk_dip is 1/(24/16)M = 1/1.5M
    */
    delay_us(2);//3 * 1/24M + 1 * 1/1.5M = 0.667us

    reg_dsp_rst0 |= FLD_DSP_EN;
    reg_dsp_clken0 |= FLD_DSP_CLK_EN;
}

/**
 * @brief       This function serves to start dsp core system.
 * @return      none
 */
void sys_dsp_start(void)
{
    reg_dsp_ctrl0 &= (~FLD_DSP_CTRL0_STALL);
}

/**
 * @brief      This function serves to initialize n22 core system.
 * @return     none
 * @note        Only after calling this function can other N22 related functions be called. 
 *              Otherwise, other N22 function settings will not take effect.
 */
void sys_n22_init(void)
{
    /* 
     * The N22 module power source is also from module ZB(baseband), so need to power it up here. 
     * (added by jilong.liu, confirmed by jianzhi.chen at 20240229)
     */
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    reg_rst4 |= FLD_RST4_AHB1;
    reg_clk_en4 |= FLD_CLK4_HCLK1_EN;

    reg_n22_rst0 |= FLD_RST0_N22_LM;
    reg_n22_clk_en0 |= FLD_CLK0_N22_LM_EN;  
}

/**
 * @brief       This function serves to start n22 core system.
 * @return      none
 */
void sys_n22_start(void)
{
    reg_n22_rst0 |= FLD_RST0_N22_CORE;
    reg_n22_clk_en0 |= FLD_CLK0_N22_CORE_EN;
}

/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/
