/********************************************************************************************************
 * @file    clic.c
 *
 * @brief   This is the source file for tl322x
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
#include "lib/include/clic.h"

#if defined(MCU_CORE_TL322X_N22)|| (CLIC_ENABLE == 1)//todo

/**
 * @brief The global variable is used to indicate whether interrupt nesting is supported.
 */
_attribute_data_retention_sec_ volatile unsigned char g_clic_preempt_en = 0;

/**
 * @brief   This function serves to init CLIC.
 * @return  none
 * @note
 *          - The default value of irq level is set to 1 for all interrupts.
 */
void clic_init(void)
{
    reg_clic_cfg = (reg_clic_cfg & (~FLD_CLIC_NLBITS)) | MASK_VAL(FLD_CLIC_NLBITS, 2); /* NLBITS reset value is 0 and can only be set to 2. */
    reg_clic_mth = 0x3f;                                                               /* The lower 6 bits of the reg_clic_input_ctl register are 0x3f by default, \n
                            so set the lower 6 bits of the threshold to 0x3f as well to block interrupts with level 0. */

    clic_interrupt_vector_en(IRQ_MTIMER);                                              /* Enable machine timer vector for machine timer. */
    clic_set_priority(IRQ_MTIMER, IRQ_PRI_LEV1);                                       /* The default value of level is set to 1 for machine timer. */
    for (unsigned int i = IRQ_SYSTIMER; i <= IRQ_RRAM; i++) {
        clic_interrupt_vector_en(i);                                                   /* Enable vector mode for the all CLIC interrupts. */
        clic_set_priority(i, IRQ_PRI_LEV1);                                            /* The default value of level is set to 1 for all interrupts. */
    }
}

/**
 * @brief       This function serves to execute the interrupt service routine, you can call this function when an interrupt occurs.
 * @param[in]   func - Interrupt service routine.
 * @return      none
 */
_attribute_ram_code_sec_ __attribute__((always_inline)) inline void clic_isr(func_clic_isr_t func)
{
    if (g_clic_preempt_en) {
        core_save_nested_context();    /* save csr and  Enable interrupt enable */
        func();
        core_restore_nested_context(); /* disable interrupt enable and restore csr */
        fence_iorw;
    } else {
        func();
    }
}

#endif
