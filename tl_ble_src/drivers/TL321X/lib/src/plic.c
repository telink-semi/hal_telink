/********************************************************************************************************
 * @file    plic.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "lib/include/plic.h"

/**
 * @brief The global variable is used to indicate whether interrupt nesting is supported.
 */
_attribute_data_retention_sec_ volatile unsigned char g_plic_preempt_en = 0;

/**
 * @brief The global variable is used to save irq_src0 register value.
 */
unsigned int g_irq_src0_value = 0;

/**
 * @brief The global variable is used to save irq_src1 register value.
 */
unsigned int g_irq_src1_value = 0;

/**
 * @brief The global variable is used to save MIE register value.
 */
unsigned int g_mie_value = 0;

/**
 * @brief The global variable is used to save MSTATUS register value.
 */
unsigned int g_mstatus_value = 0;

/**
 * @brief This variable is used to indicate whether to disable global interrupts when entering WFI, 1: disable, 0: not disable.
 */
unsigned int g_wfi_disable_int = 0;

/**
 * @brief       This function serves to secure code by reconfiguring interrupt threshold or mstatus.MIE to enter the critical section, such as calling some flash interface to enter some function process.
 * @param[in]   preempt_en
 *                - 1 means interrupt priority larger than the given threshold which can disturb function process.
 *                - 0 means it can't be disturbed by interrupt, global interrupt(mstatus.MIE) will be disabled.
 * @param[in]   threshold
 *                - PLIC interrupt threshold. When the interrupt priority is greater than the maximum of the given threshold and the threshold before calling the interface, the function process will be disturbed by interrupt.
 * @return
 *                - When preempt_en = 1 and interrupt nesting is supported, the interrupt threshold before calling the interface is returned.
 *                - Return the value of the mstatus.MIE bit in other cases.
 * @note    plic_enter_critical_sec and plic_exit_critical_sec must be used in pairs.
 */
_attribute_ram_code_sec_noinline_ unsigned int plic_enter_critical_sec(unsigned char preempt_en, unsigned char threshold)
{
    unsigned int r;
    if (g_plic_preempt_en && preempt_en)
    {
        /**
         * Get the current value of reg_irq_threshold, if the target value to be written is larger than the current value, update it to reg_irq_threshold, otherwise do not update it, \n
         * to achieve the purpose of shielding the interrupt with priority less than or equal to the threshold.
         */
        r = reg_irq_threshold;
        if (threshold > r)
        {
            plic_set_threshold(threshold);
        }
    }
    else
    {
        r = core_interrupt_disable();
    }
    return r;
}

/**
 * @brief    This function serves to restore interrupt threshold or mstatus.MIE to exit the critical section, such as calling some flash interface to exit some function.
 * @param[in]   preempt_en
 *                - 1 means it needs to restore the value of interrupt threshold.
 *                - 0 means it needs to restore mstatus.MIE.It must be the same as the preempt_en value passed by the plic_enter_critical_sec function.
 * @param[in]   r
 *                 - The value of mstatus.MIE or threshold to restore when exit critical section, it must be the value returned by the plic_enter_critical_sec function.
 * @return  none
 */
_attribute_ram_code_sec_noinline_ void plic_exit_critical_sec(unsigned char preempt_en, unsigned int r)
{
    if (g_plic_preempt_en && preempt_en)
    {
        plic_set_threshold(r); /* Restore to the value returned by the same level plic_enter_critical_sec. */
    }
    else
    {
        core_restore_interrupt(r);
    }
}

/**
 * @brief       This function serves to execute the interrupt service routine. you can call this function when an interrupt occurs.
 * @param[in]   func - Interrupt service routine.
 * @param[in]   src - Interrupt source see @ref irq_source_index.
 * @return      none
 */
_attribute_ram_code_sec_ __attribute__((always_inline)) inline void plic_isr(func_isr_t func, unsigned int src)
{
    /**
     * Adding always_inline modifier function is for code reduction and better real-time performance.
     * - if not add always_inline, entry_irqX function in the call plic_isr, the compiler may be in accordance with the function jump processing, entry_irqX function if there is a function jump, \n
     *   see the compiler's processing is: all the general-purpose registers of the mcu are in the stack and out of the stack protection.
     * - if you add always_inline, and the user's interrupt handling function does not have a complex function call, the compiler can do, only the general-purpose registers used by the program for the stack and out of the stack protection.
     */
    if (g_plic_preempt_en)
    {
        /**
         * MEI not interrupted by MSI or MTI handling
         *   -# save MIE register;
         *   -# if MEI cannot be interrupted by MSI and MTI, clear the corresponding bit(The bit corresponding to CORE_PREEMPT_PRI_MODE3 is an invalid bit, \n
         *      clearing the corresponding bit will not affect the mie function, the purpose of doing this is to reduce the code judgment);
         *   -# restore MIE register.
         */
        save_csr(NDS_MIE);                     /* save MIE register */
        clear_csr(NDS_MIE, g_plic_preempt_en); /* select the interrupt nesting priority at which MEI can be interrupted by MSI and MTI. */
        core_save_nested_context();            /* save csr and  Enable interrupt enable */
        func();                                /* irq handler */
        core_restore_nested_context();         /* disable interrupt enable and restore csr */
        restore_csr(NDS_MIE);                  /* restore MIE */
        plic_interrupt_complete(src);          /* complete interrupt */
        /**
         * Fence IO to avoid this competing state of interrupt completion and interrupt claim occurring at the same time. \n
         * PLIC is required to ensure that a complete message for the previous interrupt has reached the PLIC before sending the interrupt claim.
         */
        fence_iorw;
    }
    else
    {
        func();                       /* irq handler */
        plic_interrupt_complete(src); /* complete interrupt */
    }
}

/**
 * @brief   This function serves to clear all PLIC request When global interrupt is disabled.
 * @return
 *          - 1 indicates that all PLIC interrupt requests were cleared successfully.
 *          - 0 indicates failure to clear all PLIC interrupt requests, failure of interrupt clearing indicates that the interrupt source keeps triggering in one of two ways:
                -# The corresponding interrupt status is not cleared.
                -# The level that triggers the interrupt is always present.
 * @note
 *          - When global interrupt is disabled, the application needs to call this interface to ensure that all PLIC interrupt requests have been processed.
 *          - When global interrupt is enabled, the application does not need to call this interface, the hardware and interrupt service routine handle interrupt requests.
 */
_attribute_ram_code_sec_noinline_ int plic_clr_all_request(void)
{
    unsigned int claim_cnt = 0;
    unsigned int cur_claim = 0;

    /**
     * when global interrupts are disabled, software needs to ensure that all interrupts have been claimed and completed, \n
     * When global interrupt enable, hardware handles the claim, complete is handled in the plic_isr function.
     */
    do
    {
        cur_claim = plic_interrupt_claim();
        plic_interrupt_complete(cur_claim);
        claim_cnt++;
    } while ((cur_claim != 0) && (claim_cnt <= IRQ_RZ * 2));

    /* Clearing all PLIC interrupt requests fails, returning an error code to notify the application program. */
    if (claim_cnt > IRQ_RZ * 2)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief      This function serves to save and disable all PLIC interrupt sources.
 * @return     none
 * @note       plic_all_interrupt_save_and_disable and plic_all_interrupt_restore must be used in pairs.
 */
_attribute_ram_code_sec_noinline_ void plic_all_interrupt_save_and_disable(void)
{
    /* save PLIC irq and disable. */
    g_irq_src1_value = reg_irq_src1;
    g_irq_src0_value = reg_irq_src0;
    reg_irq_src1 = 0;
    reg_irq_src0 = 0;
}

/**
 * @brief      This function serves to restore and all PLIC interrupt sources.
 * @return     none
 * @note       plic_all_interrupt_save_and_disable and plic_all_interrupt_restore must be used in pairs.
 */
_attribute_ram_code_sec_noinline_ void plic_all_interrupt_restore(void)
{
    /* restore PLIC irq. */
    reg_irq_src0 = g_irq_src0_value;
    reg_irq_src1 = g_irq_src1_value;
}

/**
 * @brief      This function serves to save and disable interrupts, including the MIE bit of the MSTATUS register(depends on the first parameter,regardless of whether the global interrupt is enabled or not.), \n
 *             MSIE, MTIE, and MEIE bits of the MIE register, and all PLIC interrupt sources.
 * @param[in]  flag - Global interrupt disable flag, 1: disable, 0: not disable.
 * @param[in]  mie - MSIE, MTIE, and MEIE Which one wakes up WFI.
 * @note
 *  - When the core is awoken by the taken interrupt and global interrupts enable, it will resume and start to execute from the corresponding interrupt service routine.
 *  - When the core is awoken by the pending interrupt and global interrupts disable, it will resume and start to execute from the instruction after the WFI instruction.
 *  - plic_irqs_preprocess_for_wfi and plic_irqs_postprocess_for_wfi must be used in pairs.
 */
_attribute_ram_code_sec_noinline_ void plic_irqs_preprocess_for_wfi(unsigned char flag, mie_e mie)
{
    if (flag)
    {
        g_wfi_disable_int = 1;
        g_mstatus_value = core_interrupt_disable();
    }
    else
    {
        g_wfi_disable_int = 0;
    }

    plic_all_interrupt_save_and_disable();
    g_mie_value = core_mie_disable(FLD_MIE_MSIE | FLD_MIE_MTIE | FLD_MIE_MEIE);
    core_mie_enable(mie);
}

/**
 * @brief      This function serves to restore interrupts, including the MIE bit of the MSTATUS register, \n
 *             MSIE, MTIE, and MEIE bits of the MIE register, and all PLIC interrupt sources.
 * @return     none
 * @note
 *  - Make sure that the status flag of the corresponding interrupt is cleared before calling this interface.
 *  - This function will enable PLIC interrupt so this interface cannot be called from the interrupt service routine.
 *  - plic_irqs_preprocess_for_wfi and plic_irqs_postprocess_for_wfi must be used in pairs.
 */
_attribute_ram_code_sec_noinline_ void plic_irqs_postprocess_for_wfi(void)
{
    if (g_wfi_disable_int)
    {
        /* When global interrupt is disabled, the application needs to call this interface to ensure that the corresponding PLIC interrupt requests have been processed. */
        plic_clr_all_request();
    }

    plic_all_interrupt_restore();
    core_mie_restore(g_mie_value);

    if (g_wfi_disable_int)
    {
        core_restore_interrupt(g_mstatus_value);
    }
}
