/********************************************************************************************************
 * @file    cis_stack.h
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
#ifndef STACK_BLE_CONTROLLER_LL_ISO_CIS_STACK_H_
#define STACK_BLE_CONTROLLER_LL_ISO_CIS_STACK_H_

#include "iso_stack.h"
#include "stack/ble/ble_common.h"
#include "stack/ble/ble_stack.h"
#include "stack/ble/ble_format.h"
#include "stack/ble/controller/ll/acl_conn/acl_stack.h"
#include "stack/ble/controller/csa/csa_stack.h"


/* temporary solution for assert error "CIS_CONN_PARAM_LENGTH == sizeof(ll_cis_conn_t)" */
#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    #undef          CIS_CONN_PARAM_LENGTH
    #define         CIS_CONN_PARAM_LENGTH                   (396 + 36)
#endif



#ifndef         DBG_CREATE_FF0D_ERROR
#define         DBG_CREATE_FF0D_ERROR                       0
#endif


#ifndef         LL_CIG_MST_NUM_MAX
#define         LL_CIG_MST_NUM_MAX                          (TSKNUM_CIG_MST) //Number of CIG MST tasks supported
#endif

#ifndef         LL_CIG_SLV_NUM_MAX
#define         LL_CIG_SLV_NUM_MAX                          (TSKNUM_CIG_SLV) //Number of CIG SLV tasks supported
#endif

#ifndef         CIS_IN_CIGM_NUM_MAX
#define         CIS_IN_CIGM_NUM_MAX                         4 //The number of CIS supported by each CIG Master
#endif

#ifndef         LL_CIS_IN_PER_CIG_SLV_NUM_MAX
#define         LL_CIS_IN_PER_CIG_SLV_NUM_MAX               4 //The number of CIS supported by each CIG Slv
#endif

#ifndef         LL_SE_IN_PER_CIS_NUM_MAX
#define         LL_SE_IN_PER_CIS_NUM_MAX                    8 //The number of SEs supported by each CIS
#endif

#ifndef         LL_SE_IN_PER_CIG_MST_NUM_MAX
#define         LL_SE_IN_PER_CIG_MST_NUM_MAX                (CIS_IN_CIGM_NUM_MAX * LL_SE_IN_PER_CIS_NUM_MAX)
#endif

//Not used now [cig_slv]
#ifndef         LL_SE_IN_PER_CIG_SLV_NUM_MAX
#define         LL_SE_IN_PER_CIG_SLV_NUM_MAX                (LL_CIS_IN_PER_CIG_SLV_NUM_MAX * LL_SE_IN_PER_CIS_NUM_MAX)
#endif

#ifndef         LL_BN_IN_PER_CIS_NUM_MAX
#define         LL_BN_IN_PER_CIS_NUM_MAX                    16    //core spec: BN shall be in the range 0 to 15  //must be align 4 fanqh
#endif


#define         CIG_SLAVE_NSE_NUM_MAX                       32




/* LL/CIS/CEN/BV-26-C, EBQ 8280 version new check error
 *  
    Expected: greater than or equal to 27.0d
    Actual: 22
    SDU is 16, old code calculate PDU 24, EBQ think it error
    
    but we think EBQ misunderstand it, so we use a temp macro
 */
#ifndef EBQ_8280_CHECK_PDU_EN
#define EBQ_8280_CHECK_PDU_EN                               0
#endif



/******************************* cis start ******************************************************************/
#define     CIS_PDU_MAX                     251  //Vol 4, Part E, 7.8.98 LE Set CIG Parameters Test command, Max_PDU_M_To_S[i]:




#define     CIG_SLOT_BUILD_MSK              BIT(7)
#define     CIS_SLOT_BUILD_MSK              BIT(6)


/* CIS FLOW */
#define     CIS_FLOW_IDLE                   0

#define     CIS_FLOW_SLAVE_REQ_HOST         BIT(0)
#define     CIS_FLOW_SLAVE_SEND_CIS_RSP     BIT(1)
#define     CIS_FLOW_SLAVE_REJECT_CIS_REQ   BIT(2)
#define     CIS_FLOW_SLAVE_WAIT_CIS_IND     BIT(3)

#define     CIS_FLOW_MASTER_START_NEW_CIS   BIT(0)
#define     CIS_FLOW_MASTER_SEND_CIS_REQ    BIT(1)
#define     CIS_FLOW_MASTER_SEND_CIS_IND    BIT(2)
#define     CIS_FLOW_MASTER_REJECT_CIS_RSP  BIT(3)

#define     CIS_FLOW_CIS_SYNCING            BIT(4)
#define     CIS_FLOW_CIS_SYNC_SUCCESS       BIT(5)
#define     CIS_FLOW_CIS_SYNC_FAIL          BIT(6)

#define     CIS_FLOW_CLEAR_ESTABLISH_STATUS BIT(7)  //use "cisFlow Flag" to do something else




/* CREATE STATE */
//#define   CREATE_STATE_START              BIT(3)
#define     CREATE_STATE_SEND_IND           BIT(4)
#define     CREATE_STATE_SET_1STAP          BIT(5)
//#define   CREATE_STATE_ESTABLISH          BIT(6)



#define     CIS_ROLE_MASTER                 1
#define     CIS_ROLE_SLAVE                  0
#define     CIS_ID_INVALID                  0xFF


#define     BLT_CIS_HANDLE                  BIT(5)  //CIS connection handle, BIT(5), 0x0020
#define     BLT_CIS_CONN_HANDLE             BLT_CIS_HANDLE
#define     BLT_CIS_IDX_MSK                 0x0F  //15 most, though standard 31 max, 15 is enough in our SDK
#define     SLOT_BUILD_IDX_MSK              0x0F



#define     CIS_REQ_ACCEPT                  BIT(0)
#define     CIS_REQ_REJECT                  BIT(1)




//ISO CIS TX Data Buffer struct begin
typedef struct {
    iso_evtcnt_t    cis_pdu_number;
    iso_evtcnt_t    eventCnt;

    u16     offset;
    u8      RFU[2]; //4B align
    rf_packet_ll_data_t isoTxPdu;
}cis_tx_pdu_t;

typedef struct {
    u16     fifo_size;
    u8      fifo_num;
    u8      fifo_mask;

    u8*     cis_tx_pdu;
}cis_tx_pdu_fifo_t;



typedef struct {
    u16     fifo_size;
    u8      fifo_num;
    u8      wptr;
//  u8      rptr;       //not used now, use another CIS RX event FIFO to control read flow

    rf_packet_ll_data_t *isoRxPdu;
}iso_rx_pdu_fifo_t;





typedef struct {
    u32     size;
    u8      num;
    u8      mask;
    u8      wptr;
    u8      rptr;
    u8*     p;
}cis_rxEvt_fifo_t;


extern cis_rxEvt_fifo_t                 bltCisRxEvt;


#define NOC_ACK_MAXCNT      16
#define NOC_ACK_MASK        (NOC_ACK_MAXCNT - 1)


//CIS parameters
_attribute_aligned_(4)
typedef struct
{

// 0x00  //
    u8      cis_index;
    u8      cis_connHandle;     /* Used to identify a CIS */
    u8      cisRole;         //1:master CIS role; 0:Slave CIS role
    u8      cis_ID;             //correct range: 0x00 ~ 0xEF


    u8      link_cigid;         //CIS linked master or slave CIG ID
    u8      clink_cig_idx;      //CIS connection link CIG index: CIG master index or CIG slave index
    u8      link_acl_index;
    u8      link_acl_handle;    //link ACL_conn_handle, associated ACL connection handle, for CIS master: 0x80/0x81...; for CIS slave 0x40,0x41....
                                //u8 is enough, due to our special design
                                //reason for not use link_acl_conn_idx: 0 has confusing meanings, one for master 0x80, one for data clear
                                //TODO, when CIS destroy, clear it. so acl_handle is used to judge whether a ACL connection is  associated


    u8      cis_occupied;       //main_loop variable
    u8      cisFlowFlg;
    u8      cis_reject_reason;
    u8      cis_receive_packet;

    u8      nse;
    u8      phy_ms;             //symmetrical PHY: phy m2s & s2m,    le_phy_prefer_type_t: BIT(0) BIT(1) BIT(2)
    u8      curCisPhy;          // le_phy_type_t: 0x01/0x02/0x03. do not support asymmetric PHY, so one variable is enough
    u8      cis_frame;      /* Framed indicates whether the CIS carries framed or unframed data */

    conn_trmt_t     cis_termin_union;   //4B

    u8      updateMap_cmd;
    u8      conState;            // IRQ variable, 0/Conn_Complete/Conn_Establish   different name from ACL
    u8      cis_established;     // main_loop variable, important status variable: set when CIS establish, clear when CIS disconnect and HCI reset
    u8      discon_evt;


    //CIS TX && RX buffer ctrl
    u8      cisPduTxFifoWptr;
    u8      cisPduTxFifoRptr;
    u8      cisSubEventCnt;     //Internal sub-event accumulation, the first subeventcnt is 1, subevent <=nse.Need to be cleared before each CIG event is triggered
    u8      closeIsoEvent;      /* 1 means close current CIS event */ // not use now


    u8      locl_sn;        //different name from ACL local SN
    u8      locl_nesn;      //different name from ACL local NESN
    u8      txNullPduFlag;  // risk, set in IRQ, but it is used in mainloop
    u8      cie_flag;
    u8      bn_loca;
    u8      bn_peer;
    u8      ft_loca;
    u8      ft_peer;

    u8      nse_div_bnLoca;  // NSE/local_bn
    u8      nse_div_bnPeer;  // NSE/peer_bn
    /*  mainLoop variable,for master only. set when host create CIS,
        clear when: 1. first allocate CIS; 2. create CIS cancel with HCI disconnect */
    u8      createCmd;
    u8      createStatus; // mainLoop variable, for master only

    u8      cis_rxSduStatus;
    u8      lossFlag;
    u8      rx_numSdu2Pdu;  //at receiver1SDU PDU
    u8      tx_numSdu2Pdu;


    u8      cisSduIn_wptr;  //CIS SDU in FIFO WPTR(receive from HCI Data)
    u8      cisSduIn_rptr;  //CIS SDU in FIFO RPTR(receive from hci data)
    u8      cisSduOut_wptr; //CIS SDU out FIFO WPTR(report to HCI)
    u8      cisSduOut_rptr; //CIS SDU out FIFO RPTR(report to HCI)



    u8      tx_first_flag;  //todo: confirm with QingHua
    u8      cis_rx_stream_start;
    u8      cis_tx_stream_start;
    u8      dpId;// 0:HCI, 1--0xfe: vendor spec, 0xff: disable. TODO: question: should ID same for in and out data path both used ?

    /* data Path Setup
     * refer HCI/CIS/BI-10-C, master not clear when connect_common or establish, but clear when allocate
     * refer HCI/CIS/BI-06-C, set after host ACK CisReq  event
     */
    u8      cis_dapth_setup;  //CIS data path setup
    u8      cisSchedFlg;        /* CIS has begun to participate in the scheduling */   // change to IRQ variable by SiHui
    u8      cis_jump_num;
    s8      rssi; //IRQ variable, in irq update it's value,

    u8      align_with_acl;
    u8      get_rxTimestamp;   //remove later
    u8      cism_alloc_position;  //for CIS master
    u8      offset_us_const;  //offsetMax should equal to offsetMin


    //power control
    u8      peer_coded_phy_ci; //IRQ variable, only valid in LE coded PHY mode, cis used
    u8      rfPwrLvlIdx;
    s8      cisUsedTxPwr;       /* pcl used, current used tx pwr */
    u8      ack_CisReq;         //for slave only, host ACK CIS Request
//  u8      u8_rsvd3;

    u8      max_pdu_loca;   //u8 is enough, MAX value 251
    u8      max_pdu_peer;   //u8 is enough, MAX value 251
    u8      sdu_split_pdu_loca;
    u8      sdu_split_pdu_peer;

    u16     rx_lastPktSeqNum;   //HCI packet sequence number
    u16     tx_lastPktSeqNum;       //HCI packet sequence number
    u16     align_mul_coeff;
    u16     subInter_idle_us;   //now ony for slave
    u16     TMSS;
    u8      local_cie;
    u8      peer_cie;

    u16     max_sdu_loca;
    u16     max_sdu_peer;


    u32     sdu_int_loca_us;    //local SDU interval US
    u32     sdu_int_peer_us;    //peer  SDU interval  US

    u32     cig_ap_distan_us;    //timing distance between current CIS anchor point and CIG anchor point
    u32     cis_maxPossible_us;  /* maximum possible time, The maximum possible time for a CIS event equals 
                                                            Sub_Interval + MPTM + T_IFS + MPTS for that CIS */
    int     cis_idle_us;
    u32     cig_next_tick;
    u32     updateMap_tick;

    u32     sub_intvl_us;       /* Sub_Interval is the time between start of two consecutive subevents of a CIS*/
    u32     sub_intvl_tick;
    u32     se_length_us;       /* SE_Length is the maximum length of a subevent, SE_Length = MPTM + T_IFS + MPTS + T_MSS */
    u32     se_length_tick;
    u32     cis_task_us;

    u32     iso_intvl_us;
    u32     iso_intvl_tick;     /* Time between consecutive CIS anchor points */;
    u32     MPTM_TIFS_MPTS;     //unit: uS


    u32     own_cisOffsetMin_us;
    u32     own_cisOffsetMax_us;
    u32     peer_cisOffsetMin_us;
    u32     peer_cisOffsetMax_us;
    u32     cisOffset_use;

    u32     cig_sync_delay;
    u32     cis_sync_delay;
    u32     transLaty_m2s;      /* unit: 1us */
    u32     transLaty_s2m;      /* unit: 1us */
    u32     cis_tick;
    u32     cis_timeout;        //optimize later: use ACL timeout
    u32     cis_expect_tick;
    u32     cis_trigger_tick;   //CIS trigger tick, for CIS master
    u32     cis1stSchedAPbSlot;
    u32     cis_1st_rx_tick;

    u8 *    cis_sduInBuf;
    u8 *    cis_sduOutBuf;
    u8 *    cis_txPduBuf;

#if(WALKAROUND_ISO_TIMESTAMP_EN)
    u32     cis_ap_tick;
#endif

//  u32     rx_lastSduTimestamp;    //HCI timestamp, SDU synchronization reference
//  u32     tx_lastSduTimestamp;    //HCI timestamp, SDU synchronization reference



    // ISO link quality record (Ref Core5.2 | Vol 4,Part E, Page2741)
//  u32     tx_unAckedPktsCntr;
//  u32     tx_flushedPktsCntr;
//  u32     tx_lastSubEvtPktsCntr;
//  u32     retransmittedPktsCntr;
//  u32     crcErrPktsCntr;
//  u32     rxUnrcvedPktsCntr;
//  u32     duplicatePktsCntr;

    //Channel parameters
    u8      u8_rsvd2;
    u8      insertEmtpy_flg;
    u16     chnIdentifier;      //BISes use the same channel map as BIG, only ChnId different. Put outside

    u32     cisAccessAddr;      //AA shall be set to the Access Address of the CIS
    u32     cisCrcInit;         //Same as the ACL's CRC_Init value
    struct csa2_param cis_chnParam;

    // risk, set in IRQ, but it is used in mainloop
    volatile iso_evtcnt_t cisSendPldNum;  //When CIS is first established, it is initialized to 0, and it is used locally to record the currently sent CisPayloadNum. Every time a CIS Data PDU is sent or FT is incremented
    volatile iso_evtcnt_t cisRcvdPldNum;  //When CIS is established, it is initialized to 0. It is used locally to record the currently received CisPayloadNum. Every time a CIS Data PDU is received or the FT is incremented
                             /// L && F is used to determine the peer_CisPayloadNum corresponding to the received peer CIS data PDU ///
    iso_evtcnt_t        tx_lastpldNum;
    iso_evtcnt_t        cisEvtCnt_initJump;
    iso_evtcnt_t        cisEventCnt;        /* TODO: u64? Both the master and slave shall have a 39-bit counter cisEventCounter */
    //iso_evtcnt_t      lastAckedPldNum; //L be the last PDU that you have acknowledged        not used now
    //iso_evtcnt_t      lastPassFtPldNum;//F be the last PDU that has passed its flush point   not used now





    // encryption concerned
    ble_crypt_para_t crypt;//48 bytes



#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    u8      nocRsvd1;
    u8      nocRsvd2;
    u8      cisNocpWptr;
    u8      cisNocpRptr;

    u8      cisNocTxWptr[NOC_ACK_MAXCNT];
    u8      cisNocPktCnt[NOC_ACK_MAXCNT];
#endif



#if (LL_FEATURE_ENABLE_CONNECTIONLESS_ISO)
    iso_test_param_t    *pCisTestParam;
#endif

#if(CIS_TSK_JUMP_ADD_PDU)
    u8 ftPt_peer[LL_BN_IN_PER_CIS_NUM_MAX];
    u8 ftPt_loca[LL_BN_IN_PER_CIS_NUM_MAX];
#endif

} ll_cis_conn_t;

_attribute_aligned_(4)
typedef struct
{
    u8      maxNum_cig_mst;
    u8      curNum_cig_mst;
    u8      maxNum_cig_slv;
    u8      maxNum_cisConn;

    u8      maxNum_cisMaster;
    u8      curNum_cisMaster;   //not used now
    u8      maxNum_cisSlave;
    u8      curNum_cisSlave;   //main_loop variable

    u8      cisFlow_pending;    //u8, support 8 CIS most now, mainLoop variable
    u8      cisFlow_idx;        //mainLoop variable
}ll_cis_mng_t;

extern ll_cis_conn_t        *global_pCisConn;
extern ll_cis_conn_t        *blt_pCisConn;
extern ll_cis_mng_t         bltCisMng;

extern  st_ll_conn_t*       blt_pAclConn;





_attribute_aligned_(4)
typedef struct {
    u8      blt_cis_sel;
    u8      cis_rx_num;
    u8      cis_rx_dma_size;
    u8      u8_rsvd1[3];

    /* CIS master & slave share this variable, so we need forbidden CIS master & slave creating at same time,
     * just like ACL master & slave */
    u16     cis_1st_anchor_evtCnt;  //ACL event counter
    /*
     * if ISO data from app don't present in the SDU_interval,
     * 0(default): supplement Empty PDU
     * 1: PDUsupplement empty in the last bn subEvent and the other subEvent padding with NULL PDU
     * 2: supplement NULL PDU
     */
    u16     cis_supplement_strategy;
    u16         rsvd16;

    u32     cis_1st_anchor_tick;
    u32     cis_sduDataNum;



    u32     cis_rx_dma_buff;    //TODO: when CIS interrupt by other task, need modify
} cis_conn_para_t;  //CIS connection parameters

extern  _attribute_aligned_(4)  cis_conn_para_t     cisConn_param;



_attribute_aligned_(4)
typedef struct {
    u8      blt_cigMstSel;
    u8      rsvd[3];
    u32     ctx_trigger_tick;
} cis_mas_para_t;  //CIS master parameters

extern  _attribute_aligned_(4)  cis_mas_para_t      cisMas_param;



_attribute_aligned_(4)
typedef struct {
    u8      blt_cisSlvSel;
    u8      rsvd[3];

    u16     cisReq_aclEvtCnt;


    /* Vol 4, Part E, 6.7 CONNECTION ACCEPT TIMEOUT
     * Range: 0x0001 to 0xB540
       Default: 0x1F40
       Mandatory Range: 0x00A0 to 0xB540
       Time = N * 0.625 ms
       Time Range: 0.625 ms to 29 s
       Time Default:
       BR/EDR 5 s   */
    u32     connAcpt_to_us;     //Connection Accept Timeout, unit uS. 0.625 ~ 29S,  default 5S.
    u32     connAcpt_begin_tick;
} cis_slv_para_t;  //CIS slave parameters

extern  _attribute_aligned_(4)  cis_slv_para_t      cisSlv_param;

extern blt_ll_pushIsoDataFun  blt_ll_pushCisDataFun;



extern rf_packet_ll_data_t  gCisNullPdu;
extern rf_packet_ll_data_t* pCurrCisPdu;

extern cis_tx_pdu_fifo_t        bltCisPduTxfifo;
extern iso_rx_pdu_fifo_t        bltCisPduRxfifo;

void        blt_cis_scheduler_task(void);
int         blt_cis_conn_interrupt_task (int, void *p);
int         blt_cis_conn_mainloop_task (int, void *p);
int         blt_cis_cmd_process_task (int, void*, void*);
int         blt_cis_update_chn_map (int, void *);


int         blt_ll_cis_conn_mainloop(void);


void        blt_cis_mark_numOfcmpEvt_status(ll_cis_conn_t *, int);
int         blt_cis_tx_loop(ll_cis_conn_t *);
int         blt_cis_release_iso_data_buffer(ll_cis_conn_t *);
int         blt_cis_rx_event_loop(void);
void        blt_cis_sdu_out_loop(ll_cis_conn_t *);

init_err_t  blt_ll_checkCisInit(void);
void        blt_ll_reset_cis_conn(void);

void        irq_cis_tx(void);
int         irq_cis_rx(void);
int         blt_cis_connect_common(st_ll_conn_t*, ll_cis_conn_t *);
int         blt_ll_cis_start_common_1(ll_cis_conn_t *);
int         blt_ll_cis_start_common_2(ll_cis_conn_t *);


void        blt_cis_establish_common(ll_cis_conn_t *);
u8*         blt_ll_cis_prepare_data_common(ll_cis_conn_t *);
void        blt_ll_cis_ft_event_jump(ll_cis_conn_t *, u32);
void        blt_ll_cis_ft_subevent_commm(ll_cis_conn_t *, u8 jump);
int         blt_cis_pushRxEvtInfoToFifo(ll_cis_conn_t *, rf_packet_ll_data_t*, u8 jump_num);

void        blt_ll_cis_start_jump (ll_cis_conn_t *, int);


ble_sts_t   blt_cisgrp_post_common(void);
ble_sts_t   blt_ll_cis_post_common(ll_cis_conn_t *);
void        blt_cis_post_common_2(ll_cis_conn_t *);

void        blt_cis_test_process(ll_cis_conn_t *);
int         blt_ll_cis_procCisConnectionEvent(u16, ll_cis_conn_t *);


ble_sts_t   blt_ll_cis_disconnect(ll_cis_conn_t *, u8);

void        blt_cis_calculateInterval(ll_cis_conn_t *, u16, u16);


bool        blt_ll_cis_encryptPdu(ll_cis_conn_t *, cis_tx_pdu_t *);

ll_cis_conn_t* blt_isCisEstablished_by_handle(u16);
ll_cis_conn_t* blt_isCisAllocated_by_handle(u16);



/**
 * @brief      for user to initialize CIS RX EVT FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size, size must be 4*n
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_initCisRxEvtFifo(u8 *pRxbuf, int fifo_size, int fifo_number);



ble_sts_t   blc_hci_le_rejectCisReq(hci_le_rejectCisReq_cmdParams_t *pCmdParam, hci_le_rejectCisReq_retParams_t *pRetParam);



ble_sts_t   blc_hci_le_pushCisData(iso_data_packet_t *);



ble_sts_t   blc_ll_cis_iso_transmit_test_cmd(u16 connHandle, u8 type);
ble_sts_t   blc_ll_cis_iso_receive_mode(u16 connHandle, itest_payload_type_t pdu_type);
ble_sts_t   blc_ll_cis_iso_read_test_count_cmd(u16 connHandle, hci_le_isoRxTestStatusParam_t *pRetParam);



ble_sts_t   blc_hci_le_setupCisDataPath(hci_le_setupIsoDataPath_cmdParam_t *pCmdPara, hci_le_setupIsoDataPath_retParam_t *pRetParam);

ble_sts_t   blc_hci_le_removeCisDataPath(hci_le_rmvIsoDataPath_cmdParam_t *pCmdPara, hci_le_rmvIsoDataPath_retParam_t *pRetParam);



/**
 * @brief      This function is  used to identify and create the isochronous data path between the Host and the Controller for a CIS,
 *             CIS  configuration, or BIS identified by the Connection_Handle parameter. can also be used to configure a codec for each data path.
 * @param[in]  cis_handle - CIS connection handle.
 * @param[in]  dir - Data_Path_Direction
 * @param[in]  id - Data_Path_ID.
 * @param[in]  cid_assignNum - Assigned Numbers for Coding Format
 * @param[in]  cidcompId - Company ID
 * @param[in]  cid_vendorDef - Vendor-defined codec ID.
 * @param[in]  control_dly - Controller delay in microseconds
 * @param[in]  codec_cfg_len - Length of codec configuration
 * @param[in]  codec_cfg1 - Codec-specific configuration data 1
 * @param[in]  codec_cfg2 - Codec-specific configuration data 2
 * @param[in]  codec_cfg3 - Codec-specific configuration data 3
 * @param[in]  codec_cfg4 - Codec-specific configuration data 4
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_setupCisDataPath(u16 cis_handle, dat_path_dir_t dir, dat_path_id_t id, u8 cid_assignNum, u16 cidcompId, u16 cid_vendorDef,
                                     u32 control_dly, u8 codec_cfg_len,      u8 codec_cfg1,    u8 codec_cfg2,     u8 codec_cfg3, u8 codec_cfg4);



/**
 * @brief      This function is used to remove the input and/or output data path(s) associated with a CIS, CIS configuration, or BIS
               identified by the Connection_Handle parameter.
 * @param[in]  cisHandle - Connection handle of the CIS or BIS.
 * @param[in]  dir_mask - Data_Path_Direction
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_removeCisDataPath(u16 cis_handle, dp_dir_msk_t dir_mask);

void blc_isoal_clearCisSdu(u16 cis_connHandle);


/******************************* cis end ********************************************************************/






















/******************************* cis_master start ******************************************************************/
//ISO interval shall be a multiple of 1.25mS in the range of 5ms to 4S

#if(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define             CIG_MST_FIFONUM             16  //16*5 = 80 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_120MS)
    #define             CIG_MST_FIFONUM             24  //24*5 = 120 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_160MS)
    #define             CIG_MST_FIFONUM             32  //32*5 = 160 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_240MS)
    #define             CIG_MST_FIFONUM             48  //48*5 = 240 mS
#else
    #error "unsupported CIG MST FIFONUM!!!"
#endif






#define         CIG_TASK_NO_CHANGE                          0
#define         CIG_TASK_DURATION_CHANGE                    1
#define         CIG_TASK_FIRST_AP_CHANGE                    2

#define         TYPE_ADD_CIS                                1
#define         TYPE_DELETE_CIS                             2


//  CIG parameters
_attribute_aligned_(4)
typedef struct
{
// 0x00  //
    u8      cig_master_index;
    u8      cig_ID_mas;             // CIG ID for master
    u8      cism_packing;           // 'cis' but not 'cig'
    u8      cig_frame;


    u8      ft_m2s;                     /* The flush timeout in multiples of ISO_Interval for each payload sent from the master to slave */
    u8      ft_s2m;                     /* The flush timeout in multiples of ISO_Interval for each payload sent from the slave to master */
    u8      sca;                    /* Slaves_Clock_Accuracy */   //type same as u8
    u8      cig_test_cmd;           //not used now

    u8      cis_delete;
    u8      config_state;  //HCI/CIS/BI-13-C & HCI/CIS/BV-02-C
    s8      bSlot_ap_offset; //must be signed
    u8      offset_cus_en;

    u8      offset_cus_aclcIdx;
    u8      ull_used;       //only for ULL
    u8      u8_rsvd1[2];

    u8      cism_total_se_num;   //The cumulative number of cis arrangement mapping tables from the beginning of the idx0 to the last valid se (se may belong to different cis)
    u8      cism_se_en_num; //not used now
    u8      cism_map_next_taskIdx;//Starting idx of cis arrangement mappings
    u8      cigm_se_idx;


    u8      cigm_finish;
    u8      first_ap_update;
    u8      last_alloc_position;    // mainLoop Variable
    u8      cism_create_pending_msk;

    u8      cism_create_flg;            // mainLoop Variable, not used now
    u8      cism_create_msk;            // mainLoop Variable
    u8      cism_set_cnt;           // mainLoop Variable
    u8      cism_set_msk;           // mainLoop Variable        //support 8 CIS max with "u8"

    u8      cism_estab_cnt;         // mainLoop Variable
    u8      cism_estab_msk;         // mainLoop Variable        //support 8 CIS max with "u8"
    u8      cism_task_cnt;          // IRQ variable
    u8      cism_task_msk;          // IRQ variable             //support 8 CIS max with "u8"

    u8      cism_set_order[CIS_IN_CIGM_NUM_MAX];
    u8      cis_alloc_exist[CIS_IN_CIGM_NUM_MAX];       // IRQ Variable
    u8      cis_alloc_markIdx[CIS_IN_CIGM_NUM_MAX];     // IRQ Variable
    u32     cis_ap_distan_mark_us[CIS_IN_CIGM_NUM_MAX]; // mainLoop Variable


    ll_cis_conn_t *pcisPos_1st;         // mainLoop Variable
    ll_cis_conn_t *pcisPos_earliest;    // IRQ Variable
    ll_cis_conn_t *pcisPos_latest;      // IRQ Variable


    u16     cism_isoIntvl;          //unit: 1.25 mS
    u16     cism_bSlotInterval;     //unit: 625 uS
    u16     max_trans_lat_m2s;      /* Maximum time, in milliseconds, for an SDU to be transported from the master Controller to slave Controller */
    u16     max_trans_lat_s2m;      /* Maximum time, in milliseconds, for an SDU to be transported from the slave Controller to master Controller */
    u16     cism_sSlotAllocNum;
    u16     cism_sSlotDuration;     // u8 is enough
    u16     offset_range_us;
    u16     offset_cus_us;

    u32     cism_sSlotInterval;     //unit: 19.5 uS, 65536*20uS = 1.3S, interval maximum value is 4S, so need change to u32
    u32     se_length_total_us;
    u32     task_total_us;
    u32     cism_intvl_us;          //not used now
    u32     cigm_sync_delay;        // unit: uS
    u32     sdu_int_loca;           /* The interval, in microseconds, of periodic SDUs */
    u32     sdu_int_peer;           /* The interval, in microseconds, of periodic SDUs */

    s32     sSlot_mark_cigm;
    u32     bSlot_mark_cigm;

    u32     cigm_expect_tick;
    u32     cigm_trigger_tick;
    u32     cig_ref_point; //CIG reference point
    u32     tx_trigger_diff_tick;


    u32     cism_arrgmtMap_en_msk;
    u8      cism_arrgmtMap_cisIdx[LL_SE_IN_PER_CIG_MST_NUM_MAX];  //CIS index
    u8      cism_arrgmtMap_seIdx[LL_SE_IN_PER_CIG_MST_NUM_MAX];   //sub_event index


    u8      u8_rsvd22;
    u8      u8_rsvd33;
    u8      cigTsk_wptr;
    u8      cigTsk_rptr;

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  cigTsk_fifo[CIG_MST_FIFONUM];



} ll_cig_mst_t;

extern ll_cig_mst_t     *global_pCigMst;
extern ll_cig_mst_t     *latest_pCigMst;
extern ll_cig_mst_t     *blt_pCigMst;







int         blt_ll_searchExistingCigId(u8 cur_cigId);
int         blt_ll_AllocateNewCigId(u8 cur_cigId);

int         blt_cig_mst_interrupt_task (int flag, void *p);
int         blt_cig_mst_mainloop_task (int flag, void *p);

int         blt_ctx_start(void);
int         blt_ctx_post(ll_cis_conn_t *);
int         blt_cig_mst_start(int slotTask_idx);
int         blt_ll_cis_master_mainloop(void);

int         blt_cig_mst_calculate_timing(ll_cig_mst_t *, int);
void        blt_cig_mst_set_first_anchor_point(void);
int         blt_ll_buildCigSchedulerLinklist(void);

int         blt_ll_cigMstMainloop(void);

void        blt_ll_reset_cig_mst(void);
bool        blt_ll_sendCisReq(st_ll_conn_t *, ll_cis_conn_t *, ll_cig_mst_t *);
bool        blt_ll_sendCisInd(st_ll_conn_t *, ll_cis_conn_t *, ll_cig_mst_t *);
bool        blt_ll_rejectCisRsp(ll_cis_conn_t *, ll_cig_mst_t *);
int         blt_ll_cis_master_cis_establish(ll_cis_conn_t *, ll_cig_mst_t *, ble_sts_t);

ble_sts_t   blt_ll_cis_master_control_pdu_process(st_ll_conn_t*, u8, u8 *);




ble_sts_t   blc_ll_read_cis_tx_sync(u16 cisHandle, hci_le_readIsoTxSync_retParam_t *pRetParam);


//typedef struct
//{
//  u16 cis_handle;
//}ll_CreateCisCancel_cmdParam_t;

ble_sts_t   blt_ll_createCisCancel(ll_cis_conn_t *);


/**
 * @brief
 * @param
 * @param
 * @return     ble_sts_t
 */
ble_sts_t   blc_hci_le_setCigParamsTest(hci_le_setCigParamTest_cmdParam_t* pCmdParam, hci_le_setCigParam_retParam_t* pRetParam);



/**
 * @brief      remove the CIG identified by CIG_ID.
 * @param[in]  cigId - Identifier of a CIG
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_ll_removeCig(u8 cigId);
/******************************* cis_master end ********************************************************************/

















/******************************* cis_slave start ******************************************************************/
#if(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define             CIG_SLV_FIFONUM             16  //16*5 = 80 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_120MS)
    #define             CIG_SLV_FIFONUM             24  //24*5 = 120 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_160MS)
    #define             CIG_SLV_FIFONUM             32  //32*5 = 160 mS
#elif(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_240MS)
    #define             CIG_SLV_FIFONUM             48  //48*5 = 240 mS
#else
    #error "unsupported CIG SLV FIFONUM!!!"
#endif


#define         CIS_REQ_ACCEPT_TIMEOUT          4000000 //TODO: 4s timeout unit:us




#if FAST_SETTLE
    #define         CIGSLV_EARLY_SET_US                             100
    #define         CIGSLV_EARLY_SET_TICK                           (CIGSLV_EARLY_SET_US*SYSTEM_TIMER_TICK_1US)

    #define         CRX_EARLY_SET_US                                90
    #define         CRX_EARLY_SET_TICK                              (CRX_EARLY_SET_US * SYSTEM_TIMER_TICK_1US)
#else // refer ACL peripheral: 7 + (3+1+3)

    #define         CIGSLV_EARLY_SET_US                             195  // 10*(625/32) = 195.3uS, 10=7+3

    #if (SYSTICK_NUM_PER_US == 16)
        #define         CIGSLV_EARLY_SET_TICK                       3125 // 312.5*10
    #elif(SYSTICK_NUM_PER_US == 24)
        #define         CIGSLV_EARLY_SET_TICK                       4688 // 468.75*10=4687.5
    #else
        #error "SYSTICK NUM PER US other than 16/24, add SYSTICK NUM PER US here"
    #endif

    /* can not exceed 150uS choose 120uS, also need consider TX IRQ delay and running time */
    #define         CRX_EARLY_SET_US                                120
    #define         CRX_EARLY_SET_TICK                              (CRX_EARLY_SET_US * SYSTEM_TIMER_TICK_1US)
#endif







_attribute_aligned_(4)
typedef struct cigs_sub_task_t{
    u8      cis_c_idx; //a name different form other
    u8      subevt_idx;
    u8      se_enable;
    u8      rsvd;
}cigs_sub_task_t;


_attribute_aligned_(4)
typedef struct {
    u8      cig_slave_index;
    u8      link_acl_slave_idx;
    u8      link_acl_slave_handle;  //u8 is enough in our design
    u8      cig_slv_occupied;

    u8      cig_ID_slv;         //CIG ID for slave
    u8      crx_early_set_sSlot_num;
    u8      crx_half_margin_sSlot_num;
    u8      u8_rsvd0;

    u8      ciss_alloc_msk;     //main_loop variable, set when receive LL_CIS_IND
    u8      ciss_alloc_count;   //main_loop variable
    u8      ciss_task_cnt;      // IRQ variable
    u8      ciss_task_msk;      // IRQ variable, support 8 CIS max with "u8"

    u8      packing_style;
    u8      cigSlvPacking;
    u8      cigs_working;
    u8      cigs_se_idx;


    u8      cigs_finish;
    u8      cigs_sSlotWiden;
    s8      cigs_sSlotOffset;
    u8      u8_rsvd1[1];

    u8      ciss_total_se_num;
    u8      ciss_se_en_num;
    u8      ciss_map_cur_taskIdx;
    u8      ciss_map_next_taskIdx;//Starting index of CIS arrangement mappings


    u16     ciss_isoIntvl;      //unit: 1.25 mS
    u16     ciss_bSlotInterval; //unit: 625 uS
    u32     ciss_sSlotInterval; //unit: 19.5 uS, 65536*20uS = 1.3S, interval maximum value is 4S, so need change to u32
    u16     ciss_sSlotAllocNum;
    u16     ciss_sSlotDuration;
    u16     ciss_widen_us;
    u16     u16_rsvd;

    u32     offset_diff_us;
    u32     cisOffset_us;
    u32     cisOffset_tick;     /* CIS_Offset shall be set to the time, in microseconds */
    u32     cigs_start_tick;
    u32     cigs_expect_tick;
    u32     cigs_expectTick_2;  //optimize later, temp solve CIS/PER/BV-45  CIS_WINDOW_WIDENING_FOR_BIG_PPM
    u32     cigs_1st_rx_tick;
    u32     cigs_mark_rx_tick;
    s32     sSlot_mark_cigs;
    u32     bSlot_mark_cigs;


    u32     ciss_arrgmtMap_en_msk;  //enable mask
    u8      ciss_arrgmtMap_cisIdx[LL_SE_IN_PER_CIG_SLV_NUM_MAX];  //CIS index
    u8      ciss_arrgmtMap_seIdx[LL_SE_IN_PER_CIG_SLV_NUM_MAX];   //sub_event index
    u8      ciss_order_tbl[LL_CIS_IN_PER_CIG_SLV_NUM_MAX];

    ll_cis_conn_t *pCis1st;
    ll_cis_conn_t *pCis2nd;
    ll_cis_conn_t *pcisDisconn;

    u8      u8_rsvd22;
    u8      u8_rsvd33;
    u8      schTsk_wptr;
    u8      schTsk_rptr;


    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  cigs_schTsk_fifo[CIG_SLV_FIFONUM];

}ll_cis_slv_t;

extern ll_cis_slv_t     *global_pCisSlv;
extern ll_cis_slv_t     *blt_pCisSlv;


int         blt_cig_slv_interrupt_task(int flag, void *p);
int         blt_cig_slv_mainloop_task(int flag);

int         blt_ll_cigSlvMainloop(void);
void        blt_ll_reset_cig_slv(void);

int         blt_cig_slv_start(int);
int         blt_crx_start(void);


int         blt_crx_post(ll_cis_conn_t *);


int         blt_ll_calcCigSlv1stAndCis1stAnchorPoint(void);
int         blt_ll_buildCisSlaveSchedulerLinklist(void);



ble_sts_t   blt_ll_cis_slave_control_pdu_process(st_ll_conn_t*, u8, u8 *);
bool        blt_ll_sendCisRsp(ll_cis_conn_t *);
bool        blt_ll_rejectCisReq(ll_cis_conn_t *, ll_cis_slv_t *);
int         blt_ll_cis_slave_cis_establish(ll_cis_conn_t *, ll_cis_slv_t *, ble_sts_t);
void        blt_cis_clearCisSlaveMainloopStatus(ll_cis_conn_t *, ll_cis_slv_t *);  //clear some CIS slave mainLoop status

ble_sts_t   blt_llCisReqParamsChk(st_ll_conn_t* pc, rf_packet_ll_cis_req_t*);

ll_cis_conn_t *blt_ll_findCisByHandle(u16 cisHandle);


void blc_set_cis_nse(u8 nse);

/******************************* cis_slave end ********************************************************************/




#endif /* STACK_BLE_CONTROLLER_LL_ISO_CIS_STACK_H_ */
