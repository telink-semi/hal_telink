/********************************************************************************************************
 * @file    sys.c
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
#include "lib/include/sys.h"
#include "lib/include/core.h"
#include "compiler.h"
#include "lib/include/analog.h"
#include "gpio.h"
#include "lib/include/mspi.h"
#include "lib/include/stimer.h"
#include "lib/include/rf/rf_common.h"


unsigned int                                 g_chip_version = 0;
_attribute_data_retention_sec_ unsigned char g_pm_vbat_v;

//Protection Code checking related macro
#define SDK_VERSION_DRIVER 0
#define SDK_VERSION_IGNORE 0xff // Used for internal debugging, ignoring SDK version restrictions.

/*note:When releasing externally, this macro may need to be modified.
       Special: The rule for the driver is that all chips are available,
       so this macro can be left unchanged when the driver is released externally*/
#define SDK_VERSION_SELECT SDK_VERSION_IGNORE

/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_ram_code_sec_optimize_o2_noinline_ void sys_reboot_ram(void)
{
    core_interrupt_disable(); //It must be inlined to ensure that the text segment cannot be entered.
    mspi_stop_xip();
    sys_reset_all();
    while (1);
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
    analog_write_reg8(0x05, ana_05 & 0xf7);                 //<3>1b'0: Power up 24MHz XTL oscillator
}

#if 0
/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC_LDO)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @attention   If vbat_v is set to VBAT_MAX_VALUE_LESS_THAN_3V6, then gpio_v can only be set to GPIO_VOLTAGE_3V3.
 * @return      none
 * @note        -# For crystal oscillator with slow start-up or poor quality, after calling this function,
 *                 a reboot will be triggered(whether a reboot has occurred can be judged by using pm_update_status_info() and pm_get_sw_reboot_event()).
 *                 For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface
 *                 to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *                 When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 *              -# Before calling this interface, you need to ensure that the input function of PA2 is disable.
 *                 and in order to prevent errors, the PA2 input function is disabled on this interface.(BUT-53)
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
     * Turn on the following modules compared to the default values: uart0, uart1, uart2, stimer, dma, algm.
     * Overall, the enabled modules here includes: dbgen, swires, stimer, dma, algm, machinetime, mcu, lm, trace, mspi, cclk.
     * uart0, uart1, uart2, efuse, pwm, timer.
     * among them, the uart, pwm, and timer modules do not have appropriate positions to enable them in the module interface, so they are also enabled here.
     */
    reg_rst    = 0x92390ed4;   //reset_0_dft:  0x96388080 -> 0x92380ed4
    reg_clk_en = 0x1a312ef4;   //clken_0_dft:  0x1e30a0a0 -> 0x1a312ef4

    reg_rst_1    = 0x00000204; //reset_1_dft:  0x00000004 -> 0x00000204
    reg_clk_en_1 = 0x00000244; //clken_1_dft:  0x00000044 -> 0x00000244
#endif
    /*
     * 1. Before calling the function pm_wait_xtal_ready, you need to ensure that the cclk is set to 24M,
     * otherwise a reboot may occur (for example, the following use case: when the flash is running a cclk>24M program,
     * without powering down the chip to load a ram program, at this time the nop timing judgment in the pm_wait_xtal_ready function is incorrect,
     * which may lead to a reboot)(add by weihua.zhang, confirmed by kaixin 20230609)
     * 2. Before calling the function crystal_manual_settle, you need to ensure that the cclk is set to 24M,
     * cause the crystal_manual_settle will power down xtal first then power it up.(add by jilong.liu at 20240513)
     */
    analog_write_reg8(areg_aon_0x05, analog_read_reg8(areg_aon_0x05) & ~(FLD_24M_RC_PD)); //power on 24M RC
    clock_set_all_clock_to_default();

    //A0: 0x00, A1: 0x01
    g_chip_version = read_reg8(0x14083d);

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20240621)
    if (cap != INTERNAL_CAP_XTAL24M) {
        rf_turn_off_internal_cap();
    }

    if (g_chip_version == CHIP_VERSION_A0) {
        /* 
         * For version A0, the RC 24M is high to 32M, register ana0x16<1:0> needs to be configured as 2b'01, the frequency is about 25MHz. 
         * XTAL will kick by RC, so trim it before.(updated by jilong.liu, confirmed by yangya at 20240827)
         */
        analog_write_reg8(0x16, (analog_read_reg8(0x16) & 0xfc) | 0x01);
    }

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

    g_areg_aon_7f = analog_read_reg8(areg_aon_0x7f);
    if (!(g_areg_aon_7f & FLD_BOOTFROMBROM)) {
        g_pm_status_info.mcu_status = MCU_DEEPRET_BACK;
    }

    pm_set_power_mode(power_mode);

    /*
     *                      poweron_dft:    0x7f -> 0x3f.
     *      pd_bit                      note
     * ---------------------------------------------------------------------------
     * <0>:pd_nvt_0p94  default:1,->1 power down 0p94 native transistor.
     * <1>:pd_nvt_1p8   default:1,->1 power down 1p8 native transistor.
     * <6>:mscn_pullup_res_enb  default:1,->0 enable 1M pullup resistor for mscn PAD.
     */
    /*
     * After waking up, it is not safe to power supply both the native LDO and the normal LDO together.
     * Therefore, this code will be processed in advance here to reduce the shared power supply time.(add by jilong.liu, 20240221)
     */
    analog_write_reg8(areg_aon_0x0b, (analog_read_reg8(areg_aon_0x0b) & (~FLD_MSCN_PULLUP_RES_ENB)) | (FLD_PD_NVT_0P94 | FLD_PD_NVT_1P8));

    /*
     *                      poweron_dft:    0xff -> 0xff.
     *      pd_bit                      note
     * ---------------------------------------------------------------------------
     * <0>:pd_bbpll_ldo  default:1,->0 Power up BBPLL LDO.
     * <3>:pd_vbat_sw  default:1,->vbat_v Power down of bypass switch(VBAT LDO).
     * <6>:spd_ldo_pd  default:1,->1 Power down of suspend LDO.
     * <7>:dig_ret_pd  default:1,->0 Power up of retention LDO.
     */
    /*
     * For tl321x, the retention ldo should turn on before use cause it need more time to be stable.
     * The follow chip version will not change this situation.
     * (modified by jilong.liu, confirmed by wenfeng 20240620)
     */
    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) | FLD_PD_SPD_LDO | FLD_PD_VBAT_SW) & ~(FLD_PD_DIG_RET_LDO | vbat_v));

    g_pm_status_info.wakeup_src    = pm_get_wakeup_src();
    g_pm_status_info.is_pad_wakeup = (g_pm_status_info.wakeup_src & BIT(0));

    g_pm_vbat_v = vbat_v >> 3;

    if (g_chip_version == CHIP_VERSION_A0) {
        //For version A0, the theoretical value is low, and the current enumeration value is set after actual testing.
        pm_set_dig_ldo_voltage(DIG_LDO_TRIM_1P010V); //trim VDDDEC to 1V
        pm_set_1p25v(TRIM_1P25V_TO_1P25V);           //trim VDD1P25 to 1.25V
    }

    /*
    * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
    * Because the stimer is necessary for the pm_wait_xtal_ready.
    * (add by jilong.liu, confirmed by wenfeng.lou 20240513)
    */
    analog_write_reg8(areg_0x8c, 0x02);

    //Before powering up the zb, you need to make sure that the input function of PA2 is disable, otherwise it may cause the RF module to not have clock.
    //See jira for details:BUT-53.(add by weihua.zhang, confirmed by peng.hao 20250101)
    reg_gpio_pa_ie &= (~0x04);
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
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
//check protection code
#if (SDK_VERSION_SELECT != SDK_VERSION_IGNORE)
    efuse_check_protection_code(SDK_VERSION_SELECT); //0:driver sdk  0xff:sdk_version_ignore
#endif
}
#endif
/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/
