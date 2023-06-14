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
#ifndef ISO_H_
#define ISO_H_

#include "stack/ble/B92/hci/hci_cmd.h"



typedef struct{

	u16 pkt_seq_num;       /*Packet_Sequence_Number*/
	u16 iso_sdu_len;       /*ISO_SDU_Length*/

	u32 timestamp;    	   /*Time_Stamp*/
	u16 sduOffset;
	u8  numHciPkt;

	u8 pkt_st		:2;   /*Packet_Status_Flag*/
	u8 pb	 		:2;   /*PB_FLAG*/
	u8 ts	 		:1;   /*TS_Flag*/
	u8 numOfCmplt_en:1;
	u8 rsvd			:2;

	u8 isoHandle;  		 /*connection_handle*/
	u8 data[1];    		 /*SDU payload*/

}sdu_packet_t;


/**
 * @brief      This function is used to send ISO data.
 * @param[in]  cisHandle or bisHandle
 * @param[in]  pData  point to data to send
 * @param[in]  len  the length to send
 * @return      Status - 0x00:  succeeded;
 * 						 other:  failed
 */
void blc_ll_timestamp_en(u8 en);






/**
 * @brief      This function is  used to identify and create the isochronous data path between the Host and the Controller for a CIS,
 * 			   CIS  configuration, or BIS identified by the Connection_Handle parameter. can also be used to configure a codec for each data path.
 * @param[in]  conn_handle - Connection handle of the CIS or BIS
 * @param[in]  dir - Data_Path_Direction
 * @param[in]  id - Data_Path_ID.
 * @param[in]  cid_assignNum - Assigned Numbers for Coding Format
 * @param[in]  cidcompId - Company ID
 * @param[in]  cid_venderDef - Vendor-defined codec ID.
 * @param[in]  contro_dly - Controller delay in microseconds
 * @param[in]  codec_cfg_len - Length of codec configuration
 * @param[in]  codec_cfg1 - Codec-specific configuration data 1
 * @param[in]  codec_cfg2 - Codec-specific configuration data 2
 * @param[in]  codec_cfg3 - Codec-specific configuration data 3
 * @param[in]  codec_cfg4 - Codec-specific configuration data 4
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_setupIsoDataPath(u16 conn_handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_venderDef,
									 u32 contro_dly, u8 codec_cfg_len, 	 u8 codec_cfg1,	   u8 codec_cfg2,     u8 codec_cfg3, u8 codec_cfg4);



/**
 * @brief      This function is used to remove the input and/or output data path(s) associated with a CIS, CIS configuration, or BIS
			   identified by the Connection_Handle parameter.
 * @param[in]  conn_handle - Connection handle of the CIS or BIS
 * @param[in]  dir_mask - Data_Path_Direction
 * @return     status, 0x00:  succeed
 * 					   other: failed
 */
ble_sts_t	blc_ll_removeIsoDataPath(u16 conn_handle, dp_dir_msk_t dir_mask);




/**
 * @brief      This function is used to send ISO data.
 * @param[in]  cisHandle or bisHandle
 * @param[in]  pData  point to data to send
 * @param[in]  len  the length to send
 * @return      Status - 0x00:  succeeded;
 * 						 other:  failed
 */
ble_sts_t blc_iso_sendData(u16 handle, u8 *pData, u16 len);


#endif


