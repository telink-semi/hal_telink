/********************************************************************************************************
 * @file    tbs_server.c
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


#define STATUS_FLAGS_VALID_MASK   (0x0003)
#define CCP_ORIGINATE_URI_LEN_MAX (64)
#define CCP_JOIN_LIST_LEN_MAX     (64)

#define GTBSS_SET_VALUE_CHANGED(GTBSS, ATTR, VAL)              \
    for (u8 i = 0; i < ARRAY_SIZE(GTBSS->valueChanged); i++) { \
        if (GTBSS->valueChanged[i].active) {                   \
            GTBSS->valueChanged[i].ATTR = VAL;                 \
        }                                                      \
    }

typedef struct
{
    u8        callIndex;
    const u8 *data;
    u16       dataLen;
} callIndexData_t;

static void blt_gtbss_serviceInit(const blc_ccps_regParam_t *param);
static int  blt_gtbss_connect(u16 connHandle, prf_acl_state_enum connState);

_attribute_ble_data_retention_
    blc_ccp_server_ctrl_t ccp_server_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_GTBS_SERVER,
                    .usedAclRole = 0,
                    .init        = blt_tbss_init,
                    .connect     = blt_gtbss_connect,
                    .discov      = NULL,
                    .loop        = NULL,
                    },
};

void blc_audio_registerCallControlServer(const blc_ccps_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&ccp_server_ctrl, param);
}

blc_ccp_server_t *blt_ccp_getServerInst(u16 connHandle)
{
#if (0)
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ret == ACL_ROLE_CENTRAL) {
        BLT_TBS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* VCP Volume Renderer GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_CCP_CALL_CONTROL_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &ccp_server_ctrl.ccpServer;
}

static blc_gtbs_server_t *blc_gtbss_getServerInst(u16 connHandle)
{
    blc_ccp_server_t *server = blt_ccp_getServerInst(connHandle);

    return server ? &server->gtbs : NULL;
}

static bool blt_gtbss_callIndexInCallState(blc_gtbs_server_t *gtbss, u8 callIndex)
{
    u8  *callStateValue    = NULL;
    u16 *callStateValueLen = NULL;

    blc_gatts_getAttributeInformationByHandle(0xFFFF, gtbss->callStateHandle, &callStateValue, &callStateValueLen);
    if (!callStateValue) {
        return false;
    }

    for (u16 i = 0; i < *callStateValueLen; i += 3) {
        if (callStateValue[i] == callIndex) {
            return true;
        }
    }

    return false;
}

static bool blt_gtbss_getCallState(blc_gtbs_server_t *gtbss, u8 callIndex, u8 *callState, u8 *callFlags)
{
    u8  *callStateValue    = NULL;
    u16 *callStateValueLen = NULL;

    blc_gatts_getAttributeInformationByHandle(0xFFFF, gtbss->callStateHandle, &callStateValue, &callStateValueLen);
    if (!callStateValue) {
        return false;
    }

    for (u8 i = 0; i < *callStateValueLen; i += 3) {
        if (callStateValue[i] == callIndex) {
            if (callState) {
                *callState = callStateValue[i + 1];
            }

            if (callFlags) {
                *callFlags = callStateValue[i + 2];
            }

            return true;
        }
    }

    return false;
}

static int blt_gtbss_setCallIndexDataChar(blc_gtbs_server_t *gtbss, u16 connHandle, u16 handle, u16 maxSize, callIndexData_t *data)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    blc_gatts_getAttributeInformationByHandle(connHandle, handle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (data) {
        if (data->dataLen >= maxSize) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        if (!blt_gtbss_callIndexInCallState(gtbss, data->callIndex)) {
            // Call State characteristic does not contain call index
            BLT_TBS_LOG("ERR: Call index (0x%02X) does not exist", data->callIndex);
            return GATT_ERR_INVALID_PARAMETER;
        }

        *value = data->callIndex;

        memcpy(value + 1, data->data, data->dataLen);
    }

    *len = data ? data->dataLen + 1 : 0;

    if (*len) {
        u16 mtu = blt_gap_getEffectiveMTU(connHandle);
        // TBS specification "If the characteristic value is longer than (ATT_MTU-3),
        // then the first (ATT_MTU-3) octets shall be included in the notification"
        if (mtu > 3) {
            return blc_gatts_notifyValue(connHandle, handle, value, min(mtu - 3, *len));
        }
    }

    return BLE_SUCCESS;
}

static int blt_gtbss_setIncomingCallTargetBearerURI(u16 connHandle, callIndexData_t *data)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    ble_sts_t          status;
    extern const u16   gtbsIncomingCallTargetBearerURIMaxSize;

    status = blt_gtbss_setCallIndexDataChar(gtbss, connHandle, gtbss->incomingCallTargetBearerURIHandle, gtbsIncomingCallTargetBearerURIMaxSize, data);
    if (status == BLE_SUCCESS) {
        GTBSS_SET_VALUE_CHANGED(gtbss, incomingCallTargetBearerURIChanged, true);
    }

    return status;
}

static int blt_gtbss_setBearerSignalStrengthReportingInterval(u16 connHandle, u8 interval)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    u8                *value = NULL;

    value = blc_gatts_getAttributeValueByHandle(connHandle, gtbss->bearerSignalStrengthReportingIntervalHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = interval;

    return BLE_SUCCESS;
}

static int blt_gtbss_setIncomingCall(u16 connHandle, callIndexData_t *data)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    ble_sts_t          status;
    extern const u16   gtbsIncomingCallMaxSize;

    status = blt_gtbss_setCallIndexDataChar(gtbss, connHandle, gtbss->incomingCallHandle, gtbsIncomingCallMaxSize, data);
    if (status == BLE_SUCCESS) {
        GTBSS_SET_VALUE_CHANGED(gtbss, incomingCallChanged, true);
    }

    return status;
}

static int blt_gtbss_setFriendlyName(u16 connHandle, callIndexData_t *data)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    ble_sts_t          status;
    extern const u16   gtbsFriendlyNameMaxSize;

    status = blt_gtbss_setCallIndexDataChar(gtbss, connHandle, gtbss->callFriendlyNameHandle, gtbsFriendlyNameMaxSize, data);
    if (status == BLE_SUCCESS) {
        GTBSS_SET_VALUE_CHANGED(gtbss, callFriendlyNameChanged, true);
    }

    return status;
}

static int blt_gtbss_bearerSignalStrengthReportingIntervalWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_gtbss_bearerSignalStrengthReportingIntervalEvt_t pEvt;

    if (valueLen != sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    blt_gtbss_setBearerSignalStrengthReportingInterval(connHandle, *writeValue);

    pEvt.interval = *writeValue;

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
}

static bool blt_gtbss_callStateChangedCheckChar(blc_gtbs_server_t *gtbss, u16 connHandle, u16 handle)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    blc_gatts_getAttributeInformationByHandle(connHandle, handle, &value, &len);
    if (!value || !(*len)) {
        return false;
    }

    // Check if call index is present in Call State
    if (!blt_gtbss_callIndexInCallState(gtbss, value[0])) {
        BLT_TBS_LOG("Clear (0x%04X) handle", handle);
        *len = 0;
        return true;
    }

    return false;
}

static void blt_gtbss_callStateChangedFriendlyName(blc_gtbs_server_t *gtbss, u16 connHandle)
{
    if (blt_gtbss_callStateChangedCheckChar(gtbss, connHandle, gtbss->callFriendlyNameHandle)) {
        GTBSS_SET_VALUE_CHANGED(gtbss, callFriendlyNameChanged, true);
    }
}

static void blt_gtbss_callStateChangedIncomingCall(blc_gtbs_server_t *gtbss, u16 connHandle)
{
    if (blt_gtbss_callStateChangedCheckChar(gtbss, connHandle, gtbss->incomingCallHandle)) {
        GTBSS_SET_VALUE_CHANGED(gtbss, incomingCallChanged, true);
    }
}

static void blt_gtbss_callStateChangedIncomingCallTargetBearerURI(blc_gtbs_server_t *gtbss, u16 connHandle)
{
    if (blt_gtbss_callStateChangedCheckChar(gtbss, connHandle, gtbss->incomingCallTargetBearerURIHandle)) {
        GTBSS_SET_VALUE_CHANGED(gtbss, incomingCallTargetBearerURIChanged, true);
    }
}

static void blt_gtbss_callStateChanged(blc_gtbs_server_t *gtbss, u16 connHandle)
{
    GTBSS_SET_VALUE_CHANGED(gtbss, callStateChanged, true);

    // Update Incoming Call Target Bearer URI
    blt_gtbss_callStateChangedIncomingCallTargetBearerURI(gtbss, connHandle);

    // Update Incoming Call
    blt_gtbss_callStateChangedIncomingCall(gtbss, connHandle);

    // Update Friendly Name
    blt_gtbss_callStateChangedFriendlyName(gtbss, connHandle);
}

static int blt_gtbss_sendControlPointNotification(blc_gtbs_server_t *gtbss, u16 connHandle, u8 opcode, u8 callIndex, u8 resultCode)
{
    u8 *value;

    value = blc_gatts_getAttributeValueByHandle(connHandle, gtbss->callControlPointHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    U8_TO_STREAM(value, opcode);
    U8_TO_STREAM(value, callIndex);
    U8_TO_STREAM(value, resultCode);

    return blc_gatts_notifyAttr(connHandle, gtbss->callControlPointHandle);
}

typedef int (*tbs_callControlPointCb)(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen);

typedef struct
{
    u8                     opcode;
    u16                    optionalOpcodeMask;
    tbs_callControlPointCb cb;
} blt_gtbss_callControlPoint_t;

static int blt_gtbss_callControlPointWriteCbackAccept(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen)
{
    blc_gtbss_callControlPointAcceptEvt_t pEvt;
    u8                                    callState, resultCode;

    if (valueLen != sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (!blt_gtbss_getCallState(gtbss, value[0], &callState, NULL)) {
        resultCode = GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX;
        goto fail;
    }

    if (callState != GTBS_CALL_STATE_INCOMING) {
        resultCode = GTBS_NTF_RESULT_CODE_STATE_MISMATCH;
        goto fail;
    }

    pEvt.callIndex = value[0];

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_ACCEPT, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
fail:
    blt_gtbss_sendControlPointNotification(gtbss, connHandle, GTBS_OPCODE_ACCEPT, 0, resultCode);

    return ATT_SUCCESS;
}

static int blt_gtbss_callControlPointWriteCbackTerminate(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen)
{
    blc_gtbss_callControlPointTerminateEvt_t pEvt;
    u8                                       resultCode;

    if (valueLen != sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (!blt_gtbss_getCallState(gtbss, value[0], NULL, NULL)) {
        resultCode = GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX;
        goto fail;
    }

    pEvt.callIndex = value[0];

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_TERMINATE, (u8 *)&pEvt, sizeof(pEvt));

    return ATT_SUCCESS;
fail:
    blt_gtbss_sendControlPointNotification(gtbss, connHandle, GTBS_OPCODE_TERMINATE, 0, resultCode);

    return ATT_SUCCESS;
}

static int blt_gtbss_callControlPointWriteCbackLocalHold(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen)
{
    u8 callState, resultCode;

    if (valueLen != sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (!blt_gtbss_getCallState(gtbss, value[0], &callState, NULL)) {
        resultCode = GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX;
        goto fail;
    }

    if (callState == GTBS_CALL_STATE_INCOMING || callState == GTBS_CALL_STATE_ACTIVE || callState == GTBS_CALL_STATE_REMOTELY_HELD) {
        blc_gtbss_callControlPointLocalHoldEvt_t pEvt = {
            .callIndex = value[0],
        };

        blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_LOCAL_HOLD, (u8 *)&pEvt, sizeof(pEvt));

        return ATT_SUCCESS;
    } else {
        resultCode = GTBS_NTF_RESULT_CODE_STATE_MISMATCH;
    }
fail:
    blt_gtbss_sendControlPointNotification(gtbss, connHandle, GTBS_OPCODE_LOCAL_HOLD, 0, resultCode);

    return ATT_SUCCESS;
}

static int blt_gtbss_callControlPointWriteCbackLocalRetrieve(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen)
{
    u8 callState, resultCode;

    if (valueLen != sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (!blt_gtbss_getCallState(gtbss, value[0], &callState, NULL)) {
        resultCode = GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX;
        goto fail;
    }

    if (callState == GTBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD || callState == GTBS_CALL_STATE_LOCALLY_HELD) {
        blc_gtbss_callControlPointLocalRetrieveEvt_t pEvt = {
            .callIndex = value[0],
        };

        blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_LOCAL_RETRIEVE, (u8 *)&pEvt, sizeof(pEvt));

        return ATT_SUCCESS;
    } else {
        resultCode = GTBS_NTF_RESULT_CODE_STATE_MISMATCH;
    }
fail:
    blt_gtbss_sendControlPointNotification(gtbss, connHandle, GTBS_OPCODE_LOCAL_RETRIEVE, 0, resultCode);

    return ATT_SUCCESS;
}

static int blt_gtbss_callControlPointWriteCbackOriginate(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen)
{
    blc_gtbss_callControlPointOriginateEvt_t *pEvt;
    u8                                        buf[sizeof(*pEvt) + CCP_ORIGINATE_URI_LEN_MAX];

    if (valueLen > CCP_ORIGINATE_URI_LEN_MAX) {
        blt_gtbss_sendControlPointNotification(gtbss, connHandle, GTBS_OPCODE_ORIGINATE, 0, GTBS_NTF_RESULT_CODE_LACK_OF_RESOURCES);

        return ATT_SUCCESS;
    }

    pEvt = (blc_gtbss_callControlPointOriginateEvt_t *)buf;

    pEvt->uriLen = valueLen;
    memcpy(pEvt->uri, value, valueLen);

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_ORIGINATE, (u8 *)pEvt, sizeof(*pEvt) + pEvt->uriLen);

    return ATT_SUCCESS;
}

static int blt_gtbss_callControlPointWriteCbackJoin(blc_gtbs_server_t *gtbss, u16 connHandle, u8 *value, u16 valueLen)
{
    blc_gtbss_callControlPointJoinEvt_t *pEvt;
    u8                                   buf[sizeof(*pEvt) + CCP_JOIN_LIST_LEN_MAX];
    u8                                   resultCode;

    pEvt               = (blc_gtbss_callControlPointJoinEvt_t *)buf;
    pEvt->callIndexNum = 0;

    if (valueLen == 0) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    for (u16 i = 0; i < valueLen; i++) {
        u8   callState;
        bool unique = true;

        if (!blt_gtbss_getCallState(gtbss, value[i], &callState, NULL)) {
            resultCode = GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX;
            goto fail;
        }

        if (callState == GTBS_CALL_STATE_INCOMING) {
            resultCode = GTBS_NTF_RESULT_CODE_OPERATION_NOT_POSSIBLE;
            goto fail;
        }

        for (u16 j = 0; j < i; j++) {
            if (value[i] == value[j]) {
                unique = false;
                break;
            }
        }

        if (unique) {
            pEvt->callIndexNum++;
            if (pEvt->callIndexNum > CCP_JOIN_LIST_LEN_MAX) {
                resultCode = GTBS_NTF_RESULT_CODE_LACK_OF_RESOURCES;
                goto fail;
            }
            pEvt->callIndexes[pEvt->callIndexNum - 1] = value[i];
        }
    }

    if (pEvt->callIndexNum < 2) {
        resultCode = GTBS_NTF_RESULT_CODE_OPERATION_NOT_POSSIBLE;
        goto fail;
    }

    blt_prf_sendEvent(connHandle, AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_JOIN, (u8 *)pEvt, sizeof(*pEvt) + pEvt->callIndexNum);

    return ATT_SUCCESS;

fail:
    blt_gtbss_sendControlPointNotification(gtbss, connHandle, GTBS_OPCODE_JOIN, 0, resultCode);

    return ATT_SUCCESS;
}

static blt_gtbss_callControlPoint_t gtbss_callControlPoint[] = {
    {.opcode = GTBS_OPCODE_ACCEPT,         .optionalOpcodeMask = 0,                                                .cb = blt_gtbss_callControlPointWriteCbackAccept       },
    {.opcode = GTBS_OPCODE_TERMINATE,      .optionalOpcodeMask = 0,                                                .cb = blt_gtbss_callControlPointWriteCbackTerminate    },
    {.opcode = GTBS_OPCODE_LOCAL_HOLD,     .optionalOpcodeMask = GTBS_CCP_OPT_OPCODE_SUPP_LOCAL_HOLD_AND_RETRIEVE, .cb = blt_gtbss_callControlPointWriteCbackLocalHold    },
    {.opcode = GTBS_OPCODE_LOCAL_RETRIEVE, .optionalOpcodeMask = GTBS_CCP_OPT_OPCODE_SUPP_LOCAL_HOLD_AND_RETRIEVE, .cb = blt_gtbss_callControlPointWriteCbackLocalRetrieve},
    {.opcode = GTBS_OPCODE_ORIGINATE,      .optionalOpcodeMask = 0,                                                .cb = blt_gtbss_callControlPointWriteCbackOriginate    },
    {.opcode = GTBS_OPCODE_JOIN,           .optionalOpcodeMask = GTBS_CCP_OPT_OPCODE_SUPP_JION_CALL,               .cb = blt_gtbss_callControlPointWriteCbackJoin         },
};

static int blt_gtbss_callControlPointWriteCback(u16 connHandle, u8 *writeValue, u16 valueLen)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);

    if (valueLen == 0) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    for (u8 i = 0; i < ARRAY_SIZE(gtbss_callControlPoint); i++) {
        if (gtbss_callControlPoint[i].opcode == writeValue[0]) {
            // check optional opcode mask
            if (gtbss_callControlPoint[i].optionalOpcodeMask) {
                u16 *optionalOpcodes;

                optionalOpcodes = (u16 *)blc_gatts_getAttributeValueByHandle(connHandle, gtbss->callControlPointOptionalOpcodesHandle);
                if (!optionalOpcodes || (!(*optionalOpcodes & gtbss_callControlPoint[i].optionalOpcodeMask))) {
                    break;
                }
            }

            return gtbss_callControlPoint[i].cb(gtbss, connHandle, &writeValue[1], valueLen - 1);
        }
    }

    // Invalid Opcode
    blt_gtbss_sendControlPointNotification(gtbss, connHandle, writeValue[0], 0, GTBS_NTF_RESULT_CODE_OPCODE_NOT_SUPP);

    return ATT_SUCCESS;
}

static int blt_gtbss_longValueReadCback(u16 connHandle, u8 opcode, u16 attrHandle, bool *valueChangedFlag, u8 **outValue, u16 *outValueLen)
{
    u8  *value = NULL;
    u16 *len   = NULL;

    // Treat blob reaquest as always non-zero offset read
    if (opcode == ATT_OP_READ_BLOB_REQ) {
        if (*valueChangedFlag) {
            return GTBS_ERRCODE_VALUE_CHANGED_DURING_READ_LONG;
        }
    } else {
        *valueChangedFlag = false;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, attrHandle, &value, &len);
    if (!value) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    *outValueLen = *len;
    *outValue    = value;

    return ATT_SUCCESS;
}

static value_changed_conn_t *blt_gtbss_getValueChanged(blc_gtbs_server_t *gtbss, u16 connHandle)
{
    for (u8 i = 0; i < ARRAY_SIZE(gtbss->valueChanged); i++) {
        if (gtbss->valueChanged[i].active && gtbss->valueChanged[i].connHandle == connHandle) {
            return &gtbss->valueChanged[i];
        }
    }

    return NULL;
}

static int blt_gtbss_readCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen)
{
    blc_gtbs_server_t    *gtbss        = blc_gtbss_getServerInst(connHandle);
    value_changed_conn_t *valueChanged = blt_gtbss_getValueChanged(gtbss, connHandle);
    bool                  valueChangedFlag;
    int                   ret = ATT_SUCCESS;

    BLT_TBS_LOG("%d %d %p", opcode, *outValueLen, outValueLen);

    if (!valueChanged) {
        // Shouldn't happen
        return ATT_ERR_UNLIKELY_ERR;
    }

    if (attrHandle == gtbss->bearerListCurrentCallsHandle) {
        valueChangedFlag                            = valueChanged->bearerListCurrentCallsChanged;
        ret                                         = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->bearerListCurrentCallsChanged = valueChangedFlag;
    } else if (attrHandle == gtbss->bearerProviderNameHandle) {
        valueChangedFlag                        = valueChanged->bearerProviderNameChanged;
        ret                                     = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->bearerProviderNameChanged = valueChangedFlag;
    } else if (attrHandle == gtbss->bearerURISchemesSupportedListHandle) {
        valueChangedFlag                                   = valueChanged->bearerURISchemesSupportedListChanged;
        ret                                                = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->bearerURISchemesSupportedListChanged = valueChangedFlag;
    } else if (attrHandle == gtbss->incomingCallTargetBearerURIHandle) {
        valueChangedFlag                                 = valueChanged->incomingCallTargetBearerURIChanged;
        ret                                              = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->incomingCallTargetBearerURIChanged = valueChangedFlag;
    } else if (attrHandle == gtbss->callStateHandle) {
        valueChangedFlag               = valueChanged->callStateChanged;
        ret                            = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->callStateChanged = valueChangedFlag;
    } else if (attrHandle == gtbss->incomingCallHandle) {
        valueChangedFlag                  = valueChanged->incomingCallChanged;
        ret                               = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->incomingCallChanged = valueChangedFlag;
    } else if (attrHandle == gtbss->callFriendlyNameHandle) {
        valueChangedFlag                      = valueChanged->callFriendlyNameChanged;
        ret                                   = blt_gtbss_longValueReadCback(connHandle, opcode, attrHandle, &valueChangedFlag, outValue, outValueLen);
        valueChanged->callFriendlyNameChanged = valueChangedFlag;
    }

    return ret;
}

static int blt_gtbss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    (void)opcode;
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);

    if (attrHandle == gtbss->bearerSignalStrengthReportingIntervalHandle) {
        return blt_gtbss_bearerSignalStrengthReportingIntervalWriteCback(connHandle, writeValue, valueLen);
    } else if (attrHandle == gtbss->callControlPointHandle) {
        return blt_gtbss_callControlPointWriteCback(connHandle, writeValue, valueLen);
    }

    return ATT_SUCCESS;
}

static int blt_gtbss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);

    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_TBS_LOG("blt_gtbss_disconnect: 0x%x", connHandle);

        for (u8 i = 0; i < ARRAY_SIZE(gtbss->valueChanged); i++) {
            if (gtbss->valueChanged[i].active && gtbss->valueChanged[i].connHandle == connHandle) {
                memset(&gtbss->valueChanged[i], 0, sizeof(gtbss->valueChanged[i]));
                break;
            }
        }
    } else {
        BLT_TBS_LOG("blt_gtbss_connect: 0x%x", connHandle);

        for (u8 i = 0; i < ARRAY_SIZE(gtbss->valueChanged); i++) {
            if (!gtbss->valueChanged[i].active) {
                gtbss->valueChanged[i].active     = true;
                gtbss->valueChanged[i].connHandle = connHandle;

                // Require client to start read below characteristics with zero offset
                gtbss->valueChanged[i].bearerListCurrentCallsChanged        = true;
                gtbss->valueChanged[i].bearerProviderNameChanged            = true;
                gtbss->valueChanged[i].bearerURISchemesSupportedListChanged = true;
                gtbss->valueChanged[i].callFriendlyNameChanged              = true;
                gtbss->valueChanged[i].callStateChanged                     = true;
                gtbss->valueChanged[i].incomingCallChanged                  = true;
                gtbss->valueChanged[i].incomingCallTargetBearerURIChanged   = true;
                break;
            }
        }
    }

    return 0;
}

int blt_tbss_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_ccp_server_t)), blc_ccp_server_t);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_TBS_LOG("Server init");
        blc_svc_addGtbsGroup();
        blc_svc_gtbsCbackRegister(blt_gtbss_readCback, blt_gtbss_writeCback);
        blt_gtbss_serviceInit(param);
    }
    // else if (initType == PRF_PROC_DEINIT) {
    //  blc_svc_removeGtbsGroup();
    //  BLT_TBS_LOG("Server Deinit");
    // }
    return 0;
}

static void blt_tbss_initBearerProviderNameChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer Provider Name char too many, max num is %d", p->num);
    } else {
        tbss->bearerProviderNameHandle = p->charHandle;
    }
}

static void blt_tbss_initBearerUCIChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer UCI char too many, max num is %d", p->num);
    } else {
        tbss->bearerUCIHandle = p->charHandle;
    }
}

static void blt_tbss_initBearerTechnologyChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer technology char too many, max num is %d", p->num);
    } else {
        tbss->bearerTechnologyHandle = p->charHandle;
    }
}

static void blt_tbss_initBearerURISchemesSupportedListChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer URI schemes supported list char too many, max num is %d", p->num);
    } else {
        tbss->bearerURISchemesSupportedListHandle = p->charHandle;
    }
}

static void blt_tbss_initBearerSignalStrengthChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer signal strength char too many, max num is %d", p->num);
    } else {
        tbss->bearerSignalStrengthHandle = p->charHandle;
    }
}

static void blt_tbss_initBearerSignalStrengthReportingIntervalChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer Signal Strength Reporting Interval char too many, max num is %d", p->num);
    } else {
        tbss->bearerSignalStrengthReportingIntervalHandle = p->charHandle;
    }
}

static void blt_tbss_initBearerListCurrentCallsChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Bearer List Current Calls char too many, max num is %d", p->num);
    } else {
        tbss->bearerListCurrentCallsHandle = p->charHandle;
    }
}

static void blt_tbss_initCCIDChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: CCID char too many, max num is %d", p->num);
    } else {
        tbss->CCIDHandle = p->charHandle;
    }
}

static void blt_tbss_initStatusFlagsChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Status Flagsd char too many, max num is %d", p->num);
    } else {
        tbss->statusFlagsHandle = p->charHandle;
    }
}

static void blt_tbss_initIncomingCallTargetBearerURIChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Incoming Call Target Bearer URI char too many, max num is %d", p->num);
    } else {
        tbss->incomingCallTargetBearerURIHandle = p->charHandle;
    }
}

static void blt_tbss_initCallStateChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Call State char too many, max num is %d", p->num);
    } else {
        tbss->callStateHandle = p->charHandle;
    }
}

static void blt_tbss_initCallControlPointChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Call Control Point char too many, max num is %d", p->num);
    } else {
        tbss->callControlPointHandle = p->charHandle;
    }
}

static void blt_tbss_initCallControlPointOptionalOpcodesChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Call Control Point Optional Opcodes char too many, max num is %d", p->num);
    } else {
        tbss->callControlPointOptionalOpcodesHandle = p->charHandle;
    }
}

static void blt_tbss_initTerminatingReasonChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Terminating Reason char too many, max num is %d", p->num);
    } else {
        tbss->terminatingReasonHandle = p->charHandle;
    }
}

static void blt_tbss_initIncomingCallHandleChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Incoming Call char too many, max num is %d", p->num);
    } else {
        tbss->incomingCallHandle = p->charHandle;
    }
}

static void blt_tbss_initCallFriendlyNameChar(atts_foundCharParam_t *p, void *input)
{
    blc_tbs_server_t *tbss = (blc_tbs_server_t *)input;
    if (p->num) {
        BLT_TBS_LOG("ERR: Call Friendly Name char too many, max num is %d", p->num);
    } else {
        tbss->callFriendlyNameHandle = p->charHandle;
    }
}

static const atts_findCharList_t tbssChar[] = {
    {
     .charUuid    = characteristicBearerProviderNameUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerProviderNameChar,
     },
    {
     .charUuid    = characteristicBearerUciUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerUCIChar,
     },
    {
     .charUuid    = characteristicBearerTechnologyUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerTechnologyChar,
     },
    {
     .charUuid    = characteristicBearerUriSchemesSuppListUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerURISchemesSupportedListChar,
     },
    {
     .charUuid    = characteristicBearerSsUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerSignalStrengthChar,
     },
    {
     .charUuid    = characteristicBearerSsReportingIntervalUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerSignalStrengthReportingIntervalChar,
     },
    {
     .charUuid    = characteristicBearerListCurrentCallsUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initBearerListCurrentCallsChar,
     },
    {
     .charUuid    = characteristicContentControlIdUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initCCIDChar,
     },
    {
     .charUuid    = characteristicStatusFlagsUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initStatusFlagsChar,
     },
    {
     .charUuid    = characteristicIncomingCallTargetBearerUriUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initIncomingCallTargetBearerURIChar,
     },
    {
     .charUuid    = characteristicCallStateUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initCallStateChar,
     },
    {
     .charUuid    = characteristicCallCtrlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initCallControlPointChar,
     },
    {
     .charUuid    = characteristicCallCtrlPointOptionalOpcodesUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initCallControlPointOptionalOpcodesChar,
     },
    {
     .charUuid    = characteristicTerminationReasonUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initTerminatingReasonChar,
     },
    {
     .charUuid    = characteristicIncomingCallUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initIncomingCallHandleChar,
     },
    {
     .charUuid    = characteristicCallFriendlyNameUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_tbss_initCallFriendlyNameChar,
     },
};

static void blt_tbs_print_handles(blc_tbs_server_t *tbs)
{
    BLT_TBS_LOG("Handle information:");
    BLT_TBS_LOG("Bearer Provider Name:0x%04x, Bearer UCI:0x%04x", tbs->bearerProviderNameHandle, tbs->bearerUCIHandle);
    BLT_TBS_LOG("Bearer Technology:0x%04x, Bearer URI Schemes Supported List:0x%04x", tbs->bearerTechnologyHandle, tbs->bearerURISchemesSupportedListHandle);
    BLT_TBS_LOG("Bearer Signal Strength:0x%04x, Bearer Signal Strength Reporting Inverval:0x%04x", tbs->bearerSignalStrengthHandle, tbs->bearerSignalStrengthReportingIntervalHandle);
    BLT_TBS_LOG("Bearer List Current Calls:0x%04x, CCID:0x%04x", tbs->bearerListCurrentCallsHandle, tbs->CCIDHandle);
    BLT_TBS_LOG("Status Flags:0x%04x, Incoming Call Target Bearer:0x%04x", tbs->statusFlagsHandle, tbs->incomingCallTargetBearerURIHandle);
    BLT_TBS_LOG("Call State:0x%04x, Call Control Point:0x%04x", tbs->callStateHandle, tbs->callControlPointHandle);
    BLT_TBS_LOG("Call Control Point Optional Opcodes:0x%04x, Terminate Reason:0x%04x", tbs->callControlPointOptionalOpcodesHandle, tbs->terminatingReasonHandle);
    BLT_TBS_LOG("Incoming Call:0x%04x, Call Friendly Name:0x%04x", tbs->incomingCallHandle, tbs->callFriendlyNameHandle);
}

static void blt_gtbss_serviceInit(const blc_ccps_regParam_t *param)
{
    blc_gtbs_server_t         *server = blc_gtbss_getServerInst(0xFFFF);
    const blc_ccps_regParam_t *ccpsParam;

    blc_atts_findCharacteristicByServiceUuid(serviceGenericTelephoneBearerUuid, ATT_16_UUID_LEN, tbssChar, ARRAY_SIZE(tbssChar), server);
    blt_tbs_print_handles(server);

    ccpsParam = param ? param : &defaultCppsParam;

    blc_gtbss_updateBearerProviderName(0xFFFF, ccpsParam->gtbsParam.bearerProviderName, ccpsParam->gtbsParam.bearerProviderNameLen);
    blc_gtbss_updateBearerUniformCallerIdentifier(0xFFFF, ccpsParam->gtbsParam.bearerUci, ccpsParam->gtbsParam.bearerUciLen);
    blc_gtbss_updateBearerTechnology(0xFFFF, ccpsParam->gtbsParam.bearerTechnology);
    blc_gtbss_updateBearerURISchemesSupportedList(0xFFFF, ccpsParam->gtbsParam.bearerUriSchemeListLen, ccpsParam->gtbsParam.bearerUriSchemeList);
    blc_gtbss_updateBearerSignalStrength(0xFFFF, ccpsParam->gtbsParam.signalStrength);
    blt_gtbss_setBearerSignalStrengthReportingInterval(0xFFFF, 0);
    blc_gtbss_updateContentCtrlID(0xFFFF, ccpsParam->gtbsParam.CCID);
    blc_gtbss_updateStatusFlags(0xFFFF, ccpsParam->gtbsParam.statusFlags);
    blt_gtbss_setIncomingCallTargetBearerURI(0xFFFF, NULL);
    blt_gtbss_setIncomingCall(0xFFFF, NULL);
    blt_gtbss_setFriendlyName(0xFFFF, NULL);
}

int blc_gtbss_updateBearerProviderName(u16 connHandle, const u8 *pPrName, u16 prNameLen)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    extern const u16   gtbsBearerProviderNameMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;
    u16                mtu;

    if (prNameLen > gtbsBearerProviderNameMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, gtbss->bearerProviderNameHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    memcpy(value, pPrName, prNameLen);
    *len = prNameLen;
    mtu  = blt_gap_getEffectiveMTU(connHandle);

    GTBSS_SET_VALUE_CHANGED(gtbss, bearerProviderNameChanged, true);

    // TBS specification 3.1.1 Bearer Provider Name behavior "If the characteristic value is longer than (ATT_MTU-3),
    // then the first (ATT_MTU-3) octets shall be included in the notification"
    if (mtu > 3) {
        return blc_gatts_notifyValue(connHandle, gtbss->bearerProviderNameHandle, value, min(mtu - 3, *len));
    }

    return BLE_SUCCESS;
}

int blc_gtbss_updateBearerUniformCallerIdentifier(u16 connHandle, const u8 *uci, u16 uciLen)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    extern const u16   gtbsBearerUCIMaxSize;
    u8                *value = NULL;
    u16               *len   = NULL;

    if (uciLen > gtbsBearerUCIMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    blc_gatts_getAttributeInformationByHandle(connHandle, gtbss->bearerUCIHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    memcpy(value, uci, uciLen);
    *len = uciLen;

    return BLE_SUCCESS;
}

int blc_gtbss_updateBearerTechnology(u16 connHandle, u8 Technology)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    u8                *value = NULL;

    value = blc_gatts_getAttributeValueByHandle(connHandle, gtbss->bearerTechnologyHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = Technology;

    return blc_gatts_notifyAttr(connHandle, gtbss->bearerTechnologyHandle);
}

static int blt_gtbss_fillBearerURISchemesSupportedList(u8 uriSchemesNum, const blc_tbss_uri_scheme_t *schemes, u8 *out, u16 *outLen)
{
    u16 size = uriSchemesNum ? uriSchemesNum - 1 : 0; // commas

    for (u8 i = 0; i < uriSchemesNum; i++) {
        size += schemes[i].uriLen;
    }
    // Check if there is enough room for list
    if (size > *outLen) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
    }

    for (u8 i = 0; i < uriSchemesNum; i++) {
        if (i) {
            U8_TO_STREAM(out, ',');
        }

        STR_TO_STREAM(out, schemes[i].uri, schemes[i].uriLen);
    }

    *outLen = size;

    return BLE_SUCCESS;
}

int blc_gtbss_updateBearerURISchemesSupportedList(u16 connHandle, u8 uriSchemesNum, const blc_tbss_uri_scheme_t *schemes)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    extern const u16   gtbsBearerURISchemesSupportedListMaxSize;
    u8                *value   = NULL;
    u16               *len     = NULL;
    u16                new_len = gtbsBearerURISchemesSupportedListMaxSize;
    u16                mtu;
    ble_sts_t          status;

    blc_gatts_getAttributeInformationByHandle(connHandle, gtbss->bearerURISchemesSupportedListHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    status = blt_gtbss_fillBearerURISchemesSupportedList(uriSchemesNum, schemes, value, &new_len);
    if (status != BLE_SUCCESS) {
        return status;
    }

    GTBSS_SET_VALUE_CHANGED(gtbss, bearerURISchemesSupportedListChanged, true);

    *len = new_len;
    mtu  = blt_gap_getEffectiveMTU(connHandle);
    if (mtu > 3) {
        return blc_gatts_notifyValue(connHandle, gtbss->bearerURISchemesSupportedListHandle, value, min(mtu - 3, *len));
    }

    return BLE_SUCCESS;
}

int blc_gtbss_updateBearerSignalStrength(u16 connHandle, u8 signalStrength)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    u8                *value = NULL;

    if (signalStrength > 100 && signalStrength != GTBS_SIGNAL_STRENGTH_UNAVAILABLE) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    value = blc_gatts_getAttributeValueByHandle(connHandle, gtbss->bearerSignalStrengthHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = signalStrength;

    return blc_gatts_notifyAttr(connHandle, gtbss->bearerSignalStrengthHandle);
}

int blc_gtbss_updateContentCtrlID(u16 connHandle, u8 ccid)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    u8                *value = NULL;

    value = blc_gatts_getAttributeValueByHandle(connHandle, gtbss->CCIDHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = ccid;

    return BLE_SUCCESS;
}

int blc_gtbss_updateStatusFlags(u16 connHandle, blc_tbs_status_flags_t statusFlags)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    u8                *value = NULL;

    value = blc_gatts_getAttributeValueByHandle(connHandle, gtbss->statusFlagsHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    U16_TO_STREAM(value, statusFlags.statusFlags & STATUS_FLAGS_VALID_MASK);

    return blc_gatts_notifyAttr(connHandle, gtbss->statusFlagsHandle);
}

int blc_gtbss_updateIncomingCallTargetBearerURI(u16 connHandle, u8 callIndex, const u8 *uri, u16 uriLen)
{
    callIndexData_t data = {
        .callIndex = callIndex,
        .data      = uri,
        .dataLen   = uriLen,
    };

    return blt_gtbss_setIncomingCallTargetBearerURI(connHandle, &data);
}

int blc_gtbss_updateCallState(u16 connHandle, u16 callMembersCnt, blc_gtbs_call_state_t *callState)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    extern const u16   gtbsCallStateMaxSize;
    u8                *value = NULL;
    u8                *ptr;
    u16               *len = NULL;
    u16                mtu;

    blc_gatts_getAttributeInformationByHandle(connHandle, gtbss->callStateHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (!gtbss->callStateHandle) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if ((callMembersCnt * sizeof(u8) * 3) > gtbsCallStateMaxSize) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
    }

    ptr = value;
    for (u16 i = 0; i < callMembersCnt; i++) {
        U8_TO_STREAM(ptr, callState[i].callIndex);
        U8_TO_STREAM(ptr, callState[i].state);
        U8_TO_STREAM(ptr, callState[i].callFlags);
    }

    *len = ptr - value;
    mtu  = blt_gap_getEffectiveMTU(connHandle);

    blt_gtbss_callStateChanged(gtbss, connHandle);

    if (mtu > 3) {
        return blc_gatts_notifyValue(connHandle, gtbss->callStateHandle, value, min(mtu - 3, *len));
    }

    return BLE_SUCCESS;
}

int blc_gtbss_updateTerminationReason(u16 connHandle, blc_gtbss_terminationReasonNtf_t val)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);

    if (!blt_gtbss_callIndexInCallState(gtbss, val.callIndex)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    gtbss->terminationReasonValue[0] = val.callIndex;
    gtbss->terminationReasonValue[1] = val.termRsn;

    return blc_gatts_notifyValue(connHandle, gtbss->terminatingReasonHandle, gtbss->terminationReasonValue, sizeof(gtbss->terminationReasonValue));
}

int blc_gtbss_updateIncomingCall(u16 connHandle, u8 callIndex, const u8 *uri, u16 uriLen)
{
    callIndexData_t data = {
        .callIndex = callIndex,
        .data      = uri,
        .dataLen   = uriLen,
    };

    return blt_gtbss_setIncomingCall(connHandle, &data);
}

int blc_gtbss_updateCallFriendlyName(u16 connHandle, u8 callIndex, const u8 *callFriendlyName, u16 callFriendlyNameLen)
{
    callIndexData_t data = {
        .callIndex = callIndex,
        .data      = callFriendlyName,
        .dataLen   = callFriendlyNameLen,
    };

    return blt_gtbss_setFriendlyName(connHandle, &data);
}

int blc_gtbss_updateCallControlPointOptionalOpcodes(u16 connHandle, u16 optionalOpcodes)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    u16               *value;

    value = (u16 *)blc_gatts_getAttributeValueByHandle(connHandle, gtbss->callControlPointOptionalOpcodesHandle);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *value = optionalOpcodes;

    return BLE_SUCCESS;
}

int blc_gtbss_updateBearerListCurrentCalls(u16 connHandle, blc_gtbs_bearer_list_item_t *listCurrCalls, u16 listCurrCallsLen)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);
    extern const u16   gtbsBearerListCurrentCallsMaxSize;
    u8                *value = NULL;
    u8                *ptr;
    u16               *len = NULL;
    u16                mtu, size_needed = 0;

    blc_gatts_getAttributeInformationByHandle(connHandle, gtbss->bearerListCurrentCallsHandle, &value, &len);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    for (u16 i = 0; i < listCurrCallsLen; i++) {
        if (!blt_gtbss_callIndexInCallState(gtbss, listCurrCalls[i].state.callIndex)) {
            return GATT_ERR_INVALID_PARAMETER;
        }

        size_needed += 4;
        size_needed += listCurrCalls[i].uriLen;
    }

    if (size_needed > gtbsBearerListCurrentCallsMaxSize) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *len = 0;
    ptr  = value;

    for (u16 i = 0; i < listCurrCallsLen; i++) {
        // TBS specification 3.7 Bearer List Current Calls, Bearer List Current Calls characteristic format:
        // List_Item_Length[i] (1 octet)
        U8_TO_STREAM(ptr, 3 + listCurrCalls[i].uriLen)
        // Call_Index[i] (1 octet)
        U8_TO_STREAM(ptr, listCurrCalls[i].state.callIndex);
        // Call_State[i] (1 octet)
        U8_TO_STREAM(ptr, listCurrCalls[i].state.state);
        // Call_Flags[i] (1 octet)
        U8_TO_STREAM(ptr, listCurrCalls[i].state.callFlags);
        // Call_URI[i] (variable)
        memcpy(ptr, listCurrCalls[i].uri, listCurrCalls[i].uriLen);
        ptr += listCurrCalls[i].uriLen;
        *len += 4 + listCurrCalls[i].uriLen;
    }

    GTBSS_SET_VALUE_CHANGED(gtbss, bearerListCurrentCallsChanged, true);

    mtu = blt_gap_getEffectiveMTU(connHandle);
    if (mtu > 3) {
        return blc_gatts_notifyValue(connHandle, gtbss->bearerListCurrentCallsHandle, value, min(mtu - 3, *len));
    }

    return BLE_SUCCESS;
}

int blc_gtbss_updateCallCtrlPoint(u16 connHandle, blc_gtbss_callCtrlPointNtf_t val)
{
    blc_gtbs_server_t *gtbss = blc_gtbss_getServerInst(connHandle);

    return blt_gtbss_sendControlPointNotification(gtbss, connHandle, val.reqOpcode, val.callIndex, val.resultCode);
}
