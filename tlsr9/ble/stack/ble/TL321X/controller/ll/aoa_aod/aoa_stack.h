/********************************************************************************************************
 * @file    aoa_stack.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef AOA_STACK_H_
#define AOA_STACK_H_


#define     ANTENNA_MAX_NUM                                 0x07 //confirm with YeYang,our device can support 7 antennas.
//75;reference period only use 1 antenna; (160us-4-8)/2 = 74 antenna. so max 75 antennas
#define     SWITCH_PATTERN_MAX_LEN                          0x10 //0x4b //0 ~ 15, table of antenna TODO: to check
#define     CTE_COUNT_MAX_NUM                               0x10
#define     SAMPLED_CTES_MAX_NUM                            0x10
#define     CTE_TIME_MIN                                    0x02 //unit is 8us
#define     CTE_TIME_MAX                                    0x14 //unit is 8us

#define     CTE_SET_PARAM_ADVHANDLE0_DONE_FLAG              BIT(0)
#define     CTE_SET_PARAM_ADVHANDLE1_DONE_FLAG              BIT(1)
#define     CTE_SET_PARAM_ADVHANDLE2_DONE_FLAG              BIT(2)
#define     CTE_SET_PARAM_ADVHANDLE3_DONE_FLAG              BIT(3) ///now only support 4 adv set.
#define     PRD_ADV_SET_PARAM_DONE_FLAG                     BIT(4)


#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE0              BIT(0) ///
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE1              BIT(1)
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE2              BIT(2)
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE3              BIT(3)
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE4              BIT(4)
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE5              BIT(5)
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE6              BIT(6)
#define     CTE_SET_TRANSMIT_PARAM_CONNHANDLE7              BIT(7) ///now support 8 connection device:4 master and 4 slave.

#define     CTE_SET_RECEIVE_PARAM_CONNHANDLE0               BIT(0)
#define     CTE_SET_RECEIVE_PARAM_CONNHANDLE1               BIT(1)
#define     CTE_SET_RECEIVE_PARAM_CONNHANDLE2               BIT(2)
#define     CTE_SET_RECEIVE_PARAM_CONNHANDLE3               BIT(3)


enum{
    AOA_TYPE     = 0,
    AOD_TYPE_1US = 1,
    AOD_TYPE_2US = 2,
};

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
    AOD_1US_TRANSMIT            = BIT(0),
    AOD_1US_RECEIVE             = BIT(1),
    AOA_1US_SWITCH_SAMPLE       = BIT(2),
};

enum{
    SWITCH_SAMPLE_SLOT_1US      = 0x01, //confirm with YeYang, our device support 1us sample.
    SWITCH_SAMPLE_SLOT_2US      = 0x02,
};

enum{
    CTE_NOT_EXIST               = 0x00,
    CTE_TRANSMIT                = 0x01,
    CTE_RECEIVE                 = 0x02,
};

enum{
    NOT_SYNC_AOA                = BIT(0),
    NOT_SYNC_AOD_1US            = BIT(1),
    NOT_SYNC_AOD_2US            = BIT(2),
    NOT_SYNC_WITHOUT_CTE        = BIT(4),
    NOT_SYNC_ALL_PDA            = BITS4(0, 1, 2, 4),
};

typedef struct{
    //transmit and receive setting
    u8 cte_switch_pattern_len;
    u8 cte_swtich_pattern[SWITCH_PATTERN_MAX_LEN];
    u8 cte_trsmitRev_flag;  //transmit or receive flag. just for operate more convenient.
    u8 rsvd1[2];

    //transmit setting
    u8 cte_len;
    u8 cte_type;            //for transmit. AOA/AOD_1US/AOD_2US
    u8 cte_count;
    u8 cte_transmit_en;

    //receive setting
    u8 cte_slot_duration;   //for receive; 1us/2us
    u8 Max_Sampled_CTEs;    //for receive; max CTE count per periodic adv interval.
    u8 cte_sample_en;       //sample enable, operate in mainloop
    u8 cte_rx_mode_en;      //CTE RX mode enable, operate in interrupt

    u8 cte_req_en;
    u8 cte_rsp_en;
    u16 cte_req_intvl;

    u8 sequence_ctrl;
    u8 rsvd2[3];

    //IQ sample
    u8 cte_slot_real;
    u8 cte_total_samples;
    u8 rsvd3[2];
    u8 cte_iq_data[164];
}switch_pattern_t;

extern _attribute_data_retention_   _attribute_aligned_(4) switch_pattern_t     cte_connLess_switchPattern[];
extern _attribute_data_retention_   _attribute_aligned_(4) switch_pattern_t     cte_conn_switchPattern[];


ble_sts_t   blc_hci_le_setConnectionless_CTETransmitParams(hci_le_setConnectionless_CTETransmitParam_t* connLessTxParams);
ble_sts_t   blc_hci_le_setConnectionless_CTETransmit_Enable(hci_le_CTE_enable_type* connLessTxCtr);
ble_sts_t   blc_hci_le_setConnectionless_IQsample_Enable(hci_le_setConnectionless_IQsampleEn_t* IQsampleEn);
ble_sts_t   blc_hci_le_setConnection_CTEReceiveParams(hci_le_setConnection_CTERevParams_t* cteRevParam);
ble_sts_t   blc_hci_le_setConnection_CTETransmitParams(hci_le_setConnection_CTETransmitParams_t* cteTransmitParams);
ble_sts_t   blc_hci_le_connection_CTEReq_Enable(hci_le_cteReqEn_t* connCTEReqEn);
ble_sts_t   blc_hci_le_connection_CTERsp_Enable(hci_le_cteRspEn_t* connCTERspEn);
ble_sts_t   blc_hci_le_ReadAntennaInfor(u8* inforBuff);
bool        blc_le_setAntennaInfor(cte_antenna_info_t* antennaInfor);


#endif /* AOA_STACK_H_ */
