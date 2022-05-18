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
 * @file	plic.c
 *
 * @brief	This is the source file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "plic.h"
 unsigned char g_plic_preempt_en=0;

/**
 * @brief    This function serves to config plic when enter some function process such as flash.
 * @param[in]   preempt_en - 1 can disturb by interrupt, 0 can't disturb by interrupt.
 * @param[in]   threshold  - interrupt threshold.when the interrupt priority> interrupt threshold,the function process will be disturb by interrupt.
 * @return  none
*/
_attribute_ram_code_sec_noinline_ unsigned int plic_enter_critical_sec(unsigned char preempt_en ,unsigned char threshold)
{
	unsigned int r;
	if(g_plic_preempt_en&&preempt_en)
	{
		plic_set_threshold(threshold);
		r=0;
	}
	else
	{
	   r = core_interrupt_disable();
	}
	return r ;
}

/**
 * @brief    This function serves to config plic when exit some function process such as flash.
 * @param[in]   preempt_en - 1 can disturb by interrupt, 0 can disturb by interrupt.
 * @param[in]    r         - the value of mie register to restore.
 * @return  none
*/
_attribute_ram_code_sec_noinline_ void  plic_exit_critical_sec(unsigned char preempt_en ,unsigned int r)
{
	if (g_plic_preempt_en&&preempt_en)
	{
		plic_set_threshold(0);
	}
	else
	{
		core_restore_interrupt(r);
	}
}

