/********************************************************************************************************
 * @file    sys.c
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
#include "lib/include/sys.h"
#include "lib/include/otp.h"
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
_attribute_ram_code_sec_noinline_ void sys_reboot_ram(void)
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
    if ((g_chip_version != CHIP_VERSION_A0) && (g_chip_version != CHIP_VERSION_A1)) {
        /*
        *       bit                      default value               note
        * ---------------------------------------------------------------------------
        * <5:4>:reg_xo_force_amp_ana     default:0x00,->0x03 Increase crystal LDO trim voltage.
        * Since the A2 version changed the default meaning of crystal ldo trim voltage, the default voltage of crystal ldo trim of A2 chips 
        * is lower than the previous version, and some A2 chips do not work RF due to low crystal ldo trim voltage.So modify this configuration 
        * on the A2 version of the chip to resolve the RF anomaly.Add by zhiwei,confirmed by wenfeng.20241206
        */
        analog_write_reg8(0x8b, analog_read_reg8(0x8b) | 0x30);
    }
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

    analog_write_reg8(0x05, ana_05 & 0xf7); //<3>1b'0: Power up 24MHz XTL oscillator
}

/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @attention   If vbat_v is set to VBAT_MAX_VALUE_LESS_THAN_3V6, then gpio_v can only be set to GPIO_VOLTAGE_3V3.
 * @return      none
 * @note        For crystal oscillator with slow start-up or poor quality, after calling this function, 
 *              a reboot will be triggered(whether a reboot has occurred can be judged by using pm_update_status_info() and pm_get_sw_reboot_event()).
 *              For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface 
 *              to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *              When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 */
/**< BLE USED *****/
#include "ext_driver/driver_internal/ext_lib.h"
/**< BLE USED END */
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
     * Turn on the following modules compared to the default values: uart0, stimer, dma, algm.
     * Overall, the enabled modules here includes: dbgen, swires, stimer, dma, algm, machinetime, mcu, lm, trace, mspi, cclk.
     * uart0, uart1, uart2, pwm, timer.
     * among them, the uart, pwm, and timer modules do not have appropriate positions to enable them in the module interface, so they are also enabled here.
     */
    reg_rst    = 0x92390ed4;   //reset_0_dft:  0x96388080 -> 0x92380e80 -> 0x92390ed4
    reg_clk_en = 0x12312ef4;   //clken_0_dft:  0x1630a0a0 -> 0x12302ea0 -> 0x12312ef4

    reg_rst_1    = 0x00000204; //reset_1_dft:  0x00000004 -> 0x00000000 -> 0x00000204
    reg_clk_en_1 = 0x00000244; //clken_1_dft:  0x00000044 -> 0x000000c0 -> 0x00000244
#endif

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

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20240621)
    if (cap != INTERNAL_CAP_XTAL24M) {
        rf_turn_off_internal_cap();
    } else {
        /*
         * Updating the crystal internal capacitance value to adjust RF frequency offset.
         * This should be done before crystal_manual_settle().(added by chenxi, confirmed by wenfeng. 20240627)
         */
        rf_update_internal_cap(0x4c);
    }

    //A0: 0x00, A1: 0x80, A2: 0xc0
    //The A2 chip changes the default values of some analog registers to commonly configured values,
    //which saves the time of configuring registers during initialization.
    g_chip_version = read_reg8(0x14083d);

    if (g_chip_version == CHIP_VERSION_A1) {
        /*
         * Increase the current of the crystal oscillator to avoid the poor product failure to vibrate.
         * This should better be operate as soon as possible and must be done before the crystal_manual_settle.
         * (adjusted by jilong.liu, confirmed by wenfeng.lou at 20240221)
         */
        analog_write_reg8(areg_aon_0x4e, (analog_read_reg8(areg_aon_0x4e) & (~FLD_XO_ISEL_PMU)) | 0x28);
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
    /*
     * If back from deep ret sleep, there is no need to read or set calibration value cause they are all maintained.
     * If back from deep sleep, the calibration value need to read but not need to set cause they are all maintained.
     * For other case(power on or reboot), it's necessary to both read and set calibration value for both LDO and DCDC mode.
     */
    if (!(g_areg_aon_7f & FLD_BOOTFROMBROM)) {
        g_pm_status_info.mcu_status = MCU_DEEPRET_BACK;
    } else {
        //read from otp
        otp_get_vdd0p94_vddo1p8_calib_value();
    }

    pm_set_power_mode(power_mode);

    if (analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0) & POWERON_FLAG) {
        //power on or reboot
        pm_set_vdd0p94(CAL_0P94V_TO_0P95V);

        pm_set_vddo1p8(g_pm_cal_vddo1p8_info);
    }
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
     * <7>:dig_ret_pd  default:1,->1 Power down of retention LDO.
     */
    analog_write_reg8(areg_aon_0x06, (analog_read_reg8(areg_aon_0x06) | FLD_PD_SPD_LDO | FLD_PD_DIG_RET_LDO | FLD_PD_VBAT_SW) & ~(vbat_v));

    g_pm_status_info.wakeup_src    = pm_get_wakeup_src();
    g_pm_status_info.is_pad_wakeup = (g_pm_status_info.wakeup_src & BIT(0));
#if GENERATE_LIB_FOR_GOOGLE //ble use
    pm_update_boot_info();
#endif
    g_pm_vbat_v = vbat_v >> 3;

    if (g_chip_version == CHIP_VERSION_A1) {
        /*
         *                      poweron_dft:    0x01 -> 0x03.
         *      bit                     note
         * ---------------------------------------------------------------------------
         * <1>:pd_PGA_bias,         default:0,->1 power down PGA bias current initial state.
         * This will reduce power consumption and has little impact to operate early or late.
         */
        analog_write_reg8(areg_0x8f, analog_read_reg8(areg_0x8f) | FLD_AUDIO_VMID_PD);

        if (g_pm_status_info.mcu_status != MCU_STATUS_DEEPRET_BACK) {
            /*
            * A1 version add 0x13<3> as bb power switch to fix the ISSUE(TER-32).
            * vdd_bb was powered by dig_ldo/spd_ldo instead dcore_ldo, so here need to power it up first.
            * (added by jilong.liu at 20240517)
            */
            analog_write_reg8(areg_aon_0x13, analog_read_reg8(areg_aon_0x13) & 0xf7);
        }

        /*
        * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
        * Because the stimer is necessary for the pm_wait_xtal_ready.
        * (add by jilong.liu, confirmed by wenfeng.lou 20240513)
        */
        analog_write_reg8(areg_0x8c, 0x82);
    }
    /*
     * Because the linearity (slope) of the data collected by the adc at low temperatures is inconsistent with that at room temperature, the temperature drift test is performed.
     * According to the result of temperature drift test, Haitao Wenfeng suggests to adjust Bandgap voltage trimming to the maximum, to 111 (binary).
     * (add by bolong.zhang, confirmed haitao 20241219)
     */
    if (g_pm_status_info.mcu_status != MCU_STATUS_DEEPRET_BACK) {
        analog_write_reg8(areg_aon_0x00, (analog_read_reg8(areg_aon_0x00) & (~FLD_BG_TRIM_3V)) | 0x0e);
    }
#if PM_XTAL_READY_TIME
    gpio_set_low_level(GPIO_PB4);
#endif

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230531)
    pm_wait_xtal_ready(0x00);

    if (g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK) {
        /**< BLE USED *****/
        #if 0
            pm_stimer_recover();
        #else
            ext_pm_tim_recover();
        #endif
        /**< BLE USED END */
    } else {
        //update vdd0p94 level when power on or wakeup from deep sleep
        pm_update_vdd0p94_level();
#if SYS_TIMER_AUTO_MODE
        stimer_enable(STIMER_AUTO_MODE_W_TRIG, 0x01);
        stimer_32k_tracking_enable(); //enable 32k cal
#else
        stimer_enable(STIMER_MANUAL_MODE, 0x01);
        stimer_32k_tracking_enable(); //enable 32k cal
#endif
        /**< BLE USED *****/
        extern void cpu_wakeup_no_deepretn_back_init(void);
        cpu_wakeup_no_deepretn_back_init(); // to save ramcode

        //check protection code
        extern void efuse_check_protection_code(void);
        efuse_check_protection_code();//BLE use

        //check protection code
        #if (SDK_VERSION_SELECT != SDK_VERSION_IGNORE)
            otp_check_protection_code(SDK_VERSION_SELECT);//0:driver sdk  0xff:sdk_version_ignore
        #endif

        clock_cal_24m_rc();
        /**< BLE USED END */
    }
    /**< BLE USED *****/
    // enable pke module by default
    //pke_reset()
    reg_rst1 &= ~FLD_RST1_PKE;
    reg_rst1 |= FLD_RST1_PKE;
    //pke_clk_en
    reg_clk_en1 |= FLD_CLK1_PKE_EN;
    /**< BLE USED END */

}

/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/
