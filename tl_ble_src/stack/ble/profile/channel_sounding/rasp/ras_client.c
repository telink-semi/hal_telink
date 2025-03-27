/********************************************************************************************************
 * @file    ras_client.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

int blt_rasc_init(u8 initType, const void* param);
int blt_rasc_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_rasc_discovery(u16 connHandle);
int blt_rasc_loop(u16 connHandle);
static void blt_rasc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discRas;
#define BLC_RAS_START_SDP(connHandle)           blc_gapc_registerDiscoveryService(connHandle, &discRas)

static const blc_gapc_reconnList_t reconnRas;
#define BLC_RAS_START_RECONN(connHandle)       blc_gapc_registerReconnectService(connHandle, &reconnRas)

u16 storedRecordProcedureIndex = 0;

_attribute_ble_data_retention_
blc_ras_client_ctrl_t ras_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = CS_RAS_CLIENT,
        .usedAclRole = 0,
        .init = blt_rasc_init,
        .connect = blt_rasc_connect,
        .discov = blt_rasc_discovery,
        .loop = blt_rasc_loop,
    },
};

_attribute_ble_data_retention_
blc_ras_client_t gRasPrfClient[ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM];

/**
 * @brief       ranging profile get client control buffer.
 * @param[in]   instIndx: ACL connect index.
 * @return      ranging profile client control buffer pointer.
 */
blc_ras_client_t *blt_ras_prf_getClientControlBuffer(u8 instIdx)
{
    return &gRasPrfClient[instIdx];
}


/**
 * @brief       register ranging profile client controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_cs_registerRasProfileControlClient(const blc_rasc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t*)&ras_client_ctrl, param);
}

/**
 * @brief       ranging profile client get Client control instance by connect handle.
 * @param[in]   connHandle: ACL connection.
 * @return      client control instance.
 */
blc_ras_client_t *blt_rasc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return ras_client_ctrl.pRasPrfClient[idx];
}

/**
 * @brief       ranging profile client initial function.
 * @param[in]   initType: only PRF_PROC_INIT.
 * @param[in]   param: initial parameter.
 * @return      0.
 */
int blt_rasc_init(u8 initType, const void* param)
{
    (void)param;
    if(initType == PRF_PROC_INIT) {
        for (int i = 0; i < ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM; i++) {
            blc_ras_client_t *rasPrfClient = blt_ras_prf_getClientControlBuffer(i);
            ras_client_ctrl.pRasPrfClient[i] = rasPrfClient;
            memset(rasPrfClient, 0, sizeof(blc_ras_client_t));
        }
        BLC_RAS_LOG("client init");
    }
    return 0;
}

/**
 * @brief       ranging profile client connect/disconnect event callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   connState: PRF_ACL_STATE_DISCONN/PRF_ACL_STATE_CONNECT.
 * @return      0.
 */
int blt_rasc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    blc_ras_client_t *client = blt_rasc_getClientInst(connHandle);
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLC_RAS_LOG("Disconnect:0x%x", connHandle);
        memset(client, 0, sizeof(blc_ras_client_t));
    } else {
        BLC_RAS_LOG("Connect:0x%x", connHandle);
    }

    return 0;
}

/**
 * @brief       ranging profile client SDP discovery function.
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
int blt_rasc_discovery(u16 connHandle)
{
    if(blc_prf_checkDiscoveryBusy(connHandle))
        return 0;
    if(blc_prf_checkReconnectFlag(connHandle))
    {
        blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

        if(client->ntfInput.startHdl)
        {
            if(BLC_RAS_START_RECONN(connHandle) == BLE_SUCCESS)
            {
                blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CS_RAS_CLIENT, client->ntfInput.startHdl, client->ntfInput.endHdl);
                blc_prf_setDiscoveryStatusBusy(connHandle);
                CS_PRF_LOG("reconnect handle: 0x%x", connHandle);
            }
        }
        else
        {
            blc_prf_sendServiceDiscoveryFailEvent(connHandle, CS_RAS_CLIENT);
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
        return 0;
    }

    if(BLC_RAS_START_SDP(connHandle) == BLE_SUCCESS)
    {
        blc_prf_setDiscoveryStatusBusy(connHandle);
        CS_PRF_LOG("start discovery 0x%x", connHandle);
    }
    return 0;
}

/**
 * @brief       ranging profile client main loop function.
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
int blt_rasc_loop(u16 connHandle)
{
    (void)connHandle;
    return 0;
}

static bool blt_rasc_checkRecvProcedureDataState(blc_ras_client_t* client, blt_rasc_recv_state_enum state)
{
    if(client->recvState == RASC_RECV_STATE_NULL) {
        client->recvState = state;
        return true;
    }

    return client->recvState == state;
}

static void blt_rasc_recordLostSegmentInfo(u16 connHandle, u16 startSegment, u16 length)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    u16 buf_index = storedRecordProcedureIndex;
    u16 procCounter = client->rang_data.proc_data[buf_index].procedureCounter;

    ras_recordLostSegment_t* segmentLost = (ras_recordLostSegment_t*)(&client->rang_data.proc_data[buf_index].ras_segment);

    u8 index = segmentLost->index;
    u16 endSegment = 0;

    if(segmentLost->index == RECORD_LOST_SEGMENT_BUFF_LEN){
        segmentLost->index = RECORD_LOST_SEGMENT_BUFF_LEN - 1;
        index = segmentLost->index;
    }
    else{
        segmentLost->segment[index].segmentStart = startSegment;
    }
    segmentLost->index++;
    endSegment = startSegment + length - 1;
    segmentLost->segment[index].segmentEnd  = endSegment;
    segmentLost->segment[index].recordNumber = procCounter;
    CS_PRF_LOG("record lost segment info:index<%d> procCounter<%d>,startSegment<%d>,endSegment<%d>",index, procCounter,startSegment,endSegment);
}

static u32 blt_rasc_checkRealTimeSegmentationHeader(u16 connHandle, ras_raningData_t* rangingData, u8 dataLen)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    u16 MTU = blt_gap_getEffectiveMTU(connHandle)-4;

    if(rangingData->header.firstSegment && rangingData->header.segmentCounter)
    {
        return 0;
    }

    u16 segmentCount = client->rang_data.proc_data[0].expectSegmentCounter;

    if(rangingData->header.segmentCounter != (segmentCount & 0x3F))
    {
        u16 len = (rangingData->header.segmentCounter + 64 - (segmentCount & 0x3F))%64;
        segmentCount += len;
    }

    u8* pData = client->rang_data.proc_data[0].rangingData + MTU*segmentCount;

    client->rang_data.proc_data[0].rangingDataLen += dataLen;
    memcpy(pData, rangingData->data, dataLen);

    segmentCount ++;
    client->rang_data.proc_data[0].expectSegmentCounter =  segmentCount;
    return rangingData->header.lastSegment;
}

static u32 blt_rasc_checkOnDemandSegmentationHeader(u16 connHandle, ras_raningData_t* rangingData, u8 dataLen)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    u16 MTU = blt_gap_getEffectiveMTU(connHandle)-4;

    if(rangingData->header.firstSegment && rangingData->header.segmentCounter)
    {
        return 0;
    }

    if(client->rang_data.rangingDataIndex == 0){
        //err
        CS_PRF_LOG("on-Demand Data rcv abnormal:index == 0");
        return 0;
    }

    u16 onDemandProcCounter = 0;
    if(rangingData->header.firstSegment){
        onDemandProcCounter = rangingData->data[0]|(rangingData->data[1]<<8);
        storedRecordProcedureIndex = 0xffff;
        for(int i = 0;i< client->rang_data.rangingDataIndex;i++){
            if(onDemandProcCounter == client->rang_data.proc_data[i].procedureCounter){//procedure counter match
                storedRecordProcedureIndex = i;
                CS_PRF_LOG("prepare get on-Demand Data:index<%d>,procedureCounter<%d>",i,onDemandProcCounter);
                break;
            }
        }
        if(storedRecordProcedureIndex == 0xffff){
            storedRecordProcedureIndex = 0;
            CS_PRF_LOG("on-Demand Data rcv abnormal:procCount abnormal");
            return 0;
        }
    }


    u16 buf_index = storedRecordProcedureIndex;
    u16 segmentCount = client->rang_data.proc_data[buf_index].expectSegmentCounter;

    if(rangingData->header.segmentCounter != (segmentCount & 0x3F))
    {
        u16 len = (rangingData->header.segmentCounter + 64 - (segmentCount & 0x3F))%64;

        blt_rasc_recordLostSegmentInfo(connHandle,segmentCount,len);

        segmentCount += len;
    }

    u8* pData = client->rang_data.proc_data[buf_index].rangingData + MTU*segmentCount;

    client->rang_data.proc_data[buf_index].rangingDataLen += dataLen;
    memcpy(pData, rangingData->data, dataLen);

    segmentCount ++;
    client->rang_data.proc_data[buf_index].expectSegmentCounter =  segmentCount;
    return rangingData->header.lastSegment;
}

static u32 blt_rasc_checkOnDemandSegmentationLostHeader(u16 connHandle, ras_raningData_t* rangingData, u8 dataLen)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    u16 MTU = blt_gap_getEffectiveMTU(connHandle)-4;

    if(client->rang_data.lost_data_ctl.rcvSegmentLostStart == 0){
        return 0;
    }
    if(client->rang_data.rangingDataIndex == 0){
        //err
        CS_PRF_LOG("on-Demand Data rcv abnormal:index == 0");
        return 0;
    }

    storedRecordProcedureIndex = client->rang_data.lost_data_ctl.bffIndex;
    u16 buf_index = client->rang_data.lost_data_ctl.bffIndex;
    u16 segmentCount = client->rang_data.lost_data_ctl.expectSegmentCounter;

    if(rangingData->header.segmentCounter != (segmentCount & 0x3F))
    {
        u16 len = (rangingData->header.segmentCounter + 64 - (segmentCount & 0x3F))%64;

        blt_rasc_recordLostSegmentInfo(connHandle,segmentCount,len);

        segmentCount += len;
    }

    u8* pData = client->rang_data.proc_data[buf_index].rangingData + MTU*segmentCount;

    client->rang_data.proc_data[buf_index].rangingDataLen += dataLen;
    memcpy(pData, rangingData->data, dataLen);

    segmentCount ++;
    client->rang_data.lost_data_ctl.expectSegmentCounter =  segmentCount;
    return client->rang_data.lost_data_ctl.expectSegmentCounter == (client->rang_data.lost_data_ctl.segmentEnd+1);
}

static void blt_rasc_finishRecvRangingData(blc_ras_client_t* client)
{
    if(client->recvState == RASC_RECV_STATE_LIVE_DATA) {

    }
    else if(client->recvState == RASC_RECV_STATE_STORED_DATA) {

    }
    else
    {

    }
    client->recvState = RASC_RECV_STATE_NULL;
    storedRecordProcedureIndex = 0;
}

/**
 * @brief       ranging profile client receive server report notify data.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   attHdl: notify Attribute handle.
 * @param[in]   val: notify value pointer.
 * @param[in]   valLen: value length.
 * @return      none.
 */
static void blt_rasc_recvRealTimeProcedureData(u16 connHandle, u8* val, u16 valLen)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    if(client->recvState == RASC_RECV_STATE_NULL) {
        memset(&client->rang_data,0,sizeof(ras_rangingData_t));
    }
    if(!blt_rasc_checkRecvProcedureDataState(client, RASC_RECV_STATE_LIVE_DATA)) {
        BLC_RAS_LOG("connHandle is 0x%x Incorrect receiving real-time procedure data, receive state is %d", connHandle, client->recvState);
        return ;
    }

    if(blt_rasc_checkRealTimeSegmentationHeader(connHandle, (ras_raningData_t*)val, valLen-1))
    {
        tlkapi_send_string_data(DBG_CS_LOG_PRF_MASK_EN, "[CS][PRF]Real-time procedure data is", &client->rang_data,100);
        blt_prf_sendEvent(connHandle, CS_EVT_PROCEDURE_DATA,&client->rang_data,sizeof(ras_rangingData_t));
        blt_rasc_finishRecvRangingData(client);
    }
    CS_PRF_LOG("recv Real-time Procedure Data Notification");
}

static void blt_rasc_recvOnDemandProcedureData(u16 connHandle, u8* val, u16 valLen)
{
    CS_PRF_LOG("recv On-Demand Procedure Data Notification");
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    if(!blt_rasc_checkRecvProcedureDataState(client, RASC_RECV_STATE_STORED_DATA)) {
        BLC_RAS_LOG("connHandle: 0x%x Incorrect receiving On-Demand Procedure Data, receive state is %d", connHandle, client->recvState);
        return ;
    }
    if(RECORD_ONDEMAND_BUFF_LEN >= client->rang_data.rangingDataIndex){
        if(client->rang_data.lost_data_ctl.rcvSegmentLostStart){
            u32 complt = blt_rasc_checkOnDemandSegmentationLostHeader(connHandle, (ras_raningData_t*)val, valLen-1);
            if(complt){
                BLC_RAS_LOG("recv lost segment data end,segmentStart<%d>,segmentEnd<%d>",client->rang_data.lost_data_ctl.segmentStart,client->rang_data.lost_data_ctl.segmentEnd);
                client->rang_data.lost_data_ctl.rcvSegmentLostStart = 0;
            }
            return;
        }

        u32 lastSegmentFlag = blt_rasc_checkOnDemandSegmentationHeader(connHandle, (ras_raningData_t*)val, valLen-1);

        if(lastSegmentFlag){
            //BLC_RAS_LOG("recv one procedure data end,index<%d>",storedRecordProcedureIndex);
            storedRecordProcedureIndex++;
        }
    }
    else{
        //err
        BLC_RAS_LOG("rcv on-Demand Procedure Data buff not enough");
    }

}
static u8 blt_rasc_checkOnDemandProcedureSegment(u16 connHandle){
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    u8 segmentLostFlag = 0;
    for(int i = 0;i<client->rang_data.rangingDataIndex;i++){
        u8 index = client->rang_data.proc_data[i].ras_segment.index;
        if(index){
            segmentLostFlag = 1;
            index--;
            ras_getLostProcSegment_t* segmentLost = (ras_getLostProcSegment_t*)(&client->rang_data.proc_data[i].ras_segment.segment[index]);

            client->rang_data.lost_data_ctl.rcvSegmentLostStart = 1;
            client->rang_data.lost_data_ctl.bffIndex = i;
            client->rang_data.lost_data_ctl.recordNumber =  segmentLost->recordNumber;
            client->rang_data.lost_data_ctl.segmentStart = segmentLost->segmentStart;
            client->rang_data.lost_data_ctl.segmentEnd = segmentLost->segmentEnd;
            client->rang_data.lost_data_ctl.expectSegmentCounter = segmentLost->segmentStart;
            client->rang_data.proc_data[i].ras_segment.index--;
            int ret = blt_rasc_writeGetRecordSegments(connHandle, segmentLost->recordNumber, segmentLost->segmentStart, segmentLost->segmentEnd,NULL);
            BLC_RAS_LOG("msg Get Lost Procedure Data cmd:recordNumber<%d>,segmentStart<%d>,segmentEnd<%d>,ret<%d>",segmentLost->recordNumber, segmentLost->segmentStart, segmentLost->segmentEnd,ret);
            break;
        }
    }

    return segmentLostFlag;
}


static void blt_rasc_recvRasControlPoint(u16 connHandle, u8* val, u16 valLen)
{
    (void)valLen;
    ras_cp_msg_t* msg = (ras_cp_msg_t*) val;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    if ((msg->opcode == RAS_CP_RSP_OPCODE_COMPLETE_REPORT_RECORDS) \
            || (msg->opcode == RAS_CP_RSP_OPCODE_COMPLETE_RECORD_SEGMENT)) {

        u16 numOfRecord = msg->operand.completeReportRecord.numOfRecords;
        (void)numOfRecord;
        CS_PRF_LOG("recv Complete Report Records Response: Number of record<%d>", numOfRecord);

        u8 segmentLostFlag = blt_rasc_checkOnDemandProcedureSegment(connHandle);
        u16 procedureCounter = client->rang_data.proc_data[0].procedureCounter;
        if(segmentLostFlag == 0){
            CS_PRF_LOG("msg ACK Stored Records Command:index<%d>",client->rang_data.rangingDataIndex);
            if(client->rang_data.rangingDataIndex >1){//todo ,profile decide
                blt_rasc_writeAckRecordsGreaterEqual(connHandle,procedureCounter,NULL);
            }
            else{
                blt_rasc_writeAckSpecificRecord(connHandle, procedureCounter, NULL);
            }
            tlkapi_send_string_data(DBG_CS_LOG_PRF_MASK_EN, "store data is", &client->rang_data, 100);

            blt_prf_sendEvent(connHandle, CS_EVT_PROCEDURE_DATA, &client->rang_data,sizeof(ras_rangingData_t));

            blt_rasc_finishRecvRangingData(client);
        }

    }
}

static void blt_rasc_recvProcedureDataReady(u16 connHandle, u8* val, u16 valLen)
{
    (void)valLen;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    ras_dataReady_t* ready = (ras_dataReady_t*) val;
    if(client->recvState == RASC_RECV_STATE_NULL) {
        memset(&client->rang_data,0,sizeof(ras_rangingData_t));
    }
    if( client->rang_data.rangingDataIndex < RECORD_ONDEMAND_BUFF_LEN){
        client->rang_data.proc_data[client->rang_data.rangingDataIndex].procedureCounter = ready->procedureCounter;
        client->rang_data.rangingDataIndex++;
        client->rang_data.onDemandDataFlag = 1;
        CS_PRF_LOG("recv Procedure Data Ready, connHandle<0x%x>, procedureCounter<%d>, index<%d>", connHandle, ready->procedureCounter ,client->rang_data.rangingDataIndex);
    }
    else{
        CS_PRF_LOG("recv Procedure Data buffer not enough");
    }

    //todo, profile decide
    blt_rasc_writeGetSpecificRecord(connHandle, ready->procedureCounter, NULL);//one procedure
}

static void blt_rasc_recvRangingDataOverwritten(u16 connHandle, u8* val, u16 valLen)
{
    (void)connHandle;
    (void)valLen;
    ras_dataOverwritten_t* overwrite = (ras_dataOverwritten_t*) val;
    (void)overwrite;
    storedRecordProcedureIndex = 0;
    CS_PRF_LOG("recv Procedure Data Overwritten Indication, connHandle<0x%x>, procedureCounter<%d>", connHandle, overwrite->procedureCounter);
    //@XH: add overwritten logic on Client
}

static void blt_rasc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    CS_PRF_LOG("recv data<connHdl:0x%X> on<attHdl:0x%X>:%s", connHandle, attHdl, hex_to_str(val, valLen));

    if(attHdl == client->liveRangingDataHandle) {
        blt_rasc_recvRealTimeProcedureData(connHandle, val, valLen);
    }
    else if(attHdl == client->storedRangingDataHandle) {
        blt_rasc_recvOnDemandProcedureData(connHandle, val, valLen);
    }
    else if(attHdl == client->rasControlPointHandle) {
        blt_rasc_recvRasControlPoint(connHandle, val, valLen);
    }
    else if(attHdl == client->rangingDataReadyHandle) {
        blt_rasc_recvProcedureDataReady(connHandle, val, valLen);
    }
    else if(attHdl == client->rangingDataOverwrittenHandle) {
        blt_rasc_recvRangingDataOverwritten(connHandle, val, valLen);
    }
}

/**
 * @brief       ranging profile client display SDP/reconnect server information.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   client: notify Attribute handle.
 * @return      none.
 */
static void blt_rasc_displayInfo(u16 connHandle, blc_ras_client_t* client)
{
    (void)connHandle;
    (void)client;
    CS_PRF_LOG("RAS sdp over connHandle[0x%02x]", connHandle);

    CS_PRF_LOG("RAS Feature Handle: 0x%04x, Properties: 0x%02x , Value: 0x%08x", client->rasFeatureHandle, client->rasFeatureProperties ,client->ras_feature.features);

    CS_PRF_LOG("Live Ranging Data Handle: 0x%04x, Properties: 0x%02x", client->liveRangingDataHandle, client->liveRangingDataProperties);

    CS_PRF_LOG("Stored Ranging Data Handle: 0x%04x, Properties: 0x%02x", client->storedRangingDataHandle, client->storedRangingDataProperties);

    CS_PRF_LOG("RAS Control Point Handle: 0x%04x, Properties: 0x%02x", client->rasControlPointHandle, client->rasControlPointProperties);

    CS_PRF_LOG("Ranging Data Ready Handle: 0x%04x, Properties: 0x%02x", client->rangingDataReadyHandle, client->rangingDataReadyProperties);

    CS_PRF_LOG("Ranging Data Overwritten Handle: 0x%04x, Properties: 0x%02x", client->rangingDataOverwrittenHandle, client->rangingDataOverwrittenProperties);

}

/**
 * @brief       ranging profile client SDP found/not-found service UUID.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   count: 0xFF: mean not found this service UUID. 0x00: mean found service UUID finish. other: service number.
 * @param[in]   startHandle: service start attribute handle.
 * @param[in]   endHandle: service end attribute handle
 * @return      none.
 */
static void blt_rasc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, CS_RAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLC_RAS_LOG("ERR: not found RAS");
        return ;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, CS_RAS_CLIENT);
        blt_rasc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }

    client->ntfInput.startHdl = startHandle;
    client->ntfInput.endHdl = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_rasc_dataInput;
    CS_PRF_LOG("    INFO: RAS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CS_RAS_CLIENT, startHandle, endHandle);
}

static void blt_rasc_rasFeatureStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    if(client == NULL)
    {
        return ;
    }

    *read = (u8*)&client->ras_feature.features;
    *readLen = NULL;
    *readMaxSize = sizeof(client->ras_feature.features);
    *rdCbFunc = NULL;
    CS_PRF_LOG("Ras feature read info");
}

static void blt_rasc_rasFeatureChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    client->rasFeatureHandle = valueHandle;
    client->rasFeatureProperties = properties;

    CS_PRF_LOG("Ras Feature connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

/**
 * @brief       ranging profile client SDP found ranging in characteristic UUID.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   serviceCount: service number.
 * @param[in]   properties: characteristic properties.
 * @param[in]   valueHandle: characteristic attribute value handle
 * @return      none.
 */
static void blt_rasc_realTimeProcedureDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    client->liveRangingDataHandle = valueHandle;
    client->liveRangingDataProperties = properties;

    CS_PRF_LOG("Real-time Procedure Data connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_rasc_onDemandProcedureDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    client->storedRangingDataHandle = valueHandle;
    client->storedRangingDataProperties = properties;

    CS_PRF_LOG("On-Demand Procedure Data connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_rasc_controlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    client->rasControlPointHandle = valueHandle;
    client->rasControlPointProperties = properties;

    CS_PRF_LOG("RAS Control Point connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_rasc_rangingDataReadyChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    client->rangingDataReadyHandle = valueHandle;
    client->rangingDataReadyProperties = properties;

    CS_PRF_LOG("Ranging Data Ready connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_rasc_rangingDataOverwrittenChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    client->rangingDataOverwrittenHandle = valueHandle;
    client->rangingDataOverwrittenProperties = properties;

    CS_PRF_LOG("Ranging Data Overwritten connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static const blc_gapc_discService_t rasService = {
    .uuid = UUID16_INIT(SERVICE_UUID_RANGING),
    .sfun = blt_rasc_foundService,
};

static const blc_gapc_discChar_t rasChar[] = {
    {
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_RAS_FEATURE),
        .cfun = blt_rasc_rasFeatureChar,
        .rfun = blt_rasc_rasFeatureStartRead,
    },

    {
        .subscribeNtf = true,
        .subscribeInd = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_REAL_TIME_PROCEDURE_DATA),
        .cfun = blt_rasc_realTimeProcedureDataChar,
    },

    {
        .subscribeNtf = true,
        .subscribeInd = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_ON_DEMAND_PROCEDURE_DATA),
        .cfun = blt_rasc_onDemandProcedureDataChar,
    },

    {
        .subscribeInd = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_CONTROL_POINT),
        .cfun = blt_rasc_controlPointChar,
    },

    {
        .subscribeNtf = true,
        .subscribeInd = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_RANGING_DATA_READY),
        .cfun = blt_rasc_rangingDataReadyChar,
    },

    {
        .subscribeNtf = true,
        .subscribeInd = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_RANGING_DATA_OVERWRITTEN),
        .cfun = blt_rasc_rangingDataOverwrittenChar,
    },
};

static const blc_gapc_discList_t discRas = {
    .maxServiceCount = 1,
    .service = &rasService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(rasChar),
        .characteristic = rasChar,
    },
};

/**
 * @brief       ranging profile client reconnect service callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   count: 0: mean reconnect service finish. other: service count.
 * @return      none.
 */
static bool blt_rasc_reconnService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);
        blt_rasc_displayInfo(connHandle, client);
        CS_PRF_LOG("  INFO: Ranging Profile connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, CS_RAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}

static const blc_gapc_reconnChar_t reRasChar[] = {

};

static const blc_gapc_reconnList_t reconnRas = {
    .resfun = blt_rasc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reRasChar),
        .characteristic = reRasChar,
    },
    .inclSize = 0,
};



/**
 * @brief       ranging profile client write ranging in characteristic value callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   err: ATT layer return error code.
 * @param[in]   data: RFU.
 * @return      none.
 */
static void blc_rasc_writeControlPointCb(u16 connHandle, u8 err, void* data)
{
    (void)data;
    CS_PRF_LOG("write control point callback, connHandle is 0x%02x, err is %x", connHandle, err);
    blc_prf_writeAttributeValueCallback(connHandle, err);
}

/**
 * @brief       ranging profile client write ranging in characteristic value with ATT_WRITE_REQ/PREPARE_WRITE_REQ command.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   val: want write value .
 * @param[in]   writeCb: write command send callback function.
 * @return      ble_sts_t.
 */
static int blt_rasc_writeRasControlPoint(u16 connHandle, blt_ras_cp_command_opcode_enum opcode,
                                                blt_ras_cp_operator_enum operator, void* operand, u16 operandLen, prf_write_cb_t writeCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLC_RAS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_ras_client_t* client = blt_rasc_getClientInst(connHandle);

    if(!client->rasControlPointHandle || !(client->rasControlPointProperties & CHAR_PROP_WRITE) ||
            RAS_CP_CHECK_CMD_OPCODE(opcode))
    {
        BLC_RAS_LOG("Control Point, properties not had write");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    ras_cp_msg_t msg = {
        .opcode = opcode,
        .operator = operator,
    };

    memcpy(&msg.operand.val[0], operand, operandLen);

    gapc_write_cfg_t pGapWrCfg;
    pGapWrCfg.func = blc_rasc_writeControlPointCb;
    pGapWrCfg.handle = client->rasControlPointHandle;
    pGapWrCfg.data = &msg;
    pGapWrCfg.length = operandLen + RAS_CP_MSG_HEADER_SIZE;
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData = NULL;

    CS_PRF_LOG("send Write RAS-CP: %s", hex_to_str(pGapWrCfg.data, pGapWrCfg.length));
    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
}


int blt_rasc_writeGetSpecificRecord(u16 connHandle, u32 procedureCounter, prf_write_cb_t writeCb)
{
    operands_getOneRecord_t operand = {
        .procedureCounter = procedureCounter
    };
    CS_PRF_LOG("msg Get Procedure Data Command: procedureCounter = %d", procedureCounter);
    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_GET_REPORT_RECORDS, RAS_CP_OPERATOR_ONE_RECORD,
                                            &operand, sizeof(operands_getOneRecord_t), writeCb);
}

int blt_rasc_writeGetRecordsGreaterEqual(u16 connHandle, u32 recordNum, prf_write_cb_t writeCb)
{
    operands_getMoreRecord_t operand = {
        .recordNum = recordNum
    };

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_GET_REPORT_RECORDS, RAS_CP_OPERATOR_GREATER_OR_EQUAL,
                                            &operand, sizeof(operands_getMoreRecord_t), writeCb);
}

int blt_rasc_writeAckSpecificRecord(u16 connHandle, u32 procedureCounter, prf_write_cb_t writeCb)
{
    operands_ackStoredOneRecord_t operand = {
        .procedureCounter = procedureCounter
    };

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_ACK_STORED_RECORDS, RAS_CP_OPERATOR_ONE_RECORD,
                                            &operand, sizeof(operands_ackStoredOneRecord_t), writeCb);
}

int blt_rasc_writeAckRecordsGreaterEqual(u16 connHandle, u32 recordNum, prf_write_cb_t writeCb)
{
    operands_ackStoredMoreRecord_t operand = {
        .recordNum = recordNum
    };

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_ACK_STORED_RECORDS, RAS_CP_OPERATOR_GREATER_OR_EQUAL,
                                            &operand, sizeof(operands_ackStoredMoreRecord_t), writeCb);
}

int blt_rasc_writeGetRecordSegments(u16 connHandle, u16 recordNum, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb)
{
    operands_getRecordSegments_t operand = {
        .recordNum = recordNum,
        .startAbsoluteSegment = startSegment,
        .endAbsoluteSegment = endSegment,
    };

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_GET_RECORD_SEGMENT, RAS_CP_OPERATOR_NULL,
                                            &operand, sizeof(operands_getRecordSegments_t), writeCb);
}

int blt_rasc_writeAbortOperation(u16 connHandle, u32 recordNum, u32 startSegment, u32 endSegment, prf_write_cb_t writeCb)
{
    (void)recordNum;
    (void)startSegment;
    (void)endSegment;
    operands_abortOperation_t operand;

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_ABORD_OPERATION, RAS_CP_OPERATOR_NULL,
                                            &operand, sizeof(operands_abortOperation_t), writeCb);
}
