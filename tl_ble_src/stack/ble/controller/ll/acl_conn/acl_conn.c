/********************************************************************************************************
 * @file    acl_conn.c
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


#if OS_SUP_EN
    #include "stack/ble/os_sup/os_sup.h"
    #include "stack/ble/os_sup/os_sup_stack.h"
#endif

#define DBG_DECRYPTION_ERR_EN 0


_attribute_ble_data_retention_ _attribute_aligned_(4) st_ll_conn_t AA_blms[LL_MAX_ACL_CONN_NUM] = {};

_attribute_ble_data_retention_ st_ll_conn_t *blms_pconn; //for common connection (salve & master)
_attribute_ble_data_retention_ volatile int  blms_conn_sel = 0;


_attribute_ble_data_retention_ _attribute_aligned_(4) acl_conn_para_t aclConn_param;

#if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
_attribute_ble_data_retention_ _attribute_aligned_(4) acl_conn_etbsh_t aclConn_etbsh;
#endif

_attribute_ble_data_retention_ acl_rx_fifo_t blt_rxfifo;
_attribute_ble_data_retention_ acl_tx_fifo_t blt_m_txfifo; //TODO: can optimize, master data no need retention
_attribute_ble_data_retention_ acl_tx_fifo_t blt_s_txfifo;

_attribute_ble_data_retention_ u8 conn_req_info[16];       //14 for connect information, 2 for other use


_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_slave_mlp_task_cb  = NULL;
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_master_mlp_task_cb = NULL;

extern blt_event_callback_t blt_p_event_callback;

/* special reason for this additional callBack pointer, can not use "ll_cis_conn_irq_task_cb" and "ll_cis_conn_mlp_task_cb"
 * first one is execute in IRQ, ACL main_loop will call it, so can not use
 * second one is in flash, but ACL slave will call it in IRQ when "BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE" enable,
 *                         consider 9518, this code must in SRAM, so can not use */
_attribute_ble_data_retention_ ll_task_callback_2_t ll_cis_map_update_cb = NULL;

#if (LL_FEATURE_ENABLE_PAST)
/*
 *  Callback used by PAST
 */
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_past_irq_task_cb = NULL;
_attribute_ble_data_retention_ ll_task_callback_t   ll_acl_past_mlp_task_cb = NULL;
#endif

#if (LL_FEATURE_ENABLE_POWER_CONTROL)
/*
 *  Callback used by POWER_CONTROL
 */
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_pcl_irq_task_cb;
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_pcl_mlp_task_cb;
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
/*
 *  Callback used by channel classification
 */
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_chnclass_irq_task_cb = NULL;
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_chnclass_mlp_task_cb = NULL;
#endif

/*
 *  Callback used by connection subrate
 */
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_subrate_irq_task_cb = NULL;
_attribute_ble_data_retention_ ll_task_callback_2_t ll_acl_subrate_mlp_task_cb = NULL;

_attribute_ble_data_retention_ ll_subrate_ctrl_handler_t        ll_acl_subrate_ctrl_handler = NULL;
_attribute_ble_data_retention_ ll_subrate_process_req_handler_t ll_acl_subrate_process_req  = NULL;
#endif

/*
#define         SCA_MASTER_SLAVE_251_500_PPM                    0
#define         SCA_MASTER_SLAVE_151_250_PPM                    1
#define         SCA_MASTER_SLAVE_101_150_PPM                    2
#define         SCA_MASTER_SLAVE_76_100_PPM                     3
#define         SCA_MASTER_SLAVE_51_75_PPM                      4
#define         SCA_MASTER_SLAVE_31_50_PPM                      5
#define         SCA_MASTER_SLAVE_21_30_PPM                      6
#define         SCA_MASTER_SLAVE_0_20_PPM                       7
*/
#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
__attribute__((section(".data"))) 
#else
const
#endif
u8    ppmDiv10_tbl[8] = {50, 25, 15, 10, 7, 5, 3, 2};




#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
/* disconnect command will use it, so define here */
_attribute_ble_data_retention_ hci_cmd_callback_t ll_cis_cmd_task_cb = NULL;

    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
_attribute_aligned_(4) ll_cis_ctrl_handler_t ll_cis_master_ctrl_handler = NULL;
    #endif


    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
_attribute_aligned_(4) ll_cis_ctrl_handler_t ll_cis_slave_ctrl_handler = NULL;
    #endif
#endif


#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
_attribute_aligned_(4) ll_ctrl_pdu_handle_t ll_chn_sounding_ctrl_handler = NULL;
#endif

_attribute_ble_data_retention_  volatile u8 ll_acl_Per_AckCentralDisconnectNum = 3;

_attribute_ble_data_retention_ blms_LTK_req_callback_t blt_llms_ltk_request = NULL; // shall init to sm callback

void blt_llms_registerLtkReqEvtCb(blms_LTK_req_callback_t evtCbFunc)
{
    blt_llms_ltk_request = evtCbFunc;
}

void blc_ll_initAclConnection_module(void)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_data_extension_t)), acl_conn);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_ll_conn_t)), acl_conn);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(acl_conn_para_t)), acl_conn);
#endif

    ll_acl_conn_irq_task_cb = blt_acl_conn_interrupt_task;
    ll_acl_conn_mlp_task_cb = blt_acl_conn_mainloop_task;


    ll_push_tx_fifo_handler = blt_llms_pushTxfifo;


    if (!blmsParam.max_master_num) {
        blmsParam.max_master_num = LL_MAX_ACL_CEN_NUM; //default value
    }
    if (!blmsParam.max_slave_num) {
        blmsParam.max_slave_num = LL_MAX_ACL_PER_NUM;  //default value
    }

    aclConn_param.auto_dle          = 1;
    aclConn_param.connDleSendTimeUs = 220 * 1000; //    //init DLE pending time, default 220 mS

    st_ll_conn_t *pAclConn;
    for (int i = ACL_CONN_IDX_CEN0; i < LL_MAX_ACL_CONN_NUM; i++) {
        pAclConn = (st_ll_conn_t *)&blms[i];

        pAclConn->acl_conIndex = i;

        if (i < LL_MAX_ACL_CEN_NUM) {
#if (LL_ACL_CEN_EN)
            pAclConn->acl_conHandle = BLM_CONN_HANDLE | i;
            pAclConn->aclRole       = ACL_ROLE_CENTRAL;
#endif
        } else {
#if (LL_ACL_PER_EN)
            pAclConn->acl_conHandle = BLS_CONN_HANDLE | i;
            pAclConn->aclRole       = ACL_ROLE_PERIPHERAL;
#endif
        }

#if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        blc_ll_fsu_reset(pAclConn->acl_conHandle);
#endif

        pAclConn->bSlot_shift_margin = 5; //625uS *5 = 3125 uS
    }
}

_attribute_ram_code_ int blt_acl_conn_interrupt_task(int flag)
{
    if (flag == FLAG_IRQ_RX) {
        irq_acl_conn_rx();
    } else if (flag == FLAG_IRQ_TX) {
        irq_acl_conn_tx();
    } else if (flag == FLAG_ACL_CONN_PARAM_UPDATE_CHECK) {
        /* master & slave role connection parameter update concerned */
        blt_llms_procConnCreateConnParamUpdate();

        /*    one slave/master connection is in sync state (connSync changes only in "irq_system_timer")
         * || one slave is near conn_param update timing(connUpdate_busy changes only in "blt_llms_procConnCreateConnParamUpdate")  */
        blmsParam.new_conn_forbidden       = aclConn_param.connSync || aclConn_param.connUpdate_busy;
        blmsParam.newConn_forbidden_slave  = blmsParam.scanInitEn_union.leg_init_en;
        blmsParam.newConn_forbidden_master = aclConn_param.connUpdate_master_busy;
    }

    return 0;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    int
    blt_acl_conn_mainloop_task(int flag, void *p)
{
    (void)p; //unused, remove warning

    if (flag == (int)FLAG_MODULE_MAINLOOP) {
        blt_ll_acl_conn_mainloop();
    } else if (flag == (int)FLAG_MODULE_RESET) {
        blt_ll_reset_acl_conn();
    } else if (flag == (int)FLAG_CHECK_INIT) {
        return blt_ll_checkAclInit();
    } else if (flag == (int)FLAG_ACL_CONN_EXIT_CHECK) {
        for (int i = ACL_CONN_IDX_CEN0; i < LL_MAX_ACL_CONN_NUM; i++) {
            if (blms[i].connState) {
                return 1;
            }
        }
        //Although the connection is gone, the write disconnect mask is not cleared
        if (blmsParam.disconnEvt_mask) {
            return 1;
        }
    }
    return 0;
}

void blt_ll_reset_acl_conn(void)
{
    //very important !
    blmsParam.cur_slave_num  = 0;
    blmsParam.cur_master_num = 0;

    blmsParam.new_conn_forbidden       = 0;
    blmsParam.newConn_forbidden_master = 0;

    /*pay attention: all parameters which clear in connection terminate procedure should be considered if need clear in HCI reset callback function*/
    blmsParam.disconnEvt_mask    = 0;
    aclConn_param.connSync       = 0;
    blmsParam.connectEvt_mask    = 0;
    blmsParam.connUptCmd_pending = 0;

    aclConn_param.updateCmd_pending = 0;
    aclConn_param.prefMaxTxLen      = 27;
    aclConn_param.prefMaxTxTime     = LL_PDU_TIME_1M(27);


    st_ll_conn_t *pAclConn;
    for (u8 conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) {
        pAclConn            = (st_ll_conn_t *)&blms[conn_idx];
        pAclConn->connState = CONN_STATUS_DISCONNECT;

        pAclConn->conn_update_union.update_pack = 0; //clear: update_cmd & update_param & update_map & update_phy
        pAclConn->irq_event1_union.irqevt1_pack = 0; //clear: connect_evt & disconn_evt & conn_update_evt & phy_update_evt
        pAclConn->conn_termin_union.termin_pack = 0; //clear: peer_terminate & local_terminate & terminate_reason
        pAclConn->conn_termin_union.terminate_pending = 0;

        pAclConn->connUpt_inst_jump     = 0;         //must clear
        pAclConn->ll_rsp_timeout_tick   = 0;
        pAclConn->conn_established_tick = 0;

        pAclConn->llcp_flag.flagBits = 0;

        pAclConn->encryption_tmp_st = 0;

        pAclConn->rejectReason = 0;

        pAclConn->conn_updateEvt_pending = 0;

        //////Frame Space Update//////
#if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        blc_ll_fsu_reset(pAclConn->acl_conHandle);
#endif

#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        pAclConn->cisEstablish_msk = 0;
#endif
    }


    if (ll_acl_slave_mlp_task_cb) {
        ll_acl_slave_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_acl_slave_mainloop_task
    }


    if (ll_acl_master_mlp_task_cb) {
        ll_acl_master_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_acl_master_mainloop_task
    }


#if (LL_FEATURE_ENABLE_PAST)
    if (ll_acl_past_mlp_task_cb) {
        ll_acl_past_mlp_task_cb(FLAG_MODULE_RESET); //blt_past_mainloop_task
    }
#endif


#if (LL_FEATURE_ENABLE_POWER_CONTROL)
    if (ll_acl_pcl_mlp_task_cb) {
        ll_acl_pcl_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_ll_pclMainloopTask
    }
#endif


#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
    if (ll_acl_chnclass_mlp_task_cb) {
        ll_acl_chnclass_mlp_task_cb(FLAG_MODULE_RESET, NULL); //blt_chnclass_mainloop_task
    }
#endif
}

//common code for s_connect/m_connect, to save ram_code
_attribute_ram_code_ int blms_connect_common(st_ll_conn_t *pc, rf_packet_connect_t *pInit, bool aux_conn)
{
    /*SiHui: potential risk, main_loop block, two new connection share same buffer "conn_req_info", lead to callback data error to host
     *       but not easy to trigger*/
    smemcpy(conn_req_info, ((&pInit->rf_len) - 1), 14);

#if (LL_FEATURE_ENABLE_PRIVACY)
    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
    if (bltAddr.local_use_rpa) {
        pc->conn_locUseRpa = 1;
        pc->pRslvlst_conn  = bltAddr.pRslvlst_locRpa;
        smemcpy(pc->conn_localRpa, pc->aclRole ? pInit->advA : pInit->initA, BLE_ADDR_LEN); // role, 1 for ACL_ROLE_PERIPHERAL
    } else {
        pc->conn_locUseRpa = 0;
    }
    #endif


    pc->conn_peerAddrType = bltAddr.peer_pka_or_ida_type;
    smemcpy(pc->conn_peerAddr, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
    if (bltAddr.peer_use_rpa) {
        pc->conn_peerUseRpa = 1;
        smemcpy(pc->conn_peerRpa, pc->conn_peerPktA, BLE_ADDR_LEN);
    } else {
        pc->conn_peerUseRpa = 0;
    }
#else
    pc->conn_peerAddrType = pc->conn_peerPktA_type;
    smemcpy(pc->conn_peerAddr, pc->conn_peerPktA, BLE_ADDR_LEN);
#endif


    smemcpy(&pc->aclAccessAddr, pInit->accessCode, 4);
    smemcpy(&pc->aclCrcInit, pInit->crcinit, 3);
    pc->conn_intvl_n_1m25 = pInit->interval;
    pc->conn_intvl_tick   = pInit->interval * SYSTEM_TIMER_TICK_1250US;

    blt_ll_set_interval_level(TSKOFT_ACL_CONN + blms_conn_sel, pInit->interval);

    pc->conn_latency = pInit->latency;
    pc->conn_timeout = pInit->timeout * 10000 * SYSTEM_TIMER_TICK_1US;
    pc->conn_chn_hop = pInit->hop;
    pc->conn_sca     = pInit->sca;
    pc->ppm_div_10   = ppmDiv10_tbl[pc->conn_sca];
    smemcpy(pc->acl_chnParam.chmTbl, pInit->chm, 5);

#if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
    if (pc->peer_chnSel && local_chsel) {
        pc->conn_chnsel   = 1;
        pc->chnIdentifier = (pc->aclAccessAddr >> 16) ^ (pc->aclAccessAddr & 0xffff);
        csa2_calculateMapInfo(&pc->acl_chnParam);
    } else
#endif
    {
        pc->conn_chnsel = 0; ///find by lijing.if CSA#2,not power down, connect with CSA#1, if not clear, it will connect fail.
        pc->chn_idx     = 0;
        blt_csa1_calculateChannelTable(pInit->chm, pc->conn_chn_hop, pc->acl_chnParam.rempChmTbl);
    }


    pc->connState                    = CONN_STATUS_COMPLETE;
    pc->irq_event1_union.connect_evt = 1 | (aux_conn ? ENHANCED_CONN_FLAG_AUX_CONNECT : 0); //CallBack process later in mainLoop

    aclConn_param.connSync |= (1 << blms_conn_sel);
    my_dump_str_u8s(0, "connSync connect", aclConn_param.connSync, 0, 0, 0);
    blmsParam.connectEvt_mask |= (1 << blms_conn_sel);
    pc->conn_updateEvt_pending = 0; //clear in "connect_common", no need in terminate, enough
    pc->remoteFeatureReq       = 0;
    pc->FeatureRsp             = 0;


    pc->conn_inst            = 0;
    pc->conn_successive_miss = 0;
    pc->tx_wptr = pc->tx_rptr = 0;

    pc->ll_upd_flag                   = 0;
    pc->rxPktCnt                      = 0;
    pc->conn_termin_union.termin_pack = 0;


#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
    pc->authPayloadEnable    = 0;
    pc->authPayloadTimeoutUs = 30 * 1000 * 1000; //30s
#endif

#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    pc->nocAckNum  = 32;
    pc->nocAckMsk  = 32 - 1;
    pc->nocAckWptr = pc->nocAckRptr = 0;
    smemset(pc->nocAclTxWptr, 0, 32 * 2);
#endif

    /* clear in connect start and connect end, for more secure */
    pc->conn_update_union.update_pack = 0; //clear: update_cmd & update_param & update_map & update_phy

    /* can not clear terminate_reason, main_loop event callback need use, if new connect too quick, clearing will lead to reason lost */
    pc->conn_termin_union.peer_terminate = pc->conn_termin_union.local_terminate = 0; //clear local terminate/peer terminate

    pc->conn_termin_union.terminate_pending = 0;


    pc->conn_fifo_flag = BIT(1);                                                      //to make sure first BTX/BRX event must have empty packet insert to HW FIFO if SW FIFO has valid data
    pc->local_sn       = 0;
    pc->local_nesn     = 0;
#if (OPTIMIZE_INSERT_EMPTY_EN)
    pc->local_last_sn = pc->local_sn;
#endif
    pc->peer_last_sn    = 0x10;
    pc->peer_last_rfLen = 0;

    pc->conn_established_tick = 0;
    pc->conn_complete_tick    = clock_time() | 1;
    //todo later: main_loop variable can set in main_loop code, save RamCode, consider connection complete timing
    pc->ll_remoteFeature0   = 0;             /* low 4 byte */
    pc->ll_remoteFeature1   = 0;             /* high 4 byte */
    pc->crypt.mic_fail      = 0;             //clear mic fail flag
    pc->crypt.enable        = 0;
    pc->crypt.st            = MS_LL_ENC_OFF; //clear status
    pc->ll_enc_busy         = 0;
    pc->llcp_flag.flagBits  = 0;             //clean ll_version_ind/ll_feature_exchange/ll_conn_upd/ll_map_upd/ll_phy_upd flags
    pc->ll_rsp_timeout_tick = 0;             //PROCEDURE RESPONSE TIMEOUT
    pc->inter_jump_num      = 0;
    pc->connUpt_inst_jump   = 0;
#if (HW_AES_CCM_ALG_EN)
    pc->hw_aes_ccm_flag = 0;
#endif

#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    pc->actual_txrx_sche_us = (200 + SCHE_NEW_TASK_MARGIN_US) + pdu_27b_tifs_27b_us[pc->connPhyCtrl.conn_cur_phy - 1][pc->crypt.enable]; //200+50+806=1056
    pc->limit_txrx_sche_us  = 0;
#endif

#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    blt_cfg_conn_phy_param(&pc->connPhyCtrl, bltPHYs.cur_llPhy, bltPHYs.cur_own_CI); //Reset conn_cur_phy and conn_cur_CI to the dft settings.
#endif


#if (LL_FEATURE_ENABLE_POWER_CONTROL)
    if (ll_acl_pcl_irq_task_cb) {
        ll_acl_pcl_irq_task_cb(blms_conn_sel | FLAG_PCL_INIT_AFT_ACL_CONN, NULL); //blt_ll_pclInterruptTask
    }
#endif

#if (LL_FEATURE_ENABLE_PAST)
    if (ll_acl_past_irq_task_cb) {
        ll_acl_past_irq_task_cb(blms_conn_sel | FLAG_PAST_INIT_AFT_ACL_CONN, NULL); //blt_ll_pastInterruptTask
    }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
    if (ll_acl_chnclass_irq_task_cb) {
        ll_acl_chnclass_irq_task_cb(blms_conn_sel | FLAG_CHNC_INIT_AFT_ACL_CONN, NULL); //blt_ll_chnclassInterruptTask
    }
#endif

#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)

    ll_data_extension_t *pExt_data = &pc->ext_data;

    pExt_data->connEffectiveMaxTxOctets = MAX_OCTETS_DATA_LEN_27;
    pExt_data->connEffectiveMaxRxOctets = MAX_OCTETS_DATA_LEN_27;
    pExt_data->connEffectiveMaxRxTime   = LL_PDU_TIME_1M(27); //328us
    pExt_data->connEffectiveMaxTxTime   = LL_PDU_TIME_1M(27); //328us

    if ((pExt_data->connMaxRxOctets != MAX_OCTETS_DATA_LEN_27 || pExt_data->connMaxTxOctets != MAX_OCTETS_DATA_LEN_27) && aclConn_param.auto_dle) {
        pExt_data->connMaxTxRxOctets_req = DATA_LENGTH_REQ_PENDING;
    } else {
        pExt_data->connMaxTxRxOctets_req = 0;
    }
#endif

#if 0 //(LL_FEATURE_ENABLE_CONNECTED_ISO)
    pc->acl_cis_req_pendingMsk = 0;
#endif


    blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_CONN_CREATE);


#if (BLMS_PM_ENABLE)
    pc->pm_error_us = 0;
#endif


#if OS_SUP_EN
    if (blt_os_semCountIncrementIrq_cb) {
        blt_os_semCountIncrementIrq_cb();
    }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
    if (ll_chn_sounding_irq_task_cb) {
        ll_chn_sounding_irq_task_cb(FLAG_CS_ACL_CB_FLAG, (void *)pc);
    }
#endif

    return 0;
}

__attribute__((weak))
_attribute_ram_code_ void blt_debug_acl_conn_start(int conn_sel)
{
#if (SL01_acl_0)
    log_task_begin_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL01_acl_0 + blms_conn_sel);
#endif

    DBG_CS_CHN0_HIGH;

#if (0) //save other dbg gpio
    DBG_CHN4_HIGH;
    DBG_SIHUI_CHN4_HIGH;
    DBG_FANQH_CHN1_HIGH;
#else
    if (conn_sel == 0) {
        DBG_CHN4_HIGH;
        DBG_SIHUI_CHN4_HIGH;
        DBG_TIANXIANG_CHN3_HIGH;
    }
    #if (LL_MAX_ACL_CONN_NUM > 1)
    else if (conn_sel == 1) {
        DBG_CHN5_HIGH;
        DBG_SIHUI_CHN5_HIGH;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 2)
    else if (conn_sel == 2) {
        DBG_CHN6_HIGH;
        DBG_SIHUI_CHN6_HIGH;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 3)
    else if (conn_sel == 3) {
        DBG_CHN7_HIGH;
        DBG_SIHUI_CHN7_HIGH;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 4)
    else if (conn_sel == 4) {
        DBG_CHN8_HIGH;
        DBG_SIHUI_CHN8_HIGH;

        DBG_TIANXIANG_CHN4_HIGH;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 5)
    else if (conn_sel == 5) {
        DBG_CHN9_HIGH;
        DBG_SIHUI_CHN9_HIGH;
        DBG_TIANXIANG_CHN3_HIGH;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 6)
    else if (conn_sel == 6) {
        DBG_CHN10_HIGH;
        DBG_SIHUI_CHN10_HIGH;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 7)
    else if (conn_sel == 7) {
        DBG_CHN11_HIGH;
        DBG_SIHUI_CHN11_HIGH;
    }
    #endif
#endif
}

__attribute__((weak))
_attribute_ram_code_ void blt_debug_acl_conn_post(int conn_sel)
{
#if (SL01_acl_0)
    log_task_end_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL01_acl_0 + blms_conn_sel);
#endif

    DBG_CS_CHN0_LOW;

#if (0) //save other dbg gpio
    DBG_CHN4_LOW;
    DBG_FANQH_CHN1_LOW;
#else
    if (conn_sel == 0) {
        DBG_CHN4_LOW;
        DBG_SIHUI_CHN4_LOW;
        DBG_TIANXIANG_CHN3_LOW;
    }
    #if (LL_MAX_ACL_CONN_NUM > 1)
    else if (conn_sel == 1) {
        DBG_CHN5_LOW;
        DBG_SIHUI_CHN5_LOW;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 2)
    else if (conn_sel == 2) {
        DBG_CHN6_LOW;
        DBG_SIHUI_CHN6_LOW;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 3)
    else if (conn_sel == 3) {
        DBG_CHN7_LOW;
        DBG_SIHUI_CHN7_LOW;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 4)
    else if (conn_sel == 4) {
        DBG_CHN8_LOW;
        DBG_SIHUI_CHN8_LOW;
        DBG_TIANXIANG_CHN4_LOW;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 5)
    else if (conn_sel == 5) {
        DBG_CHN9_LOW;
        DBG_SIHUI_CHN9_LOW;
        DBG_TIANXIANG_CHN3_LOW;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 6)
    else if (conn_sel == 6) {
        DBG_CHN10_LOW;
        DBG_SIHUI_CHN10_LOW;
    }
    #endif
    #if (LL_MAX_ACL_CONN_NUM > 7)
    else if (conn_sel == 7) {
        DBG_CHN11_LOW;
        DBG_SIHUI_CHN11_LOW;
    }
    #endif
#endif
}


#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ int
    blms_start_pre_process(int conn_idx)
{
    //make sure BTX/BRX state machine is clean, and to prevent boundary packet from Scan state
    STOP_RF_STATE_MACHINE;

    blmsParam.rf_fsm_busy = 1;
    blm_btxbrx_state      = 1;

    blms_conn_sel = conn_idx;
    blms_pconn    = (st_ll_conn_t *)&blms[blms_conn_sel];

    blms_pconn->tick_qihang_mark_conn = clock_time(); //only for Qihang: broadcast sink conflict with ACL. Later will delete.

    blt_debug_acl_conn_start(blms_conn_sel);                  //debug GPIO

    return 1;
}

//common code for BTX & BRX start, to save ram_code
#if OS_COMPILE_OPTIMIZE_EN
_attribute_ram_code_sec_optimize_o2_
#endif
    _attribute_ram_code_ int
    blms_start_common_1(st_ll_conn_t *pc)
{
//--------------- interval jump process --------------------------------------------------------//
/* old code use "if(onn_inst_u32)" as a premise to judge if jump happens
     * new code(20221013) delete this, to solve problem described in "bSlot mark conn" calculation acl_periperal.c
     * After deleting "if(onn_inst_u32)", we should also pay attention about other code in this "if" branch to prevent
     * some error, such as  "connUpt inst jump"(have processed in new code, confirm it,s OK) */

/* 3125 uS = 625uS *5 = 32*5 sSlot
     * consider slot adjust when low power enable, margin set to 5 slot is more safer, minimum connection interval 7.5mS is 12 Slot,
     * 5 slot margin can handle 3.125 mS timing shit, and maximum 10 slot 6.25mS error no risk for inter_jump_num */

/* when peripheral low power used, 5 sSlot margin is not enough for some RX window shift seriously */
#if (BLMS_PM_ENABLE)
    blms_pconn->inter_jump_num = (bltSche.bSlot_idx_irq_real + blms_pconn->bSlot_shift_margin - blms_pconn->bSlot_mark_conn) / blms_pconn->bSlot_interval - 1;
#else
    blms_pconn->inter_jump_num = (bltSche.bSlot_idx_irq_real + 5 - blms_pconn->bSlot_mark_conn) / blms_pconn->bSlot_interval - 1;
#endif

    if (blms_pconn->connUpt_inst_jump) {
        blms_pconn->inter_jump_num += blms_pconn->connUpt_inst_jump;
        blms_pconn->connUpt_inst_jump = 0;         // clear after use
    }

    if (blms_pconn->inter_jump_num) {              //conn_interval jump happens
        blms_pconn->conn_inst += blms_pconn->inter_jump_num;
        blms_pconn->chn_idx = ((u32)(blms_pconn->chn_idx + blms_pconn->inter_jump_num)) % 37;
        blms_pconn->ll_upd_flag &= ~CONN_UPD_FLAG; //clean connection update flag

        blt_ll_incSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, bltPri.step_final[TSKOFT_ACL_CONN + blms_conn_sel] * 2 * blms_pconn->inter_jump_num);
    }


    /* Different process for different MCU: ******************************************/
    //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
    //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
    //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
    //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
    //  otherwise send DMA Tx fifo non-default area.
    //reg_dma_rf_tx_mode = 0x80;  //DMA : BLE TX FIFO mode enable, eagle not exist
    /**********************************************************************************/

    /* Different process for different MCU: set RX DMA FIFO and RX threshold **********/
    //Switch dma rx buffer to ACL's dma rx buffer
    ble_rf_set_rx_dma((u8 *)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);
    //Config dma tx fifo depth && buffer size: M and S use their own independent TX FIFO, here needs to be distinguished
    u8  txFifoDepth;
    u16 txFifoSizeDiv16;
    if (pc->aclRole == ACL_ROLE_CENTRAL) {
        txFifoDepth     = blt_m_txfifo.depth;
        txFifoSizeDiv16 = blt_m_txfifo.size >> 4;
    } else {
        txFifoDepth     = blt_s_txfifo.depth;
        txFifoSizeDiv16 = blt_s_txfifo.size >> 4;
    }
    ble_rf_set_tx_dma(txFifoDepth, txFifoSizeDiv16);

    /*
        1. ACL-U and ACL-C RX/Tx Limits are not the same.
        2. ACL-U (data) limits are managed by Data Length procedure (LENGTH_REQ/RSP)
        3. ACL-C (LLCP) limits are managed by  'Long Control PDU' feature support. (See 4.5.11  Control PDU length management)
        4. If an IUT supports CIS or Periodic Advertiser Sync Transfer, it shall be able to receive control PDU greater than 26 bytes.
     */
    if ((LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE << 29)) ||
        (LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_PAST_RECIPIENT << 25)) || (LL_FEATURE_MASK_1 & (LL_FEATURE_MASK_CHANNEL_SOUNDING))) {
        rf_set_rx_maxlen(blt_rxfifo.size - 17); //17: 4DMA + 2 header + 3CRC + 8 extInfor
    } else {
        u16 rx_max_len = blt_llms_get_connEffectiveMaxRxOctets_by_connIdx(blms_conn_sel);
        rf_set_rx_maxlen(rx_max_len + 4);       //MIC 4Bytes //TODO: if LL_CIS_REQ:  Payload Data Length 36Byte, need DLE process
    }
    /**********************************************************************************/


    //--------------- channel map update ------------------------------------------------------------
    //pay attention here, BTX/BRX slot may dropped, pc->conn_inst >= pc->conn_map_inst_next(consider 0xffff->0 problem, (u16).... < 1024 )
    if ((pc->conn_update_union.update_mark & CONN_UPDATE_MAP) && (u16)(pc->conn_inst - pc->conn_map_inst_next) < BIT(10)) {
        pc->ll_upd_flag &= ~CHN_MAP_FLAG; //clean map update flag

        pc->conn_update_union.update_mark &= ~CONN_UPDATE_MAP;
        pc->conn_update_union.update_cmd = 0;

        smemcpy(&pc->acl_chnParam, &pc->nextChn, sizeof(struct le_channel_map));
        //      my_dump_str_data(STACK_DUMP_EN, "Map update", &pc->acl_conHandle, 2);

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
        pc->chn_map_update_evt = 1;
#endif
    }
#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    if ((pc->conn_update_union.update_mark & CONN_PHY_UPDATE_IND_CMD) && (u16)(pc->conn_inst - pc->conn_phy_inst_next) < BIT(10)) {
        pc->ll_upd_flag &= ~PHY_UPD_FLAG;
        pc->llcp_flag.bit.ll_phy_ind_send_flag = 0;
        pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 0;

        pc->conn_update_union.update_mark &= ~CONN_PHY_UPDATE_IND_CMD;
        pc->conn_update_union.update_cmd = 0;

        if (llms_conn_phy_update_cb) {
            llms_conn_phy_update_cb(blms_conn_sel); ////blt_ll_updateConnPhy
        }
    }

    if (llms_conn_phy_switch_cb) {
        llms_conn_phy_switch_cb(blms_conn_sel); ///blt_ll_switchConnPhy
    }
#endif


#if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
    if (pc->conn_chnsel) {
        pc->conn_chn = ll_chn_index_calc_cb(&pc->acl_chnParam, pc->conn_inst, pc->chnIdentifier);
    } else
#endif
    {
        pc->conn_chn = pc->acl_chnParam.rempChmTbl[pc->chn_idx];
    }

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_PCL, pc->currRfPwrIdx);

    rf_set_ble_channel(pc->conn_chn);
    rf_set_ble_access_code((u8 *)&pc->aclAccessAddr);
    rf_set_ble_crc_value(pc->aclCrcInit); //save ram_code 12 byte compare to "rf_set_ble_crc"

#if (HW_AES_CCM_ALG_EN)

    if (pc->hw_aes_ccm_flag) { //&& pc->crypt.enable
                               //      blt_restore_aes_ccm_para(pc);

        blt_ll_setAesCcmPara(pc->aclRole, pc->crypt.sk, pc->crypt.nonce.iv, 0x03, pc->crypt.enc_pno, pc->crypt.dec_pno, pc->lastTxLen_flag);
    }

    my_dump_str_u32s(0, "brx", blt_debug_hex_2_dec_display(pc->conn_inst), blt_debug_hex_2_dec_display(pc->crypt.enc_pno), blt_debug_hex_2_dec_display(pc->crypt.dec_pno), reg_ccm_control);
#endif

#if (LL_FEATURE_ENABLE_LE_CODED_PHY)
    rf_trigger_codedPhy_accesscode(); //This function must be placed after the "reset_baseband"
#endif

#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    #if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        u8 ifs_idx = pc->aclRole == ACL_ROLE_CENTRAL ? ST_ACL_CP_POS: ST_ACL_PC_POS;
        u16 rx_timeout = gFsuValidFsVal[blms_pconn->connPhyCtrl.conn_cur_phy-1][ifs_idx];

        u8 isValid = 0;
        if(blms_pconn->fsu_param.phyMask&BIT(blms_pconn->connPhyCtrl.conn_cur_phy-1)){
            if(pc->aclRole == ACL_ROLE_CENTRAL){ //master
                if(pc->fsu_param.spacingType & FSU_ST_ACL_PC){
                    isValid = 1;
                }
            }else{ //slave
                if(pc->fsu_param.spacingType & FSU_ST_ACL_CP){
                    isValid = 1;
                }
            }
        }

        if(isValid){
            if(pc->fsu_param.fsu_procedure_status == FSU_REQ_CMD_SEND){
                rx_timeout = max(pc->fsu_param.fs_max, pc->fsu_param.fs_valid);
            }
            else if(pc->fsu_param.fsu_procedure_status == FSU_RSP_CMD_SEND){
                u16 preFsVal = gFsuPreFsVal[blms_pconn->connPhyCtrl.conn_cur_phy-1][ifs_idx];
                rx_timeout = max(pc->fsu_param.fs_valid, preFsVal);
            }
        }

        if(blms_pconn->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED){
            rx_timeout += 450; //margin, actually, it should be set proportionally rather than as a fixed value. todo
        }else{
            rx_timeout += 100; //margin, actually, it should be set proportionally rather than as a fixed value. todo
        }

        rf_ble_set_rx_timeout(rx_timeout);
    #else
        /* must set RX timeout, cause some other function(e.g. ext_adv) may change this value */
        rf_ble_set_rx_timeout((pc->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED) ? 600 : 250);
    #endif
#else
    rf_ble_set_rx_timeout(250);
#endif


    /* 20240411
     *  1. old code before fast settle function, rx_settle value set in "rf_drv_ble_init" part 3(setting for BLE by BLE_Team),
     *     add no need set in any other task. means that: we use this init_value all time.
     *  2. when fast settle function enabled, we set the smaller rx_settle value for some modules, but under the macro "FAST_SETTLE"
     *     means that: we use smaller rx_settle value when we need fast RX settle, and use others(maybe init_value or smaller rx_settle value)
     *     when we do not care about fast RX settle.
     *  3. when we want process potential register loss when reset bandband for B91,
     *     we should set rx_settle value whenever RX settle value is used in modules. So we remove the macro "FAST_SETTLE",
     *     use "RX_SETTLE_US" which will be different values according to macro "FAST_SETTLE" processed by ext_rf.h
     *
     * 20240528
     *  4. consider Onca RF bring more problem which make code harder to maintain, we decide to set TX & TX settle for
     *     every different linklayer task if RX or TX is used */
    rf_ble_set_rx_settle(RX_SETTLE_US);


    /* 20240411, process TX_wait & RX_wait for BTX/BRX
     * move from "rf_drv_ble_init" part 3(setting for BLE by BLE_Team)
     * to handle potential register loss when reset bandband for B91 */
    rf_ble_set_rx_wait(RF_RX_WAIT_MIN_VALUE); //only involved in BTX/BRX/TX2RX

#if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
    if ((pc->conn_cte_type == AOA_TYPE) && (cte_conn_switchPattern[blms_conn_sel].cte_slot_duration == SWITCH_SAMPLE_SLOT_1US)) {
        aoa_set_sample_slot_time(SAMPLE_1US_SLOT);
        DBG_CHN10_HIGH;
    } else {
        aoa_set_sample_slot_time(SAMPLE_NORMAL_SLOT);
        DBG_CHN10_HIGH;
        DBG_CHN10_TOGGLE;
        DBG_CHN10_TOGGLE;
    }
    rf_set_aoa_aod_trx_mode(RF_RX_ACL_AOA_EN);
    cte_conn_switchPattern[blms_conn_sel].cte_rx_mode_en = 1;
#endif

    blt_llms_push_fifo_hw();

    CLEAR_ALL_RFIRQ_STATUS; //important: drop boundary RX packet after potential Scan state


    return 0;
}

_attribute_ram_code_ int blms_start_common_2(st_ll_conn_t *pc)
{
    aclConn_param.conn_rx_num = 0;   //RX number (regardless of CRC correct or wrong)
    aclConn_param.txPktCnt    = 0;
    pc->sync_num++;
    pc->conn_receive_packet     = 0; //RX with CRC correct
    pc->conn_receive_new_packet = 0; //RX with CRC correct & new SN
    pc->curCEsyncAP             = FALSE;

    //master & slave share. for master: equal to  bSlot_idx_irq, for slave: must use ..._real
    //for master: must do it after calculate "inter_jump_num"
    pc->bSlot_mark_conn = bltSche.bSlot_idx_irq_real;

    /* some special use for main_loop: conn_inst not match with bSlot_mark, cause conn_inst will increase by 1 at BTXRX_post,
     * and main_loop code may run in anywhere: before BTXRX start, or between BTXRX start and post, or after BTXRX post. */
    pc->conn_inst_mark = pc->conn_inst;

#if (SL16_acl_cen_evtCnt)
    log_b16_irq(SL_STACK_CS_TIME_EN, (pc->aclRole == ACL_ROLE_CENTRAL) ? SL16_acl_cen_evtCnt : SL16_acl_per_evtCnt, pc->conn_inst_mark);
#endif

    pc->conn_tick_mark = bltSche.sSlot_tick_irq; //attention that: for slave PM, this tick may may 1~4 mS earlier than BRX anchor point

    if (pc->conn_updateEvt_pending)              //BIT(0):conn para changes,  BIT(1):should report HCI EVT
    {
        //      if(pc->conn_updateEvt_pending & BIT(1))
        {
            //      pc->conn_updateEvt_pending = 0;
            pc->irq_event1_union.conn_update_evt = 1;
            blmsParam.conupdtEvt_mask |= (1 << blms_conn_sel);
            pc->conn_latency = pc->conn_latency_next; //some extreme case, new_param may overwrite it, only exist in Sihui's pressure test

#if OS_SUP_EN
            if (blt_os_semCountIncrementIrq_cb) {
                blt_os_semCountIncrementIrq_cb();
            }
#endif

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            if ((pc->factor > 1) && (pc->conn_updateEvt_pending & BIT(0))) {
                pc->factor    = 1;
                pc->conti_num = 0;
                pc->insertTsk = 0;

                blt_sche_addUpdate(SLOT_UPDT_SLAVE_SUBRATE_STATE_CHANGE);

                //              DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;DBG_CHN6_TOGGLE;

                my_dump_str_u32s(DBG_SUBRATE_EN, "conn update", pc->conn_inst, pc->factor, pc->conti_num, 0);
            }

#endif
        }

        pc->conn_updateEvt_pending = 0;
    }


    u32 sSlot_extend_num;
#if (BLMS_CONN_TIMING_EXTEND)
    // -1 is actual bound, -3 for more margin
    u32 sSlot_available = (bltSche.pTask_next ? bltSche.pTask_next->begin : bltSche.sSlot_endIdx_maxPri) - pc->sSlot_sche_use - 3 - bltSche.sSlot_idx_irq_real;

    #if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        #if (CS_SCH_OPTIMIZE)
    if (blms_pconn->csTaskEnableMask || blms_pconn->cs_pending) {
        u8           cs_cfg_idx   = pc->cs_pending & CS_IDX_MSK;
        cs_config_t *pCsCfg       = gCsMng.gGlobal_pCsCfg + cs_cfg_idx;
        u16          cs_remainder = pCsCfg->Procedure_Interval; //if Procedure_Interval is 0, skip the following judgment. By SunWei
        if (cs_remainder) {
            if ((u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) < BIT(14)) {
                cs_remainder = (u16)(pc->conn_inst_mark - pCsCfg->inst_start_proc) % pCsCfg->Procedure_Interval;
            }
        }
        if (cs_remainder == 0 && pCsCfg->cs_procedure_en) {
            sSlot_available = 0;
        }
    }
        #endif
    #endif

    #if (LL_RSSI_SNIFFER_MODE_ENABLE)
        #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
    bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
        #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
    bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
        #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
        #endif
    if (snif_used && (sSlot_available > SNIFFER_SYNC_RX_WINDOW_MAX_SSLOT)) {
        /*
                     * The packet receipt window is too large, resulting in a missed insert CS Scheduler.
                     * This can easily cause the CS procedure count to not accumulate properly.
                     * The sub node only listens to two consecutive packets, acl_sniffer_sync rx_window_max 3.5ms is sufficient.
                     * Needs to be optimized and fundamentally fixed later.
                     */
        sSlot_available = SNIFFER_SYNC_RX_WINDOW_MAX_SSLOT; // 3.5ms
    }
    #endif

    /* 1. sSlot available bigger than sSlot duration
         * 2. do not extend when sync_timing  */
    if (sSlot_available > pc->sSlot_allocNum && !blms_pconn->sync_timing) {
        sSlot_extend_num = sSlot_available;

    #if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        u16 tMCES_us = gFsuValidFsVal[blms_pconn->connPhyCtrl.conn_cur_phy-1][ST_ACL_MCES_POS];
        u32 sSlot_conn_max = (pc->bSlot_interval - max(pc->bSlot_interval/8, tMCES_us/625))*32;
    #else
        u32 sSlot_conn_max = pc->bSlot_interval * 32 * 7 / 8; // 7/8 interval
    #endif

        if (sSlot_conn_max < sSlot_extend_num) {
            sSlot_extend_num = sSlot_conn_max;
        }
    } else
#endif
    {
        sSlot_extend_num = pc->sSlot_allocNum;
    }

    aclConn_param.task_end_tick = bltSche.sSlot_tick_irq_real + sSlot_extend_num * SSLOT_TICK_NUM;
    systimer_set_irq_capture(aclConn_param.task_end_tick);

    return 0;
}

_attribute_ram_code_ bool blms_post_pre_process(void)
{
    /* BTX post or BRX_post, should stop RF, wait, then re_check RX, to deal with the boundary RX packet near */
    if (blmsParam.rf_fsm_busy) {
        blmsParam.rf_fsm_busy = 0;
        /*
        1. To avoid RX DMA rewrite bug(only Eagle), using rf_len to find CRC, stopping FSM for boundary RX may cause error RX data passing CRC check, leading to post data lost
        2. Using software method to avoid above bug, drop all boundary RX(regardless of correct or error data), must make sure SN/NESN/TX_RPTR not updated by
           potential correct boundary RX.
        */

#if (DBG_BOUNDARY_RX)
        if (rf_receiving_flag()) {
            Adbg_boundary_rx_num++;
            my_dump_str_data(DBG_BOUNDARY_RX, "boundary rx", &Adbg_boundary_rx_num, 4);
        }
#endif


        STOP_RF_STATE_MACHINE;

        /* very important to clear RX status: boundary RX packet may enter other state
        consider timing margin, we clear it after 'dly_duration_clr_rf_sts' us, so add a mark here, clear it later */
        blmsParam.delay_clear_rf_status = 30; //set mark and duration = 30us
        /* Mark how many microseconds from here to clear the RF status */
        blmsParam.dly_start_tick_clr_rf_sts = clock_time() | 1;
    }


    return TRUE;
}

_attribute_ram_code_ bool blt_ll_isMarkFifoTxDone(st_ll_conn_t *pc)
{
    bool ret          = FALSE;
    u8   tx_num       = pc->tx_num;
    u8   tx_rptr      = pc->tx_rptr;
    u8   tx_fifo_flag = pc->conn_fifo_flag;
    u8   dma_tx_rptr  = pc->conn_dma_tx_rptr;

    if (!pc->save_flg) { //for more insurance, if rx irq lost(empty packet), operation here would get correct sn/nesn/tx_rptr
        dma_tx_rptr = (HAL_REG_RF_DMA_FIFO_TX_RPTR & FLD_DMA_RPTR_MASK);
    }

    u8 n = (HAL_REG_RF_DMA_FIFO_TX_WPTR - dma_tx_rptr) & 15;
    if (tx_fifo_flag & BIT(2)) { //BIT(2) valid, means tx_num not zero, HW FIFO no empty
        tx_num--;
    }
    if (tx_num > n) {
        tx_rptr += tx_num - n;
    }

    if ((1 && pc->tx_wptr == tx_rptr) || (1 && tx_rptr >= pc->connMarkTxFifoWptr)) {
        ret = TRUE;
    }

    return ret;
}

//common code for BTX & BRX post, to save ram_code
_attribute_ram_code_ int blms_post_common_1(st_ll_conn_t *pc)
{
    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_OFF);
    }

#if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
    DBG_CHN10_LOW;
    rf_set_aoa_aod_trx_mode(RF_AOA_OFF);
    cte_conn_switchPattern[blms_conn_sel].cte_rx_mode_en = 0;
#endif

    int acl_terminate = 0, cis_terminate = 0;
    pc->conn_inst++;
    pc->chn_idx++;
    if (pc->chn_idx >= 37) {
        pc->chn_idx = 0;
    }

    if (pc->conn_receive_packet) {
        if (pc->connState == CONN_STATUS_COMPLETE) {
            pc->connState             = CONN_STATUS_ESTABLISH;
            pc->conn_established_tick = clock_time() | 1;

#if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
            if (aclConn_etbsh.cusConnEtbsh_en && pc->aclRole == ACL_ROLE_CENTRAL) {
                pc->establish_evt = 1;
            }
#endif
        }

#if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        if(pc->fsu_param.fsu_procedure_status == FSU_RSP_CMD_SEND){
            pc->fsu_param.fsu_procedure_status = FSU_PROCEDURE_COMPLETE;
        }
#endif
    }


    // 4 situation of terminate
#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
    bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
    bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
    #endif
    if ((pc->connState == CONN_STATUS_COMPLETE) && (snif_used ? ((u16)(pc->conn_inst - pc->acl_sniffer_establish_inst) >= (BLMS_CONN_CREATE_RX_MAX_TRY_NUM * 3)) : (pc->conn_inst >= BLMS_CONN_CREATE_RX_MAX_TRY_NUM)))
#else
    if ((pc->connState == CONN_STATUS_COMPLETE) && pc->rxPktCnt == 0 && pc->conn_inst >= BLMS_CONN_CREATE_RX_MAX_TRY_NUM)
#endif
    {
        pc->conn_termin_union.terminate_reason = HCI_ERR_CONN_FAILED_TO_ESTABLISH;
    }

    else if (pc->conn_termin_union.peer_terminate) {
        pc->conn_termin_union.terminate_pending++;
        if((pc->conn_termin_union.terminate_pending >= ll_acl_Per_AckCentralDisconnectNum) || (pc->aclRole == ACL_ROLE_CENTRAL) ) //slave try 3 count, this value comes from the BLE single code
        {
            pc->conn_termin_union.terminate_reason = pc->conn_termin_union.peer_terminate;
            pc->conn_termin_union.terminate_pending = 0;
        }
        cis_terminate = 1;
    } else if (pc->conn_termin_union.local_terminate) {
#if (!BQB_TEST_EN)
        if (blt_ll_isMarkFifoTxDone(pc) || clock_time_exceed(pc->conn_terminate_tick, 500000))
#else
        if (blt_ll_isMarkFifoTxDone(pc) || clock_time_exceed(pc->conn_terminate_tick, 2000000))
#endif
        {
            pc->conn_termin_union.terminate_reason = pc->conn_termin_union.local_terminate;
        }
    }
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING & 0)
    else if ((pc->factor > 1) && ((u32)(clock_time() - pc->conn_tick) > pc->subrate_timeout * 10000 * SYSTEM_TIMER_TICK_1US)) {
        pc->conn_termin_union.terminate_reason = HCI_ERR_CONN_TIMEOUT;
        cis_terminate                          = 1;
    }
#endif
    else if ((u32)(clock_time() - pc->conn_tick) > pc->conn_timeout) {
        pc->conn_termin_union.terminate_reason = HCI_ERR_CONN_TIMEOUT;
        cis_terminate                          = 1;
    }


    if (pc->conn_termin_union.terminate_reason) { //terminate happens
        /*pay attention: all parameters which clear here should be considered if need clear in HCI reset callback function*/
        pc->irq_event1_union.disconn_evt = 1;
        blmsParam.disconnEvt_mask |= (1 << blms_conn_sel);
        pc->conn_termin_union.peer_terminate = pc->conn_termin_union.local_terminate = 0;

        pc->conn_termin_union.terminate_pending = 0;
        /* clear in connect start and connect end, for more secure */
        pc->conn_update_union.update_pack = 0; //clear: update_cmd & update_param & update_map & update_phy

        pc->connUpt_inst_jump = 0;             //must clear
        pc->connState         = CONN_STATUS_DISCONNECT;
        aclConn_param.connSync &= ~(1 << blms_conn_sel);
        my_dump_str_u8s(0, "connSync terminate", aclConn_param.connSync, 0, 0, 0);
        blmsParam.connUptCmd_pending &= ~(1 << blms_conn_sel);
        pc->ll_rsp_timeout_tick   = 0;
        pc->conn_established_tick = 0;

        pc->llcp_flag.flagBits = 0;
        pc->rejectReason       = 0; //clear in terminate is enough, no need clear it in start_common, save RamCode
        blc_ll_acl_resetInfoRSSI(pc->acl_conHandle);

#if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        ///////////Frame space update terminate processing/////////////
        blc_ll_fsu_reset(pc->acl_conHandle);
#endif

        acl_terminate = 1;
        //log_b8_irq(SL_STACK_ACL_BASIC_TIMING_EN, SL08_acl_tmnt, pc->conn_termin_union.terminate_reason);

#if OS_SUP_EN
        if (blt_os_semCountIncrementIrq_cb) {
            blt_os_semCountIncrementIrq_cb();
        }
#endif

#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        /* for peer terminate & acl_timeout, notify CIS disconnect if exist.
             * local terminate:  already processed in blc_ll_disconnect, FLAG_ACL_LOCAL_DISCONNECT
             * FAILED_TO_ESTABLISH: no CIS
             */
        if (ll_cis_conn_irq_task_cb && pc->cisEstablish_msk && cis_terminate) {
            ll_cis_conn_irq_task_cb(FLAG_ACL_IRQ_TERMINATE, pc); // blt_cis_conn_interrupt_task
        }
#else
        (void)cis_terminate;
#endif

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
        if (ll_acl_subrate_irq_task_cb) {
            ll_acl_subrate_irq_task_cb(FLAG_ACL_SUBRATE_RESET, (void *)pc);
        }
#endif

#if (LL_FEATURE_ENABLE_POWER_CONTROL)
        if (ll_acl_pcl_irq_task_cb && pc->pPclCb) {
            pc->pPclCb->pcl_occpied = 0;
            pc->pPclCb              = NULL;
        }
#endif

#if (LL_FEATURE_ENABLE_PAST)
        if (ll_acl_past_irq_task_cb && pc->pPastCb) {
            pc->pPastCb->past_occpied = 0;
            pc->pPastCb               = NULL;
        }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
        if (ll_acl_chnclass_irq_task_cb && pc->pChncCb) {
            pc->pChncCb->chnc_occpied = 0;
            pc->pChncCb               = NULL;
        }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
        if (ll_chn_sounding_irq_task_cb && pc->csTaskEnableMask) {
            ll_chn_sounding_irq_task_cb(FLAG_CS_ACL_DISCONN_CB, pc);
        }

#endif

    }
#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    else {
        pc->every_conn_evt = 1;
    }
#endif


    return acl_terminate;
}

_attribute_ram_code_ int blt_ll_acl_conn_sync_process(int sync_ok)
{
    if (sync_ok) {
        if (blms_pconn->conn_update_union.update_mark & CONN_UPDATE_SYNC) {
            /* for slave:
             * can not clear to 0 here: some extreme case: new CONN_UPDATE_CMD get in main_loop before this first sync brx_post,
             * then brx_post clear CONN_UPDATE_CMD, this update will lose, lead to connection timeout */
            blms_pconn->conn_update_union.update_mark &= ~CONN_UPDATE_SYNC;
        }

        if (aclConn_param.connSync & (1 << blms_conn_sel)) {
            aclConn_param.connSync &= ~(1 << blms_conn_sel);
            my_dump_str_u8s(0, "connSync sync OK", aclConn_param.connSync, 0, 0, 0);
            blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_LOW);
        } else {
            blt_ll_incSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, -(bltPri.step_final[TSKOFT_ACL_CONN + blms_conn_sel] * 4));
        }

        blms_pconn->conn_successive_miss = 0;

#if (LL_FEATURE_ENABLE_POWER_CONTROL)
        if (ll_acl_pcl_irq_task_cb) {
            ll_acl_pcl_irq_task_cb(blms_conn_sel | FLAG_PCL_MONITORING_PATH_LOSS, NULL); //blt_ll_pclInterruptTask
        }
#endif
    } else {
        blms_pconn->conn_successive_miss++;
        if (blms_pconn->conn_successive_miss > 10) {
            blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, TASK_PRIORITY_LOW);
            blms_pconn->conn_successive_miss = 5;
        } else {
            blt_ll_incSchedulerTaskPriority(TSKOFT_ACL_CONN + blms_conn_sel, bltPri.step_final[TSKOFT_ACL_CONN + blms_conn_sel]);
        }
    }


    return 0;
}

_attribute_ram_code_ int blms_post_common_2(void)
{
    blm_btxbrx_state = 0;


#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)
    if (ll_chn_sounding_irq_task_cb && (blms_pconn->connState == CONN_STATUS_ESTABLISH) && (blms_pconn->csTaskEnableMask || blms_pconn->cs_pending)) {
        ll_chn_sounding_irq_task_cb(FLAG_SCHEDULE_POLL, blms_pconn); //blt_cs_interrupt_task   blt_ll_acl_post_checkCsTask()
    }
    // cs indication done
    #if (LL_CS_CEN_REF_BV_01_C)
    if (blms_pconn->indFlagPending && blt_ll_isMarkFifoTxDone(blms_pconn)) {
        blms_pconn->csReportEvtFlag = 1;
    }
    #endif
#endif
    if (tick1_exceed_tick2(aclConn_param.task_end_tick, clock_time())) {
        aclConn_param.task_end_tick = clock_time();
    }
    blt_ll_calculate_sSlot_next(aclConn_param.task_end_tick + blms_pconn->sSlot_sche_use * SSLOT_TICK_NUM);

    blt_debug_acl_conn_post(blms_conn_sel); //debug GPIO

    rf_ble_set_tx_wait(RF_RX_WAIT_MIN_VALUE);

    return 0;
}

_attribute_ram_code_ int irq_acl_conn_rx(void)
{
#if 1                                   //optimize, to save RamCode
    u8 *raw_pkt = ble_curr_rx_dma_buff; //or cisConn_param.cis_rx_dma_buff
    blt_rxfifo.wptr++;
#else
    u8 *raw_pkt = (u8 *)(blt_rxfifo.p_base + (blt_rxfifo.wptr++ & blt_rxfifo.mask) * blt_rxfifo.size);
#endif
    u8 *new_pkt = (u8 *)(blt_rxfifo.p_base + (blt_rxfifo.wptr & blt_rxfifo.mask) * blt_rxfifo.size);

    aclConn_param.acl_rx_dma_buff = (u32)new_pkt; //Update the next acl dma rx buffer
    ble_rf_set_rx_dma((u8 *)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);

    HAL_CLEAR_RF_RX_IRQ;


    u8 drop_rx_data = 0;
    u8 next_buffer  = 0;
    raw_pkt[2]      = 0;             //for data mark, connHandle
    raw_pkt[3]      = 0;             //for some other mark


    if (blms_pconn->rxPktCnt == 0) { //blms_pconn->rxPktCnt is u8 type
        blms_pconn->rxPktCnt++;
    }

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    blms_pconn->llcp_flag.bit.peer_ack_flag = 0;
#endif

    /* "rx header tick" none zero is under condition CRC correct, so here do not check CRC by
       "RF_BLE_PACKET_VALIDITY_CHECK" to save RamCode and running timing
       Or we can use "bltRxPkt.crc correct" */
    if (bltRxPkt.rx_header_tick) {
#if (SLEV_acl_rx)
        log_event_irq(SL_STACK_ACL_BASIC_TIMING_EN, SLEV_acl_rx);
#endif

#if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
    #ifdef HAL_CHIP_USE_CSEM_MODEM_IP
        #error "CTE, tx rx settle process !!!"
    #endif
        if (cte_conn_switchPattern[blms_conn_sel].cte_rx_mode_en) {
            u8 rx_header = raw_pkt[DMA_RFRX_OFFSET_HEADER];
            if (rx_header & BIT(5)) { //CP
    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                if (blms_pconn->connPhyCtrl.conn_cur_phy == BLE_PHY_1M) {
                    rf_ble_set_tx_wait(TX_STL_AUTO_MODE_1M + 8 - TX_FAST_SETTLE_TIME);
                    rf_ble_set_tx_settle(TX_FAST_SETTLE_TIME);
                } else if (blms_pconn->connPhyCtrl.conn_cur_phy == BLE_PHY_2M) {
                    rf_ble_set_tx_wait(TX_STL_AUTO_MODE_2M + 1 - TX_FAST_SETTLE_TIME);
                    rf_ble_set_tx_settle(TX_FAST_SETTLE_TIME);
                }
    #else
                rf_ble_set_tx_wait(TX_STL_AUTO_MODE_1M + 8 - TX_FAST_SETTLE_TIME);
                rf_ble_set_tx_settle(TX_FAST_SETTLE_TIME);
    #endif
            }
        }
#endif

        if (blms_state == BLMS_STATE_BRX_S) {
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            if ((HAL_REG_RF_DMA_FIFO_TX_RPTR & FLD_DMA_RPTR_MASK) != (HAL_REG_RF_DMA_FIFO_TX_WPTR & FLD_DMA_WPTR_MASK)) {
                blms_pconn->subrate_flag.bit.validDataRxTx_flag = 1;
            }
#endif
        }

        blms_pconn->conn_tick           = clock_time();
        blms_pconn->conn_receive_packet = 1;

        if (((u8)(blt_rxfifo.wptr - blt_rxfifo.rptr) & 63) >= blt_rxfifo.num) {
            drop_rx_data = 1;

#if OS_SUP_EN
            if (blt_os_semCountIncrementIrq_cb) {
                blt_os_semCountIncrementIrq_cb();
            }
#endif
        }


        if (drop_rx_data) {
            blms_pconn->save_flg = 2;
            STOP_RF_STATE_MACHINE;
            /*
             * CSEM chip, Here, We need to stop the RF FSM to completely end the current BTX
             * or BRX mode. User must call the following macro to cleanly end the RF process
             * (possibly currently in the more data phase).
             */
            HAL_CSEM_IP_RESET_BASEBAND;

            systimer_set_irq_capture(clock_time() + 800);
        } else {
            blms_pconn->save_flg         = 1;
            blms_pconn->local_sn         = reg_rf_ll_pid_l & FLD_RF_SN;
            blms_pconn->local_nesn       = HAL_GET_RF_NESN;
            blms_pconn->conn_dma_tx_rptr = (HAL_REG_RF_DMA_FIFO_TX_RPTR & FLD_DMA_RPTR_MASK);

            //blt_save_aes_ccm_para(blms_pconn);
        }


        if (!drop_rx_data) {
            u8 peer_cur_rfLen = raw_pkt[DMA_RFRX_OFFSET_RFLEN];
            //u8 llid   = raw_pkt[DMA_RFRX_OFFSET_HEADER] & 3;
            u8 peer_cur_sn = (raw_pkt[DMA_RFRX_OFFSET_HEADER] >> 3) & 1;
#if (OPTIMIZE_INSERT_EMPTY_EN)
            u8 peer_cur_nesn = (raw_pkt[DMA_RFRX_OFFSET_HEADER] >> 2) & 1; //llid nesn sn md
#endif

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            if (peer_cur_rfLen) {
                blms_pconn->subrate_flag.bit.validDataRxTx_flag = 1;
            }
#endif


#if (OPTIMIZE_INSERT_EMPTY_EN)
            if (blms_pconn->local_last_sn != peer_cur_nesn) {
                blms_pconn->llcp_flag.bit.peer_ack_flag = 1;

                //DBG_C HN5_TOGGLE; DBG_C HN5_TOGGLE;
            }
            blms_pconn->local_last_sn = blms_pconn->local_sn;
#endif

#if (LL_ASYNC_LEA_EN)
            if (blms_pconn->async_lea_link) {
                extern my_fifo_t async_tx_fifo;
                extern my_fifo_t async_rx_fifo;
                if (async_tx_fifo.rptr != async_tx_fifo.wptr) {
                    blc_async_sdu_t *pSdu =
                        (blc_async_sdu_t *)(async_tx_fifo.p + (async_tx_fifo.rptr & (async_tx_fifo.num - 1)) * async_tx_fifo.size);
                    if (peer_cur_sn != blms_pconn->peer_last_sn) {
                        pSdu->ackIndex++;
                    }
                }
                if (async_rx_fifo.rptr != async_rx_fifo.wptr) {
                    blc_async_sdu_t *pSdu =
                        (blc_async_sdu_t *)(async_rx_fifo.p + (async_rx_fifo.rptr & (async_rx_fifo.num - 1)) * async_rx_fifo.size);
                    if (peer_cur_sn != blms_pconn->peer_last_sn) {
                        pSdu->ackIndex++;
                    }
                }
            }
#endif

            ///////////////////////////////  NEW pkt  //////////////////////////////////////
            if (peer_cur_sn != blms_pconn->peer_last_sn || (1 && peer_cur_rfLen && !blms_pconn->peer_last_rfLen)) {
                blms_pconn->peer_last_sn    = peer_cur_sn;
                blms_pconn->peer_last_rfLen = peer_cur_rfLen;

                blms_pconn->conn_receive_new_packet = 1;

#if (HW_AES_CCM_ALG_EN & 0)
                if ((blms_pconn->hw_aes_ccm_flag) && (reg_rf_dec_err & FLD_RF_TLK_MIC_ERR)) {
                    blms_pconn->crypt.mic_fail = 2; // pending need to
                }
#endif

                /* must before "blt_acl_slave_rx_procUpdateReq", cause update_cmd may set 1 in slave_rx_procUpdateReq */
                if (blms_pconn->conn_update_union.update_cmd == 1) {
                    blms_pconn->conn_update_union.update_cmd = 2;
                }

                if (peer_cur_rfLen > 0) {
                    raw_pkt[2]  = blms_pconn->acl_conHandle;
                    next_buffer = 1;
#if BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
    #if (LL_FEATURE_ENABLE_PAST)
                    if (ll_acl_past_irq_task_cb) {
                        ll_acl_past_irq_task_cb(FLAG_PAST_RCVD_PRD_SYNC_IND | blms_conn_sel, (void *)raw_pkt);
                    }
    #endif

                    if (blms_state == BLMS_STATE_BRX_S) {
    #if (ACL_SLAVE_NON_MODULAR)
                        blt_acl_slave_rx_procUpdateReq(raw_pkt);
    #else
        #error "acl module code"
    #endif
                    }
                    blms_pconn->conn_pkt_rcvd++;
#endif
                }
            }
        }


        if (!aclConn_param.conn_rx_num) {
            if (bltRxPkt.rx_header_tick) {
                if (blms_state == BLMS_STATE_BRX_S) {
#if (LL_ACL_PER_EN)
                    if (!bls_pconn->tick_1st_rx) {
                        bls_pconn->tick_1st_rx = bltRxPkt.rx_header_tick;
                    }
#endif
#if (BLE_STACK_MCU_STALL_EN)
                    u32 txCompleteTick = blt_llms_get_connEffectiveMaxTxTime_by_connIdx(blms_conn_sel) + 200 * SYSTEM_TIMER_TICK_1US; //150us+50us
                    blt_pm_setWfiWakeupTick(clock_time() + txCompleteTick);
#endif
                }

                //Mark, under the current connection event, M and S interact
                if (!blms_pconn->curCEsyncAP) {
                    blms_pconn->curCEsyncAP = TRUE;
                }
            }
        }

#if (LL_FEATURE_ENABLE_POWER_CONTROL)
        if (ll_acl_pcl_irq_task_cb) {
            /* peer_coded_phy_ci is only used by PCL */
            if (bltPHYs.cur_llPhy == BLE_PHY_CODED) {
                blms_pconn->peer_coded_phy_ci = bltPHYs.cur_peer_CI;
            }
            /* Monitoring ACL rx packet's RSSI value */
            s8 rssi = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110;                                 //OR get rssi by HW register
            ll_acl_pcl_irq_task_cb(blms_conn_sel | FLAG_PCL_MONITORING_ACL_RX_RSSI, (void *)&rssi); //blt_ll_pclInterruptTask
        }
#endif

        blc_ll_acl_recordLatestRSSI(blms_pconn->acl_conHandle, raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)]);
        blc_ll_acl_recordRSSI(blms_pconn->acl_conHandle, raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)]);

#if OS_SUP_EN
        if (blt_os_semCountIncrementIrq_cb) {
            blt_os_semCountIncrementIrq_cb();
        }
#endif
    }


    aclConn_param.conn_rx_num++;                      //do not care CRC

    if (!next_buffer)                                 //reuse buffer
    {
        blt_rxfifo.wptr--;
        aclConn_param.acl_rx_dma_buff = (u32)raw_pkt; //Reuse the last dma rx buffer
        ble_rf_set_rx_dma((u8 *)aclConn_param.acl_rx_dma_buff, aclConn_param.acl_rx_dma_size);
    }


    raw_pkt[0] = 1; //destroy RX dma_len to prevent one buffer data repeated use

    return 0;
}

_attribute_ram_code_ void rf_txWait_txSettle_handle(u8 acl_role){

    u8 tTxWaitIdx = 1; //default 1M

    #if(LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
        tTxWaitIdx = blms_pconn->connPhyCtrl.conn_cur_phy;

        #if (BLE_S2_S8_NEW_PATH)
            if (blms_pconn->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED && blms_pconn->connPhyCtrl.conn_cur_CI == LE_CODED_S8) {
                tTxWaitIdx += 1;
            }
        #endif
    #endif

    #if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        u8 directIdx = (acl_role==ACL_ROLE_CENTRAL) ? ST_ACL_PC_POS : ST_ACL_CP_POS;
        u16 tTxWaitUs = gFsuValidFsVal[blms_pconn->connPhyCtrl.conn_cur_phy-1][directIdx] -\
                        tx_rxPathDly_extraPreamble[tTxWaitIdx] - TX_FAST_SETTLE_TIME + (HW_AES_CCM_ALG_EN ? 1 : 0);
    #else
        u16 tTxWaitUs = tx_stl_auto_mode[tTxWaitIdx] - TX_FAST_SETTLE_TIME + (HW_AES_CCM_ALG_EN ? 1 : 0);
    #endif

    rf_ble_set_tx_wait(tTxWaitUs);
    rf_ble_set_tx_settle(TX_FAST_SETTLE_TIME);
    rf_ble_csem_set_tx_rx_settle(1, tx_stl_auto_mode[tTxWaitIdx], 0);
}

_attribute_ram_code_ int irq_acl_conn_tx(void)
{
    /* for ACL Central
     * first TX no need concern TIFS, should consider TX packet quality
     * when btx_start, TX settle can use smaller value than auto mode. if fast settle enable, TX settle can be smaller, to save bandwidth
     * in first TX IRQ, must change TX settle value to auto mode value to guarantee TIFS 150uS */
    if (blms_pconn->aclRole == ACL_ROLE_CENTRAL && aclConn_param.txPktCnt == 0) {

        rf_txWait_txSettle_handle(ACL_ROLE_CENTRAL);

        aclConn_param.txPktCnt++;
    }


#if BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
    if (blms_pconn->conn_pkt_dec_pending) {
        blt_acl_slave_tx_procUpdateReq();
    } else
#endif
    {
        /*
         * Only when the number of hardware Tx fifos available is greater than 2 will MD compression be performed.
         */
        u8 n          = (HAL_REG_RF_DMA_FIFO_TX_WPTR - blms_pconn->conn_dma_tx_rptr) & 15;
        s8 leftTxNums = blms_pconn->tx_num - n;
        if (leftTxNums > 0) {
            //------------  save TX FIFO pointer --------------
            blt_llms_saveTxfifoRptr();

            //------------ Re-push to HW fifo ----------------------------
            blt_llms_pushToHWfifo();
        }
    }
#if (BLE_STACK_MCU_STALL_EN)
    if (blms_state == BLMS_STATE_BRX_S) {
        u32 rxCompleteTick = blt_llms_get_connEffectiveMaxRxTime_by_connIdx(blms_conn_sel) + 200 * SYSTEM_TIMER_TICK_1US; //150us+50us
        blt_pm_setWfiWakeupTick(clock_time() + rxCompleteTick);
    }
#endif
    return 0;
}

_attribute_ram_code_ void blt_llms_procConnCreateConnParamUpdate(void)
{
    //TODO: when PM used, slave slot adjust, conn_param_update trigger condition need fix.
    //Even no PM, when peripheral slot fine tune used, 1 slot adjust happens, potential risk exist.

#if (1)
    int           i;
    st_ll_conn_t *pc = NULL;
    #if (LL_ACL_CEN_EN)
    st_llm_conn_t *pm = NULL;
    #endif
    st_lls_conn_t *ps                    = NULL;
    aclConn_param.connUpdate_busy        = 0;
    aclConn_param.connUpdate_master_busy = 0;

    for (i = ACL_CONN_IDX_CEN0; i < LL_MAX_ACL_CONN_NUM; i++) //i< ACL_CONN_IDX_PER0 + max_slave_num
    {
        pc = (st_ll_conn_t *)&blms[i];

        if (pc->connState) {
    #if (LL_ACL_CEN_EN)
            if (i < LL_MAX_ACL_CEN_NUM) {
                pm = (st_llm_conn_t *)&blmsMaster[i];
            } else
    #endif
            {
                ps = (st_lls_conn_t *)&blmsSlave[i - LL_MAX_ACL_CEN_NUM];
            }


            if (pc->conn_update_union.update_mark & CONN_UPDATE_CMD) {
                pc->conn_update_union.update_mark &= ~CONN_UPDATE_CMD;

                s16 inst_diff = (u16)(pc->conn_para_inst_next - pc->conn_inst + 1); // conn_para_inst_next - (conn_inst - 1)
                if (inst_diff >= 1) {
                    //find first update BTX point winSize  header
                    pc->conn_update_tick = pc->conn_tick_mark + inst_diff * pc->conn_intvl_tick + pc->conn_offset_next * SYSTEM_TIMER_TICK_1250US;
                    if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
                        ps->conn_update_pre_sSlotIndex = ps->sSlot_mark_brx + (inst_diff - 1) * ps->sSlot_interval;
                        /* for slave: pre_sSlotIndex is calculated by sSlot_mark_brx, so sSlot_index can not reset before CONN_UPDATE_SYNC take effect*/
                        blmsParam.connUptCmd_pending |= (1 << i);
                    }
                    pc->conn_update_pre_bSlotIndex = pc->bSlot_mark_conn + (inst_diff - 1) * pc->bSlot_interval;
                    pc->conn_update_union.update_mark |= CONN_UPDATE_PENDING;


    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                    pc->subrate_flag.bit.conn_update_flag = 2;

                    //                  DBG_C HN1_TOGGLE;DBG_C HN1_TOGGLE;

                    my_dump_str_u32s(DBG_SUBRATE_EN, "set conn update flag", pc->subrate_flag.bit.conn_update_flag, pc->conn_update_union.update_mark, 0, 0);
    #endif

                } else {
                    /* instant passed, connection will terminate
                      TODO, special  design can handle this case, now drop this command */
                    my_dump_str_u32s(DBG_CONN_UPDATE, "inst passed 2", i, pc->conn_para_inst_next, pc->conn_inst, pc->conn_update_union.update_mark & CONN_UPDATE_PARAM_MASK);
                }
            }


            if (pc->conn_update_union.update_mark & CONN_UPDATE_PENDING) {
                //TODO: if only one connection task, and conn_interval very big, 240mS will error
                u32 tick_diff = (u32)(pc->conn_update_tick - pc->conn_offset_next * SYSTEM_TIMER_TICK_1250US - clock_time());

                u32 tick_thres;
                if (pc->conn_intvl_tick < (81 * SYSTEM_TIMER_TICK_1MS)) {
                    tick_thres = 360 * SYSTEM_TIMER_TICK_1MS;
                } else if (pc->conn_intvl_tick < (1001 * SYSTEM_TIMER_TICK_1MS)) {
                    tick_thres = pc->conn_intvl_tick * 3;
                } else {
                    tick_thres = pc->conn_intvl_tick * 2;
                }

                if (tick_diff < tick_thres) {
                    pc->conn_update_union.update_mark &= ~CONN_UPDATE_PENDING;
                    pc->conn_update_union.update_mark |= CONN_UPDATE_NEARBY;
                }
            }


            if (pc->conn_update_union.update_mark & CONN_UPDATE_NEARBY) {
    #if 1 //fix issue by QiHang's email "ACL connect update failed" 20230719
                if (BLMS_STATE_CHECK_ACL_CONN_UPDATE_NEARBY)
    #else
                if (blms_state & (BLMS_STATE_BTX_E | BLMS_STATE_BRX_E)) //TODO: not only BTX/BRX task occupy ACL connection in later design
    #endif

                {
                    /* task end timing pre_planned not equal to timing actual when ACL connection timing extending used(more efficient)
                     * so here use "bltSche.pTask_cur->end + 1" replace  bltSche.sSlot_idx_next */
                    //int slot_diff =  bltSche.sSlot_idx_next - ps->conn_update_pre_sSlotIndex;
                    int slot_diff = 0;
                    if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
                        slot_diff = bltSche.pTask_cur->end + 1 - ps->conn_update_pre_sSlotIndex;
                        //my_dump_str_u32s(DBG_SLAVE_CONN_UPDATE, "nearby", i, bltSche.pTask_cur->end, ps->conn_update_pre_sSlotIndex, slot_diff);
                    } else {
    #if (LL_ACL_CEN_EN)
                        u32 bSlot_next = ((bltSche.pTask_cur->end + 31) - 0) / 32 + bltSche.bSlot_idx_start;
                        slot_diff      = bSlot_next - pc->conn_update_pre_bSlotIndex;
    #endif
                    }

                    if (slot_diff > 0) {
                        //                  log_tick_irq(DBG_SLAVE_CONN_UPDATE, SLET_upt_sync_1 + i - LL_MAX_ACL_CEN_NUM);
                        pc->conn_update_union.update_mark &= ~CONN_UPDATE_NEARBY;
                        pc->conn_update_union.update_mark |= CONN_UPDATE_SYNC;
                        pc->conn_update_union.update_cmd = 0;
                        pc->connUpt_inst_jump            = (s8)(pc->conn_para_inst_next - pc->conn_inst); // (conn_para_inst_next - 1) - (conn_inst - 1)

                        s32 jump_inst = pc->connUpt_inst_jump;


                        /* improve */
                        if (pc->connUpt_inst_jump == -1 || pc->connUpt_inst_jump == -2) {
                            if ((s32)(pc->conn_update_tick - clock_time()) > 3000 * SYSTEM_TIMER_TICK_1US) {
                                pc->connUpt_inst_jump = 0;
                                my_dump_str_u32s(DBG_CONN_UPDATE, "overtime inst rescue", i, pc->connUpt_inst_jump, pc->conn_para_inst_next, pc->conn_inst);
                            }
                        }


                        if (pc->connUpt_inst_jump < 0) {
                            my_dump_str_u32s(DBG_SLAVE_CONN_UPDATE, "jump bug 1", i, pc->connUpt_inst_jump & 0xff, pc->conn_para_inst_next, pc->conn_inst);
                            //my_dump_str_u32s(DBG_SLAVE_CONN_UPDATE, "jump bug 1", bltSche.pTask_cur->end, ps->conn_update_pre_sSlotIndex, bltSche.sSlot_idx_irq_real, bltSche.sSlot_idx_next);
                            BLMS_ERR_DEBUG(DBG_CONN_UPDATE, 0xDD010000);
                            pc->connUpt_inst_jump = 0;
                        }

                        u16 bSlot_interval_next = pc->conn_intvl_next_n_1m25 * 2;      // 1.25 mS unit -> 625 uS unit
                        u32 sSlot_interval_next = pc->conn_intvl_next_n_1m25 * 2 * 32; //conn_intvl_next_n_1m25: max is 3200(4s),so 3200*2*32 = 0x32000,u32 is OK.
                        // + old_interval + offset - new_interval
                        u32 bSlot_offset;
                        if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
                            bSlot_offset = pc->conn_offset_next * 2;
                        } else {
                            bSlot_offset = pc->bSlot_oft_num_next;
                        }
                        pc->bSlot_mark_conn = pc->conn_update_pre_bSlotIndex + pc->bSlot_interval + bSlot_offset - bSlot_interval_next;
                        pc->bSlot_interval  = bSlot_interval_next;

                        u8 needConnUpdEvt = true;

                        if ((pc->conn_intvl_n_1m25 == pc->conn_intvl_next_n_1m25) && (pc->conn_latency == pc->conn_latency_next) && ((pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS)) == pc->conn_timeout_next)) {
                            needConnUpdEvt = false;
                        }
    #if (LL_ASYNC_LEA_EN)
                        needConnUpdEvt = true;
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                        if (pc->conn_intvl_n_1m25 != pc->conn_intvl_next_n_1m25) {
                            pc->conn_updateEvt_pending = BIT(0);
                        }
    #endif

                        if (pc->aclRole == ACL_ROLE_PERIPHERAL) {
    #if (LL_ACL_PER_EN)
        #if 0
                                ps->sSlot_mark_conn = ps->conn_update_pre_sSlotIndex + ps->sSlot_interval + (pc->conn_offset_next*32*2) - sSlot_interval_next;
                                ps->sSlot_interval = sSlot_interval_next;
                                pc->sSlot_allocNum = pc->conn_winsize_next*32*2;
                                /*TODO: optimize later SiHui */
                                blmsPm.slave_no_sleep |= (1<<(i - LL_MAX_ACL_CEN_NUM));
        #else
                            int tolerance_us = 0;
            #if (BLMS_PM_ENABLE)
                            u16 diff = pc->conn_intvl_next_n_1m25 - pc->conn_winsize_next;
                            if (diff < 4) {
                                //0 : not allowed in SPEC
                                //1 : 250       500
                                //2:  500       1000
                                //3:  750       1500
                                tolerance_us = diff * 250;
                                blmsPm.slave_no_sleep |= (1 << (i - LL_MAX_ACL_CEN_NUM));
                            } else {
                                tolerance_us = 1000;
                            }

                            if (pc->conn_offset_next == 0 && diff > 2) {
                                tolerance_us = 500;
                                blmsPm.slave_no_sleep |= (1 << (i - LL_MAX_ACL_CEN_NUM));
                            }
            #endif

                            ps->sSlot_interval   = sSlot_interval_next;
                            u16 cur_tolerance_us = blmsPm.pm_inited ? tolerance_us : 0;
                            if (ps->conn_tolerance_us < cur_tolerance_us) {
                                ps->conn_tolerance_us = cur_tolerance_us;
                            }
                            ps->connExpectTime  = ps->expectTimeMark + (s32)((jump_inst + 1) * pc->conn_intvl_tick) + pc->conn_offset_next * SYSTEM_TIMER_TICK_1250US;
                            ps->conn_start_time = ps->connExpectTime - BRX_LEFT_EARLY_TICK - ps->conn_tolerance_us * SYSTEM_TIMER_TICK_1US;
                            int n_sSlot         = (ps->conn_start_time - bltSche.sSlot_tick_irq_real) * SSLOT_TICK_REVERSE;
                            ps->sSlot_mark_conn = bltSche.sSlot_idx_irq_real + n_sSlot - ps->sSlot_interval;

            #if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
                            //If use DLE, extend transmit window to cover the max rx data length + 40 us for tolerance.
                            //If not use DLE, extend transmit window to cover rx-tx.
                            //TODO: this solution do not cover Coded PHY, just a temp solution for ConnUpdate under DLE.
                            ll_data_extension_t *pExt_data      = &pc->ext_data;
                            u32                  sSlot_data_len = (pExt_data->connEffectiveMaxRxTime) * SSLOT_US_REVERSE + 3; //14: Pre(1B)+AA(4B)+Head(2B)+MIC(4B)+CRC(3B)  //3 unknown
            #else
                            u32 sSlot_data_len = (LL_PDU_TIME_1M(MAX_OCTETS_DATA_LEN_27)) * SSLOT_US_REVERSE + 3; //14: Pre(1B)+AA(4B)+Head(2B)+MIC(4B)+CRC(3B)  //3 unknown
            #endif

            #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                            /* If the length is less than the 27byte, select the 27byte*/
                            if (sSlot_data_len < pdu_27b_tifs_27b_sslot[pc->connPhyCtrl.conn_cur_phy - 1][pc->crypt.enable]) {
                                sSlot_data_len = pdu_27b_tifs_27b_sslot[pc->connPhyCtrl.conn_cur_phy - 1][pc->crypt.enable];
                            }
            #else
                            /* If the length is less than the 27byte, select the 27byte*/
                            if (sSlot_data_len < pdu_27b_tifs_27b_sslot[0][pc->crypt.enable]) {
                                sSlot_data_len = pdu_27b_tifs_27b_sslot[0][pc->crypt.enable];
                            }
            #endif
                            /* task window can not too close to interval, leave 300uS margin, sSlot 15*19.53 = 293 uS */
                            u32 sSlot_rf_max = ps->sSlot_interval - 15;
                            if (sSlot_data_len > sSlot_rf_max) {
                                sSlot_data_len = sSlot_rf_max;
                            }

            #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                            pc->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + sSlot_data_len + pc->conn_winsize_next * 64 + ps->conn_tolerance_us * 64 / 625; // tolerance*2/(625/32) = tolerances*64/625
            #else
                            pc->sSlot_allocNum = BRX_MARGIN_SSLOT_NUM + pdu_27b_tifs_27b_sslot[0][pc->crypt.enable] + pc->conn_winsize_next * 64 + ps->conn_tolerance_us * 64 / 625; // tolerance*2/(625/32) = tolerances*64/625
            #endif
                            /* in case that tolerance too big, lead to duration exceed interval */
                            if (pc->sSlot_allocNum > (ps->sSlot_interval - 40)) { //20*40=800uS margin
                                pc->sSlot_allocNum = (ps->sSlot_interval - 40);
                            }
        #endif

                            blmsParam.connUptCmd_pending &= ~(1 << i);
                            pc->sync_timing = SLAVE_SYNC_CONN_UPDATE;
                            pc->sync_num    = 0;
                            blt_ll_set_slave_conn_interval_level(pc, ps, pc->conn_intvl_next_n_1m25);
    #endif
                        } else {
    #if (LL_ACL_CEN_EN)
                            /* update timing slot information */
                            aclMas_param.position_mask[pm->init_pos_idx] &= ~pm->init_pos_msk;
                            pm->init_pos_idx = pm->updt_pos_idx;
                            pm->init_pos_msk = pm->updt_pos_msk;
                            aclMas_param.position_mask[pm->init_pos_idx] |= pm->init_pos_msk;


                            my_dump_str_u32s(ACL_MASTER_INITIATE, "update change 1", pm->init_pos_idx, pm->init_pos_msk, pm->bSlot_1stBtx_mark_update_hold, pm->bSlotMark_position_update_hold);
                            my_dump_str_u32s(ACL_MASTER_INITIATE, "update change 2", aclMas_param.position_mask[0], aclMas_param.position_mask[1], aclMas_param.position_mask[2], aclMas_param.position_mask[3]);

                            if (pm->bSlot_1stBtx_mark_update_hold) {
                                aclMas_param.bslot_1st_btx_mark   = pm->bSlot_1stBtx_mark_update_hold;
                                pm->bSlot_1stBtx_mark_update_hold = 0;
                            }
                            if (pm->bSlotMark_position_update_hold) {
                                aclMas_param.bSlot_mark_position[pm->init_pos_idx] = pm->bSlotMark_position_update_hold;
                                pm->bSlotMark_position_update_hold                 = 0;
                            }
    #endif
                        }

                        /* use "conn_updateEvt_pending "hold this change until new conn_param used, to handle main_loop PM logic*/
                        //pc->conn_latency = pc->conn_latency_next;
                        pc->conn_intvl_n_1m25 = pc->conn_intvl_next_n_1m25;
                        pc->conn_intvl_tick   = pc->conn_intvl_n_1m25 * SYSTEM_TIMER_TICK_1250US;
                        pc->conn_timeout      = pc->conn_timeout_next * 10 * SYSTEM_TIMER_TICK_1MS;

                        /*
                         * LL/CON/PER/BV-139-C Subrate Factor set to 1 and Continuation Number set to 0 on
                         *  Connection Interval change
                         *  1. when the Peripheral IUT receives a connection interval change in a connection update from the Lower Tester, but
                         *  connInterval not change, The IUT does not send an HCI_LE_Connection_Update_Complete event to the Upper Tester
                         *  2. But if the if the instant between 2 subrate events, the IUT should listen the connect event where connEventCnt equals Instant
                         */
                        if (needConnUpdEvt | (pc->conn_update_union.update_mark & CONN_UPDATE_EVT)) {
                            pc->conn_updateEvt_pending |= BIT(1);
                            my_dump_str_u32s(DBG_SUBRATE_EN, "conn_upate", pc->conn_updateEvt_pending, needConnUpdEvt, pc->conn_update_union.update_mark, 0);

                            pc->conn_update_union.update_mark &= ~CONN_UPDATE_EVT;
                        }

                        aclConn_param.connSync |= (1 << i);
                        my_dump_str_u8s(0, "connSync update", aclConn_param.connSync, 0, 0, 0);
                        blt_ll_set_interval_level(TSKOFT_ACL_CONN + i, pc->conn_intvl_next_n_1m25);

                        int interval_weight = 0; //0~80
                        if (pc->conn_intvl_next_n_1m25 < CONN_INTERVAL_100MS) {
                            interval_weight = CONN_INTERVAL_100MS - pc->conn_intvl_next_n_1m25;
                        }
                        /* small interval get big priority, cause it can be sync_ed with less duration, better for timing*/
                        blt_ll_setSchedulerTaskPriority(TSKOFT_ACL_CONN + i, TASK_PRIORITY_CONN_UPDATE + interval_weight);
                        blt_sche_addUpdate(SLOT_UPDT_SLAVE_CONN_UPDATE);

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
                        if (pc->factor > 1) {
                            pc->subrate_flag.bit.conn_update_flag = 1;
                        }
    #endif


    #if (DBG_SLAVE_CONN_UPDATE)
                        if (i == 4) {
                            DBG_C HN4_LOW;
                        } else if (i == 5) {
                            DBG_C HN5_LOW;
                        } else if (i == 6) {
                            DBG_C HN6_LOW;
                        } else {
                            DBG_C HN7_LOW;
                        }
    #endif
    #if (DBG_MASTER_CONN_UPDATE)
                        if (i == 0) {
                            DBG_C HN8_TOGGLE;
                        } else if (i == 1) {
                            DBG_C HN9_TOGGLE;
                        } else if (i == 2) {
                            DBG_C HN10_TOGGLE;
                        } else if (i == 3) {
                            DBG_ CHN11_TOGGLE;
                        }
    #endif
                    }
                }
            }


            if (i < LL_MAX_ACL_CEN_NUM) {
                aclConn_param.connUpdate_master_busy |= (pc->conn_update_union.update_mark & CONN_UPDATE_PARAM_MASK);
            }

            aclConn_param.connUpdate_busy |= (pc->conn_update_union.update_mark & (CONN_UPDATE_NEARBY | CONN_UPDATE_SYNC));
        }
    }

    //TODO, when CONN_UPDATE_NEARBY, connection slot timing can not extend, cause conn_update_pre_slotIndex may error
#else
    aclConn_param.connUpdate_busy = 0;
#endif
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
    bool
    blt_llms_pushTxfifo(u16 connHandle, u8 *p)
{
    if (BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_TX)) {
        u8 LLID = p[0] & 0x03;
        if (LLID == 1) {
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_TX), "[LL][TX] L2CAP Continuation Fragment/Empty", p + 2, p[1]);
        } else if (LLID == 2) {
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_TX), "[LL][TX] Start Fragment/Complete", p + 2, p[1]);
        } else if (LLID == 3) {
            tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_TX), "[LL][TX] Packet", p + 2, p[1]);
        }
    }
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return 0;
    }

    u8            ll_master_role = (connHandle & BLM_CONN_HANDLE); //role:  1: master, 0 slave
    u8            conn_idx       = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc             = (st_ll_conn_t *)&blms[conn_idx];


    if (!pc->connState) { // CONN_STATUS_DISCONNECT
        return 0;
    }

    /* Different process for different MCU: ******************************************/
    u32 r         = irq_disable(); //must add IRQ protect
    int sw_fifo_n = ((pc->tx_wptr - pc->tx_rptr) & 31);
    irq_restore(r);


    u8 *pd         = NULL;
    u8  tx_num_max = 0;
    if (ll_master_role) { //Attention: connHandle can not be 0x00 ~ 0x06
#if (LL_ACL_CEN_EN)
        pd         = (u8 *)(blt_m_txfifo.p_base + (conn_idx * blt_m_txfifo.real_num + 1 + (pc->tx_wptr & blt_m_txfifo.mask)) * blt_m_txfifo.size);
        tx_num_max = blt_m_txfifo.logic_num;
#endif
    } else {
#if (LL_ACL_PER_EN)
        u8 slave_idx = conn_idx - LL_MAX_ACL_CEN_NUM; //Attention: get slave CONN FIFO correctly
        pd           = (u8 *)(blt_s_txfifo.p_base + (slave_idx * blt_s_txfifo.real_num + 1 + (pc->tx_wptr & blt_s_txfifo.mask)) * blt_s_txfifo.size);
        tx_num_max   = blt_s_txfifo.logic_num;
#endif
    }

    int empty_space = (pc->conn_fifo_flag & BIT(1)) ? 1 : 0; //need insert empty packet


    if (sw_fifo_n >= ((connHandle & HANDLE_STK_FLAG) ? (tx_num_max - empty_space) : (tx_num_max - empty_space - BLMS_STACK_USED_TX_FIFO_NUM))) {
        return 0;
    }


#if (TX_PUSH_DATA_LOG)
    if ((p[0] & 0x03) != 0x03) { //none control packet
        if (p[4] == 4) {
            my_dump_str_data(TX_PUSH_DATA_LOG, "ATT TX", p, p[1] + 2);
        } else if (p[4] == 5) {
            my_dump_str_data(TX_PUSH_DATA_LOG, "SIG TX", p, p[1] + 2);
        } else if (p[4] == 6) {
            my_dump_str_data(TX_PUSH_DATA_LOG, "SMP TX", p, p[1] + 2);
        }
    }
#endif

    //TODO: add length check, prevent memory error
    smemcpy(pd + 4, p, p[1] + 2);

#if (HW_AES_CCM_ALG_EN)
    if ((pc->crypt.enable) && (!pc->hw_aes_ccm_flag))
#else
    if (pc->crypt.enable)
#endif
    {
        /*
         * ll_ccm_enc: Master role must use 1, Slave role must use 0;
         * ll_ccm_dec: Master role must use 0, Slave role must use 1;
         */
        aes_enc_dec_busy = 1;
        aes_ll_ccm_encryption((llPhysChnPdu_t *)(pd + 4), ll_master_role, CRYPT_NONCE_TYPE_ACL, &pc->crypt);
        aes_enc_dec_busy = 0;
    }

    *(u32 *)pd = rf_tx_packet_dma_len(pd[5] + 2);


#if (HW_AES_CCM_ALG_EN)
    if (pc->crypt.enable && pc->hw_aes_ccm_flag && (pd[5] != 0)) {
        pd[5] += 4;
    }
#endif

    pc->tx_wptr++;

#if (BLMS_PM_ENABLE)
    if (!ll_master_role && ll_acl_slave_mlp_task_cb) {
        ll_acl_slave_mlp_task_cb(FLAG_ACL_SLAVE_CLEAR_SLEEP_LATENCY | conn_idx, NULL); //blt_acl_slave_mainloop_task
    }
#endif


    /**********************************************************************************/

    return 1;
}

_attribute_ram_code_ void blt_llms_pushToHWfifo(void)
{
    blms_pconn->tx_num      = 0;
    int insert_empty_number = 0;
    int new_tx_rptr;
    if (blms_conn_sel < LL_MAX_ACL_CEN_NUM) { //Master
        new_tx_rptr = blms_pconn->tx_rptr & blt_m_txfifo.mask;
    } else {                                  //Slave
        new_tx_rptr = blms_pconn->tx_rptr & blt_s_txfifo.mask;
    }

    int sw_fifo_n = (blms_pconn->tx_wptr - blms_pconn->tx_rptr) & 31;


#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)

    if (sw_fifo_n && (blms_pconn->aclRole == ACL_ROLE_CENTRAL)) {
        blms_pconn->subrate_flag.bit.validDataRxTx_flag = 1;

        //DBG_C HN6_TOGGLE;DBG_C HN6_TOGGLE;

        my_dump_str_u32s(DBG_SUBRATE_EN, "blt_llms_pushToHWfifo1", blms_pconn->conn_inst, blms_pconn->tx_wptr, blms_pconn->tx_rptr, sw_fifo_n);
    }
#endif

    //------------ push to HW fifo ----------------------------
    // 1. SW FIFO has valid data to send
    // 2. last BTX/BRX event all valid data send OK(regardless of empty packet insert or not ),
    //     or last BTX/BRX event has valid data and empty packet insert, but no packet send OK(first empty packet not ACKed)
    if (sw_fifo_n && (blms_pconn->conn_fifo_flag & BIT(1))) {
        insert_empty_number = 1;

        u8 *p = NULL;
        if (blms_conn_sel < LL_MAX_ACL_CEN_NUM) { //Master
#if (LL_ACL_CEN_EN)
            new_tx_rptr = (blms_pconn->tx_rptr - 1) & blt_m_txfifo.mask;
            p           = (u8 *)(blt_m_txfifo.p_base + (blms_conn_sel * blt_m_txfifo.real_num + 1 + new_tx_rptr) * blt_m_txfifo.size);
#endif
        } else { //Slave
#if (LL_ACL_PER_EN)
            new_tx_rptr = (blms_pconn->tx_rptr - 1) & blt_s_txfifo.mask;
            p           = (u8 *)(blt_s_txfifo.p_base + (bls_conn_sel * blt_s_txfifo.real_num + 1 + new_tx_rptr) * blt_s_txfifo.size);
#endif
        }

        smemcpy(p, (const void *)blms_tx_empty_packet, 6);

        // <2>: mark first packet is empty packet, to decide whether subtract 1 when recover software FIFO at BRX/BTX post
        blms_pconn->conn_fifo_flag |= BIT(2);
    }


    HAL_REG_RF_DMA_FIFO_TX_RPTR = (FLD_DMA_RPTR_SET | new_tx_rptr);
    HAL_REG_RF_DMA_FIFO_TX_WPTR = (new_tx_rptr + insert_empty_number + sw_fifo_n) & FLD_DMA_WPTR_MASK;
    blms_pconn->tx_num          = (insert_empty_number + sw_fifo_n);


    //record
    blms_pconn->conn_dma_tx_rptr = new_tx_rptr; //update newest dma tx rptr
}

_attribute_ram_code_ void blt_llms_saveTxfifoRptr(void)
{
    int n = (HAL_REG_RF_DMA_FIFO_TX_WPTR - blms_pconn->conn_dma_tx_rptr) & FLD_DMA_RPTR_MASK;

    // all data packet send OK when HW FIFO no empty ,or no data to send when HW FIFO empty
    if (n == 0) {
        blms_pconn->conn_fifo_flag |= BIT(1);
    }

    if (blms_pconn->conn_fifo_flag & BIT(2)) //first packet is empty packet
    {                                        //BIT(2) valid, means tx_num not zero, HW FIFO no empty

        if (n == blms_pconn->tx_num) {       // no packet send OK, all data hold
            blms_pconn->conn_fifo_flag |= BIT(1);
        }
        // at least one data packet send OK, but not all data packet send OK
        else if (n != 0) {
            blms_pconn->conn_fifo_flag &= ~BIT(1);
        }

        blms_pconn->tx_num--;
    }
    if (blms_pconn->tx_num > n) {
        blms_pconn->tx_rptr += blms_pconn->tx_num - n;
    }

    //only keep BIT(1) for next conn_event
    blms_pconn->conn_fifo_flag &= BIT(1); // <1> is used in next BRX/BTX push_fifo_he, can not clear here

#if (OPTIMIZE_INSERT_EMPTY_EN)
    if (blms_pconn->llcp_flag.bit.peer_ack_flag && (blms_pconn->aclRole == ACL_ROLE_CENTRAL)) {
        blms_pconn->conn_fifo_flag = 0;

        //DBG_C HN6_TOGGLE;DBG_C HN6_TOGGLE;
    }
#endif


#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    /* Mark ACL Data pushed read pointer W. If it is checked in RF ISR that the current TX RPTR exceeds the W write pointer, send Num of cmp EVT. */
    if (blms_pconn->nocAckWptr != blms_pconn->nocAckRptr) {
        u8 mrkAclTxW = blms_pconn->nocAclTxWptr[blms_pconn->nocAckRptr & blms_pconn->nocAckMsk];
        u8 deltaTx   = (mrkAclTxW - blms_pconn->tx_rptr) & 63;

        //my_dump_str_data(STACK_DUMP_EN, "@AclTxRptr", &blms_pconn->tx_rptr, 1);
        //my_dump_str_data(STACK_DUMP_EN, "@mrkAclTxW", &mrkAclTxW, 1);
        //my_dump_str_data(STACK_DUMP_EN, "@nocAckWptr", &blms_pconn->nocAckWptr, 1);
        //my_dump_str_data(STACK_DUMP_EN, "@nocAckRptr", &blms_pconn->nocAckRptr, 1);


        if (deltaTx == 0 || deltaTx > 31) {
            hci_numberOfCompletePacket_evt(blms_pconn->acl_conHandle, blms_pconn->numOfCmpCnt[(blms_pconn->nocAckRptr++) & blms_pconn->nocAckMsk]);
            //DBG_C HN10_TOGGLE;
            //DBG_C HN10_TOGGLE;

            //my_dump_str_data(STACK_DUMP_EN, "@noc", &blms_pconn->acl_conHandle, 1);
        } else {
            //my_dump_str_data(STACK_DUMP_EN, "@deltaTx", &deltaTx, 1);
            //DBG_C HN15_TOGGLE;
            //DBG_C HN15_TOGGLE;
        }
    }
#endif
}

_attribute_ram_code_ void blt_llms_push_fifo_hw(void)
{
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    blms_pconn->subrate_flag.bit.validDataRxTx_flag = 0;
#endif

    //------------ restore SN(bit4)/NESN(bit5) ---------------------------
    /* Different process for different MCU: ******************************************/
    //kite_vulture(comment:move it to the beginning of "blms_start_common_1"); eagle(not comment)
    //reset_baseband(); //QiangKai: Eagle can not reset, all RF baseband setting will lost(But Kite/Vulture must add this)
    /**********************************************************************************/

    reset_sn_nesn();

    reg_rf_ll_ctrl_1 &= ~(FLD_RF_BRX_SN_INIT | FLD_RF_BRX_NESN_INIT | FLD_RF_BTX_SN_INIT | FLD_RF_BTX_NESN_INIT);
    reg_rf_ll_ctrl_1 |= blms_pconn->local_sn << 4 | blms_pconn->local_nesn << 5 | blms_pconn->local_sn << 6 | blms_pconn->local_nesn << 7;

#if (OPTIMIZE_INSERT_EMPTY_EN)
    blms_pconn->local_last_sn = blms_pconn->local_sn;
#endif

    //------------ push to HW fifo ----------------------------
    blt_llms_pushToHWfifo();
    blms_pconn->save_flg = 0;
}

_attribute_ram_code_ void blt_llms_update_fifo_sw(void)
{
/* for more insurance, if rx irq lost(empty packet), operation here would get correct sn/nesn/tx_rptr
    1. IF RX IRQ never lose, this save operation is not mandatory
    2. If no RX DMA rewrite bug, this operation never cause problem(only for Eagle)
    3. To avoid RX DMA rewrite bug, using rf_len to find CRC, stopping FSM for boundary RX may cause error RX data passing CRC check, leading to post data lost
    4. Using software method to avoid above bug, drop all boundary RX(regardless of correct or error data), must make sure SN/NESN/TX_RPTR not updated by
       potential correct boundary RX.
    */
#if (0)
    if (!blms_pconn->save_flg) {
        blms_pconn->local_sn         = reg_rf_ll_pid_l & FLD_RF_SN;
        blms_pconn->local_nesn       = (reg_rf_ll_pid_h & FLD_RF_NESN) >> 4;
        blms_pconn->conn_dma_tx_rptr = (HAL_REG_RF_DMA_FIFO_TX_RPTR & FLD_DMA_RPTR_MASK);
    }
#endif

#if (HW_AES_CCM_ALG_EN) //only salve need

                        //      if(blms_pconn->hw_aes_ccm_flag)
    if (reg_rf_tx_mode2 & FLD_TLK_CRYPT_ENABLE) {
        blt_save_aes_ccm_para(blms_pconn);
        blms_pconn->lastTxLen_flag = reg_ccm_control;
        reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
    }
#endif
    //------------  save TX FIFO pointer --------------
    blt_llms_saveTxfifoRptr();
}

ble_sts_t blc_hci_disconnect(hci_disconnect_cmdParam_t *pCmdParam)
{
    u8 status = HCI_ERR_UNKNOWN_CONN_ID;

    if (pCmdParam->connHandle & (BLT_ACL_CONN_HANDLE)) {
        status = blc_ll_disconnect(pCmdParam->connHandle, pCmdParam->reason);
    }
#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    else if (pCmdParam->connHandle & BLT_CIS_HANDLE) {
        if (ll_cis_cmd_task_cb) {
            status = ll_cis_cmd_task_cb(HCI_CMD_DISCONNECT, pCmdParam, NULL); // blt_cis_cmd_process_task  blc_ll_cis_disconnect
        }
    }
#endif


#if (IUT_HCI_LOG_EN)
    u8 temp_buffer[4];
    smemcpy(temp_buffer, pCmdParam, 3);
    temp_buffer[3] = status;
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Disconnect", temp_buffer, 4);
#endif


    return status;
}

ble_sts_t blc_ll_disconnect(u16 connHandle, u8 reason)
{
    u8            idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc  = (st_ll_conn_t *)&blms[idx];

    if (blt_ll_isAclhdlInvalid(pc->acl_conHandle)) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }


    if (pc->conn_termin_union.local_terminate) { // if one terminate is sending, no push new terminate
        return BLE_SUCCESS;
    }


/* CISes associated with ACL connection, should also send terminate(if no terminate on CIS) */
#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
    if (ll_cis_conn_mlp_task_cb && pc->cisEstablish_msk) {
        pc->reason_tmp = reason;
        ll_cis_conn_mlp_task_cb(FLAG_ACL_LOCAL_DISCONNECT, pc); // blt_cis_conn_mainloop_task
    }
#endif


    u8 pkt_terminate[] = {0x03, 0x02, LL_TERMINATE_IND, reason};              //reason 13
    if (blt_llmsPushLlCtrlPkt(connHandle, LL_TERMINATE_IND, pkt_terminate)) { //push terminate packet OK
        pc->connMarkTxFifoWptr = pc->tx_wptr;

        if (reason >= HCI_ERR_REMOTE_USER_TERM_CONN && reason <= HCI_ERR_REMOTE_DEVICE_TERM_CONN_POWER_OFF) {
            pc->conn_termin_union.local_terminate = HCI_ERR_CONN_TERM_BY_LOCAL_HOST;
        } else {
            pc->conn_termin_union.local_terminate = reason;
        }

        pc->conn_terminate_tick = clock_time() | 1;
    } else {
        return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
    }


    return BLE_SUCCESS;
}


#if (DBG_DECRYPTION_ERR_EN)
u8 AMICbuf[4][64] = {0};
u8 AMICcnt        = 0;
#endif


#if (BLUETOOTH_VER == BLUETOOTH_VER_5_0)
u8 const cmd_length_array[26] = {
    12,
    8,
    2,
    23,
    13,
    1,
    1,
    2,
    9,
    9,
    1,
    1,
    6,
    2,
    9,
    24,
    24,
    3,
    1,
    1,
    9,
    9,
    3,
    3,
    5,
    3,
};
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_1)
    #error "to de done"
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_2)
u8 const cmd_length_array[38] = {
    12,
    8,
    2,
    23,
    13,
    1,
    1,
    2,
    9,
    9,
    1,
    1,
    6,
    2,
    9,
    24,
    24,
    3,
    1,
    1,
    9,
    9,
    3,
    3,
    5,
    3,
    2,
    2,
    35,
    2,
    2,
    36,
    9,
    16,
    4,
    4,
    5,
    5,
};
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_3)
//max opcode LL_CHANNEL_STATUS_IND 0x29 = 41, so define 42
//  u8 const cmd_length_array[42] =  {  12,  8,  2, 23, 13,  1,  1,  2,  9,  9,
//                                       1,  1,  6,  2,  9, 24, 24,  3,  1,  1,
//                                       9,  9,  3,  3,  5,  3,  2,  2, 35,  2,
//                                       2, 36,  9, 16,  4,  4,  5,  5, 11, 11,
//                                       4, 11,
//                                   };

//todo add len fanqh
u8 const cmd_length_array[59] = {
    12,
    8,
    2,
    23,
    13,
    1,
    1,
    2,
    9,
    9,
    1,
    1,
    6,
    2,
    9,
    24,
    24,
    3,
    1,
    1,
    9,
    9,
    3,
    3,
    5,
    3,
    2,
    2,
    35,
    2,
    2,
    36,
    9,
    16,
    4,
    4,
    5,
    5,
    11,
    11,
    4,
    11,
    43,
    0,
    0,
    21,
    26,
    26,
    28,
    2,
    29,
    22,
    19,
    5,
    1,
    73,
    13,
    21,
    5 //channel sounding
};
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_4)

u8 const cmd_length_array[43] = {
    12,
    8,
    2,
    23,
    13,
    1,
    1,
    2,
    9,
    9,
    1,
    1,
    6,
    2,
    9,
    24,
    24,
    3,
    1,
    1,
    9,
    9,
    3,
    3,
    5,
    3,
    2,
    2,
    35,
    2,
    2,
    36,
    9,
    16,
    4,
    4,
    5,
    5,
    11,
    11,
    4,
    11,
    43,
};
#elif (BLUETOOTH_VER == BLUETOOTH_VER_6_0)

//todo add len fanqh
u8 const cmd_length_array[61] = {
    12,
    8,
    2,
    23,
    13,
    1,
    1,
    2,
    9,
    9,
    1,
    1,
    6,
    2,
    9,
    24,
    24,
    3,
    1,
    1,
    9,
    9,
    3,
    3,
    5,
    3,
    2,
    2,
    35,
    2,
    2,
    36,
    9,
    16,
    4,
    4,
    5,
    5,
    11,
    11,
    4,
    11,
    43,
    27,
    27,
    21,
    26,
    26,
    28,
    2,
    29,
    22,
    19,
    5,
    1,
    73,
    13,
    21,
    5, //channel sounding
    8, //frame space req
    6, //frame space rsp
};
#elif (BLUETOOTH_VER == BLUETOOTH_VER_6_X)

u8 const cmd_length_array[256] = {
    /*0-63*/
    12,    8,    2,    23,    13,    1,    1,    2,    9,    9,    1,    1,    6,    2,    9,    24,
    24,    3,    1,    1,    9,    9,    3,    3,    5,    3,    2,    2,    35,    2,    2,    36,
    9,    16,    4,    4,    5,    5,    11,    11,    4,    11,    43,    0,    0,    21,    26,    26,
    28,    2,    29,    22,    19,    5,    1,    73,    13,    21,    5, //channel sounding, 59
    0,    0,    0,    0,    0,
    /*64-127*/
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /*128-191*/
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    /*192-255*/
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    16,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    9,    3,    3,    16,    49,    0,    0,    0,    0,    0
};
#else
    #error "to de done"
#endif


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    int
    blt_llms_main_loop_data(u16 connHandle, u8 *raw_pkt)
{
    u8            conn_idx       = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc             = (st_ll_conn_t *)&blms[conn_idx];
    u8            ll_master_role = connHandle & BLM_CONN_HANDLE; //role:  1: master, 0 slave

    if (raw_pkt[DMA_RFRX_OFFSET_RFLEN]) {                        //rf_len != 0

        int st = 0;

        //////////// decrypt packet if security on ////////////////////////
        if (pc->crypt.enable && raw_pkt[0] != BLM_ENC_PKT_SKIP_ENCRYPT) {
            if (pc->crypt.mic_fail) {
                return 0;
            }

#if (HW_AES_CCM_ALG_EN)
            if (!pc->hw_aes_ccm_flag) {
#endif

#if (DBG_DECRYPTION_ERR_EN)
                smemcpy(AMICbuf[AMICcnt], raw_pkt, raw_pkt[5] + 17);
#endif

#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
                if (pc->authPayloadEnable) {
                    pc->authPayloadTick = clock_time() | 1;
                }
#endif
                /*
             * ll_ccm_enc: Master role must use 1, Slave role must use 0;
             * ll_ccm_dec: Master role must use 0, Slave role must use 1;
             */

                /* Eagle test data by SiHui
             * BTBLE dual_mode SDK, 20200925
             * 48M, HW AES, some code in SRAM, 140~167 uS ,  120~130 uS later(some code in cache)
             * 48M, HW AES,   no code in SRAM, 250 uS first, 140~180 uS later(some code in cache)
             *
             * BLE multi_conn SDK, 20210702
             * 32M, HW AES< all code in SRAM, 122 ~ 128 uS
             */
                aes_enc_dec_busy = 1;
                st               = aes_ll_ccm_decryption((llPhysChnPdu_t *)(raw_pkt + DMA_RFRX_OFFSET_HEADER), !ll_master_role, CRYPT_NONCE_TYPE_ACL, &pc->crypt);
                aes_enc_dec_busy = 0;

#if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
                if (pc->conn_pkt_dec_pending) {
                    u32 r       = irq_disable();    //IRQ boundary protect
                    u8 *dec_pkt = NULL;
                    if (pc->conn_pkt_dec_pending) { //to prevent cache optimize, AA_blms must be volatile
                        dec_pkt                  = pc->conn_pkt_dec_pending;
                        pc->conn_pkt_dec_pending = 0;
                        aclConn_param.updateCmd_pending &= ~BIT(pc->acl_conIndex - LL_MAX_ACL_CEN_NUM);
                    }
                    irq_restore(r);

                    if (dec_pkt) {
                        /* Eagle:32M, 180~220uS, 20210720
                         */
                        st = blt_ll_conn_chn_phy_update(pc, dec_pkt);
                    }
                }
#endif

#if (HW_AES_CCM_ALG_EN)
            } else {
                raw_pkt[5] -= 4;
            }
#endif
        }


        if (st) //decrypt err
        {
            pc->crypt.mic_fail = 1;

            if (pc->crypt.st >= MS_LL_ENC_RSP_T && pc->crypt.st < MS_LL_ENC_START_RSP_T) {
                /*
                 * Disconnect the local connection directly, and report the HOST disconnection
                 * event, and it will not send LL_Terminate_IND to the peer device.
                 */
                blt_ll_enc_proc_disConnect(connHandle, HCI_ERR_CONN_TERM_MIC_FAILURE);

                my_dump_str_data(SMP_DBG_EN, "report host decrypt err1", &pc->crypt.st, 1);
            } else {
                //LL/SEC/PER/BI-04-C    [Peripheral MIC Failure: Corrupted Header]
                blt_ll_enc_proc_disConnect(connHandle, HCI_ERR_CONN_TERM_MIC_FAILURE);

                my_dump_str_data(SMP_DBG_EN, "send term rsn: MIC failure", &pc->crypt.st, 1);
            }


            BLMS_ERR_DEBUG(DBG_DECRYPTION_ERR_EN, 0x88990000);


            return 0;
        }

#if (DBG_DECRYPTION_ERR_EN)
        AMICcnt++;
        AMICcnt &= 3;
#endif

        rf_packet_ll_control_t *pll    = (rf_packet_ll_control_t *)(raw_pkt + DMA_RFRX_OFFSET_HEADER);
        u8                      type   = pll->type & 3;
        u8                      opcode = pll->opcode;
        u16                     dat[8];

#if (1) //  check
        if (ll_master_role) {
    #if (LL_ACL_CEN_EN)
            if (pc->crypt.st >= MS_LL_ENC_RSP_T && pc->crypt.st < MS_LL_ENC_START_REQ) {
                /*
                     * From the time the Master sends the LL_ENC_REQ to the LL_ENC_RSP sent by the Slave, if it receives LL_TERMINATE_IND,
                     * LL_REJECT_IND, LL_START_ENC_RSP, LL_START_ENC_REQ, LL_ENC_RSP, LL_REJECT_EXT_INDs, and empty packets from the Slave
                     * during this period Control packet, you need to wait until the Master receives the LL_START_ENC_RSP (the ll_encryption
                     * process ends) sent by the Slave before processing. If it is a data packet, report it to Host
                     */
                bool spPktflt = (type == LLID_DATA_START || type == LLID_DATA_CONTINUE) || //data pkt
                                ((type == LLID_CONTROL) && ((opcode == LL_ENC_RSP || opcode == LL_START_ENC_REQ || opcode == LL_START_ENC_RSP) ||
                                                            opcode == LL_TERMINATE_IND || opcode == LL_REJECT_IND_EXT || opcode == LL_REJECT_IND));
                if (!spPktflt) {
                    blm_pkt_pending_t *enc_pkt_pending = &blmsMasterEncPktPending[conn_idx];
                    //hold pending pkts
                    smemcpy(enc_pkt_pending->buff[enc_pkt_pending->wptr & enc_pkt_pending->mask], raw_pkt, raw_pkt[DMA_RFRX_OFFSET_RFLEN] + 6);
                    enc_pkt_pending->buff[enc_pkt_pending->wptr][0] = BLM_ENC_PKT_SKIP_ENCRYPT;
                    enc_pkt_pending->wptr++;
                    enc_pkt_pending->wptr &= enc_pkt_pending->mask;

                    //printf("pending pkt hold\n");
                    my_dump_str_data(SMP_DBG_EN, "pending pkt hold", 0, 0);
                    //array_printf(raw_pkt, raw_pkt[5]+6);
                    return 0;
                }
            }
            //When the Master receives the ENC_RSP sent by the Slave, and before receiving the LL_START_ENC_RSP sent by the Slave
            else if (pc->crypt.st >= MS_LL_ENC_START_REQ && pc->crypt.st < MS_LL_ENC_START_RSP_T) {
                /*
                     * during this time, if it receives the LL_TERMINATE_IND, LL_REJECT_IND, LL_START_ENC_RSP, LL_START_ENC_REQ, LL_ENC_RSP,
                     * LL_sJECT_PDU, and other packages other than the LL_REJECT_IND, LL_START_ENC_RSP, LL_START_ENC_REQ, LL_ENC_RSP,
                     * LL_REJECT_IND_EXT, and other packages Disconnect directly and report Mic fail to Host without sending LL_TERMINATE_IND
                     * to Slave.
                     */
                bool spPktflt = (type == LLID_CONTROL) && ((opcode == LL_ENC_RSP || opcode == LL_START_ENC_REQ || opcode == LL_START_ENC_RSP) ||
                                                           opcode == LL_TERMINATE_IND || opcode == LL_REJECT_IND_EXT || opcode == LL_REJECT_IND);
                if (!spPktflt) { //unexpected pdu, current ACL connection shall terminate.
                    pc->crypt.mic_fail = 1;
                    blt_ll_enc_proc_disConnect(connHandle, HCI_ERR_CONN_TERM_MIC_FAILURE);

                    my_dump_str_data(SMP_DBG_EN, "M: unexpt pdu, cur conn shall term", 0, 0);

                    return 0;
                }
            }
    #endif
        } else { //ll slave role:
    #if (LL_ACL_PER_EN)
            //The time between receiving LL_ENC_REQ sent by Master and receiving LL_START_ENC_RSP sent by Master on the Slave side.
            if (pc->crypt.st >= MS_LL_ENC_REQ_T && pc->crypt.st < MS_LL_ENC_START_RSP_T) {
                /*
                     * The slave side only allows to receive LL_TERMINATE_IND, LL_REJECT_IND, LL_START_ENC_RSP, LL_ENC_REQ, LL_REJECT_EXT_IND
                     * and empty packets sent by the Master during the period between receiving LL_ENC_REQ sent by the Master and receiving
                     * LL_START_ENC_RSP sent by the Master, otherwise it will be disconnected directly (not Will send out LL_TERMINATE_IND),
                     * and report the disconnection event to the Host layer, instead of sending LL_TERMINATE_IND to the Master.
                     */
                if ((type == LLID_CONTROL) && (opcode == LL_ENC_REQ || opcode == LL_START_ENC_RSP ||
                                               opcode == LL_TERMINATE_IND || opcode == LL_REJECT_IND_EXT || opcode == LL_REJECT_IND)) {
                } else { //unexpected pdu, current ACL connection shall terminate.
                    pc->crypt.mic_fail = 1;
                    blt_ll_enc_proc_disConnect(connHandle, HCI_ERR_CONN_TERM_MIC_FAILURE);

                    my_dump_str_data(SMP_DBG_EN, "S: unexpt pdu, cur conn shall term", 0, 0);

                    return 0;
                }
            }
    #endif
        }
#endif

        //------------------ LL control --------------------------------------------
        if (type == LLID_CONTROL) {
            my_dump_str_data(DBG_LL_CTRL_LOG_EN, ">> opcode", &opcode, 1);

            u8 *pLlCtrlPkt = (raw_pkt + DMA_RFRX_OFFSET_HEADER);

            int errcode = BLE_SUCCESS;

            //Check whether the received LL_Ctrl_Cmd meets the requirements based on the swapd feature set (Req/Rsp)
            if (opcode > LL_CMD_MAX || pll->rf_len != cmd_length_array[opcode]) {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] opcode > LL_CMD_MAX", 0, 0);
                errcode = LL_ERR_UNKNOWN_OPCODE;
            }
#if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE) // LL Control Opcode: 0x00/0x01/0x18
            else if (opcode == LL_CONNECTION_UPDATE_REQ) {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_CONNECTION_UPDATE_REQ", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            } else if (opcode == LL_CHANNEL_MAP_REQ) {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_CHANNEL_MAP_REQ", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            } else if (opcode == LL_SUBRATE_IND) {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_SUBRATE_IND", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            }
#else
            else if (opcode == LL_CONNECTION_UPDATE_REQ) {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_CONNECTION_UPDATE_REQ", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }

                rf_packet_connect_upd_req_t *pUpdate   = (rf_packet_connect_upd_req_t *)pLlCtrlPkt;
                s16                          diff_inst = pUpdate->instant - pc->conn_inst;
                if (diff_inst > 0) {
                    if (!(pc->conn_update_union.update_mark & (CONN_UPDATE_CMD | CONN_UPDATE_PENDING | CONN_UPDATE_NEARBY))) {
    #if (DBG_SLAVE_CONN_UPDATE)
                        if (conn_idx == 4) {
                            DBG_C HN4_HIGH;
                        } else if (conn_idx == 5) {
                            DBG_C HN5_HIGH;
                        } else if (conn_idx == 6) {
                            DBG_C HN6_HIGH;
                        } else {
                            DBG_C HN7_HIGH;
                        }
    #endif

                        pc->conn_para_inst_next = pUpdate->instant;

                        pc->conn_winsize_next      = pUpdate->winSize;
                        pc->conn_offset_next       = pUpdate->winOffset;
                        pc->conn_intvl_next_n_1m25 = pUpdate->interval;
                        pc->conn_latency_next      = pUpdate->latency;
                        pc->conn_timeout_next      = pUpdate->timeout;

                        pc->conn_inst_next               = pc->conn_para_inst_next;
                        pc->conn_update_union.update_cmd = 1;                 //for slave PM
                        pc->conn_update_union.update_mark |= CONN_UPDATE_CMD; //set flag at last is more safer, consider IRQ problem
    #if (LL_ACL_PER_EN && ACL_SLAVE_PM_LATENCY_EN)
        #if (ACL_SLAVE_NON_MODULAR)
                        st_lls_conn_t *ps     = (st_lls_conn_t *)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
                        u32            r      = irq_disable();
                        ps->latency_available = 0;
                        irq_restore(r);
        #else
            #error "to be done"
        #endif
    #endif
                    }
                } else {
                    errcode = HCI_ERR_INSTANT_PASSED;

                    //my_dump_str_u32s(DBG_SLAVE_CONN_UPDATE, "inst passed 1", conn_idx, pc->conn_para_inst_next, pc->conn_inst, pc->conn_update_union.update_mark&CONN_UPDATE_PARAM_MASK);
                    //BLMS_ERR_DEBUG(DBG_SLAVE_CONN_UPDATE, 0xDD020000);
                }
            } else if (opcode == LL_CHANNEL_MAP_REQ) //update channel map
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_CHANNEL_MAP_REQ", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                } else {
                    rf_packet_chm_upd_req_t *pReq = (rf_packet_chm_upd_req_t *)pLlCtrlPkt;

                    pc->conn_map_inst_next = pReq->instant;
                    s16 diff_inst          = pc->conn_map_inst_next - pc->conn_inst;


                    if (diff_inst > 0) {
                        if (!(pc->conn_update_union.update_mark & CONN_UPDATE_MAP)) {
                            smemcpy(pc->nextChn.chmTbl, pReq->chm, 5);

    #if (LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2)
                            if (pc->conn_chnsel) {
                                csa2_calculateMapInfo(&pc->nextChn);
                            } else
    #endif
                            {
                                /* when calculate new channel map in BRX/BTX start, 70uS is used, so calculate table in advance */
                                blt_csa1_calculateChannelTable(pReq->chm, pc->conn_chn_hop, pc->nextChn.rempChmTbl);
                            }

                            pc->conn_inst_next               = pc->conn_map_inst_next;
                            pc->conn_update_union.update_cmd = 1;                 //for slave PM
                            pc->conn_update_union.update_mark |= CONN_UPDATE_MAP; //set flag at last is more safer, consider IRQ problem

    /* for ACL slave low power sleep latency */
    #if (LL_ACL_PER_EN && ACL_SLAVE_PM_LATENCY_EN)
        #if (ACL_SLAVE_NON_MODULAR)
                            st_lls_conn_t *ps     = (st_lls_conn_t *)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
                            u32            r      = irq_disable();
                            ps->latency_available = 0;
                            irq_restore(r);
        #else
            #error "to be done"
        #endif
    #endif

    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
                            if (ll_cis_map_update_cb && pc->cisEstablish_msk) {
                                u32 r            = irq_disable();
                                u32 trigger_tick = pc->ap_tick_mark + (pc->conn_map_inst_next - pc->conn_inst_mark) * pc->conn_intvl_tick;
                                irq_restore(r);
                                ll_cis_map_update_cb(trigger_tick, pc); // blt_cis_update_chn_map
                            }
    #endif
                        }
                    } else {
                        //terminate with reason: instant passed
                        errcode = HCI_ERR_INSTANT_PASSED;
                    }
                }
            }

    #if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
            else if (opcode == LL_SUBRATE_IND) {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_SUBRATE_IND", &(pll->opcode), pll->rf_len);
                if (ll_master_role || !ll_acl_subrate_ctrl_handler) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                } else {
                    ll_acl_subrate_ctrl_handler(pc, LL_SUBRATE_IND, raw_pkt); //blt_ll_subrate_control_pdu_process
                }
            }
    #endif
#endif
            else if (opcode == LL_TERMINATE_IND) // termination
            {
                rf_packet_ll_terminate_t *pTermInd   = (rf_packet_ll_terminate_t *)pLlCtrlPkt;
                pc->conn_termin_union.peer_terminate = pTermInd->reason;
                pc->conn_termin_union.terminate_pending = 0;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_TERMINATE_IND", &(pll->opcode), pll->rf_len);
                my_dump_str_data(LL_CTRL_LOG_EN, "[ACL][CTRL] LL_TERMINATE_IND tx", &pTermInd->reason, 1);
            }
#if (LL_FEATURE_ENABLE_LE_ENCRYPTION)      //LL Control Opcode: 0x03/0x04/0x05/0x06,  0x0A/0x0B
            else if (opcode == LL_ENC_REQ) //ll_enc_req rx =>  tx (rsp) => tx(start)
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_ENC_REQ", &(pll->opcode), pll->rf_len);
                pc->ll_rsp_timeout_tick = clock_time() | 1;

                if ((!ll_master_role) && (pc->crypt.st == MS_LL_ENC_OFF || pc->crypt.st == MS_LL_ENC_PAUSE_RSP)) { //slave role

                    pc->crypt.st = MS_LL_ENC_REQ_T;

                    rf_packet_ll_enc_req_t *pLlEncReq = (rf_packet_ll_enc_req_t *)pLlCtrlPkt;
                    //SDKs and IVs random generate every new pair
                    generateRandomNum(8, (u8 *)pc->enc_skds);
                    generateRandomNum(4, (u8 *)&pc->enc_ivs);

                    blt_ll_setEncryptionBusy(connHandle, 1);
                    blt_ll_smpPushEncPkt(connHandle, LL_ENC_RSP);
                    pc->ll_rsp_timeout_tick = clock_time() | 1;

                    smemcpy(pc->enc_skdm, pLlEncReq->skdm, 8);
                    smemcpy(&pc->enc_ivm, pLlEncReq->ivm, 4);

                    if (blt_llms_ltk_request) { //smp.c: bls_smp_llGetLtkReq
                        blt_llms_ltk_request(connHandle, pLlEncReq->rand, pLlEncReq->ediv);
                    }

                    if (hci_le_eventMask & HCI_LE_EVT_MASK_LONG_TERM_KEY_REQUEST) {
                        u8                              result[13];
                        hci_le_longTermKeyRequestEvt_t *pEvt = (hci_le_longTermKeyRequestEvt_t *)result;

                        pEvt->subEventCode = HCI_SUB_EVT_LE_LONG_TERM_KEY_REQUESTED;
                        pEvt->connHandle   = connHandle;
                        pEvt->ediv         = pLlEncReq->ediv;
                        smemcpy(pEvt->random, pLlEncReq->rand, 8);

                        blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 13);

                        my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] LTK Request Evt", 0, 0);
                    }
                }
            } else if (opcode == LL_ENC_RSP) //ll_enc_rsp tx
            {
                pc->ll_rsp_timeout_tick = clock_time() | 1;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_ENC_RSP", &(pll->opcode), pll->rf_len);
                if (ll_master_role && (pc->crypt.st == MS_LL_ENC_RSP_T)) {
                    rf_packet_ll_enc_rsp_t *pLlEncRsp = (rf_packet_ll_enc_rsp_t *)pLlCtrlPkt;
                    smemcpy(&pc->enc_ivs, pLlEncRsp->ivs, 4);
                    smemcpy(pc->enc_skds, pLlEncRsp->skds, 8);

                    aes_ll_ccm_encryption_init(pc->crypt.ltk, pc->enc_skdm, pc->enc_skds, (u8 *)&pc->enc_ivm, (u8 *)&pc->enc_ivs, &pc->crypt);

                    pc->crypt.st = MS_LL_ENC_START_REQ;
                }
            } else if (opcode == LL_START_ENC_REQ) //ll_start_enc_req tx
            {
                pc->ll_rsp_timeout_tick = clock_time() | 1;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_START_ENC_REQ", &(pll->opcode), pll->rf_len);
                if (ll_master_role && (pc->crypt.st == MS_LL_ENC_START_REQ)) {
                    pc->crypt.enable = 1; //master start encryption

    #if (HW_AES_CCM_ALG_EN)
                    pc->hw_aes_ccm_flag = 1;

                    if (blt_ll_getRealTxFifoNumber(pc->acl_conHandle)) {
                        BLMS_ERR_DEBUG(BLMS_DEBUG_EN, 0xBB01);
                    }
    #endif
                    aclConn_param.last_master_mic_en = 1;               //special use for CIS master
                    blt_ll_smpPushEncPkt(connHandle, LL_START_ENC_RSP); //start encryption
                    pc->crypt.st            = MS_LL_ENC_START_REQ_T;
                    pc->ll_rsp_timeout_tick = clock_time() | 1;
                }
            } else if (opcode == LL_START_ENC_RSP) //ll_start_enc_rsp rx => tx (rsp)
            {
                pc->ll_rsp_timeout_tick = 0;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_START_ENC_RSP", &(pll->opcode), pll->rf_len);
                if (pc->crypt.st == MS_LL_ENC_START_REQ_T) {
                    pc->crypt.st = MS_LL_ENC_START_RSP_T;

                    pc->crypt.st = MS_LL_ENC_OFF; // flag encryption end

                    if (pc->encryption_tmp_st & MS_CONN_ENC_REFRESH_T) {
                        pc->encryption_evt |= MS_CONN_ENC_REFRESH;
                    } else {
                        pc->encryption_evt |= MS_CONN_ENC_CHANGE;
                    }

                    if (!ll_master_role) { //slave role

    #if (HW_AES_CCM_ALG_EN)
                        pc->hw_aes_ccm_flag = 1;
                        pc->lastTxLen_flag  = 0;


                            //                          if(blt_ll_getRealTxFifoNumber(pc->acl_conHandle)){
                            //                                  BLMS_ERR_DEBUG(BLMS_DEBUG_EN, 0xBB01);
                            //                              }
    #endif
                        blt_ll_smpPushEncPkt(connHandle, LL_START_ENC_RSP);
                    }

                    blt_ll_setEncryptionBusy(connHandle, 0);

    #if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
                    if (pc->authPayloadEnable) {
                        pc->authPayloadTick = clock_time() | 1;
                    }
    #endif

    #if OS_SUP_EN
                    /* pending data needs to be processed after the encryption is complete. */
                    //blt_ll_sem_give();
                    if (blt_os_semCountIncrement_cb) {
                        blt_os_semCountIncrement_cb();
                    }
    #endif
                }
            } else if (opcode == LL_PAUSE_ENC_REQ) //ll_pause_enc_req rx => tx (rsp)
            {
                pc->ll_rsp_timeout_tick = clock_time() | 1;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PAUSE_ENC_REQ", &(pll->opcode), pll->rf_len);

                if ((!ll_master_role) && pc->crypt.st == MS_LL_ENC_OFF) {
                    pc->crypt.st = MS_LL_ENC_PAUSE_RSP_T;
                    //send response encrypted

                    blt_ll_smpPushEncPkt(connHandle, LL_PAUSE_ENC_RSP);
                    pc->ll_rsp_timeout_tick = clock_time() | 1;
                    pc->crypt.enable        = 0;

                    blt_ll_setEncryptionBusy(connHandle, 0);

    #if (LL_PAUSE_ENC_FIX_EN)
                    if (ll_encryption_pause_cb) {
                        ll_encryption_pause_cb(connHandle);
                    }
    #endif

    #if (HW_AES_CCM_ALG_EN)
                    pc->hw_aes_ccm_flag = 0;
                    reg_rf_tx_mode2 &= ~FLD_TLK_CRYPT_ENABLE;
    #endif
                }
            } else if (opcode == LL_PAUSE_ENC_RSP) //ll_pause_enc_rsp rx
            {
                pc->ll_rsp_timeout_tick = 0;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PAUSE_ENC_RSP", &(pll->opcode), pll->rf_len);
                if (pc->crypt.st == MS_LL_ENC_PAUSE_RSP_T) {
                    pc->encryption_tmp_st |= MS_CONN_ENC_REFRESH_T;
                    pc->crypt.st = MS_LL_ENC_PAUSE_RSP;

                    if (ll_master_role) {
                        pc->crypt.enable = 0;
                    }
                }
            }
#else
    #if !BQB_TEST_EN
            else if (opcode == LL_ENC_REQ) {
                if (!ll_master_role) { //slave role NOT support encryption procedure
                    //TODO: LL_(ext)_reject(rsn: HCI_ERR_UNSUPPORTED_REMOTE_FEATURE)
                }
            }
    #endif
#endif
            else if (opcode == LL_UNKNOWN_RSP) //LL Control Opcode: 0x07
            {
                rf_packet_ll_unknown_rsp_t *pLlUnknownRsp = (rf_packet_ll_unknown_rsp_t *)pLlCtrlPkt;
                pc->ll_rsp_timeout_tick                   = 0;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_UNKNOWN_RSP", &(pll->opcode), pll->rf_len);
                if (pLlUnknownRsp->unknownType == LL_SLAVE_FEATURE_REQ) {
                    if (!ll_master_role && pc->remoteFeatureReq) {
                        //clear remote feature set
                        u8 remoteFeature[8]   = {0};
                        pc->ll_remoteFeature0 = 0;
                        pc->ll_remoteFeature1 = 0;

                        if (pc->remoteFeatureReq & FEATURE_HCI_REPORT) {
                            hci_le_readRemoteFeaturesComplete_evt(HCI_ERR_UNSUPPORTED_REMOTE_FEATURE, connHandle, remoteFeature);
                        }
                        pc->remoteFeatureReq = 0;
                    }
                }
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
                else if (pLlUnknownRsp->unknownType == LL_LENGTH_REQ) {

                    ll_data_extension_t *pExt_data = &pc->ext_data;

                    //pExt_data->connRemoteMaxTxOctets = pExt_data->connRemoteMaxRxOctets = MAX_OCTETS_DATA_LEN_27;
                    pExt_data->connEffectiveMaxTxOctets = pExt_data->connEffectiveMaxRxOctets = MAX_OCTETS_DATA_LEN_27;
                    //blt_p_event_callback (BLT_EV_FLAG_DATA_LENGTH_EXCHANGE, (u8 *)&pExt_data->connEffectiveMaxRxOctets, 12);
                }
#endif
#if (LL_FEATURE_ENABLE_CONNECTED_ISO)
                else if (pLlUnknownRsp->unknownType == LL_CIS_REQ) {
                    //TODO: Cancel the creation of CIG
                }
#endif
            } else if (opcode == LL_FEATURE_REQ || opcode == LL_SLAVE_FEATURE_REQ || opcode == LL_FEATURE_RSP) //LL Control Opcode: 0x08/0x09/0x0E
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_FEATURE_REQ/RSP/LL_SLAVE_FEATURE_REQ", &(pll->opcode), pll->rf_len);
                if (opcode == LL_FEATURE_REQ && ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                } else if (opcode == LL_SLAVE_FEATURE_REQ &&
                           (!ll_master_role || !(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE << 3)))) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                } else if (opcode == LL_FEATURE_RSP && !(pc->remoteFeatureReq & FEATURE_WAIT_FEAT_RSP)) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                } else {
                    rf_packet_ll_feature_exg_t *pLlFeatureExg = (rf_packet_ll_feature_exg_t *)pLlCtrlPkt;
                    //feature set low 4 byte
                    pc->ll_remoteFeature0 = MAKE_U32(pLlFeatureExg->featureSet[3], pLlFeatureExg->featureSet[2], pLlFeatureExg->featureSet[1], pLlFeatureExg->featureSet[0]);
                    pc->ll_remoteFeature1 = MAKE_U32(pLlFeatureExg->featureSet[7], pLlFeatureExg->featureSet[6], pLlFeatureExg->featureSet[5], pLlFeatureExg->featureSet[4]);

#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
                    if (LL_FEATURE_ENABLE_LE_PING && (pc->ll_remoteFeature0 & LL_FEATURE_MASK_LE_PING)) {
                        pc->authPayloadEnable = 1;
                    }
#endif


                    pc->llcp_flag.bit.ll_feat_exg_flag = 1;

                    if (opcode == LL_FEATURE_RSP) { //feature_rsp
                        if (pc->remoteFeatureReq & FEATURE_HCI_REPORT) {
                            hci_le_readRemoteFeaturesComplete_evt(BLE_SUCCESS, connHandle, pLlFeatureExg->featureSet);
                        }
                        pc->remoteFeatureReq    = 0;
                        pc->ll_rsp_timeout_tick = 0;
                    } else { //feature_req or slave_feature_req
                        if (!pc->FeatureRsp) {
                            pc->FeatureRsp = 1;
                        }
                    }
                }
            } else if (opcode == LL_VERSION_IND) //LL Control Opcode: 0x0C
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_VERSION_IND", &(pll->opcode), pll->rf_len);
                if (!pc->llcp_flag.bit.ll_ver_ind_flag) {
#if 0 //not used
                        rf_packet_version_ind_t* pLlVersionInd = (rf_packet_version_ind_t*)pLlCtrlPkt;
                        pLlVersionInd->versNr;
                        pLlVersionInd->compId;
                        pLlVersionInd->subVersNr;
#endif

                    rf_packet_version_ind_t versionInd;
                    versionInd.type      = LLID_CONTROL;
                    versionInd.rf_len    = 6;
                    versionInd.opcode    = LL_VERSION_IND;
                    versionInd.versNr    = BLUETOOTH_VER;
                    versionInd.compId    = VENDOR_ID;
                    versionInd.subVersNr = BLUETOOTH_VER_SUBVER;
                    blt_llmsPushLlCtrlPkt(connHandle, LL_VERSION_IND, (u8 *)&versionInd);
                    pc->llcp_flag.bit.ll_ver_ind_flag = 2;
                } else {
                    pc->ll_rsp_timeout_tick = 0; ///if our device send LL_VERSION_IND
                }

                if (pc->llcp_flag.bit.ll_ver_ind_flag == 1) {
                    pc->llcp_flag.bit.ll_ver_ind_flag = 2;

                    rf_packet_version_ind_t *pBuf = (rf_packet_version_ind_t *)pLlCtrlPkt;

                    if ((hci_eventMask & HCI_EVT_MASK_READ_REMOTE_VERSION_INFORMATION_COMPLETE)) {
                        u8                           result[8];
                        hci_readRemVerInfoCmplEvt_t *pEvt = (hci_readRemVerInfoCmplEvt_t *)result;
                        pEvt->status                      = BLE_SUCCESS;
                        pEvt->connHandle                  = connHandle;
                        pEvt->verNr                       = pBuf->versNr;
                        pEvt->compId                      = pBuf->compId;
                        pEvt->subVerNr                    = pBuf->subVersNr;
                        blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_READ_REMOTE_VER_INFO_COMPLETE, result, 8);
                    }
                }
            } else if (opcode == LL_REJECT_IND) //LL Control Opcode: 0x0D
            {
                rf_packet_ll_reject_ind_t *pLlRejectInd = (rf_packet_ll_reject_ind_t *)pLlCtrlPkt;
                pc->ll_rsp_timeout_tick                 = 0;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_REJECT_IND", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    pc->crypt.mic_fail = pLlRejectInd->errCode;
                    pc->crypt.st       = MS_LL_ENC_OFF;
                    blt_ll_setEncryptionBusy(connHandle, 0);

                    pc->encryption_evt |= MS_CONN_ENC_CHANGE;
                    pc->encryption_evt |= (pc->crypt.mic_fail) & 0x3F; //Attention: fail reason can not exceed 0x3F
                }
            }
#if (LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE)
            else if (opcode == LL_CONNECTION_PARAM_REQ || opcode == LL_CONNECTION_PARAM_RSP) //LL Control Opcode: 0x0F/0x10
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_CONNECTION_PARAM_REQ/RSP", &(pll->opcode), pll->rf_len);
            }
#endif
#if (LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION)
            else if (opcode == LL_REJECT_IND_EXT) //LL Control Opcode: 0x11
            {
                rf_packet_ll_reject_ext_ind_t *pLlRejectExtInd = (rf_packet_ll_reject_ext_ind_t *)pLlCtrlPkt;
                pc->ll_rsp_timeout_tick                        = 0;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_REJECT_IND_EXT", &(pll->opcode), pll->rf_len);
                if (pLlRejectExtInd->rejectOpcode == LL_ENC_REQ) {
                    if (ll_master_role) {
                        pc->crypt.mic_fail = pLlRejectExtInd->errCode;
                        pc->crypt.st       = MS_LL_ENC_OFF;
                        blt_ll_setEncryptionBusy(connHandle, 0);

                        pc->encryption_evt |= MS_CONN_ENC_CHANGE;
                        pc->encryption_evt |= (pc->crypt.mic_fail) & 0x3F; //Attention: fail reason can not exceed 0x3F
                    }
                }
    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
                else if (pLlRejectExtInd->rejectOpcode == LL_CIS_REQ && ll_master_role) {
                    if (ll_cis_master_ctrl_handler) {
                        errcode = ll_cis_master_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_cis_master_control_pdu_process
                    }
                }
    #endif
    #if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
                else if (pLlRejectExtInd->rejectOpcode == LL_CIS_RSP && !ll_master_role) {
                    if (ll_cis_slave_ctrl_handler) {
                        errcode = ll_cis_slave_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_cis_slave_control_pdu_process
                    }
                }
    #endif
    #if (LL_FEATURE_ENABLE_POWER_CONTROL)
                else if (pLlRejectExtInd->rejectOpcode == LL_POWER_CONTROL_REQ) {
                    if (ll_pcl_ctrl_handler) {
                        errcode = ll_pcl_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_pclControlPduProc
                    }
                }
    #endif
    #if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
                else if (pLlRejectExtInd->rejectOpcode == LL_CHANNEL_REPORTING_IND && ll_master_role) {
                    if (ll_acl_chnclass_ctrl_handler) {
                        errcode = ll_acl_chnclass_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_chnclassControlPduProc
                    }
                } else if (pLlRejectExtInd->rejectOpcode == LL_CHANNEL_STATUS_IND && !ll_master_role) {
                    if (ll_acl_chnclass_ctrl_handler) {
                        errcode = ll_acl_chnclass_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_chnclassControlPduProc
                    }
                }
    #endif
    #if (LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
                else if (pLlRejectExtInd->rejectOpcode == LL_FRAME_SPACE_REQ) {
                    if (ll_fsu_ctrl_handler) {
                        errcode = ll_fsu_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_fsuControlPduProc
                    }
                }
    #endif
    #if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
                if (pLlRejectExtInd->rejectOpcode == LL_PHY_REQ && pc->aclRole == ACL_ROLE_PERIPHERAL) {
                    hci_le_phyUpdateComplete_evt(connHandle, pLlRejectExtInd->errCode, pc->connPhyCtrl.conn_cur_phy);
                }
    #endif
    #if LL_FEATURE_ENABLE_CHANNEL_SOUNDING
                else if ((pLlRejectExtInd->rejectOpcode >= LL_CS_SEC_RSP) && (pLlRejectExtInd->rejectOpcode <= LL_CS_SEC_REQ)) {
                    if (ll_chn_sounding_ctrl_handler) {
                        errcode = ll_chn_sounding_ctrl_handler(pc, opcode, pLlCtrlPkt); //blt_ll_cs_ctrl_pdu_proc
                    }
                }
    #endif
            }
#endif
#if (LL_FEATURE_ENABLE_LE_PING)
            else if (opcode == LL_PING_REQ)   //LL Control Opcode: 0x12
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PING_REQ", &(pll->opcode), pll->rf_len);
                dat[0] = 0x0103;              //type, len
                dat[1] = LL_PING_RSP;
                blt_llmsPushLlCtrlPkt(connHandle, LL_PING_RSP, (u8 *)dat);
            } else if (opcode == LL_PING_RSP) //LL Control Opcode: 0x13
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PING_RSP", &(pll->opcode), pll->rf_len);
            }
#endif
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
            else if (opcode == LL_LENGTH_REQ || opcode == LL_LENGTH_RSP) //LL Control Opcode: 0x14/0x15
            {
                ll_data_extension_t *pExt_data  = &pc->ext_data;
                int                  change     = 1;
                uint8_t              paramValid = true;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_LENGTH_REQ/RSP", &(pll->opcode), pll->rf_len);
                u16 remoteRxLen  = pll->dat[0] | (pll->dat[1] << 8);
                u16 remoteTxLen  = pll->dat[4] | (pll->dat[5] << 8);
                u16 remoteRxTime = pll->dat[2] | (pll->dat[3] << 8);
                u16 remoteTxTime = pll->dat[6] | (pll->dat[7] << 8);

                u16 new_effectiveMaxRx     = pExt_data->connEffectiveMaxRxOctets;
                u16 new_effectiveMaxTx     = pExt_data->connEffectiveMaxTxOctets;
                u16 new_effectiveMaxRxTime = pExt_data->connEffectiveMaxRxTime;
                u16 new_effectiveMaxTxTime = pExt_data->connEffectiveMaxTxTime;

                /* Check LL_LENGTH_RSP parameter */
                if (opcode == LL_LENGTH_RSP) {
                    //my_dump_str_data(0, "[I] Rx LL_LENGTH_RSP remote Rx", &remoteRxLen,2);
                    //my_dump_str_data(0, "[I] Rx LL_LENGTH_RSP local Tx", &pExt_data->connMaxTxOctets,2);

                    if (remoteRxLen < 27 || remoteRxLen > 251 || remoteTxLen < 27 || remoteTxLen > 251 ||
                        remoteRxTime < 328 || (remoteRxTime > 2120 && remoteRxTime < 2704) || remoteRxTime > 17040 ||
                        remoteTxTime < 328 || (remoteTxTime > 2120 && remoteTxTime < 2704) || remoteTxTime > 17040) {
                        change     = 0;
                        paramValid = false;
                    }

                    if (paramValid) {
                        new_effectiveMaxRx = min(pExt_data->connMaxRxOctets, remoteTxLen);
                        new_effectiveMaxTx = min(pExt_data->connMaxTxOctets, remoteRxLen);

    #if (LL_FEATURE_ENABLE_LE_CODED_PHY)
                        if (pc->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED) {
                            if (remoteRxTime < LL_PDU_TIME_1M(remoteRxLen) || remoteTxTime < LL_PDU_TIME_1M(remoteTxLen)) {
                                new_effectiveMaxRx     = pExt_data->connEffectiveMaxRxOctets;
                                new_effectiveMaxTx     = pExt_data->connEffectiveMaxTxOctets;
                                new_effectiveMaxRxTime = pExt_data->connEffectiveMaxRxTime;
                                new_effectiveMaxTxTime = pExt_data->connEffectiveMaxTxTime;
                            } else {
                                new_effectiveMaxRxTime = 2704;
                                new_effectiveMaxTxTime = 2704;
                            }
                        } else
    #endif
                        {
                            new_effectiveMaxRxTime = min(LL_PDU_TIME_1M(pExt_data->connMaxRxOctets), remoteTxTime);
                            new_effectiveMaxTxTime = min(LL_PDU_TIME_1M(pExt_data->connMaxTxOctets), remoteRxTime);
                        }
                    }
                    pExt_data->connMaxTxRxOctets_req = 0;
                    pc->ll_rsp_timeout_tick          = 0;
                } else { //LL_LENGTH_REQ
                    new_effectiveMaxRx = max(27, min(pExt_data->supportedMaxRxOctets, remoteTxLen));
                    new_effectiveMaxTx = max(27, min(pExt_data->supportedMaxTxOctets, remoteRxLen));

                    if (remoteTxTime > LL_PDU_TIME_1M(251)) {
                        remoteTxTime = LL_PDU_TIME_1M(251);
                    } else if (remoteTxTime < LL_PDU_TIME_1M(27)) {
                        remoteTxTime = LL_PDU_TIME_1M(27);
                    }

                    if (remoteRxTime > LL_PDU_TIME_1M(251)) {
                        remoteRxTime = LL_PDU_TIME_1M(251);
                    } else if (remoteRxTime < LL_PDU_TIME_1M(27)) {
                        remoteRxTime = LL_PDU_TIME_1M(27);
                    }

    #if (LL_FEATURE_ENABLE_LE_CODED_PHY)
                    if (pc->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED) {
                        new_effectiveMaxRxTime = 2704;
                        new_effectiveMaxTxTime = 2704;
                    } else
    #endif
                    {
                        new_effectiveMaxRxTime = min(LL_PDU_TIME_1M(pExt_data->supportedMaxRxOctets), remoteTxTime);
                        new_effectiveMaxTxTime = min(LL_PDU_TIME_1M(pExt_data->supportedMaxTxOctets), remoteRxTime);
                    }

                    blt_ll_exchangeDataLength(connHandle, LL_LENGTH_RSP, pExt_data->supportedMaxTxOctets);
                }

                //my_dump_str_data(0, "[I] effect len rx", &new_effectiveMaxRx,2);
                //my_dump_str_data(0, "[I] effect len tx", &new_effectiveMaxTx,2);
                //my_dump_str_data(0, "[I] effect len rx time", &new_effectiveMaxRxTime,2);
                //my_dump_str_data(0, "[I] effect len tx time", &new_effectiveMaxTxTime,2);

                change = (pExt_data->connEffectiveMaxRxOctets != new_effectiveMaxRx) ||
                         (pExt_data->connEffectiveMaxTxOctets != new_effectiveMaxTx) ||
                         pExt_data->connEffectiveMaxRxTime != new_effectiveMaxRxTime ||
                         pExt_data->connEffectiveMaxTxTime != new_effectiveMaxTxTime;

                pExt_data->connEffectiveMaxRxOctets = new_effectiveMaxRx;
                pExt_data->connEffectiveMaxTxOctets = new_effectiveMaxTx;
                pExt_data->connEffectiveMaxRxTime   = new_effectiveMaxRxTime;
                pExt_data->connEffectiveMaxTxTime   = new_effectiveMaxTxTime;

                if (change && (hci_le_eventMask & HCI_LE_EVT_MASK_DATA_LENGTH_CHANGE)) {
                    u8                            result[11];
                    hci_le_dataLengthChangeEvt_t *pEvt = (hci_le_dataLengthChangeEvt_t *)result;

                    pEvt->subEventCode = HCI_SUB_EVT_LE_DATA_LENGTH_CHANGE;
                    pEvt->connHandle   = connHandle; ///
                    pEvt->maxTxOct     = pExt_data->connEffectiveMaxTxOctets;
                    pEvt->maxTxtime    = pExt_data->connEffectiveMaxTxTime;
                    pEvt->maxRxOct     = pExt_data->connEffectiveMaxRxOctets;
                    pEvt->maxRxtime    = pExt_data->connEffectiveMaxRxTime;

                    blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 11);
                }

                //blt_p_event_callback (BLT_EV_FLAG_DATA_LENGTH_EXCHANGE, (u8 *)&pExt_data->connEffectiveMaxRxOctets, 12);
            }
#endif
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
            else if (opcode == LL_PHY_REQ || opcode == LL_PHY_RSP) //LL Control Opcode: 0x16
            {
                pc->ll_rsp_timeout_tick = clock_time() | 1;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PHY_REQ/IND", &(pll->opcode), pll->rf_len);
                if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_2M_PHY << 8))) {//todo hdt
                    blt_llms_unknownRsp(connHandle, opcode);
                } else {
                    rf_pkt_ll_phy_req_rsp_t *pReq = (rf_pkt_ll_phy_req_rsp_t *)pLlCtrlPkt;

                    u8 comm_phy = pReq->tx_phys & pReq->rx_phys; //support symmetric PHYs only; support 1M/2M/Coded PHY all
                    if (ll_master_role) {                        //master role.need to send LL_PHY_UPDATE_IND to slave
                        //master initiated
                        if (opcode == LL_PHY_REQ) {
                            if (pc->ll_upd_flag) {
                                if (pc->ll_upd_flag & PHY_UPD_FLAG) {
                                    blt_llms_rejectInd(connHandle, opcode, HCI_ERR_LMP_ERR_TRANSACTION_COLLISION, 1);

                                } else {
                                    blt_llms_rejectInd(connHandle, opcode, HCI_ERR_DIFFERENT_TRANSACTION_COLLISION, 1);
                                }
                                return 0;
                            }
                            comm_phy = comm_phy & 0x1f;
                        } else {
                            comm_phy = comm_phy & pc->connPhyCtrl.conn_prefer_phys;
                        }

                        comm_phy &= ~BIT(pc->connPhyCtrl.conn_cur_phy - 1);

                        //in case that comm_phy include more than 1 PHY
                        if (comm_phy & PHY_PREFER_1M) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_1M;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_1M;
                        } else if (comm_phy & PHY_PREFER_2M) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_2M;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_2M;
                        } else if (comm_phy & PHY_PREFER_CODED) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_CODED;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_CODED;
                        } else if (comm_phy & PHY_PREFER_HDT) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_HDT;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_HDT;
                        } else { //no PHY Update
                            pc->connPhyCtrl.conn_updatePhy = 0;
                            pc->connPhyCtrl.conn_next_phy  = 0;
                        }

                        pc->connPhyCtrl.phy_update_pending = 1;

                    } else {                        ///slave role. need to send LL_PHY_RSP to master
                        if (opcode == LL_PHY_REQ) { ///slave not receive LL_PHY_RSP
                            if (comm_phy == 0)      // if rx and tx have none common PHYs, select 1M PHY
                            {
                                comm_phy = BLE_PHY_1M;
                            }

                            if (comm_phy == BIT(pc->connPhyCtrl.conn_cur_phy - 1)) {
                            } else {
                                comm_phy &= ~BIT(pc->connPhyCtrl.conn_cur_phy - 1);
                                if (comm_phy & PHY_PREFER_1M) {
                                    comm_phy = PHY_PREFER_1M;
                                } else if (comm_phy & PHY_PREFER_2M) {
                                    comm_phy = PHY_PREFER_2M;
                                } else if (comm_phy & PHY_PREFER_CODED) {
                                    comm_phy = PHY_PREFER_CODED;
                                } else if (comm_phy & PHY_PREFER_HDT) {
                                    comm_phy = PHY_PREFER_HDT;
                                } else {
                                    comm_phy = BIT(pc->connPhyCtrl.conn_cur_phy - 1);
                                }
                            }

                            dat[0] = 0x0303;
                            dat[1] = LL_PHY_RSP | (comm_phy << 8); ///send LL_PHY_RSP to master
                            dat[2] = comm_phy;
                            blt_llmsPushLlCtrlPkt(connHandle, LL_PHY_RSP, (u8 *)dat);
                            pc->ll_rsp_timeout_tick = clock_time() | 1;

                            pc->connPhyCtrl.conn_prefer_phys       = comm_phy;
                            pc->connPhyCtrl.conn_next_CI           = pc->connPhyCtrl.conn_cur_CI;
                            pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 1;
                        } else { //slave not receive LL_PHY_RSP
                            errcode = LL_ERR_UNKNOWN_OPCODE;
                        }
                        ///slave not process command LL_PHY_RSP
                    }
                }
            }
    #if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
            else if (opcode == LL_PHY_UPDATE_IND_V2) //LL Control Opcode: 0x18
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PHY_UPDATE_IND V2", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            }
    #else
            else if (opcode == LL_PHY_UPDATE_IND_V2) //LL Control Opcode: 0x18
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PHY_UPDATE_IND V2", &(pll->opcode), pll->rf_len);
                ///actually here should add if( !(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_2M_PHY<<8) ) ){blt_llms_unknownRsp(connHandle, opcode);}
                if (!ll_master_role) { ///only slave can receive this command

                    rf_pkt_ll_phy_update_ind_v2_t *pUpdt = (rf_pkt_ll_phy_update_ind_v2_t *)pLlCtrlPkt;
                    pc->ll_rsp_timeout_tick           = 0;

                    u8 comm_phy = pUpdt->m_to_s_phy & pUpdt->s_to_m_phy;

                    if (comm_phy == 0) {
                        if (pc->ll_upd_flag) {
                            pc->irq_event1_union.phy_update_evt = 1;
                        }
                        pc->ll_upd_flag                        = 0;
                        pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 0;
                        return 0;
                    }

                    pc->conn_phy_inst_next = pUpdt->instant0 | (pUpdt->instant1 << 8);

                    s16 diff_inst = pc->conn_phy_inst_next - pc->conn_inst;
                    if (diff_inst > 0) {
                        if (!(pc->conn_update_union.update_mark & CONN_PHY_UPDATE_IND_CMD)) {
                            if (comm_phy & PHY_PREFER_1M) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_1M;
                            } else if (comm_phy & PHY_PREFER_2M) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_2M;
                            } else if (comm_phy & PHY_PREFER_CODED) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_CODED;
                            } else if (comm_phy & PHY_PREFER_2M_2BT) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_2M_2BT;
                            } else if (comm_phy & PHY_PREFER_HDT) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_HDT;
                            } else { //no PHY Update
                                pc->connPhyCtrl.conn_next_phy = pc->connPhyCtrl.conn_cur_phy;
                            }

                            pc->conn_inst_next               = pc->conn_phy_inst_next;
                            pc->conn_update_union.update_cmd = 1; //for slave PM
                            pc->conn_update_union.update_mark |= CONN_PHY_UPDATE_IND_CMD;
        #if (LL_ACL_PER_EN && ACL_SLAVE_PM_LATENCY_EN)
            #if (ACL_SLAVE_NON_MODULAR)
                            st_lls_conn_t *ps     = (st_lls_conn_t *)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
                            u32            r      = irq_disable();
                            ps->latency_available = 0;
                            irq_restore(r);
            #else
                #error "to be done"
            #endif
        #endif
                        }
                    } else //todo wrap around
                    {
                        errcode = HCI_ERR_INSTANT_PASSED;
                    }

                } else {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            }
    #endif
#else
#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
            else if (opcode == LL_PHY_REQ || opcode == LL_PHY_RSP) //LL Control Opcode: 0x16
            {
                pc->ll_rsp_timeout_tick = clock_time() | 1;
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PHY_REQ/IND", &(pll->opcode), pll->rf_len);
                if (!(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_2M_PHY << 8))) {
                    blt_llms_unknownRsp(connHandle, opcode);
                } else {
                    rf_pkt_ll_phy_req_rsp_t *pReq = (rf_pkt_ll_phy_req_rsp_t *)pLlCtrlPkt;

                    u8 comm_phy = pReq->tx_phys & pReq->rx_phys; //support symmetric PHYs only; support 1M/2M/Coded PHY all
                    if (ll_master_role) {                        //master role.need to send LL_PHY_UPDATE_IND to slave
                        //master initiated
                        if (opcode == LL_PHY_REQ) {
                            if (pc->ll_upd_flag) {
                                if (pc->ll_upd_flag & PHY_UPD_FLAG) {
                                    blt_llms_rejectInd(connHandle, opcode, HCI_ERR_LMP_ERR_TRANSACTION_COLLISION, 1);

                                } else {
                                    blt_llms_rejectInd(connHandle, opcode, HCI_ERR_DIFFERENT_TRANSACTION_COLLISION, 1);
                                }
                                return 0;
                            }
                            comm_phy = comm_phy & 0x07;
                        } else {
                            comm_phy = comm_phy & pc->connPhyCtrl.conn_prefer_phys;
                        }

                        comm_phy &= ~BIT(pc->connPhyCtrl.conn_cur_phy - 1);

                        //in case that comm_phy include more than 1 PHY
                        if (comm_phy & PHY_PREFER_1M) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_1M;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_1M;
                        } else if (comm_phy & PHY_PREFER_2M) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_2M;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_2M;
                        } else if (comm_phy & PHY_PREFER_CODED) {
                            pc->connPhyCtrl.conn_updatePhy = PHY_PREFER_CODED;
                            pc->connPhyCtrl.conn_next_phy  = BLE_PHY_CODED;
                        } else { //no PHY Update
                            pc->connPhyCtrl.conn_updatePhy = 0;
                            pc->connPhyCtrl.conn_next_phy  = 0;
                        }

                        pc->connPhyCtrl.phy_update_pending = 1;

                    } else {                        ///slave role. need to send LL_PHY_RSP to master
                        if (opcode == LL_PHY_REQ) { ///slave not receive LL_PHY_RSP
                            if (comm_phy == 0)      // if rx and tx have none common PHYs, select 1M PHY
                            {
                                comm_phy = BLE_PHY_1M;
                            }

                            if (comm_phy == BIT(pc->connPhyCtrl.conn_cur_phy - 1)) {
                            } else {
                                comm_phy &= ~BIT(pc->connPhyCtrl.conn_cur_phy - 1);
                                if (comm_phy & PHY_PREFER_1M) {
                                    comm_phy = PHY_PREFER_1M;
                                } else if (comm_phy & PHY_PREFER_2M) {
                                    comm_phy = PHY_PREFER_2M;
                                } else if (comm_phy & PHY_PREFER_CODED) {
                                    comm_phy = PHY_PREFER_CODED;
                                } else {
                                    comm_phy = BIT(pc->connPhyCtrl.conn_cur_phy - 1);
                                }
                            }

                            dat[0] = 0x0303;
                            dat[1] = LL_PHY_RSP | (comm_phy << 8); ///send LL_PHY_RSP to master
                            dat[2] = comm_phy;
                            blt_llmsPushLlCtrlPkt(connHandle, LL_PHY_RSP, (u8 *)dat);
                            pc->ll_rsp_timeout_tick = clock_time() | 1;

                            pc->connPhyCtrl.conn_prefer_phys       = comm_phy;
                            pc->connPhyCtrl.conn_next_CI           = pc->connPhyCtrl.conn_cur_CI;
                            pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 1;
                        } else { //slave not receive LL_PHY_RSP
                            errcode = LL_ERR_UNKNOWN_OPCODE;
                        }
                        ///slave not process command LL_PHY_RSP
                    }
                }
            }
    #if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
            else if (opcode == LL_PHY_UPDATE_IND) //LL Control Opcode: 0x18
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PHY_UPDATE_IND", &(pll->opcode), pll->rf_len);
                if (ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            }
    #else
            else if (opcode == LL_PHY_UPDATE_IND) //LL Control Opcode: 0x18
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_PHY_UPDATE_IND", &(pll->opcode), pll->rf_len);
                ///actually here should add if( !(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_LE_2M_PHY<<8) ) ){blt_llms_unknownRsp(connHandle, opcode);}
                if (!ll_master_role) { ///only slave can receive this command

                    rf_pkt_ll_phy_update_ind_t *pUpdt = (rf_pkt_ll_phy_update_ind_t *)pLlCtrlPkt;
                    pc->ll_rsp_timeout_tick           = 0;

                    u8 comm_phy = pUpdt->m_to_s_phy & pUpdt->s_to_m_phy;

                    if (comm_phy == 0) {
                        if (pc->ll_upd_flag) {
                            pc->irq_event1_union.phy_update_evt = 1;
                        }
                        pc->ll_upd_flag                        = 0;
                        pc->llcp_flag.bit.ll_phy_req_rcvd_flag = 0;
                        return 0;
                    }

                    pc->conn_phy_inst_next = pUpdt->instant0 | (pUpdt->instant1 << 8);

                    s16 diff_inst = pc->conn_phy_inst_next - pc->conn_inst;
                    if (diff_inst > 0) {
                        if (!(pc->conn_update_union.update_mark & CONN_PHY_UPDATE_IND_CMD)) {
                            if (comm_phy & PHY_PREFER_1M) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_1M;
                            } else if (comm_phy & PHY_PREFER_2M) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_2M;
                            } else if (comm_phy & PHY_PREFER_CODED) {
                                pc->connPhyCtrl.conn_next_phy = BLE_PHY_CODED;
                            } else { //no PHY Update
                                pc->connPhyCtrl.conn_next_phy = pc->connPhyCtrl.conn_cur_phy;
                            }

                            pc->conn_inst_next               = pc->conn_phy_inst_next;
                            pc->conn_update_union.update_cmd = 1; //for slave PM
                            pc->conn_update_union.update_mark |= CONN_PHY_UPDATE_IND_CMD;
        #if (LL_ACL_PER_EN && ACL_SLAVE_PM_LATENCY_EN)
            #if (ACL_SLAVE_NON_MODULAR)
                            st_lls_conn_t *ps     = (st_lls_conn_t *)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
                            u32            r      = irq_disable();
                            ps->latency_available = 0;
                            irq_restore(r);
            #else
                #error "to be done"
            #endif
        #endif
                        }
                    } else //todo wrap around
                    {
                        errcode = HCI_ERR_INSTANT_PASSED;
                    }

                } else {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            }
    #endif
#endif
#endif

#if (LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS)
            else if (opcode == LL_MIN_USED_CHN_IND) //LL Control Opcode: 0x19
            {
                tlkapi_send_string_data(BLC_LL_LOG_EN || (stkLog_mask & STK_LOG_LL_RX), "[LL][RX] LL_MIN_USED_CHN_IND", &(pll->opcode), pll->rf_len);
                if (!ll_master_role) {
                    errcode = LL_ERR_UNKNOWN_OPCODE;
                }
            }
#endif
#if LL_FEATURE_ENABLE_CONNECTION_CTE_REQUEST
            else if (opcode == LL_CTE_REQ) //LL Control Opcode: 0x1A
            {

            }
#endif
#if LL_FEATURE_ENABLE_CONNECTION_CTE_RESPONSE
            else if (opcode == LL_CTE_RSP) //LL Control Opcode: 0x1B
            {

            }
#endif
#if (LL_FEATURE_ENABLE_PAST)
            else if (opcode == LL_PERIODIC_SYNC_IND || opcode == LL_PERIODIC_SYNC_WR_IND) //LL Control Opcode: 0x1C
            {
                if (ll_past_ctrl_handler) {
                    errcode = ll_past_ctrl_handler(pc, opcode, pLlCtrlPkt);               // blt_ll_pastControlPduProc
                }
            }
#endif
#if LL_FEATURE_ENABLE_SLEEP_CLK_ACCURACY_UPDATE
            else if (opcode == LL_CLOCK_ACCURACY_REQ)   //LL Control Opcode: 0x1D
            {

            } else if (opcode == LL_CLOCK_ACCURACY_RSP) //LL Control Opcode: 0x1E
            {
            }
#endif
#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER)
            //only master can receive
            else if ((opcode == LL_CIS_RSP || opcode == LL_CIS_TERMINATE_IND) && ll_master_role) {
                if (ll_cis_master_ctrl_handler) {
                    errcode = ll_cis_master_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_cis_master_control_pdu_process
                }
            }
#endif
#if (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE)
            //only slave can receive
            else if ((opcode == LL_CIS_REQ || opcode == LL_CIS_IND || opcode == LL_CIS_TERMINATE_IND) && !ll_master_role) {
                if (ll_cis_slave_ctrl_handler) {
                    errcode = ll_cis_slave_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_cis_slave_control_pdu_process
                }
            }
#endif
#if (LL_FEATURE_ENABLE_POWER_CONTROL)
            else if ((opcode == LL_POWER_CONTROL_REQ || opcode == LL_POWER_CONTROL_RSP || opcode == LL_POWER_CHANGE_IND)) {
                if (ll_pcl_ctrl_handler) {
                    errcode = ll_pcl_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_pclControlPduProc
                }
            }
#endif
#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
            else if (opcode == LL_CHANNEL_REPORTING_IND && !ll_master_role) {
                if (ll_acl_chnclass_ctrl_handler) {
                    errcode = ll_acl_chnclass_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_chnclassControlPduProc
                }
            } else if (opcode == LL_CHANNEL_STATUS_IND && ll_master_role) {
                if (ll_acl_chnclass_ctrl_handler) {
                    errcode = ll_acl_chnclass_ctrl_handler(pc, opcode, pLlCtrlPkt); // blt_ll_chnclassControlPduProc
                }
            }
#endif
#if LL_FEATURE_ENABLE_CONNECTION_SUBRATING

            else if (opcode == LL_SUBRATE_REQ && ll_master_role) {

                if (ll_acl_subrate_process_req) {
                    ll_acl_subrate_process_req(pc, pLlCtrlPkt); //blt_ll_subrate_control_pdu_process
                }
            }
#endif

#if (LL_FEATURE_ENABLE_CHANNEL_SOUNDING)

            else if ((opcode >= LL_CS_SEC_RSP) && (opcode <= LL_CS_TERMINATE_RSP)) {
                ll_chn_sounding_ctrl_handler(pc, opcode, pLlCtrlPkt); //blt_ll_cs_ctrl_pdu_proc
            }
#endif

#if (LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
            else if(opcode == LL_FRAME_SPACE_REQ || opcode == LL_FRAME_SPACE_RSP){
                if(ll_fsu_ctrl_handler){
                    errcode = ll_fsu_ctrl_handler(pc, opcode, pLlCtrlPkt); //blt_ll_fsuControlPduProc
                }
            }
#endif
            else { //Control PDUs not supported, send "LL_UNKNOWN_RSP"
                blt_llms_unknownRsp(connHandle, opcode);
            }


            //for all opcode above, if err_code, send ll_unknown or ll_reject_ind_ext
            //TODO: consider push TX FIFO may not success
            if (errcode) {
                pc->ll_rsp_timeout_tick = 0;

                if (LL_ERR_UNKNOWN_OPCODE == errcode) {
                    blt_llms_unknownRsp(connHandle, opcode);
                } else if (HCI_ERR_INSTANT_PASSED == errcode) {
                    blc_ll_disconnect(connHandle, HCI_ERR_INSTANT_PASSED);
                } else { // public reject process
                    my_dump_str_data(LL_CTRL_LOG_EN, "[ACL][CTRL] LL_REJECT tx", &errcode, 1);
                    if (!blt_llms_rejectInd(connHandle, opcode, errcode, 1)) {
                        //if push TX fail, hold data in next mainLoop process
                        blt_acl_send_ext_reject(pc, opcode, errcode);
                    }
                }
            }


        } //end of if (type == LLID_CONTROL)
        //------------- LL L2CAP single packet ---------------------------------
        else if (type == 2 || type == 1) {
            if (blc_hci_data_handler) {
                blc_hci_data_handler(connHandle, (raw_pkt + DMA_RFRX_OFFSET_HEADER)); ////blc_l2cap_pktHandler
            }
        }
    } //end of  if (raw_pkt[DMA_RFRX_OFFSET_RFLEN])


    return 0;
}

_attribute_noinline_ int blt_ll_procConnectionEvent(u16 connHandle, st_ll_conn_t *pc)
{
    u8  idx = connHandle & CONN_IDX_MASK;
    u16 dat16[32];
    u8 *p8 = (u8 *)dat16;

#if (LL_ASYNC_LEA_EN)
    connHandle = pc->async_lea_link ? pc->async_lea_link : connHandle;
#endif
    //---------- create connection event ------------------------------------------
    if (pc->irq_event1_union.connect_evt) {
        //connection complete CallBack must executed before HCI   connection complete event
        if (ll_connComplete_handler) {                          // blms_gap_conn_complete_handler  //32M M/S take T: 140us/245us
            ll_connComplete_handler(connHandle, conn_req_info); ///blt_gap_conn_complete_handler
        }


        /*
        If LE Enhanced Connection Complete event is unmasked and LE Connection Complete event is unmasked,
        only the LE Enhanced Connection Complete event is sent when a new connection has been created. */
        if ((hci_le_eventMask & HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE) || (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_ENHANCED_CONNECTION_COMPLETE_V2)) {
            u8 peerRpa[BLE_ADDR_LEN]  = {0};
            u8 localRpa[BLE_ADDR_LEN] = {0};
            u8 peer_addressType       = pc->conn_peerAddrType;

#if (LL_FEATURE_ENABLE_PRIVACY)
            if (pc->conn_peerUseRpa) {
                smemcpy(peerRpa, pc->conn_peerRpa, BLE_ADDR_LEN);
                peer_addressType |= PEERATYPE_IDENTITY_MASK; //attention that report type should be 2/3 for RPA resolve OK to IDA
            }
    #if (LL_FEATURE_ENABLE_LOCAL_RPA)
            if (pc->conn_locUseRpa) {
                smemcpy(localRpa, pc->conn_localRpa, BLE_ADDR_LEN);
            }
    #endif
#endif

#if (LL_ASYNC_LEA_EN)
            blt_async_connStateCallback(connHandle, 1);
#endif

            if (hci_le_eventMask_2 & HCI_LE_EVT_MASK_2_ENHANCED_CONNECTION_COMPLETE_V2) {
                hci_le_enhancedConnectionComplete_evt_v2(pc->create_conn_status, connHandle, pc->aclRole, peer_addressType, pc->conn_peerAddr, localRpa, peerRpa, pc->conn_intvl_n_1m25, pc->conn_latency, pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS), pc->conn_sca, 0x00, 0xffff);
            } else { //HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE
                hci_le_enhancedConnectionComplete_evt(pc->create_conn_status, connHandle, pc->aclRole, peer_addressType, pc->conn_peerAddr, localRpa, peerRpa, pc->conn_intvl_n_1m25, pc->conn_latency, pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS), pc->conn_sca);
            }

            blmsParam.create_connection = 0;

        } else if (hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTION_COMPLETE) {
            if (!(pc->irq_event1_union.connect_evt & ENHANCED_CONN_FLAG_ALL)) {
#if (LL_ASYNC_LEA_EN)
                blt_async_connStateCallback(connHandle, 1);
#endif
                /* peer address: address appear on RF packet, but can not do any process(such as RPA -> IDA )*/
                hci_le_connectionComplete_evt(BLE_SUCCESS, connHandle, pc->aclRole, pc->conn_peerPktA_type, pc->conn_peerPktA, pc->conn_intvl_n_1m25, pc->conn_latency, pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS), pc->conn_sca);
            }
        }

        //generate this after (enhanced ) connection complete event
        if (hci_le_eventMask & HCI_LE_EVT_MASK_CHANNEL_SELECTION_ALGORITHM) {
            hci_le_channel_selection_algorithm_evt(pc->acl_conHandle, pc->conn_chnsel);
        }

        ////////////////////////////////////////////////
        if (pc->aclRole == ACL_ROLE_PERIPHERAL) { //ACL slave
#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
            if (IS_EXTENDED_ADV_VALID) {
                //TODO optimize, function move to hci_event.c
                blt_extAdv_terminateEvt(connHandle, pc->adv_handle, pc->num_completeTerminateEvt, CONN_STATUS_COMPLETE); //connection state
            }
#endif
        } else {                                                                                                         // ACL master
            aclConn_param.last_master_mic_en = 0;                                                                        //special use for CIS master
        }

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
        bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
        bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
        bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
    #endif
        if (snif_used) {
            u8 reason = BLE_SUCCESS;
            blt_ll_acl_sniffer_sync_result(connHandle, &reason);
        }
#endif

        pc->irq_event1_union.connect_evt = 0; //must clear it at the end
        blmsParam.connectEvt_mask &= ~(1 << idx);

        // reset some parameters when connect
        pc->encryption_evt = 0;
    }


    //---------- disconnect event ------------------------------------------
    if (pc->irq_event1_union.disconn_evt) {
#if (LL_FEATURE_ENABLE_PRIVACY && LL_FEATURE_ENABLE_LOCAL_RPA)
        //attention: in ADV packet, only advA RPA need refresh when disconnect, initA RPA no need,
        //so here depend on "LL_FEATURE_ENABLE_LOCAL_RPA"
        if (pc->conn_locUseRpa) { //refresh RPA when disconnect
            blt_ll_resolvRefreshRpa(pc->pRslvlst_conn);
        }
#endif

        if (ll_connTerminate_handler) { // blt_gap_conn_terminate_handler
            ll_connTerminate_handler(connHandle, &pc->conn_termin_union.terminate_reason);
        }


#if (FIX_CIS_CREATE_CMD_ERR_DUE_TO_PREVIOUS_CREATE_NOT_CLEAR_WHEN_ACL_TERMINATE)
    #if (LL_FEATURE_ENABLE_CONNECTED_ISO)
        //should execute before ACL disconnect event
        if (ll_cig_mst_mlp_task_cb) {
            ll_cig_mst_mlp_task_cb(FLAG_ACL_MLP_DISCONNECT_EVT, pc); // blt_cig_mst_mainloop_task
        }

    #endif
#endif


#if HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
        extern void hci_resetCurHostAvailBufNum(void);
        hci_resetCurHostAvailBufNum();
#endif

        if (hci_eventMask & HCI_EVT_MASK_DISCONNECTION_COMPLETE) {
#if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
            if (pc->conn_termin_union.terminate_reason == HCI_ERR_CONN_FAILED_TO_ESTABLISH && aclConn_etbsh.cusConnEtbsh_en && pc->aclRole == ACL_ROLE_CENTRAL) {
                if (aclConn_etbsh.crtConn_cur_cnt < (aclConn_etbsh.crtConn_retry_num + 1)) {
                    aclConn_etbsh.re_create_conn_tick = clock_time() | 1;
                } else {
                    hci_tlk_createConnectionFail_evt(CONNECT_FAIL, aclConn_etbsh.crtConn_cur_cnt);
                    aclConn_etbsh.crtConn_cur_cnt     = 0;
                    aclConn_etbsh.re_create_conn_tick = 0;
                }
            } else
#endif
            {
#if (LL_ASYNC_LEA_EN)
                blt_async_connStateCallback(connHandle, 0);
#endif
                hci_disconnectionComplete_evt(BLE_SUCCESS, connHandle, pc->conn_termin_union.terminate_reason);
            }
        }

#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE && LL_RSSI_SNIFFER_MASTER_ENABLE)
        bool snif_used = (ll_acl_sniffer_mst_irq_task_cb || ll_acl_sniffer_slv_irq_task_cb) ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_MASTER_ENABLE)
        bool snif_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
    #elif (LL_RSSI_SNIFFER_SLAVE_ENABLE)
        bool snif_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
    #endif
        if (snif_used) {
            blt_ll_acl_sniffer_sync_result(connHandle, &pc->conn_termin_union.terminate_reason);
        }
#endif

        pc->conn_termin_union.terminate_reason = 0; //clear, important, PM code will use termin_pack to decide latency used
        pc->irq_event1_union.disconn_evt       = 0; //must clear it at the end
        blmsParam.disconnEvt_mask &= ~(1 << idx);


        // clear some parameters when disconnect
        //      pc->conn_established_tick = 0;
        pc->blt_tx_pkt_hold = 0;

#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)

        ll_data_extension_t *pExt_data = &pc->ext_data;

        pExt_data->connEffectiveMaxTxOctets = MAX_OCTETS_DATA_LEN_27;
        pExt_data->connEffectiveMaxRxOctets = MAX_OCTETS_DATA_LEN_27;
        pExt_data->connMaxTxRxOctets_req    = 0;
#endif

#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
        pc->authPayloadEnable    = 0;
        pc->authPayloadTimeoutUs = 30 * 1000 * 1000; //30s
#endif
    }


    //---------- connection update complete -----------------------------
    if (pc->irq_event1_union.conn_update_evt) {
        hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)p8;
        DBG_TIANXIANG_CHN6_HIGH;
        DBG_TIANXIANG_CHN6_LOW;
#if (LL_ASYNC_LEA_EN)
        blt_async_connUpdateCallback(connHandle);
#endif
        pUpt->subEventCode       = HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE;                                                                           // sub code
        pUpt->status             = (pc->conn_intvl_n_1m25 == pc->conn_intvl_next_n_1m25) ? BLE_SUCCESS : HCI_ERR_REMOTE_DEVICE_TERM_CONN_LOW_RESOURCES; // status: success
        pUpt->connHandle         = connHandle;                                                                                                          // handle
        pUpt->connInterval       = pc->conn_intvl_n_1m25;                                                                                               //pkt_init.interval;             // interval
        pUpt->connLatency        = pc->conn_latency;                                                                                                    //pkt_init.latency;                    // latency
        pUpt->supervisionTimeout = pc->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS);                                                                     //pkt_init.timeout;

        if (hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE) {
            blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, p8, 10);

            my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][EVT] Conn update Evt", &pUpt->connHandle, 8);
        }
        pc->irq_event1_union.conn_update_evt = 0; //must clear it at the end
        blmsParam.conupdtEvt_mask &= ~(1 << idx);
    }


    //---------- encryption event ------------------------------------------
    if (pc->encryption_evt) {
        u8 status     = pc->encryption_evt & 0x3F;
        u8 enc_enable = status == BLE_SUCCESS ? 1 : 0;

        //after three handshakes, encryption is complete(Successful)
        if (ll_encryption_done_cb) {
            ll_encryption_done_cb(connHandle, status, enc_enable); // blt_smp_llEncryptionDone
        }

        if (pc->encryption_evt & MS_CONN_ENC_CHANGE) {
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
            if (hci_eventMaskPage2_2 & HCI_EVT_MASK_ENCRYPTION_CHANGE_EVENT_V3) {
                u8                        result[8] = {0};
                hci_encryptEnableV3Evt_t *pEvt      = (hci_encryptEnableV3Evt_t *)result;

                pEvt->encryptEnable.status            = BLE_SUCCESS;
                pEvt->encryptEnable.connHandle        = connHandle;
                pEvt->encryptEnable.encryption_enable = enc_enable;
                pEvt->encryptionKeySize               = pc->enc_key_size;
                pEvt->micLength                       = pc->mic_length;
                pEvt->pfs_dbg_key                     = pc->pfs_dbg_key;
                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_CHANGE_V3, result, 7);
            }else
#endif
            if (hci_eventMask & HCI_EVT_MASK_ENCRYPTION_CHANGE) {
                event_enc_change_t *pEncCh = (event_enc_change_t *)p8;
                pEncCh->status             = status;     //ll reject rsn
                pEncCh->handle             = connHandle; // handle
                pEncCh->enc_enable         = enc_enable;

                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_CHANGE, p8, 4);
            }
        } else if (pc->encryption_evt & MS_CONN_ENC_REFRESH) {
#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
            if (hci_eventMaskPage2_2 & HCI_EVT_MASK_ENCRYPTION_KEY_REFRESH_COMPLETE_EVENT_V2) {
                u8                            result[6] = {0};
                hci_encryptKeyRefreshV2Evt_t *pEvt      = (hci_encryptKeyRefreshV2Evt_t *)result;

                pEvt->keyFresh.status     = BLE_SUCCESS;
                pEvt->keyFresh.connHandle = connHandle;
                pEvt->micLength           = pc->mic_length;
                pEvt->pfs_dbg_key         = pc->pfs_dbg_key;
                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_KEY_REFRESH_COMPLETE_V2, result, 5);
            }else
#endif
            if (hci_eventMask_2 & HCI_EVT_MASK_ENCRYPTION_KEY_REFRESH_COMPLETE) {
                event_enc_refresh_t *pEncRf = (event_enc_refresh_t *)p8;
                pEncRf->status              = BLE_SUCCESS; // status: success
                pEncRf->handle              = connHandle;  // handle

                blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_ENCRYPTION_KEY_REFRESH, p8, 3);
            }
        }


        pc->encryption_tmp_st = 0;
        pc->encryption_evt    = 0; //must clear it at the end
    }
#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    if (pc->irq_event1_union.phy_update_evt) {
        if (hci_le_eventMask & HCI_LE_EVT_MASK_PHY_UPDATE_COMPLETE) {
            hci_le_phyUpdateCompleteEvt_t *phyUpt = (hci_le_phyUpdateCompleteEvt_t *)p8;

            phyUpt->subEventCode = HCI_SUB_EVT_LE_PHY_UPDATE_COMPLETE;
            phyUpt->status       = 0; //ok
            phyUpt->connHandle   = connHandle;
            phyUpt->tx_phy       = pc->connPhyCtrl.conn_cur_phy;
            phyUpt->rx_phy       = pc->connPhyCtrl.conn_cur_phy;
            blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, p8, sizeof(hci_le_phyUpdateCompleteEvt_t));
            //          hci_le_phyUpdateComplete_evt(connHandle, BLE_SUCCESS, pc->connPhyCtrl.conn_cur_phy);
        }
        pc->irq_event1_union.phy_update_evt = 0;
        blmsParam.phyupdtEvt_mask &= ~(1 << idx);


        u8  sendEvt   = false;
        u16 effRxTime = 0;
        u16 effTxTime = 0;
        if (pc->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED) {
            effRxTime = 2704;
            effTxTime = 2704;

            if (pc->ext_data.connEffectiveMaxRxTime != effRxTime || pc->ext_data.connEffectiveMaxTxTime != effTxTime) {
                pc->ext_data.connEffectiveMaxRxTime = effRxTime;
                pc->ext_data.connEffectiveMaxTxTime = effTxTime;
                sendEvt                             = true;
            }
        }
        // must LL/CON/CEN/BV-55-C  [Mandatory Minimum PDU Length, LE Coded]
        else {
            if (pc->connPhyCtrl.conn_last_phy == BLE_PHY_CODED) {
                effRxTime = LL_PACKET_OCTET_TIME(pc->ext_data.connEffectiveMaxRxOctets);
                effTxTime = LL_PACKET_OCTET_TIME(pc->ext_data.connEffectiveMaxTxOctets);
                if (pc->ext_data.connEffectiveMaxRxTime != effRxTime || pc->ext_data.connEffectiveMaxTxTime != effTxTime) {
                    pc->ext_data.connEffectiveMaxRxTime = effRxTime;
                    pc->ext_data.connEffectiveMaxTxTime = effTxTime;
                    sendEvt                             = true;
                }
            }
        }

        if (sendEvt) {
            u8                            result[11];
            hci_le_dataLengthChangeEvt_t *pEvt = (hci_le_dataLengthChangeEvt_t *)result;
            pEvt->subEventCode                 = HCI_SUB_EVT_LE_DATA_LENGTH_CHANGE;
            pEvt->connHandle                   = connHandle; ///
            pEvt->maxTxOct                     = pc->ext_data.connEffectiveMaxTxOctets;
            pEvt->maxTxtime                    = pc->ext_data.connEffectiveMaxTxTime;
            pEvt->maxRxOct                     = pc->ext_data.connEffectiveMaxRxOctets;
            pEvt->maxRxtime                    = pc->ext_data.connEffectiveMaxRxTime;

            blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, result, 11);
        }
    }
#endif

    return 1;
}

u8 blt_ll_smpPushEncPkt(u16 connHandle, u8 type)
{
    u8            idx            = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc             = (st_ll_conn_t *)&blms[idx];
    u8            ll_master_role = connHandle & BLM_CONN_HANDLE;

    u8 pkt_push_result = 0;
    u8 pkt_enc[28];
    pkt_enc[0] = 0x03;

    if (type == LL_ENC_RSP) {
        if (!ll_master_role) {
            pkt_enc[1] = 0x0d;
            smemcpy(pkt_enc + 3, pc->enc_skds, 8);
            smemcpy(pkt_enc + 11, &pc->enc_ivs, 4);
            pkt_push_result = 1;
        }
    }

    else if (type == LL_ENC_REQ) {
        if (ll_master_role) {
            pkt_enc[1] = 0x17;
            smemcpy(pkt_enc + 3, pc->enc_random, 8);
            smemcpy(pkt_enc + 11, &pc->enc_ediv, 2);
            smemcpy(pkt_enc + 13, pc->enc_skdm, 8);
            smemcpy(pkt_enc + 21, &pc->enc_ivm, 4);
            pkt_push_result = 1;
        }
    } else //LL_START_ENC_RSP
    {
        pkt_enc[1]      = 1;
        pkt_push_result = 1;
    }

    pkt_enc[2] = type;

    if (pkt_push_result) {
        blt_llmsPushLlCtrlPkt(connHandle, type, pkt_enc);
    }

    return pkt_push_result;
}

_attribute_noinline_ void blt_ll_smpSecurityProc(u16 connHandle)
{
    u8            idx            = connHandle & CONN_IDX_MASK;
    st_ll_conn_t *pc             = (st_ll_conn_t *)&blms[idx];
    u8            ll_master_role = (connHandle & BLM_CONN_HANDLE);

    if (pc->connState == CONN_STATUS_ESTABLISH && blms[idx].conn_receive_new_packet) {
        blms[idx].conn_receive_new_packet = 0;   //RX with CRC correct & new SN

        if (ll_master_role) {
            if (pc->crypt.st == MS_LL_ENC_REQ) { //for master
                //SDKs and IVs random generate every new pair
                generateRandomNum(8, (u8 *)pc->enc_skdm);
                generateRandomNum(4, (u8 *)&pc->enc_ivm);

                blt_ll_smpPushEncPkt(connHandle, LL_ENC_REQ);
                pc->crypt.st            = MS_LL_ENC_RSP_T;
                pc->ll_rsp_timeout_tick = clock_time() | 1;
            } else if (pc->crypt.st == MS_LL_ENC_PAUSE_RSP) { //for master
                blt_ll_smpPushEncPkt(connHandle, LL_PAUSE_ENC_RSP);
                pc->crypt.st            = MS_LL_ENC_REQ;
                pc->ll_rsp_timeout_tick = 0;
            } else if (pc->crypt.st == MS_LL_ENC_PAUSE_REQ) { //for master
                blt_ll_smpPushEncPkt(connHandle, LL_PAUSE_ENC_REQ);
                pc->crypt.st            = MS_LL_ENC_PAUSE_RSP_T;
                pc->ll_rsp_timeout_tick = clock_time() | 1;
            }
        } else {
            if (blt_ll_getRealTxFifoNumber(connHandle) != 0) {
            } else if (pc->crypt.st == MS_LL_ENC_RSP_T) { //for slave
                pc->crypt.st = MS_LL_ENC_START_REQ_T;
                blt_ll_smpPushEncPkt(connHandle, LL_START_ENC_REQ);
                pc->ll_rsp_timeout_tick = clock_time() | 1;

#if (BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE)
                pc->conn_pkt_rcvd = 0;
#endif

                pc->crypt.enable = 1; //slave start encryption
                aes_ll_ccm_encryption_init(pc->crypt.sk, pc->enc_skdm, pc->enc_skds, (u8 *)&pc->enc_ivm, (u8 *)&pc->enc_ivs, &pc->crypt);
                //AAcrypt_para[i_connhandle] = pc->crypt;
                my_dump_str_u32s(0, "aes_ccm init", blt_debug_hex_2_dec_display(pc->conn_inst), (u32)pc->crypt.enc_pno, (u32)pc->crypt.dec_pno, 0);
            } else if (pc->crypt.st == MS_LL_REJECT_IND_T) { //for slave
                pc->crypt.st = MS_LL_ENC_OFF;
                blt_ll_setEncryptionBusy(connHandle, 0);     //no ltk match, must clear enc busy flag
                pc->ll_rsp_timeout_tick = 0;

                //LL/SEC/PER/BV-11-C    [Peripheral Sending LL_REJECT_EXT_IND]
                u8 suppExtReject = 0; //dft
                if (pc->ll_remoteFeature0 & LL_FEATURE_MASK_EXTENDED_REJECT_INDICATION) {
                    suppExtReject = 1;
                }
                blt_llms_rejectInd(connHandle, LL_ENC_REQ, HCI_ERR_PIN_KEY_MISSING, suppExtReject);
                blt_p_event_callback(BLT_EV_FLAG_KEY_MISSING, (u8 *)&suppExtReject, 1);
            }
        }
    }
}


#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #endif
    ble_sts_t
    blt_ll_sendHostNumOfCmpPktNonConn(u8 *numOfCmpConn)
{
    u8  connhand_idx[LL_MAX_ACL_CONN_NUM];
    u8  connhand_handle[LL_MAX_ACL_CONN_NUM]; //u8 is enough for our design
    int connhand_cnt = 0;

    u8                    evt_buffer[1 + 4 * LL_MAX_ACL_CONN_NUM];
    hci_numOfCmpPktEvt_t *pEvt = (hci_numOfCmpPktEvt_t *)evt_buffer;

    for (int i = 0; i < LL_MAX_ACL_CONN_NUM; i++) {
        if (numOfCmpConn[i]) {
            int j;
            for (j = 0; j < connhand_cnt; j++) {
                if (i == connhand_idx[j]) {
                    break;
                }
            }

            if (j >= connhand_cnt) {
                connhand_handle[connhand_cnt] = i | (i < LL_MAX_ACL_CEN_NUM ? BLM_CONN_HANDLE : BLS_CONN_HANDLE);
                connhand_idx[connhand_cnt++]  = i;
            }
        }
    }

    pEvt->numHandles = connhand_cnt;
    for (int i = 0; i < connhand_cnt; i++) {
        pEvt->retParams[i].connHandle   = connhand_handle[i];
        pEvt->retParams[i].numOfCmpPkts = numOfCmpConn[connhand_idx[i]];
    }

    //It must be ensured that Num_Of_Complete_Evt is sent successfully
    if (blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_NUM_OF_COMPLETE_PACKETS, evt_buffer, 1 + 4 * connhand_cnt) != 0) {
        //          printf("It must be ensured that Num_Of_Complete_Evt is sent successfully: ERR here\n");
        //          BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCC110623);
    }

    return BLE_SUCCESS;
}


#endif


#if (HCI_SEND_NUM_OF_CMP_AFT_ACK && ACL_TXFIFO_4K_LIMITATION_WORKAROUND)
    #define FIX_NUM_OF_CMP_FOR_ACL_TXFIFO_4K_LIMITATION 1 //EBQ tested OK
#endif

#ifndef FIX_NUM_OF_CMP_FOR_ACL_TXFIFO_4K_LIMITATION
    #define FIX_NUM_OF_CMP_FOR_ACL_TXFIFO_4K_LIMITATION 0
#endif

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    ble_sts_t
    blt_hci_rx_aclfifo_to_rf_txfifo(void)
{
    u32 numOfCmpCntConn                   = 0; /*  count conn */
    u8  numOfCmpConn[LL_MAX_ACL_CONN_NUM] = {0};

#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    u32 numOfCmpCntNoConn                   = 0; /*  count non-conn  */
    u8  numOfCmpNoConn[LL_MAX_ACL_CONN_NUM] = {0};
#endif

#if (FIX_NUM_OF_CMP_FOR_ACL_TXFIFO_4K_LIMITATION)
    u8 tx_push_pkt_num[LL_MAX_ACL_CONN_NUM] = {0, 0, 0, 0, 0, 0, 0, 0};
#endif


#if BQB_HOST_SEND_EMPTY_ACL_DATA_HANDLE_EN
    static u8 lenZeroFlag = 0;
#endif


    while (bltHci_rxAclfifo.wptr != bltHci_rxAclfifo.rptr) {
        u8 *p = bltHci_rxAclfifo.p + (bltHci_rxAclfifo.rptr & bltHci_rxAclfifo.mask) * bltHci_rxAclfifo.size;

        u8            conn_idx   = p[0] & CONN_IDX_MASK;
        u16           connHandle = p[0] | p[1] << 8;
        st_ll_conn_t *pc         = (st_ll_conn_t *)&blms[conn_idx];

        if (!pc->connState) { //if connection terminate, drop data to prevent other connection data block
            bltHci_rxAclfifo.rptr++;

#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
            numOfCmpNoConn[conn_idx]++;
            numOfCmpCntNoConn++;
#else
            numOfCmpConn[conn_idx]++;
            numOfCmpCntConn++;
#endif

            continue;
        }

#if (1)
        if (pc->ll_enc_busy) //one connection en_busy will block other connection
#else
        if (blt_ll_isEncryptionBusy(connHandle))
#endif
        {
            break;
        }


#if LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION
        int tempLen = (pc->ext_data.connEffectiveMaxTxTime - 112) >> 3; //1M PHY
        u8  curPhy  = BLE_PHY_1M;
        u8  phyOpt  = LE_CODED_S2;

    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
        u8 is_master_role = pc->aclRole == ACL_ROLE_CENTRAL ? 1 : 0;
        if ((is_master_role && pc->llcp_flag.bit.ll_phy_ind_send_flag) ||
            (!is_master_role && ((pc->ll_upd_flag & PHY_UPD_FLAG) || pc->llcp_flag.bit.ll_phy_req_rcvd_flag))) {
            curPhy = pc->connPhyCtrl.conn_next_phy;
            phyOpt = pc->connPhyCtrl.conn_next_CI;

            if ((!is_master_role && ((pc->ll_upd_flag & PHY_UPD_FLAG) || pc->llcp_flag.bit.ll_phy_req_rcvd_flag))) {
                curPhy = pc->connPhyCtrl.conn_prefer_phys == 0x04 ? BLE_PHY_CODED : pc->connPhyCtrl.conn_prefer_phys;
            }
        } else {
            curPhy = pc->connPhyCtrl.conn_cur_phy;
            phyOpt = pc->connPhyCtrl.conn_cur_CI;
        }
    #endif

        if (curPhy == BLE_PHY_2M) {
            tempLen = (pc->ext_data.connEffectiveMaxTxTime - 112) >> 2; //2M PHY
        } else if (curPhy == BLE_PHY_CODED) {
            tempLen = 27;
            if (phyOpt == LE_CODED_S2) {
                if (pc->ext_data.connEffectiveMaxTxTime >= 958) {
                    tempLen = ((pc->ext_data.connEffectiveMaxTxTime - 382) >> 4) - 9; //coded phy S2
                }
            } else {
                if (pc->ext_data.connEffectiveMaxTxTime >= 2704) {
                    tempLen = ((pc->ext_data.connEffectiveMaxTxTime - 400) >> 6) - 9; //coded phy S8
                }
            }
        }
        //my_dump_str_data(0, "[I] temp len      ",&tempLen, 1);
        //my_dump_str_data(0, "[I] effect tx len",&pc->ext_data.connEffectiveMaxTxOctets, 2);
        //my_dump_str_data(0, "[I] effect tx time",&pc->ext_data.connEffectiveMaxTxTime, 2);
        int conn_MAX_TX_OCTETS = min(pc->ext_data.connEffectiveMaxTxOctets, tempLen);
#else
        int conn_MAX_TX_OCTETS = MAX_OCTETS_DATA_LEN_27;
#endif
        u8  PB_Flag = p[2];
        int len     = p[3] | p[4] << 8;
        int pktNum  = (len + conn_MAX_TX_OCTETS - 1) / conn_MAX_TX_OCTETS;

#if ACL_TXFIFO_4K_LIMITATION_WORKAROUND
        if (blmsParam.cache_txfifo_used) {
            int sw_fifo_n = ((blt_cache_txFifo.wptr - blt_cache_txFifo.rptr) & 63);
            if (sw_fifo_n + pktNum >= (blt_cache_txFifo.num - BLMS_STACK_USED_TX_FIFO_NUM)) {
                break;
            }
        } else {
            if (((pc->tx_wptr - pc->tx_rptr) & 31) + pktNum >= (pc->max_fifo_num - BLMS_STACK_USED_TX_FIFO_NUM)) { //FIFO not enough
                break;
            }
        }
#else
        if (((pc->tx_wptr - pc->tx_rptr) & 31) + pktNum >= (pc->max_fifo_num - BLMS_STACK_USED_TX_FIFO_NUM)) { //FIFO not enough
            break;
        }
#endif

        u8 llid = (PB_Flag == HCI_CONTINUING_PACKET) ? LLID_DATA_CONTINUE : LLID_DATA_START;
#if BQB_HOST_SEND_EMPTY_ACL_DATA_HANDLE_EN
        if (len == 0 && lenZeroFlag == false && llid == LLID_DATA_START) {
            lenZeroFlag = true;
            bltHci_rxAclfifo.rptr++;
            continue;
        }
        if (lenZeroFlag && llid == LLID_DATA_CONTINUE) {
            llid        = LLID_DATA_START;
            lenZeroFlag = false;
        } else {
            lenZeroFlag = false;
        }
#endif


        int n = min(len, conn_MAX_TX_OCTETS);
        /* connHandle(2B) + pb_flag(1B) + data_total_len(2B) + data(data_total_lenB)*/
        p += 3;
        p[0] = llid; //PB_Flag == HCI_CONTINUING_PACKET ? LLID_DATA_CONTINUE : LLID_DATA_START;         //llid
        p[1] = n;


#if (FIX_NUM_OF_CMP_FOR_ACL_TXFIFO_4K_LIMITATION)
        tx_push_pkt_num[conn_idx] += pktNum;
#endif


        ll_push_tx_fifo_handler(connHandle, p); /// blt_llms_pushTxfifo    OR    blt_acl_pushCacheTxfifo

        for (int i = n; i < len; i += conn_MAX_TX_OCTETS) {
            p += conn_MAX_TX_OCTETS;            //move forward
            n    = len - i > conn_MAX_TX_OCTETS ? conn_MAX_TX_OCTETS : len - i;
            p[0] = LLID_DATA_CONTINUE;          //llid, fragment pkt
            p[1] = n;
            ll_push_tx_fifo_handler(connHandle, p);
        }


        bltHci_rxAclfifo.rptr++; //FIFO pop
        numOfCmpConn[conn_idx]++;
        numOfCmpCntConn++;

        //printf("ll_push_tx_fifo succ\n");
        //printf("H2C ACL Data belong to connHandle:0x%x,", connHandle);
    }

#if (HCI_SEND_NUM_OF_CMP_AFT_ACK)
    /* NoConn numOfCmp process */
    if (numOfCmpCntNoConn) {
        blt_ll_sendHostNumOfCmpPktNonConn(numOfCmpNoConn);
        //DBG_C HN15_TOGGLE;
        //DBG_C HN15_TOGGLE;
    }
    /* Conn numOfCmp process */
    if (numOfCmpCntConn) {
        u32           r  = 0;
        st_ll_conn_t *pc = NULL;
        for (int i = 0; i < LL_MAX_ACL_CONN_NUM; i++) {
            if (numOfCmpConn[i]) { //1 or more ACL data
                pc = (st_ll_conn_t *)&blms[i];

                r = irq_disable();
    #if (FIX_NUM_OF_CMP_FOR_ACL_TXFIFO_4K_LIMITATION)
                //Mark the current TX write pointer position
                if (pc->nocAckWptr == pc->nocAckRptr) {
                    pc->nocAclTxWptr[pc->nocAckWptr & pc->nocAckMsk] = pc->tx_wptr + tx_push_pkt_num[i];
                } else {
                    pc->nocAclTxWptr[pc->nocAckWptr & pc->nocAckMsk] = pc->nocAclTxWptr[(pc->nocAckWptr - 1) & pc->nocAckMsk] + tx_push_pkt_num[i];
                }
    #else
                pc->nocAclTxWptr[pc->nocAckWptr & pc->nocAckMsk] = pc->tx_wptr; //Mark the current TX write pointer position
    #endif
                pc->numOfCmpCnt[(pc->nocAckWptr++) & pc->nocAckMsk] = numOfCmpConn[i];
                irq_restore(r);
                //my_dump_str_data(STACK_DUMP_EN, "aclIdx", &i, 1);
                //my_dump_str_data(STACK_DUMP_EN, "nocAclTxWptr", &pc->tx_wptr, 1);
                //my_dump_str_data(STACK_DUMP_EN, "nocAckWptr", &pc->nocAckWptr, 1);
                //my_dump_str_data(STACK_DUMP_EN, "nocAckRptr", &pc->nocAckRptr, 1);
                //my_dump_str_data(STACK_DUMP_EN, "numOfCmpCnt", &numOfCmpConn[i], 1);
                //In ISR blt_llms_saveTxfifoRptr call API: hci_numberOfCompletePacket_evt(connHandle, numOfCmpConn[i]);
            }
        }
    }
#else
    if (numOfCmpCntConn) {
        u8  connhand_idx[LL_MAX_ACL_CONN_NUM];
        u8  connhand_handle[LL_MAX_ACL_CONN_NUM]; //u8 is enough for our design
        int connhand_cnt = 0;

        u8                    evt_buffer[1 + 4 * LL_MAX_ACL_CONN_NUM];
        hci_numOfCmpPktEvt_t *pEvt = (hci_numOfCmpPktEvt_t *)evt_buffer;

        for (int i = 0; i < LL_MAX_ACL_CONN_NUM; i++) {
            if (numOfCmpConn[i]) {
                int j;
                for (j = 0; j < connhand_cnt; j++) {
                    if (i == connhand_idx[j]) {
                        break;
                    }
                }

                if (j >= connhand_cnt) {
                    connhand_handle[connhand_cnt] = i | (i < LL_MAX_ACL_CEN_NUM ? BLM_CONN_HANDLE : BLS_CONN_HANDLE);
                    connhand_idx[connhand_cnt++]  = i;
                }
            }
        }

        pEvt->numHandles = connhand_cnt;
        for (int i = 0; i < connhand_cnt; i++) {
            pEvt->retParams[i].connHandle   = connhand_handle[i];
            pEvt->retParams[i].numOfCmpPkts = numOfCmpConn[connhand_idx[i]];
        }

        //It must be ensured that Num_Of_Complete_Evt is sent successfully
        if (blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_NUM_OF_COMPLETE_PACKETS, evt_buffer, 1 + 4 * connhand_cnt) != 0) {
            //          printf("It must be ensured that Num_Of_Complete_Evt is sent successfully: ERR here\n");
            //          BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCC110623);
        }
    }
#endif

    return BLE_SUCCESS;

    ////Handle:12|PB flag:2|BC flag:2 (2B)|Data Total Length(2B)  *pData
    ////pData: type 1B | rflen 1B | l2cap len 2B | l2cap CID 2B | Payload xB |
    ////         llid    rf_len       l2cap_len      ATT_0x0004   ATT_payload
    ////                                                          ATT_payload: opcode 1B | ATT_handle 2B | ATT_data yB|
    ///           06       13           0B 00          04 00                     1B            12 00        00 00 80 00  00 00 00 00

    ///////////// acl_type  conn_handle_pb_pc  data_total_len l2cap_len  ATT_0x0004  opcode ATT_handle 2B  ATT_data 20B|
    /// testcase:   02          44 00             1b 00         17 00      04 00       1B       30 00       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  [SPP S2C handle: 48]
    //  UART send data          02 44 00 1b 00 17 00 04 00 1B 30 00 11 22 33 44 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF
    /// testcase:   02          44 00             1b 00         17 00      04 00       52       34 00       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  [SPP C2S handle: 52]
    //  UART send data          02 44 00 1b 00 17 00 04 00 52 34 00 11 22 33 44 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 FF
}


#if HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
extern u16 hci_getHostAvailBufNum(void);

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_acl_conn_rx_fifo_proc(void)
{
    int sum;
    u8  rptr = blt_rxfifo.rptr;
    u8  wptr = blt_rxfifo.wptr;
    u16 ren_num;
    while (rptr != wptr) {
        u8 *raw_pkt = (u8 *)(blt_rxfifo.p_base + (rptr & blt_rxfifo.mask) * blt_rxfifo.size);

        ren_num = hci_getHostAvailBufNum();
        if (raw_pkt[2]) {
            if (blms[raw_pkt[2] & CONN_IDX_MASK].irq_event1_union.connect_evt) {
                //connection complete CallBack has not finished, can not process data on this connection,
                //cause "pairing_random" will use some data(initA & advA) of connection complete CallBack
                break; //all RX FIFO data hold for a while, until connection complete CallBack finished
            }

            if (ren_num == 0) {
                rf_packet_ll_control_t *pll  = (rf_packet_ll_control_t *)(raw_pkt + DMA_RFRX_OFFSET_HEADER);
                u8                      type = pll->type & 3;
                if (type == LLID_CONTROL) //no ACL
                {
    #if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
                    //CTEInfo is extra data.  It needs to be placed before blt_llms_main_loop_data
                    if (ll_aoa_aod_mlp_task_cb) {
                        ll_aoa_aod_mlp_task_cb(FLAG_AOA_AOD_CONNECTION_MAINLOOP); //blt_ll_aoa_aod_acl_mainloop
                    }
    #endif

                    blt_llms_main_loop_data(raw_pkt[2], raw_pkt);

                    sum = (u8)(rptr - blt_rxfifo.rptr);
                    for (int j = 0; j < sum; j++) //All data transport
                    {
                        u8 *raw_pkt_out = (u8 *)(blt_rxfifo.p_base + ((rptr - j) & blt_rxfifo.mask) * blt_rxfifo.size);
                        u8 *raw_pkt_in  = (u8 *)(blt_rxfifo.p_base + ((rptr - j - 1) & blt_rxfifo.mask) * blt_rxfifo.size);
                        memcpy(raw_pkt_out, raw_pkt_in, blt_rxfifo.size);
                    }
                    blt_rxfifo.rptr++;
                }
            } else {
    #if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
                //CTEInfo is extra data.  It needs to be placed before blt_llms_main_loop_data
                if (ll_aoa_aod_mlp_task_cb) {
                    ll_aoa_aod_mlp_task_cb(FLAG_AOA_AOD_CONNECTION_MAINLOOP); //blt_ll_aoa_aod_acl_mainloop
                }
    #endif

                blt_llms_main_loop_data(raw_pkt[2], raw_pkt);
            }
        }
        if (ren_num != 0) {
            blt_rxfifo.rptr++;
        }
        rptr++;
    }
}
#else

    #if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
    #else
_attribute_no_inline_
    #endif
    void
    blt_acl_conn_rx_fifo_proc(void)
{
    while (blt_rxfifo.rptr != blt_rxfifo.wptr) {
        wd_clear(); //clear watch dog

        u8 *raw_pkt = (u8 *)(blt_rxfifo.p_base + (blt_rxfifo.rptr & blt_rxfifo.mask) * blt_rxfifo.size);

        if (raw_pkt[2]) {
            if (blms[raw_pkt[2] & CONN_IDX_MASK].irq_event1_union.connect_evt) {
                //connection complete CallBack has not finished, can not process data on this connection,
                //cause "pairing_random" will use some data(initA & advA) of connection complete CallBack
                break; //all RX FIFO data hold for a while, until connection complete CallBack finished
            }

    #if (LL_UNREQUESTED_CONSTANT_TONE_EXTENSION_RECEIVING_ENABLE)
            //CTEInfo is extra data.  It needs to be placed before blt_llms_main_loop_data
            if (ll_aoa_aod_mlp_task_cb) {
                ll_aoa_aod_mlp_task_cb(FLAG_AOA_AOD_CONNECTION_MAINLOOP); //blt_ll_aoa_aod_acl_mainloop
            }
    #endif

            blt_llms_main_loop_data(raw_pkt[2], raw_pkt);
        }

        blt_rxfifo.rptr++; //handle rx data overflow in irq to prevent rx_fifo.rptr from re-entering
    }
}
#endif


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
    int
    blt_ll_acl_conn_mainloop(void)
{
#if (LL_RSSI_SNIFFER_MODE_ENABLE)
    #if (LL_RSSI_SNIFFER_MASTER_ENABLE)
    bool snifm_used = ll_acl_sniffer_mst_irq_task_cb ? TRUE : FALSE;
    #else
    bool snifm_used = FALSE;
    #endif
    #if (LL_RSSI_SNIFFER_SLAVE_ENABLE)
    bool snifs_used = ll_acl_sniffer_slv_irq_task_cb ? TRUE : FALSE;
    #else
    bool snifs_used = FALSE;
    #endif
    if ((!snifm_used && (!snifs_used)) && blt_rxfifo.rptr != blt_rxfifo.wptr) {
#else
    if (blt_rxfifo.rptr != blt_rxfifo.wptr) {
#endif
        blt_acl_conn_rx_fifo_proc();
    }

    u16 connHandle;
    for (int conn_idx = 0; conn_idx < LL_MAX_ACL_CONN_NUM; conn_idx++) //TODO: save some checking time
    {
        st_ll_conn_t *pAclConn = (st_ll_conn_t *)&blms[conn_idx];
        connHandle             = pAclConn->acl_conHandle;

#if (LL_ACL_CEN_EN)
        //TODO, if connection never used, do not check flags_pendings to save running time
        if (pAclConn->aclRole == ACL_ROLE_CENTRAL) { //Master

            //------------ master enc pending pkt proc --------------------------------------------
            if (pAclConn->crypt.st == MS_LL_ENC_OFF) {
                blm_pkt_pending_t *enc_pkt_pending = &blmsMasterEncPktPending[conn_idx];
                while (enc_pkt_pending->wptr != enc_pkt_pending->rptr) {
                    //printf("pending pkt proc\n");
                    u8 *raw_pkt = enc_pkt_pending->buff[enc_pkt_pending->rptr & enc_pkt_pending->mask];
                    //array_printf(raw_pkt, raw_pkt[5]+6);
                    if (raw_pkt[2]) {
                        blt_llms_main_loop_data(raw_pkt[2], raw_pkt);
                    }
                    enc_pkt_pending->rptr++;
                    enc_pkt_pending->rptr &= enc_pkt_pending->mask;
                }
            }

            //------------ master channel map pending proc ----------------------------------------
            if (blmhostChnClassUpt.hostMapUptCmdPending & BIT(conn_idx)) {
                if (blt_ll_pushAclChClassUpdPkt(connHandle, blmhostChnClassUpt.gLlChannelMap)) {
                    blmhostChnClassUpt.hostMapUptCmdPending &= ~BIT(conn_idx);
                }
            }

    #if (CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE)
            if (pAclConn->establish_evt) {
                pAclConn->establish_evt = 0;
                hci_tlk_connectionEstablish_evt(BLE_SUCCESS, connHandle, pAclConn->aclRole, pAclConn->conn_peerPktA_type, pAclConn->conn_peerPktA, pAclConn->conn_intvl_n_1m25, pAclConn->conn_latency, pAclConn->conn_timeout / (10 * SYSTEM_TIMER_TICK_1MS), pAclConn->conn_sca);

                aclConn_etbsh.crtConn_cur_cnt     = 0;
                aclConn_etbsh.re_create_conn_tick = 0;
            }
    #endif
        } else
#endif
        { //Slave
//todo: blmsSlave not appeared in acl_conn.c
#if BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
            //LL/CON/PER/BI-09-C    [Responding to PHY Update Procedure - Instant In Past]
            st_lls_conn_t *ps = (st_lls_conn_t *)&blmsSlave[conn_idx - LL_MAX_ACL_CEN_NUM];
            if (ps->errFlag & SLV_FLAG_INSTANT_PASS) {
                blc_ll_disconnect(connHandle, HCI_ERR_INSTANT_PASSED);
                ps->errFlag &= ~SLV_FLAG_INSTANT_PASS;
            }

            if (ps->errFlag & SLV_FLAG_LEN_ERR) {
                blt_llms_unknownRsp(connHandle, ps->unknownType);
                ps->errFlag &= ~SLV_FLAG_LEN_ERR;
            }
#endif
        }


        if (pAclConn->connState) {
#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
            if (pAclConn->authPayloadEnable && pAclConn->authPayloadTick &&
                clock_time_exceed(pAclConn->authPayloadTick, pAclConn->authPayloadTimeoutUs)) {
                pAclConn->authPayloadTick = clock_time() | 1;
                blt_ll_authPayloadTimeoutExpiredHandler(connHandle);
            }
#endif


            if (pAclConn->FeatureRsp || (pAclConn->remoteFeatureReq & FEATURE_SEND_FEAT_REQ) || (pAclConn->remoteFeatureReq == FEATURE_HCI_REPORT)) {
                blt_ll_processFeatureExchange(pAclConn);
            }
        }


        //------------ LL ctrl procedure timeout proc ----------------------------------
        if (pAclConn->ll_rsp_timeout_tick) {
            blt_ll_rspTimeoutLoopEvt(connHandle);

#if OS_SUP_EN
            if (pAclConn->ll_rsp_timeout_tick) {
                //blt_ll_sem_give();
                if (blt_os_semCountIncrement_cb) {
                    blt_os_semCountIncrement_cb();
                }
            }
#endif
        }

        //------------ security handle proc --------------------------------------------
        if (pAclConn->crypt.st) {
            blt_ll_smpSecurityProc(connHandle);

#if OS_SUP_EN
            if (pAclConn->crypt.st) {
                //blt_ll_sem_give();
                if (blt_os_semCountIncrement_cb) {
                    blt_os_semCountIncrement_cb();
                }
            }
#endif
        }

#ifndef BLC_ZEPHYR_BLE_INTEGRATION
        //----------- Push hold tx pkt (after ll_enc not busy) -------------------------
        if (pAclConn->blt_tx_pkt_hold) {
            blt_ll_push_hold_data(connHandle);
#if OS_SUP_EN
            if (pAclConn->blt_tx_pkt_hold) {
                //blt_ll_sem_give();
                if (blt_os_semCountIncrement_cb) {
                    blt_os_semCountIncrement_cb();
                }
            }
#endif
        }
#endif

        //---------- process pending event (Connection dependent evt) -------------------

        /* pAclConn->connect_evt || pAclConn->disconn_evt || pAclConn->conn_update_evt || pAclConn->phy_update_evt || pAclConn->encryption_evt */
        if (pAclConn->irq_event1_union.irqevt1_pack || pAclConn->encryption_evt) {
            blt_ll_procConnectionEvent(connHandle, pAclConn);
        }


        if (pAclConn->rejectReason) {
            if (blt_llms_rejectInd(connHandle, pAclConn->rejectOpcode, pAclConn->rejectReason, 1)) {
                pAclConn->rejectReason = 0;
            }
        }


#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
        if (pAclConn->ext_data.connMaxTxRxOctets_req) {
            blt_ll_procDlePending(connHandle);

    #if OS_SUP_EN
            if (pAclConn->ext_data.connMaxTxRxOctets_req) {
                //blt_ll_sem_give();
                if (blt_os_semCountIncrement_cb) {
                    blt_os_semCountIncrement_cb();
                }
            }
    #endif
        }
#endif

#if(LL_FEATURE_ENABLE_FRAME_SPACE_UPDATE)
        if(pAclConn->fsu_param.fsu_pending || pAclConn->fsu_param.fsu_complete_evt || fsuCmpletEvt.fsu_cmplet_bk_valid){
            blt_ll_fsu_mainloop_proc(connHandle);
        }
#endif

#if LL_FEATURE_ENABLE_HIGHER_DATA_THROUGHPUT
    //only master send PHY update CMD
    if (pAclConn->connPhyCtrl.phy_update_pending) {
        blt_ll_sendPhyUpdateInd_V2(connHandle); //TODO: some code not very clear
    }
    if (pAclConn->connPhyCtrl.phy_req_pending) {
        blt_ll_sendPhyReq(connHandle);
    }

    #if OS_SUP_EN
        if (pAclConn->ext_data.connMaxTxRxOctets_req) {
            //blt_ll_sem_give();
            if (blt_os_semCountIncrement_cb) {
                blt_os_semCountIncrement_cb();
            }
        }
    #endif
#else
#if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
        //only master send PHY update CMD
        if (pAclConn->connPhyCtrl.phy_update_pending) {
            blt_ll_sendPhyUpdateInd(connHandle); //TODO: some code not very clear
        }
        if (pAclConn->connPhyCtrl.phy_req_pending) {
            blt_ll_sendPhyReq(connHandle);
        }

    #if OS_SUP_EN
        if (pAclConn->ext_data.connMaxTxRxOctets_req) {
            //blt_ll_sem_give();
            if (blt_os_semCountIncrement_cb) {
                blt_os_semCountIncrement_cb();
            }
        }
    #endif
#endif
#endif
#if (LL_RSSI_SNIFFER_MODE_ENABLE)
        if (pAclConn->chn_map_update_evt) {
            pAclConn->chn_map_update_evt = 0;
            blt_ll_acl_chnMapUpdateEvent(connHandle, pAclConn);
        }

        if (pAclConn->every_conn_evt) {
            pAclConn->every_conn_evt = 0;
            blt_ll_acl_everyConnEvent(connHandle, pAclConn);
        }
#endif
    }

#if (LL_FEATURE_ENABLE_PAST)
    if (ll_acl_past_mlp_task_cb) {
        ll_acl_past_mlp_task_cb(FLAG_MODULE_MAINLOOP); //blt_ll_pastMainloopTask
    }
#endif


#if (LL_FEATURE_ENABLE_POWER_CONTROL)
    if (ll_acl_pcl_mlp_task_cb) {
        ll_acl_pcl_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_ll_pclMainloopTask
    }
#endif


#if (LL_FEATURE_ENABLE_CHANNEL_CLASSIFICATION)
    if (ll_acl_chnclass_mlp_task_cb) {
        ll_acl_chnclass_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL); //blt_ll_chnclassMainloopTask
    }
#endif

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    if (ll_acl_subrate_mlp_task_cb) { //blt_subrate_mainloop_task
        ll_acl_subrate_mlp_task_cb(FLAG_MODULE_MAINLOOP, NULL);
    }
#endif


#if (LL_ACL_CEN_EN)
    //---------- create connection timeout ------------------------------------------
    if (aclConn_param.tick_connectDevice && ll_init_mlp_task_cb) {
        ll_init_mlp_task_cb(FLAG_MODULE_MAINLOOP); //blt_init_mainloop_task   blt_ll_procInitiateConnectionTimeout
    }
#endif


/* H2C: HCI rx acl buffer flow ctrl */
#if (0) //just for debug, make ACL data block
    static u32 fifo_tick = 0;
    if (clock_time_exceed(fifo_tick, 2000)) {
        fifo_tick = clock_time();
        if (bltHci_rxAclfifo.wptr != bltHci_rxAclfifo.rptr) {
            blt_hci_rx_aclfifo_to_rf_txfifo();
        }
    }
#else
    if (bltHci_rxAclfifo.wptr != bltHci_rxAclfifo.rptr) {
        blt_hci_rx_aclfifo_to_rf_txfifo();
    }
#endif

#if (ACL_TXFIFO_4K_LIMITATION_WORKAROUND)
    if (blmsParam.cache_txfifo_used && blt_cache_txFifo.wptr != blt_cache_txFifo.rptr) {
        blt_acl_cache_tx_fifo_to_hw_tx_fifo();
    }
#endif

    return 0;
}

//Since the customer does not use much, hold for now.(@ronglu)
ble_sts_t  blc_ll_setPeripheralAckCentralDisconnectNum(u8 num)
{
    if(num > 10)  // Too large, timeout may occur
    {
      return HCI_ERR_UNSPECIFIED_ERROR;
    }
    ll_acl_Per_AckCentralDisconnectNum = num ;

    return BLE_SUCCESS;
}
