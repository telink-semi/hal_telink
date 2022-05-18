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
 * @file	trng.c
 *
 * @brief	This is the source file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "trng.h"
#include "compiler.h"
/**********************************************************************************************************************
 *                                			  local constants                                                       *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                           	local macro                                                        *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                             local data type                                                     *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              global variable                                                       *
 *********************************************************************************************************************/

_attribute_data_retention_sec_ unsigned int g_rnd_m_w = 0;
_attribute_data_retention_sec_ unsigned int g_rnd_m_z = 0;

/**********************************************************************************************************************
 *                                              local variable                                                     *
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *                                          local function prototype                                               *
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/
/**
 * @brief     This function performs to get one random number.If chip in suspend TRNG module should be close.
 *            else its current will be larger.
 * @return    the value of one random number.
 */
void trng_init(void)
{
	//TRNG module Reset clear
	reg_rst2 |= FLD_RST2_TRNG;
	//turn on TRNG clock
	reg_clk_en2 |= FLD_CLK2_TRNG_EN;

	reg_trng_cr0 &= ~(FLD_TRNG_CR0_RBGEN); //disable
	reg_trng_rtcr = 0x00;				//TCR_MSEL
	reg_trng_cr0 |= (FLD_TRNG_CR0_RBGEN); //enable

	while(!(reg_rbg_sr & FLD_RBG_SR_DRDY));
	g_rnd_m_w = reg_rbg_dr;   //get the random number
	while(!(reg_rbg_sr & FLD_RBG_SR_DRDY));
	g_rnd_m_z = reg_rbg_dr;

	//Reset TRNG module
	reg_rst2 &= (~FLD_RST2_TRNG);
	//turn off TRNG module clock
	reg_clk_en2 &= ~(FLD_CLK2_TRNG_EN);

	reg_trng_cr0 &= ~(FLD_TRNG_CR0_RBGEN | FLD_TRNG_CR0_ROSEN0 | FLD_TRNG_CR0_ROSEN1 \
											| FLD_TRNG_CR0_ROSEN2 | FLD_TRNG_CR0_ROSEN3);
}

/**
 * @brief     This function performs to get one random number.
 * @return    the value of one random number.
 */
_attribute_ram_code_sec_noinline_  unsigned int trng_rand(void)  //16M clock, code in flash 23us, code in sram 4us
{

	g_rnd_m_w = 18000 * (g_rnd_m_w & 0xffff) + (g_rnd_m_w >> 16);
	g_rnd_m_z = 36969 * (g_rnd_m_z & 0xffff) + (g_rnd_m_z >> 16);
	unsigned int result = (g_rnd_m_z << 16) + g_rnd_m_w;

	return (unsigned int)( result  ^ stimer_get_tick() );
}

