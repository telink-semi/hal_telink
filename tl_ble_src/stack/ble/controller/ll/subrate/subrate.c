/********************************************************************************************************
 * @file    subrate.c
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

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)

subrate_param_t bltSubrateDft;


void      blt_ll_sendSubrateReq(u16 connHandle);
bool      blt_ll_sendSubrateInd(u16 connHandle);
ble_sts_t blt_ll_subrate_control_pdu_process(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt);
int       blt_subrate_interrupt_task(int flag, void *p);
int       blt_ll_subrate_mainloop(void *p);
int       blt_ll_subrate_insertContiTask(st_ll_conn_t *pAcl);
void      blt_ll_resetSubrate(void);
int       blt_subrate_mainloop_task(int flag, void *p);
ble_sts_t blt_ll_subrate_process_subrate_req(st_ll_conn_t *pAclConn, u8 *pLlCtrlPkt);

void blt_ll_resetSubrateByHandle(u16 handle);

ble_sts_t blc_setHostFeatureConnSubrate_en(u8 en)
{
    return blc_ll_setHostFeature(LL_FEATURE_BIT_NUMBER_CONNECTION_SUBRATING_HOST, en);
}

_attribute_noinline_ void blc_ll_initConnSubrate_feature(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
        //STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(XXX)), subrate);
    #endif

    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_CONNECTION_SUBRATING; //| (LL_FEATURE_SUPPORT_CONNECTION_SUBRATING_HOST << 6)

    ll_acl_subrate_ctrl_handler = blt_ll_subrate_control_pdu_process;
    ll_acl_subrate_irq_task_cb  = blt_subrate_interrupt_task;
    ll_acl_subrate_mlp_task_cb  = blt_subrate_mainloop_task;
    ll_acl_subrate_process_req  = blt_ll_subrate_process_subrate_req;

    blmsParam.subrate_en = 1;
}

ble_sts_t blt_ll_subrate_process_subrate_req(st_ll_conn_t *pAclConn, u8 *pLlCtrlPkt)
{
    ble_sts_t        ret       = BLE_SUCCESS;
    subrate_param_t *pAcptPara = &pAclConn->subrateParam_req;
    u16              factor_min_req, factor_max_req, max_latency_req, conti, timeout_req;

    if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CONNECTION_SUBRATING_HOST)) {
        ret = HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    } else {
        factor_min_req  = pLlCtrlPkt[3] | (pLlCtrlPkt[4] << 8);
        factor_max_req  = pLlCtrlPkt[5] | (pLlCtrlPkt[6] << 8);
        max_latency_req = pLlCtrlPkt[7] | (pLlCtrlPkt[8] << 8);
        conti           = pLlCtrlPkt[9] | (pLlCtrlPkt[10] << 8);
        timeout_req     = pLlCtrlPkt[11] | (pLlCtrlPkt[12] << 8);

        if ((pAcptPara->valid) && ((max_latency_req > pAcptPara->factor_max) || (timeout_req > pAcptPara->subrate_timeout) ||
                                   (factor_max_req < pAcptPara->factor_min) || (factor_min_req > pAcptPara->factor_max) || ((pAclConn->conn_intvl_n_1m25 * 1250 * factor_min_req * (max_latency_req + 1)) * 2 >= timeout_req * 10000))) {
            ret = HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }


    if (ret != BLE_SUCCESS) {
        blt_llms_rejectInd(pAclConn->acl_conHandle, LL_SUBRATE_REQ, ret, 1);
    } else {
        if (pAcptPara->valid) {
            u16 factor_min = max(factor_min_req, pAcptPara->factor_min);
            u16 factor_max = min(factor_max_req, pAcptPara->factor_max);

            pAclConn->factor_next    = (factor_min + factor_max) / 2;
            pAclConn->conti_num_next = min(max(pAcptPara->conti_num, conti), pAclConn->factor_next - 1);

            pAclConn->per_latency_next     = min(pAcptPara->max_latency, max_latency_req);
            pAclConn->subrate_timeout_next = min(timeout_req, pAcptPara->subrate_timeout);

        } else {
        }
        pAclConn->subrate_flag.bit.subrate_req_pending = 1;
    }

    return ret;
}

_attribute_ram_code_
    ble_sts_t
    blt_ll_subrate_control_pdu_process(st_ll_conn_t *pAclConn, u8 opcode, u8 *pLlCtrlPkt)
{
    if (opcode == LL_SUBRATE_IND) {
        u16 factor           = pLlCtrlPkt[3] | (pLlCtrlPkt[4] << 8);
        u16 subrateBaseEvent = pLlCtrlPkt[5] | (pLlCtrlPkt[6] << 8);
        u16 latency          = pLlCtrlPkt[7] | (pLlCtrlPkt[8] << 8);
        u16 continue_num     = pLlCtrlPkt[9] | (pLlCtrlPkt[10] << 8);
        u16 timeout          = pLlCtrlPkt[11] | (pLlCtrlPkt[12] << 8);

        /*
             * This event(HCI_LE_Subrate_Change) shall be issued
             * 1. if the HCI_LE_Subrate_Request command was issued by the Host
             * 2. the parameters are updated successfully following a request from the peer device
             * 3. If no parameters are updated following a request from the peer device or the parameters were changed using the Connection
             *    Update procedure, then this event shall not be issued
             */
        if (pAclConn->subrate_flag.bit.subrate_evt_trige) {
            pAclConn->subrate_flag.bit.subrate_evt_trige = 0;

            pAclConn->subrate_flag.bit.subrate_update_evt = 1;
            blmsParam.subrateUpdtEvt_mask |= (1 << pAclConn->acl_conIndex);
        }


        if ((factor == 0) || (factor > 500) || (continue_num >= factor) || (latency > 0x1F3) || (timeout < 0x0a) || (timeout > 0xc80))
        //||(pAclConn->conn_intvl_n_1m25 * 1250 * factor * (latency+1) * 2 > timeout*10000)
        {
            pAclConn->subrate_rej_reason = HCI_ERR_UNACCEPTABLE_CONN_PARAMETERS;
            //              blt_llms_rejectInd(pAclConn->acl_conHandle, LL_SUBRATE_IND, HCI_ERR_UNACCEPTABLE_CONN_PARAMETERS, 1);
        } else {
            if ((pAclConn->factor != factor) || (pAclConn->conti_num != continue_num) ||
                (pAclConn->per_latency != latency) || (pAclConn->conn_timeout != timeout * 10000 * SYSTEM_TIMER_TICK_1US)) { //conn para change

                pAclConn->subrate_flag.bit.subrate_update_flag = 1;
                pAclConn->subrate_flag.bit.subrate_update_evt  = 1;

                blmsParam.subrateUpdtEvt_mask |= (1 << pAclConn->acl_conIndex);
            } else if (pAclConn->subrateBaseEvent != subrateBaseEvent % factor) {
                pAclConn->subrate_flag.bit.subrate_update_flag = 1;
            }

            if (pAclConn->subrate_flag.bit.subrate_update_flag) {
                pAclConn->factor    = factor;
                pAclConn->conti_num = continue_num;

                pAclConn->lastSubEventCnt  = subrateBaseEvent;
                pAclConn->subrateBaseEvent = subrateBaseEvent % factor;

                // Used to determine whether a wrap occurred
                if ((pAclConn->lastSubEventCnt >> 14) == 0x03) {
                    pAclConn->subrate_flag.bit.subrate_wrap_flag = 1;
                } else {
                    pAclConn->subrate_flag.bit.subrate_wrap_flag = 0;
                }


                pAclConn->per_latency  = latency;
                pAclConn->conn_timeout = timeout * 10000 * SYSTEM_TIMER_TICK_1US;


                // for /LL/CON/PER/BV-141-C  on ACL connection subrate factor = 2, base = 0, continue = 0;
                //receiving LL_Subrate_IND on EC = 4 with factor = 6, base = 0, continue = 3,   In blt_ll_subrate_insertContiTask() should not
                // insert the continue task
                pAclConn->subrate_flag.bit.subrate_evt_flag   = 0;
                pAclConn->subrate_flag.bit.validDataRxTx_flag = 0;
                pAclConn->insertTsk                           = 0;


                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SUBRATE_STATE_CHANGE);
                my_dump_str_u32s(DBG_SUBRATE_EN, "LL_SUBRATE_IND", pAclConn->factor, pAclConn->conti_num, pAclConn->subrateBaseEvent, pAclConn->conn_timeout);
            }
        }

    } else if (opcode == LL_REJECT_IND_EXT) {
        //      rf_packet_ll_reject_ext_ind_t* pRejectExtInd = (rf_packet_ll_reject_ext_ind_t*)pLlCtrlPkt;
    }

    return BLE_SUCCESS;
}

_attribute_ram_code_ int blt_subrate_interrupt_task(int flag, void *p)
{
    if (flag & FLAG_ACL_SUBRATE_INSERT_CONTI_TASK) {
        blt_ll_subrate_insertContiTask((st_ll_conn_t *)p);
    } else if (flag & FLAG_ACL_SUBRATE_CONN_CB) {
        st_ll_conn_t *pAcl = (st_ll_conn_t *)p;
        blt_ll_initSubrateByHandle(pAcl->acl_conHandle);
    } else if (flag & FLAG_ACL_SUBRATE_RESET) {
        st_ll_conn_t *pAcl = (st_ll_conn_t *)p;

        blt_ll_resetSubrateByHandle(pAcl->acl_conHandle);
    }
    return 0;
}
    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    int
    blt_subrate_mainloop_task(int flag, void *p)
{
    //int conn_idx = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if (flag == FLAG_MODULE_RESET) {
        blt_ll_resetSubrate();
    } else if (flag == FLAG_MODULE_MAINLOOP) {
        blt_ll_subrate_mainloop(p);
    }


    return 0;
}

//Hci Reset callBack
void blt_ll_resetSubrate(void)
{
    bltSubrateDft.factor_min      = 1;
    bltSubrateDft.factor_max      = 1;
    bltSubrateDft.max_latency     = 0;
    bltSubrateDft.conti_num       = 0;
    bltSubrateDft.subrate_timeout = 3200; //32s

    for (int conn_idx = ACL_CONN_IDX_CEN0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        blt_ll_resetSubrateByHandle(conn_idx);
    }
}

//acl terminate callBack
_attribute_ram_code_ void blt_ll_resetSubrateByHandle(u16 handle)
{
    st_ll_conn_t    *pAcl     = (st_ll_conn_t *)blt_ll_getAclConnPtr(handle);
    subrate_param_t *pSubrate = &pAcl->subrateParam_req;

    pSubrate->valid    = 0;
    pAcl->noDataEvtCnt = 0;
    pAcl->per_latency  = 0;
    pAcl->conti_num    = 0;
    pAcl->factor       = 1;
    pAcl->insertTsk    = 0;

    pAcl->subrate_dft_flag      = 0;
    pAcl->subrate_flag.flagBits = 0;
    pAcl->subrate_rej_reason    = 0;
}

//connection CallBack
_attribute_ram_code_
    ble_sts_t
    blt_ll_initSubrateByHandle(u16 handle)
{
    st_ll_conn_t *pAcl = (st_ll_conn_t *)blt_ll_getAclConnPtr(handle);

    if (((pAcl->aclRole == ACL_ROLE_CENTRAL) && (pAcl->subrate_dft_flag))) {
        subrate_param_t *pSubrate = &pAcl->subrateParam_req;

        pSubrate->factor_min      = bltSubrateDft.factor_min;
        pSubrate->factor_max      = bltSubrateDft.factor_max;
        pSubrate->max_latency     = bltSubrateDft.max_latency;
        pSubrate->conti_num       = bltSubrateDft.conti_num;
        pSubrate->subrate_timeout = bltSubrateDft.subrate_timeout; // 32s
        pSubrate->valid           = 1;
    }


    pAcl->per_latency    = 0;
    pAcl->conti_num      = 0;
    pAcl->factor         = 1;
    pAcl->insertTsk      = 0;
    pAcl->factor_next    = 1;
    pAcl->conti_num_next = 0;
    pAcl->noDataEvtCnt   = 0;

    pAcl->subrate_flag.flagBits = 0;

    pAcl->subrate_rej_reason = 0;

    return BLE_SUCCESS;
}

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    int
    blt_ll_subrate_mainloop(void *p)
{
    for (int conn_idx = ACL_CONN_IDX_CEN0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        st_ll_conn_t *pc  = (st_ll_conn_t *)&blms[conn_idx];
        ble_sts_t     ret = BLE_SUCCESS;

        if (conn_idx < LL_MAX_ACL_CEN_NUM) { //master
            if (pc->subrate_flag.bit.subrate_req_pending) {
                if (pc->llcp_flag.bit.ll_feat_exg_flag) {
                    if (pc->ll_remoteFeature1 & LL_FEATURE_MASK_CONNECTION_SUBRATING_HOST) {
                        //if not do the procedure of feature exchange should not init this feature
                        blt_ll_sendSubrateInd(pc->acl_conHandle);
                    } else {
                        pc->subrate_flag.bit.subrate_update_evt  = 1;
                        pc->subrate_flag.bit.subrate_req_pending = 0;
                        ret                                      = HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
                    }
                } else if (!pc->remoteFeatureReq) {
                    blt_ll_send_feature_req(pc);
                }
            }

            if (pc->subrate_rej_reason) {
                u8 suppExtReject = 0;
                if (pc->ll_remoteFeature0 & LL_FEATURE_MASK_EXTENDED_REJECT_INDICATION) {
                    suppExtReject = 1;
                }
                if (blt_llms_rejectInd(pc->acl_conHandle, LL_SUBRATE_IND, pc->subrate_rej_reason, suppExtReject)) {
                    pc->subrate_rej_reason = 0;
                }
            }
            //

        } else { //slave

            /*
             * The Peripheral shall accept an LL_SUBRATE_IND PDU. However, if the
             *  Peripheral's Host would prefer a different subrate factor it may, after this
             *  procedure has completed, initiate the Connection Subrate Request procedure
             *  or the Connection Parameters Request procedure to change the connection
             *  parameters  && !pc->subrate_flag.bit.subrate_update_flag
             */
            if (pc->subrate_flag.bit.subrate_req_pending) {
                blt_ll_sendSubrateReq(pc->acl_conHandle);
            }
        }

        if (pc->subrate_flag.bit.subrate_update_evt) {
            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_SUBRATE_CHANGE) {
                hci_le_subrateChangeEvt_t subrateChg_evt;

                subrateChg_evt.subEventCode       = HCI_SUB_EVT_LE_SUBRATE_CHANGE;
                subrateChg_evt.status             = ret;
                subrateChg_evt.connHandle         = pc->acl_conHandle;
                subrateChg_evt.subrate_factor     = pc->factor;
                subrateChg_evt.peripheral_latency = pc->per_latency;
                subrateChg_evt.conti_num          = pc->conti_num;
                subrateChg_evt.subrate_timeout    = pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS);

                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, (u8 *)(&subrateChg_evt), 12);
            }

            pc->subrate_flag.bit.subrate_update_evt = 0;
            blmsParam.subrateUpdtEvt_mask &= ~(1 << conn_idx);
        }
    }

    return 0;
}

_attribute_noinline_ void blt_ll_sendSubrateReq(u16 connHandle)
{
    st_ll_conn_t *pAcl = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);

    if (!blt_ll_isEncryptionBusy(connHandle)) {
        rf_pkt_ll_subrate_req_t subrate_req;
        subrate_req.llid              = LLID_CONTROL;
        subrate_req.rf_len            = 11;
        subrate_req.opcode            = LL_SUBRATE_REQ;
        subrate_req.subrateFactor_min = pAcl->subrateParam_req.factor_min;
        subrate_req.subrateFactor_max = pAcl->subrateParam_req.factor_max;
        subrate_req.max_latency       = pAcl->subrateParam_req.max_latency;
        subrate_req.continue_num      = pAcl->subrateParam_req.conti_num;
        subrate_req.timeout           = pAcl->subrateParam_req.subrate_timeout;

        my_dump_str_data(DBG_SUBRATE_EN, "send ll subrate req", 0, 0);

        if (blt_llmsPushLlCtrlPkt(connHandle, LL_SUBRATE_REQ, (u8 *)&subrate_req)) {
            pAcl->subrate_flag.bit.subrate_req_pending = 0;
        }
    }
}

_attribute_noinline_ bool blt_ll_sendSubrateInd(u16 connHandle)
{
    bool          ret  = FALSE;
    st_ll_conn_t *pAcl = (st_ll_conn_t *)blt_ll_getAclConnPtr(connHandle);


    if (!blt_ll_isEncryptionBusy(connHandle) && !(pAcl->conn_update_union.update_mark & (CONN_UPDATE_CMD | CONN_UPDATE_PENDING | CONN_UPDATE_NEARBY))) //&& (blt_ll_getRealTxFifoNumber(connHandle)==0)
    {
        u16 base    = pAcl->conn_inst - 1;
        u8  dat[13] = {3, 11, LL_SUBRATE_IND};
        dat[3]      = pAcl->factor_next & 0xff;
        dat[4]      = (pAcl->factor_next >> 8) & 0xff;

        //todo S15-14 = E15-14
        dat[5] = base & 0xff;        //pAcl->conn_inst&0xff;
        dat[6] = (base >> 8) & 0xff; //(pAcl->conn_inst>>8)&0xff;

        dat[7] = pAcl->per_latency_next & 0xff;
        dat[8] = (pAcl->per_latency_next >> 8) & 0xff;

        dat[9]  = pAcl->conti_num_next & 0xff;
        dat[10] = (pAcl->conti_num_next >> 8) & 0xff;

        dat[11] = pAcl->subrate_timeout_next & 0xff;
        dat[12] = (pAcl->subrate_timeout_next >> 8) & 0xff;

        pAcl->subrateBaseEvent_next = base % pAcl->factor_next;
        pAcl->lastSubEventCnt       = base;


        if (blt_llmsPushLlCtrlPkt(connHandle, LL_SUBRATE_IND, dat) == TRUE) {
            pAcl->connMarkTxFifoWptr                   = pAcl->tx_wptr;
            pAcl->subrate_flag.bit.subrate_trans_mode  = 1;
            pAcl->subrate_flag.bit.subrate_req_pending = 0;


            if ((pAcl->lastSubEventCnt >> 14) == 0x03) {
                pAcl->subrate_flag.bit.subrate_wrap_flag = 1;
            } else {
                pAcl->subrate_flag.bit.subrate_wrap_flag = 0;
            }


            if ((pAcl->factor != pAcl->factor_next) || (pAcl->conti_num != pAcl->conti_num_next) ||
                (pAcl->per_latency != pAcl->per_latency_next) || (pAcl->conn_timeout != pAcl->subrate_timeout_next * 10000 * SYSTEM_TIMER_TICK_1US)) { //conn para change

                pAcl->subrate_flag.bit.subrate_para_change_flag = 1;
            } else {
                pAcl->subrate_flag.bit.subrate_para_change_flag = 0;
            }

            pAcl->subrate_flag.bit.subrate_update_flag = 0;

            my_dump_str_u32s(DBG_SUBRATE_EN, "send2 subrate_ind", pAcl->conn_inst - 1, pAcl->connMarkTxFifoWptr, pAcl->tx_rptr, pAcl->tx_wptr);
            ret = TRUE;
        }
    }

    return ret;
}

bool blt_hci_checkSubrateParams(u16 factor_min, u16 factor_max, u16 max_latency, u16 continue_num, u16 subrate_timeout)
{
    if ((factor_min == 0) || (factor_min > factor_max) || (factor_min > 500) || (factor_max > 500) || (max_latency > 0x1F3) || (continue_num >= factor_max) || (subrate_timeout < 0x0A) || (subrate_timeout > 0x0c80) || (factor_max * (max_latency + 1) > 500)) {
        return FALSE;
    }
    return TRUE;
}

ble_sts_t blc_hci_le_set_default_subrate(hci_le_setDefaultSubrateCmdParams_t *pCmdPara)
{
    if (!blt_hci_checkSubrateParams(pCmdPara->factor_min, pCmdPara->factor_max, pCmdPara->max_latency, pCmdPara->conti_num, pCmdPara->timeout)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    bltSubrateDft.factor_min      = pCmdPara->factor_min;
    bltSubrateDft.factor_max      = pCmdPara->factor_max;
    bltSubrateDft.max_latency     = pCmdPara->max_latency;
    bltSubrateDft.conti_num       = pCmdPara->conti_num;
    bltSubrateDft.subrate_timeout = pCmdPara->timeout;

    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CEN_NUM; conn_idx++) {
        st_ll_conn_t *pAcl = (st_ll_conn_t *)&blms[conn_idx];
        if (pAcl->connState != CONN_STATUS_ESTABLISH) {
            pAcl->subrate_dft_flag = 1;
        }
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_subrate_request(hci_le_subrateRequestCmdParams_t *pCmdPara)
{
    if (!blt_hci_checkSubrateParams(pCmdPara->factor_min, pCmdPara->factor_max, pCmdPara->max_latency, pCmdPara->conti_num, pCmdPara->timeout)) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    if (blt_ll_isAclhdlInvalid(pCmdPara->connection_handle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    /*
     * If the Central's Host issues this command when the Connection Subrating
        (Host Support) bit is not set in the Peripheral's FeatureSet, the Controller shall
        return the error code Unsupported Remote Feature (0x1A)
     */
    if (!(LL_FEATURE_MASK_1 & LL_FEATURE_MASK_CONNECTION_SUBRATING_HOST)) {
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    st_ll_conn_t *pAcl = (st_ll_conn_t *)blt_ll_getAclConnPtr(pCmdPara->connection_handle);
    /*
 * The Supervision_Timeout parameter specifies the link supervision timeout for
 *  the connection. The Supervision_Timeout, in milliseconds, shall be greater
 *  than 2 x current connection interval x Subrate_Max x (Max_Latency + 1).
 */
    if (pAcl->conn_intvl_n_1m25 * 1250 * pCmdPara->factor_max * (pCmdPara->max_latency + 1) * 2 > pCmdPara->timeout * 10000) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    /*
     * The Central shall not initiate this
        procedure while a Connection Parameters Request procedure is in progress.
        The Central shall not initiate this procedure until it has performed a Feature
        Exchange procedure (see Section 5.1.4) to determine that the Connection
        Subrating (Host Support) bit is set in the Peripheral's FeatureSet
     */
    //todo
    if (pCmdPara->connection_handle & BLM_CONN_HANDLE) { //central
    }

    pAcl->subrate_flag.bit.subrate_req_pending = 1;
    pAcl->subrate_flag.bit.subrate_evt_trige   = 1;


    subrate_param_t *pSubrate = &pAcl->subrateParam_req;

    pSubrate->factor_min = pCmdPara->factor_min;
    pSubrate->factor_max = pCmdPara->factor_max;
    //The Peripheral latency shall be less than or equal to Max_Latency
    pSubrate->max_latency = pCmdPara->max_latency;
    /*The continuation number shall be equal to the lesser of Continuation_-
    Number and (subrate factor - 1)*/
    pSubrate->conti_num       = min(pCmdPara->conti_num, pSubrate->factor_max - 1);
    pSubrate->subrate_timeout = pCmdPara->timeout; //unit 10ms
    pSubrate->valid           = 1;


    if (pCmdPara->connection_handle & BLM_CONN_HANDLE) { //central

        pAcl->factor_next          = (pAcl->subrateParam_req.factor_min + pAcl->subrateParam_req.factor_max) / 2;
        pAcl->per_latency_next     = pAcl->subrateParam_req.max_latency;
        pAcl->conti_num_next       = min(pAcl->subrateParam_req.conti_num, pAcl->factor_next - 1);
        pAcl->subrate_timeout_next = pAcl->subrateParam_req.subrate_timeout;
    }

    return BLE_SUCCESS;
}

_attribute_ram_code_
    u16
    blt_ll_subrate_calNextEvent(st_ll_conn_t *pc, u16 e, u16 factor, u16 *pBase)
{
    u16 k;
    u32 s;
    u16 base = *pBase;

    my_dump_str_u32s(DBG_SUBRATE_EN, "cal event", pc->lastSubEventCnt, e, factor, base);

    //last EventCnt E15-14 = 0b11, current E15-14 = 0b00, then event wrap must have been happen
    if ((pc->subrate_flag.bit.subrate_wrap_flag) && (((e >> 14) & 0x03) == 0)) {
        base   = factor - (65536 - base) % factor;
        *pBase = base;

        pc->subrate_flag.bit.subrate_wrap_flag = 0;

        my_dump_str_data(DBG_SUBRATE_EN & 0, "subrate wrap", pBase, 2);
    }


    if (e >= base) {
        k = (e - base) / factor;
        s = (u32)(base + (k + 1) * factor);

        if (s >= 65536) {
            //k = ((65536-base) + factor - 1)/factor;
            //s = base + k*factor - 65536;
            s -= 65536;
            *pBase = s;

            pc->subrate_flag.bit.subrate_wrap_flag = 0;
        }
    } else {
        s = base;
    }

    my_dump_str_u32s(DBG_SUBRATE_EN, "cal", blt_debug_hex_2_dec_display(s), e, factor, *pBase);
    return s;
}

// Just get BaseEvent + k*subrate exclude continuation
_attribute_ram_code_
    u32
    blt_ll_subrate_getNextEvent(st_ll_conn_t *pAclConn, u16 start_inst)
{
    u16 nextEvent    = 0;
    u8  recover_flag = 0;
    u8  isSubEvtFlag = 0;
    u32 inst;

    start_inst = start_inst & 0xffff;
    //&& (!pAclConn->subrate_flag.bit.subrate_update_flag)
    if ((((pAclConn->factor == 1)) || ((pAclConn->subrate_flag.bit.subrate_trans_mode) && (pAclConn->conti_num_next != 0)))) {
        nextEvent = ++start_inst;

        if ((pAclConn->subrate_flag.bit.subrate_trans_mode) && (pAclConn->subrate_flag.bit.subrate_wrap_flag) && (((nextEvent >> 14) & 0x03) == 0)) {
            pAclConn->subrateBaseEvent = pAclConn->factor - (65536 - pAclConn->subrateBaseEvent) % pAclConn->factor;
        }

    } else {                                  //(pAclConn->factor>1)

        s16 insertTask = pAclConn->insertTsk; //backup insertTas;
        my_dump_str_u32s(DBG_SUBRATE_EN & 0, "GetNextSub Input", start_inst, pAclConn->insertTsk, pAclConn->factor, pAclConn->subrate_flag.bit.conn_update_flag);


        if ((pAclConn->insertTsk > 0) && (!pAclConn->subrate_flag.bit.subrate_trans_mode)) {
            ////the Central shall retransmit the PDU on all connection events which are subrated connection events based on the old subrate base event
            nextEvent = ++start_inst;
            pAclConn->insertTsk--;
        } else {
            isSubEvtFlag = 1;
            nextEvent    = blt_ll_subrate_calNextEvent(pAclConn, start_inst, pAclConn->factor, &pAclConn->subrateBaseEvent);

            my_dump_str_u32s(DBG_SUBRATE_EN & 0, "N", nextEvent, pAclConn->factor, pAclConn->subrateBaseEvent, start_inst);
        }


        //core5.3, P2875
        if (pAclConn->subrate_flag.bit.subrate_trans_mode) {
            u16 nextEvent_new;
            nextEvent_new = blt_ll_subrate_calNextEvent(pAclConn, start_inst, pAclConn->factor_next, &pAclConn->subrateBaseEvent_next);

            my_dump_str_u32s(DBG_SUBRATE_EN, "new", start_inst, nextEvent_new, nextEvent, 0);

            if ((s16)(nextEvent - nextEvent_new) > 0) {
                isSubEvtFlag = 1;
                recover_flag = 1;
                nextEvent    = nextEvent_new;
            }
        }


        // if conn_para_update_ind is pending, then the instant between the subrate event should listen
        // for example, if instant=100, connect event 100 will wake up , and connect parament will update after EC=99;
        if ((pAclConn->subrate_flag.bit.conn_update_flag) && (((s16)(nextEvent - (pAclConn->conn_para_inst_next - pAclConn->subrate_flag.bit.conn_update_flag + 1))) > 0)) {
            my_dump_str_u32s(DBG_SUBRATE_EN, "GetNextSub1", nextEvent, pAclConn->subrate_flag.bit.conn_update_flag, pAclConn->conn_para_inst_next, start_inst);
            if ((s16)((pAclConn->conn_para_inst_next - pAclConn->subrate_flag.bit.conn_update_flag + 1) - start_inst) > 0) {
                recover_flag = 1;
                nextEvent    = pAclConn->conn_para_inst_next - pAclConn->subrate_flag.bit.conn_update_flag + 1;

                isSubEvtFlag = 0;
            }
        }

        if (recover_flag) {
            pAclConn->insertTsk = insertTask; //restore insert task
        }

        my_dump_str_u32s(DBG_SUBRATE_EN & 0, "GetNextSub2", nextEvent, pAclConn->insertTsk, pAclConn->acl_conHandle, pAclConn->subrate_flag.bit.conn_update_flag);
    }

    inst = (u32)nextEvent;

    if (isSubEvtFlag) {
        inst |= BIT(31);
    }

    my_dump_str_u32s(DBG_SUBRATE_EN & 0, "ret", inst, isSubEvtFlag, nextEvent, pAclConn->subrate_flag.bit.conn_update_flag);
    return inst;
}

_attribute_ram_code_

    int
    blt_ll_subrate_insertContiTask(st_ll_conn_t *pAcl)
{
    my_dump_str_u32s(DBG_SUBRATE_EN, "insertConti", pAcl->conn_inst - 1, pAcl->subrate_flag.flagBits, pAcl->subrate_flag.bit.conn_update_flag, pAcl->insertTsk);
    if (pAcl->subrate_flag.bit.subrate_evt_flag) {            // subrate event conn_inst = baseEvent + k*subrate

        if (pAcl->subrate_flag.bit.validDataRxTx_flag != 0) { //subrateEvent have valid data

            pAcl->insertTsk        = pAcl->conti_num;
            pAcl->subrateEvtRemain = pAcl->factor - 1;
            pAcl->noDataEvtStart   = pAcl->conn_inst;
        } else { // not need to insert continuation event
            pAcl->insertTsk = 0;
        }

        pAcl->noDataEvtCnt = 0;
    } else {
        //
        pAcl->subrateEvtRemain -= (pAcl->inter_jump_num + 1);

        if (pAcl->subrate_flag.bit.validDataRxTx_flag != 0) {
            pAcl->noDataEvtCnt   = 0;
            pAcl->noDataEvtStart = pAcl->conn_inst;
            pAcl->insertTsk      = pAcl->conti_num;

        } else {
            pAcl->noDataEvtCnt += pAcl->inter_jump_num + 1;

            if (pAcl->insertTsk > 0) { // if last rebuilt not insert all the task, then try to rebuilt all the remain taskList,
                pAcl->insertTsk = pAcl->conti_num - pAcl->noDataEvtCnt;
            }
        }

        if (pAcl->insertTsk > pAcl->subrateEvtRemain) {
            pAcl->insertTsk = pAcl->subrateEvtRemain;
        }
    }

    if (pAcl->insertTsk > 0) {
        blt_sche_addUpdate(SLOT_UPDT_SLAVE_SUBRATE_STATE_CHANGE); //rebuilt schedule task map
        my_dump_str_u32s(DBG_SUBRATE_EN, "insert", pAcl->insertTsk, pAcl->conn_inst - 1, pAcl->insertTsk, pAcl->noDataEvtStart);
    }

    return pAcl->insertTsk;
}


#endif
