/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
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
