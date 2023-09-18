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
 * @brief      for user to initialize periodic advertising withe response module.
 * @param[in]  pBuff - global buffer, same buffer as periodic advertising parameters buffer
 * @param[in]  num_periodic_adv - number of application adv_sets
 * @return     Status - 0x00: command succeeded;
 * 						0x12: num_periodic_adv exceed maximum number of supported periodic advertising.
 */
ble_sts_t	blc_ll_initPeriodicAdvWrModule_initPeriodicdAdvWrSetParamBuffer(u8 *pBuff, int num_periodic_adv);

/**
 * @brief      initialize Periodic Advertising Data buffer for all adv_set
 * @param[in]  perdAdvData -
 * @param[in]  max_len_perdAdvData -
 * @param[in]  subevent_data_cnt -
 * @return     none
 */
void 		blc_ll_initPeriodicAdvWrDataBuffer(u8 *pSubeventData, int subeventDataLenMax, int subeventDataCnt);

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

/**
 * @brief      This function is used by the Host to set periodic advertising subevent data.
 * @param[in]  adv_handle - Used to identify a periodic advertising train
 * @param[in]  numSubevents -
 * @param[in]  pSubevtCfg - refer to 'pdaSubevtData_subevtCfg_t'
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t	blc_ll_setPeriodicAdvSubeventData(adv_handle_t adv_handle, u8 num_subevent, pdaSubevtData_subevtCfg_t* pSubevtCfg);

/**
 * @brief      This function is used to create an ACL connection to a connectable advertiser [v2].
 *
 * @param[in]  adv_handle - - Used to identify a periodic advertising train
 * @param[in]  subevent - Subevent where the connection request is to be sent.
 * @param[in]  filter_policy - used to determine whether the WhiteList is used. If the White List is not used, the Peer_Address_Type and the
							   Peer_Address parameters specify the address type and address of the advertising device to connect to.
 * @param[in]  ownAdr_type - indicates the type of address being used in the connection request packets.
 * @param[in]  peerAdrType - indicates the type of address used in the connectable advertisement sent by the peer.
 * @param[in]  *peerAddr - indicates the Peer's Public Device Address, Random (static) Device Address, Non-Resolvable Private Address, or
							Resolvable Private Address depending on the Peer_Address_Type parameter.
 * @param[in]  init_phys - indicates the PHY(s) on which the advertising packets should be received on the primary advertising physical channel and
							the PHYs for which connection parameters have been specified.
 *
 * 			   Attention:
 * 			   scanInter_0/scanWindow_0/conn_min_0/conn_max_0/timeout_0 are only for 	1M PHY.  If    1M PHY is not supported, these parameters are ignored.
 * 			   scanInter_1/scanWindow_1/conn_min_1/conn_max_1/timeout_1 are only for 	2M PHY.  If    2M PHY is not supported, these parameters are ignored.
 * 			   scanInter_2/scanWindow_2/conn_min_2/conn_max_2/timeout_2 are only for Coded PHY.  If Coded PHY is not supported, these parameters are ignored.
 *
 * @param[in]  scanInter_0 - for 1M PHY: recommendations from the Host on how frequently (LE_Scan_Interval) the Controller should scan.
 * @param[in]  scanWindow_0 - for 1M PHY: recommendations from the Host on how long (LE_Scan_Window) the Controller should scan.
 * @param[in]  conn_min_0 - for 1M PHY: the minimum allowed connection interval.
 * @param[in]  conn_max_0 - for 1M PHY: the maximum allowed connection interval.
 * @param[in]  timeout_0 - for 1M PHY: Supervision timeout for the LE Link.
 * @param[in]  scanInter_1 - for 2M PHY: recommendations from the Host on how frequently (LE_Scan_Interval) the Controller should scan.
 * @param[in]  scanWindow_1 - for 2M PHY: recommendations from the Host on how long (LE_Scan_Window) the Controller should scan.
 * @param[in]  conn_min_1 - for 2M PHY: the minimum allowed connection interval.
 * @param[in]  conn_max_1 - for 2M PHY: the maximum allowed connection interval.
 * @param[in]  timeout_1 - for 2M PHY: Supervision timeout for the LE Link.
 * @param[in]  scanInter_2 - for Coded PHY: recommendations from the Host on how frequently (LE_Scan_Interval) the Controller should scan.
 * @param[in]  scanWindow_2 - for Coded PHY: recommendations from the Host on how long (LE_Scan_Window) the Controller should scan.
 * @param[in]  conn_min_2 - for Coded PHY: the minimum allowed connection interval.
 * @param[in]  conn_max_2 - for Coded PHY: the maximum allowed connection interval.
 * @param[in]  timeout_2 - for Coded PHY: Supervision timeout for the LE Link.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_extended_createConnection_v2 (adv_handle_t adv_handle, u8 subevent,
												init_fp_t  filter_policy, own_addr_type_t ownAdrType, u8 peerAdrType, u8 *peerAddr, init_phy_t init_phys,
												scan_inter_t scanInter_0, scan_wind_t scanWindow_0, conn_inter_t conn_min_0, conn_inter_t conn_max_0, conn_tm_t timeout_0,
												scan_inter_t scanInter_1, scan_wind_t scanWindow_1, conn_inter_t conn_min_1, conn_inter_t conn_max_1, conn_tm_t timeout_1,
												scan_inter_t scanInter_2, scan_wind_t scanWindow_2, conn_inter_t conn_min_2, conn_inter_t conn_max_2, conn_tm_t timeout_2 );




#endif
