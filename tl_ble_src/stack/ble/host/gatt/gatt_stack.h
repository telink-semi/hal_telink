/********************************************************************************************************
 * @file    gatt_stack.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once

#define GATT_PROCEDURE_TIMEOUT 30 * 1000 //about 30s

#define GATT_TIMEOUT_ATT_ERROR 0x9F

/** @brief GATT notify procedure parameters configuration */
typedef struct gatts_notify_cfg
{
    struct gatts_notify_cfg *pNext;
    u16                      attrhandle;
    u16                      valueLen;
    u8                      *value;
} gatts_notify_cfg_t;

ble_sts_t blt_gattc_mtuSizeExchangeReq(u16 connHandle, u16 mtuSize);
void      blt_gattc_notification(u16 connHandle, attr_pkt_t *attr, u16 attrLen);
void      blt_gattc_multiNotification(u16 connHandle, attr_pkt_t *attr, u16 attrLen);
void      blt_gatts_recvIndCfm(u16 connHandle, u16 scid);

u16 blt_gattc_exchangeMtu_rsp(u16 connHandle, u16 mtu);
u16 blt_gattc_handle_rsp(u16 connHandle, attr_pkt_t *attr, u16 attrLen);
