/********************************************************************************************************
 * @file    att_uuid.c
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
#include "att_uuid.h"
#include <string.h>
#include "utility.h"

#define UUID_16_BASE_OFFSET 12


/* Base UUID : 0000[0000]-0000-1000-8000-00805F9B34FB
 * 0x2800    : 0000[2800]-0000-1000-8000-00805F9B34FB
 *  little endian 0x2800 : [00 28] -> no swapping required
 *  big endian    0x2800 : [28 00] -> swapping required
 */
static const uuid_t uuid128_base = {
    .uuidLen = ATT_128_UUID_LEN,
    .uuidVal.u128 = { 0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

static void uuid_to_uuid128(const uuid_t *src, uuid_t *dst)
{
    dst->uuidLen = ATT_128_UUID_LEN;

    switch (src->uuidLen) {
        case ATT_16_UUID_LEN:
            *dst = uuid128_base;
            memcpy((u8*)&dst->uuidVal.u128[UUID_16_BASE_OFFSET], (u8*)(u32)&src->uuidVal.u16, src->uuidLen);
            return;
        case ATT_128_UUID_LEN:
            memcpy(dst->uuidVal.u128, src->uuidVal.u128, ATT_128_UUID_LEN);
            return;
    }
}

static int uuid128_cmp(const uuid_t *u1, const uuid_t *u2)
{
    uuid_t uuid128_1, uuid128_2;

    uuid_to_uuid128(u1, &uuid128_1);
    uuid_to_uuid128(u2, &uuid128_2);

    return memcmp(uuid128_1.uuidVal.u128, uuid128_2.uuidVal.u128, ATT_128_UUID_LEN);
}

int blc_uuid_cmp(const uuid_t *u1, const uuid_t *u2)
{
    /* UUID1 length check */
    if (u1 == NULL || \
       (u1->uuidLen != ATT_16_UUID_LEN && u1->uuidLen != ATT_128_UUID_LEN)) {
        return -1;
    }

    /* UUID2 length check */
    if (u2 == NULL || \
       (u2->uuidLen != ATT_16_UUID_LEN && u2->uuidLen != ATT_128_UUID_LEN)) {
        return -1;
    }

    /* Convert to 128 bit if types don't match */
    if (u1->uuidLen != u2->uuidLen) {
        return uuid128_cmp(u1, u2);
    }

    return memcmp(u1->uuidVal.u128, u2->uuidVal.u128, u1->uuidLen);
}

bool blc_uuid_create(uuid_t *uuid, const u8 *data, u8 data_len)
{
    /* UUID2 length check */
    if (uuid == NULL || data == NULL || \
       (data_len != ATT_16_UUID_LEN && data_len != ATT_128_UUID_LEN)) {
        return false;
    }

    uuid->uuidLen = data_len;
    memcpy(uuid->uuidVal.u, data, data_len);

    return true;
}


bool blt_uuid_cmp16to128(const u8 *pUuid16, const u8 *pUuid128)
{
    u8 attBaseUuid[16] = {0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
            0x00, 0x10, 0x00, 0x00, pUuid16[0], pUuid16[1], 0x00, 0x00};
    return (memcmp(attBaseUuid, pUuid128, ATT_128_UUID_LEN) == 0);
}

bool blt_uuid_cmp16or128(const u8 *pUuid16, u8 uuidLen, const u8 *pUuid)
{
    if (uuidLen == ATT_16_UUID_LEN) {
        return ((pUuid16[0] == pUuid[0]) && (pUuid16[1] == pUuid[1]));
    } else {
        return blt_uuid_cmp16to128(pUuid16, pUuid);
    }
}



