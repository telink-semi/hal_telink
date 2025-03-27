/********************************************************************************************************
 * @file    acl_conn_2.c
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



_attribute_ble_data_retention_  u8 ll_acl_recentAvgRSSI[LL_MAX_ACL_CONN_NUM];
_attribute_ble_data_retention_  u8 ll_acl_recentRSSI[LL_MAX_ACL_CONN_NUM];




ble_sts_t blc_ll_setMaxConnectionNumber(int max_master_num, int max_slave_num)
{
    if(blmsParam.max_master_num > LL_MAX_ACL_CEN_NUM){
        return LL_ERR_INVALID_PARAMETER;
    }
    else{
        blmsParam.max_master_num = max_master_num;
    }

    if(blmsParam.max_slave_num > LL_MAX_ACL_PER_NUM){
        return LL_ERR_INVALID_PARAMETER;
    }
    else{
        blmsParam.max_slave_num = max_slave_num;
    }

    #if (LL_ACL_CEN_EN)
        /* update master bSlot number when master_connInter or max_master_num changes */
        if(aclMas_param.master_connInter && blmsParam.max_master_num){
            blt_ll_calculateAclMasterBslotNumber();
        }
    #endif

    return BLE_SUCCESS;
}


/**
 * @brief      for user to initialize LinkLayer ACL connection RX FIFO.
 *             all connection will share the FIFO.
 * @param[in]  pRxbuf - RX FIFO buffer address.
 * @param[in]  fifo_size - RX FIFO size
 * @param[in]  fifo_number - RX FIFO number, can only be 4, 8, 16 or 32
 * @return     status, 0x00:  succeed
 *                     other: failed
 */
ble_sts_t blc_ll_initAclConnRxFifo(u8 *pRxbuf, int fifo_size, int fifo_number)
{
    bltempParam.ll_aclRxFifo_set = 1;

    /* number must be 2^n */
    if( IS_POWER_OF_2(fifo_number) && fifo_number > 3){
        blt_rxfifo.num = fifo_number;
        blt_rxfifo.mask = fifo_number - 1;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    /* size must be 16*n */
    if( (fifo_size & 15) == 0){
        blt_rxfifo.size = fifo_size;
        blt_rxfifo.size_div_16 = fifo_size>>4;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    blt_rxfifo.wptr = blt_rxfifo.rptr = 0;
    blt_rxfifo.p_base = pRxbuf;

    aclConn_param.acl_rx_dma_buff = (u32)(blt_rxfifo.p_base + (blt_rxfifo.wptr & blt_rxfifo.mask) * blt_rxfifo.size);
    aclConn_param.acl_rx_dma_size = blt_rxfifo.size_div_16;

    return BLE_SUCCESS;
}


ble_sts_t   blc_ll_setAclConnMaxOctetsNumber(u8 maxRxOct, u8 maxTxOct_master, u8 maxTxOct_slave)
{
    if(maxRxOct < MAX_OCTETS_DATA_LEN_27 || maxRxOct > MAX_OCTETS_DATA_LEN_EXTENSION){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if(maxTxOct_master < MAX_OCTETS_DATA_LEN_27 || maxTxOct_master > MAX_OCTETS_DATA_LEN_EXTENSION){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    if(maxTxOct_slave < MAX_OCTETS_DATA_LEN_27 || maxTxOct_slave > MAX_OCTETS_DATA_LEN_EXTENSION){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    aclConn_param.maxRxOct = maxRxOct;
    aclConn_param.maxTxOct_master = maxTxOct_master;
    aclConn_param.maxTxOct_slave = maxTxOct_slave;

    aclConn_param.prefMaxTxLen = 27;
    aclConn_param.prefMaxTxTime = LL_PDU_TIME_1M(27);

    //[!!!] HCI ACL buffer size >= 4 + DLE(blmsParam.acl_packet_length)
    blmsParam.acl_packet_length = max(aclConn_param.maxTxOct_master, aclConn_param.maxTxOct_slave);

    return BLE_SUCCESS;
}





int blt_ll_checkAclInit(void)
{
    if(!aclConn_param.maxRxOct){
        aclConn_param.maxRxOct = MAX_OCTETS_DATA_LEN_27;
    }

    if(!aclConn_param.maxTxOct_master){
        aclConn_param.maxTxOct_master = MAX_OCTETS_DATA_LEN_27;
    }

    if(!aclConn_param.maxTxOct_slave){
        aclConn_param.maxTxOct_slave = MAX_OCTETS_DATA_LEN_27;
    }

    //in our design, RX ACL DATA buffer equal to MAX_TX_OCTETS
    if(blmsParam.acl_master_en && blmsParam.acl_slave_en){  //master and slave
        aclConn_param.maxTxOct = max(aclConn_param.maxTxOct_master, aclConn_param.maxTxOct_slave);
    }
    else if(blmsParam.acl_master_en){ //master only
        aclConn_param.maxTxOct = aclConn_param.maxTxOct_master;
    }
    else if(blmsParam.acl_slave_en){ //slave only
        aclConn_param.maxTxOct = aclConn_param.maxTxOct_slave;
    }
    else{ //no master no slave
        aclConn_param.maxTxOct = MAX_OCTETS_DATA_LEN_27;
    }
    //[!!!] HCI ACL buffer size >= 4 + DLE(blmsParam.acl_packet_length)
    blmsParam.acl_packet_length = aclConn_param.maxTxOct;

#if (ACL_TXFIFO_4K_LIMITATION_WORKAROUND)
    // CAL_LL_ACL_TX_FIFO_SIZE(230) = 240, CAL_LL_ACL_TX_FIFO_SIZE(231) = 256,   256*16=4096, error
    if( bltempParam.ll_aclTxCacheFifo_set && aclConn_param.maxTxOct > 230){
        blmsParam.cache_txfifo_used = 1;
        ll_push_tx_fifo_handler = blt_acl_pushCacheTxfifo;
    }
#endif


    if(bltempParam.ll_aclRxFifo_set){

        u16 rx_len_limit_min = 48; //27+21

        if(LL_FEATURE_MASK_0 & (LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE<<29)){
            rx_len_limit_min = 64; //36 + 21 = 57 then 16 align = 64
        }

        if(blt_rxfifo.num == 0 || blt_rxfifo.size < rx_len_limit_min){
            return LL_ACL_RX_BUF_PARAM_INVALID;
        }
        else if(blt_rxfifo.size < aclConn_param.maxRxOct + 21){
            return LL_ACL_RX_BUF_SIZE_NOT_MEET_MAX_RX_OCT;
        }
    }
    else{ //ACL RX buffer not set
        if(blmsParam.acl_master_en || blmsParam.acl_slave_en){  //ACL master or slave module init_d but ACL RX buffer not set
            return LL_ACL_RX_BUF_NO_INIT;
        }
    }

    if(bltempParam.ll_aclTxMasFifo_set){
        if(blt_m_txfifo.logic_num == 0 || blt_m_txfifo.size == 0)
        {
            return LL_ACL_TX_BUF_PARAM_INVALID;
        }
    #if (MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION)  //only B91 have this limitation
        else if(blt_m_txfifo.logic_num * blt_m_txfifo.size >= 4096 ){
            return LL_ACL_TX_BUF_SIZE_MUL_NUM_EXCEED_4K;
        }
    #endif
        else if(blt_m_txfifo.size < aclConn_param.maxTxOct_master + 10){
            return LL_ACL_TX_BUF_SIZE_NOT_MEET_MAX_TX_OCT;
        }
    }
    else{
        if(blmsParam.acl_master_en){  //ACL master module init_d but ACL TX buffer not set
            return LL_ACL_TX_BUF_NO_INIT;
        }
    }

    if(bltempParam.ll_aclTxSlvFifo_set){
        if(blt_s_txfifo.logic_num == 0 || blt_s_txfifo.size == 0)
        { //eagle:if(blt_s_txfifo.logic_num == 0 || blt_s_txfifo.size == 0)
            return LL_ACL_TX_BUF_PARAM_INVALID;
        }
    #if (MCU_HARDWARE_TX_FIFO_4K_BYTES_LIMITATION)  //only B91 have this limitation
        else if(blt_s_txfifo.logic_num * blt_s_txfifo.size >= 4096 ){
            return LL_ACL_TX_BUF_SIZE_MUL_NUM_EXCEED_4K;
        }
    #endif
        else if(blt_s_txfifo.size < aclConn_param.maxTxOct_slave + 10){
            return LL_ACL_TX_BUF_SIZE_NOT_MEET_MAX_TX_OCT;
        }
    }
    else{
        if(blmsParam.acl_slave_en){  //ACL slave module init_d but ACL TX buffer not set
            return LL_ACL_TX_BUF_NO_INIT;
        }
    }
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    for(u8 i=0; i<LL_MAX_ACL_CONN_NUM; i++){

        ll_data_extension_t *pExt_data = &blms[i].ext_data;

        pExt_data->connInitialMaxTxOctets   =  MAX_OCTETS_DATA_LEN_27;
        pExt_data->connEffectiveMaxTxOctets =  MAX_OCTETS_DATA_LEN_27;
        pExt_data->connEffectiveMaxRxOctets =  MAX_OCTETS_DATA_LEN_27;

        pExt_data->supportedMaxRxOctets = pExt_data->connMaxRxOctets = aclConn_param.maxRxOct;

#if (LL_MAX_ACL_CEN_NUM)
        if(i < LL_MAX_ACL_CEN_NUM){ //master
            pExt_data->supportedMaxTxOctets = pExt_data->connMaxTxOctets = aclConn_param.maxTxOct_master;
        }
        else
#endif
        {  //slave
            pExt_data->supportedMaxTxOctets = pExt_data->connMaxTxOctets = aclConn_param.maxTxOct_slave;
        }
    }
#endif





    if(ll_acl_master_mlp_task_cb){ // blt_acl_master_mainloop_task
        u32 init_status = ll_acl_master_mlp_task_cb(FLAG_CHECK_INIT, NULL); //blt_aclc_check_init
        if(init_status){
            return init_status;
        }
    }





    return INIT_SUCCESS;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
u8 blt_ll_getMaxAclCentralNumber(void)
{
    return blmsParam.max_master_num;
}



ble_sts_t blc_ll_readRemoteVersion(u16 connHandle)
{
    if (blt_ll_isAclhdlInvalid(connHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    u8 buf[20]={0};
    buf[0] = LLID_CONTROL;
    buf[1] = 6;//len
    buf[2] = LL_VERSION_IND;//opcode
    buf[3] = BLUETOOTH_VER;
    buf[4] = (u8)VENDOR_ID;
    buf[5] = VENDOR_ID>>8;
    buf[6] = (u8)BLUETOOTH_VER_SUBVER;
    buf[7] = (u8)(BLUETOOTH_VER_SUBVER >> 8);

    if(TRUE == ll_push_tx_fifo_handler(connHandle | HANDLE_STK_FLAG, buf) ){
        st_ll_conn_t* pc = (st_ll_conn_t*)&blms[connHandle&CONN_IDX_MASK];
        pc->llcp_flag.bit.ll_ver_ind_flag = 1;
        pc->ll_rsp_timeout_tick = clock_time() | 1;
    }
    return BLE_SUCCESS;
}




ble_sts_t   blc_hci_le_getRemoteSupportedFeatures(u16 connHandle)
{
    if (blt_ll_isAclhdlInvalid(connHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pAclConn = (st_ll_conn_t*)&blms[idx];


    if(pAclConn->llcp_flag.bit.ll_feat_exg_flag){ //already exchanged, used cached value
        /* here if we use "hci_le_readRemoteFeaturesComplete_evt", this event will report to host before CMD status event,
         * host may think it error, so we cache the event */
        pAclConn->remoteFeatureReq = FEATURE_HCI_REPORT; //only pending HCI report event
    }
    else if(pAclConn->remoteFeatureReq){ //other place have triggered feature_req, but feature_rsp not come
        pAclConn->remoteFeatureReq |= FEATURE_HCI_REPORT; //add HCI report event mark
    }
    else{
        if(!LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE && (pAclConn->aclRole == ACL_ROLE_PERIPHERAL)){
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
        else{
            pAclConn->remoteFeatureReq = FEATURE_SEND_FEAT_REQ | FEATURE_HCI_REPORT;
        }
    }

    return BLE_SUCCESS;
}

_attribute_noinline_
void blt_ll_authPayloadTimeoutExpiredHandler(u16 connHandle)
{
    //send ll ping cmd to peer device
    u16 dat[8];
    dat[0] = 0x0103;        //type, len
    dat[1] = LL_PING_REQ;
    ll_push_tx_fifo_handler(connHandle, (u8 *)dat);

    if(hci_eventMask_2 & HCI_EVT_MASK_AUTH_PAYLOAD_TIMEOUT_EXPIRED){
        hci_le_authPayloadTimeoutExpired_evt(connHandle);
    }
}

_attribute_noinline_
void blt_ll_processFeatureExchange(st_ll_conn_t* pAclConn)
{
    /*
     * If the Link Layer in the Central role supports receiving LL Control PDUs with a
     * CtrData field longer than 26 octets, it should initiate the Feature Exchange
     * procedure on each connection.
     */

    if(pAclConn->remoteFeatureReq == FEATURE_HCI_REPORT){
        hci_le_readRemoteFeaturesComplete_evt(BLE_SUCCESS, pAclConn->acl_conHandle, (u8 *)&pAclConn->ll_remoteFeature0);
        pAclConn->remoteFeatureReq = 0;
    }
    else{ // & FEATURE_SEND_FEAT_REQ or FeatureRsp
        u8 temp_buffer[sizeof(rf_packet_ll_feature_exg_t)];
        rf_packet_ll_feature_exg_t* pFeatExg = (rf_packet_ll_feature_exg_t*)temp_buffer;

        pFeatExg->type = LLID_CONTROL;
        pFeatExg->rf_len = 9;

        smemcpy(pFeatExg->featureSet, &LL_FEATURE_MASK_0, 4);
        smemcpy(pFeatExg->featureSet + 4, &LL_FEATURE_MASK_1, 4);
        pFeatExg->featureSet[3] &= ~BIT(3); //clean Remote public key valid bit, BIT(27)


        if(pAclConn->FeatureRsp){
            pFeatExg->opcode = LL_FEATURE_RSP;
            /*
             * Refer to <<Version 5.3 | Vol 6, Part B page 2707>>
             * The LL_FEATURE_RSP CtrData consists of one field:
             * # FeatureSet[0] shall contain a set of features supported by the Link Layers of
             *   both the Central and Peripheral.
             * # FeatureSet[1-7] shall contain a set of features supported by the Link Layer
             *   that transmits this PDU.
             */
            pFeatExg->featureSet[0] = (u8)(pAclConn->ll_remoteFeature0 & LL_FEATURE_MASK_0);
        }
        else{
            if(pAclConn->aclRole == ACL_ROLE_PERIPHERAL){
                #if(LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE)
                    pFeatExg->opcode = LL_SLAVE_FEATURE_REQ;
                #else //can not initiate feature exchange, return
                    pAclConn->remoteFeatureReq = 0;
                    return;
                #endif
            }
            else{ //ACL_ROLE_CENTRAL
                pFeatExg->opcode = LL_FEATURE_REQ;
            }
        }


        if(ll_push_tx_fifo_handler (pAclConn->acl_conHandle | HANDLE_STK_FLAG, (u8 *)pFeatExg)){
            if(pAclConn->FeatureRsp){
                pAclConn->FeatureRsp = 0;
            }
            else{
                pAclConn->remoteFeatureReq &= ~FEATURE_SEND_FEAT_REQ;
                pAclConn->remoteFeatureReq |= FEATURE_WAIT_FEAT_RSP;
                pAclConn->ll_rsp_timeout_tick = clock_time() | 1;
            }

        }
    #if OS_SUP_EN
        else{
            if(blt_os_giveSem_cb)
            {
                blt_os_giveSem_cb();
            }

        }
    #endif
    }
}


ble_sts_t   blc_hci_le_readChannelMap(u16 connHandle, u8 *returnChannelMap)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Chn_Map", &connHandle, 2);

    if (blt_ll_isAclhdlInvalid(connHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    smemcpy (returnChannelMap, pc->acl_chnParam.chmTbl, 5);

    return BLE_SUCCESS;
}






ble_sts_t   blc_hci_le_readBufferSize_cmd(hci_le_readBufSize_v1_retParam_t *pRetPara)
{
    if(!bltHci_rxAclfifo.num || !blmsParam.acl_packet_length){
        my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Buf_Size fail", 0, 0);
        return HCI_ERR_MEM_CAP_EXCEEDED;
    }

    //LE_ACL_Data_Packet_Length not bigger than 251(max DLE) in our design
    pRetPara->status = BLE_SUCCESS;

    pRetPara->acl_data_pkt_len = blmsParam.acl_packet_length;//[!!!] HCI ACL buffer size >= 4 + DLE(blmsParam.acl_packet_length)

    pRetPara->num_le_data_pkt = bltHci_rxAclfifo.num - 1;  //leave 1 for host error handle

    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Read_Buf_Size", &pRetPara->acl_data_pkt_len, 3);

    return BLE_SUCCESS;
}



//TODO: efficiency
bool blt_llmsPushLlCtrlPkt(u16 connHandle, u8 opcode, u8*pkt)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    pc->sentLlOpcode = opcode;

    return ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, pkt);///blt_llms_pushTxfifo
}


bool    blt_llms_unknownRsp(u16 connHandle, u8 unknownType)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    pc->ll_rsp_timeout_tick = 0;

    u8 unknownRsp[4];
    rf_packet_ll_unknown_rsp_t *pCtrl = (rf_packet_ll_unknown_rsp_t* )unknownRsp;
    pCtrl->type = LLID_CONTROL;
    pCtrl->rf_len = 2;
    pCtrl->opcode = LL_UNKNOWN_RSP;
    pCtrl->unknownType = unknownType;

    blt_llmsPushLlCtrlPkt(connHandle, LL_UNKNOWN_RSP, unknownRsp);


    return BLE_SUCCESS;
}

bool    blt_llms_rejectInd(u16 connHandle, u8 opCode, u8 errCode, u8 extRejectInd)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    pc->ll_rsp_timeout_tick = 0;

    u8 rejectPkt[5];
    if(extRejectInd){
        rf_packet_ll_reject_ext_ind_t* pRejectInd = (rf_packet_ll_reject_ext_ind_t*)rejectPkt;
        pRejectInd->rf_len = 3;
        pRejectInd->type = LLID_CONTROL;
        pRejectInd->opcode = LL_REJECT_IND_EXT;
        pRejectInd->rejectOpcode = opCode;
        pRejectInd->errCode = errCode;
    }
    else{
        rf_packet_ll_reject_ind_t* pRejectInd = (rf_packet_ll_reject_ind_t*)rejectPkt;
        pRejectInd->type = LLID_CONTROL;
        pRejectInd->rf_len = 2;
        pRejectInd->opcode = LL_REJECT_IND;
        pRejectInd->errCode = errCode;
    }


#if 1 //efficiency, REJECT_IND no need be marked
    return ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, rejectPkt);///blt_llms_pushTxfifo
#else

    blt_llmsPushLlCtrlPkt(connHandle, opCode, rejectPkt);

#endif
}

int  blt_llms_isConnectionEncrypted(u16 connHandle)
{
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    return  pc->crypt.enable;
}


ble_sts_t  blt_hci_ltkRequestReply(u16 connHandle,  u8*ltk)
{
    my_dump_str_data(0,"[HCI][CMD] LTK_Request_Reply", ltk, 16);


    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(!blt_ll_isAclhdlInvalid(connHandle))
    {
        smemcpy(pc->crypt.sk, ltk, 16);
        pc->crypt.st = MS_LL_ENC_RSP_T;
        return BLE_SUCCESS;
    }
    return HCI_ERR_UNKNOWN_CONN_ID;
}


ble_sts_t blt_hci_ltkRequestNegativeReply(u16 connHandle)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] LTK_Request_Negative_Reply", 0, 0);

    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];

    if(!blt_ll_isAclhdlInvalid(connHandle))
    {
        pc->crypt.st = MS_LL_REJECT_IND_T;
        return BLE_SUCCESS;
    }
    return HCI_ERR_UNKNOWN_CONN_ID;
}


ble_sts_t blt_ll_startEncryption (u16 connHandle ,u16 ediv, u8* random, u8* ltk)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];
    u8 ll_master_role = connHandle & BLM_CONN_HANDLE;

    if(ll_master_role){
        /*
        If the Connection_Handle parameter identifies an ACL with an associated CIS
        that has been created, the Controller shall return the error code Command
        Disallowed (0x0C).
        */
        #if(LL_FEATURE_ENABLE_CONNECTED_ISO)
            if(pc->cisEstablish_msk){
                return HCI_ERR_CMD_DISALLOWED;
            }
        #endif

        if(pc->crypt.enable){

            /*
            If the connection is already encrypted then the Controller shall pause
            connection encryption before attempting to authenticate the given encryption
            key, and then re-encrypt the connection. While encryption is paused no user
            data shall be transmitted.
             */
            pc->crypt.st = MS_LL_ENC_PAUSE_REQ;
        }
        else{
            pc->crypt.st = MS_LL_ENC_REQ;
            blt_ll_setEncryptionBusy (connHandle, 1);
        }
        pc->enc_ediv = ediv;
        smemcpy ( pc->enc_random , random, 8);

        smemcpy ( pc->crypt.ltk , ltk, 16);

        pc->crypt.mic_fail = BLE_SUCCESS; //clear fail reason

        return BLE_SUCCESS;
    }
    else{
        return HCI_ERR_UNKNOWN_HCI_CMD;
    }
}



ble_sts_t blc_hci_le_enableEncryption (hci_le_enableEncryption_cmdParam_t* pCmdParam)
{
    my_dump_str_data(IUT_HCI_LOG_EN, "[HCI][CMD] Start_Encryption", 0, 0);

    u16 ediv = pCmdParam->enc_div[1]<<8 | pCmdParam->enc_div[0];
    return blt_ll_startEncryption(pCmdParam->connHandle, ediv, pCmdParam->random_number, pCmdParam->long_term_key);
}



ble_sts_t   blc_hci_readMaximumDataLength(hci_le_readMaxDataLengthCmd_retParam_t  *para)
{
    para->status = BLE_SUCCESS;
    para->support_max_tx_oct = aclConn_param.maxTxOct;
    para->support_max_tx_time = LL_PACKET_OCTET_TIME(aclConn_param.maxTxOct);
    para->support_max_rx_oct = aclConn_param.maxRxOct;
    para->support_max_rx_time = LL_PACKET_OCTET_TIME(aclConn_param.maxRxOct);

    return BLE_SUCCESS;
}


ble_sts_t   blc_hci_setTxDataLength (u16 connHandle, u16 tx, u16 txtime)
{
    my_dump_str_data(STACK_DUMP_EN, "[I] HCI_CMD_LE_SET_DATA_LENGTH", 0,0);

    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
//  st_llm_conn_t* pm = (st_llm_conn_t *) &blmsMaster[conn_idx];

    if (!(connHandle & (BLS_CONN_HANDLE | BLM_CONN_HANDLE)) || !pc->connState)
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(tx < 27 || tx > 251 ||
       txtime < 328 || (txtime > 2120 && txtime < 2704 && txtime != 2128) || txtime > 17040){
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    blt_ll_exchangeDataLength(connHandle, LL_LENGTH_REQ, tx);


#endif

    return BLE_SUCCESS;
}


void        blc_ll_setDataLengthReqSendingTime_after_connCreate(int time_ms)
{
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    aclConn_param.connDleSendTimeUs = time_ms * 1000;
#endif
}



ble_sts_t   blc_hci_readSuggestedDefaultTxDataLength (u8 *tx, u8 *txtime)
{
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    tx[0] = aclConn_param.prefMaxTxLen & 0xff;
    tx[1] = (aclConn_param.prefMaxTxLen>>8)&0xff;
    txtime[0] = aclConn_param.prefMaxTxTime & 0xff;
    txtime[1] = (aclConn_param.prefMaxTxTime>>8) & 0xff;
#endif

    return BLE_SUCCESS;
}


ble_sts_t   blc_hci_writeSuggestedDefaultTxDataLength (u16 tx, u16 txtime)
{
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)  ///no matter master or slave, the initial function is same.
    aclConn_param.prefMaxTxLen = tx;
    aclConn_param.prefMaxTxTime = txtime;
#endif
    (void) tx;
    (void) txtime;
    return BLE_SUCCESS;
}


_attribute_noinline_
ble_sts_t blt_ll_exchangeDataLength (u16 connHandle, u8 opcode, u16 maxTxOct)
{
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    u8 conn_idx =  (connHandle & CONN_IDX_MASK);
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    ll_data_extension_t *pExt_data = &pc->ext_data;

    pExt_data->connMaxTxOctets = min(max(maxTxOct,27), pExt_data->supportedMaxTxOctets);

    u8  dat[12] = {0x03, 0x09, opcode};
    dat[3] = pExt_data->connMaxRxOctets;            //maxRxOctets
    dat[4] = 0;
    u16 t = LL_PACKET_OCTET_TIME (pExt_data->connMaxRxOctets);
    dat[5] = t;                             //maxRxTime
    dat[6] = t >> 8;

    dat[7] = pExt_data->connMaxTxOctets;            //maxTxOctets
    dat[8] = 0;
    t = LL_PACKET_OCTET_TIME (pExt_data->connMaxTxOctets);
    dat[9] = t;                             //maxTxTime
    dat[10] = t >> 8;

    if(blt_llmsPushLlCtrlPkt(connHandle, opcode, dat) ){  // BLS_CONN_HANDLE here for master is OK
        if(opcode == LL_LENGTH_REQ){
            pExt_data->connMaxTxRxOctets_req = 0;
            pc->ll_rsp_timeout_tick = clock_time() | 1;
        }
        else if(opcode == LL_LENGTH_RSP){
            pExt_data->connMaxTxRxOctets_req = 0;
            pc->ll_rsp_timeout_tick = 0;
        }

        //my_dump_str_u32s(0,"length exch", opcode, pExt_data->connMaxRxOctets, pExt_data->connMaxTxOctets, 0);
    }
    else{
        pExt_data->connMaxTxRxOctets_req = (opcode == LL_LENGTH_REQ) ? DATA_LENGTH_REQ_PENDING : DATA_LENGTH_RSP_PENDING;
    }
#endif
    (void) connHandle;
    (void) opcode;
    (void) maxTxOct;
    return BLE_SUCCESS;
}



ble_sts_t blc_ll_sendDateLengthExtendReq (u16 connHandle,  u16 maxTxOct)
{
    return blt_ll_exchangeDataLength (connHandle, LL_LENGTH_REQ, maxTxOct);
}

_attribute_noinline_
void blc_ll_dataLenAutoExgDisable(u16 connHandle)
{
    #if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    u8 conn_idx =  (connHandle & CONN_IDX_MASK);
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    ll_data_extension_t *pExt_data = &pc->ext_data;

    if(pExt_data->connMaxTxRxOctets_req == DATA_LENGTH_REQ_PENDING ){
        pExt_data->connMaxTxRxOctets_req = 0;
    }
    #endif
    (void) connHandle;
}

_attribute_noinline_
void blt_ll_procDlePending(u16 connHandle)
{
#if (LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION)
    u8 conn_idx =  (connHandle & CONN_IDX_MASK);
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    ll_data_extension_t *pExt_data = &pc->ext_data;

    if(pExt_data->connMaxTxRxOctets_req == DATA_LENGTH_REQ_PENDING ){
        if (!blt_ll_isEncryptionBusy(connHandle) && \
            pc->conn_established_tick && clock_time_exceed(pc->conn_established_tick, aclConn_param.connDleSendTimeUs)){
            blt_ll_exchangeDataLength(connHandle, LL_LENGTH_REQ, pExt_data->connMaxTxOctets);
        }
    }
    else if(pExt_data->connMaxTxRxOctets_req == DATA_LENGTH_RSP_PENDING ){
        if (!blt_ll_isEncryptionBusy(connHandle)){ //rcvd ll_length_req, and send ll_length_rsp failed, pending to send here
            blt_ll_exchangeDataLength(connHandle, LL_LENGTH_RSP, pExt_data->connMaxTxOctets);
        }
    }
#endif
    (void) connHandle;
}






#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
ble_sts_t   blc_hci_receiveHostACLData(hci_acl_data_pkt_t *pAclDatPkt)
{

    /* attention that: here code is different for multi_conn & single_conn SDK
     * multi_conn SDK must use HCI ACL RX FIFO managed by "bltHci_rxAclfifo", old code use "HCI_NEW_FIFO_FEATURE_ENABLE" to distinguish.
     * old single_conn SDK can save this FIFO, push data to ACL TX FIFO directly, and give a num_of_complete event */

    u16 connHandle = pAclDatPkt->connHandle;
    int len = pAclDatPkt->data_total_len;
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];
    if (!(connHandle & (BLS_CONN_HANDLE | BLM_CONN_HANDLE)) || !pc->connState)
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    if(len > blmsParam.acl_packet_length){
        //my_dump_str_u32s(DBG_HCI_FIFO, "acl data len exceed", len, blmsParam.acl_packet_length, 0, 0);
        BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCCC10000);
    }


    if( ((bltHci_rxAclfifo.wptr - bltHci_rxAclfifo.rptr) & 255) >= bltHci_rxAclfifo.num){
        //my_dump_str_u32s(DBG_HCI_FIFO, "acl fifo full", bltHci_rxAclfifo.wptr, bltHci_rxAclfifo.rptr, 0, 0);
        BLMS_ERR_DEBUG(DBG_HCI_FIFO, 0xCCC20000);

        return HCI_ERR_MEM_CAP_EXCEEDED;
    }


    u8 *p = bltHci_rxAclfifo.p + (bltHci_rxAclfifo.wptr++ & bltHci_rxAclfifo.mask) * bltHci_rxAclfifo.size;
    p[0] = U16_LO(connHandle);
    p[1] = U16_HI(connHandle);
    p[2] = pAclDatPkt->PB_Flag;
    p[3] = U16_LO(len);
    p[4] = U16_HI(len);

    smemcpy (p + 5, pAclDatPkt->data, len);



    return BLE_SUCCESS;
}








void blc_hci_writeAuthPayloadTimeout(hci_writeAuthPayloadTimeout_cmdParam_t *pCmdParam, hci_writeAuthPayloadTimeout_retParam_t *pRetParam)
{
#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
    pRetParam->status = BLE_SUCCESS;

    if (blt_ll_isAclhdlInvalid(pCmdParam->connHandle)){
        pRetParam->status = HCI_ERR_UNKNOWN_CONN_ID;
    }
    else{
        u8 idx = pCmdParam->connHandle & CONN_IDX_MASK;
        st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];
        //u8 ll_master_role = connHandle & BLM_CONN_HANDLE;

    #if LL_FEATURE_ENABLE_CONNECTION_SUBRATING
        if(pCmdParam->timeout*10*1000 < pc->conn_intvl_n_1m25*pc->factor*(1 + pc->conn_latency)*1250){
            pRetParam->status = HCI_ERR_CMD_DISALLOWED;
        }
    #else
        if(pCmdParam->timeout*10*1000 < pc->conn_intvl_n_1m25*(1 + pc->conn_latency)*1250){
            pRetParam->status = HCI_ERR_CMD_DISALLOWED;
        }
    #endif

        if(pRetParam->status == BLE_SUCCESS){
            pc->authPayloadTimeoutUs = pCmdParam->timeout*10*1000;
            pc->authPayloadTick = clock_time()|1;
        }
    }
#else
    (void)pCmdParam; //unused, remove warning
    (void)pRetParam; //unused, remove warning
    pRetParam->status = HCI_ERR_UNKNOWN_HCI_CMD;
#endif
}



void blc_hci_readAuthPayloadTimeout(u16 connHandle, hci_readAuthPduTimeout_retParam_t *pRetParam)
{
#if (LL_FEATURE_ENABLE_LE_PING && LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN)
    u8 ret_status = BLE_SUCCESS;
    if (blt_ll_isAclhdlInvalid(connHandle)){
        ret_status = HCI_ERR_UNKNOWN_CONN_ID;
    }
    else{
        u8 idx = connHandle & CONN_IDX_MASK;
        st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];
        //u8 ll_master_role = connHandle & BLM_CONN_HANDLE;


        pRetParam->auth_pdu_timeout = pc->authPayloadTimeoutUs/ 10000;
    }


    pRetParam->status = ret_status;
    pRetParam->connHandle = connHandle;
#else
    (void)connHandle; //unused, remove warning
    pRetParam->status = HCI_ERR_UNKNOWN_HCI_CMD;
#endif
}







#ifndef BLC_ZEPHYR_BLE_INTEGRATION
_attribute_ram_code_sec_noinline_
#endif
bool blt_ll_isRepeatedAclConnDevice(int start_idx, int end_idx)
{
    st_ll_conn_t* pAclConn;
    for(int i=start_idx; i<end_idx; i++)
    {
        pAclConn = (st_ll_conn_t*)&blms[i];
        if(pAclConn->connState && (pAclConn->conn_peerAddrType == bltAddr.peer_pka_or_ida_type) && \
            (!smemcmp(pAclConn->conn_peerAddr, bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN))){
            my_dump_str_data(STACK_DUMP_EN, "ERROR, reject repeated MAC address", bltAddr.peer_pka_or_ida_addr, BLE_ADDR_LEN);
            return TRUE;
        }
    }

    return FALSE;
}



#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION)
_attribute_ram_code_
#endif
void *blt_ll_getAclConnPtr(u16 connHandle)
{
    return (void*)&blms[connHandle & CONN_IDX_MASK];
}

u8 blt_llms_getConnState(u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    return pc->connState;
}

u8 blt_ll_getAclConnPeerAddrType(u16 connHandle)
{
    return blms[connHandle&CONN_IDX_MASK].conn_peerAddrType;
}

u8* blt_ll_getAclConnPeerAddr(u16 connHandle)
{
    return blms[connHandle&CONN_IDX_MASK].conn_peerPktA;
}

bool  blc_ll_isAclConnEstablished(u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    return (pc->connState==CONN_STATUS_ESTABLISH);
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
u32 blc_ll_getConnectionStartTick(u16 connHandle)
{
    return blms[connHandle & 15].conn_complete_tick;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
ble_sts_t blt_ll_isAclHandleOutOfRange(u16 connHandle)
{

    connHandle &= ~HANDLE_STK_FLAG;

    if( (connHandle >= BLM_HANDLE_MIN && connHandle < BLM_HANDLE_MAX_ADD_1) || \
        (connHandle >= BLS_HANDLE_MIN && connHandle < BLS_HANDLE_MAX_ADD_1) ){
        return BLE_SUCCESS;
    }
    else{
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
}


ble_sts_t blt_ll_isAclhdlInvalid(u16 connHandle)
{
    connHandle &= ~HANDLE_STK_FLAG;

    if( (connHandle >= BLM_HANDLE_MIN && connHandle < BLM_HANDLE_MAX_ADD_1) || \
        (connHandle >= BLS_HANDLE_MIN && connHandle < BLS_HANDLE_MAX_ADD_1) ){

        u8 conn_idx = connHandle & CONN_IDX_MASK;
        if(blms[conn_idx].connState){
            return BLE_SUCCESS;
        }
    }

    return HCI_ERR_UNKNOWN_CONN_ID;
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif
u8  blc_ll_getTxFifoNumber (u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    u32 r = irq_disable();

    u8 num = (pc->tx_wptr - pc->tx_rptr) & 31;

    if(pc->conn_fifo_flag){
        num += 1; //leave one space for inserting empty packet
    }


    irq_restore(r);

    return  num;
}




u8  blt_ll_getRealTxFifoNumber (u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    u32 r = irq_disable();

    u8 num = (pc->tx_wptr - pc->tx_rptr) & 31;

    irq_restore(r);

    return num;
}



u8  blt_llms_get_tx_fifo_max_num (u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    return pc->max_fifo_num;
}





int blc_ll_getCurrentConnectionNumber(void)
{
    return (blmsParam.cur_master_num + blmsParam.cur_slave_num);
}

int blc_ll_getSupportedMaxConnNumber(void)
{
    return (blmsParam.max_master_num + blmsParam.max_slave_num);
}

int blc_ll_getCurrentMasterRoleNumber(void)
{
    return blmsParam.cur_master_num;
}

int blc_ll_getCurrentSlaveRoleNumber(void)
{
    return blmsParam.cur_slave_num;
}





void        blc_ll_setAutoExchangeDataLengthEnable(int auto_dle_en)
{
    aclConn_param.auto_dle = auto_dle_en;
}



ble_sts_t  blt_ll_enc_proc_disConnect (u16 connHandle, u8 reason)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    if (blt_ll_isAclhdlInvalid(pc->acl_conHandle)){
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    pc->conn_termin_union.local_terminate = reason;  //mark //terminate reason
    pc->connMarkTxFifoWptr = pc->tx_wptr;
    pc->conn_terminate_tick = clock_time() | 1;

    return BLE_SUCCESS;
}


_attribute_noinline_
void blt_ll_rspTimeoutLoopEvt(u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    if(pc->ll_rsp_timeout_tick){
        /*
         * If the procedure response timeout timer reaches 40 seconds, the connection is considered lost.
         * The Link Layer exits the Connection State and shall transition to the Standby State. The Host
         * shall be notified of the loss of connection.
         */
        if(clock_time_exceed(pc->ll_rsp_timeout_tick, 40*1000*1000)){
            /*
             * Disconnect the local connection directly, and report the HOST disconnection
             * event, and it will not send LL_Terminate_IND to the peer device.
             */
            blt_ll_enc_proc_disConnect(connHandle, HCI_ERR_LMP_LL_RESP_TIMEOUT);
        }
    }
}



void  blt_ll_setEncryptionBusy(u16 connHandle, u8 enc_busy)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    pc->ll_enc_busy = enc_busy;
}


#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
int  blt_ll_isEncryptionBusy(u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    return pc->ll_enc_busy;
}




/////////////////// rf hold packet //////////////////////////

#ifndef BLC_ZEPHYR_BLE_INTEGRATION

_attribute_ble_data_retention_  static u8   ble_host_packet_hold[48] ;



u8 blt_ll_pushTxfifoHold (u16 connHandle, u8 *p)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    if (blt_ll_isAclhdlInvalid(connHandle)){
        return 0;
    }

    if (0 || blt_ll_isEncryptionBusy(connHandle)) // blt_smp_ms_isPairingBusy(connHandle)
    {
        if(!pc->blt_tx_pkt_hold ){
            pc->blt_tx_pkt_hold = 1;
            u8 max_size = (p[1]+2 > 48) ? 48 : p[1]+2;
            smemcpy (ble_host_packet_hold, p, max_size);
        }

        return 0;
    }
    else
    {
        ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, p);
    }

    return 1;
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#else
_attribute_noinline_
#endif
u8 blt_ll_push_hold_data (u16 connHandle)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[idx];

    if(pc->blt_tx_pkt_hold && !blt_ll_isEncryptionBusy(connHandle) && 1) // !blt_smp_ms_isPairingBusy(connHandle)
    {
        ll_push_tx_fifo_handler (connHandle | HANDLE_STK_FLAG, ble_host_packet_hold);
        pc->blt_tx_pkt_hold = 0;
        return 1;
    }

    return 0;
}

#endif /* #if !defined(BLC_ZEPHYR_BLE_INTEGRATION) */


#if(HW_AES_CCM_ALG_EN)
_attribute_ram_code_
void blt_save_aes_ccm_para(st_ll_conn_t *pc)
{
    pc->crypt.dec_pno = reg_rf_rx_ccm_pkt_cnt0_31;
    pc->crypt.enc_pno = reg_rf_tx_ccm_pkt_cnt0_31;
}

#endif




#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif  
void blc_ll_acl_resetInfoRSSI(u16 connHandle)
{
    ll_acl_recentAvgRSSI[connHandle & CONN_IDX_MASK] = 0;
}


#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif 
void blc_ll_acl_recordRSSI(u16 connHandle, u8 rssi)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    if(ll_acl_recentAvgRSSI[idx] == 0){
        ll_acl_recentAvgRSSI[idx] = rssi;
    }
    else{
        ll_acl_recentAvgRSSI[idx] = (ll_acl_recentAvgRSSI[idx] + rssi) >> 1;
    }
}

#if (STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2) //for RISC-V IRQ priority
_attribute_ram_code_
#endif 
void blc_ll_acl_recordLatestRSSI(u16 connHandle, u8 rssi)
{
    u8 idx = connHandle & CONN_IDX_MASK;
    ll_acl_recentRSSI[idx] = rssi;
}

u8 blc_ll_getAclLatestAvgRSSI(u16 connHandle)
{
    return  ll_acl_recentAvgRSSI[connHandle & CONN_IDX_MASK];
}

u8 blc_ll_getAclLatestRSSI(u16 connHandle)
{
    return  ll_acl_recentRSSI[connHandle & CONN_IDX_MASK];
}




u16 blc_ll_getAclConnectionInterval(u16 connHandle)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return 0;
    }
    st_ll_conn_t * pAclConn;
    u8 idx = connHandle & CONN_IDX_MASK;
    pAclConn = (st_ll_conn_t *)&blms[idx];
    return pAclConn->conn_intvl_n_1m25;
}

u16  blc_ll_getAclConnectionLatency(u16 connHandle)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return 0;
    }
    st_ll_conn_t * pAclConn;
    u8 idx = connHandle & CONN_IDX_MASK;
    pAclConn = (st_ll_conn_t *)&blms[idx];
    return pAclConn->conn_latency;
}

u16  blc_ll_getAclConnectionTimeout(u16 connHandle)
{
    if(BLE_SUCCESS != blt_ll_isAclhdlInvalid(connHandle))
    {
        return 0;
    }
    st_ll_conn_t * pAclConn;
    u8 idx = connHandle & CONN_IDX_MASK;
    pAclConn = (st_ll_conn_t *)&blms[idx];
    return (pAclConn->conn_timeout/(10*SYSTEM_TIMER_TICK_1MS));
}



//Customers who need to obtain protocol stack parameters can call this API directly
u8 blt_ll_getAclConnectionBlmsValue(u16 connHandle,u16 offset)
{
    u8 * pAclConn;
    if(sizeof(st_ll_conn_t) > offset)
    {
        u8 idx = connHandle & CONN_IDX_MASK;
        pAclConn = (u8 *)&blms[idx];
        return pAclConn[offset];
    }
    return 0;//
}



















#if (ACL_TXFIFO_4K_LIMITATION_WORKAROUND)
_attribute_ble_data_retention_  acl_cache_txfifo_t          blt_cache_txFifo;


ble_sts_t blc_ll_initAclConnCacheTxFifo(u8 *pTxbuf, int fifo_size, int fifo_number)
{

    bltempParam.ll_aclTxCacheFifo_set = 1;

    if( fifo_number == 16 ){
        blt_cache_txFifo.num = 16;
        blt_cache_txFifo.mask = 15;
    }
    else if( fifo_number == 32 ){
        blt_cache_txFifo.num = 32;
        blt_cache_txFifo.mask = 31;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }


    /*          1                     1           2       1      1      1~251     3
     * low byte of connHandle    encrypt_en     rsvd1    llid  rf_len    PDU    rsvd2
     *
     */
    if( fifo_size == 260 ){
        blt_cache_txFifo.size = fifo_size;
    }
    else{
        return LL_ERR_INVALID_PARAMETER;
    }

    blt_cache_txFifo.p = pTxbuf;



    return BLE_SUCCESS;
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
bool blt_acl_pushCacheTxfifo(u16 connHandle, u8 *p)
{
//  DBG_C HN4_TOGGLE;

    //Attention: connHandle can not be 0x00 ~ 0x06
    //u8 ll_master_role = (connHandle & BLM_CONN_HANDLE) ? 1 : 0; //role:  1: master, 0 slave
    u8 conn_idx = connHandle & CONN_IDX_MASK;
    st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];


    if( !pc->connState ){  // CONN_STATUS_DISCONNECT
        //my_dump_str_data(0, "[I] connection state : disconnect",0,0);
        return 0;
    }

    int sw_fifo_n = ((blt_cache_txFifo.wptr - blt_cache_txFifo.rptr) & 63 );
    if ( sw_fifo_n >= ((connHandle & HANDLE_STK_FLAG) ? blt_cache_txFifo.num : (blt_cache_txFifo.num - BLMS_STACK_USED_TX_FIFO_NUM)))
    {
        //my_dump_str_data(0, "[I] Tx FIFO overflow",0,0);
        return 0;
    }

//  DBG_C HN5_TOGGLE;

    u8 *pd = blt_cache_txFifo.p + (blt_cache_txFifo.wptr & blt_cache_txFifo.mask) * blt_cache_txFifo.size;

    pd[0] = U16_LO(connHandle);
    pd[1] = pc->crypt.enable;

    if(p[1] > 251){
        //STACK_ERR_DEBUG(TX_FIFO_DBG_EN, 0xF3010000);
        //my_dump_str_data(0, "[I] pushCacheTxfifo error",0,0);
        return 0;
    }


    #if 0 //ll_push_tx_fifo_handler used too many place, can not guarantee *p address is 4B aligned, for security, not use 4B copy
        //use 4 byte copy to save time
        /*  attention: 1. pointer address must be 4 byte aligned
         *             2. consider: length%4 not zero */
        u8 yushu = (p[1] + 2) & 3;
        u8 len = (p[1] + 2) - yushu;
        smemcpy4 (pd + 4, p, len);
        if(yushu){
            smemcpy (pd + 4 + len, p + len, yushu);
        }
    #else
        smemcpy (pd + 4, p, p[1] + 2);
    #endif

    blt_cache_txFifo.wptr++;

    return 1;
}

#if (TIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM)
_attribute_ram_code_
#endif
void blt_acl_cache_tx_fifo_to_hw_tx_fifo(void)
{
//  DBG_C HN6_TOGGLE;

    u16 connHandle;
    while (blt_cache_txFifo.wptr != blt_cache_txFifo.rptr)
    {
        u8 *raw_pkt = (u8 *) (blt_cache_txFifo.p + (blt_cache_txFifo.rptr & blt_cache_txFifo.mask) * blt_cache_txFifo.size);


        connHandle = (u16)raw_pkt[0];  //1 byte is enough for in design, attention "HANDLE_STK_FLAG" missed
        u8 ll_master_role = (connHandle & BLM_CONN_HANDLE) ? 1 : 0; //role:  1: master, 0 slave
        u8 conn_idx = connHandle & CONN_IDX_MASK;
        st_ll_conn_t* pc = (st_ll_conn_t*)&blms[conn_idx];


        /* if connection not available, drop data to prevent other connection data block */
        if( !pc->connState ){
            blt_cache_txFifo.rptr++;
            continue;
        }

        /* Different process for different MCU: ******************************************/
        u32 r = irq_disable();  //must add IRQ protect
        int sw_fifo_n = ((pc->tx_wptr     -     pc->tx_rptr) & 31 );
        irq_restore(r);

        u8 *pd;
        u8 tx_num_max = 0;
        if(ll_master_role){
        #if (LL_ACL_CEN_EN)
            pd = (u8 *)(blt_m_txfifo.p_base  +  (conn_idx * blt_m_txfifo.real_num + 1 + (pc->tx_wptr & blt_m_txfifo.mask)) *blt_m_txfifo.size );
            tx_num_max = blt_m_txfifo.logic_num;
        #endif
        }
        else{
        #if (LL_ACL_PER_EN)
            u8 slave_idx = conn_idx - LL_MAX_ACL_CEN_NUM;
            pd = (u8 *)(blt_s_txfifo.p_base  +  (slave_idx * blt_s_txfifo.real_num + 1 + (pc->tx_wptr & blt_s_txfifo.mask)) *blt_s_txfifo.size );
            tx_num_max = blt_s_txfifo.logic_num;
        #endif
        }

        int empty_space = ( pc->conn_fifo_flag & BIT(1) ) ? 1 : 0;   //need insert empty packet

        if ( sw_fifo_n >= ((connHandle & HANDLE_STK_FLAG) ? (tx_num_max - empty_space) : (tx_num_max - empty_space - BLMS_STACK_USED_TX_FIFO_NUM)))
        {
            break;
        }

        u8 *p = raw_pkt + 4; //low byte of connHandle(1) +   encrypt_en(1)   +  rsvd1(2)

        //TODO: add length check, prevent memory error
        smemcpy (pd + 4, p, p[1] + 2);

        if (raw_pkt[1]) //encrypt_en
        {
            /*
             * ll_ccm_enc: Master role must use 1, Slave role must use 0;
             * ll_ccm_dec: Master role must use 0, Slave role must use 1;
             */
            aes_enc_dec_busy = 1;
            aes_ll_ccm_encryption((llPhysChnPdu_t*)(pd + 4), ll_master_role, CRYPT_NONCE_TYPE_ACL, &pc->crypt);
            aes_enc_dec_busy = 0;
        }

        *(u32 *)pd = rf_tx_packet_dma_len(pd[5] + 2);

//      DBG_C HN7_TOGGLE;
        pc->tx_wptr++;
        blt_cache_txFifo.rptr++;

    #if (BLMS_PM_ENABLE)
        if(!ll_master_role && ll_acl_slave_mlp_task_cb){
            ll_acl_slave_mlp_task_cb(FLAG_ACL_SLAVE_CLEAR_SLEEP_LATENCY | conn_idx, NULL); //blt_acl_slave_mainloop_task
        }
    #endif
    }
}
#endif
