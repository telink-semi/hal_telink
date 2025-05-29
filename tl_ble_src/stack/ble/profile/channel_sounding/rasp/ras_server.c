/********************************************SDK************************************************************
 * @file    ras_server.c
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
#include "stack/ble/host/ble_host.h"

#include "stack/ble/host/gatt/tlk_list_stack.h"
#include "stack/ble/host/gatt/tlk_malloc_stack.h"
#if OS_SUP_EN
    #include "stack/ble/os_sup/os_sup.h"
    #include "stack/ble/os_sup/os_sup_stack.h"
#endif

#include "ras_internal.h"

typedef att_err_t (*ras_cmd_cb)(u16 connHandle, blt_ras_cp_operand_t *operand);

typedef struct __attribute__((packed))
{
    u8         opcode;      //blt_ras_cp_command_opcode_enum
    u8         featureMask; //blt_ras_cp_command_opcode_feature_mask
    u8         expectLen;
    ras_cmd_cb cb;
} rasCtrlLega_t;

#define RAS_SEGMENTMASK_START 0x01
#define RAS_SEGMENTMASK_END   0x02

#define ATT_CCC_INDICATENOTIFY_MASK 0x0003

static int       blt_rass_init(u8 initType, const void *param);
static void      blt_rass_serviceInit(const blc_rass_regParam_t *param);
static int       blt_rass_connect(u16 connHandle, prf_acl_state_enum connState);
static int       blt_rass_loop(u16 connHandle);
static int       blt_rass_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);
static att_err_t blt_rass_allocateProtData(blt_rass_server_client_t *serverClient, u8 index, u16 size);
static void      blt_rass_releaseProtData(blt_rass_server_client_t *serverClient, u8 index);
static void      blt_rass_releaseAllProtData(blt_rass_server_client_t *serverClient);

static ble_sts_t                        blt_rass_clearQueue(u16 connHandle);
static blt_rass_protocol_query_result_t blc_rass_protocolQuery(blt_rass_server_client_t *serverClient, u16 rangingCounter);
static void                             blt_rass_setActiveOnDemand(blt_rass_server_client_t *serverClient, u8 active, u16 mtu, u16 rangingCounter, u8 startSegment, u8 endSegment);
static u8                               blt_rass_getActiveOnDemand(blt_rass_server_client_t *serverClient, u16 *rangingCounter);
static u16                              blt_rass_getActiveMtu(blt_rass_server_client_t *serverClient);
static bool                             blt_rass_validateRequestedSegments(blt_rass_server_client_t *serverClient, u8 reqStartSegment, u8 reqEndSegment);

static int blt_rass_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen);
static int blt_rass_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen);

static att_err_t blt_rass_recvRasControlPointCommand(u16 connHandle, u8 *writeValue, u16 valueLen);
static att_err_t blt_rass_recvDataReadyCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen);
static att_err_t blt_rass_recvDataOverwrittenCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen);
static att_err_t blt_rass_recvRealtimeCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen);
static att_err_t blt_rass_recvOnDemandCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen);
static att_err_t blt_rass_recvRasControlCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen);
static att_err_t blt_rass_recvDataReadyCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen);
static att_err_t blt_rass_recvDataOverwrittenCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen);
static att_err_t blt_rass_recvRealtimeCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen);
static att_err_t blt_rass_recvOnDemandCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen);
static void      blt_rass_indCfmCb(u16 connHandle, u16 scid);
static ble_sts_t blt_rass_sendValueIndicate(u16 connHandle, u16 attrHdl, u8 *p, int len);

static ble_sts_t blt_rass_reportRangingDataReady(u16 connHandle, blt_rass_report_ranging_data_ready_t *format);
static ble_sts_t blt_rass_reportRangingDataOverwritten(u16 connHandle, blt_rass_report_ranging_data_overwritten_t *format);
static ble_sts_t blt_rass_sendRasControlPoint(u16 connHandle, blt_ras_cp_response_opcode_enum opcode, void *operand, u16 operandLen);
static ble_sts_t blt_rass_sendCompleteReportRecordsResponse(u16 connHandle, u16 rangingCounter);
static ble_sts_t blt_rass_sendCompleteRecordsSegmentResponse(u16 connHandle, u16 rangingCounter, u8 startSegment, u8 endSegment);
static ble_sts_t blt_rass_sendResponseCodeResponse(u16 connHandle, u8 responseCode);
static ble_sts_t blt_rass_pushRollingRangingDataNotification(u16 connHandle, u16 attrHdl, blt_rass_report_ranging_data_t *rangingData);
static ble_sts_t blt_rass_pushRollingRangingDataIndication(u16 connHandle, u16 attrHdl, blt_rass_report_ranging_data_t *rangingData);
static ble_sts_t blt_rass_pushRollingRangingData(u16 connHandle, u16 attrHdl, blt_rass_report_ranging_data_t *rangingData);

static blt_rass_msg_t                             *blt_rass_mallocMsg(u16 connHandle, u8 type);
static blt_rass_report_realtime_procedure_data_t  *blt_rass_mallocReportRealTimeProcedureDataMsg(u16 connHandle);
static blt_rass_report_ondemand_procedure_data_t  *blt_rass_mallocReportOnDemandProcedureDataMsg(u16 connHandle);
static blt_rass_report_ranging_data_ready_t       *blt_rass_mallocReportRangingDataReadyMsg(u16 connHandle);
static blt_rass_report_ranging_data_overwritten_t *blt_rass_mallocProcedureDataOverwrittenMsg(u16 connHandle);
static blt_rass_report_complete_record_response_t *blt_rass_mallocReportRecordResponseMsg(u16 connHandle);
static blt_rass_report_lost_segment_response_t    *blt_rass_mallocReportSegmentResponseMsg(u16 connHandle);
static blt_rass_report_response_code_response_t   *blt_rass_mallocResponseCodeResponseMsg(u16 connHandle);

static ble_sts_t blt_rass_initialReportRealTimeData(u16 connHandle, u8 last, u16 rangingcounter, u8 *pData, u16 pDataLen);
static ble_sts_t blt_rass_initialOnDemandProcedureData(u16 connHandle, u16 rangingCounter, u8 *pData, u16 pDataLen, u8 lost, u8 *startSegment, u8 *endSegment);
static ble_sts_t blt_rass_initialProcedureDataReady(u16 connHandle, u16 rangingCounter);
static ble_sts_t blt_rass_initialProcedureDataOverwritten(u16 connHandle, u16 rangingCounter);
static ble_sts_t blt_rass_initialCompleteProcedureDataResponse(u16 connHandle, u16 numOfRecords);
static ble_sts_t blt_rass_initialCompleteLostProcedureSegmentResponse(u16 connHandle, u16 rangingCounter, u16 segmentStart, u16 segmentEnd);
static ble_sts_t blt_rass_initialResponseCode(u16 connHandle, blt_ras_response_enum responseCode);

typedef blt_ras_response_enum (*rass_dealMsgCb)(blt_rass_msg_t *msg);

static blt_ras_response_enum blt_rass_dealError(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealReportRealTimeData(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealReportOnDemandProcedureData(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealRangingDataReady(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealRangingDataOverwritten(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealReportRecordsResponse(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealRecordSegmentResponse(blt_rass_msg_t *msg);
static blt_ras_response_enum blt_rass_dealResponseCodeResponse(blt_rass_msg_t *msg);

static blt_ras_response_enum blt_rass_protocolDeleteLocal(blt_rass_server_client_t *serverClient, u16 rangingCounter);

rass_dealMsgCb dealMsgCb[] = {
    blt_rass_dealError,
    blt_rass_dealReportRealTimeData,
    blt_rass_dealReportOnDemandProcedureData,
    blt_rass_dealRangingDataReady,
    blt_rass_dealRangingDataOverwritten,
    blt_rass_dealReportRecordsResponse,
    blt_rass_dealRecordSegmentResponse,
    blt_rass_dealResponseCodeResponse,
};

#if (RAS_TIMEOUT_EN)
ble_sts_t blt_rass_checkForTimeout(u16 connHandle);
ble_sts_t blt_rass_refreshTimeout(u16 connHandle, u16 rangingCounter);
ble_sts_t blt_rass_clearTimeout(u16 connHandle, u16 rangingCounter);
#endif


#define RASS_MALLOC(len)  malloc_nonreten(len)
#define RASS_FREE(ptr)    free_nonreten(ptr)

#define SERVER_CONNHANDLE 0xFFFF

_attribute_ble_data_retention_
    SLIST_DEF(rassMsgList);
#if ((!defined(HOST_V2_ENABLE)))

_attribute_ble_data_retention_
    blt_ras_server_ctrl_t ras_server_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = CS_RAS_SERVER,
                    .usedAclRole = 0,
                    .init        = blt_rass_init,
                    .connect     = blt_rass_connect,
                    .discov      = NULL,
                    .loop        = blt_rass_loop,
                    .store       = blt_rass_store,
                    },
};
#else
static const struct blc_prf_process_params s_ras_server_process_params = {
    .id          = BAS_SERVER,
    .usedAclRole = PRF_GAP_ACL_UNSPECIF,
    .init        = blt_rass_init,
    .connect     = blt_rass_connect,
    .discovery   = NULL,
};

_attribute_ble_data_retention_ blt_ras_server_ctrl_t ras_server_ctrl = {
    .process = {
                .next       = SLIST_HEAD_INITIALIZER(),
                .prf_params = &s_ras_server_process_params,
                },
};
#endif

const blc_rass_regParam_t defaultRasPrfParam = {
    .ras_feature.realTimeProcedureDataSupport        = 1,
    .ras_feature.getLostProcedureDataSegmentsSupport = 1,
    .ras_feature.abortOperationSupport               = 1,
#if (RAS_STEP_FILTER)
    .ras_feature.filterProcedureDataSupport = 1,
#else
    .ras_feature.filterProcedureDataSupport = 0,
#endif
};
#if(Google_SRS)
// Google special ranging setting parameter
const google_srs_local_cap_t gl_local_cap = {
        .type = 0x00,
        .inline_pct = 0,
        .mode0ChanMap = 1,
        .prefered_aclIntval = 0,
};
const u8 srsSendCmdRspVal[1] = {0x00}; // reply value after receive srs write command
#endif
/**
 * @brief       register ranging profile server controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_rap_registerRasProfileControlServer(const blc_rass_regParam_t *param)
{
#if ((!defined(HOST_V2_ENABLE)))
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&ras_server_ctrl, param);
#else
    blc_prf_registerServiceModule((struct blc_prf_process *)&ras_server_ctrl, param);
#endif
}

/**
 * @brief       ranging profile server get Server for Client instance by connect handle.
 * @param[in]   connHandle: ACL connection.
 * @return      server control instance.
 */
static blt_rass_server_client_t *blt_rass_getServerClientsInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);
    return idx >= 0 ? ras_server_ctrl.rasServerClients[idx] : NULL;
}

/**
 * @brief       ranging profile server get Server control instance by connect handle.
 * @param[in]   connHandle: ACL connection.
 * @return      server control instance.
 */
static blt_rass_server_t *blt_rass_getServerInst(u16 connHandle)
{
    (void)connHandle;
    return &ras_server_ctrl.rasPrfServer;
}

/**
 * @brief       ranging profile server initial function.
 * @param[in]   initType: only PRF_PROC_INIT.
 * @param[in]   param: initial parameter.
 * @return      0.
 */
static int blt_rass_init(u8 initType, const void *param)
{
    if (initType == PRF_PROC_INIT) {
        BLC_RAS_LOG("server init");
        blc_svc_addRasGroup();
        blc_svc_rasCbackRegister(blt_rass_readCback, blt_rass_writeCback);
        blt_rass_serviceInit(param);
    }
    return BLE_SUCCESS;
}

/**
 * @brief       ranging profile server connect/disconnect event callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   connState: PRF_ACL_STATE_DISCONN/PRF_ACL_STATE_CONNECT.
 * @return      0.
 */
static int blt_rass_connect(u16 connHandle, prf_acl_state_enum connState)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);
    if (idx < 0) {
        goto failed;
    }

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLC_RAS_LOG("Disconnect:0x%x", connHandle);

        blt_rass_server_client_t *serverClient = ras_server_ctrl.rasServerClients[idx];
        if (serverClient == NULL) {
            goto failed;
        }
        if (serverClient->dataNtfIndBuffer != NULL) {
            free_nonreten(serverClient->dataNtfIndBuffer);
            serverClient->dataNtfIndBuffer = NULL;
        }
#if (RAS_REALTIME_PROT_FLOW)
        if (serverClient->realtime_prot_flow.rptr != serverClient->realtime_prot_flow.wptr) {
            u8 ptr_offset = (u8)(serverClient->realtime_prot_flow.wptr - serverClient->realtime_prot_flow.rptr);
            for (u8 i = 1; i < ptr_offset; i++) {
                blc_rass_realtime_prot_subevt_data_t *pSubevent = (blc_rass_realtime_prot_subevt_data_t *)(&serverClient->realtime_prot_flow.subEvtData[((serverClient->realtime_prot_flow.rptr + i) & 0xff) % (CS_REALTIME_SUBEVENT_MAX)]);
                if (pSubevent->pSubEvt != NULL) {
                    free_nonreten(pSubevent->pSubEvt);
                    pSubevent->pSubEvt = NULL;
                }
            }
        }
        serverClient->realtime_prot_flow.wptr = serverClient->realtime_prot_flow.rptr = 0;
#endif

        blt_rass_releaseAllProtData(serverClient);
        memset(serverClient, 0, sizeof(blt_rass_server_client_t));
        free_nonreten(serverClient);
        ras_server_ctrl.rasServerClients[idx] = NULL;

        blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
        if (rasDataset == NULL) {
            goto failed;
        }
        blt_ras_clearAndInitializeLocal(rasDataset);
        memset(rasDataset, 0, sizeof(blt_ras_dataset_t));
        free_nonreten(rasDataset);
        blc_ras_writeDataset(idx, NULL);

        blt_rass_clearQueue(connHandle);
    } else {
        BLC_RAS_LOG("Connect:0x%x", connHandle);
        blt_rass_server_client_t *serverClient = ras_server_ctrl.rasServerClients[idx];
        if (serverClient != NULL) {
            blt_rass_releaseAllProtData(serverClient);
#if (RAS_REALTIME_PROT_FLOW)
            if (serverClient->realtime_prot_flow.rptr != serverClient->realtime_prot_flow.wptr) {
                u8 ptr_offset = (u8)(serverClient->realtime_prot_flow.wptr - serverClient->realtime_prot_flow.rptr);
                for (u8 i = 1; i < ptr_offset; i++) {
                    blc_rass_realtime_prot_subevt_data_t *pSubevent = (blc_rass_realtime_prot_subevt_data_t *)(&serverClient->realtime_prot_flow.subEvtData[((serverClient->realtime_prot_flow.rptr + i) & 0xff) % (CS_REALTIME_SUBEVENT_MAX)]);
                    if (pSubevent->pSubEvt != NULL) {
                        free_nonreten(pSubevent->pSubEvt);
                        pSubevent->pSubEvt = NULL;
                    }
                }
            }
            serverClient->realtime_prot_flow.wptr = serverClient->realtime_prot_flow.rptr = 0;
#endif
            if (serverClient->dataNtfIndBuffer != NULL) {
                free_nonreten(serverClient->dataNtfIndBuffer);
                serverClient->dataNtfIndBuffer = NULL;
            }
        }
        ras_server_ctrl.rasServerClients[idx] = malloc_nonreten(sizeof(blt_rass_server_client_t));
        serverClient                          = ras_server_ctrl.rasServerClients[idx];
        if (serverClient == NULL) {
            goto failed;
        }
        memset(serverClient, 0, sizeof(blt_rass_server_client_t));
#if ((!defined(HOST_V2_ENABLE)))
        serverClient->dataNtfIndBuffer = malloc_nonreten(l2cap_buff_s.max_tx_size);
#else
        serverClient->dataNtfIndBuffer = malloc_nonreten(1024);
#endif
        if (serverClient->dataNtfIndBuffer == NULL) {
            BLC_RAS_LOG("ERROR! Out of memory spot 007");
            debugwait();
            goto failed2;
        }
        blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
        if (rasDataset != NULL) {
            blt_ras_clearAndInitializeLocal(rasDataset); //no persistence between connections
        }
        rasDataset = malloc_nonreten(sizeof(blt_ras_dataset_t));
        if (rasDataset == NULL) {
            goto failed;
        }
        blc_ras_writeDataset(idx, rasDataset);
        memset(rasDataset, 0, sizeof(blt_ras_dataset_t));
        blt_rass_setActiveOnDemand(serverClient, FALSE, 0, 0, 0, 0);
        blt_ras_initFilterDefault(rasDataset); //set filter to all 1 //OR TODO: read and restore bonding filter data, if we implement it
        //blc_rass_ClearAndInitializeRemote(); //server doesnt care about remote
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
failed2:
    return HCI_ERR_CONN_REJ_LIMITED_RESOURCES;
}

static int blt_rass_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    blt_rass_server_client_t* serverClient = blt_rass_getServerClientsInst(connHandle);

    if (nvState == PRF_NV_STATE_STORE) {
        blt_rass_nv_info_t nvInfo;
        nvInfo.realtimeProcedureCccValue = serverClient->realtimeProcedureCccValue;
        nvInfo.ondemandProcedureCccValue = serverClient->ondemandProcedureCccValue;
        nvInfo.rasControlPointCccValue = serverClient->rasControlPointCccValue;
        nvInfo.rangingDataOverwrittenCccValue = serverClient->rangingDataOverwrittenCccValue;
        nvInfo.rangingDataReadyCccValue = serverClient->rangingDataReadyCccValue;
#if(RAS_PERSISTENT_FILTER)
        memcpy(&nvInfo.filter, blt_ras_getFilter(connHandle), sizeof(blt_ras_filter_t));
        BLC_RAS_LOG("Filter store");
#endif
        U8_TO_STREAM(param->dataPtr, sizeof(blt_rass_nv_info_t));
        U8_TO_STREAM(param->dataPtr, CS_RAS_SERVER);
        STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(blt_rass_nv_info_t));
        param->currentTotalLen += 2 + sizeof(blt_rass_nv_info_t);
    } else if (nvState == PRF_NV_STATE_LOAD) {
        blt_rass_nv_info_t *nvInfo = (blt_rass_nv_info_t *) param->dataPtr;
        serverClient->realtimeProcedureCccValue = nvInfo->realtimeProcedureCccValue;
        serverClient->ondemandProcedureCccValue = nvInfo->ondemandProcedureCccValue;
        serverClient->rasControlPointCccValue = nvInfo->rasControlPointCccValue;
        serverClient->rangingDataOverwrittenCccValue = nvInfo->rangingDataOverwrittenCccValue;
        serverClient->rangingDataReadyCccValue = nvInfo->rangingDataReadyCccValue;
#if(RAS_PERSISTENT_FILTER)
        memcpy(blt_ras_getFilter(connHandle), &nvInfo->filter, sizeof(blt_ras_filter_t));
        blt_ras_filter_t* f = blt_ras_getFilter(connHandle);
        BLC_RAS_LOG("Filter load: %x, %x, %x, %x, %x, %x", connHandle, serverClient, f->mode0.raw, f->mode1.raw, f->mode2.raw, f->mode3.raw);
#endif
    }

    return BLE_SUCCESS;
}


#define RASS_RAS_FEATURE_HANDLE                   (blt_rass_getServerInst(SERVER_CONNHANDLE)->rasFeatureHandle)
#define RASS_REAL_TIME_DATA_HANDLE                (blt_rass_getServerInst(SERVER_CONNHANDLE)->realtimeProcedureDataHandle)
#define RASS_REAL_TIME_CCC_HANDLE                 (blt_rass_getServerInst(SERVER_CONNHANDLE)->realtimeProcedureAttCCCHandle)
#define RASS_ON_DEMAND_DATA_HANDLE                (blt_rass_getServerInst(SERVER_CONNHANDLE)->ondemandProcedureDataHandle)
#define RASS_ON_DEMAND_CCC_HANDLE                 (blt_rass_getServerInst(SERVER_CONNHANDLE)->ondemandProcedureAttCCCHandle)
#define RASS_CONTROL_POINT_DATA_HANDLE            (blt_rass_getServerInst(SERVER_CONNHANDLE)->controlPointDataHandle)
#define RASS_CONTROL_POINT_CCC_HANDLE             (blt_rass_getServerInst(SERVER_CONNHANDLE)->controlPointCccHandle)
#define RASS_RANGING_DATA_READY_DATA_HANDLE       (blt_rass_getServerInst(SERVER_CONNHANDLE)->rangingDataReadyDataHandle)
#define RASS_RANGING_DATA_READY_CCC_HANDLE        (blt_rass_getServerInst(SERVER_CONNHANDLE)->rangingDataReadyCccHandle)
#define RASS_RANGING_DATA_OVERWRITTEN_DATA_HANDLE (blt_rass_getServerInst(SERVER_CONNHANDLE)->rangingDataOverwrittenDataHandle)
#define RASS_RANGING_DATA_OVERWRITTEN_CCC_HANDLE  (blt_rass_getServerInst(SERVER_CONNHANDLE)->rangingDataOverwrittenCccHandle)

static void blt_rass_initRasFeatureChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    if (p->num > 0) {
        BLC_RAS_LOG("ERR: ras feature char too many");
        return;
    }
    server->rasFeatureHandle = p->charHandle;
}

/**
 * @brief       ranging profile server initial real time data characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initRealTimeProcedureDataChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    if (p->num > 0) {
        BLC_RAS_LOG("ERR: real time procedure data char too many");
        return;
    }
#if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
#endif
    server->realtimeProcedureDataHandle = p->charHandle;
    // TODO: Obtain CCC handle in other way as CCC may occupy another handle
    server->realtimeProcedureCccHandle = p->charHandle + 1;
}

/**
 * @brief       ranging profile server initial on demand procedure data characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initOnDemandProcedureDataChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    if (p->num > 0) {
        BLC_RAS_LOG("ERR: on demand procedure data char too many");
        return;
    }
#if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
#endif
    server->ondemandProcedureDataHandle = p->charHandle;
    // TODO: Obtain CCC handle in other way as CCC may occupy another handle
    server->ondemandProcedureCccHandle = p->charHandle + 1;
}

/**
 * @brief       ranging profile server initial control point characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initControlPointChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    if (p->num > 0) {
        BLC_RAS_LOG("ERR: control point char too many");
        return;
    }
    server->controlPointDataHandle = p->charHandle;
    // TODO: Obtain CCC handle in other way as CCC may occupy another handle
    server->controlPointCccHandle = p->charHandle + 1;
}

/**
 * @brief       ranging profile server initial ranging data ready characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initRangingDataReadyChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    if (p->num > 0) {
        BLC_RAS_LOG("ERR: ranging data ready char too many");
        return;
    }
#if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
#endif
    server->rangingDataReadyDataHandle = p->charHandle;
    // TODO: Obtain CCC handle in other way as CCC may occupy another handle
    server->rangingDataReadyCccHandle = p->charHandle + 1;
}

/**
 * @brief       ranging profile server initial ranging data overwritten characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initRangingDataOverwrittenChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    if (p->num > 0) {
        BLC_RAS_LOG("ERR: ranging data overwritten char too many");
        return;
    }
#if OS_SUP_EN
    if (blt_os_giveSem_cb) {
        blt_os_giveSem_cb();
    }
#endif
    server->rangingDataOverwrittenDataHandle = p->charHandle;
    // TODO: Obtain CCC handle in other way as CCC may occupy another handle
    server->rangingDataOverwrittenCccHandle = p->charHandle + 1;
}
#if(Google_SRS)
static void blt_rass_glSrs_ReadCapChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    server->googleSrsReadCapHandle = p->charHandle;
    server->googleSrsReadCapCccHandle = p->charHandle+1;
}

static void blt_rass_glSrs_SendCmdChar(atts_foundCharParam_t *p, void *input)
{
    blt_rass_server_t *server = (blt_rass_server_t *)input;
    server->googleSrsSendCmdHandle = p->charHandle;
    server->googleSrsSendCmdCccHandle = p->charHandle+1;
}
#endif
static const atts_findCharList_t rassChar[] = {
    {
     .charUuid    = characteristicRasFeatureUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_rass_initRasFeatureChar,
     },
    {
     .charUuid    = characteristicRealTimeProcedureDataUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_rass_initRealTimeProcedureDataChar,
     },
    {
     .charUuid    = characteristicOnDemandProcedureDataUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_rass_initOnDemandProcedureDataChar,
     },
    {
     .charUuid    = characteristicControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_rass_initControlPointChar,
     },
    {
     .charUuid    = characteristicRangingDataReadyUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_rass_initRangingDataReadyChar,
     },
    {
     .charUuid    = characteristicRangingDataOverwrittenUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_rass_initRangingDataOverwrittenChar,
     },
#if(Google_SRS)
     {
     .charUuid    = characteristicGoogleSrsReadCapUuid,
     .charUuidLen = ATT_128_UUID_LEN,
     .foundCback  = blt_rass_glSrs_ReadCapChar,
     },
     {
     .charUuid    = characteristicGoogleSrsSendCmdUuid,
     .charUuidLen = ATT_128_UUID_LEN,
     .foundCback  = blt_rass_glSrs_SendCmdChar,
     },
#endif
};

static u8 *blt_rass_getRasFeature(u16 connHandle)
{
    return blc_gatts_getAttributeValueByHandle(connHandle, RASS_RAS_FEATURE_HANDLE);
}

static ble_sts_t blt_rass_setFeature(u8 feature)
{
    u8 *pFeature = blt_rass_getRasFeature(SERVER_CONNHANDLE);
    if (!pFeature) {
        goto failed;
    }

    *pFeature = feature;
    return BLE_SUCCESS;
failed:
    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
}

/**
 * @brief       ranging profile server initial service value.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
static void blt_rass_serviceInit(const blc_rass_regParam_t *param)
{
    blt_rass_server_t *server = blt_rass_getServerInst(SERVER_CONNHANDLE);
    blc_atts_findCharacteristicByServiceUuid(serviceRangingUuid, ATT_16_UUID_LEN, rassChar, ARRAY_SIZE(rassChar), server);
    BLC_RAS_LOG("Handle information:");
    BLC_RAS_LOG("Ras feature Handle:0x%04x", server->rasFeatureHandle);
    BLC_RAS_LOG("Real Time procedure data Handle:0x%04x", server->realtimeProcedureDataHandle);
    BLC_RAS_LOG("Real Time procedure CCC Handle:0x%04x", server->realtimeProcedureCccHandle);
    BLC_RAS_LOG("On demand procedure data Handle:0x%04x", server->ondemandProcedureDataHandle);
    BLC_RAS_LOG("On demand procedure CCC Handle:0x%04x", server->ondemandProcedureCccHandle);
    BLC_RAS_LOG("Control point data Handle:0x%04x", server->controlPointDataHandle);
    BLC_RAS_LOG("Control point ccc Handle:0x%04x", server->controlPointCccHandle);
    BLC_RAS_LOG("Ranging data ready Data Handle:0x%04x", server->rangingDataReadyDataHandle);
    BLC_RAS_LOG("Ranging data ready Ccc Handle:0x%04x", server->rangingDataReadyCccHandle);
    BLC_RAS_LOG("Ranging data overwritten Data Handle:0x%04x", server->rangingDataOverwrittenDataHandle);
    BLC_RAS_LOG("Ranging data overwritten Ccc Handle:0x%04x", server->rangingDataOverwrittenCccHandle);

    blt_rass_setFeature(param->ras_feature.features);

    svc_ras_feature_t *feature = (svc_ras_feature_t *)blt_rass_getRasFeature(SERVER_CONNHANDLE);
    BLC_RAS_LOG("Feature: %x, rt: %d, lost %d, abort %d, filter %d", feature->features, feature->realTimeProcedureDataSupport, feature->getLostProcedureDataSegmentsSupport, feature->abortOperationSupport, feature->filterProcedureDataSupport);
}

static void blt_rass_setActiveOnDemand(blt_rass_server_client_t *serverClient, u8 active, u16 mtu, u16 rangingCounter, u8 startSegment, u8 endSegment)
{
    serverClient->activeOnDemand.active         = active;
    serverClient->busy                          = active;
    serverClient->activeOnDemand.activeMTU      = mtu;
    serverClient->activeOnDemand.rangingCounter = rangingCounter;
    serverClient->activeOnDemand.startSegment   = startSegment;
    serverClient->activeOnDemand.endSegment     = endSegment;

    BLC_RAS_LOG("setActive: a:%d, mtu:%d, ranC:%d, stS:%x, endS:%x", active, mtu, rangingCounter, startSegment, endSegment);
}

static u8 blt_rass_getActiveOnDemand(blt_rass_server_client_t *serverClient, u16 *rangingCounter)
{
    if (serverClient->activeOnDemand.active) {
        *rangingCounter = serverClient->activeOnDemand.rangingCounter;
    }
    return serverClient->activeOnDemand.active;
}

static u16 blt_rass_getActiveMtu(blt_rass_server_client_t *serverClient)
{
    if (serverClient->activeOnDemand.active) {
        return serverClient->activeOnDemand.activeMTU;
    }
    return serverClient->activeOnDemand.active; // 0
}

static bool blt_rass_validateRequestedSegments(blt_rass_server_client_t *serverClient, u8 reqStartSegment, u8 reqEndSegment)
{
    if (reqEndSegment < reqStartSegment) {
        return FALSE;
    }

    BLC_RAS_LOG("validate segments: reqStart<%x>,reqEnd<%x>, procStSeg<%x>, procEndSeg<%x>", reqStartSegment, reqEndSegment, serverClient->activeOnDemand.startSegment, serverClient->activeOnDemand.endSegment);
    debugwait();

    if (reqStartSegment < serverClient->activeOnDemand.startSegment) { //in case of 0
        return FALSE;
    }
    if (reqEndSegment == RAS_LOST_SEGMENT_WILDCARD) {
        return TRUE;
    }
    if (reqEndSegment > serverClient->activeOnDemand.endSegment) { //reqStart is also validated, as the reqEndSegment > reqStartSegment is already checked
        return FALSE;
    }
    if (reqStartSegment & RAS_SEGMENTMASK_START) {                 //procedure start segment
        if (reqStartSegment == serverClient->activeOnDemand.startSegment) {
            return TRUE;
        }
        return FALSE;                          // no other segment is allowed to have this bit set
    }
    if (reqEndSegment & RAS_SEGMENTMASK_END) { //procedure end segment
        if (reqEndSegment == serverClient->activeOnDemand.endSegment) {
            return TRUE;
        }
        return FALSE; // no other segment is allowed to have this bit set
    }
    return TRUE;
}

static void blt_rass_setRasServerBusy(blt_rass_server_client_t *serverClient, u8 busy)
{
    BLC_RAS_LOG("busyflag: %d", busy);
    serverClient->busy = busy;
}

static u8 blt_rass_getRasServerBusy(blt_rass_server_client_t *serverClient)
{
    return serverClient->busy;
}

static void blt_rass_setRealTimeSubEventBusy(blt_rass_server_client_t *serverClient, u8 busy)
{
    BLC_RAS_LOG("RealTimeSubEventBusy: %d", busy);
    serverClient->realtime_subevent_busy = busy;
}

static u8 blt_rass_getRealTimeSubEventBusy(blt_rass_server_client_t *serverClient)
{
    return serverClient->realtime_subevent_busy;
}

static att_err_t blt_rass_recvGetReportOneRecord(u16 connHandle, blt_ras_cp_operand_t *operand)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }
#if(TTF_EN)
    if(serverClient->stopGetEnabled) {
        TTF_LOG("recvGet get stopped");
        return ATT_SUCCESS;
    }
#endif
    if (blt_rass_getRasServerBusy(serverClient)) {
        BLC_RAS_LOG("busy");
        blt_rass_initialResponseCode(connHandle, CS_RAS_SERVER_BUSY);
        return ATT_SUCCESS;
    }

    u16 rangingCounter = operand->getOneRecord.rangingCounter;
    BLC_RAS_LOG("recvGet rangingCounter %d", rangingCounter);
    debugwait();

    blt_rass_protocol_query_result_t res = blc_rass_protocolQuery(serverClient, rangingCounter);

    if (res.status == 0) {
        BLC_RAS_LOG("not found");
        blt_rass_initialResponseCode(connHandle, CS_RAS_NO_RECORDS_FOUND);
        return ATT_SUCCESS; //ATT itself worked fine, but RAS has an error handled with response code
    }
    BLC_RAS_LOG("recv Get Procedure Data Command: rangingCounter<%d>", rangingCounter);
    debugwait();
    BLC_RAS_LOG("msg On-Demand Procedure Data Notify: %s", hex_to_str(res.protData->prot.pData, (res.protData->prot.dataLen > 100) ? 100 : res.protData->prot.dataLen));
    debugwait();

    u16 currentMtu   = blt_gap_getEffectiveMTU(connHandle) - 4;
    u8  startSegment = 0;
    u8  endSegment   = 0;

    blt_rass_setRasServerBusy(serverClient, TRUE); // mark as busy
    blt_rass_initialOnDemandProcedureData(connHandle, rangingCounter, res.protData->prot.pData, res.protData->prot.dataLen, RAS_ONDEMAND_ALL, &startSegment, &endSegment);
    blt_rass_setActiveOnDemand(serverClient, TRUE, currentMtu, rangingCounter, startSegment, endSegment);

    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvAckOnDemandOneRecord(u16 connHandle, blt_ras_cp_operand_t *operand)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }

    if (blt_rass_getRasServerBusy(serverClient)) {
        BLC_RAS_LOG("busy");
        blt_rass_initialResponseCode(connHandle, CS_RAS_SERVER_BUSY);
        return ATT_SUCCESS;
    }

    u16 rangingCounter = operand->ackOneRecord.rangingCounter;
#if (RAS_TIMEOUT_EN)
    blt_rass_clearTimeout(connHandle, rangingCounter);
#endif
    BLC_RAS_LOG("recv ACK Procedure Data Command: rangingCounter<%d> -> Delete", rangingCounter);
    int response = CS_RAS_SUCCESS;
    response = blt_rass_protocolDeleteLocal(serverClient, rangingCounter);
    BLC_RAS_LOG("blt_rass_protocolDeleteLocal response %d", response);
    blt_rass_setActiveOnDemand(serverClient, FALSE, 0, 0, 0, 0);
    //free protocol buffer.
    blt_rass_initialResponseCode(connHandle, response);
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvGetLostSegments(u16 connHandle, blt_ras_cp_operand_t *operand)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }

    if (blt_rass_getRasServerBusy(serverClient)) {
        BLC_RAS_LOG("busy");
        blt_rass_initialResponseCode(connHandle, CS_RAS_SERVER_BUSY);
        return ATT_SUCCESS;
    }

    u16 rangingCounter = operand->getLostSegments.rangingCounter;
#if (RAS_TIMEOUT_EN)
    blt_rass_clearTimeout(connHandle, rangingCounter); //TODO: Shouldn't it be update here and not clear?
#endif
    blt_rass_protocol_query_result_t res = blc_rass_protocolQuery(serverClient, rangingCounter);

    if (res.status == 0) {
        BLC_RAS_LOG("not found");
        blt_rass_initialResponseCode(connHandle, CS_RAS_NO_RECORDS_FOUND);
        return ATT_SUCCESS; //ATT itself worked fine, but RAS has an error handled with response code
    }

    u16 activeRangingCounter = 0;
    u8  active               = blt_rass_getActiveOnDemand(serverClient, &activeRangingCounter);
    if ((!active) || (activeRangingCounter != rangingCounter)) {
        BLC_RAS_LOG("not active %d, %d, %d", rangingCounter, active, activeRangingCounter);
        blt_rass_initialResponseCode(connHandle, CS_RAS_INVALID_PARAMETER);
        return ATT_SUCCESS; //Will respond ONLY if the on-demand ranging request has already been started previously
    }

    u16 currentMTU = blt_gap_getEffectiveMTU(connHandle) - 4;
    if (currentMTU != blt_rass_getActiveMtu(serverClient)) {
        BLC_RAS_LOG("mtu changed");
        blt_rass_initialResponseCode(connHandle, CS_RAS_PROCEDURE_NOT_COMPLETED);
        return ATT_SUCCESS; //ATT itself worked fine, but RAS has an error handled with response code
    }

    //ES-26224, ES-26235 - translate to the old format at this point. TODO: Lost segment functionality could be refactored into "index" format only, but this introduces a lot of risk with little gain
    // u8 startSegment = operand->getLostSegments.startSegment;
    // u8 endSegment = operand->getLostSegments.endSegment;
    u8 segmentCount = res.protData->prot.dataLen / currentMTU + (res.protData->prot.dataLen % currentMTU ? 1 : 0);
    u8 startSegmentAsRoll = blt_ras_indexToRollingSegment(operand->getLostSegments.startSegmentAsIndex, segmentCount);
    u8 segmentEndAsRoll = blt_ras_indexToRollingSegment(operand->getLostSegments.endSegmentAsIndex, segmentCount);

    if (!blt_rass_validateRequestedSegments(serverClient, startSegmentAsRoll, segmentEndAsRoll)) {
        blt_rass_initialResponseCode(connHandle, CS_RAS_NO_RECORDS_FOUND);
        return ATT_SUCCESS; //ATT itself worked fine, but RAS has an error handled with response code
    }

    BLC_RAS_LOG("recv Get Lost Procedure Segment Command: rangingCounter<%d>,startSegment<%d>,endSegment<%d>", rangingCounter, startSegmentAsRoll, segmentEndAsRoll);

    blt_rass_setRasServerBusy(serverClient, TRUE); // mark as busy
    blt_rass_initialOnDemandProcedureData(connHandle, rangingCounter, res.protData->prot.pData, res.protData->prot.dataLen, RAS_ONDEMAND_LOST, &startSegmentAsRoll, &segmentEndAsRoll);

    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvAbortOperation(u16 connHandle, blt_ras_cp_operand_t *operand)
{
    (void)operand;

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    if (serverClient->ondemandProcedureCccValue > 0) { //abort behaviour only defined for OnDemand functionality
        //clear the RAS message queue for this connHandle
        blt_rass_clearQueue(connHandle);
        blt_rass_setActiveOnDemand(serverClient, FALSE, 0, 0, 0, 0);
        blt_rass_initialResponseCode(connHandle, CS_RAS_SUCCESS);
    } else {
        BLC_RAS_LOG("ERROR recvAbortOperation in wrong mode: %d, %d", serverClient->ondemandProcedureCccValue, serverClient->realtimeProcedureCccValue);
    }

    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvFilterOperation(u16 connHandle, blt_ras_cp_operand_t *operand)
{
    BLC_RAS_LOG("blt_rass_recvFilterOperation");

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }

    if (serverClient->procDataExchgMechanism != PROC_DATA_EXCHG_NULL) {
        BLC_RAS_LOG("Exchange mechanism needs to be PROC_DATA_EXCHG_NULL");
        blt_rass_initialResponseCode(connHandle, CS_RAS_INVALID_PARAMETER);
        return ATT_SUCCESS;
    }
    BLC_RAS_LOG("Recv filter value: %x", operand->filterOperation.raw);
    debugwait();

    // u8 mode = operand->filterOperation.raw & 0x03; // u16 filterValue = (operand->filterOperation.raw & 0xFFFC) >> 2;
    blt_ras_setFilterMode(rasDataset, operand->filterOperation.bits.mode, operand->filterOperation.bits.filterBitMask);

#if(RAS_PERSISTENT_FILTER)
    blt_prf_updatePairingInfoByAclHandle(connHandle);
    blt_rass_initialResponseCode(connHandle, CS_RAS_SUCCESS_PERSISTED);
#else
    blt_rass_initialResponseCode(connHandle, CS_RAS_SUCCESS); //TODO: Should be changed to CS_RAS_SUCCESS_PERSISTED when/if we support persisted filter info stored with bonding information
#endif
    return ATT_SUCCESS;

failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static const rasCtrlLega_t rasCtrlLega[] = {
  //blt_ras_cp_operand_t
    {(u8)RAS_CP_CMD_OPCODE_GET_RANGING_DATA,      (u8)RAS_FEATURE_ACTIVE_MASK,  sizeof(blt_ras_operands_get_one_record_t),    blt_rass_recvGetReportOneRecord  },
    {(u8)RAS_CP_CMD_OPCODE_ACK_RANGING_DATA,      (u8)RAS_FEATURE_ACTIVE_MASK,  sizeof(blt_ras_operands_ack_one_record_t),    blt_rass_recvAckOnDemandOneRecord},
    {(u8)RAS_CP_CMD_OPCODE_GET_LOST_RANGING_DATA, (u8)RAS_FEATURE_GETLOST_MASK, sizeof(blt_ras_operands_get_lost_segments_t), blt_rass_recvGetLostSegments     },
    {(u8)RAS_CP_CMD_OPCODE_ABORT_OPERATION,       (u8)RAS_FEATURE_ABORT_MASK,   sizeof(blt_ras_operands_abort_operation_t),   blt_rass_recvAbortOperation      },
    {(u8)RAS_CP_CMD_OPCODE_FILTER,                (u8)RAS_FEATURE_FILTER_MASK,  sizeof(blt_ras_operands_filter_operation_t),  blt_rass_recvFilterOperation     },
};

static att_err_t blt_rass_recvRasControlPointCommand(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    if (valueLen < RAS_CP_MSG_HEADER_SIZE) {
        return ATT_ERR_INVALID_PDU;
    }
    blt_ras_cp_msg_t *msg = (blt_ras_cp_msg_t *)writeValue;
#if (TTF_EN)
    TTF_LOG("TTF_CPOP:%d TTF_CParg:%02x%02x", msg->opcode, msg->operand.val[0], msg->operand.val[1]);
    debugwait();
#endif
    for (size_t i = 0; i < ARRAY_SIZE(rasCtrlLega); i++) {
        if (msg->opcode == rasCtrlLega[i].opcode) {
            BLC_RAS_LOG("recv RASCP connHandle %x, i: %d, opcode %d", connHandle, i, rasCtrlLega[i].opcode);
            debugwait();
            if (((*blt_rass_getRasFeature(SERVER_CONNHANDLE)) | rasCtrlLega[i].featureMask) == RAS_FEATURE_ACTIVE_MASK) {
                if ((valueLen - RAS_CP_MSG_HEADER_SIZE) == rasCtrlLega[i].expectLen) {
                    return rasCtrlLega[i].cb(connHandle, &msg->operand);
                } else { //wrong length of parameter
                    BLC_RAS_LOG("rascp wronglen i: %d, opcode %d, len %d, expectlen %d", i, rasCtrlLega[i].opcode, valueLen - RAS_CP_MSG_HEADER_SIZE, rasCtrlLega[i].expectLen);
                    debugwait();
                    blt_rass_initialResponseCode(connHandle, CS_RAS_INVALID_PARAMETER);
                    return ATT_ERR_INVALID_ATTR_VALUE_LEN;
                }
            } else {
                BLC_RAS_LOG("rascp no featureBit i: %d, opcode %d, feature %x, mask %x", i, rasCtrlLega[i].opcode, *blt_rass_getRasFeature(SERVER_CONNHANDLE), rasCtrlLega[i].featureMask);
                debugwait();
                break;
            }
        }
    }
    //no valid opcode found
    blt_rass_initialResponseCode(connHandle, CS_RAS_OPCODE_NOT_SUPPORTED);
    return ATT_ERR_INVALID_PDU;
}

static att_err_t blt_rass_recvDataReadyCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    *outValue    = (u8 *)&serverClient->rangingDataReadyCccValue;
    *outValueLen = sizeof(serverClient->rangingDataReadyCccValue);
    BLC_RAS_LOG("data ready rcback 0x%04x", serverClient->rangingDataReadyCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvDataOverwrittenCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    *outValue    = (u8 *)&serverClient->rangingDataOverwrittenCccValue;
    *outValueLen = sizeof(serverClient->rangingDataOverwrittenCccValue);
    BLC_RAS_LOG("data overwritten rcback 0x%04x", serverClient->rangingDataOverwrittenCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvRasControlCccRead(u16 connHandle, u8** outValue, u16* outValueLen)
{
    blt_rass_server_client_t* serverClient = blt_rass_getServerClientsInst(connHandle);
    if(!serverClient) {
        goto failed;
    }

    *outValue    = (u8 *)&serverClient->rasControlPointCccValue;
    *outValueLen = sizeof(serverClient->rasControlPointCccValue);
    BLC_RAS_LOG("rascp ccc rcback 0x%04x", serverClient->rasControlPointCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvRealtimeCccRead(u16 connHandle, u8** outValue, u16* outValueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    *outValue    = (u8 *)&serverClient->realtimeProcedureCccValue;
    *outValueLen = sizeof(serverClient->realtimeProcedureCccValue);
    BLC_RAS_LOG("realtime rcback 0x%04x", serverClient->realtimeProcedureCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvOnDemandCccRead(u16 connHandle, u8 **outValue, u16 *outValueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    *outValue    = (u8 *)&serverClient->ondemandProcedureCccValue;
    *outValueLen = sizeof(serverClient->ondemandProcedureCccValue);
    BLC_RAS_LOG("realtime rcback 0x%04x", serverClient->ondemandProcedureCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvRasControlCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    (void)valueLen;   //check for valueLen == 2 ?

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    u16 newCccValue = 0;
    BYTE_TO_UINT16(newCccValue, writeValue);
    // newCccValue &= 0x0003; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    serverClient->rasControlPointCccValue = newCccValue;
    BLC_RAS_LOG("rascp ccc wcback 0x%04x", serverClient->rasControlPointCccValue);
    debugwait();
    TTF_LOG("rascp ccc wcback 0x%04x", serverClient->rasControlPointCccValue);
    debugwait();
    return ATT_SUCCESS; //ATT_ERR_WRITE_REQUEST_REJECT; //needed by PTS
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvDataReadyCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    (void)valueLen; //check for valueLen == 2 ?

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    u16 newCccValue = 0;
    BYTE_TO_UINT16(newCccValue, writeValue);
    // newCccValue &= 0x0003; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    serverClient->rangingDataReadyCccValue = newCccValue;
    BLC_RAS_LOG("data ready wcback newCccValue 0x%04x", serverClient->rangingDataReadyCccValue);
    debugwait();
    TTF_LOG("data ready wcback newCccValue 0x%04x", serverClient->rangingDataReadyCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvDataOverwrittenCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    (void)valueLen; //check for valueLen == 2 ?

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    u16 newCccValue = 0;
    BYTE_TO_UINT16(newCccValue, writeValue);
    // newCccValue &= 0x0003; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    serverClient->rangingDataOverwrittenCccValue = newCccValue;
    BLC_RAS_LOG("data overwritten newCccValue 0x%04x", serverClient->rangingDataOverwrittenCccValue);
    debugwait();
    TTF_LOG("data overwritten newCccValue 0x%04x", serverClient->rangingDataOverwrittenCccValue);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvRealtimeCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    (void)valueLen; //check for valueLen == 2 ?

    svc_ras_feature_t *feature = (svc_ras_feature_t *)blt_rass_getRasFeature(SERVER_CONNHANDLE);
    if (feature->realTimeProcedureDataSupport == 0) {
        BLC_RAS_LOG("get lost feature not supported %d", feature->realTimeProcedureDataSupport);
        debugwait();
        return ATT_ERR_CCC_DESCRIPTOR_IMPROPERLY_CONFIGURED; //realtime feature disabled, disallow enabling rtccc
    }

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    u16 newCccValue = 0;
    BYTE_TO_UINT16(newCccValue, writeValue);
    // newCccValue &= 0x0003; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    u16 newCccValueMasked = newCccValue & ATT_CCC_INDICATENOTIFY_MASK; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    BLC_RAS_LOG("realtime wcback newCccValue 0x%04x", newCccValue);
    debugwait();
    TTF_LOG("realtime wcback newCccValue 0x%04x", newCccValue);
    debugwait();
    if (newCccValueMasked > 0) {
        if ((serverClient->ondemandProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) > 0) {
            return ATT_ERR_CCC_DESCRIPTOR_IMPROPERLY_CONFIGURED;
        }
        if (((*blt_rass_getRasFeature(SERVER_CONNHANDLE)) | RAS_FEATURE_REALTIME_MASK) != RAS_FEATURE_ACTIVE_MASK) {
            BLC_RAS_LOG("realtime feature bit not set feature %x, mask %x", *blt_rass_getRasFeature(SERVER_CONNHANDLE), RAS_FEATURE_REALTIME_MASK);
            debugwait();
            return ATT_ERR_CCC_DESCRIPTOR_IMPROPERLY_CONFIGURED;
        }
        if (newCccValueMasked & ATT_CCC_INDICATE) {
            serverClient->procDataExchgMechanism = PROC_DATA_EXCHG_REALTIME_INDICATIONS;
        }
        //if both ATT_CCC_INDICATE and ATT_CCC_NOTIFY are set, NOTIFY gets chosen
        if (newCccValueMasked & ATT_CCC_NOTIFY) {
            serverClient->procDataExchgMechanism = PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS;
        }
    }
    //on unsubscribe clear the queue
    if ((newCccValueMasked == 0) && ((serverClient->realtimeProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) > 0)) {
        blt_rass_clearQueue(connHandle);
    }
    //zero value is always allowed. Any value allowed when ondemand not set
    serverClient->realtimeProcedureCccValue = newCccValue;
    if (((serverClient->realtimeProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) == 0) && ((serverClient->ondemandProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) == 0)) {
        serverClient->procDataExchgMechanism = PROC_DATA_EXCHG_NULL;
    }
    BLC_RAS_LOG("realtime wcback 0x%04x, xchng %d", serverClient->realtimeProcedureCccValue, serverClient->procDataExchgMechanism);
    debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_recvOnDemandCccWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    (void)valueLen; //check for valueLen == 2 ?

    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    u16 newCccValue = 0;
    BYTE_TO_UINT16(newCccValue, writeValue);
    // newCccValue &= 0x0003; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    u16 newCccValueMasked = newCccValue & ATT_CCC_INDICATENOTIFY_MASK; //filter out valid values ATT_CCC_NOTIFY and/or ATT_CCC_INDICATE
    BLC_RAS_LOG("ondemand wcback newCccValue 0x%04x", newCccValue);debugwait();
    TTF_LOG("ondemand wcback newCccValue 0x%04x", newCccValue);debugwait();
    if (newCccValueMasked > 0) {
        if ((serverClient->realtimeProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) > 0) {
            return ATT_ERR_CCC_DESCRIPTOR_IMPROPERLY_CONFIGURED;
        }
        // if (blt_rass_allocateProtData(serverClient) != ATT_SUCCESS) {
        //     goto failed2;
        // }
        if (newCccValueMasked & ATT_CCC_INDICATE) {
            serverClient->procDataExchgMechanism = PROC_DATA_EXCHG_ONDEMAND_INDICATIONS;
        }
        //if both ATT_CCC_INDICATE and ATT_CCC_NOTIFY are set, NOTIFY gets chosen
        if (newCccValueMasked & ATT_CCC_NOTIFY) {
            serverClient->procDataExchgMechanism = PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS;
        }
    }
    //on unsubscribe clear the queue
    if ((newCccValueMasked == 0) && ((serverClient->ondemandProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) > 0)) {
        blt_rass_clearQueue(connHandle);
        blt_rass_releaseAllProtData(serverClient); //clear all in case there is anything allocated
    }
    //zero value is always allowed. Any value allowed when realtime not set
    serverClient->ondemandProcedureCccValue = newCccValue;
    if (((serverClient->realtimeProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) == 0) && ((serverClient->ondemandProcedureCccValue & ATT_CCC_INDICATENOTIFY_MASK) == 0)) {
        serverClient->procDataExchgMechanism = PROC_DATA_EXCHG_NULL;
    }
    BLC_RAS_LOG("ondemand wcback 0x%04x, xchng %d", serverClient->ondemandProcedureCccValue, serverClient->procDataExchgMechanism);debugwait();
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
// failed2:
//     return ATT_ERR_UNLIKELY_ERR;
}
#if(Google_SRS)
static att_err_t blt_rass_srs_recvReadCap(u16 connHandle, u8 **outValue, u16 *outValueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    *outValue    = (u8*)&gl_local_cap;
    *outValueLen = sizeof(gl_local_cap);

    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_srs_sendCmd(u16 connHandle, u8 **outValue, u16 *outValueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    *outValue    = (u8*)&srsSendCmdRspVal;
    *outValueLen = sizeof(srsSendCmdRspVal);

    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_srs_recvCapWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    if(writeValue[0] == 0x01) {
        BLC_RAS_LOG("receive reply from client, srs read success");
    } else {
        goto failed;
    }
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static att_err_t blt_rass_srs_recvCmdWrite(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    u8 type             =   writeValue[0];
    u8 enable_1side_pct =   writeValue[1]; // todo: not support in jaguar,tercel,ignore now
    u8 evt_mask         =   writeValue[2]; // todo: current HAL is wrong in pixel, event should be 4 bytes, now is 1 bytes
    u8 enable_mode0_csMap = writeValue[3];
    if(type == 0x01) { // client reply
        if(enable_mode0_csMap == 0x01) { // todo: enable mode0 channel map feature. should use hci, now directly enable.
            BLC_RAS_LOG("enable mode0 channel map feature");
            tlkapi_printf(1,"enable mode0 channel map feature");
            // mode0 channel map feature is enabled before cs config procedure, so we can't determine which config to use
            // this feature, wo need default enable this feature for all cs config -- yuexin,2025/03/17
            gCsMng.gGlobal_pCsCfg->srs_chn_en = 1;
        } else if (enable_mode0_csMap == 0x00) {
            BLC_RAS_LOG("disable mode0 channel map feature");
            gCsMng.gGlobal_pCsCfg->srs_chn_en = 0;
        } else if (enable_mode0_csMap == 0x02) {
            BLC_RAS_LOG("ignore this cmd");
        } else {
            goto failed;
        }
    } else {
        goto failed;
    }
    return ATT_SUCCESS;
failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}
#endif
static int blt_rass_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen)
{
    (void)opcode;
    blt_rass_server_t *server = blt_rass_getServerInst(SERVER_CONNHANDLE);

    if (attrHandle == server->controlPointCccHandle) {
        return blt_rass_recvRasControlCccRead(connHandle, outValue, outValueLen);
    } else if (attrHandle == server->rangingDataReadyCccHandle) {
        return blt_rass_recvDataReadyCccRead(connHandle, outValue, outValueLen);
    } else if (attrHandle == server->rangingDataOverwrittenCccHandle) {
        return blt_rass_recvDataOverwrittenCccRead(connHandle, outValue, outValueLen);
    } else if (attrHandle == server->realtimeProcedureCccHandle) {
        return blt_rass_recvRealtimeCccRead(connHandle, outValue, outValueLen);
    } else if (attrHandle == server->ondemandProcedureCccHandle) {
        return blt_rass_recvOnDemandCccRead(connHandle, outValue, outValueLen);
    }
#if(Google_SRS)
    else if (attrHandle == server->googleSrsReadCapHandle) {
        return blt_rass_srs_recvReadCap(connHandle, outValue, outValueLen);
    } else if (attrHandle == server->googleSrsSendCmdHandle) {
        return blt_rass_srs_sendCmd(connHandle, outValue, outValueLen);
    }
#endif
    return ATT_ERR_INVALID_HANDLE;
}

static int blt_rass_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;
    blt_rass_server_t *server = blt_rass_getServerInst(SERVER_CONNHANDLE); //static - no need to validate

    if (attrHandle == server->controlPointDataHandle) {
        return blt_rass_recvRasControlPointCommand(connHandle, writeValue, valueLen);
    } else if (attrHandle == server->controlPointCccHandle) {
        return blt_rass_recvRasControlCccWrite(connHandle, writeValue, valueLen);
    } else if (attrHandle == server->rangingDataReadyCccHandle) {
        return blt_rass_recvDataReadyCccWrite(connHandle, writeValue, valueLen);
    } else if (attrHandle == server->rangingDataOverwrittenCccHandle) {
        return blt_rass_recvDataOverwrittenCccWrite(connHandle, writeValue, valueLen);
    } else if (attrHandle == server->realtimeProcedureCccHandle) {
        return blt_rass_recvRealtimeCccWrite(connHandle, writeValue, valueLen);
    } else if (attrHandle == server->ondemandProcedureCccHandle) {
        return blt_rass_recvOnDemandCccWrite(connHandle, writeValue, valueLen);
    }
#if(Google_SRS)
    else if (attrHandle == server->googleSrsReadCapHandle) {
        return blt_rass_srs_recvCapWrite(connHandle, writeValue, valueLen);
    } else if (attrHandle == server->googleSrsSendCmdHandle) {
        return blt_rass_srs_recvCmdWrite(connHandle, writeValue, valueLen);
    }
#endif
    return ATT_ERR_INVALID_HANDLE;
}

static void blt_rass_indCfmCb(u16 connHandle, u16 scid)
{
    (void)scid;
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    BLC_RAS_LOG("blt_rass_indCfmCb");
    debugwait();
    serverClient->pendingIndCfm = 0;
failed:
    return;
}

static ble_sts_t blt_rass_sendValueIndicate(u16 connHandle, u16 attrHdl, u8 *p, int len)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    if (serverClient->pendingIndCfm) {
        return GATT_ERR_NOTIFY_INDICATION_BUSY;
    }

    gattsIndValue_t ind = {
        .attrHandle = attrHdl,
        .cb         = blt_rass_indCfmCb,
        .connHandle = connHandle,
        .scid       = L2CAP_CID_ATTR_PROTOCOL,
        .value      = p,
        .valueLen   = len,
    };

    ble_sts_t ret = blc_gatts_indicateValue(&ind);
    if (ret == BLE_SUCCESS) {
        serverClient->pendingIndCfm = 1;
    }
    return ret;
failed:
    return LL_ERR_INVALID_PARAMETER;
}

static ble_sts_t blt_rass_reportRangingDataReady(u16 connHandle, blt_rass_report_ranging_data_ready_t *format)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

#if (RAS_DEBUG_PRINTSENT)
    BLC_RAS_LOG("send Data Ready: ccc 0x%04x data %s", serverClient->rangingDataReadyCccValue, hex_to_str(format, sizeof(blt_rass_report_ranging_data_ready_t)));
#endif
    if (serverClient->rangingDataReadyCccValue > 0) {
        if (serverClient->rangingDataReadyCccValue & ATT_CCC_INDICATE) { //d0.9r11 if both set use indicate for data ready and overwrite by default
            return blt_rass_sendValueIndicate(connHandle, RASS_RANGING_DATA_READY_DATA_HANDLE, (u8 *)format, sizeof(blt_rass_report_ranging_data_ready_t));
        } else {
            return blc_atts_sendHandleValueNotify(connHandle, RASS_RANGING_DATA_READY_DATA_HANDLE, (u8 *)format, sizeof(blt_rass_report_ranging_data_ready_t));
        }
    } else {
        return BLE_SUCCESS;         //ccc value => not subscribed - dont send anything
    }
failed:
    return HCI_ERR_UNKNOWN_CONN_ID; //this should never happen
}

static ble_sts_t blt_rass_reportRangingDataOverwritten(u16 connHandle, blt_rass_report_ranging_data_overwritten_t *format)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

#if (RAS_DEBUG_PRINTSENT)
    BLC_RAS_LOG("send Data Overwritten: ccc 0x%04x data %s", serverClient->rangingDataOverwrittenCccValue, hex_to_str(format, sizeof(blt_rass_report_ranging_data_overwritten_t)));
#endif
    if (serverClient->rangingDataOverwrittenCccValue > 0) {
        if (serverClient->rangingDataOverwrittenCccValue & ATT_CCC_INDICATE) { //d0.9r11 if both set use indicate for data ready and overwrite by default
            return blt_rass_sendValueIndicate(connHandle, RASS_RANGING_DATA_OVERWRITTEN_DATA_HANDLE, (u8 *)format, sizeof(blt_rass_report_ranging_data_overwritten_t));
        } else {
            return blc_atts_sendHandleValueNotify(connHandle, RASS_RANGING_DATA_OVERWRITTEN_DATA_HANDLE, (u8 *)format, sizeof(blt_rass_report_ranging_data_overwritten_t));
        }
    } else {
        return BLE_SUCCESS;         //ccc value => not subscribed - dont send anything
    }
failed:
    return HCI_ERR_UNKNOWN_CONN_ID; //this should never happen
}

static ble_sts_t blt_rass_sendRasControlPoint(u16 connHandle, blt_ras_cp_response_opcode_enum opcode, void *operand, u16 operandLen)
{
    blt_ras_cp_msg_t msg = {
        .opcode = opcode,
    };
    memcpy(&msg.operand.val[0], operand, operandLen);
    return blt_rass_sendValueIndicate(connHandle, RASS_CONTROL_POINT_DATA_HANDLE, (u8 *)&msg, operandLen + RAS_CP_MSG_HEADER_SIZE);
}

static ble_sts_t blt_rass_sendCompleteReportRecordsResponse(u16 connHandle, u16 rangingCounter)
{
    blt_ras_operands_complete_record_t operand = {
        .rangingCounter = rangingCounter};
    return blt_rass_sendRasControlPoint(connHandle, RAS_CP_RSP_OPCODE_COMPLETE_REPORT_RECORDS, &operand, sizeof(blt_ras_operands_complete_record_t));
}

static ble_sts_t blt_rass_sendCompleteRecordsSegmentResponse(u16 connHandle, u16 rangingCounter, u8 startSegment, u8 endSegment)
{
    blt_ras_operands_complete_lost_segment_t operand = {
        .rangingCounter = rangingCounter,
        .startSegment   = startSegment,
        .endSegment     = endSegment,
    };
    return blt_rass_sendRasControlPoint(connHandle, RAS_CP_RSP_OPCODE_COMPLETE_RECORD_SEGMENT, &operand, sizeof(blt_ras_operands_complete_lost_segment_t));
}

static ble_sts_t blt_rass_sendResponseCodeResponse(u16 connHandle, u8 responseCode)
{
    blt_ras_operands_response_code_t operand = {
        .responseCode = responseCode};
    return blt_rass_sendRasControlPoint(connHandle, RAS_CP_RSP_OPCODE_RESPONSE_CODE, &operand, sizeof(blt_ras_operands_response_code_t));
}

static ble_sts_t blt_rass_pushRollingRangingDataNotification(u16 connHandle, u16 attrHdl, blt_rass_report_ranging_data_t *rangingData) // must retrun ble_sts_t
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    u8 *payload    = rangingData->pData + rangingData->segmentIndex * rangingData->MTU;
    int payloadLen = rangingData->MTU;

    blt_ras_segmentation_ranging_data_t *pSegmBuffer = (blt_ras_segmentation_ranging_data_t *)(serverClient->dataNtfIndBuffer);
    u8                                  *pBuffer     = (u8 *)(serverClient->dataNtfIndBuffer);
    pBuffer += sizeof(pSegmBuffer->header.raw);
    memcpy(pBuffer, payload, payloadLen);

    pSegmBuffer->header.data.firstSegment = 0;
    pSegmBuffer->header.data.lastSegment  = 0;

    if ((rangingData->segmentIndex + rangingData->segmentOffset) == 0) {
        pSegmBuffer->header.data.firstSegment = 1;
    }
    if (rangingData->segmentIndex == rangingData->segmentCount - 1) {
        payloadLen = rangingData->pDataLen - rangingData->segmentIndex * rangingData->MTU;
        if (!rangingData->isRealtime) {
            pSegmBuffer->header.data.lastSegment = 1;
        } else { //isRealtime
            if (rangingData->last) {
                pSegmBuffer->header.data.lastSegment = 1;
            }
        }
    }
    pSegmBuffer->header.data.segmentCounter = (rangingData->segmentIndex + rangingData->segmentOffset) & 0x3F;

#if (TTF_EN)
    //stubbed on client side
    extern int  segment_skipper_check(u8 segment_to_check);
    extern void delay_segment(u8 segment);

    delay_segment(pSegmBuffer->header.data.segmentCounter);
    if (segment_skipper_check(pSegmBuffer->header.data.segmentCounter) == 1) {
        return BLE_SUCCESS;
    }
#endif

#if (RAS_IOPTEST_LOSTSEGMENTS)
    //IOP: for testing lost segments
    static u8 done1 = TRUE;
    static u8 done2 = TRUE;
    if (done1) {
        if (pSegmBuffer->header.data.segmentCounter == 2) {
            done1 = FALSE;
            BLC_RAS_LOG("blt_rass_pushRollingRangingDataNotification2");
            debugwait();
            return BLE_SUCCESS;
        }
    }
    if (done2) {
        if (pSegmBuffer->header.data.lastSegment) {
            done2 = FALSE;
            BLC_RAS_LOG("blt_rass_pushRollingRangingDataNotification3");
            debugwait();
            return BLE_SUCCESS;
        }
    }
#endif

    return blc_atts_sendHandleValueNotify(connHandle, attrHdl, (u8 *)pSegmBuffer, sizeof(pSegmBuffer->header.raw) + payloadLen); //blc_gatts_notifyValue(connHandle, attrHdl, pSegmBuffer, sizeof(pSegmBuffer->header.raw) + payloadLen);
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rass_pushRollingRangingDataIndication(u16 connHandle, u16 attrHdl, blt_rass_report_ranging_data_t *rangingData) // must retrun ble_sts_t
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    if (serverClient->pendingIndCfm) {
        return GATT_ERR_NOTIFY_INDICATION_BUSY;
    }

    gattsIndValue_t ind = {
        .attrHandle = attrHdl,
        .cb         = blt_rass_indCfmCb,
        .connHandle = connHandle,
        .scid       = L2CAP_CID_ATTR_PROTOCOL,
    };

    u8 *payload    = rangingData->pData + rangingData->segmentIndex * rangingData->MTU;
    int payloadLen = rangingData->MTU;

    blt_ras_segmentation_ranging_data_t *pSegmBuffer = (blt_ras_segmentation_ranging_data_t *)(serverClient->dataNtfIndBuffer);
    u8                                  *pBuffer     = (u8 *)(serverClient->dataNtfIndBuffer);
    pBuffer += sizeof(pSegmBuffer->header.raw);
    memcpy(pBuffer, payload, payloadLen);

    pSegmBuffer->header.data.firstSegment = 0;
    pSegmBuffer->header.data.lastSegment  = 0;

    if ((rangingData->segmentIndex + rangingData->segmentOffset) == 0) {
        pSegmBuffer->header.data.firstSegment = 1;
    }

    if (rangingData->segmentIndex == rangingData->segmentCount - 1) {
        payloadLen = rangingData->pDataLen - rangingData->segmentIndex * rangingData->MTU;
        if (!rangingData->isRealtime) {
            pSegmBuffer->header.data.lastSegment = 1;
        } else { //isRealtime
            if (rangingData->last) {
                pSegmBuffer->header.data.lastSegment = 1;
            }
        }
    }
    pSegmBuffer->header.data.segmentCounter = (rangingData->segmentIndex + rangingData->segmentOffset) & 0x3F;

    ind.value    = pSegmBuffer;
    ind.valueLen = sizeof(pSegmBuffer->header.raw) + payloadLen;

    ble_sts_t ret = blc_gatts_indicateValue(&ind);
    if (ret == BLE_SUCCESS) {
        serverClient->pendingIndCfm = 1;
    }
    return ret;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rass_pushRollingRangingData(u16 connHandle, u16 attrHdl, blt_rass_report_ranging_data_t *rangingData)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
#if (RAS_IOPTEST_ORD_008)
    //IOP test ORD-008
    static u8 iopCounter = 0;

    if (iopCounter >= 1) {
        BLC_RAS_LOG(" * * * iop just one!");
        return BLE_SUCCESS;
    }
    iopCounter++;
#endif
    switch (serverClient->procDataExchgMechanism) {
    case PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS:
    case PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS:
        // BLC_RAS_LOG("Notification");debugwait();
        return blt_rass_pushRollingRangingDataNotification(connHandle, attrHdl, rangingData);
        break;
    case PROC_DATA_EXCHG_REALTIME_INDICATIONS:
    case PROC_DATA_EXCHG_ONDEMAND_INDICATIONS:
        // BLC_RAS_LOG("Indication");debugwait();
        return blt_rass_pushRollingRangingDataIndication(connHandle, attrHdl, rangingData);
        break;
    default:
        BLC_RAS_LOG("Error! Invalid procDataExchgMechanism");
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        break;
    }
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rass_IsRealTimeReport(u16 connHandle)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    switch (serverClient->procDataExchgMechanism) {
    case PROC_DATA_EXCHG_REALTIME_INDICATIONS:
    case PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS:
        // BLC_RAS_LOG("REALTIME");debugwait();
        return BLE_SUCCESS;
        break;
    default:
        BLC_RAS_LOG("not realtime report");
        return HCI_ERR_UNSUPPORTED_FEATURE_PARAM_VALUE;
        break;
    }
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

#if (RAS_DEBUG_PRINTQUEUE)
static void blt_rass_printMsgQueue()
{
    blt_rass_msg_t *msg = (blt_rass_msg_t *)SLIST_FIRST(&rassMsgList);
    while (msg) {
        if (msg->type == RASS_MSG_TYPE_REPORT_REAL_TIME_PROCEDURE_DATA) {
            BLC_RAS_LOG("msg type %d, pData %x, pDataLen %d", msg->type, msg->realtime.pData, msg->realtime.pDataLen);
            debugwait();
        }
        msg = msg->next;
    }
}
#endif

static blt_rass_msg_t *blt_rass_mallocMsg(u16 connHandle, u8 type)
{
    blt_rass_msg_t *msg = (blt_rass_msg_t *)RASS_MALLOC(sizeof(blt_rass_msg_t));

    if (msg != NULL) {
        msg->connHandle = connHandle;
        msg->type       = type;
        SLIST_INSERT_TAIL(&rassMsgList, msg);
        return msg;
    }
    BLC_RAS_LOG(" * * * blt_rass_mallocMsg failed!");
    return NULL;
}

static blt_rass_report_realtime_procedure_data_t *blt_rass_mallocReportRealTimeProcedureDataMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_REAL_TIME_PROCEDURE_DATA);
    return (msg==NULL) ? NULL : (&msg->realtime);
}

static blt_rass_report_ondemand_procedure_data_t *blt_rass_mallocReportOnDemandProcedureDataMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_ON_DEMAND_PROCEDURE_DATA);
    return (msg==NULL) ? NULL : (&msg->ondemand);
}

static blt_rass_report_ranging_data_ready_t *blt_rass_mallocReportRangingDataReadyMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_RANGING_DATA_READY);
    // BLC_RAS_LOG("malloc datar pmsg %x type %d", msg, msg->type);debugwait();
    return (msg==NULL) ? NULL : (&msg->ready);
}

static blt_rass_report_ranging_data_overwritten_t *blt_rass_mallocProcedureDataOverwrittenMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_RANGING_DATA_OVERWRITTEN);
    // BLC_RAS_LOG("malloc overwr pmsg %x type %d", msg, msg->type);debugwait();
    return (msg==NULL) ? NULL: (&msg->overwritten);
}

static blt_rass_report_complete_record_response_t *blt_rass_mallocReportRecordResponseMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_COMPLETE_RECORD_RESPONSE);
    return (msg==NULL) ? NULL : (&msg->records);
}

static blt_rass_report_lost_segment_response_t *blt_rass_mallocReportSegmentResponseMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_COMPLETE_LOST_SEGMENT_RESPONSE);
    return (msg==NULL) ? NULL : (&msg->segment);
}

static blt_rass_report_response_code_response_t *blt_rass_mallocResponseCodeResponseMsg(u16 connHandle)
{
    blt_rass_msg_t *msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_RESPONSE_CODE_RESPONSE);
    return (msg==NULL) ? NULL : (&msg->response);
}

static ble_sts_t blt_rass_initialReportRealTimeData(u16 connHandle, u8 last, u16 rangingcounter, u8 *pData, u16 pDataLen)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    blt_rass_setRasServerBusy(serverClient, TRUE); // mark as busy

#if (RAS_REALTIME_PROT_FLOW)
    u8 wptr = serverClient->realtime_prot_flow.wptr % (CS_REALTIME_SUBEVENT_MAX);
    u8 rptr = serverClient->realtime_prot_flow.rptr % (CS_REALTIME_SUBEVENT_MAX);


    if (blt_rass_getRealTimeSubEventBusy(serverClient) == FALSE) {
        if (wptr == rptr) {
            blt_rass_setRealTimeSubEventBusy(serverClient, TRUE); // mark as busy
#endif

            blt_rass_report_realtime_procedure_data_t *realtime = blt_rass_mallocReportRealTimeProcedureDataMsg(connHandle);
            if (realtime == NULL) {
                goto failed;
            }

            realtime->isRealtime                                = TRUE;
            realtime->pData                                     = pData;
            realtime->pDataLen                                  = pDataLen;
            realtime->segmentIndex                              = 0;
            realtime->segmentOffset                             = serverClient->realtime.realtimeCurrentSegmentIndex; //0;
            realtime->MTU                                       = blt_gap_getEffectiveMTU(connHandle) - 4;
            realtime->last                                      = last;
            realtime->segmentCount                              = realtime->pDataLen / realtime->MTU + (realtime->pDataLen % realtime->MTU ? 1 : 0);
            realtime->segmentIndexEnd                           = realtime->segmentCount;
            serverClient->realtime.realtimeCurrentSegmentIndex += realtime->segmentIndexEnd;
            // BLC_RAS_LOG("blt_rass_initialReportRealTimeData pData %x, pDataLen %d, offset %d, last %d, segmCount %d, segmIndEnd %d, realtimeCurrSegmInd %d", realtime->pData, realtime->pDataLen, realtime->segmentOffset, realtime->last, realtime->segmentCount, realtime->segmentIndexEnd, serverClient->realtime.realtimeCurrentSegmentIndex);debugwait();
#if (RAS_REALTIME_PROT_FLOW)
        }
    }
    serverClient->realtime_prot_flow.subEvtData[wptr].flag.data.last             = last;
    serverClient->realtime_prot_flow.subEvtData[wptr].flag.data.procedureCounter = rangingcounter & 0xFFF;
    serverClient->realtime_prot_flow.subEvtData[wptr].pSubEvt                    = pData;
    serverClient->realtime_prot_flow.subEvtData[wptr].subEvtLen                  = pDataLen;


    u8 ptr_offset = (u8)(serverClient->realtime_prot_flow.wptr - serverClient->realtime_prot_flow.rptr);

    if (ptr_offset > CS_REALTIME_SUBEVENT_MAX) {
        //buffer over flow
    } else if (ptr_offset > 1) {
        //need check weather need to delete history data or not.
        u16 last_ranging_counter = RAS_INVALID_INDEX_PROCEDURE;
        u8 diff_ranging_counter_num = 0;
        //Counting the number of cached procedures
        for (u8 i = 0; i <= ptr_offset; i++) {
            blc_rass_realtime_prot_subevt_data_t *pSubevent = (blc_rass_realtime_prot_subevt_data_t *)(&serverClient->realtime_prot_flow.subEvtData[((serverClient->realtime_prot_flow.rptr + i) & 0xff) % (CS_REALTIME_SUBEVENT_MAX)]);
            u16                                   counter   = pSubevent->flag.data.procedureCounter;
            if(last_ranging_counter != counter){
                diff_ranging_counter_num++;
                last_ranging_counter = counter;
            }
        }
        last_ranging_counter = serverClient->realtime_prot_flow.subEvtData[rptr].flag.data.procedureCounter;
        if (diff_ranging_counter_num > 1) { //need delete
            u8 last_subevnet_wptr = 0;
            u16 penging_delete_counter = RAS_INVALID_INDEX_PROCEDURE;
            u8 penging_delete_flag = 0;
            for (u8 i = 0; i <= ptr_offset; i++) {
                blc_rass_realtime_prot_subevt_data_t *pSubevent = (blc_rass_realtime_prot_subevt_data_t *)(&serverClient->realtime_prot_flow.subEvtData[((serverClient->realtime_prot_flow.rptr + i) & 0xff) % (CS_REALTIME_SUBEVENT_MAX)]);
                u16                                   counter   = pSubevent->flag.data.procedureCounter;
                if (last_ranging_counter == counter) { //update ranging counter,counter +1
                    pSubevent->flag.data.procedureCounter++;
                    if (pSubevent->flag.data.last) {
                        last_subevnet_wptr = ((serverClient->realtime_prot_flow.rptr + i) & 0xff);
                    }
                }
                else{
                    if ((penging_delete_flag == 0) || (counter == penging_delete_counter)) { //delete
                        penging_delete_flag = 1;
                        penging_delete_counter = counter;
                        if (pSubevent->pSubEvt != NULL) {
                            free_nonreten(pSubevent->pSubEvt);
                            pSubevent->pSubEvt = NULL;
                        }
                    } else{ //cur procedure,need update wptr
                        serverClient->realtime_prot_flow.wptr                                            = (last_subevnet_wptr + 1) & 0xff;
                        u8 new_wptr                                                                      = serverClient->realtime_prot_flow.wptr % (CS_REALTIME_SUBEVENT_MAX);
                        serverClient->realtime_prot_flow.subEvtData[new_wptr].flag.data.last             = last;
                        serverClient->realtime_prot_flow.subEvtData[new_wptr].flag.data.procedureCounter = rangingcounter & 0xFFF;
                        serverClient->realtime_prot_flow.subEvtData[new_wptr].pSubEvt                    = pData;
                        serverClient->realtime_prot_flow.subEvtData[new_wptr].subEvtLen                  = pDataLen;
                    }
                }
            }
        }
    }
    serverClient->realtime_prot_flow.wptr++;
#endif

    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

//startSegment and endSegment need to be stored for active ondemand procedure
static ble_sts_t blt_rass_initialOnDemandProcedureData(u16 connHandle, u16 rangingCounter, u8 *pData, u16 pDataLen, u8 lost, u8 *startSegment, u8 *endSegment)
{
    blt_rass_report_ondemand_procedure_data_t *ondemand = blt_rass_mallocReportOnDemandProcedureDataMsg(connHandle);
    if (ondemand == NULL) {
        return PRF_ERR_INVALID_PARAMETER;
    }

    ondemand->pData                                     = pData;
    ondemand->pDataLen                                  = pDataLen;
    ondemand->segmentIndex                              = 0;
    ondemand->segmentOffset                             = 0;
    ondemand->MTU                                       = blt_gap_getEffectiveMTU(connHandle) - 4; // - sizeof(rangingCounter);
    BLC_RAS_LOG("blt_rass_initialOnDemandProcedureData MTU: <%d>", ondemand->MTU);
    ondemand->segmentCount   = ondemand->pDataLen / ondemand->MTU + (ondemand->pDataLen % ondemand->MTU ? 1 : 0);
    ondemand->rangingCounter = rangingCounter;
    ondemand->lost           = lost;
    if (lost == RAS_ONDEMAND_ALL) {
        *startSegment             = blt_ras_indexToRollingSegment(ondemand->segmentIndex, ondemand->segmentCount);
        *endSegment               = blt_ras_indexToRollingSegment(ondemand->segmentCount - 1, ondemand->segmentCount);
        ondemand->segmentIndexEnd = ondemand->segmentCount;
        ondemand->segmentStart    = *startSegment;
        ondemand->segmentEnd      = *endSegment;
    } else {
        ondemand->segmentStart = *startSegment; //needed for reporting the info on CompleteLostProcedureSegmentResponse
        if (*endSegment != RAS_LOST_SEGMENT_WILDCARD) {
            ondemand->segmentEnd = *endSegment;
        } else {
            BLC_RAS_LOG("blt_rass_initialOnDemandProcedureData wildcard trigger: %x", *endSegment);
            ondemand->segmentEnd = blt_ras_indexToRollingSegment(ondemand->segmentCount - 1, ondemand->segmentCount);
        }

        ondemand->segmentIndex    = blt_ras_rollingSegmentToIndex(ondemand->segmentStart, ondemand->segmentCount);
        ondemand->segmentIndexEnd = blt_ras_rollingSegmentToIndex(ondemand->segmentEnd, ondemand->segmentCount) + 1; //+1 is there to avoid complex logic on end condition in blt_rass_dealReportOnDemandProcedureData
        BLC_RAS_LOG("blt_rass_initialOnDemandProcedureData startIndex: %d, startS %d, endIndex %d, endS %d, segmEnd %d", ondemand->segmentIndex, *startSegment, ondemand->segmentIndexEnd, *endSegment, ondemand->segmentEnd);
        debugwait();
    }
    return BLE_SUCCESS;
}

static ble_sts_t blt_rass_initialProcedureDataReady(u16 connHandle, u16 rangingCounter)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    if (serverClient->ondemandProcedureCccValue > 0) {
        blt_rass_report_ranging_data_ready_t *ready = blt_rass_mallocReportRangingDataReadyMsg(connHandle);
        if (ready == NULL) {
            goto failed;
        }
        ready->rangingCounter                       = rangingCounter;
        BLC_RAS_LOG("msg Procedure Data Ready: rangingCounter<%d>", ready->rangingCounter);
    } else {
        BLC_RAS_LOG("ERROR msg Procedure Data Ready in wrong mode: proc<%d>, %d, %d", rangingCounter, serverClient->ondemandProcedureCccValue, serverClient->realtimeProcedureCccValue);
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rass_initialProcedureDataOverwritten(u16 connHandle, u16 rangingCounter)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    if (serverClient->ondemandProcedureCccValue > 0) {
        blt_rass_report_ranging_data_overwritten_t *overwritten = blt_rass_mallocProcedureDataOverwrittenMsg(connHandle);
        if (overwritten == NULL) {
            goto failed;
        }
        overwritten->rangingCounter                             = rangingCounter;
        BLC_RAS_LOG("msg Procedure Data Overwritten: rangingCounter<%d>", overwritten->rangingCounter);
    } else {
        BLC_RAS_LOG("ERROR msg Procedure Data Overwritten in wrong mode: proc<%d>, %d, %d", rangingCounter, serverClient->ondemandProcedureCccValue, serverClient->realtimeProcedureCccValue);
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

static ble_sts_t blt_rass_initialCompleteProcedureDataResponse(u16 connHandle, u16 rangingCounter)
{
    blt_rass_report_complete_record_response_t *records = blt_rass_mallocReportRecordResponseMsg(connHandle);
    if (records == NULL) {
        return PRF_ERR_INVALID_PARAMETER;
    }
    records->rangingCounter                             = rangingCounter;
    return BLE_SUCCESS;
}

static ble_sts_t blt_rass_initialCompleteLostProcedureSegmentResponse(u16 connHandle, u16 rangingCounter, u16 segmentStart, u16 segmentEnd)
{
    blt_rass_report_lost_segment_response_t *segment = blt_rass_mallocReportSegmentResponseMsg(connHandle);
    if (segment == NULL) {
        return PRF_ERR_INVALID_PARAMETER;
    }
    segment->rangingCounter                          = rangingCounter;
    segment->segmentStart                            = segmentStart;
    segment->segmentEnd                              = segmentEnd;
    return BLE_SUCCESS;
}

static ble_sts_t blt_rass_initialResponseCode(u16 connHandle, blt_ras_response_enum responseCode)
{
    blt_rass_report_response_code_response_t *response = blt_rass_mallocResponseCodeResponseMsg(connHandle);
    if (response == NULL) {
        return PRF_ERR_INVALID_PARAMETER;
    }
    response->responseCode                             = responseCode;
    return BLE_SUCCESS;
}

static blt_ras_response_enum blt_rass_dealError(blt_rass_msg_t *msg)
{
    (void)msg;
    BLC_RAS_LOG("%s reached!", __FUNCTION__); // major error if this function gets called at any time
    return CS_RAS_SUCCESS;
}

static blt_ras_response_enum blt_rass_dealReportRealTimeData(blt_rass_msg_t *msg)
{
    u16                       connHandle   = msg->connHandle;
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }

    blt_rass_report_realtime_procedure_data_t *realtime = &msg->realtime;
    if (blt_rass_pushRollingRangingData(connHandle, RASS_REAL_TIME_DATA_HANDLE, realtime) == BLE_SUCCESS) {
        realtime->segmentIndex++;
        if (realtime->segmentIndex == realtime->segmentIndexEnd) {
            BLC_RAS_LOG("Completed realtime data chunk");
            if (realtime->last) {
                serverClient->realtime.realtimeCurrentSegmentIndex = 0;
#if (!RAS_REALTIME_PROT_FLOW)
                blt_rass_setRasServerBusy(serverClient, FALSE); //sending data completed, clear busy flag
#endif
            }
            if (realtime->pData != NULL) {
                BLC_RAS_LOG("Freeing realtime prot pData %x, len %d", realtime->pData, realtime->pDataLen);
                debugwait();
                free_nonreten(realtime->pData);
                realtime->pData = NULL;
            }
#if (RAS_REALTIME_PROT_FLOW)
            serverClient->realtime_prot_flow.rptr++;

            if (serverClient->realtime_prot_flow.wptr != serverClient->realtime_prot_flow.rptr) {
                u8                                    rptr      = serverClient->realtime_prot_flow.rptr % (CS_REALTIME_SUBEVENT_MAX);
                blc_rass_realtime_prot_subevt_data_t *pSubevent = (blc_rass_realtime_prot_subevt_data_t *)(&serverClient->realtime_prot_flow.subEvtData[rptr]);
                blt_rass_setRealTimeSubEventBusy(serverClient, TRUE); // mark as busy
                blt_rass_setRasServerBusy(serverClient, TRUE);        // mark as busy
                realtime                  = blt_rass_mallocReportRealTimeProcedureDataMsg(connHandle);
                if (realtime == NULL) {
                    goto failed;
                }
                realtime->isRealtime      = TRUE;
                realtime->pData           = pSubevent->pSubEvt;
                realtime->pDataLen        = pSubevent->subEvtLen;
                realtime->segmentIndex    = 0;
                realtime->segmentOffset   = serverClient->realtime.realtimeCurrentSegmentIndex; //0;
                realtime->MTU             = blt_gap_getEffectiveMTU(connHandle) - 4;
                realtime->last            = pSubevent->flag.data.last & 0x01;
                realtime->segmentCount    = realtime->pDataLen / realtime->MTU + (realtime->pDataLen % realtime->MTU ? 1 : 0);
                realtime->segmentIndexEnd = realtime->segmentCount;
                serverClient->realtime.realtimeCurrentSegmentIndex += realtime->segmentIndexEnd;
            } else {
                if (realtime->last) {
                    blt_rass_setRasServerBusy(serverClient, FALSE); //sending data completed, clear busy flag
                }
                blt_rass_setRealTimeSubEventBusy(serverClient, FALSE);
            }
#endif
            return CS_RAS_SUCCESS;
        }
    }
    return CS_RAS_REPEAT;
failed:
    return CS_RAS_INVALID_PARAMETER;
}

static blt_ras_response_enum blt_rass_dealReportOnDemandProcedureData(blt_rass_msg_t *msg)
{
#if (RAS_IOPTEST_ORD_007)
    //IOP test ORD-007
    return CS_RAS_SUCCESS;
#endif

    u16                       connHandle   = msg->connHandle;
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    if (!serverClient) {
        goto failed;
    }
    blt_rass_report_ondemand_procedure_data_t *ondemand = &msg->ondemand;
    if (blt_rass_pushRollingRangingData(connHandle, RASS_ON_DEMAND_DATA_HANDLE, ondemand) == BLE_SUCCESS) {
        ondemand->segmentIndex++;
#if(RAS_TIMEOUT_EN)
        blt_rass_refreshTimeout(connHandle, msg->ondemand.rangingCounter); //delay triggering timeout, as we are busy with sending
#endif
        BLC_RAS_LOG("dealReport segmIndex %d, segmIndexEnd %d, segmStart %d, segmEnd %d", ondemand->segmentIndex, ondemand->segmentIndexEnd, ondemand->segmentStart, ondemand->segmentEnd);
        debugwait();
        //stop if the index has reached the end or if there is just one, then stop now, as there is just one to send
        if ((ondemand->segmentIndex == ondemand->segmentIndexEnd) || (ondemand->segmentStart == ondemand->segmentEnd)) {
            if (ondemand->lost == RAS_ONDEMAND_LOST) {
                BLC_RAS_LOG("msg Complete Lost Segment Rsp: rc:%d, st: %x end:%x, cnt:%d", msg->ondemand.rangingCounter, msg->ondemand.segmentStart, msg->ondemand.segmentEnd, msg->ondemand.segmentCount);//debugwait();
                blt_rass_setRasServerBusy(serverClient, FALSE); //sending data completed, clear busy flag
                //blt_rass_initialCompleteLostProcedureSegmentResponse(connHandle, msg->ondemand.rangingCounter, msg->ondemand.segmentStart, msg->ondemand.segmentEnd); //ES-26224, ES-26235
                u8 segmentStartIndex = blt_ras_rollingSegmentToIndex(msg->ondemand.segmentStart, msg->ondemand.segmentCount);
                u8 segmentEndIndex = blt_ras_rollingSegmentToIndex(msg->ondemand.segmentEnd, msg->ondemand.segmentCount);
                blt_rass_initialCompleteLostProcedureSegmentResponse(connHandle, msg->ondemand.rangingCounter, segmentStartIndex, segmentEndIndex);
                return CS_RAS_SUCCESS;
            } else {
                BLC_RAS_LOG("msg Complete Procedure Data Response: rangingCounter<%d>", msg->ondemand.rangingCounter);
                debugwait();
                blt_rass_setRasServerBusy(serverClient, FALSE); //sending data completed, clear busy flag
                //IOP test timeout test comment this out //ORD-008
#if(TTF_EN)
                if(!serverClient->stopRspEnabled) {
                    blt_rass_initialCompleteProcedureDataResponse(connHandle, msg->ondemand.rangingCounter);
                }
#else
                blt_rass_initialCompleteProcedureDataResponse(connHandle, msg->ondemand.rangingCounter);
#endif
                return CS_RAS_SUCCESS;
            }
        }
    }
    return CS_RAS_REPEAT;
failed:
    return CS_RAS_INVALID_PARAMETER;
}

static blt_ras_response_enum blt_rass_dealRangingDataReady(blt_rass_msg_t *msg)
{
    if (blt_rass_reportRangingDataReady(msg->connHandle, &msg->ready) == BLE_SUCCESS) {
        return CS_RAS_SUCCESS;
    }
    // BLC_RAS_LOG("send Procedure Data Ready Indication Fail");
    return CS_RAS_REPEAT;
}

static blt_ras_response_enum blt_rass_dealRangingDataOverwritten(blt_rass_msg_t *msg)
{
    u8 ret = blt_rass_reportRangingDataOverwritten(msg->connHandle, &msg->overwritten);

    if (ret == BLE_SUCCESS) {
        BLC_RAS_LOG("send Procedure Data Overwritten rangingCounter %d", msg->overwritten.rangingCounter);
        return CS_RAS_SUCCESS;
    }
    // BLC_RAS_LOG("send Procedure Data Overwritten Indication Fail by 0x%X", ret);
    return CS_RAS_REPEAT;
}

static blt_ras_response_enum blt_rass_dealReportRecordsResponse(blt_rass_msg_t *msg)
{
    u16                                         connHandle = msg->connHandle;
    blt_rass_report_complete_record_response_t *records    = &msg->records;
    u8                                          ret        = blt_rass_sendCompleteReportRecordsResponse(connHandle, records->rangingCounter);
    if (ret == BLE_SUCCESS) {
        BLC_RAS_LOG("send Complete Procedure Data Response: Ranging Counter<%d>", records->rangingCounter);
        return CS_RAS_SUCCESS;
    }
    // BLC_RAS_LOG("send Complete Procedure Data Response Fail by 0x%X", ret);   //This may be caused by not enough ACL Tx Fifo Num
    return CS_RAS_REPEAT;
}

static blt_ras_response_enum blt_rass_dealRecordSegmentResponse(blt_rass_msg_t *msg)
{
    u16                                      connHandle = msg->connHandle;
    blt_rass_report_lost_segment_response_t *segment    = &msg->segment;

    if (blt_rass_sendCompleteRecordsSegmentResponse(connHandle, segment->rangingCounter, segment->segmentStart, segment->segmentEnd) == BLE_SUCCESS) {
        BLC_RAS_LOG("send Complete Lost Procedure Segment Response: rangingCounter<%d>,startSegment<%x>,endSegment<%x>", segment->rangingCounter, segment->segmentStart, segment->segmentEnd);
        return CS_RAS_SUCCESS;
    }
    // BLC_RAS_LOG("send Complete Lost Procedure Segment Response Fail");
    return CS_RAS_REPEAT;
}

static blt_ras_response_enum blt_rass_dealResponseCodeResponse(blt_rass_msg_t *msg)
{
    u16                                       connHandle = msg->connHandle;
    blt_rass_report_response_code_response_t *response   = &msg->response;
    u8                                        ret        = blt_rass_sendResponseCodeResponse(connHandle, response->responseCode);
    if (ret == BLE_SUCCESS) {
        BLC_RAS_LOG("send Response Code Response: responseCode<%d>", response->responseCode);
        return CS_RAS_SUCCESS;
    }
    // BLC_RAS_LOG("send Response Code Response: responseCode Fail by 0x%X", ret);
    return CS_RAS_REPEAT;
}

static ble_sts_t blt_rass_clearQueue(u16 connHandle)
{
#if 0
    blt_rass_msg_t* msg = (blt_rass_msg_t*)SLIST_FIRST(&rassMsgList);
    blt_rass_msg_t* next;
    while(msg) {
        if(msg->connHandle == connHandle) {
            if(msg->type == RASS_MSG_TYPE_REPORT_REAL_TIME_PROCEDURE_DATA || msg->type == RASS_MSG_TYPE_REPORT_ON_DEMAND_PROCEDURE_DATA) {
                blt_rass_report_realtime_procedure_data_t* realtime = &msg->realtime;
                if(realtime->pData != NULL) {
                    free_nonreten(realtime->pData);
                    realtime->pData = NULL;
                }
            }
            msg->type = RASS_MSG_TYPE_NULL;
            next = msg->next;
            SLIST_DELETE_NODE(&rassMsgList, msg);
            RASS_FREE(msg);
            msg = next;
        }
        else {
            msg = msg->next;
        }
    }
#else
    struct single_list_node *cur;
    struct single_list_node *tmp;
    SLIST_FOREACH_SAFE(cur, &rassMsgList, next, tmp)
    {
        blt_rass_msg_t *msg = (blt_rass_msg_t *)cur;
        if (msg && (msg->connHandle == connHandle)) {
            if (msg->type == RASS_MSG_TYPE_REPORT_REAL_TIME_PROCEDURE_DATA || msg->type == RASS_MSG_TYPE_REPORT_ON_DEMAND_PROCEDURE_DATA) {
                blt_rass_report_realtime_procedure_data_t *realtime = &msg->realtime;
                if (realtime->pData != NULL) {
                    free_nonreten(realtime->pData);
                    realtime->pData = NULL;
                }
            }
            RASS_FREE(msg);
        }
    }
    rassMsgList.slh_first = NULL;
#endif
    return BLE_SUCCESS;
}

/**
 * @brief       ranging profile server main loop function.
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
static int blt_rass_loop(u16 connHandle)
{
#if (RAS_TIMEOUT_EN)
    blt_rass_checkForTimeout(connHandle);
#else
    (void)connHandle;
#endif
    blt_rass_msg_t *msg = (blt_rass_msg_t *)SLIST_FIRST(&rassMsgList);
    if (msg) {
#if (RAS_DEBUG_PRINTQUEUE)
        blt_rass_printMsgQueue();                      //for debugging realtime
#endif
        blt_ras_response_enum result = CS_RAS_SUCCESS; //1
        if (msg->type <= ARRAY_SIZE(dealMsgCb)) {
            result = dealMsgCb[msg->type](msg);
        }
        if (result) {
            msg->type = RASS_MSG_TYPE_NULL;
            SLIST_DELETE_HEAD(&rassMsgList);
            RASS_FREE(msg);
        } else {
#if OS_SUP_EN
            if (blt_os_giveSem_cb) {
                blt_os_giveSem_cb();
            }
#endif
        }
    }
    return BLE_SUCCESS;
}

ble_sts_t blt_rass_newProcedure(u16 connHandle)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((serverClient == NULL) || (rasDataset == NULL)) {
        goto failed;
    }
    switch (serverClient->procDataExchgMechanism) {
    case PROC_DATA_EXCHG_LOCAL:
        break;
    case PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS:
    case PROC_DATA_EXCHG_REALTIME_INDICATIONS:
    {
        BLC_RAS_LOG("blt_rass_newProcedure - Clear - only on peripheral");
        debugwait();
        blt_ras_clearAndInitializeLocal(rasDataset);
    } break;
    case PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS:
    case PROC_DATA_EXCHG_ONDEMAND_INDICATIONS:
        break;
    default:
        break;
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rass_procedureDataReadyIntermediate(u16 connHandle, blt_ras_proc_ctrl_t *procCtrl, u8 last)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }
    switch (serverClient->procDataExchgMechanism) {
    case PROC_DATA_EXCHG_LOCAL:
        break;
    case PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS:
    case PROC_DATA_EXCHG_REALTIME_INDICATIONS:
    {
        blc_rass_subevt_data_t *procSubEvt = (blc_rass_subevt_data_t *)&(procCtrl->subEvtData[procCtrl->subEvtNum]);
        BLC_RAS_LOG("blt_rass_procedureDataReadyIntermediate pData: %x, dataLen %d", procSubEvt->pSubEvt, procSubEvt->subEvtLen);
        blc_rass_subevt_data_t protSubEvt;
        protSubEvt.pSubEvt = (u8 *)malloc_nonreten(procSubEvt->subEvtLen); //freed in blt_rass_dealReportRealTimeData
        if (protSubEvt.pSubEvt == NULL) {
            BLC_RAS_LOG("ERROR! Out of memory spot 008");
            if (procSubEvt->pSubEvt != NULL) {
                free_nonreten(procSubEvt->pSubEvt);
                procSubEvt->pSubEvt = NULL;
            }
            goto failed2;
        }
        BLC_RAS_LOG("intermediate proc pData: %x, dataLen %d", procSubEvt->pSubEvt, procSubEvt->subEvtLen);
        blt_rass_procedureSubeventToProtocolSubevent(&protSubEvt, procSubEvt, &procCtrl->procedureHead, (procCtrl->subEvtNum == 0), rasDataset);
        BLC_RAS_LOG("intermediate prot pData: %x, dataLen %d", protSubEvt.pSubEvt, protSubEvt.subEvtLen);
        blt_rass_initialReportRealTimeData(connHandle, last, procCtrl->rangingCounter, protSubEvt.pSubEvt, protSubEvt.subEvtLen); //4: blc_rass_prot_head_t -> rangingCounter
        //free realtime subevent data after procedure to protocol.
        if (procSubEvt->pSubEvt != NULL) {
            free_nonreten(procSubEvt->pSubEvt);
            procSubEvt->pSubEvt = NULL;
        }
        else{
            BLC_RAS_LOG("ERROR!");
        }
    } break;
    case PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS:
    case PROC_DATA_EXCHG_ONDEMAND_INDICATIONS:
        break;
    default:
        break;
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
failed2:
    return HCI_ERR_LIMIT_REACHED;
}

ble_sts_t blt_rass_procedureDataReady(u16 connHandle, u16 rangingCounter)
{
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }

    switch (serverClient->procDataExchgMechanism) {
    case PROC_DATA_EXCHG_LOCAL:
    {
        blt_rass_procedure_query_result_t res = blt_rass_procedureQuery(rasDataset, rangingCounter);
        blc_rasc_local_ranging_data_evt_t evt = {
            .connHandle = connHandle,
            .dataPtr    = res.procData->proc.pData,
            .dataLen    = res.procData->proc.dataLen,
        };
        blt_prf_sendEvent(connHandle, CS_EVT_LOCAL_RANGING_DATA, &evt, sizeof(evt));
    } break;
    case PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS:
    case PROC_DATA_EXCHG_REALTIME_INDICATIONS:
    {
        /* buffer free immediately after subevent procedure data to protocol data*/
        BLC_RAS_LOG("blt_rass_procedureDataReady - Clear - only on peripheral");
        debugwait();
        blt_ras_clearAndInitializeLocal(rasDataset); //can be cleared at this point
        
    } break;
    case PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS:
    case PROC_DATA_EXCHG_ONDEMAND_INDICATIONS:
    {

        blt_rass_procedure_query_result_t res = blt_rass_procedureQuery(rasDataset, rangingCounter);
        BLC_RAS_LOG("query res: pRes %x, status %d, index %x, res.procData %x pData %x", &res, res.status, res.index, res.procData, res.procData->proc.pData);
        debugwait();
        if (res.status == 0) {
            BLC_RAS_LOG("ranging counter not found");
            blt_ras_procedureDeleteLocal(rasDataset, rangingCounter);  //needed? query did not find it
        } else {
            int ret = ATT_SUCCESS;
            blt_ras_prot_ctrl_t tempProtCtrl = {0};
            //prot will always fit in size of proc - allocated temporarily before placing in final buffer below
            tempProtCtrl.prot.pData = (u8 *)malloc_nonreten(res.procData->proc.dataLen); //blc_rass_proc_data_t * //PROCEDURE_DATA_LEN
            if (tempProtCtrl.prot.pData == NULL) {
                BLC_RAS_LOG("ERROR! Out of memory spot 010");
                blt_ras_procedureDeleteLocal(rasDataset, rangingCounter); //cleanup
                ret = ATT_ERR_UNLIKELY_ERR;
                BLC_RAS_LOG("blt_ras_procedureDeleteLocal error! %d", ret);
                goto failed;
            }

            //Overwrite handler on prot side
            if (serverClient->protStoredNum >= RAS_PROCEDURE_COUNT) {
                BLC_RAS_LOG("Prot overwritten storedNum %d", serverClient->protStoredNum);

                blt_ras_prot_ctrl_t *protCtrl = (blt_ras_prot_ctrl_t *)&(serverClient->protCtrl[0]); //0 index is the oldest record
                u16 overwrittenRangingCounter = protCtrl->rangingCounter;
                BLC_RAS_LOG("protCtrl: %x, overwrittenRangingCounter: %d", protCtrl, overwrittenRangingCounter);

                blc_rass_procedureDataOverwritten(connHandle, overwrittenRangingCounter);
                //lets delete here instead of when the overwritten msg gets sent/confirmed, as we need space now
                blt_rass_protocolDeleteLocal(serverClient, overwrittenRangingCounter);
            }
            BLC_RAS_LOG("blt_rass_procedureDataReady protStoredNum %d", serverClient->protStoredNum);
            s8 protIndex = serverClient->protStoredNum; //res.index previously

            blt_rass_procedureDataToProtocolData(&tempProtCtrl, res.procData, rasDataset);
#if (RAS_DEBUG_PRINTBUFFERS)
            // debug only
            BLC_RAS_LOG("tempProtCtrl protocol: %x, %d", tempProtCtrl.prot.pData, tempProtCtrl.prot.dataLen);
            log_buffer(tempProtCtrl.prot.pData, 20);
            ///
#endif
            blt_ras_procedureDeleteLocal(rasDataset, rangingCounter);  //we can free after procedure data converstion to protocol data is finished.
            ret = blt_rass_allocateProtData(serverClient, protIndex, tempProtCtrl.prot.dataLen); //allocate final size & serverClient->protStoredNum++
            if (ret == ATT_ERR_UNLIKELY_ERR) {
                free_nonreten(tempProtCtrl.prot.pData); //cleanup
                tempProtCtrl.prot.pData = NULL;
                BLC_RAS_LOG("blt_ras_procedureDeleteLocal error! %d", ret);
                goto failed;
            }
            memcpy(serverClient->protCtrl[protIndex].prot.pData, tempProtCtrl.prot.pData, tempProtCtrl.prot.dataLen); //copy
            serverClient->protCtrl[protIndex].prot.dataLen = tempProtCtrl.prot.dataLen;
#if (RAS_DEBUG_PRINTBUFFERS)
            // debug only
            BLC_RAS_LOG("serverClient->protCtrl[%d]: %x, %d", protIndex, serverClient->protCtrl[protIndex].prot.pData, serverClient->protCtrl[protIndex].prot.dataLen);
            log_buffer(serverClient->protCtrl[protIndex].prot.pData, 20);
            ///
#endif
            free_nonreten(tempProtCtrl.prot.pData); //release temporary buffer
            tempProtCtrl.prot.pData = NULL;

            serverClient->protCtrl[protIndex].rangingCounter = rangingCounter;
            //serverClient->protStoredNum = serverClient->protStoredNum + 1; //done in blt_rass_allocateProtData
            BLC_RAS_LOG("blt_rass_procedureDataReady protIndex %d, protStoredNum %d", protIndex, serverClient->protStoredNum);
            debugwait();
            blt_rass_initialProcedureDataReady(connHandle, rangingCounter);
            BLC_RAS_LOG("blt_rass_procedureDataReady ret %d", ret);
        }
    } break;
    default:
        break;
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blc_rass_procedureDataOverwritten(u16 connHandle, u16 rangingCounter)
{
    blt_rass_initialProcedureDataOverwritten(connHandle, rangingCounter);
    return BLE_SUCCESS;
}


#if (RAS_TIMEOUT_EN)
ble_sts_t blt_rass_checkForTimeout(u16 connHandle)
{
    // TODO: Does connHandle belong to the server
    blt_ras_dataset_t        *rasDataset   = blc_ras_getDataset(connHandle);
    blt_rass_server_client_t *serverClient = blt_rass_getServerClientsInst(connHandle);

    if ((!serverClient) || (!rasDataset)) {
        goto failed;
    }
    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    if (dataCtrl->storedNum == 0) {
        // nothing to do yet
        return BLE_SUCCESS;
    }

    if ((serverClient->procDataExchgMechanism != PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS) &&
        (serverClient->procDataExchgMechanism != PROC_DATA_EXCHG_ONDEMAND_INDICATIONS)) {
        // This timeout works only for on-demand mode.
        return BLE_SUCCESS;
    }


    for (u8 k = 0; k != dataCtrl->storedNum; k++) {
        blt_ras_proc_ctrl_t *procCtrl   = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k]);
        u32                  delta_time = stimer_get_tick() - procCtrl->timestamp;
        if ((procCtrl->timestamp != 0) && (delta_time > PROCEDURE_DATA_TIMEOUT)) { // timestamp = 0 means timeout disabled

            BLC_RAS_LOG("Server: Timeout hit for Data connh:%d, RangCtr:%d, timestamp:%lu curr_time:%lu",
                        connHandle,
                        procCtrl->rangingCounter,
                        procCtrl->timestamp,
                        stimer_get_tick());
            // Delete procedure
            blt_ras_procedureDeleteLocal(rasDataset, procCtrl->rangingCounter);
            return BLE_SUCCESS;
        }
    }
    return BLE_SUCCESS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rass_refreshTimeout(u16 connHandle, u16 rangingCounter)
{
    blt_ras_dataset_t* rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    for(u8 k=0; k!= RAS_PROCEDURE_COUNT; k++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k]);
        if (procCtrl->rangingCounter == rangingCounter) {
            procCtrl->timestamp = stimer_get_tick();
            BLC_RAS_LOG("Server: Timeout refreshed for conn:%x rangCtr:%d", connHandle, rangingCounter);
            return BLE_SUCCESS;
        }
    }
    BLC_RAS_LOG("FAILED to refresh timeout for for conn:%x rangCtr:%d. Not Found", connHandle, rangingCounter);
    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}

ble_sts_t blt_rass_clearTimeout(u16 connHandle, u16 rangingCounter)
{
    blt_ras_dataset_t *rasDataset = blc_ras_getDataset(connHandle);
    if (!rasDataset) {
        goto failed;
    }

    blt_ras_data_ctrl_t *dataCtrl = (blt_ras_data_ctrl_t *)&(rasDataset->dataCtrl);

    for (u8 k = 0; k != RAS_PROCEDURE_COUNT; k++) {
        blt_ras_proc_ctrl_t *procCtrl = (blt_ras_proc_ctrl_t *)&(dataCtrl->procCtrl[k]);
        if (procCtrl->rangingCounter == rangingCounter) {
            procCtrl->timestamp = 0;
            BLC_RAS_LOG("Server: Timeout disabled for conn:%x rangCtr:%d", connHandle, rangingCounter);
            return BLE_SUCCESS;
        }
    }
    BLC_RAS_LOG("FAILED to remove timeout for for conn:%x rangCtr:%d. Not Found", connHandle, rangingCounter);
    return HCI_ERR_INVALID_HCI_CMD_PARAMS;
failed:
    return HCI_ERR_UNKNOWN_CONN_ID;
}
#endif

static blt_rass_protocol_query_result_t blc_rass_protocolQuery(blt_rass_server_client_t *serverClient, u16 rangingCounter)
{
    blt_rass_protocol_query_result_t queryData = {0};

    if (!serverClient) {
        goto failed;
    }

    for (int i = 0; i < RAS_PROCEDURE_COUNT; i++) {
        blt_ras_prot_ctrl_t *protCtrl = (blt_ras_prot_ctrl_t *)&(serverClient->protCtrl[i]);
        BLC_RAS_LOG("pProtCtrl: %x i: %d", protCtrl, i);
        debugwait();
        BLC_RAS_LOG("queryIndex pDataCtrl:%x, pProtStAddr: %x", protCtrl, protCtrl->prot.pData, protCtrl->prot.dataLen);
        debugwait();

        if (rangingCounter == protCtrl->rangingCounter) {
            queryData.status   = 1;
            queryData.index    = i;
            queryData.protData = protCtrl;

            BLC_RAS_LOG("protocolQuery rangCtr %d, protStart %x, protLen %d", protCtrl->rangingCounter, protCtrl->prot.pData, protCtrl->prot.dataLen);
            debugwait();
            break;
        }

        if (RAS_PROCEDURE_COUNT == i + 1) {
            BLC_RAS_LOG("blc_rass_protocolQuery index not found %d", rangingCounter);
        }
    }
failed:
    return queryData;
}

static att_err_t blt_rass_allocateProtData(blt_rass_server_client_t *serverClient, u8 index, u16 size)
{
    if (index >= RAS_PROCEDURE_COUNT) {
        BLC_RAS_LOG("ERROR! blt_rass_allocateProtData Index out of bounds %d", index);
        goto failed;
    }

    serverClient->protCtrl[index].prot.dataLen = 0;
    if (!serverClient->protCtrl[index].prot.pData) {
        serverClient->protCtrl[index].prot.pData = (u8 *)malloc_nonreten(size);
        if (serverClient->protCtrl[index].prot.pData == NULL) {
            BLC_RAS_LOG("ERROR! blt_rass_allocateProtData Out of memory spot 009");
            goto failed;
        }
        serverClient->protCtrl[index].rangingCounter = RAS_INVALID_INDEX_PROCEDURE;
        serverClient->protStoredNum = serverClient->protStoredNum + 1; //a record gets stored, lets increase the number
    }
    return ATT_SUCCESS;
failed:
    return ATT_ERR_UNLIKELY_ERR;
}

static void blt_rass_releaseProtData(blt_rass_server_client_t *serverClient, u8 index)
{
    blt_ras_prot_ctrl_t *protCtrl = (blt_ras_prot_ctrl_t *)&(serverClient->protCtrl[index]);
    if (protCtrl->prot.pData != NULL) {
        free_nonreten(protCtrl->prot.pData);
    }
    memset(protCtrl, 0x00, sizeof(blt_ras_prot_ctrl_t)); //serverClient->protCtrl[0].prot.pData = NULL / .prot.dataLen = 0 / .rangingCounter = 0 / .timestamp = 0
    protCtrl->rangingCounter = RAS_INVALID_INDEX_PROCEDURE;
}

static void blt_rass_releaseAllProtData(blt_rass_server_client_t *serverClient)
{
    for (u8 i = 0; i < RAS_PROCEDURE_COUNT; i++) {
        blt_rass_releaseProtData(serverClient, i);
    }
}

static blt_ras_response_enum blt_rass_protocolDeleteLocal(blt_rass_server_client_t *serverClient, u16 rangingCounter)
{
    BLC_RAS_LOG("blt_rass_protocolDeleteLocal called protStoredNum %d", serverClient->protStoredNum);
    debugwait();

    if (serverClient->protStoredNum == 0) {
        BLC_RAS_LOG("prot index delete error - empty");
        return CS_RAS_NO_RECORDS_FOUND;
    }

    if (serverClient->protStoredNum == 1) { //blt_rass_releaseProtData(serverClient);
        if (rangingCounter == serverClient->protCtrl[0].rangingCounter) {
            BLC_RAS_LOG("Prot - One record. Deleting....");
            blt_rass_releaseProtData(serverClient, 0);
            return CS_RAS_SUCCESS;
        }
        BLC_RAS_LOG("prot delete - No records found");
        return CS_RAS_NO_RECORDS_FOUND;
    }

    for (int i = 0; i < serverClient->protStoredNum; i++) {
        if (rangingCounter == serverClient->protCtrl[i].rangingCounter) {
            BLC_RAS_LOG("Prot - Record found at index %d. Deleting....", i);
            blt_rass_releaseProtData(serverClient, i);

            if (i < serverClient->protStoredNum - 1) { // there are entries following entry we remove, we need to realign them
                for (int k = i; k < serverClient->protStoredNum - 1; k++) {
                    //copy from next to current index
                    blt_ras_prot_ctrl_t *protCtrl_this = (blt_ras_prot_ctrl_t *)&(serverClient->protCtrl[k]);
                    blt_ras_prot_ctrl_t *protCtrl_next = (blt_ras_prot_ctrl_t *)&(serverClient->protCtrl[k + 1]);
                    memcpy(protCtrl_this, protCtrl_next, sizeof(blt_ras_prot_ctrl_t));

                    if (k == serverClient->protStoredNum - 2) { //optimization - only needed on final iteration
                        //clear pointer data, as the pointer information repeats in next to last and last now
                        memset(protCtrl_next, 0x00, sizeof(blt_ras_prot_ctrl_t)); //serverClient->protCtrl[0].prot.pData = NULL / .prot.dataLen = 0 / .rangingCounter = 0 / .timestamp = 0
                        protCtrl_next->rangingCounter = RAS_INVALID_INDEX_PROCEDURE;
                    }
                }
            }
            serverClient->protStoredNum = serverClient->protStoredNum - 1;
            return CS_RAS_SUCCESS;
        }
    }
    return CS_RAS_NO_RECORDS_FOUND;
}

/* PTS_TESTING */
#if(TTF_EN)
void blc_rass_iop_stopGet(u16 connHandle, bool stopGetEnabled)
{
    blt_rass_server_client_t* serverClient = blt_rass_getServerClientsInst(connHandle);

    if(!serverClient) {
        goto failed;
    }

    serverClient->stopGetEnabled = stopGetEnabled;

failed:
    return;
}

void blc_rass_iop_stopRsp(u16 connHandle, bool stopRspEnabled)
{
    blt_rass_server_client_t* serverClient = blt_rass_getServerClientsInst(connHandle);

    if(!serverClient) {
        goto failed;
    }

    serverClient->stopRspEnabled = stopRspEnabled;

failed:
    return;
}

att_err_t blc_rass_iop_issueGetReportOneRecord(u16 connHandle, u16 rangingCounter)
{
    blt_ras_cp_operand_t operand;
    operand.getOneRecord.rangingCounter = rangingCounter;
    blt_rass_recvGetReportOneRecord(connHandle, &operand);
}
#endif

#if(RAS_IOPTEST_ENABLE)
ble_sts_t blc_rass_iop_initialCompleteProcedureDataResponse(u16 connHandle, u16 rangingCounter)
{
    return blt_rass_initialCompleteProcedureDataResponse(connHandle, rangingCounter);
}

ble_sts_t blc_rass_iop_initialResponseCode(u16 connHandle, u8 responseCode)
{
    return blt_rass_initialResponseCode(connHandle, responseCode);
}

ble_sts_t blc_rass_iop_initialCompleteLostProcedureSegmentResponse(u16 connHandle, u16 rangingCounter, u16 segmentStart, u16 segmentEnd)
{
    return blt_rass_initialCompleteLostProcedureSegmentResponse(connHandle, rangingCounter, segmentStart, segmentEnd);
}

ble_sts_t blc_rass_iop_procedureDataReady(u16 connHandle, u16 rangingCounter)
{
    return blt_rass_procedureDataReady(connHandle, rangingCounter);
}
#endif
