/********************************************************************************************************
 * @file    csa1.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "tl_common.h"

#include "stack/ble/ble_common.h"
#include "stack/ble/ble_stack.h"
#include "csa_stack.h"

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif
void blt_csa1_calculateChannelTable(u8* chm, u8 hop, u8 *ptbl){
    u8 tableTemp[37], num = 0;
    foreach (k, 37) {
        if (chm[k >> 3] & BIT(k & 0x07)) {
            tableTemp[num++] = k;
        }
    }
    u8 k = 0, l = 0;
    foreach (i, 37) {
        k += hop;
        if (k >= 37) {
            k -= 37;
        }
        if (chm[k >> 3] & BIT(k & 0x07)) {
            ptbl[l] = k;
        } else {
            u8 m = k;
            while (m >= num) {
                m -= num;
            }
            ptbl[l] = tableTemp[m];
        }
        ++l;
    }
}
