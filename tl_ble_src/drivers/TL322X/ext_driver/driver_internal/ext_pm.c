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
#include "driver.h"
#include "../../compatibility_pack/cmpt.h"
#include "../ext_misc.h"
#include "../ext_pm.h"
#include "ext_lib.h"
#include "../mcu_boot.h"
#include "config/user_config.h"
#include "../../usb0hw.h"




_attribute_data_retention_  _attribute_aligned_(4) misc_para_t      blt_miscParam;

_attribute_data_retention_sec_  suspend_handler_t   func_before_suspend = 0;

_attribute_data_retention_sec_  cpu_pm_handler_t            cpu_sleep_wakeup;  //no need retention,  cause it will be set every wake_up
_attribute_data_retention_sec_  pm_tim_recover_handler_t    ext_pm_tim_recover;
_attribute_data_retention_sec_  check_32k_clk_handler_t     pm_check_32k_clk_stable = 0;

_attribute_data_retention_sec_  unsigned char   pm_check_info = 0;
_attribute_data_retention_sec_  unsigned int        ota_program_bootAddr = MULTI_BOOT_ADDR_0x40000; //default 256K
_attribute_data_retention_sec_  unsigned int        ota_firmware_max_size = (MULTI_BOOT_ADDR_0x40000 - 0x1000);  //unit: Byte, - 4K is important
_attribute_data_retention_sec_  unsigned int        ota_program_offset = 0;

//extern unsigned char efuse_read(unsigned char addr, unsigned char* buff, unsigned char len);
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

/**
 * @brief       This function serves to initiate the cpu after power on or deepsleep mode.
 * @param[in]   none.
 * @return      none.
 */
 _attribute_no_inline_ void cpu_wakeup_no_deepretn_back_init(void)
{


    if(blt_miscParam.pad32k_en){
        clock_32k_init(CLK_32K_XTAL);
        clock_kick_32k_xtal(10);
    }else {
        clock_32k_init(CLK_32K_RC);
        clock_cal_32k_rc(); //6.68ms
    }




    /*
     * 0,64K,128K,256K,512K,1M,2M,4M,8M
     */
    //boot flag storage
    //Only need read 1 byte. If read 2 bytes, in secure boot mode, erase err
//    unsigned short boot_flag = reg_mspi_xip_core_offset;
//    if (boot_flag)
//    {
//        ota_program_offset = 0;
//    }
//    else
//    {
//        ota_program_offset = ota_program_bootAddr;
//    }
}
//#include "../../lib/include/otp.h"
void efuse_check_protection_code(void)
{
    unsigned int pCode = 0;
    /* set otp active */
    otp_set_active_mode();
    otp_read(116, 1, (unsigned int *)&pCode);
    /* shutdown otp */
    otp_set_deep_standby_mode();
    unsigned char sdk_version = 3; //BLE :p3  Matter:P0
    switch(sdk_version)
    {
    case 0: //p0
        //Different SDKs have different restrictions. Please modify the code according to your own situation.
        //The driver here is only for example reference.
        if(0xFFFFFFFF > pCode)
        {
            sys_reset_all();
            while(1);
        }
        break;
    case 3: //p3
            if(0xFFFFFFFC > pCode)
            {
                sys_reset_all();
                while(1);
            }
        break;
    case 0xff:
        break;
    default:
        if(1) // Prevent macro setting exceptions from invalidating the ProtectionCode function
        {
            sys_reset_all();
            while(1);
        }
    }
}





/**
 * @brief      This function serves to determine whether wake up source is external 32k RC.
 * @param[in]  none.
 * @return     none.
 */
extern void check_32k_clk_stable(void);
//extern void pm_stimer_recover(void);
extern int cpu_sleep_wakeup_32k_xtal(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick);
_attribute_ram_code_ void blc_pm_select_external_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_xtal;
    pm_check_32k_clk_stable = check_32k_clk_stable;
    ext_pm_tim_recover      = pm_stimer_recover;
    g_clk_32k_src = CLK_32K_XTAL;
    blt_miscParam.pad32k_en     = 1; // set '1': 32k clk src use external 32k crystal

}

#if 0
/**
 * @brief      sys_init
 * @param[in]  none.
 * @return     none.
 */
void sys_init(power_mode_e power_mode, vbat_type_e vbat_v, cap_typedef_e cap)
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
    reg_rst      =  0x92390ed4;             //reset_0_dft:  0x96388080 -> 0x92380e80 -> 0x92390ed4
    reg_clk_en   =  0x12312ef4;             //clken_0_dft:  0x1630a0a0 -> 0x12302ea0 -> 0x12312ef4

    reg_rst_1    =  0x00000204;             //reset_1_dft:  0x00000004 -> 0x00000000 -> 0x00000204
    reg_clk_en_1 =  0x00000244;             //clken_1_dft:  0x00000044 -> 0x000000c0 -> 0x00000244
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

    //If the crystal oscillator uses an external capacitor, the internal capacitor must be turned off at the very beginning,
    //otherwise it will affect the start-up.(add by bingyu.li, confirmed by wenfeng.lou 20240621)
    if(cap != INTERNAL_CAP_XTAL24M)
    {
        rf_turn_off_internal_cap();
    }

//    analog_write_reg8(0x8b, analog_read_reg8(0x8b) | 0x08);

    //must to set xo_quick_settle with manual and wait it stable(added by jilong.liu, confirmed by wenfeng 20240320)
    crystal_manual_settle();

    /*
     * Increase the current of the crystal oscillator to avoid the poor product failure to vibrate.
     * This should better be operate as soon as possible.(aujusted by jilong.liu, confirmed by wenfeng.lou at 20240221)
     */
    analog_write_reg8(areg_aon_0x4e, (analog_read_reg8(areg_aon_0x4e) & (~FLD_XO_ISEL_PMU)) | 0x28);

    /*
     *                      poweron_dft:    0x7f -> 0x3f.
     *      pd_bit                      note
     * ---------------------------------------------------------------------------
     * <0>:pd_nvt_0p94          default:1,->1 power down native 0P94 dcdc.
     * <1>:pd_nvt_1p8           default:1,->1 power down native 1P8 dcdc.
     * <6>:mscn_pullup_res_enb  default:1,->0 enable 1M pullup resistor for mscn PAD.
     *
     * After waking up, it is not safe to power supply both the native LDO and the normal LDO together.
     * Therefore, this code will be processed in advance here to reduce the shared power supply time.(add by jilong.liu, confirmed by weihua.zhang 20240221)
     */
    analog_write_reg8(areg_aon_0x0b, (analog_read_reg8(areg_aon_0x0b) & (~FLD_MSCN_PULLUP_RES_ENB))| (FLD_PD_NVT_0P94 | FLD_PD_NVT_1P8));

    /*
    * Turn on xtal_24M clock to analog (includes stimer), this should setup before power up PLL and call pm_wait_xtal_ready.
    * Because the stimer is necessary for the pm_wait_xtal_ready.
    * (add by jilong.liu, confirmed by wenfeng.lou 20240513)
    */
    analog_write_reg8(areg_0x8c, 0x82);

    /*
     * Can be configured before the crystal oscillator stabilizes, which may allowing more time for PLL stabilization.
     * (modified by jilong.liu, confirmed by yangya 20240219)
     */
    extern void clock_bbpll_config(sys_pll_clk_e pll_clk);
    clock_bbpll_config(PLL_CLK_240M);

    /*
     *                      poweron_dft:    0x83 -> 0x82.
     *      bit                     note
     * ---------------------------------------------------------------------------
     * <0>:dcdc_cal_twohigh_en,     default:1,->0 disable calibrate the logic bug when two EA output is high
     * This will reduce power consumption and has little impact to operate early or late.
     */
    analog_write_reg8(areg_aon_0x02, analog_read_reg8(areg_aon_0x02) & (~FLD_DCDC_CAL_TWOHIGH_EN));

    /*
     *                      poweron_dft:    0x01 -> 0x03.
     *      bit                     note
     * ---------------------------------------------------------------------------
     * <1>:pd_PGA_bias,         default:0,->1 power down PGA bias current initial state.
     * This will reduce power consumption and has little impact to operate early or late.
     */
    analog_write_reg8(areg_0x8f, analog_read_reg8(areg_0x8f) | FLD_PGA_BIAS_PD);

    /* The default setting is power up, for safety reasons, power it up again here. */
    analog_write_reg8(areg_aon_0x05, analog_read_reg8(areg_aon_0x05) & ~(FLD_24M_XTAL_PD));

    sys_set_power_mode(power_mode);
    sys_set_vbat_type(vbat_v);

    pm_power_supply_config(CORE_0P8V_SRAM_0P8V_BB_0P8V);//power supply configuration

    pm_set_sleep_ldo_voltage();

    //A0: 0x00, A1: 0x80
    g_chip_version = read_reg8(0x14083d);
    extern void pm_update_status_info(void);
    pm_update_status_info();
    g_pm_vbat_v = vbat_v >> 3;

    pm_wait_xtal_ready(0x00);
    pm_wait_bbpll_done();

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
        cpu_wakeup_no_deepretn_back_init(); // to save ramcode

        //check protection code
        //efuse_check_protection_code();//BLE move by SunWei. //todo

        clock_cal_24m_rc();
    }
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);  //BLE must enable ronglu
    if(g_pm_status_info.mcu_status == MCU_STATUS_REBOOT_BACK)  //todo
    {
        //todo This is a temporary , and you need to locate the specific cause  ronglu
        cpu_sleep_wakeup(DEEPSLEEP_MODE , PM_WAKEUP_TIMER, (clock_time () + 5*SYSTEM_TIMER_TICK_1MS));//TODO:deep time need to be validated

    }
}
#endif




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
    while(!stimer_get_tracking_32k_value());
    int tc = stimer_get_tracking_32k_value();

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
    //todo
    return 0;
}


extern void pm_tim_recover_32k_rc(void);
extern   int cpu_sleep_wakeup_32k_rc(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick);
void blc_pm_select_internal_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_rc;
    ext_pm_tim_recover      = pm_tim_recover_32k_rc;
    g_clk_32k_src           = CLK_32K_RC;
    blt_miscParam.pm_enter_en   = 1; // allow enter pm, 32k rc does not need to wait for 32k clk to be stable
}
#if GENERATE_LIB_FOR_GOOGLE
extern volatile unsigned char g_pm_system_reboot_event;
_attribute_ram_code_ void pm_update_boot_info(void)
{

    if(g_pm_status_info.mcu_status == MCU_DEEPRET_BACK){
        return;
    }
    //PM_ANA_REG_POWER_ON_CLR_BUF2 default value is 0xff, so if its BIT(0)=0, than it is 32K_WD reboot back
    unsigned char analog_3c = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF2);
    if(!(analog_3c&BIT(0))){
        g_pm_status_info.mcu_status = MCU_HW_REBOOT_32K_WATCHDOG;
        return;
    }
    unsigned char wd_clr0 = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);

    if(wd_clr0 & POWERON_FLAG){
        unsigned char poweron_clr0 = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0);
        if(poweron_clr0 & REBOOT_FLAG){
            g_pm_status_info.mcu_status = MCU_SW_REBOOT_BACK;
            if(wd_get_status()){
                g_pm_status_info.mcu_status = MCU_HW_REBOOT_TIMER_WATCHDOG;
            }else{
                g_pm_system_reboot_event = (poweron_clr0>>1);
            }
        }else{
            g_pm_status_info.mcu_status = MCU_POWER_ON;
            if(wd_32k_get_status()){
                g_pm_status_info.mcu_status = MCU_HW_REBOOT_32K_WATCHDOG;
                analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF2, analog_3c&(~BIT(0)));
            }
        }
    }else{
        g_pm_status_info.mcu_status = MCU_DEEP_BACK;
    }
}

_attribute_ram_code_ void pm_clear_boot_info(void)
{
    if(g_pm_status_info.mcu_status == MCU_DEEPRET_BACK){
        return;
    }
    unsigned char wd_clr0 = analog_read_reg8(PM_ANA_REG_WD_CLR_BUF0);
    unsigned char analog_3c = analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF2);
    if(wd_clr0 & POWERON_FLAG){
        analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, wd_clr0 & (~POWERON_FLAG));
    }else{
        analog_write_reg8(PM_ANA_REG_WD_CLR_BUF0, wd_clr0 & (~POWERON_FLAG));
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, REBOOT_FLAG);
    }
    if(!(analog_3c&BIT(0))){
        analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF2, analog_3c|BIT(0));
    }
}
#endif /* GENERATE_LIB_FOR_GOOGLE */
