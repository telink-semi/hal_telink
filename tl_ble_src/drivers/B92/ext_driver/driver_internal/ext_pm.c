/********************************************************************************************************
 * @file    ext_pm.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "../../lib/include/analog.h"
#include "../../dma.h"
#include "../../gpio.h"
#include "../../lib/include/pm.h"
#include "../../adc.h"
#include "../../timer.h"
#include "../../flash.h"
#include "../../lib/include/trng.h"
#include "../../lib/include/sys.h"
#include "../../lib/include/plic.h"
#include "../../stimer.h"
#include "../../clock.h"
#include "../../compatibility_pack/cmpt.h"
#include "../ext_misc.h"
#include "../ext_pm.h"
#include "ext_lib.h"
#include "../mcu_boot.h"
#include "config/user_config.h"
#include "usbhw.h"




_attribute_data_retention_  _attribute_aligned_(4) misc_para_t      blt_miscParam;

_attribute_data_retention_sec_  suspend_handler_t   func_before_suspend = 0;

_attribute_data_retention_sec_  cpu_pm_handler_t            cpu_sleep_wakeup;  //no need retention,  cause it will be set every wake_up
_attribute_data_retention_sec_  pm_tim_recover_handler_t    pm_tim_recover;
_attribute_data_retention_sec_  check_32k_clk_handler_t     pm_check_32k_clk_stable = 0;

_attribute_data_retention_sec_  unsigned char   pm_check_info = 0;
_attribute_data_retention_sec_  unsigned int        ota_program_bootAddr = MULTI_BOOT_ADDR_0x40000; //default 256K
_attribute_data_retention_sec_  unsigned int        ota_firmware_max_size = (MULTI_BOOT_ADDR_0x40000 - 0x1000);  //unit: Byte, - 4K is important
_attribute_data_retention_sec_  unsigned int        ota_program_offset = 0;

extern unsigned int             g_pm_tick_32k_calib_repair;
extern unsigned char            g_areg_aon_0a;
extern unsigned char efuse_read(unsigned char addr, unsigned char* buff, unsigned char len);
/**
 * @brief     This function servers to get protection code from EFUSE(byte 104).
 * @return    The protection code value.
 */
unsigned char efuse_get_protection_code(void)
{
    unsigned char protection_code;
    efuse_read(104, (u8*)&protection_code, 1);
    return protection_code;
}


void bls_pm_registerFuncBeforeSuspend (suspend_handler_t func )
{
    func_before_suspend = func;
}


/**
 * @brief     this function servers to start reboot.
 * @param[in] none
 * @return    none
 */
_attribute_ram_code_ void mcu_reboot(void)
{
    mspi_stop_xip();

    if(blt_miscParam.pad32k_en){
        analog_write_reg8(SYS_DEEP_ANA_REG, analog_read_reg8(SYS_DEEP_ANA_REG) & (~SYS_NEED_REINIT_EXT32K)); //clear
    }

    #if (!WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
    if(blt_miscParam.pad32k_en || blt_miscParam.pm_enter_en){
        analog_write_reg8(0x7f, 0x01);  // 0x7f reboot does not reset
    }
    #endif

    core_interrupt_disable ();
    REG_ADDR8(0x1401ef) = 0x20;  //reboot
    while (1);
}

_attribute_text_sec_ _attribute_no_inline_ void start_reboot(void)
{
    DISABLE_BTB;
    mcu_reboot();
    ENABLE_BTB;
}

/**
 * @brief       This function serves to initiate the cpu after power on or deepsleep mode.
 * @param[in]   none.
 * @return      none.
 */
static _attribute_no_inline_ void cpu_wakeup_no_deepretn_back_init(void)
{
    unsigned char sel_32k   = analog_read_reg8(0x4e)&0x7f;
    unsigned char power_32k = analog_read_reg8(0x05)&0xfc;
    //Set 32k clk src: external 32k crystal, only need init when deep+pad wakeup or 1st power on
    if(blt_miscParam.pad32k_en){
        analog_write_reg8(0x4e, sel_32k|(CLK_32K_XTAL<<7));
        analog_write_reg8(0x05, power_32k|0x1);//32k xtal
        g_clk_32k_src = CLK_32K_XTAL;
        //in this case: ext 32k clk was closed, here need re-init.
        //when deep_pad wakeup or first power on, it needs pwm acc 32k pad vibration time(dly 10ms)
        if(!(analog_read_reg8(SYS_DEEP_ANA_REG) & SYS_NEED_REINIT_EXT32K)){
            analog_write_reg8(SYS_DEEP_ANA_REG,  analog_read_reg8(SYS_DEEP_ANA_REG) | SYS_NEED_REINIT_EXT32K); //if initialized, the FLG is set to "1"
            clock_kick_32k_xtal(10);
        }
        else{
            delay_us(6000);
        }
    }
    else{
        analog_write_reg8(0x4e, sel_32k|(CLK_32K_RC<<7));
        analog_write_reg8(0x05, power_32k|0x2);//32k rc
        clock_cal_32k_rc();  //6.69 ms/6.7 ms
    }


    /*  0x23FFFF20 mspi_set_l: mutiboot address offset option, 0:0k;  1:128k;  2:256k;  4:512k
     *  0x23FFFF21 mspi_set_h: program space size = (mspi_set_h+1)*4k
     *
     *  Normal mode       mspi_set_l            mspi_set_h          mspi_multi_boot in description
     *  FW on 0x00000:      0x00                   0x00                             N/A
     *  FW on 0x20000:      0x01                   0x1F                             N/A
     *  FW on 0x40000:      0x02                   0x3F                             N/A
     *  FW on 0x80000:      0x04                   0x7F                             N/A
     *
     *  Secure boot mode
     *  FW on 0x00000:      0x00                   0x7F                         0x0000
     *  FW on 0x20000:      0x01                   0x1F                         0x2020
     *  FW on 0x40000:      0x02                   0x3F                         0x4040
     *  FW on 0x80000:      0x04                   0x7F                         0x8080
     */



    //boot flag storage
    //Only need read 1 byte. If read 2 bytes, in secure boot mode, erase err
    unsigned char boot_flag = read_reg8(0x23FFFF20);
    if (boot_flag)
    {
        ota_program_offset = 0;
    }
    else
    {
        ota_program_offset = ota_program_bootAddr;
    }
}
static _always_inline void efuse_check_protection_code(void)
{
    unsigned char pCode;

    if(efuse_read(104, &pCode, 1) == 0)
    {
        pCode = 0xff;
    }

    pCode &= 0x0f;

    /* Jaguar: IOT + LE Audio ( pCode == 0 ) */
    if(pCode == 0){
        pm_check_info = 0xFF;
    }

    #if (DRV_RSSI_SNIFFER_MODE_ENABLE)
        if(pCode > 4) //B92 RSSI Sniffer SDK: pCode <= 4
        {
            reg_pwdn_en = 0x20;/*reboot*/
            while(1);
        }
    #endif

    if(9 < pCode) //LE AUDIO:0   LE multi:9
    {
        reg_pwdn_en = 0x20;/*reboot*/
        while(1);
    }
}




/**
 * @brief      This function serves to determine whether wake up source is external 32k RC.
 * @param[in]  none.
 * @return     none.
 */
extern void check_32k_clk_stable(void);
_attribute_ram_code_ void blc_pm_select_external_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_xtal;
    pm_check_32k_clk_stable = check_32k_clk_stable;
    pm_tim_recover          = pm_tim_recover_32k_xtal;
    g_clk_32k_src = CLK_32K_XTAL;
    blt_miscParam.pad32k_en     = 1; // set '1': 32k clk src use external 32k crystal
}

/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @param[in]   gpio_v      - This is the configuration of GPIO voltage.
 *                            For some chip models the GPIO voltage is fixed 3.3V or fixed 1.8V,
 *                            For other GPIO models the voltage is configurable:
 *                            Requires hardware configuration: 3v3 (CFG_VIO connects to VSS) or 1V8 (CFG_VIO connects to VDDO3/AVDD3)),
 *                            please configure this parameter correctly according to the chip data sheet and the corresponding board design.
 * @attention   If vbat_v is set to VBAT_MAX_VALUE_LESS_THAN_3V6, then gpio_v can only be set to GPIO_VOLTAGE_3V3.
 * @return      none
 */
_attribute_ram_code_ void sys_init(power_mode_e power_mode, vbat_type_e vbat_v, gpio_voltage_e gpio_v, cap_typedef_e cap)
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

        BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer

        #if SYS_TIMER_AUTO_MODE

            REG_ADDR8(0x140218) = 0x02;//sys tick 16M set upon next 32k posedge
            reg_system_ctrl     |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN) ;

            //unsigned int deepRet_32k_tick = clock_get_digital_32k_tick();
            unsigned int deepRet_32k_tick = clock_get_32k_tick();

            unsigned int deepRet_tick = pm_tim_recover(deepRet_32k_tick + 1); // pm_tim_recover_32k_rc

            reg_system_tick = deepRet_tick + 1;

            //wait cmd set dly done upon next 32k posedge
            //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
            while((reg_system_st & BIT(7)) == 0);

            REG_ADDR8(0x140218) = 0;//normal sys tick (16/sys) update
        #else
            BM_CLR(reg_system_ctrl,FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal and stimer

            //unsigned int deepRet_32k_tick = clock_get_digital_32k_tick();
            unsigned int deepRet_32k_tick = clock_get_32k_tick();
            unsigned int deepRet_tick = pm_tim_recover(deepRet_32k_tick);
            reg_system_tick = deepRet_tick;
            reg_system_ctrl |=(FLD_SYSTEM_TIMER_EN|FLD_SYSTEM_32K_TRACK_EN) ;
        #endif
        }else{
#if SYS_TIMER_AUTO_MODE
            reg_system_ctrl |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN);  //enable 32k track and system timer auto.
            reg_system_tick = 0x01; //initial next tick is 1,kick system timer
#else
            reg_system_ctrl |= FLD_SYSTEM_32K_TRACK_EN | FLD_SYSTEM_TIMER_EN;   //enable 32k track and system timer. Wait for pll to stabilize before using system timer.
#endif
        cpu_wakeup_no_deepretn_back_init(); // to save ramcode

        /* ADC calibration result is stored in variables on retention data area, can keep in deepSleep retention mode.
         * So do not need do this after  deepSleep retention wake up. */
        efuse_calib_adc_vref(gpio_v);

        g_chip_version = read_reg8(0x1401fd);

        //check protection code
        efuse_check_protection_code();//BLE move by SunWei.

        clock_cal_24m_rc();
        }

        reg_system_irq_mask |= BIT(0);

        g_chip_version = read_reg8(0x1401fd);


        reg_embase_addr = 0xc0000000;  //set the embase addr //BLE MUST

        rf_clr_irq_mask(FLD_RF_IRQ_ALL);//The default interrupt mask in RF is open.
                                        //Close the interrupt mask in the initialization code and reopen it when in use
}

unsigned int clock_get_digital_32k_tick(void)
{
    unsigned int timer_32k_tick;
    reg_system_st = FLD_SYSTEM_CLR_RD_DONE;//clr rd_done
    while((reg_system_st & FLD_SYSTEM_CLR_RD_DONE) != 0);//wait rd_done = 0;
    reg_system_ctrl &= ~FLD_SYSTEM_32K_WR_EN;   //1:32k write mode; 0:32k read mode
    while((reg_system_st & FLD_SYSTEM_CLR_RD_DONE) == 0);//wait rd_done = 1;
    timer_32k_tick = reg_system_timer_read_32k;
    reg_system_ctrl |= FLD_SYSTEM_32K_WR_EN;    //1:32k write mode; 0:32k read mode
    return timer_32k_tick;
}




_attribute_data_retention_ int tick_rc24mCal = BIT(31);


/**
 * @brief   internal oscillator calibration for environment change such as voltage, temperature
 *          to keep some critical PM or RF performance stable
 *          attention: this is a stack API, user can not call it
 * @param   none
 * @return  none
 */
void mcu_oscillator_crystal_calibration(void)
{
    if( clock_time_exceed(tick_rc24mCal, 59995*1000)){  //cal 24m every 10 second
        tick_rc24mCal = clock_time();
        clock_cal_24m_rc();  //469 us/474 us
    }
}


_attribute_data_retention_ pm_clock_drift_t pmbcd; //= {0, 0, 0, 0, 0, 0};
/**
 * @brief       When 32k rc sleeps, the calibration function is initialized.
 * @return      none.
 */
void pm_32k_rc_offset_init(void)
{
    pmbcd.offset = 0;
    pmbcd.tc = 0;
    pmbcd.ref_tick = 0;
}

_attribute_ram_code_ void pm_ble_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick)
{
    pmbcd.rc32_rt = tick_32k - pmbcd.rc32_wakeup; //rc32_rt not used
    if (pmbcd.calib || pmbcd.ref_no > 20 || !pmbcd.ref_tick || ((tick_32k - pmbcd.ref_tick_32k) & 0xfffffff) > 32 * 3000)//3S
    {
        pmbcd.calib = 0;
        pmbcd.ref_tick_32k = tick_32k;
        pmbcd.ref_tick = tick | 1;
        pmbcd.ref_no = 0;
    }
    else
    {
        pmbcd.ref_no++;
    }
}

_attribute_ram_code_sec_noinline_ void pm_ble_32k_rc_cal_reset(void)
{
    pmbcd.offset = 0;
    pmbcd.tc = 0;
    pmbcd.ref_tick = 0;
    pmbcd.offset_cal_tick = 0;
}

#if 1
_attribute_ram_code_sec_noinline_ void pm_ble_cal_32k_rc_offset (int offset_tick, int rc32_cnt)
{
    int offset = offset_tick * (256 * 31) / rc32_cnt;       //256mS / sleep_period
    int thres = rc32_cnt/9600;  //240*32=7680  300*32= 9600  400*32= 12800
    if(!thres){
        thres = 1;
    }
//  else if(thres > 8){
//      thres = 8;
//  }
    thres *= 0x100;

    if (offset > thres)
    {
        offset = thres;
    }
    else if (offset < -thres)
    {
        offset = -thres;
    }
    pmbcd.calib = 1;
    pmbcd.offset += (offset - pmbcd.offset) >> 4;
//  pmbcd.offset_dc += (offset_tick - pmbcd.offset_dc) >> 3;
    pmbcd.offset_cal_tick  = clock_time() | 1;
}
#else
_attribute_ram_code_sec_noinline_ void pm_ble_cal_32k_rc_offset (int offset_tick)
{
//  pmbcd.offset_cur = offset_tick;
    int offset = offset_tick * (240 * 31) / pmbcd.rc32;     //240ms / sleep_period
    if (offset > 0x100)
    {
        offset = 0x100;
    }
    else if (offset < -0x100)
    {
        offset = -0x100;
    }
    pmbcd.calib = 1;
    pmbcd.offset += (offset - pmbcd.offset) >> 4;
    pmbcd.offset_dc += (offset_tick - pmbcd.offset_dc) >> 3;
}
#endif
/**
 * @brief       32k rc calibration clock compensation.
 * @return      32k calibration value after compensation.
 */
_attribute_ram_code_ unsigned int pm_ble_get_32k_rc_calib (void)
{
    while(!read_reg32(0x140214));   //Wait for the 32k clock calibration to complete.
    int tc = 0;
    if(g_chip_version == 0x11)
    {
         tc = read_reg32(0x140214) - g_pm_tick_32k_calib_repair;
    }
    else
    {
         tc = read_reg32(0x140214);
    }
    pmbcd.s0 = tc;
    tc = tc << 4;
    if (!pmbcd.tc)
    {
        pmbcd.tc = tc;
    }
    else
    {
        pmbcd.tc += (tc - pmbcd.tc) >> (4 - pmbcd.calib);
    }

    int offset = (pmbcd.offset * (pmbcd.tc >> 4)) >> 18;        //offset : tick per 256ms
    offset = (pmbcd.tc >> 4) + offset;
    return (unsigned int)offset;
}











_attribute_ram_code_
unsigned int cpu_stall_WakeUp_By_RF_SystemTick(int WakeupSrc, unsigned short rf_mask, unsigned int tick)
{
    (void)WakeupSrc;(void)rf_mask;(void)tick;
    /*
   unsigned short rf_irq_mask_save =0;
   unsigned char systimer_irq_mask_save = 0;

   unsigned int r=core_interrupt_disable();

   rf_irq_mask_save = reg_rf_irq_mask;
   systimer_irq_mask_save = reg_system_irq_mask;


   unsigned int plic_save = 0;
   plic_save = reg_irq_src(0);

   plic_interrupt_enable(IRQ_SYSTIMER);
   plic_interrupt_enable(IRQ_ZB_BT);

    if( WakeupSrc & FLD_IRQ_ZB_RT_EN )
    {
        reg_rf_irq_mask = rf_mask;
    }

    if( WakeupSrc & FLD_IRQ_SYSTEM_TIMER )
    {
        stimer_set_irq_capture(tick);
        stimer_set_irq_mask(FLD_SYSTEM_IRQ);
    }

    core_entry_wfi_mode();//WFI instruction enables the processor to enter the wait-for-interrupt (WFI) mode

    reg_rf_irq_status = FLD_RF_IRQ_ALL;
    stimer_clr_irq_status(FLD_SYSTEM_IRQ);

    reg_irq_src(0) &= ~(BIT(IRQ_SYSTIMER)|BIT(IRQ_ZB_BT));
    reg_irq_src(0) |= plic_save;

    reg_rf_irq_mask = rf_irq_mask_save;
    reg_system_irq_mask = systimer_irq_mask_save;

    core_restore_interrupt(r);
*/
    return 0;
}


void blc_pm_select_internal_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_rc;
    pm_tim_recover          = pm_tim_recover_32k_rc;

    blt_miscParam.pm_enter_en   = 1; // allow enter pm, 32k rc does not need to wait for 32k clk to be stable
}



