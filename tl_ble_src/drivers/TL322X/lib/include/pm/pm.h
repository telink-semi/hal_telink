/********************************************************************************************************
 * @file    pm.h
 *
 * @brief   This is the header file for tl322x
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "lib/include/clock.h"

/********************************************************************************************************
 *                                          internal
 *******************************************************************************************************/

/********************************************************************************************************
 *              This is currently included in the H file for compatibility with other SDKs.
 *******************************************************************************************************/

//When the watchdog comes back, the Eagle chip does not clear 0x7f[0]. To avoid this problem, this macro definition is added.
#ifndef WDT_REBOOT_RESET_ANA7F_WORK_AROUND
    #define WDT_REBOOT_RESET_ANA7F_WORK_AROUND 1
#endif

/********************************************************************************************************
 *                                          external
 *******************************************************************************************************/


/**
 * @brief these analog register can store data in deep sleep mode or deep sleep with SRAM retention mode.
 *        Reset these analog registers by watchdog, software reboot (sys_reboot()), RESET Pin, power cycle, 32k watchdog, vbus detect.
 */
#define PM_ANA_REG_WD_CLR_BUF1 0x36 // initial value 0x00.
#define PM_ANA_REG_WD_CLR_BUF2 0x37 // initial value 0x00.
#define PM_ANA_REG_WD_CLR_BUF3 0x38 // initial value 0x00.
#define PM_ANA_REG_WD_CLR_BUF4 0x39 // initial value 0x00.

/**
 * @brief analog register below can store information when MCU in deep sleep mode or deep sleep with SRAM retention mode.
 *        Reset these analog registers by power cycle, 32k watchdog, RESET Pin,vbus detect.
 */
#define PM_ANA_REG_POWER_ON_CLR_BUF1 0x3b // initial value 0x00.
#define PM_ANA_REG_POWER_ON_CLR_BUF2 0x3c // initial value 0xff.

/**
 * @brief   gpio wakeup level definition
 */
typedef enum
{
    WAKEUP_LEVEL_LOW  = 0,
    WAKEUP_LEVEL_HIGH = 1,
} pm_gpio_wakeup_level_e;

/**
 * @brief   wakeup tick type definition
 */
typedef enum
{
    PM_TICK_STIMER = 0, // 24M
    PM_TICK_32K    = 1,
} pm_wakeup_tick_type_e;

/**
 * @brief   sleep mode.
 */
typedef enum
{
    //available mode for customer
    SUSPEND_MODE   = 0x00,
    DEEPSLEEP_MODE = 0x70,                 //when use deep mode pad wakeup(low or high level), if the high(low) level always in the pad,
                                           //system will not enter sleep and go to below of pm API, will reboot by core_6f = 0x20.
                                           //deep retention also had this issue, but not to reboot.
    DEEPSLEEP_MODE_RET_SRAM_LOW32K = 0x61, //for boot from sram
    DEEPSLEEP_MODE_RET_SRAM_LOW64K = 0x43, //for boot from sram
    DEEPSLEEP_MODE_RET_SRAM_LOW96K = 0x07, //for boot from sram
    //not available mode
    DEEPSLEEP_RETENTION_FLAG = 0x0F,
} pm_sleep_mode_e;

/**
 * @brief   available wake-up source for customer
 */
typedef enum
{
    PM_WAKEUP_PAD        = BIT(0),
    PM_WAKEUP_CORE       = BIT(1),
    PM_WAKEUP_TIMER      = BIT(2),
    PM_WAKEUP_COMPARATOR = BIT(3), /**<
                                            There are two things to note when using LPC wake up:
                                            1.After the LPC is configured, you need to wait 100 microseconds before go to sleep because the LPC need 1-2 32k tick to calculate the result.
                                              If you enter the sleep function at this time, you may not be able to sleep normally because the data in the result register is abnormal.

                                            2.When entering sleep, keep the input voltage and reference voltage difference must be greater than 30mV, otherwise can not enter sleep normally, crash occurs.
                                          */
    //   PM_WAKEUP_SHUTDOWN     = BIT(7),
} pm_sleep_wakeup_src_e;

/**
 * @brief   wakeup status
 */
typedef enum
{
    WAKEUP_STATUS_PAD        = FLD_WAKEUP_STATUS_PAD,
    WAKEUP_STATUS_CORE       = FLD_WAKEUP_STATUS_CORE,
    WAKEUP_STATUS_TIMER      = FLD_WAKEUP_STATUS_TIMER,
    WAKEUP_STATUS_COMPARATOR = FLD_WAKEUP_STATUS_COMPARATOR,
    WAKEUP_STATUS_ALL        = FLD_WAKEUP_STATUS_ALL,
    WAKEUP_STATUS_INUSE_ALL  = FLD_WAKEUP_STATUS_INUSE_ALL,

    STATUS_GPIO_ERR_NO_ENTER_PM = BIT(8), /**<Bit8 is used to determine whether the wake source is normal.*/
    STATUS_EXCEED_MAX           = BIT(27),
    STATUS_EXCEED_MIN           = BIT(28),
    STATUS_CLEAR_FAIL           = BIT(29),
    STATUS_ENTER_SUSPEND        = BIT(30),
} pm_suspend_wakeup_status_e;

/**
 * @brief   mcu status
 */
typedef enum
{
    MCU_POWER_ON                 = BIT(0), /**< power on, vbus detect or reset pin */
    //BIT(1) RSVD
    MCU_SW_REBOOT_BACK           = BIT(2), /**< Clear the watchdog status flag in time, otherwise, the system reboot may be wrongly judged as the watchdog.*/
    MCU_DEEPRET_BACK             = BIT(3),
    MCU_DEEP_BACK                = BIT(4),
    MCU_HW_REBOOT_TIMER_WATCHDOG = BIT(5), /**< If determine whether is 32k watchdog/timer watchdog,can also use the interface wd_32k_get_status()/wd_get_status() to determine. */
    MCU_HW_REBOOT_32K_WATCHDOG   = BIT(6), /**< - When the 32k watchdog/timer watchdog status is set to 1, if it is not cleared:
                                              - power cyele/vbus detect/reset pin come back, the status is lost;
                                              - but software reboot(sys_reboot())/deep/deepretation/32k watchdog come back,the status remains;
                                              */

    MCU_STATUS_POWER_ON          = MCU_POWER_ON,
    MCU_STATUS_REBOOT_BACK       = MCU_SW_REBOOT_BACK,
    MCU_STATUS_DEEPRET_BACK      = MCU_DEEPRET_BACK,
    MCU_STATUS_DEEP_BACK         = MCU_DEEP_BACK,
} pm_mcu_status;

/**
 * @brief power sel
 * 
 */
typedef enum
{
    PM_POWER_UP   = 0,
    PM_POWER_DOWN = 1,
} pm_power_sel_e;

/**
 * @brief dcdc trim soc out
 * 
 */
typedef enum
{
    TRIM_1P25V_TO_1P050V = 0x0,
    TRIM_1P25V_TO_1P065V,
    TRIM_1P25V_TO_1P080V,
    TRIM_1P25V_TO_1P095V,
    TRIM_1P25V_TO_1P110V,
    TRIM_1P25V_TO_1P125V,
    TRIM_1P25V_TO_1P140V,
    TRIM_1P25V_TO_1P155V,
    TRIM_1P25V_TO_1P170V,
    TRIM_1P25V_TO_1P19V,
    TRIM_1P25V_TO_1P21V,
    TRIM_1P25V_TO_1P23V,
    TRIM_1P25V_TO_1P25V,
    TRIM_1P25V_TO_1P27V,
    TRIM_1P25V_TO_1P29V,
    TRIM_1P25V_TO_1P31V,
} pm_trim_1p25v_e;

/**
 * @brief trim dig ldo
 * 
 */
typedef enum
{
    DIG_LDO_TRIM_0P735V = 0,
    DIG_LDO_TRIM_0P760V,
    DIG_LDO_TRIM_0P785V,
    DIG_LDO_TRIM_0P810V,
    DIG_LDO_TRIM_0P835V,
    DIG_LDO_TRIM_0P860V,
    DIG_LDO_TRIM_0P885V,
    DIG_LDO_TRIM_0P910V,
    DIG_LDO_TRIM_0P935V,
    DIG_LDO_TRIM_0P960V,
    DIG_LDO_TRIM_0P985V,
    DIG_LDO_TRIM_1P010V,
    DIG_LDO_TRIM_1P035V,
    DIG_LDO_TRIM_1P060V,
    DIG_LDO_TRIM_1P085V,
    DIG_LDO_TRIM_1P100V,
} pm_dig_ldo_trim_e;

/**
 * @brief trim suspend LDO
 *
 */
typedef enum
{
    SPD_LDO_TRIM_0P75V = 0,
    SPD_LDO_TRIM_0P80V,
    SPD_LDO_TRIM_0P85V,
    SPD_LDO_TRIM_0P90V,
    SPD_LDO_TRIM_0P95V,
    SPD_LDO_TRIM_1P00V,
    SPD_LDO_TRIM_1P05V,
    SPD_LDO_TRIM_1P10V,
} pm_spd_ldo_trim_e;

/**
 * @brief trim deep retention LDO
 *
 */
typedef enum
{
    RET_LDO_TRIM_0P55V = 0,
    RET_LDO_TRIM_0P60V,
    RET_LDO_TRIM_0P65V,
    RET_LDO_TRIM_0P70V,
    RET_LDO_TRIM_0P75V,
    RET_LDO_TRIM_0P80V,
    RET_LDO_TRIM_0P85V,
    RET_LDO_TRIM_0P90V,
} pm_ret_ldo_trim_e;

typedef enum
{
    DIG_VOL_1V_MODE =0,
    DIG_VOL_1V1_MODE ,
} pm_dig_vol_mode_e;

/**
 * @brief voltage select
 *
 */
typedef enum
{
    VDD_BASEBAND = 0x02,
    VDD_EFUSE    = 0x03,
    VDD_CORE     = 0x04,
    VDD_RETRAM   = 0x05,
} pm_vol_mux_sel_e;

/**
 * @brief   early wakeup time
 */
typedef struct
{
    unsigned short suspend_early_wakeup_time_us;  /**< suspend_early_wakeup_time_us = deep_ret_r_delay_us + xtal_stable_time + early_time*/
    unsigned short deep_ret_early_wakeup_time_us; /**< deep_ret_early_wakeup_time_us = deep_ret_r_delay_us + early_time*/
    unsigned short deep_early_wakeup_time_us;     /**< deep_early_wakeup_time_us = suspend_ret_r_delay_us*/
    unsigned short sleep_min_time_us;             /**< sleep_min_time_us = suspend_early_wakeup_time_us + 200*/
} pm_early_wakeup_time_us_s;

extern volatile pm_early_wakeup_time_us_s g_pm_early_wakeup_time_us;

/**
 * @brief   hardware delay time
 */
typedef struct
{
    unsigned short deep_r_delay_cycle;           /**< hardware delay time ,deep_ret_r_delay_us = deep_r_delay_cycle * 1/16k */
    unsigned short suspend_ret_r_delay_cycle;    /**< hardware delay time ,suspend_ret_r_delay_us = suspend_ret_r_delay_cycle * 1/16k */
    unsigned short deep_xtal_delay_cycle;        /**< hardware delay time ,deep_ret_xtal_delay_us = deep_xtal_delay_cycle * 1/16k */
    unsigned short suspend_ret_xtal_delay_cycle; /**< hardware delay time ,suspend_ret_xtal_delay_us = suspend_ret_xtal_delay_cycle * 1/16k */
} pm_r_delay_cycle_s;

extern volatile pm_r_delay_cycle_s g_pm_r_delay_cycle;

/**
 * @brief   deep sleep wakeup status
 */
typedef struct
{
    unsigned char is_pad_wakeup;
    unsigned char wakeup_src; //The pad pin occasionally wakes up abnormally in A0. The core wakeup flag will be incorrectly set in A0.
    unsigned char mcu_status;
    unsigned char rsvd;
} pm_status_info_s;

extern _attribute_aligned_(4) pm_status_info_s g_pm_status_info;
extern _attribute_data_retention_sec_ unsigned char g_pm_vbat_v;
extern unsigned char                                g_areg_aon_7f;

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
 * @note        After the wake source is obtained, &WAKEUP_STATUS_INUSE_ALL is needed to determine
 *              whether the wake source in use has been cleared, because some of the wake sources
 *              that are not in use may have been set up.
 */
static _always_inline pm_wakeup_status_e pm_get_wakeup_src(void)
{
    return ((pm_wakeup_status_e)analog_read_reg8(areg_aon_0x64));
}

/**
 * @brief       This function serves to clear the wakeup bit.
 * @param[in]   status  - the interrupt status that needs to be cleared.
 * @return      none.
 * @note        To clear all wake sources, the parameter of this interface is usually FLD_WAKEUP_STATUS_ALL
 *              instead of FLD_WAKEUP_STATUS_INUSE_ALL.
 */
static _always_inline void pm_clr_irq_status(pm_wakeup_status_e status)
{
    analog_write_reg8(areg_aon_0x64, status);
}

/**
 * @brief       This function serves to set the wakeup source.
 * @param[in]   wakeup_src  - wake up source select.
 * @return      none.
 */
static _always_inline void pm_set_wakeup_src(pm_sleep_wakeup_src_e wakeup_src)
{
    analog_write_reg8(areg_aon_0x4b, wakeup_src);
}

/**
 * @brief       This function serves to reboot system.
 * @return      none
 */
_always_inline void sys_reset_all(void)
{
    reg_pwdn_en = 0x20;
}

/**
 * @brief       This function configures a GPIO pin as the wakeup pin.
 * @param[in]   pin - the pins can be set to all GPIO except PB0/PC5 and GPIOG groups.
 * @param[in]   pol - the wakeup polarity of the pad pin(0: low-level wakeup, 1: high-level wakeup).
 * @param[in]   en  - enable or disable the wakeup function for the pan pin(1: enable, 0: disable).
 * @return      none.
 */
void pm_set_gpio_wakeup(gpio_pin_e pin, pm_gpio_wakeup_level_e pol, int en);

/**
 * @brief       This function serves to recover system timer.
 *              The code is placed in the ram code section, in order to shorten the time.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_stimer_recover(void);

/**
 * @brief       This function configures pm wakeup time parameter.
 * @param[in]   param - deep/suspend/deep_retention r_delay time.(default value: suspend/deep_ret=3, deep=11)
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
 * @brief       this function serves to clear all irq status.
 * @return      Indicates whether clearing irq status was successful.
 */
_attribute_ram_code_sec_noinline_ unsigned char pm_clr_all_irq_status(void);

/**
 * @brief       This function serves to set baseband/usb/npe power on/off before suspend sleep,If power
 *              on this module,the suspend current will increase;power down this module will save current,
 *              but you need to re-init this module after suspend wakeup.All module is power down default
 *              to save current.
 * @param[in]   value - whether to power on/off the baseband/usb/npe.
 * @param[in]   on_off - select power on or off, 0 - power off; other value - power on.
 * @return      none.
 */
void pm_set_suspend_power_cfg(pm_pd_module_e value, unsigned char on_off);

/**
 * @brief       This function serves to set the working mode of MCU based on 32k crystal,e.g. suspend mode, deep sleep mode, deep sleep with SRAM retention mode and shutdown mode.
 * @param[in]   sleep_mode          - sleep mode type select.
 * @param[in]   wakeup_src          - wake up source select.
 * @param[in]   wakeup_tick_type    - tick type select. Use 32K tick count for long-time sleep or 24M tick count for short-time sleep.
 * @param[in]   wakeup_tick         - The tick value at the time of wake-up.
                                      If the wakeup_tick_type is PM_TICK_STIMER, then wakeup_tick is converted to 24M. The range of tick that can be set is approximately:
                                      current tick value + (18352~0xe0000000), and the corresponding sleep time is approximately: 2ms~234.88s.It cannot go to sleep normally when it exceeds this range.
                                      If the wakeup_tick_type is PM_TICK_32K, then wakeup_tick is converted to 32K. The range of tick that can be set is approximately:
                                      64~0xffffffff, and the corresponding sleep time is approximately: 2ms~37hours.It cannot go to sleep normally when it exceeds this range.
 * @return      indicate whether the cpu is wake up successful.
 */
_attribute_text_sec_ int pm_sleep_wakeup(pm_sleep_mode_e sleep_mode, pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int wakeup_tick);


/**
 * @brief       This function is used to determine the stability of the crystal oscillator.
 *              To judge the stability of the crystal oscillator, xo_ready_ana is invalid, and use an alternative solution to judge.
 *              Alternative principle: Because the clock source of the stimer is the crystal oscillator,
 *              if the crystal oscillator does not vibrate, the tick value of the stimer does not increase or increases very slowly (when there is interference).
 *              So first use 24M RC to run the program and wait for a fixed time, calculate the number of ticks that the stimer should increase during this time,
 *              and then read the tick value actually increased by the stimer.
 *              When it reaches 50% of the calculated value, it proves that the crystal oscillator has started.
 *              If it is not reached for a long time, the system will reboot.
 * @param[in]   all_ramcode_en  - Whether all processing in this function is required to be ram code. If this parameter is set to 1, it requires that:
 *              before calling this function, you have done the disable BTB, disable interrupt, mspi_stop_xip and other operations as the corresponding function configured to 0.
 * @attention   This function can only be called with the 24M clock configuration
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_wait_xtal_ready(unsigned char all_ramcode_en);


/**
 * @brief       This function serves to switch digital module power.    
 * @param[in]   module - digital module.
 * @param[in]   power_sel - power up or power down.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_set_dig_module_power_switch(pm_pd_module_e module, pm_power_sel_e power_sel);

/********************************************************************************************************
 *                                          internal
 *******************************************************************************************************/

/********************************************************************************************************
 *              This is just for internal debug purpose, users are prohibited from calling.
 *******************************************************************************************************/

/**
 * @brief       The function of this interface is to configure the voltage of the sleep-related LDO.
 * @return      none.
 */
void pm_set_sleep_ldo_voltage(void);

/**
 * @brief       This function serves to trim dig ldo voltage
 * @param[in]   dig_ldo_trim - dig ldo trim voltage
 * @return      none
 */
void pm_set_dig_ldo_voltage(pm_dig_ldo_trim_e dig_ldo_trim);

/**
 * @brief       This function serves to trim deep retention LDO voltage
 * @param[in]   ret_ldo_trim - deep retention LDO trim voltage
 * @return      none
 */
void pm_set_ret_ldo_voltage(pm_ret_ldo_trim_e ret_ldo_trim);

/**
 * @brief       This function serves to trim ldo 1.25v out.     
 * @param[in]   trim_1p25vldo - ldo trim 1.25v out
 */
void pm_set_1p25v(pm_trim_1p25v_e trim_1p25vldo);

/**
 * @brief       This function serves to test different voltages from pd3.
 * @param[in]   mux_sel - select different voltages from pd3.
 * @return      none.
 */
void pm_set_probe_vol_to_pc3(pm_vol_mux_sel_e mux_sel);

/**
 * @brief       This function serves to update wakeup status.
 * @param[in]   clr_en  - Whether to set the value of the status register to a fixed value.
 *                        If the interface is called twice, the first time it is not modified, clr_en=0;
 *                        if the interface is called once, it is modified, clr_en=1.
 * @return      none.
 * @note        After calling this interface, it is necessary to clear the flag of the timer watchdog or the 32k watchdog.
 *              Otherwise, if the flag remains set, it may affect the next judgment.
 *              After calling this interface, other states are set to fixed values.
 *              Therefore, this interface cannot be called twice,
 *              and if it is called twice, the state will be fixed to one state, not the correct state.
 */
_attribute_ram_code_sec_noinline_ void pm_update_status_info(unsigned char clr_en);

/**
 * @brief       This function serves to set system power mode.
 * @param[in]   power_mode  - power mode(LDO/DCDC/LDO_DCDC).
 * @return      none.
 * @note        pd_dcdc_ldo_sw<1:0>, dcdc & bypass ldo status bits:
                    dcdc_1p25   dcdc_1p8     ldo_1p25    ldo_1p8
                00:     N           N           Y           Y
                01:     Y           N           N           Y
                10:     Y           N           N           N
                11:     Y           Y           N           N
 */
_attribute_ram_code_sec_noinline_ void pm_set_power_mode(power_mode_e power_mode);

/**
 * @brief       This function serves to set dig ldo mode.
 * @param[in]   vol         - dig ldo mode.
 * @param[in]   dma_timeout_us - wait dma all chn complete timeout.
 * @return      DRV_API_SUCCESS - successful;
 *              DRV_API_INVALID_PARAM - select the not support mode;
 *              DRV_API_TIMEOUT - wait for dma all chn idle timeout to exit;
 * @note        1.If the voltage goes up, after calling the interface first, then adjust the frequency;
 *                If the voltage goes down, adjust the frequency first, then call the interface;
 *              2.When adjusting this voltage, no access ram operation is allowed, so it will wait for dma idle in this interface,
 *                modifying dma_timeout_us won't work if there are dma chains working all the time, and needs to be turned off by the upper layers themselves depending on the situation.
 *              3.When adjusting this voltage, no access ram operation is allowed, disable swire.
 *              4.If the check configuration fails, reboot.
 */
_attribute_ram_code_sec_noinline_ drv_api_status_e pm_set_dig_ldo(pm_dig_vol_mode_e vol, unsigned int dma_timeout_us);
