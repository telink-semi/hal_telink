/********************************************************************************************************
 * @file    aoa_aod.c
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

#if (LL_FEATURE_ENABLE_LE_AOA_AOD)
//constant tone extension
//{Antenna_ID0, Antenna_ID1, Antenna_ID2, Antenna_ID3, Antenna_ID4, Antenna_ID5, Antenna_ID6};

_attribute_data_retention_    _attribute_aligned_(4) switch_pattern_t cte_connLess_switchPattern[TSKNUM_PERD_ADV];
_attribute_data_retention_    _attribute_aligned_(4) switch_pattern_t cte_conn_switchPattern[LL_MAX_ACL_CONN_NUM];
_attribute_data_retention_ u8 antenna_switch_seq[SWITCH_PATTERN_MAX_LEN] = {0, 2, 1, 3, 4, 6, 5, 7, 0, 1, 2, 3, 4, 5, 6, 7};

///support all
cte_antenna_info_t antenna_infor = {
    .support_switch_sample_rate = AOD_1US_TRANSMIT | AOD_1US_RECEIVE | AOA_1US_SWITCH_SAMPLE, //BIT(0)|BIT(1)|BIT(2), 2us is mandatory.
    .antenna_num                = ANTENNA_MAX_NUM,
    .max_switch_pattern_len     = SWITCH_PATTERN_MAX_LEN,
    .max_cte_len                = CTE_TIME_MAX,                                               //unit is 8us
};

ble_sts_t blc_hci_le_setConnectionless_CTETransmitParams(hci_le_setConnectionless_CTETransmitParam_t *connLessTxParams)
{
    u8 adv_handle = connLessTxParams->Advertising_Handle;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_setConnectionless_CTETransmitParams", connLessTxParams, connLessTxParams->CTE_length - 1 + sizeof(hci_le_setConnectionless_CTETransmitParam_t));

    if (cte_connLess_switchPattern[adv_handle].cte_transmit_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    ///cte length/cte type and cte count must be in some range.
    if (connLessTxParams->CTE_length < CTE_TIME_MIN || connLessTxParams->CTE_length > CTE_TIME_MAX) { ///spec between 16us and 160us. unit is 8us
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if (connLessTxParams->CTE_type > 0x02) { ///>0x02 is for future.
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if (connLessTxParams->CTE_count < 1 || connLessTxParams->CTE_count > CTE_COUNT_MAX_NUM) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if (connLessTxParams->Advertising_Handle > TSKNUM_PERD_ADV) { ///now support 4 adv set.
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    if (connLessTxParams->Switch_pattern_len > SWITCH_PATTERN_MAX_LEN) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    for (u8 i = 0; i < connLessTxParams->Switch_pattern_len; i++) {
        if (connLessTxParams->Antenna_IDs[i] > Antenna_ID6) { ///now our device only support 7 antennas.
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    ///switch pattern
    cte_connLess_switchPattern[adv_handle].cte_len   = connLessTxParams->CTE_length;
    cte_connLess_switchPattern[adv_handle].cte_type  = connLessTxParams->CTE_type; ///AOA;AOD_1US;AOD_2US. it is for transmitting.
    cte_connLess_switchPattern[adv_handle].cte_count = connLessTxParams->CTE_count;

    cte_connLess_switchPattern[adv_handle].cte_switch_pattern_len = connLessTxParams->Switch_pattern_len;
    smemcpy(cte_connLess_switchPattern[adv_handle].cte_swtich_pattern, connLessTxParams->Antenna_IDs, connLessTxParams->Switch_pattern_len);

    if (cte_connLess_switchPattern[adv_handle].cte_switch_pattern_len) {
        smemcpy(antenna_switch_seq, cte_connLess_switchPattern[adv_handle].cte_swtich_pattern, connLessTxParams->Switch_pattern_len);
        rf_aoa_aod_ant_pattern(SWITCH_SEQ_MODE0);
        rf_aoa_aod_ant_lut(antenna_switch_seq);
    }

    cte_connLess_switchPattern[adv_handle].sequence_ctrl |= (CTE_SET_PARAM_ADVHANDLE0_DONE_FLAG << adv_handle);

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &adv_handle, 1);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_setConnectionless_CTETransmit_Enable(hci_le_CTE_enable_type *connLessTxCtr)
{
    u8 adv_handle = connLessTxCtr->adv_handle; ///adv_handle: 0,1,2,3

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_setConnectionless_CTETransmit_Enable", connLessTxCtr, sizeof(hci_le_CTE_enable_type));

    if (adv_handle >= TSKNUM_PERD_ADV) {
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    if (!(cte_connLess_switchPattern[adv_handle].sequence_ctrl & PRD_ADV_SET_PARAM_DONE_FLAG) ||
        !(cte_connLess_switchPattern[adv_handle].sequence_ctrl & (CTE_SET_PARAM_ADVHANDLE0_DONE_FLAG << adv_handle))) {
        return HCI_ERR_CMD_DISALLOWED; //before setting period adv parameter
    }

    //if the specified period adv's phy is coded phy, need to reject.
    st_prd_adv_t *prd_adv = blt_ll_search_existing_perdAdv_index_by_advHandle(adv_handle);
    if (prd_adv == NULL) {
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }
    if (prd_adv && prd_adv->pda_tx.pda_phy == BLE_PHY_CODED) {
        return HCI_ERR_CMD_DISALLOWED;
    }


    if (connLessTxCtr->CTE_enable != cte_connLess_switchPattern[adv_handle].cte_transmit_en) {
        cte_connLess_switchPattern[adv_handle].cte_trsmitRev_flag = CTE_TRANSMIT; //CTE_TRANSMIT or CTE_RECEIVE
        cte_connLess_switchPattern[adv_handle].cte_transmit_en    = connLessTxCtr->CTE_enable;

        blt_prdadv_updatePram(prd_adv);
    }

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &adv_handle, 1);

    return BLE_SUCCESS;
}

//LE Set Connectionless IQ Sampling Enable command
ble_sts_t blc_hci_le_setConnectionless_IQsample_Enable(hci_le_setConnectionless_IQsampleEn_t *IQsampleEn)
{
    u16 sync_index = IQsampleEn->Sync_Handle & BLT_SYNC_IDX_MARK;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_setConnectionless_IQsample_Enable", IQsampleEn, IQsampleEn->Switching_pattern_len - 1 + sizeof(hci_le_setConnectionless_IQsampleEn_t));

    if (sync_index > TSKNUM_PERD_ADV) {
        return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    }

    if (IQsampleEn->Sampling_Enable == 0) { //sampling disable, ignore other parameter.
        cte_connLess_switchPattern[sync_index].cte_sample_en      = 0;
        cte_connLess_switchPattern[sync_index].cte_trsmitRev_flag = CTE_NOT_EXIST;

        my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &IQsampleEn->Sync_Handle, 2);

        return BLE_SUCCESS;
    }

    //  st_prd_adv_t* prd_adv = blt_ll_search_existing_perdAdv_index_by_advHandle(adv_handle);
    //  if(!prd_adv){
    //      return HCI_ERR_UNKNOWN_ADV_IDENTIFIER;
    //  }

    if (IQsampleEn->Slot_Duration != SWITCH_SAMPLE_SLOT_1US && IQsampleEn->Slot_Duration != SWITCH_SAMPLE_SLOT_2US) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    //  if( (IQsampleEn->Slot_Duration == 1) && (antenna_infor.support_switch_sample_rate & 0x06 == 0)){
    //      return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    //  }

    if (IQsampleEn->Max_Sampled_CTEs > SAMPLED_CTES_MAX_NUM) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if (IQsampleEn->Switching_pattern_len > SWITCH_PATTERN_MAX_LEN) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    for (u8 i = 0; i < IQsampleEn->Switching_pattern_len; i++) {
        if (IQsampleEn->Antenna_IDs[i] > Antenna_ID6) { ///now our device only support 7 antennas.
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    cte_connLess_switchPattern[sync_index].cte_slot_duration      = IQsampleEn->Slot_Duration;
    cte_connLess_switchPattern[sync_index].cte_switch_pattern_len = IQsampleEn->Switching_pattern_len;
    smemcpy(cte_connLess_switchPattern[sync_index].cte_swtich_pattern, IQsampleEn->Antenna_IDs, IQsampleEn->Switching_pattern_len);

    cte_connLess_switchPattern[sync_index].Max_Sampled_CTEs   = IQsampleEn->Max_Sampled_CTEs; //the number need to sample.
    cte_connLess_switchPattern[sync_index].cte_trsmitRev_flag = CTE_RECEIVE;
    cte_connLess_switchPattern[sync_index].cte_sample_en      = IQsampleEn->Sampling_Enable;

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &IQsampleEn->Sync_Handle, 2);

    return BLE_SUCCESS;
}

//LE Set Connection CTE Receive Parameters command
ble_sts_t blc_hci_le_setConnection_CTEReceiveParams(hci_le_setConnection_CTERevParams_t *cteRevParam)
{
    u8 conn_idx = cteRevParam->conn_handle & CONN_IDX_MASK;
    //  if(cteRevParam->conn_handle > ){ //whether or not need to judge conn_handle is connection state.
    //      return ;
    //  }

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_setConnection_CTEReceiveParams", cteRevParam, cteRevParam->switch_pattern_len - 1 + sizeof(hci_le_setConnection_CTERevParams_t));

    if (cteRevParam->sampling_en == 0) {
        cte_conn_switchPattern[conn_idx].cte_sample_en      = 0;
        cte_conn_switchPattern[conn_idx].cte_trsmitRev_flag = CTE_NOT_EXIST;

        my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &cteRevParam->conn_handle, 2);

        return BLE_SUCCESS;
    }

    if (cteRevParam->slot_duration != SWITCH_SAMPLE_SLOT_1US && cteRevParam->slot_duration != SWITCH_SAMPLE_SLOT_2US) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    //  if( (cteRevParam->slot_duration == 1) && (antenna_infor.support_switch_sample_rate & 0x06 == 0)){
    //      return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    //  }

    if (cteRevParam->switch_pattern_len > SWITCH_PATTERN_MAX_LEN) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }


    for (u8 i = 0; i < cteRevParam->switch_pattern_len; i++) {
        if (cteRevParam->antenna_ids[i] >= ANTENNA_MAX_NUM) { ///now our device only support 7 antennas.
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    cte_conn_switchPattern[conn_idx].cte_switch_pattern_len = cteRevParam->switch_pattern_len;
    smemcpy(cte_conn_switchPattern[conn_idx].cte_swtich_pattern, cteRevParam->antenna_ids, cteRevParam->switch_pattern_len);

    cte_conn_switchPattern[conn_idx].cte_slot_duration  = cteRevParam->slot_duration; ///it is for receiving.
    cte_conn_switchPattern[conn_idx].cte_trsmitRev_flag = CTE_RECEIVE;
    cte_conn_switchPattern[conn_idx].cte_sample_en      = cteRevParam->sampling_en;

    cte_conn_switchPattern[conn_idx].sequence_ctrl |= (CTE_SET_RECEIVE_PARAM_CONNHANDLE0 << conn_idx);

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &cteRevParam->conn_handle, 2);

    return BLE_SUCCESS;
}

//7.8.84 LE Set Connection CTE Transmit Parameters command
ble_sts_t blc_hci_le_setConnection_CTETransmitParams(hci_le_setConnection_CTETransmitParams_t *cteTransmitParams)
{
    u8 conn_idx = cteTransmitParams->conn_handle & 0x0f;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_setConnection_CTETransmitParams", cteTransmitParams, cteTransmitParams->switching_pattern_len - 1 + sizeof(hci_le_setConnection_CTETransmitParams_t));

    if (cte_conn_switchPattern[conn_idx].cte_rsp_en) { ///if rsp_enable has been set, return error 0x0c.
        return HCI_ERR_CMD_DISALLOWED;
    }

    if (!(cteTransmitParams->CTE_type & 0x07)) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    if (cteTransmitParams->switching_pattern_len > SWITCH_PATTERN_MAX_LEN) {
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
    }

    for (u8 i = 0; i < cteTransmitParams->switching_pattern_len; i++) {
        if (cteTransmitParams->antenna_IDs[i] > Antenna_ID6) { ///now our device only support 7 antennas.
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
    }

    cte_conn_switchPattern[conn_idx].cte_switch_pattern_len = cteTransmitParams->switching_pattern_len;
    smemcpy(cte_conn_switchPattern[conn_idx].cte_swtich_pattern, cteTransmitParams->antenna_IDs, cteTransmitParams->switching_pattern_len);

    cte_conn_switchPattern[conn_idx].cte_type           = cteTransmitParams->CTE_type;
    cte_conn_switchPattern[conn_idx].cte_trsmitRev_flag = CTE_TRANSMIT;

    cte_conn_switchPattern[conn_idx].sequence_ctrl |= (CTE_SET_TRANSMIT_PARAM_CONNHANDLE0 << conn_idx);

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &cteTransmitParams->conn_handle, 2);

    return BLE_SUCCESS;
}

ble_sts_t blc_hci_le_connection_CTEReq_Enable(hci_le_cteReqEn_t *connCTEReqEn)
{
    u8 conn_idx = connCTEReqEn->conn_handle & 0x0f;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_connection_CTEReq_Enable", connCTEReqEn, sizeof(hci_le_cteReqEn_t));

    st_ll_conn_t *pc = (st_ll_conn_t *)&blms[conn_idx];

    ///check whether peer device support CTE_RSP feature. ?if not exchange feature?
    if (!(pc->ll_remoteFeature0 & LL_FEATURE_MASK_CONNECTION_CTE_REQUEST)) {
        return HCI_ERR_UNSUPPORTED_REMOTE_FEATURE;
    }

    if (connCTEReqEn->cte_req_en == 0) {
        cte_conn_switchPattern[conn_idx].cte_req_en = 0;

        my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &connCTEReqEn->conn_handle, 2);

        return BLE_SUCCESS;
    }

    ///here connCTEReqEn->cte_req_en == 1
    if (connCTEReqEn->cte_req_en == cte_conn_switchPattern[conn_idx].cte_req_en) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    if (!(cte_conn_switchPattern[conn_idx].sequence_ctrl & (CTE_SET_RECEIVE_PARAM_CONNHANDLE0 << conn_idx))) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    if (pc->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    #endif

    //If the Host sets CTE_Request_Interval to a non-zero value less than or equal to connSlaveLatency
    //confusing??????


    cte_conn_switchPattern[conn_idx].cte_req_intvl = connCTEReqEn->cte_req_intvl;
    cte_conn_switchPattern[conn_idx].cte_len       = connCTEReqEn->req_cte_len;
    cte_conn_switchPattern[conn_idx].cte_type      = connCTEReqEn->req_cte_type;
    cte_conn_switchPattern[conn_idx].cte_req_en    = connCTEReqEn->cte_req_en;

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &connCTEReqEn->conn_handle, 2);

    return BLE_SUCCESS;
}

//7.8.86 LE Connection CTE Response Enable command
ble_sts_t blc_hci_le_connection_CTERsp_Enable(hci_le_cteRspEn_t *connCTERspEn)
{
    u8 conn_idx = connCTERspEn->conn_handle & 0x0f;

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_connection_CTERsp_Enable", connCTERspEn, sizeof(hci_le_cteRspEn_t));

    st_ll_conn_t *pc = (st_ll_conn_t *)&blms[conn_idx];

    if (!(cte_conn_switchPattern[conn_idx].sequence_ctrl & (CTE_SET_TRANSMIT_PARAM_CONNHANDLE0 << conn_idx))) {
        return HCI_ERR_CMD_DISALLOWED;
    }

    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    if (pc->connPhyCtrl.conn_cur_phy == BLE_PHY_CODED) {
        return HCI_ERR_CMD_DISALLOWED;
    }
    #endif

    cte_conn_switchPattern[conn_idx].cte_rsp_en = connCTERspEn->rsp_enable;

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &connCTERspEn->conn_handle, 2);

    return BLE_SUCCESS;
}

bool blc_le_setAntennaInfor(cte_antenna_info_t *antennaInfor)
{
    antenna_infor.support_switch_sample_rate = antennaInfor->support_switch_sample_rate;
    antenna_infor.antenna_num                = antennaInfor->antenna_num;
    antenna_infor.max_switch_pattern_len     = antennaInfor->max_switch_pattern_len;
    antenna_infor.max_cte_len                = antennaInfor->max_cte_len;

    return TRUE;
}

ble_sts_t blc_hci_le_ReadAntennaInfor(u8 *inforBuff)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] le_ReadAntennaInfor", 0, 0);

    u8 *ant_infor = inforBuff;
    ant_infor[0]  = antenna_infor.support_switch_sample_rate;
    ant_infor[1]  = antenna_infor.antenna_num;
    ant_infor[2]  = antenna_infor.max_switch_pattern_len;
    ant_infor[3]  = antenna_infor.max_cte_len;

    my_dump_str_data(IUT_HCI_LOG_EN, "@return SUCCESS", &ant_infor, 4);

    return BLE_SUCCESS;
}

void blc_le_resetCTEInfor(void)
{
    u8 idx = 0;
    ///adv state
    for (idx = 0; idx < TSKNUM_PERD_ADV; idx++) {
        cte_connLess_switchPattern[idx].cte_sample_en   = 0;
        cte_connLess_switchPattern[idx].cte_transmit_en = 0;
    }

    ///connection state
    for (idx = 0; idx < LL_MAX_ACL_CONN_NUM; idx++) {
        cte_conn_switchPattern[idx].cte_sample_en = 0;
        cte_conn_switchPattern[idx].cte_rsp_en    = 0;

        cte_conn_switchPattern[idx].cte_req_en = 0;
    }
}

///wait driver to complete. not sure whether need to set hw register.
unsigned char blc_le_setHardWareForCTE(unsigned char cteTrRxFlag, u8 handle)
{
    u8            idx = handle & 0x0f;
    st_ll_conn_t *pc  = (st_ll_conn_t *)&blms[idx];

    if (cteTrRxFlag == CTE_TRANSMIT) {
        //config relevant hardware register for transmitting.
        if (!pc->connState) { //disconnect
            ///set relevant hw register
        } else { //connect
        }
    } else if (cteTrRxFlag == CTE_RECEIVE) {
        //config relevant hardware register for receiving.
        if (!pc->connState) { //disconnect
            ///
        } else { //connect
        }
    }

    return BLE_SUCCESS;
}

int blt_ll_aoa_aod_mainloop(void)
{
    return BLE_SUCCESS;
}

int blt_ll_aoa_aod_connectionless_data_process(void)
{
    st_secchn_scn_t *cur_pPdascan = NULL;

    u8 aux_idx = 0;

    u8 *raw_pkt  = (u8 *)(scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (scan_secRxFifo.rptr & SCAN_SECCHN_RXFIFO_MASK));
    aux_idx      = raw_pkt[2] & (~SECCHN_IDX_MARK); // index stored on raw_pkt[2]
    cur_pPdascan = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];

    u16            sync_index = cur_pPdascan->pdaSync_idx;
    st_pda_sync_t *pPdA_sync  = (st_pda_sync_t *)&pdAsync_tbl[sync_index]; //bltPdaSync.pdA_sync_sel

    if (pPdA_sync->sync_cte_type == 0xFF) {
        return -1;
    }

    if (!pPdA_sync->sync_cte_IQ_valid) {
        return -1;
    }

    u8 scanRx_flag = raw_pkt[3];

    //for reduce IQ Report number, only sample AUX_SYNC_IND
    if (!(scanRx_flag & SCANRX_FLAG_FIRST_DATA)) {
        return -1;
    }

    if (scanRx_flag & SCANRX_FLAG_DATA_DROP) {
        return -1;
    }

    if ((pPdA_sync->sync_cte_type == AOD_TYPE_1US) || ((pPdA_sync->sync_cte_type == AOA_TYPE) && (cte_connLess_switchPattern[sync_index].cte_slot_duration == SWITCH_SAMPLE_SLOT_1US))) {
        cte_connLess_switchPattern[sync_index].cte_slot_real = SWITCH_SAMPLE_SLOT_1US;
    } else {
        cte_connLess_switchPattern[sync_index].cte_slot_real = SWITCH_SAMPLE_SLOT_2US;
    }

    //Total samples = 8 + ((CTE_Time * 8) - 12)/(2[1us slot] or 4[2us slot])
    u8 total_samples = (pPdA_sync->sync_cte_time << 3) - 12;
    if (cte_connLess_switchPattern[sync_index].cte_slot_real == SWITCH_SAMPLE_SLOT_1US) { //1us slot
        cte_connLess_switchPattern[sync_index].cte_total_samples = (total_samples >> 1) + 8;
    } else {                                                                              //2us slot
        cte_connLess_switchPattern[sync_index].cte_total_samples = (total_samples >> 2) + 8;
    }

    if ((cte_connLess_switchPattern[sync_index].cte_total_samples > 82) || (cte_connLess_switchPattern[sync_index].cte_total_samples < 9)) {
        return -1;
    }

    //extraction IQ data, 18 Bytes ~ 164 Bytes, IQ data before CRC, rf_len not included
    smemcpy(cte_connLess_switchPattern[sync_index].cte_iq_data, raw_pkt + DMA_RFRX_OFFSET_DATA + raw_pkt[DMA_RFRX_OFFSET_RFLEN], cte_connLess_switchPattern[sync_index].cte_total_samples << 1);

    //shift hardware attachment information, 11 Bytes
    smemcpy(raw_pkt + DMA_RFRX_OFFSET_DATA + raw_pkt[DMA_RFRX_OFFSET_RFLEN], raw_pkt + DMA_RFRX_OFFSET_DATA + raw_pkt[DMA_RFRX_OFFSET_RFLEN] + (cte_connLess_switchPattern[sync_index].cte_total_samples << 1), 11);

    return BLE_SUCCESS;
}

_attribute_no_inline_ int blt_ll_aoa_aod_connectionless_IQ_report(void)
{
    st_secchn_scn_t *cur_pPdascan = NULL;

    u8 aux_idx       = 0;
    u8 extadv_report = 1;                           //default need to report event

    u8 *raw_pkt  = (u8 *)(scan_secRxFifo.p + SCAN_SECCHN_RXFIFO_SIZE * (scan_secRxFifo.rptr & SCAN_SECCHN_RXFIFO_MASK));
    aux_idx      = raw_pkt[2] & (~SECCHN_IDX_MARK); // index stored on raw_pkt[2]
    cur_pPdascan = (st_secchn_scn_t *)&secChnScn_tbl[aux_idx];

    u16            sync_index = cur_pPdascan->pdaSync_idx;
    st_pda_sync_t *pPdA_sync  = (st_pda_sync_t *)&pdAsync_tbl[sync_index]; //bltPdaSync.pdA_sync_sel

    if (!cte_connLess_switchPattern[sync_index].cte_sample_en) {
        return -1;
    }

    if (pPdA_sync->sync_cte_type == 0xFF) {
        return -1;
    }

    if (!pPdA_sync->sync_cte_IQ_valid) {
        return -1;
    }

    u8 scanRx_flag = raw_pkt[3];

    //for reduce IQ Report number, only sample AUX_SYNC_IND
    if (!(scanRx_flag & SCANRX_FLAG_FIRST_DATA)) {
        return -1;
    }

    if (scanRx_flag & SCANRX_FLAG_DATA_DROP) {
        extadv_report = 0;
    }

    if (extadv_report) {                                        //scanRx_flag & SCANRX_FLAG_PDA
        if (pPdA_sync->sync_rcv_enable) {
            u8                                  temp_buff[178]; // max length 177
            hci_le_connectionlessIQReportEvt_t *leConnectionlessIQReportEvt = NULL;

            leConnectionlessIQReportEvt                = (hci_le_connectionlessIQReportEvt_t *)temp_buff;
            leConnectionlessIQReportEvt->sub_code      = HCI_SUB_EVT_LE_CONNECTIONLESS_IQ_REPORT;
            leConnectionlessIQReportEvt->sync_handle   = BLT_SYNC_HANDLE | sync_index;
            leConnectionlessIQReportEvt->channel_index = pPdA_sync->sync_cte_chnIdx;

            leConnectionlessIQReportEvt->rssi_antenna_id = 0;
            leConnectionlessIQReportEvt->rssi            = (raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110) * 10; //Units: 0.1 dBm;
            leConnectionlessIQReportEvt->cte_type        = pPdA_sync->sync_cte_type;

            leConnectionlessIQReportEvt->slot_durations         = cte_connLess_switchPattern[sync_index].cte_slot_real;
            leConnectionlessIQReportEvt->packet_status          = 0; // 0x00 CRC was correct
            leConnectionlessIQReportEvt->periodic_event_counter = pPdA_sync->sync_cte_EvtCnt;


            if ((cte_connLess_switchPattern[sync_index].cte_total_samples > 82) || (cte_connLess_switchPattern[sync_index].cte_total_samples < 9)) {
                return -1;
            }
            leConnectionlessIQReportEvt->sample_count = cte_connLess_switchPattern[sync_index].cte_total_samples;

            foreach (i, cte_connLess_switchPattern[sync_index].cte_total_samples) {
                leConnectionlessIQReportEvt->IQ_sample[i].I_sample = cte_connLess_switchPattern[sync_index].cte_iq_data[i * 2];
                leConnectionlessIQReportEvt->IQ_sample[i].Q_sample = cte_connLess_switchPattern[sync_index].cte_iq_data[i * 2 + 1];

                //0x80(-128), No valid sample available
                if (leConnectionlessIQReportEvt->IQ_sample[i].I_sample == 0x80) {
                    leConnectionlessIQReportEvt->IQ_sample[i].I_sample = 0x81;
                }
                if (leConnectionlessIQReportEvt->IQ_sample[i].Q_sample == 0x80) {
                    leConnectionlessIQReportEvt->IQ_sample[i].Q_sample = 0x81;
                }
            }

            u8 total_rptevt_len = 13;
            total_rptevt_len += cte_connLess_switchPattern[sync_index].cte_total_samples << 1;

            if (hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTIONLESS_IQ_REPORT) {
                if (BLE_SUCCESS != blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, total_rptevt_len)) {
                    my_dump_str_data(DBG_AOA_AOD_LOGIC, "ConnlessIQReport send fail", 0, 0);
                } else {
                    DBG_CHN7_TOGGLE;
                    my_dump_str_data(DBG_AOA_AOD_LOGIC, "ConnlessIQReport", &temp_buff, total_rptevt_len);
                }
            }
        }
    }

    return BLE_SUCCESS;
}

_attribute_no_inline_ int blt_ll_aoa_aod_connection_IQ_report(void)
{
    u8           *raw_pkt  = (u8 *)(blt_rxfifo.p_base + (blt_rxfifo.rptr & blt_rxfifo.mask) * blt_rxfifo.size);
    u8            conn_idx = raw_pkt[2] & CONN_IDX_MASK;
    st_ll_conn_t *pc       = (st_ll_conn_t *)&blms[conn_idx];

    if (!cte_conn_switchPattern[conn_idx].cte_sample_en) {
        return -1;
    }

    u8                              temp_buff[178]; // max length 178
    hci_le_connectionIQReportEvt_t *leConnectionIQReportEvt = NULL;

    leConnectionIQReportEvt           = (hci_le_connectionIQReportEvt_t *)temp_buff;
    leConnectionIQReportEvt->sub_code = HCI_SUB_EVT_LE_CONNECTION_IQ_REPORT;
    //////////////////////
    leConnectionIQReportEvt->conn_handle = raw_pkt[2];
    #if (LL_FEATURE_ENABLE_LE_2M_PHY | LL_FEATURE_ENABLE_LE_CODED_PHY)
    leConnectionIQReportEvt->rx_phy = pc->connPhyCtrl.conn_cur_phy;
    #else
    leConnectionIQReportEvt->rx_phy = BLE_PHY_1M;
    #endif
    leConnectionIQReportEvt->data_channel_index = pc->conn_chn;
    leConnectionIQReportEvt->rssi               = (raw_pkt[DMA_RFRX_OFFSET_RSSI(raw_pkt)] - 110) * 10; //Units: 0.1 dBm;
    leConnectionIQReportEvt->rssi_antenna_id    = 0;
    leConnectionIQReportEvt->cte_type           = pc->conn_cte_type;

    leConnectionIQReportEvt->slot_durations     = cte_conn_switchPattern[conn_idx].cte_slot_real;
    leConnectionIQReportEvt->packet_status      = 0; // 0x00 CRC was correct
    leConnectionIQReportEvt->conn_event_counter = pc->conn_inst_mark;

    if ((cte_conn_switchPattern[conn_idx].cte_total_samples > 82) || (cte_conn_switchPattern[conn_idx].cte_total_samples < 9)) {
        return -1;
    }
    leConnectionIQReportEvt->sample_count = cte_conn_switchPattern[conn_idx].cte_total_samples;

    foreach (i, cte_conn_switchPattern[conn_idx].cte_total_samples) {
        leConnectionIQReportEvt->IQ_sample[i].I_sample = cte_conn_switchPattern[conn_idx].cte_iq_data[i * 2];
        leConnectionIQReportEvt->IQ_sample[i].Q_sample = cte_conn_switchPattern[conn_idx].cte_iq_data[i * 2 + 1];

        //0x80, No valid sample available
        if (leConnectionIQReportEvt->IQ_sample[i].I_sample == 0x80) {
            leConnectionIQReportEvt->IQ_sample[i].I_sample = 0x81;
        }
        if (leConnectionIQReportEvt->IQ_sample[i].Q_sample == 0x80) {
            leConnectionIQReportEvt->IQ_sample[i].Q_sample = 0x81;
        }
    }

    u8 total_rptevt_len = 14;
    total_rptevt_len += cte_conn_switchPattern[conn_idx].cte_total_samples << 1;

    if (hci_le_eventMask & HCI_LE_EVT_MASK_CONNECTION_IQ_REPORT) {
        if (BLE_SUCCESS != blc_hci_send_event(HCI_FLAG_EVENT_BT_STD | HCI_EVT_LE_META, temp_buff, total_rptevt_len)) {
            my_dump_str_data(DBG_AOA_AOD_LOGIC, "ConnIQReport send fail", 0, 0);
        } else {
            DBG_CHN7_TOGGLE;
            my_dump_str_data(DBG_AOA_AOD_LOGIC, "ConnIQReport", &temp_buff, total_rptevt_len);
        }
    }

    return 0;
}

int blt_ll_aoa_aod_acl_mainloop(void)
{
    u8 *raw_pkt = (u8 *)(blt_rxfifo.p_base + (blt_rxfifo.rptr & blt_rxfifo.mask) * blt_rxfifo.size);

    // TODO: encryption has not been considered
    if (raw_pkt[DMA_RFRX_OFFSET_RFLEN]) { //rf_len != 0
        rf_cte_data_header_t *pcte = (rf_cte_data_header_t *)(raw_pkt + DMA_RFRX_OFFSET_HEADER);

        if (pcte->cp) {
            u8 cte_info_print = pcte->cte_info;
            my_dump_str_u32s(DBG_AOA_AOD_LOGIC, "Conn_CTE:head,rf_len,cte_info,DMA_len", raw_pkt[4], pcte->rf_len, cte_info_print, raw_pkt[0]);

            u8 cte_info = pcte->cte_info;
            u8 cte_time = cte_info & 0x1F;
            u8 cte_type = (cte_info & 0xC0) >> 6;

            //remove CTEInfo Byte
            smemcpy(raw_pkt + DMA_RFRX_OFFSET_DATA, raw_pkt + DMA_RFRX_OFFSET_DATA + 1, pcte->rf_len);

            if ((cte_time >= CTE_TIME_MIN) && (cte_time <= CTE_TIME_MAX) && (cte_type <= AOD_TYPE_2US)) {
                u8            conn_idx = raw_pkt[2] & CONN_IDX_MASK;
                st_ll_conn_t *pc       = (st_ll_conn_t *)&blms[conn_idx];

                pc->conn_cte_type = cte_type;
                pc->conn_cte_time = cte_time;

                if ((pc->conn_cte_type == AOD_TYPE_1US) || ((pc->conn_cte_type == AOA_TYPE) && (cte_conn_switchPattern[conn_idx].cte_slot_duration == SWITCH_SAMPLE_SLOT_1US))) {
                    cte_conn_switchPattern[conn_idx].cte_slot_real = SWITCH_SAMPLE_SLOT_1US;
                } else {
                    cte_conn_switchPattern[conn_idx].cte_slot_real = SWITCH_SAMPLE_SLOT_2US;
                }

                //Total samples = 8 + ((CTE_Time * 8) - 12)/(2[1us slot] or 4[2us slot])
                u8 total_samples = (pc->conn_cte_time << 3) - 12;
                if (cte_conn_switchPattern[conn_idx].cte_slot_real == SWITCH_SAMPLE_SLOT_1US) { //1us slot
                    cte_conn_switchPattern[conn_idx].cte_total_samples = (total_samples >> 1) + 8;
                } else {                                                                        //2us slot
                    cte_conn_switchPattern[conn_idx].cte_total_samples = (total_samples >> 2) + 8;
                }

                if ((cte_conn_switchPattern[conn_idx].cte_total_samples > 82) || (cte_conn_switchPattern[conn_idx].cte_total_samples < 9)) {
                    return -1;
                }

                //extraction IQ data, 18 Bytes ~ 164 Bytes, IQ data before CRC, rf_len not included
                smemcpy(cte_conn_switchPattern[conn_idx].cte_iq_data, raw_pkt + DMA_RFRX_OFFSET_DATA + raw_pkt[DMA_RFRX_OFFSET_RFLEN] + 1, cte_conn_switchPattern[conn_idx].cte_total_samples << 1); // attention to CTEInfo 1 Byte

                //shift hardware attachment information, 11 Bytes
                smemcpy(raw_pkt + DMA_RFRX_OFFSET_DATA + raw_pkt[DMA_RFRX_OFFSET_RFLEN], raw_pkt + DMA_RFRX_OFFSET_DATA + raw_pkt[DMA_RFRX_OFFSET_RFLEN] + 1 + (cte_conn_switchPattern[conn_idx].cte_total_samples << 1), 11); // attention to CTEInfo 1 Byte

                blt_ll_aoa_aod_connection_IQ_report();
            }
        }
    }

    return BLE_SUCCESS;
}

_attribute_noinline_ int blt_aoa_aod_mainloop_task(int flag)
{
    if (flag == FLAG_MODULE_MAINLOOP) {
        blt_ll_aoa_aod_mainloop();
    } else if (flag == FLAG_AOA_AOD_CONNECTIONLESS_DATA_PROCESS) {
        blt_ll_aoa_aod_connectionless_data_process();
    } else if (flag == FLAG_AOA_AOD_CONNECTIONLESS_IQ_REPORT) {
        blt_ll_aoa_aod_connectionless_IQ_report();
    } else if (flag == FLAG_AOA_AOD_CONNECTION_MAINLOOP) {
        blt_ll_aoa_aod_acl_mainloop();
    } else if (flag == FLAG_MODULE_RESET) {
        blc_le_resetCTEInfor();
    }
    return 0;
}

void blc_ll_initAoaAod_module(void)
{
    #if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cte_connLess_switchPattern)), aoa_aod);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(cte_conn_switchPattern)), aoa_aod);
    #endif

    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_TRANSMITTER << 19);
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_CONNECTIONLESS_CTE_RECEIVER << 20);
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD << 21);
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_ANTENNA_SWITCHING_CTE_RECEPTION_AOA << 22);
    LL_FEATURE_MASK_0 |= (LL_FEATURE_ENABLE_RECEIVING_CONSTANT_TONE_EXTENSIONS << 23);

    ll_aoa_aod_mlp_task_cb = blt_aoa_aod_mainloop_task;

    blmsParam.cte_connLess_en = 1;

    #if (MCU_CORE_TYPE == MCU_CORE_B91)
    rf_aoa_aod_ant_pattern(SWITCH_SEQ_MODE0);
    rf_aoa_aod_ant_lut(antenna_switch_seq);
    write_reg8(0x140834, 0x07);
    triangle_all_open();
    set_antenna_num(ANTENNA_MAX_NUM);
    rf_aoa_aod_iq_data_mode(IQ_8_BIT_MODE);
    #else //kite/vulture
        #error "to be done"
    #endif

    blc_le_resetCTEInfor();

    my_dump_str_data(DBG_AOA_AOD_LOGIC, "initAoaAod", 0, 0);
}


#endif
