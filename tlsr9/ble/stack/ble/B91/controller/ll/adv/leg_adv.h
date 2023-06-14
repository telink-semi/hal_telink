/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef LEG_ADV_H_
#define LEG_ADV_H_

enum{
	LEG_ADV_STRATEGY_0 = 0, /*!< default adv strategy */
	LEG_ADV_STRATEGY_1 = 1, /*!< adv keep sending. eg: if 1M1S configuration, After a slave establishes a link, adv can continue to send,
	                         but after disabling adv, adv can no longer be enbled. */
	LEG_ADV_STRATEGY_2 = 2, /*!< Advertiser. eg: if  1M1S configuration,
	                         After a slave establishes a link, adv can still be switched on and off normally.
	                         It should be noted that: adv will be closed every time the link is established.
	                         If you need to send adv, you need to manually enable adv (HCI_LE_Set_Adv_Enable_Cmd)*/
};

/**
 * @brief      for user to initialize legacy advertising module
 * 			   notice that only one module can be selected between legacy advertising module and extended advertising module
 * @param	   none
 * @return     none
 */
void 		blc_ll_initLegacyAdvertising_module(void);



/**
 * @brief	   set the data used in advertising packets that have a data field.
 * @param[in]  *data -  advertising data buffer
 * @param[in]  len - The number of significant octets in the Advertising_Data.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t  	blc_ll_setAdvData(u8 *data, u8 len);


/**
 * @brief	   This function is used to provide data used in Scanning Packets that have a data field.
 * @param[in]  *data -  Scan_Response_Data buffer
 * @param[in]  len - The number of significant octets in the Scan_Response_Data.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t 	blc_ll_setScanRspData(u8 *data, u8 len);



/**
 * @brief      This function is used to set the advertising parameters.
 * @param[in]  intervalMin - Minimum advertising interval(Time = N * 0.625 ms, Range: 0x0020 to 0x4000)
 * @param[in]  intervalMin - Maximum advertising interval(Time = N * 0.625 ms, Range: 0x0020 to 0x4000)
 * @param[in]  advType - Advertising_Type
 * @param[in]  ownAddrType - Own_Address_Type
 * @param[in]  peerAddrType - Peer_Address_Type
 * @param[in]  *peerAddr - Peer_Address
 * @param[in]  adv_channelMap - Advertising_Channel_Map
 * @param[in]  advFilterPolicy - Advertising_Filter_Policy
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_setAdvParam( adv_inter_t intervalMin, adv_inter_t intervalMax, adv_type_t 	advType,  		own_addr_type_t ownAddrType,  \
							    u8 peerAddrType, 		 u8  *peerAddr,     	  adv_chn_map_t adv_channelMap, adv_fp_type_t   advFilterPolicy);


/**
 * @brief      This function is used to request the Controller to start or stop advertising.
 * @param	   adv_enable - Advertising_Enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_setAdvEnable(adv_en_t adv_enable);

/**
 * @brief      this function is used to set whether to continue sending broadcast packets when receiving scan request in the current adv interval.
 * @param[in]  enable - enable:continue sending broadcast packets when receiving scan request.
 * @return     none.
 */
void 		blc_ll_continue_adv_after_scan_req(u8 enable);


/**
 * @brief      This function is used to set some other channel to replace advertising chn37/38/39.
 * @param[in]  chn0 - channel to replace channel 37
 * @param[in]  chn1 - channel to replace channel 38
 * @param[in]  chn2 - channel to replace channel 39
 * @return     none
 */
void 		blc_ll_setAdvCustomedChannel (u8 chn0, u8 chn1, u8 chn2);



/**
 * @brief      This function is used to configure leg_adv enabling by API only.
 * 			   e.g. M4S4, even slave connection number is 4, leg_adv still work but can not be connected
 * @param[in]  advStrategy    can be LEG_ADV_STRATEGY_0/LEG_ADV_STRATEGY_1/LEG_ADV_STRATEGY_2
 * @return     none
 */
void 		blc_ll_ConfigLegacyAdvEnable_by_API_only (u8 advStrategy);





#endif /* LEG_ADV_H_ */
