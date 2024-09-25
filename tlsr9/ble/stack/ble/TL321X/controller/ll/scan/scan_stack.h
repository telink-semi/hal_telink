/********************************************************************************************************
 * @file    scan_stack.h
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
#ifndef SCAN_STACK_H_
#define SCAN_STACK_H_

#include "stack/ble/hci/hci_cmd.h"
#include "stack/ble/controller/ll/ll_stack.h"




/* leg_adv enable trigger by "blc_ll_setScanEnable" only
 * e.g. M4S4, even master connection number is 4, leg_scan still work  */
#ifndef         SCAN_EN_MORE_STRATEGY
#define         SCAN_EN_MORE_STRATEGY                           1
#endif


#ifndef         TASK_VERY_CLOSE_DROP_EN
#define         TASK_VERY_CLOSE_DROP_EN                         0
#endif

/******************************* scan start ******************************************************************/
/**
 * <<Core5.2>> Refer to Vol6, Part B, Section 4.4.3.2. Active scanning
 *  backoff procedure
 */
#define         SCAN_BACKOFF_FEATURE_EN                         0


/* active scan sending scan_req flow control method */
#define         SCANREQ_NO_CTRL                                 0
#define         SCANRSP_DEVICE_TBL                              1  // Telink used for a long time
#define         BACKOFF_ALGORITHM                               2


#if (BQB_TEST_EN)
    #define         LEGSCAN_SCANREQ_FLOW_CTRL                   SCANREQ_NO_CTRL
#else
    #define         LEGSCAN_SCANREQ_FLOW_CTRL                   SCANRSP_DEVICE_TBL
#endif



#define         SCNTSK_FIFO_NUM                                 1

#define         CHN_INDEX_INIT                                  -1
#define         CHN_INDEX_37                                    0
#define         CHN_INDEX_38                                    1
#define         CHN_INDEX_39                                    2


#define         NEW_SCAN_DEVICE_STORAGE                         0

_attribute_aligned_(4)
typedef struct {
    u8      evt_type;  //now "u8" is enough for BLE 5.2
    u8      adr_type;
    u8      addr[6];
}scan_adv_t;


_attribute_aligned_(4)
typedef struct {
    u8      bigEncrypt; //encrypt flag
    u8      bigInfor_flag;
    u8      creating_bisSync_flag;
    u8      rsvd1;

    u32     bSlot_idx_Rx;
    s32     sSlot_idx_Rx;
    u32     bigInfor_rx_tick;

    u8      bigInfo[60];     //33/57bytes 4 align so set60,
}bigInfor_para_t;



typedef struct {
    u8      sid;
    u8      adrType;
    u8      addr[6];
}extadv_id_t;  //extended ADV identity



_attribute_aligned_(4)
typedef struct {
    u8      scan_type;
    u8      scan_percent;  //to optimize code running effect, Denominator set to 128, "*128" -> "<<7",  "/128" -> ">>7"
    s8      chn_index;
    s8      phy_index;    //0: 1M; 1: 2M; 2: Coded


    u8      scanPhy_msk;
    u8      scan_en_strategy;
    u8      duplicate_filter;
    u8      scan_filterPolicy;

    u8      scan_fp_wl;
    u8      scan_fp_targetA_rpaPass;  //for policy 0x01/x03, if targetA is RPA, pass without any check
    u8      scan_ownAddr_random;
    u8      scan_ownAddr_rpa;

    u8      direct_adv;
    u8      direct_initA_rpa_resolve_fail; //direct initA is RPA, resolve fail. scan filter policy is 2/3, accept no care RPA result
    u8      u8_rsvd1[2];

    u8      scan_mac_type;
    u8      scan_mac_addr[6];
    u8      ownAddrType;

    u8      scanDevice_num;
    u8      scanRspDevice_num;
    u8      initiate_going;
    u8      pda_syncing_idx;

    //12
    u8      durationPeriod_stateFlag;
    u8      extScan_1stFlag;
    u8      extScanType[3];
    u8      scanPercent[3];

    u16     scanInter[3];
    u16     scanInterval;  //unit: 625 uS, u8 max 160mS not enough,  u16 max 40.96S enough
//  u16     u16_rsvd;

#if(SCAN_BACKOFF_FEATURE_EN)
    u16     upperLimit;
    u16     backoffCount;

    u8      scnRspSucc;
    u8      scnRspFail;
    u16     u16_backoffRsvd;
#endif


    u32     extScan_duration;
    u32     extScan_period;
    u32     extScan_startTick;


    u32     scanInte_tick[3];  //1M/2M/Coded
    u32     scnInterval_tick;
    u32     tick_scan;
    u32     bSlot_mark_scan;
    u32     last_scan_end_time;
    u32     scan_post_tick;
    u32     scan_post_margin;
    u32     early_stop_tick;

    u32     scan_rx_pri_chn_dma_buff;

#if (LL_ASYNC_LEA_EN)
    u32     asyncScanIndex;
#endif

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  scnTsk_fifo[SCNTSK_FIFO_NUM];

}ll_scn_t;

extern  _attribute_aligned_(4) ll_scn_t  bltScn;





#if(SCHE_PRE_ALLOCATE_MAX_LEN == SCHE_PRE_ALLOCATE_LEN_80MS)
    #define         SCNALIGN_FIFO_NUM               4  //scan interval no less than 20mS
#else
    #error "SCNALIGN FIFO NUM error!!!"
#endif


_attribute_aligned_(4)
typedef struct {
    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  scnAlign_fifo[SCNALIGN_FIFO_NUM];
}ll_scn_align_t;

extern  _attribute_aligned_(4) ll_scn_align_t  bltAlignScn;



#define EXTSCAN_PERIOD_CHECK_PENDING        BIT(0)
#define EXTSCAN_DURATION_CHECK_PENDING      BIT(1)


#define SCAN_PRICHN_RXFIFO_NUM              8   //must be 2^n   //TODO: 4 is enough, Kite/Vulture can use 4
#define SCAN_PRICHN_RXFIFO_MASK             7


#define SCAN_PRICHN_RXFIFO_SIZE             64  //37+24(maybe 17, 24 is come from 826x,) Eagle: 37+21,16B align(dma rx size)
#define SCAN_PRICHN_RX_DMA_SIZE             4   //64>>4







#define SCAN_SECCHN_RXFIFO_SIZE             288 //must be 16*n for both Eagle/Kite/Vulture
#define SCAN_SECCHN_RXFIFO_NUM              8   //must be 2^n //the number of chain packet is large---EBQ
#define SCAN_SECCHN_RXFIFO_MASK             (SCAN_SECCHN_RXFIFO_NUM - 1)




typedef struct{
    u32 dma_len;

    u8  type   :4;
    u8  rfu1   :1;
    u8  chan_sel:1;
    u8  txAddr :1;
    u8  rxAddr :1;

    u8  rf_len;             //LEN(6)_RFU(2)

    u8  scanA[6];           //
    u8  advA[6];            //
}rf_packet_scan_req_t;
extern rf_packet_scan_req_t pkt_scanReq;


typedef int  (*ll_rx_pkt_callback_t)(u8 *);
extern ll_rx_pkt_callback_t   ll_prichn_initPkt_cb;
extern ll_rx_pkt_callback_t   ll_secchn_initPkt_cb;
extern ll_rx_pkt_callback_t   ll_extadv_pkt_cb;



void blt_ll_initScanningCommon(void);


#if (NEW_SCAN_DEVICE_STORAGE)
    scan_adv_t  blt_ll_filterAdvDevice (u8 type, u8 * mac);
#else
    int blt_ll_filterAdvDevice (u8 type, u8 * mac);
#endif


int     blt_ll_addScanRspDevice(u8 type, u8 *mac);
void    blt_ll_clearScanRspDevice(void);
bool    blt_ll_isScanRspReceived(u8 type, u8 *mac);


u32     blt_ll_calculateScanDuration (u32 scan_tick_left);

void    blt_set_prichn_scan_start(void);
int     blt_ll_prichn_scan_insert(void);
void    blt_ll_prichn_scan_post(void);

int     blt_ll_buildPrimaryChannelScanTask(void);


void    blt_setScan_cal_chn_phy (int next_chn); //set primary channel scan step 1: calculate channel and PHY
void    blt_setScan_enter_manual_rx(void);      //set primary channel scan step 2: enter manual RX

u32     blt_quick_tx_prepare(fsm_mode_e, void*, u8);

int     irq_scan_rx_primary_channel(void);


int     blt_ll_prichn_scan_align_build(void);

/******************************* scan end ********************************************************************/









/******************************* leg_scan start ******************************************************************/

#define         BLMS_SCAN_SLOT                                  56  //56*0.625 = 35mS
#define         BLMS_SCAN_INTERVL                               ( BLMS_SCAN_SLOT * SYSTEM_TIMER_TICK_625US)

#define         BLMS_PERCENT_MAX                                128




int     blt_prichn_scan_interrupt_task (int flag);
int     blt_leg_scan_mainloop_task (int flag);

int     blt_ll_procLegacyScanData(void);
void    blt_reset_leg_scan(void);

ble_sts_t   blc_hci_le_setScanParameter(hci_le_setScanParam_cmdParam_t *);
ble_sts_t   blc_hci_le_setScanEnable(hci_le_setScanEnable_cmdParam_t *);
/******************************* leg_scan end ********************************************************************/










/******************************* ext_scan start ******************************************************************/
//(EXTADV_RPT_DATA_LEN_MAX - 1) = 229 - 1 for ext adv;
//246 for periodic adv report. use the same hold buffer.
#define         EXTADV_PDA_RPT_DATA_HOLD_MAX_LEN                246//228


#define         AUXPTR_FLAG_EXIST                               BIT(0)
#define         AUXPTR_TIME_CORRECT                             BIT(1)
#define         AUXPTR_INFO_VALID                               (AUXPTR_FLAG_EXIST | AUXPTR_TIME_CORRECT)


//#define       PKTDATA2_FLAG_SCAN_RSP                          BIT(7)
//#define       PKTDATA2_FLAG_DATA_STATUS_MASK                  (BIT(6) | BIT(5))

#define         PKTDATA2_FLAG_INDEX_MASK                        0x07


#define         SYS_IRQ_TRIG_BTX_POST                           BIT(0)
#define         EXTSCAN_PREPARE_US                              100   //TODO: test which value is best, relative with RX settle time
#define         EXTSCAN_TAIL_MARGIN_US                          60    //RX IRQ delay + some other timing: TODO, need debug with sniffer GPIO
#define         ACTIVE_SCAN_EXTRA_US                            (176+150+20)//(12B+10B other)*8; 150--T_IFS;20 for parse aux scan response packet.
#define         ACTIVE_SCAN_EXTRA_US_CODED                      (1408+150+20)//(12B+10B other)*8*8; 150--T_IFS;20 for parse aux scan response packet.

enum{
    RTN_SUCCESS,
    RTN_DROP,
    RTN_BREAK,
};
_attribute_aligned_(4)
typedef struct{
    u8      prichn_phy;
    u8      tx_power_rpt;  //tx_power report
    u8      direct_flag;
    u8      directA_rpa_resolve_fail; //direct initA is RPA, resolve fail. scan filter policy is 2/3, accept no care RPA result

    u8      rpt_addr_type;
    u8      u8_rsvd[3];

} st_prichn_scn_t;

extern  _attribute_aligned_(4)  st_prichn_scn_t         priChnScn_tbl[];
extern  _attribute_aligned_(4)  st_prichn_scn_t         *blt_pPrichnScn;



_attribute_aligned_(4)
typedef struct {
    /* special design begin, for fast parameters check */
    /* attention: must be 4B aligned*/
    u8      extHdrFlg;
    u8      aux_chnIdx;
    u8      aux_ca;
    u8      aux_offsetUnit;

    u8      aux_secPhy;  // attention: 1/2/3   "le_phy_type_t"
    u8      rsvd;
    u16     ext_adiInfo;
    /* special design end */
}ext_pkt_info_t;




#define EXT_ADV_REPORT_TRANCATED_DATA_PROCESS_EN                        1




#define SECCHN_IDX_MARK                         BIT(7)
#define BISSYNC_IDX_MARK                        BIT(7)

#define BLT_SYNC_HANDLE                         BIT(3)
#define BLT_SYNC_IDX_MARK                       (0x07)


#define SCANRX_IDX_MASK                         0x07    //BIT(0..2)

//if both SCANRX_FLAG_FIRST_DATA and SCANRX_FLAG_LAST_DATA are set, that indicate only one packet,no chain packet.
//The packet is both the first and last packet.
#define SCANRX_FLAG_FIRST_DATA                  BIT(0)
#define SCANRX_FLAG_LAST_DATA                   BIT(1)

#define SCANRX_FLAG_BIGINFOR                    BIT(2)  //exist big information in period ADV
#define SCANRX_FLAG_PDA                         BIT(3)  //periodic ADV data

#define SCANRX_FLAG_REPORT2HOST                 BIT(4) //indicate there has been some packet reported to host.
#define SCANRX_FLAG_DATA_DROP                   BIT(5)

#define SCANRX_FALG_PAWR                        BIT(6) //Above value need to be stored in raw_pkt[2] and used in mainloop

#define SCANRX_FLAG_EXTSCAN_TRUNCATED_PEND      BIT(8)  //6;not store raw_pkt, only used in scan_rx_flag(u16)
#define SCANRX_FLAG_PDASCAN_TRUNCATED_PEND      BIT(9)  //7;not store raw_pkt, only used in scan_rx_flag(u16)


#define DURATION_FLAG_MAX_TIME                  BIT(0)
#define DURATION_FLAG_MID_TIME                  BIT(1)
#define DURATION_FLAG_MIN_TIME                  BIT(2)

typedef struct{
    u8 addr_type;
    u8 address[6];
}addrInforBackup_t;



_attribute_aligned_(4)
typedef struct {
//0x00
    u8      scnIndex; //second table index ---secChnScn_tbl
    u8      occupied;
    u8      pdaSync_flag;
    u8      pdaSync_idx; //used to find the related pdAsync_tbl[]

    u8      rfLen_max;
    u8      scan_advMode;
    u8      aux_scan_cnt;
    u8      aux_scnRsp_chain_flag;


    u8      next_chnIdx;
    u8      scan_duration_flag;
    u8      peerAdv_txPower;
    u8      prichn_phy;

    u8      secchn_phy;          //"le_phy_type_t"    1:1M    2:2M   3:Coded
    u8      aux_chain_flag;      //must clear when new aux_task start
    u8      rpt_data_pending;    //not used now
    u8      advrpt_hold_dat_len; //must clear when new aux_task start

//0x10
    u8      peerAdv_datOffset;
    u8      peerTargetA_exist;
    u8      peerAdvA_exist;
    u8      ext_event_type_8bit;  //must clear when new aux_task start

    u8      advrpt_holdDataOffset;
    u8      direct_adrType;
    u8      direct_addr[6];

    extadv_id_t     peerAdv_id;

    //privacy
    u8      prichn_advA_exist;
    u8      prichn_targetA_exist;

    u8      secchn_advA_exist;
    u8      secchn_targetA_exist;

    u8      total_advA_exist;
    u8      total_targetA_exist;

    /* be packet address or IDA for RPA resolve success */
    u8      record_advA_adrType;
    u8      record_advA_addr[6];
//  u8      u8_rsvd;

    u8      advA_rpt_adrType;
    u8      advA_rpt_addr[6];
//  u8      u8_rsvd;

    u8      record_direct_adrType;
    u8      record_direct_addr[6];
    u8      direct_rpa_resolve_fail; //direct initA is RPA, resolve fail. scan filter policy is 2/3, accept no care RPA result

    ll_resolv_list_t *pRslvlst_extScn;
    //end privacy

    u16     scan_adi;
    u16     peerAdv_datLen;
    u16     tolerance_peer_us; //max 66356 uS, enough
    u16     scan_early_set_us;
    u16     scan_duration_us;
    u16     scan_duration_max_us;
    u16     perdAdv_interval;
    u16     scan_rx_flag;

    u16     aux_pkt_1stRxTm_us;
    u16     u16_rsvd0;

    u32     scan_tolerance_tick;  //not used now
    u32     aux_expect_tick;
    u32     aux_irq_tick;

    u32     pawr_rsp_tick;

    ext_pkt_info_t  extPkt_info;  //attention: 4B aligned

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  auxScnTsk;

}st_secchn_scn_t;
extern _attribute_aligned_(4)   st_secchn_scn_t     secChnScn_tbl[];
extern _attribute_aligned_(4)   st_secchn_scn_t     *blt_pSecChnScn;


_attribute_aligned_(4)
typedef struct {
    u8      rx_received;
}auxscn_common_para_t;  //aux_scan common status parameters
extern _attribute_aligned_(4) auxscn_common_para_t  auxScnCmnParam;

#define         PHY_INDEX_1M                                    0
#define         PHY_INDEX_2M                                    1
#define         PHY_INDEX_CODED                                 2


_attribute_aligned_(4)
typedef struct {
    u8      pri_phy;
    u8      cur_phy; //
    u8      auxList_taskNum;
    u8      auxadv_sel;      //can optimize, not use

    u8      scan_rx_sec_chn_dma_size;  //optimize: constant value, never change
    u8      truncated_scan_msk;
    u8      prdAdv_syncFlg;  //no need, use pdaSync_flag to replace
    u8      u8_rsvd;



    u32     scan_rx_sec_chn_dma_buff;
}ll_extscn_t;

extern ll_extscn_t      bltExtScn;






#define  AUX_SCAN_FUTURE_TASK_MAX_NUM        (TSKNUM_SECCHN_SCAN)

_attribute_aligned_(4)
typedef struct {
    u8  task_idx;  //scheTask_idx
    u8  task_oft;
    u8  u8_rsvd[2];

    u32 tick_s;
    u32 tick_e;
}fut_task_type_2;  //auxiliary scan future task

_attribute_aligned_(4)
typedef struct {
    fut_task_type_2 auxScan_tbl[AUX_SCAN_FUTURE_TASK_MAX_NUM];

    u8  number;
}aux_scn_fut_task_t; //auxiliary scan future task
extern aux_scn_fut_task_t   AuxScnFutTask;






#define         NUM_OF_EXT_SCAN_DEVICE              16

_attribute_aligned_(4)
typedef struct {
    u8      dev_sid;
    u8      dev_did;


    u8      u8_rsvd2;
    u8      dev_adrType;
    u8      dev_addr[6];

    u32     report_tick;
}scan_dev_t;




static inline void blt_set_auxscan_enable(st_secchn_scn_t * pAuxScn, int en)
{
    pAuxScn->occupied = en;
    //pAuxScn->scan_rx_flag = 0; ///clear relevant variable. enable or disable.
    pAuxScn->aux_scnRsp_chain_flag = 0;
    pAuxScn->aux_pkt_1stRxTm_us = 0;
}


int irq_scan_rx_secondary_channel(void);






/**
 * @brief      this function is used to set the extended scan parameters to be used on the advertising physical channels.
 * @param[in]  pCmdParam - point to a buffer for extended scan parameters.
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_hci_le_setExtScanParam (hci_le_setExtScanParam_cmdParam_t* pCmdParam);
ble_sts_t   blc_hci_le_setExtScanEnable( hci_le_setExtScanEnable_cmdParam_t * pCmdParam);

int     blt_ext_scan_interrupt_task (int, void*);
int     blt_ext_scan_mainloop_task (int);

int     blt_ll_procAuxiliaryScanTask (int sequence);
void    blt_ll_switchExtScanChannel (int set_chn);
int     blt_ext_adv_rx_process(u8 *raw_pkt);
u8      blt_extscan_convert_direct_adr_type(u8 direct_adrType, u8 *direct_addr, u8 rpa_resole_fail);
int     blt_ll_procExtAdvReportEvent(void);
int     blt_ll_procAuxScanTruncatedPend(void);
int     blt_ll_procPdaScanTruncatedPend(void);
void    blt_ll_ExtScan_mainloop_checkPendTask(void);
void    blt_reset_ext_scan(void);


void    blt_aux_scan_start (int aux_idx);
void    blt_aux_scan_post (void);

void    blt_release_secchn_scan(st_secchn_scn_t *, u8);

void    blt_add_aux_scan_future_task(u8, u8, u32, u32);
bool    blt_remove_aux_scan_future_task(u8);
int     blt_check_aux_scan_future_task(u32 );
int     blt_insert_aux_scan_future_task(void);

u8*     blt_ext_scan_searchAcadInfos(u8* pAcad, int acad_len, data_type_t advType);
/******************************* ext_scan end ********************************************************************/




void    blt_set_scan_default(void);
void    blt_set_leg_scan_default(void);
void    blt_set_ext_scan_default(void);






#endif /* SCAN_STACK_H_ */
