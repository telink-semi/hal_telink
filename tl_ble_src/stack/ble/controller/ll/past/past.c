/********************************************************************************************************
 * @file    past.c
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


#if (LL_FEATURE_ENABLE_PAST)


_attribute_ble_data_retention_ ll_past_mng_t blt_PastMng;

_attribute_ble_data_retention_ ll_past_cb_t global_PastCb[LL_PAST_CB_NUMS];

_attribute_ble_data_retention_ _attribute_aligned_(4) ll_past_ctrl_handler_t ll_past_ctrl_handler = NULL;

_attribute_ble_data_retention_ u32 offsetUsForLargeInterval = 0;

#if (LL_CON_PER_BV105C)
_attribute_ble_data_retention_ u8 pastRcvdInd[sizeof(rf_pkt_ll_periodic_sync_ind_t)] = {0};
    #endif

static ble_sts_t blt_ll_pastControlPduProc(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt);
static int       blt_ll_pastGetRcvdCEt(u8 *raw_pkt);
#if (!ESL_RAM_OPTIMIZATION)
static int blt_ll_pastSendSyncIndProc(st_ll_conn_t *pAclConn);
#endif //(!ESL_RAM_OPTIMIZATION)
static int blt_ll_pastInterruptTask(int flag, void *p);
static int blt_ll_pastMainloopTask(int flag);

static void blt_ll_pastReset(void);
static void blt_ll_pastMainloop(void);
static void blt_ll_pastInitDftParamsAftConnect(st_ll_conn_t *pAclConn);

#define PAWR_TIMEOUT_NUMBER 8

/**
 * @brief      This function is used to initialize the PAST module
 * @param[in]  none
 * @return     none
 */
void blc_ll_initPAST_module(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_past_cb_t)), past);
    #endif


    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_LE_PAST_SENDER << 24);
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25);

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER || \
         LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER;
    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER;
    #endif

    ll_past_ctrl_handler    = blt_ll_pastControlPduProc;
    ll_acl_past_irq_task_cb = blt_ll_pastInterruptTask;
    ll_acl_past_mlp_task_cb = blt_ll_pastMainloopTask;

    //default value
    blt_PastMng.pastSkip        = 0;
    blt_PastMng.cteType         = PAST_CTE_TYPE_SYNC_TO_WITHOUT_CTE;
    blt_PastMng.pastSyncTimeout = PAST_SYNC_MIN_TIMEOUT;
    blt_PastMng.pastMode        = PAST_MODE_OFF;

    blmsParam.past_en = 1;

    for (int i = 0; i < LL_PAST_CB_NUMS; i++) {
        smemset(&global_PastCb[i], 0, sizeof(ll_past_cb_t));
    }
}

static inline ll_past_cb_t *blt_ll_findAvailablePast(u16 connIdx)
{
    (void)connIdx;
    assert(connIdx < LL_MAX_ACL_CONN_NUM);
    #if (LL_PAST_CB_NUMS == LL_MAX_ACL_CONN_NUM)
    return (global_PastCb + connIdx); //past_occpied is not used
    #elif (LL_PAST_CB_NUMS == 1)
    ll_past_cb_t *cur_pPast = &global_PastCb[0];
    if (cur_pPast->past_occpied == 0) {
        cur_pPast->past_occpied = 1;
        return cur_pPast;
    }
    return NULL;
    #else
    (void)connIdx;
    ll_past_cb_t *cur_pPast = NULL;
    for (int i = 0; i < LL_PAST_CB_NUMS; i++) {
        cur_pPast = global_PastCb + i;
        if (cur_pPast->past_occpied == 0) {
            cur_pPast->past_occpied = 1;
            return cur_pPast;
        }
    }
    return NULL;
    #endif
}

_attribute_ram_code_ int blt_ll_pastInterruptTask(int flag, void *p)
{
    int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if (flag & FLAG_PAST_INIT_AFT_ACL_CONN) {
        blt_ll_pastInitDftParamsAftConnect((st_ll_conn_t *)&blms[conn_idx]);
    } else if (flag & FLAG_PAST_RCVD_PRD_SYNC_IND) {
        blt_ll_pastGetRcvdCEt((u8 *)p); //called by acl_rx_irq
    } else {
    }

    return 0;
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    int
    blt_ll_pastMainloopTask(int flag)
{
    if (flag == FLAG_MODULE_RESET) {
        blt_ll_pastReset();
    } else if (flag == FLAG_MODULE_MAINLOOP) {
        blt_ll_pastMainloop();
    }

    return 0;
}

void blt_ll_pastReset(void)
{
    st_ll_conn_t  *pAclConn = NULL;
    st_pda_sync_t *pPdaSync = NULL;

    //ACL (PAST) concerned

    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        pAclConn          = (st_ll_conn_t *)&blms[conn_idx];
        pAclConn->pPastCb = NULL;
    }
    //PDA sync (PAST) concerned
    for (int pda_idx = 0; pda_idx < TSKNUM_PDA_SYNC; pda_idx++) {
        pPdaSync                 = (st_pda_sync_t *)&pdAsync_tbl[pda_idx];
        pPdaSync->sync_establish = 0;
    }

    for (int i = 0; i < LL_PAST_CB_NUMS; i++) {
        ll_past_cb_t *pPastCb = &global_PastCb[i];
    #if (0)
        pPastCb->pastSendPending = 0;
        pPastCb->pastCreateSync  = 0;
        pPastCb->perServiceData  = 0;
        pPastCb->perSyncHandle   = 0;
    #else //optimized
        smemset(pPastCb, 0, sizeof(ll_past_cb_t));
    #endif
    }

    //reset to the default value
    blt_PastMng.pastSkip        = 0;
    blt_PastMng.cteType         = PAST_CTE_TYPE_SYNC_TO_WITHOUT_CTE;
    blt_PastMng.pastSyncTimeout = PAST_SYNC_MIN_TIMEOUT;
    blt_PastMng.pastMode        = PAST_MODE_OFF;
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_ll_pastMainloop(void)
{
    //ACL (PAST) concerned main_loop
    st_ll_conn_t  *pc = NULL;
    st_pda_sync_t *pPdaSync;
    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        pc                    = (st_ll_conn_t *)&blms[conn_idx];
        ll_past_cb_t *pPastCb = pc->pPastCb;
        if (pPastCb == NULL) {
            continue;
        }

        if (pc->connState == CONN_STATUS_ESTABLISH) {
            if (pc->llcp_flag.bit.ll_feat_exg_flag) {
                //PAST sender used: send LL_PERIODIC_SYNC_IND packet loop
                if (pPastCb->pastSendPending) {
#if (!ESL_RAM_OPTIMIZATION)
                    blt_ll_pastSendSyncIndProc(pc);
#endif //(!ESL_RAM_OPTIMIZATION)
                }
            }

    //PAST recipient used: PDA sync concerned
    #if (LL_CON_PER_BV105C)
            if (pPastCb->pastRcvdTick && clock_time_exceed(pPastCb->pastRcvdTick, 200)) {
                pPastCb->pastRcvdTick = 0;
                //smemset(pastRcvdInd, 0, sizeof(rf_pkt_ll_periodic_sync_ind_t));
                my_dump_str_data(DBG_LL_PAST_EN, "dly rcvd LL_PAST_IND proc", 0, 0);
                blt_ll_pastControlPduProc(pc, LL_PERIODIC_SYNC_IND, pastRcvdInd);
            }
    #endif
            if (pPastCb->pastCreateSync && (pPastCb->perSyncHandle & BLT_SYNC_HANDLE)) {
                pPdaSync = (st_pda_sync_t *)&pdAsync_tbl[pPastCb->perSyncHandle & BLT_SYNC_IDX_MARK];

                if (pPdaSync->sync_establish == SYNC_ESTABLISHED_BY_PAST) {
                    my_dump_str_data(DBG_LL_PAST_EN, "hci_le_periodicAdvSyncTransferRcvd_evt", 0, 0);
                    if (hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED) {
                        u8 status = BLE_SUCCESS;
                        if (1 && pPdaSync->sync_state == SYNC_STATE_IDLE) {
                            status = HCI_ERR_CONN_FAILED_TO_ESTABLISH;
                        }

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
                        if (pPdaSync->pawr_acad_check) {
                            extern int hci_le_periodicAdvSyncTransferRcvd_evt_V2(u8 status, u16 connHandle, u16 serviceData, u16 syncHandle, u8 advSID, u8 advAddrType, u8 advAddr[6], u8 advPHY, u16 perdAdvItvl, u8 advClkAccuracy, pawr_acad_t * pPawrInfo);
                            hci_le_periodicAdvSyncTransferRcvd_evt_V2(status, pc->acl_conHandle, pPastCb->perServiceData, pPastCb->perSyncHandle, pPdaSync->pda_id.sid, pPdaSync->adrIdType, pPdaSync->pda_id.addr, pPdaSync->pda_rx.pda_phy, //pda_phy's type: refer to 'le_phy_type_t'
                                                                      pPdaSync->pda_interval,
                                                                      pPdaSync->sca,
                                                                      &pPdaSync->pawr_acadInfo);
                        } else
    #endif
                        {
                            hci_le_periodicAdvSyncTransferRcvd_evt(status, pc->acl_conHandle, pPastCb->perServiceData, pPastCb->perSyncHandle, pPdaSync->pda_id.sid, pPdaSync->adrIdType, pPdaSync->pda_id.addr, pPdaSync->pda_rx.pda_phy, //pda_phy's type: refer to 'le_phy_type_t'
                                                                   pPdaSync->pda_interval,
                                                                   pPdaSync->sca);
                        }
                    }

                    pPastCb->pastCreateSync  = 0;
                    pPdaSync->sync_establish = 0;
                }
            }
        }
    }
}


    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    int
    blt_ll_pastGetRcvdCEt(u8 *raw_pkt) //called by acl_rx_irq
{
    ll_past_cb_t *pPastCb = blms_pconn->pPastCb;
    if (pPastCb == NULL) {
        return 0;
    }

    /*
     * CEt is the value of connEventCounter for the connection event when the
     * LL_PERIODIC_SYNC_IND PDU was (re)transmitted by device B and received by device C.
     */
    u8 llid   = raw_pkt[DMA_RFRX_OFFSET_HEADER] & 0x03;
    u8 rf_len = raw_pkt[DMA_RFRX_OFFSET_RFLEN];

    /*
     * proc ll_periodic_sync_ind in irq
     *      ll_periodic_sync_ind        rf_len = 35    encryption rf_len = 39
     *      ll_periodic_sync_wr_ind     rf_len = 43    encryption rf_len = 47
     */
    if (llid == 3 && (blms_pconn->crypt.enable ? (rf_len == 39 || rf_len == 47) : (rf_len == 35 || rf_len == 43))) {
        if (blms_pconn->crypt.enable) {
            smemcpy(pPastCb->pastTemBuf, raw_pkt, rf_len + 2 + 4);
            pPastCb->pastDecPending = pPastCb->pastTemBuf;
            pPastCb->pastRcvdNo     = blms_pconn->conn_pkt_rcvd;
            my_dump_str_data(DBG_LL_PAST_EN, "past rcvd holding", &pPastCb->pastRcvdNo, 4);

            /* Save Current Timing information first, but need to check if they are valid for PAST IND latter */
            pPastCb->pastRcvdSucc = FALSE;
            pPastCb->pastRcvdCEt  = blms_pconn->conn_inst_mark; //keep CEt
    #if (LL_CON_PER_BV105C)
            pPastCb->pastRcvdTick = clock_time() | 1;
            my_dump_str_data(DBG_LL_PAST_EN, "pastRcvdTick", &pPastCb->pastRcvdTick, 4);
    #endif
            my_dump_str_data(DBG_LL_PAST_EN, "IRQ:pastRcvdCEt[enc]", &blms_pconn->conn_inst_mark, 2);
        } else {
            rf_packet_ll_control_t *pll = (rf_packet_ll_control_t *)(raw_pkt + DMA_RFRX_OFFSET_HEADER);

            if (pll->opcode == LL_PERIODIC_SYNC_IND || pll->opcode == LL_PERIODIC_SYNC_WR_IND) {
                my_dump_str_data(DBG_LL_PAST_EN, "IRQ:pastRcvdCEt[no_enc]", &blms_pconn->conn_inst_mark, 2);
                pPastCb->pastRcvdSucc = TRUE;
                pPastCb->pastRcvdCEt  = blms_pconn->conn_inst_mark; //keep CEt
    #if (LL_CON_PER_BV105C)
                pPastCb->pastRcvdTick = clock_time() | 1;
                my_dump_str_data(DBG_LL_PAST_EN, "pastRcvdTick", &pPastCb->pastRcvdTick, 4);
    #endif
            }
        }
    }

    return 1;
}

int blt_ll_pastDecRcvdPast(st_ll_conn_t *pAcl)
{
    assert(pAcl != NULL);
    ll_past_cb_t *pPastCb = pAcl->pPastCb;
    assert(ll_past_cb_t * pPastCb != NULL);

    rf_packet_ll_control_t *pll = (rf_packet_ll_control_t *)(pPastCb->pastTemBuf + DMA_RFRX_OFFSET_HEADER);
    if (pAcl->crypt.enable) {
        u32 bak             = pAcl->crypt.dec_pno;
        pAcl->crypt.dec_pno = pPastCb->pastRcvdNo;
        aes_enc_dec_busy    = 1;
        int st              = aes_ll_ccm_decryption((llPhysChnPdu_t *)pll, 1, CRYPT_NONCE_TYPE_ACL, &pAcl->crypt);
        aes_enc_dec_busy    = 0;
        pAcl->crypt.dec_pno = bak;

        if (st) { //decrypt err
            return 0;
        }

        if (pll->opcode == LL_PERIODIC_SYNC_IND || pll->opcode == LL_PERIODIC_SYNC_WR_IND) {
            pPastCb->pastRcvdSucc = TRUE;
        }
    }
    return 0;
}

#if (!ESL_RAM_OPTIMIZATION)
int blt_ll_pastSendSyncIndProc(st_ll_conn_t *pAclConn)
{
    ll_past_cb_t *pPastCb = pAclConn->pPastCb;
    assert(pPastCb != NULL);

    //no other LLCP pending process
    if (pPastCb->pastSendPending && !pAclConn->ll_enc_busy && (!pAclConn->ll_rsp_timeout_tick && pAclConn->curCEsyncAP)) {
    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER)
        u8 tmp[sizeof(rf_pkt_ll_periodic_sync_wr_ind_t)]; //buffer for LL_PERIODIC_SYNC_WR_IND
    #else
        u8 tmp[sizeof(rf_pkt_ll_periodic_sync_ind_t)]; //buffer for LL_PERIODIC_SYNC_IND
    #endif

        //Prepare to pack the common parameters in LL_PERIODIC_SYNC_IND
        rf_pkt_ll_periodic_sync_ind_t *pPerdSyncInd = (rf_pkt_ll_periodic_sync_ind_t *)tmp;
        pPerdSyncInd->llid                          = LLID_CONTROL;
        pPerdSyncInd->rf_len                        = sizeof(rf_pkt_ll_periodic_sync_ind_t) - OFFSETOF(rf_pkt_ll_periodic_sync_ind_t, opcode);
        pPerdSyncInd->opcode                        = LL_PERIODIC_SYNC_IND;
        pPerdSyncInd->id                            = pPastCb->perServiceData;
    /* Here use our own device's SCA */
    #if BLMS_PM_ENABLE
        pPerdSyncInd->sca = SCA_MASTER_SLAVE_251_500_PPM; //251PPM - 500PPM
    #else
        pPerdSyncInd->sca = SCA_MASTER_SLAVE_31_50_PPM; //31PPM - 50PPM
    #endif

        st_pda_t    *p_curPda   = NULL;
        sync_info_t *pSyncInfo  = &pPerdSyncInd->syncInfo;
        u16          syncHandle = pPastCb->perSyncHandle;

        if (pPastCb->perSyncSrc == PAST_SYNC_SRC_SCAN) { //By ext_scan to get syncInfo
            u8 pda_idx = syncHandle & BLT_SYNC_IDX_MARK;
            //already checked by API: blt_isSyncHandleValid
            if (pdAsync_tbl[pda_idx].sync_state != SYNC_STATE_SYNCED) {
                pPastCb->pastSendPending = 0;
                return 0;
            } else {
                st_pda_sync_t *pPdAsync = (st_pda_sync_t *)&pdAsync_tbl[pda_idx];
                p_curPda                = (st_pda_t *)&pPdAsync->pda_rx;

                pSyncInfo->itvl = pPdAsync->pda_interval;
                ////// ExtAdvID
                pPerdSyncInd->sid = pPdAsync->pda_id.sid;

                /*
                 * # If the advertiser's address in the advertising set pointing to the periodic
                 *   advertising is a resolvable private address, AdvA shall be set to any
                 *   resolvable private address that was generated using the same IRK.
                 * # Otherwise, AdvA shall be set to the advertiser's address in the advertising
                 *   set.
                 */
                u8 *pOwnAddr        = pPdAsync->pda_id.addr;
                pPerdSyncInd->aType = pPdAsync->pda_id.adrType; //Identity Address Type should be used
    #if (LL_FEATURE_ENABLE_PRIVACY)
                u8 advA_type;
                u8 advA_addr[6];

                advA_type = pPdAsync->record_advA_adrType;
                smemcpy(advA_addr, pPdAsync->record_advA_addr, BLE_ADDR_LEN);

                my_dump_str_data(DBG_LL_PAST_EN, "PDA_SYNC ADR record", advA_addr, 6);

                u8 advA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(advA_type, advA_addr);
                if (advA_is_rpa) { //RPA
                    my_dump_str_data(DBG_LL_PAST_EN, "PAST sender[src_scan]: peer used RPA", advA_addr, 6);
                    ll_resolv_list_t *ppRL = blt_ll_resolve_rpa(0, advA_addr, NULL);
                    if (ppRL && ppRL->peerIrk_valid) {
                        pPerdSyncInd->aType = PEERATYPE_RANDOM_DEVICE_ADDRESS;
                        pOwnAddr            = ppRL->genrt_peerRpa;
                    } else {
                        if (ppRL == NULL) {
                            my_dump_str_data(DBG_LL_PAST_EN, "PAST rap fail", &advA_type, 1);
                        } else {
                            my_dump_str_data(DBG_LL_PAST_EN, "PAST rap fail*", &advA_type, 1);
                        }
                    }
                }
    #endif
                smemcpy(pPerdSyncInd->advA, pOwnAddr, BLE_ADDR_LEN); //Identity Address should be used
                /* here use the received chm && sca */
                smemcpy(pSyncInfo->chm, p_curPda->chnParam.map.chmTbl, 5); //[0:36]chm, : [37:39]sca
                pSyncInfo->chm[4] |= (pPdAsync->sca) << 5;
            }
        } else {                                                           // if(pAclConn->perSyncSrc = PAST_SYNC_SRC_BCST){ //By periodic_bcst(itself send the periodic_adv) to get syncInfo
            int           i           = 0;
            st_ext_adv_t *cur_pextadv = NULL;
            for (i = 0; i < bltExtA.maxNum_advSets; i++) {
                cur_pextadv = (st_ext_adv_t *)(global_pextadv + i);
                if (cur_pextadv->adv_handle == syncHandle &&
                    cur_pextadv->prdadv_task_en) { //existing ADV set && periodic ADV enabled
                    break;
                }
            }
            if (i == bltExtA.maxNum_advSets) { //Not found
                pPastCb->pastSendPending = 0;
                return 0;
            } else {
                st_prd_adv_t *cur_pPerdadv = (st_prd_adv_t *)(global_pPerdadv + cur_pextadv->mapping_prdadv_idx);
                p_curPda                   = (st_pda_t *)&cur_pPerdadv->pda_tx;

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER) //re-prepare for LL_PERIODIC_SYNC_WR_IND
                if (cur_pPerdadv->num_subevents) {
                    if (!(pAclConn->ll_remoteFeature1 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)) {
                        return 0;
                    }
                    rf_pkt_ll_periodic_sync_wr_ind_t *pPerdSyncWrInd = (rf_pkt_ll_periodic_sync_wr_ind_t *)tmp;
                    pPerdSyncWrInd->pastInd.rf_len                   = sizeof(rf_pkt_ll_periodic_sync_wr_ind_t) - 2; //Header 2Byte
                    pPerdSyncWrInd->pastInd.opcode                   = LL_PERIODIC_SYNC_WR_IND;
                    /* For PAwR timing information */
                    pPerdSyncWrInd->pawrAcadInfo.num_subevent     = cur_pPerdadv->num_subevents;
                    pPerdSyncWrInd->pawrAcadInfo.rsp_AA           = cur_pPerdadv->responseAA;
                    pPerdSyncWrInd->pawrAcadInfo.rsp_slot_delay   = cur_pPerdadv->response_slot_delay;
                    pPerdSyncWrInd->pawrAcadInfo.rsp_slot_spacing = cur_pPerdadv->response_slot_spacing;
                    pPerdSyncWrInd->pawrAcadInfo.subevent_intvl   = cur_pPerdadv->subevent_interval;
                }
    #endif


                pSyncInfo->itvl = cur_pextadv->auxSyncInfo.itvl;
                ////// ExtAdvID
                pPerdSyncInd->sid = cur_pextadv->adv_sid;
                /*
                 * AType shall be 0 if the AdvA field holds a public address and 1 if it holds a random address.
                 * If using RPA, refresh aType to Random.
                 */
                pPerdSyncInd->aType = cur_pextadv->own_addr_type & OWN_ADDRESS_TYPE_RANDOM_MASK; //default: public[0] OR random[1]
                u8 *pOwnAddr        = NULL;
                if (cur_pextadv->own_addr_type == OWN_ADDRESS_PUBLIC || cur_pextadv->own_addr_type == OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC) {
                    pOwnAddr = cur_pextadv->public_addr;
                } else if (cur_pextadv->own_addr_type == OWN_ADDRESS_RANDOM || cur_pextadv->own_addr_type == OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM) {
                    pOwnAddr = cur_pextadv->eAdv_rand_addr;
                }

    /*
                 * If the advertiser's address in the advertising set pointing to the periodic
                 * advertising is a resolvable private address, AdvA shall be set to any
                 * resolvable private address that was generated using the same IRK.
                 * Otherwise, AdvA shall be set to the advertiser's address in the advertising set.
                 */
    #if (LL_FEATURE_ENABLE_PRIVACY && LL_FEATURE_ENABLE_LOCAL_RPA)
                if (cur_pextadv->own_addr_rpa) { //OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC / OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM
                    ll_resolv_list_t *pRL = blt_ll_searchResolvingListEntry(cur_pextadv->eAdvParaCmd_peerAdrType, cur_pextadv->eAdvParaCmd_peerAddr);
                    if (pRL && pRL->localIrk_valid) {
                        pOwnAddr = pRL->rlLocalRpa;
                        //If using RPA, refresh aType to Random
                        pPerdSyncInd->aType = OWN_ADDRESS_RANDOM;
                    }
                }
    #endif

                smemcpy(pPerdSyncInd->advA, pOwnAddr, BLE_ADDR_LEN);

                smemcpy(pSyncInfo->chm, p_curPda->chnParam.map.chmTbl, 5); //[0:36]chm, : [37:39]sca
                /* PAST_SYNC_SRC_BCST: itself broadcast ppm fixed */
    #if BLMS_PM_ENABLE
                pSyncInfo->chm[4] |= (SCA_MASTER_SLAVE_251_500_PPM << 5); //251PPM - 500PPM
    #else
                pSyncInfo->chm[4] |= (SCA_MASTER_SLAVE_31_50_PPM << 5); //31PPM - 50PPM
    #endif
            }
        }

        u32 r = irq_disable();

        u16 conn_inst_mark;
        u32 conn_tick_mark;
        if (pAclConn->aclRole == ACL_ROLE_CENTRAL) {
            conn_tick_mark = pAclConn->ap_tick_mark;
            conn_inst_mark = pAclConn->conn_inst_mark;
        } else {
    #if (LL_ACL_PER_EN)
            u8 conn_sel     = pAclConn->acl_conIndex;
            u8 conn_sel_slv = conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
            bls_pconn       = (st_lls_conn_t *)&blmsSlave[conn_sel_slv];
            conn_inst_mark  = bls_pconn->evtCnt_mark_1strx;
            conn_tick_mark  = bls_pconn->tick_mark_1strx;
    #else
            irq_restore(r);
            return HCI_ERR_UNSPECIFIED_ERROR;
    #endif
        }

        my_dump_str_data(DBG_LL_PAST_EN, "conn_tick_mark", &conn_tick_mark, 4);
        my_dump_str_data(DBG_LL_PAST_EN, "conn_inst_mark", &conn_inst_mark, 2);

        pPerdSyncInd->syncConnEvtCnt = conn_inst_mark; //CEs:
        //PEb is the value of paEventCounter for a recent AUX_SYNC_IND PDU that device B has received
        pPerdSyncInd->lastPaEvtCnt = p_curPda->lastPaEvtCnt; //PEb:
        u32 PEaAnchorPoint         = p_curPda->lastPaAnchorPoint;

        u32 jump_conn_number = 0;
        if ((PEaAnchorPoint + pSyncInfo->itvl * 1250 * SYSTEM_TIMER_TICK_1US - conn_tick_mark) / SYSTEM_TIMER_TICK_1US > 2457600 * 2) {
            jump_conn_number = (PEaAnchorPoint + pSyncInfo->itvl * 1250 * SYSTEM_TIMER_TICK_1US - conn_tick_mark - 2457600 * SYSTEM_TIMER_TICK_1US) / pAclConn->conn_intvl_tick;
        } else {
            jump_conn_number = 6;
        }

        u16 CEref      = conn_inst_mark + jump_conn_number + pAclConn->conn_latency;
        u32 refAclTime = conn_tick_mark + (jump_conn_number + pAclConn->conn_latency) * pAclConn->conn_intvl_tick;
        //CEs is the connEventCounter value for connect events when devices B and C synchronize their anchors


        irq_restore(r);

        //CEref is the value of connEventCount in the LL_PERIODIC_SYNC_IND PDU.
        pPerdSyncInd->connEvtCnt = CEref; //CEref:

        my_dump_str_data(DBG_LL_PAST_EN, "CEref", &CEref, 2);
        my_dump_str_data(DBG_LL_PAST_EN, "CEs", &conn_inst_mark, 2);
        my_dump_str_data(DBG_LL_PAST_EN, "PEb", &pPerdSyncInd->lastPaEvtCnt, 2);

        u16 PEa, jumpPrdNum;
        if ((u32)((refAclTime + pAclConn->sSlot_allocNum * SSLOT_TICK_NUM + SLOT_PROCESS_MAX_TICK) - PEaAnchorPoint) < BIT(31)) {
            jumpPrdNum = 1 + ((refAclTime + pAclConn->sSlot_allocNum * SSLOT_TICK_NUM + SLOT_PROCESS_MAX_TICK) - PEaAnchorPoint) / (pSyncInfo->itvl * 1250 * SYSTEM_TIMER_TICK_1US);
            PEa        = pPerdSyncInd->lastPaEvtCnt + jumpPrdNum;
            PEaAnchorPoint += jumpPrdNum * (pSyncInfo->itvl * 1250 * SYSTEM_TIMER_TICK_1US);
            my_dump_str_data(DBG_LL_PAST_EN, "PEa+", &PEa, 2);
        } else {
            jumpPrdNum =
                (PEaAnchorPoint - (refAclTime + pAclConn->sSlot_allocNum * SSLOT_TICK_NUM + SLOT_PROCESS_MAX_TICK)) / (pPerdSyncInd->syncInfo.itvl * 1250 * SYSTEM_TIMER_TICK_1US);
            PEa = pPerdSyncInd->lastPaEvtCnt - jumpPrdNum;
            PEaAnchorPoint -= jumpPrdNum * (pSyncInfo->itvl * 1250 * SYSTEM_TIMER_TICK_1US);
            my_dump_str_data(DBG_LL_PAST_EN, "PEa-", &PEa, 2);
        }

        //The above code will guarantee offsetUs > 0
        u32 offsetUs     = (PEaAnchorPoint - refAclTime) / SYSTEM_TIMER_TICK_1US;
        u8  offsetAdjust = 0;
        u8  offsetUnit   = EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US;
        u16 syncPktOffset;
        int num_us_30u = offsetUs / 30; //Round down 1 unit
        num_us_30u     = num_us_30u * 30;
        if (num_us_30u >= 245700 + 30) {
            if (num_us_30u > 2457600) {
                offsetAdjust = 1;
                num_us_30u -= 2457600;
            }
            int number_300us = num_us_30u / 300;
            offsetUnit       = EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US;
            syncPktOffset    = number_300us;
        } else {
            int number_30us = offsetUs / 30; //Round down 1 unit
            syncPktOffset   = number_30us;
        }
        //Pack sync_info field 1
        pSyncInfo->syncPktOffset = syncPktOffset;
        pSyncInfo->offsetUnit    = offsetUnit;
        pSyncInfo->offsetAdjust  = offsetAdjust;
        pSyncInfo->evtCounter    = PEa;

        pSyncInfo->AA = p_curPda->paAccessAddr;
        smemcpy(pSyncInfo->crcInit, &p_curPda->paCrcInit, 3);
        /* Refer to <<Core5.3>> Page2712 Table 2.19: convert 'le_phy_type_t' to 'le_phy_prefer_type_t' */
        pPerdSyncInd->phy = BIT(p_curPda->pda_phy - 1); //pda_phy's type: refer to 'le_phy_type_t'

        if (blt_llmsPushLlCtrlPkt(pAclConn->acl_conHandle, pPerdSyncInd->opcode, tmp)) {
            pPastCb->pastSendPending = 0;
        }
    }

    return 1;
}
#endif //(!ESL_RAM_OPTIMIZATION)

    #if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
    #endif
    void
    blt_ll_pastInitDftParamsAftConnect(st_ll_conn_t *pAclConn) //called by blms_connect_common in IRQ
{
    /* Initialize PAST_CB Pointer */
    u8            conn_idx = pAclConn->acl_conIndex;
    ll_past_cb_t *pPastCb  = blt_ll_findAvailablePast(conn_idx);
    pAclConn->pPastCb      = pPastCb;
    if (pPastCb == NULL) {
        LL_FEATURE_MASK_0 &= ~(LL_FEATURE_ENABLE_LE_PAST_SENDER << 24);
        LL_FEATURE_MASK_0 &= ~(LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25);
        my_dump_str_data(DBG_LL_PAST_EN, "PAST feature not supported", 0, 0);
        return;
    } else {
        LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_LE_PAST_SENDER << 24);
        LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25);
    }

    //Default settings for PAST (for Recipient)
    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECIPIENT) { //needless current, here must be TRUE.
        pPastCb->pastSkip        = blt_PastMng.pastSkip;
        pPastCb->pastSyncTimeout = blt_PastMng.pastSyncTimeout;
        pPastCb->pastMode        = blt_PastMng.pastMode;
        pPastCb->pastSyncCteType = PAST_CTE_TYPE_SYNC_TO_WITHOUT_CTE;
        pPastCb->pastCreateSync  = 0;
        pPastCb->pastRcvdSucc    = FALSE;
        pPastCb->pastDecPending  = NULL;
    #if (LL_CON_PER_BV105C)
        pPastCb->pastRcvdTick = 0;
    #endif
    }
    //Default settings for PAST (for Sender)
    if (LL_FEATURE_MASK_0 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_SENDER) { //needless current, here must be TRUE.
        pPastCb->pastSendPending = 0;
    }
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    ble_sts_t
    blt_ll_pastControlPduProc(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    // Feature available check
    if (opcode == LL_PERIODIC_SYNC_IND || opcode == LL_PERIODIC_SYNC_WR_IND) {
        my_dump_str_data(DBG_LL_PAST_EN, "Rcvd: LL_PERIODIC_SYNC_IND", 0, 0);
        if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25))) {
            return LL_ERR_UNKNOWN_OPCODE; //HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }

        ll_past_cb_t *pPastCb = pAclConn->pPastCb;
        assert(pPastCb != NULL);

        if (pPastCb->pastMode == PAST_MODE_OFF) {
            return BLE_SUCCESS; // Ignore received PDU
        }

        rf_pkt_ll_periodic_sync_ind_t *pPerdSyncInd = (rf_pkt_ll_periodic_sync_ind_t *)pLlCtrlPkt;

        if (pPastCb->pastDecPending) {
            u32 r                   = irq_disable(); //IRQ boundary protect
            pPastCb->pastDecPending = NULL;
            irq_restore(r);
            blt_ll_pastDecRcvdPast(pAclConn);
        }

        if (pPastCb->pastRcvdSucc == FALSE) {
            my_dump_str_data(DBG_LL_PAST_EN, "unlikely err", 0, 0);
            return HCI_ERR_UNSPECIFIED_ERROR;
        }

    #if (LL_CON_PER_BV105C)
        if (pPastCb->pastRcvdTick && !clock_time_exceed(pPastCb->pastRcvdTick, 200)) {
            smemcpy(pastRcvdInd, pLlCtrlPkt, sizeof(rf_pkt_ll_periodic_sync_ind_t));
            my_dump_str_data(DBG_LL_PAST_EN, "keep rcvd LL_PAST_IND", &pPerdSyncInd->phy, 1);
            return BLE_SUCCESS;
        } else {
            pPastCb->pastRcvdTick = 0;
            my_dump_str_data(DBG_LL_PAST_EN, "pass rcvd LL_PAST_IND", &pPerdSyncInd->phy, 1);
        }
    #endif

        /* 0.Check PHY validity
         * if the PHY field of the LL_PERIODIC_SYNC_IND PDU
         * has no bits or more than one bit set, or the bit set corresponds to a PHY that
         * the recipient does not support or is reserved for future use, the recipient shall
         * ignore the PDU.
         */
        if ((pPerdSyncInd->phy & 0xF8) || (!(pPerdSyncInd->phy & 0x07))) {
            //HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE
            return BLE_SUCCESS; // Ignore received PDU
        } else {
            u8 suppPhyBits = PHY_PREFER_1M;
            if ((LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_2M_PHY << 8))) {
                suppPhyBits |= PHY_PREFER_2M;
            }
            if ((LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_CODED_PHY << 11))) {
                suppPhyBits |= PHY_PREFER_CODED;
            }

            if (!(pPerdSyncInd->phy & suppPhyBits)) {
                //HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE
                return BLE_SUCCESS; // Ignore received PDU
            }
            /* Multiple PHYs skipped */
            else if (pPerdSyncInd->phy == (PHY_PREFER_1M | PHY_PREFER_2M) ||
                     pPerdSyncInd->phy == (PHY_PREFER_1M | PHY_PREFER_CODED) ||
                     pPerdSyncInd->phy == (PHY_PREFER_1M | PHY_PREFER_2M | PHY_PREFER_CODED) ||
                     pPerdSyncInd->phy == (PHY_PREFER_2M | PHY_PREFER_CODED)) {
                return BLE_SUCCESS; // Ignore received PDU
            }
        }

        //1.Check  if the controller has already synced with the same perd_adv
        u8           temp_buffer[sizeof(extadv_id_t)];
        u8           advAdrIdType = 0;
        extadv_id_t *cur_pAdv     = (extadv_id_t *)temp_buffer;
        cur_pAdv->sid             = pPerdSyncInd->sid;

        cur_pAdv->adrType = pPerdSyncInd->aType;
        advAdrIdType      = cur_pAdv->adrType; //hci event used
        smemcpy(cur_pAdv->addr, pPerdSyncInd->advA, BLE_ADDR_LEN);

        //if peer used RPA , here we need to get it's Identity Address.
        int advA_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(pPerdSyncInd->aType, pPerdSyncInd->advA);
        if (advA_is_rpa) {                                                                 //RPA
    #if (LL_FEATURE_ENABLE_PRIVACY)
            my_dump_str_data(DBG_LL_PAST_EN, "PAST recipient: peer used RPA", 0, 0);
            ll_resolv_list_t *pRL_match = blt_ll_resolve_rpa(0, pPerdSyncInd->advA, NULL); //peer rpa check
            if (pRL_match && pRL_match->dev_iden_valid) {                                  //resolving pass, have a RL entry
                cur_pAdv->adrType = pRL_match->rlIdAddrType;
                smemcpy(cur_pAdv->addr, pRL_match->rlIdAddr, BLE_ADDR_LEN);
                advAdrIdType = pRL_match->rlIdAddrType == BLE_ADDR_PUBLIC ?
                                   OWN_ADDRESS_RESOLVE_PRIVATE_PUBLIC :
                                   OWN_ADDRESS_RESOLVE_PRIVATE_RANDOM; //TODO: rename MICRO
                my_dump_str_data(DBG_LL_PAST_EN, "RPA revert to Identity address", pRL_match->rlIdAddr, BLE_ADDR_LEN);
                my_dump_str_data(DBG_LL_PAST_EN, "              Identity addrType", &pRL_match->rlIdAddrType, 1);
            } else {
                my_dump_str_data(DBG_LL_PAST_EN, "RPA revert to Identity address failed", 0, 0);
                my_dump_str_data(DBG_LL_PAST_EN, "Identity address", cur_pAdv->addr, BLE_ADDR_LEN);
                my_dump_str_data(DBG_LL_PAST_EN, "Identity addrType", &cur_pAdv->adrType, 1);
            }
    #endif
        } else {
            my_dump_str_data(DBG_LL_PAST_EN, "Identity address", cur_pAdv->addr, BLE_ADDR_LEN);
            my_dump_str_data(DBG_LL_PAST_EN, "Identity addrType", &cur_pAdv->adrType, 1);
        }

        for (int i = 0; i < TSKNUM_PDA_SYNC; i++) {
            st_pda_sync_t *cur_p_pda_sync = (st_pda_sync_t *)&pdAsync_tbl[i];
            /* current device is neither already synchronized with nor in the process of synchronizing with */
            if (cur_p_pda_sync->sync_state != SYNC_STATE_IDLE) {
                if (!smemcmp(&cur_p_pda_sync->pda_id, cur_pAdv, sizeof(extadv_id_t))) {
                    //already in sync
                    my_dump_str_data(DBG_LL_PAST_EN, "already in sync", 0, 0);
                    return BLE_SUCCESS; //Ignore LL_PERIODIC_SYNC_IND
                }
            }
        }


        //2.Calculate Sync anchor point and sync window...
        sync_info_t *pSyncInfo = &pPerdSyncInd->syncInfo;
        u16          U         = (pSyncInfo->offsetUnit == EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US ? 30 : 300);
        u32          offsetUs  = pSyncInfo->syncPktOffset * U;
        offsetUs += pSyncInfo->offsetAdjust * 2457600;
        u16 CEref = pPerdSyncInd->connEvtCnt;

        my_dump_str_data(DBG_LL_PAST_EN, "offsetUs", &offsetUs, 4);
        my_dump_str_data(DBG_LL_PAST_EN, "CEref", &CEref, 2);

        u32 r = irq_disable();

        u16 conn_inst_mark;
        u32 conn_tick_mark;
        if (pAclConn->aclRole == ACL_ROLE_CENTRAL) {
            conn_tick_mark = pAclConn->ap_tick_mark;
            conn_inst_mark = pAclConn->conn_inst_mark;
        } else {
    #if (LL_ACL_PER_EN)
            u8 conn_sel     = pAclConn->acl_conIndex;
            u8 conn_sel_slv = conn_sel - LL_MAX_ACL_CEN_NUM; //pay attention here
            bls_pconn       = (st_lls_conn_t *)&blmsSlave[conn_sel_slv];
            conn_inst_mark  = bls_pconn->evtCnt_mark_1strx;
            conn_tick_mark  = bls_pconn->tick_mark_1strx;
    #else
            irq_restore(r);
            return HCI_ERR_UNSPECIFIED_ERROR;
    #endif
        }

        my_dump_str_data(DBG_LL_PAST_EN, "conn_tick_mark", &conn_tick_mark, 4);
        my_dump_str_data(DBG_LL_PAST_EN, "conn_inst_mark", &conn_inst_mark, 2);

        /* Determining the expected CEc and its anchors */
        u16 CEc            = conn_inst_mark + 1; //Not sure ? attempt to receive in next CE
        u32 attemptAclTime = conn_tick_mark;
        my_dump_str_data(DBG_LL_PAST_EN, "CEc", &CEc, 2);

        if ((u16)(CEc - conn_inst_mark) < 32767) { //0xFFFF -1
            attemptAclTime += (CEc - conn_inst_mark) * pAclConn->conn_intvl_tick;
            my_dump_str_data(DBG_LL_PAST_EN, "attemptAclTime0", &attemptAclTime, 4);
        } else {
            attemptAclTime -= (conn_inst_mark - CEc) * pAclConn->conn_intvl_tick;
            my_dump_str_data(DBG_LL_PAST_EN, "attemptAclTime1", &attemptAclTime, 4);
        }

        irq_restore(r);

        u16 PEa = pSyncInfo->evtCounter;
        u16 PEb = pPerdSyncInd->lastPaEvtCnt;
        u16 PEc = 0; //attempt to receive.  calculate below

        u16 CEs = pPerdSyncInd->syncConnEvtCnt;
        u16 CEt = pPastCb->pastRcvdCEt;

        my_dump_str_u32s(DBG_LL_PAST_EN, "PEa-PEb-CEs-CEt", PEa, PEb, CEs, CEt);

        u16 CAa = scaPpmTbl[pSyncInfo->chm[4] >> 5];       //scaPpmA
        u16 CAb = scaPpmTbl[pPerdSyncInd->sca];            //scaPpmB
    #if BLMS_PM_ENABLE
        u16 CAc = scaPpmTbl[SCA_MASTER_SLAVE_251_500_PPM]; //scaPpmC
    #else
        u16 CAc = scaPpmTbl[SCA_MASTER_SLAVE_31_50_PPM]; //scaPpmC
    #endif
        my_dump_str_u32s(DBG_LL_PAST_EN, "CAa-CAb-CAc", CAa, CAb, CAc, 0);

        u32 ceref_offset_us  = offsetUs;
        u32 perd_interval_us = pPerdSyncInd->syncInfo.itvl * 1250;
        u32 conn_interval_us = pAclConn->conn_intvl_tick / SYSTEM_TIMER_TICK_1US;
        u32 conn_task_dur_us = pAclConn->sSlot_allocNum * SSLOT_US_NUM + SLOT_PROCESS_MAX_US;
        my_dump_str_data(DBG_LL_PAST_EN, "perd_itvl_us", &perd_interval_us, 4);
        my_dump_str_data(DBG_LL_PAST_EN, "conn_itvl_us", &conn_interval_us, 4);
        my_dump_str_data(DBG_LL_PAST_EN, "conn_tsk_dur_us", &conn_task_dur_us, 4);

        /* Determine the expected PEc (calculate the Target) and its anchor point */
        //Reverse the offset corresponding to PEc
        if ((u16)(CEref - CEc) < 32767) { //future
            u32 jumpPrdNum = ((u64)(CEref - CEc) * conn_interval_us - conn_task_dur_us + ceref_offset_us) / perd_interval_us;
            PEc            = PEa - jumpPrdNum;

    #if (DBG_LL_PAST_EN) //debug used,
            s64 temp0 = ((s64)(CEref - CEc) * conn_interval_us - jumpPrdNum * perd_interval_us);
                //s64 temp1 = (s64)(conn_task_dur_us - ceref_offset_us);
                //if(temp0 == temp1)my_dump_str_data(DBG_LL_PAST_EN, "@temp0==temp1", 0, 0);
    #endif

            offsetUs = (((s64)(CEref - CEc) * conn_interval_us - jumpPrdNum * perd_interval_us) + ceref_offset_us);

    #if (DBG_LL_PAST_EN) //debug used,
            my_dump_str_data(DBG_LL_PAST_EN, "@temp", &temp0, 8);
            //my_dump_str_data(DBG_LL_PAST_EN, "@temp1", &temp1, 8);
            my_dump_str_data(DBG_LL_PAST_EN, "@jumpPrdNum", &jumpPrdNum, 4);
            my_dump_str_data(DBG_LL_PAST_EN, "@PEc", &PEc, 2);
            my_dump_str_data(DBG_LL_PAST_EN, "@Target[offsetUs]", &offsetUs, 4);
    #endif
        } else { // past
            u32 jumpPrdNum           = 0;
            u64 delta_CEc_plus_CEref = ((u64)((s16)CEc - (s16)CEref) * conn_interval_us);
            if (delta_CEc_plus_CEref + conn_task_dur_us >= ceref_offset_us) {
                jumpPrdNum = 1 + (delta_CEc_plus_CEref + conn_task_dur_us - ceref_offset_us) / perd_interval_us;
            } else {
                my_dump_str_data(DBG_LL_PAST_EN, "|TCEc-TCEref| < CEref_offset", 0, 0);
            }
            PEc      = PEa + jumpPrdNum;
            s64 temp = (s64)(delta_CEc_plus_CEref - jumpPrdNum * perd_interval_us);
            offsetUs = -(s64)(delta_CEc_plus_CEref - jumpPrdNum * perd_interval_us) + ceref_offset_us;
            (void)temp; //remove compiler warning
            my_dump_str_data(DBG_LL_PAST_EN, "!temp", &temp, 8);
            my_dump_str_data(DBG_LL_PAST_EN, "!jumpPrdNum", &jumpPrdNum, 4);
            my_dump_str_data(DBG_LL_PAST_EN, "!PEc", &PEc, 2);
            my_dump_str_data(DBG_LL_PAST_EN, "!Target[offsetUs]", &offsetUs, 4);
        }

        /*
         *  Tnominal <= Target < Tnominal + U
         *              Tnominal = (CEref - CEc) * CI + Offset - (PEa - PEc) * PAI
         *          =>  Tnominal - D - 16 us * Target < Tnominal + U + D + 16 us
         *          =>  Tnominal = (PEc - PEb) * PAI + (TPEb - TCEs) - (CEc - CEs) * CI
         *
         *  D = (Da + Db) * (1 + CAa + CAb + CAc)
         *      Da = |PEc -PEb| * PAI * (CAa + CAc)
         *      Db = |CEt - CEs| * CI * (CAb + CAc)
         *
         */
        u32 Da = ((((u16)(PEc - PEb) < 32767 ? (PEc - PEb) : (PEb - PEc)) * pPerdSyncInd->syncInfo.itvl * 1250) * (CAa + CAc) + 999999) / 1000000;
        u32 Db = ((((u16)(CEt - CEs) < 32767 ? (CEt - CEs) : (CEs - CEt)) * pAclConn->conn_intvl_n_1m25 * 1250) * (CAb + CAc) + 999999) / 1000000;

        u32 D           = ((Da + Db) * (1 + CAa + CAb + CAc) + 999999) / 1000000;
        u32 rxSyncDlyUs = ((16 + D) << 1) + U; /* window widen */
        my_dump_str_data(DBG_LL_PAST_EN, "(16+D)*2+U", &rxSyncDlyUs, 4);

        if (offsetUs > 2457600 * 2) {
            offsetUsForLargeInterval = offsetUs;
        }

        //3.re-calculate sync_info: "offset-unit-adjust" field
        u8  offsetAdjust = 0;
        u8  offsetUnit   = EXT_ADV_PDU_SYNC_OFFSET_UNITS_30_US;
        u16 syncPktOffset;
        int num_us_30u = offsetUs / 30; //Round down 1 unit
        num_us_30u     = num_us_30u * 30;
        if (num_us_30u >= 245700 + 30) {
            if (num_us_30u > 2457600) {
                offsetAdjust = 1;
                num_us_30u -= 2457600;
            }
            int number_300us = num_us_30u / 300;
            offsetUnit       = EXT_ADV_PDU_SYNC_OFFSET_UNITS_300_US;
            syncPktOffset    = number_300us;
        } else {
            int number_30us = offsetUs / 30; //Round down 1 unit
            syncPktOffset   = number_30us;
        }

        my_dump_str_u32s(DBG_LL_PAST_EN, "sync_info: offsetUs-syncPktOffset-offsetUnit-offsetAdjust", offsetUs, syncPktOffset, offsetUnit, offsetAdjust);

        //3. Create sync: used below API to create sync
        option_msk_t options = SYNC_ADV_SPECIFY;
        //        if(pAclConn->pastMode == PAST_MODE_OFF){ //the above code has already checked
        //            //my_dump_str_data(DBG_LL_PAST_EN, "PAST_MODE_OFF: Ignore received LL_PERIODIC_SYNC_IND", 0, 0);
        //            return BLE_SUCCESS; //Ignore LL_PERIODIC_SYNC_IND
        //        }
        //        else
        if (pPastCb->pastMode == PAST_MODE_RPT_DISABLED) {
            options |= REPORTING_INITIALLY_DIS;
        } else if (pPastCb->pastMode == PAST_MODE_RPT_ENABLED_DUP_FILTER_DIS) { // rpting_enable dup filter disabled => 0, needless
            options |= REPORTING_INITIALLY_EN | DUPLICATE_FILTERING_INITIALLY_DIS;
        } else if (pPastCb->pastMode == PAST_MODE_RPT_ENABLED_DUP_FILTER_EN) {
            options |= REPORTING_INITIALLY_EN | DUPLICATE_FILTERING_INITIALLY_EN;
        }

        //4.replace pda_cache:  put received concerned sync info into pda_cache table
        pda_cache_t *p_pda_cache_idle   = NULL;
        pda_cache_t *p_pda_cache_oldest = NULL;
        pda_cache_t *pPdA_cache         = NULL;
        int          existed_cache_dev  = 0;

        r = irq_disable();

        for (int i = 0; i < PERDADV_CACHE_NUM; i++) {
            pPdA_cache = (pda_cache_t *)&pdaCache_tbl[i];

            if (pPdA_cache->cach_flag == CACHE_FLAG_OCCUPIED) {
                if (!smemcmp(cur_pAdv, &pPdA_cache->pda_dev_id, sizeof(extadv_id_t))) {
                    existed_cache_dev = 1;
                    //my_dump_str_data(DBG_LL_PAST_EN, "cache 0", 0, 0);
                    pPdA_cache->seq_number = 0; //mark 1st received sync_info
                    break;
                } else {
                    //my_dump_str_data(DBG_LL_PAST_EN, "cache 1", 0, 0);
                    p_pda_cache_oldest     = pPdA_cache;
                    pPdA_cache->seq_number = 0; //mark 1st received sync_info
                }
            } else if (pPdA_cache->cach_flag == CACHE_FLAG_IDLE) {
                if (!p_pda_cache_idle) {
                    //my_dump_str_data(DBG_LL_PAST_EN, "cache 2", 0, 0);
                    p_pda_cache_idle = pPdA_cache; //use the first cache table
                }
            } else {                               //CACHE_FLAG_SYNCING & CACHE_FLAG_SYNCED
                //my_dump_str_data(DBG_LL_PAST_EN, "cache 3", 0, 0);
                continue;
            }
        }

        if (!existed_cache_dev) {
            if (p_pda_cache_idle) {
                pPdA_cache = p_pda_cache_idle;
                //              bltPdaSync.pdA_cacheNum ++;
            } else if (p_pda_cache_oldest) {
                pPdA_cache = p_pda_cache_oldest;
            }
            my_dump_str_data(DBG_LL_PAST_EN, "CACHE_FLAG_OCCUPIED", 0, 0);
            pPdA_cache->cach_flag = CACHE_FLAG_OCCUPIED;
            smemcpy(&pPdA_cache->pda_dev_id, cur_pAdv, sizeof(extadv_id_t));

            /* Convert 'le_phy_prefer_type_t' to le_phy_type_t, used to keep pPdA_cache->prdphy */
            u8 prdPhy = BLE_PHY_1M;
            for (u8 phyIdx = BLE_PHY_1M; phyIdx <= BLE_PHY_CODED; phyIdx++) {
                if (BIT(phyIdx - 1) == pPerdSyncInd->phy) { //pPerdSyncInd->phy: refer to 'pc_phy_bits_t'
                    prdPhy = phyIdx;
                    break;
                }
            }
            pPdA_cache->prdphy = prdPhy; //refer to 'le_phy_type_t'
            smemcpy(&pPdA_cache->sncInf, &pPerdSyncInd->syncInfo, sizeof(sync_info_t));
        }

        pPdA_cache->header_tick_backup   = attemptAclTime; //Anchor point corresponding to the desired ACL connect event
        pPdA_cache->sncInf.syncPktOffset = syncPktOffset;
        pPdA_cache->sncInf.offsetAdjust  = offsetAdjust;
        pPdA_cache->sncInf.offsetUnit    = offsetUnit;
        pPdA_cache->sncInf.evtCounter    = PEc;
        pPdA_cache->syncWwUs             = rxSyncDlyUs;

    #if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)
        pPdA_cache->pawr_acad.num_subevent = 0; //will be used to judge whether is PAwR sync. so every time need to clear its value.
        if (opcode == LL_PERIODIC_SYNC_WR_IND) {
            rf_pkt_ll_periodic_sync_wr_ind_t *tmpPerdSyncInd = (rf_pkt_ll_periodic_sync_wr_ind_t *)pPerdSyncInd;
            smemcpy(&pPdA_cache->pawr_acad, &tmpPerdSyncInd->pawrAcadInfo, sizeof(pawr_acad_t));
            pPdA_cache->pawr_acad_valid = 1;

            u32 pawr_interval_us     = pSyncInfo->itvl * 1250;
            pPastCb->pastSyncTimeout = pawr_interval_us * PAWR_TIMEOUT_NUMBER / 10000;

        } else {
            pPdA_cache->pawr_acad_valid = 0;
        }
    #endif

        pPastCb->pastCreateSync = 1;

        irq_restore(r);

        /*
         * The existing API is used here. The advantage is that there is no need to
         * rewrite similar APIs. The createSync here is based on the extScan mechanism.
         */
        u8 status = blc_ll_periodicAdvertisingCreateSync(options, cur_pAdv->sid, cur_pAdv->adrType, cur_pAdv->addr, pPastCb->pastSkip, pPastCb->pastSyncTimeout, pPastCb->pastSyncCteType);

        for (int i = 0; i < TSKNUM_PDA_SYNC; i++) { //use "LL_MAX_ACL_CEN_NUM" here
            st_pda_sync_t *pPdaSync = (st_pda_sync_t *)&pdAsync_tbl[i];

            /*
             * Note: by design, it should be able to be called here. If it cannot be satisfied
             * here, we can only ignore the received LL_PERIODIC_SYNC_IND packet.Ignore this
             * packet, no risk.
             */
            if (!smemcmp(&pPdaSync->pda_id, cur_pAdv, sizeof(extadv_id_t))) {
                r = irq_disable();

                /* mark adv_addr_id_type: 0,1,2,3 hci event will used */
                pPdaSync->adrIdType = advAdrIdType;

                if (status == BLE_SUCCESS) {
                    my_dump_str_data(DBG_LL_PAST_EN, "PAST create sync success", 0, 0);
                    pPdaSync->createSyncType = PDA_CREATE_SYNC_BY_PAST;
                    pPastCb->perServiceData  = pPerdSyncInd->id;
                    pPastCb->perSyncHandle   = pPdaSync->pda_index | BLT_SYNC_HANDLE;
                } else { // create sync failed, immediately mark in loop
                    pPdaSync->sync_establish = SYNC_ESTABLISHED_BY_PAST;
                    pPdaSync->sync_state     = SYNC_STATE_IDLE;

                    my_dump_str_data(DBG_LL_PAST_EN, "PAST create sync failed", 0, 0);
                }

                irq_restore(r);

                break;
            }
        }
    }

    my_dump_str_data(DBG_LL_PAST_EN, "PAST BLE_SUCCESS", 0, 0);
    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to instruct the Controller to send synchronization information
 *             about the periodic advertising train identified by the Sync_Handle parameter to a connected
 *             device.
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @param[in]  serviceData - A value provided by the Host
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_noinline_
    ble_sts_t
    blc_ll_periodicAdvSyncTransfer(u16 connHandle, u16 serviceData, u16 syncHandle)
{
    if (IS_LEGACY_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    SET_EXTENDED_SCAN_VALID;

    /* If the periodic advertising train corresponding to the Sync_Handle parameter does not exist,
     * then the Controller shall return the error code Unknown Advertising Identifier (0x42). */
    if (!blt_isSyncHandleValid(syncHandle)) {
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    /*
     *  If the Connection_Handle parameter does not identify a current connection, the
     *  Controller shall return the error code Unknown Connection Identifier (0x02).
     */
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc      = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_past_cb_t *pPastCb = pc->pPastCb;
    /*
     * If the remote device has not indicated support for the Periodic Advertising Sync
     * Transfer - Recipient feature, the Controller shall return the error code
     * Unsupported Remote Feature / Unsupported LMP Feature (0x1A).
     */
    #if BQB_TEST_EN
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_SENDER << 24))) {
    #else
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_SENDER << 24)) ||
        !(pc->ll_remoteFeature0 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECIPIENT)) {
    #endif
        //Sender should support to send LL_PERIODIC_SYNC_IND and Recipient should support to receive LL_PERIODIC_SYNC_IND
        /*
         * The Link Layer shall not transmit a packet containing an LL Control PDU with a
         * CtrData field longer than 26 octets until it has successfully completed a Feature
         * Exchange procedure (see Section 5.1.4) on the same connection.
         *
         * if (pc->ll_remoteFeature0 == 0), it means Feature Exchange procedure is not yet complete.
         */
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    assert(pPastCb != NULL);
    pPastCb->perServiceData = serviceData;
    pPastCb->perSyncHandle  = syncHandle;
    pPastCb->perSyncSrc     = PAST_SYNC_SRC_SCAN; //Periodic sync info from ext_scanner

    pPastCb->pastSendPending = 1;


    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to to instruct the Controller to send synchronization information
 *             about the periodic advertising in an advertising set to a connected device.
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @param[in]  serviceData - A value provided by the Host
 * @param[in]  advHandle - Used to identify an advertising set: 0x00�C0xEF
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_noinline_
    ble_sts_t
    blc_ll_periodicAdvSetInfoTransfer(u16 connHandle, u16 serviceData, u8 advHandle)
{
    if (IS_LEGACY_SCAN_VALID) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    SET_EXTENDED_SCAN_VALID;

    /*
     *  If the Connection_Handle parameter does not identify a current connection, the
     *  Controller shall return the error code Unknown Connection Identifier (0x02).
     */
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc      = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_past_cb_t *pPastCb = pc->pPastCb;

    /*
     * If the remote device has not indicated support for the Periodic Advertising Sync
     * Transfer - Recipient feature, the Controller shall return the error code
     * Unsupported Remote Feature (0x1A).
     */
    #if BQB_TEST_EN
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_SENDER << 24))) {
    #else
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_SENDER << 24)) ||
        !(pc->ll_remoteFeature0 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECIPIENT)) {
    #endif
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    assert(pPastCb != NULL);
    /*
     * If the advertising set corresponding to the Advertising_Handle parameter does
     * not exist, the Controller shall return the error code Unknown Advertising Identifier (0x42)
     */
    st_ext_adv_t *cur_pextadv = NULL;
    if (advHandle == INVALID_ADVHD_FLAG) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    } else {
        cur_pextadv = blt_extadv_search_existed_adv_set(advHandle);
        /*If the advertising set corresponding to the Advertising_Handle parameter does
        not exist, then the Controller shall return the error code Unknown Advertising
        Identifier (0x42). */
        if (!cur_pextadv) {
            return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
        }
    }

    /*
     * If periodic advertising is not currently in progress for the advertising set, the
     * Controller shall return the error code Command Disallowed (0x0C).
     */
    if (!cur_pextadv->prdadv_task_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    pPastCb->perSyncSrc     = PAST_SYNC_SRC_BCST; //Periodic sync info from ext_broadcaster
    pPastCb->perServiceData = serviceData;
    pPastCb->perSyncHandle  = advHandle;

    pPastCb->pastSendPending = 1;

    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to specify how the Controller will process periodic advertising
 *             synchronization information received from the device identified by the Connection_Handle
 *             parameter (the "transfer mode")
 * @param[in]  connHandle - Connection_Handle Range: 0x0000 to 0x0EFF
 * @param[in]  mode -
 * @param[in]  skip - The number of periodic advertising packets that can be skipped after a successful receive
 *                    Range: 0x0000 to 0x01F3
 * @param[in]  syncTimeout - Synchronization timeout for the periodic advertising train Range: 0x000A to 0x4000
 *                           Time = N*10 ms: Time Range: 100 ms to 163.84 s
 * @param[in]  cteType -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_noinline_
    ble_sts_t
    blc_ll_setPeriodicAdvSyncTransferParams(u16 connHandle, u8 mode, u16 skip, u16 syncTimeout, u8 cteType)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) { //SCAN_LEGACY_MASK
        return HCI_ERR_CMD_DISALLOWED;
    }

    SET_EXTENDED_ADV_VALID; // SCAN_EXTENDED_MASK

    /*
     *  If the Connection_Handle parameter does not identify a current connection, the
     *  Controller shall return the error code Unknown Connection Identifier (0x02).
     */
    if (blt_ll_isAclhdlInvalid(connHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    st_ll_conn_t *pc      = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);
    ll_past_cb_t *pPastCb = pc->pPastCb;

    if (skip > PAST_MAX_SKIP ||
        mode > PAST_MODE_TOTAL ||
        syncTimeout < PAST_SYNC_MIN_TIMEOUT || syncTimeout > PAST_SYNC_MAX_TIMEOUT) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * The PAST sender has relevant checks. When SPEC describes the HCI commands
     * related to the PAST receiver, there is no relevant description.
     */
    #if BQB_TEST_EN
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25))) {
    #else
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25)) ||
        !(pc->ll_remoteFeature0 & LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_SENDER)) {
    #endif
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    assert(pPastCb != NULL);
    pPastCb->pastMode        = mode;
    pPastCb->pastSkip        = skip;
    pPastCb->pastSyncTimeout = syncTimeout;
    pPastCb->pastSyncCteType = cteType;

    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to specify the initial value for the mode, skip, timeout, and
 *             Constant Tone Extension type (set by the HCI_LE_Set_Periodic_Advertising_Sync_Transfer_Parameters command;
 *             see Section 7.8.91) to be used for all subsequent connections over the LE transport.
 * @param[in]  mode -
 * @param[in]  skip - The number of periodic advertising packets that can be skipped after a successful receive
 *                    Range: 0x0000 to 0x01F3
 * @param[in]  syncTimeout - Synchronization timeout for the periodic advertising train Range: 0x000A to 0x4000
 *                           Time = N*10 ms: Time Range: 100 ms to 163.84 s
 * @param[in]  cteType -
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
_attribute_noinline_
    ble_sts_t
    blc_ll_setDftPeriodicAdvSyncTransferParams(u8 mode, u16 skip, u16 syncTimeout, u8 cteType)
{
    //HCI/GEV/BV-02-C [Disallow Mixing Legacy and Extended Advertising Commands]
    if (IS_LEGACY_ADV_VALID) { //SCAN_LEGACY_MASK
        return HCI_ERR_CMD_DISALLOWED;
    }

    SET_EXTENDED_ADV_VALID; //SCAN_EXTENDED_MASK

    if (skip > PAST_MAX_SKIP || mode > PAST_MODE_TOTAL ||
        (syncTimeout < PAST_SYNC_MIN_TIMEOUT || syncTimeout > PAST_SYNC_MAX_TIMEOUT)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * The PAST sender has relevant checks. When SPEC describes the HCI commands
     * related to the PAST receiver, there is no relevant description.
     */
    if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25))) {
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    blt_PastMng.pastMode        = mode;
    blt_PastMng.pastSkip        = skip;
    blt_PastMng.pastSyncTimeout = syncTimeout;
    blt_PastMng.cteType         = cteType; //TODO: AOA/AOD concerned

    return BLE_SUCCESS;
}

/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_periodicAdvSyncTransfer(hci_le_pastCmdParams_t *cmdPara, hci_le_pastRetParams_t *retPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Periodic_Ad_Sync_Transfer", cmdPara, sizeof(hci_le_pastCmdParams_t));
    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_periodicAdvSyncTransfer(cmdPara->connHandle, cmdPara->serviceData, cmdPara->syncHandle);

    return retPara->status;
}

/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_periodicAdvSetInfoTransfer(hci_le_paSetInfoTransferCmdParams_t *cmdPara, hci_le_paSetInfoTransferRetParams_t *retPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Periodic_Ad_Set_Info_Transfer", cmdPara, sizeof(hci_le_paSetInfoTransferCmdParams_t));
    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_periodicAdvSetInfoTransfer(cmdPara->connHandle, cmdPara->serviceData, cmdPara->advHandle);

    return retPara->status;
}

/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_setPeriodicAdvSyncTransferParams(hci_le_pastParamsCmdParams_t *cmdPara, hci_le_pastParamsRetParams_t *retPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_Periodic_Adv_Sync_Transfer_Parameters", cmdPara, sizeof(hci_le_pastParamsCmdParams_t));
    retPara->connHandle = cmdPara->connHandle;
    retPara->status     = blc_ll_setPeriodicAdvSyncTransferParams(cmdPara->connHandle, cmdPara->mode, cmdPara->skip, cmdPara->syncTimeout, cmdPara->cteType);

    return retPara->status;
}

/**
 * @brief      This function is used to enable or disable reports for the periodic advertising train
 *             identified by the Sync_Handle parameter
 * @param[in]  sync_handle - Sync_Handle identifying the periodic advertising train
 * @param[in]  enable
 * @return     Status - 0x00: command succeeded; 0x01-0xFF: command failed
 */
ble_sts_t blc_hci_le_setDftPeriodicAdvSyncTransferParams(hci_le_dftPastParamsCmdParams_t *cmdPara)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LE_Set_efault_Periodic_Adv_Sync_Transfer_Params", cmdPara, sizeof(hci_le_dftPastParamsCmdParams_t));
    return blc_ll_setDftPeriodicAdvSyncTransferParams(cmdPara->mode, cmdPara->skip, cmdPara->syncTimeout, cmdPara->cteType);
}


#endif //end of LL_FEATURE_ENABLE_PAST
