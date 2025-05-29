/********************************************************************************************************
 * @file    pm.h
 *
 * @brief   This is the header file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once

#include "reg_include/register.h"
#include "compiler.h"
#include "gpio.h"
#include "clock.h"
#include "dma.h"
/*
 * DVDD1 and AVDD2 share a common analog register, however, when adjusting the DVDD, the dma needs to move the data from the flash to configure,
 * so the configuration needs to be given the initial value in advance, and can not be modified later, however, the AVDD1 may be modified, there is a conflict!
 * The current solution is as follows, through the macro to control whether to adjust the DVDD, to avoid the use of flash erase and then write the situation:
 * 1. If enabled, AVDD2 is also configured using the macro, and when AVDD2 needs to be calibrated, the static library needs to be recompiled.
 * 2. If not enabled, it can be set via the interface.
 * The situation is more complicated if the calibration is specific to each chip.
 */
#define  PM_IS_TUNE_RAM_VOL             1
#define  AVDD2_VOL_CONFG                PM_AVDD2_VOLTAGE_2V500
#define  DVDD1_DVDD2_VOL_0P8_CONFG      PM_DVDD1_DVDD2_0V800
#define  DVDD1_DVDD2_VOL_0P9_CONFG      PM_DVDD1_DVDD2_0V900






#define  DVDD1_DVDD2_DEFAULT_VOL  PM_DVDD1_DVDD2_0V800
#define  ALG0X21_DEFAULT_CONFIG      0xca
#define  ALG0X22_DEFAULT_CONFIG      0xbc


/**
 * @brief these analog register can store data in deep sleep mode or deep sleep with SRAM retention mode.
 *        Reset these analog registers by watchdog, software reboot (sys_reboot()), RESET Pin, power cycle, 32k watchdog, vbus detect.
 */
/**
 * Customers cannot use [bit0] of analog register 0x35 because driver and chip functions are occupied, details are as follows:
 * [Bit0]: If this bit is 1, it means that reboot has occurred.
 */
#define PM_ANA_REG_WD_CLR_BUF0          0x35 // initial value 0xff.
#define PM_ANA_REG_WD_CLR_BUF1          0x36 // initial value 0x00.
#define PM_ANA_REG_WD_CLR_BUF2          0x37 // initial value 0x00.
#define PM_ANA_REG_WD_CLR_BUF3          0x38 // initial value 0x00.
/**
 * Customers cannot use [bit0] of analog register 0x35 because driver and chip functions are occupied, details are as follows:
 * [Bit0]: If this bit is 1, it means that enable the LDO of icyTRX-DM digital part (vddd power domain).
 */
#define PM_ANA_REG_WD_CLR_BUF4          0x39 // initial value 0x00.

/**
 * @brief analog register below can store information when MCU in deep sleep mode or deep sleep with SRAM retention mode.
 *        Reset these analog registers by power cycle, 32k watchdog, RESET Pin,vbus detect.
 */
/**
 * Customers cannot use [bit0],[bit1],[bit2],[bit7] of analog register 0x3a because driver and chip functions are occupied, details are as follows:
 * [Bit0]: If this bit is 1, it means that reboot has occurred.
 * [Bit1]: If this bit is 1, it means that the software calls the function sys_reboot() when the crystal oscillator does not start up normally.
 * [Bit2]: If this bit is 1, it means that the pm_sleep_wakeup function failed to clear the pm wake flag bit when using the deep wake source, and the software called sys_reboot().
 */
#define PM_ANA_REG_POWER_ON_CLR_BUF0    0x3a // initial value 0x00.
#define PM_ANA_REG_POWER_ON_CLR_BUF1    0x3b // initial value 0x00.
#define PM_ANA_REG_POWER_ON_CLR_BUF2    0x3c // initial value 0xff.

/**
 * @brief   gpio wakeup level definition
 */
typedef enum{
    WAKEUP_LEVEL_LOW        = 0,
    WAKEUP_LEVEL_HIGH       = 1,
}pm_gpio_wakeup_level_e;

/**
 * @brief   wakeup tick type definition
 */
typedef enum {
     PM_TICK_STIMER         = 0,    // 24M
     PM_TICK_32K            = 1,
}pm_wakeup_tick_type_e;

/**
 * @brief   sleep mode.
 */
typedef enum {
    //available mode for customer
    SUSPEND_MODE                        = 0x00, //The A0 version of the suspend execution process is abnormal and the program restarts.
    DEEPSLEEP_MODE                      = 0x80, //when use deep mode pad wakeup(low or high level), if the high(low) level always in
                                                //the pad, system will not enter sleep and go to below of pm API, will reboot by core_6f = 0x20
                                                //deep retention also had this issue, but not to reboot.
    DEEPSLEEP_MODE_RET_SRAM_LOW128K     = 0x10, //for boot from sram
    DEEPSLEEP_MODE_RET_SRAM_LOW256K     = 0x30, //for boot from sram
    DEEPSLEEP_MODE_RET_SRAM_LOW384K     = 0x70, //for boot from sram
    //not available mode
    DEEPSLEEP_RETENTION_FLAG            = 0x7F,
}pm_sleep_mode_e;

/**
 * @brief   available wake-up source for customer
 */
typedef enum {
    PM_WAKEUP_PAD           = BIT(0),
//  PM_WAKEUP_CORE          = BIT(1),
    PM_WAKEUP_TIMER         = BIT(2),
    PM_WAKEUP_COMPARATOR    = BIT(3),   /**<
                                            There are two things to note when using LPC wake up:
                                            1.After the LPC is configured, you need to wait 100 microseconds before go to sleep because the LPC need 1-2 32k tick to calculate the result.
                                              If you enter the sleep function at this time, you may not be able to sleep normally because the data in the result register is abnormal.

                                            2.When entering sleep, keep the input voltage and reference voltage difference must be greater than 30mV, otherwise can not enter sleep normally, crash occurs.
                                          */
//  PM_WAKEUP_CTB           = BIT(5),
//  PM_WAKEUP_SHUTDOWN      = BIT(7),
}pm_sleep_wakeup_src_e;

/**
 * @brief   wakeup status
 */
typedef enum {
    WAKEUP_STATUS_PAD               = FLD_WAKEUP_STATUS_PAD,
//  WAKEUP_STATUS_CORE              = FLD_WAKEUP_STATUS_CORE,
    WAKEUP_STATUS_TIMER             = FLD_WAKEUP_STATUS_TIMER,
    WAKEUP_STATUS_COMPARATOR        = FLD_WAKEUP_STATUS_COMPARATOR,
//  WAKEUP_STATUS_CTB               = FLD_WAKEUP_STATUS_CTB,
    WAKEUP_STATUS_ALL               = FLD_WAKEUP_STATUS_ALL,

    STATUS_GPIO_ERR_NO_ENTER_PM     = BIT(8), /**<Bit8 is used to determine whether the wake source is normal.*/
    STATUS_CLEAR_FAIL               = BIT(29),
    STATUS_ENTER_SUSPEND            = BIT(30),
}pm_suspend_wakeup_status_e;

/**
 * @brief   mcu status
 */
typedef enum{
    MCU_STATUS_POWER_ON         = BIT(0), /**<  power on, vbus detect or reset pin */
    //BIT(1) RSVD
    MCU_STATUS_REBOOT_BACK      = BIT(2), /**<  the reboot specific categories,see pm_reboot_event_e:
                                                1.If want to know which reboot it is, call the pm_get_mcu_reboot_status() interface to determine after calling sys_init().
                                                2.If determine whether is 32k watchdog/timer watchdog,can also use the interface wd_32k_get_status()/wd_get_status() to determine.
                                                */
    MCU_STATUS_DEEPRET_BACK     = BIT(3),
    MCU_STATUS_DEEP_BACK        = BIT(4),
}pm_mcu_status;

/**
 * @brief  reboot status
 */
typedef enum{
    SW_SYSTEM_REBOOT            = BIT(0),/**< Clear the watchdog status flag in time, otherwise, the system reboot may be wrongly judged as the watchdog.*/
    HW_TIMER_WATCHDOG_REBOOT    = BIT(1),
    HW_32K_WATCHDOG_REBOOT      = BIT(2),/**< - When the 32k watchdog/timer watchdog status is set to 1, if it is not cleared:
                                              - power cyele/vbus detect/reset pin come back, the status is lost;
                                              - but software reboot(sys_reboot())/deep/deepretation/32k watchdog come back,the status remains;
                                              */
}pm_reboot_event_e;

/**
 * @brief power sel
 * 
 */
typedef enum{
    PM_POWER_UP         = 0,
    PM_POWER_DOWN       = 1,
}pm_power_sel_e;


/**
 * @brief   early wakeup time
 */
typedef struct {
    unsigned short  suspend_early_wakeup_time_us;   /**< suspend_early_wakeup_time_us = deep_ret_r_delay_us + xtal_stable_time + early_time*/
    unsigned short  deep_ret_early_wakeup_time_us;  /**< deep_ret_early_wakeup_time_us = deep_ret_r_delay_us + early_time*/
    unsigned short  deep_early_wakeup_time_us;      /**< deep_early_wakeup_time_us = suspend_ret_r_delay_us*/
    unsigned short  sleep_min_time_us;              /**< sleep_min_time_us = suspend_early_wakeup_time_us + 200*/
}pm_early_wakeup_time_us_s;

//extern volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us;
/**
 * @brief   hardware delay time
 */
typedef struct {
    unsigned short  deep_r_delay_cycle ;            /**< hardware delay time ,deep_ret_r_delay_us = deep_r_delay_cycle * 1/16k */
    unsigned short  suspend_ret_r_delay_cycle ;     /**< hardware delay time ,suspend_ret_r_delay_us = suspend_ret_r_delay_cycle * 1/16k */
    unsigned short  deep_xtal_delay_cycle ;         /**< hardware delay time ,deep_ret_xtal_delay_us = deep_xtal_delay_cycle * 1/16k */
    unsigned short  suspend_ret_xtal_delay_cycle ;  /**< hardware delay time ,suspend_ret_xtal_delay_us = suspend_ret_xtal_delay_cycle * 1/16k */
}pm_r_delay_cycle_s;

extern volatile pm_r_delay_cycle_s g_pm_r_delay_cycle;
/**
 * @brief   deep sleep wakeup status
 */
typedef struct{
    unsigned char is_pad_wakeup;
    unsigned char wakeup_src;   //The pad pin occasionally wakes up abnormally in A0. The core wakeup flag will be incorrectly set in A0.
    unsigned char mcu_status;
    unsigned char rsvd;
}pm_status_info_s;

extern _attribute_aligned_(4) pm_status_info_s g_pm_status_info;
extern _attribute_data_retention_sec_ unsigned char g_pm_vbat_v;

/**
 * @brief   active mode DVDD1/DVDD2 output trim definition
 * @note    The voltage values of the following gears are all theoretical values, and there may be deviations between the actual and theoretical values.
 */
typedef enum {
    PM_DVDD1_DVDD2_0V800        = 0x03<<8|0x02,
    PM_DVDD1_DVDD2_0V825        = 0x04<<8|0x03,
    PM_DVDD1_DVDD2_0V850        = 0x05<<8|0x04,
    PM_DVDD1_DVDD2_0V875        = 0x06<<8|0x05,
    PM_DVDD1_DVDD2_0V800_0V825  = 0x03<<8|0x03,
    PM_DVDD1_DVDD2_0V800_0V850  = 0x03<<8|0x04,
    PM_DVDD1_DVDD2_0V800_0V875  = 0x03<<8|0x05,
    PM_DVDD1_DVDD2_0V825_0V850  = 0x04<<8|0x04,
    PM_DVDD1_DVDD2_0V825_0V875  = 0x04<<8|0x05,
    PM_DVDD1_DVDD2_0V850_0V875  = 0x05<<8|0x05,
    PM_DVDD1_DVDD2_0V900        = 0x07<<8|0x06,
    PM_DVDD1_DVDD2_0V900_0V925  = 0x07<<8|0x07,
}pm_dvdd_voltage_e;

/**
 * @brief       This function serves to get deep retention flag.
 * @return      1 deep retention, 0 deep.
 */
static inline unsigned char pm_get_deep_retention_flag(void)
{
    return !(analog_read_reg8(0x7f) & BIT(0));
}

/**
 * @brief       This function serves to get wakeup source.
 * @return      wakeup source.
 */
static _always_inline pm_wakeup_status_e pm_get_wakeup_src(void)
{
    return ((pm_wakeup_status_e)analog_read_reg8(0x64) & FLD_WAKEUP_STATUS_ALL);
}

/**
 * @brief       This function serves to clear the wakeup bit.
 * @param[in]   status  - the interrupt status that needs to be cleared.
 * @return      none.
 */
static inline void pm_clr_irq_status(pm_wakeup_status_e status)
{
    analog_write_reg8(0x64, status);
}

/**
 * @brief       This function serves to set the wakeup source.
 * @param[in]   wakeup_src  - wake up source select.
 * @return      none.
 */
static inline void pm_set_wakeup_src(pm_sleep_wakeup_src_e wakeup_src)
{
    analog_write_reg8(0x4b, wakeup_src);
}

/**
 * @brief       This function configures a GPIO pin as the wakeup pin.
 * @param[in]   pin - the pin needs to be configured as wakeup pin.
 * @param[in]   pol - the wakeup polarity of the pad pin(0: low-level wakeup, 1: high-level wakeup).
 * @param[in]   en  - enable or disable the wakeup function for the pan pin(1: enable, 0: disable).
 * @return      none.
 */
void pm_set_gpio_wakeup (gpio_pin_e pin, pm_gpio_wakeup_level_e pol, int en);

/**
 * @brief       This function servers to wait bbpll to audio clock lock.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_wait_audio_pll_done(void);

/**
 * @brief       This function servers to powen on bbpll to audio clock lock.
 * @return      none.
 */
void pm_audio_pll_power_on(void);

/**
 * @brief       This function servers to powen down bbpll to audio clock lock.
 * @return      none.
 */
void pm_audio_pll_power_down(void);
#if 0
/**
 * @brief       This function serves to recover system timer.
 *              The code is placed in the ram code section, in order to shorten the time.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_stimer_recover(void);

/**
 * @brief       This function configures pm wakeup time parameter.
 * @param[in]   param - pm wakeup time parameter.
 * @return      none.
 */
void pm_set_wakeup_time_param(pm_r_delay_cycle_s param);

/**
 * @brief       This function is used in applications where the crystal oscillator is relatively slow to start.
 *              When the start-up time is very slow, you can call this function to avoid restarting caused
 *              by insufficient crystal oscillator time (it is recommended to leave a certain margin when setting).
 * @param[in]   delay_us - The time wait for xtal stable and flash restore to the active working state in the ramcode
 *                          when wakeup from suspend sleep (default value: 200).
 * @param[in]   loopnum - The time for the crystal oscillator to stabilize is approximately: loopnum*40us(default value: loopnum=10).
 * @return      none.
 * @note        Those parameters will be lost after reboot or deep sleep, so it required to be reconfigured.
 */
void pm_set_xtal_stable_timer_param(unsigned int delay_us, unsigned int loopnum);

/**
 * @brief       This function serves to set baseband/usb/npe power on/off before suspend sleep,If power
 *              on this module,the suspend current will increase;power down this module will save current,
 *              but you need to re-init this module after suspend wakeup.All module is power down default
 *              to save current.
 * @param[in]   value - weather to power on/off the baseband/usb/npe.
 * @param[in]   on_off - select power on or off, 0 - power off; other value - power on.
 * @return      none.
 */
void pm_set_suspend_power_cfg(pm_pd_module_e value, unsigned char on_off);

/**
 * @brief       This function serves to set the working mode of MCU based on 32k crystal,e.g. suspend mode, deepsleep mode, deepsleep with SRAM retention mode and shutdown mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @param[in]   wakeup_src          - wake up source select.
 *      A0      note: The reference current values under different configurations are as followsUnit (uA):
 *                  |   pad     |   32k rc  |   32k xtal    |   mdec    |   lpc     |
 *  deep            |   0.7     |   1.3     |   1.7         |   1.4     |   1.6     |
 *  deep ret 32k    |   1.8     |   2.4     |   2.8         |   2.6     |   2.8     |
 *  deep ret 64k    |   2.7     |   3.2     |   3.7         |   3.4     |   3.7     |
 *              A0 chip, the retention current will float up.
 * @param[in]   wakeup_tick_type    - tick type select. For long timer sleep.currently only 24M is supported(PM_TICK_STIMER).
 * @param[in]   wakeup_tick         - the time of short sleep, which means MCU can sleep for less than 5 minutes.
 * @return      indicate whether the cpu is wake up successful.
 * @attention   Must ensure that all GPIOs cannot be floating status before going to sleep to prevent power leakage.
 */
_attribute_text_sec_ int pm_sleep_wakeup(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick);
#endif
/**
 * @brief       Calculate the offset value based on the difference of 16M tick.
 * @param[in]   offset_tick - the 16M tick difference between the standard clock and the expected clock.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_cal_32k_rc_offset (int offset_tick);

/**
 * @brief       When 32k rc sleeps, the calibration function is initialized.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_32k_rc_offset_init(void);

/**
 * @brief       This function serves to switch digital module power.
 * @param[in]   module - digital module.
 * @param[in]   power_sel - power up or power down.
 * @return      none.
 */
void pm_set_dig_module_power_switch(pm_pd_module_e module, pm_power_sel_e power_sel);

/**
 * @brief       This function serves to set dvdd
 * @param[in]   vol      - DVDD1_DVDD2_VOL_0P8_CONFG/DVDD1_DVDD2_VOL_0P9_CONFG.
 *                       - the 0.8v/0.9v confirms which of the pm_dvdd1_dvdd2_voltage_e enumeration is configured, and then assigns the value to the macro DVDD1_DVDD2_VOL_0P8_CONFG/DVDD1_DVDD2_VOL_0P9_CONFG.
 * @param[in]   chn      - dma channel.
 * @param[in]   core     - sys_core_e,which cores are used in the application choose the corresponding enumeration (or just).
 * @param[in]   dma_timeout_us - wait dma all chn complete timeout.
 * @return      DRV_API_SUCCESS - successful;
 *              DRV_API_INVALID_PARAM - equal to the current voltage configuration or dvdd1_dvdd2_vol error;
 *              DRV_API_FAILURE - core error(need contains all the cores used);
 *              DRV_API_TIMEOUT - wait for dma all chn idle timeout to exit;
 *              DRV_API_OTHER_ERROR - clear all interrupt requests failed;
 * @note        1.If the voltage goes up, after calling the interface first, then adjust the frequency;
 *                If the voltage goes down,adjust the frequency first,then  calling the interface;
 *              2.When adjusting this voltage, no access ram operation is allowed, so it will wait for dma idle in this interface,
 *                modifying dma_timeout_us won't work if there are dma chains working all the time, and needs to be turned off by the upper layers themselves depending on the situation.
 *              3.When adjusting this voltage, the mcu will be stalled because the ram cannot be operated, use the dma method to modify the dvdd configuration and wake up the d25f with this dma interrupt,
 *                so will turns off the general interrupt and clears all interrupt requests.
 *              4.When adjusting this voltage, no access ram operation is allowed,disable swire.
 *              5.if the check configuration fails, reboot.
 */
drv_api_status_e pm_set_dvdd(pm_dvdd_voltage_e vol,dma_chn_e chn,sys_core_e core,unsigned int dma_timeout_us);
