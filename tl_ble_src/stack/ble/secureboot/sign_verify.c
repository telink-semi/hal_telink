/********************************************************************************************************
 * @file    sign_verify.c
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
#include "drivers.h"
#include "secureboot_stack.h"

unsigned int sign_verify(unsigned int data_adr, unsigned int data_size, unsigned char *pub_key, unsigned char *sign)
{
    unsigned char data[256];
    unsigned char bin_hash[32];
    unsigned int  cycle   = (data_size) >> 8;
    unsigned char leftlen = (data_size & 0xff);

    SHA256_Ctx ctx[1];
    SHA256_Init(ctx, bin_hash);

    for (unsigned int i = 0; i < cycle; i++) {
        flash_read_page(data_adr + (i * 256), 256, data);
        SHA256_Process(ctx, data, 256);
    }
    if (leftlen != 0) {
        flash_read_page(data_adr + (cycle * 256), leftlen, data);
        SHA256_Process(ctx, data, leftlen);
    }
    SHA256_Done(ctx);
    eccp_curve_t *curve = secp256r1;
    return ecdsa_verify(curve, bin_hash, 32, pub_key, sign);
}
