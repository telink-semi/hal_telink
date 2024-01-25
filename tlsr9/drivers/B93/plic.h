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
 * @brief	This is the header file for B93
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
/** @page PLIC
 *
 *  Introduction
 *  ===============
 *  Platform-Level Interrupt Controller (PLIC)
 *
 *  API Reference
 *  ===============
 *  Header File: plic.h
 *
 *  PLIC features
 *  ===============
 *    - Compatible with RISC-V PLIC
 *    - For preemptive priority interrupt extension, when preempt is enabled, high priority interrupt can disturb low priority interrupt.
 *    - There are two modes of interrupt, vector and regular mode, and the driver defaults to vector mode(each external interrupt has its own entry address, which can speed up the response to interrupts).
 *
 *  How to use this driver
 *  ===============
 *    - Use the feature of preemptive priority interrupt:
 *        -# Enable preempt using the function plic_preempt_feature_en().
 *        -# Set interrupt priority and threshold using the function plic_set_priority() and plic_set_threshold(), interrupt occurs only when priority is greater than threshold.
 *            - default priority is 1.
 *            - default threshold is 0.
 *        -# Enable global interrupt using the function core_interrupt_enable().
 *        -# Enable plic interrupt source, for example stimer module plic_interrupt_enable(IRQ1_SYSTIMER).
 *        -# Enable the mask of the corresponding module, for example stimer_set_irq_mask().
 *    - Do not use the feature of preemptive priority interrupt:
 *        -# Enable global interrupt using the function core_interrupt_enable().
 *        -# Enable plic interrupt source, for example stimer module plic_interrupt_enable(IRQ1_SYSTIMER).
 *        -# Enable the mask of the corresponding module, for example stimer_set_irq_mask().
 *    - Interrupt service routine
 *        -# Since the default interrupt service routine is weak (defined in plic_isr.c), you need to re-implement your interrupt service routine.
 *        -# Get the interrupt status flag bit and clear it, for example stimer_get_irq_status() determines whether corresponding interrupt occurs, and stimer_clr_irq_status() clears it.
 *        -# Your application code.
 *        -# Return from interrupt.
 *        -# The machine attribute was added to the interrupt function, and the compiler saw that it would insert code that modified the protection register, for example:
 *           void  entry_irq1(void) __attribute__ ((interrupt ("machine")));
 *
 */

#ifndef  INTERRUPT_H
#define  INTERRUPT_H
#include "core.h"

#include "compiler.h"
#include "reg_include/register.h"

/**
 * @brief Interrupt preempt config.
 */
typedef struct
{
	unsigned char preempt_en;
	unsigned char threshold;
}preempt_config_t ;

/**
 * @brief Declare the interrupt service routine type.
 */
typedef void (*func_isr_t) (void);

/**
 * @brief The global variable is used to indicate whether interrupt nesting is supported.
 */
extern _attribute_data_retention_sec_ volatile unsigned char g_plic_preempt_en;

/**
 * @brief Interrupt source number.
 */
typedef enum{
	IRQ0_EXCEPTION ,
	IRQ1_SYSTIMER,              /**< STIMER_IRQ: system timer interrupt, - stimer_irq_handler */
	IRQ2_ALG,                   /**< ALGM_IRQ: analog register master interface interrupt, - analog_irq_handler */
	IRQ3_TIMER1,                /**< TIMER1_IRQ, - timer1_irq_handler */
	IRQ4_TIMER0,                /**< TIMER0_IRQ, - timer0_irq_handler */
	IRQ5_DMA,                   /**< DMA_IRQ_IRQ, - dma_irq_handler */
	IRQ6_BMC,                   /**< BMC_IRQ: ahb bus matrix controller interrupt, - bmc_irq_handler */
	IRQ7_USB_CTRL_EP_SETUP,     /**< USB_SETUP_IRQ: USB setup interrupt, - usb_ctrl_ep_setup_irq_handler */
	IRQ8_USB_CTRL_EP_DATA,      /**< USB_DATA_IRQ: USB data interrupt, - usb_ctrl_ep_data_irq_handler */
	IRQ9_USB_CTRL_EP_STATUS,    /**< USB_STATUS_IRQ: USB status interrupt, - usb_ctrl_ep_status_irq_handler */
	IRQ10_USB_CTRL_EP_SETINF,   /**< USB_SETINF_IRQ: USB set interface interrupt, - usb_ctrl_ep_setinf_irq_handler */
	IRQ11_USB_ENDPOINT,         /**< USB_EDP_IRQ: USB edp (1-8) interrupt, - usb_endpoint_irq_handler */

	IRQ14_ZB_BT = 14,           /**< ZB_BT_IRQ: BR/EDR sub-system interrupt, - rf_bt_irq_handler */
	IRQ15_ZB_RT,                /**< ZB_BLE_TL_IRQ: BLE (TL) sub-system interrupt, - rf_irq_handler */
	IRQ16_PWM,                  /**< PWM_IRQ, - pwm_irq_handler */
	IRQ17_PKE,                  /**< PKE_IRQ, - pke_irq_handler */
	IRQ18_UART1,                /**< UART1_IRQ, - uart1_irq_handler */
	IRQ19_UART0,                /**< UART0_IRQ, - uart0_irq_handler */
	IRQ20_DFIFO,                /**< DFIFO_IRQ: audio dma fifo interrupt, - audio_irq_handler */
	IRQ21_I2C,                  /**< I2C_IRQ, - i2c_irq_handler */
	IRQ22_LSPI,                 /**< LSPI_IRQ, - lspi_irq_handler */
	IRQ23_GSPI,                 /**< GSPI_IRQ, - gspi_irq_handler */
	IRQ24_USB_PWDN,             /**< USB_PWDN_IRQ: USB suspend interrupt, - usb_pwdn_irq_handler */
	IRQ25_GPIO,                 /**< GPIO_IRQ, - gpio_irq_handler */
	IRQ26_GPIO2RISC0,           /**< GPIO2RISC[0]_IRQ, - gpio_risc0_irq_handler */
	IRQ27_GPIO2RISC1,           /**< GPIO2RISC[1]_IRQ, - gpio_risc1_irq_handler */
	IRQ28_SOFT,                 /**< SOFT_IRQ: software interrupt, - soft_irq_handler */
	IRQ29_MSPI,                 /**< MSPI_IRQ, - mspi_irq_handler */
	IRQ30_USB_RESET,            /**< USB_RESET_IRQ: USB reset interrupt, - usb_reset_irq_handler */
	IRQ31_USB_250US,            /**< USB_250US_SOF_IRQ: USB 250us or SOF interrupt, - usb_250us_irq_handler */

	IRQ33_QDEC = 33,            /**< QDEC_IRQ, - qdec_irq_handler */
	IRQ34_GPIO_SRC0,            /**< GPIO_GROUP_IRQ[0], - gpio_src0_irq_handler */
	IRQ35_GPIO_SRC1,            /**< GPIO_GROUP_IRQ[1], - gpio_src1_irq_handler */
	IRQ36_GPIO_SRC2,            /**< GPIO_GROUP_IRQ[2], - gpio_src2_irq_handler */
	IRQ37_GPIO_SRC3,            /**< GPIO_GROUP_IRQ[3], - gpio_src3_irq_handler */
	IRQ38_GPIO_SRC4,            /**< GPIO_GROUP_IRQ[4], - gpio_src4_irq_handler */
	IRQ39_GPIO_SRC5,            /**< GPIO_GROUP_IRQ[5], - gpio_src5_irq_handler */
	IRQ40_GPIO_SRC6,            /**< GPIO_GROUP_IRQ[6], - gpio_src6_irq_handler */
	IRQ41_GPIO_SRC7,            /**< GPIO_GROUP_IRQ[7], - gpio_src7_irq_handler */

	IRQ44_PM_WKUP = 44,         /**< PM_WKUP_IRQ: PM wakeup interrupt, - pm_wkup_irq_handler */
	IRQ45_PM_IRQ,               /**< PM_MIX_IRQ: PM mixed interrupt, - pm_irq_handler */
	IRQ46_DPR_IRQ,              /**< DPR_IRQ: dynamic power reduction, - dpr_irq_handler */
} irq_source_e;

/**
 * @brief Interrupt source priority. The larger the value, the higher the priority. The default priority value is 1.
 * @note  An interrupt occurs only when priority is greater than threshold.
 */
typedef enum{
	IRQ_PRI_LEV0, /**< 0 indicates that no interrupt is generated. */
	IRQ_PRI_LEV1,
	IRQ_PRI_LEV2,
	IRQ_PRI_LEV3,
}irq_priority_e;

/**
 * @brief Target interrupt priority threshold. The larger the value, the higher the threshold. The default threshold value is 0.
 */
typedef enum{
	IRQ_PRI_NUM0,
	IRQ_PRI_NUM1,
	IRQ_PRI_NUM2,
	IRQ_PRI_NUM3,
}irq_threshold_e;

/**
 * @brief    This function serves to set plic feature.
 * @param[in]   feature - Preemptive priority interrupt feature and the vector mode.
 * @return  none
 */
static inline void plic_set_feature (feature_e feature)
{
	reg_irq_feature = feature; /* set preempt and vector mode */
}

/**
 * @brief    This function serves to enable preemptive priority interrupt feature.
 *              - These three types of interrupts occur simultaneously, they are handled under the following order:M-mode external interrupt(MEI) > M-mode software interrupt(MSI) > M-mode timer interrupt(MTI).
 *              - MEI, MSI and MTI do not occur simultaneously, they can be nested within each other.
 *              - Only MEI interrupt sources can set different priority levels.
 * @return   none
 *
 */
static inline void plic_preempt_feature_en (void)
{
	reg_irq_feature |= FLD_FEATURE_PREEMPT_PRIORITY_INT_EN;
	g_plic_preempt_en=1;
}

/**
 * @brief    This function serves to disable preemptive priority interrupt feature.
 * @return  none
 */
static inline void plic_preempt_feature_dis (void)
{
	reg_irq_feature &=(~ FLD_FEATURE_PREEMPT_PRIORITY_INT_EN);
	g_plic_preempt_en=0;
}


/**
 * @brief    This function serves to set plic pending. It provides a way for software to trigger an interrupt without relying on interrupt source from external devices.
 * @param[in]  src - Interrupt source.
 * @return  none
 */
static inline void plic_set_pending (irq_source_e src)
{
	reg_irq_pending(src)=BIT(src%32);
}

/**
 * @brief    This function serves to set external interrupt priority threshold,only active interrupts with priorities strictly greater than the threshold will cause interrupt, the default threshold value is 0.
 * @param[in]   threshold -  Threshold level.
 * @return  none
 */
static inline void plic_set_threshold (irq_threshold_e threshold)
{
/*The priority number in the threshold register will be saved to a preempted priority stack and the new priority number of the claimed interrupt will be written to the threshold register.
when the mcu sends an interrupt completion message to the PLIC (plic_interrupt_complete()), the PLIC will restore(Hardware automatic operation) the highest priority number in the preempted priority stack back to the priority threshold register.
it is possible that when hardware and software write threshold registers at the same time, there is a risk of software write failure, so turn off the global interrupt before software writes, and resume after writing*/
	unsigned int r = core_interrupt_disable();
	reg_irq_threshold=threshold;
	core_restore_interrupt(r);
}

/**
 * @brief    This function serves to set external interrupts preemptive priority level. The priority value 0 is reserved to mean "never interrupt".
 * The larger the priority value, the higher the interrupt priority, the default priority value is 1.
 * @param[in]   src - Interrupt source.
 * @param[in]   priority - Priority level.
 * @return  none
 */
static inline void plic_set_priority (irq_source_e src, irq_priority_e priority)
{
	reg_irq_src_priority(src)=priority;
}


/**
 * @brief    This function serves to enable plic interrupt source.
 * @param[in]   src - Interrupt source.
 * @return  none
 */
static inline void plic_interrupt_enable(irq_source_e  src)
{
	reg_irq_src(src) |= BIT(src%32);

}

/**
 * @brief    This function serves to disable plic interrupt source.
 * @param[in]   src - Interrupt source.
 * @return  none
 */
static inline void plic_interrupt_disable(irq_source_e  src)
{
	reg_irq_src(src) &= (~ BIT(src%32));
}

/**
 * @brief    This function serves to send a complete interrupt message(by software) to the gateway to allow a new interrupt request when current interrupt is done.
 * @param[in] src - Interrupt source.
 * @return  none
 */
static inline void plic_interrupt_complete(irq_source_e  src)
{
	reg_irq_done = src;
}

/**
 * @brief    This function serves to get claimed interrupt id.
 * @return   It returns the claimed interrupt source id in irq_source_e enum.
 * @note     For vector interrupt, the hardware will automatically claim an interrupt, in general, the software does not need to claim.
 *
 */
static inline  unsigned int plic_interrupt_claim(void)
{
	return reg_irq_done;
}



/**
 * @brief       This function serves to secure code by reconfiguring interrupt threshold or mstatus.MIE to enter the critical section, such as calling some flash interface to enter some function process.
 * @param[in]   preempt_en
 *                - 1 means interrupt priority larger than the given threshold which can disturb function process.
 *                - 0 means it can't be disturbed by interrupt, global interrupt(mstatus.MIE) will be disabled.
 * @param[in]   threshold
 *                - PLIC interrupt threshold. When the interrupt priority > given threshold, the function process will be disturbed by interrupt.
 * @return
 *                - Return 0 when preempt_en = 1 and interrupt nesting is supported.
 *                - Return the value of the mstatus.MIE bit in other cases.
 * @note    plic_enter_critical_sec and plic_exit_critical_sec must be used in pairs.
 */
_attribute_ram_code_sec_noinline_ unsigned int plic_enter_critical_sec(unsigned char preempt_en ,unsigned char threshold);



/**
 * @brief    This function serves to restore interrupt threshold or mstatus.MIE to exit the critical section, such as calling some flash interface to exit some function.
 * @param[in]   preempt_en
 *                - 1 means it needs to restore the value of interrupt threshold.
 *                - 0 means it needs to restore mstatus.MIE.It must be the same as the preempt_en value passed by the plic_enter_critical_sec function.
 * @param[in]   r
 *                 - The value of mstatus.MIE or threshold to restore when exit critical section, it must be the value returned by the plic_enter_critical_sec function.
 * @return  none
 * @note        When preempt_en = 1 and interrupt nesting is supported, the interrupt threshold will be set to 0 when this function returns.
 */
_attribute_ram_code_sec_noinline_ void plic_exit_critical_sec(unsigned char preempt_en ,unsigned int r);

/**
 * @brief       This function serves to execute the interrupt service routine. you can call this function when an interrupt occurs.
 * @param[in]   func - Interrupt service routine.
 * @param[in]   src - Interrupt source.
 * @return      none
 */
_attribute_ram_code_sec_ void plic_isr(func_isr_t func, irq_source_e src);

#endif
