/********************************************************************************************************
 * @file    sys.c
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
#include "lib/include/rf.h"
#include "lib/include/sys.h"
#include "core.h"
#include "compiler.h"
#include "lib/include/analog.h"
#include "gpio.h"
#include "mspi.h"
#include "stimer.h"
#include "usbhw.h"
#include "adc.h"

//Protection Code checking related macro
#define SDK_VERSION_DRIVER 2
#define SDK_VERSION_IGNORE 255

#define SDK_VERSION_SELECT SDK_VERSION_IGNORE

unsigned int                                 g_chip_version = 0;
_attribute_data_retention_sec_ unsigned char g_areg_aon_0a  = 0;

#if (SDK_VERSION_SELECT != SDK_VERSION_IGNORE)

static _always_inline void efuse_check_protection_code(void);

#endif

/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void sys_reboot_ram(void)
{
    core_interrupt_disable(); //It must be inlined to ensure that the text segment cannot be entered.
    mspi_stop_xip();
    write_reg8(0x1401ef, 0x20);
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
_attribute_ram_code_sec_optimize_o2_ void crystal_manual_settle(void)
{
    //Due to differences in chip design, the analog registers operated here are different.
    //(modified by jilong.liu, confirmed by wenfeng.lou 20240328)
    analog_write_reg8(0x65, analog_read_reg8(0x65) | 0x40); //0x65<6>: write 1 to reset xtal quick start cnt

    unsigned char ana_05 = analog_read_reg8(0x05);          //0x05<3>: 24M_xtl_pd
    analog_write_reg8(0x05, ana_05 | 0x08);                 //<3>1b'1: Power down 24MHz XTL oscillator
    analog_write_reg8(0x05, ana_05 & 0xf7);                 //<3>1b'0: Power up 24MHz XTL oscillator
}

#if 0   //BLE SDK use:  in ext_driver
/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @param[in]   gpio_v      - This is the configuration of GPIO voltage.
 *                            For some chip models the GPIO voltage is fixed 3.3V or fixed 1.8V,
 *                            For other GPIO models the voltage is configurable:
 *                            Requires hardware configuration: 3v3 (CFG_VIO connects to VSS) or 1V8 (CFG_VIO connects to VDDO3/AVDD3)),
 *                            please configure this parameter correctly according to the chip data sheet and the corresponding board design.
 * @param[in]   cap     - This parameter is used to determine whether to close the internal capacitor.
 * @attention   If vbat_v is set to VBAT_MAX_VALUE_LESS_THAN_3V6, then gpio_v can only be set to GPIO_VOLTAGE_3V3.
 * @return      none
 * @note        For crystal oscillator with slow start-up or poor quality, after calling this function, 
 *              a reboot will be triggered(whether a reboot has occurred can be judged by using PM_ANA_REG_POWER_ON_CLR_BUF0[bit1]).
 *              For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface 
 *              to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *              When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 */
void sys_init(power_mode_e power_mode, vbat_type_e vbat_v, gpio_voltage_e gpio_v, cap_typedef_e cap)
{
    /**
     * reset function will be cleared by set "1",which is different from the previous configuration.
     * This setting turns off the TRNG modules in order to test power consumption.
     * The current decrease about 3mA when those two modules be turn off.TODO:Update notes after testing current
     */
    reg_rst    = 0xffbfffff;
    reg_clk_en = 0xffbfffff;

    //Before calling the function pm_wait_xtal_ready, you need to ensure that the cclk is set to 24M,
    //otherwise a reboot may occur (for example, the following use case: when the flash is running a cclk>24M program,
    //without powering down the chip to load a ram program, at this time the nop timing judgment in the pm_wait_xtal_ready function is incorrect,
    //which may lead to a reboot)(add by weihua.zhang, confirmed by kaixin 20230609)
    write_reg8(0x1401e8, read_reg8(0x1401e8) & 0x8f);          //cclk to 24M rc clock
    write_reg8(0x1401d8, read_reg8(0x1401d8) & 0xf8);          //clock division to 1:1:1
    write_reg8(0x1401c0, (read_reg8(0x1401c0) & 0x80) | 0x01); //mspiclk to 24M rc clock

    g_areg_aon_7f = analog_read_reg8(0x7f);
    if (!(g_areg_aon_7f & 0x01)) {
        g_pm_status_info.mcu_status = MCU_DEEPRET_BACK;
    }

    //The value must be set to 1 for the VBUS and 0 or 1 for the VBAT.
    //In order to simplify usage, there is no distinction between VBAT and VBUS power supplies at the user level.
    //(add by weihua.zhang, confirmed by yu.ling 20230529)
    if (g_pm_status_info.mcu_status != MCU_DEEPRET_BACK) {
        if (gpio_v == GPIO_VOLTAGE_1V8) {
            analog_write_reg8(0x19, (analog_read_reg8(0x19) & 0x80) | BIT(3) | 0x66); //poweron_dft: 0x44 -> 0x66 or 0x6e.
                                                                                      //<3>:chg_ldo_sw_3p3v_1p8v, default:0,->1 charger's 3.3V LDO output voltage trim 1.8V.
        } else {
            analog_write_reg8(0x19, (analog_read_reg8(0x19) & 0x88) | 0x66);          //poweron_dft: 0x44 -> 0x66.
                                                                                      //<2-0>:trim_VBAT_LCLDO<2:0>,3.1/1.7-3.3/1.8V   default:100,->110 vbat_lcldo output trim:3.3/1.8V
                                                                                      //<6-4>:trim_VBAT_LDO<2:0>,  3.1/1.7-3.3/1.8V   default:100,->110 vbat_ldo output trim:3.3/1.8V
        }
        analog_write_reg8(0x18, (analog_read_reg8(0x18) & 0x8f) | 0x60);              //poweron_dft: 0xc0 -> 0xe0.
                                                                                      //<6-4>:trim_VBAT_AOLDO<2:0>,3.1/1.7-3.3/1.8V   default:100,->110 vbat_ldo output trim:3.3/1.8V
    }

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20240305)
    if (cap == EXTERNAL_CAP_XTAL24M) {
        rf_turn_off_internal_cap();
    }

    //must to set xo_quick_settle with manual and wait it stable(added by bingyu.li,confired by wenfeng 20231123)
    crystal_manual_settle();

    /*
    * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
    * Because the stimer is necessary for the pm_wait_xtal_ready.
    * (add by jilong.liu, confirmed by wenfeng.lou 20240513)
    */
    analog_write_reg8(0x8c, 0x02);
    //<0-1>:pd_dcdc_ldo_sw, default:00, dcdc & bypass ldo status bits.
    //      dcdc_1p2    dcdc_2p0    ldo_1p2     ldo_2p0
    //00:       N           N           Y           Y
    //01:       Y           N           N           Y
    //10:       Y           N           N           N
    //11:       Y           Y           N           N
    if (g_pm_status_info.mcu_status != MCU_DEEPRET_BACK) {
        g_areg_aon_0a = analog_read_reg8(0x0a); //poweron_dft:   0x90.
    }
    if ((g_areg_aon_0a & 0x03) != power_mode) {
        g_areg_aon_0a = (g_areg_aon_0a & 0xfc) | power_mode;
        analog_write_reg8(0x0a, g_areg_aon_0a);
    }

    analog_write_reg8(0x0b, (analog_read_reg8(0x0b) & (~BIT(6))) | (BIT(0) | BIT(1))); //poweron_dft:    0x7f -> 0x3f.
                                                                                       //<0>:pd_nvt_1p2,   default:1,->1 power down native 1P2 dcdc.
                                                                                       //<1>:pd_nvt_2p0,   default:1,->1 power down native 2P0 dcdc.
                                                                                       //<6>:mscn_pullup_res_enb,  default:1,->0 enable 1M pullup resistor for mscn PAD.
    analog_write_reg8(0x02, (analog_read_reg8(0x02) | 0x77) & 0xf4);                   //poweron_dft:  0x42 -> 0x74.
                                                                                       //<2-0>:ldo_ret_trim,0.734-0.799V   default:010,->100 retention LDO output voltage trim: 0.799V
                                                                                       //<3>:LDO_flash_en_bypass,          default:0,->0 LDO_flash's bypass mode disable
                                                                                       //<6-4>:ldo_spd_trim,1.0-1.15V      default:100,->111 suspend LDO output voltage trim: 1.15V
    //The supply voltage of the SRAM needs to be more than 1.2V, so the output voltage of the SRAM LDO is set to 1.2V.
    //When passing through the SRAM LDO, the voltage will drop, so the input voltage of the SRAM LDO needs to be above 1.2V+100mV,
    //so the voltage of 1.2V needs to be configured above 1.3V.(add by weihua.zhang, confirmed by wenfeng.lou 20230607)
    analog_write_reg8(0x06, (analog_read_reg8(0x06) | BIT(3) | BIT(6) | BIT(7)) & ~(BIT(0) | vbat_v | BIT(4) | BIT(5))); //poweron_dft: 0xff -> 0xc6 or 0xce.
                                                                                                                         //<0>:pd_bbpll_ldo,     default:1,->0 Power up bbpll LDO.
                                                                                                                         //<3>:pd_vbus_sw,       default:1,->0 Power up of bypass switch.
                                                                                                                         //<4>:pd_ldo_dcore,     default:1,->0 Power up of digital core ldo.
                                                                                                                         //<5>:pd_ldo_sram,      default:1,->0 Power up of sram ldo.
                                                                                                                         //<6>:spd_ldo_pd,       default:1,->1 Power down spd ldo.
                                                                                                                         //<7>:dig_ret_pd,       default:1,->1 Power down retention ldo.
    analog_write_reg8(0x09, 0xdb);                                                                                       //poweron_dft:  0x1b -> 0xdb.
                                                                                                                         //<6>:pd_sw_dcore,      default:0,->1 power down the main dig ldo to dcore.
                                                                                                                         //<7>:pd_sw_sram,       default:0,->1 power down the main dig ldo to sram.
    analog_write_reg8(0x00, 0x98);                                                                                       //poweron_dft:  0xf8 -> 0x98.
                                                                                                                         //<7-5>:ldo_main_trim,1.15-1.0V default:111,->100 digital LDO output voltage trim: 1.0V

    //After powering on the ZB, you have to wait 5us before you can operate the registers inside the ZB.
    //(add by weihua.zhang, confirmed by jianzhi.chen 20221206)
    analog_write_reg8(0x7d, 0x84); //poweron_dft:  0x07 -> 0x84.
                                   //<0>:pg_zb_en,     default:1,->0 power on baseband.
                                   //<1>:pg_usb_en,    default:1,->0 power on usb.
                                   //<2>:pg_audio_en,  default:1,    can not power on here
                                   //<7>:pg_clk_en,    default:0,->1 enable change power sequence clk.

    //When calling pm_wait_bbpll_done interface, the value of bbpll_ldo_trim may be changed, which needs to be restored during initialization.
    //(add by weihua.zhang, confirmed by wenfeng 20230506)
    analog_write_reg8(0x01, 0x41); //poweron_dft:  0x41 -> 0x41.
                                   //<0-2>:bbpll_ldo_trim,         default:001,->001 B92 is designed with 1V output.
                                   //<4-6>:ana_ldo_trim,1.0-1.4V   default:100,->100 analog LDO output voltage trim: 1.2V

    g_pm_status_info.wakeup_src    = pm_get_wakeup_src();
    g_pm_status_info.is_pad_wakeup = (g_pm_status_info.wakeup_src & BIT(0));

    //in B91,the dma_mask is turned on by default and cleared uniformly during initialization.
    for (unsigned char dma_chn = 0; dma_chn <= 7; dma_chn++) {
        dma_clr_irq_mask(dma_chn, TC_MASK | ERR_MASK | ABT_MASK);
    }
    //the usb ep mask is turned on by default and cleared uniformly during initialization.
    usbhw_clr_eps_irq_mask(FLD_USB_EDP8_IRQ | FLD_USB_EDP1_IRQ | FLD_USB_EDP2_IRQ | FLD_USB_EDP3_IRQ | FLD_USB_EDP4_IRQ | FLD_USB_EDP5_IRQ | FLD_USB_EDP6_IRQ | FLD_USB_EDP7_IRQ);

    g_pm_vbat_v = vbat_v >> 3;

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by bingyu.li, confirmed by wenfeng.lou 20230601.)
    pm_wait_xtal_ready(0x00);

    //When bbpll_ldo_trim is set to the default voltage value, when doing high and low temperature stability tests,it is found that
    //there is a crash.The current workaround is to set other voltage values to see if it is stable.If it fails,repeat the setting
    //up to three times.The bbpll ldo trim must wait until 24M is stable.(add by weihua.zhang, confirmed by yi.bao and wenfeng 20200924)
    pm_wait_bbpll_done();

    if (g_pm_status_info.mcu_status == MCU_DEEPRET_BACK) {
        pm_stimer_recover();
    } else {
#if SYS_TIMER_AUTO_MODE
        reg_system_ctrl |= (FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN); //enable 32k track and system timer auto.
        reg_system_tick = 0x01;                                               //initial next tick is 1,kick system timer
#else
        reg_system_ctrl |= FLD_SYSTEM_32K_TRACK_EN | FLD_SYSTEM_TIMER_EN; //enable 32k track and system timer. Wait for pll to stabilize before using system timer.
#endif
    }

    g_chip_version = read_reg8(0x1401fd);

    //check protection code
#if (SDK_VERSION_SELECT != SDK_VERSION_IGNORE)
    efuse_check_protection_code();
#endif
    rf_clr_irq_mask(FLD_RF_IRQ_ALL); //The default interrupt mask in RF is open.
                                     //Close the interrupt mask in the initialization code and reopen it when in use
}
#endif
/**
 * @brief      This function performs a series of operations of writing digital or analog registers
 *             according to a command table
 * @param[in]  pt - pointer to a command table containing several writing commands
 * @param[in]  size  - number of commands in the table
 * @return     number of commands are carried out
 */

int write_reg_table(const tbl_cmd_set_t *pt, int size)
{
    int l = 0;

    while (l < size) {
        unsigned int  cadr = ((unsigned int)0x80000000) | pt[l].adr;
        unsigned char cdat = pt[l].dat;
        unsigned char ccmd = pt[l].cmd;
        unsigned char cvld = (ccmd & TCMD_UNDER_WR);
        ccmd &= TCMD_MASK;
        if (cvld) {
            if (ccmd == TCMD_WRITE) {
                write_reg8(cadr, cdat);
            } else if (ccmd == TCMD_WAREG) {
                analog_write_reg8(cadr, cdat);
            } else if (ccmd == TCMD_WAIT) {
                delay_us(pt[l].adr * 256 + cdat);
            }
        }
        l++;
    }
    return size;
}

/**
 * @brief       This function servers to get data from EFUSE. EFUSE default value is 0. The EFUSE address range is 0~127 bytes.
 * @param[in]   addr    - the start address of the EFUSE location.
 * @param[in]   buf     - the start address of the buffer.
 * @param[in]   len     - the length(in byte) of content needs to read out from EFUSE.
 * @return      1: operation completed.
 *              0: operation timeout.
 * @note        When EFUSE is read, the VDD25EF cannot be powered. VDD25EF cannot be powered at any time except when burning.
 */
unsigned char efuse_read(unsigned char addr, unsigned char *buff, unsigned char len)
{
    unsigned long start_tick;
    unsigned char ret = 1;

    reg_efuse_read_en = 0x65;
    reg_efuse_ctrl |= (FLD_EFUSE_CLKEN | FLD_EFUSE_RDEN);
    reg_efuse_addr = addr;
    while (len--) {
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;

        start_tick = stimer_get_tick();
        while (FLD_EFUSE_BUSY == (reg_efuse_ctrl & FLD_EFUSE_BUSY)) {
            if (stimer_get_tick() - start_tick > 300000) {
                ret = 0;
                break;
            }
        }

        if (0 == ret) {
            break;
        }

        *buff++ = reg_efuse_rdat;
    }
    reg_efuse_ctrl &= ~(FLD_EFUSE_CLKEN | FLD_EFUSE_RDEN);
    reg_efuse_read_en = 0x00;

    return ret;
}

/**
 * @brief       this function servers to set data to EFUSE. The efuse address range is 0~127 bytes. VDD25EF is need to give 2.5V power when write.
 * @param[in]   addr    - the start address of the efuse location.
 * @param[in]   buf     - the start address of the buffer.
 * @param[in]   len     - the length(in byte) of content needs to be written to efuse.
 * @return      1: operation completed. It does not mean that the written data is correct and requires read back verification.
 *              0: operation timeout.
 */
unsigned char efuse_write(unsigned char addr, unsigned char *buff, unsigned char len)
{
    unsigned long start_tick;
    unsigned char ret = 1;

    reg_efuse_ctrl |= (FLD_EFUSE_CLKEN | FLD_EFUSE_WREN);
    reg_efuse_addr = addr;

    while (len--) {
        reg_efuse_wdat = *buff++;
        reg_efuse_ctrl |= FLD_EFUSE_WR_TRIG;

        start_tick = stimer_get_tick();
        while (FLD_EFUSE_BUSY == (reg_efuse_ctrl & FLD_EFUSE_BUSY)) {
            if (stimer_get_tick() - start_tick > 300000) {
                ret = 0;
                break;
            }
        }

        if (0 == ret) {
            break;
        }
    }

    reg_efuse_ctrl &= ~(FLD_EFUSE_CLKEN | FLD_EFUSE_WREN);

    return ret;
}

/**
 * @brief      This function servers to get calibration value from EFUSE.
 * @param[in]  gpio_type - select the type of GPIO.
 * @return     1 - the calibration value update, 0 - the calibration value is not update.
 */
unsigned char efuse_calib_adc_vref(gpio_voltage_e gpio_type)
{
    unsigned char efuse_adc_calib_info[6] = {0};

    unsigned short gpio_calib_gain   = 0;
    signed char    gpio_calib_offset = 0;

    unsigned short vbat_calib_gain   = 0;
    signed char    vbat_calib_offset = 0;

    if (0 != efuse_read(0x78, efuse_adc_calib_info, 6)) {
        if (gpio_type == GPIO_VOLTAGE_3V3) {
            if (efuse_adc_calib_info[0] > 100) {
                gpio_calib_gain = efuse_adc_calib_info[0] + 1000;
                /**
                * The offset value stored in efuse is not of 'signed' type, and the rules for ATE to write offset value to efuse are as follows:
                * bit[7] = 1 for negative value, bit[7] = 0 for positive value, and the absolute value of bit[0:6] represents the absolute value of offset.
                * So after taking out the offset from efuse, it needs to be converted to 'signed' type.
                */
                gpio_calib_offset = (efuse_adc_calib_info[1] & BIT(7)) ? ((-1) * (efuse_adc_calib_info[1] & 0x7f)) : efuse_adc_calib_info[1];
                adc_set_gpio_calib_vref(gpio_calib_gain);
                adc_set_gpio_two_point_calib_offset(gpio_calib_offset);
            }

            if (efuse_adc_calib_info[2] > 100) {
                vbat_calib_gain   = efuse_adc_calib_info[2] + 1000;
                vbat_calib_offset = (efuse_adc_calib_info[3] & BIT(7)) ? ((-1) * (efuse_adc_calib_info[3] & 0x7f)) : efuse_adc_calib_info[3];
                adc_set_vbat_calib_vref(vbat_calib_gain);
                adc_set_vbat_two_point_calib_offset(vbat_calib_offset);
            }
        } else if (gpio_type == GPIO_VOLTAGE_1V8) {
            if (efuse_adc_calib_info[4] > 100) {
                gpio_calib_gain   = efuse_adc_calib_info[4] + 1000;
                gpio_calib_offset = (efuse_adc_calib_info[5] & BIT(7)) ? ((-1) * (efuse_adc_calib_info[5] & 0x7f)) : efuse_adc_calib_info[5];
                adc_set_gpio_calib_vref(gpio_calib_gain);
                adc_set_gpio_two_point_calib_offset(gpio_calib_offset);
            }
        }
        return 1;
    } else {
        return 0;
    }
}

/**
* @brief      This function servers to get chip id from EFUSE.
* @param[in]  chip_id_buff - store chip id. Chip ID is 16 bytes.
* @return     1 - operation completed,  0 - operation failed.
* @note       Only A3 and later are written as chip id values.
*/
unsigned char efuse_get_chip_id(unsigned char *chip_id_buff)
{
    return efuse_read(0x24, chip_id_buff, 16);
}

/**
 * @brief        This function retrieves specific functionality bits from the EFUSE.
 * @return       result:
 *               - bit 0: JTAG function (0 = enabled, 1 = disabled)
 *               - bit 1: SWS function (0 = enabled, 1 = disabled)
 *               - bit 2: Mode selector (1 = secure boot mode, 0 = normal mode)
 * @note         If there's an error reading the EFUSE, returns 0xFF.
 */
unsigned char efuse_get_chip_status(void)
{
    unsigned char diefun_1   = 0;
    unsigned char identifier = 0;

    // Read eFuse addresses 1 and 3
    if (efuse_read(1, &diefun_1, 1) == 0 || efuse_read(103, &identifier, 1) == 0) {
        return 0xFF;
    }

    // Extract relevant bits and combine the result
    return (((diefun_1 >> 6) & 0x03) | ((identifier >> 3) & 0x04));
}

/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/
#if (SDK_VERSION_SELECT != SDK_VERSION_IGNORE)
/**
 * @brief       This function serves to check protection code according SDK version.
 * @param[in]   none.
 * @return      none.
 */
static _always_inline void efuse_check_protection_code(void)
{
    unsigned char pCode;

    if (efuse_read(104, &pCode, 1) == 0) {
        pCode = 0xff;
    }

    pCode &= 0x0f;

    #if (SDK_VERSION_SELECT == SDK_VERSION_DRIVER)
    if (9 < pCode) {
        reg_pwdn_en = 0x20; /*reboot*/
        while (1);
    }
    #else
    if (1)                  // Prevent macro setting exceptions from invalidating the ProtectionCode function
    {
        reg_pwdn_en = 0x20; /*reboot*/
        while (1);
    }
    #endif
}
#endif
