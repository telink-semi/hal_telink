/********************************************************************************************************
 * @file    host_stack.h
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
#ifndef STACK_BLE_HOST_HOST_STACK_H_
#define STACK_BLE_HOST_HOST_STACK_H_

#include "tl_common.h"
#include "drivers.h"

#ifdef MCU_CORE_D25F_ENABLE
// todo: xh
// todo: remove later qihang.mou
#include "stack/ble/host_v1/host_internal.h"
#endif

_attribute_aligned_(4) typedef struct
{
    u32 host_init_err;

} host_param_t;

extern host_param_t blhPara;

_attribute_aligned_(4) typedef struct __attribute__((packed))
{
    u16 l2cap_connParaUpdateReq_minInterval;
    u16 l2cap_connParaUpdateReq_maxInterval;
    u16 l2cap_connParaUpdateReq_latency;
    u16 l2cap_connParaUpdateReq_timeout;

    u32 l2cap_connParaUpReq_pending; //must "u32"
    //  u8      u8_rsvd[3];


} host_acl_ms_t;

extern host_acl_ms_t blhAclms[];

_attribute_aligned_(4) typedef struct
{
    u32 rsvd;

} host_acl_m_t;

extern host_acl_m_t blhAclm[];

_attribute_aligned_(4) typedef struct
{
    u32 rsvd;

} host_acl_s_t;

extern host_acl_s_t blhAcls[];


#define BLT_HOST_DBUG(en, fmt, ...) \
    if (DBG_HOST_LOG)               \
        tlkapi_printf(en, "[HOST]" fmt "\n", ##__VA_ARGS__);

void blt_host_init(void *base, u32 size);

void *blt_host_mallocAclConn(void *head, u16 connHandle, u16 scid, u16 len);
void *blt_host_getAclConn(void *head, u16 connHandle, u16 scid);
void  blt_host_freeAclConn(void *head, void *node);


#endif /* STACK_BLE_HOST_HOST_STACK_H_ */
