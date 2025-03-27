/********************************************************************************************************
 * @file    trap_entry.h
 *
 * @brief   This is the header file for BLE SDK
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

#ifndef APP_TRAP_IRQ_H_
#define APP_TRAP_IRQ_H_


#if (__PROJECT_SNIF_MAIN_NODE__ || __PROJECT_SNIF_SUB_NODE__)

/* Save RISCV core GRegs FPU and additional CSR registers */
#define SAVE_FPU_AND_ADDITIANL_REG_EN   0

#if (SAVE_FPU_AND_ADDITIANL_REG_EN)
/* RISCV core support FPU */
#ifdef __riscv_flen
#define ARCH_RISCV_FPU
#define ARCH_RISCV_FPU_S
#endif
#endif

/* Bytes of register width  */
#ifdef ARCH_CPU_64BIT
#define STORE                   sd
#define LOAD                    ld
#define REGBYTES                8
#else
#define STORE                   sw
#define LOAD                    lw
#define REGBYTES                4
#endif

/* FPU */
#ifdef ARCH_RISCV_FPU
    #ifdef ARCH_RISCV_FPU_D
        #define FSTORE          fsd
        #define FLOAD           fld
        #define FREGBYTES       8
        #define rv_floatreg_t   s64
    #endif
    #ifdef ARCH_RISCV_FPU_S
        #define FSTORE          fsw
        #define FLOAD           flw
        #define FREGBYTES       4
        #define rv_floatreg_t   s32
    #endif
#endif

#if (SAVE_FPU_AND_ADDITIANL_REG_EN)
/* Constants to define the additional CSRs. */
#define mxstatus                0x7c4
#define ucode                   0x801

/* One additional registers to save and restore, as per the #defines above. */
#define ADDITIONAL_CSR_SIZE     2
#endif

#endif /* the end of #if (__PROJECT_SNIF_MAIN_NODE__ || __PROJECT_SNIF_SUB_NODE__) */

#endif /* APP_TRAP_IRQ_H_ */

