/********************************************************************************************************
 * @file    subrate_stack.h
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
#ifndef STACK_BLE_CONTROLLER_LL_SUBRATE_SUBRATE_STACK_H_
#define STACK_BLE_CONTROLLER_LL_SUBRATE_SUBRATE_STACK_H_

typedef struct{
    u8  llid;
    u8  rf_len;
    u8  opcode;
    u16 subrateFactor_min;
    u16 subrateFactor_max;
    u16 max_latency;
    u16 continue_num;
    u16 timeout;
}rf_pkt_ll_subrate_req_t;


typedef struct{
    u8  llid;
    u8  rf_len;
    u8  opcode;
    u16 subrateFactor;
    u16 subrateBaseEvent;
    u16 latency;
    u16 continue_num;
    u16 timeout;
}rf_pkt_ll_subrate_ind_t;


u32 blt_ll_subrate_getNextEvent(st_ll_conn_t* pAclConn, u16 start_inst);
ble_sts_t blt_ll_initSubrateByHandle(u16 handle);
void blt_ll_resetSubrateByHandle(u16 handle);

#endif /* STACK_BLE_CONTROLLER_LL_SUBRATE_SUBRATE_STACK_H_ */
