/********************************************************************************************************
 * @file    smp2.c
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
#include "stack/ble/host/ble_host.h"
#include "stack/ble/controller/ble_controller.h"

#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif

extern int blt_smp_saveBondingInfoToFlash(u8 isCentral, u8 perDevIdx, smp_param_save_t* save_param);


// H: Initiator Capabilities
// V: Responder Capabilities
// See the Core_v5.0(Vol 3/Part H/2.3.5.1) for more information.
static const stk_generationMethod_t gen_method_legacy[5 /*Responder IOCap*/][5 /*Initiator IOCap*/] = {
    { JustWorks,                JustWorks,                PK_Resp_Display_Init_Input, JustWorks, PK_Resp_Display_Init_Input },
    { JustWorks,                JustWorks,                PK_Resp_Display_Init_Input, JustWorks, PK_Resp_Display_Init_Input },
    { PK_Init_Display_Resp_Input, PK_Init_Display_Resp_Input, PK_BOTH_INPUT,            JustWorks, PK_Init_Display_Resp_Input },
    { JustWorks,                JustWorks,                JustWorks,                JustWorks, JustWorks                },
    { PK_Init_Display_Resp_Input, PK_Init_Display_Resp_Input, PK_Resp_Display_Init_Input, JustWorks, PK_Init_Display_Resp_Input },
};

static const stk_generationMethod_t gen_method_sc[5 /*Responder IOCap*/][5 /*Initiator IOCap*/] = {
    { JustWorks,                JustWorks,                PK_Resp_Display_Init_Input, JustWorks, PK_Resp_Display_Init_Input },
    { JustWorks,                Numeric_Comparison,        PK_Resp_Display_Init_Input, JustWorks, Numeric_Comparison },
    { PK_Init_Display_Resp_Input, PK_Init_Display_Resp_Input, PK_BOTH_INPUT,            JustWorks, PK_Init_Display_Resp_Input },
    { JustWorks,                JustWorks,                JustWorks,                JustWorks, JustWorks                },
    { PK_Init_Display_Resp_Input, Numeric_Comparison,         PK_Resp_Display_Init_Input, JustWorks, Numeric_Comparison },
};


/***********************************************************************************************
                                 Slave Security Request
 ***********************************************************************************************/
void blc_smp_configSecurityRequestSending( secReq_cfg newConn_cfg,  secReq_cfg reConn_cfg, u16 pending_ms)
{
    blc_SecReq_ctrl.secReq_conn = (reConn_cfg<<4) | newConn_cfg;
    blc_SecReq_ctrl.pending_ms = pending_ms;
}

int blt_smp_sendSecurityRequest (u16 connHandle)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    blt_smp_setCertTimeoutTick(connHandle, clock_time()|1);

    u8 pkt_sec_req[8] = {
            0x02,           // type
            0x06,           //rf len
            0x02, 0x00,     //l2cap len
            0x06, 0x00,     // l2chn id
            SMP_OP_SEC_REQ, //opcode
            0
    };

    pkt_sec_req[7] = smp_param_own[ (conn_idx - LL_MAX_ACL_CEN_NUM + 1)].auth_req.authType;  //AuthReq

    return ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, pkt_sec_req);
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blt_smp_procSlaveSecurityRequest(u16 connHandle)
{
    u8 conn_idx = (connHandle & CONN_IDX_MASK);
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(!blt_ll_isEncryptionBusy(connHandle) && \
        pc->conn_established_tick && clock_time_exceed(pc->conn_established_tick, blc_SecReq_ctrl.pending_ms * 1000) )
    {
        if( blt_smp_sendSecurityRequest(connHandle) ){  //Security Request push FIFO OK
            blc_SecReq_ctrl.secReq_pending &= ~BIT(connHandle & CONN_IDX_MASK);  //clear security request pending flag
            my_dump_str_data(SMP_DBG_EN, "  Slv RF send SecReq", 0, 0);
        }
    }
}




/***********************************************************************************************
                                 Central Pairing Request
 ***********************************************************************************************/
_attribute_ble_data_retention_  _attribute_aligned_(4) smp_m_trig_t blt_smpTrig = {
        .trigger_mask = PairReq_AUTO_SEND | (PairReq_AUTO_SEND<<1),  //default master auto send pairing request
};


/**
 * when trigger_mask = PairReq_SEND_upon_SecReq|PairReq_SEND_upon_SecReq<<1,
 * this case need slave to send sec_req to trigger master start smp.
 * but if slave not send sec_req, master will never start smp. At this time,
 * user can call this API to set master start smp.
 * User can use this API to flexibly trigger master SMP.
 */
void blc_smp_triggerCentralManualSmp(void){
    blt_smpTrig.manual_smp_start = 1;
}

void blc_smp_configPairingRequestSending( PairReq_cfg newConn_cfg,  PairReq_cfg reConn_cfg)
{
    blt_smpTrig.trigger_mask = newConn_cfg | (reConn_cfg<<1);
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
void blt_smp_procCentralPairingRequest(u16 connHandle)
{
    if(blt_smpTrig.manual_smp_start)
    {
        if(clock_time_exceed(blc_ll_getConnectionStartTick(connHandle), 50*1000) )    //50 mS
        {
            blt_smpTrig.manual_smp_start = 0;

            if(!blt_smpTrig.smp_begin_flg){

                u8 pSecReq[8] = {0,0,0,0,0,0, SMP_OP_SEC_REQ, 0};  //rf_packet_l2cap_req_t
                u8* pr = blt_smp_l2capSmpCmdHandler (connHandle, pSecReq);
                if(pr)
                {
                    ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG,  pr);
                }

            }
        }
    }
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
int  blc_smp_isPairingBusy(u16 connHandle)   //this API may be used by user
{
    return smp_sts_param[connHandle & CONN_IDX_MASK].pairing_busy;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
void  blt_smp_setPairingBusy(u16 connHandle, u8 busy)
{
    smp_sts_param[connHandle & CONN_IDX_MASK].pairing_busy = busy;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
void blt_smp_setCertTimeoutTick (u16 conn_handle, u32 t)
{
    smp_sts_param[conn_handle & CONN_IDX_MASK].smp_timeout_start_tick = t;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
void blt_smp_certTimeoutLoopEvt(u16 connHandle)
{
    if( smp_sts_param[connHandle & CONN_IDX_MASK].smp_timeout_start_tick)
    {
        if(clock_time_exceed(smp_sts_param[connHandle & CONN_IDX_MASK].smp_timeout_start_tick, 30*1000*1000))
        {
            /*
             * In the case of SMP protocol command interaction timeout, the stack will not disconnect the ACL connection when the
             * timeout occurs. The processing of disconnecting the ACL connection will be handed by the user in the GAP event callback.
             */
            //blc_ll_disconnect(connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);

            //smp_sts_param[connHandle & CONN_IDX_MASK].smp_timeout_start_tick = 0;  //this will reset in "blt_smp_procPairingEnd" below
            blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_PAIRING_TIMEOUT);  //pairing timeout
        }
    #if OS_SUP_EN
        else{
            //blt_ll_sem_give();
            if(blt_os_semCountIncrement_cb)
            {
                blt_os_semCountIncrement_cb();
            }
        }
    #endif
    }
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
void blt_smp_procPairingEnd(u16 connHandle, u8 err_reason)  // 0 is OK; other for fail
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    smp_st_t * pSmpSts  = (smp_st_t *)&smp_sts_param[conn_idx];

    int is_master = (connHandle & BLM_CONN_HANDLE);
    u8 smp_status_idx = is_master ? 0: (conn_idx - LL_MAX_ACL_CEN_NUM + 1);
    smp_param_own_t *pSmpOwn  = (smp_param_own_t *)&smp_param_own[smp_status_idx];
    memset(pSmpOwn->pairing_tk, 0, 16); //careful

    //When a Pairing process completes or fail, the Security Manager Timer shall be stopped.
    blt_smp_setCertTimeoutTick(connHandle, 0);

    blt_smp_setPairingBusy(connHandle, 0);
    blt_ll_setEncryptionBusy(connHandle, 0);

    pSmpSts->key_distribute = 0;     //no this code on old single connection, evaluate
    pSmpSts->tk_status = TK_ST_IDLE; //no this code on old single connection, evaluate
    pSmpSts->smp_phase_chk = PAIRING_PHASE_IDLE; //need clear when pairing failed
    pSmpSts->smpDistributeKeyOrder = SMP_TRANSPORT_SPECIFIC_KEY_END;
    pSmpSts->smp_DistributeKeySend.keyIni = 0;
    pSmpSts->smp_DistributeKeyRecv.keyIni = 0;

    if(err_reason && (gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_FAIL) ){
        u8 param_evt[4];
        gap_smp_pairingFailEvt_t *pEvt = (gap_smp_pairingFailEvt_t *)param_evt;
        pEvt->connHandle = connHandle;
        pEvt->reason = err_reason;
        blc_gap_send_event ( GAP_EVT_SMP_PAIRING_FAIL, param_evt, sizeof(gap_smp_pairingFailEvt_t) );
    }

}





/**************************************************************************************************
blt_smp_saveBondingKey called by
1. blt_smp_sendInfo               <- Pairing Loop Entry

2. blt_smp_l2capSmpCmdHandler      <- SMP Data Entry

global blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here
 **************************************************************************************************/
void blt_smp_saveBondingKey(u16 connHandle)
{
    u8 is_master = connHandle & BLM_CONN_HANDLE;
    u8 conn_idx = connHandle & CONN_IDX_MASK;

    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);


#if 0 //global blms_p_sts/blms_p_prop/blms_p_own/blms_p_peer can be used here
    u8 smp_status_idx = is_master ? 0:(conn_idx - LL_MAX_ACL_CEN_NUM +1);
    smp_st_t * blms_p_sts  = (smp_st_t *)&smp_sts_param[conn_idx];
    smp_param_own_t *blms_p_own = (smp_param_own_t *)&smp_param_own[smp_status_idx];
    smp_param_peer_t *blms_p_peer = (smp_param_peer_t *)&smp_param_peer[smp_status_idx];
#endif


    if(blms_p_sts->save_key_flag){
        return;
    }
    blms_p_sts->save_key_flag = 1;

    if(blms_p_sts->smp_phase_chk == PAIRING_PHASE_2_OK){
        blms_p_sts->smp_phase_chk = PAIRING_PHASE_IDLE;
    }
    else{
    #if (DBG_SMP_ERR_EN) //Can be removed after debugging
        //printf("not possible2(hdl:0x%x), %d\n", connHandle, blms_p_sts->smp_phase_chk);
        SMP_ERR_DEBUG(0x66000077);
    #endif
    }

    int res = 0;
    if(blms_p_sts->bonding_enable)
    {
    #if (SMP_DATABASE_INFO_SOURCE == SMP_INFO_STORAGE_IN_FLASH)
        smp_param_save_t smp_param_save;

        smp_param_save.role_dev_idx = (is_master ? BIT(7) : 0) | (slave_dev_idx & MSK_SLAVE_DEV_IDX);

        smp_param_save.peer_addr_type = blms_p_peer->peer_addr_type;
        smemcpy(smp_param_save.peer_addr, blms_p_peer->peer_conn_addr, 6);

        smp_param_save.peer_id_adrType = blms_p_peer->peer_id_address_type;
        smemcpy(smp_param_save.peer_id_addr, blms_p_peer->peer_id_address, 6);

        smp_param_save.encrypt_key_size = blms_p_own->encrypt_key_size;

        #if (SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY)
            if(blms_p_own->cur_rpa)
            {
                smp_param_save.local_id_adrType = blms_p_own->idenAdr_type;
                smemcpy(smp_param_save.local_id_addr, blms_p_own->idenAdr_addr, 6);
            }
            else
        #endif
            {
                smp_param_save.local_id_adrType = blms_p_own->own_addr_type;
                smemcpy(smp_param_save.local_id_addr, blms_p_own->own_conn_addr, 6);
            }

        smemcpy(smp_param_save.local_peer_ltk, is_master ? blms_p_peer->peer_ltk:blms_p_own->own_ltk, 16);//LTK


        tlkapi_send_string_data(CS_IOP_EN || (stkLog_mask & STK_LOG_SMP_LTK), "[SMP][LTK] ", smp_param_save.local_peer_ltk, 16);

        //if the current connection is slave role, the value of ediv_rand is saved
        //using its own parameters, otherwise, the value of the peer device is used.
        smemcpy(smp_param_save.random, blms_p_peer->peer_random, 8);//Random
        smp_param_save.ediv = blms_p_peer->peer_ediv;//EDIV

        smemcpy(smp_param_save.peer_irk,  blms_p_peer->peer_irk, 16);
        smemcpy(smp_param_save.local_irk, blms_p_own->own_irk, 16);

        //core5.0 Vol3, Part H
        //The authentication requirements include the type of bonding and man-in-the-middle protection (MITM) requirements
        smpMStblBondDevice.pairing_status[conn_idx] = (blms_p_own->stk_method == JustWorks)
                                                      ? Unauthenticated_LTK
                                                      : (blms_p_own->sc_pairing ? Authenticated_LTK_Secure_Connection
                                                                                 : Authenticated_LTK_Legacy_Pairing);
        smp_param_save.flag = (FLAG_SMP_PARAM_SAVE_BASE | (smpMStblBondDevice.pairing_status[conn_idx]<<4));

        res = blt_smp_saveBondingInfoToFlash(is_master, slave_dev_idx, &smp_param_save);
    #else
        // add pairing info save interface here
    #endif
    }

    //pairing end successful
    blt_smp_procPairingEnd(connHandle, 0);   //pairing success

    if(gap_eventMask & GAP_EVT_MASK_SMP_PAIRING_SUCCESS){
        u8 param_evt[4];
        gap_smp_pairingSuccessEvt_t *pEvt = (gap_smp_pairingSuccessEvt_t *)param_evt;
        pEvt->connHandle = connHandle;
        pEvt->bonding = blms_p_sts->bonding_enable;
        pEvt->bonding_result = res ? 1:0;//1:successfully, 0: fail

        blc_gap_send_event( GAP_EVT_SMP_PAIRING_SUCCESS, param_evt, sizeof(gap_smp_pairingSuccessEvt_t) );
    }

    if(gap_eventMask & GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE){
        u8 param_evt[4];
        gap_smp_securityProcessDoneEvt_t *pEvt = (gap_smp_securityProcessDoneEvt_t *)param_evt;
        pEvt->connHandle = connHandle;
        pEvt->re_connect = SMP_STANDARD_PAIR;

        blc_gap_send_event( GAP_EVT_SMP_SECURITY_PROCESS_DONE, param_evt, sizeof(gap_smp_securityProcessDoneEvt_t) );
    }
}




/*
 * Return STK generate method.
 * See the Core_v5.0(Vol 3/Part H/2.3.5.1) for more information.
 * */
int blt_smp_getStkGenMethod (smp_param_own_t *pSmpOwn, int SC_en)
{
    int responder_MITM  = pSmpOwn->pairing_rsp.authReq.MITM;
    int responder_oob   = pSmpOwn->pairing_rsp.oobDataFlag;
    int responder_iocap = pSmpOwn->pairing_rsp.ioCapability;

    int initiator_MITM  = pSmpOwn->pairing_req.authReq.MITM;
    int initiator_oob   = pSmpOwn->pairing_req.oobDataFlag;
    int initiator_iocap = pSmpOwn->pairing_req.ioCapability;

    if (SC_en + responder_oob + initiator_oob >= 2)  //simplify the judge: 2 of them is true, OOB is available
    {
    #if(SMP_SC_OOB_EN)
        if(SC_en){
            return SC_OOB_Authentication;
        }
        else
    #endif
        {
            return OOB_Authentication;
        }
    }

    if(!responder_MITM && !initiator_MITM)
    {
        return JustWorks;
    }

    if(responder_iocap > IO_CAPABILITY_KEYBOARD_DISPLAY || initiator_iocap > IO_CAPABILITY_KEYBOARD_DISPLAY)    //data[0] io cap
    {
        return JustWorks;
    }

    if(SC_en)
    {
        return gen_method_sc[responder_iocap][initiator_iocap];
    }
    else
    {
        return gen_method_legacy[responder_iocap][initiator_iocap];
    }
}



