/********************************************************************************************************
 * @file    acl_stack.h
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
#ifndef ACL_STACK_H_
#define ACL_STACK_H_


#include "stack/ble/ble_common.h"
#include "stack/ble/ble_stack.h"
#include "stack/ble/ble_config.h"
#include "stack/ble/ble_format.h"

#include "stack/ble/controller/ll/ll_stack.h"
#include "stack/ble/controller/csa/csa_stack.h"
#include "stack/ble/controller/phy/phy_stack.h"
#include "stack/ble/hci/hci_stack.h"
#include "stack/ble/controller/ll/chn_sound/chn_sound_stack.h"
#include "stack/ble/controller/ll/acl_conn/async_lea/acl_async_stack.h"


/******************************* acl_conn start ******************************************************************/
#define         BLMS_CONN_TIMING_EXTEND                         1


#ifndef ACL_TXFIFO_4K_LIMITATION_WORKAROUND
#define ACL_TXFIFO_4K_LIMITATION_WORKAROUND                              0
#endif


#if(LL_ASYNC_LEA_EN)
#define         BLM_ASYNC_HANDLE                                BIT(4)
#endif

#define         BLM_CONN_HANDLE                                 BIT(7)
#define         BLS_CONN_HANDLE                                 BIT(6)
#define         BLT_ACL_CONN_HANDLE                             (BLM_CONN_HANDLE | BLS_CONN_HANDLE)

#define         BLM_HANDLE_MIN                                  BLM_CONN_HANDLE
#define         BLM_HANDLE_MAX                                  (BLM_CONN_HANDLE + LL_MAX_ACL_CEN_NUM - 1)
#define         BLM_HANDLE_MAX_ADD_1                            (BLM_CONN_HANDLE + LL_MAX_ACL_CEN_NUM)
#define         BLS_HANDLE_MIN                                  (BLS_CONN_HANDLE + LL_MAX_ACL_CEN_NUM)
#define         BLS_HANDLE_MAX                                  (BLS_CONN_HANDLE + LL_MAX_ACL_CONN_NUM - 1)
#define         BLS_HANDLE_MAX_ADD_1                            (BLS_CONN_HANDLE + LL_MAX_ACL_CONN_NUM)





#define         CONN_IDX_MASK                                   15


#define         INVALID_CIS_IDX                                 0xFF



#define         MAX_OCTETS_DATA_LEN_27                          27
#define         MAX_OCTETS_DATA_LEN_EXTENSION                   251


/////////////////////////////////////////////////////////////////////////////
#define         HANDLE_STK_FLAG                                 BIT(15)
#define         BLMS_STACK_USED_TX_FIFO_NUM                     2

/////////////////////////////////////////////////////////////////////////////



#define         LL_PACKET_OCTET_TIME(n)                         ((n) * 8 + 112)

#define         DATA_LENGTH_REQ_PENDING                         1
#define         DATA_LENGTH_RSP_PENDING                         2



#define         ENHANCED_CONN_FLAG_AUX_CONNECT                  BIT(1)
#define         ENHANCED_CONN_FLAG_CSA2                         BIT(2)

#define         ENHANCED_CONN_FLAG_ALL                          (ENHANCED_CONN_FLAG_AUX_CONNECT | ENHANCED_CONN_FLAG_CSA2)



//////////////////////////////// LL ENC ///////////////////////////////////
#define         MS_LL_ENC_OFF                                   0
#define         MS_LL_ENC_REQ                                   1
#define         MS_LL_ENC_REQ_T                                 2
#define         MS_LL_ENC_RSP_T                                 3
#define         MS_LL_ENC_START_REQ_T                           4
#define         MS_LL_ENC_START_REQ                             5
#define         MS_LL_ENC_START_RSP_T                           6
#define         MS_LL_ENC_PAUSE_REQ                             7
#define         MS_LL_ENC_PAUSE_RSP_T                           8
#define         MS_LL_ENC_PAUSE_RSP                             9
#define         MS_LL_REJECT_IND_T                              10
#define         MS_LL_ENC_SMP_INFO_S                            11
#define         MS_LL_ENC_SMP_INFO_E                            12

#define         MS_CONN_ENC_CHANGE                              BIT(7)
#define         MS_CONN_ENC_REFRESH                             BIT(6)
#define         MS_CONN_ENC_REFRESH_T                           BIT(5)





#define         CONN_STATUS_DISCONNECT                          0   //disconnect must be "0"
#define         CONN_STATUS_COMPLETE                            1
#define         CONN_STATUS_ESTABLISH                           2


#if BQB_TEST_EN
#define         BLMS_CONN_CREATE_RX_MAX_TRY_NUM                 6
#else
#define         BLMS_CONN_CREATE_RX_MAX_TRY_NUM                 6  //consider when SDK release
#endif
#define         BLMS_CONN_UPDATE_BRX_MAX_TRY_NUM                6




#define         CONN_UPDATE_MAP                                 BIT(0)

#define         CONN_UPDATE_CMD                                 BIT(1)
#define         CONN_UPDATE_PENDING                             BIT(2)
#define         CONN_UPDATE_NEARBY                              BIT(3)
#define         CONN_UPDATE_SYNC                                BIT(4)

#define         CONN_PHY_UPDATE_IND_CMD                         BIT(5)

#define         CONN_SUBRATE_REQ                                BIT(6)

#define         CONN_UPDATE_EVT                                 BIT(7)

#define         CONN_UPDATE_PARAM_MASK                          (CONN_UPDATE_CMD|CONN_UPDATE_PENDING|CONN_UPDATE_NEARBY|CONN_UPDATE_SYNC)

#define         LL_PDU_TIME_1M(payload)                   (u16)((10 + (payload) + 4)<<3)
#define         LL_PDU_TIME_CODED_S8(payload)             (u16_t)(80+256+16+24 + (2+payload+4+3)*8*8 + 3*8)

enum{
    CONN_UPD_FLAG   = 0x01,
    CHN_MAP_FLAG    = 0x02,
    PHY_UPD_FLAG    = 0x04,
};

_attribute_aligned_(4)
typedef struct {
    u16     connEffectiveMaxRxOctets;
    u16     connEffectiveMaxTxOctets;
    u16     connEffectiveMaxRxTime;
    u16     connEffectiveMaxTxTime;

    u16     connMaxRxOctets;
    u16     connMaxTxOctets;
    u16     supportedMaxRxOctets;
    u16     supportedMaxTxOctets;

    u8      connInitialMaxTxOctets;  //u8 is enough
    u8      connMaxTxRxOctets_req;
    u8      connRxDiff100;  //not used now
    u8      connTxDiff100;  //not used now
}ll_data_extension_t;


typedef struct {
    u16     size;
    u8      size_div_16;
    u8      num;

    u8      mask;
    u8      wptr;
    u8      rptr;
    u8      rsvd;

    u8*     p_base;
}acl_rx_fifo_t;

typedef struct {
    u16     size;
    u16     conn_full_size;
    u8      depth;
    u8      real_num;
    u8      logic_num;
    u8      mask;

    u8*     p_base;
}acl_tx_fifo_t;



typedef union {
    struct{
        u8      update_cmd;
        u8      u8_rsvd;
        u16     update_mark;
    };
    u32 update_pack;
}conn_updt_t;

typedef union {
    struct{
        u8      peer_terminate;
        u8      local_terminate;
        u8      terminate_reason;
        u8      rsvd;
    };
    u32 termin_pack;
}conn_trmt_t;


typedef union {
    struct{
        u8      connect_evt;        //Event triggered by IRQ
        u8      disconn_evt;        //Event triggered by IRQ
        u8      conn_update_evt;    //Event triggered by IRQ
        u8      phy_update_evt;     //Event triggered by IRQ
    };
    u32 irqevt1_pack;
}irq_evt_1_t;



typedef struct{
    u16 factor_min;
    u16 factor_max;

    u16 max_latency;
    u16 conti_num;//minimum

    u16 subrate_timeout;
    u8 valid;
    u8 u8_rsvd;

}subrate_param_t;

typedef union{
    struct{
        u8   ll_ver_ind_flag     :2;
        u8   ll_feat_exg_flag    :1;
        u8   ll_phy_ind_send_flag:1;
        u8   ll_phy_req_rcvd_flag:1;
        u8   peer_ack_flag        :1;
        u8   recvd0               :2;
    }bit;
    u8 flagBits;
}llcp_flag_t;




typedef union{
    struct{
        u16   subrate_update_flag  :1;
        u16   validDataRxTx_flag   :1;
        u16   conn_update_flag     :2;
        u16   subrate_evt_trige    :1;
        u16   subrate_req_pending  :1;
        u16   subrate_trans_mode   :1;
        u16   subrate_evt_flag     :1;
        u16   subrate_wrap_flag    :1;
        u16   subrate_para_change_flag :1;
        u16   subrate_update_evt:1;

        u16   rsvd     :5;

    }bit;
    u16 flagBits;
}ll_subrate_flag_t;


enum{
    FEATURE_SEND_FEAT_REQ       = BIT(0),
    FEATURE_WAIT_FEAT_RSP       = BIT(1),
    FEATURE_HCI_REPORT          = BIT(2),
};

_attribute_aligned_(4)
typedef struct {
    conn_updt_t     conn_update_union;
    irq_evt_1_t     irq_event1_union;
    conn_trmt_t     conn_termin_union;

    u8      acl_conIndex;
    u8      connState;           // 0/Conn_Complete/Conn_Establish
    u8      aclRole;
    u8      conn_fifo_flag;

    u8      tx_wptr;
    u8      tx_rptr;
    u8      tx_num;
    u8      max_fifo_num;

    u8      save_flg;
    u8      local_sn;
    u8      local_nesn;
    u8      conn_dma_tx_rptr;

    u8      peer_last_rfLen;
    u8      peer_last_sn;
    u8      chn_idx;
    u8      conn_chn;

    u8      conn_sca;
    s8      connUpt_inst_jump;
    u8      conn_updateEvt_pending; //hold this event until new conne_param_used  BIT(0): receive ll_conn_update_ind, BIT(1): connInterval change
    u8      blt_tx_pkt_hold;


    u8      conn_receive_packet;       //optimize: different CONN share same variable
    u8      conn_receive_new_packet;   //optimize: different CONN share same variable
    u8      connMarkTxFifoWptr;
#if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
    u8      establish_evt;
#else
    u8      u8_rsvd_cus_conn;
#endif


    u8      conn_chn_hop;
    u8      adv_handle;                 //todo: this variable & blt_extAdv_terminateEvt  shoule move to ext_adv module
    u8      num_completeTerminateEvt;
    u8      conn_successive_miss;

    u8      conn_winsize_next;      //u8 is enough
    u8      sentLlOpcode;
    u8      peer_chnSel;
    u8      conn_chnsel;   //1: CSA#2;  0: CSA#1



    u8      ll_enc_busy;
    u8      encryption_evt;     //Event triggered by MainLoop
    u8      encryption_tmp_st;
    llcp_flag_t llcp_flag;

    u8      ll_upd_flag;        //BIT(0):CONN_UPD_FLAG; BIT(1):CHN_MAP_FLAG; BIT(2):PHY_UPD_FLAG
    u8      rxPktCnt;           //rx packet counter. Note: Contains CRC error packets and CRC Ok packets.
    u8      local_last_sn;      //under macro "OPTIMIZE_INSERT_EMPTY_EN"
    u8      peer_coded_phy_ci;  //IRQ variable, only valid in LE coded PHY mode

    u8      rejectOpcode;
    u8      rejectReason;
    u8      ppm_div_10;         //todo: under PM macro
    bool    curCEsyncAP;    //Under the current connection event, M and S interact

    u8      sync_timing;        //slave used only
    u8      sync_num;           //slave used only
    u8      remoteFeatureReq;   //master and slave all used
    u8      FeatureRsp;

    u8      phy_chged;
    s8      rssi;
    u8      currRfPwrIdx; // use by API: rf_set_power_level_index
    u8      create_conn_status;


    //unit: 1.25mS
    u16     conn_intvl_n_1m25;          //u8: max interval 320mS, not enough for big interval such as 4S;  u16: max 81.9S
    u16     conn_intvl_next_n_1m25;     //u8: max interval 320mS, not enough for big interval such as 4S;  u16: max 81.9S
    u16     conn_offset_next;       //u8: max interval 320mS, not enough for big interval such as 4S;  u16: max 81.9S
    u16     conn_inst_next;
    u16     conn_phy_inst_next;
    u16     conn_para_inst_next;
    u16     conn_map_inst_next;     //update conn_param & update_map should separate, update map may delay to process due to BRX slot dropped
    u16     conn_latency;

    u16     conn_latency_next;
    u16     conn_timeout_next;     //Note: unit 10mS
    u16     acl_conHandle;  //use "con" instead of "conn", for code search
    u16     conn_inst;
    u16     conn_inst_mark;
    u16     enc_ediv;               //EDIV
    u16     pm_error_us;            //optimize, only slave use now, can depend on PM macro
    u16     inter_jump_num;

    u16     pdu_task_us;
    u16     u16_rsvd_g;

    u32     tick_qihang_mark_conn;       //only for Qihang: broadcast sink conflict with ACL. Later will delete.
    u32     conn_tick_mark;
    u32     ap_tick_mark;  //anchor point tick mark, now used for CIS map_update, can extend later

    u32     conn_update_tick;
    u32     conn_terminate_tick;
    u32     conn_intvl_tick;
    u32     conn_timeout;
    u32     conn_complete_tick;
    u32     conn_established_tick;  
    u32     ll_rsp_timeout_tick;    // PROCEDURE RESPONSE TIMEOUT
    u32     conn_tick;

    /* these 2 must be together, can feature 0 in front of feature1, because we use
     * "&pAclConn->ll_remoteFeature0" in code */
    u32     ll_remoteFeature0;
    u32     ll_remoteFeature1;



    /* slot relative begin */
    u8      u8_rsvd_slot;
    u8      bSlot_shift_margin;
    u16     bSlot_interval;       //u8: max 160mS, u16: max 40.96S

    u16     sSlot_allocNum;       //u16: max 1.3S, big enough; u8: max 4992 uS not enough
    u16     sSlot_sche_use;       //small slot number for potential scheduler process time, u8 is enough
    u16     sSlot_duration;       //u16: max 1.3S, big enough; u8: max 4992 uS not enough
    u16     bSlot_oft_num_next;     //used for master update connection, new interval bSlot offset number

    u32     bSlot_mark_conn;
    u32     conn_update_pre_bSlotIndex;
    /* slot relative end */


#if 1//(BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
    u32     conn_pkt_rcvd;
    u32     conn_pkt_rcvd_no;
    u8* volatile conn_pkt_dec_pending;
#endif


    u8      u8_rsvd50;
    u8      conn_peerPktA_type;  //peer address type of RF packet in air
    u8      conn_peerPktA[6];    //peer address      of RF packet in air

    /* if RPA resolve success, peerAddr is IDA; else peerAddr is pktA in air */
    u8      u8_rsvd51;
    u8      conn_peerAddrType;
    u8      conn_peerAddr[6];

#if (LL_FEATURE_ENABLE_PRIVACY)
    u8      u8_rsvd52;
    u8      conn_peerUseRpa;
    u8      conn_peerRpa[6];

    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
        u8      u8_rsvd53;
        u8      conn_locUseRpa;
        u8      conn_localRpa[6];

        ll_resolv_list_t * pRslvlst_conn;
    #endif
#endif


    /* encryption parameters begin */
    u8      enc_random[8]; //RANDOM
    u8      enc_skdm[8];   //master SKDm
    u8      enc_skds[8];   //slave  SKDs
    u32     enc_ivs;       //slave  IVs
    u32     enc_ivm;       //master IVm

    ble_crypt_para_t    crypt;   //40 Byte
    /* encryption parameters end */


    /* Channel map parameters begin */
    struct le_channel_map acl_chnParam;
    u32     aclAccessAddr;      //AA shall be set to the Access Address of the ACL connection
    u32     aclCrcInit;         //The ACL's CRC_Init value

    struct le_channel_map nextChn;

    u16     chnIdentifier;      //the ACL connection's Channel Identifier
    #if(HW_AES_CCM_ALG_EN)
        u8      lastTxLen_flag;
        u8      hw_aes_ccm_flag;
    #else
        u16     u16_rsvd3;
    #endif
    /* Channel map parameters end */


#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    u8      nocAckMsk;
    u8      nocAckNum;
    u8      nocAckWptr;
    u8      nocAckRptr;

    u8      nocAclTxWptr[32];  //optimize: 32 is too big
    u8      numOfCmpCnt[32];
#endif


#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    ll_conn_phy_t    connPhyCtrl;
#endif


#if (LL_FEATURE_ENABLE_PAST)
    ll_past_cb_t    *pPastCb;   //PAST concerned
                                //ll_past_cb_t   pclCb;
#endif

#if (LL_FEATURE_ENABLE_POWER_CONTROL)
    ll_pcl_cb_t     *pPclCb;    //power control concerned
                                //ll_pcl_cb_t    pclCb;
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
    ll_chnc_cb_t    *pChncCb;   //channel classification concerned
                                //ll_pcl_cb_t    chncCb;
#endif


#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    ll_data_extension_t ext_data;
#endif


#if(LL_FEATURE_ENABLE_CONNECTED_ISO)
    u8      cisEstablish_msk;   //u8: support 8 CIS most   IRQ variable
    u8      alink_cig_idx;      //not used now, ACL connection link CIG index: CIG master index or CIG slave index
    u8      reason_tmp;
    u8      ignore_cis_cmd; //for create CIS cancel

    u16     actual_txrx_sche_us;  //timing for: a TX&RX(master) or RX&TX(slave) + scheduler process, do not include BTX or BRX early set
    u16     limit_txrx_sche_us;   //now only slave use
#endif

#if(LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    subrate_param_t  subrateParam_req;
    u16 per_latency;
    u16 conti_num;
    u16 subrate_timeout1;//unused
    u16 factor;

    u16 subrateBaseEvent;
    ll_subrate_flag_t subrate_flag;

    u16 per_latency_next;
    u16 conti_num_next;
    u16 subrate_timeout_next;
    u16 factor_next;

    u16 subrateBaseEvent_next;
    u16 lastSubEventCnt;

    s16 noDataEvtCnt;      //Continuous Invalid Date event
    s16 subrateEvtRemain;  // remain event between 2 subrate event
    u16 noDataEvtStart;
    s16 insertTsk;

    u8 subrate_dft_flag;

    u8 subrate_rej_reason;
    u8 u8_resvd5[2];
#endif

#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
    u32     authPayloadTick;         //timer start tick
    u32     authPayloadTimeoutUs;    //timer threshold
    u8      authPayloadEnable;
    u8      align1[3];
#endif

#if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
    u8      conn_cte_type;
    u8      conn_cte_time;
    u8      cte_rsvd[2];
#endif

#if(LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
    u8      cs_pending;
    u8      rsvd8_1;
    u16     rsvd16_1;

    u32     csTaskEnableMask;
    u32     csMapMask;

    cs_param_t csParam; //csParam
    chn_sound_capbilities_t csRemoteSupCap;
#endif

#if (LL_ASYNC_LEA_EN)
    u16     async_lea_link;
    u16     async_rsvd16;
#endif

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    u8      every_conn_evt; //Event triggered by IRQ, in acl task post
    u8      chn_map_update_evt; //Event triggered by IRQ, in acl task start
    u8      acl_sniffer_sync_creating;
    u8      acl_sniffer_sync_update_ignore;
    
    u16     acl_sniffer_establish_inst;
    u16     snif_u16_rsvd;
#endif

} st_ll_conn_t;



_attribute_aligned_(4)
typedef struct {
    u8      conn_rx_num;
    u8      updateCmd_pending;
    u8      auto_dle;
    u8      last_master_mic_en; //special use for CIS master, mark latest ACL master encryption state

    u8      acl_rx_dma_size;
    u8      txPktCnt;
    u8      connUpdate_busy;
    u8      connUpdate_master_busy;

    u8      connSync;               //if more than 8 connections, u8 -> u16
    u8      u8_rsvd[3];

    u16     prefMaxTxLen;
    u16     prefMaxTxTime;

    u8      maxRxOct;
    u8      maxTxOct;
    u8      maxTxOct_master;
    u8      maxTxOct_slave;

    //TODO: remember one thing, dma bug found by Qiuwei/Lijing,  re_fix it
    u32     acl_rx_dma_buff;

    u32     connDleSendTimeUs;
    u32     tick_connectDevice;
    u32     task_end_tick;
} acl_conn_para_t;  //ACL connection parameters

extern  _attribute_aligned_(4)  acl_conn_para_t     aclConn_param;






typedef struct{
    u16     scan_inter;
    u16     scan_wind;
    u8      fp; //init_filter_policy
    u8      peerAddr_type;
    u8      peer_addr[6];
    u8      ownAddr_type;
    u16     conn_min;
    u16     conn_max;
    u16     connLatency;
    u16     timeout;
} ll_createConn_t;

_attribute_aligned_(4)
typedef struct {
#if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
    u8      cusConnEtbsh_en;
    u8      crtConn_retry_num;  //global config variable set by user, can not change
    u8      crtConn_cur_cnt;    //global state variable, must update/clear at particular situation
    u8      u8_rsvd;

    u32     re_create_conn_tick; //used under condition "tick connectDevice not zero", so pay attention about "tick connectDevice"

    ll_createConn_t crtConn_buf;
#endif
} acl_conn_etbsh_t;  //ACL connection parameters

extern  _attribute_aligned_(4)  acl_conn_etbsh_t        aclConn_etbsh;



typedef int (*blms_LTK_req_callback_t)(u16 handle, u8* rand, u16 ediv);
typedef bool (*ll_push_fifo_handler_t) (u16, u8 *);



extern  ll_task_callback_2_t                    ll_acl_past_irq_task_cb;
extern  ll_task_callback_t                      ll_acl_past_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_acl_pcl_irq_task_cb;
extern  ll_task_callback_2_t                    ll_acl_pcl_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_acl_chnclass_irq_task_cb;
extern  ll_task_callback_2_t                    ll_acl_chnclass_mlp_task_cb;

extern  ll_task_callback_2_t                    ll_acl_subrate_irq_task_cb;
extern  ll_task_callback_2_t                    ll_acl_subrate_mlp_task_cb;


typedef ble_sts_t (*ll_cis_ctrl_handler_t)(st_ll_conn_t*, u8, u8*);
#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
    extern ll_cis_ctrl_handler_t                ll_cis_master_ctrl_handler;
#endif
#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
    extern ll_cis_ctrl_handler_t                ll_cis_slave_ctrl_handler;
#endif

typedef ble_sts_t (*ll_pcl_ctrl_handler_t)(st_ll_conn_t*, u8, u8*);
#if (LL_FEATURE_ENABLE_POWER_CONTROL)
    #define LL_PCL_CB_NUMS                      min(1, LL_MAX_ACL_CONN_NUM) //TODO: only use 1
    #if (LL_PCL_CB_NUMS == 0)
        #error "Error: the number is at least 1"
    #endif
    extern ll_pcl_ctrl_handler_t                ll_pcl_ctrl_handler;
#endif

typedef ble_sts_t (*ll_past_ctrl_handler_t)(st_ll_conn_t*, u8, u8*);
#if (LL_FEATURE_ENABLE_PAST)
    #define LL_PAST_CB_NUMS                     max(LL_MAX_ACL_CEN_NUM, LL_MAX_ACL_CONN_NUM) //TODO: only use LL_MAX_ACL_CEN_NUM
    #if (LL_PAST_CB_NUMS == 0)
        #error "Error: the number is at least 1"
    #endif
    extern ll_past_ctrl_handler_t               ll_past_ctrl_handler;
#endif

typedef ble_sts_t (*ll_chnclass_ctrl_handler_t)(st_ll_conn_t*, u8, u8*);
#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
    #define LL_CHNC_CB_NUMS                     min(1, LL_MAX_ACL_CONN_NUM) //TODO: only use 1
    #if (LL_CHNC_CB_NUMS == 0)
        #error "Error: the number is at least 1"
    #endif
    extern ll_chnclass_ctrl_handler_t           ll_acl_chnclass_ctrl_handler;
#endif


#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    typedef ble_sts_t (*ll_subrate_ctrl_handler_t)(st_ll_conn_t*, u8, u8*);
    extern ll_subrate_ctrl_handler_t            ll_acl_subrate_ctrl_handler;

    typedef ble_sts_t (*ll_subrate_process_req_handler_t)(st_ll_conn_t*, u8*);
    extern ll_subrate_process_req_handler_t     ll_acl_subrate_process_req;
#endif



typedef int (*ll_ctrl_pdu_handle_t)(st_ll_conn_t*, u8, u8*);
extern  ll_ctrl_pdu_handle_t            ll_chn_sounding_ctrl_handler;

extern _attribute_aligned_(4)   st_ll_conn_t    AA_blms[];   // AA_ just for debug
#define blms    AA_blms
extern  st_ll_conn_t*       blms_pconn;
extern  volatile int            blms_conn_sel;
extern  acl_rx_fifo_t           blt_rxfifo;
extern  acl_tx_fifo_t           blt_m_txfifo;
extern  acl_tx_fifo_t           blt_s_txfifo;
extern ll_push_fifo_handler_t   ll_push_tx_fifo_handler;

extern ll_task_callback_2_t     ll_acl_slave_mlp_task_cb;
extern ll_task_callback_2_t     ll_acl_master_mlp_task_cb;


#if (ACL_TXFIFO_4K_LIMITATION_WORKAROUND)

typedef struct {
    u16     size;
    u8      num;
    u8      mask;
    u8      wptr;
    u8      rptr;
    u8      rsvd1;
    u8      rsvd2;
    u8*     p;
}   acl_cache_txfifo_t;

extern  acl_cache_txfifo_t  blt_cache_txFifo;

bool blt_acl_pushCacheTxfifo(u16 connHandle, u8 *p);
void blt_acl_cache_tx_fifo_to_hw_tx_fifo(void);

#endif

/**
 * @brief      This function is used to check whether the current handle parameter is legal and in the connected state.
 * @param[in]  connHandle - ACL connection handle.
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blt_ll_isAclhdlInvalid(u16 connHandle);
ble_sts_t   blt_ll_isAclHandleOutOfRange(u16 connHandle);

int         blt_acl_conn_interrupt_task (int flag);
int         blt_acl_conn_mainloop_task (int flag, void *p);

int         blt_ll_set_interval_level (u8 task_offset, u16 interval);
int         blt_ll_checkAclInit(void);
void        blt_ll_reset_acl_conn(void);
int         blt_ll_procConnectionEvent(u16 connHandle,  st_ll_conn_t* pc);
int         blms_connect_common(st_ll_conn_t *pc, rf_packet_connect_t * pInit, bool);

int         blms_start_pre_process(int);
int         blms_start_common_1  (st_ll_conn_t *pc);
int         blms_start_common_2  (st_ll_conn_t *pc);

bool        blms_post_pre_process(void);
int         blms_post_common_1(st_ll_conn_t *pc);
int         blms_post_common_2(void);


int         irq_acl_conn_rx(void);
int         irq_acl_conn_tx(void);

bool        blt_ll_isRepeatedAclConnDevice(int start_idx, int end_idx);

int         blt_ll_acl_conn_sync_process(int sync_ok);
bool        blt_ll_isMarkFifoTxDone(st_ll_conn_t *pc);

u8          blt_ll_getRealTxFifoNumber (u16 connHandle);
u8          blt_llms_get_tx_fifo_max_num (u16 connHandle);

void        blt_llms_pushToHWfifo(void);
void        blt_llms_saveTxfifoRptr(void);

bool        blt_llms_pushTxfifo(u16 connHandle, u8 *p);
void        blt_llms_push_fifo_hw(void);
void        blt_llms_update_fifo_sw(void);
bool        blt_llmsPushLlCtrlPkt(u16 connHandle, u8 opcode, u8*pkt);


bool        blt_llms_unknownRsp(u16 connHandle, u8 unknownType);
bool        blt_llms_rejectInd(u16 connHandle, u8 opCode, u8 errCode, u8 extRejectInd);
int         blt_llms_main_loop_data (u16 connHandle, u8 *raw_pkt);
void        blt_ll_rspTimeoutLoopEvt(u16 connHandle);
void        blt_llms_procConnCreateConnParamUpdate(void);

void        blt_llms_registerLtkReqEvtCb(blms_LTK_req_callback_t evtCbFunc);


int         blt_llms_isConnectionEncrypted(u16 connHandle);

ble_sts_t   blt_ll_startEncryption(u16 connHandle ,u16 ediv, u8* random, u8* ltk);

u8          blt_llms_getConnState(u16 connHandle);
u8 blt_ll_getAclConnPeerAddrType(u16 connHandle);
u8* blt_ll_getAclConnPeerAddr(u16 connHandle);

int         blt_ll_acl_conn_mainloop(void);

ble_sts_t   blt_ll_enc_proc_disConnect (u16 connHandle, u8 reason);
ble_sts_t   blt_hci_ltkRequestReply(u16 connHandle,  u8*ltk);
ble_sts_t   blt_hci_ltkRequestNegativeReply(u16 connHandle);
ble_sts_t   blc_hci_disconnect(hci_disconnect_cmdParam_t *para);


ble_sts_t   blc_hci_receiveHostACLData(hci_acl_data_pkt_t *);
ble_sts_t   blc_hci_setTxDataLength (u16 connHandle, u16 tx, u16 txtime);
ble_sts_t   blt_hci_rx_aclfifo_to_rf_txfifo(void);

void blc_ll_acl_resetInfoRSSI(u16);
void blc_ll_acl_recordRSSI(u16, u8);
void blc_ll_acl_recordLatestRSSI(u16, u8);

void blt_ll_authPayloadTimeoutExpiredHandler(u16 connHandle);

/**
 * @brief      get remote support feature by HCI in slave role .
 * @param[in]  connHandle - connect handle
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_getRemoteSupportedFeatures(u16 connHandle);

void        blt_ll_processFeatureExchange(st_ll_conn_t* pAclConn);
static inline void      blt_ll_send_feature_req(st_ll_conn_t* pAclConn)
{
    pAclConn->remoteFeatureReq = FEATURE_SEND_FEAT_REQ;
}


static inline void  blt_acl_send_ext_reject (st_ll_conn_t *pc, u8 opCode, u8 errCode)
{
    if(!pc->rejectReason){
        pc->rejectOpcode = opCode;
        pc->rejectReason = errCode;
    }
}


static inline u8 blt_llms_get_connEffectiveMaxTxOctets_by_connIdx(u16 conn_idx)
{
    #if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
        return blms[conn_idx].ext_data.connEffectiveMaxTxOctets;
    #else
        (void) conn_idx;
        return 27;
    #endif
}

static inline u8 blt_llms_get_connEffectiveMaxRxOctets_by_connIdx(u16 conn_idx)
{
    #if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
        return blms[conn_idx].ext_data.connEffectiveMaxRxOctets;
    #else
        (void) conn_idx;
        return 27;
    #endif
}



void        blc_hci_writeAuthPayloadTimeout(hci_writeAuthPayloadTimeout_cmdParam_t *pCmdParam, hci_writeAuthPayloadTimeout_retParam_t *pRetParam);
void        blc_hci_readAuthPayloadTimeout(u16 connHandle, hci_readAuthPduTimeout_retParam_t *pRetParam);


ble_sts_t   blt_ll_sendHostNumOfCmpPktNonConn(u8* numOfCmpConn);

ble_sts_t   blc_hci_le_readBufferSize_cmd(hci_le_readBufSize_v1_retParam_t*);


ble_sts_t   blc_hci_le_enableEncryption (hci_le_enableEncryption_cmdParam_t* pCmdParam);



int         blt_acl_conn_check_expire(void);

void        blt_save_aes_ccm_para(st_ll_conn_t *pc);



u8          blt_ll_pushTxfifoHold (u16 connHandle, u8 *p);
u8          blt_ll_push_hold_data(u16 connHandle);

void        blt_ll_setEncryptionBusy(u16 connHandle, u8 enc_busy);
int         blt_ll_isEncryptionBusy(u16 connHandle);

u8          blt_ll_smpPushEncPkt (u16 connHandle, u8 type);
void        blt_ll_smpSecurityProc(u16 connHandle);
void        blt_ll_procDlePending(u16 connHandle);
/******************************* acl_conn end   ******************************************************************/







/******************************* acl_slave start ******************************************************************/
#define         ACL_SLAVE_NON_MODULAR                           1   //save some RamCode




#define         SLAVE_SYNC_CONN_CREATE                          BIT(0)
#define         SLAVE_SYNC_CONN_UPDATE                          BIT(1)
#define         SLAVE_SYNC_CONN_UPT_FAIL                        BIT(2)


/* eagle fast settle, RX save 45uS, here we reduce 3 bSlot, about 59uS, not correct in theory
 * We give sufficient RX prepare time in old code, so here 14uS is not big deal
 * we will see the effect.    20230425 SiHui comment */
#ifdef HAL_CHIP_USE_CSEM_MODEM_IP
#define HAL_CSEM_IP_ENABLE                                      1
#else
#define HAL_CSEM_IP_ENABLE                                      0
#endif


#if (FAST_SETTLE || HAL_CSEM_IP_ENABLE) // 4 + (2+1+2)
    #if (SYSTICK_NUM_PER_US == 16)
        #if 0
            #define         BRX_EARLY_SET_TICK                              1250 //312.5*4=1250  = 78.125 uS
            #define         BRX_EARLY_SET_SSLOT_NUM                         4

        #else   //TODO: For B91 calibrate this later.
            #define         BRX_EARLY_SET_TICK                              1563 //312.5*5=1563  = 97.656 uS
            #define         BRX_EARLY_SET_SSLOT_NUM                         5
        #endif

        #define         BRX_HALF_MARGIN_TICK                            625  //312.5*2=625 = 39 uS
        #define         BRX_HALF_MARGIN_SSLOT_NUM                       2
    #elif(SYSTICK_NUM_PER_US == 24)
        #if 0   //Original value before B92 SDK Release
            #define         BRX_EARLY_SET_TICK                              1875 //468.75*4=1875
            #define         BRX_EARLY_SET_SSLOT_NUM                         4
        #else   //For B92 SDK Release, give more early window for stable. TODO: calibrate this later.
            #define         BRX_EARLY_SET_TICK                              2344 //468.75*5=2344
            #define         BRX_EARLY_SET_SSLOT_NUM                         5
        #endif

        #define         BRX_HALF_MARGIN_TICK                            938  //468.75*2=625 = 39 uS
        #define         BRX_HALF_MARGIN_SSLOT_NUM                       2
    #else
        #error "SYSTICK NUM PER US other than 16, add code here"
    #endif

    #if 0   //Original value before B92 SDK Release
        #define         BRX_LEFT_EARLY_SSLOT_NUM                            6    // 4+2
    #else       //For B92 SDK Release, give more early window for stable. TODO: calibrate this later.
        #define         BRX_LEFT_EARLY_SSLOT_NUM                            7    // 5+2
    #endif

    #define         BRX_RIGHT_USE_US                                    59   // 3*(625/32) = 58.59uS
#else // 7 + (3+1+3)
    #if (SYSTICK_NUM_PER_US == 16)
        #define         BRX_EARLY_SET_TICK                              2185  //312.5*7=2185.7  = 136.7uS
        #define         BRX_EARLY_SET_SSLOT_NUM                         7

        #define         BRX_HALF_MARGIN_TICK                            938  //312.5*3=937.5
        #define         BRX_HALF_MARGIN_SSLOT_NUM                       3
    #elif(SYSTICK_NUM_PER_US == 24)
        #define         BRX_EARLY_SET_TICK                              3281  //468.75*7=3281.25  = 136.7uS
        #define         BRX_EARLY_SET_SSLOT_NUM                         7

        #define         BRX_HALF_MARGIN_TICK                            1406  //468.75*3=1406.25 = 58.6uS
        #define         BRX_HALF_MARGIN_SSLOT_NUM                       3
    #else
        #error "SYSTICK NUM PER US other than 16/24, add code here"
    #endif


    #define         BRX_LEFT_EARLY_SSLOT_NUM                            10   // 7+3
    #define         BRX_RIGHT_USE_US                                    78   // 4*(625/32) = 78.125

//  #define         BRX_LEFT_EARLY_US                                   195  //10*(625/32) = 195.3uS
//  #define         BRX_MARGIN_US                                       273  //14*(625/32) = 273.4uS
//  #define         BRX_RIGHT_USE_SSLOT_NUM                             4    // 1+3
#endif


#define         BRX_LEFT_EARLY_TICK                             (BRX_EARLY_SET_TICK + BRX_HALF_MARGIN_TICK)
#define         BRX_MARGIN_SSLOT_NUM                            (BRX_EARLY_SET_SSLOT_NUM + BRX_HALF_MARGIN_SSLOT_NUM*2 + 1)




#define         ACL_SLAVE_FIFONUM                               12

enum{
    SLV_FLAG_LEN_ERR       = 0x01,
    SLV_FLAG_INSTANT_PASS  = 0x02,
};

_attribute_aligned_(4)
typedef struct {
    ////////////////// common for master sniffer used vars begin /////////////////////
    /// refer to 'st_llm_conn_t' begin part
    //0x0
    u8      timing_update;
    u8      brx_pkt_miss;
    u8      sync_timing;
    u8      latency_wakeup_flg;
    u8      role; //refer to 'acl_connection_role_t'
    u8      slave_sleep_flg;
    u16     tolerance_max_us;

    u16     conn_tolerance_us;
    u16     latency_available;


    u32     sleep_32k_rc;
    u32     tick_1st_rx;
    u32     tick_last_1st_rx;
    u32     expectTimeMark;


    u32     connExpectTime;
    u32     conn_start_time;
    s32     conn_offset_tick;
    u32     sleep_sys_ms; //slv used only
    u32     tick_conn_expect; //mst used only
    ////////////////// common for master sniffer used vars end /////////////////////


    u8      acl_slv_Index;
    u8      brx_cal_synced;
    u8      brx_cal_long_sleep_synced;
    s8      ppm_cal_idx_cur;

    s8      ppm_cal_idx_last;
    s8      ppm_idx;
    u8      errFlag;               /* use for CONN_UPD/CONN_MAP_UPD/CONN_PHY_UPD in RAM. */
    u8      unknownType;         /* use for CONN_UPD/CONN_MAP_UPD/CONN_PHY_UPD in RAM. */

    u8      aclTsk_wptr;
    u8      aclTsk_rptr;
    u16     latency_used;

    u16     evtCnt_mark_1strx;
    u16     offsetUs_mark1stRx;   //u16 is enough

    //0x40
    u32     tick_mark_1strx;
    u32     latency_wakeup_tick;


    /* slot relative begin */
    s16     sSlot_shift_tor;  //sSlot number shift for current tolerance
    s16     sSlot_offset;

    u32     sSlot_interval;  //u16: 65536*20uS = 1.3S, not enough, max value 4S for BQB, so use u32
    s32     sSlot_mark_conn;    //now master not use
    s32     sSlot_mark_brx;
    u32     bSlot_mark_1strx;
    s32     conn_update_pre_sSlotIndex;
    /* slot relative end */



    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  aclTsk_fifo[ACL_SLAVE_FIFONUM];

#if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
    u8      blt_buff_conn[24];
#endif

} st_lls_conn_t;

extern  _attribute_aligned_(4)  st_lls_conn_t   blmsSlave[];
extern  st_lls_conn_t   *bls_pconn;
extern  int             bls_conn_sel;




bool    blt_s_connect (rf_packet_connect_t * pInit, bool aux_conn);

int     blt_brx_start(int conn_idx, void *p);
int     blt_brx_post(void);


void    blt_brx_timing_init(void);
void    blt_brx_timing_update(void);


int     blt_ll_build_acl_slave_schedule(void);

int     blt_ll_set_slave_conn_interval_level (st_ll_conn_t *pc, st_lls_conn_t *ps, u16 conn_interval);

void    blt_ll_reset_acl_slave(void);

int     blt_acl_slave_interrupt_task (int flag, void*p);
int     blt_acl_slave_mainloop_task (int flag, void*p);

int     blt_acl_slave_clear_sleep_latency (u8 conn_idx);

void    blt_acl_slave_rx_procUpdateReq(u8 *raw_pkt);
void    blt_acl_slave_tx_procUpdateReq(void);
void    blt_acl_slave_slotgap_procUpdateReq(void);
int     blt_ll_conn_chn_phy_update(st_ll_conn_t* pc, u8 *pkt);
/******************************* acl_slave end   ******************************************************************/









/******************************* acl_master start ******************************************************************/
#ifndef         ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN
#define         ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN           1
#endif


#ifndef         IMPROVE_MASTER_INTERVAL
#define         IMPROVE_MASTER_INTERVAL                         1
#endif


#ifndef         ACL_CENTRAL_BASE_INTERVAL_FOLLOW_UPPER_LAYER
#define         ACL_CENTRAL_BASE_INTERVAL_FOLLOW_UPPER_LAYER        0
#endif


#define         ACL_MASTER_FIFONUM                              12
#define         BLM_ENC_PKT_PENDING_NUM                         4
#define         BLM_ENC_PKT_SKIP_ENCRYPT                        (BIT(3))





/* time distance from IRQ tick to FSM trigger tick
 * this time should be evaluated and measured,
 * ode running time from IRQ to here can not exceed  IRQ BTX DELAY US
 */
#define         IRQ_BTX_DELAY_US                                50

/* time distance from IRQ tick to TX packet in the air
 * our strategy: make 2M & Coded PHY timing equal to 1M, so when you  test PHY update, pay attention TX packet
 * distance when PHY changed. So we can use 1M timing for all three kind of PHYs
 * 40 uS: 5 byte preamble * 8uS for 1M PHY
 * 2 uS: margin, consider some baseband switching time */
#define         ACL_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US        TX_STL_BTX_1ST_PKT_REAL //different PHY share same value


#define         IRQ_BTX_SEND_DELAY_US                           (IRQ_BTX_DELAY_US + ACL_TX_TRIGGER_TO_PKT_IN_AIR_DISTANCE_US)


#define         ACL_CMD_DONE_MANUAL_TRIGGER_STIMER_DELAY_US                  50

_attribute_aligned_(4)
typedef struct {
    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    ////////////////// common for acl slave used vars begin /////////////////////
    /// refer to 'st_lls_conn_t' begin part
    u8      timing_update;
    u8      rx_pkt_miss; //for slave, here name: brx_pkt_miss
    u8      sync_timing;
    u8      latency_wakeup_flg;
    u8      role; //refer to 'acl_connection_role_t'
    u8      slave_sleep_flg; //mst unused now
    u16     tolerance_max_us;

    u16     conn_tolerance_us;
    s16     sSlot_shift_tor;  //sSlot number shift for current tolerance
    s16     sSlot_offset;
    u16     latency_available;

    u32     sleep_32k_rc;
    u32     tick_1st_rx;
    u32     tick_last_1st_rx;
    u32     expectTimeMark;
    u32     connExpectTime;
    u32     conn_start_time;
    s32     conn_offset_tick;
    u32     sSlot_interval;  //u16: 65536*20uS = 1.3S, not enough, max value 4S for BQB, so use u32
    s32     sSlot_mark_conn;
    u32     sleep_sys_ms; //slv used only
    u32     tick_conn_expect; //mst used only
    ////////////////// common for acl slave used vars end /////////////////////
    #endif

    u8      init_pos_idx;   //initiate timing position index
    u8      updt_pos_idx;
    u8      aclTsk_wptr;
    u8      aclTsk_rptr;

#if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
//  u8      aclc_idx_mark;
//  u8      u8_rsvd[3];
#endif

    u32     init_pos_msk;  //initiate timing position index
    u32     updt_pos_msk;

    u32     bSlot_1stBtx_mark_update_hold;
    u32     bSlotMark_position_update_hold;


    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  aclTsk_fifo[ACL_MASTER_FIFONUM];

} st_llm_conn_t;

_attribute_aligned_(4)
typedef struct
{
    u8      wptr;
    u8      rptr;
    u8      mask;
    u8      num;
    u8      buff[BLM_ENC_PKT_PENDING_NUM][48];
}blm_pkt_pending_t;

extern  _attribute_aligned_(4)  st_llm_conn_t       blmsMaster[];
extern  _attribute_aligned_(4)  blm_pkt_pending_t   blmsMasterEncPktPending[];
extern  _attribute_aligned_(4)  st_llm_conn_t      *blm_pconn;


_attribute_aligned_(4)
typedef struct {
#if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    u8      timCus_en;
    u8      bSlotDurn_diffMod;  //1: bSlot duration number different mode; 0: bSlot duration number same mode
    u8      bSlotNum_sameMod;
    u8      timposn_max;


    u8      aclc_idx_conn; //current created ACL CEN index   confirmed by "create connection" API
    u8      cur_timPosn_diffMod;    //current timing position for BSLOT different mode
    u8      cur_bslotOft_diffMod;   //current timing position bSlot offset form zero position for BSLOT different mode
    u8      u8_rsvd[1];
#endif

    u16     bSlotNum_piece_inter;
    u16     bSlotNum_whole_inter;    //"u8": can not exceed 160 mS

    u16     master_connInter;           //"u8": conn_interval can not exceed 320 mS, "u16" is OK
    u16     u16_rsvd;

    u32     bslot_1st_btx_mark;

    u32     position_mask[LL_MAX_ACL_CEN_NUM]; //
    u32     bSlot_mark_position[LL_MAX_ACL_CEN_NUM]; //

#if (ACL_CEN_SUPPORT_TASK_TIMING_CUSTOM_EN)
    u8      timposn_idx[LL_MAX_ACL_CEN_NUM];
    u8      bSlot_number[LL_MAX_ACL_CEN_NUM];
    u8      bSlot_offset_pos0[LL_MAX_ACL_CEN_NUM]; //current position distance(bSlot offset) from position 0
#endif

} acl_mas_para_t;  //ACL master parameters

extern  _attribute_aligned_(4)  acl_mas_para_t      aclMas_param;

typedef int (*ll_acl_master_terminate_irq_callback_t)(u16 connHandle);
extern ll_acl_master_terminate_irq_callback_t       ll_acl_master_terminate_irq_cb;

bool    blms_m_connect (rf_packet_connect_t * pInit, u8 *raw_pkt);
int     blt_btx_start (int conn_idx, void *p);
int     blt_btx_post (void);

void    blt_ll_reset_acl_master(void);
int     blt_aclc_check_init(void);

int     blt_acl_master_interrupt_task (int flag, void*p);
int     blt_acl_master_mainloop_task (int flag, void*p);

void    blt_ll_registerAclMasterTerminateIrqCb(ll_acl_master_terminate_irq_callback_t  cb);
int     blt_ll_ctrlAclMstChClassUpd(unsigned char *pChm);
int     blt_ll_ctrlAclSlvChClassUpd(unsigned char *pChm);
bool    blt_ll_pushAclChClassUpdPkt(u16 connHandle, u8 *pChm);

int     blt_ll_buildAclMasterSchedulerLinklist(void);
void    blt_ll_calculateAclMasterBslotNumber(void);

ble_sts_t blt_ll_exchangeDataLength (u16 connHandle, u8 opcode, u16 maxTxOct);

st_llm_conn_t* blt_ll_isValidAclCentralHandle(u16 connHandle);


u8 blt_ll_getMaxAalCentralNumber(void);

#define         INVALID_ACL_IDX                                 0xFF


/* ACL Central task arrange pattern */
typedef enum{
    ACLC_EVENLY_SCATTERED       =   0,  // evenly scattered, default value
    ACLC_CONCENTRATED           =   1,  // concentrated
}aclc_arng_patn_t;  //arrangement pattern



/* ACL Central slot duration mode */
typedef enum{
    ACLC_SET_ARNG_DISABLE       =   0,  // disable
    ACLC_SET_ARNG_ENABLE        =   1,  // enable
}aclc_arng_en_t;    //timing customize enable


/* ACL Central slot duration mode */
typedef enum{
    ACLC_SLOT_DURN_SAME     =   0,  // slot duration all same for different ACL Central task
    ACLC_SLOT_DURN_DIFF     =   1,  // slot duration different for different ACL Central task
}aclc_slotDur_mod_t;    //slot duration mode



typedef enum{
    ACLC_TIMPOSN_IDX0       =  0,
    ACLC_TIMPOSN_IDX1       =  1,
    ACLC_TIMPOSN_IDX2       =  2,
    ACLC_TIMPOSN_IDX3       =  3,
}timposn_idx_t;  //ACL Central timing position index


/**
 * @brief      for user to set multiple ACL Central tasks arrange pattern
 * @param[in]  pattern - set tasks evenly scattered or concentrated
 * @return     none
 */
//void      blc_ll_setAclCentralArrangePattern(aclc_arng_patn_t  pattern);

/**
 * @brief      for user to set ACL Central tasks slot duration mode, and slot duration number for "slot duration same" mode
 *             here 1 slot is 625uS
 *             attention: this API must called after "blc_ll_setMaxConnectionNumber" & "blc_ll_setAclCentralBaseConnectionInterval"
 * @param[in]  st_mode - slot duration mode: same or different
 * @param[in]  slotDurNum_sameMode - slot duration number for "slot duration same";  ignore this value under "slot duration different" mode
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
//ble_sts_t     blc_ll_setAclCentralSlotDurationMode(aclc_slotDur_mod_t sd_mode, u8 slotDurNum_sameMode);
ble_sts_t   blc_ll_setAclCentralTaskTimingArrangement(aclc_arng_en_t enable, aclc_slotDur_mod_t sd_mode, u8 slotDurNum_sameMode);

// customize  Acl Central Task Timing Order

/**
 * @brief      for user to set ACL Central task slot duration by index, under "slot duration different" mode
 *             attention: this API must called after "blc_ll_setMaxConnectionNumber" & "blc_ll_setAclCentralBaseConnectionInterval"
 * @param[in]  acl_cen_index - ACL Central index managed by application.
 *             should in range of 0 ~ "ACL Central number - 1", here ACL Central number is max_master_num specified by "blc_ll_setMaxConnectionNumber"
 * @param[in]  slotDur_num - slot duration number specified by acl_cen_index
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
//ble_sts_t     blc_ll_setAclCentralSlotDurationNumberDiffMode(int acl_cen_index, u8 slotDur_num);

ble_sts_t   blc_ll_setAclCentralTimingPositionSlotNumber_for_diffMode(int acl_cen_index, timposn_idx_t time_position_index, u8 slotDur_num);






/**
 * @brief      This function is used to
 * @param[in]  acl_cen_idx -
 * @return     Status - 0x00: command succeeded;
 *                     other: failed
 */
ble_sts_t   blc_ll_setAclCentralIndex_arrangedTaskTiming_diffMode(u8 cur_aclcen_idx);
u8          blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(u16 acl_handle);

/******************************* acl_master end   ******************************************************************/




/******************************* acl_phy start ******************************************************************/
void blt_ll_sendPhyReq(u16 connHandle);
void blt_ll_sendPhyUpdateInd(u16 connHandle);
int  blt_ll_updateConnPhy(u16 connHandle);
int  blt_ll_switchConnPhy(u16 connHandle);
int  blt_cfg_conn_phy_param(ll_conn_phy_t* pconn_phy, le_phy_type_t curPhy, le_coding_ind_t CI);

/**
 * @brief       this function is used to set PHY type for connection, same as API: blc_ll_setPhy
 * @param[in]   connHandle - Connection_Handle Range:0x0000 to 0x0EFF
 * @param[in]   all_phys - preference PHY for TX & RX
 * @param[in]   tx_phys - preference PHY for TX
 * @param[in]   rx_phys - preference PHY for RX
 * @param[in]   phy_options - LE coding indication prefer
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t   blc_hci_le_setPhy(hci_le_setPhyCmd_param_t* para);
void *blt_ll_getAclConnPtr(u16 connHandle);

/******************************* acl_phy end   ******************************************************************/




#endif /* ACL_STACK_H_ */
