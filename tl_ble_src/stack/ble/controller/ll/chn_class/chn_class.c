/********************************************************************************************************
 * @file    chn_class.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/controller/ble_controller.h"


#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)

_attribute_ble_data_retention_  ll_chnc_cb_t        global_ChncCb[LL_CHNC_CB_NUMS];

_attribute_aligned_(4)  ll_chnclass_ctrl_handler_t  ll_acl_chnclass_ctrl_handler;
// Channel local classification value
_attribute_ble_data_retention_ u8 gLocalControllerChnClass[10] = {
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN, //CHN_STATUS_UNKNOWN | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6, //0
        CHN_STATUS_UNKNOWN | CHN_STATUS_RSVD<<2 | CHN_STATUS_RSVD<<4 | CHN_STATUS_RSVD<<6, //0
};


static  ble_sts_t   blt_ll_chnclassControlPduProc(st_ll_conn_t* pAclConn, u8 opcode, u8 *pLlCtrlPkt);
static  int         blt_ll_chnclassInterruptTask (int flag, void*p);
static  int         blt_ll_chnclassMainloopTask (int flag, void*p);
static  void        blt_ll_chnclassReset(void);
static  void        blt_ll_chnclassMainloop(void);
static  void        blt_ll_chnclassPreChnStsProc(u8 pChm[5]);
static  void        blt_ll_chnClassInitDftParamsAftConnect(st_ll_conn_t* pAclConn);
static  void        blt_ll_chnClassSendChnStsIndProc(st_ll_conn_t *pAclConn);
static  void        blt_ll_chnclassRemapChnClassTblByChm(u8 pInChm[5], u8 pOutChnClassTbl[10]);
static  void        blt_ll_chnclassRemapChmByChnClassTbl(u8 pInChnClassTbl[10], u8 pOutChm[5]);
static  void        blt_ll_chnClassSendChnRptIndProc(st_ll_conn_t *pAclConn);

/**
 * @brief      This function is used to initialize the ChnClassification feature
 * @param[in]  none
 * @return     none
 */
_attribute_noinline_
void        blc_ll_initChnClass_feature(void)
{
    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_chnc_cb_t)), chn_class);
    #endif

    LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION << 7);
    blmsParam.chncSup_en = 1; //can only use 1 or 0, for "blc_hci_read Local Supported Commands"

    ll_acl_chnclass_ctrl_handler = blt_ll_chnclassControlPduProc;
    ll_acl_chnclass_irq_task_cb = blt_ll_chnclassInterruptTask;
    ll_acl_chnclass_mlp_task_cb = blt_ll_chnclassMainloopTask;

    for(int i = 0; i < LL_CHNC_CB_NUMS; i++){
        smemset(&global_ChncCb[i], 0, sizeof(ll_chnc_cb_t));
    }

#if (0) //Test use only
    u8 chnMap[5] = {0xFF, 0xFF, 0xFF, 0xFF, 0x1F};
    blt_ll_chnclassRemapChnClassTblByChm(chnMap, gLocalControllerChnClass);

    my_dump_str_data(0, "###  chnMap  ###", chnMap, 5);
    my_dump_str_data(0, "### cChnClass ###", gLocalControllerChnClass, 10);

    u8 chnMap1[5] = {0x55, 0x55, 0x55, 0x55, 0x15};
    blt_ll_chnclassRemapChnClassTblByChm(chnMap1, gLocalControllerChnClass);

    my_dump_str_data(0, "###  chnMap'  ###", chnMap1, 5);
    my_dump_str_data(0, "### cChnClass ###", gLocalControllerChnClass, 10);

    gLocalControllerChnClass[0] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[1] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[2] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[3] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[4] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[5] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[6] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[7] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[8] = CHN_STATUS_UNKNOWN | CHN_STATUS_BAD<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_BAD<<6;
    gLocalControllerChnClass[9] = CHN_STATUS_UNKNOWN | CHN_STATUS_RSVD<<2 | CHN_STATUS_RSVD<<4 | CHN_STATUS_BAD<<6;

    blt_ll_chnclassRemapChmByChnClassTbl(gLocalControllerChnClass, chnMap);

    my_dump_str_data(0, "=== cChnClass ===", gLocalControllerChnClass, 10);
    my_dump_str_data(0, "===  chnMap  ===", chnMap, 5);

    gLocalControllerChnClass[0] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[1] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[2] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[3] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[4] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[5] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[6] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[7] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[8] = CHN_STATUS_BAD | CHN_STATUS_UNKNOWN<<2 | CHN_STATUS_UNKNOWN<<4 | CHN_STATUS_UNKNOWN<<6;
    gLocalControllerChnClass[9] = CHN_STATUS_BAD |    CHN_STATUS_RSVD<<2 |    CHN_STATUS_RSVD<<4 |     CHN_STATUS_BAD<<6;

    blt_ll_chnclassRemapChmByChnClassTbl(gLocalControllerChnClass, chnMap);

    my_dump_str_data(0, "=== cChnClass' ===", gLocalControllerChnClass, 10);
    my_dump_str_data(0, "===  chnMap  ===", chnMap, 5);
#endif

}

static inline
ll_chnc_cb_t* blt_ll_findAvailableChnc(u16 connIdx)
{
    (void)connIdx;
    assert(connIdx < LL_MAX_ACL_CONN_NUM);
    #if (LL_CHNC_CB_NUMS == LL_MAX_ACL_CONN_NUM)
        return (global_ChncCb + connIdx); //pcl_occpied is not used
    #elif (LL_CHNC_CB_NUMS == 1)
        ll_chnc_cb_t *cur_pChnc = &global_ChncCb[0];
        if(cur_pChnc->chnc_occpied == 0){
            cur_pChnc->chnc_occpied = 1;
            return cur_pChnc;
        }
        return NULL;
    #else
        (void)connIdx;
        ll_chnc_cb_t *cur_pChnc = NULL;
        for(int i = 0; i< LL_CHNC_CB_NUMS; i++){
            cur_pChnc = global_ChncCb + i;
            if(cur_pChnc->chnc_occpied == 0){
                cur_pChnc->chnc_occpied = 1;
                return cur_pChnc;
            }
        }
        return NULL;
    #endif
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
ble_sts_t   blt_ll_chnclassControlPduProc(st_ll_conn_t* pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    ll_chnc_cb_t *cur_pChnc = pAclConn->pChncCb;

    if(opcode == LL_CHANNEL_REPORTING_IND){
        my_dump_str_data(DBG_LL_CC_EN, "Rcvd: LL_CHANNEL_REPORTING_IND", 0, 0);
        //Feature available check
        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_CLASSIFICATION)){
            return LL_ERR_UNKNOWN_OPCODE;
        }

        /* The Central can initiate this procedure at any time after entering the
         * Connection state by sending an LL_CHANNEL_REPORTING_IND PDU. The
         * Peripheral shall not send this PDU.
         */
//      if(pAclConn->aclRole = ACL_ROLE_CENTRAL){ //Needless: The bottom layer has been verified
//          return BLE_SUCCESS; //ignore, do nothing.
//      }

        rf_pkt_ll_chn_rpt_ind_t *pChnRptInd = (rf_pkt_ll_chn_rpt_ind_t*)pLlCtrlPkt;

        //Parameter check invalid
        if((pChnRptInd->minSpacing < LL_CHN_CLASSIFICATION_SPACING_MIN || \
            pChnRptInd->minSpacing > LL_CHN_CLASSIFICATION_SPACING_MAX) || \
           (pChnRptInd->maxDelay < LL_CHN_CLASSIFICATION_SPACING_MIN || \
            pChnRptInd->maxDelay > LL_CHN_CLASSIFICATION_SPACING_MAX) || \
           (pChnRptInd->maxDelay < pChnRptInd->minSpacing) || \
            pChnRptInd->enable > CHN_CLASSIFICATION_REPORTING_ENABLE){
            //return BLE_SUCCESS; //ignore, do nothing
            return LL_ERR_UNKNOWN_OPCODE; //refer to <<LL_TS.p19>>,Page 901, LL/FRH/PER/BI-01-C
        }

        /*
         * Min_Spacing shall be set to indicate, in units of 200 ms, the minimum
         * amount of time from the last LL_CHANNEL_STATUS_IND PDU that was
         * sent before the next LL_CHANNEL_STATUS_IND PDU may be sent.
         *
         * Max_Delay shall be set to indicate, in units of 200 ms, the maximum amount
         * of time between the change in the channel classification being detected by a
         * Peripheral and its generation of an LL_CHANNEL_STATUS_IND PDU.
         */
        assert(cur_pChnc != NULL);

        cur_pChnc->maxChnDelayUs = pChnRptInd->maxDelay * LL_CHN_CLASSIFICATION_SPACING_UNIT * 1000;
        cur_pChnc->minChnSpacingUs = pChnRptInd->minSpacing * LL_CHN_CLASSIFICATION_SPACING_UNIT * 1000;
        cur_pChnc->chnRptEnable = pChnRptInd->enable;

        /* Update channel status indicate monitor timeout start tick */
        cur_pChnc->chnStsMonitorRdyTick = clock_time();

        if(cur_pChnc->chnRptEnable){
            cur_pChnc->chnStsIndSendPending = 1;
            my_dump_str_data(DBG_LL_CC_EN, "chnStsIndSendPending'", &cur_pChnc->chnStsMonitorRdyTick, 4);
        }

    }
    else if(opcode == LL_CHANNEL_STATUS_IND){
        my_dump_str_data(DBG_LL_CC_EN, "Rcvd: LL_CHANNEL_STATUS_IND", 0, 0);
        //Feature available check
        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CHANNEL_CLASSIFICATION)){
            return LL_ERR_UNKNOWN_OPCODE;
        }

        assert(cur_pChnc != NULL);

        /* The Peripheral may initiate this procedure by sending an LL_CHANNEL_-
         * STATUS_IND PDU after channel classification reporting has been enabled by
         * the Central. The Central shall not send this PDU. */
//      if(pAclConn->aclRole = ACL_ROLE_PERIPHERAL){ //Needless: The bottom layer has been verified
//          return BLE_SUCCESS; //ignore, do nothing.
//      }

        if(cur_pChnc->lastChnStsRcvdTick && !clock_time_exceed(cur_pChnc->lastChnStsRcvdTick, cur_pChnc->minChnSpacingUs)){
            /* Ignore the received packet, do nothing. */
            return BLE_SUCCESS;
        }

        cur_pChnc->lastChnStsRcvdTick = clock_time()|1;

        if(cur_pChnc->chnRptEnable == CHN_CLASSIFICATION_REPORTING_DISABLE){
            /* Ignore the received packet, do nothing. */
            return BLE_SUCCESS;
        }

        rf_pkt_ll_chn_status_t *pChnStsInd = (rf_pkt_ll_chn_status_t*)pLlCtrlPkt;

        /* It may send an LL_CHANNEL_MAP_IND PDU to the peer device. */
        u8 chnMap[5] = {0};
        blt_ll_chnclassRemapChmByChnClassTbl(pChnStsInd->cChnClass, chnMap);

        my_dump_str_data(DBG_LL_CC_EN, "=== cChnClass ===", pChnStsInd->cChnClass, 10);
        my_dump_str_data(DBG_LL_CC_EN, "===  chnMap  ===", &chnMap, 5);
        /* Refer to <<LL_TS.p19>>, LL/FRH/CEN/BI-02-C
         * Pull one hair and move the whole body, do you want this? Our current design has
         * only one global channel table, and a single modification causes all relevant
         * links to update the channel map table.  Vendor-special
         */
        if(0 && !memcmp(blmhostChnClassUpt.gLlChannelMap, chnMap, 5)){ //TODO:  not necessary to send
            blc_ll_setHostChannel(chnMap);
        }
    }
    else if(opcode == LL_REJECT_IND_EXT){
        my_dump_str_data(DBG_LL_CC_EN, "Rcvd: LL_REJECT_IND_EXT", 0, 0);
        rf_packet_ll_reject_ext_ind_t* pRejectExtInd = (rf_packet_ll_reject_ext_ind_t*)pLlCtrlPkt;

        if(pRejectExtInd->opcode == LL_CHANNEL_REPORTING_IND){

        }
        else if(pRejectExtInd->opcode == LL_CHANNEL_STATUS_IND){

        }
    }

    return BLE_SUCCESS;
}


_attribute_ram_code_
int         blt_ll_chnclassInterruptTask (int flag, void*p)
{
    (void)p; //unused, remove warning

    int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_CHNC_INIT_AFT_ACL_CONN){
        blt_ll_chnClassInitDftParamsAftConnect((st_ll_conn_t*)(u32)&blms[conn_idx]);
    }
    else{

    }

    return 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_ll_chnclassMainloopTask (int flag, void*p)
{
    //int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_MODULE_RESET){
        blt_ll_chnclassReset();
    }
    else if(flag & FLAG_MODULE_MAINLOOP){
        blt_ll_chnclassMainloop();
    }
    else if(flag & FLAG_MODULE_SET_HOST_CHM){ //Only slave role used
        blt_ll_chnclassPreChnStsProc((u8*)p);
    }

    return 0;
}

void blt_ll_chnclassReset(void)
{
    st_ll_conn_t *pAclConn = NULL;

    for(int conn_idx=0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++){
        pAclConn = (st_ll_conn_t*)(u32)&blms[conn_idx];
        pAclConn->pChncCb = NULL;

    }

    for(int i = 0; i < LL_CHNC_CB_NUMS; i++){
        ll_chnc_cb_t *pChncCb = &global_ChncCb[i];
        #if(0)
            pChncCb->chnStsIndSendPending = 0;
            pChncCb->chnRptIndSendPending = 0;
            pChncCb->lastChnStsSendTick = 0;
            pChncCb->chnStsMonitorRdyTick = 0;
        #else //optimized
            smemset(pChncCb, 0, sizeof(ll_chnc_cb_t));
        #endif
    }

    //Reset 'gLocalControllerChnClass' to the default values
    smemset(gLocalControllerChnClass, CHN_STATUS_UNKNOWN, sizeof(gLocalControllerChnClass));
    gLocalControllerChnClass[9] = CHN_STATUS_UNKNOWN | CHN_STATUS_RSVD<<2 | CHN_STATUS_RSVD<<4 | CHN_STATUS_RSVD<<6;
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blt_ll_chnclassMainloop(void)
{
    //Channel classification concerned main_loop
    st_ll_conn_t *pAclConn = NULL;

    for(int conn_idx=0; conn_idx<LL_MAX_ACL_CONN_NUM; conn_idx++){
        pAclConn = (st_ll_conn_t*)(u32)&blms[conn_idx];
        ll_chnc_cb_t *pChncCb = pAclConn->pChncCb;
        if (pChncCb == NULL) continue;

        if(pAclConn->connState == CONN_STATUS_ESTABLISH){
            if(pAclConn->aclRole == ACL_ROLE_CENTRAL){
                if(pChncCb->chnRptIndSendPending){
                    blt_ll_chnClassSendChnRptIndProc(pAclConn);
                }
            }
            else{ /* slave role */
                /* Slave send LL_CHN_STATUS_IND concerned. */
                if(pChncCb->chnStsIndSendPending){
                    /*assert(pAclConn->chnRptEnable);                    */
                    /* If chnStsIndSendPending is TRUE, then chnRptEnable must be TRUE.
                                                      Note: plus 100ms refer to  LL/FRH/PER/BV-03-C */
                    if(pChncCb->lastChnStsSendTick && !clock_time_exceed(pChncCb->lastChnStsSendTick, pChncCb->minChnSpacingUs + 100000)){
                        /* Skip current ACL connection task. */
                        my_dump_str_data(DBG_LL_CC_EN && 0, "< minChnSpacing: skip ", &pChncCb->lastChnStsSendTick, 4);
                        continue;
                    }
                    else if(!clock_time_exceed(pChncCb->chnStsMonitorRdyTick, pChncCb->maxChnDelayUs)){
                        my_dump_str_data(DBG_LL_CC_EN && 0, "< maxChnDelay: sendLlChnStsInd ", &pChncCb->chnStsMonitorRdyTick, 4);
                        blt_ll_chnClassSendChnStsIndProc(pAclConn);
                    }
                    else{
                        /* It can be considered that the current sending opportunity is
                         * used up, and the monitoring timeout start tick is reset.*/
                        pChncCb->chnStsIndSendPending = 0;
                        pChncCb->lastChnStsSendTick = clock_time()|1;
                        my_dump_str_data(DBG_LL_CC_EN, "It can be considered that the current sending opportunity is \
                          used up, and the monitoring timeout start tick is reset", &pChncCb->lastChnStsSendTick, 4);
                    }
                }
            }
        }
    }
}

static void blt_ll_chnclassRemapChnClassTblByChm(u8 pInChm[5], u8 pOutChnClassTbl[10])
{
    assert(pInChm != NULL);
    assert(pOutChnClassTbl != NULL);

    /* channel map: Channel n is bad = 0. Channel n is unknown = 1. */
    u8 chn4Grp = CHN_STATUS_BAD<<0 | CHN_STATUS_BAD<<2 | \
                 CHN_STATUS_BAD<<4 | CHN_STATUS_BAD<<6;
    smemset(pOutChnClassTbl, chn4Grp, 10);

    /* Calculate channel classification table information. */
    for(int i = 0; i< 5; i++){
        for(int j = 0; j< 8; j++){
            if(pInChm[i] & BIT(j)){
                if(j < 4){
                    pOutChnClassTbl[i*2] &= ~(CHN_STATUS_BAD<<(j*2));
                }
                else{
                    pOutChnClassTbl[i*2+1] &= ~(CHN_STATUS_BAD<<((j-4)*2));
                }
            }
        }
    }

    pOutChnClassTbl[9] = CHN_STATUS_RSVD<<2 | CHN_STATUS_RSVD<<4 | CHN_STATUS_RSVD<<6 | (pOutChnClassTbl[9]&3); //reserved
}

static void blt_ll_chnclassRemapChmByChnClassTbl(u8 pInChnClassTbl[10], u8 pOutChm[5])
{
    assert(pInChnClassTbl != NULL);
    assert(pOutChm != NULL);

    smemset(pOutChm, 0, 5);
    /* Calculate channel map table information. */
    for(int i = 0; i< 10; i++){
        for(int j = 0; j< 8; j++){
            if(j < 4){
                if(((pInChnClassTbl[i]>>(j*2))&3) <= CHN_STATUS_GOOD){
                    pOutChm[i>>1] |= CHN_STATUS_GOOD<<j;
                }
            }
            else{
                if(((pInChnClassTbl[i+1]>>((j-4)*2))&3) <= CHN_STATUS_GOOD){
                    pOutChm[i>>1] |= CHN_STATUS_GOOD<<j;
                }
            }
        }
    }

    pOutChm[4] &= 0x1F; //reserved
}


_attribute_no_inline_
void blt_ll_chnclassPreChnStsProc(u8 pChm[5])
{
    assert(pChm != NULL);
    my_dump_str_data(DBG_LL_CC_EN, "PreChnStsProc: Host set_chm", 0, 0);

    /*  Mapping the channel map table to the channel classification table. */
    blt_ll_chnclassRemapChnClassTblByChm(pChm, gLocalControllerChnClass);

    my_dump_str_data(DBG_LL_CC_EN, "###  chnMap  ###", pChm, 5);
    my_dump_str_data(DBG_LL_CC_EN, "### cChnClass ###", gLocalControllerChnClass, 10);

    st_ll_conn_t *pAclConn = NULL;
    for(int conn_idx=LL_MAX_ACL_CEN_NUM; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++){
        pAclConn = (st_ll_conn_t*)(u32)&blms[conn_idx];
        ll_chnc_cb_t *pChncCb = pAclConn->pChncCb;
        if(pChncCb == NULL) continue;

        assert(pAclConn->aclRole == ACL_ROLE_PERIPHERAL);

        if(pAclConn->connState == CONN_STATUS_ESTABLISH){
            /* Slave send LL_CHN_STATUS_IND concerned. */
            if(pChncCb->chnRptEnable){
                /* Update channel status indicate monitor timeout start tick */
                pChncCb->chnStsMonitorRdyTick = clock_time();
                pChncCb->chnStsIndSendPending = 1;
                my_dump_str_data(DBG_LL_CC_EN, "chnStsIndSendPending", &pChncCb->chnStsMonitorRdyTick, 4);
            }
        }
    }
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
/**
 * @brief      This function is used to set default PAST parameters
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
void blt_ll_chnClassInitDftParamsAftConnect(st_ll_conn_t* pAclConn) //called by blms_connect_common in IRQ
{
    /* Initialize CHNC_CB Pointer */
    u8 conn_idx = pAclConn->acl_conIndex;
    ll_chnc_cb_t *pChncCb = blt_ll_findAvailableChnc(conn_idx);
    pAclConn->pChncCb = pChncCb;
    if(pChncCb == NULL){
        //Do not modify the featureSet, and judge whether the feature is supported by whether pChncCb is NULL.
        LL_FEATURE_MASK_1 &= ~(LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION << 7);
        return;
    } else {
        LL_FEATURE_MASK_1 |= (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION << 7);
    }

    //Default settings parameters
    pChncCb->chnMinSpacing = LL_CHN_CLASSIFICATION_SPACING_MIN; /* 1s */ /* Vendor-Special value. */
    pChncCb->chnMaxDelay = 5*LL_CHN_CLASSIFICATION_SPACING_MIN; /* 5s */ /* Vendor-Special value. */
    pChncCb->minChnSpacingUs = 1000 * (pChncCb->chnMinSpacing * LL_CHN_CLASSIFICATION_SPACING_UNIT);
    pChncCb->maxChnDelayUs = 1000 * (pChncCb->chnMaxDelay * LL_CHN_CLASSIFICATION_SPACING_UNIT);
    pChncCb->chnRptEnable = CHN_CLASSIFICATION_REPORTING_DISABLE; //dft disable.
    pChncCb->lastChnStsSendTick = 0;
    pChncCb->chnRptIndSendPending = 0;
    pChncCb->chnStsIndSendPending = 0;
    pChncCb->chnStsMonitorRdyTick = 0;

    /* Refer to <<LL_TS.p19>>, LL/FRH/CEN/BI-02-C. */
    /* The Lower Tester waits 2s from the end of the HCI_LE_Connection_Complete or
     * HCI_LE_Enhanced_Connection_Complete event for the IUT to send an
     * LL_CHANNEL_REPORTING_IND PDU to the Lower Tester with Enabled set to 0x01. If the
     * LL_CHANNEL_REPORTING_IND PDU isn't sent within 2s, then the test ends with an
     * inconclusive verdict.  */
    if(pAclConn->aclRole == ACL_ROLE_CENTRAL){
        /* Controller will auto initiate this LLPC aft the Center enter connected state. */
        pChncCb->chnRptEnable = CHN_CLASSIFICATION_REPORTING_ENABLE; /* Vendor-Special */
        pChncCb->chnRptIndSendPending = clock_time()|1;
    }


}


//Only Master (Center) role used
_attribute_no_inline_
void blt_ll_chnClassSendChnRptIndProc(st_ll_conn_t *pAclConn)
{
    assert(pAclConn != NULL);
    ll_chnc_cb_t *pChncCb = pAclConn->pChncCb;
    assert(pChncCb != NULL);

    if(pChncCb->chnRptIndSendPending){
        /* feature_req/rsp already exchanged OR the connection has been established for more than 1.5s */
        if(pAclConn->ll_remoteFeature0 || clock_time_exceed(pChncCb->chnRptIndSendPending, 1500000)){
            /* if peer ll_feature doesn't support channel_classification, we don't need to send channel_report_ind */
            if(!(pAclConn->ll_remoteFeature1 & LL_FEATURE_MASK_CHANNEL_CLASSIFICATION)){
                pChncCb->chnRptIndSendPending = 0;
                return;
            }else{
                //no other LLCP pending process //TODO: re-check 'll_rsp_timeout_tick''s design.
                if(!pAclConn->ll_enc_busy && !pAclConn->ll_rsp_timeout_tick){
                    //Prepare to pack the parameters in LL_CHANNEL_REPORTING_IND
                    u8 tmp[sizeof(rf_pkt_ll_chn_rpt_ind_t)];
                    rf_pkt_ll_chn_rpt_ind_t *pChnRptInd = (rf_pkt_ll_chn_rpt_ind_t*)tmp;
                    pChnRptInd->llid = LLID_CONTROL;
                    pChnRptInd->rf_len = sizeof(rf_pkt_ll_chn_rpt_ind_t) - OFFSETOF(rf_pkt_ll_chn_rpt_ind_t, opcode);
                    pChnRptInd->opcode = LL_CHANNEL_REPORTING_IND;
                    pChnRptInd->enable = pChncCb->chnRptEnable;
                    pChnRptInd->minSpacing = pChncCb->chnMinSpacing;
                    pChnRptInd->maxDelay = pChncCb->chnMaxDelay;

                    if(blt_llmsPushLlCtrlPkt(pAclConn->acl_conHandle, LL_CHANNEL_REPORTING_IND, tmp)){
                        pChncCb->chnRptIndSendPending = 0;
                    }
                }
            }
        }
    }
}

//Only Slave (Peripheral) role used
_attribute_no_inline_
void blt_ll_chnClassSendChnStsIndProc(st_ll_conn_t *pAclConn)
{
    ll_chnc_cb_t *pChncCb = pAclConn->pChncCb;
    assert(pChncCb != NULL);

    //no other LLCP pending process //TODO: re-check 'll_rsp_timeout_tick''s design.
    if(pChncCb->chnStsIndSendPending && !pAclConn->ll_enc_busy && !pAclConn->ll_rsp_timeout_tick){
        //Prepare to pack the parameters in LL_CHANNEL_REPORTING_IND
        u8 tmp[sizeof(rf_pkt_ll_chn_status_t)];
        rf_pkt_ll_chn_status_t *pChnStsInd = (rf_pkt_ll_chn_status_t*)tmp;
        pChnStsInd->llid = LLID_CONTROL;
        pChnStsInd->rf_len = sizeof(rf_pkt_ll_chn_status_t) - OFFSETOF(rf_pkt_ll_chn_status_t, opcode);
        pChnStsInd->opcode = LL_CHANNEL_STATUS_IND;
        memcpy(pChnStsInd->cChnClass, gLocalControllerChnClass, sizeof(gLocalControllerChnClass));

        if(blt_llmsPushLlCtrlPkt(pAclConn->acl_conHandle, LL_CHANNEL_STATUS_IND, tmp)){
            pChncCb->chnStsIndSendPending = 0;
            pChncCb->lastChnStsSendTick = clock_time()|1;
        }
    }
}

/**
 * @brief       This function is used to known whether the Controller's channel
 *              assessment scheme is enabled or disabled.
 * @param[out]  pChnAssMode - .
 * @return      Status     - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_chnclassRdAfhChnAssessmentMode(u8 *pChnAssMode)
{
    assert(pChnAssMode != NULL);

//  if(pChnAssMode)
    {
        /* Telink's controller do not support channel assessment feature */
        *pChnAssMode = CONTROLLER_CHN_ASSESSMENT_DISABLE;
    }

    return BLE_SUCCESS;
}
/**
 * @brief      This function is used to controls whether the Controller's channel
 *             assessment scheme is enabled or disabled.
 * @param[in]  chnAssMode - 0x00: disable; 0x01: enable.
 * @return     Status     - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t   blc_ll_chnclassWrAfhChnAssessmentMode(u8 chnAssMode)
{
    /*
     * The HCI_Write_AFH_Channel_Assessment_Mode command writes the value
     * for the AFH_Channel_Assessment_Mode parameter. The AFH_Channel_-
     * Assessment_Mode parameter controls whether the Controller's channel
     * assessment scheme is enabled or disabled.
     */
    if(chnAssMode > CONTROLLER_CHN_ASSESSMENT_ENABLE){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * If the AFH_Channel_Assessment_Mode parameter is enabled and the
     * Controller does not support a channel assessment scheme, other than via the
     * HCI_LE_Set_Host_Channel_Classification command (for LE), then a Status
     * parameter of "Channel Assessment Not Supported" should be returned.
     */
    if(chnAssMode == CONTROLLER_CHN_ASSESSMENT_ENABLE){
        /* Telink's controller do not support channel assessment feature */
        return HCI_ERR_CHAN_ASSESSMENT_NOT_SUPPORTED;
    }
    else{
        /*
         * Disabling channel assessment also forces all channels to be unknown in the
         * local classification for the LE physical transport
         */
        memset(gLocalControllerChnClass, CHN_STATUS_UNKNOWN, sizeof(gLocalControllerChnClass));
    }

    return BLE_SUCCESS;
}

#endif  // end of LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION
