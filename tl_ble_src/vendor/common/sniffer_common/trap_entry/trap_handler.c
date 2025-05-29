/********************************************************************************************************
 * @file    trap_handler.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "core.h"
#include "tl_common.h"
#include "drivers.h"
#include "trap_entry.h"

#include "vendor/common/sniffer_common/sniffer_common.h"


#if (__PROJECT_SNIF_MAIN_NODE__ || __PROJECT_SNIF_SUB_NODE__ || __PROJECT_SNIF_FOB_NODE__)

    /* freeRTOS use its own trap_ */
    #if (!FREERTOS_ENABLE)


        #ifndef APP_EXCEPTION_STUCK_EN
            #define APP_EXCEPTION_STUCK_EN 0
        #endif


/* RISCV exception core CSRs & GRegs status */
struct exception_core_status
{
    unsigned int pc;
    unsigned int ra;
    unsigned int sp;
    unsigned int gp;
    unsigned int mcause;
    unsigned int mdcause;
    unsigned int mtval;
} __attribute__((packed));

_attribute_data_retention_ volatile struct exception_core_status g_exception_core_status;


    ///* CSR Address: 0x7c9 (non-standard read/write) */
    //#define mdcause                           0x7c9
    ///* CSR Address: 0x343 (standard read/write) */
    //#define mtval                             0x343

        #define RISCV_TRAP_DEBUG_EN  1
        #define RISCV_TRAP_ECALL_DBG 0

        #if __riscv_xlen == 64
            #define RISCV_MCAUSE_INT   0x8000000000000000UL
            #define RISCV_MCAUSE_CAUSE 0x7FFFFFFFFFFFFFFFUL
        #else
            #define RISCV_MCAUSE_INT   0x80000000UL
            #define RISCV_MCAUSE_CAUSE 0x7FFFFFFFUL
        #endif

        #define RISCV_IRQ_S_SOFT  1
        #define RISCV_IRQ_H_SOFT  2
        #define RISCV_IRQ_M_SOFT  3  /* Machine software interrupt */
        #define RISCV_IRQ_S_TIMER 5
        #define RISCV_IRQ_H_TIMER 6
        #define RISCV_IRQ_M_TIMER 7  /* Machine timer interrupt */
        #define RISCV_IRQ_S_EXT   9
        #define RISCV_IRQ_H_EXT   10
        #define RISCV_IRQ_M_EXT   11 /* Machine external interrupt */
        #define RISCV_IRQ_COP     12
        #define RISCV_IRQ_HOST    13

        /* Machine mode MCAUSE */
        #define RISCV_TRAP_M_I_ADD_MISALIGNED    0  /* Instruction address misaligned */
        #define RISCV_TRAP_M_I_ACC_FAULT         1  /* Instruction access fault */
        #define RISCV_TRAP_M_ILLEGAL_INSTRUCTION 2  /* Illegal instruction */
        #define RISCV_TRAP_M_BREAKPOINT          3  /* Breakpoint */
        #define RISCV_TRAP_M_L_ADD_MISALIGNED    4  /* Load address misaligned */
        #define RISCV_TRAP_M_L_ACC_FAULT         5  /* Data load access fault */
        #define RISCV_TRAP_M_S_ADD_MISALIGNED    6  /* Store/AMO address misaligned */
        #define RISCV_TRAP_M_S_ACC_FAULT         7  /* Store/AMO access fault */
        #define RISCV_TRAP_U_ECALL               8  /* Environment call from U-mode */
        #define RISCV_TRAP_S_ECALL               9
        #define RISCV_TRAP_H_ECALL               10
        #define RISCV_TRAP_M_ECALL               11 /* Environment call from M-mode */
        #define RISCV_TRAP_M_I_PAGE_FAULT        12 /* Instruction page fault */
        #define RISCV_TRAP_M_L_PAGE_FAULT        13 /* Data load page fault */
        #define RISCV_TRAP_M_S_PAGE_FAULT        15 /* Data store page fault */
        #define RISCV_TRAP_M_STACKOVF            32 /* Stack overflow exception */
        #define RISCV_TRAP_M_STACKUDF            33 /* Stack underflow exception */

        /* Supervisor mode SCAUSE */
        #define RISCV_TRAP_S_I_ACC_FAULT  1  /* Instruction access fault */
        #define RISCV_TRAP_S_L_ACC_FAULT  5  /* Data load access fault */
        #define RISCV_TRAP_S_S_ACC_FAULT  7  /* Data store access fault */
        #define RISCV_TRAP_S_I_PAGE_FAULT 12 /* Instruction page fault */
        #define RISCV_TRAP_S_L_PAGE_FAULT 13 /* Data load page fault */
        #define RISCV_TRAP_S_S_PAGE_FAULT 15 /* Data store page fault */

        #define MIP_SSIP                  (1 << RISCV_IRQ_S_SOFT)
        #define MIP_HSIP                  (1 << RISCV_IRQ_H_SOFT)
        #define MIP_MSIP                  (1 << RISCV_IRQ_M_SOFT)
        #define MIP_STIP                  (1 << RISCV_IRQ_S_TIMER)
        #define MIP_HTIP                  (1 << RISCV_IRQ_H_TIMER)
        #define MIP_MTIP                  (1 << RISCV_IRQ_M_TIMER)
        #define MIP_SEIP                  (1 << RISCV_IRQ_S_EXT)
        #define MIP_HEIP                  (1 << RISCV_IRQ_H_EXT)
        #define MIP_MEIP                  (1 << RISCV_IRQ_M_EXT)

/* RISCV exception stack frame */
struct exception_stack_frame
{
    unsigned int x1;  //ra   : Return address
    unsigned int x2;  //sp   : Stack pointer
    unsigned int x3;  //gp   : Global pointer
    unsigned int x4;  //tp   : Thread pointer
    unsigned int x5;  //t0   : Temporary
    unsigned int x6;  //t1   : Temporary
    unsigned int x7;  //t2   : Temporary
    unsigned int x8;  //s0/fp: Save register,frame pointer
    unsigned int x9;  //s1   : Save register
    unsigned int x10; //a0   : Function argument,return value
    unsigned int x11; //a1   : Function argument,return value
    unsigned int x12; //a2   : Function argument
    unsigned int x13; //a3   : Function argument
    unsigned int x14; //a4   : Function argument
    unsigned int x15; //a5   : Function argument
    unsigned int x16; //a6   : Function argument
    unsigned int x17; //a7   : Function argument
    unsigned int x18; //s2   : Save register
    unsigned int x19; //s3   : Save register
    unsigned int x20; //s4   : Save register
    unsigned int x21; //s5   : Save register
    unsigned int x22; //s6   : Save register
    unsigned int x23; //s7   : Save register
    unsigned int x24; //s8   : Save register
    unsigned int x25; //s9   : Save register
    unsigned int x26; //s10  : Save register
    unsigned int x27; //s11  : Save register
    unsigned int x28; //t3   : Temporary
    unsigned int x29; //t4   : Temporary
    unsigned int x30; //t5   : Temporary
    unsigned int x31; //t6   : Temporary
} __attribute__((packed));

static void print_stack_frame(u32 *sp)
{
    struct exception_stack_frame *esf = (struct exception_stack_frame *)(sp + 1);

    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "=================================================================\r\n");
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x1 (ra   : Return address                ) ==> 0x%x\r\n", esf->x1);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x2 (sp   : Stack pointer                 ) ==> 0x%x\r\n", esf->x2);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x3 (gp   : Global pointer                ) ==> 0x%x\r\n", esf->x3);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x4 (tp   : Thread pointer                ) ==> 0x%x\r\n", esf->x4);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x5 (t0   : Temporary                     ) ==> 0x%x\r\n", esf->x5);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x6 (t1   : Temporary                     ) ==> 0x%x\r\n", esf->x6);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x7 (t2   : Temporary                     ) ==> 0x%x\r\n", esf->x7);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x8 (s0/fp: Save register,frame pointer   ) ==> 0x%x\r\n", esf->x8);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x9 (s1   : Save register                 ) ==> 0x%x\r\n", esf->x9);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x10(a0   : Function argument,return value) ==> 0x%x\r\n", esf->x10);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x11(a1   : Function argument,return value) ==> 0x%x\r\n", esf->x11);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x12(a2   : Function argument             ) ==> 0x%x\r\n", esf->x12);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x13(a3   : Function argument             ) ==> 0x%x\r\n", esf->x13);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x14(a4   : Function argument             ) ==> 0x%x\r\n", esf->x14);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x15(a5   : Function argument             ) ==> 0x%x\r\n", esf->x15);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x16(a6   : Function argument             ) ==> 0x%x\r\n", esf->x16);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x17(a7   : Function argument             ) ==> 0x%x\r\n", esf->x17);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x18(s2   : Save register                 ) ==> 0x%x\r\n", esf->x18);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x19(s3   : Save register                 ) ==> 0x%x\r\n", esf->x19);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x20(s4   : Save register                 ) ==> 0x%x\r\n", esf->x20);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x21(s5   : Save register                 ) ==> 0x%x\r\n", esf->x21);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x22(s6   : Save register                 ) ==> 0x%x\r\n", esf->x22);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x23(s7   : Save register                 ) ==> 0x%x\r\n", esf->x23);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x24(s8   : Save register                 ) ==> 0x%x\r\n", esf->x24);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x25(s9   : Save register                 ) ==> 0x%x\r\n", esf->x25);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x26(s10  : Save register                 ) ==> 0x%x\r\n", esf->x26);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x27(s11  : Save register                 ) ==> 0x%x\r\n", esf->x27);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x28(t3   : Temporary                     ) ==> 0x%x\r\n", esf->x28);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x29(t4   : Temporary                     ) ==> 0x%x\r\n", esf->x29);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x30(t5   : Temporary                     ) ==> 0x%x\r\n", esf->x30);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "x31(t6   : Temporary                     ) ==> 0x%x\r\n", esf->x31);
    tlkapi_printf(RISCV_TRAP_DEBUG_EN, "=================================================================\r\n");
}

/**
 * @brief  This defines an trap handler to handle RISCV exceptions.
 * @param[in]  mcause:
 * @param[in]  mepc:
 * @param[in]  sp:
 * @return none
 */
u32 trap_handler(u32 mcause, u32 mepc, u32 *sp)
{
    /* Do your trap handling */
    if (mcause & RISCV_MCAUSE_INT) {
        if ((mcause & RISCV_MCAUSE_CAUSE) == RISCV_IRQ_M_TIMER) {
            /* Machine timer interrupt */
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "MTMR_IRQ\r\n");
        } else if ((mcause & RISCV_MCAUSE_CAUSE) == RISCV_IRQ_M_SOFT) {
            /* Machine software interrupt */
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "MSW_IRQ\r\n");
        } else if ((mcause & RISCV_MCAUSE_CAUSE) == RISCV_IRQ_M_EXT) {
            /* Machine external interrupt */
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "MEXT_IRQ\r\n");
        }
    }
        #if (RISCV_TRAP_ECALL_DBG)
    else if ((mcause & RISCV_MCAUSE_CAUSE) == RISCV_TRAP_M_ECALL) {
        /* Environment call from M-mode */
        tlkapi_printf(RISCV_TRAP_DEBUG_EN, "ECALL_M-mode\r\n");
        mepc += 4;
    }
        #endif
    else {
        struct exception_stack_frame *esf = (struct exception_stack_frame *)(sp + 1);
        g_exception_core_status.pc        = mepc;
        g_exception_core_status.ra        = esf->x1;
        g_exception_core_status.sp        = (u32)sp;
        g_exception_core_status.gp        = esf->x3;
        g_exception_core_status.mcause    = mcause;
        g_exception_core_status.mdcause   = read_csr(NDS_MDCAUSE);
        g_exception_core_status.mtval     = read_csr(NDS_MTVAL);

        /* Unhandled Trap */

        /**************************** Machine Trap Value: MTVAL *****************************************************
         * This register is updated when a trap is taken to M-mode. The updated value is dependent on the
         * cause of traps:
         * @ For    hardware breakpoint exceptions,
         *          address-misaligned exceptions, or
         *          access-fault exceptions, it is the effective faulting addresses.
         * @ For    illegal instruction exceptions, the updated value is the faulting instruction.
         *          If the length of the instruction is less than XLEN bits long, the upper bits of
         *          mtval are cleared to zero.
         * @ For other exceptions, mtval is set to zero.
         ************************************************************************************************************/

        /**************************** Machine Detailed Trap Cause: mdcause *****************************************************
         * This register further disambiguates causes of traps recorded in the mcause register.
         *
         * The value of MDCAUSE for precise exception:
         * @ When mcause == 1 (Instruction access fault)
         *      Value   Meaning
         *      0       Reserved
         *      1       ECC/Parity error
         *      2       PMP instruction access violation
         *      3       Bus error
         *      4       Reserved
         * @ When mcause == 2 (Illegal instruction)
         *      Value   Meaning
         *      0       Please parse the mtval CSR
         *      1       FP disabled exception
         *      2       ACE disabled exception
         * @ When mcause == 5 (Load access fault)
         *      Value   Meaning
         *      0       Reserved
         *      1       ECC/Parity error
         *      2       PMP load access violation
         *      3       Bus error
         *      4       Misaligned address
         *      5       Reserved
         *      6       Reserved
         *      7       Reserved
         * @ When mcause == 7 (Store access fault)
         *      Value   Meaning
         *      0       Reserved
         *      1       ECC/Parity error
         *      2       PMP store access violation
         *      3       Bus error
         *      4       Misaligned address
         *      5       Reserved
         *      6       Reserved
         *      7       Reserved
         *
         * The value of MDCAUSE for imprecise exception:
         * @ When mcause == Local Interrupt 16 (ECC error local interrupt)
         *      Value   Meaning
         *      0       Reserved
         *      1       LM slave port ECC/Parity error
         *      2       Imprecise store ECC/Parity error
         *      3       Imprecise load ECC/Parity error
         * @ When mcause == Local Interrupt 17 (Bus read/write transaction error local interrupt)
         *      Value   Meaning
         *      0       Reserved
         *      1       Bus read error
         *      2       Bus write error
         *      3       PMP error caused by load instructions
         *      4       PMP error caused by store instructions
         *      5       Reserved
         *      6       Reserved
         *
         * @ For other exceptions and interrupts, this register will not be updated.
         ************************************************************************************************************/

        tlkapi_printf(RISCV_TRAP_DEBUG_EN, "UnHandled Trap: mcause=0x%x, mdcause=0x%x, mvtal=0x%x\r\n", mcause, read_csr(NDS_MDCAUSE), read_csr(NDS_MTVAL));

        switch (mcause) {
        case RISCV_TRAP_M_I_ADD_MISALIGNED:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Instruction address misaligned\r\n");
            break;
        case RISCV_TRAP_M_I_ACC_FAULT:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Instruction access fault\r\n");
            break;
        case RISCV_TRAP_M_ILLEGAL_INSTRUCTION:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Illegal instruction\r\n");
            break;
        case RISCV_TRAP_M_BREAKPOINT:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Breakpoint\r\n");
            break;
        case RISCV_TRAP_M_L_ADD_MISALIGNED:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Load address misaligned\r\n");
            break;
        case RISCV_TRAP_M_L_ACC_FAULT:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Data load access fault\r\n");
            break;
        case RISCV_TRAP_M_S_ADD_MISALIGNED:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Store/AMO address misaligned\r\n");
            break;
        case RISCV_TRAP_M_S_ACC_FAULT:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Store/AMO access fault\r\n");
            break;
        case RISCV_TRAP_U_ECALL:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Environment call from U-mode\r\n");
            break;
        case RISCV_TRAP_M_ECALL:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Environment call from M-mode\r\n");
            break;
        case RISCV_TRAP_M_STACKOVF:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Stack overflow exception\r\n");
            break;
        case RISCV_TRAP_M_STACKUDF:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Stack underflow exception\r\n");
            break;
        default:
            tlkapi_printf(RISCV_TRAP_DEBUG_EN, "Uknown exception: 0x%x\r\n", mcause);
            break;
        }

        tlkapi_printf(RISCV_TRAP_DEBUG_EN, "epc(mepc : mepc                          ) ==> 0x%x\r\n", mepc);
        print_stack_frame(sp);

        #if (APP_EXCEPTION_STUCK_EN)
            #if (UI_LED_ENABLE)
                #ifdef GPIO_LED_RED
                    gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
                #endif

                #ifdef GPIO_LED_WHITE
                    gpio_write(GPIO_LED_WHITE, LED_ON_LEVEL);
                #endif

                #ifdef GPIO_LED_BLUE
                    gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
                #endif

                #ifdef GPIO_LED_GREEN
                    gpio_write(GPIO_LED_GREEN, !LED_ON_LEVEL);
                #endif
            #endif

            while (1) {
            #if (TLKAPI_DEBUG_ENABLE)
                tlkapi_debug_handler();

                #if (TLKAPI_DEBUG_CHANNEL == TLKAPI_DEBUG_CHANNEL_UART)
                    extern void tlkapi_uart_irq_handler(void);
                    tlkapi_uart_irq_handler();
                #endif
            #endif
        }
        #else
            app_start_reboot();
        #endif
    }

    return mepc;
}

    #endif

#endif /* the end of #if (__PROJECT_SNIF_MAIN_NODE__ || __PROJECT_SNIF_SUB_NODE__ || __PROJECT_SNIF_FOB_NODE__) */
