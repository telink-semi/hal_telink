/********************************************************************************************************
 * @file    esls_client.c
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
#include "esls_internal.h"

#define BLC_ESLSC_SENSOR_SIZE_0 3
#define BLC_ESLSC_SENSOR_SIZE_1 5
#define BLC_ESLSC_DISPLAY_SIZE  5

static const blc_gapc_discList_t discEsls;
#define BLC_ESLS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discEsls)

static const blc_gapc_reconnList_t reconnEsls;
#define BLC_ESLS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnEsls)

static void blt_eslsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
static int  blt_eslsc_init(u8 initType, const void *param);
static int  blt_eslsc_connect(u16 connHandle, prf_acl_state_enum connState);
static int  blt_eslsc_discovery(u16 connHandle);
static int  blt_eslsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);

_attribute_ble_data_retention_ /* retention TODO: */
    blc_esls_client_ctrl_t esls_client_ctrl = {
        .process =
            {
                      .pNext       = NULL,
                      .id          = ESL_ESLS_CLIENT,
                      .usedAclRole = 0,
                      .init        = blt_eslsc_init,
                      .connect     = blt_eslsc_connect,
                      .discov      = blt_eslsc_discovery,
                      .loop        = NULL,
                      .store       = blt_eslsc_nv_store,
                      },
};

_attribute_ble_data_retention_ eslsCallback_t clientCb;

void blc_esl_registerESLSControlClient(const blc_eslsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_UNSPECIF, (blc_prf_proc_t *)&esls_client_ctrl, param);
}

blc_esls_client_t *blt_eslsc_getClientInst(u16 connHandle)
{
    int idx = blc_prf_getAclConnectIndex(connHandle);

    return ((idx >= 0) && ARRAY_SIZE(esls_client_ctrl.pEslsClient)) ? esls_client_ctrl.pEslsClient[idx] : NULL;
}

static void blt_eslsc_sendEvt(u16 connHandle, int evtID, u8 *data, u16 dataLen)
{
    if (clientCb) {
        clientCb(connHandle, evtID, data, dataLen);
    }
}

static int blt_eslsc_init(u8 initType, const void *param)
{
    (void)param;
#if (0)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_esls_client_t)), blc_esls_client_t);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_ESLS_LOG("Client init");

        for (int i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {
            esls_client_ctrl.pEslsClient[i] = blc_eslsc_getClientBuf(i);

            memset(esls_client_ctrl.pEslsClient[i], 0, sizeof(blc_esls_client_t));
            esls_client_ctrl.pEslsClient[i]->eslLEDInformation     = blc_eslsc_getLedInformationBuf(i);
            esls_client_ctrl.pEslsClient[i]->eslSensorInformation  = blc_eslsc_getSensorInformationBuf(i);
            esls_client_ctrl.pEslsClient[i]->eslDisplayInformation = blc_eslsc_getDisplayInformationBuf(i);
        }
    }
    //  else if (initType == AUDIO_PROC_DEINIT) {
    //      BLT_ESLS_LOG("Client deinit");
    //  }
    return 0;
}

static int blt_eslsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_ESLS_LOG("Disconnect: 0x%x", connHandle);

        blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

        memset(client, 0, OFFSETOF(blc_esls_client_t, eslDisplayInformation));
        client->eslDisplayInformation->len = 0;
        client->eslSensorInformation->len  = 0;
        client->eslLEDInformation->len     = 0;
    } else {
        BLT_ESLS_LOG("Connect: 0x%x", connHandle);
    }
    return 0;
}

static int blt_eslsc_discovery(u16 connHandle)
{
    BLC_PRF_SDP_DISCOVERY(connHandle, ESLS, esls, ESL_ESLS_CLIENT);

    return 0;
}

static int blt_eslsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_PRF_NV_STORE(connHandle, ESL_ESLS_CLIENT, esls, eslControlPointCccHdl);

    return 0;
}

static int blt_eslsc_eslProcessControlPointNotificationCback(u8 *writeValue, u16 valueLen, blc_eslss_controlPointResponseHdr_t *rsp, u16 *rspLen)
{
    u8 opcode, len;

    *rspLen = 0;

    // ESLS spec 3.9.1.1 Command opcodes: "The shortest possible TLV is 2 octets in size, and the longest is 17 octets."
    if (valueLen < 2 || valueLen > 17) {
        return ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    STREAM_TO_U8(opcode, writeValue);
    len = (opcode & 0xF0) >> 4;
    if (len + 2 != valueLen) {
        return ATT_ERR_INVALID_PDU;
    }

    if ((opcode & 0x0F) == BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_SENSOR_VALUE_0) {
        // Sensor Value
        blc_eslss_controlPointResponseSensorValue_t *sensorValueRsp = (blc_eslss_controlPointResponseSensorValue_t *)rsp;

        STREAM_TO_U8(sensorValueRsp->sensorId, writeValue);
        memcpy(sensorValueRsp->sensorData, writeValue, len);

        *rspLen = sizeof(*sensorValueRsp) + len;
    } else if ((opcode & 0x0F) == BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_VENDOR_SPECIFIC_RESPONSE_0) {
        // Vendor-specific Response
        blc_eslss_controlPointResponseVendorSpecific_t *vendorSpecificRsp = (blc_eslss_controlPointResponseVendorSpecific_t *)rsp;

        memcpy(vendorSpecificRsp->parameters, writeValue, len + 1);

        *rspLen = sizeof(*vendorSpecificRsp) + len + 1;
    } else {
        switch (opcode) {
        case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_ERROR:
        {
            blc_eslss_controlPointResponseError_t *errorRsp = (blc_eslss_controlPointResponseError_t *)rsp;

            STREAM_TO_U8(errorRsp->error, writeValue);
            *rspLen = sizeof(*errorRsp);

            break;
        }
        case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_LED_STATE:
        {
            blc_eslss_controlPointResponseLedState_t *ledState = (blc_eslss_controlPointResponseLedState_t *)rsp;

            STREAM_TO_U8(ledState->ledId, writeValue);
            *rspLen = sizeof(blc_eslss_controlPointResponseLedState_t);

            break;
        }
        case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_BASIC_STATE:
        {
            blc_eslss_controlPointResponseBasicState_t *basicStateRsp = (blc_eslss_controlPointResponseBasicState_t *)rsp;
            u16                                         basicStateVal;

            STREAM_TO_U16(basicStateVal, writeValue);
            basicStateRsp->serviceNeeded        = basicStateVal & 0x0001;
            basicStateRsp->synchronized         = (basicStateVal & 0x0002) >> 1;
            basicStateRsp->activeLed            = (basicStateVal & 0x0004) >> 2;
            basicStateRsp->pendingLedUpdate     = (basicStateVal & 0x0008) >> 3;
            basicStateRsp->pendingDisplayUpdate = (basicStateVal & 0x0010) >> 4;

            *rspLen = sizeof(*basicStateRsp);

            break;
        }
        case BLC_ESLSS_CONTROL_POINT_RESPONSE_OPCODE_DISPLAY_STATE:
        {
            blc_eslss_controlPointResponseDisplayState_t *displayStateRsp = (blc_eslss_controlPointResponseDisplayState_t *)rsp;

            STREAM_TO_U8(displayStateRsp->displayId, writeValue);
            STREAM_TO_U8(displayStateRsp->imageId, writeValue);
            *rspLen = sizeof(*displayStateRsp);

            break;
        }
        default:
            break;
        }
    }

    if (*rspLen == 0) {
        return ATT_ERR_INVALID_PDU;
    }

    rsp->opcode = opcode;

    return ATT_SUCCESS;
}

u16 blc_eslsc_eslResponsePayloadParse(u8 *val, u16 len, blc_eslss_controlPointResponseHdr_t *rsp, u16 *rspLen)
{
    u16 reqLen = 0;

    if (len < 2) {
        return 0;
    }

    reqLen = ((val[0] & 0xF0) >> 4) + 2;
    if (reqLen > len) {
        return 0;
    }

    blt_eslsc_eslProcessControlPointNotificationCback(val, reqLen, rsp, rspLen);

    return reqLen;
}

static void blt_eslsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    if (attHdl == client->eslControlPointHdl) {
        u8                                      rspBuf[sizeof(bls_eslsc_elsControlPointResponseEvt_t) + BLC_ESLS_CMD_RSP_MAX_LENGTH];
        bls_eslsc_elsControlPointResponseEvt_t *evt = (bls_eslsc_elsControlPointResponseEvt_t *)rspBuf;
        u16                                     rspLen;

        if (blt_eslsc_eslProcessControlPointNotificationCback(val, valLen, &evt->rsp[0], &rspLen) == ATT_SUCCESS) {
            blt_eslsc_sendEvt(connHandle, ESL_EVT_ESLSC_ESL_CONTROL_POINT_RESPONSE, rspBuf, rspLen);
        }
    }
}

static void blt_eslsc_displayInfo(u16 connHandle, blc_esls_client_t *client)
{
    BLT_ESLS_LOG("ESLS sdp over connHandle: 0x%x", connHandle);
    BLT_ESLS_LOG("ESL Address: 0x%04x ESL Current Absolute Time: 0x%04x", client->eslAddressHdl, client->eslCurrentAbsoluteTimeHdl);
    BLT_ESLS_LOG("AP Sync Key Material: 0x%04x ESL Response Key Material: 0x%04x", client->apSyncKetMaterialHdl, client->eslResponseKeyMaterialHdl);
    BLT_ESLS_LOG("ESL Display Information: 0x%04x ESL Image Information: 0x%04x", client->eslDisplayInformationHdl, client->eslImageInformationHdl);
    BLT_ESLS_LOG("ESL Sensor Information: 0x%04x ESL LED Information: 0x%04x", client->eslSensorInformationHdl, client->eslLedInformationHdl);
    BLT_ESLS_LOG("ESL Control Point: 0x%04x CCC: 0x%04x", client->eslControlPointHdl, client->eslControlPointCccHdl);
}

static void blt_eslsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, ESL_ESLS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_ESLS_LOG("ERR:not found ESLS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, ESL_ESLS_CLIENT);
        blt_eslsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_eslsc_dataInput;
    BLT_ESLS_LOG("INFO: ESLS connHandle: 0x%x, startHandle: 0x%x, EndHandle: 0x%x", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, ESL_ESLS_CLIENT, startHandle, endHandle);
}

static const blc_gapc_discService_t eslsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_ELECTRONIC_SHELF_LABEL),
    .sfun = blt_eslsc_foundService,
};

static void blt_eslsc_foundEslAddressChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_WRITE) {
        client->eslAddressHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Address connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundApSyncKeyMaterialChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_WRITE) {
        client->apSyncKetMaterialHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS AP Sync Key Material connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslResponseKeyMaterialChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_WRITE) {
        client->eslResponseKeyMaterialHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Response Key Material connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslCurrentAbsoluteTimeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_WRITE) {
        client->eslCurrentAbsoluteTimeHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Current Absolute Time connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslDisplayInformationChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->eslDisplayInformationHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Display Information connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslImageInformationChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->eslImageInformationHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Image Information connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslSensorInformationChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->eslSensorInformationHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Sensor Information connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslLEDInformationChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if (properties & CHAR_PROP_READ) {
        client->eslLedInformationHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL LED Information connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_eslsc_foundEslCtrlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)serviceCount;

    if ((properties & CHAR_PROP_WRITE) && (properties & CHAR_PROP_WRITE_WITHOUT_RSP) && (properties & CHAR_PROP_NOTIFY)) {
        client->eslControlPointHdl = valueHandle;
    }

    BLT_ESLS_LOG("ESLS ESL Control Point connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}
#if BLC_ESLSC_DISCOVERY_READ_ATTRS
static void blt_eslsc_readCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    u16 handle = pRdCfg->single.handle;

    (void)connHandle;

    if (err) {
        if (err == GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION) {
            BLT_ESLS_LOG("Can't read value due to memory restrictions, handle 0x%04X", handle);
        } else {
            BLT_ESLS_LOG("Read handle: 0x%04x err: 0x%02x", handle, err);
        }
    }
}

static void blt_eslsc_readEslDisplayInformationChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->eslDisplayInformation->val;
    *readLen     = &client->eslDisplayInformation->len;
    *readMaxSize = gAppEslscDisplayInformationMaxSize;
    *rdCbFunc    = blt_eslsc_readCb;
}

static void blt_eslsc_readEslSensorInformationChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->eslSensorInformation->val;
    *readLen     = &client->eslSensorInformation->len;
    *readMaxSize = gAppEslscSensorInformationMaxSize;
    *rdCbFunc    = blt_eslsc_readCb;
}

static void blt_eslsc_readEslLEDInformationChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = client->eslLEDInformation->val;
    *readLen     = &client->eslLEDInformation->len;
    *readMaxSize = gAppEslscLedInformationMaxSize;
    *rdCbFunc    = blt_eslsc_readCb;
}

static void blt_eslsc_readEslImageInformationChar(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)attrHandle;

    *read        = (u8 *)&client->eslImageInformation;
    *readLen     = NULL;
    *readMaxSize = sizeof(client->eslImageInformation);
    *rdCbFunc    = blt_eslsc_readCb;
}
#else
    #define blt_eslsc_readEslDisplayInformationChar NULL
    #define blt_eslsc_readEslSensorInformationChar  NULL
    #define blt_eslsc_readEslImageInformationChar   NULL
    #define blt_eslsc_readEslLEDInformationChar     NULL
#endif

static void blt_eslsc_subscribedEslCtrPointCcc(u16 connHandle, u16 cccHandle, u8 result)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    (void)result;

    client->eslControlPointCccHdl = cccHandle;
}

static const blc_gapc_discChar_t eslsChar[] = {
    {
        .setting = 0,
        .uuid    = UUID16_INIT(CHARACTERISTIC_UUID_ESL_ADDRESS),
        .cfun    = blt_eslsc_foundEslAddressChar,
    },
    {
        .setting = 0,
        .uuid    = UUID16_INIT(CHARACTERISTIC_UUID_AP_SYNC_KEY_MATERIAL),
        .cfun    = blt_eslsc_foundApSyncKeyMaterialChar,
    },
    {
        .setting = 0,
        .uuid    = UUID16_INIT(CHARACTERISTIC_UUID_ESL_RESPONSE_KEY_MATERIAL),
        .cfun    = blt_eslsc_foundEslResponseKeyMaterialChar,
    },
    {
        .setting = 0,
        .uuid    = UUID16_INIT(CHARACTERISTIC_UUID_ESL_CURRENT_ABSOLUTE_TIME),
        .cfun    = blt_eslsc_foundEslCurrentAbsoluteTimeChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_ESL_DISPLAY_INFORMATION),
        .cfun      = blt_eslsc_foundEslDisplayInformationChar,
        .rfun      = blt_eslsc_readEslDisplayInformationChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_ESL_IMAGE_INFORMATION),
        .cfun      = blt_eslsc_foundEslImageInformationChar,
        .rfun      = blt_eslsc_readEslImageInformationChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_ESL_SENSOR_INFORMATION),
        .cfun      = blt_eslsc_foundEslSensorInformationChar,
        .rfun      = blt_eslsc_readEslSensorInformationChar,
    },
    {
        .readValue = true,
        .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_ESL_LED_INFORMATION),
        .cfun      = blt_eslsc_foundEslLEDInformationChar,
        .rfun      = blt_eslsc_readEslLEDInformationChar,
    },
    {
        .subscribeNtf = true,
        .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_ESL_CONTROL_POINT),
        .cfun         = blt_eslsc_foundEslCtrlPointChar,
        .scfun        = blt_eslsc_subscribedEslCtrPointCcc,
    },
};

static const blc_gapc_discList_t discEsls = {
    .maxServiceCount = 1,
    .service         = &eslsService,
    .includeTable =
        {
                       .size = 0,
                       },
    .characteristicTable =
        {
                       .size           = ARRAY_SIZE(eslsChar),
                       .characteristic = eslsChar,
                       },
};

/**********sdp function ending**************/

/**********reconnect function start*********/

static bool blt_eslsc_recService(u16 connHandle, int count)
{
    if (count == 0) {
        blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);
        blt_eslsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, ESL_ESLS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    return count <= 1;
}

static int blt_eslsc_eslDisplayInformationGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->eslDisplayInformationHdl;

    return 1;
}

static int blt_eslsc_eslSensorInformationGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->eslSensorInformationHdl;

    return 1;
}

static int blt_eslsc_eslImageInformationGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->eslImageInformationHdl;

    return 1;
}

static int blt_eslsc_eslLedInformationGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->eslLedInformationHdl;

    return 1;
}

static int blt_eslsc_eslControlPointGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_WRITE | CHAR_PROP_WRITE_WITHOUT_RSP | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->eslControlPointHdl;
#if BLC_ESLSC_WRITE_CCC_ON_RECONNECT
    charInfo->cccHandle = client->eslControlPointCccHdl;
#else
    charInfo->cccHandle = 0;
#endif

    return 1;
}

static const blc_gapc_reconnChar_t reEslsChar[] = {
    {
        .ifun = blt_eslsc_eslDisplayInformationGetInfo,
        .rfun = blt_eslsc_readEslDisplayInformationChar,
    },
    {
        .ifun = blt_eslsc_eslSensorInformationGetInfo,
        .rfun = blt_eslsc_readEslSensorInformationChar,
    },
    {
        .ifun = blt_eslsc_eslImageInformationGetInfo,
        .rfun = blt_eslsc_readEslImageInformationChar,
    },
    {
        .ifun = blt_eslsc_eslLedInformationGetInfo,
        .rfun = blt_eslsc_readEslLEDInformationChar,
    },
    {
        .ifun = blt_eslsc_eslControlPointGetInfo,
    },
};

static const blc_gapc_reconnList_t reconnEsls = {
    .resfun = blt_eslsc_recService,
    .charTb =
        {
                 .size           = ARRAY_SIZE(reEslsChar),
                 .characteristic = reEslsChar,
                 },
    .inclSize = 0,
};

ble_sts_t blc_eslsc_getDisplayInformation(u16 connHandle, u8 *numDisplays, blc_esls_displayData_t *displays)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);
    u8                *val;

    if (!numDisplays || !displays || !client || !client->eslDisplayInformationHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    if (*numDisplays < (client->eslDisplayInformation->len / BLC_ESLSC_DISPLAY_SIZE)) {
        return GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION;
    }

    if (client->eslDisplayInformation->len % BLC_ESLSC_DISPLAY_SIZE) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    val          = client->eslDisplayInformation->val;
    *numDisplays = 0;

    for (u16 i = 0; i < client->eslDisplayInformation->len; i += sizeof(blc_esls_displayData_t)) {
        STREAM_TO_U16(displays[*numDisplays].width, val);
        STREAM_TO_U16(displays[*numDisplays].height, val);
        STREAM_TO_U8(displays[*numDisplays].displayType, val);
        *numDisplays += 1;
    }

    return BLE_SUCCESS;
}

ble_sts_t blc_eslsc_getSensorInformation(u16 connHandle, u16 *numSensors, blc_esls_sensorInformation_t *sensors)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);
    u8                *val;
    u16                numSensorsRcvd = 0;

    if (!numSensors || !sensors || !client || !client->eslSensorInformationHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    val = client->eslSensorInformation->val;

    // First, validate the characteristic value
    for (u16 i = client->eslSensorInformation->len; i > 0; numSensorsRcvd++) {
        u8 consumed = 0;

        STREAM_TO_U8(sensors[numSensorsRcvd].size, val);

        switch (sensors[numSensorsRcvd].size) {
        case BLC_ESLS_SENSOR_INFORMATION_SIZE_0:
            if (i >= BLC_ESLSC_SENSOR_SIZE_0) {
                STREAM_TO_U16(sensors[numSensorsRcvd].sensorType0, val);
                consumed = BLC_ESLSC_SENSOR_SIZE_0;
            }
            break;
        case BLC_ESLS_SENSOR_INFORMATION_SIZE_1:
            if (i >= BLC_ESLSC_SENSOR_SIZE_1) {
                STREAM_TO_U32(sensors[numSensorsRcvd].sensorType1, val);
                consumed = BLC_ESLSC_SENSOR_SIZE_1;
            }
            break;
        default:
            break;
        }

        if (!consumed) {
            BLT_ESLS_LOG("Invalid value of Sensor Information characteristic");
            return GATT_ERR_INVALID_PARAMETER;
        }

        i -= consumed;
    }

    *numSensors = numSensorsRcvd;

    return BLE_SUCCESS;
}

ble_sts_t blc_eslsc_getImageInformation(u16 connHandle, u8 *imageInformation)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    if (!client || !imageInformation || !client->eslImageInformationHdl) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    *imageInformation = client->eslImageInformation;

    return BLE_SUCCESS;
}

ble_sts_t blc_eslsc_getLedInformation(u16 connHandle, u16 *numLeds, blc_esls_ledInformation_t *leds)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);
    u8                *val;

    if (!client || !leds || !numLeds || !client->eslImageInformationHdl || (*numLeds < client->eslLEDInformation->len)) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    val = client->eslLEDInformation->val;

    for (u16 i = 0; i < client->eslLEDInformation->len; i++) {
        u8 ledInfo;

        STREAM_TO_U8(ledInfo, val);

        leds[i].type  = BLC_ESLS_LED_FIELD_GET(BLC_ESLS_LED_TYPE, ledInfo);
        leds[i].blue  = BLC_ESLS_LED_FIELD_GET(BLC_ESLS_LED_BLUE, ledInfo);
        leds[i].green = BLC_ESLS_LED_FIELD_GET(BLC_ESLS_LED_GREEN, ledInfo);
        leds[i].red   = BLC_ESLS_LED_FIELD_GET(BLC_ESLS_LED_RED, ledInfo);
    }

    *numLeds = client->eslLEDInformation->len;

    return BLE_SUCCESS;
}

static void blt_eslsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);

    if (pRdCfg->single.handle == client->eslDisplayInformationHdl) {
        BLT_ESLS_LOG("Display Information read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->eslSensorInformationHdl) {
        BLT_ESLS_LOG("Sensor Information read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->eslImageInformationHdl) {
        BLT_ESLS_LOG("Image Information read callback, status: 0x%02X", err);
    } else if (pRdCfg->single.handle == client->eslLedInformationHdl) {
        BLT_ESLS_LOG("Led Information read callback, status: 0x%02X", err);
    }

    blc_prf_readAttributeValueCallback(connHandle, err);
}

static ble_sts_t blc_eslsc_readAttrVal(u16 connHandle, blt_eslsc_read_t rdType, prf_read_cb_t readCb)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);
    gapc_read_cfg_t    pGapReCfg;

    if (blc_prf_getAclConnectIndex(connHandle) < 0 || !client) {
        BLT_ESLS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= BLT_ESLSC_READ_MAX) {
        BLT_ESLS_LOG("ERR: Invalid read type %d", rdType);
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapReCfg.handle = 0;
    pGapReCfg.func   = blt_eslsc_readAttrValCb;

    switch (rdType) {
    case BLT_ESLSC_READ_DISPLAY_INFORMATION:
    {
        pGapReCfg.handle   = client->eslDisplayInformationHdl;
        pGapReCfg.wBuff    = (u8 *)client->eslDisplayInformation->val;
        pGapReCfg.wBuffLen = &client->eslDisplayInformation->len;
        pGapReCfg.maxLen   = gAppEslscDisplayInformationMaxSize;

        break;
    }
    case BLT_ESLSC_READ_SENSOR_INFORMATION:
    {
        pGapReCfg.handle   = client->eslSensorInformationHdl;
        pGapReCfg.wBuff    = (u8 *)client->eslSensorInformation->val;
        pGapReCfg.wBuffLen = &client->eslSensorInformation->len;
        pGapReCfg.maxLen   = gAppEslscSensorInformationMaxSize;

        break;
    }
    case BLT_ESLSC_READ_IMAGE_INFORMATION:
    {
        pGapReCfg.handle   = client->eslImageInformationHdl;
        pGapReCfg.wBuff    = (u8 *)&client->eslImageInformation;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(client->eslImageInformation);

        break;
    }
    case BLT_ESLSC_READ_LED_INFORMATION:
    {
        pGapReCfg.handle   = client->eslLedInformationHdl;
        pGapReCfg.wBuff    = (u8 *)client->eslLEDInformation->val;
        pGapReCfg.wBuffLen = &client->eslLEDInformation->len;
        pGapReCfg.maxLen   = gAppEslscLedInformationMaxSize;
        break;
    }
    default:
        break;
    }

    if (pGapReCfg.handle == 0) {
        BLT_ESLS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

ble_sts_t blc_eslsc_readDisplayInformation(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_eslsc_readAttrVal(connHandle, BLT_ESLSC_READ_DISPLAY_INFORMATION, readCb);
}

ble_sts_t blc_eslsc_readSensorInformation(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_eslsc_readAttrVal(connHandle, BLT_ESLSC_READ_SENSOR_INFORMATION, readCb);
}

ble_sts_t blc_eslsc_readImageInformation(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_eslsc_readAttrVal(connHandle, BLT_ESLSC_READ_IMAGE_INFORMATION, readCb);
}

ble_sts_t blc_eslsc_readLedInformation(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_eslsc_readAttrVal(connHandle, BLT_ESLSC_READ_LED_INFORMATION, readCb);
}

static void blt_eslsc_writeCb(u16 connHandle, u8 err, void *data)
{
    (void)data;

    blc_prf_writeAttributeValueCallback(connHandle, err);
}

static ble_sts_t blt_eslsc_write(u16 connHandle, blt_eslsc_write_t wrType, u8 *data, u16 len, bool withoutRsp, prf_write_cb_t writeCb)
{
    blc_esls_client_t *client = blt_eslsc_getClientInst(connHandle);
    gapc_write_cfg_t   pGapWrCfg;
    u16                handle;

    if (blc_prf_getAclConnectIndex(connHandle) < 0 || !client) {
        BLT_ESLS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (wrType >= BLT_ESLSC_WRITE_MAX) {
        BLT_ESLS_LOG("ERR: Invalid write type %d", wrType);
        return GATT_ERR_INVALID_PARAMETER;
    }

    switch (wrType) {
    case BLT_ESLSC_WRITE_ESL_ADDRESS:
        handle = client->eslAddressHdl;
        break;
    case BLT_ESLSC_WRITE_ESL_RESPONSE_KEY_MATERIAL:
        memcpy(client->eslResponseKeyMaterialBuf, data, len);
        data   = client->eslResponseKeyMaterialBuf;
        handle = client->eslResponseKeyMaterialHdl;
        break;
    case BLT_ESLSC_WRITE_AP_SYNC_KEY_MATERIAL:
        memcpy(client->apSyncKeyMaterialBuf, data, len);
        data   = client->apSyncKeyMaterialBuf;
        handle = client->apSyncKetMaterialHdl;
        break;
    case BLT_ESLSC_WRITE_CURRENT_ABSOLUTE_TIME:
        handle = client->eslCurrentAbsoluteTimeHdl;
        break;
    case BLT_ESLSC_WRITE_CONTROL_POINT:
        handle = client->eslControlPointHdl;
        break;
    default:
        handle = 0;
    }

    if (!handle) {
        return GATT_ERR_INVALID_PARAMETER;
    }

    pGapWrCfg.func       = blt_eslsc_writeCb;
    pGapWrCfg.handle     = handle;
    pGapWrCfg.data       = data;
    pGapWrCfg.length     = len;
    pGapWrCfg.withoutRsp = withoutRsp;
    pGapWrCfg.cbData     = NULL;

    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
}

ble_sts_t blc_eslsc_writeEslAddress(u16 connHandle, blc_esls_eslAddress_t *eslAddress, prf_write_cb_t cb)
{
    u16 val;

    val = eslAddress->eslId | (eslAddress->groupId) << 8;

    return blt_eslsc_write(connHandle, BLT_ESLSC_WRITE_ESL_ADDRESS, (u8 *)&val, sizeof(val), false, cb);
}

ble_sts_t blc_eslsc_writeApSyncKeyMaterial(u16 connHandle, blc_esls_keyMaterial_t *key, prf_write_cb_t cb)
{
    u8 val[sizeof(blc_esls_keyMaterial_t)];

    memcpy(val, key->sessionKey, sizeof(key->sessionKey));
    memcpy(&val[sizeof(key->sessionKey)], key->IV, sizeof(key->IV));

    return blt_eslsc_write(connHandle, BLT_ESLSC_WRITE_AP_SYNC_KEY_MATERIAL, val, sizeof(val), false, cb);
}

ble_sts_t blc_eslsc_writeEslResponseKeyMaterial(u16 connHandle, blc_esls_keyMaterial_t *key, prf_write_cb_t cb)
{
    u8 val[sizeof(blc_esls_keyMaterial_t)];

    memcpy(val, key->sessionKey, sizeof(key->sessionKey));
    memcpy(&val[sizeof(key->sessionKey)], key->IV, sizeof(key->IV));

    return blt_eslsc_write(connHandle, BLT_ESLSC_WRITE_ESL_RESPONSE_KEY_MATERIAL, val, sizeof(val), false, cb);
}

ble_sts_t blc_eslsc_writeEslCurrentAbsolutTime(u16 connHandle, u32 currentAbsoluteTime, prf_write_cb_t cb)
{
    u8 val[sizeof(currentAbsoluteTime)] = {U32_TO_BYTES(currentAbsoluteTime)};

    return blt_eslsc_write(connHandle, BLT_ESLSC_WRITE_CURRENT_ABSOLUTE_TIME, val, sizeof(val), false, cb);
}

static ble_sts_t blt_esl_fillCommand(blc_eslss_controlPointCommandHdr_t *cmd, u8 *value, u16 *len)
{
    u8 *ptr;

    ptr = value + 2;

    switch (cmd->opcode) {
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_PING:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UNASSOCIATE_FROM_AP:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_SERVICE_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_FACTORY_RESET:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_UPDATE_COMPLETE:
        break;
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_0:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_1:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_2:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_3:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_4:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_5:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_6:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_7:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_8:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_9:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_A:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_B:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_C:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_D:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_E:
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_VENDOR_F:
    {
        blc_eslss_controlPointCommandVendorSpecific_t *vendorCmd = (blc_eslss_controlPointCommandVendorSpecific_t *)cmd;

        u8 payloadLen = (cmd->opcode & 0xF0) >> 4;

        memcpy(ptr, vendorCmd->parameters, payloadLen);
        ptr += payloadLen;

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_READ_SENSOR_DATA:
    {
        blc_eslss_controlPointCommandReadSensorData_t *readSensorCmd = (blc_eslss_controlPointCommandReadSensorData_t *)cmd;

        U8_TO_STREAM(ptr, readSensorCmd->sensorId);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_REFRESH_DISPLAY:
    {
        blc_eslss_controlPointCommandRefreshDisplay_t *refreshDisplayCmd = (blc_eslss_controlPointCommandRefreshDisplay_t *)cmd;

        U8_TO_STREAM(ptr, refreshDisplayCmd->displayId);
        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_IMAGE:
    {
        blc_eslss_controlPointCommandDisplayImage_t *diplayImageCmd = (blc_eslss_controlPointCommandDisplayImage_t *)cmd;

        U8_TO_STREAM(ptr, diplayImageCmd->displayId);
        U8_TO_STREAM(ptr, diplayImageCmd->imageId);

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_DISPLAY_TIMED_IMAGE:
    {
        blc_eslss_controlPointCommandDisplayTimedImage_t *diplayTimedImageCmd = (blc_eslss_controlPointCommandDisplayTimedImage_t *)cmd;

        U8_TO_STREAM(ptr, diplayTimedImageCmd->displayId);
        U8_TO_STREAM(ptr, diplayTimedImageCmd->imageId);
        U32_TO_STREAM(ptr, diplayTimedImageCmd->absoluteTime);

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_CONTROL:
    {
        blc_eslss_controlPointCommandLedControl_t *ledControlCmd = (blc_eslss_controlPointCommandLedControl_t *)cmd;
        u8 color = ledControlCmd->colorRed | (ledControlCmd->colorGreen << 2) | (ledControlCmd->colorBlue << 4) | (ledControlCmd->brightness << 6);

        U8_TO_STREAM(ptr, ledControlCmd->ledId);
        U8_TO_STREAM(ptr, color);
        memcpy(ptr, ledControlCmd->flashingPattern, sizeof(ledControlCmd->flashingPattern));
        ptr += sizeof(ledControlCmd->flashingPattern);
        U16_TO_STREAM(ptr, (ledControlCmd->repeatType | (ledControlCmd->repeatDuration << 1)));

        break;
    }
    case BLC_ESLSS_CONTROL_POINT_COMMAND_OPCODE_LED_TIMED_CONTROL:
    {
        blc_eslss_controlPointCommandLedTimedControl_t *ledTimedControlCmd = (blc_eslss_controlPointCommandLedTimedControl_t *)cmd;
        u8 color = ledTimedControlCmd->colorRed | (ledTimedControlCmd->colorGreen << 2) | (ledTimedControlCmd->colorBlue << 4) | (ledTimedControlCmd->brightness << 6);

        U8_TO_STREAM(ptr, ledTimedControlCmd->ledId);
        U8_TO_STREAM(ptr, color);
        memcpy(ptr, ledTimedControlCmd->flashingPattern, sizeof(ledTimedControlCmd->flashingPattern));
        ptr += sizeof(ledTimedControlCmd->flashingPattern);
        U16_TO_STREAM(ptr, (ledTimedControlCmd->repeatType | (ledTimedControlCmd->repeatDuration << 1)));
        U32_TO_STREAM(ptr, ledTimedControlCmd->absoluteTime);
        break;
    }
    default:
        return GATT_ERR_INVALID_PARAMETER;
    }

    value[0] = cmd->opcode;
    value[1] = cmd->eslId;
    *len     = ptr - value;

    return BLE_SUCCESS;
}

u16 ble_eslsc_eslCommandPayloadWrite(u8 *buffer, u16 length, blc_eslss_controlPointCommandHdr_t *cmd)
{
    return blt_esl_fillCommand(cmd, buffer, &length) == BLE_SUCCESS ? length : 0;
}

static ble_sts_t blt_eslsc_writeControlPoint(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd, bool withoutRsp, prf_write_cb_t cb)
{
    u8        buffer[BLC_ESLS_CMD_RSP_MAX_LENGTH];
    u16       len;
    ble_sts_t status;

    status = blt_esl_fillCommand(cmd, buffer, &len);
    if (status != BLE_SUCCESS) {
        return status;
    }

    return blt_eslsc_write(connHandle, BLT_ESLSC_WRITE_CONTROL_POINT, buffer, len, withoutRsp, cb);
}

ble_sts_t blc_eslsc_writeControlPoint(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd, prf_write_cb_t cb)
{
    return blt_eslsc_writeControlPoint(connHandle, cmd, false, cb);
}

ble_sts_t blc_eslsc_writeControlPointNoRsp(u16 connHandle, blc_eslss_controlPointCommandHdr_t *cmd)
{
    return blt_eslsc_writeControlPoint(connHandle, cmd, true, NULL);
}

void blc_eslsc_setElectronicShelfLabelCback(eslsCallback_t cb)
{
    clientCb = cb;
}
