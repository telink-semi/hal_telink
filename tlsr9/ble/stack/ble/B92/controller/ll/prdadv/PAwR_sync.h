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

#define 	PAWR_AUX_SYNC_SUBEVENT_ENCRYPTION  0

#define 	PAWR_LAST_SUBEVENT_FLAG            BIT(7)  //subevent range from 0x00 to 0x7F, so BIT(7) can be used to indicate other function.

#define		LTV_LEN_OFFSET					   0
#define		LTV_TYPE_OFFSET					   1

#define     RSP_SLOT_EARLY_TICK                (100*SYSTEM_TIMER_TICK_1US)

enum{
	NOT_FIND_ESL_ID_FLAG = BIT(7),
};


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
	u8      rsvd0;
	u16     rsp_slot_duration_us;

	u8*     pRsp_data;
} setPDARspData_cmdParam_t;


typedef struct {
	u8      sync_subevt_num;
	u8      sync_subevt_prop;
	u8      set_syncSubevt_flag;
	u8      rsvd1[1];

	u8      sync_subevt[128]; //max subevent number is 128
} setPDASyncSubevt_cmdParam_t;


ble_sts_t   blc_hci_le_setPeriodicSyncSubevent(u16 sync_handle, u16 pda_prop, u8 num_subevent, u8* pSubevent);
ble_sts_t	blc_ll_initPeriodicAdvertisingWithResponseSync_module(void);
ble_sts_t 	blc_ll_initPeriodicAdvertisingResponseDataBuffer(u8 *pdaRspData, int maxLen_pdaRspData);
ble_sts_t   blc_hci_le_setPeriodicAdvertisingResponseData( u16 sync_handle, u16 req_pdaEvtCnt, u8 req_subEvtCnt,
		                                      	  	       u8 rsp_subEvtCnt, u8 rsp_slotIdx,   u8 rspDataLen,  u8* pRspData);

_attribute_ram_code_ void blt_pawr_sync_build_task(void);
_attribute_ram_code_ void blt_pawr_sync_subevt_start(u8 index);
_attribute_ram_code_ int blt_pawr_sync_sub_interrupt_task (int flag, void *p0, void* p1);
_attribute_noinline_ int blt_pawr_sync_mainloop_task (int flag);
_attribute_ram_code_ int blt_ll_pawrRspSlotTxTask(int flag);

//the following need to update to version 2 from v1.
// LE Periodic Advertising Report event[v2]
// LE Periodic Advertising Sync Established event[v2]
// LE Periodic Advertising Sync Transfer Received event[v2]
#endif
