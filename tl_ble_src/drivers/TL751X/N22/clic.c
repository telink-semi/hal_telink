/********************************************************************************************************
 * @file    clic.c
 *
 * @brief   This is the source file for TL751X
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
#include "clic.h"

#if defined(MCU_CORE_TL751X_N22)

/**
 * @brief       This function serves to execute the interrupt service routine, you can call this function when an interrupt occurs.
 * @param[in]   func - Interrupt service routine.
 * @return      none
 */
_attribute_ram_code_sec_ __attribute__((always_inline)) inline void clic_isr(func_clic_isr_t func)
{
    func(); /* irq handler */
}


#endif
