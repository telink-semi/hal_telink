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
#ifndef STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_ADV_H_
#define STACK_BLE_CONTROLLER_LL_PERID_WITH_RSP_ADV_H_

#include "tl_common.h"
#include "stack/ble/B92/hci/hci_cmd.h"


#define   PAWR_SUBEVENT_MAX_NUM                  128 //spec max support 128, now telink also support 128
#define   PAWR_SLOT_PER_SUBEVENT_MAX_NUM		 255 //described in ESL profile

typedef struct{
	u8 subevtStart;
	u8 subevtCount;
}subDataReq_subInfo_t;

//typedef struct{
//	u8 subevt_idx;
//	u8 rspSlot_start;
//	u8 rspSlot_count;
//	u8 rsvd;
//}setPdaSubevtData_subInfo_t;

typedef struct{
	u8 subevent;
	u8 response_slotStart;
	u8 response_slotCnt;
	u8 subevent_dataLen;

	u8* subevent_Data;
}setSubeventDataCmd_subInfo_t;


/**
 * @brief      This function is used by the Host to set the parameters for periodic advertising.
 * @param[in]  adv_handle - - Used to identify a periodic advertising train
 * @param[in]  advInter_min - Periodic_Advertising_Interval_Min(Range: 0x0006 to 0xFFFF, Time = N * 1.25 ms Time Range: 7.5 ms to 81.91875 s)
 * @param[in]  advInter_max - Periodic_Advertising_Interval_Max
 * @param[in]  property - Periodic_Advertising_Properties
 * @param[in]  numSubevents -
 * @param[in]  subeventInterval -
 * @param[in]  responseSlotDelay -
 * @param[in]  responseSlotSpace -
 * @param[in]  numResponseSlots -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_setPeriodicAdvParam_v2(adv_handle_t adv_handle, u16 advInter_min,
		                                  u16 advInter_max, perd_adv_prop_t property,
		                                  u8 numSubevents,u8 subeventInterval,
										  u8 responseSlotDelay,u8 responseSlotSpace,u8 numResponseSlots);

/*
 * LE Periodic Advertising Subevent Data Request event
 */
ble_sts_t	blc_ll_periodicAdvSubeventDataReq_evt(adv_handle_t adv_handle,u8 subeventStart, u8 subeventDataCount);


ble_sts_t	blc_ll_setPeriodicAdvSubeventData_cmd(adv_handle_t adv_handle, u8 numSubevents, setSubeventDataCmd_subInfo_t* subeventSlotParam);


#endif
