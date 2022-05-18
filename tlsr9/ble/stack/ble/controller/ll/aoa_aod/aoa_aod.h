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

#ifndef AOA_AOD_H_
#define AOA_AOD_H_

#if (LL_FEATURE_EANBLE_LE_AOA_AOD)

#define		ANTENNA_MAX_NUM									0x07 //confirm with YeYang,our device can support 7 antennas.
//75;reference period only use 1 antenna; (160us-4-8)/2 = 74 antenna. so max 75 antennas
#define		SWITCH_PATTERN_MAX_LEN							0x4b

#define		CTE_SET_PARAM_ADVHANDLE0_DONE_FLAG				BIT(0)
#define		CTE_SET_PARAM_ADVHANDLE1_DONE_FLAG				BIT(1)
#define		CTE_SET_PARAM_ADVHANDLE2_DONE_FLAG				BIT(2)
#define		CTE_SET_PARAM_ADVHANDLE3_DONE_FLAG				BIT(3) ///now only support 4 adv set.
#define 	PRD_ADV_SET_PARAM_DONE_FLAG						BIT(4)


#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE0				BIT(0) ///
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE1				BIT(1)
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE2				BIT(2)
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE3				BIT(3)
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE4				BIT(4)
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE5				BIT(5)
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE6				BIT(6)
#define		CTE_SET_TRANSMIT_PARAM_CONNHANDLE7				BIT(7) ///now support 8 connection device:4 master and 4 slave.

#define		CTE_SET_RECEIVE_PARAM_CONNHANDLE0				BIT(0)
#define		CTE_SET_RECEIVE_PARAM_CONNHANDLE1				BIT(1)
#define		CTE_SET_RECEIVE_PARAM_CONNHANDLE2				BIT(2)
#define		CTE_SET_RECEIVE_PARAM_CONNHANDLE3				BIT(3)

typedef struct{
	u8		AOA_type;
	u8		AOD_type_1us;
	u8		AOD_type_2us;
	u8		rsvd;
}CTE_type_t;


enum{
	Antenna_ID0 = 0,
	Antenna_ID1 = 1,
	Antenna_ID2 = 2,
	Antenna_ID3 = 3,
	Antenna_ID4 = 4,
	Antenna_ID5 = 5,
	Antenna_ID6 = 6,
};

enum{
	AOD_1US_TRANSMIT			= BIT(0),
	AOD_1US_RECEIVE				= BIT(1),
	AOA_1US_SWITCH_SAMPLE		= BIT(2),
};

enum{
	SWITCH_SAMPLE_SLOT_1US		= 0x01, //confirm with YeYang, our device support 1us sample.
	SWITCH_SAMPLE_SLOT_2US		= 0x02,
};

enum{
	CTE_NOT_EXIST				= 0x00,
	CTE_TRANSMIT				= 0x01,
	CTE_RECEIVE					= 0x02,
};


typedef struct{
	//transmit and receive setting
	u8 cte_switch_pattern_len;
	u8 cte_swtich_pattern[SWITCH_PATTERN_MAX_LEN];

	//transmit setting
	u8 cte_len;
	u8 cte_type;          	//for transmit. AOA/AOD_1US/AOD_2US
	u8 cte_count;
	u8 cte_transmit_en;

	//receive setting
	u8 cte_slot_duration; 	//for receive; 1us/2us
	u8 Max_Sampled_CTEs;  	//for receive; max CTE count per period adv interval.
	u8 cte_sample_en;   	//sample enable
	u8 cte_trsmitRev_flag; 	//transmit or receive flag. just for operate more convenient.

	u8 cte_req_en;
	u8 cte_rsp_en;
	u16 cte_req_intvl;

	u8 sequence_ctrl;
	u8 rsvd[3];
}switch_pattern_t;

extern _attribute_data_retention_	_attribute_aligned_(4) switch_pattern_t		cte_connLess_switchPattern[];
extern _attribute_data_retention_	_attribute_aligned_(4) switch_pattern_t		cte_conn_switchPattern[];


ble_sts_t	blc_hci_le_setConnectionless_CTETransmitParams(hci_le_setConnectionless_CTETransmitParam_t* connLessTxParams);
ble_sts_t	blc_hci_le_setConnectionless_CTETransmit_Enable(hci_le_CTE_enable_type* connLessTxCtr);
ble_sts_t	blc_hci_le_setConnectionless_IQsample_Enable(hci_le_setConnectionless_IQsampleEn_t* IQsampleEn);
ble_sts_t	blc_hci_le_setConnection_CTEReceiveParams(hci_le_setConnection_CTERevParams_t* cteRevParam);
ble_sts_t	blc_hci_le_setConnection_CTETransmitParams(hci_le_setConnection_CTETransmitParams_t* cteTransmitParams);
ble_sts_t	blc_hci_le_connection_CTEReq_Enable(hci_le_cteReqEn_t* connCTEReqEn);
ble_sts_t	blc_hci_le_connection_CTERsp_Enable(hci_le_cteRspEn_t* connCTERspEn);
ble_sts_t	blc_hci_le_ReadAntennaInfor(u8* inforBuff);
bool		blc_le_setAntennaInfor(cte_antenna_infor_t* antennaInfor);

#endif ///#if (LL_FEATURE_EANBLE_LE_AOA_AOD)

#endif	///AOA_AOD_H_
