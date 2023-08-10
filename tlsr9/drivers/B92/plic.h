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
 * @file	plic.h
 *
 * @brief	This is the header file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
/**	@page PLIC
 *
 *	Introduction
 *	===============
 *   platform-level interrupt controller (PLIC)
 *
 *	API Reference
 *	===============
 *	Header File: plic.h
 */

#ifndef  INTERRUPT_H
#define  INTERRUPT_H
#include "core.h"

#include "compiler.h"
#include "reg_include/register.h"

typedef struct
{
	unsigned char preempt_en;
	unsigned char threshold;
}preempt_config_t ;

typedef void (*func_isr_t) (void);
extern _attribute_data_retention_sec_ volatile unsigned char g_plic_preempt_en;

typedef enum{
	IRQ0_EXCEPTION ,
	IRQ1_SYSTIMER,
	IRQ2_ALG,
	IRQ3_TIMER1,
	IRQ4_TIMER0,
	IRQ5_DMA,
	IRQ6_BMC,
	IRQ7_USB_CTRL_EP_SETUP,
	IRQ8_USB_CTRL_EP_DATA,
	IRQ9_USB_CTRL_EP_STATUS,
	IRQ10_USB_CTRL_EP_SETINF,
	IRQ11_USB_ENDPOINT,
    IRQ12_ZB_DM,
	IRQ13_ZB_BLE,
	IRQ14_ZB_BT,
	IRQ15_ZB_RT,
	IRQ16_PWM,
	IRQ17_PKE,
	IRQ18_UART1,
	IRQ19_UART0,
	IRQ20_DFIFO,
	IRQ21_I2C,
	IRQ22_LSPI,
	IRQ23_GSPI,
	IRQ24_USB_PWDN,
	IRQ25_GPIO,
	IRQ26_GPIO2RISC0,
	IRQ27_GPIO2RISC1,
	IRQ28_SOFT,

	IRQ29_MSPI,
	IRQ30_USB_RESET,
	IRQ31_USB_250US,
	IRQ32_CHARGE,
	IRQ33_QDEC,
	IRQ34_GPIO_SRC0,
	IRQ35_GPIO_SRC1,
	IRQ36_GPIO_SRC2,
	IRQ37_GPIO_SRC3,
	IRQ38_GPIO_SRC4,
	IRQ39_GPIO_SRC5,
	IRQ40_GPIO_SRC6,
	IRQ41_GPIO_SRC7,
	RSVD1,
	RSVD2,
	IRQ44_PM_WKUP,
	IRQ45_PM_IRQ,
	IRQ46_EMQ_IRQ,
} irq_source_e;

typedef enum{
	IRQ_PRI_LEV0,//Never interrupt
	IRQ_PRI_LEV1,
	IRQ_PRI_LEV2,
	IRQ_PRI_LEV3,
}irq_priority_e;

typedef enum{
	IRQ_PRI_NUM0,
	IRQ_PRI_NUM1,
	IRQ_PRI_NUM2,
	IRQ_PRI_NUM3,
}irq_threshold_e;

/**
 * @brief    This function serves to set plic feature.
 * @param[in]   feature - preemptive priority interrupt feature and the vector mode.
 * @return  none
 */
static inline void plic_set_feature (feature_e feature)
{
	reg_irq_feature = feature;//enable vectored in PLIC
}

/**
 * @brief    This function serves to enable preemptive priority interrupt feature.
 * @return  none
 */
static inline void plic_preempt_feature_en (void)
{
	reg_irq_feature |= FLD_FEATURE_PREEMPT_PRIORITY_INT_EN;
	g_plic_preempt_en=1;
}

/**
 * @brief    This function serves to enable preemptive priority interrupt feature.
 * @return  none
 */
static inline void plic_preempt_feature_dis (void)
{
	reg_irq_feature &=(~ FLD_FEATURE_PREEMPT_PRIORITY_INT_EN);
	g_plic_preempt_en=0;
}


/**
 * @brief    This function serves to set plic pending.
 * @param[in]  src - interrupt source.
 * @return  none
 */
static inline void plic_set_pending (irq_source_e src)
{
	reg_irq_pending(src)=BIT(src%32);
}

/**
 * @brief    This function serves to set Priority Threshold,Only active interrupts with priorities strictly greater than the threshold will cause interrupt.
 * @param[in]   threshold -  threshold level.
 * @return  none
 */
static inline void plic_set_threshold (irq_threshold_e threshold)
{
/*the priority number in the threshold Register will be saved to a preempted priority stack and the new priority number of the claimed interrupt will be written to the threshold Register.
When the mcu sends an interrupt completion message to the PLIC (plic_interrupt_complete()), the PLIC will restore(Hardware automatic operation) the highest priority number in the preempted priority stack back to the Priority Threshold Register.
It is possible that when hardware and software write threshold registers at the same time, there is a risk of software write failure, so turn off the global interrupt before software writes, and resume after writing*/
	unsigned int r = core_interrupt_disable();
	reg_irq_threshold=threshold;
	core_restore_interrupt(r);
}

/**
 * @brief    This function serves to set preemptive priority level,The priority value 0 is reserved to mean "never interrupt".
 * the larger the priority value, the higher the interrupt priority.
 * @param[in]   src- interrupt source.
 * @param[in]   priority-  priority level.
 * @return  none
 */
static inline void plic_set_priority (irq_source_e src, irq_priority_e priority)
{
	reg_irq_src_priority(src)=priority;
}


/**
 * @brief    This function serves to enable plic interrupt source.
 * @param[in]   src - interrupt source.
 * @return  none
 */
static inline void plic_interrupt_enable(irq_source_e  src)
{
	reg_irq_src(src) |= BIT(src%32);

}

/**
 * @brief    This function serves to disable plic interrupt source.
 * @param[in]   src - interrupt source.
 * @return  none
 */
static inline void plic_interrupt_disable(irq_source_e  src)
{
	reg_irq_src(src) &= (~ BIT(src%32));
}

/**
 * @brief    This function serves to clear interrupt source has completed.
 * @param[in] src - interrupt source.
 * @return  none
 */
static inline void plic_interrupt_complete(irq_source_e  src)
{
	reg_irq_done = src;
}

/**
 * @brief    This function serves to claim  interrupt.
 * @return   it return the source id which interrupted in irq_source_e enum .
 */
static inline  unsigned int plic_interrupt_claim(void)
{
	return reg_irq_done;
}



/**
 * @brief    This function serves to config plic when enter some function process such as flash.
 * @param[in]   preempt_en - 1 can disturb by interrupt, 0 can't disturb by interrupt.
 * @param[in]   threshold  - interrupt threshold.when the interrupt priority> interrupt threshold,the function process will be disturb by interrupt.
 * @return  none
*/
_attribute_ram_code_sec_noinline_ unsigned int plic_enter_critical_sec(unsigned char preempt_en ,unsigned char threshold);



/**
 * @brief    This function serves to config plic when exit some function process such as flash.
 * @param[in]   preempt_en - 1 can disturb by interrupt, 0 can disturb by interrupt.
 * @param[in]    r         - the value of mie register to restore.
 * @return  none
*/
_attribute_ram_code_sec_noinline_   void  plic_exit_critical_sec(unsigned char preempt_en ,unsigned int r);


_attribute_ram_code_sec_ void plic_isr(func_isr_t func, irq_source_e src);
#endif
