/********************************************************************************************************
 * @file    trap_entry.S
 *
 * @brief   This is the boot file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    02,2024
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "vendor/common/user_config.h"
#include "trap_entry.h"


#if (__PROJECT_SNIF_MAIN_NODE__ || __PROJECT_SNIF_SUB_NODE__)

#if !FREERTOS_ENABLE

/* * !!!NOTE!!!
 * TAKE CARE TO INCLUDE THE CORRECT cpuport.h HEADER FILE FOR THE CHIP IN USE.
 * This is done using the assembler's (not the compiler's!) include path. Add
 * the path below to the assembler's include path: rt_thread\libcpu\common\
 */

  /**
   * \brief  Macro for save necessary CSRs to stack
   * \details
   * This macro store MXSTATUS, UCODE to stack.
   */
  .macro SAVE_ADDITIONAL_CSR_CONTEXT
      #ifdef ADDITIONAL_CSR_SIZE
           addi sp, sp, -(ADDITIONAL_CSR_SIZE * REGBYTES)
           /* Store CSR mxstatus to stack */
           csrr t0, mxstatus
           STORE t0, 0 * REGBYTES(sp)
           /* Store CSR ucode to stack */
           csrr t0, ucode
           STORE t0, 1 * REGBYTES(sp)
       #endif
  .endm

  /**
   * \brief  Macro for restore necessary CSRs from stack
   * \details
   * This macro restore MXSTATUS, UCODE from stack.
   */
  .macro RESTORE_ADDITIONAL_CSR_CONTEXT
      #ifdef ADDITIONAL_CSR_SIZE
           /* Restore CSR mxstatus to stack */
           LOAD t0, 0 * REGBYTES(sp)
           csrw mxstatus, t0
           /* Restore CSR ucode to stack */
           LOAD t0, 1 * REGBYTES(sp)
           csrw ucode, t0
           addi sp, sp, +(ADDITIONAL_CSR_SIZE * REGBYTES)
       #endif
  .endm

    /**
   * \brief  Macro for save Floating GPRs to stack
   * \details
   * This macro save floating GPRs to stack(fcsr + 32 FPUs).
   */
  .macro SAVE_FLOATING_GPRS_CONTEXT
      #ifdef ARCH_RISCV_FPU
        addi    sp, sp, -33 * FREGBYTES
        frcsr   t0
        FSTORE  f0,   0 * FREGBYTES(sp)
        FSTORE  f1,   1 * FREGBYTES(sp)
        FSTORE  f2,   2 * FREGBYTES(sp)
        FSTORE  f3,   3 * FREGBYTES(sp)
        FSTORE  f4,   4 * FREGBYTES(sp)
        FSTORE  f5,   5 * FREGBYTES(sp)
        FSTORE  f6,   6 * FREGBYTES(sp)
        FSTORE  f7,   7 * FREGBYTES(sp)
        FSTORE  f8,   8 * FREGBYTES(sp)
        FSTORE  f9,   9 * FREGBYTES(sp)
        FSTORE  f10, 10 * FREGBYTES(sp)
        FSTORE  f11, 11 * FREGBYTES(sp)
        FSTORE  f12, 12 * FREGBYTES(sp)
        FSTORE  f13, 13 * FREGBYTES(sp)
        FSTORE  f14, 14 * FREGBYTES(sp)
        FSTORE  f15, 15 * FREGBYTES(sp)
        FSTORE  f16, 16 * FREGBYTES(sp)
        FSTORE  f17, 17 * FREGBYTES(sp)
        FSTORE  f18, 18 * FREGBYTES(sp)
        FSTORE  f19, 19 * FREGBYTES(sp)
        FSTORE  f20, 20 * FREGBYTES(sp)
        FSTORE  f21, 21 * FREGBYTES(sp)
        FSTORE  f22, 22 * FREGBYTES(sp)
        FSTORE  f23, 23 * FREGBYTES(sp)
        FSTORE  f24, 24 * FREGBYTES(sp)
        FSTORE  f25, 25 * FREGBYTES(sp)
        FSTORE  f26, 26 * FREGBYTES(sp)
        FSTORE  f27, 27 * FREGBYTES(sp)
        FSTORE  f28, 28 * FREGBYTES(sp)
        FSTORE  f29, 29 * FREGBYTES(sp)
        FSTORE  f30, 30 * FREGBYTES(sp)
        FSTORE  f31, 31 * FREGBYTES(sp)
        STORE   t0,  32 * FREGBYTES(sp)
      #endif
  .endm

    /**
   * \brief  Macro for restore Floating GPRs from stack
   * \details
   * This macro restore floating GPRs from stack.
   */
  .macro RESTORE_FLOATING_GPRS_CONTEXT
      #ifdef ARCH_RISCV_FPU
        LOAD    t0,  32 * FREGBYTES(sp)
        FLOAD   f0,   0 * FREGBYTES(sp)
        FLOAD   f1,   1 * FREGBYTES(sp)
        FLOAD   f2,   2 * FREGBYTES(sp)
        FLOAD   f3,   3 * FREGBYTES(sp)
        FLOAD   f4,   4 * FREGBYTES(sp)
        FLOAD   f5,   5 * FREGBYTES(sp)
        FLOAD   f6,   6 * FREGBYTES(sp)
        FLOAD   f7,   7 * FREGBYTES(sp)
        FLOAD   f8,   8 * FREGBYTES(sp)
        FLOAD   f9,   9 * FREGBYTES(sp)
        FLOAD   f10, 10 * FREGBYTES(sp)
        FLOAD   f11, 11 * FREGBYTES(sp)
        FLOAD   f12, 12 * FREGBYTES(sp)
        FLOAD   f13, 13 * FREGBYTES(sp)
        FLOAD   f14, 14 * FREGBYTES(sp)
        FLOAD   f15, 15 * FREGBYTES(sp)
        FLOAD   f16, 16 * FREGBYTES(sp)
        FLOAD   f17, 17 * FREGBYTES(sp)
        FLOAD   f18, 18 * FREGBYTES(sp)
        FLOAD   f19, 19 * FREGBYTES(sp)
        FLOAD   f20, 20 * FREGBYTES(sp)
        FLOAD   f21, 21 * FREGBYTES(sp)
        FLOAD   f22, 22 * FREGBYTES(sp)
        FLOAD   f23, 23 * FREGBYTES(sp)
        FLOAD   f24, 24 * FREGBYTES(sp)
        FLOAD   f25, 25 * FREGBYTES(sp)
        FLOAD   f26, 26 * FREGBYTES(sp)
        FLOAD   f27, 27 * FREGBYTES(sp)
        FLOAD   f28, 28 * FREGBYTES(sp)
        FLOAD   f29, 29 * FREGBYTES(sp)
        FLOAD   f30, 30 * FREGBYTES(sp)
        FLOAD   f31, 31 * FREGBYTES(sp)
        fscsr   t0
        addi    sp, sp, 33 * FREGBYTES
      #endif
  .endm


  .section .ram_code, "ax"
  .align 2
  .global trap_entry
  .func
trap_entry:

    /* Save all riscv_float regs from thread context */
    SAVE_FLOATING_GPRS_CONTEXT

    /* Save the additional CSR registers */
    SAVE_ADDITIONAL_CSR_CONTEXT

    /* Save all GPRs to thread context */
    addi sp, sp, -32 * REGBYTES

    csrr  t0, mepc
    STORE t0,   0 * REGBYTES(sp) /* XX - mepc */
    STORE x1,   1 * REGBYTES(sp) /* x1 - ra */
    STORE x2,   2 * REGBYTES(sp) /* x2 - sp */
    STORE x3,   3 * REGBYTES(sp) /* x3 - gp */
    STORE x4,   4 * REGBYTES(sp)
    STORE x5,   5 * REGBYTES(sp)
    STORE x6,   6 * REGBYTES(sp)
    STORE x7,   7 * REGBYTES(sp)
    STORE x8,   8 * REGBYTES(sp)
    STORE x9,   9 * REGBYTES(sp)
    STORE x10, 10 * REGBYTES(sp)
    STORE x11, 11 * REGBYTES(sp)
    STORE x12, 12 * REGBYTES(sp)
    STORE x13, 13 * REGBYTES(sp)
    STORE x14, 14 * REGBYTES(sp)
    STORE x15, 15 * REGBYTES(sp)
    STORE x16, 16 * REGBYTES(sp)
    STORE x17, 17 * REGBYTES(sp)
    STORE x18, 18 * REGBYTES(sp)
    STORE x19, 19 * REGBYTES(sp)
    STORE x20, 20 * REGBYTES(sp)
    STORE x21, 21 * REGBYTES(sp)
    STORE x22, 22 * REGBYTES(sp)
    STORE x23, 23 * REGBYTES(sp)
    STORE x24, 24 * REGBYTES(sp)
    STORE x25, 25 * REGBYTES(sp)
    STORE x26, 26 * REGBYTES(sp)
    STORE x27, 27 * REGBYTES(sp)
    STORE x28, 28 * REGBYTES(sp)
    STORE x29, 29 * REGBYTES(sp)
    STORE x30, 30 * REGBYTES(sp)
    STORE x31, 31 * REGBYTES(sp)

    /* exception trap handle */
    csrr  a0, mcause
    csrr  a1, mepc
    mv    a2, sp /* print debug log used */

    /* trap_handler(mcause, mepc, sp): args: a0, a1, a2 */
    call  trap_handler

    csrw mepc, a0

    STORE a0,   0 * REGBYTES(sp) /* epc - epc - program counter */

    /* Restore all GPRs from thread context */
    LOAD  x1,   1 * REGBYTES(sp) /* x1 - ra */
//  LOAD  x3,   2 * REGBYTES(sp) /* x3 - gp */
    LOAD  x4,   4 * REGBYTES(sp)
    LOAD  x5,   5 * REGBYTES(sp)
    LOAD  x6,   6 * REGBYTES(sp)
    LOAD  x7,   7 * REGBYTES(sp)
    LOAD  x8,   8 * REGBYTES(sp)
    LOAD  x9,   9 * REGBYTES(sp)
    LOAD  x10, 10 * REGBYTES(sp)
    LOAD  x11, 11 * REGBYTES(sp)
    LOAD  x12, 12 * REGBYTES(sp)
    LOAD  x13, 13 * REGBYTES(sp)
    LOAD  x14, 14 * REGBYTES(sp)
    LOAD  x15, 15 * REGBYTES(sp)
    LOAD  x16, 16 * REGBYTES(sp)
    LOAD  x17, 17 * REGBYTES(sp)
    LOAD  x18, 18 * REGBYTES(sp)
    LOAD  x19, 19 * REGBYTES(sp)
    LOAD  x20, 20 * REGBYTES(sp)
    LOAD  x21, 21 * REGBYTES(sp)
    LOAD  x22, 22 * REGBYTES(sp)
    LOAD  x23, 23 * REGBYTES(sp)
    LOAD  x24, 24 * REGBYTES(sp)
    LOAD  x25, 25 * REGBYTES(sp)
    LOAD  x26, 26 * REGBYTES(sp)
    LOAD  x27, 27 * REGBYTES(sp)
    LOAD  x28, 28 * REGBYTES(sp)
    LOAD  x29, 29 * REGBYTES(sp)
    LOAD  x30, 30 * REGBYTES(sp)
    LOAD  x31, 31 * REGBYTES(sp)

    addi  sp, sp, 32 * REGBYTES /* x2 - sp */

    /* Restore the additional CSR registers */
    RESTORE_ADDITIONAL_CSR_CONTEXT

    /* Restore all riscv_float regs from thread context */
    RESTORE_FLOATING_GPRS_CONTEXT

    mret
.endfunc

#endif

#endif /* the end of #if (__PROJECT_SNIF_MAIN_NODE__ || __PROJECT_SNIF_SUB_NODE__) */
