/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_SYNC_H_
#define STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_SYNC_H_


enum{
	PDA_SYNC_SUBEVT_PROP_INCLUDE_TXPOWER = BIT(6),
};

/**
 *  @brief  Command Parameters for "7.8.126 LE Set Periodic Advertising Response Data command"
 */
typedef struct {
	u16     sync_handle;
	u16     req_event_count; // indicate PAwR event count. i.e. receive packet in the PAwR event count. ---paEventCounter

	u8      req_subevt_idx;  // indicate subevent. i.e. receive packet in the subevent idx.
	u8      rsp_subevt_idx;  // the subevent that the response shall be sent in
	u8      rsp_slot_idx;    // the response slot in which this response data is to be transmitted. note slot need to be in the response subevent.
	u8      rsp_data_len;

	u8      rsp_max_dataLen;
	u8      rsvd0[3];

	u8*     pRsp_data;
} setPDARspData_cmdParam_t;


typedef struct {
	u8      sync_subevt_num;
	u8      sync_subevt_prop;
	u8      rsvd1[2];

	u8      sync_subevt[128]; //max subevent number is 128
} setPDASyncSubevt_cmdParam_t;

//the following need to update to version 2 from v1.
// LE Periodic Advertising Report event[v2]
// LE Periodic Advertising Sync Established event[v2]
// LE Periodic Advertising Sync Transfer Received event[v2]
#endif
