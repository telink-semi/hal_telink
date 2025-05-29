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
#include "stack/ble/host/gatt/tlk_malloc_stack.h"


#include "ras_internal.h"

static int       blt_rasc_init(u8 initType, const void *param);
static int       blt_rasc_connect(u16 connHandle, prf_acl_state_enum connState);
static int       blt_rasc_discovery(u16 connHandle);
static int       blt_rasc_loop(u16 connHandle);
static int       blt_rasc_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);
static void      blt_rasc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
static void      blt_rasc_finishRecvRangingData(blc_rasc_client_t *client);
static ble_sts_t blt_rasc_setCccValue(u16 connHandle, u16 cccHandle, u16 value, prf_write_cb_t writeCb);
static void      blt_rasc_clearAndInitializeRemote(blc_rasc_client_t *client);

static void      blt_rasc_insertSegmentAtEnd(blc_rasc_client_t *client, u8 *pData, u8 dataLen, u8 index);
static ble_sts_t blt_rasc_insertSegmentByIndex(blc_rasc_client_t *client, u8 *pData, u8 dataLen, u8 index);
static void      blt_rasc_clearSegmentList(blc_rasc_client_t *client);
static void      blt_rasc_clearMergedData(blc_rasc_client_t *client);
static u8       *blt_rasc_mergeSegmentList(blc_rasc_client_t *client);

static ble_sts_t blt_rasc_writeGetSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb);
static ble_sts_t blt_rasc_writeAckSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb);
static ble_sts_t blt_rasc_writeGetRecordSegments(u16 connHandle, u16 rangingCounter, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb);
static ble_sts_t blt_rasc_writeAbortOperation(u16 connHandle, prf_write_cb_t writeCb);

static void blt_rasc_recordLostSegmentInfo(blc_rasc_client_t *client, u16 startSegment, u16 consecutiveLostSegmCountWildcard);
static u32  blt_rasc_checkRealTimeSegmentationHeader(blc_rasc_client_t *client, u16 connHandle, blt_ras_segmentation_ranging_data_t *rangingData, u8 dataLen);
static u32  blt_rasc_checkOnDemandSegmentationHeader(blc_rasc_client_t *client, u16 connHandle, u8 lost, blt_ras_segmentation_ranging_data_t *rangingData, u8 dataLen);
static u8   blt_rasc_checkOnDemandProcedureSegment(blc_rasc_client_t *client, u16 connHandle);

void blt_rasc_setRemoteDataReady(u16 connHandle);

static void blt_rasc_recvRealTimeProcedureData(u16 connHandle, u8 *val, u16 valLen);
static void blt_rasc_recvOnDemandProcedureData(u16 connHandle, u8 *val, u16 valLen);
static void blt_rasc_recvRasControlPoint(u16 connHandle, u8 *val, u16 valLen);
static void blt_rasc_recvProcedureDataReady(u16 connHandle, u8 *val, u16 valLen);
static void blt_rasc_recvRangingDataOverwritten(u16 connHandle, u8 *val, u16 valLen);

static void blt_rasc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle);
static bool blt_rasc_reconnService(u16 connHandle, int count);
static void blt_rasc_displayInfo(u16 connHandle, blc_rasc_client_t *client);
static void blt_rasc_rasFeatureStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc);
static void blt_rasc_rasFeatureChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle);
static int blt_rasc_rasFeatureGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo);
static void blt_rasc_realTimeProcedureDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle);
static int blt_rasc_realTimeProcedureGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo);
static void blt_rasc_realtimeProcedureCcc(u16 connHandle, u16 cccHandle, u8 result);
static void blt_rasc_onDemandProcedureDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle);
static int blt_rasc_onDemandProcedureGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo);
static void blt_rasc_ondemandProcedureCcc(u16 connHandle, u16 cccHandle, u8 result);
static void blt_rasc_controlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle);
static int blt_rasc_controlPointGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo);
static void blc_rasc_writeControlPointCb(u16 connHandle, u8 err, void *data);
static void blt_rasc_rangingDataReadyChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle);
static int blt_rasc_rangingDataReadyGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo);
static void blt_rasc_rangingDataOverwrittenChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle);
static int blt_rasc_rangingDataOverwrittenGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo);

static ble_sts_t blt_rasc_writeRasControlPoint(u16 connHandle, blt_ras_cp_command_opcode_enum opcode, void *operand, u16 operandLen, prf_write_cb_t writeCb);

static bool blt_rasc_checkRecvProcedureDataState(blc_rasc_client_t *client, blt_rasc_recv_state_enum state);
static void blt_rasc_setRequestedRangingCounter(blc_rasc_client_t *client, u16 rangingCounter);
static u16  blt_rasc_getRequestedRangingCounter(blc_rasc_client_t *client);

//TODO: Fix if needed, as its not working properly now
//static int __attribute__((unused)) blt_rasc_readAttributeValue(u16 connHandle, u16 handle, u8 *wBuff, u16 *wBuffLen, u16 maxLen, void *rdCbFunc);
static ble_sts_t blt_rasc_readAttributeValue(u16 connHandle, blt_rasc_read_t readType, prf_read_cb_t rdCbFunc);
static void blt_rasc_readAttributeValueCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg);

#if (RAS_TIMEOUT_EN)
static ble_sts_t blt_rasc_addTimeout(u16 connHandle, u16 rangingCounter, blc_ras_timer_type_enum type, u32 interval, blt_ras_timer_cb_t cb);
static ble_sts_t blt_rasc_updateTimeout(u16 connHandle, blc_ras_timer_type_enum type, u32 interval);
static ble_sts_t blt_rasc_deleteTimeout(u16 connHandle);
static ble_sts_t blt_rasc_checkTimeout(u16 connHandle);
static ble_sts_t blt_rasc_checkLocalDataTimeout(u16 connHandle);

static ble_sts_t blt_rasc_ondemandDataTimeout(u16 connHandle, u16 rangingCounter, u8 /*blc_ras_timer_type_enum*/ type);
static ble_sts_t blt_rasc_rangingDataReadyTimeout(u16 connHandle, u16 rangingCounter, u8 /*blc_ras_timer_type_enum*/ type);
static ble_sts_t blt_rasc_realtimeDataTimeout(u16 connHandle, u16 rangingCounter, u8 /*blc_ras_timer_type_enum*/ type);
#endif

#if (TTF_EN)
extern void ttf_log_buffer_with_label(const void *buff, u16 len, char *label);
#endif

#if (RAS_PTS_LOST_SEGMENT_WORKAROUND)
u8 segmentsToSkipCount; //I did not want to add it to the ras client object
#endif

static const blc_gapc_discList_t discRas;
#define BLC_RAS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discRas)

static const blc_gapc_reconnList_t reconnRas;
#define BLC_RAS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnRas)
#if ((!defined(HOST_V2_ENABLE)))
_attribute_ble_data_retention_
    blc_ras_client_ctrl_t ras_client_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = CS_RAS_CLIENT,
                    .usedAclRole = 0,
                    .init        = blt_rasc_init,
                    .connect     = blt_rasc_connect,
                    .discov      = blt_rasc_discovery,
                    .loop        = blt_rasc_loop,
                    .store       = blt_rasc_store,
                    },
};
#else
static const struct blc_prf_process_params s_ras_client_process_params = {
    .id          = CS_RAS_CLIENT,
    .usedAclRole = PRF_GAP_ACL_UNSPECIF,
    .init        = blt_rasc_init,
    .connect     = blt_rasc_connect,
    .discovery   = blt_rasc_discovery,
    .store       = NULL,
};

_attribute_ble_data_retention_ blc_ras_client_ctrl_t ras_client_ctrl = {
    .process = {
                .next       = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_ras_client_process_params,
                },
};
#endif

/**
 * @brief       ranging profile client get Client control instance by connect handle.
 * @param[in]   connHandle: ACL connection.
 * @return      client control instance.
 */
blc_rasc_client_t *blc_rasc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return idx >= 0 ? ras_client_ctrl.pRasPrfClient[idx] : NULL;
}

#if (RAS_TIMEOUT_EN)
static ble_sts_t blt_rasc_addTimeout(u16 connHandle, u16 rangingCounter, blc_ras_timer_type_enum type, u32 interval, blt_ras_timer_cb_t cb)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    if (!cb) {
        return HCI_ERR_INVALID_HCI_CMD_PARAMS;
    }

    client->timeoutRecord.connHandle     = connHandle;
    client->timeoutRecord.rangingCounter = rangingCounter;
    client->timeoutRecord.type           = type;
    client->timeoutRecord.interval       = interval;
    client->timeoutRecord.timestamp      = stimer_get_tick();
    client->timeoutRecord.cb             = cb;
    BLC_RAS_LOG("Timeout added on handle: %x, rangingCounter %d, type %d, interval %d", connHandle, rangingCounter, type, interval);
    debugwait();

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rasc_updateTimeout(u16 connHandle, blc_ras_timer_type_enum type, u32 interval)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    if (client->timeoutRecord.type != type) {
        BLC_RAS_LOG("WARNING Timeout type different: old %d, new %d, type updated", client->timeoutRecord.type, type);
        debugwait();
        client->timeoutRecord.type = type;
    }
    client->timeoutRecord.interval  = interval;
    client->timeoutRecord.timestamp = stimer_get_tick();
    BLC_RAS_LOG("Timeout updated on: %x", connHandle);

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rasc_deleteTimeout(u16 connHandle)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    memset(&client->timeoutRecord, 0, sizeof(blt_rasc_timeout_record_t));
    BLC_RAS_LOG("Timeout deleted on: %x", connHandle);
    debugwait();

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rasc_checkTimeout(u16 connHandle)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    if (client->timeoutRecord.cb != NULL) {
        u32 delta_time = stimer_get_tick() - client->timeoutRecord.timestamp;
        if (client->timeoutRecord.interval < delta_time) {
            BLC_RAS_LOG("Action timeout hit! timestamp:%lu delta:%lu", client->timeoutRecord.timestamp, delta_time);
            ble_sts_t ret = client->timeoutRecord.cb(client->timeoutRecord.connHandle, client->timeoutRecord.rangingCounter, client->timeoutRecord.type);
            // Clear entry after timeout hits
            memset(&client->timeoutRecord, 0, sizeof(blt_rasc_timeout_record_t));
            return ret;
        }
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rasc_checkLocalDataTimeout(u16 connHandle)
{
    blc_rasc_client_t *client     = blc_rasc_getClientInst(connHandle);
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);

    if ((!client) || (!rasDataset)) {
        goto failed;
    }
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    if (dataCtrl->storedNum == 0) {
        // nothing to do yet
        return BLE_SUCCESS;
    }

    for (u8 k = 0; k != dataCtrl->storedNum; k++) {
        blt_ras_proc_ctrl_t *procCtrl   = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k]);
        u32                  delta_time = stimer_get_tick() - procCtrl->timestamp;
        if ((procCtrl->timestamp != 0) && (delta_time > PROCEDURE_DATA_TIMEOUT)) { // timestamp = 0 means timeout disabled
            BLC_RAS_LOG("Client: Timeout hit for Data connh:%d, RangCtr:%d, timestamp:%lu curr_time:%lu",
                        connHandle,
                        procCtrl->rangingCounter,
                        procCtrl->timestamp,
                        stimer_get_tick());
            procCtrl->timestamp = 0; //disable
            //send an event to the app, that the local procedure data is already old
            blc_ras_timeout_evt_t timeoutEvent;
            timeoutEvent.connHandle     = connHandle;
            timeoutEvent.rangingCounter = procCtrl->rangingCounter;
            timeoutEvent.type           = RAS_TIMER_LOCAL_DATA;
            blt_prf_sendEvent(connHandle, CS_EVT_TIMEOUT, &timeoutEvent, sizeof(blc_ras_timeout_evt_t));
            return BLE_SUCCESS;
        }
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

// static ble_sts_t blt_rasc_clearLocalDataTimeout(u16 connHandle, u16 rangingCounter)
// {
//  blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
//  blt_ras_dataset_t* rasDataset = blc_ras_getDataset(connHandle);

//  if((!client) || (!rasDataset)) {
//      goto failed;
//  }

//  blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

//  for(u8 k=0; k!= RAS_PROCEDURE_COUNT; k++) {
//      blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k]);
//      if(procCtrl->rangingCounter == rangingCounter) {
//          procCtrl->timestamp = 0;
//          BLC_RAS_LOG("Client: Timeout disabled for conn:%x rangCtr:%d", connHandle, rangingCounter);
//          return BLE_SUCCESS;
//      }
//  }
//  BLC_RAS_LOG("FAILED to remove timeout for for conn:%x rangCtr:%d. Not Found", connHandle, rangingCounter);
//  return HCI_ERR_INVALID_HCI_CMD_PARAMS;
// failed:
//  return HCI_ERR_UNKNOWN_CONN_ID;
// }

static ble_sts_t blt_rasc_ondemandDataTimeout(u16 connHandle, u16 rangingCounter, u8 /*blc_ras_timer_type_enum*/ type)
{
    BLC_RAS_LOG("# # # onDemandDataTimeout hit!");
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    blt_rasc_setRequestedRangingCounter(client, RAS_INVALID_INDEX_PROCEDURE);
    blt_rasc_writeAbortOperation(connHandle, NULL);
    blc_ras_timeout_evt_t timeoutEvent;
    timeoutEvent.connHandle     = connHandle;
    timeoutEvent.rangingCounter = rangingCounter;
    timeoutEvent.type           = type;
    blt_prf_sendEvent(connHandle, CS_EVT_TIMEOUT, &timeoutEvent, sizeof(blc_ras_timeout_evt_t));
    blt_rasc_finishRecvRangingData(client);
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rasc_rangingDataReadyTimeout(u16 connHandle, u16 rangingCounter, u8 /*blc_ras_timer_type_enum*/ type) // RAS 4.4.3.1 (first paragraph only, Telink implements indications)
{
    BLC_RAS_LOG("# # # rangingDataReadyTimeout hit!");
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    blc_ras_timeout_evt_t timeoutEvent;
    timeoutEvent.connHandle     = connHandle;
    timeoutEvent.rangingCounter = rangingCounter;
    timeoutEvent.type           = type;
    blt_prf_sendEvent(connHandle, CS_EVT_TIMEOUT, &timeoutEvent, sizeof(blc_ras_timeout_evt_t));
    blt_rasc_finishRecvRangingData(client);
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rasc_realtimeDataTimeout(u16 connHandle, u16 rangingCounter, u8 /*blc_ras_timer_type_enum*/ type) // RAS 4.4.1.1
{
    BLC_RAS_LOG("# # # realTimeDataTimeout hit!");
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    blc_ras_timeout_evt_t timeoutEvent;
    timeoutEvent.connHandle     = connHandle;
    timeoutEvent.rangingCounter = rangingCounter;
    timeoutEvent.type           = type;
    blt_prf_sendEvent(connHandle, CS_EVT_TIMEOUT, &timeoutEvent, sizeof(blc_ras_timeout_evt_t));
    blt_rasc_setCccValue(connHandle, client->realtimeProcedureCccHandle, 0, NULL);
    blt_rasc_finishRecvRangingData(client);
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}
#endif

static void blt_rasc_insertSegmentAtEnd(blc_rasc_client_t *client, u8 *pData, u8 dataLen, u8 index)
{
    blt_rasc_segment_node_t *newNode = (blt_rasc_segment_node_t *)malloc_nonreten(sizeof(blt_rasc_segment_node_t));
    if (newNode == NULL) {
        BLC_RAS_LOG("ERROR! Out of memory spot 011");
        debugwait();
        return;
    }
    newNode->pData = (u8 *)malloc_nonreten(dataLen);
    if (newNode->pData == NULL) {
        #if (LL_CS_SNIFFER_MODE_ENABLE)
            if (newNode != NULL) {
                free_nonreten(newNode);
                tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_rasc_insertSegmentAtEnd, snewNode->pData==NULL", 0);
            }
        #endif

        BLC_RAS_LOG("ERROR! Out of memory spot 011-2");
        debugwait();
        return;
    }

    blt_rasc_segment_node_t *last = client->rang_data.proc_data.head;
    newNode->index                = index;
    memcpy(newNode->pData, pData, dataLen);
    newNode->dataLen = dataLen;
    newNode->next    = NULL;

    if (client->rang_data.proc_data.head == NULL) {
        client->rang_data.proc_data.head           = newNode;
        client->rang_data.proc_data.rangingDataLen = dataLen; //it is = and not +=
        return;
    }

    while (last->next != NULL) {
        last = last->next;
    }
    last->next = newNode;

    client->rang_data.proc_data.rangingDataLen += dataLen;
}

ble_sts_t blt_rasc_insertSegmentByIndex(blc_rasc_client_t *client, u8 *pData, u8 dataLen, u8 index)
{
    blt_rasc_segment_node_t *newNode = (blt_rasc_segment_node_t *)malloc_nonreten(sizeof(blt_rasc_segment_node_t));
    if (newNode == NULL) {
        BLC_RAS_LOG("ERROR! Out of memory spot 012");
        debugwait();
        goto failed;
    }
    newNode->pData = (u8 *)malloc_nonreten(dataLen);
    if (newNode->pData == NULL) {
        #if (LL_CS_SNIFFER_MODE_ENABLE)
            if (newNode != NULL) {
                free_nonreten(newNode);
                tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_rasc_insertSegmentByIndex, newNode->pData==NULL", 0);
            }
        #endif

        BLC_RAS_LOG("ERROR! Out of memory spot 012-2");
        debugwait();
        goto failed;
    }

    blt_rasc_segment_node_t *curr = client->rang_data.proc_data.head;
    newNode->index                = index;
    memcpy(newNode->pData, pData, dataLen);
    newNode->dataLen = dataLen;
    newNode->next    = NULL;

    if (client->rang_data.proc_data.head == NULL) {
        client->rang_data.proc_data.head           = newNode;
        client->rang_data.proc_data.rangingDataLen = dataLen; //it is = and not +=
        return BLE_SUCCESS;
    }

    while (curr->next != NULL) {
        //      BLC_RAS_LOG("curr %x, curr->next %x, index %d curr->next->index %d", curr, curr->next, index, curr->next->index);debugwait();
        if (curr->next->index == index) { //if index already in place, we can skip adding it, just release memory
            BLC_RAS_LOG("Index already present, release %d", index);
            debugwait();
            free_nonreten(newNode->pData);
            free_nonreten(newNode);
            return BLE_SUCCESS;
        }
        if (curr->next->index > index) {
            break;
        }
        curr = curr->next;
    }

    newNode->next = curr->next;
    curr->next    = newNode;

    client->rang_data.proc_data.rangingDataLen += dataLen;
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static void blt_rasc_clearSegmentList(blc_rasc_client_t *client)
{
    blt_rasc_segment_node_t *curr = client->rang_data.proc_data.head;
    blt_rasc_segment_node_t *next;

    while (curr != NULL) {
        next = curr->next;
        if (curr->pData != NULL) {
            free_nonreten(curr->pData);
            curr->pData   = NULL;
            curr->dataLen = 0;
            curr->index   = 0;
        }

        free_nonreten(curr);
        curr = next;
    }

    client->rang_data.proc_data.head = NULL;
}

static void blt_rasc_clearMergedData(blc_rasc_client_t *client)
{
    if (client->rang_data.proc_data.rangingData != NULL) {
        free_nonreten(client->rang_data.proc_data.rangingData);
        client->rang_data.proc_data.rangingData = NULL;
    }
    // client->rang_data.proc_data.rangingDataLen = 0; //still useful - it never gets cleared, but it gets set to initial value when head is initialised
}

static u8 *blt_rasc_mergeSegmentList(blc_rasc_client_t *client)
{
    u8 *mergedData = (u8 *)malloc_nonreten(client->rang_data.proc_data.rangingDataLen);
    if (mergedData == NULL) {
        BLC_RAS_LOG("ERROR! Out of memory spot 013");
        debugwait();
        return NULL;
    }
    u8 *writePtr = mergedData;

    blt_rasc_segment_node_t *curr = client->rang_data.proc_data.head;

    while (curr != NULL) {
        // BLC_RAS_LOG("curr %x, writePtr %x, dataLen %d, curr->pData %x", curr, writePtr, curr->dataLen, curr->pData);debugwait();
        memcpy(writePtr, curr->pData, curr->dataLen);
        writePtr += curr->dataLen;
        curr = curr->next;
    }

    return mergedData;
}

/**
 * @brief       register ranging profile client controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_rap_registerRasProfileControlClient(const blc_rasc_regParam_t *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&ras_client_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *)&ras_client_ctrl, param);
#endif
}

/**
 * @brief       ranging profile client initial function.
 * @param[in]   initType: only PRF_PROC_INIT.
 * @param[in]   param: initial parameter.
 * @return      0.
 */
static int blt_rasc_init(u8 initType, const void *param)
{
    (void)param;
    if (initType == PRF_PROC_INIT) {
        BLC_RAS_LOG("client init");
    }
    return BLE_SUCCESS;
}

/**
 * @brief       ranging profile client connect/disconnect event callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   connState: PRF_ACL_STATE_DISCONN/PRF_ACL_STATE_CONNECT.
 * @return      0.
 */
static int blt_rasc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);
    if (idx < 0) {
        goto failed;
    }

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLC_RAS_LOG("Disconnect:0x%x", connHandle);
        blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
        if (client == NULL) {
            goto failed;
        }
        blt_rasc_clearAndInitializeRemote(client);
        memset(client, 0, sizeof(blc_rasc_client_t));
        free_nonreten(client);
        ras_client_ctrl.pRasPrfClient[idx] = NULL;

        blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
        if (rasDataset == NULL) {
            goto failed;
        }
        blt_ras_clearAndInitializeLocal(rasDataset);
        memset(rasDataset, 0, sizeof(blt_ras_dataset_t));
        free_nonreten(rasDataset);
        blc_ras_writeDataset(idx, NULL);

    } else {
        BLC_RAS_LOG("Connect:0x%x", connHandle);
        blc_rasc_client_t *client = ras_client_ctrl.pRasPrfClient[idx];
        if (client != NULL) {
            blt_rasc_clearAndInitializeRemote(client);
        }
        ras_client_ctrl.pRasPrfClient[idx] = malloc_nonreten(sizeof(blc_rasc_client_t));
        client                             = ras_client_ctrl.pRasPrfClient[idx];
        if (client == NULL) {
            goto failed;
        }
        memset(client, 0, sizeof(blc_rasc_client_t));


        blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
        if (rasDataset != NULL) {
            blt_ras_clearAndInitializeLocal(rasDataset); //no persistence between connections
        }
        rasDataset = malloc_nonreten(sizeof(blt_ras_dataset_t));
        if (rasDataset == NULL) {
            goto failedconn;
        }
        blc_ras_writeDataset(idx, rasDataset);
        memset(rasDataset, 0, sizeof(blt_ras_dataset_t));
        blt_ras_initFilterDefault(rasDataset); //TODO: To be changed if we store filter with bonding info
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
failedconn:
    return HCI_ERR_CONN_FAILED_TO_ESTABLISH;
}

/**
 * @brief       ranging profile client SDP discovery function.
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
static int blt_rasc_discovery(u16 connHandle)
{
    if (blc_prf_checkDiscoveryBusy(connHandle)) {
        return BLE_SUCCESS;
    }
    if (blc_prf_checkReconnectFlag(connHandle)) {
        blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
        if (!client) {
            goto failed;
        }

        if (client->ntfInput.startHdl) {
            if (BLC_RAS_START_RECONN(connHandle) == BLE_SUCCESS) {
                blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CS_RAS_CLIENT, client->ntfInput.startHdl, client->ntfInput.endHdl);
                blc_prf_setDiscoveryStatusBusy(connHandle);
                BLC_RAS_LOG("reconnect handle: 0x%x", connHandle);
            }
        } else {
            blc_prf_sendServiceDiscoveryFailEvent(connHandle, CS_RAS_CLIENT);
            blc_prf_setDiscoveryStatusFinish(connHandle);
        }
        return BLE_SUCCESS;
    }

    if (BLC_RAS_START_SDP(connHandle) == BLE_SUCCESS) {
        blc_prf_setDiscoveryStatusBusy(connHandle);
        BLC_RAS_LOG("start discovery 0x%x", connHandle);
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

/**
 * @brief       ranging profile client main loop function.
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
static int blt_rasc_loop(u16 connHandle)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
#if (RAS_TIMEOUT_EN)
    blt_rasc_checkTimeout(connHandle);
    blt_rasc_checkLocalDataTimeout(connHandle);
#endif

    if (client->asyncSegmentLostFlag) {
        client->asyncSegmentLostFlag = FALSE;
#if (RAS_LOGIC_MANUAL)
        blc_ras_lost_segments_evt_t lostSegmentsEvent;
        lostSegmentsEvent.connHandle = connHandle;
        lostSegmentsEvent.rangingCounter = client->requestedRangingCounter;
        lostSegmentsEvent.segmentStart = client->asyncLostSegment.segmentStartAsIndex;
        lostSegmentsEvent.segmentEnd = client->asyncLostSegment.segmentEndAsIndex;
        blt_prf_sendEvent(connHandle, CS_EVT_LOST_SEGMENTS, &lostSegmentsEvent, sizeof(blc_ras_lost_segments_evt_t));
#else
        if (client->ras_feature.getLostProcedureDataSegmentsSupport) {
            u8 segmentCount = client->rang_data.proc_data.segmentCount;
            if (0 == segmentCount) {
                segmentCount = RAS_LOST_SEGMENT_WILDCARD;
            }

            u16 rangingCounter = client->asyncLostSegment.rangingCounter;
            // u8 segmentStartAsRoll = blt_ras_indexToRollingSegment(client->asyncLostSegment.segmentStartAsIndex, segmentCount); //ES-26224, ES-26235
            // u8 segmentEndAsRoll = blt_ras_indexToRollingSegment(client->asyncLostSegment.segmentEndAsIndex, segmentCount); //ES-26224, ES-26235

            client->rasCPState = RAS_CP_STATE_EXPECTED_LOST_RESPONSE;
            // int ret = blt_rasc_writeGetRecordSegments(connHandle, rangingCounter, segmentStartAsRoll, segmentEndAsRoll, NULL); //ES-26224, ES-26235
            ble_sts_t ret = blt_rasc_writeGetRecordSegments(connHandle, rangingCounter, client->asyncLostSegment.segmentStartAsIndex, client->asyncLostSegment.segmentEndAsIndex, NULL);
            BLC_RAS_LOG("msg (async) Get Lost Procedure Data cmd:rangingCounter<%d>,segmentStartAsRoll<%d>,segmentEndAsRoll<%d>,ret<%d>", rangingCounter, client->asyncLostSegment.segmentStartAsIndex, client->asyncLostSegment.segmentEndAsIndex, ret);
            return ret;
        } else {
            BLC_RAS_LOG("msg (async) Get Lost NOT SUPPORTED");
            return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        }
#endif
    }
    if(client->asyncDataReady) {
        client->asyncDataReady = FALSE;
#if (RAS_LOGIC_MANUAL)
        blc_ras_data_ready_evt_t dataReadyEvent;
        dataReadyEvent.connHandle = connHandle;
        dataReadyEvent.rangingCounter = client->requestedRangingCounter;
        blt_prf_sendEvent(connHandle, CS_EVT_DATA_READY, &dataReadyEvent, sizeof(blc_ras_data_ready_evt_t));
#else
        client->rasCPState = RAS_CP_STATE_EXPECTED_GET_RESPONSE;
        u16 rangingCounter = client->requestedRangingCounter;
        return blt_rasc_writeGetSpecificRecord(connHandle, rangingCounter, NULL);//one procedure
#endif
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static int blt_rasc_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    if (nvState == PRF_NV_STATE_STORE) {
        if (client && client->ntfInput.startHdl) {
            blt_rasc_nv_info_t nvInfo;

            blt_prf_storeClientHdl(&nvInfo.att, client, &client->rangingDataOverwrittenCccHandle);
            nvInfo.realtimeProcedureDataProperties = client->realtimeProcedureDataProperties;
            nvInfo.ondemandProcedureDataProperties = client->ondemandProcedureDataProperties;
            nvInfo.rasControlPointProperties = client->rasControlPointProperties;
            nvInfo.rangingDataReadyProperties = client->rangingDataReadyProperties;
            nvInfo.rangingDataOverwrittenProperties = client->rangingDataOverwrittenProperties;
#if(RAS_PERSISTENT_FILTER)
            memcpy(&nvInfo.filter, (void *)blt_ras_getFilter(connHandle), sizeof(blt_ras_filter_t));
#endif
            U8_TO_STREAM(param->dataPtr, sizeof(blt_rasc_nv_info_t));
            U8_TO_STREAM(param->dataPtr, CS_RAS_CLIENT);
            STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(blt_rasc_nv_info_t));
            param->currentTotalLen += 2 + sizeof(blt_rasc_nv_info_t);
        }
    } else if (nvState == PRF_NV_STATE_LOAD) {
        blt_rasc_nv_info_t *nvInfo = (blt_rasc_nv_info_t *) param->dataPtr;

        blt_prf_loadClientHdl(client, &nvInfo->att, &client->rangingDataOverwrittenCccHandle);
        client->realtimeProcedureDataProperties = nvInfo->realtimeProcedureDataProperties;
        client->ondemandProcedureDataProperties = nvInfo->ondemandProcedureDataProperties;
        client->rasControlPointProperties = nvInfo->rasControlPointProperties;
        client->rangingDataReadyProperties = nvInfo->rangingDataReadyProperties;
        client->rangingDataOverwrittenProperties = nvInfo->rangingDataOverwrittenProperties;
#if(RAS_PERSISTENT_FILTER)
        memcpy((void *)blt_ras_getFilter(connHandle), &nvInfo->filter, sizeof(blt_ras_filter_t));
#endif
        client->ntfInput.ntfOrIndFunc = blt_rasc_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
    }

    return BLE_SUCCESS;
}

/**
 * @brief       informs RAS client that new CS procedure has started
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
ble_sts_t blt_rasc_newProcedure(u16 connHandle)
{
#if (RAS_TIMEOUT_EN)
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    if (client->ondemandProcedureCccValue) {
        BLC_RAS_LOG("Setting timeout for ranging data ready."); // On demand Mode
        u16 activeRangingCounter = blt_rasc_getRequestedRangingCounter(client);
        blt_rasc_addTimeout(connHandle, activeRangingCounter, RAS_TIMER_ONDEMAND_DATAREADY, RANGING_DATA_READY_TIMEOUT, blt_rasc_rangingDataReadyTimeout);
        return BLE_SUCCESS;
    }

    if (client->realtimeProcedureCccValue) {
        BLC_RAS_LOG("Setting timeout for realtime."); // realtime mode
        blt_rasc_addTimeout(connHandle, RAS_INVALID_INDEX_PROCEDURE, RAS_TIMER_REALTIME_DATA, REALTIME_DATA_TIMEOUT, blt_rasc_realtimeDataTimeout);
        return BLE_SUCCESS;
    }
    BLC_RAS_LOG("CS started with no timeout set. Connh:%02x", connHandle);
failed:
#endif
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static void blt_rasc_setRequestedRangingCounter(blc_rasc_client_t *client, u16 rangingCounter)
{
    client->requestedRangingCounter = rangingCounter;
}

static u16 blt_rasc_getRequestedRangingCounter(blc_rasc_client_t *client)
{
    return client->requestedRangingCounter;
}

static bool blt_rasc_checkRecvProcedureDataState(blc_rasc_client_t *client, blt_rasc_recv_state_enum state)
{
    if (client->recvState == RASC_RECV_STATE_NULL) {
        client->recvState = state;
        return true;
    }
    return client->recvState == state;
}

static void blt_rasc_recordLostSegmentInfo(blc_rasc_client_t *client, u16 startSegment, u16 consecutiveLostSegmCountWildcard)
{
    blt_rasc_record_lost_segment_t *segmentLost = (blt_rasc_record_lost_segment_t *)(&client->rang_data.proc_data.ras_segment);

    u8  lostSegmentEntriesCount = segmentLost->lostSegmentEntriesCount;
    u16 endSegment              = 0;

    if (segmentLost->lostSegmentEntriesCount == RAS_LOST_SEGMENT_RECORDS_COUNT) {
        segmentLost->lostSegmentEntriesCount = RAS_LOST_SEGMENT_RECORDS_COUNT - 1;
        lostSegmentEntriesCount              = segmentLost->lostSegmentEntriesCount;
        BLC_RAS_LOG("recordLostSegmentInfo: unable to fit next lost segment entry lostSegmentEntriesCount<%d>", lostSegmentEntriesCount);
        debugwait();
        //TODO: Do we want some flag to inform upper layers?, currently we will update the last segment to stretch it to include all, even if there were some successful
    } else {
        segmentLost->segment[lostSegmentEntriesCount].segmentStartAsIndex = startSegment;
        segmentLost->lostSegmentEntriesCount++;
    }

    if (RAS_LOST_SEGMENT_WILDCARD == consecutiveLostSegmCountWildcard) {
        endSegment = RAS_LOST_SEGMENT_WILDCARD;
    } else {
        endSegment = startSegment + consecutiveLostSegmCountWildcard - 1;
    }

    segmentLost->segment[lostSegmentEntriesCount].segmentEndAsIndex = endSegment;
    segmentLost->segment[lostSegmentEntriesCount].rangingCounter    = client->rang_data.proc_data.rangingCounter;
    BLC_RAS_LOG("recordLostSegmentInfo: rangingCounter: %d, lostSegmentEntriesCount: %d, lostSegmentIndex %d, startSegment: %d,endSegment: %d", segmentLost->segment[lostSegmentEntriesCount].rangingCounter, segmentLost->lostSegmentEntriesCount, lostSegmentEntriesCount, startSegment, endSegment);
    debugwait();
}

static u32 blt_rasc_checkRealTimeSegmentationHeader(blc_rasc_client_t *client, u16 connHandle, blt_ras_segmentation_ranging_data_t *rangingData, u8 dataLen)
{
    (void)connHandle;

    if (rangingData->header.data.firstSegment && rangingData->header.data.segmentCounter) {
        BLC_RAS_LOG("firstSegment && segmentCounter");
        return 0;
    }
    //first segment contains the procedure header - ranging counter information
    if (rangingData->header.data.firstSegment) {
        client->rang_data.proc_data.realtimeDataLost = FALSE;
        client->rang_data.proc_data.rangingCounter   = blc_ras_extractRangingCounter(rangingData->data);
    }

    u16 segmentCountAsIndex = client->rang_data.proc_data.expectedSegmentAsIndex;

    BLC_RAS_LOG("checkRealTimeSegmentationHeader: raw: %d, segmentCounter: %d, segmentCountAsIndex %d", rangingData->header.raw, rangingData->header.data.segmentCounter, segmentCountAsIndex);
    debugwait();
    if (rangingData->header.data.segmentCounter != (segmentCountAsIndex & 0x3F)) {
        u8 consecutiveLostSegmCount = (rangingData->header.data.segmentCounter + 64 - (segmentCountAsIndex & 0x3F)) % 64;
        segmentCountAsIndex += consecutiveLostSegmCount;
        //When data segments get lost in realtime we mark this flag. It is up to the application to decide what to do with it ie. decode partially
        client->rang_data.proc_data.realtimeDataLost = TRUE;
    }

    blt_rasc_insertSegmentAtEnd(client, rangingData->data, dataLen, segmentCountAsIndex);

    segmentCountAsIndex++;
    client->rang_data.proc_data.expectedSegmentAsIndex = segmentCountAsIndex; //we dont mask it with 0x3F, as we want to store memory offset for MTU * segmentCountAsIndex
    return rangingData->header.data.lastSegment;
}

static u32 blt_rasc_checkOnDemandSegmentationHeader(blc_rasc_client_t *client, u16 connHandle, u8 lost, blt_ras_segmentation_ranging_data_t *rangingData, u8 dataLen)
{
    if (rangingData->header.data.firstSegment && rangingData->header.data.segmentCounter) {
        BLC_RAS_LOG("firstSegment && segmentCounter");
        return 0;
    }

    if (lost) {
        if (client->rang_data.lost_data_ctl.lostSegmentsFlag == 0) {
            return 0;
        }
    }

    u16 activeRangingCounter = blt_rasc_getRequestedRangingCounter(client);

    if (rangingData->header.data.firstSegment) {
        if (activeRangingCounter != client->rang_data.proc_data.rangingCounter) { //procedure counter mismatch
            BLC_RAS_LOG("checkOnDemandSegmentationHeader: rangingCounter mismatch: %d activeRangingCounter: %d", client->rang_data.proc_data.rangingCounter, activeRangingCounter);
        }
        u16 recvRangingCounter = blc_ras_extractRangingCounter(rangingData->data);
        if (recvRangingCounter != client->rang_data.proc_data.rangingCounter) {
            BLC_RAS_LOG("checkOnDemandSegmentationHeader: rangingCounter mismatch: %d recvRangingCounter: %d", client->rang_data.proc_data.rangingCounter, recvRangingCounter);
        }
        client->rang_data.proc_data.finalSegmentReceived = FALSE;
    }

    u16 segmentCountAsIndex = 0;
    if (lost) {
        segmentCountAsIndex = client->rang_data.lost_data_ctl.expectedSegmentAsIndex;
    } else {
        segmentCountAsIndex = client->rang_data.proc_data.expectedSegmentAsIndex;
    }

    BLC_RAS_LOG("checkOnDemandSegmentationHeader: lost: %d raw: %d, segmentCounter: %d, segmentCountAsIndex %d", lost, rangingData->header.raw, rangingData->header.data.segmentCounter, segmentCountAsIndex);
    debugwait();
    if (rangingData->header.data.segmentCounter != (segmentCountAsIndex & 0x3F)) {
        BLC_RAS_LOG("Segment lost! Expected: %d, received %d", segmentCountAsIndex, rangingData->header.data.segmentCounter);
        u8 consecutiveLostSegmCount = (rangingData->header.data.segmentCounter + 64 - (segmentCountAsIndex & 0x3F)) % 64;
        blt_rasc_recordLostSegmentInfo(client, segmentCountAsIndex, consecutiveLostSegmCount);
        segmentCountAsIndex += consecutiveLostSegmCount;
    }

    blt_rasc_insertSegmentByIndex(client, rangingData->data, dataLen, segmentCountAsIndex);
    BLC_RAS_LOG("TotalLen: %d DataLen: %d", client->rang_data.proc_data.rangingDataLen, dataLen);

    #if (RAS_TIMEOUT_EN)
        // segment recieved, reset timeout
        blt_rasc_updateTimeout(connHandle, RAS_TIMER_ONDEMAND_DATA, ON_DEMAND_DATA_TIMEOUT_INTERVAL_CONTINUE);
    #endif
    segmentCountAsIndex++;

    if (rangingData->header.data.lastSegment) {
        client->rang_data.proc_data.finalSegmentReceived = TRUE;
        #if(TTF_EN)
            TTF_LOG("cli_lastsegm.");
        #endif
        //count is one over the final current index, which contains even numbers above ones allowed by rolling segment - adding the 0th segment
        client->rang_data.proc_data.segmentCount = rangingData->header.data.segmentCounter + 1;

        #if (LL_CS_SNIFFER_MODE_ENABLE)
            tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_rasc_checkOnDemandSegmentationHeader", client->rang_data.proc_data.rangingCounter);
        #endif

        #if(RAS_TIMEOUT_EN)
            // last segment recieved, stop timeout
            blt_rasc_deleteTimeout(connHandle);
        #endif

        BLC_RAS_LOG("recv one procedure data end: segmentCount: %d", client->rang_data.proc_data.segmentCount);debugwait();
    }

    if (lost) {
        client->rang_data.lost_data_ctl.expectedSegmentAsIndex = segmentCountAsIndex; //we dont mask it with 0x3F, as we want to store memory offset for MTU * segmentCountAsIndex
        //check if the next - expected segment would be the one after the segment ends - it means we completed this lost flag entry handling
        BLC_RAS_LOG("Lost segment completed when equal expectedSegmentAsIndex: %d segmentEndAsIndex + 1: %d", client->rang_data.lost_data_ctl.expectedSegmentAsIndex, client->rang_data.lost_data_ctl.segmentEndAsIndex + 1);
        //for wildcard end segment -> return when lastSegment is detected
        if (client->rang_data.lost_data_ctl.segmentEndAsIndex == RAS_LOST_SEGMENT_WILDCARD) {
            return rangingData->header.data.lastSegment;
        }
        return ((client->rang_data.lost_data_ctl.expectedSegmentAsIndex & 0x3F) == ((client->rang_data.lost_data_ctl.segmentEndAsIndex + 1) & 0x3F ));
    } //else
    client->rang_data.proc_data.expectedSegmentAsIndex = segmentCountAsIndex; //we dont mask it with 0x3F, as we want to store memory offset for MTU * segmentCountAsIndex
    return rangingData->header.data.lastSegment;
}

static void blt_rasc_finishRecvRangingData(blc_rasc_client_t *client)
{
    if (client->recvState == RASC_RECV_STATE_REALTIME_DATA) {
    } else if (client->recvState == RASC_RECV_STATE_ONDEMAND_DATA) {
    } else {
    }
    client->recvState = RASC_RECV_STATE_NULL;
}

/**
 * @brief       ranging profile client receive server report notify data.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   attHdl: notify Attribute handle.
 * @param[in]   val: notify value pointer.
 * @param[in]   valLen: value length.
 * @return      none.
 */
static void blt_rasc_recvRealTimeProcedureData(u16 connHandle, u8 *val, u16 valLen)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    if (client->recvState == RASC_RECV_STATE_NULL) {
        blt_rasc_clearAndInitializeRemote(client);
    }
    if (!blt_rasc_checkRecvProcedureDataState(client, RASC_RECV_STATE_REALTIME_DATA)) {
        BLC_RAS_LOG("connHandle is 0x%x Incorrect receiving real-time procedure data, receive state is %d", connHandle, client->recvState);
        return;
    }

#if (TTF_EN)
    TTF_LOG("realtime received."); // mark realtime data reception for TTF
#endif

#if (RAS_TIMEOUT_EN)
        // segment recieved at this point, reset timeout
    blt_rasc_updateTimeout(connHandle, RAS_TIMER_REALTIME_DATA, REALTIME_DATA_TIMEOUT_CONTINUE);
#endif
    if (blt_rasc_checkRealTimeSegmentationHeader(client, connHandle, (blt_ras_segmentation_ranging_data_t *)val, valLen - sizeof(blt_ras_segmentation_header_t))) //-1
    {
#if (RAS_TIMEOUT_EN)
        // last segment recieved, timeout can be deleted
        blt_rasc_deleteTimeout(connHandle);
#endif
#if (TTF_EN)
        TTF_LOG("realtime_end.");
        debugwait();
#endif
        //all segments for a procedure received
        /*double check remote procedure buffer weather is clear or not*/
        blt_rasc_clearMergedData(client);
        client->rang_data.proc_data.rangingData = blt_rasc_mergeSegmentList(client);
        u16  rangingCounter = client->rang_data.proc_data.rangingCounter;
        blt_rasc_clearSegmentList(client);

        blt_rasc_setRemoteDataReady(connHandle);
        #if (LL_CS_SNIFFER_MODE_ENABLE)
            tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_rasc_recvRealTimeProcedureData", rangingCounter, client->remoteDataReady, client->localDataReady, client->localReadyCnt);
        #endif
        blt_rasc_issueDataReadyAppEvent(connHandle);
        blt_rasc_finishRecvRangingData(client);
    }
    BLC_RAS_LOG("recv Real-time Procedure Data");
    return;
failed:
    return;
}

static void blt_rasc_recvOnDemandProcedureData(u16 connHandle, u8 *val, u16 valLen)
{
    BLC_RAS_LOG("recv On-Demand Procedure Data");
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

#if (RAS_PTS_LOST_SEGMENT_WORKAROUND)
    blt_ras_segmentation_ranging_data_t *rangingData = (blt_ras_segmentation_ranging_data_t *)val;
    if (segmentsToSkipCount) {
        if ((rangingData->header.data.segmentCounter == 1) ||
           (rangingData->header.data.segmentCounter == 2) ||
           (rangingData->header.data.segmentCounter == 4) ||
           (rangingData->header.data.lastSegment)) {
            BLC_RAS_LOG("PTS - Skipped segment %x", rangingData->header.raw);
            segmentsToSkipCount--;
            return;
        }
    }
#endif

#if (TTF_EN)
    ttf_log_buffer_with_label(val, valLen, "TTFdata:");
    TTF_LOG("TTFdata_end.");
    debugwait();
#endif
    if (!blt_rasc_checkRecvProcedureDataState(client, RASC_RECV_STATE_ONDEMAND_DATA)) {
        BLC_RAS_LOG("connHandle: 0x%x Incorrect receiving On-Demand Procedure Data, receive state is %d", connHandle, client->recvState);
        return;
    }

    if (client->rang_data.lost_data_ctl.lostSegmentsFlag) {
        u32 complt = blt_rasc_checkOnDemandSegmentationHeader(client, connHandle, TRUE, (blt_ras_segmentation_ranging_data_t *)val, valLen - sizeof(blt_ras_segmentation_header_t));
        if (complt) {
            BLC_RAS_LOG("recv lost segment data end,segmentStartAsIndex<%d>,segmentEndAsIndex<%d>", client->rang_data.lost_data_ctl.segmentStartAsIndex, client->rang_data.lost_data_ctl.segmentEndAsIndex);
            client->rang_data.lost_data_ctl.lostSegmentsFlag = 0; //finished
            client->rang_data.proc_data.ras_segment.lostSegmentEntriesCount--;
        }
        return;                                                   //TODO we need to have info on last segment flag also from LOST case ...
    } else {
        BLC_RAS_LOG("blt_rasc_checkOnDemandSegmentationHeader");
        blt_rasc_checkOnDemandSegmentationHeader(client, connHandle, FALSE, (blt_ras_segmentation_ranging_data_t *)val, valLen - sizeof(blt_ras_segmentation_header_t));
    }
    return;
failed:
    return;
}

static u8 blt_rasc_checkOnDemandProcedureSegment(blc_rasc_client_t *client, u16 connHandle)
{
    (void)connHandle;
    //When here, he should have the final segment, if not, then it got lost, if lost then use WILDCARD
    if (!client->rang_data.proc_data.finalSegmentReceived) {
        u8 lostCount = client->rang_data.proc_data.ras_segment.lostSegmentEntriesCount;
        BLC_RAS_LOG("checkOnDemandProcedureSegment: WILDCARD needed expectedSegmentAsIndex: %d, lostCount: %d", client->rang_data.proc_data.expectedSegmentAsIndex, lostCount);
        debugwait();
        if (client->rang_data.proc_data.expectedSegmentAsIndex > 0) {
            blt_rasc_recordLostSegmentInfo(client, client->rang_data.proc_data.expectedSegmentAsIndex - 1, RAS_LOST_SEGMENT_WILDCARD);
        }
        //this can happen only if we havent received even a single segment
        else {
            blt_rasc_recordLostSegmentInfo(client, 0, RAS_LOST_SEGMENT_WILDCARD);
        }
    }

    u8 lostSegmentEntriesCount = client->rang_data.proc_data.ras_segment.lostSegmentEntriesCount;
    if (lostSegmentEntriesCount) {
        client->rang_data.lost_data_ctl.lostSegmentsFlag = 1;
        lostSegmentEntriesCount--;
        blt_rasc_get_lost_proc_segment_t *segmentLost       = (blt_rasc_get_lost_proc_segment_t *)(&client->rang_data.proc_data.ras_segment.segment[lostSegmentEntriesCount]);
        client->rang_data.lost_data_ctl.rangingCounter      = segmentLost->rangingCounter;
        client->rang_data.lost_data_ctl.segmentStartAsIndex = segmentLost->segmentStartAsIndex;
        //final segment got lost, we modify the requested end segment to WILDCARD
        if (!client->rang_data.proc_data.finalSegmentReceived) {
            BLC_RAS_LOG("checkOnDemandProcedureSegment: WILDCARD needed segmentStartAsIndex: %d, segmentEndAsIndex: %d, lostEntriesCount: %d", segmentLost->segmentStartAsIndex, client->rang_data.proc_data.expectedSegmentAsIndex, lostSegmentEntriesCount);
            client->rang_data.lost_data_ctl.segmentEndAsIndex = RAS_LOST_SEGMENT_WILDCARD;
        } else {
            client->rang_data.lost_data_ctl.segmentEndAsIndex = segmentLost->segmentEndAsIndex;
        }
        client->rang_data.lost_data_ctl.expectedSegmentAsIndex = segmentLost->segmentStartAsIndex;
        // client->rang_data.proc_data.ras_segment.lostSegmentEntriesCount--;

        // u8 segmentCount = client->rang_data.proc_data.segmentCount;
        // if(0 == segmentCount) {
        //  segmentCount = RAS_LOST_SEGMENT_WILDCARD;
        // }
        // u8 segmentStartAsRoll = blt_ras_indexToRollingSegment(segmentLost->segmentStartAsIndex, segmentCount);
        // u8 segmentEndAsRoll = blt_ras_indexToRollingSegment(segmentLost->segmentEndAsIndex, segmentCount);

        client->asyncLostSegment.rangingCounter      = segmentLost->rangingCounter;
        client->asyncLostSegment.segmentStartAsIndex = segmentLost->segmentStartAsIndex;
        client->asyncLostSegment.segmentEndAsIndex   = segmentLost->segmentEndAsIndex;

        // client->asyncLostSegmentStartAsRoll = segmentStartAsRoll;
        // client->asyncLostSegmentEndAsRoll = segmentEndAsRoll;
        client->asyncSegmentLostFlag = TRUE;
        //  //int ret = blt_rasc_writeGetRecordSegments(connHandle, segmentLost->rangingCounter, segmentStartAsRoll, segmentEndAsRoll, NULL);
        // BLC_RAS_LOG("msg (sync) Get Lost Procedure Data cmd:rangingCounter<%d>,segmentStartAsRoll<%d>,segmentEndAsRoll<%d>,ret<%d>",segmentLost->rangingCounter, segmentStartAsRoll, segmentEndAsRoll, ret);
        BLC_RAS_LOG("msg (sync) Get Lost Procedure Data cmd");
    }
    return client->rang_data.lost_data_ctl.lostSegmentsFlag;
}

void blt_rasc_setRemoteDataReady(u16 connHandle)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    BLC_RAS_LOG("blt_rasc_setRemoteDataReady - only on central");
    debugwait();
    client->remoteDataReady = TRUE;
    return;
failed:
    return;
}

void blt_rasc_setLocalDataReady(u16 connHandle)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!client) {
        goto failed;
    }
    if (!rasDataset) {
        BLC_RAS_LOG("rasDataset is NULL");
        goto failed;
    }
    BLC_RAS_LOG("blt_rasc_setClientLocalDataReady - only on central");
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);
    client->localReadyCnt = dataCtrl->storedNum;
    if(client->localReadyCnt < 1)
    {
        client->localDataReady = FALSE;
    }else{
        client->localDataReady = TRUE;
    }
    return;
failed:
    return;
}

void blt_ras_RangingCounterQuery(u16 connHandle, blt_ras_dataset_t *rasDataset, u16 rangingCounter)
{
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    blc_rasc_client_t *client     = blc_rasc_getClientInst(connHandle);

    if(client == NULL){
        return;
    }

    if (!rasDataset) {
        return;
    }

    if (dataCtrl->storedNum < 1) {
        return;
    }

    #if (LL_CS_SNIFFER_MODE_ENABLE)
        u8 validIndexNum = 0;
        for (int i = 0; i < RAS_PROCEDURE_COUNT; i++) {
            blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
            /* checking for the existence of valid data */
            if (procCtrl->rangingCounter != RAS_INVALID_INDEX_PROCEDURE) {
                validIndexNum++;
            }
        }
        if (validIndexNum == 0) {
            tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_ras_RangingCounterQuery validIndexNum==0", dataCtrl->storedNum);
            /* delete all buffer  */
            blt_ras_clearAndInitializeLocal(rasDataset);
            client->localReadyCnt = 0;
            client->localDataReady = FALSE;
            return;
        }
    #endif

    u8 maxStoredNum;
    #if (LL_CS_SNIFFER_MODE_ENABLE)
        maxStoredNum = RAS_PROCEDURE_COUNT;
    #else
        maxStoredNum = dataCtrl->storedNum;
    #endif
    for (int i = 0; i < maxStoredNum; i++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
        if(procCtrl->rangingCounter == RAS_INVALID_INDEX_PROCEDURE){
            BLC_RAS_LOG("procedure store abnormal@storedNum:%d,i: %d,local counter: %x,remote counter:%x", dataCtrl->storedNum, i,procCtrl->rangingCounter, rangingCounter);
        }
        else if (rangingCounter != (procCtrl->rangingCounter & 0xFFF)) {
            u16 offset = ((procCtrl->rangingCounter & 0xFFF) + 4096 - rangingCounter) & 0xfff;
            if(offset > 2048){//history data
                blt_ras_procedureDeleteLocal(rasDataset,procCtrl->rangingCounter);
                #if (LL_CS_SNIFFER_MODE_ENABLE)
                    client->localReadyCnt = dataCtrl->storedNum;
                #else
                    client->localReadyCnt--;
                #endif
                if (client->localReadyCnt) {
                    client->localDataReady = TRUE;
                } else {
                    client->localDataReady = FALSE;
                    break;
                }
            }
        }
    }
}

void blt_rasc_issueDataReadyAppEvent(u16 connHandle)
{
    blc_rasc_client_t *client     = blc_rasc_getClientInst(connHandle);
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if ((!client) || (!rasDataset)) {
        goto failed;
    }

    if ((client->remoteDataReady) && (client->localDataReady)) {
        //when local and remote are all ready,but remote ranging counter is not consist with all local ranging counter ,this case can return directly.
        blt_ras_dataset_t                *rasDataset = blc_ras_getDataset(connHandle);
        blt_rass_procedure_query_result_t res        = blt_rass_procedureQuery(rasDataset, client->rang_data.proc_data.rangingCounter);
        if (res.procData == NULL) {
            #if (LL_CS_SNIFFER_MODE_ENABLE)
                tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_rasc_issueDataReadyAppEvent res.procData==NULL", client->rang_data.proc_data.rangingCounter);
                u8 counterArray[RAS_PROCEDURE_COUNT + 1] = {0};
                blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);
                counterArray[0] = dataCtrl->storedNum;
                for (int i = 0; i < RAS_PROCEDURE_COUNT; i++) {
                    blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[i]);
                    counterArray[i + 1] = procCtrl->rangingCounter;
                }
                tlkapi_send_string_data(DBG_CS_DATA_EN, "[RAS][CLI] issueDataReady error, storedNum and counterArray", counterArray, RAS_PROCEDURE_COUNT + 1);
            #endif

            client->remoteDataReady = FALSE;
            blt_ras_RangingCounterQuery(connHandle, rasDataset, client->rang_data.proc_data.rangingCounter);
            blc_rapc_clearAndInitializeRemote(connHandle);//delete remote procedure data
            return;
        }

        BLC_RAS_LOG("blt_rasc_issueDataReadyAppEvent - only on central remoteDatReady %d, localDataReady %d, pSubEvt[0] %x", client->remoteDataReady, client->localDataReady, rasDataset->dataCtrl.procCtrl->subEvtData[0].pSubEvt);
        debugwait();

        blc_rasc_ranging_data_evt_t rangingDataEvt;
        rangingDataEvt.connHandle       = connHandle;
        rangingDataEvt.rangingCounter   = client->rang_data.proc_data.rangingCounter;
        rangingDataEvt.rangingData      = client->rang_data.proc_data.rangingData;
        rangingDataEvt.rangingDataLen   = client->rang_data.proc_data.rangingDataLen;
        rangingDataEvt.realtimeDataLost = client->rang_data.proc_data.realtimeDataLost;

        #if (LL_CS_SNIFFER_MODE_ENABLE)
            //tlkapi_send_string_u8s(DBG_CS_DATA_EN, "[RAS][CLI] blt_rasc_issueDataReadyAppEvent", rangingDataEvt.rangingCounter, client->remoteDataReady, client->localDataReady, rangingDataEvt.realtimeDataLost);
        #endif

        if(rangingDataEvt.realtimeDataLost == FALSE){//only calculate completed procedure data.
            blt_prf_sendEvent(connHandle, CS_EVT_RANGING_DATA, &rangingDataEvt, sizeof(blc_rasc_ranging_data_evt_t));
        }
        else{
            BLC_RAS_LOG("realtime segment lost");
        }
        client->remoteDataReady = FALSE;

        //free history data buffer
        blt_ras_procedureDeleteLocal(rasDataset, client->rang_data.proc_data.rangingCounter);//delete local procedure data
        blc_rapc_clearAndInitializeRemote(connHandle);//delete remote procedure data
        blt_rasc_setLocalDataReady(connHandle);
    }
    else{
        //when only remote ready, no any process.
    }
    return;
failed:
    return;
}

static void blt_rasc_recvRasControlPointHandler(blc_rasc_client_t* client, u16 connHandle, blt_ras_cp_msg_t* msg)
{
    u16 rangingCounter = msg->operand.completeRecord.rangingCounter;
    BLC_RAS_LOG("recv Complete Report Records Response: Ranging Counter<%d>", rangingCounter);debugwait();

#if(RAS_IOPTEST_ENABLE)
    if(!client->iopTestingManual) {
#endif
        // u8 segmentLostFlag = blt_rasc_checkOnDemandProcedureSegment(client, connHandle);
        client->asyncSegmentLostFlag = blt_rasc_checkOnDemandProcedureSegment(client, connHandle);
        // if(segmentLostFlag == 0) {
        if (client->asyncSegmentLostFlag == FALSE) {
            //all segments for a procedure received
            blt_rasc_clearMergedData(client);
            client->rang_data.proc_data.rangingData = blt_rasc_mergeSegmentList(client);
            blt_rasc_clearSegmentList(client);

#if(RAS_DEBUG_PRINTBUFFERS)
            BLC_RAS_LOG("Merged");debugwait();
            log_buffer(client->rang_data.proc_data.rangingData, client->rang_data.proc_data.rangingDataLen);
#endif

            BLC_RAS_LOG("msg ACK Stored Records Command:rangingCounter<%d>", rangingCounter);debugwait();
            blt_rasc_setRequestedRangingCounter(client, RAS_INVALID_INDEX_PROCEDURE);

#if (RAS_LOGIC_MANUAL)
            blc_ras_procedure_ack_evt_t procedureAckEvent;
            procedureAckEvent.connHandle = connHandle;
            procedureAckEvent.rangingCounter = rangingCounter;
            blt_prf_sendEvent(connHandle, CS_EVT_PROCEDURE_ACK, &procedureAckEvent, sizeof(blc_ras_procedure_ack_evt_t));
#else
            blt_rasc_writeAckSpecificRecord(connHandle, rangingCounter, NULL);
#endif

            blt_rasc_setRemoteDataReady(connHandle);
            blt_rasc_issueDataReadyAppEvent(connHandle);

            blt_rasc_finishRecvRangingData(client);
        }
#if(RAS_IOPTEST_ENABLE)
    }
#endif
}

static void blt_rasc_recvRasControlPoint(u16 connHandle, u8* val, u16 valLen)
{
    (void)valLen;
    blt_ras_cp_msg_t  *msg    = (blt_ras_cp_msg_t *)val;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
#if (TTF_EN)
    TTF_LOG("TTF_CPOP:%d TTF_CParg:%02x%02x", msg->opcode, msg->operand.val[0], msg->operand.val[1]);
    debugwait();
#endif

    switch (client->rasCPState) {
        case RAS_CP_STATE_EXPECTED_GET_RESPONSE:
            if (msg->opcode == RAS_CP_RSP_OPCODE_COMPLETE_REPORT_RECORDS) {
                blt_rasc_recvRasControlPointHandler(client, connHandle, msg);
                client->rasCPState = RAS_CP_STATE_NULL;
            }
            break;

        case RAS_CP_STATE_EXPECTED_LOST_RESPONSE:
            if (msg->opcode == RAS_CP_RSP_OPCODE_COMPLETE_RECORD_SEGMENT) {
                //TBD: validation of response received vs requested (stored in asyncLostSegment) ?
                blt_rasc_recvRasControlPointHandler(client, connHandle, msg);
                client->rasCPState = RAS_CP_STATE_NULL;
            }
            break;
        case RAS_CP_STATE_NULL:
        default:
            break;
    }
    return;
failed:
    return;
}

static void blt_rasc_recvProcedureDataReady(u16 connHandle, u8 *val, u16 valLen)
{
    (void)valLen;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    blt_ras_dataReady_t *ready = (blt_ras_dataReady_t *)val;
#if (RAS_TIMEOUT_EN)
    // Data Ready recieved. Stop timeout RAP 4.4.3.1
    blt_rasc_deleteTimeout(connHandle);
#endif

    if (client->recvState == RASC_RECV_STATE_NULL) {
        blt_rasc_clearAndInitializeRemote(client);
    }
    client->rang_data.proc_data.rangingCounter = ready->rangingCounter;
    client->rangingDataReadyValue = ready->rangingCounter;

    BLC_RAS_LOG("recv Procedure Data Ready, connHandle<0x%x>, rangingCounter<%d>", connHandle, ready->rangingCounter);
    TTF_LOG("recv Procedure Data Ready, connHandle<0x%x>, rangingCounter<%d>", connHandle, ready->rangingCounter);

    blt_rasc_setRequestedRangingCounter(client, ready->rangingCounter);
    client->asyncDataReady = TRUE; // blt_rasc_writeGetSpecificRecord(connHandle, ready->rangingCounter, NULL);//one procedure
    return;
failed:
    return;
}

static void blt_rasc_recvRangingDataOverwritten(u16 connHandle, u8 *val, u16 valLen)
{
    (void)connHandle;
    (void)valLen;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    blt_ras_data_overwritten_t *overwrite = (blt_ras_data_overwritten_t *)val;
    BLC_RAS_LOG("recv Procedure Data Overwritten Indication, connHandle<0x%x>, rangingCounter<%d>", connHandle, overwrite->rangingCounter);

    blc_rasc_overwritten_evt_t overwrittenEvt;
    overwrittenEvt.connHandle     = connHandle;
    overwrittenEvt.rangingCounter = overwrite->rangingCounter;
    client->rangingDataOverwrittenValue = overwrite->rangingCounter;

    blt_prf_sendEvent(connHandle, CS_EVT_OVERWRITTEN, &overwrittenEvt, sizeof(blc_rasc_overwritten_evt_t));
failed:
    return;
}

static void blt_rasc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
#if (RAS_DEBUG_PRINTBUFFERS)
    BLC_RAS_LOG("recv data<connHdl:0x%X> on<attHdl:0x%X>:%s", connHandle, attHdl, hex_to_str(val, valLen));
#endif

#if ((RAS_IOPTEST_ORD_001) || (RAS_IOPTEST_RRD_003))
    static u8 iopCounter = 0;
    if (iopCounter >= 1) {
        /* IOP test ORD-001 + RRD-003 */
        BLC_RAS_LOG(" * * * iop just one!");
        blc_ll_disconnect(connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
    }
    iopCounter++;
#endif
#if (RAS_IOPTEST_ORD_PAIR_03)
    static u8 iopCounter = 0;
    if (iopCounter >= 1) {
        /* IOP test ORD-Pair-03 */
        blt_rasc_writeAbortOperation(connHandle, NULL);
    }
    iopCounter++;
#endif

    if (attHdl == client->realtimeProcedureDataHandle) {
        blt_rasc_recvRealTimeProcedureData(connHandle, val, valLen);
    } else if (attHdl == client->ondemandProcedureDataHandle) {
        blt_rasc_recvOnDemandProcedureData(connHandle, val, valLen);
    } else if (attHdl == client->rasControlPointHandle) {
        blt_rasc_recvRasControlPoint(connHandle, val, valLen);
    } else if (attHdl == client->rangingDataReadyDataHandle) {
        blt_rasc_recvProcedureDataReady(connHandle, val, valLen);
    } else if (attHdl == client->rangingDataOverwrittenDataHandle) {
        blt_rasc_recvRangingDataOverwritten(connHandle, val, valLen);
    }
    return;
failed:
    return;
}

/**
 * @brief       ranging profile client display SDP/reconnect server information.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   client: notify Attribute handle.
 * @return      none.
 */
static void blt_rasc_displayInfo(u16 connHandle, blc_rasc_client_t *client)
{
    (void)connHandle;
    (void)client;
    BLC_RAS_LOG("RAS sdp over connHandle[0x%02x]", connHandle);

    BLC_RAS_LOG("RAS Feature Handle: 0x%04x, Properties: 0x%02x , Value: 0x%08x", client->rasFeatureHandle, CHAR_PROP_READ, client->ras_feature.features);

    BLC_RAS_LOG("Real Time Procedure Data Handle: 0x%04x, Properties: 0x%02x", client->realtimeProcedureDataHandle, client->realtimeProcedureDataProperties);

    BLC_RAS_LOG("Real Time Procedure CCC Handle: 0x%04x, Value: 0x%04x", client->realtimeProcedureCccHandle, client->realtimeProcedureCccValue);

    BLC_RAS_LOG("On demand Procedure Data Handle: 0x%04x, Properties: 0x%02x", client->ondemandProcedureDataHandle, client->ondemandProcedureDataProperties);

    BLC_RAS_LOG("On demand Procedure CCC Handle: 0x%04x, Value: 0x%04x", client->ondemandProcedureCccHandle, client->ondemandProcedureCccValue);

    BLC_RAS_LOG("RAS Control Point Data Handle: 0x%04x, Properties: 0x%02x", client->rasControlPointHandle, client->rasControlPointProperties);

    BLC_RAS_LOG("RAS Control Point CCC Handle: 0x%04x, Value: 0x%04x", client->rasControlPointCccHandle, client->rasControlPointCccValue);

    BLC_RAS_LOG("Ranging Data Ready Data Handle: 0x%04x, Properties: 0x%02x", client->rangingDataReadyDataHandle, client->rangingDataReadyProperties);

    BLC_RAS_LOG("Ranging Data Ready CCC Handle: 0x%04x, Value: 0x%04x", client->rangingDataReadyCccHandle, client->rangingDataReadyCccValue);

    BLC_RAS_LOG("Ranging Data Overwritten Data Handle: 0x%04x, Properties: 0x%02x", client->rangingDataOverwrittenDataHandle, client->rangingDataOverwrittenProperties);

    BLC_RAS_LOG("Ranging Data Overwritten CCC Handle: 0x%04x, Value: 0x%04x", client->rangingDataOverwrittenCccHandle, client->rangingDataOverwrittenCccValue);
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
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, CS_RAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLC_RAS_LOG("ERR: not found RAS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, CS_RAS_CLIENT);
#if (TTF_EN)
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_SER_BV_01_C Data:%02x", 1);
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_CHA_BV_01_C Data:%02x", CHAR_PROP_READ); //rasFeatureProperties
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_CHA_BV_02_C Data:%02x", client->realtimeProcedureDataProperties);
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_CHA_BV_03_C Data:%02x", client->ondemandProcedureDataProperties);
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_CHA_BV_04_C Data:%02x", client->rasControlPointProperties);
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_CHA_BV_05_C Data:%02x", client->rangingDataReadyProperties);
        TTF_LOG("GATT_T:RAP_REQ_CGGIT_CHA_BV_06_C Data:%02x", client->rangingDataOverwrittenProperties);
#endif
        blt_rasc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_rasc_dataInput;
    BLC_RAS_LOG("INFO: RAS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, CS_RAS_CLIENT, startHandle, endHandle);
    return;
failed:
    return;
}

static void blt_rasc_rasFeatureStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);

    if (!client) {
        goto failed;
    }

    *read        = (u8 *)&client->ras_feature.features;
    *readLen     = NULL;
    *readMaxSize = sizeof(client->ras_feature.features);
    *rdCbFunc    = NULL; //rdCbFunc
    BLC_RAS_LOG("Ras feature read info");
    return;
failed:
    return;
}

static void blt_rasc_rasFeatureChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    if (properties & CHAR_PROP_READ) {
        client->rasFeatureHandle = valueHandle;
    }

    BLC_RAS_LOG("Ras Feature connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
failed:
    return;
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
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->realtimeProcedureDataHandle     = valueHandle;
    client->realtimeProcedureDataProperties = properties;

    BLC_RAS_LOG("Real-time Procedure Data connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);

    // TODO: Obtain CCC handle in other way
    client->realtimeProcedureCccHandle = valueHandle + 1;
    return;
failed:
    return;
}

static void blt_rasc_onDemandProcedureDataChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->ondemandProcedureDataHandle     = valueHandle;
    client->ondemandProcedureDataProperties = properties;

    BLC_RAS_LOG("On-Demand Procedure Data connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);

    // TODO: Obtain CCC handle in other way
    client->ondemandProcedureCccHandle = valueHandle + 1;
    return;
failed:
    return;
}

static void blt_rasc_controlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->rasControlPointHandle     = valueHandle;
    client->rasControlPointProperties = properties;

    BLC_RAS_LOG("RAS Control Point connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
    // TODO: Obtain CCC handle in other way
    client->rasControlPointCccHandle = valueHandle + 1;
    return;
failed:
    return;
}

static void blt_rasc_rangingDataReadyChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->rangingDataReadyDataHandle = valueHandle;
    client->rangingDataReadyProperties = properties;

    BLC_RAS_LOG("Ranging Data Ready connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);

    // TODO: Obtain CCC handle in other way
    client->rangingDataReadyCccHandle = valueHandle + 1;
    return;
failed:
    return;
}

static void blt_rasc_rangingDataOverwrittenChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->rangingDataOverwrittenDataHandle = valueHandle;
    client->rangingDataOverwrittenProperties = properties;

    BLC_RAS_LOG("Ranging Data Overwritten connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);

    // TODO: Obtain CCC handle in other way
    client->rangingDataOverwrittenCccHandle = valueHandle + 1;
    return;
failed:
    return;
}

ble_sts_t blt_rasc_setCccValue(u16 connHandle, u16 cccHandle, u16 value, prf_write_cb_t writeCb)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    if (cccHandle == client->realtimeProcedureCccHandle) {
        client->realtimeProcedureCccValue = value;
    } else if (cccHandle == client->ondemandProcedureCccHandle) {
        client->ondemandProcedureCccValue = value;
    } else if (cccHandle == client->rasControlPointCccHandle) {
        client->rasControlPointCccValue = value;
    } else if (cccHandle == client->rangingDataReadyCccHandle) {
        client->rangingDataReadyCccValue = value;
    } else if (cccHandle == client->rangingDataOverwrittenCccHandle) {
        client->rangingDataOverwrittenCccValue = value;
    } else {
        BLC_RAS_LOG("ERR: Invalid ATT CCC handle %d", cccHandle);
        return GATT_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;
    pGapWrCfg.func       = NULL;
    pGapWrCfg.handle     = cccHandle;
    pGapWrCfg.data       = &value;
    pGapWrCfg.length     = sizeof(value);
    pGapWrCfg.withoutRsp = false; //true;
    pGapWrCfg.cbData     = NULL;

    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static void blt_rasc_readAttributeValueCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);

    if (pRdCfg->single.handle == client->rasFeatureHandle) {
        BLC_RAS_LOG("RAS Feature read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->rangingDataReadyDataHandle) {
        BLC_RAS_LOG("Data Ready read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->rangingDataOverwrittenDataHandle) {
        BLC_RAS_LOG("Data Overwritten read callback, status: 0x%02X", err);
    }

    blc_prf_readAttributeValueCallback(connHandle, err);
}

static ble_sts_t blt_rasc_readAttributeValue(u16 connHandle, blt_rasc_read_t readType, prf_read_cb_t rdCbFunc)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        BLC_RAS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (readType >= BLT_RASC_READ_MAX) {
        BLC_RAS_LOG("ERR: Invalid read type %d", readType);
        return GATT_ERR_INVALID_PARAMETER;
    }
    gapc_read_cfg_t pGapReCfg;

    pGapReCfg.handle = 0;
    pGapReCfg.func   = blt_rasc_readAttributeValueCb;

    switch (readType) {
        case BLT_RASC_READ_RAS_FEATURE:
        {
            pGapReCfg.handle   = client->rasFeatureHandle;
            pGapReCfg.wBuff    = (u8 *)&client->ras_feature.features;
            pGapReCfg.wBuffLen = NULL;
            pGapReCfg.maxLen   = sizeof(client->ras_feature.features);
            break;
        }
        case BLT_RASC_READ_RAS_DATA_READY:
        {
            pGapReCfg.handle   = client->rangingDataReadyDataHandle;
            pGapReCfg.wBuff    = &client->rangingDataReadyValue;
            pGapReCfg.wBuffLen = NULL;
            pGapReCfg.maxLen   = sizeof(u16);
            break;
        }
        case BLT_RASC_READ_RAS_DATA_OVERWRITTEN:
        {
            pGapReCfg.handle   = client->rangingDataOverwrittenDataHandle;
            pGapReCfg.wBuff    = &client->rangingDataOverwrittenValue;
            pGapReCfg.wBuffLen = NULL;
            pGapReCfg.maxLen   = sizeof(u16);
            break;
        }
        default:
            break;
    }

    if (pGapReCfg.handle == 0) {
        BLC_RAS_LOG("ERR: handle not set");
        return PRF_ERR_INVALID_ATTR_HANDLE;
    }
    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, rdCbFunc);
}

static const blc_gapc_discService_t rasService = {
    .uuid = UUID16_INIT(SERVICE_UUID_RANGING),
    .sfun = blt_rasc_foundService,
};

static void blt_rasc_realtimeProcedureCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    (void)result;

    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->realtimeProcedureCccHandle = cccHandle;
    return;
failed:
    return;
}

static void blt_rasc_ondemandProcedureCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    (void)result;

    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    client->ondemandProcedureCccHandle = cccHandle;
    return;
failed:
    return;
}

static const blc_gapc_discChar_t rasChar[] = {
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_RAS_FEATURE),
     .cfun      = blt_rasc_rasFeatureChar,
     .rfun      = blt_rasc_rasFeatureStartRead,
     },

    {
     .uuid  = UUID16_INIT(CHARACTERISTIC_UUID_REAL_TIME_PROCEDURE_DATA),
     .cfun  = blt_rasc_realTimeProcedureDataChar,
     .scfun = blt_rasc_realtimeProcedureCcc,
     },

    {
     .uuid  = UUID16_INIT(CHARACTERISTIC_UUID_ON_DEMAND_PROCEDURE_DATA),
     .cfun  = blt_rasc_onDemandProcedureDataChar,
     .scfun = blt_rasc_ondemandProcedureCcc,
     },

    {
     .subscribeInd = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_CONTROL_POINT),
     .cfun         = blt_rasc_controlPointChar,
     },

    {
     .subscribeInd = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_RANGING_DATA_READY),
     .cfun         = blt_rasc_rangingDataReadyChar,
     },

    {
     .subscribeInd = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_RANGING_DATA_OVERWRITTEN),
     .cfun         = blt_rasc_rangingDataOverwrittenChar,
     },
};

static const blc_gapc_discList_t discRas = {
    .maxServiceCount = 1,
    .service         = &rasService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(rasChar),
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
    if (count == 0) {
        blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
        if (!client) {
            goto failed;
        }
        blt_rasc_displayInfo(connHandle, client);
        BLC_RAS_LOG("INFO: Ranging Profile connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, CS_RAS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if (count > 1) {
        return false;
    }
    return true;
failed:
    return 0; //TODO
}

static int blt_rasc_rasFeatureGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ;
    charInfo->valueHandle = client->rasFeatureHandle;

    return 1;
}

static int blt_rasc_realTimeProcedureGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    charInfo->properties = client->realtimeProcedureDataProperties;
    charInfo->valueHandle = client->realtimeProcedureDataHandle;
    //charInfo->cccHandle = client->realtimeProcedureCccHandle; //comment if we want ccc to be kept 0x00, do we?

    return 1;
}

static int blt_rasc_onDemandProcedureGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    charInfo->properties = client->ondemandProcedureDataProperties;
    charInfo->valueHandle = client->ondemandProcedureDataHandle;
    //charInfo->cccHandle = client->ondemandProcedureCccHandle; //comment if we want ccc to be kept 0x00, do we?

    return 1;
}

static int blt_rasc_controlPointGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    charInfo->properties = client->rasControlPointProperties;
    charInfo->valueHandle = client->rasControlPointHandle;
    charInfo->cccHandle = client->rasControlPointCccHandle;

    return 1;
}

static int blt_rasc_rangingDataReadyGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    charInfo->properties = client->rangingDataReadyProperties;
    charInfo->valueHandle = client->rangingDataReadyDataHandle;
    charInfo->cccHandle = client->rangingDataReadyCccHandle;

    return 1;
}

static int blt_rasc_rangingDataOverwrittenGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);

    charInfo->properties = client->rangingDataOverwrittenProperties;
    charInfo->valueHandle = client->rangingDataOverwrittenDataHandle;
    charInfo->cccHandle = client->rangingDataOverwrittenCccHandle;

    return 1;
}

static const blc_gapc_reconnChar_t reRasChar[] = {
    {
        .ifun = blt_rasc_rasFeatureGetInfo,
        .rfun = blt_rasc_rasFeatureStartRead,
    },
    {
        .ifun = blt_rasc_realTimeProcedureGetInfo,
    },
    {
        .ifun = blt_rasc_onDemandProcedureGetInfo,
    },
    {
        .ifun = blt_rasc_controlPointGetInfo,
    },
    {
        .ifun = blt_rasc_rangingDataReadyGetInfo,
    },
    {
        .ifun = blt_rasc_rangingDataOverwrittenGetInfo,
    },
};

static const blc_gapc_reconnList_t reconnRas = {
    .resfun = blt_rasc_reconnService,
    .charTb = {
               .size           = ARRAY_SIZE(reRasChar),
               .characteristic = reRasChar,
               },
    .inclSize = 0,
};

ble_sts_t blc_rasc_readRasFeature(u16 connHandle, prf_read_cb_t readCb)
{
    return blt_rasc_readAttributeValue(connHandle, BLT_RASC_READ_RAS_FEATURE, readCb);
}

ble_sts_t blc_rasc_readRasDataReady(u16 connHandle, prf_read_cb_t readCb)
{
    return blt_rasc_readAttributeValue(connHandle, BLT_RASC_READ_RAS_DATA_READY, readCb);
}

ble_sts_t blc_rasc_readRasDataOverwritten(u16 connHandle, prf_read_cb_t readCb)
{
    return blt_rasc_readAttributeValue(connHandle, BLT_RASC_READ_RAS_DATA_OVERWRITTEN, readCb);
}

/**
 * @brief       ranging profile client write ranging in characteristic value callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   err: ATT layer return error code.
 * @param[in]   data: RFU.
 * @return      none.
 */
static void blc_rasc_writeControlPointCb(u16 connHandle, u8 err, void *data)
{
    (void)data;
    BLC_RAS_LOG("write control point callback, connHandle is 0x%02x, err is %x", connHandle, err);
    blc_prf_writeAttributeValueCallback(connHandle, err);
}

/**
 * @brief       ranging profile client write ranging in characteristic value with ATT_WRITE_REQ/PREPARE_WRITE_REQ command.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   val: want write value .
 * @param[in]   writeCb: write command send callback function.
 * @return      ble_sts_t.
 */
static ble_sts_t blt_rasc_writeRasControlPoint(u16 connHandle, blt_ras_cp_command_opcode_enum opcode, void *operand, u16 operandLen, prf_write_cb_t writeCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLC_RAS_LOG("ERR: ACL handle invalid");
        goto failed;
    }

    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        BLC_RAS_LOG("ERR: Client Inst handle invalid %d", connHandle);
        goto failed;
    }

    if (!client->rasControlPointHandle || !(client->rasControlPointProperties & CHAR_PROP_WRITE_WITHOUT_RSP) ||
        RAS_CP_CHECK_CMD_OPCODE(opcode)) {
        BLC_RAS_LOG("Control Point, properties not had write wo response");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blt_ras_cp_msg_t msg = {
        .opcode = opcode,
    };

    memcpy(&msg.operand.val[0], operand, operandLen);

    gapc_write_cfg_t pGapWrCfg;
    pGapWrCfg.func       = blc_rasc_writeControlPointCb;
    pGapWrCfg.handle     = client->rasControlPointHandle;
    pGapWrCfg.data       = &msg;
    pGapWrCfg.length     = operandLen + RAS_CP_MSG_HEADER_SIZE;
    pGapWrCfg.withoutRsp = true; //false;
    pGapWrCfg.cbData     = NULL;

    BLC_RAS_LOG("send Write RAS-CP: %s", hex_to_str(pGapWrCfg.data, pGapWrCfg.length));
    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rasc_writeGetSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb)
{
    blt_ras_operands_get_one_record_t operand = {
        .rangingCounter = rangingCounter};
    BLC_RAS_LOG("msg Get Procedure Data Command: rangingCounter = %d", rangingCounter);
#if (RAS_TIMEOUT_EN)
    // set timeout according to 4.5.4.1 RAS Spec
    blt_rasc_addTimeout(connHandle, rangingCounter, RAS_TIMER_ONDEMAND_DATA, ON_DEMAND_DATA_TIMEOUT_INTERVAL, blt_rasc_ondemandDataTimeout);
#endif
    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_GET_RANGING_DATA, &operand, sizeof(blt_ras_operands_get_one_record_t), writeCb);
}

ble_sts_t blt_rasc_writeAckSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb)
{
    blt_ras_operands_ack_one_record_t operand = {
        .rangingCounter = rangingCounter};

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_ACK_RANGING_DATA, &operand, sizeof(blt_ras_operands_ack_one_record_t), writeCb);
}

static ble_sts_t blt_rasc_writeGetRecordSegments(u16 connHandle, u16 rangingCounter, u16 startSegmentAsIndex, u16 endSegmentAsIndex, prf_write_cb_t writeCb)
{
    blt_ras_operands_get_lost_segments_t operand = {
        .rangingCounter      = rangingCounter,
        .startSegmentAsIndex = startSegmentAsIndex,
        .endSegmentAsIndex   = endSegmentAsIndex,
    };

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_GET_LOST_RANGING_DATA, &operand, sizeof(blt_ras_operands_get_lost_segments_t), writeCb);
}

ble_sts_t blt_rasc_writeAbortOperation(u16 connHandle, prf_write_cb_t writeCb)
{
    blt_ras_operands_abort_operation_t operand; //N/A

    return blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_ABORT_OPERATION, &operand, sizeof(blt_ras_operands_abort_operation_t), writeCb);
}

ble_sts_t blc_rapc_writeOnDemandCcc(u16 connHandle, u16 value, prf_write_cb_t writeCb)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    BLC_RAS_LOG("blc_rapc_writeOnDemandCcc handle: %x, value: %x", client->ondemandProcedureCccHandle, value);
    debugwait();

    return blt_rasc_setCccValue(connHandle, client->ondemandProcedureCccHandle, value, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rapc_writeRealtimeCcc(u16 connHandle, u16 value, prf_write_cb_t writeCb)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }
    BLC_RAS_LOG("blc_rapc_writeRealtimeCcc handle: %x, value: %x", client->realtimeProcedureCccHandle, value);
    debugwait();

    return blt_rasc_setCccValue(connHandle, client->realtimeProcedureCccHandle, value, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static void blt_rasc_clearAndInitializeRemote(blc_rasc_client_t* client)
{
    blt_rasc_clearSegmentList(client);
    blt_rasc_clearMergedData(client);
    memset(&client->rang_data, 0, sizeof(blt_ras_ranging_data_t));
}

ble_sts_t blc_rapc_clearAndInitializeRemote(u16 connHandle)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    blt_rasc_clearAndInitializeRemote(client);

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rapc_writeAbortOperation(u16 connHandle, prf_write_cb_t writeCb)
{
    return blt_rasc_writeAbortOperation(connHandle, writeCb);
}

ble_sts_t blc_rapc_setFilterModeAndValue(u16 connHandle, u8 mode, u16 filterValue, prf_write_cb_t writeCb)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    blt_ras_operands_filter_operation_t operand;
    operand.bits.filterBitMask = filterValue;
    operand.bits.mode          = mode;

    ble_sts_t result = blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_FILTER, &operand, sizeof(operand), writeCb);

    if(result == BLE_SUCCESS) {
        blt_ras_setFilterMode(rasDataset, operand.bits.mode, operand.bits.filterBitMask);
#if(RAS_PERSISTENT_FILTER)
        blt_prf_updatePairingInfoByAclHandle(connHandle);
#endif
    }
    return result;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rapc_setFilterSetting(u16 connHandle, u16 filterSetting, prf_write_cb_t writeCb)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        BLC_RAS_LOG("ERR: invalid dataset %d", connHandle);
        goto failed;
    }
    blt_ras_operands_filter_operation_t operand = (blt_ras_operands_filter_operation_t)filterSetting;

    ble_sts_t result = blt_rasc_writeRasControlPoint(connHandle, RAS_CP_CMD_OPCODE_FILTER, &operand, sizeof(operand), writeCb);

    if(result == BLE_SUCCESS) {
        blt_ras_setFilterMode(rasDataset, operand.bits.mode, operand.bits.filterBitMask);
#if(RAS_PERSISTENT_FILTER)
        blt_prf_updatePairingInfoByAclHandle(connHandle);
#endif
    }
    return result;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rapc_writeGetSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
    client->rasCPState = RAS_CP_STATE_EXPECTED_GET_RESPONSE;
    blt_rasc_setRequestedRangingCounter(client, rangingCounter);
    return blt_rasc_writeGetSpecificRecord(connHandle, rangingCounter, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rapc_writeAckSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
    blt_rasc_setRequestedRangingCounter(client, RAS_INVALID_INDEX_PROCEDURE);
    return blt_rasc_writeAckSpecificRecord(connHandle, rangingCounter, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rapc_writeGetRecordSegments(u16 connHandle, u16 rangingCounter, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
    client->rasCPState = RAS_CP_STATE_EXPECTED_LOST_RESPONSE;
    return blt_rasc_writeGetRecordSegments(connHandle, rangingCounter, startSegment, endSegment, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rasc_getRasFeature(u16 connHandle, svc_ras_feature_t **feature)
{
    blc_rasc_client_t *client = blc_rasc_getClientInst(connHandle);
    if (!client) {
        goto failed;
    }

    *feature = &client->ras_feature;


    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

/* PTS_TESTING */
#if(RAS_IOPTEST_ENABLE)
ble_sts_t blc_rasc_iop_writeGetRecordSegments(u16 connHandle, u16 rangingCounter, u16 startSegment, u16 endSegment, prf_write_cb_t writeCb)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
    client->rasCPState = RAS_CP_STATE_EXPECTED_LOST_RESPONSE;
    return blt_rasc_writeGetRecordSegments(connHandle, rangingCounter, startSegment, endSegment, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rapc_iop_setCccValue(u16 connHandle, u16 cccHandle, u16 value, prf_write_cb_t writeCb)
{
    return blt_rasc_setCccValue(connHandle, cccHandle, value, writeCb);
}

void blc_rasc_iop_enableRasClientManualTesting(u16 connHandle)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
#if(RAS_IOPTEST_ENABLE)
    client->iopTestingManual = true;
#endif
    return;
failed:
    return;
}

ble_sts_t blt_rasc_iop_issueAnyCommandControlPoint(u16 connHandle, blt_ras_cp_command_opcode_enum opcode, void* operand, u16 operandLen, prf_write_cb_t writeCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLC_RAS_LOG("ERR: ACL handle invalid");
        goto failed;
    }

    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }

    blt_ras_cp_msg_t msg = {
        .opcode = opcode,
    };

    memcpy(&msg.operand.val[0], operand, operandLen);

    gapc_write_cfg_t pGapWrCfg;
    pGapWrCfg.func = blc_rasc_writeControlPointCb;
    pGapWrCfg.handle = client->rasControlPointHandle;
    pGapWrCfg.data = &msg;
    pGapWrCfg.length = operandLen + RAS_CP_MSG_HEADER_SIZE;
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData = NULL;

    BLC_RAS_LOG("send Write RAS-CP: %s", hex_to_str(pGapWrCfg.data, pGapWrCfg.length));
    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rasc_iop_issueAnyCommandControlPoint(u16 connHandle, u8 opcode, void* operand, u16 operandLen, prf_write_cb_t writeCb)
{
    return blt_rasc_iop_issueAnyCommandControlPoint(connHandle, (blt_ras_cp_command_opcode_enum)opcode, operand, operandLen, writeCb);
}

ble_sts_t blc_rapc_iop_writeGetSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
    client->rasCPState = RAS_CP_STATE_EXPECTED_GET_RESPONSE;
    blt_rasc_setRequestedRangingCounter(client, rangingCounter);
    return blt_rasc_writeGetSpecificRecord(connHandle, rangingCounter, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rapc_iop_writeAckSpecificRecord(u16 connHandle, u16 rangingCounter, prf_write_cb_t writeCb)
{
    blc_rasc_client_t* client = blc_rasc_getClientInst(connHandle);
    if(!client) {
        goto failed;
    }
    blt_rasc_setRequestedRangingCounter(client, RAS_INVALID_INDEX_PROCEDURE);
    return blt_rasc_writeAckSpecificRecord(connHandle, rangingCounter, writeCb);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}
#endif

#if (RAS_PTS_LOST_SEGMENT_WORKAROUND)
void blc_rapc_iop_setSegmentsToSkipCount(u16 connHandle, u8 value)
{
    segmentsToSkipCount = value;
}
#endif
