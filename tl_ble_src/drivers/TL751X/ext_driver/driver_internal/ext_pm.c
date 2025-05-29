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
#include "../../analog.h"
#include "../../dma.h"
#include "../../gpio.h"
#include "../../lib/include/pm/pm.h"
#include "../../adc.h"
#include "../../timer.h"
#include "../../flash.h"
#include "../../lib/include/trng/trng.h"
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
    __asm__("csrci  mmisc_ctl,8");  //disable BTB
    mcu_reboot();
    __asm__("csrsi  mmisc_ctl,8");  //enable BTB
}

/**
 * @brief       This function serves to initiate the cpu after power on or deepsleep mode.
 * @param[in]   none.
 * @return      none.
 */
_attribute_no_inline_ void cpu_wakeup_no_deepretn_back_init(void)
{
    /***
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

     ***/
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
    unsigned short boot_flag = reg_mspi_xip_core_offset(0);
    if (boot_flag)
    {
        ota_program_offset = 0;
    }
    else
    {
        ota_program_offset = ota_program_bootAddr;
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
_attribute_ram_code_ void sys_init11(void)
{

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


//_attribute_data_retention_ pm_clock_drift_t   pmbcd; //= {0, 0, 0, 0, 0, 0};


_attribute_ram_code_ void pm_ble_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick)
{
//  pmbcd.rc32_rt = tick_32k - pmbcd.rc32_wakeup; //rc32_rt not used
//  if (pmbcd.calib || pmbcd.ref_no > 20 || !pmbcd.ref_tick || ((tick_32k - pmbcd.ref_tick_32k) & 0xfffffff) > 32 * 3000)//3S
//  {
//      pmbcd.calib = 0;
//      pmbcd.ref_tick_32k = tick_32k;
//      pmbcd.ref_tick = tick | 1;
//      pmbcd.ref_no = 0;
//  }
//  else
//  {
//      pmbcd.ref_no++;
//  }
}

_attribute_ram_code_sec_noinline_ void pm_ble_32k_rc_cal_reset(void)
{
//  pmbcd.offset = 0;
//  pmbcd.tc = 0;
//  pmbcd.ref_tick = 0;
//  pmbcd.offset_cal_tick = 0;
}

#if 1
_attribute_ram_code_sec_noinline_ void pm_ble_cal_32k_rc_offset (int offset_tick, int rc32_cnt)
{
//  int offset = offset_tick * (256 * 31) / rc32_cnt;       //256mS / sleep_period
//  int thres = rc32_cnt/9600;  //240*32=7680  300*32= 9600  400*32= 12800
//  if(!thres){
//      thres = 1;
//  }
////    else if(thres > 8){
////        thres = 8;
////    }
//  thres *= 0x100;
//
//  if (offset > thres)
//  {
//      offset = thres;
//  }
//  else if (offset < -thres)
//  {
//      offset = -thres;
//  }
//  pmbcd.calib = 1;
//  pmbcd.offset += (offset - pmbcd.offset) >> 4;
////    pmbcd.offset_dc += (offset_tick - pmbcd.offset_dc) >> 3;
//  pmbcd.offset_cal_tick  = clock_time() | 1;
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
//  while(!read_reg32(0x140214));   //Wait for the 32k clock calibration to complete.
//  int tc = 0;
//  if(g_chip_version == 0x11)
//  {
//       tc = read_reg32(0x140214) - g_pm_tick_32k_calib_repair;
//  }
//  else
//  {
//       tc = read_reg32(0x140214);
//  }
//  pmbcd.s0 = tc;
//  tc = tc << 4;
//  if (!pmbcd.tc)
//  {
//      pmbcd.tc = tc;
//  }
//  else
//  {
//      pmbcd.tc += (tc - pmbcd.tc) >> (4 - pmbcd.calib);
//  }
//
//  int offset = (pmbcd.offset * (pmbcd.tc >> 4)) >> 18;        //offset : tick per 256ms
//  offset = (pmbcd.tc >> 4) + offset;
//  return (unsigned int)offset;
}











_attribute_ram_code_
unsigned int cpu_stall_WakeUp_By_RF_SystemTick(int WakeupSrc, unsigned short rf_mask, unsigned int tick)
{
    (void)WakeupSrc;(void)rf_mask;(void)tick;
    return 0;
}


void blc_pm_select_internal_32k_crystal(void)
{
    cpu_sleep_wakeup        = cpu_sleep_wakeup_32k_rc;
    pm_tim_recover          = pm_tim_recover_32k_rc;

    blt_miscParam.pm_enter_en   = 1; // allow enter pm, 32k rc does not need to wait for 32k clk to be stable
}



