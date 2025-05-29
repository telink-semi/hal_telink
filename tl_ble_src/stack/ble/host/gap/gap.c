/********************************************************************************************************
 * @file    gap.c
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
#include "stack/ble/ble.h"
#include "stack/ble/host/gatt/tlk_list_stack.h"
#include "stack/ble/host/gatt/tlk_timer_stack.h"

#if OS_SUP_EN
    #include "stack/ble/os_sup/os_sup.h"
    #include "stack/ble/os_sup/os_sup_stack.h"
#endif

_attribute_aligned_(4) _attribute_ble_data_retention_ gap_ms_para_t gap_ms_para[LL_MAX_ACL_CONN_NUM];
_attribute_aligned_(4) _attribute_ble_data_retention_ gap_s_para_t gap_s_para[LL_MAX_ACL_PER_NUM];

//connHandle -> slave device_index (master device index do not care)
_attribute_ble_data_retention_ u8 local_dev_index[LL_MAX_ACL_CONN_NUM] = {0};

_attribute_ble_data_retention_ host_ota_main_loop_callback_t host_ota_main_loop_cb = NULL;
_attribute_ble_data_retention_ host_ota_terminate_callback_t host_ota_terminate_cb = NULL;

_attribute_ble_data_retention_ coc_main_loop_callback_t coc_main_loop_cb = NULL;

_attribute_ble_data_retention_
    SLIST_DEF(gapAclStateList);

_attribute_ble_data_retention_
    SLIST_DEF(gapHciEventList);

#ifdef MCU_CORE_D25F_ENABLE
_attribute_ble_data_retention_ u32 acl_host_run_mask = 0;
#endif

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    gap_ms_para_t *
    blc_gap_getMasterSlavePara(u16 connHandle)
{
    u8 conn_idx = (connHandle & CONN_IDX_MASK);

    if (conn_idx >= LL_MAX_ACL_CONN_NUM) {
        return NULL;
    } else {
        return &gap_ms_para[conn_idx];
    }
}

#if DUAL_CORE_MODE_ENABLED
static uint16_t dual_mode_mtu_size = 23;
#endif

u16 blt_gap_getInitMtu(u16 connHandle)
{
#if DUAL_CORE_MODE_ENABLED
    dual_mode_mtu_size = 65;
    return dual_mode_mtu_size;
#endif
    return connHandle & BLM_CONN_HANDLE ? l2cap_buff_m.init_MTU : l2cap_buff_s.init_MTU;
}

u16 blt_gap_recvRemoteMtu(u16 connHandle, u16 remoteMtu)
{
    gap_ms_para_t *pGap_ms_para = blt_gap_getServerPara(connHandle);
    u16            initMtu      = blt_gap_getInitMtu(connHandle);

    remoteMtu                   = max(ATT_MTU_SIZE, remoteMtu);
    pGap_ms_para->effective_MTU = min(initMtu, remoteMtu);

    if (pGap_ms_para->mtu_exg_pending) {
        pGap_ms_para->mtu_exg_pending = 0;
    }

    return pGap_ms_para->effective_MTU;
}

_attribute_no_inline_
    u16
    blt_gap_getEffectiveMTU(u16 connHandle)
{
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if (pGap_ms_para != NULL) {
        return pGap_ms_para->effective_MTU;
    }
    return 0;
}

u16 blt_gap_getScidMtu(u16 connHandle, u8 scid)
{
#if DUAL_CORE_MODE_ENABLED
    return dual_mode_mtu_size;
#endif

    if (scid == L2CAP_CID_ATTR_PROTOCOL) { //L2CAP_CID_ATTR_PROTOCOL
        return blt_gap_getEffectiveMTU(connHandle);
    }

#if (L2CAP_SERVER_FEATURE_SUPPORTED_EATT)
    l2cap_coc_cid_t *pCid     = blt_l2cap_cocGetSrcCID(connHandle, scid);
    u16              localMtu = blt_l2cap_cocGetRecvMtu();
    return min(pCid->mtu, localMtu);
#endif

    return 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
    gap_s_para_t *
    bls_gap_getSlavePara(u16 connHandle)
{
    u8 h = connHandle & CONN_IDX_MASK;

    if (h < LL_MAX_ACL_CEN_NUM || h >= LL_MAX_ACL_CONN_NUM) {
        return NULL;
    } else {
        u8 conn_slave_idx = (connHandle & CONN_IDX_MASK) - LL_MAX_ACL_CEN_NUM;

        return &gap_s_para[conn_slave_idx];
    }
}

u8 blc_gap_setSingleServerDataPendingTime_upon_ClientCmd(u16 connHandle, u16 num_10ms)
{
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);
    if (pGap_ms_para != NULL) {
        pGap_ms_para->data_pending_time = num_10ms;
        return TRUE;
    }
    return FALSE;
}

void blc_att_setServerDataPendingTime_upon_ClientCmd(u16 num_10ms)
{
    for (int i = 0; i < LL_MAX_ACL_CONN_NUM; i++) {
        gap_ms_para[i].data_pending_time = num_10ms; //10ms unit
    }
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
    u8
    blt_gap_getSlaveDeviceIndex_by_connIdx(u8 conn_idx)
{
#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
    if (mlDevMng.mldev_en) {
        return local_dev_index[conn_idx];
    } else
#endif
    {
        (void)conn_idx;
        return DEFAULT_DEVICE_INDEX; //only slave device 0 exist
    }
}

void blc_gap_mtuAutoExgDisable(u16 connHandle)
{
    gap_ms_para_t *pGap_ms_para   = blc_gap_getMasterSlavePara(connHandle);
    pGap_ms_para->mtu_exg_pending = 0;
}

void blt_gap_regAclConnState(struct gap_stateChangeNode *node)
{
    SLIST_INSERT_NODE_HEAD(&gapAclStateList, node);
}

void blt_gap_unregAclConnState(struct gap_stateChangeNode *node)
{
    SLIST_DELETE_NODE(&gapAclStateList, node);
}

int blt_gap_conn_complete_handler(u16 connHandle, u8 *p)
{
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    /* must update device index before SMP func_smp_init */
    u8 conn_idx = (connHandle & CONN_IDX_MASK);
#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
    if (mlDevMng.mldev_en) {
        if (connHandle & BLS_CONN_HANDLE) {
            //local_dev_index[conn_idx] = mlDevMng.cur_dev_idx;
            local_dev_index[conn_idx] = p[14]; //refer "conn_req_info[14]"
        } else {                               //master now do not support multiple device
            local_dev_index[conn_idx] = DEFAULT_DEVICE_INDEX;
        }
    } else
#endif
    {
        local_dev_index[conn_idx] = DEFAULT_DEVICE_INDEX;
    }

#ifdef MCU_CORE_D25F_ENABLE
    acl_host_run_mask |= BIT(conn_idx);
#endif

    if (func_smp_init) {              // blt_smp_setAddress
        func_smp_init(connHandle, p); // blt_smp_setAddress
    }

    //Variables are stored in the structure 'blhAclms' to facilitate code processing.
    pl2cap_buff = (connHandle & BLM_CONN_HANDLE) ? &l2cap_buff_m : &l2cap_buff_s;
    if (pl2cap_buff->init_MTU != ATT_MTU_SIZE) {
        /* use old ATT process, if use new ATT design, we use new process */
        if (blc_hci_data_handler == blc_l2cap_pktHandler) {
            pGap_ms_para->mtu_exg_pending = 1;
        } else {
            pGap_ms_para->mtu_exg_pending = 2;
        }
    }

    struct single_list_node *cur;
    struct single_list_node *nextNode;
    SLIST_FOREACH_SAFE(cur, &gapAclStateList, next, nextNode)
    {
        struct gap_stateChangeNode *node = (struct gap_stateChangeNode *)cur;
        node->cb(connHandle, GAP_STATE_ACL_CONNECTED, cur);
    }

    return 0;
}

int blt_gap_conn_terminate_handler(u16 connHandle, u8 *p)
{
    (void)p;
    u8             is_master    = (connHandle & BLM_CONN_HANDLE);
    u8             conn_idx     = connHandle & CONN_IDX_MASK;
    gap_ms_para_t *pGap_ms_para = blc_gap_getMasterSlavePara(connHandle);

    host_acl_ms_t *pHostAclms = (host_acl_ms_t *)&blhAclms[conn_idx];
    //  if(pHostAclms->l2cap_connParaUpReq_pending)
    {
        pHostAclms->l2cap_connParaUpReq_pending = 0;
    }

    /*
 * bug: should not clear here
 *  bug trigger condition:
 *  1. link lay receive data and hold in fifo
 *  2. terminate event occur and call this function clear flag pPendingPkt(haven't set)
 *  3. in main_loop find FIFO have data and process it using  blt_l2cap_pushData_2_controller send response, but here
 *      can't send successfully,reason of invalid ll state(if(pc->connState == 0)),then set flag pPendingPkt
 *  4. at this condition,flag of pPendingPkt can't clear forever
 *
 *  solution:
 *  clear this flag in  blt_process_pendingPkt()
 */
    if (pGap_ms_para != NULL) {
#if 1
    #if DUAL_CORE_MODE_ENABLED
        dual_mode_mtu_size = 23;
    #endif
        pGap_ms_para->effective_MTU   = ATT_MTU_SIZE;
        pGap_ms_para->indicate_handle = 0;
        pGap_ms_para->pPendingPkt     = NULL;
        pGap_ms_para->mtu_exg_pending = 0;
#else
        pGap_ms_para->indicate_handle = 0;
        //      pGap_ms_para->pPendingPkt = NULL;
#endif
    }

    if (connHandle & BLS_CONN_HANDLE) {
        gap_s_para_t *pGap_s_para = bls_gap_getSlavePara(connHandle);

        if (pGap_s_para != NULL) {
            pGap_s_para->l2cap_connParaUpReq_pending = 0; ///clear
        }
    }


    if (is_master) {                                          //Master

    } else {                                                  //Slave
        if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
            blc_SecReq_ctrl.secReq_pending &= ~BIT(conn_idx); //clear security request pending flag
        }
    }


    if (blc_smp_isPairingBusy(connHandle)) { //same as: if(blms_smpMng[idx].smpMng.pairing_busy){
        blt_smp_procPairingEnd(connHandle, PAIRING_FAIL_REASON_CONN_DISCONNECT);
    }


//Attention: must do it at last of this function
#if (MULTIPLE_LOCAL_DEVICE_ENABLE)
    //local_dev_index[connHandle & CONN_IDX_MASK] = device_index_by_controller;  //TODO
#else
    local_dev_index[conn_idx] = 0;
#endif

#if 0
    ////// OTA clear
    bls_ota_conn_terminate_clear_flag(connHandle);
#endif

#if L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
    if (coc_disconnect_handler) {
        coc_disconnect_handler(connHandle);
    }
#endif

    if (host_ota_terminate_cb) {
        host_ota_terminate_cb(connHandle);
    }

    struct single_list_node *cur;
    struct single_list_node *nextNode;
    SLIST_FOREACH_SAFE(cur, &gapAclStateList, next, nextNode)
    {
        struct gap_stateChangeNode *node = (struct gap_stateChangeNode *)cur;
        node->cb(connHandle, GAP_STATE_ACL_DISCONNECTED, cur);
    }

    return 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    void
    blt_process_pendingPkt(u16 connHandle, gap_ms_para_t *para)
{
    l2cap_pkt_t *l2cap_pkt = para->pPendingPkt;
    if (l2cap_pkt->cid == L2CAP_CID_ATTR_PROTOCOL) {
        ble_sts_t ret = blt_l2cap_pushData_2_controller(connHandle, l2cap_pkt->cid, &l2cap_pkt->payload.att.opcode, 1, (u8 *)l2cap_pkt->payload.att.data, l2cap_pkt->pduLen - 1);
        if ((ret == BLE_SUCCESS) || (ret == LL_ERR_CONNECTION_NOT_ESTABLISH)) {
            para->pPendingPkt = NULL;
        }
    }
}

//TODO: gap central conn_terminate handler  (   blt_att_resetEffectiveMtuSize() )  NOTE: add this in blm_host.c   disconnect callBack


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif

    int
    blt_gap_mainloop(void)
{
    u16 connHandle;

    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        gap_ms_para_t *pGap_ms_para = &gap_ms_para[conn_idx];
        host_acl_ms_t *pHostAclms   = (host_acl_ms_t *)&blhAclms[conn_idx];
        blms_p_sts                  = (smp_st_t *)&smp_sts_param[conn_idx];
        st_ll_conn_t *pAclConn      = (st_ll_conn_t *)&blms[conn_idx];
        if (pAclConn->connState == CONN_STATUS_DISCONNECT) {
            continue;                        //if the connection is not complete, it will not be run later.
        }
        if (conn_idx < LL_MAX_ACL_CEN_NUM) { //Master
            connHandle  = BLM_CONN_HANDLE | conn_idx;
            blms_p_prop = (smp_prop_t *)&blt_smpProp[0];

            if (pHostAclms->l2cap_connParaUpReq_pending) {
                blt_l2cap_processConnParamUpdateReq(connHandle, pHostAclms);

#if OS_SUP_EN
                if (pHostAclms->l2cap_connParaUpReq_pending) {
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
                }
#endif
            }

            /* Master auto trigger:1.bonded device re-connect; 2.the first smp pairing  */
            if (blt_smpTrig.manual_smp_start && (blt_smpTrig.smp_start_pending & BIT(conn_idx))) {
                blt_smp_procCentralPairingRequest(connHandle);
#if OS_SUP_EN
                if (blt_smpTrig.manual_smp_start) {
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
                }
#endif
            }
        } else { //Slave
            connHandle = BLS_CONN_HANDLE | conn_idx;

            gap_s_para_t *pGap_s_para = bls_gap_getSlavePara(connHandle);
#if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)
            if (smpMng.dynamic_cfg_smp) {
                blms_p_prop = (smp_prop_t *)&blt_smpProp[conn_idx - LL_MAX_ACL_CEN_NUM + 1];
            } else
#endif
            {
                u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);
                blms_p_prop      = (smp_prop_t *)&blt_smpProp[slave_dev_idx + 1];
            }

#if 1 //TODO: check slave connection l2cap param_update_req sending
            ///conn_idx-LL_MAX_ACL_CEN_NUM is not good for customer.
            if (pGap_s_para->l2cap_connParaUpReq_pending) {
                blt_UpdateParameter_request(connHandle);
    #if OS_SUP_EN
                if (pGap_s_para->l2cap_connParaUpReq_pending) {
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
                }
    #endif
            }
#endif

            /* Slave auto trigger:1.Trigger the master to actively encrypt the re-connected link(pending xms);
             * 2.Send SecReq to trigger the master to start smp pairing */
            if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
                blt_smp_procSlaveSecurityRequest(connHandle);
#if OS_SUP_EN
                if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
                }
#endif
            }
        }

        if (pGap_ms_para->pPendingPkt) {
            blt_process_pendingPkt(connHandle, pGap_ms_para);
#if OS_SUP_EN
            if (pGap_ms_para->pPendingPkt) {
                //blt_ll_sem_give();
                if (blt_os_semCountIncrement_cb) {
                    blt_os_semCountIncrement_cb();
                }
            }
#endif
        }

        if (pGap_ms_para->mtu_exg_pending) {
            blt_att_procMtuExgPending(connHandle);
#if OS_SUP_EN
            if (pGap_ms_para->mtu_exg_pending) {
                //blt_ll_sem_give();
                if (blt_os_semCountIncrement_cb) {
                    blt_os_semCountIncrement_cb();
                }
            }
#endif
        }

        //SMP pairing process: slave and master share loop
        if (blms_p_prop->security_level != No_Authentication_No_Encryption) {
            if (blms_p_sts->pairing_busy) { // blc_smp_isPairingBusy()
                blt_smp_proc_pairing_loop(connHandle);
            }

            if (blms_p_sts->smp_timeout_start_tick) {
                //If the Security Manager Timer reaches 30 seconds, the procedure shall be
                //considered to have failed, and the local higher layer shall be notified.
                blt_smp_certTimeoutLoopEvt(connHandle);
            }
        }
    }


    /////////////////////////////////
    //smp storage area reaches the alarm line 0
    /* Different process for different MCU: ******************************************/

    if (smp_bond_device_flash_cfg_idx >= SMP_PARAM_CLEAN_INDEX_ALARM_HIGH && !blm_btxbrx_state) {
        blt_smp_procBondingInfoIndexAlarm(); //Eagle can use this func
    }
    /**********************************************************************************/


    if (host_ota_main_loop_cb) {
        host_ota_main_loop_cb(); //blt_ota_server_main_loop
    }

    if (coc_main_loop_cb) {
        coc_main_loop_cb();
    }

    //  struct single_list_node *cur;
    //  struct single_list_node *nextNode;
    //  SLIST_FOREACH_SAFE(cur, &gapAclStateList, next, nextNode) {
    //      struct gap_stateChangeNode* node = (struct gap_stateChangeNode*)cur;
    //      node->cb(connHandle, GAP_STATE_MAINLOOP, cur);
    //  }

    return 0;
}

#ifdef MCU_CORE_D25F_ENABLE
int blt_gap_ll_enc_done_handler(u16 connHandle, u8 status, u8 enc_enable)
#else
static int blt_gap_ll_enc_done_handler(u16 connHandle, u8 status, u8 enc_enable)
#endif
{
    struct single_list_node *cur;
    struct single_list_node *nextNode;
    SLIST_FOREACH_SAFE(cur, &gapAclStateList, next, nextNode)
    {
        struct gap_stateChangeNode *node = (struct gap_stateChangeNode *)cur;
        node->cb(connHandle, GAP_STATE_SECURITY_DONE, cur);
    }
    return blt_smp_llEncryptionDone(connHandle, status, enc_enable);
}

void blt_gap_regHciEventCb(struct gap_hciEventNode *node)
{
    SLIST_INSERT_NODE_HEAD(&gapHciEventList, node);
}

static int blt_gap_ble_hci_rever_handler(u32 h, u8 *p, int n)
{
    struct single_list_node *cur;
    SLIST_FOREACH(cur, &gapHciEventList, next)
    {
        struct gap_hciEventNode *node = (struct gap_hciEventNode *)cur;
        node->cb(h, p, n); //prfHciEventCallBack, blt_prf_hciEventCb
    }
    return 0;
}

void blc_gap_init(void)
{
    soft_timer_initial();

    blt_ll_registerConnectionCompleteHandler(blt_gap_conn_complete_handler);
    blt_ll_registerConnectionTerminateHandler(blt_gap_conn_terminate_handler);
    blt_ll_registerConnectionEncryptionDoneCallback(blt_gap_ll_enc_done_handler);

#if (LL_PAUSE_ENC_FIX_EN)
    blc_ll_registerConnectionEncryptionPauseCallback(blt_smp_llEncryptionPause);
#endif

    blt_gap_registerEventHandler(blt_gap_ble_hci_rever_handler);

    blt_ll_registerHostMainloopCallback(blt_gap_mainloop);

    blt_smp_param_pre_init();  //some configuration value pre_init, must called before any user API
    blt_l2cap_para_pre_init(); //some configuration value pre_init, must called before any user API
}

#ifdef MCU_CORE_D25F_ENABLE
void blc_gap_initial_hci_event(void)
{
    blt_gap_registerEventHandler(blt_gap_ble_hci_rever_handler);
}

#if DUAL_CORE_MODE_ENABLED

void blc_gap_init_v1(void)
{
    soft_timer_initial();

    blt_gap_registerEventHandler(blt_gap_ble_hci_rever_handler);

    extern int blt_gap_mainloop_v1(void);
    blt_ll_registerHostMainloopCallback(blt_gap_mainloop_v1);

    blt_smp_param_pre_init();  //some configuration value pre_init, must called before any user API
    blt_l2cap_para_pre_init(); //some configuration value pre_init, must called before any user API
}

#if (SDK_MLP_CODE_IN_RAM_SPEED_UP_RUN_TIME)
_attribute_ram_code_
#endif
#ifdef MCU_CORE_D25F_ENABLE
int blt_gap_mainloop_v1(void)
{
    u16 connHandle;

    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        if (acl_host_run_mask & BIT(conn_idx)) {
            gap_ms_para_t *pGap_ms_para = &gap_ms_para[conn_idx];
            host_acl_ms_t *pHostAclms = (host_acl_ms_t *) &blhAclms[conn_idx];
            blms_p_sts = (smp_st_t *) &smp_sts_param[conn_idx];

            if (conn_idx < LL_MAX_ACL_CEN_NUM) { //Master
                connHandle = BLM_CONN_HANDLE | conn_idx;
                blms_p_prop = (smp_prop_t *) &blt_smpProp[0];

                if (pHostAclms->l2cap_connParaUpReq_pending) {
                    //TODO:
                }

                /* Master auto trigger:1.bonded device re-connect; 2.the first smp pairing  */
                if (blt_smpTrig.manual_smp_start && (blt_smpTrig.smp_start_pending & BIT(conn_idx))) {
                    blt_smp_procCentralPairingRequest(connHandle);
#if OS_SUP_EN
                    if (blt_smpTrig.manual_smp_start) {
                        //blt_ll_sem_give();
                        if (blt_os_semCountIncrement_cb) {
                            blt_os_semCountIncrement_cb();
                        }
                }
#endif
            }
        } else { //Slave
                connHandle = BLS_CONN_HANDLE | conn_idx;

                gap_s_para_t *pGap_s_para = bls_gap_getSlavePara(connHandle);
#if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)
                if (smpMng.dynamic_cfg_smp) {
                    blms_p_prop = (smp_prop_t *) &blt_smpProp[conn_idx - LL_MAX_ACL_CEN_NUM + 1];
                } else
#endif
                {
                    u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);
                    blms_p_prop = (smp_prop_t *) &blt_smpProp[slave_dev_idx + 1];
                }

                if (pGap_s_para->l2cap_connParaUpReq_pending) {
                    //TODO:
                }

                /* Slave auto trigger:1.Trigger the master to actively encrypt the re-connected link(pending xms);
 * 2.Send SecReq to trigger the master to start smp pairing */
                if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
                    blt_smp_procSlaveSecurityRequest(connHandle);
#if OS_SUP_EN
                    if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
                        //blt_ll_sem_give();
                        if (blt_os_semCountIncrement_cb) {
                            blt_os_semCountIncrement_cb();
                        }
                }
#endif
            }
    }

            if (pGap_ms_para->pPendingPkt) {
                //TODO:
            }

            if (pGap_ms_para->mtu_exg_pending) {
                //TODO:
            }

            //SMP pairing process: slave and master share loop
            if (blms_p_prop->security_level != No_Authentication_No_Encryption) {
                if (blms_p_sts->pairing_busy) { // blc_smp_isPairingBusy()

                    //#include "vendor/common/tlkapi_debug.h"
                    //tlkapi_printf(1, "blms_p_sts->pairing_busy = %d\r\n",blms_p_sts->pairing_busy);
                    blt_smp_proc_pairing_loop(connHandle);
                }

                if (blms_p_sts->smp_timeout_start_tick) {
                    //If the Security Manager Timer reaches 30 seconds, the procedure shall be
                    //considered to have failed, and the local higher layer shall be notified.
                    blt_smp_certTimeoutLoopEvt(connHandle);
                }
            }
}
    }


    /////////////////////////////////
    //smp storage area reaches the alarm line 0
    /* Different process for different MCU: ******************************************/

    if (smp_bond_device_flash_cfg_idx >= SMP_PARAM_CLEAN_INDEX_ALARM_HIGH) {
        blt_smp_procBondingInfoIndexAlarm(); //Eagle can use this func
    }
    /**********************************************************************************/


    if (host_ota_main_loop_cb) {
        host_ota_main_loop_cb(); //blt_ota_server_main_loop
    }

    if (coc_main_loop_cb) {
        coc_main_loop_cb();
    }

    soft_timer_process(MAINLOOP_ENTRY);
    //  struct single_list_node *cur;
    //  struct single_list_node *nextNode;
    //  SLIST_FOREACH_SAFE(cur, &gapAclStateList, next, nextNode) {
    //      struct gap_stateChangeNode* node = (struct gap_stateChangeNode*)cur;
    //      node->cb(connHandle, GAP_STATE_MAINLOOP, cur);
    //  }

    return 0;
}
#else
int blt_gap_mainloop_v1(void)
{
    u16 connHandle;

    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        gap_ms_para_t *pGap_ms_para = &gap_ms_para[conn_idx];
        host_acl_ms_t *pHostAclms   = (host_acl_ms_t *)&blhAclms[conn_idx];
        blms_p_sts                  = (smp_st_t *)&smp_sts_param[conn_idx];
        st_ll_conn_t *pAclConn      = (st_ll_conn_t *)&blms[conn_idx];
        if (pAclConn->connState == CONN_STATUS_DISCONNECT) {
            continue;                        //if the connection is not complete, it will not be run later.
        }

        if (conn_idx < LL_MAX_ACL_CEN_NUM) { //Master
            connHandle  = BLM_CONN_HANDLE | conn_idx;
            blms_p_prop = (smp_prop_t *)&blt_smpProp[0];

            if (pHostAclms->l2cap_connParaUpReq_pending) {
                //TODO:
            }

            /* Master auto trigger:1.bonded device re-connect; 2.the first smp pairing  */
            if (blt_smpTrig.manual_smp_start && (blt_smpTrig.smp_start_pending & BIT(conn_idx))) {
                blt_smp_procCentralPairingRequest(connHandle);
#if OS_SUP_EN
                if (blt_smpTrig.manual_smp_start) {
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
                }
#endif
            }
        } else { //Slave
            connHandle = BLS_CONN_HANDLE | conn_idx;

            gap_s_para_t *pGap_s_para = bls_gap_getSlavePara(connHandle);
#if (SMP_PERIPHERAL_LEVEL_CFG_SEPARATE_EN)
            if (smpMng.dynamic_cfg_smp) {
                blms_p_prop = (smp_prop_t *)&blt_smpProp[conn_idx - LL_MAX_ACL_CEN_NUM + 1];
            } else
#endif
            {
                u8 slave_dev_idx = blt_gap_getSlaveDeviceIndex_by_connIdx(conn_idx);
                blms_p_prop      = (smp_prop_t *)&blt_smpProp[slave_dev_idx + 1];
            }

            if (pGap_s_para->l2cap_connParaUpReq_pending) {
                //TODO:
            }

            /* Slave auto trigger:1.Trigger the master to actively encrypt the re-connected link(pending xms);
* 2.Send SecReq to trigger the master to start smp pairing */
            if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
                blt_smp_procSlaveSecurityRequest(connHandle);
#if OS_SUP_EN
                if (blc_SecReq_ctrl.secReq_pending & BIT(conn_idx)) {
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
                }
#endif
            }
        }

        if (pGap_ms_para->pPendingPkt) {
            //TODO:
        }

        if (pGap_ms_para->mtu_exg_pending) {
            //TODO:
        }

        //SMP pairing process: slave and master share loop
        if (blms_p_prop->security_level != No_Authentication_No_Encryption) {
            if (blms_p_sts->pairing_busy) { // blc_smp_isPairingBusy()
                blt_smp_proc_pairing_loop(connHandle);
            }

            if (blms_p_sts->smp_timeout_start_tick) {
                //If the Security Manager Timer reaches 30 seconds, the procedure shall be
                //considered to have failed, and the local higher layer shall be notified.
                blt_smp_certTimeoutLoopEvt(connHandle);
            }
        }
    }


    /////////////////////////////////
    //smp storage area reaches the alarm line 0
    /* Different process for different MCU: ******************************************/

    if (smp_bond_device_flash_cfg_idx >= SMP_PARAM_CLEAN_INDEX_ALARM_HIGH) {
        blt_smp_procBondingInfoIndexAlarm(); //Eagle can use this func
    }
    /**********************************************************************************/


    if (host_ota_main_loop_cb) {
        host_ota_main_loop_cb(); //blt_ota_server_main_loop
    }

    if (coc_main_loop_cb) {
        coc_main_loop_cb();
    }

    soft_timer_process(MAINLOOP_ENTRY);
    //  struct single_list_node *cur;
    //  struct single_list_node *nextNode;
    //  SLIST_FOREACH_SAFE(cur, &gapAclStateList, next, nextNode) {
    //      struct gap_stateChangeNode* node = (struct gap_stateChangeNode*)cur;
    //      node->cb(connHandle, GAP_STATE_MAINLOOP, cur);
    //  }

    return 0;
}
#endif
#endif
#endif
