/********************************************************************************************************
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

#if OS_SUP_EN
#include "stack/ble/os_sup/os_sup.h"
#include "stack/ble/os_sup/os_sup_stack.h"
#endif

int blt_rass_init(u8 initType, const void* param);
int blt_rass_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_rass_loop(u16 connHandle);
static int blt_rass_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen);
static void blt_rass_serviceInit(const blc_rass_regParam_t *param);

_attribute_ble_data_retention_
blc_ras_server_ctrl_t ras_server_ctrl = {
    .process = {
        .pNext = NULL,
        .id = CS_RAS_SERVER,
        .usedAclRole = 0,
        .init = blt_rass_init,
        .connect = blt_rass_connect,
        .discov = NULL,
        .loop = blt_rass_loop,
    },
};


const blc_rass_regParam_t defaultRasPrfParam = {
    .procDataExchgMechanism = PROC_DATA_EXCHG_LOCAL,
    .ras_feature.realTimeProcedureDataSupport               = 0,
    .ras_feature.getLostProcedureDataSegmentsSupport        = 1,
    .ras_feature.abortOperationSupport                      = 0,
    .ras_feature.filterProcedureDataSupport                 = 0,
    .ras_feature.pctPahseFormatSupport                      = 0,
};

/**
 * @brief       register ranging profile server controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_cs_registerRasProfileControlServer(const blc_rass_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t*)&ras_server_ctrl, param);
}

/**
 * @brief       ranging profile server get Server control instance by connect handle.
 * @param[in]   connHandle: ACL connection.
 * @return      server control instance.
 */
static blc_ras_server_t* blt_rass_getServerInst(u16 connHandle)
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
int blt_rass_init(u8 initType, const void* param)
{
    if(initType == PRF_PROC_INIT) {
        BLC_RAS_LOG("server init");
        blc_svc_addRasGroup();
        blc_svc_rasCbackRegister(blt_rass_writeCback);
        blt_rass_serviceInit(param);
    }
    return 0;
}

/**
 * @brief       ranging profile server connect/disconnect event callback function.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   connState: PRF_ACL_STATE_DISCONN/PRF_ACL_STATE_CONNECT.
 * @return      0.
 */
int blt_rass_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLC_RAS_LOG("Disconnect:0x%x", connHandle);
    } else {
        BLC_RAS_LOG("Connect:0x%x", connHandle);
    }

    return 0;
}

#define RASS_RAS_FEATURE_HANDLE                 (blt_rass_getServerInst(0xFFFF)->rasFeatureHandle)
#define RASS_LIVE_RNGING_DATA_HANDLE            (blt_rass_getServerInst(0xFFFF)->liveRangingDataHandle)
#define RASS_STORED_RANGING_DATA_HANDLE         (blt_rass_getServerInst(0xFFFF)->storedRangingDataHandle)
#define RASS_CONTROL_POINT_HANDLE               (blt_rass_getServerInst(0xFFFF)->controlPointHandle)
#define RASS_RANGING_DATA_READY_HANDLE          (blt_rass_getServerInst(0xFFFF)->rangingDataReadyHandle)
#define RASS_RANGING_DATA_OVERWRITTEN_HANDLE    (blt_rass_getServerInst(0xFFFF)->rangingDataOverwrittenHandle)

static void blt_rass_initRasFeatureChar(atts_foundCharParam_t * p, void *input)
{
    blc_ras_server_t *server = (blc_ras_server_t*)input;
    if(p->num > 0)
    {
        BLC_RAS_LOG("ERR: ras feature char too many");
        return ;
    }
    server->rasFeatureHandle = p->charHandle;
}
/**
 * @brief       ranging profile server initial live ranging data characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initLiveRangingDataChar(atts_foundCharParam_t * p, void *input)
{
    blc_ras_server_t *server = (blc_ras_server_t*)input;
    if(p->num > 0)
    {
        BLC_RAS_LOG("ERR: live ranging data char too many");
        return ;
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
            blt_os_giveSem_cb();
    }
#endif
    server->liveRangingDataHandle = p->charHandle;
}

/**
 * @brief       ranging profile server initial stored ranging data characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initStoredRangingDataChar(atts_foundCharParam_t * p, void *input)
{
    blc_ras_server_t *server = (blc_ras_server_t*)input;
    if(p->num > 0)
    {
        BLC_RAS_LOG("ERR: stored ranging data char too many");
        return ;
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
            blt_os_giveSem_cb();
    }
#endif
    server->storedRangingDataHandle = p->charHandle;
}

/**
 * @brief       ranging profile server initial control point characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initControlPointChar(atts_foundCharParam_t * p, void *input)
{
    blc_ras_server_t *server = (blc_ras_server_t*)input;
    if(p->num > 0)
    {
        BLC_RAS_LOG("ERR: control point char too many");
        return ;
    }
    server->controlPointHandle = p->charHandle;
}

/**
 * @brief       ranging profile server initial ranging data ready characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initRangingDataReadyChar(atts_foundCharParam_t * p, void *input)
{
    blc_ras_server_t *server = (blc_ras_server_t*)input;
    if(p->num > 0)
    {
        BLC_RAS_LOG("ERR: ranging data ready char too many");
        return ;
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
            blt_os_giveSem_cb();
    }
#endif
    server->rangingDataReadyHandle = p->charHandle;
}

/**
 * @brief       ranging profile server initial ranging data overwritten characteristic information.
 * @param[in]   p: characteristic information, include attribute handle, attribute value pointer, attribute value length point, characteristic number.
 * @param[in]   input: high layer input value.
 * @return      none.
 */
static void blt_rass_initRangingDataOverwrittenChar(atts_foundCharParam_t * p, void *input)
{
    blc_ras_server_t *server = (blc_ras_server_t*)input;
    if(p->num > 0)
    {
        BLC_RAS_LOG("ERR: ranging data overwritten char too many");
        return ;
    }
#if OS_SUP_EN
    if(blt_os_giveSem_cb)
    {
            blt_os_giveSem_cb();
    }
#endif
    server->rangingDataOverwrittenHandle = p->charHandle;
}


static const atts_findCharList_t rassChar[] = {
    {
        .charUuid = characteristicRasFeatureUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_rass_initRasFeatureChar,
    },

    {
        .charUuid = characteristicLiveRangingDataUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_rass_initLiveRangingDataChar,
    },
    {
        .charUuid = characteristicStoredRangingDataUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_rass_initStoredRangingDataChar,
    },
    {
        .charUuid = characteristicControlPointUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_rass_initControlPointChar,
    },
    {
        .charUuid = characteristicRangingDataReadyUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_rass_initRangingDataReadyChar,
    },
    {
        .charUuid = characteristicRangingDataOverwrittenUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_rass_initRangingDataOverwrittenChar,
    },
};

static u8* blt_rass_getRasFeature(u16 connHandle)
{
    return blc_gatts_getAttributeValueByHandle(connHandle, RASS_RAS_FEATURE_HANDLE);
}

static void blt_rass_setFeature(u8 feature)
{
    u8 *pFeature = blt_rass_getRasFeature(0xFFFF);
    if(!pFeature)       return ;

    *pFeature = feature;
}

/**
 * @brief       ranging profile server initial service value.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
static void blt_rass_serviceInit(const blc_rass_regParam_t* param)
{
    blc_ras_server_t *server = blt_rass_getServerInst(0xFFFF);
    blc_atts_findCharacteristicByServiceUuid(serviceRangingUuid, ATT_16_UUID_LEN, rassChar, ARRAY_SIZE(rassChar), server);
    CS_PRF_LOG("Handle information:");
    CS_PRF_LOG("Ras feature Handle:0x%04x", server->rasFeatureHandle);
    CS_PRF_LOG("Live ranging data Handle:0x%04x", server->liveRangingDataHandle);
    CS_PRF_LOG("Stored ranging data Handle:0x%04x", server->storedRangingDataHandle);
    CS_PRF_LOG("Control point Handle:0x%04x", server->controlPointHandle);
    CS_PRF_LOG("Ranging data ready Handle:0x%04x", server->rangingDataReadyHandle);
    CS_PRF_LOG("Ranging data overwritten Handle:0x%04x", server->rangingDataOverwrittenHandle);

    const blc_rass_regParam_t* regParam = param? (const blc_rass_regParam_t*)param: &defaultRasPrfParam;
    server->procDataExchgMechanism = regParam->procDataExchgMechanism;

    blt_rass_setFeature(regParam->ras_feature.features);

}

typedef int(*ras_cmd_cb)(u16 connHandle, ras_cp_operand_t* operand);

typedef struct __attribute__((packed)) {
    u8 opcode;
    u8 operator;
    u8 expectLen;
    ras_cmd_cb cb;
} rasCtrlLega_t;



static int blt_rass_recvGetReportOneRecords(u16 connHandle, ras_cp_operand_t* operand)
{
    u16 procedureCounter = operand->getOneRecord.procedureCounter;

    blc_rass_query_result_t res = blc_rass_procedureQueryIndexStored(procedureCounter);

    if(res.status == 0) {
        BLC_RAS_LOG("not found");
        //TODO: not found
    }
    CS_PRF_LOG("recv Get Procedure Data Command: procedureCounter<%d>", procedureCounter);
    CS_PRF_LOG("msg On-Demand Procedure Data Notify: %s", hex_to_str(res.procPtr, (res.procLen > 100) ? 100: res.procLen));
    blt_rass_initialOnDemandProcedureData(connHandle, res.procPtr, res.procLen, 0, 0 , 0);

    CS_PRF_LOG("msg Complete Procedure Data Response: Number of record<1(HC)>");
    blt_rass_initialCompleteProcedureDataResponse(connHandle, 1);
    return ATT_SUCCESS;
}

static int blt_rass_recvGetReportGreaterEqualRecords(u16 connHandle, ras_cp_operand_t* operand)
{
    (void)connHandle;
    (void)operand;
    return ATT_SUCCESS;
}

static int blt_rass_recvAckStoreOneRecords(u16 connHandle, ras_cp_operand_t* operand)
{
    (void)connHandle;
    u8 procedureCounter= operand->ackStoredOneRecord.procedureCounter;
    CS_PRF_LOG("recv ACK Procedure Data Command: Procedure Counter<%d> -> Delete", procedureCounter);
    blc_rass_procedureDeleteIndex(procedureCounter);

    return ATT_SUCCESS;
}

static int blt_rass_recvAckStoreGreaterEqualRecords(u16 connHandle, ras_cp_operand_t* operand)
{
    (void)connHandle;
    (void)operand;
    return ATT_SUCCESS;
}

static int blt_rass_recvGetRecordSegments(u16 connHandle, ras_cp_operand_t* operand)
{
    u16 procedureCounter = operand->completeRecordSegment.recordNum;

    blc_rass_query_result_t res = blc_rass_procedureQueryIndexStored(procedureCounter);

    if(res.status == 0) {
        BLC_RAS_LOG("not found");
        //TODO: not found
    }
    u16 start_segment = operand->completeRecordSegment.startSegment;
    u16 end_segment = operand->completeRecordSegment.endSegment;
    CS_PRF_LOG("recv Get Lost Procedure Segment Command: procedureCounter<%d>,startSegment<%d>,endSegment<%d>", procedureCounter, start_segment, end_segment);

    blt_rass_initialOnDemandProcedureData(connHandle, res.procPtr, res.procLen, 1, start_segment, end_segment);

    CS_PRF_LOG("msg Complete Lost Procedure Segment Response");

    blt_rass_initialCompleteLostProcedureSegmentResponse(connHandle, procedureCounter, start_segment, end_segment);


    return ATT_SUCCESS;
}

static int blt_rass_recvAbortOperation(u16 connHandle, ras_cp_operand_t* operand)
{
    (void)connHandle;
    (void)operand;
    return ATT_SUCCESS;
}

static const rasCtrlLega_t rasCtrlLega[] = {
    {RAS_CP_CMD_OPCODE_GET_REPORT_RECORDS, RAS_CP_OPERATOR_ONE_RECORD, sizeof(operands_getOneRecord_t), blt_rass_recvGetReportOneRecords},
    {RAS_CP_CMD_OPCODE_GET_REPORT_RECORDS, RAS_CP_OPERATOR_GREATER_OR_EQUAL, sizeof(operands_getMoreRecord_t), blt_rass_recvGetReportGreaterEqualRecords},
    {RAS_CP_CMD_OPCODE_ACK_STORED_RECORDS, RAS_CP_OPERATOR_ONE_RECORD, sizeof(operands_ackStoredOneRecord_t), blt_rass_recvAckStoreOneRecords},
    {RAS_CP_CMD_OPCODE_ACK_STORED_RECORDS, RAS_CP_OPERATOR_GREATER_OR_EQUAL, sizeof(operands_ackStoredMoreRecord_t), blt_rass_recvAckStoreGreaterEqualRecords},
    {RAS_CP_CMD_OPCODE_GET_RECORD_SEGMENT, RAS_CP_OPERATOR_NULL, sizeof(operands_getRecordSegments_t), blt_rass_recvGetRecordSegments},
    {RAS_CP_CMD_OPCODE_ABORD_OPERATION, RAS_CP_OPERATOR_NULL, sizeof(operands_abortOperation_t), blt_rass_recvAbortOperation},

};

static int blt_rass_recvRasControlPointCommand(u16 connHandle, u8* writeValue, u16 valueLen)
{
    if(valueLen < RAS_CP_MSG_HEADER_SIZE)   return ATT_ERR_INVALID_PDU;
    ras_cp_msg_t* msg = (ras_cp_msg_t*)writeValue;

    for(size_t i=0; i<ARRAY_SIZE(rasCtrlLega); i++)
    {
        if(msg->opcode == rasCtrlLega[i].opcode &&
                msg->operator == rasCtrlLega[i].operator &&
                (valueLen-RAS_CP_MSG_HEADER_SIZE) == rasCtrlLega[i].expectLen)
        {
            return rasCtrlLega[i].cb(connHandle, &msg->operand);
        }
    }
    return ATT_ERR_INVALID_PDU;
}

static int blt_rass_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    (void)opcode;
    blc_ras_server_t *server = blt_rass_getServerInst(0xFFFF);

    if(attrHandle == server->controlPointHandle)
    {
        return blt_rass_recvRasControlPointCommand(connHandle, writeValue, valueLen);
    }

    return ATT_ERR_INVALID_HANDLE;
}

typedef struct __attribute__((packed)) {
    u8 attOp;
    u16 attrHdl;
    struct {
        u8 firstSegment:    1;
        u8 lastSegment:     1;
        u8 segmentCounter:  6;
    };
} ranging_data_head_t;

static int blt_rass_pushNotifyData(u16 connHandle, ranging_data_head_t* head, u8* payload, int payloadLen)
{
    return blt_l2cap_pushData_2_controller(connHandle, L2CAP_CID_ATTR_PROTOCOL, (u8*)head, sizeof(ranging_data_head_t), payload, payloadLen);
}

static int blt_rass_reportRangingDataReady(u16 connHandle, rass_report_rangingDataReady_t* format)
{
    CS_PRF_LOG("send Procedure Data Ready Indication: %s", hex_to_str(format, sizeof(rass_report_rangingDataReady_t)));
    return blc_atts_sendHandleValueIndicate(connHandle, RASS_RANGING_DATA_READY_HANDLE, (u8*)format, sizeof(rass_report_rangingDataReady_t));
//  return blc_atts_sendHandleValueNotify(connHandle, RASS_RANGING_DATA_READY_HANDLE, (u8*)format, sizeof(rass_report_rangingDataReady_t));
}

static int blt_rass_reportRangingDataOverwritten(u16 connHandle, rass_report_rangingDataOverwritten_t* format)
{
    CS_PRF_LOG("send Procedure Data Overwritten Indication: %s", hex_to_str(format, sizeof(rass_report_rangingDataOverwritten_t)));
    return blc_atts_sendHandleValueIndicate(connHandle, RASS_RANGING_DATA_OVERWRITTEN_HANDLE, (u8*)format, sizeof(rass_report_rangingDataOverwritten_t));
//  return blc_atts_sendHandleValueNotify(connHandle, RASS_RANGING_DATA_OVERWRITTEN_HANDLE, (u8*)format, sizeof(rass_report_rangingDataOverwritten_t));
}

static int blt_rass_sendRasControlPoint(u16 connHandle, blt_ras_cp_response_opcode_enum opcode,
                                                blt_ras_cp_operator_enum operator, void* operand, u16 operandLen)
{
    ras_cp_msg_t msg = {
        .opcode = opcode,
        .operator = operator,
    };
    memcpy(&msg.operand.val[0], operand, operandLen);
    return blc_atts_sendHandleValueIndicate(connHandle, RASS_CONTROL_POINT_HANDLE, (u8*)&msg, operandLen + RAS_CP_MSG_HEADER_SIZE);
//  return blc_atts_sendHandleValueNotify(connHandle, RASS_CONTROL_POINT_HANDLE, (u8*)&msg, operandLen + RAS_CP_MSG_HEADER_SIZE);
}

static int blt_rass_sendCompleteReportRecordsResponse(u16 connHandle, u32 numOfRecords)
{
    operands_completeReportRecord_t operand = {
        .numOfRecords = numOfRecords
    };
    return blt_rass_sendRasControlPoint(connHandle, RAS_CP_RSP_OPCODE_COMPLETE_REPORT_RECORDS, RAS_CP_OPERATOR_NULL,
                                        &operand, sizeof(operands_completeReportRecord_t));
}

static ble_sts_t __attribute__((unused)) blt_rass_sendCompleteRecordsSegmentResponse(u16 connHandle, u32 recordNum, u32 startSegment, u32 endSegment)
{
    operands_completeRecordSegment_t operand = {
        .recordNum = recordNum,
        .startSegment = startSegment,
        .endSegment = endSegment,
    };
    return blt_rass_sendRasControlPoint(connHandle, RAS_CP_RSP_OPCODE_COMPLETE_RECORD_SEGMENT, RAS_CP_OPERATOR_NULL,
                                        &operand, sizeof(operands_completeRecordSegment_t));
}

static int blt_rass_pushRollingRangingData(u16 connHandle, u16 attrHdl, rass_report_rangingData_t* rangingData)
{
    u8* payload = rangingData->pData + rangingData->segmentIndex*rangingData->MTU;
    int payloadLen = rangingData->MTU;

    ranging_data_head_t head = {
        .attOp = ATT_OP_HANDLE_VALUE_NTF,
        .attrHdl = attrHdl,
    };

    head.firstSegment = 0;
    head.lastSegment = 0;

    if(rangingData->segmentIndex == 0)
    {
        head.firstSegment = 1;
    }

    if(rangingData->segmentIndex == rangingData->segmentCount-1)
    {
        head.lastSegment = 1;
        payloadLen = rangingData->pDataLen - rangingData->segmentIndex*rangingData->MTU;
    }
    head.segmentCounter = rangingData->segmentIndex&0x3F;
    return blt_rass_pushNotifyData(connHandle, &head, payload, payloadLen);
}

_attribute_ble_data_retention_
SLIST_DEF(rassMsgList);

static rass_msg_t rassMsg[10];      //TODO: by Qihang.mou

static rass_msg_t* blt_rass_mallocMsg(u16 connHandle, u8 type)
{
    for(size_t i=0; i<ARRAY_SIZE(rassMsg); i++)
    {
        if(!rassMsg[i].type) {
            rassMsg[i].connHandle = connHandle;
            rassMsg[i].type = type;
            SLIST_INSERT_TAIL(&rassMsgList, &rassMsg[i]);
            return &rassMsg[i];
        }
    }

    return NULL;
}


static rass_report_liveRangingData_t* blt_rass_mallocReportLiveRangingDataMsg(u16 connHandle)
{
    rass_msg_t* msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_LIVE_RANGING_DATA);
    return &msg->live;
}

static rass_report_storedRangingData_t* blt_rass_mallocReportStoredRangingDataMsg(u16 connHandle)
{
    rass_msg_t* msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_STORED_RANGING_DATA);
    return &msg->store;
}

static rass_report_rangingDataReady_t* blt_rass_mallocReportRangingDataReadyMsg(u16 connHandle)
{
    rass_msg_t* msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_RANGING_DATA_READY);
    return &msg->ready;
}

static rass_report_rangingDataOverwritten_t* blt_rass_mallocProcedureDataOverwrittenMsg(u16 connHandle)
{
    rass_msg_t* msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_REPORT_RANGING_DATA_OVERWRITTEN);
    return &msg->overwritten;
}

static rass_report_reportRecordsResponse_t* blt_rass_mallocReportRecordResponseMsg(u16 connHandle)
{
    rass_msg_t* msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_COMPLETE_REPORT_RECORDS_RESPONSE);
    return &msg->records;
}

static rass_report_recordSegmentResponse_t* blt_rass_mallocReportSegmentResponseMsg(u16 connHandle)
{
    rass_msg_t* msg = blt_rass_mallocMsg(connHandle, RASS_MSG_TYPE_COMPLETE_REPORT_SEGMENT_RESPONSE);
    return &msg->segment;
}

u16 blc_rass_getRasClientConnHandle(void)
{
    return 0x40;    //TODO: qihang.mou, need enable notify/indicate flag.
}

void blt_rass_initialReportLiveRangingData(u8* pData, u16 pDataLen)
{
    u16 connHandle = blc_rass_getRasClientConnHandle();
    rass_report_liveRangingData_t* live = blt_rass_mallocReportLiveRangingDataMsg(connHandle);
    live->pData = pData;
    live->pDataLen = pDataLen;
    live->segmentIndex = 0;
    live->MTU = blt_gap_getEffectiveMTU(connHandle)-4;

    live->segmentCount = live->pDataLen/live->MTU + (live->pDataLen%live->MTU? 1: 0);
}

void blt_rass_initialOnDemandProcedureData(u16 connHandle, u8* pData, u16 pDataLen,u8 lost, u16 start_index , u16 end_index)
{
    rass_report_storedRangingData_t* store = blt_rass_mallocReportStoredRangingDataMsg(connHandle);
    store->pData = pData;
    store->pDataLen = pDataLen;
    store->segmentIndex = 0;
    store->MTU = blt_gap_getEffectiveMTU(connHandle)-4;
    store->segmentCount = store->pDataLen/store->MTU + (store->pDataLen%store->MTU? 1: 0);
    if(lost == 0){
        store->segmentEnd = store->segmentCount;
    }
    else{
        store->segmentIndex = start_index;
        store->segmentEnd = end_index + 1;
    }
}

void blt_rass_initialProcedureDataReady(u16 procedureCounter)
{
    u16 connHandle = blc_rass_getRasClientConnHandle();
    rass_report_rangingDataReady_t* ready = blt_rass_mallocReportRangingDataReadyMsg(connHandle);
    ready->procedureCounter = procedureCounter;
    CS_PRF_LOG("msg Procedure Data Ready Indication: ProcedureCounter<%d>", ready->procedureCounter);
}

void blt_rass_initialProcedureDataOverwritten(u16 procedureCounter)
{
    u16 connHandle = blc_rass_getRasClientConnHandle();
    rass_report_rangingDataOverwritten_t* overwritten = blt_rass_mallocProcedureDataOverwrittenMsg(connHandle);
    overwritten->procedureCounter = procedureCounter;
    CS_PRF_LOG("msg Procedure Data Overwritten Indication: ProcedureCounter<%d>", overwritten->procedureCounter);
}

void blt_rass_initialCompleteProcedureDataResponse(u16 connHandle, u16 numOfRecords)
{
    rass_report_reportRecordsResponse_t* records = blt_rass_mallocReportRecordResponseMsg(connHandle);
    records->numOfRecords = numOfRecords;
}
void blt_rass_initialCompleteLostProcedureSegmentResponse(u16 connHandle, u16 recordNumber, u16 segmentStart, u16 segmentEnd)
{
    rass_report_recordSegmentResponse_t* segment = blt_rass_mallocReportSegmentResponseMsg(connHandle);
    segment->recordNumber = recordNumber;
    segment->segmentStart = segmentStart;
    segment->segmentEnd = segmentEnd;
}

static int blt_rass_dealError(rass_msg_t* msg)
{
    (void)msg;
    return 0;
}

static int blt_rass_dealReportLiveRangingData(rass_msg_t* msg)
{
    u16 connHandle = msg->connHandle;
    rass_report_liveRangingData_t* live = &msg->live;
    if(blt_rass_pushRollingRangingData(connHandle, RASS_LIVE_RNGING_DATA_HANDLE, live) == BLE_SUCCESS) {
        live->segmentIndex ++;
        if(live->segmentIndex == live->segmentEnd) {
            return 1;
        }
    }
    return 0;
}

static int blt_rass_dealReportStoredRangingData(rass_msg_t* msg)
{
    u16 connHandle = msg->connHandle;
    rass_report_storedRangingData_t* store = &msg->store;
    if(blt_rass_pushRollingRangingData(connHandle, RASS_STORED_RANGING_DATA_HANDLE, store) == BLE_SUCCESS) {
        store->segmentIndex ++;
        if(store->segmentIndex == store->segmentEnd) {
            return 1;
        }
    }
    return 0;
}

static int blt_rass_dealRangingDataReady(rass_msg_t* msg)
{
    if(blt_rass_reportRangingDataReady(msg->connHandle, &msg->ready) == BLE_SUCCESS)
    {
        return 1;
    }
    CS_PRF_LOG("send Procedure Data Ready Indication Fail");
    return 0;
}

static int blt_rass_dealRangingDataOverwritten(rass_msg_t* msg)
{
    u8 ret = blt_rass_reportRangingDataOverwritten(msg->connHandle, &msg->overwritten);

    if (ret == BLE_SUCCESS) {
        blc_rass_procedureDeleteIndex(msg->overwritten.procedureCounter);
        return 1;
    }
    CS_PRF_LOG("send Procedure Data Overwritten Indication Fail by 0x%X", ret);
    return 0;
}

static int blt_rass_dealReportRecordsResponse(rass_msg_t* msg)
{
    u16 connHandle = msg->connHandle;
    rass_report_reportRecordsResponse_t* records = &msg->records;
    u8 ret = blt_rass_sendCompleteReportRecordsResponse(connHandle, records->numOfRecords);
    if( ret == BLE_SUCCESS)
    {
        CS_PRF_LOG("send Complete Procedure Data Response: Number of records<%d>", records->numOfRecords);
        return 1;
    }
    CS_PRF_LOG("send Complete Procedure Data Response Fail by 0x%X", ret);  //This may be caused by not enough ACL Tx Fifo Num
    return 0;
}

static int blt_rass_dealRecordSegmentResponse(rass_msg_t* msg)
{
    u16 connHandle = msg->connHandle;
    rass_report_recordSegmentResponse_t* segment = &msg->segment;

    if(blt_rass_sendCompleteRecordsSegmentResponse(connHandle, segment->recordNumber, segment->segmentStart, segment->segmentEnd) == BLE_SUCCESS)
    {
        CS_PRF_LOG("send Complete Lost Procedure Segment Response: procedureCounter<%d>,startSegment<%d>,endSegment<%d>", segment->recordNumber, segment->segmentStart, segment->segmentEnd);
        return 1;
    }
    CS_PRF_LOG("send Complete Lost Procedure Segment Response Fail");
    return 0;

}

typedef int(*rass_dealMsgCb)(rass_msg_t* msg);

rass_dealMsgCb dealMsgCb[] = {
    blt_rass_dealError,
    blt_rass_dealReportLiveRangingData, blt_rass_dealReportStoredRangingData,
    blt_rass_dealRangingDataReady, blt_rass_dealRangingDataOverwritten,
    blt_rass_dealReportRecordsResponse, blt_rass_dealRecordSegmentResponse
};

/**
 * @brief       ranging profile server main loop function.
 * @param[in]   connHandle: ACL handle.
 * @return      0.
 */
int blt_rass_loop(u16 connHandle)
{
    (void)connHandle;
    rass_msg_t* msg = (rass_msg_t*)SLIST_FIRST(&rassMsgList);
    if(msg)
    {
        int result = 1;
        if(msg->type <= ARRAY_SIZE(dealMsgCb)) {
            result = dealMsgCb[msg->type](msg);
        }
        if(result) {
            msg->type = RASS_MSG_TYPE_NULL;
            SLIST_DELETE_HEAD(&rassMsgList);
        }
        else
        {
            #if OS_SUP_EN
                if(blt_os_giveSem_cb)
                {
                    blt_os_giveSem_cb();
                }
            #endif
        }
    }
    return 0;
}



int blc_rass_procedureDataReady(u16 connHandle, u8 procedureCounter)
{
    blc_ras_server_t *server = blt_rass_getServerInst(0xFFFF);

    switch(server->procDataExchgMechanism)
    {
        case PROC_DATA_EXCHG_LOCAL:{
            blc_rass_query_result_t res = blc_rass_procedureQueryIndexStored(procedureCounter);
            blc_rasc_localRangingDataEvt_t evt = {
                .connHandle = connHandle,
                .dataPtr = res.procPtr,
                .dataLen = res.procLen
            };
            blt_prf_sendEvent(connHandle, CS_EVT_LOCAL_RANGING_DATA, &evt, sizeof(evt));
            blc_rass_procedureDeleteIndex(procedureCounter);
        }break;
        case PROC_DATA_EXCHG_REAL_TIME: {
            blc_rass_query_result_t res = blc_rass_procedureQueryIndexLiving(procedureCounter);
            blt_rass_initialReportLiveRangingData(res.procPtr, res.procLen);    //4: blc_rass_proc_head_t -> recordNumber
        }break;
        case PROC_DATA_EXCHG_ON_DEMAND: {
            blt_rass_initialProcedureDataReady(procedureCounter);
        }break;
        default:
            break;
    }
    return CS_RAS_SUCCESS;
}

int blc_rass_procedureDataOverwritten(u16 connHandle, u16 procedureCounter)
{
    (void)connHandle;
    blt_rass_initialProcedureDataOverwritten(procedureCounter);
    return CS_RAS_SUCCESS;
}





