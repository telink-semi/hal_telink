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

#include "../../adc.h"
#include "../../analog.h"
#include "../../dma.h"
#include "../../gpio.h"
#include "../../lib/include/pm.h"
#include "../../timer.h"
#include "../../flash.h"
#include "../../mdec.h"
#include "../../lib/include/trng.h"
#include "../../lib/include/sys.h"
#include "../../lib/include/plic.h"
#include "../../stimer.h"
#include "../../clock.h"
#include "../../compatibility_pack/cmpt.h"
#include "../../usbhw.h"
#include "../../mspi.h"
#include "../ext_misc.h"
#include "../ext_pm.h"
#include "ext_lib.h"
#include "../mcu_boot.h"
#include "common/config/user_config.h"
#include "ext_lib.h"

//Protection Code checking related macro
#define SDK_VERSION_S2          2
#define SDK_VERSION_IGNORE      5

#define SDK_VERSION_SELECT      SDK_VERSION_S2

typedef enum{
    SDK_VERSION_PROTECTION_CODE_S2      = 0x02, // The value of protection code is less than or equal to 9 but not equal to 3.
    SDK_VERSION_PROTECTION_CODE_IGNORE  = 0x05, // Ignore the value of protection code.
}sdk_version_protection_code_e;


_attribute_data_retention_sec_  _attribute_aligned_(4) misc_para_t      blt_miscParam;

_attribute_data_retention_sec_  suspend_handler_t   func_before_suspend = 0;

_attribute_data_retention_sec_  _attribute_data_retention_sec_  cpu_pm_handler_t            cpu_sleep_wakeup;  //no need retention,  cause it will be set every wake_up
_attribute_data_retention_sec_  pm_tim_recover_handler_t    pm_tim_recover;
_attribute_data_retention_sec_  check_32k_clk_handler_t     pm_check_32k_clk_stable = 0;
_attribute_data_retention_sec_  pm_get_32k_clk_handler_t    pm_get_32k_tick = 0;

_attribute_data_retention_sec_  pmb_clock_drift_t       pmbcd = {0, 0, 0, 0, 0, 0};

_attribute_data_retention_sec_  unsigned char   pm_check_info = 0;
_attribute_data_retention_sec_  unsigned int        ota_program_bootAddr = MULTI_BOOT_ADDR_0x40000; //default 256K
_attribute_data_retention_sec_  unsigned int        ota_firmware_max_size = (MULTI_BOOT_ADDR_0x40000 - 0x1000);  //unit: Byte, - 4K is important
_attribute_data_retention_sec_  unsigned int        ota_program_offset = 0;


extern unsigned int  efuse_get_low_word(void);


/**
 * @brief       This function serves to check protection code according SDK version.
 * @param[in]   version - SDK version.
 * @return      none.
 */
static __attribute__((always_inline)) inline void efuse_check_protection_code(sdk_version_protection_code_e version)
{
    unsigned char pCode;
    pCode = efuse_get_low_word() & 0xf;

    /* Eagle: IOT + LE Audio ( pCode == 0 || pCode == 2�� */
    if(pCode == 0 || pCode == 2){
        pm_check_info = 0xFF;
    }

    switch(version)
    {
    case SDK_VERSION_PROTECTION_CODE_S2:
        if((3 == pCode) || (9 < pCode))
        {
            start_reboot(); //reboot the MCU
        }
        break;
    default:
        break;
    }
}

void bls_pm_registerFuncBeforeSuspend (suspend_handler_t func )
{
    func_before_suspend = func;
}
/**
 * @brief       This function serves to kick external crystal.
 * @param[in]   kick_ms - duration of kick.
 * @return      none.
 */
#if 0
static _attribute_no_inline_ void pwm_kick_32k_pad(unsigned int kick_ms)
{


    write_reg8(0x14031e,0xfe);
    write_reg8(0x140336,0x02);
    write_reg8(0x140355,0x01);

    write_reg16(0x140414,0x01);
    write_reg16(0x140416,0x02);

    write_reg8(0x140402,0xb6);                      //12M/(0xb6 + 1)/2 = 32k
    write_reg8(0x140401,0x01);  //PWM_EN pwm0 enable
    //wait for PWM wake up Xtal
    delay_ms(kick_ms);

    write_reg8(0x14031e,0xff);
    write_reg8(0x140336,0xf0);

    write_reg8(0x140355,0x00);
    write_reg16(0x140418,0x00);
    write_reg16(0x14041a,0x00);
    write_reg8(0x140400,0x00);
    write_reg8(0x140402,0x00);


    //Recover PD0 as Xtal pin
//  write_reg8(0x1401e8,0x02);//default
//  write_reg8(0x1401d8,0x00);
}
#endif


#if (BLC_PM_EN)
static _attribute_no_inline_ void pwm_kick_32k_pad_times(unsigned int times)
{
    if(times){

        //1. select 32k xtal
        analog_write_reg8(0x4e, 0x95);//32k select:[7]:0 sel 32k rc,1:32k XTAL

        #if 0 //must close, reason is as follows:
            /*
             * This problem occurs with suspend and deep and deep retention. When the power supply voltage is low, suspend/deep/deep retention cannot
             * be reset within 12ms, otherwise softstart will work. However, there is not enough delay to wait for softstart to complete. This delay
             * will be postponed to the code execution area and it will not be able to handle larger code, otherwise it will be dropped by 1.8v, load
             * error, and finally stuck.(Root: DCDC dly depends on the 32k rc clock, so the 32k rc power supply can't be turned off here.)
             */
        analog_write_reg8(0x05, 0x01);//Power down 32KHz RC,  Power up [32KHz crystal, 24MHz RC, 24MHz XTAL,DCDC, VBUS_LDO, baseband pll LDO]
        #else
        analog_write_reg8(0x05, 0x00);//Power up 32KHz RC,  Power up 32KHz crystal
        #endif
        //condition: PCLK is 24MHZ,PCLK = HCLK
        write_reg8(0x1401d8,0x01);//PCLK = 12M
        write_reg8(0x1401e8,0x12);//24M crystal,cclk = pclk = 24Mhz
        delay_ms(1);


        int last_32k_tick;
        int curr_32k_tick;
        int i = 0;
        for(i = 0; i< times; i++){

            //After 10ms, the external 32k crystal clk is considered stable(when using PWM to accelerate the oscillation process)
            //A0 : GPIO_PD can't use for pwm kick. A1 :need check with lingyu,they will help to confirm kick function.
#if 0       //A1 need to do
            pwm_kick_32k_pad(10);//PWM kick external 32k pad (duration 10ms)
#else
            delay_ms(10*1000);
#endif
            //Check if 32k pad vibration and basically works stably
            last_32k_tick = pm_get_32k_tick();

            delay_us(305);//for 32k tick accumulator, tick period: 30.5us, dly 10 ticks

            curr_32k_tick = pm_get_32k_tick();

            if(last_32k_tick != curr_32k_tick){ //pwm kick 32k pad success
                break;
            }
        }

        #if (0) //blt_sdk_main_loop: check if 32k pad stable, if not, reboot MCU

            if(i >= times){
                analog_write_reg8(SYS_DEEP_ANA_REG, analog_read_reg8(SYS_DEEP_ANA_REG) & (~SYS_NEED_REINIT_EXT32K)); //clr
                start_reboot(); //reboot the MCU
            }
        #endif
    }
}
#endif

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
_attribute_text_sec_ _attribute_noinline_ void start_reboot(void)
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
#if (BLC_PM_EN)
    //Set 32k clk src: external 32k crystal, only need init when deep+pad wakeup or 1st power on
    if(blt_miscParam.pad32k_en){
#if 1
        //in this case: ext 32k clk was closed, here need re-init.
        //when deep_pad wakeup or first power on, it needs pwm acc 32k pad vibration time(dly 10ms)
        if(!(analog_read_reg8(SYS_DEEP_ANA_REG) & SYS_NEED_REINIT_EXT32K)){
            analog_write_reg8(SYS_DEEP_ANA_REG,  analog_read_reg8(SYS_DEEP_ANA_REG) | SYS_NEED_REINIT_EXT32K); //if initialized, the FLG is set to "1"

            pwm_kick_32k_pad_times(10);

        }
        else{
            delay_us(6000);
        }
#endif
    }
    else{
        //default 32k clk src: internal 32k rc, here can be optimized
        //analog_write(0x2d, 0x15); //32k select:[7]:0 sel 32k rc,1:32k pad
        //analog_write(0x05, 0x02); //Power down 32k crystal,  Power up [32KHz RC, 24MHz RC, 24MHz XTAL,DCDC, VBUS_LDO, baseband pll LDO]

        clock_cal_32k_rc();  //6.69 ms/6.7 ms
    }
#endif



#if 1
    //////////////////// get Efuse bit32~63 info ////////////////////
    unsigned short  efuse_4to18bit_info = (efuse_get_low_word() >> 4) & 0x7fff;
    if(0 != efuse_4to18bit_info)
    {
        if(adc_vref_cfg.adc_calib_en)
        {
        //Before the gain is stored in efuse, in order to reduce the number of bits occupied, 1000 is subtracted.
        //gpio_calib_value:bit[12:4]+1000
            adc_vref_cfg.adc_vref = (efuse_4to18bit_info & 0x1ff) + 1000;//unit: mv
        //gpio_calib_value_offset:bit[18:13]-20
            adc_vref_cfg.adc_vref_offset = ((efuse_4to18bit_info >> 9) & 0x3f) - 20;//unit: mv
            adc_set_gpio_calib_vref(adc_vref_cfg.adc_vref);
            adc_set_gpio_two_point_calib_offset(adc_vref_cfg.adc_vref_offset);
            blt_miscParam.adc_efuse_calib_flag = 1;
        }
    }
#endif


    //boot flag storage
    unsigned short boot_flag = read_reg16(0x140104);
    if (boot_flag)
    {
        ota_program_offset = 0;
    }
    else
    {
        ota_program_offset = ota_program_bootAddr;
    }


    //check protection code
#if SDK_VERSION_SELECT == SDK_VERSION_S2
    efuse_check_protection_code(SDK_VERSION_PROTECTION_CODE_S2);
#elif SDK_VERSION_SELECT == SDK_VERSION_IGNORE
    efuse_check_protection_code(SDK_VERSION_PROTECTION_CODE_IGNORE);
#endif
}


/**
 * @brief       This function serves to initialize system.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]   vbat_v      - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 * @return      none
 */
#if (BLC_PM_EN)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void sys_init(power_mode_e power_mode, vbat_type_e vbat_v, cap_typedef_e cap)
{
    /**
     * reset function will be cleared by set "1",which is different from the previous configuration.
     * This setting turns off the TRNG and NPE modules in order to test power consumption.The current
     * decrease about 3mA when those two modules be turn off.changed by zhiwei,confirmed by kaixin.20200828.
     */
    reg_rst     = 0xffbbffff;
    reg_clk_en  = 0xffbbffff;
    
    //Before calling the function pm_wait_xtal_ready, you need to ensure that the cclk is set to 24M, 
    //otherwise a reboot may occur (for example, the following use case: when the flash is running a cclk>24M program,
    //without powering down the chip to load a ram program, at this time the nop timing judgment in the pm_wait_xtal_ready function is incorrect,
    //which may lead to a reboot)(add by weihua.zhang, confirmed by kaixin 20230609)
    //When load code twice without power down DUT, DUT will use crystal clock in here, xo_quick_settle manual mode need to use in RC clock.
    write_reg8(0x1401e8, read_reg8(0x1401e8) & 0x0f);               //mspiclk & cclk to 24M rc clock
    write_reg8(0x1401d8, read_reg8(0x1401d8) & 0xf8);               //clock division to 1:1:1

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20240531)
    if(cap == EXTERNAL_CAP_XTAL24M)
    {
        rf_turn_off_internal_cap();
    }

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20231123)
    crystal_manual_settle();

    /* 
    * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
    * Because the stimer is necessary for the pm_wait_xtal_ready.
    * (add by jilong.liu, confirmed by wenfeng.lou 20240513)
    */
    analog_write_reg8(0x8c,0x02);       //<1>:reg_xo_en_clk_ana_ana=1

    //adjust by BLE, baseband module is enabled early to reserve time for the ZB.
    analog_write_reg8(0x7d, 0x80);      //poweron_dft:  0x03 -> 0x80.
                                        //<0>:pg_zb_en,     default:1,->0 power on baseband.
                                        //<1>:pg_usb_en,    default:1,->0 power on usb.
                                        //<2>:pg_npe_en,    default:1,->0 power on npe.
                                        //<7>:pg_clk_en,    default:0,->1 enable change power sequence clk.

    //add by BLE, must be placed after baseband power is turned on. driver SDK already deleted, BLE always retain.
    write_reg32(0x160000,0x20000000);   //reset CEVA module, power on/hardware reset will cause some BT register default value changed, need soft reset.

#if (DCDC_1P4_DCDC_1P8_EN)
    //when VBAT power supply > 4.1V and LDO switch to DCDC,DCDC_1V8 voltage will ascend to the supply power in a period time,
    //cause the program can not run. Need to trim down dcdc_flash_out before switch power mode.
    //confirmed by haitao,modify by yi.bao(20210119)
    if(DCDC_1P4_DCDC_1P8 == power_mode)
    {
        analog_write_reg8(0x0c, 0x40);  //poweron_dft: 0x44 --> 0x40.
                                        //<2:0> dcdc_trim_flash_out,flash/codec 1.8V/2.8V trim down 0.2V in DCDC mode.
    }
#endif

    analog_write_reg8(0x0a, power_mode);//poweron_dft:  0x90.
                                        //<0-1>:pd_dcdc_ldo_sw, default:00, dcdc & bypass ldo status bits.
                                        //      dcdc_1p4    dcdc_1p8    ldo_1p4     ldo_1p8
                                        //00:       N           N           Y           Y
                                        //01:       Y           N           N           Y
                                        //10:       Y           N           N           N
                                        //11:       Y           Y           N           N
    analog_write_reg8(0x0b, 0x3b);      //poweron_dft:  0x7b -> 0x3b.
                                        //<6>:mscn_pullup_res_enb,  default:1,->0 enable 1M pullup resistor for mscn PAD.
    analog_write_reg8(0x05,analog_read_reg8(0x05) & (~BIT(3)));//poweron_dft:   0x02 -> 0x02.
                                        //<3>:24M_xtl_pd,       default:0,->0 Power up 24MHz XTL oscillator.
    analog_write_reg8(0x06,(analog_read_reg8(0x06) | BIT(3)) & ~(BIT(0) | vbat_v | BIT(6) | BIT(7)));//poweron_dft: 0xff -> 0x36 or 0x3e.
                                        //<0>:pd_bbpll_ldo,     default:1,->0 Power on ana LDO.
                                        //<3>:pd_vbus_sw,       default:1,->0 Power up of bypass switch.
                                        //<6>:spd_ldo_pd,       default:1,->0 Power up spd ldo.
                                        //<7>:dig_ret_pd,       default:1,->0 Power up retention  ldo.
    analog_write_reg8(0x01, 0x45);      //poweron_dft:  0x44 -> 0x45.
                                        //<0-2>:bbpll_ldo_trim,         default:100,->101 measured 1.186V.The default value is sometimes crashes.
                                        //<4-6>:ana_ldo_trim,1.0-1.4V   default:100,->100 analog LDO output voltage trim: 1.2V
    //When using the default value, during the USB charging process, the audio output will hear a sizzling electric current.
    //This problem can be solved by increasing the OCP current limit value to avoid unnecessary shutdown.
    //confirmed by ya.yang, modify by weihua.zhang(20210805)
    analog_write_reg8(0x1c, 0x4c);      //poweron_dft:  0x40 -> 0x4c.
                                        //<2-3>:ocp_i_cross_trim,   default:00,->11 the current limit value of OCP is configured to the maximum value.

#if (0)//default value
    write_csr(NDS_MILMB,0x01);
    write_csr(NDS_MDLMB,0x80001);
#endif

    //in B91,the dma_mask is turned on by default and cleared uniformly during initialization.
    for(unsigned char dma_chn =0;dma_chn<= 7;dma_chn++)
    {
        dma_clr_irq_mask(dma_chn,TC_MASK|ERR_MASK|ABT_MASK);
    }

    //the usb ep mask is turned on by default and cleared uniformly during initialization.
    usbhw_clr_eps_irq_mask(FLD_USB_EDP8_IRQ|FLD_USB_EDP1_IRQ|FLD_USB_EDP2_IRQ|FLD_USB_EDP3_IRQ|FLD_USB_EDP4_IRQ|FLD_USB_EDP5_IRQ|FLD_USB_EDP6_IRQ|FLD_USB_EDP7_IRQ);

    //adjust by BLE, driver SDK use pm_update_status_info().
#if WDT_REBOOT_RESET_ANA7F_WORK_AROUND
    unsigned char analog_7f = analog_read_reg8(0x7f);
    unsigned char analog_38 = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);
    unsigned char analog_39 = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0);

    if(analog_38 & BIT(0)){
        if(analog_39 & BIT(0)){
            g_pm_status_info.mcu_status = MCU_STATUS_REBOOT_BACK;
            analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, analog_38 & 0xfe);
        }else{
            g_pm_status_info.mcu_status = MCU_STATUS_POWER_ON;
            analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0,analog_38 & 0xfe);
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_39 | BIT(0));
        }
    }else{
        if(!(analog_7f & 0x01)){
            g_pm_status_info.mcu_status = MCU_STATUS_DEEPRET_BACK;
        }else if(analog_39 & BIT(1)){
            g_pm_status_info.mcu_status = MCU_STATUS_REBOOT_DEEP_BACK;
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_39 & 0xfd);
        }else{
            g_pm_status_info.mcu_status = MCU_STATUS_DEEP_BACK;
        }
    }

    analog_write_reg8(0x7f, analog_7f | 0x01);
#else
    if( !(analog_read_reg8(0x7f) & 0x01) ){
        g_pm_status_info.mcu_status = MCU_STATUS_DEEPRET_BACK;
    }
#endif

    g_pm_vbat_v = vbat_v>>3;

    //The xo_ready_ana signal fails, and the tick value of the clock is used to determine whether the crystal oscillator is stable.
    //(add by jilong.liu, confirmed by wenfeng.lou 20240320. Issue:EAG-59)
    pm_wait_xtal_ready(0x00);

    //When bbpll_ldo_trim is set to the default voltage value, when doing high and low temperature stability tests,it is found that
    //there is a crash.The current workaround is to set other voltage values to see if it is stable.If it fails,repeat the setting
    //up to three times.The bbpll ldo trim must wait until 24M is stable.(add by weihua.zhang, confirmed by yi.bao and wenfeng 20200924)
    pm_wait_bbpll_done();

    //adjust by BLE. optimize efficiency.
#if (BLC_PM_EN)
    if(g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK){

        g_pm_status_info.wakeup_src = analog_read_reg8(0x64);
        if((analog_read_reg8(0x64) & WAKEUP_STATUS_TIMER_PAD ) == WAKEUP_STATUS_PAD)  //pad, no timer
        {
            g_pm_status_info.is_pad_wakeup = 1;
        }
        BM_CLR(reg_system_irq_mask, BIT(0));//disable system timer irq mask

    #if SYS_TIMER_AUTO_MODE
        REG_ADDR8(0x140218) = 0x02;//sys tick 16M set upon next 32k posedge
        reg_system_ctrl |= (FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN);

        //unsigned int deepRet_32k_tick = clock_get_digital_32k_tick();
        unsigned int deepRet_32k_tick = clock_get_32k_tick();

        unsigned int deepRet_tick = pm_tim_recover(deepRet_32k_tick + 1); // pm_tim_recover_32k_rc

        reg_system_tick = deepRet_tick + 1;

        //wait cmd set dly done upon next 32k posedge
        //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
        while((reg_system_st & BIT(7)) == 0);   //system timer set done status upon next 32k posedge

        REG_ADDR8(0x140218) = 0;//normal sys tick (16/sys) update
    #else
        BM_CLR(reg_system_ctrl, FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal and stimer

        //unsigned int deepRet_32k_tick = clock_get_digital_32k_tick();
        unsigned int deepRet_32k_tick = clock_get_32k_tick();

        unsigned int deepRet_tick = pm_tim_recover(deepRet_32k_tick);
        reg_system_tick = deepRet_tick;
        reg_system_ctrl |= (FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN); //enable 32k track and system timer. Wait for pll to stabilize before using system timer.
    #endif
    }
    else
#endif
    {
    #if SYS_TIMER_AUTO_MODE
        reg_system_ctrl |= (FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN); //enable 32k track and system timer auto.
        reg_system_tick = 0x01; //initial next tick is 1,kick system timer
    #else
        reg_system_ctrl |= (FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN); //enable 32k track and system timer. Wait for pll to stabilize before using system timer.
    #endif

        //add by BLE
        cpu_wakeup_no_deepretn_back_init(); // to save ramcode

        //add by BLE, calibrate 24M RC
        clock_cal_24m_rc();
    }

    //add by BLE, enable system timer irq mask.
    reg_system_irq_mask |= BIT(0);

    g_chip_version = read_reg8(0x1401fd);

    //if clock src is PAD or PLL, and hclk = (1/2)cclk, use reboot may cause problem, need deep to resolve(add by yi.bao, confirm by guangjun 20201016)
    if(g_pm_status_info.mcu_status == MCU_STATUS_REBOOT_BACK)
    {
        if(cpu_sleep_wakeup){
            //Use PM_ANA_REG_POWER_ON_CLR_BUF0 BIT(1) to represent the reboot+deep process, which is related to the function pm_update_status_info.
            analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | DEEP_AFTER_REBOOT);   //(add by weihua.zhang, confirmed by yi.bao 20201222)

            cpu_sleep_wakeup(DEEPSLEEP_MODE , PM_WAKEUP_TIMER, (clock_time () + 5*SYSTEM_TIMER_TICK_1MS));//TODO:deep time need to be validated
        }
    }
    #if (0)//disable by BLE, A0 chip are not considered.
    //**When testing AES_demo, it was found that the timing of baseband was wrong when it was powered on, which caused some of
    //the registers of CV to go wrong, which caused the program to run abnormally.(add by weihua.zhang, confirmed by junwen 20200819)
    else if(0xff == g_chip_version) //A0
    {
        if(g_pm_status_info.mcu_status == MCU_STATUS_POWER_ON)  //power on
        {
            if(cpu_sleep_wakeup){
                cpu_sleep_wakeup(DEEPSLEEP_MODE , PM_WAKEUP_TIMER, (clock_time () + 5*SYSTEM_TIMER_TICK_1MS));
            }
        }
    }
    #endif

#if (DCDC_1P4_DCDC_1P8_EN)
    //when VBAT power supply > 4.1V and LDO switch to DCDC,DCDC_1V8 voltage will ascend to the supply power in a period time,
    //cause the program can not run. Need to trim down dcdc_flash_out before switch power mode,refer to the configuration above [analog_write_reg8(0x0c, 0x40)],
    /*Then restore the default value[analog_write_reg8(0x0c, 0x44)].There is a process of switching from LDO to DCDC, which needs to wait for a period of time, so it is restored here,
    confirmed by haitao,modify by minghai.duan(20211018)*/
    if(DCDC_1P4_DCDC_1P8 == power_mode)
    {
        analog_write_reg8(0x0c, 0x44);  //poweron_dft: 0x40 --> 0x44.
                                        //<2:0> dcdc_trim_flash_out,flash/codec 1.8V/2.8V in DCDC mode.
    }
#endif

    //add by BLE, disable automatic data length setting for RF_TX_DMA_CHN0 (disable ch_0_rnum_en_bk).
    reg_rf_bb_auto_ctrl = 0; //default 0x04

    //add by BLE, must be put at the end.
    reg_embase_addr = 0xc0000000;//default is 0xc0200000;

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



/**
 * @brief   internal oscillator calibration for environment change such as voltage, temperature
 *          to keep some critical PM or RF performance stable
 *          attention: this is a stack API, user can not call it
 * @param   none
 * @return  none
 */
_attribute_data_retention_ int tick_rc24mCal = BIT(31);
void mcu_oscillator_crystal_calibration(void)
{
    if( clock_time_exceed(tick_rc24mCal, 59995*1000)){  //cal 24m every 10 second
        tick_rc24mCal = clock_time();
        clock_cal_24m_rc();  //469 us/474 us
    }
}

/**
 * @brief       When 32k rc sleeps, the calibration function is initialized.
 * @return      none.
 */
void pm_ble_32k_rc_offset_init(void)
{
//  if(g_pm_vbat_v){            //BLE SDK use not
        pmbcd.offset = 0;
//  }else{                      //BLE SDK use not
//      pmbcd.offset = -30;     //BLE SDK use not
//  }                           //BLE SDK use not
    pmbcd.tc = 0;
    pmbcd.ref_tick = 0;
//  g_pm_tick_update_en = 0;    //BLE SDK use not
}

/**
 * @brief       Update the reference 32k tick value and 16M system clock value when needed.
 * @param[in]   tick_32k    - the reference 32k tick value.
 * @param[in]   tick        - the reference 16M system clock value.
 * @return      none.
 */
_attribute_ram_code_            //BLE SDK use: ram_code
void pm_ble_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick)
{
    pmbcd.rc32_rt = tick_32k - pmbcd.rc32_wakeup; //rc32_rt not used
    if (pmbcd.calib || pmbcd.ref_no > 20 || !pmbcd.ref_tick || ((tick_32k - pmbcd.ref_tick_32k) & 0xfffffff) > 32 * 3000)       //BLE SDK use: 3 mS
    {
        pmbcd.calib = 0;
        pmbcd.ref_tick_32k = tick_32k;
        pmbcd.ref_tick = tick | 1;
        pmbcd.ref_no = 0;       //BLE SDK use
    }
    else
    {
        pmbcd.ref_no++;         //BLE SDK use
    }
}

_attribute_ram_code_sec_noinline_ void pm_ble_32k_rc_cal_reset(void)        //BLE SDK use
{
    pmbcd.offset = 0;
    pmbcd.tc = 0;
    pmbcd.ref_tick = 0;
    pmbcd.offset_cal_tick = 0;
}


_attribute_ram_code_sec_noinline_ void pm_ble_cal_32k_rc_offset (int offset_tick, int rc32_cnt)     //BLE SDK use
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


/**
 * @brief       32k rc calibration clock compensation.
 * @return      32k calibration value after compensation.
 */
_attribute_ram_code_                            //BLE SDK use ram_code
unsigned int pm_ble_get_32k_rc_calib (void)
{
    while(!read_reg32(0x140214));   //Wait for the 32k clock calibration to complete.

    int tc = read_reg32(0x140214);
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

void blc_pm_select_internal_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_rc;
    pm_tim_recover          = pm_tim_recover_32k_rc;

    blt_miscParam.pm_enter_en   = 1; // allow enter pm, 32k rc does not need to wait for 32k clk to be stable
}
extern  check_32k_clk_handler_t     pm_check_32k_clk_stable;
void blc_pm_select_external_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_xtal;
    pm_check_32k_clk_stable = check_32k_clk_stable;
    pm_tim_recover          = pm_tim_recover_32k_xtal;
    pm_get_32k_tick         = get_32k_tick;
    blt_miscParam.pad32k_en     = 1; // set '1': 32k clk src use external 32k crystal
}
