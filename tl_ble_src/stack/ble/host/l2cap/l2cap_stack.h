/********************************************************************************************************
 * @file    l2cap_stack.h
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
#ifndef STACK_BLE_L2CAP_STACK_H_
#define STACK_BLE_L2CAP_STACK_H_

#include "stack/ble/host/host_stack.h"
#include "stack/ble/host/att/att_stack.h"

typedef struct
{
    u8 *rx_p;
    u8 *tx_p;

    u16 max_rx_size;
    u16 max_tx_size;

    u16 init_MTU;
    u16 rsvd;

    u32 mtuReqSendTimeUs;

} l2cap_buff_t;

typedef struct __attribute__((packed))
{
    u8  code;
    u8  identifier;
    u16 dataLen;
    u8  data[0];
} signal_pkt_t;

typedef struct __attribute__((packed))
{
    u8 code;
    u8 data[0];
} smp_pkt_t;

typedef struct __attribute__((packed))
{
    u16 sduLen;
    u8  info[0];
} coc_start_pkt_t;

typedef struct __attribute__((packed))
{
    u8 info[0];
} coc_cont_pkt_t;

typedef struct __attribute__((packed))
{
    union
    {
        coc_start_pkt_t start;
        coc_cont_pkt_t  cont;
    };
} coc_pkt_t;

typedef struct __attribute__((packed))
{
    u16 pduLen;
    u16 cid;

    union
    {
        attr_pkt_t   att;
        signal_pkt_t signal;
        smp_pkt_t    smp;
        coc_pkt_t    coc;
    } payload;
} l2cap_pkt_t;

extern l2cap_buff_t  l2cap_buff_m;
extern l2cap_buff_t  l2cap_buff_s;
extern l2cap_buff_t *pl2cap_buff;

void blt_l2cap_para_pre_init(void);

u8   blt_UpdateParameter_request(u16 connHandle);
u16  blt_l2cap_getAclRxBufferSize(void);
u16  blt_l2cap_getAclPeripheralRxBufferSize(void);
void blt_l2cap_processConnParamUpdateReq(u16 connHandle, host_acl_ms_t *);

u8 *blt_l2cap_get_s_tx_buff(u16 connHandle);
u8 *blt_l2cap_get_m_tx_buff(u16 connHandle);
u8 *blt_l2cap_get_tx_buff(u16 connHandle);

ble_sts_t blt_l2cap_pushData_2_controller(u16 connHandle, u16 cid, u8 *format, int format_len, u8 *pDate, int data_len);

u8 *blt_l2cap_pktPack(u16 connHandle, u8 *raw_pkt);


#endif /* L2CAP_STACK_H_ */
