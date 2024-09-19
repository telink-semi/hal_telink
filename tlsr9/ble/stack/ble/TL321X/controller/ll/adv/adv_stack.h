/********************************************************************************************************
 * @file    adv_stack.h
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
#ifndef ADV_STACK_H_
#define ADV_STACK_H_



#include "stack/ble/hci/hci_cmd.h"
#include "stack/ble/ble_stack.h"
#include "stack/ble/ble_format.h"
#include "stack/ble/controller/csa/csa_stack.h"
#include "stack/ble/controller/whitelist/whitelist_stack.h"

/******************************* adv start ******************************************************************/

/* NOTICE THAT: note below is copied from B85m single connection SDK, there might be some code differences between multiple
 *              connection SDK and single connection SDK, but the principle is the same.
 *
 * direct ADV
 * If peer device use RPA, or peer device may use RPA(means that peer device distribute valid IRK but first
 * pairing used IDA), standard code writing method on host(application layer) is too complex for user. include:
 *          blc_ll_addDeviceToResolvingList(peer_id_adrType, peer_id_addr, peer_irk, NULL);
 *          blc ll_setAddressResolutionEnable(1);
 *          app_own_address_type |= OWN_ADDRESS_TYPE_RPA_MASK; //very hard to understand, but it's reasonable !!!
 *          bls ll_setAdvParam( XXX, XXX, ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY, app_own_address_type, peer_id_adrType,  peer_id_addr, XXX, XXX);
 * If do not do above, in "blt_send_adv", code below can not pass:
 *          if(bltLegAdv.cur_directAdv){ //direct ADV, do not care about filter
 *               if(smemcmp(bltAddr.peer_pka_or_ida_addr, bltLegAdv.advParaCmd_peerAddr, BLE_ADDR_LEN) || \
 *                          bltAddr.peer_pka_or_ida_type != bltLegAdv.advParaCmd_peerAdrType){
 *                   break;
 *               }
 *          }
 *
 *  To solve the conflict above, we add a special design for controller, simplify user's operation, process direct ADV RPA check.
 *  code below are still needed, consider that all the parameters reported in enhanced connection complete event.
 *            blc ll_addDeviceToResolvingList(peer_id_adrType, peer_id_addr, peer_irk, NULL);
 *          blc ll_setAddressResolutionEnable(1);
 * If these two code above not exist, same error as SDK version V3.4.2.2 and earlier.
 *  For standard BQB test, recommend to disable this special design. So use a special macro.
*/
#if (BQB_TEST_EN) //FOR BQB: disable this special design for check peer initA RPA
#define DIRECT_ADV_CHECK_PEER_INITA_RPA_SPECIAL_DESIGN          0
#endif

#ifndef DIRECT_ADV_CHECK_PEER_INITA_RPA_SPECIAL_DESIGN
#define DIRECT_ADV_CHECK_PEER_INITA_RPA_SPECIAL_DESIGN          1
#endif



#ifndef         LEG_ADV_DELAY_CTRL_EN
#define         LEG_ADV_DELAY_CTRL_EN                           1
#endif

#ifndef         EXT_ADV_DELAY_CTRL_EN
#define         EXT_ADV_DELAY_CTRL_EN                           0
#endif

/* leg_adv enable trigger by "blc_ll_setAdvEnable" or corresponding HCI command only
 * e.g. M4S4, even slave connection number is 4, leg_adv still work but can not be connected */
#ifndef         LEG_ADV_EN_MORE_STRATEGY
#define         LEG_ADV_EN_MORE_STRATEGY                        1
#endif

/*
 * Extended adv only use strategy_3, Refer to the description of LEG_ADV_EN_STRATEGY_3
 * LEG_ADV_EN_STRATEGY_3: ADV stop when peripheral connect, upper layer should set ADV enable again if needing another slave connected
 */
#ifndef         EXT_ADV_EN_MORE_STRATEGY
#define         EXT_ADV_EN_MORE_STRATEGY                        1
#endif

#define         BLC_FLAG_STK_ADV                                BIT(24)


#define         ADV_DFT_TX_DELAY_US                             50


#define         ADV_TAIL_MARGIN_US                              30

/*
         *   1M PHY   :    (rf_len + 10) * 8,      // 10 = 1(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
         *   2M PHY   :    (rf_len + 11) * 4       // 11 = 2(BLE preamble) + 9(accesscode 4 + crc 3 + header 2)
         *  Coded PHY :    376 + (rf_len*8+43)*S // 376uS = 80uS(preamble) + 256uS(Access Code) + 16uS(CI) + 24uS(TERM1)
*/
#define         AUX_CONN_REQ_1MPHY_US                           352   // (34 + 10) * 8 = 352
#define         AUX_CONN_REQ_2MPHY_US                           180   // (34 + 11) * 4 = 180
#define         AUX_CONN_REQ_CODEDPHY_S2_US                     1006  // 376 + (34*8+43)*2=376 + 315*2 =1006
#define         AUX_CONN_REQ_CODEDPHY_S8_US                     2896  // 376 + (34*8+43)*8=376 + 315*8 =2896
#define         AUX_CONN_RSP_1MPHY_US                           192   // (14 + 10) * 8 = 192
#define         AUX_CONN_RSP_2MPHY_US                           100    // (14 + 11) * 4 =  100
#define         AUX_CONN_RSP_CODEDPHY_S2_US                     686   // 376 + (14*8+43)*2=376 + 155*2 =686
#define         AUX_CONN_RSP_CODEDPHY_S8_US                     1616  // 376 + (14*8+43)*8=376 + 155*8 =1616




//#define       RFLEN_255_1MPHY_US                              2120
//#define       RFLEN_255_2MPHY_US                              1064
//#define       RFLEN_255_CODEDPHY_S2_US                        4542
//#define       RFLEN_255_CODEDPHY_S8_US                        17040

#define         RFLEN_255_1MPHY_PKT_US                              2430 //2120+300=2420, 81*30=2430, 10uS margin
#define         RFLEN_255_1MPHY_N_30                                81

#define         RFLEN_255_2MPHY_PKT_US                              1380 //1064+300=1364, 46*30=1380, 16uS margin
#define         RFLEN_255_2MPHY_N_30                                46

#define         RFLEN_255_CODEDPHY_S2_PKT_US                        4860 //4542+300=4842, 162*30=4860, 18uS margin
#define         RFLEN_255_CODEDPHY_S2_N_30                          162

#define         RFLEN_255_CODEDPHY_S8_PKT_US                        17370 //17040+300=17340, 579*30=17370, 30uS margin
#define         RFLEN_255_CODEDPHY_S8_N_30                          579



_attribute_aligned_(4)
typedef struct {
    u8      advTxDly_us;
    u8      adv_scanReq_connReq;
    u8      legAdv_chngReason;  //this variable is for legAdv, but I put it here to save legAdv main_loop process.
    u8      blc_continue_adv_en;//default stop sending legacy advertising packets when receiving scan request in the current adv interval.

    u8      advChn_idx;
    u8      advChn_cnt;
    u16     rsvd0_u16;


    u16     delay_sSlot_mask;
    u16     delay_sSlot_value;

    u32     adv_irq_tick;


}ll_adv_t;
extern ll_adv_t bltAdv;





typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;             //LEN(6)_RFU(2)

    u8  advA[6];            //address
    u8  data[31];
}rf_packet_adv_t;
extern rf_packet_adv_t  pkt_Adv;


typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  advA[6];
    u8  targetA[6];
}rf_pkt_adv_direct_ind_t;



typedef struct{
    u32 dma_len;

    u8  type   :4;
    u8  rfu1   :1;
    u8  chan_sel:1;
    u8  txAddr :1;
    u8  rxAddr :1;

    u8  rf_len;

    u8  peerA[6];  //scanA or initA
    u8  advA[6];
}rf_pkt_adv_rx_t;



typedef struct{
    u32 dma_len;

    u8  type   :4;
    u8  rfu1   :1;
    u8  chan_sel:1;
    u8  txAddr :1;
    u8  rxAddr :1;

    u8  rf_len;             //LEN(6)_RFU(2)

    u8  advA[6];            //address
    u8  data[31];           //0-31 byte
}rf_packet_scan_rsp_t;
extern rf_packet_scan_rsp_t pkt_scanRsp;






extern  u32     blt_advExpectTime;
extern  u32     blc_rcvd_connReq_tick;


void blt_ll_initAdvertisingCommon(void);
void blt_ll_reset_adv_common(void);

ble_sts_t blc_hci_le_setDataRelatedAddressChange(hci_le_setDataAddrChange_cmdParams_t *);

/******************************* adv end ********************************************************************/







/******************************* leg_adv start ******************************************************************/



/******************************************************************************************************************************************************************
ADV data 37 bytes, Scan_Rsp 16 bytes, 32M clock,  all code running in SRAM, no Scan_Req/Conn_Req, cost 2353 uS
ADV data 37 bytes, Scan_Rsp 16 bytes, 32M clock,  all code running in SRAM, no Scan_Req, Conn_Req on chn39, 2700 uS (2019.09.03 SiHui test data)

ADV data 37 bytes, Scan_Rsp 37 bytes, 32M clock,  all code running in SRAM, no Scan_Req on chn37/38,
            Scan_Req&Scan_Rsp on chn39, 2940 uS -> 2890 uS (400->375 optimize) (2019.09.17 SiHui test data)

Worst Case:
PDU 37 bytes, 32M clock,  all code running in SRAM, receive scan_req/conn_req on chn37 failed,
              receive scan_req/conn_req on chn38 failed, receive scan_req success and send scan_rsp on chn39
                this timing is about: 2890+300*2 = 3490

*****************************************************************************************************************************************************************/
//TODO, optimize if ADV channel is not 37/38/39, or if ADV data & Scan_Rsp data not longest
//TODO, if none connecTable ADV or direct_adv, duration can be smaller
#define         MAX_SSLOT_DURATION_ADV                          179   //3490*16*2/625=178.7

#define         MAX_LEG_ADV_EVT_US                              3500




#define         ADVTSK_FIFO_NUM                                 8  //must be 2^n

_attribute_aligned_(4)
typedef struct {
//0x00
    u8      adv_chn_mask;
    u8      adv_duration_en;
    u8      adv_type;
    u8      adv_filterPolicy;


    u8      sSlotDuration_adv;  //for legacy ADV, u8 is enough, max 5mS, but extended ADV need "u16"
    u8      sSlot_diff_adv;
    u8      legadv_ownAddr_type;        //change to legadv_ownAddr_random after confirm Spec adv_enable ownTYpe 0x01/0x03 issue
    u8      legadv_ownAddr_rpa;

    u8      fifoIdx_mark;
    u8      adv_curDevIdx;  //for multiple local device
    u8      scanrsp_data_set;
    u8      backup_rfLen;

    u8      adv_data_set;
    u8      high_duty_direct;
    u8      cur_directAdv;  //current ADV type is direct ADV
    u8      last_directAdv; //last ADV type is direct ADV


//0x10
    u8      scnReq_response; //ADV_SCAN_IND or ADV_IND
    u8      conReq_response; //ADV_IND or ADV_DIRECT_IND
    u8      backup_6_advData[6];



    u8      directAdv_locIDA; //direct ADV, advA use IDA
    u8      legadv_mac_type;
    u8      legadv_mac_addr[6];

//0x20

    u8      legAdv_initAUseRpa;  //legacy ADV, initA/targetA is RPA, for ADV_DIRECT_IND only
    u8      advParaCmd_peerAdrType;             //set ADV parameters command peer address type
    u8      advParaCmd_peerAddr[BLE_ADDR_LEN];  //set ADV parameters  command peer address

#if (LL_FEATURE_ENABLE_PRIVACY)
    /* be a mark: none zero means device identity used; NULL means device identity not used
     * point to corresponding device identity in resolving list */
    ll_resolv_list_t *pRslvlst_legAdv;
#endif

//0x30
    u16     advInt_min;
    u16     advInt_max;
    u16     advInt_diff;
    u16     advInt_maxAddRandom;


    u32     bSlot_mark_adv;
    u32     systick_mark_adv;
    u32     adv_duration_us;
    u32     adv_start_tick; //LL/DDI/ADV/BV-11-C    [Directed Advertising Events]



    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  advTsk_fifo[ADVTSK_FIFO_NUM];

}ll_legadv_t;



extern _attribute_aligned_(4) ll_legadv_t  bltLegAdv;


int             blt_ll_send_adv(void);
void            blt_ll_reset_leg_adv(void);
int             blt_ll_buildLegacyAdvTask(void);


int     blt_leg_adv_interrupt_task (int, void* p);
int     blt_leg_adv_mainloop_task (int);

ble_sts_t       blc_hci_le_setAdvData(const u8 *data, u8 len);
ble_sts_t       blc_hci_le_setScanRspData(const u8 *data, u8 len);
ble_sts_t       blc_hci_le_setAdvParam(hci_le_setAdvParam_cmdParam_t *para);
ble_sts_t       blc_hci_le_setAdvEnable(adv_en_t adv_enable);
/******************************* leg_adv end ********************************************************************/






/******************************* ext_adv start **********************************************************************/
#define         INVALID_ADVHD_FLAG                              0xFF
#define         INVALID_ADVSET_IDX                              0xFF
#define         INVALID_PERDADV_IDX                             0xFF


#define         TLK_T_MAFS                                      302  // T_MAFS >= 300 uS, add some margin
#define         TLK_T_MAFS_30US_NUM                             10

#define         TX_TX_DELAY_US                                  120 // TX settle(74 uS) + 5  preamble(40uS) + some other baseband cost,   and be 30*N


#define         CHAIN_PKT_MAX_NUM                               4






#define         FLAGS_VALID                                     1
#define         FLAGS_INVALID                                   0


#define         ADV_EXT_IND_C1_ADVA_OPTIONAL_1M_PHY             FLAGS_INVALID
#define         ADV_EXT_IND_C1_TARGET_OPTIONAL_1M_PHY           FLAGS_INVALID


#define         CONTINUOUS_MODE_IN_ONE_IRQ                      0
#define         SEPARATE_IN_DIFFERENT_IRQ                       1
#define         AUX_CHAIN_IND_MODE                              CONTINUOUS_MODE_IN_ONE_IRQ


#define         FSM_TRIGGER_EARLY_WAIT_TICK                     (10*SYSTEM_TIMER_TICK_1US)



#ifndef         EXA_ADVDATA_SCANRSPDATA_SHARE_BUFFER_EN
#define         EXA_ADVDATA_SCANRSPDATA_SHARE_BUFFER_EN         1
#endif



#if 0
/* if use legacy ADV, primary ADV packet maximum length is 44 */
#define         MAX_LENGTH_PRIMARY_ADV_PKT_LEGACY               44

/* if use extended ADV, primary ADV packet maximum length is 28 */
#define         MAX_LENGTH_PRIMARY_ADV_PKT_EXTENDED             28

/* if not sure which kind of ADV will be used, use the bigger value is recommended.
 * For example, when used as BLE Controller, BLE Host can use either legacy or extended ADV */
#define         MAX_LENGTH_PRIMARY_ADV_PKT                      44



#define         MAX_LENGTH_SECOND_ADV_PKT                       264   //sizeof(rf_pkt_ext_adv_t) = 261
#endif





typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;
    u8  ext_hdr_len     :6;
    u8  adv_mode        :2;
    u8  ext_hdr_flg;

    u8  data[253];   //Extended Header + AdvData
}rf_pkt_ext_adv_t;



//#define MAX_VALID_LEN_ADV_EXT_IND     26
typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  ext_hdr_len     :6;
    u8  adv_mode        :2;

    u8  ext_hdr_flg;
    //8 Byte above

    u8  data[18]; //for ADV_EXT_IND, 18B is enough.
    u8  auxPtr_offset;
    u8  rsvd;

}ll_adv_ext_ind_header_t;  //must 4B aligned, now 28B


#define AUX_ADV_FORMAT_LEN      8
typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  ext_hdr_len     :6;
    u8  adv_mode        :2;

    u8  ext_hdr_flg;
    //8 Byte above

    //36B(advA 6 + targetA 6 + ADI 2 + Aux Ptr 3 + sync_info_18 + tx_power 1) is enough
    u8  data[36];
    u8  auxPtr_offset;
    u8  syncInfo_offset;
    u8  ACAD_advData_offset;
    u8  rsvd; //align

}ll_aux_adv_ind_header_t;  //must 4B aligned, now 48B


typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  ext_hdr_len     :6;
    u8  adv_mode        :2;

    u8  ext_hdr_flg;
    //8 Byte above

    //7B(CTE_Info_1 + ADI 2 + Aux Ptr 3 + tx_power 1) is enough, align 8B
    u8  data[7];
    u8  rsvd;
}ll_prd_adv_ind_header_t;  //must 4B aligned, now 16B



typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  advA[6];            //address
    u8  data[31];           //0-31 byte
}rf_pkt_pri_scanrsp_t;


typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;
    u8  ext_hdr_len     :6;
    u8  adv_mode        :2;
    u8  ext_hdr_flg;

    u8  advA[6];    //advA is "M"
    u8  data[247];
}rf_pkt_sec_scanrsp_t;

//Aux Adv's Sync Info field



#define EXTADV_CONST_FORMAT_LEN                     4 //type, rf_len, ext_hdr_len, ext_hdr_flg

extern rf_pkt_ext_adv_t pkt_secondary;

extern rf_pkt_ext_adv_t pkt_periodic;

extern aux_ptr_t        gbl_auxPtr;


#if (BLE_LLMIC_CONCURRENT_EN)
    #define EXT_ADV_FIFONUM                             8  //minimum interval 30mS, spit to 10mS interval for one of chn37/38/39
#else
    #define EXT_ADV_FIFONUM                             4  //TODO SiHui, consider ADV interval small
#endif

#define EXTADV_CHANGE_FLAG                          BIT(0)

#define SYNC_INFO_NEED                              BIT(0)
#define SYNC_INFO_VALID                             BIT(1)

_attribute_aligned_(4)
typedef struct
{
//0x00
    u8      extadv_en;
    u8      extadv_index;
    u8      prdadv_api_en;
    u8      prdadv_task_en;

    u8      mapping_prdadv_idx;
    u8      extAdv_chngReason;
    u8      extadv_change_flag;  //to handle: when prd_adv change, data & timing need change but old allocated task may error
    u8      prdadv_update_flag;

    u8      adv_handle;
    u8      own_addr_type;
#if(BLE_LLMIC_CONCURRENT_EN )
    u8      llmic_advIdx;
#else
    u8      u8_rsvd11;
#endif
    u8      rand_adr_flg;     //TODO: not set

    u8      cur_advMode;
    u8      directed_adv;
    u8      legacy_adv;
    u8      txPower_en_len; //1: represent tx_power bit included and length is 1B;  0: tx_power bit not included and length is 0B

    u8      evt_props;       //now u8 is enough
    u8      evt_prop_bit04;  //now u8 is enough
    u8      adv_chn_mask;
    u8      adv_chn_num;

//5
    u8      adv_chnIdx_1st;
    u8      adv_filterPolicy;
    u8      pri_phy;
    u8      sec_phy;

#if (LL_FEATURE_ENABLE_ADVERTISING_CODING_SELECTION)
    u8      pri_codedPhy_option;
    u8      sec_codedPhy_option;
    u8      rsvd22[2];
#endif

    u8      coding_ind;         //s2 or s8
    u8      adv_sid;
    u8      param_update_flag;
    u8      sSlot_diff_extadv;

    u8      scanReq_notify_en;
    u8      max_ext_adv_evt;
    u8      run_ext_adv_evt;
    u8      sec_adv_max_skip;

    u8      unfinished_advData;
    u8      unfinished_scanRsp;
    u8      aux_align_dly_us;    //to make 30uS *N
    u8      with_aux_adv_ind;

//9
    u8      aux_adv_pending;     //1: AUX ADV task has allocated but not executed
    u8      aux_chn_idx;
    u8      extAdv_terminateEvt_pending;
    u8      advSet_terminate_status;

    u8      with_aux_chain_ind;  //for "non_conn_non_scan" & "scannable"
    u8      n_30us_ext_ind;      //255*30us=7650us, big enough
    u8      aux_1st_pkt_dataLen; // for "non_conn_non_scan" & "scannable"
    u8      chain_ind_num;       // for "non_conn_non_scan" & "scannable", difference: only chain_ind in "non_conn_non_scan", but aux_scan_rsp + chain_ind in "scannable"

    u8      peerScanReq_revFlag;
    u8      peerScanReq_addrType;
    u8      peerScanReq_addr[6];
//13~15
    u16     n_30us_chain_ind;    // for "non_conn_non_scan" & "scannable"
    u16     sSlotDuration_extadv;
    u16     sSlotDuration_auxadv;
    u16     bSlotDuration_auxadv;
    u16     n_30us_aux_ind;
    u16     syncinfo_used;

//16~19
    u16     pri_pkt_us;
    u16     pri_single_chn_us;
    u16     pri_evt_us;  //primary ADV event max time
    u16     aux_adv_tx_off_us; //Mark aux_adv task begin irq --> aux_adv_ind pkt 1st bit send
    u16     rfLen_255_pkt_us;
    u16     rx_1st_pdu_us;  //TX done to RX first 4B PDU come timeout
    u16     rx_finish_us;  //TX done to RX finish timeout
    u16     adv_did;    // BIT<11:0>

//20~23
    u16     advInt_min;
    u16     advInt_max;
    u16     advInt_maxAddRandom;
    u16     advInt_diff;
    u16     maxLen_advData;         //for each ADV sets, this value can be different to save SRAM
    u16     curLen_advData;
    u16     maxLen_scanRsp;         //for each ADV sets, this value can be different to save SRAM
    u16     curLen_scanRsp; //check

#if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE && EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
    u16     backupCurLen_advData;
    u16     backupCurLen_scanRsp;
#endif

//24~28
    u32     tickDuration_auxadv;
    u32     bSlot_mark_extadv;
    s32     sSlot_aux_mark;
    u32     adv_duration_tick;
    u32     aux_evt_us;
//29
    u32     extAdv_begin_tick;

//30~31
    u8*     dat_extAdv;
    u8*     dat_scanRsp;

#if (LL_FEATURE_ENABLE_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE && EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER)
    u8*     backupDat_extAdv;
    u8*     backupDat_scanRsp;
#endif

//32~37
    u8      chain_ind_dataLen[CHAIN_PKT_MAX_NUM];
    u8      eAdv_rand_addr[6];
    u8      public_addr[6];  //can use for multiple local device


    u8      acad_used; //u8_rsvd4;
    u8      eAdvParaCmd_peerAdrType;            //set ADV parameters command peer address type
    u8      eAdvParaCmd_peerAddr[BLE_ADDR_LEN];   //set ADV parameters  command peer address

    u8      txPower_offset; //u8_rsvd5;
    u8      extadv_mac_type;
    u8      extadv_mac_addr[6];

    u8      u8_rsvd6;
    u8      cur_advA_type;
    u8      cur_advA_addr[6];

    u8      u8_rsvd7;
    u8      cur_initA_type;
    u8      cur_initA_addr[6];

//38
#if (LL_FEATURE_ENABLE_PRIVACY)
    u8      extAdv_advA_useRpa;
    u8      extAdv_initAUseRpa; //for direct ADV
    u8      own_addr_rpa;
    u8      scnReq_response; //ADV_SCAN_IND or ADV_IND
   
    u8      conReq_response; //ADV_IND or ADV_DIRECT_IND
    u8      u8_rsvd2[3];

    /* be a mark: none zero means device identity used; NULL means device identity not used
     * point to corresponding device identity in resolving list */
    ll_resolv_list_t *pRslvlst_extAdv;
#endif

    //Attention: if not 4B aligned, task scheduler will collapse. SO variable must 4B aligned above !!!
    sch_task_t  extadv_schTsk_fifo[EXT_ADV_FIFONUM];


    sch_task_t  auxadv_schTsk_fifo;


    ll_adv_ext_ind_header_t primary_adv;    //28B,attention: it's structure, no pointer
    ll_aux_adv_ind_header_t aux_adv_1stPkt; //48B,attention: it's structure, no pointer
    sync_info_t             auxSyncInfo;    //18B,
    u8  aux_sync_align_dly_us;    //to make 30uS *N
    u8  syncinfo_changed;

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
    pawr_acad_t pawr_timing_info;//08B
#endif


#if (LL_ASYNC_LEA_EN)
    u32 asyncAdvIndex;
#endif
}st_ext_adv_t;


extern st_ext_adv_t     *global_pextadv;
extern st_ext_adv_t     *blt_pextadv;

#if 0
//  u16     advInt_min;
//  u16     advInt_max;
    u8      periodicAdv_en;
    u16     maxLen_periodicAdv;
    u16     curLen_periodicAdv;
    u8*     dat_periodicAdv;
    u8 adv_secondary_chan;
    s8 adv_tx_pow;
#endif



typedef struct
{
    u8      maxNum_advSets;
    u8      useNum_advSets;  //not used now
    u8      last_advSet;
    u8      last_advHand;


    u8      custom_aux_chn;
    u8      extadv_sel;
    u8      rsvd2;
    u8      rsvd3;
}ll_extadv_t;
extern ll_extadv_t      bltExtA;

//random 0~31 (32~36 not used)  core_740<2:0> is always 0
#define BLT_GENERATE_AUX_CHN        (bltExtA.custom_aux_chn ?  bltExtA.custom_aux_chn : ((clock_time()>>3) & 0x1F ) )


int         blt_send_aux_adv(void);
void        blt_extadv_clear_adv_set_param(st_ext_adv_t *);
void        blt_ll_updateAdvState(void);



int         blt_ext_adv_interrupt_task(int, void *);
int         blt_ext_adv_mainloop_task(int, void *);


int         blt_send_legacy_adv(void);
int         blt_send_extend_adv(void);
void        blt_send_extend_no_aux_adv(void);


int         blt_extadv_start(int);
int         blt_extadv_send(void);
int         blt_extadv_post(void);
int         blt_auxadv_send(int);

u8          blt_extadv_dur_maxEvt_proc(void);

int         blt_ll_build_extadv_task(void);
int         blt_ll_build_auxadv_task(void);

st_ext_adv_t * blt_extadv_search_existed_and_allocate_new_adv_set(u8 advHandle);  //new
st_ext_adv_t * blt_extadv_search_existed_adv_set(u8 advHandle);          //new

void        blt_extAdv_pendEvtProc_mainloop(void);

void        blt_reset_ext_adv(void);

void        blt_updateExtAdvSet(st_ext_adv_t *cur_pextadv);


void    blt_add_future_task(u8, u8, u32, u32);
bool    blt_remove_future_task(u8);



ble_sts_t   blc_hci_le_setExtAdvParam( hci_le_setExtAdvParam_cmdParam_t *pCmdParam, u8 *pTxPower);
ble_sts_t   blc_hci_le_setExtAdvEnable(hci_le_setExtAdvEn_cmdParam_t *pCmdParam);
ble_sts_t   blc_hci_le_readMaxAdvDataLength(u8 *maxAdvDataLength);/* Maximum supported advertising data length 0x001F to 0x0672: 31 ~ 1650 */
ble_sts_t   blc_hci_le_readNumberOfSupportedAdvSets(u8 *Num_Supported_Advertising_Sets);

ble_sts_t   blt_ll_setSegmentExtendedAdvData(u8, data_oper_t, data_fragment_t, u8, u8 *);
ble_sts_t   blc_hci_le_setExtendedAdvData(hci_le_setExtAdvData_cmdParam_t*);

ble_sts_t   blt_ll_setSegmentExtendedScanResponseData(u8, data_oper_t, data_fragment_t, u8, u8 *);
ble_sts_t   blc_hci_le_setExtendedScanResponseData(hci_le_setExtScanRspData_cmdParam_t*);

ble_sts_t   blc_ll_setExtendedAdvDataRelatedAddressChange(hci_le_setDataAddrChange_cmdParams_t *);



ble_sts_t   blc_hci_le_setExtAdvData (u8 adv_handle, data_oper_t operation, data_fragment_t fragment_prefer, u8 advData_len, u8 *advData);
ble_sts_t   blc_hci_le_setExtScanRspData(u8 adv_handle, data_oper_t operation, data_fragment_t fragment_prefer, u8 scanRspData_len, u8 *scanRspData);


//hidden API, only for Debug
ble_sts_t   blc_ll_setAuxAdvChnIdxByCustomers(u8 aux_chn);

/******************************* ext_adv  end ********************************************************************/









typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  ext_hdr_len     :6;
    u8  adv_mode        :2;

    u8  ext_hdr_flg;

    u8  advA[6];
    u8  targetA[6];
}rf_pkt_aux_conn_rsp_t;
extern rf_pkt_aux_conn_rsp_t    pkt_aux_conn_rsp;


/****************************************************  SCAN_REQ / AUX_SCAN_REQ **************************************************

*********************************************************************************************************************************/
typedef struct{
    u32 dma_len;

    u8 type   :4;
    u8 rfu1   :1;
    u8 chan_sel:1;
    u8 txAddr :1;
    u8 rxAddr :1;

    u8  rf_len;

    u8  scanA[6];
    u8  advA[6];
}rf_pkt_ext_scan_req_t;













#endif /* ADV_STACK_H_ */
