/********************************************************************************************************
 * @file    PAwR_sync.c
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

#if (LL_FEATURE_ENABLE_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER)

#define PAWR_AUX_SYNC_SUBEVENT_ENCRYPTION 0

#define PAWR_LAST_SUBEVENT_FLAG           BIT(7) //subevent range from 0x00 to 0x7F, so BIT(7) can be used to indicate other function.

#define LTV_LEN_OFFSET                    0
#define LTV_TYPE_OFFSET                   1

#define RSP_SLOT_EARLY_TICK               (100 * SYSTEM_TIMER_TICK_1US)


#define PAWR_EVT_SUBEVT_STORE_OFFSET      (SCAN_SECCHN_RXFIFO_SIZE - 8 - 4)

/****the following structure, maybe is not useful. later will delete that according to EBQ or UPF test***/
typedef struct
{
    u8 esl_dev_groupId;
    u8 esl_dev_eslID;
    u8 esl_rsp_slotIdx;
    u8 rsvd_u8;
} esl_dev_mgr;

esl_dev_mgr esl_devMgr = {
    .esl_dev_groupId = 0,    //default sync subevent 0
    .esl_dev_eslID   = 0x33, //default
};

typedef struct __attribute__((packed))
{
    u8 TLV_tag : 4;
    u8 TLV_len : 4;

    u8 *p_param;
} esl_TLV_t;

typedef struct __attribute__((packed))
{
    u8 group_id : 7;
    u8 rsvd     : 1;

    u8 TLV_data[1];
} esl_payload_t;

typedef struct
{
    u8 adv_len;
    u8 adv_type;
    u8 adv_data[2];
} adv_ltv_t;

_attribute_ble_data_retention_ int PAWR_SYNC_PPM      = 0;
_attribute_ble_data_retention_ int PAWR_SYNC_INIT_PPM = 0;

_attribute_ble_data_retention_ u8 PAWR_SYNC_FLAG = 0;

_attribute_ble_data_retention_ rf_pkt_aux_conn_rsp_t pkt_aux_conn_rsp_pawr = {
    rf_tx_packet_dma_len(14 + 2), // dma_len: rf_len + 2

    LL_TYPE_AUX_CONNECT_RSP, // type
    0,                       // RFU
    0,                       // "ChSel" only valid in ADV_IND/ADV_DIRECT_IND/CONNECT_IND, other packet set 0'b
    0,                       // txAddr           may change
    0,                       // rxAddr           may change

    14, // rf_len:  sizeof(rf_pkt_aux_conn_rsp_t) - 6

    13,                               // ext_hdr_len: Extended Header Flags(1) + AdvA(6) + TargetA(6)
    LL_EXTADV_MODE_NON_CONN_NON_SCAN, // adv_mode

    EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA, // ext_hdr_flg : AdvA | TargetA

    {0, 0, 0, 0, 0, 0}, // advA             need change
    {0, 0, 0, 0, 0, 0}, // targetA          need change
};

//why search eslID, because advertising data can include other type data.
//such as, ESL tag and local Name tag etc in the payload.
_attribute_ram_code_ u8 *blt_search_eslGroupID_fromAdvData(u8 *pAdvData, int advDataLen, data_type_t advType)
{
    for (int i = 0; i < advDataLen;) {
        if (pAdvData[LTV_TYPE_OFFSET] == advType) {
            return (u8 *)pAdvData;
        }

        i += (pAdvData[LTV_LEN_OFFSET] + 1);
        pAdvData = &pAdvData[i];
    }
    return NULL;
}

_attribute_ram_code_ u8 blt_search_eslID_fromLTVdata(esl_TLV_t *pLTVdata, int LTV_len, u8 eslID)
{
    u8 sequence_no = 0;
    for (int i = 0; i < LTV_len;) {
        if (pLTVdata->p_param[0] == eslID) {
            return sequence_no; //decide which slot to send response data.
        }

        sequence_no++;

        i += pLTVdata->TLV_len; //now not be sure length include which field. later will check.
        pLTVdata = &pLTVdata[i];
    }
    return NOT_FIND_ESL_ID_FLAG; //error code. not find
}

/********************************************************************************************************/


_attribute_ble_data_retention_ pda_syncTiming_t  pawr_sync_timingAdjust[TSKNUM_PAWRS_SUB];
_attribute_ble_data_retention_ ll_pawrSync_mng_t bltPawrSync;
_attribute_ble_data_retention_ rf_pkt_ext_adv_t  pawr_rspSlot_pkt = {0};

static inline void subevent_sort_process(u8 *pVal, u8 num_val)
{
    for (u8 i = 0; i < num_val; i++) {
        for (u8 j = 0; j < (num_val - i - 1); j++) {
            if (pVal[j] > pVal[j + 1]) {
                u8 tmp      = pVal[j];
                pVal[j]     = pVal[j + 1];
                pVal[j + 1] = tmp;
            }
        }
    }
}

ble_sts_t blc_ll_initPAwRsync_module(int num_pawr_sync)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(ll_prdadv_sync_t)), pawr_sync);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(st_pda_sync_t)), pawr_sync);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(pda_list_t)), pawr_sync);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(pda_cache_t)), pawr_sync);
#endif

#if (!ESL_RAM_OPTIMIZATION)
    blc_ll_init2MPhyCodedPhy_feature(); //need 2M/Coded PHY feature
#endif                                  //(!ESL_RAM_OPTIMIZATION)

    LL_FEATURE_MASK_1 |= LL_FEATURE_MASK_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER;

    blmsParam.prdSyncWr_en      = 1;
    bltPawrSync.maxNum_pawrSync = num_pawr_sync;

    ll_pawr_sync_sub_irq_task_cb = blt_pawr_sync_sub_interrupt_task;
    ll_pawr_sync_mlp_task_cb     = blt_pawr_sync_mainloop_task;

    ll_pawr_sync_rspTx_irq_task_cb = blt_ll_PAwRsync_rspSlotTxProc;

    ll_pda_sync_pawr_sync_common_cb = blt_ll_pdaSync_pawrSync_info_process;

    return BLE_SUCCESS;
}

ble_sts_t blc_ll_initPAwRsync_rspDataBuffer(u8 *pdaRspData, int maxLen_pdaRspData)
{
    if (pdaRspData == NULL) {
        return LL_ERR_INVALID_PARAMETER;
    }
#if (TSKNUM_PAWRS_SUB != TSKNUM_PDA_SYNC)
#error "PAWR and PDA number must be same !"
#endif

    if (bltPawrSync.maxNum_pawrSync > TSKNUM_PDA_SYNC) {
        return LL_ERR_INVALID_PARAMETER;
    }

    my_dump_str_data(1, "maxNum pawrSync", &bltPawrSync.maxNum_pawrSync, 1);
    for (int i = 0; i < bltPawrSync.maxNum_pawrSync; i++) {
        st_pda_sync_t *cur_pawr_sync = (st_pda_sync_t *)&pdAsync_tbl[i];

        cur_pawr_sync->pdaRspDataCtrl.rsp_max_dataLen = maxLen_pdaRspData;
        cur_pawr_sync->pdaRspDataCtrl.pRsp_data       = (u8 *)(pdaRspData + maxLen_pdaRspData * i);

        //do not know how to change the aux_sync_subevent_response header.
        cur_pawr_sync->auxSyncSubevtRsp_header.type     = LL_TYPE_AUX_SYNC_SUBEVENT_RSP;
        cur_pawr_sync->auxSyncSubevtRsp_header.txAddr   = 0;
        cur_pawr_sync->auxSyncSubevtRsp_header.rxAddr   = 0;
        cur_pawr_sync->auxSyncSubevtRsp_header.chan_sel = 0;
        cur_pawr_sync->auxSyncSubevtRsp_header.adv_mode = LL_EXTADV_MODE_NON_CONN_NON_SCAN;

        cur_pawr_sync->auxSyncSubevtRsp_header.ext_hdr_flg = 0;
        cur_pawr_sync->auxSyncSubevtRsp_header.ext_hdr_len = 0;
    }


    return BLE_SUCCESS;
}

/*
 * 7.8.126 LE Set Periodic Advertising Response Data command
 * blc_hci_le_setPeriodicAdvData
 */
ble_sts_t blc_hci_le_setPAwRsync_rspData(u16 sync_handle, u16 req_pdaEvtCnt, u8 req_subEvtCnt, u8 rsp_subEvtCnt, u8 rsp_slotIdx, u8 rspDataLen, u8 *pRspData)
{
    if (!blt_isSyncHandleValid(sync_handle)) {
        my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "set PDA response data, sync handle invalid", 0, 0);
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }


    u16            syncHandle      = sync_handle & 0x03;
    st_pda_sync_t *pPdaRspDataCtrl = (st_pda_sync_t *)&pdAsync_tbl[syncHandle];

    st_secchn_scn_t *pSecChn = (st_secchn_scn_t *)&secChnScn_tbl[pPdaRspDataCtrl->mapping_auxscan_idx];

    if (pRspData == NULL) {
        pPdaRspDataCtrl->pdaRspDataCtrl.user_setRspData_flag = 0;
        my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "set PDA response data, buffer not initial", 0, 0);
        return LL_ERR_INVALID_PARAMETER;
    }

    if (req_subEvtCnt != pPdaRspDataCtrl->pdaRspDataCtrl.req_subevt_idx) {
        pPdaRspDataCtrl->pdaRspDataCtrl.user_setRspData_flag = 0;
        my_dump_str_data(1, "rsp_subevt_idx not match", 0, 0);
        return BLE_SUCCESS;
    }

    if (rspDataLen > 251) {
        pPdaRspDataCtrl->pdaRspDataCtrl.user_setRspData_flag = 0;
        my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "set PDA response data, data length invalid", 0, 0);
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    st_pda_t *p_curPAwR;

    p_curPAwR = (st_pda_t *)&pPdaRspDataCtrl->pda_rx;

    if (p_curPAwR->pda_phy == BLE_PHY_1M) {
        pPdaRspDataCtrl->pdaRspDataCtrl.rsp_slot_duration_us = (rspDataLen + 10) * 8 + SLOT_PROCESS_MAX_US;
    } else if (p_curPAwR->pda_phy == BLE_PHY_2M) {
        pPdaRspDataCtrl->pdaRspDataCtrl.rsp_slot_duration_us = (rspDataLen + 10) * 4 + SLOT_PROCESS_MAX_US;
    } else { //BLE_PHY_CODED
        pPdaRspDataCtrl->pdaRspDataCtrl.rsp_slot_duration_us = (rspDataLen + 10) * 64 + SLOT_PROCESS_MAX_US;
    }

    my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "rsp slot dur us", &pPdaRspDataCtrl->pdaRspDataCtrl.rsp_slot_duration_us, 2);

    pPdaRspDataCtrl->pdaRspDataCtrl.sync_handle     = sync_handle;
    pPdaRspDataCtrl->pdaRspDataCtrl.req_event_count = req_pdaEvtCnt;
    //  pPdaRspDataCtrl->pdaRspDataCtrl.req_subevt_idx  = req_subEvtCnt;
    pPdaRspDataCtrl->pdaRspDataCtrl.rsp_subevt_idx = rsp_subEvtCnt;
    pPdaRspDataCtrl->pdaRspDataCtrl.rsp_slot_idx   = rsp_slotIdx;
    pPdaRspDataCtrl->pdaRspDataCtrl.rsp_real_len   = rspDataLen;

    smemcpy(pPdaRspDataCtrl->pdaRspDataCtrl.pRsp_data, pRspData, rspDataLen);

    pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_flg = 0;
    pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len = 0;
    if (pPdaRspDataCtrl->pdaSyncSubevtCtrl.sync_subevt_prop & EXTHD_BIT_TX_POWER) {
        pPdaRspDataCtrl->auxSyncSubevtRsp_header.data[pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len] = ble_txPowerLevel;
        pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len += EXTHD_LEN_1_TX_POWER;
        pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_flg |= EXTHD_BIT_TX_POWER;
    }
    if (pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len != 0) {
        pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len += 1;
    }

    pPdaRspDataCtrl->auxSyncSubevtRsp_header.rf_len  = pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len + 1 + rspDataLen;
    pPdaRspDataCtrl->auxSyncSubevtRsp_header.dma_len = rf_tx_packet_dma_len(pPdaRspDataCtrl->auxSyncSubevtRsp_header.rf_len + 2);

    my_dump_str_u32s(DBG_PAwR_SYNC_LOGIC, "set rsp data", pPdaRspDataCtrl->auxSyncSubevtRsp_header.ext_hdr_len, pPdaRspDataCtrl->auxSyncSubevtRsp_header.rf_len, rspDataLen,
                     pPdaRspDataCtrl->auxSyncSubevtRsp_header.dma_len);


    esl_devMgr.esl_rsp_slotIdx = rsp_slotIdx;
    pawr_acad_t *pSubevtInfor  = &blt_pPdAsync->pawr_acadInfo;

    u32 slot_offset_tick = pSubevtInfor->rsp_slot_delay * SYSTEM_TIMER_TICK_1250US + esl_devMgr.esl_rsp_slotIdx * pSubevtInfor->rsp_slot_spacing * SYSTEM_TIMER_TICK_1250US / 10;
    slot_offset_tick -= RSP_SLOT_EARLY_TICK;

    u32 rspSlot_expectTick = pPdaRspDataCtrl->pdaRspDataCtrl.aux_sync_subevt_ind_headerTick + slot_offset_tick;

    blt_add_aux_scan_future_task(pSecChn->scnIndex, pSecChn->scnIndex + TSKOFT_PAWRS_RSP, rspSlot_expectTick,
                                 rspSlot_expectTick + pPdaRspDataCtrl->pdaRspDataCtrl.rsp_slot_duration_us * SYSTEM_TIMER_TICK_1US);


    pPdaRspDataCtrl->pdaRspDataCtrl.user_setRspData_flag = 0;

/*
 * when only PAwR task exists and task intervel is too large(> 80ms),
 * the scheduler will insert task fail by delaying 1ms trigger, it need to update in time.
 * but the method need to be updated in the future.
 */
#if (UPDATE_SCHEDULER_FOR_PAWR_RSP)
    blt_ll_updateScheduler();
#else
    //  if(blms_state == BLMS_STATE_NONE || (blms_state & BLMS_STATE_PRICHN_SCAN_S))
    //  {
    //      u32 cur_tick = clock_time();
    //      if(tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 1*SYSTEM_TIMER_TICK_1MS)){
    //          systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
    //          systimer_set_irq_capture(cur_tick + 1*SYSTEM_TIMER_TICK_1MS);
    //      }
    //  }
#endif

    return BLE_SUCCESS;
}

/*
 * @brief      for user to set periodic sync subevent. refer to core 5.4 7.8.127 LE Set Periodic Sync Subevent command.
 * @param[in]  sync_handle  - identifying the PAwR train.
 * @param[in]  pda_prop     - indicates which fields should be included in the AUX_SYNC_SUBEVENT_RSP PDUs
 * @param[in]  num_subevent - Number of subevents; max is 128. i.e. 0x80
 * @param[in]  pSubevent    - the first buffer address of storing subevent value.
 * @return
 */
//note: this API need to be called after SYNC_STATE_SYNCED.
ble_sts_t blc_hci_le_setPeriodicSyncSubevent(u16 sync_handle, perd_adv_prop_t pda_prop, u8 num_subevent, u8 *pSubevent)
{
    if(sync_handle > 0xEFF){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if (!blt_isSyncHandleValid(sync_handle)) { //already judge SYNC_STATE_SYNCED, if not SYNC_STATE_SYNCED, return error.

        my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "set PDA sync subevent, sync handle invalid", 0, 0);
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    if (num_subevent > 128 || num_subevent == 0) {
        my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "set PDA sync subevent, subevent number is error", 0, 0);
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }


    u16 syncHandle = sync_handle & 0x03;

    my_dump_str_data(1, "syncHandle", &syncHandle, 2);
    st_pda_sync_t *pPAwR_sync = (st_pda_sync_t *)&pdAsync_tbl[syncHandle];
    //  st_pda_t* pPda = (st_pda_t *) &pPAwR_sync->pda_rx;

    if (!pPAwR_sync->pawr_acad_check) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    //pPdaSyncSubevtCtrl->pdaSyncSubevtCtrl.sync_subevt_prop = 0;
    if (pda_prop & EXTHD_BIT_TX_POWER) {
        pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt_prop |= PDA_SYNC_SUBEVT_PROP_INCLUDE_TXPOWER;
    }

    // unit of 1.25ms
    //u32 PAWR_ADV_WHOLE_PERIOD = pPAwR_sync->pawr_acadInfo.num_subevent * pPAwR_sync->pawr_acadInfo.subevent_intvl;
    // ((PAWR_ADV_WHOLE_PERIOD * 1.25) / 1000 ) * PPM_IDX_MAX * 100   1.25 = 5/ 4
    //PAWR_SYNC_INIT_PPM = (PAWR_ADV_WHOLE_PERIOD * PPM_IDX_MAX) >> 3; // no need extra 800 us
    //PAWR_SYNC_INIT_PPM = pPAwR_sync->pda_interval * 1.25 / 1000 * PPM_IDX_MAX * 100
    PAWR_SYNC_INIT_PPM = (pPAwR_sync->pda_interval >> 3) * PPM_IDX_MAX;
    PAWR_SYNC_PPM      = PAWR_SYNC_INIT_PPM;
    //need to sort and the sync_subevt[0] is the first subevent.subevent is from 0x00 to 0x7f.
    subevent_sort_process(pSubevent, num_subevent);

    u32 r = irq_disable();

    smemcpy(pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt, pSubevent, num_subevent);
    pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt_num = num_subevent;

    pPAwR_sync->pSubevtOfBuild = &pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt[0];

    my_dump_str_data(1, "subevt0idx", &pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt[0], 4);
    my_dump_str_data(1, "check", pPAwR_sync->pSubevtOfBuild, 4);

    pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt[num_subevent - 1] |= PAWR_LAST_SUBEVENT_FLAG;
    pPAwR_sync->pdaSyncSubevtCtrl.set_syncSubevt_flag = 1;
    pPAwR_sync->preSubevtIdx                          = 0;


    /*********************************************************************************************/
    /******switch PADVB to PAwR. task_mask will be set or deleted in blt_ll_procStateChange ******/
    blt_sche_disableTask(TSKMSK_PDA_SYNC_0 << pPAwR_sync->pda_index);
    blt_sche_enableTask(TSKMSK_PAWRS_SUB_0 << pPAwR_sync->pda_index);

    /*PAwR use the same task fifo as PADVB. here need to switch PAwR subevent flag.
     *when more times to call this API--blc_hci_le_setPeriodicSyncSubevent, schTsk_fifo not need to set every time.now temporary every time
     *scheTask_idx not need to set. because PAwR use the same PADVB advertising set */

    for (int j = 0; j < PRDADV_SYNC_FIFONUM; j++) {
        pPAwR_sync->schTsk_fifo[j].scheTask_oft = TSKOFT_PAWRS_SUB + pPAwR_sync->pda_index;
        pPAwR_sync->schTsk_fifo[j].scheTask_flg = TSKFLG_PAWRS_SUB;
    }


    /*switch the relevant secChnScn_tbl to PAwR response slot flag.secChnScn_tbl will be used as response slot*/
    st_secchn_scn_t *pSecChn = (st_secchn_scn_t *)&secChnScn_tbl[pPAwR_sync->mapping_auxscan_idx];
    pSecChn->pdaSync_flag    = PAWR_PACKET_FLAG;

    u8 tmpOft                       = pSecChn->auxScnTsk.scheTask_oft;
    pSecChn->auxScnTsk.scheTask_oft = tmpOft << (TSKOFT_PAWRS_RSP - TSKOFT_SECCHN_SCAN);
    pSecChn->auxScnTsk.scheTask_flg = TSKFLG_PAWRS_RSP; //TSKOFT_PAWRS_RSP


    u32 tmp = (TSKOFT_PAWRS_RSP - TSKOFT_SECCHN_SCAN);
    my_dump_str_u32s(1, "pawr task oft", tmp, 0, 0, 0);
    /**************************** End switch PADVB to PAwR ***************************************/
    /*********************************************************************************************/

#if 1
    //  pPda->paEvtCnt--;
    //  pPAwR_sync->prePawrEvtCnt = blt_pPda->paEvtCnt;

    blmsParam.state_chng |= STATE_CHANGE_PAWR_SYNC;

    //only consider primary scan. secondary scan can not.
    if (blms_state == BLMS_STATE_NONE || (blms_state & BLMS_STATE_PRICHN_SCAN_S)) {
        u32 cur_tick = clock_time();
        if (tick1_exceed_tick2(systimer_get_irq_capture(), cur_tick + 8 * SYSTEM_TIMER_TICK_1MS)) {
            systick_irq_trigger = SYS_IRQ_TRIG_SCHE_INSERT;
            systimer_set_irq_capture(cur_tick + 4 * SYSTEM_TIMER_TICK_1MS);
        }
    }
#endif

    irq_restore(r);

    return 0;
}

_attribute_ram_code_ int blt_ll_PAwRsync_rspSlotTxProc(int flag)
{
    //DBG_QIUWEI_CHN7_HIGH;
#if (SL01_PAWR_SYNC_RSP_SLOT_TX)
    log_task_begin_irq(DBG_PAwR_SYNC_TIMING, SL01_PAWR_SYNC_RSP_SLOT_TX);
#endif
    /* step 1: trigger RF mode  */
    if (blt_pPdAsync->pdaRspDataCtrl.pRsp_data == NULL) {
        BLMS_ERR_DEBUG(DBG_PAwR_SYNC_LOGIC, 0xFFEA1199);
    }


    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    blt_pSecChnScn = (st_secchn_scn_t *)&secChnScn_tbl[index];

    blt_pPdAsync = (st_pda_sync_t *)&pdAsync_tbl[blt_pSecChnScn->pdaSync_idx];
    blt_pPda     = (st_pda_t *)&blt_pPdAsync->pda_rx;

//2M/Coded PHY feature must be enabled for EXT SCAN, so do not use pointer "ll_phy_switch_cb"
#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    if (ll_phy_switch_cb) {
        ll_phy_switch_cb(blt_pPda->pda_phy, LE_CODED_S8); //rf_ble_switch_phy
    }
#endif


    /* Different process for different MCU: ******************************************/
    ble_rf_set_tx_dma(0, 17);
    //  This register exists in Kite/Vulture/826x, riscv architecture chips such as
    //  Eagle do not have this register, but DMA fifo can be turned off by forcibly
    //  setting DMA TX rptr = DMA TX wptr to use DMA default tx fifo.If the hardware
    //  TX rptr of DMA TX fifo == hardware TX wptr, then send DMA Tx default fifo,
    //  otherwise send DMA Tx fifo non-default area.
    HAL_REG_RF_DMA_FIFO_TX_RPTR = FLD_DMA_RPTR_CLR;
    /**********************************************************************************/


    //  rf_set_tx_rx_off();
    rf_set_ble_channel(blt_pPdAsync->sub_rspSlot_ChnIdx);
    rf_set_ble_access_code((u8 *)&blt_pPdAsync->pawr_acadInfo.rsp_AA);
    rf_trigger_codedPhy_accesscode();
    rf_set_ble_crc_value(blt_pPda->paCrcInit);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    rf_ble_set_tx_settle(bltPHYs.tx_stl_adv); //attention: must set after PHY switch !!!
    STOP_RF_STATE_MACHINE;                    // stop SM
    HAL_CLEAR_RF_TX_RX_IRQ;

////////
#define TX_PATH_DELAY_1M 5 //5us
    if (blt_pSecChnScn->pawr_rsp_tick == 0) {
        my_dump_str_data(1, "pawr rsp tick error", 0, 0); //error
    }
    u8  extraPreamble = (bltPHYs.cur_llPhy == BLE_PHY_1M) ? (PRMBL_LENGTH_1M - 1) : ((bltPHYs.cur_llPhy == BLE_PHY_2M) ? (PRMBL_LENGTH_2M - 1) : (PRMBL_LENGTH_Coded - 10));
    u32 tx_begin_tick = blt_pSecChnScn->pawr_rsp_tick + RSP_SLOT_EARLY_TICK - (TX_PATH_DELAY_1M + bltPHYs.tx_stl_adv) * SYSTEM_TIMER_TICK_1US -
                        extraPreamble * bltPHYs.own_oneByte_us * SYSTEM_TIMER_TICK_1US;
    blt_pSecChnScn->pawr_rsp_tick = 0;

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }
    rf_start_fsm(FSM_STX, &pawr_rspSlot_pkt, tx_begin_tick); //AaAa //blt_pPdAsync->pdaRspDataCtrl.pRsp_data


    smemcpy(&pawr_rspSlot_pkt, &blt_pPdAsync->auxSyncSubevtRsp_header, blt_pPdAsync->auxSyncSubevtRsp_header.ext_hdr_len - 1 + AUX_ADV_FORMAT_LEN);

    u8 extHeaderLen = 0;
    if (blt_pPdAsync->auxSyncSubevtRsp_header.ext_hdr_flg & EXTHD_BIT_TX_POWER) {
        extHeaderLen += EXTHD_LEN_1_TX_POWER;
        smemcpy(pawr_rspSlot_pkt.data + extHeaderLen, blt_pPdAsync->pdaRspDataCtrl.pRsp_data, blt_pPdAsync->pdaRspDataCtrl.rsp_real_len);
    } else {
        smemcpy(pawr_rspSlot_pkt.data - 1, blt_pPdAsync->pdaRspDataCtrl.pRsp_data, blt_pPdAsync->pdaRspDataCtrl.rsp_real_len);
    }

    while (!HAL_GET_RF_TX_IRQ) { //wait for TX finish
        if (usr_irq_handler_cb) {
            usr_irq_handler_cb();
        }
    }


    STOP_RF_STATE_MACHINE;                                                  //important: ensure that FSM stopped
    blt_ll_calculate_sSlot_next(clock_time() + 10 * SYSTEM_TIMER_TICK_1US); //SLOT_PROCESS_MAX_TICK
    blms_state = BLMS_STATE_EXTADV_E;
    /* clear status as late as possible, cause if clear too early, some status did not come, e.g. STX cmd done*/
    CLEAR_ALL_RFIRQ_STATUS;

    if (FALSE == blt_remove_aux_scan_future_task(blt_pSecChnScn->scnIndex)) {
        my_dump_str_u32s(DBG_PAwR_SYNC_LOGIC, "future task err", bltFutTask.number, bltExtA.extadv_sel, 0, 0);
    }

    //DBG_QIUWEI_CHN7_LOW;

#if (SL01_PAWR_SYNC_RSP_SLOT_TX)
    log_task_end_irq(DBG_PAwR_SYNC_TIMING, SL01_PAWR_SYNC_RSP_SLOT_TX);
#endif

    return 0;
}

_attribute_ble_data_retention_ s32 sSlot_idx_irq_real_for_pawr = 0;

static inline int blt_ll_PAwR_AFH_Phy_chnMap_proc(void) //adapted channel hopping //blt_pda_start_common_1
{
/* PHY switch, do not consider S2 */
//2M/Coded PHY feature must be enabled for EXT SCAN, so do not use pointer "ll_phy_switch_cb"
#if (LL_FEATURE_ENABLE_LE_2M_PHY || LL_FEATURE_ENABLE_LE_CODED_PHY)
    if (ll_phy_switch_cb) {
        ll_phy_switch_cb(blt_pPda->pda_phy, LE_CODED_S8); //rf_ble_switch_phy
    }
#endif

    //--------------- interval jump process --------------------------------------------------------//
    /*
     * Because bSlot_idx_irq_real will have a large advance,
     * it is necessary to consider different subevents */
    u8  curSubevtIdx     = bltSche.pTask_cur->pawr_subevt_idx & 0x7F;
    int pawrEvt_jump_num = 0;

    if (curSubevtIdx * blt_pPdAsync->subevtIntvl_sSlot < blt_pPda->sSlot_prdadv_itvl >> 2) {
        pawrEvt_jump_num = (bltSche.bSlot_idx_irq_real + (blt_pPda->bSlot_prdadv_itvl >> 1) - blt_pPda->bSlot_mark_prdadv) / blt_pPda->bSlot_prdadv_itvl;
    } else {
        pawrEvt_jump_num = (bltSche.bSlot_idx_irq_real - blt_pPda->bSlot_mark_prdadv) / blt_pPda->bSlot_prdadv_itvl;
    }


    u16 tmpPdaEvtCnt = blt_pPda->paEvtCnt + pawrEvt_jump_num;

    if (tmpPdaEvtCnt != blt_pPda->paEvtCnt) {
        blt_pPda->paEvtCnt          = tmpPdaEvtCnt;
        blt_pPdAsync->preSubevtIdx  = 0;
        blt_pPda->bSlot_mark_prdadv = bltSche.bSlot_idx_irq_real - curSubevtIdx * (blt_pPdAsync->subevtIntvl_sSlot >> 5);
        //blt_pPda->bSlot_mark_prdadv = blt_pPda->bSlot_mark_prdadv + blt_pPda->bSlot_prdadv_itvl * pawrEvt_jump_num - curSubevtIdx * (blt_pPdAsync->subevtIntvl_sSlot >> 5);
        sSlot_idx_irq_real_for_pawr = bltSche.sSlot_idx_irq_real - curSubevtIdx * blt_pPdAsync->subevtIntvl_sSlot;
        blt_pPdAsync->sSlot_mark_prdadv += blt_pPda->sSlot_prdadv_itvl * pawrEvt_jump_num;
    }

#if (SL16_PAWR_EVT_CNT)
    log_b16_irq(DBG_PAwR_SYNC_TIMING, SL16_PAWR_EVT_CNT, blt_pPda->paEvtCnt);
#endif

    //--------------- channel map update ------------------------------------------------------------
    //pay attention here, PAD_STX slot may dropped, blt_pPda->paEvtCnt >= blt_pPda->prd_map_inst_next(consider 0xffff->0 problem, (u16).... < 1024 )
    if ((blt_pPda->update_map == PDA_UPDATE_MAP) && (u16)(blt_pPda->paEvtCnt - blt_pPda->prd_map_inst_next) < BIT(10)) {
        blt_pPda->update_map = 0;
        //blt_pPda->update_map = 0;// do this job in API: blt_pda_post_common.
        smemcpy(&blt_pPda->chnParam.map, &blt_pPda->nextChn, sizeof(struct le_channel_map));
        my_dump_str_data(0, "pad:chm update", &blt_pPda->paEvtCnt, 2);
    }


    /* Update RF channel (use CSA#2 subevent channel) */
    /* subevent jump, each jump need to calculate for the next subevent */
    u8 tmpPreSubevtCnt = blt_pPdAsync->preSubevtIdx;
    for (int j = 0; j < (curSubevtIdx - tmpPreSubevtCnt); j++) {
        blt_pPdAsync->preSubevtIdx++;
        blt_ll_generateNextChannel(&blt_pPda->chnParam, blt_pPda->paEvtCnt ^ (blt_pPdAsync->preSubevtIdx - 1), blt_pPda->chnIdentifier, 1);
    }

    /* Update RF channel (use CSA#2 subevent channel) */
    blt_pPdAsync->preSubevtIdx++;
    blt_pPdAsync->sub_rspSlot_ChnIdx = blt_ll_generateNextChannel(&blt_pPda->chnParam, blt_pPda->paEvtCnt ^ (blt_pPdAsync->preSubevtIdx - 1), blt_pPda->chnIdentifier, 1);

#if (SL08_PAWR_SUBEVT_IDX)
    log_b8_irq(DBG_PAwR_SYNC_TIMING, SL08_PAWR_SUBEVT_IDX, blt_pPdAsync->preSubevtIdx);
#endif

#if (SL16_PAwR_chnIdx)
    log_b16_irq(DBG_PAwR_SYNC_TIMING, SL16_PAwR_chnIdx, blt_pPdAsync->sub_rspSlot_ChnIdx);
#endif

    //  simulateUart_send_str_data (&blt_pPdAsync->sub_rspSlot_ChnIdx, 2);

    rf_set_ble_channel(blt_pPdAsync->sub_rspSlot_ChnIdx);

    rf_set_ble_access_code((u8 *)&blt_pPda->paAccessAddr);
    rf_trigger_codedPhy_accesscode();
    rf_set_ble_crc_value(blt_pPda->paCrcInit);

    blt_ll_set_tx_power_by_strategy(TX_POWER_STRATEGY_CUSTOMER_OR_DEFAULT, 0);

    my_dump_str_u32s(0, "pawr subevt start ", blt_pPda->paAccessAddr, blt_pPda->paCrcInit, blt_pPdAsync->sub_rspSlot_ChnIdx, 0);

    return pawrEvt_jump_num;
}

_attribute_ram_code_ void blt_ll_PAwRsync_build_task(void)
{
    u32 i, j;

    st_pda_sync_t *pPdA_sync;
    st_pda_t      *p_curPda;

    int int_jump_pawrEvtTsk;
    int int_jump_pawrSubevtTsk;
    s32 sSlot_pawrEvt_start_task;
    s32 sSlot_subevent_task;

#if (SL01_PAWR_SYNC_build)
    log_task_begin_irq(DBG_PAwR_SYNC_TIMING, SL01_PAWR_SYNC_build);
#endif

    for (i = 0; i < bltPawrSync.maxNum_pawrSync; i++) {
        if (bltSche.task_mask & (TSKMSK_PAWRS_SUB_0 << i)) {
//DBG_QIUWEI_CHN5_TOGGLE;
#if (SLEV_PAWR_SYNC_TASK_BUILD)
            log_event_irq(DBG_PAwR_SYNC_TIMING, SLEV_PAWR_SYNC_TASK_BUILD);
#endif

            pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[i];
            p_curPda  = (st_pda_t *)&pPdA_sync->pda_rx;

////////////////////////////////////////////////////////
#if (PDA_SYNC_TIMING_ADJUST_EN) //later add
            if (pawr_sync_timingAdjust[i].sSlot_offset) {
                pPdA_sync->sSlot_mark_prdadv += pawr_sync_timingAdjust[i].sSlot_offset;
                pawr_sync_timingAdjust[i].sSlot_offset = 0;
            }
#endif
            ////////////////////////////////////////////////////////

            //my_dump_str_data(1, "pawr build mark", &pPdA_sync->sSlot_mark_prdadv, 4);

            if (bltSche.build_index == 0 && bltSche.sSlot_idx_reset == 1) {
                my_dump_str_data(1, "sSlot reset", 0, 0);
                pPdA_sync->sSlot_mark_prdadv -= bltSche.sSlot_idx_past;
            }

            //find the task start sSlot.
            if (pPdA_sync->sSlot_mark_prdadv >= bltSche.sSlot_idx_next) {
                my_dump_str_data(1, "pawr build mark error", 0, 0); //DBG_PAwR_SYNC_LOGIC
                BLMS_ERR_DEBUG(DBG_PAwR_SYNC_LOGIC, 0xFFEA1122);
            }

            //find the nearest PAwR event mark before current time.
            int_jump_pawrEvtTsk = (bltSche.sSlot_idx_next - 1 - pPdA_sync->sSlot_mark_prdadv) / p_curPda->sSlot_prdadv_itvl;
            if (int_jump_pawrEvtTsk == 0) {
                sSlot_pawrEvt_start_task = pPdA_sync->sSlot_mark_prdadv;
            } else {
                sSlot_pawrEvt_start_task = pPdA_sync->sSlot_mark_prdadv + int_jump_pawrEvtTsk * p_curPda->sSlot_prdadv_itvl;
            }


            //find the nearest subevent before current time.
            int_jump_pawrSubevtTsk       = (bltSche.sSlot_idx_next - 1 - sSlot_pawrEvt_start_task) / pPdA_sync->subevtIntvl_sSlot + 1;
            s32 sSlot_jump_pawrSubevtTsk = int_jump_pawrSubevtTsk * pPdA_sync->subevtIntvl_sSlot;

            if (sSlot_jump_pawrSubevtTsk > p_curPda->sSlot_prdadv_itvl) {
                sSlot_subevent_task = sSlot_pawrEvt_start_task + p_curPda->sSlot_prdadv_itvl;
            } else {
                sSlot_subevent_task = sSlot_pawrEvt_start_task + sSlot_jump_pawrSubevtTsk;
            }
            if (sSlot_subevent_task >= bltSche.sSlot_endIdx_dft) { //to save some time for big interval
                continue;                                          //attention: can not use break !!!
            }
            //reset the subevent idx and point to the first subevent. per build reset the pSubeventOfBuild and not need to store value.
            //only according to the timing to allocate task. Even if some subevent has been allocated before but not executed.
            //when build, can build them and allocate again according to priority.
            pPdA_sync->pSubevtOfBuild = &pPdA_sync->pdaSyncSubevtCtrl.sync_subevt[0];

            //         |<--subItv-->|
            //         |____|       |____|       |____|
            //idx_next        |0    |1  |1
            //                s0    s1  s2
            //subevent is from 0 to 0x7f. and store in order. so jump is same as the subevent index.
            int k = 0;
            for (k = 0; k < pPdA_sync->pdaSyncSubevtCtrl.sync_subevt_num; k++) {
                if ((*pPdA_sync->pSubevtOfBuild & 0x7F) < int_jump_pawrSubevtTsk) {
                    pPdA_sync->pSubevtOfBuild++;
                } else {
                    break;
                }
            }
            if (k == pPdA_sync->pdaSyncSubevtCtrl.sync_subevt_num) {
                //my_dump_str_u32s(1, "PAwREvt start sSlot;next idx sSlot", sSlot_pawrEvt_start_task, bltSche.sSlot_idx_next, bltSche.sSlot_endIdx_dft, 0);

                //judge whether the next PAwR event anchor point is in this building period(80ms). i.e. across end idx.
                s32 next_pawrIntvl_sSlot = sSlot_pawrEvt_start_task + p_curPda->sSlot_prdadv_itvl;
                if (next_pawrIntvl_sSlot >= bltSche.sSlot_endIdx_dft) {
#if (SLEV_PAWR_SYNC_DEBUG0)
                    log_event_irq(1, SLEV_PAWR_SYNC_DEBUG0);
#endif
                    continue; //if the subevent has been executed, exist
                }

                //else logical code
                sSlot_pawrEvt_start_task  = next_pawrIntvl_sSlot;
                pPdA_sync->pSubevtOfBuild = &pPdA_sync->pdaSyncSubevtCtrl.sync_subevt[0]; //reset and start the first subevent that user config.
            }

            if ((sSlot_pawrEvt_start_task + (*pPdA_sync->pSubevtOfBuild & 0x7F) * pPdA_sync->subevtIntvl_sSlot) >= bltSche.sSlot_endIdx_dft) {
                continue;
            }
            ////////////////////////////////////////

            int new_task_cnt = 0;

            for (j = 0; j < PAWR_SYNC_FIFONUM; j++) {
                //PAwR_sync use the same schTsk_fifo as PADVB_SYNC.
                sch_task_t *pCur_schTask  = (sch_task_t *)&pPdA_sync->schTsk_fifo[j];
                u8          cur_subevtIdx = *pPdA_sync->pSubevtOfBuild; //maybe the MSB is used as LAST flag.

                u32 nextSyncSubevtOffset_sSlot = sSlot_pawrEvt_start_task + (cur_subevtIdx & 0x7F) * pPdA_sync->subevtIntvl_sSlot;

                pCur_schTask->begin           = nextSyncSubevtOffset_sSlot - PAWR_SYNC_PPM * SYSTICK_NUM_PER_US * SSLOT_TICK_REVERSE;
                pCur_schTask->end             = pCur_schTask->begin + p_curPda->sSlot_duration_pda - 1 + 2 * PAWR_SYNC_PPM * SYSTICK_NUM_PER_US * SSLOT_TICK_REVERSE;
                pCur_schTask->pawr_subevt_idx = cur_subevtIdx;


                if (pCur_schTask->begin >= bltSche.sSlot_endIdx_dft) { //new task beyond correct range, finish
#if (SLEV_PAWR_SYNC_DEBUG1)
                    log_event_irq(1, SLEV_PAWR_SYNC_DEBUG1);
#endif
                    break;
                } else if (pCur_schTask->end < bltSche.sSlot_endIdx_dft) { //new task in correct range
                    new_task_cnt++;
                } else { //new task across "sSlot_endIdx_dft"
                    //for those task across end_idx, find the task with highest priority, to guarantee that task not missed
                    if (bltPri.pri_cal[TSKOFT_PAWRS_SUB + i] > bltPri.priMax_value) {
                        bltPri.priMax_value         = bltPri.pri_cal[TSKOFT_PAWRS_SUB + i];
                        bltPri.priMax_index         = TSKOFT_PAWRS_SUB + i;
                        bltSche.sSlot_endIdx_maxPri = pCur_schTask->begin;
                    }

                    break;
                }

                //judge whether update the next subevent
                if (cur_subevtIdx & PAWR_LAST_SUBEVENT_FLAG) { //process the situation across PAwR event.
#if (SLEV_PAWR_SYNC_DEBUG2)
                    log_event_irq(DBG_PAwR_SYNC_TIMING, SLEV_PAWR_SYNC_DEBUG2);
#endif
                    sSlot_pawrEvt_start_task += p_curPda->sSlot_prdadv_itvl;
                    pPdA_sync->pSubevtOfBuild = &pPdA_sync->pdaSyncSubevtCtrl.sync_subevt[0];

                    pCur_schTask->pawr_last_subevt = 1;
                } else {
                    pPdA_sync->pSubevtOfBuild++;
                    pCur_schTask->pawr_last_subevt = 0;
                }

            } //(j=0;j<PAWR_SYNC_FIFONUM;j++)


            if (new_task_cnt) {
//blt_ll_incSchedulerTaskCalPriority( TSKOFT_PDA_SYNC + i, 2 * int_jump_pawrEvtTsk );
#if (SLEV_PAWR_SYNC_DEBUG3)
                log_event_irq(DBG_PAwR_SYNC_TIMING, SLEV_PAWR_SYNC_DEBUG3);
#endif
                blt_ll_addTask2ExistLinklist(&pPdA_sync->schTsk_fifo[0], new_task_cnt);
            }


        } //bltSche.task_mask & (TSKMSK_PAWRS_SUB_0<<i)

    } //for(i=0; i<TSKNUM_PAWRS_SUB; i++)

#if (SL01_PAWR_SYNC_build)
    log_task_end_irq(DBG_PAwR_SYNC_TIMING, SL01_PAWR_SYNC_build);
#endif
}

_attribute_ram_code_ void blt_ll_PAwRsync_subevtStart(u8 index)
{
#if (SL01_PAWR_SYNC_SUB)
    log_task_begin_irq(DBG_PAwR_SYNC_TIMING, SL01_PAWR_SYNC_SUB);
#endif

    //DBG_QIUWEI_CHN6_HIGH;
    PAWR_SYNC_FLAG          = 1;
    bltPdaSync.pdA_sync_sel = index;
    blt_pPdAsync            = (st_pda_sync_t *)&pdAsync_tbl[index];
    blt_pPda                = (st_pda_t *)&blt_pPdAsync->pda_rx;
    blt_pSecChnScn          = (st_secchn_scn_t *)&secChnScn_tbl[blt_pPdAsync->mapping_auxscan_idx];

    /* 1. special case, when sync_lost, remove task_mask, but not update link_list immediately, task may exist
     *    in rest of 80mS link_list timing, use sync_state_idle to control task not execute
     * 2. RX FIFO not released, can not scan, must abandon this aux_scan task  */
    if (blt_pPdAsync->sync_state == SYNC_STATE_IDLE || ((u8)(scan_secRxFifo.wptr - scan_secRxFifo.rptr) & 31) >= SCAN_SECCHN_RXFIFO_NUM) {
        systimer_set_irq_capture(bltSche.sSlot_tick_irq + 100 * SYSTEM_TIMER_TICK_1US);
        blmsParam.rf_fsm_busy = 0;
    } else {
        int tmp_jump = blt_ll_PAwR_AFH_Phy_chnMap_proc(); //refer to blt_pda_start_common_1();

        if (tmp_jump > 0) {
            blt_pPdAsync->sync_err_cnt += tmp_jump;
        }

        rf_ble_set_rx_settle(RX_SETTLE_US);

        if (blc_rf_pa_cb) {
            blc_rf_pa_cb(PA_TYPE_RX_ON);
        }
        rf_start_fsm(FSM_SRX, NULL, clock_time());


        //u16 pdaSync_1stRxTm_margin = blt_pPda->pda_phy == BLE_PHY_CODED ? 300:0;
        //rf_set_1st_rx_timeout(blt_pPdAsync->sync_early_set_us + bltPHYs.prmb_ac_us + 150 + pdaSync_1stRxTm_margin);
        rf_set_1st_rx_timeout(0x0fffffff);

        ble_rf_set_rx_dma((u8 *)bltExtScn.scan_rx_sec_chn_dma_buff, bltExtScn.scan_rx_sec_chn_dma_size);
        //rf_set_rx_maxlen(blt_pSecChnScn->rfLen_max);
        rf_set_rx_maxlen(80); //ESL profile: not exceed 48 bytes and consider the overhead, that should not larger than 65bytes.

        systimer_set_irq_capture(bltSche.sSlot_tick_irq + (blt_pPdAsync->pda_duration_us + 2 * PAWR_SYNC_PPM) * SYSTEM_TIMER_TICK_1US);

        blmsParam.rf_fsm_busy = 1;

        blt_pSecChnScn->tolerance_peer_us = blt_pPdAsync->tolerance_pda_us;
        blt_pSecChnScn->aux_expect_tick   = bltSche.sSlot_tick_irq + blt_pPdAsync->sync_early_set_us * SYSTEM_TIMER_TICK_1US;
    }


    /* logic setting executing after SRX setting to save time */
    auxScnCmnParam.rx_received = 0;


    blms_state          = BLMS_STATE_PAWRS_SUB_S;
    systick_irq_trigger = SYS_IRQ_TRIG_PAWRS_SUB_POST;

    blt_pSecChnScn->scan_rx_flag = 0;

#if (PAWR_SYNC_TIMING_ADJUST_EN)
    pawr_sync_timingAdjust[index].rx_1st_tick   = 0;
    pawr_sync_timingAdjust[index].timing_update = 0;
#endif
    //AUX_SYNC_SUBEVENT_IND not include chain packet.
}

_attribute_ram_code_ void blt_ll_PAwRsync_subevtPost(void)
{
#if (SL01_PAWR_SYNC_SUB)
    log_task_end_irq(DBG_PAwR_SYNC_TIMING, SL01_PAWR_SYNC_SUB);
#endif

    if (blmsParam.rf_fsm_busy) {
        STOP_RF_STATE_MACHINE;
        blmsParam.rf_fsm_busy           = 0;
        blmsParam.delay_clear_rf_status = 1;
    }


    //blt_pPdAsync->pda_expect_tick = (auxScnCmnParam.rx_received ? bltRxPkt.rx_header_tick : blt_pPdAsync->pda_expect_tick) + blt_pPdAsync->pda_expect_tick;
    if (auxScnCmnParam.rx_received) {
        blt_pPdAsync->sync_err_cnt = 0;
        blt_pPdAsync->sync_rx_tick = clock_time();
    } else {
        blt_pPdAsync->sync_err_cnt++;

        if (blt_pPdAsync->sync_state == SYNC_STATE_SYNCED) {
            if (blt_pPdAsync->sync_err_cnt > 10) {
                blt_ll_setSchedulerTaskPriority(TSKOFT_PAWRS_SUB + blt_pPdAsync->pda_index, TASK_PRIORITY_LOW);
                blt_pPdAsync->sync_err_cnt = 5;
            } else {
                blt_ll_incSchedulerTaskPriority(TSKOFT_PAWRS_SUB + blt_pPdAsync->pda_index, bltPri.step_final[TSKOFT_PAWRS_SUB + blt_pPdAsync->pda_index]);
            }
        }
        PAWR_SYNC_PPM += PAWR_SYNC_INIT_PPM / 2;
    }


    if (blt_pPdAsync->sync_state == SYNC_STATE_SYNCED) {
        if (auxScnCmnParam.rx_received) {
#if (PAWR_SYNC_TIMING_ADJUST_EN)
            u8 syncHandle = bltPdaSync.pdA_sync_sel;

            if (pawr_sync_timingAdjust[syncHandle].rx_1st_tick) {
                u32 tick_offset_1st_rx = pawr_sync_timingAdjust[syncHandle].rx_1st_tick - bltSche.sSlot_tick_irq_real;
#if (BLMS_PM_ENABLE)
                //u32 tick_offset_expect = (blmsParam.min_tolerance_us + PAWR_SYNC_PPM)* SYSTEM_TIMER_TICK_1US + (BRX_EARLY_SET_TICK + BRX_HALF_MARGIN_TICK);
                u32 tick_offset_expect = (blmsParam.min_tolerance_us) * SYSTEM_TIMER_TICK_1US + (BRX_EARLY_SET_TICK + BRX_HALF_MARGIN_TICK);
#else
                u32 tick_offset_expect = BRX_EARLY_SET_TICK + BRX_HALF_MARGIN_TICK;
#endif
                pawr_sync_timingAdjust[syncHandle].sSlot_offset = (signed int)(tick_offset_1st_rx - tick_offset_expect) * SSLOT_TICK_REVERSE;

                if (pawr_sync_timingAdjust[syncHandle].sSlot_offset < -1 || pawr_sync_timingAdjust[syncHandle].sSlot_offset > 2) {
                    pawr_sync_timingAdjust[syncHandle].timing_update = 1;
                    blt_sche_addUpdate(SLOT_UPDT_SLAVE_SSLOT_ADJUST);
                }
            }
#endif
            if (blt_pPdAsync->prePawrEvtCnt != blt_pPda->paEvtCnt) {
                blt_pPdAsync->prePawrEvtCnt     = blt_pPda->paEvtCnt;
                blt_pPdAsync->sSlot_mark_prdadv = sSlot_idx_irq_real_for_pawr;
            }
        } else if ((u32)(clock_time() - blt_pPdAsync->sync_rx_tick) > blt_pPdAsync->sync_timeout_tick) {
            blt_pPdAsync->sync_state          = SYNC_STATE_IDLE;
            blt_pPdAsync->sync_lost           = 1;
            blt_pPdAsync->sync_establish      = 0;
            blt_pPdAsync->sync_adv_dup_filter = 0xFFFF;
            PAWR_SYNC_FLAG                    = 0;
            my_dump_str_data(1, "pawr sync lost mmm", &blt_pPdAsync->sync_timeout_tick, 4);

            blt_pSecChnScn->occupied = 0;
            //blt_release_secchn_scan(blt_pSecChnScn, bltExtScn.auxadv_sel);   //clear occupied
            blt_pSecChnScn->pdaSync_flag                            = 0;
            blt_pSecChnScn->auxScnTsk.scheTask_flg                  = TSKFLG_SECCHN_SCAN;
            pdaCache_tbl[blt_pPdAsync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

            blt_sche_disableTask(TSKMSK_PAWRS_SUB_0 << blt_pPdAsync->pda_index);
            blmsParam.state_chng |= STATE_CHANGE_PAWR_SYNC;

            //later will use other process
            for (int j = 0; j < PRDADV_SYNC_FIFONUM; j++) {
                blt_pPdAsync->schTsk_fifo[j].scheTask_oft = TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index;
                blt_pPdAsync->schTsk_fifo[j].scheTask_flg = TSKFLG_PDA_SYNC;
            }

            /////////////////////////////////////
            blmsParam.pda_syncing_flg = 0;
            bltPdaSync.tick_pda_sync  = 0;
        }
    } else {
        //debug
        if (blt_pPdAsync->sync_state != SYNC_STATE_IDLE) {
            BLMS_ERR_DEBUG(DBG_PAwR_SYNC_LOGIC, 0xFD010000 | blt_pPdAsync->sync_state);
        }
    }


    blt_ll_calculate_sSlot_next(clock_time() + SLOT_PROCESS_MAX_TICK);


    blms_state = BLMS_STATE_PAWRS_SUB_E;
    if (auxScnCmnParam.rx_received) {
        PAWR_SYNC_PPM = PAWR_SYNC_INIT_PPM;
    }
    //DBG_QIUWEI_CHN6_LOW;
}

static inline void blt_ll_PAwRsync2AclConn(void)
{
    blt_pPdAsync->sync_state          = SYNC_STATE_IDLE;
    blt_pPdAsync->sync_lost           = 1;
    blt_pPdAsync->sync_adv_dup_filter = 0xFFFF;
    PAWR_SYNC_FLAG                    = 0;
    blt_pSecChnScn->occupied          = 0;
    blt_pSecChnScn->pdaSync_flag      = 0;

    pdaCache_tbl[blt_pPdAsync->mapping_cache_idx].cach_flag = CACHE_FLAG_IDLE;

    blt_sche_disableTask(TSKMSK_PAWRS_SUB_0 << blt_pPdAsync->pda_index);
    blmsParam.state_chng |= STATE_CHANGE_PAWR_SYNC;

    //later will use other process
    for (int j = 0; j < PRDADV_SYNC_FIFONUM; j++) {
        blt_pPdAsync->schTsk_fifo[j].scheTask_oft = TSKOFT_PDA_SYNC + blt_pPdAsync->pda_index;
        blt_pPdAsync->schTsk_fifo[j].scheTask_flg = TSKFLG_PDA_SYNC;
    }

    /////////////////////////////////////
    blmsParam.pda_syncing_flg = 0;
    bltPdaSync.tick_pda_sync  = 0;
}

_attribute_ram_code_ u8 blt_ll_PAwRsync_auxConnInd_proc(rf_pkt_ext_adv_t *pAuxConnReq)
{
    rf_pkt_adv_rx_t *pAdvRx = (rf_pkt_adv_rx_t *)(pAuxConnReq);

    if (smemcmp(pAdvRx->advA, pkt_Adv.advA, BLE_ADDR_LEN) || pAdvRx->rxAddr != pkt_Adv.txAddr) {
        return PAWR_CONN_RTN_ADDR_NO_MATCH;
    }
    //u16 *advA16 = (u16 *)bltMac.macAddress_public;//local device's Address, SiHui have confirmed that "data" is AdvA //bltMac.macAddress_random
    //u16 *peerSearchA16 = (u16 *)pAdvRx->advA;     //advA in "AUX_SCAN_REQ" and "AUX_CONNECT_REQ"

    blt_quick_tx_prepare(FSM_STX, (void *)&pkt_aux_conn_rsp_pawr, pAdvRx->rf_len); //when 48M, diff is 31us.

    if (blc_rf_pa_cb) {
        blc_rf_pa_cb(PA_TYPE_TX_ON);
    }


    //  if (MAC_MATCH16(peerSearchA16, advA16))
    {
        /*                                     AUX_SCAN_REQ  AUX_CONNECT_REQ
        Connectable Undirected  AUX_ADV_IND :       NO              YES
        Connectable   Directed  AUX_ADV_IND :       NO              YES_2
        Scannable   Undirected  AUX_ADV_IND :       YES             NO
        Scannable     Directed  AUX_ADV_IND :       YES_3           NO

        YES_2 :     Initiators other than the correctly addressed initiator shall not respond.
        YES_3 :     Scanners other than the correctly addressed scanner shall not respond.
        */
        do {
            u8 filter_enable = 0;
            /* step 1, quick check if scan_req or connect_req basic logic pass
            *         skill:  Put the hardest conditions first */
            if (pAdvRx->rf_len == 34) { //aux_conn_req //&& blt_pextadv->conReq_response
                //filter_enable = blt_pextadv->adv_filterPolicy & ALLOW_CONN_WL;
                blc_rcvd_connReq_tick = clock_time(); //need to provide API to host and need to distinguish connHandle
            } else {
                my_dump_str_u8s(1, "PAwR receive error AUX_CONNECT_REQ", pAdvRx->rf_len, 0, 0, 0);
                break; //stop
            }

            ll_resolv_list_t *pRL_match   = NULL;
            u8                peer_is_rpa = IS_RESOLVABLE_PRIVATE_ADDR(pAdvRx->txAddr, pAdvRx->peerA);

            /* step 2, network privacy ignore IDA process */

            blt_ll_addr_set_peer_address(0, pAdvRx->txAddr, pAdvRx->peerA);

            /* step 3, resolve RPA if needed, check whiteList if filtering needed
            * for peerA RPA
            * 1. scan_req, resolve if filter needed; do not resolve if no filter
            * 2. conn_req, if direct ADV,      must resolve, than can check if addressed to local device
            *              if none direct ADV, resolve if filtering needed; do not resolve if no filter
            * Consider accept list, change to:
            * 1. if direct ADV, must resolve, than check if addressed to local device, never use filtering/accept list
            * 2. if none direct ADV, resolve RPA depend on filtering, then check accept list
            */
            if (0 && filter_enable) {
#if (!ESL_RAM_OPTIMIZATION)
                if (peer_is_rpa) {
                    pRL_match = blt_ll_resolve_rpa(0, pAdvRx->peerA, blt_pextadv->pRslvlst_extAdv);
                    if (pRL_match) {
                        blt_ll_storePeerDeviceRpa(pRL_match, pAdvRx->peerA);
                        blt_ll_addr_set_peer_address(1, pRL_match->rlIdAddrType, pRL_match->rlIdAddr);
                        my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer RPA resolve OK", pRL_match->rlIdAddr, 6);
                    } else {
                        my_dump_str_data(DBG_PRVC_EXTADV_EN, "peer RPA resolve ERR, stop", 0, 0);
                        break;
                    }
                }
#endif               //(!ESL_RAM_OPTIMIZATION)
            } else { //none direct ADV, no filter, pass without any check
                     /* consider later : even no filter, maybe we need try to resolve RPA, because
               * 1. enhanced connection complete event: Peer_Address should be IDA when RPA can be resolved
               * 2. for AUX_CONNECT_RSP, targetA should not be same value with initA in AUX_CONNECT_REQ,
               *    so here need find out the RL entry by resolving RPA
               */
            }


            /* final step, respond to scan_req(send scan_rsp) or conn_req(connect) */
            //AUX_CONNECT_REQ
            my_dump_str_data(1, "extadv, accept aux_conn_req", 0, 0);

            rf_set_tx_packet_address(&pkt_aux_conn_rsp_pawr); //get ready TX packet data
            blc_rcvd_connReq_tick = clock_time();


            /* txAddr & advA process for "AUX_CONNECT_RSP" */
            pkt_aux_conn_rsp_pawr.txAddr = pAdvRx->rxAddr;
            smemcpy(pkt_aux_conn_rsp_pawr.advA, pAdvRx->advA, BLE_ADDR_LEN);

            /* copy from initA of AUX_CONNECT_REQ, may not accepted by peer device if they are very strict
             * LL/SEC/ADV/BV-23 ~ LL/SEC/ADV/BV-26
             * LL/CON/CEN/BV-84-C /LL/CON/PER/BV-88-C tested OK */
            pkt_aux_conn_rsp_pawr.rxAddr = pAdvRx->txAddr;
            smemcpy(pkt_aux_conn_rsp_pawr.targetA, pAdvRx->peerA, BLE_ADDR_LEN);


            // Timing executing codes below must ensure that AUX_CONNECT_RSP sending successfully
            //1M PHY no problem, but when Coded PHY, we should consider the timing
            if (ll_adv_2_slave_cb == NULL) {
                break;
            }


            if (TRUE == ll_adv_2_slave_cb((rf_packet_connect_t *)pAdvRx, TRUE)) { // blt_s_connect()
                while (!HAL_GET_RF_TX_IRQ) {                                      //wait for TX finish
                    if (usr_irq_handler_cb) {
                        usr_irq_handler_cb();
                    }
                }

                //blt_ll_PAwRsync2AclConn(); //switch acl state, and remove pawr sync state.
                return PAWR_CONN_RTN_FAIL;
            }

        } while (0);
    }

    return PAWR_CONN_RTN_SUCCESS;
}

_attribute_ram_code_ u8 blt_ll_PAwRsync_auxSyncSubevtInd_Proc(rf_pkt_ext_adv_t *pExtAdv, u8 *pExtHdrOffset)
{
    (void)pExtHdrOffset; //unused, remove warning

    /* AdvA & TargetA & Sync Info can not exist,  CTE Info & Aux Ptr & Tx Power optional */
    if (pExtAdv->ext_hdr_len != 0 && ((pExtAdv->ext_hdr_flg & (EXTHD_BIT_ADVA | EXTHD_BIT_TARGETA | EXTHD_BIT_AUX_PTR | EXTHD_BIT_SYNC_INFO)) != 0)) {}

    s16 AdvDataLen = pExtAdv->rf_len - pExtAdv->ext_hdr_len - 1;
    if (AdvDataLen < 0) {
        my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "ERROR, AUX_SYNC_SUBEVENT_IND advdata len error!!!", 0, 0);
    }

#if (SLEV_PAWR_SYNC_RX)
    log_event_irq(DBG_PAwR_SYNC_TIMING, SLEV_PAWR_SYNC_RX);
#endif

    u16 *paEvt_subevt = NULL;

    if (pExtAdv->ext_hdr_len != 0 || 1) {
        //not the response subevent that user set, so not need to response in this subevent.
        //      if( blt_pPdAsync->pdaRspDataCtrl.rsp_subevt_idx != (bltSche.pTask_cur->pawr_subevt_idx&0x7F)){
        //          goto PAWR_REPORT;
        //      }
        //
        //      if(!blt_pPdAsync->pdaRspDataCtrl.user_setRspData_flag){
        //          goto PAWR_REPORT; //there are not response data to send.
        //      }
        //
        //      blt_pPdAsync->pdaRspDataCtrl.user_setRspData_flag = 0;

#if 0 //according to ESL profile, need to Parse this packet to be sure response slot.
        u8* pAdvData = pExtAdv->data;

        adv_ltv_t* pESL_ltv = blt_search_eslGroupID_fromAdvData(pAdvData, AdvDataLen, DT_ESL);

        esl_payload_t* pESL_payload = (esl_payload_t*)pESL_ltv->adv_data;
        if(pESL_payload->group_id != esl_devMgr.esl_dev_groupId){
            return ;
        }

        u8 sequence_no = blt_search_eslID_fromLTVdata(pESL_payload->TLV_data, (pESL_ltv->adv_len-1), esl_devMgr.esl_dev_eslID);
        if(sequence_no == NOT_FIND_ESL_ID_FLAG){
            return ; //error, not find same as device's esl id.
        }

        esl_devMgr.esl_rsp_slotIdx = sequence_no;
#else
//      esl_devMgr.esl_rsp_slotIdx = blt_pPdAsync->pdaRspDataCtrl.rsp_slot_idx;
#endif
        if (!blt_pPdAsync->pdaRspDataCtrl.user_setRspData_flag) {
            DBG_QIUWEI_CHN3_HIGH;
            DBG_QIUWEI_CHN3_LOW;
            blt_pPdAsync->pdaRspDataCtrl.aux_sync_subevt_ind_headerTick = bltRxPkt.rx_header_tick;
            blt_pPdAsync->pdaRspDataCtrl.req_subevt_idx                 = bltSche.pTask_cur->pawr_subevt_idx & 0x7F;
        }
        blt_pPdAsync->pdaRspDataCtrl.user_setRspData_flag = 1;

    } else { //pExtAdv->ext_hdr_len == 0
        my_dump_str_data(1, "ERROR, AUX_SYNC_SUBEVENT_IND extended header is zero!!!", 0, 0);
    }

    //PAWR_REPORT:

    paEvt_subevt    = (u16 *)&pExtAdv->data[PAWR_EVT_SUBEVT_STORE_OFFSET];
    paEvt_subevt[0] = blt_pPda->paEvtCnt;
    paEvt_subevt[1] = bltSche.pTask_cur->pawr_subevt_idx & 0x7F;

    return 0;
}


#if (ESL_CURRENT_OPTIMIZATION)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blt_ll_PAwRsync_adv_rpt(void)
{
    st_secchn_scn_t            *cur_pPdascan   = NULL;
    rf_pkt_ext_adv_t           *pPdaAdv        = NULL;
    le_periodAdvReportEvt_t_v2 *pawr_advRptEvt = NULL;

    volatile u16 *pPawrEvt_subevt = NULL;

    u8 temp_buff[100]; //process max length 255
    temp_buff[0] = HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT_V2;

    u8 aux_idx     = 0;
    u8 advdat_len  = 0;
    u8 scanRx_flag = 0;

    u8 *raw_pkt = (u8 *)(scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (scan_secRxFifo.rptr & SCAN_SECCHN_RXFIFO_MASK));
    pPdaAdv     = (rf_pkt_ext_adv_t *)(raw_pkt + DMA_RFRX_LEN_HW_INFO);

#if 0 //Extended Advertise Decryption process
    if(extAdvEncryEn){
        u8 status = blt_ll_extAdv_decryption(raw_pkt);
        if(status){//decryption fail
            //how to process
        }
    }
#endif

    advdat_len  = raw_pkt[1];                      // raw_pkt[1] is pure ADV data length(not include header and extended header), already calculated in IRQ
    aux_idx     = raw_pkt[2] & (~SECCHN_IDX_MARK); // index stored on raw_pkt[2]
    scanRx_flag = raw_pkt[3];

    pPawrEvt_subevt = (u16 *)&pPdaAdv->data[PAWR_EVT_SUBEVT_STORE_OFFSET];

    cur_pPdascan = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];


    if (scanRx_flag & SCANRX_FALG_PAWR) {
#if (SLEV_PAWR_SYNC_DEBUG5)
        log_event_irq(1, SLEV_PAWR_SYNC_DEBUG5);
#endif

        st_pda_sync_t *pPdA_sync = (st_pda_sync_t *)&pdAsync_tbl[cur_pPdascan->pdaSync_idx]; //bltPdaSync.pdA_sync_sel

        if (pPdA_sync->sync_establish != 0) { //to avoid pda adv event report before pda established event.
            return RTN_BREAK;
        }

        if (pPdA_sync->terminate) {
            return RTN_DROP;
        }

        if (!(pPdA_sync->sync_rcv_enable & REPORTING_EN)) { //if not allow to report
            return RTN_DROP;
        }

        /////////////////////////////data hold process///////////////////////////////////////
        u8 *copySourceAddr = NULL;
        copySourceAddr     = pPdaAdv->ext_hdr_len == 0 ? &pPdaAdv->ext_hdr_flg : pPdaAdv->data;

        pawr_advRptEvt = (le_periodAdvReportEvt_t_v2 *)&temp_buff[0];
        smemcpy(pawr_advRptEvt->data, copySourceAddr, advdat_len);
        //////////////////////////ending of hold data processing///////////////////

        pawr_advRptEvt->data_status = PDA_SYNC_REPORT_DATA_COMPLETE;
        /////////////////////////////////////
        pawr_advRptEvt->sync_handle = BLT_SYNC_HANDLE | cur_pPdascan->pdaSync_idx; //bltPdaSync.pdA_sync_sel;
        pawr_advRptEvt->tx_power    = 0x7F;
        pawr_advRptEvt->rssi        = raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110;
        pawr_advRptEvt->cte_type    = 0xff; //default 0xff in 5.2

        pawr_advRptEvt->periodic_evt_cnt = pPawrEvt_subevt[0];
        pawr_advRptEvt->subevent         = pPawrEvt_subevt[1] & 0x7F;

        pawr_advRptEvt->data_len = advdat_len;

        //////////////////////////////////////////////////////////////////////
        if (hci_le_eventMask & HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT) {
            if (BLE_SUCCESS != blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, advdat_len)) {
                //we can make sure not run here---if( blc_hci_isHciTxFIFOfull() ).
                //so not need to process truncate. if run here, code is error,need to debug.
                my_dump_str_data(DBG_PAwR_SYNC_LOGIC, "[pda scn]pda adv event send fail", 0, 0);
            }
        }

    } ///ending of (scanRx_flag & SCANRX_FLAG_PDA)


    return RTN_SUCCESS;
}

_attribute_ram_code_ void blc_ll_switch2PAwR_syncSubevt0(st_pda_sync_t *pPAwR_sync)
{
    pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt_num = 1;

    pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt[0] = 0; //default subevent 0
    pPAwR_sync->pSubevtOfBuild                   = &pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt[0];

    pPAwR_sync->pdaSyncSubevtCtrl.sync_subevt[0] |= PAWR_LAST_SUBEVENT_FLAG;
    pPAwR_sync->pdaSyncSubevtCtrl.set_syncSubevt_flag = 1;
    pPAwR_sync->preSubevtIdx                          = 0;


    for (int j = 0; j < PRDADV_SYNC_FIFONUM; j++) {
        pPAwR_sync->schTsk_fifo[j].scheTask_oft = TSKOFT_PAWRS_SUB + pPAwR_sync->pda_index;
        pPAwR_sync->schTsk_fifo[j].scheTask_flg = TSKFLG_PAWRS_SUB;
    }

    st_secchn_scn_t *pSecChn = (st_secchn_scn_t *)&secChnScn_tbl[pPAwR_sync->mapping_auxscan_idx];
    pSecChn->pdaSync_flag    = PAWR_PACKET_FLAG;

    u8 tmpOft                       = pSecChn->auxScnTsk.scheTask_oft;
    pSecChn->auxScnTsk.scheTask_oft = tmpOft << (TSKOFT_PAWRS_RSP - TSKOFT_SECCHN_SCAN);
    pSecChn->auxScnTsk.scheTask_flg = TSKFLG_PAWRS_RSP; //TSKOFT_PAWRS_RSP

    //according to the actual situation to process
    st_pda_t *pPda = (st_pda_t *)&pPAwR_sync->pda_rx;
    pPda->paEvtCnt--;
    pPAwR_sync->prePawrEvtCnt = blt_pPda->paEvtCnt;

    blt_sche_disableTask(TSKMSK_PDA_SYNC_0 << pPAwR_sync->pda_index);
    blt_sche_enableTask(TSKMSK_PAWRS_SUB_0 << pPAwR_sync->pda_index);

    blmsParam.state_chng |= STATE_CHANGE_PAWR_SYNC;
}

_attribute_ram_code_ int blt_pawr_sync_sub_interrupt_task(int flag, void *p0, void *p1)
{
    int index = flag & FLAG_SCHEDULE_TASK_IDX_MASK;

    if (flag & FLAG_SCHEDULE_START) {
        blt_ll_PAwRsync_subevtStart(index);
    } else if (flag & FLAG_SCHEDULE_DONE) {
        blt_ll_PAwRsync_subevtPost();
    } else if (flag & FLAG_PAWR_SYNC_RX_AUX_SYNC_SUBEVT_IND) {
        return blt_ll_PAwRsync_auxSyncSubevtInd_Proc(p0, p1);
    } else if (flag & FLAG_SCHEDULE_BUILD) {
        blt_ll_PAwRsync_build_task();
    } else if (flag & FLAG_PAWR_SYNC_RX_AUX_CONN_REQ) {
        return blt_ll_PAwRsync_auxConnInd_proc(p0);
    } else if (flag & FLAG_INSERT_SCHTSK_CONFLICT) {
        //sch_task_t *pTgtTsk = (sch_task_t *)p0;
        //u8 tgtTskFlg = pTgtTsk->scheTask_flg & TSKFLG_VALID_MASK;
        //later will add logical code.
    }

    return 0;
}

#if (ESL_CURRENT_OPTIMIZATION)
_attribute_ram_code_
#else
_attribute_no_inline_
#endif
int blt_pawr_sync_mainloop_task (int flag)
{
    if (flag == FLAG_PRDADV_DATA_REPORT) {
        return blt_ll_PAwRsync_adv_rpt();
    } else if (flag == FLAG_MODULE_RESET) {
    }

    return 0;
}


#endif
