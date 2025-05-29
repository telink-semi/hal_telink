/********************************************************************************************************
 * @file    pacs_client.c
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


static int  blt_pacsc_disconnect(u16 connHandle);
static void blt_pacsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

static const blc_gapc_discList_t discPacs;
#define BLC_PACS_START_SDP(connHandle) blc_gapc_registerDiscoveryService(connHandle, &discPacs)

static const blc_gapc_reconnList_t reconnPacs;
#define BLC_PACS_START_RECONN(connHandle) blc_gapc_registerReconnectService(connHandle, &reconnPacs)

_attribute_ble_data_retention_
    blc_pacs_client_ctrl_t pacs_client_ctrl = {
        .process = {
                    .pNext       = NULL,
                    .id          = AUDIO_PACS_CLIENT,
                    .usedAclRole = 0,
                    .init        = blt_pacsc_init,
                    .connect     = blt_pacsc_connect,
                    .discov      = blt_pacsc_discovery,
                    .loop        = NULL,
                    .store       = blt_pacs_nv_store,
                    },
};

void blc_audio_registerPACSControlClient(const blc_pacsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t *)&pacs_client_ctrl, param);
}

int blt_pacsc_init(u8 initType, const void *param)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_pacs_client_ctrl_t)), gPacscCtrl);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_pacs_client_t)), blc_pacs_client_t);
#endif
    (void)param;
    if (initType == PRF_PROC_INIT) {
        for (int i = 0; i < gAppAudioAclCentralNum; i++) {
            blc_pacs_client_t *pacsClient   = blt_pacsc_getClientBuf(i);
            pacs_client_ctrl.pPacsClient[i] = pacsClient;
            /* Clear PACS Client parameters  */
            memset(pacsClient, 0, sizeof(blc_pacs_client_t));
            /* Initialize Pointer buffer */
            for (int j = 0; j < gAppPacsCltSinkPacNum; j++) {
                pacsClient->pSinkPacRcd[j] = blt_pacsc_getSinkPacBuf(i, j);
            }
            for (int k = 0; k < gAppPacsCltSrcPacNum; k++) {
                pacsClient->pSrcPacRcd[k] = blt_pacsc_getSrcPacBuf(i, k);
            }
        }
        BLT_PACS_LOG("client init");
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //  }
    return 0;
}

int blt_pacsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if (connState == PRF_ACL_STATE_DISCONN) {
        BLT_PACS_LOG("Disconnect:0x%x", connHandle);
        blt_pacsc_disconnect(connHandle);
    } else {
        BLT_PACS_LOG("Connect:0x%x", connHandle);
    }

    return 0;
}

int blt_pacsc_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, PACS, pacs);
}

int blt_pacs_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param)
{
    BLC_COMMON_NV_STORE(connHandle, PACS, pacs, srcPacHdl[STACK_AUDIO_PACS_SRC_PAC_RECORD_NUM - 1]);

    if (nvState == PRF_NV_STATE_STORE) {
        pNvInfo->sinkPacRcdNum = client->sinkPacRcdNum;
        pNvInfo->srcPacRcdNum  = client->srcPacRcdNum;
    } else if (nvState == PRF_NV_STATE_LOAD) {
        client->sinkPacRcdNum = pNvInfo->sinkPacRcdNum;
        client->srcPacRcdNum  = pNvInfo->srcPacRcdNum;
    }

    return 0;
}

static int blt_pacsc_disconnect(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_pacs_client_t *pPacsClt = blt_pacsc_getClientInst(connHandle);
    //TODO: clear pending variable
    /* Clear PACS Client parameters  */
    memset(pPacsClt, 0, OFFSETOF(blc_pacs_client_t, pSinkPacRcd));

    for (int j = 0; j < gAppPacsCltSinkPacNum; j++) {
        memset(pPacsClt->pSinkPacRcd[j], 0, sizeof(blt_audio_pac_record_param_t));
    }
    for (int k = 0; k < gAppPacsCltSrcPacNum; k++) {
        memset(pPacsClt->pSrcPacRcd[k], 0, sizeof(blt_audio_pac_record_param_t));
    }

    return BLE_SUCCESS;
}

blc_pacs_client_t *blt_pacsc_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    assert(ret == ACL_ROLE_CENTRAL);

    if (ret < 0 || ret == ACL_ROLE_PERIPHERAL) {
        BLT_PACS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if (ret >= 0) {
            /* BAP Unicast Client GAP Central */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_PACS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return pacs_client_ctrl.pPacsClient[idx];
}

/***************************PACS sdp discovery*******************************/
static void blt_pacsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    if (attHdl == client->sinkAudioLcaHdl) {
        if (valLen != sizeof(client->sinkAudioLca)) {
            return;
        }
        STREAM_TO_U32(client->sinkAudioLca, val);
        return;
    } else if (attHdl == client->srcAudioLcaHdl) {
        if (valLen != sizeof(client->srcAudioLca)) {
            return;
        }
        STREAM_TO_U32(client->srcAudioLca, val);
        return;
    } else if (attHdl == client->avaAudioCtxHdl) {
        if (valLen != sizeof(client->avaAudioCtx)) {
            return;
        }
        STREAM_TO_U16(client->avaAudioCtx.avaSinkCtx, val);
        STREAM_TO_U16(client->avaAudioCtx.avaSrcCtx, val);
        return;
    } else if (attHdl == client->supAudioCtxHdl) {
        if (valLen != sizeof(client->supAudioCtx)) {
            return;
        }
        STREAM_TO_U16(client->supAudioCtx.supSinkCtx, val);
        STREAM_TO_U16(client->supAudioCtx.supSrcCtx, val);
        return;
    }
    for (int i = 0; i < client->sinkPacRcdNum; i++) {
        if (attHdl == client->sinkPacHdl[i]) {
            client->pSinkPacRcd[i]->pacLen = min(gAppPacsCltPacMaxSize, valLen);
            memcpy(client->pSinkPacRcd[i]->pac, val, client->pSinkPacRcd[i]->pacLen);
            return;
        }
    }
    for (int i = 0; i < client->srcPacRcdNum; i++) {
        if (attHdl == client->srcPacHdl[i]) {
            client->pSrcPacRcd[i]->pacLen = min(gAppPacsCltPacMaxSize, valLen);
            memcpy(client->pSrcPacRcd[i]->pac, val, client->pSrcPacRcd[i]->pacLen);
            return;
        }
    }
}

static void blt_pacsc_displayInfo(u16 connHandle, blc_pacs_client_t *client)
{
    BLT_PACS_LOG("PACS sdp over connHandle[0x%x]", connHandle);
    BLT_PACS_LOG("audio locations:sink[handle: 0x%x value 0x%x] source[handle: 0x%x value 0x%x]", client->sinkAudioLcaHdl, client->sinkAudioLca, client->srcAudioLcaHdl, client->srcAudioLca);
    BLT_PACS_LOG("audio contexts:ava[handle:0x%x sink:0x%x src:0x%x] supp[handle: 0x%x sink:0x%x src:0x%x]",
                 client->avaAudioCtxHdl,
                 client->avaAudioCtx.avaSinkCtx,
                 client->avaAudioCtx.avaSrcCtx,
                 client->supAudioCtxHdl,
                 client->supAudioCtx.supSinkCtx,
                 client->supAudioCtx.supSrcCtx);

    BLT_PACS_LOG("sink PAC num is %d, source PAC num is %d", client->sinkPacRcdNum, client->srcPacRcdNum);
    for (int i = 0; i < client->sinkPacRcdNum; i++) {
        BLT_PACS_LOG("Sink PAC[0x%x], PAC is %s", client->sinkPacHdl[i], hex_to_str(client->pSinkPacRcd[i]->pac, client->pSinkPacRcd[i]->pacLen));
    }

    for (int i = 0; i < client->srcPacRcdNum; i++) {
        BLT_PACS_LOG("Source PAC[0x%x], PAC is %s", client->srcPacHdl[i], hex_to_str(client->pSrcPacRcd[i]->pac, client->pSrcPacRcd[i]->pacLen));
    }
}

static void blt_pacsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    if (count == 0xFF) {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_PACS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_PACS_LOG("ERR:not found PACS");
        return;
    }

    if (count == 0) {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_PACS_CLIENT);
        blt_pacsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return;
    }

    client->ntfInput.startHdl     = startHandle;
    client->ntfInput.endHdl       = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_pacsc_dataInput;
    BLT_PACS_LOG("  INFO: PACS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_PACS_CLIENT, startHandle, endHandle);
}

static void blt_pacsc_foundSinkPacChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    if (client->sinkPacRcdNum >= gAppPacsCltSinkPacNum) {
        BLT_PACS_LOG("ERR: Sink PAC Char Too Many. connHandle[0x%x], num[%d]", connHandle, client->sinkPacRcdNum);
        return;
    }

    client->sinkPacHdl[client->sinkPacRcdNum] = valueHandle;
    client->sinkPacRcdNum++;

    BLT_PACS_LOG("PACS Sink PAC connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_pacsc_pacReadCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    (void)connHandle;
    (void)pRdCfg;
    if (err == GATT_ERR_DATA_LENGTH_EXCEED_MEM_RESTRICTION) {
        BLT_PACS_LOG("APP_AUDIO_PACS_CLIENT_READ_PAC_MAX_SIZE Insufficient size, Make it bigger");
    }
}

static void blt_pacsc_sinkPacStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    if (client->rdSinkPacRcdIdx >= client->sinkPacRcdNum) {
        BLT_PACS_LOG("ERR: Sink PAC Char Too Many. connHandle[0x%x], num[%d]", connHandle, client->sinkPacRcdNum);
        return;
    }

    blt_audio_pac_record_param_t *pPacRecord = client->pSinkPacRcd[client->rdSinkPacRcdIdx];

    client->rdSinkPacRcdIdx++;

    *read        = pPacRecord->pac;
    *readLen     = &pPacRecord->pacLen;
    *readMaxSize = gAppPacsCltPacMaxSize;
    *rdCbFunc    = blt_pacsc_pacReadCb;
    BLT_PACS_LOG("Sink PAC read info :idx[%d] num[%d]", client->rdSinkPacRcdIdx, client->sinkPacRcdNum);
}

static void blt_pacsc_foundSinkLocationsChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    client->sinkAudioLcaHdl   = valueHandle;
    BLT_PACS_LOG("PACS sink audio locations ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_pacsc_sinkLocationsStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->sinkAudioLca;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static void blt_pacsc_foundSrcPacChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    if (client->srcPacRcdNum >= gAppPacsCltSrcPacNum) {
        BLT_PACS_LOG("ERR: Source PAC Char Too Many. connHandle[0x%x], num[%d]", connHandle, client->srcPacRcdNum);
        return;
    }

    client->srcPacHdl[client->srcPacRcdNum] = valueHandle;
    client->srcPacRcdNum++;

    BLT_PACS_LOG("PACS Source PAC connHandle:0x%x properties:0x%x handle:0x%x ", connHandle, properties, valueHandle);
}

static void blt_pacsc_srcPacStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    if (client->rdSrcPacRcdIdx >= gAppPacsCltSinkPacNum) {
        BLT_PACS_LOG("ERR: Source PAC Char Too Many. connHandle[0x%x], num[%d]", connHandle, client->srcPacRcdNum);
        return;
    }

    blt_audio_pac_record_param_t *pPacRecord = client->pSrcPacRcd[client->rdSrcPacRcdIdx];

    client->rdSrcPacRcdIdx++;

    *read        = pPacRecord->pac;
    *readLen     = &pPacRecord->pacLen;
    *readMaxSize = gAppPacsCltPacMaxSize;
    *rdCbFunc    = blt_pacsc_pacReadCb;
    BLT_PACS_LOG("Source PAC read info:idx[%d] num[%d]", client->rdSrcPacRcdIdx, client->srcPacRcdNum);
}

static void blt_pacsc_foundSrcLocationsChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    client->srcAudioLcaHdl    = valueHandle;
    BLT_PACS_LOG("PACS source audio locations ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_pacsc_srcLocationsStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->srcAudioLca;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static void blt_pacsc_foundAvaAudioContextsChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    client->avaAudioCtxHdl    = valueHandle;
    BLT_PACS_LOG("PACS Available Audio Contexts ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_pacsc_avaAudioContextsStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->avaAudioCtx;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static void blt_pacsc_foundSuppAudioContextsChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    client->supAudioCtxHdl    = valueHandle;
    BLT_PACS_LOG("PACS Supported Audio Contexts ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_pacsc_suppAudioContextsStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    *read                     = (u8 *)&client->supAudioCtx;
    *readLen                  = NULL;
    *readMaxSize              = 4;
    *rdCbFunc                 = NULL;
}

static const blc_gapc_discService_t pacsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_PUBLISHED_AUDIO_CAPABILITIES),
    .sfun = blt_pacsc_foundService,
};

static const blc_gapc_discChar_t pacsChar[] = {
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SINK_PAC),
     .cfun         = blt_pacsc_foundSinkPacChar,
     .rfun         = blt_pacsc_sinkPacStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SINK_AUDIO_LOCATIONS),
     .cfun         = blt_pacsc_foundSinkLocationsChar,
     .rfun         = blt_pacsc_sinkLocationsStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SOURCE_PAC),
     .cfun         = blt_pacsc_foundSrcPacChar,
     .rfun         = blt_pacsc_srcPacStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SOURCE_AUDIO_LOCATIONS),
     .cfun         = blt_pacsc_foundSrcLocationsChar,
     .rfun         = blt_pacsc_srcLocationsStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_AVAILABLE_AUDIO_CONTEXTS),
     .cfun         = blt_pacsc_foundAvaAudioContextsChar,
     .rfun         = blt_pacsc_avaAudioContextsStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_SUPPORTED_AUDIO_CONTEXTS),
     .cfun         = blt_pacsc_foundSuppAudioContextsChar,
     .rfun         = blt_pacsc_suppAudioContextsStartRead,
     },
};

static const blc_gapc_discList_t discPacs = {
    .maxServiceCount = 1,
    .service         = &pacsService,
    .includeTable    = {
                        .size = 0,
                        },
    .characteristicTable = {
                        .size           = ARRAY_SIZE(pacsChar),
                        .characteristic = pacsChar,
                        },
};

/***************************PACS sdp discovery end*******************************/

/**********reconnect function start*********/
static bool blt_pacsc_reconnService(u16 connHandle, int count)
{
    if (count == 0) {
        blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
        blt_pacsc_displayInfo(connHandle, client);
        BLT_PACS_LOG("  INFO: PACS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_PACS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if (count > 1) {
        return false;
    }
    return true;
}

static int blt_pacsc_sinkPacGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    for (int i = 0; i < client->sinkPacRcdNum; i++) {
        charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
        charInfo->valueHandle = client->sinkPacHdl[i];
        charInfo->cccHandle   = 0;
        charInfo++;
    }

    return client->sinkPacRcdNum;
}

static int blt_pacsc_sinkLocationsGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->sinkAudioLcaHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_pacsc_srcPacGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    for (int i = 0; i < client->srcPacRcdNum; i++) {
        charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
        charInfo->valueHandle = client->srcPacHdl[i];
        charInfo->cccHandle   = 0;
        charInfo++;
    }

    return client->srcPacRcdNum;
}

static int blt_pacsc_srcLocationsGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->srcAudioLcaHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_pacsc_avaAudioContextsGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->avaAudioCtxHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_pacsc_suppAudioContextsGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->supAudioCtxHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static const blc_gapc_reconnChar_t rePacsChar[] = {

    {
     .ifun = blt_pacsc_sinkPacGetInfo,
     .rfun = blt_pacsc_sinkPacStartRead,
     },

    {
     .ifun = blt_pacsc_sinkLocationsGetInfo,
     .rfun = blt_pacsc_sinkLocationsStartRead,
     },

    {
     .ifun = blt_pacsc_srcPacGetInfo,
     .rfun = blt_pacsc_srcPacStartRead,
     },

    {
     .ifun = blt_pacsc_srcLocationsGetInfo,
     .rfun = blt_pacsc_srcLocationsStartRead,
     },

    {
     .ifun = blt_pacsc_avaAudioContextsGetInfo,
     .rfun = blt_pacsc_avaAudioContextsStartRead,
     },

    {
     .ifun = blt_pacsc_suppAudioContextsGetInfo,
     .rfun = blt_pacsc_suppAudioContextsStartRead,
     },
};

static const blc_gapc_reconnList_t reconnPacs = {
    .resfun = blt_pacsc_reconnService,
    .charTb = {
               .size           = ARRAY_SIZE(rePacsChar),
               .characteristic = rePacsChar,
               },
    .inclSize = 0,
};

/**********reconnect function ending********/

/*************************************************************************
 *  GATTC Read Characteristics
 *  CHARACTERISTIC_UUID_SINK_PAC,  (*)
 *  CHARACTERISTIC_UUID_SINK_AUDIO_LOCATIONS
 *  CHARACTERISTIC_UUID_SOURCE_PAC (*)
 *  CHARACTERISTIC_UUID_SOURCE_AUDIO_LOCATIONS
 *  CHARACTERISTIC_UUID_AVAILABLE_AUDIO_CONTEXTS
 *  CHARACTERISTIC_UUID_SUPPORTED_AUDIO_CONTEXTS
 *  (*): multiple
 *************************************************************************/
static void blt_pacsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    if (pRdCfg->single.handle == client->sinkPacHdl[0]) {
        BLT_PACS_LOG("RD_CB INFO: Sink PAC[0x%04x], PAC is %s", client->sinkPacHdl[0], hex_to_str(client->pSinkPacRcd[0]->pac, client->pSinkPacRcd[0]->pacLen));
    } else if (pRdCfg->single.handle == client->sinkAudioLcaHdl) {
        BLT_PACS_LOG("RD_CB INFO: sink audio locations[handle: 0x%04x value 0x%08x]", client->sinkAudioLcaHdl, client->sinkAudioLca);
    } else if (pRdCfg->single.handle == client->srcPacHdl[0]) {
        BLT_PACS_LOG("RD_CB INFO: source PAC[0x%04x], PAC is %s", client->srcPacHdl[0], hex_to_str(client->pSrcPacRcd[0]->pac, client->pSrcPacRcd[0]->pacLen));
    } else if (pRdCfg->single.handle == client->srcAudioLcaHdl) {
        BLT_PACS_LOG("RD_CB INFO: source audio locations[handle: 0x%04x value 0x%08x]", client->srcAudioLcaHdl, client->srcAudioLca);
    } else if (pRdCfg->single.handle == client->avaAudioCtxHdl) {
        BLT_PACS_LOG("RD_CB INFO:Available audio contexts[handle:0x%04x sink:0x%04x src:0x%04x]",
                     client->avaAudioCtxHdl,
                     client->avaAudioCtx.avaSinkCtx,
                     client->avaAudioCtx.avaSrcCtx);
    } else if (pRdCfg->single.handle == client->supAudioCtxHdl) {
        BLT_PACS_LOG("RD_CB INFO:Supported audio contexts[handle:0x%04x sink:0x%04x src:0x%04x]",
                     client->supAudioCtxHdl,
                     client->supAudioCtx.supSinkCtx,
                     client->supAudioCtx.supSrcCtx);
    }
    blc_prf_readAttributeValueCallback(connHandle, err);
}

static int blc_pacsc_readAttrVal(u16 connHandle, blt_pacs_read_enum rdType, prf_read_cb_t readCb)
{
    BLT_PACS_LOG("blc_pacsc_readAttrVal:%d", rdType);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_PACS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= PACS_READ_MAX) {
        BLT_PACS_LOG("ERR: Invalid read type %d", rdType);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);

    gapc_read_cfg_t pGapReCfg;
    pGapReCfg.handle = 0;
    pGapReCfg.func   = blt_pacsc_readAttrValCb;

    switch (rdType) {
    case PACS_READ_SINK_PAC:
    {
        pGapReCfg.handle   = client->sinkPacHdl[0];
        pGapReCfg.wBuff    = (u8 *)&client->pSinkPacRcd[0]->pac;
        pGapReCfg.wBuffLen = &client->pSinkPacRcd[0]->pacLen;
        pGapReCfg.maxLen   = gAppPacsCltPacMaxSize;
    } break;
    case PACS_READ_SINK_AUDIO_LOC:
    {
        pGapReCfg.handle   = client->sinkAudioLcaHdl;
        pGapReCfg.wBuff    = (u8 *)&client->sinkAudioLca;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(client->sinkAudioLca);
    } break;
    case PACS_READ_SRC_PAC:
    {
        pGapReCfg.handle   = client->srcPacHdl[0];
        pGapReCfg.wBuff    = (u8 *)&client->pSrcPacRcd[0]->pac;
        pGapReCfg.wBuffLen = &client->pSrcPacRcd[0]->pacLen;
        pGapReCfg.maxLen   = gAppPacsCltPacMaxSize;
    } break;
    case PACS_READ_SRC_AUDIO_LOC:
    {
        pGapReCfg.handle   = client->srcAudioLcaHdl;
        pGapReCfg.wBuff    = (u8 *)&client->srcAudioLca;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(client->srcAudioLca);
    } break;
    case PACS_READ_AVA_AUDIO_CTX:
    {
        pGapReCfg.handle   = client->avaAudioCtxHdl;
        pGapReCfg.wBuff    = (u8 *)&client->avaAudioCtx;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(client->avaAudioCtx);
    } break;
    case PACS_READ_SUP_AUDIO_CTX:
    {
        pGapReCfg.handle   = client->supAudioCtxHdl;
        pGapReCfg.wBuff    = (u8 *)&client->supAudioCtx;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(client->supAudioCtx);
    } break;
    default:
        break;
    }

    if (pGapReCfg.handle == 0) {
        BLT_PACS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

int blc_pacsc_readSinkPac(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_pacsc_readAttrVal(connHandle, PACS_READ_SINK_PAC, readCb);
}

int blc_pacsc_readSinkAudioLoc(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_pacsc_readAttrVal(connHandle, PACS_READ_SINK_AUDIO_LOC, readCb);
}

int blc_pacsc_readSourcePac(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_pacsc_readAttrVal(connHandle, PACS_READ_SRC_PAC, readCb);
}

int blc_pacsc_readSourceAudioLoc(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_pacsc_readAttrVal(connHandle, PACS_READ_SRC_AUDIO_LOC, readCb);
}

int blc_pacsc_readAvaAudioCtx(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_pacsc_readAttrVal(connHandle, PACS_READ_AVA_AUDIO_CTX, readCb);
}

int blc_pacsc_readSupAudioCtx(u16 connHandle, prf_read_cb_t readCb)
{
    return blc_pacsc_readAttrVal(connHandle, PACS_READ_SUP_AUDIO_CTX, readCb);
}

#define BLT_PACSC_CHECK_PARAM(connHandle, attrHandle)                \
    if (blt_ll_isAclHandleOutOfRange(connHandle))                    \
        return AUDIO_ERR_CONNHANDLE_INVALID;                         \
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle); \
    if (client == NULL)                                              \
        return AUDIO_ERR_CONNHANDLE_INVALID;                         \
    if (client->attrHandle == 0)                                     \
    return AUDIO_ERR_GET_ATTR_HANDLE_NOT_FOUND

int blc_pacsc_getSinkPac(u16 connHandle, u8 *sinkPac, u16 *sinkPacLen)
{
    BLT_AUD_CHECK_NULL_PTR(sinkPac, sinkPacLen);
    BLT_PACSC_CHECK_PARAM(connHandle, sinkPacHdl[0]);

    memcpy(sinkPac, client->pSinkPacRcd[0]->pac, client->pSinkPacRcd[0]->pacLen);
    *sinkPacLen = client->pSinkPacRcd[0]->pacLen;

    return AUDIO_ESUCC;
}

int blc_pacsc_getSinkAudioLoc(u16 connHandle, u32 *sinkLoc)
{
    BLT_AUD_CHECK_NULL_PTR(sinkLoc);
    BLT_PACSC_CHECK_PARAM(connHandle, sinkAudioLcaHdl);

    *sinkLoc = client->sinkAudioLca;

    return AUDIO_ESUCC;
}

int blc_pacsc_getSourcePac(u16 connHandle, u8 *srcPac, u16 *srcPacLen)
{
    BLT_AUD_CHECK_NULL_PTR(srcPac, srcPacLen);
    BLT_PACSC_CHECK_PARAM(connHandle, srcPacHdl[0]);

    memcpy(srcPac, client->pSrcPacRcd[0]->pac, client->pSrcPacRcd[0]->pacLen);
    *srcPacLen = client->pSrcPacRcd[0]->pacLen;

    return AUDIO_ESUCC;
}

int blc_pacsc_getSourceAudioLoc(u16 connHandle, u32 *srcLoc)
{
    BLT_AUD_CHECK_NULL_PTR(srcLoc);
    BLT_PACSC_CHECK_PARAM(connHandle, srcAudioLcaHdl);

    *srcLoc = client->srcAudioLca;

    return AUDIO_ESUCC;
}

int blc_pacsc_getAvaAudioCtx(u16 connHandle, u16 *avaSinkCtx, u16 *avaSrcCtx)
{
    BLT_AUD_CHECK_NULL_PTR(avaSinkCtx, avaSrcCtx);
    BLT_PACSC_CHECK_PARAM(connHandle, avaAudioCtxHdl);

    *avaSinkCtx = client->avaAudioCtx.avaSinkCtx;
    *avaSrcCtx  = client->avaAudioCtx.avaSrcCtx;

    return AUDIO_ESUCC;
}

int blc_pacsc_getSupAudioCtx(u16 connHandle, u16 *supSinkCtx, u16 *supSrcCtx)
{
    BLT_AUD_CHECK_NULL_PTR(supSinkCtx, supSrcCtx);
    BLT_PACSC_CHECK_PARAM(connHandle, supAudioCtxHdl);

    *supSinkCtx = client->supAudioCtx.supSinkCtx;
    *supSrcCtx  = client->supAudioCtx.supSrcCtx;

    return AUDIO_ESUCC;
}

/*************************************************************************
 *  GATTC Write Characteristics
 *  - CHARACTERISTIC_UUID_SINK_AUDIO_LOCATIONS  (Optional)
 *  - CHARACTERISTIC_UUID_SOURCE_AUDIO_LOCATIONS    (Optional)
 *************************************************************************/
static void blt_pacsc_writeCb(u16 connHandle, u8 err, void *data)
{
    (void)data;
    blc_prf_writeAttributeValueCallback(connHandle, err);
}

static int blt_pacsc_write(u16 connHandle, blt_pacs_write_enum wrType, u32 audioLoc, prf_write_cb_t writeCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_PACS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (wrType >= PACS_WRITE_MAX) {
        BLT_PACS_LOG("ERR: Invalid write type %d", wrType);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_pacs_client_t *client    = blt_pacsc_getClientInst(connHandle);
    u16                audLcaHdl = (wrType == PACS_WRITE_SINK_AUDIO_LOC) ? client->sinkAudioLcaHdl : client->srcAudioLcaHdl;

    if (!audLcaHdl) {
        BLT_PACS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;
    pGapWrCfg.func       = blt_pacsc_writeCb;
    pGapWrCfg.handle     = audLcaHdl;
    pGapWrCfg.data       = &audioLoc;
    pGapWrCfg.length     = sizeof(audioLoc);
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData     = NULL;
    if (!pGapWrCfg.handle) {
        return AUDIO_ERR_INVALID_PARAMETER;
    }
    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
}

int blc_pacsc_writeSinkAudioLoc(u16 connHandle, u32 audioLoc, prf_write_cb_t writeCb)
{
    BLT_PACS_LOG("blc_pacsc_writeSinkAudioLoc");
    return blt_pacsc_write(connHandle, PACS_WRITE_SINK_AUDIO_LOC, audioLoc, writeCb);
}

int blc_pacsc_writeSrcAudioLoc(u16 connHandle, u32 audioLoc, prf_write_cb_t writeCb)
{
    BLT_PACS_LOG("blc_pacsc_writeSrcAudioLoc");
    return blt_pacsc_write(connHandle, PACS_WRITE_SRC_AUDIO_LOC, audioLoc, writeCb);
}

/******************************************************************************/
int blc_pacsc_checkPACLegal(blc_audio_codec_id_t *codec, blc_audio_codecSpecCfgParsed_t *codecCfg, blc_audio_metadata_parsed_t *metadata, u8 *pac, u32 locations)
{
    (void)locations;
    (void)metadata;
    u8 numPacRecords = pac[0];
    pac++;
    for (int i = 0; i < numPacRecords; i++) {
        if (memcmp(codec, pac, 5)) {
            pac += 5 + 1 + pac[5] + 1 + pac[5 + 1 + pac[5]];
            continue;
        }
        pac += 5;
        blt_audio_codecSpecCapParam_t codecCap;
        memset(&codecCap, 0, sizeof(blt_audio_codecSpecCapParam_t));
        blt_audio_getCodecSpecCapParam(pac, &codecCap);
        if (blt_audio_checkCodecParamValid(&codecCap, codecCfg)) {
            pac += 1 + pac[0] + 1 + pac[1 + pac[0]];

            continue;
        }
        //TODO: check metadata
        pac += 1 + pac[0] + 1 + pac[1 + pac[0]];
        return 0;
    }
    return 1;
}

int blc_pacsc_checkSinkPAC(u16 connHandle, blc_audio_codec_id_t *codec, blc_audio_codecSpecCfgParsed_t *codecCfg, blc_audio_metadata_parsed_t *metadata)
{
    blc_pacs_client_t *client = blt_pacsc_getClientInst(connHandle);
    if (!client || client->sinkAudioLcaHdl == 0 || client->sinkPacRcdNum == 0) {
        return -1;
    }
    for (int i = 0; i < client->sinkPacRcdNum; i++) {
        if (blc_pacsc_checkPACLegal(codec, codecCfg, metadata, client->pSinkPacRcd[i]->pac, client->sinkAudioLca) == 0) {
            return 0;
        }
    }

    return -1;
}
