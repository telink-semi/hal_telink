/********************************************************************************************************
 * @file    pda_stack.h
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
#ifndef STACK_BLE_CONTROLLER_LL_PRDADV_PDA_STACK_H_
#define STACK_BLE_CONTROLLER_LL_PRDADV_PDA_STACK_H_



#include "stack/ble/hci/hci_cmd.h"
#include "stack/ble/controller/ll/ll_stack.h"
#include "PAwR_sync.h"

#ifndef UPDATE_SCHEDULER_FOR_PAWR_RSP
#define UPDATE_SCHEDULER_FOR_PAWR_RSP   0
#endif


#define     PDA_UPDATE_MAP              BIT(0)

/******************************* pda start ***********************************************************************/
_attribute_aligned_(4)
typedef struct {
    u8      pda_phy;    //"le_phy_type_t"
    u8      update_map;
    u16     lastPaEvtCnt; //mark last received periodic_adv event count


    u16     paEvtCnt;//periodic advertisingpaEventCounter
    u16     chnIdentifier;      //periodic advertising channel identifier
    u16     bSlot_prdadv_itvl;
    u16     sSlot_duration_pda;
    u32     sSlot_prdadv_itvl; //big periodic advertising interval, u32 used 

    u32     bSlot_mark_prdadv;

    u32     lastPaAnchorPoint;
    u32     paAccessAddr;       //AA shall be set to the Access Address of the periodic advertising
    u32     paCrcInit;          //Same as the ACL's CRC_Init value
    u32     prdadv_send_cnt; //Periodic ADV sending execution times accumulated

    struct csa2_param chnParam;  //4B aligned
    struct le_channel_map nextChn;
    // next chm update use
    u16 rsvd16;
    u16     prd_map_inst_next;

}st_pda_t;
extern st_pda_t *blt_pPda;

typedef struct{
    u8  len;
    u8  type;
    u8  chm[5];
    u16 instant;
}pda_sync_chmupt_t;

int     blt_pda_start_common_1(void);
int     blt_pda_start_common_2(void);
int     blt_pda_post_common(void);

int blt_ll_pdaSync_pawrSync_info_process(sync_info_t *pSyncInf, u8* pPAwR_syncTimeInfo);
/******************************* pda end *************************************************************************/







/******************************* prd_adv start *******************************************************************/


typedef struct
{
    u8      maxNum_perdAdv;
    u8      prd_adv_sel;

}ll_prdadv_mng_t;

extern ll_prdadv_mng_t      bltPrdA;


#define     PERD_ADV_FIFONUM                        12  // 80/7.5=10.6

#define     PERD_ADV_SLOT_RMV_MSK                   BIT(6)
#define     PERD_ADV_SLOT_BUILD_MSK                 BIT(7)

/* ACAD used field define */
#define     PERD_ACAD_NONE                          0
#define     PERD_ACAD_BIGINFO_ENA                   BIT(0)
#define     PERD_ACAD_CHMUPT_ENA                    BIT(1)
#define     PERD_ACAD_BIGINFO_DIS                   BIT(2)
#define     PERD_ACAD_CHMUPT_DIS                    BIT(3)
#define     PERD_ACAD_PAwR_ENA                      BIT(4)
#define     PERD_ACAD_PAwR_DIS                      BIT(5)

/*PDA enable bit define*/
#define     PERD_ENABLE_BIT                         BIT(0)
#define     PERD_INCLUDE_ADI_BIT                    BIT(1)


#define   PAWR_SUBEVENT_MAX_NUM                     128 //spec max support 128, now telink also support 128
#define   PAWR_SLOT_PER_SUBEVENT_MAX_NUM            255 //described in ESL profile


#if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
typedef struct{
    u8 subevtStart;
    u8 subevtCount;
}subDataReq_subInfo_t;

typedef struct{
    u8      subevent_idx;
    u8      rsp_slot_start;
    u8      rsp_slot_count;
    u8      subevt_data_len;

    u8*     pSubevt_data;
} pawra_subDataCfg_t;
#endif

_attribute_aligned_(4)
typedef struct ll_prd_adv_para_t{
    u8      prd_adv_en;    //final enable status
    u8      advSet_idx; //remove later
    u8      advHand_mark;  //adv_handle mark
    u8      unfinished_advData;

    u8      prdadv_index;
    u8      mapping_extadv_idx;
    u8      u8_rsvd1[2];

    u8      big_idx;
    u8      acad_field_len;
    u8      acad_used; //refer to "ACAD used field define"
    u8      extAdvSet_idx;

    u8      prd_DID_changed;  //for PDA,core5.3 include ADI, but core5.2 not.
    u8      include_ADI_flag; //LL/DDI/ADV/BV-63-C
    u16     prd_DID;

    u8      acad[60]; //33B(no enc bis) or 57B(enc bis), use 60B enough

    u8      property;
    u8      u8_rsvd2;
    u8      with_aux_chain_ind;
    u8      prd_1st_pkt_dataLen;

    u8      chain_ind_num;
    u8      coding_ind;
    u8      txPower_en_len;
    u8      acad_chaged;

    u16     advInter_min;
    u16     advInter_max;

#if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    u32     responseAA;
    u8      num_subevents;        //0x00~0x80: Number of subevents
    u8      subevent_interval;    //0x06 to 0xFF: unit 1.25ms
    u8      response_slot_delay;  //0x01 to 0xFE: unit 1.25ms
    u8      response_slot_spacing;//0x02 to 0xFF: unit 0.125ms

    u8      num_response_slots;   //0x00:No response slots; 0x01~0xFF: Number of subevent response slots
    u8      paSubEventCnt;
    subDataReq_subInfo_t subDataReq;

    u8  subevtAvaAdvDataLen; //Available length for Aux_Sync_Subevent_Ind' Adv Data filed
    u8  response_slot_idx; //response slot current index, start form 0.
    u16 subeventData_validPaEvt;
    pawra_subDataCfg_t pdaSubevtDataCtrl[128]; //ref core Spec, support max 128 subevents.

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  subSchTsk_fifo[PERD_ADV_FIFONUM];
    u8          subSchTsk_wptr;
    u8          subSchTsk_rptr;
    u8          maxLen_subeventData;
    u8          num_subeventData;

    sch_task_t  rspSchTsk_fifo;

    u8      rspChnIdx;
    u8      initAuxConnReq;
    u8      initSubevent;
    u8      subeventData_evtTrig;

    u8      rsp_slot_start;
    u8      rsp_slot_count;
    u16     u16_rsvd1;

    s32     sSlot_mark_subx;
    s32     sSlot_mark_end_subx;
    s32     sSlot_duration_rsp;
    s32     sSlot_subevent_itvl;

#endif

    u8*     dat_perdAdvData;
    u32     curLen_perdAdvData;
    u32     link_big_handle; //Associated with the big_handle


    u16     rfLen_255_pkt_us;
    u16     n_30us_chain_ind;

    u8      chain_ind_dataLen[CHAIN_PKT_MAX_NUM];


    u16     prd_evt_us;
    u16     aux_sync_tx_off_us; //Mark aux_sync task begin irq --> aux_sync_ind pkt 1st bit send

    u16     maxLen_perdAdvData;
    u8      schTsk_wptr;
    u8      schTsk_rptr;

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  schTsk_fifo[PERD_ADV_FIFONUM];

    u8      auxPtr_offset;
    u8      txPower_offset; //Mark the start offset of the ACAD field
    u8      ACAD_advData_offset;
    u8      ADI_offset;


    st_pda_t    pda_tx;


    ll_prd_adv_ind_header_t  prd_adv_1stPkt;  // 16B

}st_prd_adv_t;

typedef int  (*ll_perd_adv_acad_callback_t)(int);
extern ll_perd_adv_acad_callback_t              perd_adv_biginfo_update_cb;

extern  st_prd_adv_t        *global_pPerdadv;
extern  st_prd_adv_t        *blt_pPerdadv;

bool            blt_ll_advPeriodicChkDataItvl(st_prd_adv_t* cur_pPerdadv, u16 itvl, u8 phy);
int             blt_ll_buildPerdAdvSchedulerLinklist(void);
st_prd_adv_t*   blt_ll_search_existing_perdAdv_index_by_advHandle(u8 adv_handle);
u8              blt_ll_isPerdAdvEnable(u8 adv_handle);
u8              blt_ll_search_existing_perdAdv_index(u8 advSet_idx);
u8              blt_ll_search_existing_and_new_perdAdv_index(u8 advSet_idx);

st_prd_adv_t * blt_prdadv_search_existed_and_allocate_new_periodic_adv(u8 advHandle);
st_prd_adv_t * blt_prdadv_search_existed_periodic_adv(u8 advHandle);

void            blt_prdadv_updatePram(st_prd_adv_t *cur_pPerdadv);
void            blt_prdadv_updateAcadPram(st_prd_adv_t *cur_pPerdadv, u8 acad_used);
void            blt_prdadv_task_begin(st_prd_adv_t  *);
void            blt_prdadv_task_end(st_prd_adv_t    *);
ble_sts_t       blc_hci_le_setPeriodicAdvParam(hci_le_setPeriodicAdvParam_cmdParam_t* pCmdParam);
ble_sts_t       blc_hci_le_setPeriodicAdvData(adv_handle_t adv_handle, data_oper_t operation, u8 advData_len, u8 *advdata);


int         blt_prd_adv_interrupt_task (int, void *p);
int         blt_prd_adv_mainloop_task (int, void *p);

int         blt_prdadv_start(int slotTask_idx);
int         blt_ll_aux_syncinfo_update(int prdadv_idx);
int         blt_ll_ctrlPerdAdvChClassUpd(unsigned char *pChm);

void        blt_reset_prd_adv(void);
/******************************* prd_adv end *********************************************************************/




/******************************* prd_adv_wr end ******************************************************************/
#define PAWRA_RXFIFO_SIZE               288 //must be 16*n for both Eagle/Kite/Vulture
#define PAWRA_RXFIFO_NUM                8   //must be 2^n
#define PAWRA_RXFIFO_MASK               (PAWRA_RXFIFO_NUM - 1)

_attribute_aligned_(4)
typedef struct {
    u8      prd_adv_sel;
    u8      pawra_rx_dma_size;
    u8      rsvd[2];

    u32     slot_trigger_tick;
    u32     pawra_rx_dma_buff;

} pawra_mng_para_t;  //PAwR advertiser parameters

extern  _attribute_aligned_(4)  my_fifo_t               pawra_RxFifo;
extern  _attribute_aligned_(4)  pawra_mng_para_t        pawraMng;

int         blt_pawra_sub0_mark(int index);
int         blt_pawra_sub_prepare_connect(int index);
int         blt_pawra_subx_sch_build(void); //For PAwR-Advertiser subevent( not contain subevent0) task
int         blt_pawra_subx_interrupt_task (int flag, void*p);
int         blt_pawra_subx_start(int index);
int         blt_pawra_rsp_interrupt_task (int flag, void*p);
int         blt_pawra_subx_advdata_update(void);
int         blt_pawra_rsp_task_insert(int index);
int         blt_pawra_rsp_start(int index);
int         blt_pawra_rsp_post(void);
int         blt_pawra_rsp_slot_start(void);
int         blt_pawra_rsp_slot_post(void);
int         irq_pawra_rsp_slot_rx(void);
int         blt_pawra_rsp_sch_build(void); //For PAwR-Advertiser rsp_slots task
int         blt_pawra_task_conflic(sch_task_t *pTgtTsk, u8 curSchTaskOft);
int         blt_pawra_mainloop_task (int flag, void *p);
void        blt_pawra_mainloop(void);
int         blt_ll_periodicAdvSubeventDataReq(st_prd_adv_t *pPerdadv);

hci_le_setPeriodicAdvParamV2_retParam_t blc_hci_le_setPeriodicAdvParam_v2(hci_le_setPeriodicAdvParamV2_cmdParam_t* pCmdParam);
ble_sts_t   blc_hci_le_setPeriodicAdvSubeventData(hci_le_setPeridAdvSubeventData_cmdParam_t* pcmdParam, hci_le_setPeridAdvSubeventDataRetParams_t *pRetParams);
ble_sts_t   blc_hci_le_extended_createConnection_v2( hci_le_ext_createConnV2_cmdParam_t * pCmdParam);
/******************************* prd_adv_wr end ******************************************************************/




/******************************* pda_sync start ******************************************************************/

#if PDA_SYNC_EBQ
    #define         PDASYNC_TAIL_MARGIN_US                          50 // TODO: need check
#else
    #define         PDASYNC_TAIL_MARGIN_US                          20
#endif

_attribute_aligned_(4)
typedef struct {
    u8      pdA_sync_sel; //only used in irq
    u8      pdA_syncing;
    u8      pdA_cacheNum;
    u8      pdA_list_num;

    u8      pdA_synced_num;
    u8      pdASync_customTmoExpire;
    u8      user_peridAdv_maxNum;
    u8      pawr_acad_check;


    u32     prdadv_seqnum;
    u32     tick_pda_sync;
    u32     pdaSync_timeout_us;
}ll_prdadv_sync_t;  //periodic ADV Sync manage

extern ll_prdadv_sync_t     bltPdaSync;


#define         PDA_SYNC_TIMING_ADJUST_EN                       1
#define         PAWR_SYNC_TIMING_ADJUST_EN                      1

#if (PDA_SYNC_TIMING_ADJUST_EN)

typedef struct{
    u32 rx_1st_tick;

    s16 sSlot_offset;
    u16 timing_update;
}pda_syncTiming_t;

typedef struct
{
    u8  maxNum_pawrSync;
    u8  pawrSync_sel;
}ll_pawrSync_mng_t;

extern pda_syncTiming_t  pda_sync_timingAdjust[];
extern pda_syncTiming_t  pawr_sync_timingAdjust[];

#endif



#define         PRDADV_SYNC_FIFONUM                             12
#define         PAWR_SYNC_FIFONUM                               12 //min interval is 7.5ms



#define SYNC_STATE_IDLE                 0
#define SYNC_STATE_WAIT_SYNC_INFO       BIT(0)
#define SYNC_STATE_SYNC_INFO_MATCH      BIT(1)
#define SYNC_STATE_SYNCING              BIT(2)
#define SYNC_STATE_SYNCED               BIT(3)

#define SYNC_STATE_PAWR_MARK            BIT(7)


#define PDA_CREATE_SYNC_BY_HCICMD       0
#define PDA_CREATE_SYNC_BY_PAST         1
#define SYNC_ESTABLISHED_BY_HCICMD      BIT(0)
#define SYNC_ESTABLISHED_BY_PAST        BIT(1)



#define PDA_SYNC_REPORT_DATA_COMPLETE       0x00
#define PDA_SYNC_REPORT_DATA_INCOMPLETE     0x01
#define PDA_SYNC_REPORT_DATA_TRUNCATED      0x02


////////////////////

#define PADVB_PACKET_FLAG               BIT(0)
#define PAWR_PACKET_FLAG                BIT(1)

_attribute_aligned_(4)
typedef struct {
    u8      pda_index;
    u8      sync_state;  //when pda_syncing_flg none zero, it may be wait_sync_info, sync_info_matsch or sync_ing
    u8      mapping_auxscan_idx;//used to find the related secChnScn_tbl
    u8      mapping_cache_idx;

    u8      sync_establish;
    u8      sync_lost;
    u8      sync_err_cnt;
    u8      terminate;

    u8      terminate_pending;
    u16     sync_adv_dup_filter; //mark DID, filter used
    u8      adrIdType; //adv_addr_type: 0,1,2,3: "LE Periodic Advertising Sync Transfer Received event" used

    u8      sync_specify;
    u8      sca;
    u8      sync_rcv_enable; //refer to 'sync_adv_rcve_en_msk'
    u8      sync_report_allow;  


    u16     pda_interval;  //1.25mS unit
    u16     pda_duration_us;
    u16     tolerance_pda_us;
    u16     sync_early_set_us;
    u16     max_pda_skip;
    u16     max_err_us_per_second;
    u16     pda_pkt_us;
    u16     skip_cnt;


    s32     sSlot_mark_prdadv;
//  u32     pda_expect_tick; //no need
    u32     pdaInterval_tick;
    u32     pda_crcinit;
    u32     sync_timeout_tick;
    u32     sync_rx_tick;

    u8      pda_reportInitDis; //no need, TODO:remove latter
    u8      createSyncType; //PAST_createSync or PeriodicAdvCreateSync
    u16     pdaSync_srx1stTimeout_us;

    st_pda_t    pda_rx;

    extadv_id_t     pda_id;       //8B
//  sync_info_t     pda_syncInfo;    //18B
//  u8      u8_rsvd2[2];

    u8      record_advA_adrType;
    u8      record_advA_addr[6];
    u8      pawr_acad_check;

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  schTsk_fifo[PRDADV_SYNC_FIFONUM];

#if(LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
    setPDARspData_cmdParam_t        pdaRspDataCtrl;  //12B//need to move from this structure? because at one time only one response slot is valid. is that correct?
    setPDASyncSubevt_cmdParam_t     pdaSyncSubevtCtrl;  //132B
    ll_auxSyncSubevtRsp_header_t    auxSyncSubevtRsp_header; //16B

    pawr_acad_t     pawr_acadInfo; //8B

    u8              subevtIdx_build;
    u8              subevtIdx_execute;
    u8              subevent_allExecute; //PerPAwRevt
    u8              preSubevtIdx;


    u16             prePawrEvtCnt;
    u16             u16_rsvd3;

    u16             subevtIntvl_sSlot;
    u16             sub_rspSlot_ChnIdx;

    u8*             pSubevtOfBuild;
#endif

#if (LL_FEATURE_ENABLE_LE_AOA_AOD)
    u8      create_sync_cte_type;
    u8      sync_wrong_cte_type;
    u8      sync_cte_IQ_valid;// RX buffer contain CTE IQ Sample data
    u8      sync_cte_chnIdx;

    u8      sync_cte_type;
    u8      sync_cte_time;
    u16     sync_cte_EvtCnt;
#endif

    bigInfor_para_t bigInfor_para;

}st_pda_sync_t;  //status of periodic ADV Sync
extern _attribute_aligned_(4)   st_pda_sync_t       pdAsync_tbl[];
extern _attribute_aligned_(4)   st_pda_sync_t       *blt_pPdAsync;


_attribute_ram_code_ void blc_ll_switch2PAwR_syncSubevt0(st_pda_sync_t* pPAwR_sync);
_attribute_ram_code_ void blt_ll_PAwRsync_build_task(void);
_attribute_ram_code_ void blt_ll_PAwRsync_subevtStart(u8 index);
_attribute_ram_code_ int  blt_ll_PAwRsync_rspSlotTxProc(int flag);
_attribute_ram_code_ int  blt_pawr_sync_sub_interrupt_task (int flag, void *p0, void* p1);
_attribute_noinline_ int  blt_pawr_sync_mainloop_task (int flag);
_attribute_noinline_ int  blt_pawr_sync_adv_rpt(void);



#define PERDADV_LIST_SIZE                       4


_attribute_aligned_(4)
typedef struct {
    extadv_id_t     list_dev_id;  //8B

    u8      synced_mark;
    u8      u8_rsvd[3];
} pda_list_t;
extern  pda_list_t  pdaList_tbl[];



#define PERDADV_CACHE_NUM                       4
#define PERDADV_CACHE_MASK                      3

#define CACHE_FLAG_IDLE                         0
#define CACHE_FLAG_OCCUPIED                     1
#define CACHE_FLAG_SYNCING                      2
#define CACHE_FLAG_SYNCED                       3

_attribute_aligned_(4)
typedef struct {
    u8      cach_index;
    u8      cach_flag;
    u8      prdphy;    //"le_phy_type_t"
    u8      record_advA_adrType;

    u8      record_advA_addr[6];
    u8      pawr_acad_valid;
    u8      u8_rsvd1;

//  u16     u16_rsvd1;

    u32     seq_number;
    u32     header_tick_backup;

    extadv_id_t     pda_dev_id;//8B
    sync_info_t     sncInf;    //18B
    u8      u8_rsvd[2];

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
    pawr_acad_t     pawr_acad; //8B
    #endif

    u32     syncWwUs; /*!< window widen for PAST recipient: (16+D)*2+U. Refer to <<Core5.3>>, Page2868 */

} pda_cache_t; //periodic ADV cache
extern  pda_cache_t pdaCache_tbl[];









void    blt_reset_pda_sync(void);

int     blt_pda_sync_interrupt_task (int, void*);
int     blt_pda_sync_mainloop_task (int);

void    blt_pda_sync_build_task(void);
void    blt_pda_sync_start(u8);
void    blt_pda_sync_post(void);
void    blt_pda_sync_rx(rf_pkt_ext_adv_t*);
void    blt_ll_pda_sync_sslot_reset(void);

int     blt_pda_sync_main_loop(void);
void    blt_pda_sync_check_timeout(void);
int     blt_pda_sync_prdadv_data_report(void);

void    blt_pda_sync_analyze_prdadv_info(st_pda_sync_t *, pda_cache_t *);
bool    blt_pda_sync_info_process(sync_info_t *);

bool    blt_isSyncHandleValid (u16 sync_handle);


ble_sts_t   blc_hci_le_periodic_advertising_create_sync (hci_le_periodicAdvCreateSync_cmdParam_t* cmdPara);

/******************************* pda_sync end ********************************************************************/





#endif /* STACK_BLE_CONTROLLER_LL_PRDADV_PDA_STACK_H_ */
