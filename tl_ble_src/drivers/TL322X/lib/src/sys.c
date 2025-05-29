/********************************************************************************************************
 * @file    sys.c
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
#include "lib/include/pm/pm_internal.h"
#include "lib/include/pm/pm.h"
#include "lib/include/sys.h"
#include "lib/include/core.h"
#include "compiler.h"
#include "lib/include/analog.h"
#include "gpio.h"
#include "lib/include/mspi.h"
#include "lib/include/stimer.h"
#include "lib/include/rf/rf_internal.h"


unsigned int g_chip_version = 0;



/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void sys_reboot_ram(void)
{
    core_interrupt_disable(); //It must be inlined to ensure that the text segment cannot be entered.
    mspi_stop_xip();
    sys_reset_all();
    while (1)
        ;
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
 * @brief     this function servers to manual set crystal.
 * @return    none.
 * @note      This function can only used when cclk is 24M RC cause the function execution process will power down the 24M crystal.
 */
_attribute_ram_code_sec_optimize_o2_noinline_ void crystal_manual_settle(void)
{
    analog_write_reg8(0x65, analog_read_reg8(0x65) | 0x40); //0x65<6>: write 1 to reset xtal quick start cnt

    unsigned char ana_05 = analog_read_reg8(0x05);          //0x05<3>: 24M_xtl_pd
    analog_write_reg8(0x05, ana_05 | 0x08);                 //<3>1b'1: Power down 24MHz XTL oscillator

#if PM_MANUAL_SETTLE_DEBUG
    core_cclk_delay_tick((unsigned long long)(sys_clk.cclk * 400000));
    gpio_function_en(GPIO_PB4);
    gpio_output_en(GPIO_PB4);
    gpio_input_dis(GPIO_PB4);
    gpio_set_high_level(GPIO_PB4);
#endif

    analog_write_reg8(0x05, ana_05 & 0xf7);                 //<3>1b'0: Power up 24MHz XTL oscillator
}

/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @param[in]   cap         - This parameter is used to determine whether to close the internal capacitor.
 * @attention   If vbat_v is set to VBAT_MAX_VALUE_LESS_THAN_3V6, then gpio_v can only be set to GPIO_VOLTAGE_3V3.
 * @return      none
 * @note        For crystal oscillator with slow start-up or poor quality, after calling this function, 
 *              a reboot will be triggered(whether a reboot has occurred can be judged by using pm_update_status_info() and pm_get_sw_reboot_event()).
 *              For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface 
 *              to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *              When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 */
_attribute_ram_code_sec_noinline_ void sys_init(power_mode_e power_mode, vbat_type_e vbat_v, cap_typedef_e cap)
{
#if 0
    /*
     * Reset function will be cleared by set "1",which is different from the previous configuration.
     * This setting turns off the TRNG and NPE modules in order to test power consumption.The current
     * decrease about 3mA when those two modules be turn off.changed by zhiwei,confirmed by kaixin.20200828.
     */
    reg_rst = 0xffffffff;
    reg_clk_en = 0xffffffff;

    reg_rst_1 = 0xffffffff;
    reg_clk_en_1 = 0xffffffff;
#else
    /*
     * Reset function will be cleared by set "1".
     * Turn off the following modules compared to the default values: sspi, brom.
     * Turn on the following modules compared to the default values: uart0, uart1, uart2, uart3, stimer, dma, dma1, algm, timer, pwm, timern22.
     * Overall, the enabled modules here includes: dbgen, swires, stimer, dma, dma1, alg, algm, machinetime, mcu, lm, trace, mspi, cclk.
     * uart0, uart1, uart2, uart3, uart4, efuse, pwm, timer, timern22, rram, mailbox.
     * among them, the uart, pwm, and timer modules do not have appropriate positions to enable them in the module interface, so they are also enabled here.
     */
    reg_rst    = 0x92b90ff4;   //reset_0_dft:  0x96b88080 -> 0x92b90ff4
    reg_clk_en = 0x1ab12ff4;   //clken_0_dft:  0x1e30a080 -> 0x1ab12ff4

    reg_rst_1    = 0x80800a04; //reset_1_dft:  0x00800004 -> 0x80800a04
    reg_clk_en_1 = 0x80800a44; //clken_1_dft:  0x00800044 -> 0x80800a44
#endif

    reg_rram_ctrl0 &= (~FLD_REG_EN_IO_RRAM);
    /*
     * 1. Before calling the function pm_wait_xtal_ready, you need to ensure that the cclk is set to 24M,
     * otherwise a reboot may occur (for example, the following use case: when the flash is running a cclk>24M program,
     * without powering down the chip to load a ram program, at this time the nop timing judgment in the pm_wait_xtal_ready function is incorrect,
     * which may lead to a reboot)(add by weihua.zhang, confirmed by kaixin 20230609)
     * 2. Before calling the function crystal_manual_settle, you need to ensure that the cclk is set to 24M,
     * cause the crystal_manual_settle will power down xtal first then power it up.(add by jilong.liu at 20240513)
     */
    analog_write_reg8(areg_aon_0x05, analog_read_reg8(areg_aon_0x05) & ~(FLD_24M_RC_PD));
    clock_set_all_clock_to_default();

    //A0:0x00
    g_chip_version = read_reg8(0x14083d);

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20250407)
    if (cap != INTERNAL_CAP_XTAL24M)
    {
        rf_turn_off_internal_cap();
    }

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

#if PM_XTAL_READY_TIME
    gpio_function_en(GPIO_PB4);
    gpio_output_en(GPIO_PB4);
    gpio_input_dis(GPIO_PB4);
    gpio_set_high_level(GPIO_PB4);
#endif

    g_areg_aon_7f = analog_read_reg8(areg_aon_0x7f);
    if (!(g_areg_aon_7f & FLD_BOOTFROMBROM))
    {
        g_pm_status_info.mcu_status = MCU_DEEPRET_BACK;
    }

    pm_set_power_mode(power_mode);

    /*
     *                      poweron_dft:    0xfb -> 0xbb.
     *      pd_bit                      note
     * ---------------------------------------------------------------------------
     * <0>:pd_nvt_1p25  default:1,->1 power down 1p25 native transistor.
     * <1>:pd_nvt_1p8   default:1,->1 power down 1p8 native transistor.
     * <6>:mscn_pullup_res_enb  default:1,->0 enable 1M pullup resistor for mscn PAD.
     */
    /*
     * After waking up, it is not safe to power supply both the native LDO and the normal LDO together.
     * Therefore, this code will be processed in advance here to reduce the shared power supply time.(add by jilong.liu, 20240221)
     */
    analog_write_reg8(areg_aon_0x0b, (analog_read_reg8(areg_aon_0x0b) & (~FLD_MSCN_PULLUP_RES_ENB)) | (FLD_PD_NVT_1P25 | FLD_PD_NVT_1P8));

    /*
     *                      poweron_dft:    0xff -> 0xff.
     *      pd_bit                      note
     * ---------------------------------------------------------------------------
     * <0>:pd_bbpll_ldo  default:1,->0 Power up BBPLL LDO.
     * <3>:pd_vbat_sw  default:1,->vbat_v Power down of bypass switch(VBAT LDO).
     * <6>:spd_ldo_pd  default:1,->1 Power down of suspend LDO.
     * <7>:dig_ret_pd  default:1,->1 Power down of retention LDO.
     */
    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) | FLD_PD_SPD_LDO | FLD_PD_DIG_RET_LDO | FLD_PD_VBAT_SW) & ~(vbat_v));

    g_pm_status_info.wakeup_src    = pm_get_wakeup_src();
    g_pm_status_info.is_pad_wakeup = (g_pm_status_info.wakeup_src & BIT(0));

    g_pm_vbat_v = vbat_v >> 3;

    /*
     * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
     * Because the stimer is necessary for the pm_wait_xtal_ready.
     * (add by jilong.liu, confirmed by wenfeng.lou 20250407)
     */
    analog_write_reg8(areg_0x10c, analog_read_reg8(areg_0x10c) | FLD_XO_EN_CLK_ANA);

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_xtal_ready(0x00);

    if (g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK) {
        pm_stimer_recover();
    } else {
#if SYS_TIMER_AUTO_MODE
        stimer_enable(STIMER_AUTO_MODE_W_TRIG, 0x01);
        stimer_32k_tracking_enable(); //enable 32k cal
#else
        stimer_enable(STIMER_MANUAL_MODE, 0x01);
        stimer_32k_tracking_enable(); //enable 32k cal
#endif
    }
}

/**
 * @brief      This function serves to initialize n22 core system.
 * @param[in]  addr - start up address
 * @return     none
 * @note        Only after calling this function can other N22 related functions be called.
 *              Otherwise, other N22 function settings will not take effect.
 */
void sys_n22_init(unsigned int addr)
{
    /*
     * The N22 module power source is also from module ZB(baseband), so need to power it up here.
     * (added by jilong.liu, confirmed by jianzhi.chen at 20240229)
     */
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);
    BM_SET(reg_rst5, FLD_RST5_MAILBOX);
    BM_SET(reg_clk_en5, FLD_CLK5_MAILBOX_EN);
    reg_n22_rst_vector = addr;
}

/**
 * @brief       This function serves to start n22 core system.
 * @return      none
 */
void sys_n22_start(void)
{
    reg_rst7 |= FLD_RST7_N22;
    reg_clk_en7 |= FLD_CLK7_N22_EN | FLD_CLK7_N22PLMT_EN;
}
