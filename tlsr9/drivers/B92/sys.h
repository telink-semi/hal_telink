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
 * @file	sys.h
 *
 * @brief	This is the header file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
/**	@page SYS
 *
 *	Introduction
 *	===============
 *	Clock init and system timer delay.
 *
 *	API Reference
 *	===============
 *	Header File: sys.h
 */

#ifndef SYS_H_
#define SYS_H_
#include "reg_include/stimer_reg.h"
#include "compiler.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/
/**
 * @brief	This configuration corresponds to the hardware configuration of the GPIO voltage
 * @note	1. If the GPIO is set to 1.8V, the maximum detection voltage of the ADC input cannot be higher than 1.8V.
 * 			   If a voltage higher than this needs to be detected, external access to the divider circuit is required.
 * 			2. If VBAT_MAX_VALUE_LESS_THAN_3V6 is used, the GPIO cannot be set to 1.8V.
 * 			3. Since 1.8V IO does not comply with USB electrical layer regulations, GPIO cannot be configured to 1.8V when using USB.
 */
#define	GPIO_POWER_3V3			0
#define	GPIO_POWER_1V8			1
#define GPIO_POWER				GPIO_POWER_3V3

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
/**
 * @brief instruction delay
 */

#define	_ASM_NOP_					__asm__("nop")

#define	CLOCK_DLY_1_CYC				_ASM_NOP_
#define	CLOCK_DLY_2_CYC				_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_3_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_4_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_5_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_6_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_7_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_8_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_9_CYC				_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define	CLOCK_DLY_10_CYC			_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_


/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/

/**
 * @brief 	Power type for different application
 */
typedef enum{
	LDO_1P2_LDO_2P0 	= 0x00,	/**< 1.2V-LDO & 2.0V-LDO mode */
//	DCDC_1P2_LDO_2P0	= 0x01,	/**< 1.2V-DCDC & 2.0V-LDO mode */
//	DCDC_1P2_DCDC_2P0	= 0x03,	/**< 1.2V-DCDC & 2.0V-DCDC mode */
}power_mode_e;

/**
 * @brief 	The maximum voltage that the chip can withstand is 3.6V.
 * 			When the vbat power supply voltage is lower than 3.6V, it is configured as VBAT_MAX_VALUE_LESS_THAN_3V6 mode,
 * 			bypass is turned on, and the vbat voltage directly supplies power to the chip.
 * 			When the vbat power supply voltage may be higher than 3.6V, it is configured as VBAT_MAX_VALUE_GREATER_THAN_3V6 mode,
 * 			the bypass is closed, and the vbat voltage passes through an LDO to supply power to the chip.
 * 			The voltage of the GPIO pin (V_ioh) is the voltage after Vbat passes through the LDO (V_ldo),
 * 			and the maximum value is about 3.3V floating 10% (V_ldoh).
 * 			When Vbat > V_ldoh, V_ioh = V_ldo = V_ldoh. When Vbat < V_ldoh, V_ioh = V_ldo = Vbat.
 */
typedef enum{
	VBAT_MAX_VALUE_GREATER_THAN_3V6	= 0x00,		/*VBAT may be greater than 3.6V. */
	VBAT_MAX_VALUE_LESS_THAN_3V6	= BIT(3),	/*VBAT must be below 3.6V. */
}vbat_type_e;

/**
 * @brief command table for special registers
 */
typedef struct tbl_cmd_set_t {
	unsigned int  	adr;
	unsigned char	dat;
	unsigned char	cmd;
} tbl_cmd_set_t;


/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

extern unsigned int g_chip_version;

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/
/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_text_sec_ void sys_reboot(void);

/**
 * @brief   	This function serves to initialize system.
 * @param[in]	power_mode - power mode(LDO/DCDC/LDO_DCDC)
 * @param[in]	vbat_v		- vbat voltage type: 0 vbat may be greater than 3.6V,  1 vbat must be below 3.6V.
 * @return  	none
 */
void sys_init(power_mode_e power_mode, vbat_type_e vbat_v);

/**
 * @brief      This function performs a series of operations of writing digital or analog registers
 *             according to a command table
 * @param[in]  pt    - pointer to a command table containing several writing commands
 * @param[in]  size  - number of commands in the table
 * @return     number of commands are carried out
 */

int write_reg_table(const tbl_cmd_set_t * pt, int size);

#endif
