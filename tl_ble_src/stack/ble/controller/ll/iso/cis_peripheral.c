/********************************************************************************************************
 * @file    cis_peripheral.c
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


#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)


_attribute_aligned_(4)  ll_cis_slv_t        *global_pCisSlv = NULL;
_attribute_aligned_(4)  ll_cis_slv_t        *blt_pCisSlv = NULL;

_attribute_aligned_(4)  cis_slv_para_t      cisSlv_param;


ble_sts_t   blc_ll_initCisPeriphrModule_initCisPeriphrParametersBuffer(u8 *pCisPerParamBuf, int cis_per_num)
{
    #if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
        STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_cis_slv_t)),     cis_peripheral);
        STATIC_ASSERT_FILE(CIS_SLV_PARAM_LEN == sizeof(ll_cis_slv_t), cis_peripheral);
    #endif

    if(cis_per_num > LL_CIG_SLV_NUM_MAX){
        return LL_ERR_INVALID_PARAMETER;
    }

    LL_FEATURE_MASK_0 |= LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_SLAVE;

    /* Special protection code for use */
    if(pm_check_info){
        ll_cis_slv_irq_task_cb      = blt_cig_slv_interrupt_task;
        ll_cis_slv_mlp_task_cb      = blt_cig_slv_mainloop_task;
        ll_cis_slave_ctrl_handler   = blt_ll_cis_slave_control_pdu_process;
    }

    blmsParam.cis_per_en = 1; //can only use 1 or 0, for "blc_hci_read Local Supported Commands"
    bltCisMng.maxNum_cig_slv = cis_per_num;
    cisSlv_param.connAcpt_to_us = 5000000;  //default 5S

    global_pCisSlv = (ll_cis_slv_t *)pCisPerParamBuf;


    ll_cis_slv_t * pCigSlave;
    for(int i=0; i< cis_per_num; i++){

        pCigSlave = (ll_cis_slv_t *)(global_pCisSlv + i);


        //TODO ciss_arrgmtMap_msk
        pCigSlave->cig_slave_index = i;
        pCigSlave->ciss_se_en_num = 0;
        pCigSlave->cig_slv_occupied = 0;

        for(int j=0; j<CIG_SLV_FIFONUM; j++){
            pCigSlave->cigs_schTsk_fifo[j].scheTask_oft = TSKOFT_CIG_SLV + i;
            pCigSlave->cigs_schTsk_fifo[j].scheTask_idx = i;
            pCigSlave->cigs_schTsk_fifo[j].scheTask_flg = TSKFLG_CIG_SLV;
        }


        blt_ll_setSchedulerTaskPriority(TSKOFT_CIG_SLV + i, TASK_PRIORITY_HIGH_THRES);
    }

    return BLE_SUCCESS;
}


_attribute_ram_code_
int         blt_cig_slv_interrupt_task(int flag, void*p)
{
    int cigs_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if(flag & FLAG_SCHEDULE_CIGSLV_START){
        blt_cig_slv_start(cigs_idx);
    }
    else if(flag & FLAG_SCHEDULE_CISSLV_START){
        blt_crx_start();
    }
    else if(flag & FLAG_SCHEDULE_CISSLV_POST){
        blt_crx_post(blt_pCisConn);
    }
    else if(flag & FLAG_SCHEDULE_CIGSLV_BUILD){
        blt_ll_buildCisSlaveSchedulerLinklist();
    }
    else if(flag & FLAG_SCHEDULE_CIGSLV_GET1ST_AP){
        blt_ll_calcCigSlv1stAndCis1stAnchorPoint();
    }
    else if(flag & FLAG_INSERT_SCHTSK_CONFLICT){
        sch_task_t *pTgtTsk = (sch_task_t *)p;
        u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        u8 curSchTaskOft = TSKOFT_CIG_SLV + cigs_idx;
        (void)tgtTskFlg; //remove compiler warning
        #if(SCH_TASK_PRIORITY_IN_CB_EN)
            s32 pri_taskCur = bltPri.pri_cal[curSchTaskOft];
            s32 pri_taskTra = bltPri.pri_cal[pTgtTsk->scheTask_oft];
             //priority higher than exist task, can insert target task
            if(pri_taskCur > pri_taskTra){
                return 1;
            }
        #endif
        
        tlkapi_send_string_data(0, "[cis_slv]insertTsk conflict, tgtTsk=", &tgtTskFlg, 1);

    }
    return 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_cig_slv_mainloop_task(int flag)
{
    if(flag == FLAG_MODULE_RESET){
        blt_ll_reset_cig_slv();
    }
    else if(flag == FLAG_MODULE_MAINLOOP){
        blt_ll_cigSlvMainloop();
    }

    return 0;
}


_attribute_noinline_
void        blt_ll_reset_cig_slv(void)
{
    //TODO: reset CIG SLV module

    ll_cis_slv_t * pCigSlave;
    for(int i=0; i< bltCisMng.maxNum_cig_slv; i++){

        pCigSlave = (ll_cis_slv_t *)(global_pCisSlv + i);

        pCigSlave->cigs_working = 0;
        pCigSlave->ciss_task_cnt = 0;
        pCigSlave->ciss_task_msk = 0;

        pCigSlave->ciss_se_en_num = 0;
        pCigSlave->cig_slv_occupied = 0;
        pCigSlave->ciss_arrgmtMap_en_msk = 0;

    }

    //cis flow clear
}


int         blt_ll_cis_slave_cis_establish(ll_cis_conn_t * pCisConn, ll_cis_slv_t * pCigSlave, ble_sts_t status)
{

    if(status == BLE_SUCCESS){
        bltCisMng.cisFlow_pending &= ~BIT(pCisConn->cis_index);
        bltCisMng.cisFlow_idx = INVALID_CIS_IDX;
        pCisConn->cisFlowFlg = CIS_FLOW_IDLE;

        blt_cis_establish_common(pCisConn);
        #if (LL_ASYNC_LEA_EN)
        if(asyncCtrl.leaUsed)
        {
            blt_async_cisConnCallback(pCisConn->cis_sync_delay,pCisConn->iso_intvl_us);
        }
        #endif
    }
    else{
        blt_cis_clearCisSlaveMainloopStatus(pCisConn, pCigSlave);
    }


    if(hci_le_eventMask & HCI_LE_EVT_MASK_CIS_ESTABLISHED)
    {
        hci_le_cisEstablished_evt(status, pCisConn->cis_connHandle, (u8*)&pCisConn->cig_sync_delay,
                                 (u8*)&pCisConn->cis_sync_delay, (u8*)&pCisConn->transLaty_m2s,
                                 (u8*)&pCisConn->transLaty_s2m, pCisConn->curCisPhy, pCisConn->curCisPhy,
                                 pCisConn->nse, pCisConn->bn_peer, pCisConn->bn_loca, pCisConn->ft_peer,
                                 pCisConn->ft_loca, pCisConn->max_pdu_peer, pCisConn->max_pdu_loca,
                                 pCigSlave->ciss_isoIntvl);
    }

    return 1;
}




ble_sts_t   blc_ll_acceptCisRequest(u16 cisHandle)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Accept_Cis_Req", &cisHandle, 2);

    tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_CMD), "[LL][CMD] Accept_Cis_Req", &cisHandle, 2);

    /*
    If the Peripheral's Host issues this command with a Connection_Handle that
    does not exist, or the Connection_Handle is not for a CIS, the Controller shall
    return the error code Unknown Connection Identifier (0x02).

    If the Peripheral's Host issues this command with a Connection_Handle for a
    CIS that has already been established or that already has an HCI_LE_-
    Accept_CIS_Request or HCI_LE_Reject_CIS_Request command in progress,
    the Controller shall return the error code Command Disallowed (0x0C).

    If the Central's Host issues this command, the Controller shall return the error
    code Command Disallowed (0x0C)
     */

    ll_cis_conn_t * pCisConn = blt_isCisAllocated_by_handle(cisHandle);
    if(pCisConn){
        if(pCisConn->cisRole == CIS_ROLE_MASTER){
            return HCI_ERR_CMD_DISALLOWED; // HCI/CIS/BV-02-C test this logic
        }
        else if(pCisConn->cis_established){
            return HCI_ERR_CMD_DISALLOWED;
        }
        else if(pCisConn->ack_CisReq){
            return HCI_ERR_CMD_DISALLOWED;
        }

        if(pCisConn->cisFlowFlg == CIS_FLOW_SLAVE_REQ_HOST){
            pCisConn->cisFlowFlg = CIS_FLOW_SLAVE_SEND_CIS_RSP;
            pCisConn->ack_CisReq = CIS_REQ_ACCEPT;
        }
    }
    else{
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}


ble_sts_t   blc_ll_rejectCisReq(u16 cisHandle, u8 reason)
{
    /*
    If the Peripheral's Host issues this command with a Connection_Handle that is
    not for a CIS, the Controller shall return the error code Unknown Connection
    Identifier (0x02).

    If the Peripheral's Host issues this command with a Connection_Handle for a
    CIS that has already been established or that already has an HCI_LE_-
    Accept_CIS_Request or HCI_LE_Reject_CIS_Request command in progress,
    the Controller shall return the error code Command Disallowed (0x0C).

    If the Central's Host issues this command, the Controller shall return the error
    code Command Disallowed (0x0C).
    */
    ll_cis_conn_t * pCisConn = blt_isCisAllocated_by_handle(cisHandle);
    if(pCisConn){
        if(pCisConn->cisRole == CIS_ROLE_MASTER){
            return HCI_ERR_CMD_DISALLOWED; // HCI/CIS/BV-02-C test this logic
        }
        else if(pCisConn->cis_established){
            return HCI_ERR_CMD_DISALLOWED;
        }
        else if(pCisConn->ack_CisReq){
            return HCI_ERR_CMD_DISALLOWED;
        }


        if(pCisConn->cisFlowFlg == CIS_FLOW_SLAVE_REQ_HOST){
            pCisConn->cis_reject_reason = reason;  //reason none zero, Host can guarantee
            pCisConn->cisFlowFlg = CIS_FLOW_SLAVE_REJECT_CIS_REQ;

            pCisConn->ack_CisReq = CIS_REQ_REJECT;
        }
    }
    else{
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    return BLE_SUCCESS;
}



ble_sts_t   blc_hci_le_rejectCisReq(hci_le_rejectCisReq_cmdParams_t *pCmdParam, hci_le_rejectCisReq_retParams_t *pRetParam)
{
    tlkapi_send_string_data(IUT_HCI_LOG_EN, "[HCI][CMD] Reject_Cis_Req", pCmdParam, sizeof(hci_le_rejectCisReq_cmdParams_t));

    pRetParam->status = blc_ll_rejectCisReq(pCmdParam->cis_handle, pCmdParam->reason);
    pRetParam->cis_handle = pCmdParam->cis_handle;

    return pRetParam->status;
}


bool        blt_ll_sendCisRsp(ll_cis_conn_t *pCisConn)
{

    u8 temp_buff[12];  //sizeof(rf_packet_ll_cis_rsp_t) = 11
    rf_packet_ll_cis_rsp_t *pCisRsp = (rf_packet_ll_cis_rsp_t *)temp_buff;

    st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[pCisConn->link_acl_index];
//  ll_cis_slv_t *pCigSlave = (ll_cis_slv_t *)(global_pCisSlv + pCisConn->clink_cig_idx );

    pCisRsp->type = LLID_CONTROL;
    pCisRsp->rf_len = sizeof(rf_packet_ll_cis_rsp_t)-2;
    pCisRsp->opcode = LL_CIS_RSP;


    if(pCisConn->align_with_acl){

        #if 0 //CIS/PER/BV-20-C do not obey this
            if(pCisConn->peer_cisOffsetMax_us > pCisConn->cis_idle_us){
                tlkapi_send_string_u32s(CIS_DEBUG_EN, "[CISP][FLW] ERROR, offset max err", pCisConn->iso_intvl_us, pCisConn->cis_maxPossible_us, idle_time, pCisConn->peer_cisOffsetMax_us);
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x990C0000);
            }
        #endif


        u32 oftMinUs_safe = pAclConn->actual_txrx_sche_us + CIGSLV_EARLY_SET_US + 100;

        // 500: post + scheduler timing
        u32 oftMaxUs_safe = pCisConn->cis_idle_us - (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US + CIGSLV_EARLY_SET_US + 50);

        if(oftMaxUs_safe < oftMinUs_safe){
            oftMaxUs_safe = oftMinUs_safe;
        }


        #if (DBG_CIS_1ST_AP_TIMING_EN)
            u32 oct_oftMinUs = blt_debug_hex_2_dec_display(oftMinUs_safe);
            u32 oct_oftMaxUs = blt_debug_hex_2_dec_display(oftMaxUs_safe);

            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] 1ST AP, offset safe", oct_oftMinUs, oct_oftMaxUs, 0, 0);
        #endif

        if(pCisConn->peer_cisOffsetMax_us < oftMinUs_safe){
            pCisConn->own_cisOffsetMin_us = pCisConn->own_cisOffsetMax_us = pCisConn->peer_cisOffsetMax_us;
            pAclConn->limit_txrx_sche_us = pCisConn->peer_cisOffsetMax_us;
            tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] 1ST AP, smaller than min_safe", 0, 0);
        }
        else if(pCisConn->peer_cisOffsetMin_us > oftMaxUs_safe){
            pCisConn->own_cisOffsetMin_us = pCisConn->own_cisOffsetMax_us = pCisConn->peer_cisOffsetMin_us;
            tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] 1ST AP, bigger than max_safe", 0, 0);
        }
        else{
            pCisConn->own_cisOffsetMin_us = max2(oftMinUs_safe, pCisConn->peer_cisOffsetMin_us);
            pCisConn->own_cisOffsetMax_us = min2(oftMaxUs_safe, pCisConn->peer_cisOffsetMax_us);

            #if 0 //temp
                int diff = pCisConn->own_cisOffsetMax_us - pCisConn->own_cisOffsetMin_us;
                if(diff > 2000){
                    diff>>=2;  //1/4 difference
                    pCisConn->own_cisOffsetMin_us += diff;
                    pCisConn->own_cisOffsetMax_us -= diff;
                }
            #endif
        }
    }
    else{ //if no align
        //TODO: more detail
        pCisConn->own_cisOffsetMin_us = pCisConn->peer_cisOffsetMin_us;
        pCisConn->own_cisOffsetMax_us = pCisConn->peer_cisOffsetMax_us;
        int oft_diff = (pCisConn->peer_cisOffsetMax_us - pCisConn->peer_cisOffsetMin_us);
        if(oft_diff > 2000){
            oft_diff>>=2;
            pCisConn->own_cisOffsetMin_us += oft_diff;
            pCisConn->own_cisOffsetMax_us -= oft_diff;
        }
    }



    smemcpy(pCisRsp->cisOffsetMin, &pCisConn->own_cisOffsetMin_us, 3);//cisOffsetMin can choose appropriate parameters according to LL
    smemcpy(pCisRsp->cisOffsetMax, &pCisConn->own_cisOffsetMax_us, 3);//cisOffsetMax can choose appropriate parameters according to LL
    pCisRsp->connEventCnt = cisSlv_param.cisReq_aclEvtCnt;




    int status = blt_llmsPushLlCtrlPkt(pCisConn->link_acl_handle, LL_CIS_RSP, (u8 *)pCisRsp);
    if (status){
        pCisConn->cisFlowFlg = CIS_FLOW_SLAVE_WAIT_CIS_IND;
        pAclConn->ll_rsp_timeout_tick = clock_time() | 1;

        #if (DBG_CIS_1ST_AP_TIMING_EN)
            u32 oct_cis_min = blt_debug_hex_2_dec_display(pCisConn->own_cisOffsetMin_us);
            u32 oct_cis_max = blt_debug_hex_2_dec_display(pCisConn->own_cisOffsetMax_us);
            u32 oct_acl_evtcnt = blt_debug_hex_2_dec_display(pCisRsp->connEventCnt);

            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] 1ST AP, cis rsp param", oct_cis_min, oct_cis_max, oct_acl_evtcnt, pCisConn->align_with_acl);
        #endif
    }

    return status;
}


bool        blt_ll_rejectCisReq(ll_cis_conn_t *pCisConn, ll_cis_slv_t *pCigSlave)
{

    if(!pCisConn->cis_reject_reason){
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99070000);
    }

    st_ll_conn_t *pAclConn = (st_ll_conn_t*)&blms[pCisConn->link_acl_index];

    //if push data error, should pending
    int status = blt_llms_rejectInd(pCisConn->link_acl_handle, LL_CIS_REQ, pCisConn->cis_reject_reason, 1);
    if (status){
        blt_cis_clearCisSlaveMainloopStatus(pCisConn, pCigSlave);

        tlkapi_send_string_u32s(CIS_FLOW_LOG_EN, "[CISP][FLW] LL reject CIS req", pCisConn->cis_reject_reason, status, 0, 0);
        pAclConn->ll_rsp_timeout_tick = 0;
        pCisConn->cis_reject_reason = 0;
    }



    return status;


}


ble_sts_t   blt_llCisReqParamsChk(st_ll_conn_t* pc, rf_packet_ll_cis_req_t* pLlCisReq)
{
    (void)pc; //unused, remove warning

    u8 comm_phy = pLlCisReq->phyM2S & pLlCisReq->phyS2M;  //support symmetric PHYs only; support 1M/2M/Coded PHY all
    u32 subIntvl = MAKE_U24(pLlCisReq->subIntvl[2], pLlCisReq->subIntvl[1], pLlCisReq->subIntvl[0])& 0xfffff;
    u32 isoIntvl = pLlCisReq->isoIntvl * 1250;
    u32 cisOffsetMin = MAKE_U24(pLlCisReq->cisOffsetMin[2], pLlCisReq->cisOffsetMin[1], pLlCisReq->cisOffsetMin[0]);
    u32 cisOffsetMax = MAKE_U24(pLlCisReq->cisOffsetMax[2], pLlCisReq->cisOffsetMax[1], pLlCisReq->cisOffsetMax[0]);


    if(!((pLlCisReq->phyM2S==PHY_PREFER_1M) || (pLlCisReq->phyM2S==PHY_PREFER_2M) || (pLlCisReq->phyM2S==PHY_PREFER_CODED))){
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, cis_req phy error 1", 0, 0);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if(!((pLlCisReq->phyS2M==PHY_PREFER_1M) || (pLlCisReq->phyS2M==PHY_PREFER_2M) || (pLlCisReq->phyS2M==PHY_PREFER_CODED))){
        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, cis_req phy error 2", 0, 0);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    //check if phyM2S is same as phyS2M


    if(pLlCisReq->nse == 0 || pLlCisReq->nse > min(31, LL_BN_IN_PER_CIS_NUM_MAX) || comm_phy == 0 || \
      (pLlCisReq->bnM2S > 0 && pLlCisReq->maxPduM2S > 251) || (pLlCisReq->bnM2S == 0 && pLlCisReq->maxPduM2S) || \
      (pLlCisReq->bnS2M > 0 && pLlCisReq->maxPduS2M > 251) || (pLlCisReq->bnS2M == 0 && pLlCisReq->maxPduS2M) || \
      (pLlCisReq->nse == 1 && subIntvl) || (pLlCisReq->nse > 1 && (subIntvl < 400 || subIntvl > pLlCisReq->isoIntvl*1250)) || \
      (!pLlCisReq->ftM2S || !pLlCisReq->ftS2M) || (isoIntvl < 5000 || isoIntvl > 4000000) || \
      (cisOffsetMin < 500) || (cisOffsetMin > cisOffsetMax))  //EBQ  connEventCnt is 0x0000
    {

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, cis_req err", 0, 0);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }



    return BLE_SUCCESS;
}


ble_sts_t   blt_ll_cis_slave_control_pdu_process(st_ll_conn_t* pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    int i;

    ll_cis_conn_t *pCisConn  = NULL;
    ll_cis_slv_t  *pCigSlave = NULL;
    st_lls_conn_t* pAclSlave = NULL;
    u8  cis_conn_idx = 0;

    u8 acl_slv_dix = pAclConn->acl_conIndex - LL_MAX_ACL_CEN_NUM;

    if(opcode == LL_CIS_REQ){  //only slave receive this opcode

        if(!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS)){

            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, feature iso dis", 0, 0);
            return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
        }

        if(bltCisMng.curNum_cisSlave >= bltCisMng.maxNum_cisSlave){ //no CIS available

            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, cis slave num exceed", 0, 0);
            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        }


        if(bltCisMng.cisFlow_pending){
            // Previous CIS flow on this ACL connection not finish
            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, pre flow ongoing", 0, 0);
            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
        }


        rf_packet_ll_cis_req_t* pLlCisReq = (rf_packet_ll_cis_req_t*)pLlCtrlPkt;

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] LL_CIS_REQ rx", &pLlCisReq->cigId, sizeof(rf_packet_ll_cis_req_t) - 3);

        if(blt_llCisReqParamsChk(pAclConn, pLlCisReq) != BLE_SUCCESS){ //CIS_Req parameters check

            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CISP][FLW] ERROR, cis_req param fail", 0, 0);
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

        //TODO: check NSE: attention:  Only consider NSE same for all CISes

        /*
         * When the slave's Link Layer receives the LL_CIS_REQ PDU, it shall either reject the proposed CIS immediately or notify the Host.
         */
        if(0){  //TODO: Slave Controller can not accept new CIS due to LL timing resource or any other problem
            return HCI_ERR_INVALID_HCI_CMD_PARAMS;
        }

#if (ULL_FOR_CIS_EN)
        bool ull_used = FALSE;
        u32 rptIntvl_us = pLlCisReq->isoIntvl*1250 / pLlCisReq->nse;
        u32 sdu_int_peer_us = MAKE_U24(pLlCisReq->sduIntvlM2S[2]&0xf, pLlCisReq->sduIntvlM2S[1],pLlCisReq->sduIntvlM2S[0]);
        u32 sdu_int_loca_us = MAKE_U24(pLlCisReq->sduIntvlS2M[2]&0xf, pLlCisReq->sduIntvlS2M[1],pLlCisReq->sduIntvlS2M[0]);
        u32 sub_intvl_us    = MAKE_U24(pLlCisReq->subIntvl[2], pLlCisReq->subIntvl[1], pLlCisReq->subIntvl[0]);
        if(pLlCisReq->nse == 1 && sub_intvl_us == 0){
            sub_intvl_us = rptIntvl_us;
        }
        //BN = NSE
        //SDU interval = report interval = sub interval
        if(((pLlCisReq->bnM2S && pLlCisReq->bnM2S == pLlCisReq->nse) || (pLlCisReq->bnS2M && pLlCisReq->bnS2M == pLlCisReq->nse))){
            u32 mod_1ms = sub_intvl_us % 1000; //1ms unit
            u32 num_1ms = sub_intvl_us / 1000;
            u32 mod_1p25ms = sub_intvl_us % 1250; //1.25ms unit
            u32 num_1p25ms = sub_intvl_us / 1250;
            if(pLlCisReq->bnM2S && sub_intvl_us == sdu_int_peer_us){
                if(mod_1ms == 0 && (num_1ms >= 1 && num_1ms <= 5)){ //[1ms ~ 5ms]
                    ull_used = TRUE;
                }
                else if(mod_1p25ms == 0 && (num_1p25ms >= 1 && num_1ms <= 4)){ //[1.25ms ~ 5ms]
                    ull_used = TRUE;
                }
            }
            else if(pLlCisReq->bnS2M && sub_intvl_us == sdu_int_loca_us){
                if(mod_1ms == 0 && (num_1ms >= 1 && num_1ms <= 5)){ //[1ms ~ 5ms]
                    ull_used = TRUE;
                }
                else if(mod_1p25ms == 0 && (num_1p25ms >= 1 && num_1ms <= 4)){ //[1.25ms ~ 5ms]
                    ull_used = TRUE;
                }
            }
        }

        if(ull_used){
            tlkapi_printf(CIS_DEBUG_EN, "[CISP][TIM] ULL used, sub_itvl_us:%d us ", sub_intvl_us);
        }
#endif

        //Controller can accept, ask if Host can accept
        if(hci_le_eventMask & HCI_LE_EVT_MASK_CIS_REQUESTED)
        {

            //find which CIS SRAM space can allocate to this new CIS command
            for(i = bltCisMng.maxNum_cisMaster; i< bltCisMng.maxNum_cisConn; i++){
                pCisConn = (ll_cis_conn_t *) (global_pCisConn + i);

                if(!pCisConn->cis_occupied){
                    pCisConn->cis_occupied = 1;
                    bltCisMng.curNum_cisSlave ++;

                    cis_conn_idx = i;
                    tlkapi_send_string_data(DEB_CIG_SLV_EN, "find CIS sram space", &i, 1);

                    break;
                }
            }

            if(i >= bltCisMng.maxNum_cisConn){
                //There's not enough CIS sram space to allocate for this new CIS command
                tlkapi_send_string_data(DEB_CIG_SLV_EN | CIS_FLOW_LOG_EN, "[CISP][FLW] There's not enough CIS sram space", &i, 1);
                return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
            }

            //Our current design is to run only one CIG_SLV on the same ACL,
            //and one CIG_SLV allows multiple CIS to be established.
            for(i=0; i < bltCisMng.maxNum_cig_slv; i++ ){
                pCigSlave = (ll_cis_slv_t  *)(global_pCisSlv + i);

                /*
                 * 1. If the CIG has been allocated and the CIG_ID is the same, continue to
                 *    establish CIS on this CIG (there are multiple CISs on one CIG);
                 */
                if(pCigSlave->cig_slv_occupied && pCigSlave->cig_ID_slv == pLlCisReq->cigId){
                    //According to BQB<<LL. TS>>: Only test one set of CIG on an ACL, we do not
                    //consider establishing multiple CIG groups on one ACL for the time being.
                    if(pCigSlave->link_acl_slave_handle == pAclConn->acl_conHandle){
                        //allow multiple CIS on one CIG SLV

                        if( pCigSlave->ciss_alloc_count == LL_CIS_IN_PER_CIG_SLV_NUM_MAX){
                            tlkapi_send_string_data(DEB_CIG_SLV_EN, "CIS slave in CIG exceed", &pCigSlave->ciss_alloc_count, 1);
                            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
                        }

                        tlkapi_send_string_data(DEB_CIG_SLV_EN, "find same CIG sram space", &i, 1);
                        //If cis_alloc_cut>1, it means that multiple CISs are established on the same CIG_SLV.
                        break;
                    }
                }
            }

            if(i == bltCisMng.maxNum_cig_slv){
                /*
                 * 2. Find the CIG_ID from the already allocated CIG and the requested CIG_ID
                 *    is different, then allocate a new CIG (there are multiple CIGs on the same ACL);
                 * 3. Or if no CIG is allocated, look for an available CIG.
                 */
                for(i=0; i < bltCisMng.maxNum_cig_slv; i++ ){
                    pCigSlave = (ll_cis_slv_t  *)(global_pCisSlv + i);

                    if(!pCigSlave->cig_slv_occupied){ //New alloc CIG
                        pCigSlave->cig_slv_occupied = 1;
                        pCigSlave->cig_ID_slv = pLlCisReq->cigId;
                        pCigSlave->link_acl_slave_handle = pAclConn->acl_conHandle;
                        pCigSlave->ciss_alloc_count = 0;
                        pCigSlave->ciss_alloc_msk = 0;

                        if(pCigSlave->ciss_task_cnt || pCigSlave->ciss_task_msk){
                            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991B0000 | pCigSlave->ciss_task_cnt | pCigSlave->ciss_task_msk);
                        }

                        tlkapi_send_string_data(DEB_CIG_SLV_EN, "find new CIG sram space", &i, 1);

                        break;
                    }
                }

                if(i == bltCisMng.maxNum_cig_slv){
                    //There's not enough CIG sram space to allocate for this new CIG command
                    tlkapi_send_string_data(CIS_FLOW_LOG_EN | DEB_CIG_SLV_EN, "[CIS][FLW] ERROR, no enough CIG slave", &i, 1);
                    return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
                }
            }


            /* check if ACL connection interval is multiple of ISO interval or  ISO interval is multiple of ACL connection interval */
            int mod;
            if(pAclConn->conn_intvl_n_1m25 >= pLlCisReq->isoIntvl){
                mod = pAclConn->conn_intvl_n_1m25 % pLlCisReq->isoIntvl;
            }
            else{
                mod = pLlCisReq->isoIntvl % pAclConn->conn_intvl_n_1m25;
            }

            if(mod){
                pCisConn->align_with_acl = 0;
            }
            else{
                pCisConn->align_with_acl = 1;
            }


            u32 r = irq_disable();
            pCigSlave->ciss_alloc_count ++;
            pCisConn->cis_tick = clock_time() + BIT(29);
            irq_restore(r);

            //CIS slave parameters process
            pCigSlave->link_acl_slave_idx = acl_slv_dix;
            pCigSlave->ciss_isoIntvl      = pLlCisReq->isoIntvl;  //record, for calculate slot_duration later
            pCigSlave->ciss_bSlotInterval = pLlCisReq->isoIntvl*2;   //1.25mS = 2*625uS
            pCigSlave->ciss_sSlotInterval = pLlCisReq->isoIntvl*64;  //1.25mS = 2*625uS = 2*32* (312.5 tick)


            bltCisMng.cisFlow_pending |= BIT(cis_conn_idx);
            bltCisMng.cisFlow_idx = cis_conn_idx;
            pCisConn->cisFlowFlg = CIS_FLOW_SLAVE_REQ_HOST;
            pAclConn->ll_rsp_timeout_tick = clock_time() | 1;   //TODO, someone add this, maybe CIS no need  SiHui
            pAclConn->alink_cig_idx = pCigSlave->cig_slave_index;


            pCisConn->link_acl_index = pAclConn->acl_conIndex;
            pCisConn->link_acl_handle = pAclConn->acl_conHandle;
            pCisConn->link_cigid = pLlCisReq->cigId;
            pCisConn->clink_cig_idx = pCigSlave->cig_slave_index;
            pCisConn->cis_ID = pLlCisReq->cisId;


            pCisConn->ack_CisReq = 0;



            //CIS connection parameters process
            pCisConn->nse = pLlCisReq->nse;
            pCisConn->cis_frame = pLlCisReq->framed;
            pCisConn->phy_ms = pLlCisReq->phyM2S;
            pCisConn->bn_peer = pLlCisReq->bnM2S;
            pCisConn->bn_loca = pLlCisReq->bnS2M;
            pCisConn->ft_peer = pLlCisReq->ftM2S;
            pCisConn->ft_loca = pLlCisReq->ftS2M;
            pCisConn->max_sdu_peer = pLlCisReq->maxSduM2S;
            pCisConn->max_sdu_loca = pLlCisReq->maxSduS2M;
            pCisConn->max_pdu_peer = pLlCisReq->maxPduM2S;
            pCisConn->max_pdu_loca = pLlCisReq->maxPduS2M;
            //timing relative


            pCisConn->iso_intvl_us   = pLlCisReq->isoIntvl * 1250;
            pCisConn->iso_intvl_tick = pLlCisReq->isoIntvl * SYSTEM_TIMER_TICK_1250US;

            pCisConn->peer_cisOffsetMin_us  = MAKE_U24(pLlCisReq->cisOffsetMin[2], pLlCisReq->cisOffsetMin[1], pLlCisReq->cisOffsetMin[0]);
            pCisConn->peer_cisOffsetMax_us  = MAKE_U24(pLlCisReq->cisOffsetMax[2], pLlCisReq->cisOffsetMax[1], pLlCisReq->cisOffsetMax[0]);
            pCisConn->sdu_int_peer_us =  MAKE_U24(pLlCisReq->sduIntvlM2S[2]&0xf, pLlCisReq->sduIntvlM2S[1],pLlCisReq->sduIntvlM2S[0]);
            pCisConn->sdu_int_loca_us =  MAKE_U24(pLlCisReq->sduIntvlS2M[2]&0xf, pLlCisReq->sduIntvlS2M[1],pLlCisReq->sduIntvlS2M[0]);

            cisSlv_param.cisReq_aclEvtCnt = pLlCisReq->connEventCnt;



            if(pCisConn->phy_ms == PHY_PREFER_1M){
                pCisConn->curCisPhy = BLE_PHY_1M; //The Controller only supports asymmetric PHYs.

            }
            else if(pCisConn->phy_ms == PHY_PREFER_2M){
                pCisConn->curCisPhy = BLE_PHY_2M;
            }
            else{
                //take it as S8
                pCisConn->curCisPhy = BLE_PHY_CODED;
            }

            u8 mic_len = pAclConn->crypt.enable ? 4 : 0;
            //TX & RX max PDU translate time, uS
            u8 mic_len_peer = pCisConn->max_pdu_peer ? mic_len : 0;
            u8 mic_len_loca = pCisConn->max_pdu_loca ? mic_len : 0;
            int tx_rx_max_us = blt_phy_getRfPacketTime_us(pCisConn->max_pdu_peer + mic_len_peer, pCisConn->curCisPhy, LE_CODED_S8) + \
                               blt_phy_getRfPacketTime_us(pCisConn->max_pdu_loca + mic_len_loca, pCisConn->curCisPhy, LE_CODED_S8);

            pCisConn->MPTM_TIFS_MPTS = tx_rx_max_us + BLE_T_IFS;

            pCisConn->sub_intvl_us   = MAKE_U24(pLlCisReq->subIntvl[2], pLlCisReq->subIntvl[1], pLlCisReq->subIntvl[0]);
            if(pCisConn->nse == 1 && pCisConn->sub_intvl_us == 0){
                #if (ULL_FOR_CIS_EN)
                    if(ull_used){
                        pCisConn->sub_intvl_us = sub_intvl_us;
                    }
                    else
                #endif
                    {
                        pCisConn->sub_intvl_us = pCisConn->MPTM_TIFS_MPTS + 150;
                    }
            }
            pCisConn->sub_intvl_tick = pCisConn->sub_intvl_us * SYSTEM_TIMER_TICK_1US;

            pCisConn->subInter_idle_us = pCisConn->sub_intvl_us - pCisConn->MPTM_TIFS_MPTS;
            pCisConn->cis_maxPossible_us = pCisConn->sub_intvl_us * (pCisConn->nse - 1) + pCisConn->MPTM_TIFS_MPTS;
            pCisConn->cis_idle_us = pCisConn->iso_intvl_us - pCisConn->cis_maxPossible_us;

            #if (DBG_CIS_1ST_AP_TIMING_EN)
                u32 oct_own_min_us = blt_debug_hex_2_dec_display(pCisConn->peer_cisOffsetMin_us);
                u32 oct_own_max_us = blt_debug_hex_2_dec_display(pCisConn->peer_cisOffsetMax_us);
                u32 oct_isoINT_us = blt_debug_hex_2_dec_display(pCisConn->iso_intvl_us);
                u32 oct_subInt_us = blt_debug_hex_2_dec_display(pCisConn->sub_intvl_us);

                u32 oct_mptm_tifs_mptm = blt_debug_hex_2_dec_display(pCisConn->MPTM_TIFS_MPTS);
                u32 oct_subInt_idle_us = blt_debug_hex_2_dec_display(pCisConn->subInter_idle_us);
                u32 oct_maxPoss_time   = blt_debug_hex_2_dec_display(pCisConn->cis_maxPossible_us);
                u32 oct_isoInt_idle_us = blt_debug_hex_2_dec_display(pCisConn->cis_idle_us);

                tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] nse", &pCisConn->nse, 1);
                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] cis req param1", oct_own_min_us, oct_own_max_us, oct_isoINT_us, oct_subInt_us);
                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] cis req param2", oct_mptm_tifs_mptm, oct_subInt_idle_us, oct_maxPoss_time, oct_isoInt_idle_us);
            #endif

            if(pCisConn->cis_idle_us < 0){
                tlkapi_send_string_u32s(CIS_DEBUG_EN, "[CISP][TIM] ERROR, task width", pCisConn->MPTM_TIFS_MPTS, pCisConn->sub_intvl_us, pCisConn->cis_maxPossible_us, pCisConn->cis_idle_us);
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x990B0000);
            }




            if(pCisConn->subInter_idle_us < 150){ //pCisConn->subInter_idle_us > 0x8000
                write_sram32(0x0018, pCisConn->subInter_idle_us);
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x990A0000);
            }

            //150(T_Mss)  + 80(empty_pkt)+ 150(T_IFS) + 80(empty_pkt) + 150(T_MSS) = 610
            else if(pCisConn->subInter_idle_us < 610){
                pCisConn->TMSS = pCisConn->subInter_idle_us;
                pCigSlave->packing_style = PACK_SEQUENTIAL;
                tlkapi_send_string_data(CIS_DEBUG_EN, "[CISP][TIM] Sequential, TMSS", &pCisConn->TMSS, 2);
            }
            else{
                pCigSlave->packing_style = PACK_UNKNOWN;
            }



            hci_le_cisReq_evt(pCisConn->link_acl_handle, pCisConn->cis_connHandle, pCisConn->link_cigid, pCisConn->cis_ID);
            cisSlv_param.connAcpt_begin_tick = clock_time() | 1;
        }
        else
        {
            //HCI_LE_CIS_Request event is masked away
            return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;  //TODO: reason
        }
    }
    else if(opcode == LL_CIS_IND){

        if(bltCisMng.cisFlow_idx == INVALID_CIS_IDX){
            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99080000);
        }

        cis_conn_idx = bltCisMng.cisFlow_idx;
        pCisConn  = (ll_cis_conn_t *)(global_pCisConn + cis_conn_idx);
        pAclSlave = (st_lls_conn_t *)&blmsSlave[acl_slv_dix];
        pCigSlave = (ll_cis_slv_t *)(global_pCisSlv + pCisConn->clink_cig_idx);

        if(pCisConn->cisFlowFlg != CIS_FLOW_SLAVE_WAIT_CIS_IND){
            tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CIS][FLW] ERROR, CIS flow err", 0, 0);
            return HCI_ERR_CONN_REJ_LIMITED_RESOURCES; //
        }

        pAclConn->ll_rsp_timeout_tick = 0;


        rf_packet_ll_cis_ind_t* pLlCisInd = (rf_packet_ll_cis_ind_t*)pLlCtrlPkt;

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CIS][FLW] LL_CIS_IND rx", pLlCisInd->cisOffset, sizeof(rf_packet_ll_cis_ind_t) - OFFSETOF(rf_packet_ll_cis_ind_t, cisOffset) );



        /* when code runs here, a CIS slave timing is about to build */
        pCigSlave->cisOffset_us   = MAKE_U24(pLlCisInd->cisOffset[2], pLlCisInd->cisOffset[1], pLlCisInd->cisOffset[0]);
        pCigSlave->cisOffset_tick = pCigSlave->cisOffset_us * SYSTEM_TIMER_TICK_1US;

        pCisConn->cisAccessAddr   = pLlCisInd->cisAccessAddr;
        pCisConn->cig_sync_delay  = MAKE_U24(pLlCisInd->cigSyncDly[2], pLlCisInd->cigSyncDly[1], pLlCisInd->cigSyncDly[0]);
        pCisConn->cis_sync_delay  = MAKE_U24(pLlCisInd->cisSyncDly[2], pLlCisInd->cisSyncDly[1], pLlCisInd->cisSyncDly[0]);
        cisConn_param.cis_1st_anchor_evtCnt = pLlCisInd->connEventCnt; //CIS 1st anchor point -> acl connEventCnt + cisOffset


        #if (DBG_CIS_1ST_AP_TIMING_EN)
            u32 oct_cis_offset = blt_debug_hex_2_dec_display(pCigSlave->cisOffset_us);
            u32 oct_cig_sync_dly = blt_debug_hex_2_dec_display(pCisConn->cig_sync_delay);
            u32 oct_cis_sync_dly = blt_debug_hex_2_dec_display(pCisConn->cis_sync_delay);
            u32 oct_acl_evt_cnt = blt_debug_hex_2_dec_display(pLlCisInd->connEventCnt);

            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] 1ST AP, cis ind param", oct_cis_offset, oct_cig_sync_dly, oct_cis_sync_dly, oct_acl_evt_cnt);
        #endif


#if 0
        if(pCigSlave->cisOffset_us < pCisConn->own_cisOffsetMin_us || pCigSlave->cisOffset_us > pCisConn->own_cisOffsetMax_us){
            tlkapi_send_string_u32s(CIS_DEBUG_EN, "ERROR, offset", pCigSlave->cisOffset_us, pCisConn->own_cisOffsetMin_us, pCisConn->own_cisOffsetMax_us, 0);
            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x990D0000);
        }
#endif


        u32 transLaty_temp_m2s = pCisConn->cig_sync_delay + pCisConn->ft_peer * pCisConn->iso_intvl_us;
        u32 transLaty_temp_s2m = pCisConn->cig_sync_delay + pCisConn->ft_loca * pCisConn->iso_intvl_us;
        if(pCisConn->cis_frame == CIS_UNFRAMED){
            pCisConn->transLaty_m2s = transLaty_temp_m2s - pCisConn->sdu_int_peer_us;
            pCisConn->transLaty_s2m = transLaty_temp_s2m - pCisConn->sdu_int_loca_us;
        }
        else{
            pCisConn->transLaty_m2s = transLaty_temp_m2s + pCisConn->sdu_int_peer_us;
            pCisConn->transLaty_s2m = transLaty_temp_s2m + pCisConn->sdu_int_loca_us;
        }

            int preCis_order_idx = 0;
            if(pCigSlave->ciss_alloc_count == 2) //multiple CIS on an ACL
            {
                // determine the CIS arrangement: Interleaved OR Sequential
                if(!IS_POWER_OF_2(pCigSlave->ciss_alloc_msk)){
                    BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x990F0000 | pCigSlave->ciss_alloc_msk);
                }

                ll_cis_conn_t *pExistCisCon = NULL;
                for(i = bltCisMng.maxNum_cisMaster; i < bltCisMng.maxNum_cisConn; i++ ){
                    if(pCigSlave->ciss_alloc_msk & BIT(i)){
                        pExistCisCon = (ll_cis_conn_t *) (global_pCisConn + i);
                        break;
                    }
                }

                if(pExistCisCon == NULL){
                    BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99110000);
                    return HCI_ERR_CONN_REJ_LIMITED_RESOURCES; //
                }

                u32 cis1stSubItvlUs;
                int cisSyncDly_diff = pExistCisCon->cis_sync_delay - pCisConn->cis_sync_delay;
                if(cisSyncDly_diff > 0){
                    pCigSlave->pCis1st = pExistCisCon;
                    pCigSlave->pCis2nd = pCisConn;

                    pCigSlave->ciss_order_tbl[0] = pExistCisCon->cis_index;
                    pCigSlave->ciss_order_tbl[1] = cis_conn_idx;  //or pCisConn->cis_index
                    pExistCisCon->cig_ap_distan_us = 0;
                    pCisConn->cig_ap_distan_us = cisSyncDly_diff;
                    cis1stSubItvlUs = pExistCisCon->sub_intvl_us;
                    //preCis_order_idx = 0; //can save, cause initial value is 0
                }
                else{
                    cisSyncDly_diff = 0 - cisSyncDly_diff;
                    pCigSlave->pCis1st = pCisConn;
                    pCigSlave->pCis2nd = pExistCisCon;

                    pCigSlave->ciss_order_tbl[0] = cis_conn_idx;  //or pCisConn->cis_index
                    pCigSlave->ciss_order_tbl[1] = pExistCisCon->cis_index;
                    pCisConn->cig_ap_distan_us = 0;
                    pExistCisCon->cig_ap_distan_us = cisSyncDly_diff;
                    cis1stSubItvlUs = pCisConn->sub_intvl_us;
                    preCis_order_idx = 1;
                }


                if(cisSyncDly_diff < (int)cis1stSubItvlUs){
                    pCigSlave->cigSlvPacking = PACK_INTERLEAVED;
                }
                else{
                    pCigSlave->cigSlvPacking = PACK_SEQUENTIAL;
                }

                pCigSlave->offset_diff_us = cisSyncDly_diff;


                //debug
                if(pCigSlave->packing_style == PACK_SEQUENTIAL && pCigSlave->cigSlvPacking == PACK_INTERLEAVED){
                    BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99100000);
                }


                /* if previous CIS task on this CIG Slave is working, must wait until it end */
                #if (CIS_DEBUG_EN)
                    u32 t0 = clock_time();
                    while(pCigSlave->cigs_working){
                        if(clock_time_exceed(t0, 1000000)){ //1 S
                            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x990E0000);
                        }
                    }
                #else
                    while(pCigSlave->cigs_working);
                #endif


                #if (DBG_CIS_1ST_AP_TIMING_EN)
                    u32 oct_1stcis_ap = blt_debug_hex_2_dec_display(pCigSlave->pCis1st->cig_ap_distan_us);
                    u32 oct_2ndcis_ap = blt_debug_hex_2_dec_display(pCigSlave->pCis2nd->cig_ap_distan_us);

                    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] 2 CIS, distance", oct_1stcis_ap, oct_2ndcis_ap, 0, 0);
                #endif
            }
            else{  //one CIS on CIG slave
                pCigSlave->cigSlvPacking = PACK_SEQUENTIAL;
                pCigSlave->pCis1st = pCigSlave->pCis2nd = pCisConn;
                pCigSlave->ciss_order_tbl[0] = cis_conn_idx;
                pCigSlave->offset_diff_us = 0;
                pCisConn->cig_ap_distan_us = 0;
//              preCis_order_idx = 0; //can save, cause initial value is 0
            }


            ////////////////////  CIS arrangement map ////////////////////////
            //////// IRQ protect ///////
            u32 r = irq_disable();  //important, different time line from IRQ, but process some shared data

            pCigSlave->ciss_total_se_num = 0;
            pCigSlave->ciss_arrgmtMap_en_msk = 0;
            ll_cis_conn_t *pCurCisCon;
            u8 cur_cisIndex, task_idx;
            for(i = 0; i < pCigSlave->ciss_alloc_count; i++){
                cur_cisIndex = pCigSlave->ciss_order_tbl[i];
                pCurCisCon = (ll_cis_conn_t *) (global_pCisConn + cur_cisIndex);

                if(pCigSlave->cigSlvPacking == PACK_SEQUENTIAL){ //Sequential: e.g.: 112233
                    for(int j=0; j< pCurCisCon->nse; j++){
                        task_idx = pCigSlave->ciss_total_se_num + j;
                        pCigSlave->ciss_arrgmtMap_cisIdx[task_idx] = cur_cisIndex;
                        pCigSlave->ciss_arrgmtMap_seIdx[task_idx] = j + 1;
                        if(pCigSlave->ciss_alloc_count == 2 && i == preCis_order_idx){
                            pCigSlave->ciss_arrgmtMap_en_msk |= BIT(task_idx);
                        }
                    }
                }
                else{ //== PACK_INTERLEAVED  //Interleaved: e.g.: 123123
                    for(int j=0; j< pCurCisCon->nse; j++){ //attention:  Only consider NSE same for all CISes
                        task_idx = i + j * pCigSlave->ciss_alloc_count;
                        pCigSlave->ciss_arrgmtMap_cisIdx[task_idx] = cur_cisIndex;
                        pCigSlave->ciss_arrgmtMap_seIdx[task_idx] = j + 1;
                        if(pCigSlave->ciss_alloc_count == 2 && i == preCis_order_idx){
                            pCigSlave->ciss_arrgmtMap_en_msk |= BIT(task_idx);
                        }

                        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] add map", i + j * pCigSlave->ciss_alloc_count, pCigSlave->ciss_order_tbl[i], 0, 0);
                    }
                }

                pCigSlave->ciss_total_se_num += pCurCisCon->nse;
            }

            irq_restore(r);
            //////// IRQ restore ///////


            pCigSlave->ciss_widen_us = 0;

            pCigSlave->ciss_alloc_msk |= BIT(cis_conn_idx);

            blt_cis_connect_common(pAclConn, pCisConn);  //attention: consider more if this function should use IRQ protect.

            pCisConn->dpId = Data_Path_Disable;  //TODO: here may need consider more detailed
            pCisConn->cis_dapth_setup = 0;

            int bslot_offset = (pCigSlave->cisOffset_us + pAclSlave->offsetUs_mark1stRx)/625;


            //////// IRQ protect ///////
            r = irq_disable();  //important, different time line from IRQ, but process some shared data

            s32 acl_event_diff = (s32)(cisConn_param.cis_1st_anchor_evtCnt - pAclSlave->evtCnt_mark_1strx);
            blmsParam.cis_1st_anchor_bSlot = pAclSlave->bSlot_mark_1strx + acl_event_diff*pAclConn->bSlot_interval + bslot_offset;
            cisConn_param.cis_1st_anchor_tick = pAclSlave->tick_mark_1strx + acl_event_diff*pAclConn->conn_intvl_tick + pCigSlave->cisOffset_tick;



            u32 tick_now = clock_time();
            u32 bSlot_cur = GET_BSLOT_IDX(tick_now) + 5;   //margin: 625uS*5=3.125mS


            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] time1", pAclSlave->evtCnt_mark_1strx, pAclSlave->bSlot_mark_1strx, pAclSlave->tick_mark_1strx, cisConn_param.cis_1st_anchor_evtCnt);
            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] time2", acl_event_diff, blmsParam.cis_1st_anchor_bSlot, cisConn_param.cis_1st_anchor_tick, bslot_offset);
            tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] time3", tick_now, bSlot_cur, pCigSlave->cisOffset_us, 0);


            if(blmsParam.cis_1st_anchor_bSlot > 0xFFFF0000){
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99000000);
            }

            if(blmsParam.cis_1st_anchor_bSlot < bSlot_cur){
                int jump_num = (bSlot_cur + pCigSlave->ciss_bSlotInterval - 1 - blmsParam.cis_1st_anchor_bSlot)/pCigSlave->ciss_bSlotInterval;
                blmsParam.cis_1st_anchor_bSlot += jump_num*pCigSlave->ciss_bSlotInterval;
                cisConn_param.cis_1st_anchor_tick += jump_num*pCigSlave->ciss_isoIntvl * SYSTEM_TIMER_TICK_1250US;
                blt_ll_cis_start_jump(pCisConn, jump_num);

                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] CIS 1st point passed", jump_num, pCisConn->cisSendPldNum, pCisConn->cisRcvdPldNum, 0);


                #if (DBG_CIS_1ST_AP_TIMING_EN)
                    u32 tick_bSlot = bltSche.bSlot_tick_irq_real + (blmsParam.cis_1st_anchor_bSlot - bltSche.bSlot_idx_irq_real)*SYSTEM_TIMER_TICK_625US;

                    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] time4", tick_bSlot, blmsParam.cis_1st_anchor_bSlot, cisConn_param.cis_1st_anchor_tick, 0);
                    if(tick1_out_range_of_tick2(tick_bSlot, cisConn_param.cis_1st_anchor_tick, 1000*SYSTEM_TIMER_TICK_1US)){
                        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99300000);
                    }
                #endif

            }

            /* first anchor point is in current 80mS task list, may consider insert task quickly */
            if(blmsParam.cis_1st_anchor_bSlot <= bltSche.lastTsk_endBslot){
                //DBG_C HN14_TOGGLE;
                if(blms_state == BLMS_STATE_NONE || blms_state == BLMS_STATE_PRICHN_SCAN_S){
                    //DBG_C HN15_TOGGLE;
                    u32 cis_1st_task_tick = cisConn_param.cis_1st_anchor_tick - 1000*SYSTEM_TIMER_TICK_1US;
                    if(tick1_exceed_tick2(systimer_get_irq_capture() - 2000*SYSTEM_TIMER_TICK_1US, cis_1st_task_tick )){
                        systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
                        systimer_set_irq_capture(cis_1st_task_tick);
                        tlkapi_send_string_data(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] ciss 1st ap create, insert 1", 0, 0);
                        //DBG_C HN10_TOGGLE;
                    }
                }
            }

            u32 task_us = pCigSlave->offset_diff_us + pCigSlave->pCis2nd->cis_maxPossible_us;

            #if (DBG_CIS_1ST_AP_TIMING_EN)
                u32 oct_task_us = blt_debug_hex_2_dec_display(task_us);
                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] cis ind debug", oct_task_us, 0, 0, 0);
            #endif


            pCigSlave->ciss_sSlotAllocNum = task_us*SSLOT_US_REVERSE + 1;

            blmsParam.cig_slv_1st_sche_build_pending = CIS_SLOT_BUILD_MSK | cis_conn_idx; //Marked value: cis_conn_idx
            //tlkapi_send_string_data(0, "cig_slv_1st_sche_build_pending", &cis_conn_idx, 1);


            irq_restore(r);  //important
            //////// IRQ restore ///////
    }
    else if(opcode == LL_REJECT_IND_EXT){

        rf_packet_ll_reject_ext_ind_t* pRejectExtInd = (rf_packet_ll_reject_ext_ind_t*)pLlCtrlPkt;

        if(pRejectExtInd->rejectOpcode == LL_CIS_RSP){
            if(bltCisMng.cisFlow_idx == INVALID_CIS_IDX){
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99090000);
            }
            pCisConn = (ll_cis_conn_t *) (global_pCisConn + bltCisMng.cisFlow_idx);
            if(pCisConn->cisFlowFlg == CIS_FLOW_SLAVE_WAIT_CIS_IND){
                pAclConn->ll_rsp_timeout_tick = 0;
                pCigSlave = (ll_cis_slv_t *) (global_pCisSlv + pCisConn->clink_cig_idx);
                blt_ll_cis_slave_cis_establish(pCisConn, pCigSlave, pRejectExtInd->rejectOpcode);
            }
        }
    }
    else if(opcode == LL_CIS_TERMINATE_IND){
        rf_packet_ll_cis_terminate_t* pTermInd = (rf_packet_ll_cis_terminate_t*)pLlCtrlPkt;

        tlkapi_send_string_data(CIS_FLOW_LOG_EN, "[CIS][FLW] LL_CIS_TERMINATE_IND rx", &pTermInd->errorCode, 1);

        for(i = bltCisMng.maxNum_cisMaster; i < bltCisMng.maxNum_cisConn; i++ ){
            if(pAclConn->cisEstablish_msk & BIT(i)){
                pCisConn = (ll_cis_conn_t *)(global_pCisConn + i);
                if( pCisConn->cis_ID == pTermInd->cis_id && pCisConn->link_cigid == pTermInd->cig_id){
                    pCisConn->cis_termin_union.peer_terminate = pTermInd->errorCode;
                }
            }
        }
    }




    return BLE_SUCCESS;

}







_attribute_ram_code_
int         blt_ll_buildCisSlaveSchedulerLinklist(void)
{
    int i = 0,j =0;
    int new_task_cnt = 0;
    ll_cis_slv_t *pCigSlave = NULL;
    s32 sSlot_start_cig;

    int int_jump_ciss;


    //int AA_cnt = 0;
    //int AA_idx[4];

    for(i=0; i<bltCisMng.maxNum_cig_slv; i++)
    {
        #if 0 //debug
            AA_idx[AA_cnt] = i;
            AA_cnt ++;
            if(AA_cnt > 3){
                tlkapi_send_string_u32s(0, "DBG1", AA_idx[0], AA_idx[1],AA_idx[2],AA_idx[3]);
                tlkapi_send_string_u32s(0, "DBG2", AA_cnt, bltCisMng.maxNum_cig_slv, bltSche.task_mask, TSKMSK_CIG_SLAVE_0);
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991C0000);
            }
        #endif

        if( bltSche.task_mask & (TSKMSK_CIG_SLAVE_0<<i) )
        {

            int cigSlv_offset = TSKOFT_CIG_SLV + i;
            blt_ll_setSchedulerTaskPriority(cigSlv_offset, TASK_PRIORITY_HIGH_THRES);

            pCigSlave = (ll_cis_slv_t *) (global_pCisSlv + i);

            pCigSlave->schTsk_wptr = pCigSlave->schTsk_rptr = 0;

            if(bltSche.build_index == 0){
                if(bltSche.sSlot_idx_reset == 1){
                    pCigSlave->sSlot_mark_cigs -= bltSche.sSlot_idx_past;
                    //tlkapi_send_string_u32s(0 || DBG_CIS_SLAVE_TIMING, "sSlot_idx_rest",i,pCigSlave->sSlot_mark_cigs, 0, 0);
                }

                if(pCigSlave->cigs_sSlotOffset){
                    //The local sSlot_mark_big anchor point should be calibrated according to the RX receiving point
                    blt_pCisSlv->sSlot_mark_cigs += pCigSlave->cigs_sSlotOffset; //Calibrate the anchor point of CIG_SLV scheduling sSlot task
                    pCigSlave->cigs_sSlotOffset = 0;
                }
            }


            if( pCigSlave->sSlot_mark_cigs >= bltSche.sSlot_idx_next){
                sSlot_start_cig = pCigSlave->sSlot_mark_cigs + pCigSlave->ciss_sSlotInterval;
                int_jump_ciss = 0;

                //tlkapi_send_string_u32s(0 || DBG_CIS_SLAVE_TIMING, "sSlot mark cig0 ",  pCigSlave->sSlot_mark_cigs, sSlot_start_cig, int_jump_ciss, bltSche.sSlot_idx_irq_real);
            }
            else{
                int_jump_ciss = (bltSche.sSlot_idx_next - 1 - pCigSlave->sSlot_mark_cigs)/pCigSlave->ciss_sSlotInterval; //fanqh todo just debug
                sSlot_start_cig = pCigSlave->sSlot_mark_cigs + (int_jump_ciss + 1)*pCigSlave->ciss_sSlotInterval;

                //tlkapi_send_string_u32s(0 || DBG_CIS_SLAVE_TIMING, "sSlot mark cig1 ",  pCigSlave->sSlot_mark_cigs, sSlot_start_cig, int_jump_ciss, bltSche.sSlot_idx_irq_real);

                //blt_ll_incSchedulerTaskCalPriority( TSKOFT_CIG_SLV + i, bltPri.step_final[TSKOFT_CIG_SLV + i]*4*int_jump_ciss[i] );
            }



            /* SiHui: consider update a new task add, so add some more time. here update may represent a task remove, neglect this
             * give another margin here */
            int sSlot_sche_use = (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US)*SSLOT_US_REVERSE;
            pCigSlave->ciss_sSlotDuration = pCigSlave->ciss_sSlotAllocNum + pCigSlave->cigs_sSlotWiden + sSlot_sche_use;


            for(j=0; j<CIG_SLV_FIFONUM; j++){

                sch_task_t  *pCur_schTask = (sch_task_t *)&pCigSlave->cigs_schTsk_fifo[j];

                pCur_schTask->begin = sSlot_start_cig + j*pCigSlave->ciss_sSlotInterval; ///ISO_interval
                pCur_schTask->end = pCur_schTask->begin + pCigSlave->ciss_sSlotDuration - 1;


                if( pCur_schTask->begin >=  bltSche.sSlot_endIdx_dft){  //new task beyond correct range, finish
                    tlkapi_send_string_u32s(0, "begin >= endidx", pCur_schTask->begin, bltSche.sSlot_endIdx_dft,0,0);
                    break;
                }
                else if(pCur_schTask->end < bltSche.sSlot_endIdx_dft){ //new task in correct range
                    pCigSlave->schTsk_wptr = j;
                    new_task_cnt ++;
                }
                else{ //new task across "sSlot_endIdx_dft"

                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if(bltPri.pri_cal[cigSlv_offset] > bltPri.priMax_value){
                        bltPri.priMax_value = bltPri.pri_cal[cigSlv_offset];
                        bltPri.priMax_index = cigSlv_offset;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                        tlkapi_send_string_u32s(SCHE_TIMING_IMPROVE_DBG_EN,"across IDX cis SLV", i, bltSche.sSlot_endIdx_dft, bltSche.sSlot_endIdx_maxPri, bltPri.priMax_value);
                    }

                    break;
                }
            }

            if(new_task_cnt){
                blt_ll_addTask2ExistLinklist( &pCigSlave->cigs_schTsk_fifo[0],pCigSlave->schTsk_wptr + 1);
            }
        }
    }

    return 0;
}



#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int         blt_ll_cigSlvMainloop(void)
{
    ll_cis_conn_t *pCisConn;
    for(int cis_conn_idx = bltCisMng.maxNum_cisMaster; cis_conn_idx < bltCisMng.maxNum_cisConn; cis_conn_idx++)
    {
        pCisConn = (ll_cis_conn_t*)( global_pCisConn + cis_conn_idx);

        if(pCisConn->cisFlowFlg){
            //if(!pConn->ll_enc_busy){  //for any data sending request flow, if enc_busy, do not process

            ll_cis_slv_t *pCigSlave = (ll_cis_slv_t *) (global_pCisSlv + pCisConn->clink_cig_idx);

            if(pCisConn->cisFlowFlg == CIS_FLOW_SLAVE_SEND_CIS_RSP){
                blt_ll_sendCisRsp(pCisConn);
            }
            else if(pCisConn->cisFlowFlg == CIS_FLOW_SLAVE_REJECT_CIS_REQ){
                blt_ll_rejectCisReq(pCisConn, pCigSlave);
            }
            else if(pCisConn->cisFlowFlg == CIS_FLOW_CIS_SYNC_SUCCESS){
                blt_ll_cis_slave_cis_establish(pCisConn, pCigSlave, BLE_SUCCESS);
            }
            else if(pCisConn->cisFlowFlg == CIS_FLOW_CIS_SYNC_FAIL){
                blt_ll_cis_slave_cis_establish(pCisConn, pCigSlave, HCI_ERR_CONN_FAILED_TO_ESTABLISH);
            }
            else if(pCisConn->cisFlowFlg == CIS_FLOW_SLAVE_REQ_HOST){ //HCI/CIS/BI-07-C
                if(clock_time_exceed(cisSlv_param.connAcpt_begin_tick, cisSlv_param.connAcpt_to_us)){
                    blt_ll_cis_slave_cis_establish(pCisConn, pCigSlave, HCI_ERR_CONN_ACCEPT_TIMEOUT_EXCEEDED);
                }
            }
            else if(pCisConn->cisFlowFlg == CIS_FLOW_CLEAR_ESTABLISH_STATUS){
                blt_cis_clearCisSlaveMainloopStatus(pCisConn, pCigSlave);
            }
        }
    }


    return 1;
}








static inline void blt_ll_removeCisSlave(u8 cis_status)
{
    ///////////////// remove CIS Slave ////////////////
    blt_pCisConn->cisSchedFlg = 0;

    u8 cis_sel = blt_pCisConn->cis_index;

    blt_pCisSlv->ciss_task_cnt --;
    blt_pCisSlv->ciss_task_msk &= ~BIT(cis_sel);


    if(cis_status != HCI_ERR_CONN_FAILED_TO_ESTABLISH){
        blt_pAclConn->limit_txrx_sche_us = 0; //TODO: consider multiple CIS

        blt_pCisSlv->pcisDisconn = blt_pCisConn;
        blt_pCisConn->cisFlowFlg = CIS_FLOW_CLEAR_ESTABLISH_STATUS;
    }


    //tlkapi_send_string_u32s(0, "remove1", blt_pCisSlv->ciss_total_se_num, blt_pCisSlv->ciss_se_en_num, blt_pCisSlv->ciss_arrgmtMap_msk, cis_sel);
    //tlkapi_send_string_data(0, "remove2", &blt_pCisSlv->ciss_arrgmt_map, 8);

    //update ciss_arrgmtMap_msk and ciss_se_en_num
    int cis_match = 0;
    for(int i=0; i<blt_pCisSlv->ciss_total_se_num; i++){
        if((blt_pCisSlv->ciss_arrgmtMap_en_msk & BIT(i)) && (blt_pCisSlv->ciss_arrgmtMap_cisIdx[i] == cis_sel)){
            blt_pCisSlv->ciss_arrgmtMap_en_msk &= ~BIT(i);
            cis_match ++;
        }
    }



    if(cis_match != blt_pCisConn->nse){
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99030000 | cis_match<<8 | blt_pCisConn->nse);
    }
    blt_pCisSlv->ciss_se_en_num -= blt_pCisConn->nse;

    tlkapi_send_string_u32s(DEB_CIG_SLV_EN, "remove cis slave", blt_pCisSlv->ciss_total_se_num, blt_pCisSlv->ciss_arrgmtMap_en_msk, 0,0);

}

_attribute_ram_code_
int         blt_ll_calcCigSlv1stAndCis1stAnchorPoint(void)
{
    if(!blmsParam.cis_1st_anchor_bSlot || !blmsParam.cig_slv_1st_sche_build_pending){
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99190000 | blmsParam.cig_slv_1st_sche_build_pending);
    }

    u8 cis_conn_idx = blmsParam.cig_slv_1st_sche_build_pending & SLOT_BUILD_IDX_MSK; //Marked value: cis_conn_idx
    blmsParam.cig_slv_1st_sche_build_pending = 0;  //clear, must after getting "cis_conn_idx"
    blmsParam.cis_create_pending |= BIT(cis_conn_idx);

    ll_cis_conn_t *pCisConn = (ll_cis_conn_t *) (global_pCisConn + cis_conn_idx);
    u8 cigSlv_index = pCisConn->clink_cig_idx;
    ll_cis_slv_t *pCigSlave = (ll_cis_slv_t *) (global_pCisSlv + cigSlv_index);




    //Enable CIS SE task scheduling mapping table
    int cis_match = 0; //for debug
    for(int i= 0; i < pCigSlave->ciss_total_se_num; i++){
        if(pCigSlave->ciss_arrgmtMap_cisIdx[i] == cis_conn_idx){
            pCigSlave->ciss_arrgmtMap_en_msk |= BIT(i);
            cis_match ++;
        }
    }
    if(cis_match != pCisConn->nse){
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99120000 | cis_match<<8 | pCisConn->nse);
    }
    pCigSlave->ciss_se_en_num += pCisConn->nse;




    pCigSlave->ciss_task_cnt ++;
    pCigSlave->ciss_task_msk |= BIT(cis_conn_idx);
    if(pCigSlave->ciss_task_cnt > 2){
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991A0000 | pCigSlave->ciss_task_cnt);
    }


    pCisConn->cisSchedFlg = 0;
    pCisConn->cis1stSchedAPbSlot = blmsParam.cis_1st_anchor_bSlot;
    blmsParam.cis_1st_anchor_bSlot = 0;
    pCisConn->cis_expect_tick = cisConn_param.cis_1st_anchor_tick;
    pCisConn->cis_tick = pCisConn->cis_expect_tick - 100*SYSTEM_TIMER_TICK_1MS;


    if(pCigSlave->ciss_alloc_count == 1){

        pCigSlave->cigs_expect_tick = cisConn_param.cis_1st_anchor_tick;
        pCigSlave->cigs_expectTick_2 = cisConn_param.cis_1st_anchor_tick;
        pCigSlave->cigs_start_tick = pCigSlave->cigs_expect_tick - CIGSLV_EARLY_SET_TICK;
        pCigSlave->cigs_mark_rx_tick = pCigSlave->cigs_start_tick; //give a initial value, avoid error calculate when first CIS run
        pCigSlave->cigs_sSlotWiden = 0;

        if(tick1_exceed_tick2(clock_time(), pCigSlave->cigs_start_tick)){
            write_dbg32(0x0018, clock_time());
            write_dbg32(0x001C, cisConn_param.cis_1st_anchor_tick);
            write_dbg32(0x0020, pCigSlave->cigs_start_tick);
            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991D0000);
        }


        int n_sSlot = (pCigSlave->cigs_start_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
        pCigSlave->sSlot_mark_cigs = bltSche.sSlot_idx_irq_real + n_sSlot - pCigSlave->ciss_sSlotInterval;

        /* no need bSlot mark cigs for first, if calculate,need consider sSlot mark cigs < 0  */
        //pCigSlave->bSlot_mark_cigs = (pCigSlave->sSlot_mark_cigs>>5) + bltSche.bSlot_idx_start;

        /* SiHui: I can not remember why this code
         * same code in cIS master cause bug 20220730 CIS_CEN_BV-51 when disconnect one CIS another is still on */
        bltSche.sSlot_idx_reset = 2;

        tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] time5", pCigSlave->cigs_start_tick, pCigSlave->sSlot_mark_cigs, 0, 0);

        blt_sche_addTaskMask(pm_check_info ? TSKMSK_CIG_SLAVE_0<<cigSlv_index : 0);
        blt_ll_incSchedulerTaskCalPriority( TSKOFT_CIG_SLV + cigSlv_index, TASK_PRIORITY_HIGH_THRES);
    }




    return 0;
}



_attribute_ram_code_
static int  blt_ll_find_next_cis_slv_subevent (int start_idx)
{
    int ret = 100;  //important

    for(int i = start_idx; i < blt_pCisSlv->ciss_total_se_num; i++){ //If cig slot task is valid, ciss_total_se_num must be greater than 0.
        if(blt_pCisSlv->ciss_arrgmtMap_en_msk & BIT(i)){
            //Get the first valid CIS or the next valid CIS(if se_skip exist)from CIS arrangement map.
            u8 cur_cis_sel = blt_pCisSlv->ciss_arrgmtMap_cisIdx[i];
            ll_cis_conn_t *pCisConn = (ll_cis_conn_t*)(global_pCisConn + cur_cis_sel); //The first valid CIS

        #if(CIS_ADD_CIE)
            if(pCisConn->cie_flag){
                continue;
            }
        #endif
            if(pCisConn->cisSchedFlg){
                cisConn_param.blt_cis_sel = cur_cis_sel;
                blt_pCisConn = pCisConn; //The first valid CIS
                blt_pCisSlv->cigs_se_idx = blt_pCisSlv->ciss_arrgmtMap_seIdx[i];
                blt_pCisSlv->ciss_map_next_taskIdx = i + 1;

                return i;
            }
        }
    }

    return ret;
}



_attribute_ram_code_
int         blt_cig_slv_start (int slotTask_idx)
{
    DBG_SIHUI_CHN4_HIGH;
    DBG_FANQH_CHN4_HIGH;
    DBG_TIANXIANG_CHN5_HIGH;
#if (SL01_cis_group)
    log_task_begin_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis_group);
#endif


    //1.First locate the CIG that belongs to
    cisSlv_param.blt_cisSlvSel = slotTask_idx;
    blt_pCisSlv =  (ll_cis_slv_t *)(global_pCisSlv + cisSlv_param.blt_cisSlvSel);


    #if 1 //for multiple CIS dynamic change, anchor point between 2 ISO event may less than one ISO interval
        int cigs_jump_num = (bltSche.bSlot_idx_irq_real + 4 - blt_pCisSlv->bSlot_mark_cigs)/blt_pCisSlv->ciss_bSlotInterval;
        if(cigs_jump_num){
            cigs_jump_num -= 1;
        }
    #else
        int cigs_jump_num = (bltSche.bSlot_idx_irq_real + 4 - blt_pCisSlv->bSlot_mark_cigs)/blt_pCisSlv->ciss_bSlotInterval - 1;
    #endif

    for(int i = bltCisMng.maxNum_cisMaster; i < bltCisMng.maxNum_cisConn; i++ ){
        if(blt_pCisSlv->ciss_task_msk & BIT(i)){

            ll_cis_conn_t *pCisConn = (ll_cis_conn_t  *)(global_pCisConn + i);
            pCisConn->cisSubEventCnt = 0; //reset current SE index
            pCisConn->get_rxTimestamp = 0;
            pCisConn->cig_next_tick = clock_time() | 1;

            #if(CIS_ADD_CIE)
                pCisConn->local_cie = 0;
                pCisConn->peer_cie = 0;
                pCisConn->cie_flag = 0;
            #endif


            int cis_scheduled = 0;
            if(!pCisConn->cisSchedFlg && pCisConn->cis1stSchedAPbSlot){


                tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] AP1", bltSche.bSlot_idx_irq_real, pCisConn->cis1stSchedAPbSlot, 0, 0);
                //BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991C0000);

                if(bltSche.bSlot_idx_irq_real < (pCisConn->cis1stSchedAPbSlot + 2)){
                    u32 schTaskEndbSlot = bltSche.bSlot_idx_irq_real + (blt_pCisSlv->ciss_sSlotDuration>>5) + 1;


                    tlkapi_send_string_u32s(DBG_CIS_1ST_AP_TIMING_EN, "[CISP][TIM] AP2", bltSche.bSlot_idx_irq_real, blt_pCisSlv->ciss_sSlotDuration, schTaskEndbSlot, pCisConn->cis1stSchedAPbSlot);
                    //BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991C0000);


                    int bSlot_diff = (int)(schTaskEndbSlot - pCisConn->cis1stSchedAPbSlot);
                    if(bSlot_diff > 0){
                        cis_scheduled = 1;
                        pCisConn->cisSchedFlg = 1;


                        #if(WALKAROUND_ISO_TIMESTAMP_EN)
                            pCisConn->cis_ap_tick = 200;
                        #endif

                        pCisConn->cis_jump_num = bSlot_diff/blt_pCisSlv->ciss_bSlotInterval;

                        if(pCisConn->cis_jump_num > 10){
                            write_dbg32(0x0018, schTaskEndbSlot);
                            write_dbg32(0x001C, pCisConn->cis1stSchedAPbSlot);
                            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x991E0000 | pCisConn->cis_jump_num);
                        }

                        pCisConn->cis1stSchedAPbSlot = 0;
                    }
                }
            }


            if(!cis_scheduled){
                pCisConn->cis_jump_num = cigs_jump_num;
                if(pCisConn->cis_jump_num > 10){
                    BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99130000 | pCisConn->cis_jump_num);
                }

            }

            if(pCisConn->cis_jump_num){


                blt_ll_cis_ft_event_jump(pCisConn, pCisConn->cis_jump_num);

            }
        }
    }

    blt_pCisSlv->cigs_1st_rx_tick = 0;
    blt_pCisSlv->ciss_map_next_taskIdx = 0;
    blt_pCisSlv->cigs_working = 1;
    blt_pCisSlv->cigs_sSlotOffset = 0;
    blt_pCisSlv->sSlot_mark_cigs = bltSche.sSlot_idx_irq_real;
    blt_pCisSlv->bSlot_mark_cigs = bltSche.bSlot_idx_irq_real;
    blt_pCisSlv->cigs_start_tick = bltSche.sSlot_tick_irq_real;

    #if (CIS_WINDOW_WIDENING_FOR_BIG_PPM)
        u32 widen_tick = blt_pCisSlv->ciss_widen_us*SYSTEM_TIMER_TICK_1US;
        /* solve problem, if CIS jumped, cigs expect tick need update */
        blt_pCisSlv->cigs_expect_tick = bltSche.sSlot_tick_irq_real + CIGSLV_EARLY_SET_TICK + widen_tick;
    #else

        /* solve problem, if CIS jumped, cigs expect tick need update */
        blt_pCisSlv->cigs_expect_tick = bltSche.sSlot_tick_irq_real + CIGSLV_EARLY_SET_TICK;
    #endif

    u8 se_idx = blt_ll_find_next_cis_slv_subevent(0);

    if(se_idx == 100){
        tlkapi_send_string_data(DEB_CIG_SLV_EN, "Big start task, se_dix == 100: impossible!!!", 0, 0);

        write_dbg32(0x0018, blt_pCisSlv->ciss_arrgmtMap_en_msk);
        write_dbg32(0x001C, *(u32 *)(&blt_pCisSlv->ciss_arrgmtMap_cisIdx[0]));
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99010000);
    }
    else{
        if(blt_pCisSlv->cigs_se_idx != 1){
            BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99140000 | blt_pCisSlv->cigs_se_idx);
        }

        if(se_idx == 0){
            blt_crx_start();
        }
        else{
            if(blt_pCisConn->cig_ap_distan_us == 0){
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99150000 | (blt_pCisConn->cig_ap_distan_us & 0xFFFF));
            }

            u32 anchor_t = blt_pCisSlv->cigs_expect_tick + blt_pCisConn->cig_ap_distan_us * SYSTEM_TIMER_TICK_1US;
            if(tick1_out_range_of_tick2(blt_pCisConn->cis_expect_tick, anchor_t, 50*SYSTEM_TIMER_TICK_1US)){
                write_dbg32(0x0018, blt_pCisConn->cis_expect_tick);
                write_dbg32(0x001C, anchor_t);
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99160000);
            }

            u32 crx_t = anchor_t - CRX_EARLY_SET_TICK;
            if(!tick1_exceed_tick2(crx_t, clock_time())){
                BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99170000);
            }
            systimer_set_irq_capture(crx_t);
            systick_irq_trigger = SYS_IRQ_TRIG_CRX_START;
        }
    }




    //these logic setting executing after FSM trigger setting to save time
    blt_pCisSlv->pcisDisconn = NULL;
    blt_pCisSlv->cigs_finish = 0;

    return 0;
}


_attribute_ram_code_
void        blt_cig_slv_post(void)
{

    blms_state = BLMS_STATE_CIG_E;
    blt_pCisSlv->cigs_sSlotWiden = 0;
    blt_pCisSlv->ciss_widen_us = 0;

    if(!blt_pCisSlv->cigs_finish)
    {

        if(blt_pCisSlv->cigs_1st_rx_tick){
            blt_pCisSlv->cigs_expect_tick = blt_pCisSlv->cigs_1st_rx_tick + blt_pCisSlv->pCis1st->iso_intvl_tick; //blt_pCisSlv->
            blt_pCisSlv->cigs_expectTick_2 = blt_pCisSlv->cigs_expect_tick;

            u32 tick_offset_1st_rx = blt_pCisSlv->cigs_1st_rx_tick - blt_pCisSlv->cigs_start_tick;
            u32 tick_offset_expect = CIGSLV_EARLY_SET_US * SYSTEM_TIMER_TICK_1US;
            blt_pCisSlv->cigs_sSlotOffset  = (s32)(tick_offset_1st_rx - tick_offset_expect)*SSLOT_TICK_REVERSE;

            if(blt_pCisSlv->cigs_sSlotOffset > 2 || blt_pCisSlv->cigs_sSlotOffset < -2){
                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
            }

            #if 0 //(DBG_CIS_TIMING && !CIS_WINDOW_WIDENING_FOR_BIG_PPM)
                if(blt_pCisSlv->cigs_sSlotOffset > 50 || blt_pCisSlv->cigs_sSlotOffset < -50)  {
                    tlkapi_send_string_data(DBG_CIS_IRQ_TIMING_DUMPLOG, "comp too much!!!", &cigSlvCompSslot, 4);
                    BLMS_ERR_DEBUG(DBG_CIS_TIMING, 0xAAAA0000);
                }
            #endif
        }
        else{
            blt_pCisSlv->cigs_expect_tick += blt_pCisSlv->pCis1st->iso_intvl_tick;
            blt_pCisSlv->cigs_expectTick_2 += blt_pCisSlv->pCis1st->iso_intvl_tick;


            #if (CIS_WINDOW_WIDENING_FOR_BIG_PPM)
                u32 tick_diff = (u32)(blt_pCisSlv->cigs_expectTick_2 - blt_pCisSlv->cigs_mark_rx_tick);
                if(tick_diff < BIT(30)){
                    u8 ppmDiv10 = blt_pAclConn->ppm_div_10;
                    if(ppmDiv10 == 50){ //500ppm
                        ppmDiv10 = 60; //add margin: 100ppm
                    }
                    /* here ppmDiv10 max value 60, use 64 = BIT(6), for diff bigger than BIT(26), consider 32bit register overflow
                     * BIT(26) for 16M  Stimer is 4.194 S, for 24M Stimer is 2.796 S */
                    int widen_us;
                    if(tick_diff > BIT(26)){
                        widen_us = tick_diff/(100*SYSTEM_TIMER_TICK_1MS)*ppmDiv10;
                    }
                    else{
                        widen_us = tick_diff * ppmDiv10/(100*SYSTEM_TIMER_TICK_1MS);
                    }

                    if(widen_us > 50 ){
                        if(widen_us > 1000){
                            widen_us = 1000;
                        }
                        tlkapi_send_string_data(CIS_DEBUG_EN, "[CISP][TIM] widen_us", &widen_us, 4);
                        //DBG_C HN12_TOGGLE;
                        blt_pCisSlv->cigs_start_tick = blt_pCisSlv->cigs_expectTick_2 - widen_us*SYSTEM_TIMER_TICK_1US - CIGSLV_EARLY_SET_TICK;
                        int n_sSlot = (blt_pCisSlv->cigs_start_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
                        blt_pCisSlv->sSlot_mark_cigs = bltSche.sSlot_idx_irq_real + n_sSlot - blt_pCisSlv->ciss_sSlotInterval;
                        blt_pCisSlv->bSlot_mark_cigs = (blt_pCisSlv->sSlot_mark_cigs>>5) + bltSche.bSlot_idx_start;
                        blt_pCisSlv->cigs_sSlotWiden = widen_us*2*SSLOT_US_REVERSE;
                        blt_pCisSlv->ciss_widen_us = widen_us;
                        blt_sche_addUpdate(SLOT_UPDT_CIS_SLAVE_TERMINATE);
                    }
                }
                else{
                    BLMS_ERR_DEBUG(DBG_CIS_SLAVE_TIMING, 0x998D0000);
                }
            #endif
        }


        //Update the next expected CIS task anchor pointer
        for(int i = bltCisMng.maxNum_cisMaster; i < bltCisMng.maxNum_cisConn; i++ ){
            if(blt_pCisSlv->ciss_task_msk & BIT(i)){

                ll_cis_conn_t *pCis = (ll_cis_conn_t  *)(global_pCisConn + i);

                if(pCis->cisSchedFlg){

                #if(CIS_ADD_CIE)
                    blt_ll_cis_ft_subevent_commm(pCis, pCis->nse - pCis->cisSubEventCnt);
                #endif
                    pCis->cisEventCnt ++;
                    pCis->cis_expect_tick = blt_pCisSlv->cigs_expect_tick + pCis->cig_ap_distan_us * SYSTEM_TIMER_TICK_1US;

                    if(pCis->pCisTestParam && (!pCis->pCisTestParam->tranMode.isoTestSendTick)){
                        pCis->pCisTestParam->tranMode.isoTestSendTick = clock_time() | 1;
                    }
                }
            }
        }


        #if (ONE_ACL_SLAVE_MATCH_2_CIS_SLAVE_ENABLE)
            if(blt_pCisSlv->pcisDisconn){ //at least one CIS disconnect, but other CIS still work
                /* now only process CIS slave, so one CIS slave is left */

                ll_cis_conn_t *pCisConn = blt_pCisSlv->pcisDisconn == blt_pCisSlv->pCis1st ? blt_pCisSlv->pCis2nd : blt_pCisSlv->pCis1st;
                blt_pCisSlv->ciss_total_se_num = pCisConn->nse;
                blt_pCisSlv->ciss_arrgmtMap_en_msk = 0;

                u8 cis_conn_idx = pCisConn->cis_index;
                for(int j=0; j< pCisConn->nse; j++){
                    blt_pCisSlv->ciss_arrgmtMap_en_msk |= BIT(j);
                    blt_pCisSlv->ciss_arrgmtMap_cisIdx[j] = cis_conn_idx;
                    blt_pCisSlv->ciss_arrgmtMap_seIdx[j] = j + 1;

                    //tlkapi_send_string_u32s(CIS_DEBUG_EN, "update map", i + j * pCigSlave->ciss_alloc_count, pCigSlave->ciss_order_tbl[i], 0, 0);
                }

                if(pCisConn == blt_pCisSlv->pCis2nd){
                    blt_pCisSlv->cigs_expect_tick += blt_pCisSlv->pCis2nd->cig_ap_distan_us*SYSTEM_TIMER_TICK_1US;
                    blt_pCisSlv->cigs_start_tick = blt_pCisSlv->cigs_expect_tick - CIGSLV_EARLY_SET_TICK;

                    int n_sSlot = (blt_pCisSlv->cigs_start_tick - bltSche.sSlot_tick_irq_real)*SSLOT_TICK_REVERSE;
                    blt_pCisSlv->sSlot_mark_cigs = bltSche.sSlot_idx_irq_real + n_sSlot - blt_pCisSlv->ciss_sSlotInterval;
                    blt_pCisSlv->bSlot_mark_cigs = (blt_pCisSlv->sSlot_mark_cigs>>5) + bltSche.bSlot_idx_start;
                }

                pCisConn->cig_ap_distan_us = 0;
                blt_pCisSlv->pCis1st = pCisConn;
                blt_pCisSlv->ciss_order_tbl[0] = cis_conn_idx;
                blt_pCisSlv->offset_diff_us = 0;
                blt_pCisSlv->ciss_sSlotAllocNum = pCisConn->cis_maxPossible_us*SSLOT_US_REVERSE + 1;

                blt_sche_addUpdate(SLOT_UPDT_CIS_SLAVE_TERMINATE);
            }
        #endif




    }


    for(int i = bltCisMng.maxNum_cisMaster; i < bltCisMng.maxNum_cisConn; i++ ){
        if(blt_pCisSlv->ciss_task_msk & BIT(i)){
            ll_cis_conn_t *pCis = (ll_cis_conn_t  *)(global_pCisConn + i);
            if(pCis->pCisTestParam && (pCis->pCisTestParam->isoTestMode==ISO_TEST_TRANSMIT_MODE)
                    &&(!pCis->pCisTestParam->tranMode.isoTestSendTick))
            {
                pCis->pCisTestParam->tranMode.isoTestSendTick = clock_time() | 1;
            }
        }
    }



    blt_pCisSlv->cigs_working = 0;



    blt_ll_calculate_sSlot_next(clock_time() + (bltSche.sche_process_us + SCHE_NEW_TASK_MARGIN_US)*SYSTEM_TIMER_TICK_1US );



    DBG_SIHUI_CHN4_LOW;
    DBG_FANQH_CHN4_LOW;

    DBG_TIANXIANG_CHN5_LOW;
#if (SL01_cis_group)
    log_task_end_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis_group);
#endif
}


_attribute_ram_code_
int         blt_crx_start (void)
{
    //make sure state machine is clean
    STOP_RF_STATE_MACHINE;

//  tlkapi_send_string_u32s(0, "brx start", blt_pCisConn->cisEventCnt,blt_pCisConn->cisSubEventCnt, blt_pCisConn->cisSendPldNum,0);

    blt_ll_cis_start_common_1(blt_pCisConn);

    rf_start_fsm(FSM_RX2TX, NULL, clock_time());

    /* SiHui confirm with QiangKai 20230428: baseband digital setting can write after triggering FSM,
     * as long as writing quickly before old setting value take effect */
    #if (CIS_WINDOW_WIDENING_FOR_BIG_PPM)
        rf_set_1st_rx_timeout(300 + bltPHYs.prmb_ac_us + blt_pCisSlv->ciss_widen_us*2);
    #else
        rf_set_1st_rx_timeout(300 + bltPHYs.prmb_ac_us);
    #endif


    #if(HW_AES_CCM_ALG_EN)
        //enable HW AES_CCM TIFS 147.625us, disable HW AES_CCM ITFS 149us
        rf_ble_set_tx_settle(tx_stl_auto_mode[blt_pCisConn->curCisPhy]+2);
    #else
        rf_ble_set_tx_settle(tx_stl_auto_mode[blt_pCisConn->curCisPhy]);
    #endif

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_RX_ON);  }

    #if 1 //debug
        u8 cis_sel_slv = cisConn_param.blt_cis_sel - bltCisMng.maxNum_cisMaster;
        if(cis_sel_slv == 0){
            DBG_SIHUI_CHN5_HIGH;
            DBG_FANQH_CHN5_HIGH;
        }
        else if(cis_sel_slv == 1){
            DBG_SIHUI_CHN6_HIGH;
            DBG_FANQH_CHN6_HIGH;
        }

        #if(SL01_cis0)
            log_task_begin_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis0 + cis_sel_slv);
        #endif
    #endif

    //these logic setting executing after CRX setting to save time
    blt_ll_cis_start_common_2(blt_pCisConn);

    blms_state = BLMS_STATE_CRX_S;


    //system trigger point: consider that RX IRQ must processed
    systick_irq_trigger = SYS_IRQ_TRIG_CRX_POST;
#if (CIS_WINDOW_WIDENING_FOR_BIG_PPM)
    systimer_set_irq_capture(blt_pCisConn->cis_expect_tick + (blt_pCisSlv->ciss_widen_us + blt_pCisConn->MPTM_TIFS_MPTS + 20)*SYSTEM_TIMER_TICK_1US);
#else
    systimer_set_irq_capture(blt_pCisConn->cis_expect_tick + (blt_pCisConn->MPTM_TIFS_MPTS + 20)*SYSTEM_TIMER_TICK_1US);
#endif
    return 1;
}


_attribute_ram_code_
int         blt_crx_post (ll_cis_conn_t *pCisConn)
{
    blms_state = BLMS_STATE_CRX_E;

    int cigs_end = 0;


    int cis_status = blt_ll_cis_post_common(pCisConn);

    if(cis_status != BLE_SUCCESS){

        blt_ll_removeCisSlave(cis_status);

        /* check if last CIS slave removed on CIG */
        if(blt_pCisSlv->ciss_se_en_num == 0){
            blt_pCisSlv->ciss_total_se_num = 0; //clear
            blt_pCisSlv->cig_slv_occupied = 0;

            blt_sche_addUpdate(SLOT_UPDT_CIS_SLAVE_TERMINATE);
            blt_sche_removeTaskMask(TSKMSK_CIG_SLAVE_0<<cisSlv_param.blt_cisSlvSel);

            blt_pCisSlv->cigs_finish = 1;
            cigs_end = 1;
        }
    }
    else{

        if(!pCisConn->get_rxTimestamp && pCisConn->cis_1st_rx_tick){
            pCisConn->get_rxTimestamp = 1;

            #if 0 //debug code, remove later
                        u32 tick = pCisConn->cis_1st_rx_tick - pCisConn->cig_ap_distan_us *SYSTEM_TIMER_TICK_1US ;
                        if(pCisConn->cis_index == 3){

                            if(blt_pCisSlv->cigs_1st_rx_tick){
                                tlkapi_send_string_u32s(DBG_CIS_IRQ_TIMING_DUMPLOG, "cig dbg1", blt_pCisSlv->cigs_start_tick, blt_pCisSlv->cigs_1st_rx_tick, (ll_cis_conn_t *) (global_pCisConn + 2)->cis_1st_rx_tick,0);
                                tlkapi_send_string_u32s(DBG_CIS_IRQ_TIMING_DUMPLOG, "cig dbg2", tick, pCisConn->cis_1st_rx_tick, pCisConn->cig_ap_distan_us,0);
                            }
                        }
            #endif

            if(!blt_pCisSlv->cigs_1st_rx_tick){
                blt_pCisSlv->cigs_1st_rx_tick = pCisConn->cis_1st_rx_tick - pCisConn->cig_ap_distan_us *SYSTEM_TIMER_TICK_1US;
                blt_pCisSlv->cigs_mark_rx_tick = blt_pCisSlv->cigs_1st_rx_tick;
            }

        }

        pCisConn->cis_expect_tick += pCisConn->sub_intvl_tick;
        blt_cis_post_common_2(pCisConn);
    }



    if(!cigs_end){
        /* attention that: blt_pCisConn may be another CIS pointer after this function */
        u8 next_nse_idx = blt_ll_find_next_cis_slv_subevent(blt_pCisSlv->ciss_map_next_taskIdx);

        if(next_nse_idx < blt_pCisSlv->ciss_total_se_num){

            /* debug code, remove later */
            #if 0
                /* EBQ test, code stuck here. use "LL-PER-BV-04-C" for example
                 * peer master use 210mS connection interval, every interval RX timeStamp from "hal_rf_get_rx_timestamp()" delta value is 210007 uS
                 * 7 uS error is crystal difference between slave and master.
                 * "cis_1st_anchor tick" is calculated when receive "LL_CIS_IND" in connection event 0x15, this will determine "cigs_expect tick"
                 * first CIS event is after connection event 0x1d, 8 connection event will bring 7*8 = 56uS error, so here we use 50uS judge is not OK
                 */
                u32 anchor_t = blt_pCisSlv->cigs_expect_tick + (blt_pCisConn->cig_ap_distan_us + blt_pCisConn->sub_intvl_us *(blt_pCisSlv->cigs_se_idx - 1))*SYSTEM_TIMER_TICK_1US;
                if(tick1_out_range_of_tick2(blt_pCisConn->cis_expect_tick, anchor_t, 50*SYSTEM_TIMER_TICK_1US)){
                    write_dbg32(0x0018, blt_pCisConn->cis_expect_tick);
                    write_dbg32(0x001C, anchor_t);


                    //tlkapi_send_string_u32s(0, "TTT1", blt_pCisSlv->cigs_expect_tick, blt_pCisConn->cig_ap_distan_us, blt_pCisConn->sub_intvl_us, blt_pCisSlv->cigs_se_idx);
                    //tlkapi_send_string_u32s(0, "TTT2", anchor_t, blt_pCisConn->cis_expect_tick, pCisConn->sub_intvl_tick, cisConn_param.cis_1st_anchor_tick);
                    BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99180000);
                }
            #endif

            u32 tick_now = clock_time();
            u32 crx_t = blt_pCisConn->cis_expect_tick - CRX_EARLY_SET_TICK;
            if(tick1_exceed_tick2(tick_now, crx_t)){ //time not enough
                if(tick1_exceed_tick2(crx_t + 30*SYSTEM_TIMER_TICK_1US, tick_now)){ //CRX_EARLY_SET_US - 30uS, time enough
                    crx_t = tick_now + 5*SYSTEM_TIMER_TICK_1US;
                }
                else{
                    write_dbg32(0x0018, crx_t);
                    write_dbg32(0x001C, tick_now);
                    BLMS_ERR_DEBUG(DBG_CIS_TIMING, 0x991F0000);
                }
            }

            systimer_set_irq_capture(crx_t);
            systick_irq_trigger = SYS_IRQ_TRIG_CRX_START;
        }
        else{
            cigs_end = 1;
            tlkapi_send_string_data(DEB_CIG_SLV_EN, "cis end", 0, 0);
        }
    }

    if(blc_rf_pa_cb){   blc_rf_pa_cb(PA_TYPE_OFF);  }




    if(cigs_end){
        blt_cig_slv_post();
    }



    u8 cis_sel_slv = pCisConn->cis_index - bltCisMng.maxNum_cisMaster;
    if(cis_sel_slv == 0){
        DBG_SIHUI_CHN5_LOW;
        DBG_FANQH_CHN5_LOW;
    }
    else if(cis_sel_slv == 1){
        DBG_SIHUI_CHN6_LOW;
        DBG_FANQH_CHN6_LOW;
    }


    #if(SL01_cis0)
        log_task_end_irq(SL_STACK_CIS_BASIC_TIMING_EN, SL01_cis0 + cis_sel_slv);
    #endif

    return 1;

}










_attribute_noinline_
void blt_cis_clearCisSlaveMainloopStatus(ll_cis_conn_t *pCisConn, ll_cis_slv_t *pCigSlave)
{
    if(!bltCisMng.curNum_cisSlave && !pCigSlave->ciss_alloc_count){
        BLMS_ERR_DEBUG(CIS_DEBUG_EN, 0x99020000 | bltCisMng.curNum_cisSlave<<8 | pCigSlave->ciss_alloc_count);
    }

    pCigSlave->ciss_alloc_count --;

    /* set when receive CIS_IND, here may clear a zero mask(if called in reject CIS_REQ), no problem */
    pCigSlave->ciss_alloc_msk &= ~BIT(pCisConn->cis_index);
    bltCisMng.curNum_cisSlave --;
    pCisConn->cis_occupied = 0;

    bltCisMng.cisFlow_pending &= ~BIT(pCisConn->cis_index);
    bltCisMng.cisFlow_idx = INVALID_CIS_IDX;
    pCisConn->cisFlowFlg = CIS_FLOW_IDLE;

}





#endif   //end of LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
