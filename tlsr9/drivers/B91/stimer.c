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
 * @file	stimer.c
 *
 * @brief	This is the source file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "stimer.h"
/**
 * @brief     This function performs to set delay time by us.
 * @param[in] microsec - need to delay.
 * @return    none
*/
_attribute_ram_code_sec_noinline_
 void delay_us(unsigned int microsec)
{
	unsigned long t = stimer_get_tick();
	while(!clock_time_exceed(t, microsec)){
	}
}

/*
 * @brief     This function performs to set delay time by ms.
 * @param[in] millisec - need to delay.
 * @return    none
*/
_attribute_ram_code_sec_noinline_
 void delay_ms(unsigned int millisec)
{
	unsigned long t = stimer_get_tick();
	while(!clock_time_exceed(t, millisec*1000)){
	}
}
