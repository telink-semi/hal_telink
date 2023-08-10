/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

/********************************************************************************************************
 * @file	pm.h
 *
 * @brief	This is the header file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#pragma once

#include "reg_include/register.h"
#include "compiler.h"
#include "gpio.h"
#include "clock.h"

/********************************************************************************************************
 *											internal
 *******************************************************************************************************/

/********************************************************************************************************
 * 				This is currently included in the H file for compatibility with other SDKs.
 *******************************************************************************************************/

//When the watchdog comes back, the Eagle chip does not clear 0x7f[0]. To avoid this problem, this macro definition is added.
#ifndef WDT_REBOOT_RESET_ANA7F_WORK_AROUND
#define WDT_REBOOT_RESET_ANA7F_WORK_AROUND	1
#endif

/********************************************************************************************************
 *											external
 *******************************************************************************************************/


/**
 * @brief these analog register can store data in deepsleep mode or deepsleep with SRAM retention mode.
 * 	      Reset these analog registers by watchdog, chip reset, RESET Pin, power cycle, 32k watchdog.
 */
#define PM_ANA_REG_WD_CLR_BUF0			0x35 // initial value 0xff. [Bit0] is already occupied. The customer cannot change!
#define PM_ANA_REG_WD_CLR_BUF1			0x36 // initial value 0x00.
#define PM_ANA_REG_WD_CLR_BUF2			0x37 // initial value 0x00
#define PM_ANA_REG_WD_CLR_BUF3			0x38 // initial value 0x00
#define PM_ANA_REG_WD_CLR_BUF4			0x39 // initial value 0x00

/**
 * @brief analog register below can store information when MCU in deepsleep mode or deepsleep with SRAM retention mode.
 * 	      Reset these analog registers by power cycle, 32k watchdog.
 */
#define PM_ANA_REG_POWER_ON_CLR_BUF0	0x3a // initial value 0x00 [Bit0] is already occupied. The customer cannot change!
#define PM_ANA_REG_POWER_ON_CLR_BUF1	0x3b // initial value 0x00
#define PM_ANA_REG_POWER_ON_CLR_BUF2	0x3c // initial value 0xff

/**
 * @brief	gpio wakeup level definition
 */
typedef enum{
	WAKEUP_LEVEL_LOW		= 0,
	WAKEUP_LEVEL_HIGH		= 1,
}pm_gpio_wakeup_level_e;

/**
 * @brief	wakeup tick type definition
 */
typedef enum {
	 PM_TICK_STIMER			= 0,	// 24M
	 PM_TICK_32K			= 1,
}pm_wakeup_tick_type_e;

/**
 * @brief	suspend power weather to power down definition
 */
typedef enum {
	 PM_POWER_BASEBAND  = BIT(0),	//weather to power on the BASEBAND before suspend.
	 PM_POWER_USB  		= BIT(1),	//weather to power on the USB before suspend.
	 PM_POWER_AUDIO 	= BIT(2),	//weather to power on the AUDIO before suspend.
}pm_suspend_power_cfg_e;

/**
 * @brief	sleep mode.
 */
typedef enum {
	//available mode for customer
	SUSPEND_MODE						= 0x00,
	DEEPSLEEP_MODE						= 0x70,	//when use deep mode pad wakeup(low or high level), if the high(low) level always in the pad,
												//system will not enter sleep and go to below of pm API, will reboot by core_6f = 0x20.
												//deep retention also had this issue, but not to reboot.
	DEEPSLEEP_MODE_RET_SRAM_LOW32K  	= 0x61, //for boot from sram
	DEEPSLEEP_MODE_RET_SRAM_LOW64K  	= 0x43, //for boot from sram
	DEEPSLEEP_MODE_RET_SRAM_LOW96K  	= 0x07, //for boot from sram
	//not available mode
	DEEPSLEEP_RETENTION_FLAG			= 0x0F,
}pm_sleep_mode_e;

/**
 * @brief	available wake-up source for customer
 */
typedef enum {
	 PM_WAKEUP_PAD   		= BIT(0),
	 PM_WAKEUP_CORE  		= BIT(1),
	 PM_WAKEUP_TIMER 		= BIT(2),
	 PM_WAKEUP_COMPARATOR 	= BIT(3),
	 PM_WAKEUP_MDEC		 	= BIT(4),
	 PM_WAKEUP_CTB 			= BIT(5),
	 PM_WAKEUP_VAD 			= BIT(6),
	 PM_WAKEUP_SHUTDOWN		= BIT(7),
}pm_sleep_wakeup_src_e;

/**
 * @brief	wakeup status
 */
typedef enum {
	WAKEUP_STATUS_PAD  				= BIT(0),
	WAKEUP_STATUS_CORE  			= BIT(1),
	WAKEUP_STATUS_TIMER 			= BIT(2),
	WAKEUP_STATUS_COMPARATOR    	= BIT(3),
	WAKEUP_STATUS_MDEC    			= BIT(4),
	WAKEUP_STATUS_CTB    			= BIT(5),
	WAKEUP_STATUS_VAD   			= BIT(6),

	STATUS_GPIO_ERR_NO_ENTER_PM		= BIT(8),
	STATUS_ENTER_SUSPEND  			= BIT(30),
}pm_wakeup_status_e;

/**
 * @brief   	zb voltage definition.
 */
typedef enum {
	ZB_0V8        =0x00,
	ZB_0V85       =0x20,
	ZB_0V9        =0x40,
	ZB_0V95       =0x60,
	ZB_1V         =0x80,
	ZB_1V05       =0xA0,
	ZB_1V1        =0xC0,
	ZB_1V15       =0xE0,
}pm_zb_voltage_e;

/**
 * @brief	digital core voltage definition.
 * @note	CORE_1V2 has the advantage of supporting higher system clock frequencies.
 * 			CORE_1V has the advantage of saving power.
 */
typedef enum {
/*
 * The naming of CORE_1V2 is to harmonize with the datasheet and actually corresponds to 1.1875V in the register table.
 */
	CORE_1V2    =0x1F,
	CORE_1V     =0x10,
}pm_digital_core_voltage_e;

/**
 * @brief	mcu status
 */
typedef enum{
	MCU_STATUS_POWER_ON  		= BIT(0),
	MCU_STATUS_REBOOT_BACK    	= BIT(2),
	MCU_STATUS_DEEPRET_BACK  	= BIT(3),
	MCU_STATUS_DEEP_BACK		= BIT(4),
}pm_mcu_status;

/**
 * @brief	early wakeup time
 */
typedef struct {
	unsigned short  suspend_early_wakeup_time_us;	/**< suspend_early_wakeup_time_us = deep_ret_r_delay_us + xtal_stable_time + early_time*/
	unsigned short  deep_ret_early_wakeup_time_us;  /**< deep_ret_early_wakeup_time_us = deep_ret_r_delay_us + early_time*/
	unsigned short  deep_early_wakeup_time_us;		/**< deep_early_wakeup_time_us = suspend_ret_r_delay_us*/
	unsigned short  sleep_min_time_us;				/**< sleep_min_time_us = suspend_early_wakeup_time_us + 200*/
}pm_early_wakeup_time_us_s;

/**
 * @brief	hardware delay time
 */
typedef struct {
	unsigned short  deep_r_delay_cycle ;			/**< hardware delay time ,deep_ret_r_delay_us = deep_r_delay_cycle * 1/16k */
	unsigned short  suspend_ret_r_delay_cycle ;		/**< hardware delay time ,suspend_ret_r_delay_us = suspend_ret_r_delay_cycle * 1/16k */

}pm_r_delay_cycle_s;

/**
 * @brief   deepsleep wakeup status
 */
typedef struct{
	unsigned char is_pad_wakeup;
	unsigned char wakeup_src;	//The pad pin occasionally wakes up abnormally in A0. The core wakeup flag will be incorrectly set in A0.
	unsigned char mcu_status;
	unsigned char rsvd;
}pm_status_info_s;


extern _attribute_aligned_(4) pm_status_info_s g_pm_status_info;
extern _attribute_data_retention_sec_ unsigned char g_pm_vbat_v;

/**
 * @brief		This function servers to set the match value for MDEC wakeup.
 * @param[in]	value - the MDEC match value for wakeup.
 * @return		none.
 */
static inline void pm_set_mdec_value_wakeup(unsigned char value)
{
	analog_write_reg8(mdec_ctrl,((analog_read_reg8(mdec_ctrl) & (~0x0f)) | value));
}

/**
 * @brief		This function serves to get deep retention flag.
 * @return		1 deep retention, 0 deep.
 */
static inline unsigned char pm_get_deep_retention_flag(void)
{
	return !(analog_read_reg8(0x7f) & BIT(0));
}

/**
 * @brief		This function serves to get wakeup source.
 * @return		wakeup source.
 */
static inline pm_wakeup_status_e pm_get_wakeup_src(void)
{
	return analog_read_reg8(0x64);
}

/**
 * @brief		This function serves to clear the wakeup bit.
 * @param[in]	status	- the interrupt status that needs to be cleared.
 * @return		none.
 */
static inline void pm_clr_irq_status(pm_wakeup_status_e status)
{
	analog_write_reg8(0x64, (analog_read_reg8(0x64) | status));
}

/**
 * @brief		This function configures a GPIO pin as the wakeup pin.
 * @param[in]	pin	- the pin needs to be configured as wakeup pin.
 * @param[in]	pol - the wakeup polarity of the pad pin(0: low-level wakeup, 1: high-level wakeup).
 * @param[in]	en  - enable or disable the wakeup function for the pan pin(1: enable, 0: disable).
 * @return		none.
 */
void pm_set_gpio_wakeup (gpio_pin_e pin, pm_gpio_wakeup_level_e pol, int en);

/**
 * @brief		This function configures pm wakeup time parameter.
 * @param[in]	param - pm wakeup time parameter.
 * @return		none.
 */
void pm_set_wakeup_time_param(pm_r_delay_cycle_s param);

/**
 * @brief		this function servers to wait bbpll clock lock.
 * @return		none.
 */
_attribute_ram_code_sec_noinline_ void pm_wait_bbpll_done(void);

/**
 * @brief		This function serves to recover system timer.
 * 				The code is placed in the ram code section, in order to shorten the time.
 * @return		none.
 */
_attribute_ram_code_sec_noinline_ void pm_stimer_recover(void);

/**
 * @brief		This function serves to set the working mode of MCU based on 32k crystal,e.g. suspend mode, deepsleep mode, deepsleep with SRAM retention mode and shutdown mode.
 * @param[in]	sleep_mode 			- sleep mode type select.
 * @param[in]	wakeup_src 			- wake up source select.
 * @param[in]	wakeup_tick_type	- tick type select. Use 32K tick count for long-time sleep or 24M tick count for short-time sleep.
 * @param[in]	wakeup_tick			- The tick value at the time of wake-up.
									  If the wakeup_tick_type is PM_TICK_STIMER, then wakeup_tick is converted to 24M. The range of tick that can be set is approximately:
									  current tick value + (18352~0xe0000000), and the corresponding sleep time is approximately: 2ms~234.88s.It cannot go to sleep normally when it exceeds this range.
									  If the wakeup_tick_type is PM_TICK_32K, then wakeup_tick is converted to 32K. The range of tick that can be set is approximately:
									  64~0xffffffff, and the corresponding sleep time is approximately: 2ms~37hours.It cannot go to sleep normally when it exceeds this range.
 * @return		indicate whether the cpu is wake up successful.
 */
_attribute_text_sec_ int pm_sleep_wakeup(pm_sleep_mode_e sleep_mode,  pm_sleep_wakeup_src_e wakeup_src, pm_wakeup_tick_type_e wakeup_tick_type, unsigned int  wakeup_tick);

/**
 * @brief		This function serves to set baseband/usb/npe power on/off before suspend sleep,If power
 * 				on this module,the suspend curent will increase;power down this module will save current,
 * 				but you need to re-init this module after suspend wakeup.All module is power down default
 * 				to save current.
 * @param[in]	value - weather to power on/off the baseband/usb/npe.
 * @param[in]	on_off - select power on or off, 0 - power off; other value - power on.
 * @return		none.
 */
void pm_set_suspend_power_cfg(pm_suspend_power_cfg_e value, unsigned char on_off);

/**
 * @brief  This function serves to set the zb voltage.
 * @param  zb_voltage - enum variable of zb voltage.
 * @return none.
 */
static inline void pm_set_zb_voltage(pm_zb_voltage_e zb_voltage)
{
	analog_write_reg8(0x00, (analog_read_reg8(0x00) & 0x1F) | zb_voltage);
}
