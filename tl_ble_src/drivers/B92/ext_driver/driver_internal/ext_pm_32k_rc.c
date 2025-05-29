/********************************************************************************************************
 * @file    ext_pm_32k_rc.c
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
#include "../../lib/include/pm.h"
#include "gpio.h"
#include "compiler.h"
#include "core.h"
#include "timer.h"
#include "../../lib/include/sys.h"
#include "clock.h"
#include "mspi.h"
#include "flash.h"
#include "stimer.h"
#include "../../lib/include/analog.h"
#include "compatibility_pack/cmpt.h"
#include "../ext_pm.h"
#include "ext_lib.h"

//extern pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us;
//extern pm_r_delay_cycle_s g_pm_r_delay_cycle;
extern unsigned char rc_24m_power;
extern unsigned char bbpll_power;
extern unsigned int             g_pm_tick_32k_calib_repair;
extern unsigned char    g_pm_pad_filter_en;
extern  unsigned int            g_pm_interrupt_status;
extern  unsigned int            g_pm_interrupt_status1;
extern  unsigned int            g_pm_interrupt_status2;
#if PM_32k_RC_CALIBRATION_ALGORITHM_EN

extern void pm_ble_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick);
extern unsigned int pm_ble_get_32k_rc_calib (void);

#endif

extern  void  pm_sleep_start(pm_sleep_mode_e sleep_mode);

_attribute_data_retention_sec_  unsigned int        g_sleep_32k_rc_cnt;
_attribute_data_retention_sec_  unsigned int        g_sleep_stimer_tick;
/**
 * @brief      This function serves to set the working mode of MCU,e.g. suspend mode, deepsleep mode, deepsleep with SRAM retention mode and shutdown mode.
 * @param[in]  sleep_mode - sleep mode type select.
 * @param[in]  wakeup_src - wake up source select.
 * @param[in]  wakeup_tick - the time of short sleep, which means MCU can sleep for less than 5 minutes.
 * @return     indicate whether the cpu is wake up successful.
 */
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
_attribute_ram_code_ int cpu_sleep_wakeup_32k_rc_ram(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
{
    g_sleep_32k_rc_cnt = 0;
    g_sleep_stimer_tick = 0;
    ////////// disable IRQ //////////////////////////////////////////
    unsigned int r= core_interrupt_disable();
    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    unsigned int span = (unsigned int)(wakeup_tick - clock_time ());
    if(timer_wakeup_enable){
        if (span > 0xE0000000){ //BIT(31)+BIT(30)+BIT(19)   7/8 cycle of 32bit, 268.44*7/8 = 234.88 S
            core_restore_interrupt(r);
            return  analog_read_reg8 (0x64) & 0x7f;
        }
        else if (span < g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US){ // EMPTYRUN_TIME_US   0 us base
            unsigned int t = clock_time ();
            analog_write_reg8 (0x64, 0xff);         //(clear all status 0x1f) but clear the timer wake_up status(bit[1])
                                                //before read 32k tick may cause the tick number error get from 32K.
            unsigned char st;
            do {
                st = analog_read_reg8 (0x64) & 0x7f;   //clear the timer wake_up status(bit[1]) before read 32k tick may
                                                  //cause the tick number error get from 32K.
            } while ( ((unsigned int)clock_time () - t < span) && !st);
            core_restore_interrupt(r);
            return st;
        }
#if !PM_32k_RC_CALIBRATION_ALGORITHM_EN
        else{
            if( span > 0x0ff00000 ){  //BIT(28) = 0x10000000   16M:16S
                g_pm_long_suspend = 1;
            }
            else{
                g_pm_long_suspend = 0;
            }
        }
#endif
    }

    //Turn off all interrupts immediately after entering the sleep function to prevent other interrupts, save the interrupt state before turning off, and restore it after waking up.
    //Turn on pm interrupt only before going to sleep,enable M-mode external interrupt first in this place.
    //modify by bingyu.li, confirmed by jianzhi.chen at 20230810.
    g_pm_interrupt_status = reg_irq_src0;
    g_pm_interrupt_status1 = reg_irq_src1;
    g_pm_interrupt_status2 = read_csr(NDS_MIE);
    reg_irq_src0 = 0;
    reg_irq_src1 = 0;
    core_mie_enable(FLD_MIE_MEIE);
    core_mie_disable(FLD_MIE_MSIE|FLD_MIE_MTIE);

    //The problem of inaccurate sleep time is solved by compensating 32K calibration value.
    if((bbpll_power==0)&&(rc_24m_power==0)){
        g_pm_tick_32k_calib_repair = 0x00000019;
    } else if ((bbpll_power==1)&&(rc_24m_power==0)){
        g_pm_tick_32k_calib_repair = 0x0000000d;
    } else if ((bbpll_power==0)&&(rc_24m_power==1)){
        g_pm_tick_32k_calib_repair = 0x0000000e;
    }

    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN//BLE ADD. by SunWei
        g_pm_tick_32k_calib = pm_ble_get_32k_rc_calib ();
    #else
        while(!read_reg32(0x140214));
        if(g_chip_version == 0x11){
            g_pm_tick_32k_calib = read_reg32(0x140214) - g_pm_tick_32k_calib_repair;
        } else {
            g_pm_tick_32k_calib = read_reg32(0x140214);
        }
    #endif
    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;

    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    mspi_stop_xip();
    unsigned char cclk_reg = read_reg8(0x1401e8);
    write_reg8(0x1401e8, cclk_reg & 0x8f);              //change cclk to 24M rc clock
    unsigned char div_reg = read_reg8(0x1401d8);
    write_reg8(0x1401d8, div_reg & 0xf8);               //change clock division to 1:1:1
    unsigned char mspiclk_reg = read_reg8(0x1401c0);
    write_reg8(0x1401c0, (mspiclk_reg & 0x80) | 0x01);  //change mspiclk to 24M rc clock

#if SYS_TIMER_AUTO_MODE
    BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer

    REG_ADDR8(0x140218) = 0x01;                       //system tick only update upon 32k posedge, must set before enable 32k read update!!!
    BM_CLR(reg_system_ctrl, FLD_SYSTEM_32K_TRACK_EN); //disable 32k track
    g_pm_tick_32k_cur = clock_get_32k_tick();
    g_pm_tick_cur     = stimer_get_tick();
    BM_SET(reg_system_st, FLD_SYSTEM_CMD_STOP);       //write 1, stop system timer when using auto mode
    REG_ADDR8(0x140218) = 0x00;
#else
    g_pm_tick_cur = clock_time () + 37 * SYSTEM_TIMER_TICK_1US;  //clock_get_32k_tick will cost 30~40 us

    BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer
    BM_CLR(reg_system_ctrl,FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal and stimer

    //g_pm_tick_32k_cur = clock_get_digital_32k_tick();
    g_pm_tick_32k_cur = clock_get_32k_tick();
#endif
    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
    pm_ble_update_32k_rc_sleep_tick (g_pm_tick_32k_cur, g_pm_tick_cur);//Todo:need check
    #endif
    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8 (0x4b, wakeup_src);
    analog_write_reg8 (0x64, 0xff);             //clear all flag
    analog_write_reg8(0x7e, sleep_mode);//sram retention

    unsigned int earlyWakeup_us;
#if !SYS_TIMER_AUTO_MODE
    unsigned int tick_adjust_us = 0;
#endif
    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG) { //deepsleep with retention
        extern void pm_retention_register_save(void);
        pm_retention_register_save();//after retention, flash/debug cipher encryption keys is lost, save it
        //0x00->0xd1
        //<0>pd_rc32k_auto=1 <4>pwdn power suspend ldo=1
        //<6>power down sequence enable=1 <7>enable isolation=1
        if( wakeup_src & PM_WAKEUP_COMPARATOR || wakeup_src & PM_WAKEUP_CTB ){
            analog_write_reg8(0x4d,0xfe);//retention
        }else{
            analog_write_reg8(0x4d,0xff);//retention
        }
        analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0));   //<7-5>:ldo_main_trim,  default:100,->111 digital LDO output voltage trim: 1.15V
        analog_write_reg8(0x09, (analog_read_reg8(0x09) & 0x3f));       //<6>:pd_sw_dcore,  default:1,->0 power up the main dig ldo to dcore.
                                                                        //<7>:pd_sw_sram,   default:1,->0 power up the main dig ldo to sram.
        analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xf0) & 0x7f);//<4>:pd_ldo_dcore, default:0,->1 Power down of digital core ldo.
                                                                        //<5>:pd_ldo_sram,  default:0,->1 Power down of sram ldo.
                                                                        //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
                                                                        //<7>:dig_ret_pd,   default:1,->0 Power on retention ldo.

        g_pm_mspi_cfg = read_reg32(0x23FFFF20);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*((wakeup_tick - g_pm_tick_cur)/160000))*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us ;
    } else if (sleep_mode == DEEPSLEEP_MODE) {  //deepsleep no retention
        if(wakeup_src & PM_WAKEUP_COMPARATOR || wakeup_src & PM_WAKEUP_CTB ) {
            analog_write_reg8(0x4d,0xfe);//deep
        } else {
            analog_write_reg8(0x4d,0xff);//deep
        }
        analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0));   //<7-5>:ldo_main_trim,  default:100,->111 digital LDO output voltage trim: 1.15V
        analog_write_reg8(0x09, (analog_read_reg8(0x09) & 0x3f));   //<6>:pd_sw_dcore,  default:1,->0 power up the main dig ldo to dcore.
                                                                    //<7>:pd_sw_sram,   default:1,->0 power up the main dig ldo to sram.
        analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xf0));   //<4>:pd_ldo_dcore, default:0,->1 Power down of digital core ldo.
                                                                    //<5>:pd_ldo_sram,  default:0,->1 Power down of sram ldo.
                                                                    //<6>:spd_ldo_pd,   default:1,->1 Power down spd ldo.
                                                                    //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*(wakeup_tick - g_pm_tick_cur)/160000)*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
#endif
        earlyWakeup_us = g_pm_early_wakeup_time_us.deep_early_wakeup_time_us;
    }
    else{  //suspend
        if(wakeup_src & PM_WAKEUP_COMPARATOR || wakeup_src & PM_WAKEUP_CTB) {
            analog_write_reg8(0x4d,0x7e);//suspend
        } else {
            analog_write_reg8(0x4d,0x7f);//suspend
        }
        analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0));   //<7-5>:ldo_main_trim,  default:100,->111 digital LDO output voltage trim: 1.15V
        analog_write_reg8(0x09, (analog_read_reg8(0x09) & 0x3f));       //<6>:pd_sw_dcore,  default:1,->0 power up the main dig ldo to dcore.
                                                                        //<7>:pd_sw_sram,   default:1,->0 power up the main dig ldo to sram.
        analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xf0) & 0xbf);//<4>:pd_ldo_dcore, default:0,->1 Power down of digital core ldo.
                                                                        //<5>:pd_ldo_sram,  default:0,->1 Power down of sram ldo.
                                                                        //<6>:spd_ldo_pd,   default:1,->0 Power up spd ldo.
                                                                        //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*(wakeup_tick - g_pm_tick_cur)/160000)*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
#endif
        earlyWakeup_us = g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us;
    }

    //auto power down
    if(((wakeup_src & PM_WAKEUP_PAD) && g_pm_pad_filter_en) || (wakeup_src & PM_WAKEUP_TIMER)/*|| (wakeup_src & PM_WAKEUP_MDEC)*/|| (wakeup_src & PM_WAKEUP_COMPARATOR) ){
        analog_write_reg8(0x4c,0xee);
    } else {
        analog_write_reg8(0x4c, 0xef);
    }

    //set DCDC delay duration
    if(sleep_mode == DEEPSLEEP_MODE) {
        analog_write_reg8 (0x3d, g_pm_r_delay_cycle.deep_xtal_delay_cycle);
        analog_write_reg8 (0x3e, g_pm_r_delay_cycle.deep_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    } else {
        analog_write_reg8 (0x3d, g_pm_r_delay_cycle.suspend_ret_xtal_delay_cycle);
        analog_write_reg8 (0x3e, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }
    unsigned int tick_reset;
    if(timer_wakeup_enable){

        unsigned int tick_wakeup_reset = wakeup_tick - earlyWakeup_us * SYSTEM_TIMER_TICK_1US;
        #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
            if( (unsigned int)(tick_wakeup_reset - pmbcd.ref_tick) > 0x0ff00000 ){  //BIT(28) = 0x10000000   16M:16S
                tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - g_pm_tick_cur)/ g_pm_tick_32k_calib * g_track_32kcnt;
                g_pm_long_suspend = 1;
            }
            else{
                tick_reset = g_pm_tick_32k_cur + ((unsigned int)(tick_wakeup_reset - g_pm_tick_cur) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                g_pm_long_suspend = 0;
            }

        #else
            #if !SYS_TIMER_AUTO_MODE
                if(g_pm_long_suspend){
                    tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - tick_adjust_us - g_pm_tick_cur)/ g_pm_tick_32k_calib * g_track_32kcnt;
                }
                else{
                    tick_reset = g_pm_tick_32k_cur + ((unsigned int)(tick_wakeup_reset - tick_adjust_us - g_pm_tick_cur) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                }
            #else
                if(g_pm_long_suspend){
                    tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset  - g_pm_tick_cur)/ g_pm_tick_32k_calib * g_track_32kcnt;
                }
                else{
                    tick_reset = g_pm_tick_32k_cur + ((unsigned int)(tick_wakeup_reset - g_pm_tick_cur) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib;
                }
            //  tick_reset = g_pm_tick_32k_cur + ((unsigned int)(wakeup_tick - g_pm_tick_cur) * g_track_32kcnt + tick_32k_halfCalib - tick_32k_halfCalib ) / g_pm_tick_32k_calib  - 1 - 3*2 - 2;
            #endif
        #endif

        clock_set_32k_tick(tick_reset);
    }



    if(analog_read_reg8(0x64)&0x1f){

    } else {
        if (sleep_mode & DEEPSLEEP_RETENTION_FLAG) {
            g_areg_aon_7f = (g_areg_aon_7f & 0xfe) | g_pm_pad_filter_en;
        } else {
            g_areg_aon_7f = (g_areg_aon_7f | 0x01 | g_pm_pad_filter_en);
        }
        analog_write_reg8(0x7f, g_areg_aon_7f);

        pm_sleep_start(sleep_mode);
    }
    if(sleep_mode == DEEPSLEEP_MODE){
       write_reg8 (0x1401ef, 0x20);  //reboot
    }
    analog_write_reg8(0x06, (analog_read_reg8(0x06) | 0xc0) & 0xcf);//<4>:pd_ldo_dcore, default:1,->0 Power up of digital core ldo.
                                                                        //<5>:pd_ldo_sram,  default:1,->0 Power up of sram ldo.
                                                                        //<6>:spd_ldo_pd,   default:0,->1 Power down spd ldo.
                                                                        //<7>:dig_ret_pd,   default:1,->1 Power down retention ldo.
    analog_write_reg8(0x09, (analog_read_reg8(0x09) | 0xc0));       //<6>:pd_sw_dcore,  default:0,->1 power down the main dig ldo to dcore.
                                                                        //<7>:pd_sw_sram,   default:0,->1 power down the main dig ldo to sram.
    analog_write_reg8(0x00, (analog_read_reg8(0x00) | 0xe0) & 0x9f);//<7-5>:ldo_main_trim,  default:111,->100 digital LDO output voltage trim: 1.0V


    reg_system_irq_mask |= BIT(0);
#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x02;//sys tick 16M set upon next 32k posedge
    reg_system_ctrl     |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN) ;

    //unsigned int now_tick_32k = clock_get_digital_32k_tick() + 1;
    unsigned int now_tick_32k = clock_get_32k_tick() + 1;
    u32 now_tick_stimer;
    #if 1  //SiHui fix
        if(g_pm_long_suspend){
            now_tick_stimer = pmbcd.ref_tick + (unsigned int)(now_tick_32k - pmbcd.ref_tick_32k) / g_track_32kcnt * g_pm_tick_32k_calib;
        } else {
            now_tick_stimer = pmbcd.ref_tick + (unsigned int)(now_tick_32k - pmbcd.ref_tick_32k) * g_pm_tick_32k_calib / g_track_32kcnt;        // current clock
        }
    #endif
    g_sleep_32k_rc_cnt = now_tick_32k - g_pm_tick_32k_cur;
    g_sleep_stimer_tick = now_tick_stimer - g_pm_tick_cur;

    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
    pmbcd.rc32_wakeup = now_tick_32k;
    pmbcd.rc32 = now_tick_32k - pmbcd.ref_tick_32k;
    #endif

    reg_system_tick = now_tick_stimer + 1;

    while((reg_system_st & BIT(7)) == 0);
    REG_ADDR8(0x140218) = 0;//normal sys tick (16/sys) update

#else
    //unsigned int now_tick_32k = clock_get_digital_32k_tick();
    unsigned int now_tick_32k = clock_get_32k_tick();
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        }
        else{
            g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;       // current clock
        }
    }

    reg_system_tick = g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US;
    reg_system_ctrl |= (FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN);    //enable 32k cal and stimer
#endif
    write_reg8(0x1401d8, div_reg);      //restore div
    write_reg8(0x1401e8, cclk_reg);     //restore cclk
    write_reg8(0x1401c0, mspiclk_reg);  //restore mspiclk
    mspi_set_xip_en();

    if( (g_pm_status_info.wakeup_src & WAKEUP_STATUS_TIMER) && timer_wakeup_enable )    //wakeup from timer only
    {
            while ((unsigned int)(stimer_get_tick() - wakeup_tick) > BIT(30));
    }
    //Resume the interrupted state before sleep.Cannot be placed in the pm_sleep_start() interface to avoid failure to recover if this interface is not called.
    //changed by weihua, confirmed by jianzhi. 20231115
    reg_irq_src0 = g_pm_interrupt_status;
    reg_irq_src1 = g_pm_interrupt_status1;
    write_csr(NDS_MIE,g_pm_interrupt_status2);
    core_restore_interrupt(r);
    return ( g_pm_status_info.wakeup_src  ? (g_pm_status_info.wakeup_src | STATUS_ENTER_SUSPEND ) : STATUS_GPIO_ERR_NO_ENTER_PM );
}

_attribute_text_sec_ _attribute_no_inline_ int cpu_sleep_wakeup_32k_rc(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
{

    if(func_before_suspend){
        if (!func_before_suspend())
        {
            return WAKEUP_STATUS_PAD;
        }
    }


    int status = 0;
    DISABLE_BTB;
    status = cpu_sleep_wakeup_32k_rc_ram(sleep_mode, wakeup_src, wakeup_tick);
    ENABLE_BTB;
    return status;
}


_attribute_ram_code_ unsigned int pm_tim_recover_32k_rc(unsigned int now_tick_32k)
{
    unsigned int deepRet_tick;


#if PM_32k_RC_CALIBRATION_ALGORITHM_EN
    if(g_pm_long_suspend){
        deepRet_tick = pmbcd.ref_tick  + (unsigned int)(now_tick_32k - pmbcd.ref_tick_32k) / g_track_32kcnt * g_pm_tick_32k_calib;
    }
    else{
        deepRet_tick = pmbcd.ref_tick  + (unsigned int)(now_tick_32k - pmbcd.ref_tick_32k) * g_pm_tick_32k_calib / g_track_32kcnt;      // current clock
    }
#else
    if(g_pm_long_suspend){
        deepRet_tick = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
    }
    else{
        deepRet_tick = g_pm_tick_cur + (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;     // current clock
    }
#endif

    g_sleep_32k_rc_cnt = now_tick_32k - g_pm_tick_32k_cur;
    g_sleep_stimer_tick = deepRet_tick - g_pm_tick_cur;

    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
        pmbcd.rc32_wakeup = now_tick_32k;
        pmbcd.rc32 = now_tick_32k - pmbcd.ref_tick_32k;
    #endif
    return deepRet_tick;
}
#if 0
int cpu_long_sleep_wakeup_32k_rc(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, unsigned int  wakeup_tick)
{
    int sys_tick0 = clock_time();
    int timer_wakeup_enable = (wakeup_src & PM_WAKEUP_TIMER);
    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
        g_pm_tick_32k_calib = pm_ble_get_32k_rc_calib ();
    #else
        while(!read_reg32(0x140214));
        g_pm_tick_32k_calib = read_reg32(0x140214);
    #endif
    unsigned int  tick_32k_halfCalib = g_pm_tick_32k_calib>>1;
    unsigned int span = (unsigned int)(wakeup_tick);
    if(timer_wakeup_enable){
        if (span < ((g_pm_early_wakeup_time_us.sleep_min_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib)){
            analog_write_reg8 (0x64, 0x1d);         //(clear all status 0x1f) but clear the timer wake_up status(bit[1])
                                                //before read 32k tick may cause the tick number error get from 32K.
            unsigned char st;
            do {
                st = analog_read_reg8 (0x64) & 0x1d;   //clear the timer wake_up status(bit[1]) before read 32k tick may
                                                  //cause the tick number error get from 32K.
            } while ( ((((unsigned int)clock_time () - sys_tick0 + tick_32k_halfCalib )/g_pm_tick_32k_calib) < span) && !st);
            return st;
        }
    }
    g_pm_long_suspend = 1;
    ////////// disable IRQ //////////////////////////////////////////
    unsigned int r= core_interrupt_disable();

    if(func_before_suspend){
        if (!func_before_suspend())
        {
            core_restore_interrupt(r);
            return WAKEUP_STATUS_PAD;
        }
    }

    //The clock source of analog is pclk, that is, the speed of reading and writing analog registers is related to cclk and pclk, before cclk=24M pclk=24M hclk=24M,
    //when the clock is switched to 24M RC before sleep, pclk is still 24M, this approach is no problem, and the early wake-up time in the pm function is calculated according to this clock.
    //When cclk=96M, the execution speed of the code will become faster, and when cclk is switched to 24M RC, pclk=6M will cause the analog register time to become longer,
    //which will cause deviations in the calculation of the early wake-up time in the previous pm function.modify by junhui.hu, confirmed by jianzhi at 20210923.
    unsigned char cclk_reg = read_reg8(0x1401e8);
    write_reg8(0x1401e8, cclk_reg & 0x8f );//change cclk to 24M rc clock
    unsigned char div_reg = read_reg8(0x1401d8);
    write_reg8(0x1401d8, div_reg & 0xf8);//change clock division

#if SYS_TIMER_AUTO_MODE
    BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer

    REG_ADDR8(0x140218) = 0x01;//sys tick only update upon 32k posedge,must set before enable 32k read update!!!

    BM_CLR(reg_system_ctrl,FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal

    //g_pm_tick_32k_cur = clock_get_digital_32k_tick();
    g_pm_tick_32k_cur=clock_get_32k_tick();
    reg_system_st = FLD_SYSTEM_CMD_STOP;//stop stimer
    g_pm_tick_cur = clock_time ();
#else
    g_pm_tick_cur = clock_time () + 37 * SYSTEM_TIMER_TICK_1US;  //clock_get_32k_tick will cost 30~40 us

    BM_CLR(reg_system_irq_mask,BIT(0));//disable 32k cal and stimer
    BM_CLR(reg_system_ctrl,FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_TIMER_AUTO | FLD_SYSTEM_32K_TRACK_EN);//disable 32k cal and stimer

    //g_pm_tick_32k_cur = clock_get_digital_32k_tick();
    g_pm_tick_32k_cur = clock_get_32k_tick();
#endif
    #if PM_32k_RC_CALIBRATION_ALGORITHM_EN
        pmbcd.calib = 1;
        pm_ble_update_32k_rc_sleep_tick (g_pm_tick_32k_cur, g_pm_tick_cur);
    #endif
    /////////////////// set wakeup source /////////////////////////////////
    analog_write_reg8 (0x4b, wakeup_src);
    analog_write_reg8 (0x64, 0x1f);             //clear all flag


    analog_write_reg8(0x7e, sleep_mode);//sram retention

    unsigned int earlyWakeup_us;
#if !SYS_TIMER_AUTO_MODE
    unsigned int tick_adjust_us = 0;
#endif
    if(sleep_mode & DEEPSLEEP_RETENTION_FLAG) { //deepsleep with retention
        //0x00->0xd1
        //<0>pd_rc32k_auto=1 <4>pwdn power suspend ldo=1
        //<6>power down sequence enable=1 <7>enable isolation=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0xd0);//retention
        }
        else
        {
            analog_write_reg8(0x4d,0xd1);//retention
        }
        #if (!WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
            analog_write_reg8(0x7f, 0x00);
        #endif

        g_pm_mspi_cfg = read_reg32(0x23FFFF20);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*((wakeup_tick - g_pm_tick_cur)/160000))*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = ((g_pm_early_wakeup_time_us.deep_ret_early_wakeup_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib);
    }
    else if(sleep_mode == DEEPSLEEP_MODE){  //deepsleep no retention
        //0x00->0xf9
        //<0>pd_rc32k_auto=1 <3>rst_xtal_quickstart_cnt=1 <4>pwdn power suspend ldo=1
        //<5>pwdn power retention ldo=1 <6>power down sequence enable=1 <7>enable isolation=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0xf8);//deep
        }
        else
        {
            analog_write_reg8(0x4d,0xf9);//deep
        }
        analog_write_reg8(0x7f, 0x01);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*(wakeup_tick - g_pm_tick_cur)/160000)*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = ((g_pm_early_wakeup_time_us.deep_early_wakeup_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib);
    }
    else{  //suspend
        //0x00->0x61
        //<0>pd_rc32k_auto=1 <5>pwdn power retention ldo=1 <6>power down sequence enable=1
        if(wakeup_src & PM_WAKEUP_COMPARATOR)
        {
            analog_write_reg8(0x4d,0x60);//suspend
        }
        else
        {
            analog_write_reg8(0x4d,0x61);//suspend
        }
        analog_write_reg8(0x7f, 0x01);
#if !SYS_TIMER_AUTO_MODE
        tick_adjust_us = (6*(wakeup_tick - g_pm_tick_cur)/160000)*SYSTEM_TIMER_TICK_1US;//10ms -- 3us
//      tick_adjust_us = ((wakeup_tick - g_pm_tick_cur)/800000) * 16 * SYSTEM_TIMER_TICK_1US;//50ms -- 16us
#endif
        earlyWakeup_us = ((g_pm_early_wakeup_time_us.suspend_early_wakeup_time_us * SYSTEM_TIMER_TICK_1US + tick_32k_halfCalib) / g_pm_tick_32k_calib);
    }

    unsigned int tick_wakeup_reset = wakeup_tick - earlyWakeup_us;
    //auto power down
    if((wakeup_src & PM_WAKEUP_TIMER)/*|| (wakeup_src & PM_WAKEUP_MDEC)*/|| (wakeup_src & PM_WAKEUP_COMPARATOR) ){
        analog_write_reg8(0x4c,0xee);
    }
    else{
        analog_write_reg8(0x4c, 0xef);
    }

    //set DCDC delay duration
#if 1 //2 * 1/16k = 125 uS
    if(sleep_mode == DEEPSLEEP_MODE){
        analog_write_reg8 (0x40, g_pm_r_delay_cycle.deep_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }else{
        analog_write_reg8 (0x40, g_pm_r_delay_cycle.suspend_ret_r_delay_cycle);//(n):  if timer wake up : (n*2) 32k cycle; else pad wake up: (n*2-1) ~ (n*2)32k cycle
    }
#else
    span = (PM_DCDC_DELAY_DURATION * (SYSTEM_TIMER_TICK_1US>>1) * g_track_32kcnt + tick_32k_halfCalib)/ g_pm_tick_32k_calib;
    unsigned char rst_cycle = 0xff - span;
    analog_write (0x1f, rst_cycle);
#endif
    unsigned int tick_reset;
#if PM_32k_RC_CALIBRATION_ALGORITHM_EN

    if(g_pm_long_suspend){
        tick_reset = pmbcd.ref_tick_32k + (unsigned int)(tick_wakeup_reset - ((pmbcd.ref_tick - sys_tick0)/ g_pm_tick_32k_calib * g_track_32kcnt));
    }
    else{
        tick_reset = pmbcd.ref_tick_32k + (unsigned int)(tick_wakeup_reset - (((pmbcd.ref_tick - sys_tick0) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib));
    }

#else
#if !SYS_TIMER_AUTO_MODE
    if(g_pm_long_suspend){
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - ((tick_adjust_us + (g_pm_tick_cur -sys_tick0))/ g_pm_tick_32k_calib * g_track_32kcnt));
    }
    else{
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - (((tick_adjust_us + (g_pm_tick_cur - sys_tick0)) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib));
    }
#else
    if(g_pm_long_suspend){
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset  - ((g_pm_tick_cur - sys_tick0)/ g_pm_tick_32k_calib * g_track_32kcnt));
    }
    else{
        tick_reset = g_pm_tick_32k_cur + (unsigned int)(tick_wakeup_reset - (((g_pm_tick_cur - sys_tick0) * g_track_32kcnt + tick_32k_halfCalib) / g_pm_tick_32k_calib));
    }
//  tick_reset = g_pm_tick_32k_cur + ((unsigned int)(wakeup_tick - g_pm_tick_cur) * g_track_32kcnt + tick_32k_halfCalib - tick_32k_halfCalib ) / g_pm_tick_32k_calib  - 1 - 3*2 - 2;
#endif
#endif

    clock_set_32k_tick(tick_reset);

    if(analog_read_reg8(0x64)&0x1f){

    }
    else{
        #if (WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
            if(sleep_mode & DEEPSLEEP_RETENTION_FLAG) { //deepsleep with retention
                analog_write_reg8(0x7f, 0x00);
            }
        #endif
        pm_sleep_start(sleep_mode);
        #if (WDT_REBOOT_RESET_ANA7F_WORK_AROUND)
            analog_write_reg8(0x7f, 0x01);
        #endif
    }
    if(sleep_mode == DEEPSLEEP_MODE){
       write_reg8 (0x1401ef, 0x20);  //reboot
    }
    reg_system_irq_mask |= BIT(0);
#if SYS_TIMER_AUTO_MODE
    REG_ADDR8(0x140218) = 0x02;//sys tick 16M set upon next 32k posedge
    reg_system_ctrl     |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN) ;

    //unsigned int now_tick_32k = clock_get_digital_32k_tick() + 1;
    unsigned int now_tick_32k = clock_get_32k_tick() + 1;
    if(g_pm_long_suspend){
        g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
    }
    else{
        g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;       // current clock
    }
#if PM_32k_RC_CALIBRATION_ALGORITHM_EN
    pmbcd.rc32_wakeup = now_tick_32k;
    pmbcd.rc32 = now_tick_32k - pmbcd.ref_tick_32k;
#endif
    reg_system_tick = g_pm_tick_cur + 1;
    //But when cclk is set to 96M, pclk is the quarter frequency of cclk to 24M, but cclk will switch to 24M RC before the program enters sleep,
    //pclk is still the quarter frequency of cclk, so pclk becomes 6M, reading 32k The tick value slows down by about four times;
    //writing a 16M tick value and reading a 32k rising edge are likely to occur at the same time, causing the 32k rising edge flag to fail to be read.
    //modify by junhui.hu, confirmed by jianzhi at 20210923.
    //wait cmd set dly done upon next 32k posedge
    //if not using status bit, wait at least 1 32k cycle to set register r_run_upon_next_32k back to 0, or before next normal set
    while((reg_system_st & BIT(7)) == 0);
    REG_ADDR8(0x140218) = 0;//normal sys tick (16/sys) update

#else
    //unsigned int now_tick_32k = clock_get_digital_32k_tick();
    unsigned int now_tick_32k = clock_get_32k_tick();
    {
        if(g_pm_long_suspend){
            g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) / g_track_32kcnt * g_pm_tick_32k_calib;
        }
        else{
            g_pm_tick_cur += (unsigned int)(now_tick_32k - g_pm_tick_32k_cur) * g_pm_tick_32k_calib / g_track_32kcnt;       // current clock
        }
    }

    reg_system_tick = g_pm_tick_cur + 20 * SYSTEM_TIMER_TICK_1US;
    reg_system_ctrl |= FLD_SYSTEM_TIMER_EN | FLD_SYSTEM_32K_TRACK_EN;    //enable 32k cal and stimer
#endif

    unsigned char anareg64 = analog_read_reg8(0x64);
    write_reg8(0x1401d8, div_reg);
    write_reg8(0x1401e8, cclk_reg );//restore cclk


    core_restore_interrupt(r);
    return (anareg64 ? (anareg64 | STATUS_ENTER_SUSPEND) : STATUS_GPIO_ERR_NO_ENTER_PM );


}
#endif
