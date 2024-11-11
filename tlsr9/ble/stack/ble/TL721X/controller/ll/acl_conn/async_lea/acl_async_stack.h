/********************************************************************************************************
 * @file    acl_async_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
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

#ifndef STACK_BLE_CONTROLLER_LL_ACL_CONN_ASYNC_LEA_ACL_ASYNC_STACK_H_
#define STACK_BLE_CONTROLLER_LL_ACL_CONN_ASYNC_LEA_ACL_ASYNC_STACK_H_
#include "vendor/common/user_config.h"
#include "acl_async.h"
#if (LL_ASYNC_LEA_EN)
typedef struct __attribute__((packed)){
    u32 aclAPTick;
    u32 cisAPTick;
    u32 cisSyncDelayTick;
    u32 isoIntervalUs;
}async_timing_info_t;

typedef struct __attribute__((packed)){
    u8  blockValid;
    u8  rsvd;
    u16 rsvd1;
    u32 start_us;
    u32 end_us;
}async_timer_block_t;

typedef struct __attribute__((packed)){
    u8  conuts;//max 5
    async_timer_block_t block[5];
}async_timer_shaft_t;

/**
 *  @brief  MACRO: define the async flow control options for async central and peripheral
 */
#define ASYNC_FLOW_SEND_NONE                        0

#define ASYNC_FLOW_PERIPHERAL_SEND_TIMING           1
#define ASYNC_FLOW_CENTRAL_PARAM_UPDATE             2

void blc_async_rx_handler(u8 *p);

void blc_async_timingLoopProcess(void);

void blt_async_creatUnorderedTimerShaft(void);

void blt_async_calTimingGapPosition(void);

void blt_l2cap_asyncLeaDataControl(u16 connHandle, rf_packet_l2cap_t *ptrAttr);

void blt_async_cisConnCallback(u32 ownCisSyncDlyUs, u32 isoIntervalUs);

void blt_async_connUpdateCallback(u16 connHandle);

void blt_async_connStateCallback(u16 connHandle,u8 connState);

void blt_async_savePeerTimingInfo(u32 T1,u32 T2,u32 T3,u32 T4, u32 T5);

void blt_async_timingInfoProcess(u8 *data);

void blt_async_calWindowSizeOffset(void);

blc_async_sts_e blt_async_calWindowOffset(void);

blc_async_sts_e blt_async_sendTimingInfo(void);
#endif

#endif /* STACK_BLE_CONTROLLER_LL_ACL_CONN_ASYNC_LEA_ACL_ASYNC_STACK_H_ */
