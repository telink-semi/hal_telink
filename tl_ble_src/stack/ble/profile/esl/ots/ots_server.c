/********************************************************************************************************
 * @file    ots_server.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    04,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "ots_internal.h"

#define INDICATIONS_ENABLED  (0x0002)
#define OTS_OACP_TIMEOUT_SEC (30)

/*
 * Below is mask of features which are supported by current implementation. It may be extended once new
 * features will be implemented
 */
#define OTSS_SUPPORTED_OLCP_FEATURES_MASK (BLC_OTS_OLCP_FEATURE_OLCP_GO_TO_OPCODE_SUPPORTED)
#define OTSS_SUPPORTED_OACP_FEATURES_MASK                                                                                                                                \
    (BLC_OTS_OACP_FEATURE_OACP_READ_OPCODE_SUPPORTED | BLC_OTS_OACP_FEATURE_OACP_WRITE_OPCODE_SUPPORTED | BLC_OTS_OACP_FEATURE_APPENDING_ADD_DATA_TO_OBJECTS_SUPPORTED | \
     BLC_OTS_OACP_FEATURE_TRUNCATION_OF_OBJECTS_SUPPORTED | BLC_OTS_OACP_FEATURE_PATCHING_OF_OBJECTS_SUPPORTED)

typedef int (*blt_otss_cpProcessCb_t)(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen);

typedef struct
{
    u8                     opcode;
    u32                    mask;
    u8                     minLen;
    u8                     maxLen;
    blt_otss_cpProcessCb_t processCb;
} blt_otss_cp_data_t;

static int blt_otss_init(u8 initType, const void *param);
static int blt_otss_loop(u16 connHandle);
static int blt_otss_connect(u16 connHandle, prf_acl_state_enum connState);
static int blt_otss_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);

_attribute_ble_data_retention_ static u8                                 l2capOtsCocBuffer[COC_MODULE_BUFFER_SIZE(STACK_PRF_ACL_CONN_MAX_NUM, STACK_PRF_ACL_CONN_MAX_NUM, 0, OTSS_L2CAP_MTU)];
_attribute_ble_data_retention_ static const blc_otss_object_callbacks_t *otss_callbacks;
_attribute_ble_data_retention_ /* retention TODO: */
    blc_otp_server_ctrl_t otsp_server_ctrl = {
        .process =
            {
                      .pNext       = NULL,
                      .id          = ESL_OTS_SERVER,
                      .usedAclRole = 0,
                      .init        = blt_otss_init,
                      .connect     = blt_otss_connect,
                      .discov      = NULL,
                      .loop        = blt_otss_loop,
                      .store       = blt_otss_nv_store,
                      },
};

static void blt_otss_timerReload(blt_otss_timer_t *timer, u8 secTimeout, blt_otss_timer_cb_t cb, void *data)
{
    timer->active       = true;
    timer->secRemaining = secTimeout;
    timer->cb           = cb;
    timer->data         = data;
    timer->tick         = clock_time();
}

static void blt_otss_timerStop(blt_otss_timer_t *timer)
{
    memset(timer, 0, sizeof(*timer));
}

static void blt_otss_timerLoop(blt_otss_timer_t *timer)
{
    if (!timer->active) {
        return;
    }

    if (!clock_time_exceed(timer->tick, 1000 * 1000)) {
        return;
    }

    timer->secRemaining--;
    timer->tick = clock_time();
    if (timer->secRemaining) {
        return;
    }

    timer->active = false;
    if (timer->cb) {
        timer->cb(timer->data);
    }
}

static blc_otp_server_client_t *blt_otss_server_client(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);

    return idx >= 0 ? &otsp_server_ctrl.otsServerClients[idx] : NULL;
}

void blc_esl_registerOTSControlServer(const blc_otss_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&otsp_server_ctrl, param);
}

blc_otp_server_t *blt_otp_getServerInst(u16 connHandle)
{
#if (0) //TODO
    int ret = blc_audio_getAclRole(connHandle);
    if (ret < 0) {
        BLT_OTS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            blt_audio_sendSvrGapRoleErrEvt(connHandle, ESL_OTS_SERVER, ret);
        }

        return NULL;
    }
#else
    (void)connHandle;
#endif

    return &otsp_server_ctrl.otpServer;
}

static int blt_otss_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    blc_otp_server_t        *server        = blt_otp_getServerInst(connHandle);
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    if (!server || !server_client) {
        return 0;
    }

    if (nvState == PRF_NV_STATE_STORE) {
        blt_otss_nv_info_t nvInfo;

        memcpy(nvInfo.oacpCccVal, server_client->oacpCccVal, sizeof(nvInfo.oacpCccVal));
        memcpy(nvInfo.olcpCccVal, server_client->olcpCccVal, sizeof(nvInfo.olcpCccVal));

        U8_TO_STREAM(param->dataPtr, sizeof(blt_otss_nv_info_t));
        U8_TO_STREAM(param->dataPtr, ESL_OTS_SERVER);
        STR_TO_STREAM(param->dataPtr, &nvInfo, sizeof(blt_otss_nv_info_t));
        param->currentTotalLen += 2 + sizeof(blt_otss_nv_info_t);
    } else if (nvState == PRF_NV_STATE_LOAD) {
        blt_otss_nv_info_t *nvInfo = (blt_otss_nv_info_t *)param->dataPtr;

        memcpy(server_client->oacpCccVal, nvInfo->oacpCccVal, sizeof(nvInfo->oacpCccVal));
        memcpy(server_client->olcpCccVal, nvInfo->olcpCccVal, sizeof(nvInfo->olcpCccVal));
    }

    return 0;
}

static void blt_otss_initOtsFeature(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS feature char: %d", p->num);
    } else {
        otss->otsFeatureHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectName(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Name char: %d", p->num);
    } else {
        otss->otsObjectNameHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectType(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Type char: %d", p->num);
    } else {
        otss->otsObjectTypeHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectSize(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Size char: %d", p->num);
    } else {
        otss->otsObjectSizeHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectFirstCreated(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object First Created char: %d", p->num);
    } else {
        otss->otsObjectFirstCreatedHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectLastModified(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Last Modified char: %d", p->num);
    } else {
        otss->otsObjectLastModifiedHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectId(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object ID char: %d", p->num);
    } else {
        otss->otsObjectIdHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectProperties(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Properties char: %d", p->num);
    } else {
        otss->otsObjectPropertiesHdl = p->charHandle;
    }
}

static void blt_otss_initOtsObjectActionControlPoint(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Action Control Point char: %d", p->num);
    } else {
        otss->otsOacpHdl = p->charHandle;
        // TODO: Obtain CCC handle in other way as CCC may occupy another handle
        otss->otsOacpCccHdl = p->cccHandle;
    }
}

static void blt_otss_initOtsObjectListControlPoint(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object List Control Point char: %d", p->num);
    } else {
        otss->otsOlcpHdl = p->charHandle;
        // TODO: Obtain CCC handle in other way as CCC may occupy another handle
        otss->otsOlcpCccHdl = p->cccHandle;
    }
}

static void blt_otss_initOtsObjectListFilter(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num >= ARRAY_SIZE(otss->otsObjectListFilterHdl)) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object List Filter char: %d", p->num);
    } else {
        otss->otsObjectListFilterHdl[p->num] = p->charHandle;
    }
}

static void blt_otss_initOtsObjectChanged(atts_foundCharParam_t *p, void *input)
{
    blc_otp_server_t *otss = (blc_otp_server_t *)input;
    if (p->num) {
        BLT_OTS_LOG("ERR: OTS: Invalid num of OTS Object Changed char: %d", p->num);
    } else {
        otss->otsObjectChangedHdl = p->charHandle;
    }
}

static const atts_findCharList_t otssChar[] = {
    {
        .charUuid    = characteristicOtsFeatureUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsFeature,
    },
    {
        .charUuid    = characteristicObjectNameUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectName,
    },
    {
        .charUuid    = characteristicObjectTypeUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectType,
    },
    {
        .charUuid    = characteristicObjectSizeUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectSize,
    },
    {
        .charUuid    = characteristicObjectFirstCreatedUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectFirstCreated,
    },
    {
        .charUuid    = characteristicObjectLastModifiedUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectLastModified,
    },
    {
        .charUuid    = characteristicObjectIdUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectId,
    },
    {
        .charUuid    = characteristicObjectPropertiesUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectProperties,
    },
    {
        .charUuid    = characteristicObjectActionControlPointUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectActionControlPoint,
    },
    {
        .charUuid    = characteristicObjectListControlPointUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectListControlPoint,
    },
    {
        .charUuid    = characteristicObjectListFilterUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectListFilter,
    },
    {
        .charUuid    = characteristicObjectChangedUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback  = blt_otss_initOtsObjectChanged,
    },
};

static ble_sts_t blt_otss_updateOtsFeature(u16 connHandle, u32 oacpFeature, u32 olcpFeature)
{
    blc_otp_server_t *server = blt_otp_getServerInst(connHandle);
    u8               *value;

    value = blc_gatts_getAttributeValueByHandle(connHandle, server->otsFeatureHdl);
    if (!value) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    U32_TO_STREAM(value, oacpFeature & OTSS_SUPPORTED_OACP_FEATURES_MASK);
    U32_TO_STREAM(value, olcpFeature & OTSS_SUPPORTED_OLCP_FEATURES_MASK);

    return BLE_SUCCESS;
}

static void blt_otss_l2capInit(void)
{
    blc_coc_initParam_t regParam = {
        .MTU           = OTSS_L2CAP_MTU,
        .SPSM          = OTS_L2CAP_SPSM,
        .createConnCnt = STACK_PRF_ACL_CONN_MAX_NUM,
        .cocCidCnt     = 1,
    };

    blc_l2cap_registerCocModule(&regParam, l2capOtsCocBuffer, sizeof(l2capOtsCocBuffer));
}

static void blt_otss_serviceInit(const void *param)
{
    blc_otp_server_t          *server    = blt_otp_getServerInst(0xFFFF);
    const blc_otss_regParam_t *otssParam = param ? (const blc_otss_regParam_t *)param : &defaultOtpsParam;
    memset((u8 *)server, 0, sizeof(server));
    blc_atts_findCharacteristicByServiceUuid(serviceObjectTransferUuid, ATT_16_UUID_LEN, otssChar, ARRAY_SIZE(otssChar), (void *)server);
    blt_otss_updateOtsFeature(0xFFFF, otssParam->oacp_features, otssParam->olcp_features);
    blt_otss_l2capInit();
}

static int blt_otss_readName(blc_otp_server_client_t *server_client, u8 **outValue, u16 *outValueLen)
{
    if (!server_client->current_object || !ots_server_data_get_name(server_client->current_object, outValue, outValueLen)) {
        return BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NOT_SELECTED;
    }

    return ATT_SUCCESS;
}

static int blt_otss_readObjectId(blc_otp_server_client_t *server_client, u8 **outValue, u16 *outValueLen)
{
    u64 id;

    if (!server_client->current_object) {
        return BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NOT_SELECTED;
    }

    id = ots_server_data_get_object_id((struct ots_server_data_object *)server_client->current_object);
    u48_to_bstream_le(id, server_client->objectIdVal);

    *outValue    = server_client->objectIdVal;
    *outValueLen = sizeof(server_client->objectIdVal);

    return ATT_SUCCESS;
}

static int blt_otss_readObjectProperties(blc_otp_server_client_t *server_client, u8 **outValue, u16 *outValueLen)
{
    u32 properties;

    if (!server_client->current_object) {
        return BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NOT_SELECTED;
    }

    if (!ots_server_data_get_properties((struct ots_server_data_object *)server_client->current_object, &properties)) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    u32_to_bstream_le(properties, server_client->objectPropertiesVal);
    *outValue    = server_client->objectPropertiesVal;
    *outValueLen = sizeof(server_client->objectPropertiesVal);

    return ATT_SUCCESS;
}

static int blt_otss_readObjectSize(blc_otp_server_client_t *server_client, u8 **outValue, u16 *outValueLen)
{
    u32 currentSize, allocatedSize;
    u8 *ptr;

    if (!server_client->current_object) {
        return BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NOT_SELECTED;
    }

    if (!ots_server_data_get_size((struct ots_server_data_object *)server_client->current_object, &allocatedSize, &currentSize)) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    ptr = server_client->objectSizeVal;
    U32_TO_STREAM(ptr, currentSize);
    U32_TO_STREAM(ptr, allocatedSize);

    *outValue    = server_client->objectSizeVal;
    *outValueLen = sizeof(server_client->objectSizeVal);

    return ATT_SUCCESS;
}

static int blt_otss_readObjectType(blc_otp_server_client_t *server_client, u8 **outValue, u16 *outValueLen)
{
    uuid_t type;
    u8    *ptr;

    if (!server_client->current_object) {
        return BLC_OTS_APPLICATION_ERROR_CODE_OBJECT_NOT_SELECTED;
    }

    if (!ots_server_data_get_type((struct ots_server_data_object *)server_client->current_object, &type)) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    ptr = server_client->objectTypeVal;
    if (type.uuidLen == ATT_16_UUID_LEN) {
        U16_TO_STREAM(ptr, type.uuidVal.u16);
        *outValueLen = sizeof(type.uuidVal.u16);
    } else if (type.uuidLen == ATT_128_UUID_LEN) {
        memcpy(ptr, type.uuidVal.u128, sizeof(type.uuidVal.u128));
        *outValueLen = sizeof(type.uuidVal.u128);
    } else {
        return ATT_ERR_UNLIKELY_ERR;
    }

    *outValue = server_client->objectTypeVal;

    return ATT_SUCCESS;
}

static int blt_otss_read_cb(u16 connHandle, u8 opcode, u16 attrHandle, u8 **outValue, u16 *outValueLen)
{
    blc_otp_server_t        *server        = blt_otp_getServerInst(connHandle);
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    (void)opcode;

    if (!server || !server_client) {
        goto failed;
    }

    if (attrHandle == server->otsObjectNameHdl) {
        return blt_otss_readName(server_client, outValue, outValueLen);
    } else if (attrHandle == server->otsOacpCccHdl) {
        *outValue    = server_client->oacpCccVal;
        *outValueLen = sizeof(server_client->oacpCccVal);
        return ATT_SUCCESS;
    } else if (attrHandle == server->otsOlcpCccHdl) {
        *outValue    = server_client->olcpCccVal;
        *outValueLen = sizeof(server_client->olcpCccVal);
        return ATT_SUCCESS;
    } else if (attrHandle == server->otsObjectIdHdl) {
        return blt_otss_readObjectId(server_client, outValue, outValueLen);
    } else if (attrHandle == server->otsObjectTypeHdl) {
        return blt_otss_readObjectType(server_client, outValue, outValueLen);
    } else if (attrHandle == server->otsObjectPropertiesHdl) {
        return blt_otss_readObjectProperties(server_client, outValue, outValueLen);
    } else if (attrHandle == server->otsObjectSizeHdl) {
        return blt_otss_readObjectSize(server_client, outValue, outValueLen);
    }

failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static ble_sts_t blt_otss_notifyOlcp(blc_otp_server_client_t *server_client, u8 reqOpcode, u8 status, u8 *param, u8 paramLen)
{
    u8 *ptr = server_client->olcpIndBuf;

    if (paramLen > (sizeof(server_client->olcpIndBuf) - 3)) {
        return PRF_ERR_INVALID_PARAMETER;
    }

    if (server_client->olcpIndLen || server_client->pendingOlcpCfm) {
        return PRF_ERR_INVALID_PARAMETER;
    }

    U8_TO_STREAM(ptr, BLC_OTS_OLCP_OPCODE_RESPONSE);
    U8_TO_STREAM(ptr, reqOpcode);
    U8_TO_STREAM(ptr, status);
    STR_TO_STREAM(ptr, param, paramLen);
    server_client->olcpIndLen = ptr - (u8 *)server_client->olcpIndBuf;

    return BLE_SUCCESS;
}

static ble_sts_t blt_otss_notifyOacp(blc_otp_server_client_t *server_client, u8 reqOpcode, u8 status, u8 *param, u8 paramLen)
{
    u8 *ptr = server_client->oacpIndBuf;

    if (paramLen > (sizeof(server_client->oacpIndBuf) - 3)) {
        return PRF_ERR_INVALID_PARAMETER;
    }

    U8_TO_STREAM(ptr, BLC_OTS_OACP_OPCODE_RESPONSE);
    U8_TO_STREAM(ptr, reqOpcode);
    U8_TO_STREAM(ptr, status);
    STR_TO_STREAM(ptr, param, paramLen);
    server_client->oacpIndLen = ptr - (u8 *)server_client->oacpIndBuf;

    return BLE_SUCCESS;
}

static bool blt_otss_getOtsFeatures(u16 connHandle, u32 *oacpFeature, u32 *olcpFeature)
{
    blc_otp_server_t *server = blt_otp_getServerInst(connHandle);
    u32               oacp, olcp;
    u8               *value;

    value = blc_gatts_getAttributeValueByHandle(connHandle, server->otsFeatureHdl);
    if (!value) {
        return false;
    }

    STREAM_TO_U32(oacp, value);
    STREAM_TO_U32(olcp, value);

    if (oacpFeature) {
        *oacpFeature = oacp;
    }

    if (olcpFeature) {
        *olcpFeature = olcp;
    }

    return true;
}

static int blt_otss_writeOlcpFirstCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *obj = ots_server_data_get_first_object();
    blc_ots_olcp_result_code_t     res;

    (void)valueLen;
    (void)connHandle;

    if (!obj) {
        res = BLC_OTS_OLCP_RESULT_CODE_NO_OBJECT;
    } else {
        server_client->current_object = obj;
        res                           = BLC_OTS_OLCP_RESULT_CODE_SUCCESS;
    }

    blt_otss_notifyOlcp(server_client, writeValue[0], res, NULL, 0);

    return ATT_SUCCESS;
}

static int blt_otss_writeOlcpLastCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *obj = ots_server_data_get_last_object();
    blc_ots_olcp_result_code_t     res;

    (void)valueLen;
    (void)connHandle;

    if (!obj) {
        res = BLC_OTS_OLCP_RESULT_CODE_NO_OBJECT;
    } else {
        server_client->current_object = obj;
        res                           = BLC_OTS_OLCP_RESULT_CODE_SUCCESS;
    }

    blt_otss_notifyOlcp(server_client, writeValue[0], res, NULL, 0);

    return ATT_SUCCESS;
}

static int blt_otss_writeOlcpPreviousCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *obj;
    blc_ots_olcp_result_code_t     res;

    (void)valueLen;
    (void)connHandle;

    if (!server_client->current_object) {
        obj = ots_server_data_get_last_object();
    } else {
        obj = ots_server_data_get_prev_object((struct ots_server_data_object *)server_client->current_object);
    }

    if (!obj) {
        res = server_client->current_object ? BLC_OTS_OLCP_RESULT_CODE_OUT_OF_BOUNDS : BLC_OTS_OLCP_RESULT_CODE_NO_OBJECT;
    } else {
        server_client->current_object = obj;
        res                           = BLC_OTS_OLCP_RESULT_CODE_SUCCESS;
    }

    blt_otss_notifyOlcp(server_client, writeValue[0], res, NULL, 0);

    return ATT_SUCCESS;
}

static int blt_otss_writeOlcpNextCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *obj;
    blc_ots_olcp_result_code_t     res;

    (void)valueLen;
    (void)connHandle;

    if (!server_client->current_object) {
        obj = ots_server_data_get_first_object();
    } else {
        obj = ots_server_data_get_next_object((struct ots_server_data_object *)server_client->current_object);
    }

    if (!obj) {
        res = server_client->current_object ? BLC_OTS_OLCP_RESULT_CODE_OUT_OF_BOUNDS : BLC_OTS_OLCP_RESULT_CODE_NO_OBJECT;
    } else {
        server_client->current_object = obj;
        res                           = BLC_OTS_OLCP_RESULT_CODE_SUCCESS;
    }

    blt_otss_notifyOlcp(server_client, writeValue[0], res, NULL, 0);

    return ATT_SUCCESS;
}

static int blt_otss_writeOlcpGotoCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *obj;
    blc_ots_olcp_result_code_t     res;

    (void)valueLen;
    (void)connHandle;
    u64 id;

    id  = bstream_to_u48_le(&writeValue[1]);
    obj = ots_server_data_get_object_by_object_id(id);
    if (obj) {
        server_client->current_object = obj;
        res                           = BLC_OTS_OLCP_RESULT_CODE_SUCCESS;
    } else {
        res = BLC_OTS_OLCP_RESULT_CODE_OBJECT_ID_NOT_FOUND;
    }

    blt_otss_notifyOlcp(server_client, writeValue[0], res, NULL, 0);

    return ATT_SUCCESS;
}

_attribute_data_retention_ static blt_otss_cp_data_t otss_olcp_data[] = {
    {.opcode = BLC_OTS_OLCP_OPCODE_FIRST,                        .mask = 0,                                                        .minLen = sizeof(u8),     .maxLen = sizeof(u8),     .processCb = blt_otss_writeOlcpFirstCback   },
    {.opcode = BLC_OTS_OLCP_OPCODE_LAST,                         .mask = 0,                                                        .minLen = sizeof(u8),     .maxLen = sizeof(u8),     .processCb = blt_otss_writeOlcpLastCback    },
    {.opcode = BLC_OTS_OLCP_OPCODE_PREVIOUS,                     .mask = 0,                                                        .minLen = sizeof(u8),     .maxLen = sizeof(u8),     .processCb = blt_otss_writeOlcpPreviousCback},
    {.opcode = BLC_OTS_OLCP_OPCODE_NEXT,                         .mask = 0,                                                        .minLen = sizeof(u8),     .maxLen = sizeof(u8),     .processCb = blt_otss_writeOlcpNextCback    },
    // TODO below are non mandatory opcodes that can be also handled
    {.opcode    = BLC_OTS_OLCP_OPCODE_GOTO,
     .mask      = BLC_OTS_OLCP_FEATURE_OLCP_GO_TO_OPCODE_SUPPORTED,
     .minLen    = sizeof(u8) + sizeof(blc_ots_object_id_t),
     .maxLen    = sizeof(u8) + sizeof(blc_ots_object_id_t),
     .processCb = blt_otss_writeOlcpGotoCback                                                                                                                                                                                      },
    {.opcode = BLC_OTS_OLCP_OPCODE_ORDER,                        .mask = BLC_OTS_OLCP_FEATURE_OLCP_ORDER_OPCODE_SUPPORTED,         .minLen = 2 * sizeof(u8), .maxLen = 2 * sizeof(u8), .processCb = NULL                           },
    {.opcode    = BLC_OTS_OLCP_OPCODE_REQUEST_NUMBER_OF_OBJECTS,
     .mask      = BLC_OTS_OLCP_FEATURE_OLCP_REQUEST_NUMBER_OF_OBJECTS_OPCODE_SUPPORTED,
     .minLen    = sizeof(u8),
     .maxLen    = sizeof(u8),
     .processCb = NULL                                                                                                                                                                                                             },
    {.opcode = BLC_OTS_OLCP_OPCODE_CLEAR_MARKING,                .mask = BLC_OTS_OLCP_FEATURE_OLCP_CLEAR_MARKING_OPCODE_SUPPORTED, .minLen = sizeof(u8),     .maxLen = sizeof(u8),     .processCb = NULL                           }
};

static int blt_otss_writeOlcpCback(u16 connHandle, u8 opcode, u8 *writeValue, u16 valueLen)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);
    blt_otss_cp_data_t      *data          = NULL;
    u32                      olcpFeatures;
    u16                      ccc;

    (void)opcode;

    BYTE_TO_UINT16(ccc, server_client->olcpCccVal);
    if (!(ccc & INDICATIONS_ENABLED)) {
        return ATT_ERR_CCC_DESCRIPTOR_IMPROPERLY_CONFIGURED;
    }

    if (valueLen < sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (!blt_otss_getOtsFeatures(connHandle, NULL, &olcpFeatures)) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    foreach_arr(i, otss_olcp_data)
    {
        if (otss_olcp_data[i].opcode == writeValue[0]) {
            data = &otss_olcp_data[i];
            break;
        }
    }

    if (!data) {
        goto not_supported;
    }

    if (data->minLen > valueLen || data->maxLen < valueLen) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (data->mask != (data->mask & olcpFeatures) || !data->processCb) {
        goto not_supported;
    }

    return data->processCb(server_client, connHandle, writeValue, valueLen);

not_supported:
    blt_otss_notifyOlcp(server_client, writeValue[0], BLC_OTS_OLCP_RESULT_CODE_OP_CODE_NOT_SUPPORTED, NULL, 0);

    return ATT_SUCCESS;
}

static void blt_otss_destroyPendingOacpTransfer(u16 connHandle)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    if (server_client->pendingOacpTransfer) {
        struct ots_server_data_object *obj;

        obj = ots_server_data_get_object_by_object_id(server_client->pendingTransferOp.id);
        ots_server_data_set_locked(obj, connHandle, false);

        blt_otss_timerStop(&server_client->oacpTimer);

        server_client->pendingOacpTransfer = 0;
        server_client->pendingOacpCfm      = 0;
    }
}

static void blt_otss_oacpTimeoutExpired(void *data)
{
    u16                      connHandle    = (u16)(u32)data;
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    blt_otss_destroyPendingOacpTransfer(connHandle);

    if (server_client->transferChannelOpened) {
        blc_l2cap_disconnectCocChannel(connHandle, server_client->transferChannelScid);
    }
}

static int blt_otss_writeOacpReadCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *currentObject = (struct ots_server_data_object *)server_client->current_object;
    u32                            properties, currentSize;
    u32                            offset, length;
    u8                             res;

    (void)valueLen;

    if (!currentObject) {
        /*
         * OTS 3.3.2.5 Read procedure Table 3.16: Requirements for Control Point Error Responses to the Read Op Code:
         * "Error condition: The Current Object is an Invalid Object."
         */
        res = BLC_OTS_OACP_RESULT_CODE_INVALID_OBJECT;
        goto done;
    }

    if (server_client->pendingOacpTransfer) {
        return ATT_ERR_PROCEDURE_ALREADY_IN_PROGRESS;
    }

    if (!ots_server_data_get_properties(currentObject, &properties) || !(properties & BLC_OTS_OBJECT_PROPERTIES_READ)) {
        /*
         * OTS 3.3.2.5 Read procedure Table 3.16: Requirements for Control Point Error Responses to the Read Op Code:
         * "Error condition: The object's properties do not permit reading the object."
         */
        res = BLC_OTS_OACP_RESULT_CODE_PROCEDURE_NOT_PEMITTED;
        goto done;
    }

    if (!server_client->transferChannelOpened) {
        /*
         * OTS 3.3.2.5 Read procedure Table 3.16: Requirements for Control Point Error Responses to the Read Op Code:
         * "Error condition: An Object Transfer Channel was not available for use."
         */
        res = BLC_OTS_OACP_RESULT_CODE_CHANNEL_UNAVAILABLE;
        goto done;
    }

    BYTE_TO_UINT32(offset, &writeValue[1]);
    BYTE_TO_UINT32(length, &writeValue[5]);

    if (!ots_server_data_get_size(currentObject, NULL, &currentSize) || offset > currentSize || (offset + length) > currentSize) {
        /*
         * OTS 3.3.2.5 Read procedure Table 3.16: Requirements for Control Point Error Responses to the Read Op Code:
         * "Error condition: The value of the Offset parameter exceeds the value of the Current Size field of the
         * Object Size characteristic."
         * "Error condition: The sum of the values of the Offset and Length parameters exceeds the value of the
         * Current Size field of the Object Size characteristic."
         */
        res = BLC_OTS_OACP_RESULT_CODE_INVALID_PARAMETER;
        goto done;
    }

    if (!ots_server_data_set_locked(currentObject, connHandle, true)) {
        /*
         * OTS 3.3.2.5 Read procedure Table 3.16: Requirements for Control Point Error Responses to the Read Op Code:
         * "Error condition: An object transfer is already in progress that is using the Current Object."
         */
        res = BLC_OTS_OACP_RESULT_CODE_OBJECT_LOCKED;
        goto done;
    }

    res = BLC_OTS_OACP_RESULT_CODE_SUCCESS;

done:
    if (blt_otss_notifyOacp(server_client, writeValue[0], res, NULL, 0) != BLE_SUCCESS) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    if (res == BLC_OTS_OACP_RESULT_CODE_SUCCESS) {
        blt_otss_timerReload(&server_client->oacpTimer, OTS_OACP_TIMEOUT_SEC, blt_otss_oacpTimeoutExpired, (void *)(u32)connHandle);
        server_client->pendingOacpTransfer                 = 1;
        server_client->pendingTransferOp.pendingOacpOpcode = writeValue[0];
        server_client->pendingTransferOp.id                = ots_server_data_get_object_id(currentObject);
        server_client->pendingTransferOp.read_op.offset    = offset;
        server_client->pendingTransferOp.read_op.remaining = length;
    }

    return ATT_SUCCESS;
}

static int blt_otss_writeOacpWriteCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    struct ots_server_data_object *currentObject = (struct ots_server_data_object *)server_client->current_object;
    u32                            properties, offset, length, oacpFeatures = 0;
    u32                            allocatedSize = 0;
    u32                            currentSize   = 0;
    u8                             res, mode;

    (void)valueLen;

    if (!currentObject) {
        res = BLC_OTS_OACP_RESULT_CODE_INVALID_OBJECT;
        goto done;
    }

    if (server_client->pendingOacpTransfer) {
        return ATT_ERR_PROCEDURE_ALREADY_IN_PROGRESS;
    }

    if (!ots_server_data_get_properties(currentObject, &properties) || !(properties & BLC_OTS_OBJECT_PROPERTIES_WRITE)) {
        res = BLC_OTS_OACP_RESULT_CODE_PROCEDURE_NOT_PEMITTED;
        goto done;
    }

    BYTE_TO_UINT32(offset, &writeValue[1]);
    BYTE_TO_UINT32(length, &writeValue[5]);
    BYTES_TO_UINT8(mode, &writeValue[9]);
    blt_otss_getOtsFeatures(connHandle, &oacpFeatures, NULL);

    if (!(mode & BLC_OTS_OACP_WRITE_MODE_TRUNCATE)) {
        if (!(oacpFeatures & BLC_OTS_OACP_FEATURE_PATCHING_OF_OBJECTS_SUPPORTED) || !(properties & BLC_OTS_OBJECT_PROPERTIES_PATCH)) {
            /*
             * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
             * "Error condition: Patching was attempted but patching is not supported by the Server."
             * "Error condition: Patching was attempted but the object's properties do not permit patching of the object contents."
             */
            res = BLC_OTS_OACP_RESULT_CODE_PROCEDURE_NOT_PEMITTED;
            goto done;
        }
    } else if (!(properties & BLC_OTS_OBJECT_PROPERTIES_TRUNCATE)) {
        /*
         * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
         * "Error condition: Truncation was attempted but the object's properties do not permit truncation of the object contents."
         */
        res = BLC_OTS_OACP_RESULT_CODE_PROCEDURE_NOT_PEMITTED;
        goto done;
    }

    if (!server_client->transferChannelOpened) {
        /*
         * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
         * "Error condition: An Object Transfer Channel was not available for use."
         */
        res = BLC_OTS_OACP_RESULT_CODE_CHANNEL_UNAVAILABLE;
        goto done;
    }

    if (~BLC_OTS_OACP_WRITE_MODE_TRUNCATE & mode) {
        /*
         * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
         * "Error condition: The Mode parameter contains an RFU value."
         */
        res = BLC_OTS_OACP_RESULT_CODE_INVALID_PARAMETER;
        goto done;
    }

    ots_server_data_get_size(currentObject, &allocatedSize, &currentSize);
    if (offset > currentSize) {
        /*
         * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
         * "Error condition: The value of the Offset parameter exceeds the value of the Current Size field of the
         * Object Size characteristic."
         */
        res = BLC_OTS_OACP_RESULT_CODE_INVALID_PARAMETER;
        goto done;
    }

    if (!(BLC_OTS_OACP_WRITE_MODE_TRUNCATE & mode) && (offset + length) > currentSize) {
        /*
         * When truncate bit is NOT set, current size is not supposed to change
         */
        res = BLC_OTS_OACP_RESULT_CODE_INVALID_PARAMETER;
        goto done;
    }

    if (((offset + length) > allocatedSize) && (BLC_OTS_OACP_WRITE_MODE_TRUNCATE & mode)) {
        if (!(oacpFeatures & BLC_OTS_OACP_FEATURE_APPENDING_ADD_DATA_TO_OBJECTS_SUPPORTED)) {
            /*
             * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
             * "Error condition: The sum of the values of the Offset and Length parameters exceeds the value of the Allocated
             * Size field of the Object Size characteristic AND the Server does NOT support appending additional data to an object."
             */
            res = BLC_OTS_OACP_RESULT_CODE_INVALID_PARAMETER;
            goto done;
        } else {
            blc_ots_object_id_t obj_id;

            u48_to_bstream_le(ots_server_data_get_object_id(currentObject), obj_id.objectId);
            if (!otss_callbacks || !otss_callbacks->append_cb || !otss_callbacks->append_cb(&obj_id, offset + length)) {
                /*
                 * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
                 * "Error condition: The value of the Length parameter exceeds the number of octets that the Server has
                 * the capacity to read from the object."
                 */
                res = BLC_OTS_OACP_RESULT_CODE_INSUFFICIENT_RESOURCES;
                goto done;
            } else {
                allocatedSize = offset + length;
                ots_server_data_set_size(currentObject, allocatedSize, currentSize);
            }
        }
    }

    if (!ots_server_data_set_locked(currentObject, connHandle, true)) {
        /*
         * OTS 3.3.2.6 Write procedure Table 3.18: Requirements for Control Point Error Responses to the Write Op Code:
         * "Error condition: An object transfer is already in progress that is using the Current Object."
         */
        res = BLC_OTS_OACP_RESULT_CODE_OBJECT_LOCKED;
        goto done;
    }

    res = BLC_OTS_OACP_RESULT_CODE_SUCCESS;

done:

    if (blt_otss_notifyOacp(server_client, writeValue[0], res, NULL, 0) != BLE_SUCCESS) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    if (res == BLC_OTS_OACP_RESULT_CODE_SUCCESS) {
        blt_otss_timerReload(&server_client->oacpTimer, OTS_OACP_TIMEOUT_SEC, blt_otss_oacpTimeoutExpired, (void *)(u32)connHandle);
        server_client->pendingOacpTransfer                  = 1;
        server_client->pendingTransferOp.pendingOacpOpcode  = writeValue[0];
        server_client->pendingTransferOp.id                 = ots_server_data_get_object_id(currentObject);
        server_client->pendingTransferOp.write_op.offset    = offset;
        server_client->pendingTransferOp.write_op.remaining = length;
        server_client->pendingTransferOp.write_op.mode      = mode;
        if (mode & BLC_OTS_OACP_WRITE_MODE_TRUNCATE) {
            /*
             * OTS specification 3.3.2.6 Write Procedure:
             * "If the Truncate bit of the Mode parameter is set to 1 (True) and the Server responds with the "Success"
             * result code, the object shall be truncated such that its Current Size becomes equal to the value of the
             * Offset parameter before new data is written to the object."
             */
            ots_server_data_set_size(currentObject, allocatedSize, offset);
        }
    }

    return ATT_SUCCESS;
}

_attribute_data_retention_ static blt_otss_cp_data_t otss_oacp_data[] = {
    {.opcode    = BLC_OTS_OACP_OPCODE_CREATE,
     .mask      = BLC_OTS_OACP_FEATURE_OACP_CREATE_OPCODE_SUPPORTED,
     .minLen    = sizeof(u32) + ATT_16_UUID_LEN,
     .maxLen    = sizeof(u32) + ATT_128_UUID_LEN,
     .processCb = NULL                                                                                                                                                             },
    {.opcode = BLC_OTS_OACP_OPCODE_DELETE,                .mask = BLC_OTS_OACP_FEATURE_OACP_DELETE_OPCODE_SUPPORTED,  .minLen = sizeof(u8), .maxLen = sizeof(u8), .processCb = NULL},
    {.opcode    = BLC_OTS_OACP_OPCODE_CALCULATE_CHECKSUM,
     .mask      = BLC_OTS_OACP_FEATURE_OACP_CALCULATE_CHECKSUM_OPCODE_SUPPORTED,
     .minLen    = sizeof(u32) + sizeof(u32),
     .maxLen    = sizeof(u32) + sizeof(u32),
     .processCb = NULL                                                                                                                                                             },
    {.opcode = BLC_OTS_OACP_OPCODE_EXECUTE,               .mask = BLC_OTS_OACP_FEATURE_OACP_EXECUTE_OPCODE_SUPPORTED, .minLen = sizeof(u8), .maxLen = 128,        .processCb = NULL},
    {.opcode    = BLC_OTS_OACP_OPCODE_READ,
     .mask      = BLC_OTS_OACP_FEATURE_OACP_READ_OPCODE_SUPPORTED,
     .minLen    = 2 * sizeof(u32) + sizeof(u8),
     .maxLen    = 2 * sizeof(u32) + sizeof(u8),
     .processCb = blt_otss_writeOacpReadCback                                                                                                                                      },
    {.opcode    = BLC_OTS_OACP_OPCODE_WRITE,
     .mask      = BLC_OTS_OACP_FEATURE_OACP_WRITE_OPCODE_SUPPORTED,
     .minLen    = 2 * sizeof(u32) + 2 * sizeof(u8),
     .maxLen    = 2 * sizeof(u32) + 2 * sizeof(u8),
     .processCb = blt_otss_writeOacpWriteCback                                                                                                                                     },
    {.opcode = BLC_OTS_OACP_OPCODE_ABORT,                 .mask = BLC_OTS_OACP_FEATURE_OACP_ABORT_OPCODE_SUPPORTED,   .minLen = sizeof(u8), .maxLen = sizeof(u8), .processCb = NULL},
};

static int blt_otss_writeOacpCback(u16 connHandle, u8 opcode, u8 *writeValue, u16 valueLen)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);
    blt_otss_cp_data_t      *data          = NULL;
    u32                      oacpFeatures;
    u16                      ccc;

    (void)opcode;

    BYTE_TO_UINT16(ccc, server_client->oacpCccVal);
    if (!(ccc & INDICATIONS_ENABLED)) {
        return ATT_ERR_CCC_DESCRIPTOR_IMPROPERLY_CONFIGURED;
    }

    if (valueLen < sizeof(u8)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (!blt_otss_getOtsFeatures(connHandle, &oacpFeatures, NULL)) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    foreach_arr(i, otss_oacp_data)
    {
        if (otss_oacp_data[i].opcode == writeValue[0]) {
            data = &otss_oacp_data[i];
            break;
        }
    }

    if (!data) {
        goto not_supported;
    }

    if (data->minLen > valueLen || data->maxLen < valueLen) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (server_client->pendingOacpInd || server_client->pendingOacpCfm) {
        return ATT_ERR_PROCEDURE_ALREADY_IN_PROGRESS;
    }

    if (data->mask != (data->mask & oacpFeatures) || !data->processCb) {
        goto not_supported;
    }

    return data->processCb(server_client, connHandle, writeValue, valueLen);

not_supported:
    if (blt_otss_notifyOacp(server_client, writeValue[0], BLC_OTS_OACP_RESULT_CODE_OP_CODE_NOT_SUPPORTED, NULL, 0) != BLE_SUCCESS) {
        return ATT_ERR_UNLIKELY_ERR;
    }

    return ATT_SUCCESS;
}

static int blt_otss_writeOlcpCccCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    if (valueLen != sizeof(server_client->olcpCccVal)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (memcmp(server_client->olcpCccVal, writeValue, sizeof(server_client->olcpCccVal))) {
        memcpy(server_client->olcpCccVal, writeValue, valueLen);
        blt_prf_updateNvData(connHandle);
    }

    return ATT_SUCCESS;
}

static int blt_otss_writeOacpCccCback(blc_otp_server_client_t *server_client, u16 connHandle, u8 *writeValue, u16 valueLen)
{
    if (valueLen != sizeof(server_client->oacpCccVal)) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    if (memcmp(server_client->oacpCccVal, writeValue, sizeof(server_client->oacpCccVal))) {
        memcpy(server_client->oacpCccVal, writeValue, valueLen);
        blt_prf_updateNvData(connHandle);
    }

    return ATT_SUCCESS;
}

static int blt_otss_writeCback(u16 connHandle, u8 opcode, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    blc_otp_server_t        *server        = blt_otp_getServerInst(connHandle);
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    if (!server || !server_client) {
        goto failed;
    }

    if (attrHandle == server->otsOlcpHdl) {
        return blt_otss_writeOlcpCback(connHandle, opcode, writeValue, valueLen);
    } else if (attrHandle == server->otsOacpHdl) {
        return blt_otss_writeOacpCback(connHandle, opcode, writeValue, valueLen);
    } else if (attrHandle == server->otsOlcpCccHdl) {
        return blt_otss_writeOlcpCccCback(server_client, connHandle, writeValue, valueLen);
    } else if (attrHandle == server->otsOacpCccHdl) {
        return blt_otss_writeOacpCccCback(server_client, connHandle, writeValue, valueLen);
    }

failed:
    return ATT_ERR_ATTR_NOT_FOUND;
}

static int blt_otss_connect(u16 connHandle, prf_acl_state_enum connState)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    if (!server_client) {
        return 0;
    }

    if (connState == PRF_ACL_STATE_DISCONN) {
        blt_otss_destroyPendingOacpTransfer(connHandle);

        memset(server_client, 0, sizeof(*server_client));
    } else {
        // set first object as current object
        server_client->current_object = ots_server_data_get_first_object();
    }

    return 0;
}

static int blt_otss_init(u8 initType, const void *param)
{
#if (0)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_ots_server_t)), blc_ots_server_t);
#endif

    if (initType == PRF_PROC_INIT) {
        blc_svc_addPrimaryOtsGroup();
        blc_svc_primaryOtsCbackRegister(blt_otss_read_cb, blt_otss_writeCback);
        BLT_OTS_LOG("Server init");
        foreach_arr(i, otsp_server_ctrl.otsServerClients)
        {
            memset(&otsp_server_ctrl.otsServerClients[i], 0, sizeof(otsp_server_ctrl.otsServerClients[i]));
        }
        blt_otss_serviceInit(param);
    }
    //    else if (initType == PRF_PROC_DEINIT) {
    //        blc_svc_removePrimaryOtsGroup();
    //        BLT_OTS_LOG("Server Deinit");
    //    }
    return 0;
}

void blc_otss_registerCallbacks(const blc_otss_object_callbacks_t *callbacks)
{
    otss_callbacks = callbacks;
}

static void blt_otss_olcpIndCfm(u16 connHandle, u16 scid)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    (void)scid;

    if (server_client) {
        server_client->pendingOlcpCfm = 0;
    }
}

static void blt_otss_oacpReadProcess(u16 connHandle)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    if (server_client && server_client->pendingOacpTransfer && otss_callbacks && otss_callbacks->read_cb) {
        u8                 *buf;
        blc_ots_object_id_t id;
        u16                 len, req_len;

        if (!server_client->pendingTransferOp.read_op.remaining) {
            blt_otss_destroyPendingOacpTransfer(connHandle);
            BLT_OTS_LOG("Read transfer finished");
            return;
        }

        u48_to_bstream_le(server_client->pendingTransferOp.id, id.objectId);
        req_len = server_client->pendingTransferOp.read_op.remaining < server_client->transferChannelMtu ? server_client->pendingTransferOp.read_op.remaining :
                                                                                                           server_client->transferChannelMtu;
        len     = otss_callbacks->read_cb(&id, server_client->pendingTransferOp.read_op.offset, req_len, &buf);
        if (len > req_len) {
            BLT_OTS_LOG("Read callback failed, requested %d bytes, received %d", req_len, len);
            return;
        }

        if (blc_l2cap_sendCocData(connHandle, server_client->transferChannelScid, buf, len) == BLE_SUCCESS) {
            server_client->pendingTransferOp.read_op.offset += len;
            server_client->pendingTransferOp.read_op.remaining -= len;
            blt_otss_timerReload(&server_client->oacpTimer, OTS_OACP_TIMEOUT_SEC, blt_otss_oacpTimeoutExpired, (void *)(u32)connHandle);
        }
    }
}

static void blt_otss_oacpIndCfm(u16 connHandle, u16 scid)
{
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    (void)scid;

    if (server_client) {
        server_client->pendingOacpCfm = 0;
        if (server_client->pendingOacpTransfer && server_client->pendingTransferOp.pendingOacpOpcode == BLC_OTS_OACP_OPCODE_READ) {
            blt_otss_oacpReadProcess(connHandle);
        }
    }
}

static int blt_otss_loop(u16 connHandle)
{
    blc_otp_server_t        *server        = blt_otp_getServerInst(connHandle);
    blc_otp_server_client_t *server_client = blt_otss_server_client(connHandle);

    if (server_client->olcpIndLen) {
        gattsIndValue_t indValue = {
            .cb         = blt_otss_olcpIndCfm,
            .connHandle = connHandle,
            .value      = server_client->olcpIndBuf,
            .valueLen   = server_client->olcpIndLen,
            .attrHandle = server->otsOlcpHdl,
            .scid       = 4,
        };

        if (blc_gatts_indicateValue(&indValue) == BLE_SUCCESS) {
            server_client->olcpIndLen     = 0;
            server_client->pendingOlcpCfm = 1;
        }
    }

    if (server_client->oacpIndLen) {
        gattsIndValue_t indValue = {
            .cb         = blt_otss_oacpIndCfm,
            .connHandle = connHandle,
            .value      = server_client->oacpIndBuf,
            .valueLen   = server_client->oacpIndLen,
            .attrHandle = server->otsOacpHdl,
            .scid       = 4,
        };

        if (blc_gatts_indicateValue(&indValue) == BLE_SUCCESS) {
            server_client->oacpIndLen     = 0;
            server_client->pendingOacpCfm = 1;
        }
    }

    blt_otss_timerLoop(&server_client->oacpTimer);

    return 0;
}

static const u32 properties2oacpFeature[][2] = {
    {BLC_OTS_OBJECT_PROPERTIES_DELETE, BLC_OTS_OACP_FEATURE_OACP_DELETE_OPCODE_SUPPORTED},
    {BLC_OTS_OBJECT_PROPERTIES_EXECUTE, BLC_OTS_OACP_FEATURE_OACP_EXECUTE_OPCODE_SUPPORTED},
    {BLC_OTS_OBJECT_PROPERTIES_READ, BLC_OTS_OACP_FEATURE_OACP_READ_OPCODE_SUPPORTED},
    {BLC_OTS_OBJECT_PROPERTIES_WRITE, BLC_OTS_OACP_FEATURE_OACP_WRITE_OPCODE_SUPPORTED},
    {BLC_OTS_OBJECT_PROPERTIES_APPEND, BLC_OTS_OACP_FEATURE_APPENDING_ADD_DATA_TO_OBJECTS_SUPPORTED},
    {BLC_OTS_OBJECT_PROPERTIES_TRUNCATE, BLC_OTS_OACP_FEATURE_TRUNCATION_OF_OBJECTS_SUPPORTED},
    {BLC_OTS_OBJECT_PROPERTIES_PATCH, BLC_OTS_OACP_FEATURE_PATCHING_OF_OBJECTS_SUPPORTED},
};

static const u32 properties2olcpFeature[][2] = {
    {BLC_OTS_OBJECT_PROPERTIES_MARK, BLC_OTS_OLCP_FEATURE_OLCP_CLEAR_MARKING_OPCODE_SUPPORTED},
};

static bool blt_otss_validate_obj_properties(u16 connHandle, u32 properties)
{
    u32 oacpFeatures, olcpFeatures;

    if (!blt_otss_getOtsFeatures(connHandle, &oacpFeatures, &olcpFeatures)) {
        return false;
    }

    for (u8 i = 0; i < ARRAY_SIZE(properties2oacpFeature); i++) {
        if ((properties2oacpFeature[i][0] & properties) & !(properties2oacpFeature[i][1] & oacpFeatures)) {
            return false;
        }
    }

    for (u8 i = 0; i < ARRAY_SIZE(properties2olcpFeature); i++) {
        if ((properties2olcpFeature[i][0] & properties) & !(properties2olcpFeature[i][1] & olcpFeatures)) {
            return false;
        }
    }

    return true;
}

ble_sts_t blc_otss_objectAdd(blc_ots_object_size_t *size, uuid_t *type, u32 properties, blc_ots_object_id_t *id)
{
    struct ots_server_data_object *obj;
    u64                            obj_id;

    if (!size || !type || size->currentSize > size->allocatedSize || !blt_otss_validate_obj_properties(0xFFFF, properties)) {
        return PRF_ERR_INVALID_PARAMETER;
    }

    obj = ots_server_data_new_object(size->allocatedSize, size->currentSize, type, properties);
    if (!obj) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
    }

    obj_id = ots_server_data_get_object_id(obj);
    u48_to_bstream_le(obj_id, id->objectId);

    return BLE_SUCCESS;
}

ble_sts_t blc_otss_objectSetName(blc_ots_object_id_t *id, u8 *name, u16 length)
{
    struct ots_server_data_object *obj;
    u64                            obj_id;

    obj_id = bstream_to_u48_le(id->objectId);
    obj    = ots_server_data_get_object_by_object_id(obj_id);
    if (!obj) {
        return PRF_ERR_INVALID_PARAMETER;
    }

    return ots_server_data_set_name(obj, name, length) ? BLE_SUCCESS : GATT_ERR_UNSPECIFIED;
}

int blc_otss_hostEventCallback(u32 h, u8 *para, int n)
{
    u8 event = h & 0xFF;

    (void)n;

    switch (event) {
    case GAP_EVT_L2CAP_COC_CONNECT:
    {
        gap_l2cap_cocConnectEvt_t *connEvt       = (gap_l2cap_cocConnectEvt_t *)para;
        blc_otp_server_client_t   *server_client = blt_otss_server_client(connEvt->connHandle);

        BLT_OTS_LOG("COC connected: connHandle: 0x%04X, SPSM: 0x%04X, MTU: 0x%04X, SCID 0x%04X, DCID 0x%04X", connEvt->connHandle, connEvt->spsm, connEvt->mtu, connEvt->srcCid, connEvt->dstCid);

        if (server_client && connEvt->spsm == OTS_L2CAP_SPSM) {
            server_client->transferChannelOpened = 1;
            server_client->transferChannelScid   = connEvt->srcCid;
            server_client->transferChannelDcid   = connEvt->dstCid;
            server_client->transferChannelMtu    = connEvt->mtu;
        }

        break;
    }

    case GAP_EVT_L2CAP_COC_DISCONNECT:
    {
        gap_l2cap_cocDisconnectEvt_t *discEvt       = (gap_l2cap_cocDisconnectEvt_t *)para;
        blc_otp_server_client_t      *server_client = blt_otss_server_client(discEvt->connHandle);

        BLT_OTS_LOG("COC disconnected: connHandle: 0x%04X, SCID 0x%04X, DCID 0x%04X", discEvt->connHandle, discEvt->srcCid, discEvt->dstCid);

        if (server_client && server_client->transferChannelOpened && server_client->transferChannelDcid == discEvt->dstCid) {
            server_client->transferChannelOpened = 0;
            server_client->transferChannelScid   = 0;
            server_client->transferChannelDcid   = 0;
            server_client->transferChannelMtu    = 0;

            blt_otss_destroyPendingOacpTransfer(discEvt->connHandle);
        }

        break;
    }
    case GAP_EVT_L2CAP_COC_RECV_DATA:
    {
        gap_l2cap_cocRecvDataEvt_t *recvDataEvt   = (gap_l2cap_cocRecvDataEvt_t *)para;
        blc_otp_server_client_t    *server_client = blt_otss_server_client(recvDataEvt->connHandle);

        BLT_OTS_LOG("COC data received: connHandle: 0x%04X, DCID 0x%04X, length 0x%04X", recvDataEvt->connHandle, recvDataEvt->dstCid, recvDataEvt->length);
        tlk_printf("recieved\n");
        if (server_client->transferChannelDcid != recvDataEvt->dstCid) {
            tlk_printf("break\n");
            break;
        }

        if (server_client->pendingOacpTransfer && server_client->pendingTransferOp.pendingOacpOpcode == BLC_OTS_OACP_OPCODE_WRITE &&
            (recvDataEvt->length <= server_client->pendingTransferOp.write_op.remaining)) {
            struct ots_server_data_object *obj = ots_server_data_get_object_by_object_id(server_client->pendingTransferOp.id);
            blc_ots_object_id_t            id;
            u32                            allocatedSize;
            tlk_printf("if branch\n");
            if (otss_callbacks && otss_callbacks->write_cb) {
                u48_to_bstream_le(server_client->pendingTransferOp.id, id.objectId);
                tlk_printf("will run call back\n");
                otss_callbacks->write_cb(&id, server_client->pendingTransferOp.write_op.mode, server_client->pendingTransferOp.write_op.offset, recvDataEvt->length, recvDataEvt->data);
                if ((server_client->pendingTransferOp.write_op.mode & BLC_OTS_OACP_WRITE_MODE_TRUNCATE) && ots_server_data_get_size(obj, &allocatedSize, NULL)) {
                    ots_server_data_set_size(obj, allocatedSize, server_client->pendingTransferOp.write_op.offset + recvDataEvt->length);
                }
            }

            server_client->pendingTransferOp.write_op.offset += recvDataEvt->length;
            server_client->pendingTransferOp.write_op.remaining -= recvDataEvt->length;
            if (!server_client->pendingTransferOp.write_op.remaining) {
                blt_otss_destroyPendingOacpTransfer(recvDataEvt->connHandle);
            } else {
                blt_otss_timerReload(&server_client->oacpTimer, OTS_OACP_TIMEOUT_SEC, blt_otss_oacpTimeoutExpired, (void *)(u32)recvDataEvt->connHandle);
            }
        } else {
            /*
             * OTS specification 3.3.2.6.3 Handling Receipt of Unexpected Data: "The Server knows the
             * total number of octets to expect to receive from the Client as this is specified by the
             * Length parameter. In the event that the Server receives data via the Object Transfer
             * Channel in excess of the expected number of octets, the Server shall close the Object
             * Transfer Channel to prevent the Client from sending further data through the Object
             * Transfer Channel. The Server may discard the data it has received."
             */
            blt_otss_destroyPendingOacpTransfer(recvDataEvt->connHandle);
            blc_l2cap_disconnectCocChannel(recvDataEvt->connHandle, server_client->transferChannelScid);
            tlk_printf("else branch\n");
        }

        break;
    }
    case GAP_EVT_L2CAP_COC_SEND_DATA_FINISH:
    {
        gap_l2cap_cocSendDataFinishEvt_t *sendDataFinishEvt = (gap_l2cap_cocSendDataFinishEvt_t *)para;
        blc_otp_server_client_t          *server_client     = blt_otss_server_client(sendDataFinishEvt->connHandle);

        BLT_OTS_LOG("COC data sent: connHandle: 0x%04X, SCID 0x%04X", sendDataFinishEvt->connHandle, sendDataFinishEvt->srcCid);

        if (server_client && server_client->transferChannelOpened && server_client->transferChannelScid == sendDataFinishEvt->srcCid) {
            blt_otss_oacpReadProcess(sendDataFinishEvt->connHandle);
        }

        break;
    }
    default:
        break;
    }

    return 0;
}
