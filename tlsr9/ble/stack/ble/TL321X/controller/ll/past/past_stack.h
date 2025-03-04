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
#ifndef PAST_STACK_H_
#define PAST_STACK_H_


#include "stack/ble/ble_stack.h"

/* refer to BLE SPEC: Vol 6, Part B, 2.4.2.27 "LL_PERIODIC_SYNC_IND" for more information. */
typedef struct __attribute__((packed))
{
    u8          llid;
    u8          rf_len;
    u8          opcode;
    u16         id;
    sync_info_t syncInfo;
    u16         connEvtCnt;
    u16         lastPaEvtCnt;
    u8          sid   : 4;
    u8          aType : 1;
    u8          sca   : 3;
    u8          phy;
    u8          advA[6];
    u16         syncConnEvtCnt;
} rf_pkt_ll_periodic_sync_ind_t;

/////////////////// PAwR ///////////////////////
/* refer to BLE SPEC: Vol 6, Part B, 2.4.2.40 "LL_PERIODIC_SYNC_WR_IND" for more information. */
typedef struct __attribute__((packed))
{
    rf_pkt_ll_periodic_sync_ind_t pastInd;
    pawr_acad_t                   pawrAcadInfo;
} rf_pkt_ll_periodic_sync_wr_ind_t;

#endif /* PAST_STACK_H_ */
