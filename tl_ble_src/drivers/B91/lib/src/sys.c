/********************************************************************************************************
 * @file    sys.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "analog.h"
#include "gpio.h"
#include "mspi.h"
#include "stimer.h"
#include "dma.h"
#include "usbhw.h"
//Protection Code checking related macro
#define SDK_VERSION_S2     2
#define SDK_VERSION_IGNORE 5

#define SDK_VERSION_SELECT SDK_VERSION_IGNORE

/**
 * There have been several customer complaints about DCDC_1P4_DCDC_1P8.
 * The problem that the driver has tested is that when the power supply voltage exceeds 3.8V,
 * the power supply voltage of flash will exceed the power supply range of flash,
 * but no relatively complete solution has been found.
 * The results of the meeting were (see jira: DRIV-1443 for details) :
 * 1. If 2.8V is used to power flash/codec, DCDC mode is not provided.
 * 2. If the flash is powered at 1.8V and no codec is used, the DCDC mode can be used.
 * (The theoretical analysis of chip colleagues is the case of 1.8V flash power supply,
 * because flash is a wide voltage flash, flash power supply voltage will not exceed the power supply range of flash.)
 * But this has not been verified internally.
 * According to the above situation, the current driver first removes this feature.
 * If there is a real demand in the future,
 * it is necessary to pull the internal chip colleagues to confirm the detailed use method and test related test items.
 * After all the tests have passed, open the feature.
 * changed by weihua.zhang, confirmed by yu.ling, at 20240319.
 */
#define DCDC_1P4_DCDC_1P8_EN 0

typedef enum
{
    SDK_VERSION_PROTECTION_CODE_S2     = 0x02, // The value of protection code is less than or equal to 9 but not equal to 3.
    SDK_VERSION_PROTECTION_CODE_IGNORE = 0x05, // Ignore the value of protection code.
} sdk_version_protection_code_e;

unsigned int g_chip_version = 0;

extern void  pm_update_status_info(void);
unsigned int efuse_get_low_word(void);

/**
 * @brief       This function serves to check protection code according SDK version.
 * @param[in]   version - SDK version.
 * @return      none.
 */
static __attribute__((always_inline)) inline void efuse_check_protection_code(sdk_version_protection_code_e version)
{
    unsigned char pCode;
    pCode = efuse_get_low_word() & 0xf;
    switch (version) {
    case SDK_VERSION_PROTECTION_CODE_S2:
        if ((3 == pCode) || (9 < pCode)) {
            reg_pwdn_en = 0x20; /*reboot*/
            while (1);
        }
        break;
    default:
        break;
    }
}

/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ void sys_reboot_ram(void)
{
    core_interrupt_disable(); //It must be inlined to ensure that the text segment cannot be entered.
    mspi_stop_xip();
    reg_pwdn_en = 0x20;
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
    //For B91, here need to write 1 to reset then write 0 to it default value which is no need for other chips.
    //(modified by jilong.liu, confirmed by wenfeng.lou 20240328)
    unsigned char ana_50 = analog_read_reg8(0x50); //0x50<7>: write 1 to reset xtal quick start cnt
    analog_write_reg8(0x50, ana_50 | 0x80);
    analog_write_reg8(0x50, ana_50 & 0x7f);

    unsigned char ana_05 = analog_read_reg8(0x05); //0x05<3>: 24M_xtl_pd
    analog_write_reg8(0x05, ana_05 | 0x08);        //<3>1b'1: Power down 24MHz XTL oscillator
    analog_write_reg8(0x05, ana_05 & 0xf7);        //<3>1b'0: Power up 24MHz XTL oscillator
}

#if 0       //BLE SDK use:  in ext_driver
/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @param[in]   cap     - This parameter is used to determine whether to close the internal capacitor.
 * @return      none
 * @note        For crystal oscillator with slow start-up or poor quality, after calling this function, 
 *              a reboot will be triggered(whether a reboot has occurred can be judged by using PM_ANA_REG_POWER_ON_CLR_BUF0[bit2]).
 *              For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface 
 *              to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *              When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 */
void sys_init(power_mode_e power_mode, vbat_type_e vbat_v, cap_typedef_e cap)
{
    /**
     * reset function will be cleared by set "1",which is different from the previous configuration.
     * This setting turns off the TRNG and NPE modules in order to test power consumption.The current
     * decrease about 3mA when those two modules be turn off.changed by zhiwei,confirmed by kaixin.20200828.
     */
    reg_rst    = 0xffbbffff;
    reg_clk_en = 0xffbbffff;

    //Before calling the function pm_wait_xtal_ready, you need to ensure that the cclk is set to 24M,
    //otherwise a reboot may occur (for example, the following use case: when the flash is running a cclk>24M program,
    //without powering down the chip to load a ram program, at this time the nop timing judgment in the pm_wait_xtal_ready function is incorrect,
    //which may lead to a reboot)(add by weihua.zhang, confirmed by kaixin 20230609)
    //When load code twice without power down DUT, DUT will use crystal clock in here, xo_quick_settle manual mode need to use in RC clock.
    write_reg8(0x1401e8, read_reg8(0x1401e8) & 0x0f); //mspiclk & cclk to 24M rc clock
    write_reg8(0x1401d8, read_reg8(0x1401d8) & 0xf8); //clock division to 1:1:1

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20240531)
    if (cap == EXTERNAL_CAP_XTAL24M) {
        rf_turn_off_internal_cap();
    }

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20231123)
    crystal_manual_settle();

    /* 
    * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
    * Because the stimer is necessary for the pm_wait_xtal_ready.
    * (add by jilong.liu, confirmed by wenfeng.lou 20240513)
    */
    analog_write_reg8(0x8c, 0x02);

#if DCDC_1P4_DCDC_1P8_EN
    //when VBAT power supply > 4.1V and LDO switch to DCDC,DCDC_1V8 voltage will ascend to the supply power in a period time,
    //cause the program can not run. Need to trim down dcdc_flash_out before switch power mode.
    //confirmed by haitao,modify by yi.bao(20210119)
    if (DCDC_1P4_DCDC_1P8 == power_mode) {
        analog_write_reg8(0x0c, 0x40); //poweron_dft: 0x44 --> 0x40.
                                       //<2:0> dcdc_trim_flash_out,flash/codec 1.8V/2.8V trim down 0.2V in DCDC mode.
    }
#endif

    analog_write_reg8(0x0a, power_mode);                                                               //poweron_dft:  0x90.
                                                                                                       //<0-1>:pd_dcdc_ldo_sw, default:00, dcdc & bypass ldo status bits.
                                                                                                       //      dcdc_1p4    dcdc_1p8    ldo_1p4     ldo_1p8
                                                                                                       //00:       N           N           Y           Y
                                                                                                       //01:       Y           N           N           Y
                                                                                                       //10:       Y           N           N           N
                                                                                                       //11:       Y           Y           N           N
    analog_write_reg8(0x0b, 0x3b);                                                                     //poweron_dft:  0x7b -> 0x3b.
                                                                                                       //<6>:mscn_pullup_res_enb,  default:1,->0 enable 1M pullup resistor for mscn PAD.
    analog_write_reg8(0x05, analog_read_reg8(0x05) & (~BIT(3)));                                       //poweron_dft:   0x02 -> 0x02.
                                                                                                       //<3>:24M_xtl_pd,       default:0,->0 Power up 24MHz XTL oscillator.
    analog_write_reg8(0x06, (analog_read_reg8(0x06) | BIT(3)) & ~(BIT(0) | vbat_v | BIT(6) | BIT(7))); //poweron_dft: 0xff -> 0x36 or 0x3e.
                                                                                                       //<0>:pd_bbpll_ldo,     default:1,->0 Power on ana LDO.
                                                                                                       //<3>:pd_vbus_sw,       default:1,->0 Power up of bypass switch.
                                                                                                       //<6>:spd_ldo_pd,       default:1,->0 Power up spd ldo.
                                                                                                       //<7>:dig_ret_pd,       default:1,->0 Power up retention  ldo.
    analog_write_reg8(0x01, 0x45);                                                                     //poweron_dft:  0x44 -> 0x45.
                                                                                                       //<0-2>:bbpll_ldo_trim,         default:100,->101 measured 1.186V.The default value is sometimes crashes.
                                                                                                       //<4-6>:ana_ldo_trim,1.0-1.4V   default:100,->100 analog LDO output voltage trim: 1.2V
    //When using the default value, during the USB charging process, the audio output will hear a sizzling electric current.
    //This problem can be solved by increasing the OCP current limit value to avoid unnecessary shutdown.
    //confirmed by ya.yang, modify by weihua.zhang(20210805)
    analog_write_reg8(0x1c, 0x4c); //poweron_dft:  0x40 -> 0x4c.
                                   //<2-3>:ocp_i_cross_trim,   default:00,->11 the current limit value of OCP is configured to the maximum value.

    //in B91,the dma_mask is turned on by default and cleared uniformly during initialization.
    for (unsigned char dma_chn = 0; dma_chn <= 7; dma_chn++) {
        dma_clr_irq_mask(dma_chn, TC_MASK | ERR_MASK | ABT_MASK);
    }
    //the usb ep mask is turned on by default and cleared uniformly during initialization.
    usbhw_clr_eps_irq_mask(FLD_USB_EDP8_IRQ | FLD_USB_EDP1_IRQ | FLD_USB_EDP2_IRQ | FLD_USB_EDP3_IRQ | FLD_USB_EDP4_IRQ | FLD_USB_EDP5_IRQ | FLD_USB_EDP6_IRQ | FLD_USB_EDP7_IRQ);
    pm_update_status_info();
    g_pm_vbat_v = vbat_v >> 3;

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by jilong.liu, confirmed by wenfeng.lou 20240320. Issue:EAG-59)
    pm_wait_xtal_ready(0x00);

    //When bbpll_ldo_trim is set to the default voltage value, when doing high and low temperature stability tests,it is found that
    //there is a crash.The current workaround is to set other voltage values to see if it is stable.If it fails,repeat the setting
    //up to three times.The bbpll ldo trim must wait until 24M is stable.(add by weihua.zhang, confirmed by yi.bao and wenfeng 20200924)
    pm_wait_bbpll_done();

    if (g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK) {
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

    //if clock src is PAD or PLL, and hclk = 1/2cclk, use reboot may cause problem, need deep to resolve(add by yi.bao, confirm by guangjun 20201016)
    if (g_pm_status_info.mcu_status == MCU_STATUS_REBOOT_BACK) {
        //Use PM_ANA_REG_POWER_ON_CLR_BUF0 BIT(1) to represent the reboot+deep process, which is related to the function pm_update_status_info.
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | DEEP_AFTER_REBOOT); //(add by weihua.zhang, confirmed by yi.bao 20201222)
        pm_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, PM_TICK_STIMER_16M, (stimer_get_tick() + 100 * SYSTEM_TIMER_TICK_1MS));
    }
    //**When testing AES_demo, it was found that the timing of baseband was wrong when it was powered on, which caused some of
    //the registers of CV to go wrong, which caused the program to run abnormally.(add by weihua.zhang, confirmed by junwen 20200819)
    else if (0xff == g_chip_version)                            //A0
    {
        if (g_pm_status_info.mcu_status == MCU_STATUS_POWER_ON) //power on
        {
            analog_write_reg8(0x7d, 0x80);                      //power on baseband
            pm_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, PM_TICK_STIMER_16M, (stimer_get_tick() + 100 * SYSTEM_TIMER_TICK_1MS));
        }
    }
    analog_write_reg8(0x7d, 0x80); //poweron_dft:  0x03 -> 0x80.
                                   //<0>:pg_zb_en,     default:1,->0 power on baseband.
                                   //<1>:pg_usb_en,    default:1,->0 power on usb.
                                   //<2>:pg_npe_en,    default:1,->0 power on npe.
                                   //<7>:pg_clk_en,    default:0,->1 enable change power sequence clk.
#if DCDC_1P4_DCDC_1P8_EN
    //when VBAT power supply > 4.1V and LDO switch to DCDC,DCDC_1V8 voltage will ascend to the supply power in a period time,
    //cause the program can not run. Need to trim down dcdc_flash_out before switch power mode,refer to the configuration above [analog_write_reg8(0x0c, 0x40)],
    /*Then restore the default value[analog_write_reg8(0x0c, 0x44)].There is a process of switching from LDO to DCDC, which needs to wait for a period of time, so it is restored here,
    confirmed by haitao,modify by minghai.duan(20211018)*/
    if (DCDC_1P4_DCDC_1P8 == power_mode) {
        analog_write_reg8(0x0c, 0x44); //poweron_dft: 0x40 --> 0x44.
                                       //<2:0> dcdc_trim_flash_out,flash/codec 1.8V/2.8V in DCDC mode.
    }
#endif
    //check protection code
#if SDK_VERSION_SELECT == SDK_VERSION_S2
    efuse_check_protection_code(SDK_VERSION_PROTECTION_CODE_S2);
#elif SDK_VERSION_SELECT == SDK_VERSION_IGNORE
    efuse_check_protection_code(SDK_VERSION_PROTECTION_CODE_IGNORE);
#endif
    rf_clr_irq_mask(FLD_RF_IRQ_ALL); //The default interrupt mask in RF is open.
                                     //Close the interrupt mask in the initialization code and reopen it when in use
}
#endif      //BLE SDK use:  in ext_driver
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
 * @brief     this function servers to get data(BIT0~BIT31) from EFUSE's register.efuse default value is 0.
 * @note      For reliability reasons, you need to disconnect the 2.5V power supply before you read the data from efuse,
 *            otherwise the value of efuse may be changed unexpectedly.
 * @param[in] none
 * @return    data(BIT0~BIT31)
 */
unsigned int efuse_get_low_word(void)
{
    unsigned int efuse_info;
    //Because low word has a key to store flash encryption function, the hardware has a protection function when reading low word.
    //In order to read the correct value, you need to remove this protection first, otherwise you can only read 0.
    write_reg8(0x1401f4, 0x65);        //remove protection, required only when reading low_word
    efuse_info = read_reg32(0x1401c8); //read the low_word(BIT<31:0>) data
    write_reg8(0x1401f4, 0x00);        //open protection
    return efuse_info;
}

/**
 * @brief     this function servers to get data(BIT32~BIT63) from EFUSE's register.efuse default value is 0.
 * @note      For reliability reasons, you need to disconnect the 2.5V power supply before you read the data from efuse,
 *            otherwise the value of efuse may be changed unexpectedly.
 * @param[in] none
 * @return    data(BIT32~BIT63)
 */
unsigned int efuse_get_high_word(void)
{
    unsigned int efuse_info;
    efuse_info = read_reg32(0x1401cc); //read the high_word(BIT<63:32>) data
    return efuse_info;
}

/**
 * @brief     this function servers to refresh efuse's data(BIT0~BIT63) after writing.efuse default value is 0.
 * @note      If you can't repower the chip after writing efuse, you need to call this function before reading efuse.
 * @param[in] none
 * @return    none
 */
_attribute_ram_code_sec_noinline_ void efuse_refresh_data_ram(void)
{
    /*   efuse_refresh_data() has two requirements:
 *   1.It must be sram code, because during the refresh period, the encryption and decryption function of mspi will be enabled.
 *     If the flash operation is performed during the refresh period, program will trap.
 *   2.Global variables and local variables cannot be used in functions.
 *     Because the limit size of sram is an unstable value during refresh, ensure that data, bss, and stack areas are not applicable here.
*/
    mspi_stop_xip();
    write_reg8(0x1401c7, 0x04); //[0]efuse_w=0,[2]efuse_sclk_en=1. [2]used as the multiplexing function to trig read here
    while (BIT(1) == (read_reg8(0x1401c7) & BIT(1)));                       //wait read data from EFuse to reg done, Bit(1) as a read busy flag will be auto set to 0,Bit(2) will be set to 0 too.
}

_attribute_text_sec_ void efuse_refresh_data(void)
{
    //Because flash operations cannot be performed during refresh period, it is necessary to disable prefetch first.
    DISABLE_BTB;
    efuse_refresh_data_ram();
    ENABLE_BTB;
}

/**
 * @brief     this function servers to get calibration value from EFUSE.
 *            Only the two-point calibration gain and offset of GPIO sampling are saved in the efuse of B91.
 * @param[out]gain - gpio_calib_value.
 * @param[out]offset - gpio_calib_value_offset.
 * @return    1 means there is a calibration value in efuse, and 0 means there is no calibration value in efuse.
 */
unsigned char efuse_get_adc_calib_value(unsigned short *gain, signed char *offset)
{
    unsigned short efuse_4to18bit_info = (efuse_get_low_word() >> 4) & 0x7fff;
    if (0 != efuse_4to18bit_info) {
        //Before the gain is stored in efuse, in order to reduce the number of bits occupied, 1000 is subtracted.
        //gpio_calib_value:bit[12:4]+1000
        *gain = (efuse_4to18bit_info & 0x1ff) + 1000; //unit: mv
        //gpio_calib_value_offset:bit[18:13]-20
        *offset = ((efuse_4to18bit_info >> 9) & 0x3f) - 20; //unit: mv
        return 1;
    } else {
        //If efuse_4to18bit_info is 0, there is no calibration value in efuse.
        return 0;
    }
}

/**
 * @brief     this function servers to set data(BIT0~BIT63) to EFUSE.VDD25FE(pin 15) is need to give 2.5V power when write.
 * @note      If you want to read the latest value of efuse after writing efuse, you need to repower it or
 *            call efuse_refresh_data() and then call efuse_get_low_word and efuse_get_high_word to read the latest value.
 *            Otherwise, it is the value that was read last time. (Power-on triggers a read operation by default.
 *            Therefore, if no write operation is performed, you can directly invoke the read interface after power-on to read the correct value.)
 *            You are advised to repower the device and read it again(this is the safest option).
 *            If you cannot repower the device, you can call efuse_refresh_data() to read it again.
 * @param[in] data - the data need to be write(2 word)
 * @return    none
 */
void efuse_set_data(unsigned int *data)
{
    write_reg8(0x1401c7, 0x01);     //[0]efuse_w=1,write_enable
    write_reg8(0x1401c7, 0x05);     //[0]efuse_w=1,[2]efuse_sclk_en=1,write_enable & clock_enable. [2]used as the clock enable bit here
    write_reg32(0x1401c8, data[0]); //write low word
    write_reg32(0x1401cc, data[1]); //write high word
    write_reg8(0x1401c7, 0x03);     //[0]efuse_w=1,[1]write trig=1 and trig write
    while (BIT(1) == (read_reg8(0x1401c7) & BIT(1)));                           //wait write data from reg to EFuse done, Bit(1) as a write busy flag will be auto set to 0
    write_reg8(0x1401c7, 0x00);     //[0]efuse_w=0, bit[0] requires software to clear 0, bit[1][2]will be set to 0 by hardware
}

/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/
