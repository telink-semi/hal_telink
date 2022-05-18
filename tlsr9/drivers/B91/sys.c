/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
 * @file	sys.c
 *
 * @brief	This is the source file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "sys.h"
#include "core.h"
#include "pm.h"
#include "compiler.h"
#include "analog.h"
#include "gpio.h"
#include "mspi.h"
#include "stimer.h"


unsigned int g_chip_version=0;

extern void pm_update_status_info(void);

/**
 * @brief   	This function serves to initialize system.
 * @param[in]	power_mode	- power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]	vbat_v		- vbat voltage type: 0 vbat may be greater than 3.6V,  1 vbat must be below 3.6V.
 * @return  	none
 */
void sys_init(power_mode_e power_mode, vbat_type_e vbat_v)
{
	/**
	 * reset function will be cleared by set "1",which is different from the previous configuration.
	 * This setting turns off the TRNG and NPE modules in order to test power consumption.The current
	 * decrease about 3mA when those two modules be turn off.changed by zhiwei,confirmed by kaixin.20200828.
	 */
	reg_rst 	= 0xffbbffff;
	reg_clk_en 	= 0xffbbffff;

	analog_write_reg8(0x8c,0x02);		//<1>:reg_xo_en_clk_ana_ana=1

	//when VBAT power supply > 4.1V and LDO switch to DCDC,DCDC_1V8 voltage will ascend to the supply power in a period time,
	//cause the program can not run. Need to trim down dcdc_flash_out before switch power mode.
	//confirmed by haitao,modify by yi.bao(20210119)
	if(DCDC_1P4_DCDC_1P8 == power_mode)
	{
		analog_write_reg8(0x0c, 0x40);  //poweron_dft: 0x44 --> 0x40.
										//<2:0> dcdc_trim_flash_out,flash/codec 1.8V/2.8V trim down 0.2V in DCDC mode.
	}
	analog_write_reg8(0x0a, power_mode);//poweron_dft:	0x90.
										//<0-1>:pd_dcdc_ldo_sw,	default:00, dcdc & bypass ldo status bits.
										//		dcdc_1p4	dcdc_1p8	ldo_1p4		ldo_1p8
										//00:		N			N			Y			Y
										//01:		Y			N			N			Y
										//10:		Y			N			N			N
										//11:		Y			Y			N			N
    analog_write_reg8(0x0b, 0x3b);		//poweron_dft:	0x7b -> 0x3b.
										//<6>:mscn_pullup_res_enb,	default:1,->0 enable 1M pullup resistor for mscn PAD.
	analog_write_reg8(0x05,analog_read_reg8(0x05) & (~BIT(3)));//poweron_dft:	0x02 -> 0x02.
										//<3>:24M_xtl_pd,		default:0,->0 Power up 24MHz XTL oscillator.
	analog_write_reg8(0x06,analog_read_reg8(0x06) & ~(BIT(0) | vbat_v | BIT(6) | BIT(7)));//poweron_dft:	0xff -> 0x36 or 0x3e.
										//<0>:pd_bbpll_ldo,		default:1,->0 Power on ana LDO.
										//<3>:pd_vbus_sw,		default:1,->0 Power up of bypass switch.
										//<6>:spd_ldo_pd,		default:1,->0 Power up spd ldo.
										//<7>:dig_ret_pd,		default:1,->0 Power up retention  ldo.
	analog_write_reg8(0x01, 0x45);		//poweron_dft:	0x44 -> 0x45.
										//<0-2>:bbpll_ldo_trim,			default:100,->101 measured 1.186V.The default value is sometimes crashes.
										//<4-6>:ana_ldo_trim,1.0-1.4V	default:100,->100 analog LDO output voltage trim: 1.2V

	write_csr(NDS_MILMB,0x01);
	write_csr(NDS_MDLMB,0x80001);

	pm_update_status_info();
	g_pm_vbat_v = vbat_v>>3;

	//xo_ready check should be done after Xtal manual on_off, we put it here to save code running time, code running time between
	//Xtal manual on_off and xo_ready check can be used as Xtal be stable timimg.
	while( BIT(7) != (analog_read_reg8(0x88) & (BIT(7))));	//<7>: xo_ready_ana, R, aura xtl ready signal.

	//When bbpll_ldo_trim is set to the default voltage value, when doing high and low temperature stability tests,it is found that
	//there is a crash.The current workaround is to set other voltage values to see if it is stable.If it fails,repeat the setting
	//up to three times.The bbpll ldo trim must wait until 24M is stable.(add by weihua.zhang, confirmed by yi.bao and wenfeng 20200924)
	pm_wait_bbpll_done();

	if(g_pm_status_info.mcu_status == MCU_STATUS_DEEPRET_BACK)
	{
		pm_stimer_recover();
	}else{
#if SYS_TIMER_AUTO_MODE
	reg_system_ctrl |=(FLD_SYSTEM_TIMER_AUTO|FLD_SYSTEM_32K_TRACK_EN);	//enable 32k track and stimer auto.
	reg_system_tick = 0x01;	//initial next tick is 1,kick system timer
#else
	reg_system_ctrl	|= FLD_SYSTEM_32K_TRACK_EN | FLD_SYSTEM_TIMER_EN;	//enable 32k track and stimer. Wait for pll to stabilize before using stimer.
#endif
	}

	g_chip_version = read_reg8(0x1401fd);

	//if clock src is PAD or PLL, and hclk = 1/2cclk, use reboot may cause problem, need deep to resolve(add by yi.bao, confirm by guangjun 20201016)
	if(g_pm_status_info.mcu_status == MCU_STATUS_REBOOT_BACK)
	{
		//Use PM_ANA_REG_POWER_ON_CLR_BUF0 BIT(1) to represent the reboot+deep process, which is related to the function pm_update_status_info.
		analog_write_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0, analog_read_reg8(PM_ANA_REG_POWER_ON_CLR_BUF0) | BIT(1));	//(add by weihua.zhang, confirmed by yi.bao 20201222)
		pm_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, PM_TICK_STIMER_16M, (stimer_get_tick() + 100*SYSTEM_TIMER_TICK_1MS));
	}
	//**When testing AES_demo, it was found that the timing of baseband was wrong when it was powered on, which caused some of
	//the registers of ceva to go wrong, which caused the program to run abnormally.(add by weihua.zhang, confirmed by junwen 20200819)
	else if(0xff == g_chip_version)	//A0
	{
		if(g_pm_status_info.mcu_status == MCU_STATUS_POWER_ON)	//power on
		{
			analog_write_reg8(0x7d, 0x80);	//power on baseband
			pm_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_TIMER, PM_TICK_STIMER_16M, (stimer_get_tick() + 100*SYSTEM_TIMER_TICK_1MS));
		}
	}
	analog_write_reg8(0x7d, 0x80);		//poweron_dft:	0x03 -> 0x80.
										//<0>:pg_zb_en,		default:1,->0 power on baseband.
										//<1>:pg_usb_en,	default:1,->0 power on usb.
										//<2>:pg_npe_en,	default:1,->0 power on npe.
										//<7>:pg_clk_en,	default:0,->1 enable change power sequence clk.
}

