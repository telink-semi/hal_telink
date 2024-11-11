/********************************************************************************************************
 * @file    iso_stack.h
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
#ifndef ISO_STACK_H_
#define ISO_STACK_H_


#include "stack/ble/controller/ll/ll_stack.h"


#include "stack/ble/ble_common.h"
#include "stack/ble/ble_stack.h"
#include "stack/ble/ble_format.h"
#include "stack/ble/controller/ll/acl_conn/acl_stack.h"
#include "stack/ble/controller/csa/csa_stack.h"


/******************************* iso_config start ******************************************************************/
#define         ISO_CODE_IMPROVE_SIHUI                          0



#define         TLK_T_MSS                                       (150)//350
#define         TLK_TX_TRIG_OFFSET                              (100) // 48MHZ use 80~90us, 96MHZ use 45~50us
#define         TLK_TM_DELAY                                    (20)


#define         PACK_UNKNOWN                                    0xFF



#define     CIS_EMPTY_SEND_DEFAULT           1

#define     ISO_TESt_SUPPORT_MAX_NUM        (1)


#define     ADD_SUD_TIMESTAMP_EN            (1)

#define     ISO_DATA_TIMESTAMP_UNIT_US_EN   (0)



//private define some command, should be different values from standard LE commands
#define     HCI_CMD_LE_ISO_DATA                                 0x1000


//now u32 is enough
typedef unsigned int        iso_evtcnt_t;  //CIS event count
//typedef unsigned long long    iso_evtcnt_t;


//ISO CIS/bis RX Event Buffer strcut begin
typedef struct iso_rx_evt_tag{
    iso_evtcnt_t            curRcvdPldNum;      //The cisPayloadNum corresponding to the received CIS Data PDU.
    rf_packet_ll_data_t*    pCurrIsoRxPdu;      //ISO RX PDU buffer address
    u32                     cisRefAP; //Calculate the corresponding CIS reference anchor value according to the received peer CisPayloadNum.
    u8                      link_idx;
    u8                      payloadLen;         //The length indicates the size of the CIS Payload and MIC, if included.
    u8                      llid;              //
    u8                      null_flag;         //mark if PDU is NULL PDU
}iso_rx_evt_t;

/******************************* iso_config end ******************************************************************/



_attribute_aligned_(4)
typedef struct {
    u8      iso_buf_num;
    u8      u8_rsvd[3];

} iso_para_t;  //ISO parameters

extern  _attribute_aligned_(4)  iso_para_t      iso_param;

extern u8 gIsoTsEn;

/* Special protection code for use */
extern unsigned char pm_check_info;


/******************************* iso start ******************************************************************/


/*
 * ISOIAL data packet  can optimize
 */
typedef struct{

//0
    u32 timestamp;
    u16 offset;     //payload offset or hci rx buffer to SDU buffer
    u16 sn_offset; //rf rx: sn_offset  when rx from hci, hci2pdu mark numHciPkt


//8
    u16 connHandle       :12;
    u16 pb               :2;
    u16 ts               :1;
    u16 RFU2             :1;
//10
    u16 iso_dl_len       :14;  //iso_data_load_length
    u16 RFU3             :2;


//12
    u8 data[1];

}ial_data_packet_t;



typedef ble_sts_t (*blt_ll_pushIsoDataFun) (u16 connHandle, iso_pb_flag_t PB_Flag, u8 TS_Flag, u32 time_stamp, u16 seqnum, u16 total_len, u16 cur_len, u8 *pData);

ble_sts_t   blc_hci_le_readBufferSize_v2_cmd(hci_le_readBufSize_v2_retParam_t *);
ble_sts_t   blc_hci_le_read_iso_tx_sync(u16 iso_connHandle, hci_le_readIsoTxSync_retParam_t *pRetParam);
ble_sts_t   blt_ll_ResetBisRxFifo(u8 bis_sync_handle);
ble_sts_t   blt_iso_proSduPacket(sdu_packet_t *sdu);





ble_sts_t   blt_iso_process_sdu_in_data(sdu_packet_t *, iso_pb_flag_t, u8 , u32, u16, u16, u16, u8 *);


ble_sts_t   blt_hci_processIsoData(iso_data_packet_t *pIsoDatPkt);
/******************************* iso end ********************************************************************/




/******************************* iso_test start ******************************************************************/

typedef enum{
    ISO_TEST_DISABLE            =  0x00,
    ISO_TEST_TRANSMIT_MODE,
    ISO_RECEIVE_MODE,
}iso_test_mode_t;

typedef struct{
    u32 successCnt;
    u32 missedCnt;
    u32 failedCnt;
    u32 expectCnt;
}iso_test_receive_infor_t;

typedef struct{
    u32 send_pkt_cnt;
    u32 isoTestSendTick;
}iso_test_transmit_infor_t;

typedef struct{

    u8  occupy;
    u8  isoTestMode;  // 0: test mode disable, 1: transmit  2: receive
    u8  isoTest_payload_type;
    u8  u8_rsvd; //unused

//  union{
            iso_test_receive_infor_t recMode;
            iso_test_transmit_infor_t tranMode;
//  };

}iso_test_param_t;

extern iso_test_param_t gIsoTestPara[];

#define DATA_PATH_INPUT_FLAG                BIT(0)   // BIT(Data_Dir_Input)
#define DATA_PATH_OUTPUT_FLAG               BIT(1)   // BIT(Data_Dir_Output)


extern  hci_cmd_callback_t              ll_cis_cmd_task_cb;
extern  hci_cmd_callback_t              ll_bis_cmd_task_cb;



/**
 * @brief      This function is used to enter ISO test mode, only for testing purposes.
 * @param      *pCmdParam
 * @return     ble_sts_t
 */
ble_sts_t blc_hci_le_iso_transmit_test(hci_le_isoTestCmdParams_t *, hci_le_isoTestRetParams_t *);
ble_sts_t blc_hci_le_iso_receive_test(hci_le_isoTestCmdParams_t *, hci_le_isoTestRetParams_t *);


ble_sts_t blc_hci_le_iso_read_test_count_cmd(hci_le_isoReadTestCountsCmdParams_t *, hci_le_isoRxTestStatusParam_t *);
ble_sts_t blc_hci_le_iso_test_end_cmd(hci_le_isoTestEndCmdParams_t* pCmd, hci_le_isoTestEndStatusParam_t *pRetParam);

int blt_iso_test_transmit_mainloop(iso_test_param_t *isoTest, u32 interval,u16 max_sdu, sdu_packet_t *sdu, u8 frame);
int blt_iso_test_receive_mainloop(sdu_packet_t *sdu, iso_test_param_t *pBisTestParam, u16 max_sdu, u8 frame);

iso_test_param_t* blt_iso_test_allocateCtrlBlock(void);

/******************************* iso_test end ********************************************************************/




#endif /* ISO_STACK_H_ */
