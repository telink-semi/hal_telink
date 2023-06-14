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
 * @file	core.h
 *
 * @brief	This is the header file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#ifndef CORE_H
#define CORE_H
#include "sys.h"

typedef enum
{
	FLD_MSTATUS_MIE = BIT(3),//M-mode interrupt enable bit
}mstatus_e;

typedef enum
{
	FLD_MIE_MSIE     = BIT(3),//M-mode software interrupt enable bit.
	FLD_MIE_MTIE     = BIT(7),//M-mode timer interrupt enable bit
	FLD_MIE_MEIE     = BIT(11),//M-mode external interrupt enable bit
}mie_e;


#define NDS_MSTATUS             0x300
#define NDS_MIE                 0x304
#define NDS_MEPC                0x341
#define NDS_MCAUSE              0x342
#define NDS_MILMB               0x7C0
#define NDS_MDLMB               0x7C1

#define read_csr(var, csr)      __asm__ volatile ("csrr %0, %1" : "=r" (var) : "i" (csr))
#define write_csr(csr, val)     __asm__ volatile ("csrw %0, %1" :: "i" (csr), "r" (val))
#define set_csr(csr, bit)       __asm__ volatile ("csrs %0, %1" :: "i" (csr), "r" (bit))
#define clear_csr(csr, bit)     __asm__ volatile ("csrc %0, %1" :: "i" (csr), "r" (bit))

/*
 * Inline nested interrupt entry/exit macros
 */
/* Svae/Restore macro */
#define save_csr(r)             long __##r;          \
                                read_csr(__##r,r);
#define restore_csr(r)          write_csr(r, __##r);
/* Support PowerBrake (Performance Throttling) feature */


#define save_mxstatus()         save_csr(NDS_MXSTATUS)
#define restore_mxstatus()      restore_csr(NDS_MXSTATUS)

 /* Nested IRQ entry macro : Save CSRs and enable global interrupt. */
#define core_save_nested_context()                              \
	 save_csr(NDS_MEPC)                              \
	 save_csr(NDS_MSTATUS)                           \
	 save_mxstatus()                                 \
	 set_csr(NDS_MSTATUS, FLD_MSTATUS_MIE);

/* Nested IRQ exit macro : Restore CSRs */
#define core_restore_nested_context()                               \
	 clear_csr(NDS_MSTATUS, FLD_MSTATUS_MIE);            \
	 restore_csr(NDS_MSTATUS)                        \
	 restore_csr(NDS_MEPC)                           \
	 restore_mxstatus()

typedef enum{
	FLD_FEATURE_PREEMPT_PRIORITY_INT_EN = BIT(0),
	FLD_FEATURE_VECTOR_MODE_EN 			= BIT(1),
}
feature_e;


/**
 * @brief Disable interrupts globally in the system.external, timer and software interrupts.
 * @return  r - the value of machine interrupt enable(MIE) register.
 * @note  this function must be used when the system wants to disable all the interrupt.
 * @return     none
 */
static inline unsigned int core_interrupt_disable(void){

	unsigned int r;
	read_csr (r, NDS_MIE);
  
	clear_csr(NDS_MIE, BIT(3)| BIT(7)| BIT(11));
  
	return r;
}

/**
 * @brief restore interrupts globally in the system. external,timer and software interrupts.
 * @param[in]  en - the value of machine interrupt enable(MIE) register before disable.
 * @return     0
 * @note this function must be used when the system wants to restore all the interrupt.
 */
static inline unsigned int core_restore_interrupt(unsigned int en){
	set_csr(NDS_MIE, en);
	return 0;
}

/**
 * @brief enable interrupts globally in the system.
 * @return  none
 */
static inline void core_interrupt_enable(void)
{
	set_csr(NDS_MSTATUS,FLD_MSTATUS_MIE);//global interrupts enable
	set_csr(NDS_MIE,FLD_MIE_MSIE |FLD_MIE_MTIE|FLD_MIE_MEIE);//machine Interrupt enable selectively
}



/**
 * @brief This function serves to get mcause(Machine Cause) value.
 * This register indicates the cause of trap, reset, NMI or the interrupt source ID of a vector interrupt.
   This register is updated when a trap, reset, NMI or vector interrupt occurs
 * @return     none
 */
static inline unsigned int core_get_mcause(void)
{
  unsigned int r;
  read_csr (r, NDS_MCAUSE);
  return r;
}

/**
 * @brief This function serves to get mepc(Machine Exception Program Counter) value.
 * When entering an exception, the hardware will automatically update the value of the mepc register
 * to the value of the instruction pc currently encountered with the exception
 * @return     none
 */
static inline unsigned int core_get_mepc(void)
{
  unsigned int r;
  read_csr (r, NDS_MEPC);
  return r;
}

#endif
