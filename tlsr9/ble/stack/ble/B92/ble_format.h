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
#ifndef BLE_FORMAT_H
#define BLE_FORMAT_H


#include "stack/ble/B92/ble_common.h"


typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 rfu1   :3;
    u8 rf_len;
}rf_acl_data_head_t;



typedef struct {
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 cie    :1;
    u8 rfu0   :1;
    u8 npi    :1;
    u8 rfu1   :1;
    u8 rf_len;
}rf_cis_data_hdr_t;



typedef struct {
    u8 llid   :2;
    u8 cssn   :3;
    u8 cstf   :1;
    u8 rfu0   :2;
    u8 rf_len;
}rf_bis_data_hdr_t;

/* EIR Data Type, Advertising Data Type (AD Type) and OOB Data Type Definitions */
typedef enum {
	DT_FLAGS								= 0x01,		//	Flag
	DT_INCOMPLT_LIST_16BIT_SERVICE_UUID		= 0x02,		//	Incomplete List of 16-bit Service Class UUIDs
	DT_COMPLETE_LIST_16BIT_SERVICE_UUID	    = 0x03,		//	Complete List of 16-bit Service Class UUIDs
	DT_INCOMPLT_LIST_32BIT_SERVICE_UUID    	= 0x04,		//	Incomplete List of 32-bit Service Class UUIDs
	DT_COMPLETE_LIST_32BIT_SERVICE_UUID		= 0x05,		//	Complete List of 32-bit Service Class UUIDs
	DT_INCOMPLT_LIST_128BIT_SERVICE_UUID   	= 0x06,		//	Incomplete List of 128-bit Service Class UUIDs
	DT_COMPLETE_LIST_128BIT_SERVICE_UUID	= 0x07,		//	Complete List of 128-bit Service Class UUIDs
	DT_SHORTENED_LOCAL_NAME					= 0x08,		//	Shortened Local Name
	DT_COMPLETE_LOCAL_NAME					= 0x09,		//	Complete Local Name
	DT_TX_POWER_LEVEL						= 0x0A,		//	Tx Power Level

	DT_CLASS_OF_DEVICE						= 0x0D,		//	Class of Device
	DT_SERVICE_DATA							= 0x16,		//	Service Data
	DT_APPEARANCE							= 0x19,		//	Appearance

	DT_CHM_UPT_IND							= 0x28,		//	Channel Map Update Indication
	DT_BIGINFO								= 0x2C,		//	BIGInfo
	DT_BROADCAST_CODE						= 0x2D,		// 	Broadcast_Code
	DT_CSIP_RSI                             = 0x2E,
	DT_BROADCAST_NAME						= 0x30,		//	Broadcast_Name
	DT_PAWR_TIMING_INFO						= 0x32,     //  PAwR timing information
	DT_3D_INFORMATION_DATA					= 0x3D,		//	3D Information Data

	DATA_TYPE_MANUFACTURER_SPECIFIC_DATA 	= 0xFF,     //	Manufacturer Specific Data
}data_type_t;


typedef struct{
	unsigned char length;
	unsigned char type;
	unsigned char data[1];
}advData_str_t;  //ADV data structure
typedef struct{
	u8	type;
	u8  rf_len;
	u8 	opcode;
	u8  cig_id;
	u8	cis_id;
	u8	errorCode;
}rf_packet_ll_cis_terminate_t;



typedef struct{
	union{
		rf_bis_data_hdr_t  bisPduHdr;
		rf_cis_data_hdr_t  cisPduHdr;
		rf_acl_data_head_t aclPduHdr;
		struct{
			u8 type;
			u8 rf_len;
		}pduHdr;
	}llPduHdr;        /* LL PDU Header: 2 */
	u8 	llPayload[1]; /* Max LL Payload length: 251 */
}llPhysChnPdu_t;

typedef struct{
	u32 dma_len;
	llPhysChnPdu_t llPhysChnPdu;
}rf_packet_ll_data_t;








typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16  handle;
	u8	dat[20];
}rf_packet_att_t;


typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2cap;
	u16	chanid;

	u8	att;
	u16 handle;

	u8	dat[20];

}rf_packet_att_data_t;













#endif	/* BLE_FORMAT_H */
