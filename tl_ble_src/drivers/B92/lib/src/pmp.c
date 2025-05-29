/********************************************************************************************************
 * @file    pmp.c
 *
 * @brief   This is the source file for B92
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

#include "core.h"
#include "../include/pmp.h"

/**
 * @brief      This function serves to configure a PMP entry using the TOR (Top of Region) scheme.
 * @param[in]  entry    PMP entry number (0-7) to configure.
 * @param[in]  va       Virtual address representing the start of the address range.
 * @param[in]  pmpcfg   Configuration value for the PMP entry.
 * @return     None
 */
void pmp_tor_config(char entry, void *va, char pmpcfg)
{
    switch (entry) {
    case 0:
        write_csr(NDS_PMPADDR0, TOR(va));
        break;
    case 1:
        write_csr(NDS_PMPADDR1, TOR(va));
        break;
    case 2:
        write_csr(NDS_PMPADDR2, TOR(va));
        break;
    case 3:
        write_csr(NDS_PMPADDR3, TOR(va));
        break;
    case 4:
        write_csr(NDS_PMPADDR4, TOR(va));
        break;
    case 5:
        write_csr(NDS_PMPADDR5, TOR(va));
        break;
    case 6:
        write_csr(NDS_PMPADDR6, TOR(va));
        break;
    case 7:
        write_csr(NDS_PMPADDR7, TOR(va));
        break;
    case 8:
        write_csr(NDS_PMPADDR8, TOR(va));
        break;
    case 9:
        write_csr(NDS_PMPADDR9, TOR(va));
        break;
    case 10:
        write_csr(NDS_PMPADDR10, TOR(va));
        break;
    case 11:
        write_csr(NDS_PMPADDR11, TOR(va));
        break;
    case 12:
        write_csr(NDS_PMPADDR12, TOR(va));
        break;
    case 13:
        write_csr(NDS_PMPADDR13, TOR(va));
        break;
    case 14:
        write_csr(NDS_PMPADDR14, TOR(va));
        break;
    case 15:
        write_csr(NDS_PMPADDR15, TOR(va));
        break;
    }
#if __riscv_xlen == 64 /*64bit-MCU*/
    switch (entry >> 3) {
    case 0:
        write_csr(NDS_PMPCFG0, ((read_csr(NDS_PMPCFG0) & (~(0xFFLL << ((long)(entry % 8) << 3)))) | (((long)pmpcfg) << ((long)(entry % 8) << 3))));
        break;
    case 1:
        write_csr(NDS_PMPCFG2, ((read_csr(NDS_PMPCFG2) & (~(0xFFLL << ((long)(entry % 8) << 3)))) | (((long)pmpcfg) << ((long)(entry % 8) << 3))));
        break;
    }
#else /*32bit-MCU*/
    switch (entry >> 2) {
    case 0:
        write_csr(NDS_PMPCFG0, ((read_csr(NDS_PMPCFG0) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    case 1:
        write_csr(NDS_PMPCFG1, ((read_csr(NDS_PMPCFG1) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    case 2:
        write_csr(NDS_PMPCFG2, ((read_csr(NDS_PMPCFG2) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    case 3:
        write_csr(NDS_PMPCFG3, ((read_csr(NDS_PMPCFG3) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    }
#endif
}

/**
 * @brief      This function serves to configure a PMP entry using the NAPOT (Not A Power Of Two) scheme.
 * @param[in]  entry    PMP entry number (0-7) to configure.
 * @param[in]  va       Virtual address representing the start of the address range.
 * @param[in]  size     Size of the address range.
 * @param[in]  pmpcfg   Configuration value for the PMP entry.
 * @return     None
 */
void pmp_napot_config(char entry, void *va, unsigned long size, char pmpcfg)
{
    switch (entry) {
    case 0:
        write_csr(NDS_PMPADDR0, NAPOT(va, size));
        break;
    case 1:
        write_csr(NDS_PMPADDR1, NAPOT(va, size));
        break;
    case 2:
        write_csr(NDS_PMPADDR2, NAPOT(va, size));
        break;
    case 3:
        write_csr(NDS_PMPADDR3, NAPOT(va, size));
        break;
    case 4:
        write_csr(NDS_PMPADDR4, NAPOT(va, size));
        break;
    case 5:
        write_csr(NDS_PMPADDR5, NAPOT(va, size));
        break;
    case 6:
        write_csr(NDS_PMPADDR6, NAPOT(va, size));
        break;
    case 7:
        write_csr(NDS_PMPADDR7, NAPOT(va, size));
        break;
    case 8:
        write_csr(NDS_PMPADDR8, NAPOT(va, size));
        break;
    case 9:
        write_csr(NDS_PMPADDR9, NAPOT(va, size));
        break;
    case 10:
        write_csr(NDS_PMPADDR10, NAPOT(va, size));
        break;
    case 11:
        write_csr(NDS_PMPADDR11, NAPOT(va, size));
        break;
    case 12:
        write_csr(NDS_PMPADDR12, NAPOT(va, size));
        break;
    case 13:
        write_csr(NDS_PMPADDR13, NAPOT(va, size));
        break;
    case 14:
        write_csr(NDS_PMPADDR14, NAPOT(va, size));
        break;
    case 15:
        write_csr(NDS_PMPADDR15, NAPOT(va, size));
        break;
    }
#if __riscv_xlen == 64 /*64bit-MCU*/
    switch (entry >> 3) {
    case 0:
        write_csr(NDS_PMPCFG0, ((read_csr(NDS_PMPCFG0) & (~(0xFFLL << ((long)(entry % 8) << 3)))) | (((long)pmpcfg) << ((long)(entry % 8) << 3))));
        break;
    case 1:
        write_csr(NDS_PMPCFG2, ((read_csr(NDS_PMPCFG2) & (~(0xFFLL << ((long)(entry % 8) << 3)))) | (((long)pmpcfg) << ((long)(entry % 8) << 3))));
        break;
    }
#else /*32bit-MCU*/
    switch (entry >> 2) {
    case 0:
        write_csr(NDS_PMPCFG0, ((read_csr(NDS_PMPCFG0) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    case 1:
        write_csr(NDS_PMPCFG1, ((read_csr(NDS_PMPCFG1) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    case 2:
        write_csr(NDS_PMPCFG2, ((read_csr(NDS_PMPCFG2) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    case 3:
        write_csr(NDS_PMPCFG3, ((read_csr(NDS_PMPCFG3) & (~((0xFF) << ((entry % 4) << 3)))) | (((long)pmpcfg) << ((entry % 4) << 3))));
        break;
    }
#endif
}
