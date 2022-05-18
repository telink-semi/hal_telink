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
#ifndef BLE_FORMAT_H
#define BLE_FORMAT_H


#include "stack/ble/ble_common.h"


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



typedef struct{
	u8	type;
	u8  rf_len;
	u8	opcode;
	u8	cigId;
	u8	cisId;
	u8  phyM2S;
	u8	phyS2M;

	u32 maxSduM2S :12;
	u32 rfu0 	  :3;
	u32 framed    :1;
	u32 maxSduS2M :12;
	u32 rfu1      :4;

	u8 sduIntvlM2S[3]; //SDU_Interval_M_To_S(20 bits) + RFU(4 bits)
	u8 sduIntvlS2M[3]; //SDU_Interval_S_To_M(20 bits) + RFU(4 bits)

	u16 maxPduM2S;
	u16 maxPduS2M;
	u8	nse;
	u8	subIntvl[3];    //unit: uS

	u8 	bnM2S:4;
	u8 	bnS2M:4;
	u8 	ftM2S;
	u8 	ftS2M;
	u16	isoIntvl;		//unit: 1.25 mS

	u8	cisOffsetMin[3];
	u8	cisOffsetMax[3];
	u16	connEventCnt; //similar to instant

}rf_packet_ll_cis_req_t;

typedef struct{
	u8	type;				//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)
	u8 	opcode;
	u8  cisOffsetMin[3];
	u8  cisOffsetMax[3];
	u16	connEventCnt;
}rf_packet_ll_cis_rsp_t;

typedef struct{
	u8	type;				//RA(1)_TA(1)_RFU(2)_TYPE(4)
	u8  rf_len;				//LEN(6)_RFU(2)
	u8 	opcode;
	u32 cisAccessAddr;      //Access Address of the CIS
	u8  cisOffset[3];
	u8  cigSyncDly[3];
	u8  cisSyncDly[3];
	u16 connEventCnt;
}rf_packet_ll_cis_ind_t;

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
