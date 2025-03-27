/********************************************************************************************************
 * @file    bass_client.c
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
#include "../bap_internal.h"

static const blc_gapc_discList_t discBass;
#define BLC_BASS_START_SDP(connHandle)      blc_gapc_registerDiscoveryService(connHandle, &discBass)

static const blc_gapc_reconnList_t reconnBass;
#define BLC_BASS_START_RECONN(connHandle)   blc_gapc_registerReconnectService(connHandle, &reconnBass)

static void blt_bassc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
int blt_bassc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);

_attribute_ble_data_retention_
blc_bass_client_ctrl_t bass_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_BASS_CLIENT,
        .usedAclRole = 0,
        .init = blt_bassc_init,
        .connect = blt_bassc_connect,
        .discov = blt_bassc_discovery,
        .loop = NULL,
        .store = blt_bassc_nv_store,
    },
};


void blc_audio_registerBASSControlClient(const blc_bassc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&bass_client_ctrl, param);
}

blc_bass_client_t* blt_bassc_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if (ret < 0 || ret == ACL_ROLE_PERIPHERAL) {
        BLT_BASS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* BAP Broadcast Sink GAP Peripheral GAP Observer */
            /* BAP Scan Delegator GAP Peripheral */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_BASS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return bass_client_ctrl.pBassClient[idx];
}

int blt_bassc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_bass_client_t)), blc_bass_client_t);
#endif
    (void)param;

    if(initType == PRF_PROC_INIT) {
        BLT_BASS_LOG("Client init");

        for (int i = 0; i < gAppAudioAclCentralNum; i++) {
            bass_client_ctrl.pBassClient[i] = blc_bassc_getClientBuf(i);
            /* Clear ASCS Client parameters  */
            memset(bass_client_ctrl.pBassClient[i], 0, sizeof(blc_bass_client_t));
            /* Initialize Pointer buffer */
            for (int j = 0; j < gAppBasscRecvStateNum; j++) {
                bass_client_ctrl.pBassClient[i]->pRecvState[j] = blc_bassc_getRecvStateBuf(i*gAppBasscRecvStateNum + j);
            }
        }
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      BLT_BASS_LOG("Client deinit");
//  }
    return 0;
}
int blt_bassc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_BASS_LOG("Disconnect: 0x%x", connHandle);

        blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

        memset(client, 0, OFFSETOF(blc_bass_client_t, pRecvState[0]));
        for (int j = 0; j < gAppBasscRecvStateNum; j++)
        {
            client->pRecvState[j]->len = 0;
        }
    } else {
        BLT_BASS_LOG("Connect: 0x%x", connHandle);
    }
    return 0;
}

int blt_bassc_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, BASS, bass);
}


int blt_bassc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    BLC_COMMON_NV_STORE(connHandle, BASS, bass, recvStateHandle[STACK_AUDIO_BASS_RECV_STATE_NUM-1]);

    if(nvState == PRF_NV_STATE_STORE)
    {
        pNvInfo->bcstRcvStateCnt = client->bcstRcvStateCnt;
    }
    else if(nvState == PRF_NV_STATE_LOAD)
    {
        client->bcstRcvStateCnt = pNvInfo->bcstRcvStateCnt;
    }

    return 0;
}

static int blt_bassc_analysisRecvState(u16 connHandle, blc_bassc_recv_state_param_t *pRecvState)
{
    if(pRecvState->len < OFFSETOF(blt_bass_recvState_t, numSubGrps)) {
        return blt_prf_sendEvent(connHandle, AUDIO_EVT_BASSC_RECV_SINK_STATE, NULL, 0);
    }

    blt_bass_recvState_t* pState = (blt_bass_recvState_t*)pRecvState->recvState;

    if(pState->paSyncState == BASS_PA_STATE_SYNCINFO_REQUEST)
    {
        return blt_audio_bcstAssistantRecvEvt(connHandle, AUDIO_EVT_BASSC_RECV_SYNCINFO_REQ, NULL, 0);;
    }

    if(pState->bigEncryption == BASS_BIG_BAD_CODE)
    {
        blc_bassc_badBroadcastCodeEvt_t pEvt;
        pEvt.sourceID = pState->sourceID;
        pEvt.paState = pState->paSyncState == BASS_PA_STATE_SYNC_TO_PA? true: false;
        memcpy(pEvt.broadcastCode, pState->badCode, 16);
        return blt_prf_sendEvent(connHandle, AUDIO_EVT_BASSC_BAD_BROADCAST_CODE, (u8*)&pEvt, sizeof(blc_bassc_badBroadcastCodeEvt_t));
    }
    else if(pState->bigEncryption == BASS_BIG_BCSTCODE_REQUIRED)
    {
        blc_bassc_bcstCodeReq_t pEvt;
        pEvt.sourceID = pState->sourceID;
        return blt_prf_sendEvent(connHandle, AUDIO_EVT_BASSC_BROADCAST_CODE_REQ, (u8*)&pEvt, sizeof(blc_bassc_bcstCodeReq_t));
    }

    u8 evtBuf[50];
    blc_bassc_recvSinkStateEvt_t *pEvt = (blc_bassc_recvSinkStateEvt_t*)evtBuf;

    pEvt->sourceID = pState->sourceID;
    pEvt->paState = pState->paSyncState == BASS_PA_STATE_SYNC_TO_PA? true: false;

    pEvt->numSubgroups = pState->numSubGrps;
    bass_subGrp_t *subgroup = pState->subGrps;
    u8* ptr = (u8*)&pEvt->bisSyncState;
    for(int i=0; i<pState->numSubGrps; i++)
    {
        U32_TO_STREAM(ptr, subgroup->bisSync);
        U8_TO_STREAM(ptr, subgroup->metadataLen);
        U32_TO_STREAM(ptr, (u32)subgroup->metadata);
        subgroup = (bass_subGrp_t*)((u8*)subgroup + 4 + 1 + subgroup->metadataLen);
    }

    return blt_prf_sendEvent(connHandle, AUDIO_EVT_BASSC_RECV_SINK_STATE, (u8*)pEvt, 3+9*pEvt->numSubgroups);
}

static void blt_bassc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    for(int i=0; i<client->bcstRcvStateCnt; i++) {
        blc_bassc_recv_state_param_t *pRecvState = client->pRecvState[i];
        if(client->recvStateHandle[i] == attHdl){
            pRecvState->len = min(valLen, gAppBasscRecvStateMaxSize);
            memcpy(pRecvState->recvState, val, valLen);
            blt_bassc_analysisRecvState(connHandle, pRecvState);
        }
    }
}

/**********sdp function start**************/

static void blt_bassc_displayInfo(u16 connHandle, blc_bass_client_t* client)
{
    BLT_BASS_LOG("BASS sdp over connHandle: 0x%x", connHandle);
    for(int i=0; i<client->bcstRcvStateCnt; i++) {
        blc_bassc_recv_state_param_t *pRecvState = client->pRecvState[i];
        BLT_BASS_LOG("Broadcast Receive State index: %d, handle:0x%x", i, client->recvStateHandle[i]);
        BLT_BASS_LOG("State: 0x%s", hex_to_str(pRecvState->recvState, pRecvState->len));
        blt_bassc_analysisRecvState(connHandle, pRecvState);
    }
    BLT_BASS_LOG("bass control point handle is 0x%x", client->bassCtrlHandle);
}

static void blt_bassc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_BASS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_BASS_LOG("ERR:not found BASS");
        return ;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_BASS_CLIENT);
        blt_bassc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }

    client->ntfInput.startHdl = startHandle;
    client->ntfInput.endHdl = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_bassc_dataInput;
    client->bcstRcvStateCnt = 0;
    BLT_BASS_LOG( " INFO: BASS connHandle: 0x%x, startHandle: 0x%x, EndHandle: 0x%x", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_BASS_CLIENT, startHandle, endHandle);
}

static void blt_bassc_foundCtrlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);
    if((properties & CHAR_PROP_WRITE_WITHOUT_RSP) && (properties & CHAR_PROP_WRITE))
        client->bassCtrlHandle = valueHandle;

    BLT_BASS_LOG( "BASS ctrl point connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_bassc_foundRecvStateChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    if(client->bcstRcvStateCnt >= gAppBasscRecvStateNum)
    {
        BLT_BASS_LOG("ERR: Recv State Char Too Many: 0x%x", connHandle);
        return ;
    }

    client->recvStateHandle[client->bcstRcvStateCnt] = valueHandle;
    client->bcstRcvStateCnt++;

    BLT_BASS_LOG( "BASS Receive State connHandle: 0x%x, properties: 0x%x, value: 0x%x", connHandle, properties, valueHandle);
}

static void blt_bassc_recvStateStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    if(client->bcstRcvStateIdx >= client->bcstRcvStateCnt)
    {
        BLT_BASS_LOG("ERR: Receive State Char Too Many: 0x%x", connHandle);
        return ;
    }
    blc_bassc_recv_state_param_t *pRecvState = client->pRecvState[client->bcstRcvStateIdx];
    BLT_BASS_LOG("start Broadcast Receive state read, connHandle is 0x%x, client is 0x%p", connHandle, client);
    client->bcstRcvStateIdx++;

    *read = pRecvState->recvState;
    *readLen = &pRecvState->len;
    *readMaxSize = gAppBasscRecvStateMaxSize;
    *rdCbFunc = NULL;
    BLT_BASS_LOG("blt_bassc_recvStateStartRead: 0x%x", connHandle);
}

static const blc_gapc_discService_t bassService = {
    .uuid = UUID16_INIT(SERVICE_UUID_BROADCAST_AUDIO_SCAN),
    .sfun = blt_bassc_foundService,
};

static const blc_gapc_discChar_t bassChar[] = {
    {
        .setting = 0,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_BAS_CONTROL_POINT),
        .cfun = blt_bassc_foundCtrlPointChar,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_BROADCAST_RECEIVE_STATE),
        .cfun = blt_bassc_foundRecvStateChar,
        .rfun = blt_bassc_recvStateStartRead,
    }
};

static const blc_gapc_discList_t discBass = {
    .maxServiceCount = 1,
    .service = &bassService,
    .includeTable = {
        .size = 0,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(bassChar),
        .characteristic = bassChar,
    },
};

/**********sdp function ending**************/

/**********reconnect function start*********/

static bool blt_bassc_recService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_bass_client_t *client = blt_bassc_getClientInst(connHandle);
        blt_bassc_displayInfo(connHandle, client);
        BLT_BASS_LOG("  INFO: BASS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_BASS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}

static int blt_bassc_recvStateGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    for(int i=0; i<client->bcstRcvStateCnt; i++)
    {
        charInfo->properties = CHAR_PROP_READ;
        charInfo->valueHandle = client->recvStateHandle[i];
        charInfo++;
    }

    return client->bcstRcvStateCnt;
}

static const blc_gapc_reconnChar_t reBassChar[] = {
    {
        .ifun = blt_bassc_recvStateGetInfo,
        .rfun = blt_bassc_recvStateStartRead,
    },
};

static const blc_gapc_reconnList_t reconnBass = {
    .resfun = blt_bassc_recService,
    .charTb = {
        .size = ARRAY_SIZE(reBassChar),
        .characteristic = reBassChar,
    },
    .inclSize = 0,
};

/**********reconnect function ending********/

/**********Read Characteristic Attribute Value*********/
int blc_bassc_readBcstRecvState(u16 connHandle, int index, prf_read_cb_t readCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_BASS_LOG("  ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    if(client->bcstRcvStateCnt <= index)
    {
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_read_cfg_t pGapReCfg;
    pGapReCfg.func = blc_prf_readAttributeValueDefaultCallback;
    pGapReCfg.handle = client->recvStateHandle[index];
    pGapReCfg.wBuff = (u8*)&client->pRecvState[index]->recvState;
    pGapReCfg.wBuffLen = &client->pRecvState[index]->len;
    pGapReCfg.maxLen = gAppBasscRecvStateMaxSize;

    if (pGapReCfg.handle == 0) {
        BLT_PACS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

int blc_bassc_getBcstRecvState(u16 connHandle, int index, u8* state, u16* len)
{
    BLT_AUD_CHECK_NULL_PTR(state, len);

    if(blt_ll_isAclHandleOutOfRange(connHandle))
        return AUDIO_ERR_CONNHANDLE_INVALID;

    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);

    if(client == NULL)
        return AUDIO_ERR_CONNHANDLE_INVALID;

    if(client->bcstRcvStateCnt <= index || client->recvStateHandle[index] == 0)
        return AUDIO_ERR_GET_ATTR_HANDLE_NOT_FOUND;

    memcpy(state, (u8*)&client->pRecvState[index]->recvState, client->pRecvState[index]->len);
    *len = client->pRecvState[index]->len;

    return AUDIO_ESUCC;
}

int blc_bassc_writeBcstScanCtrlPointWithoutRsp(u16 connHandle, u8* cmd, u16 cmdLen)
{
    BLT_BASS_LOG("blc_bassc_writeBcstScanCtrlPointWithoutRsp");
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_BASS_LOG("  ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }
    if(cmd == NULL || cmdLen == 0)
    {
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_bass_client_t* client = blt_bassc_getClientInst(connHandle);
    gattc_write_cfg_t wrCfg = {
        .func = NULL,
        .handle = client->bassCtrlHandle,
        .offset = 0,
        .data = cmd,
        .length = cmdLen,
        .withoutRsp = true,
    };

    if (!client->bassCtrlHandle) {
        BLT_BASS_LOG("  ERR: ATT write handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    return blc_gattc_writeAttributeValue(connHandle, &wrCfg);
}

static int blt_bassc_writeCtrlPoint(u16 connHandle, blt_bass_opcode_enum opcode, u8* param, u16 paramLen)
{
    u8 writeBuff[255+20+1] = {opcode};

    memcpy(writeBuff+1, param, paramLen);

    return blc_bassc_writeBcstScanCtrlPointWithoutRsp(connHandle, writeBuff, paramLen + 1);
}

int blc_bassc_writeRemoteScanStopped(u16 connHandle)
{
    return blt_bassc_writeCtrlPoint(connHandle, BASS_OPCODE_REMOTE_SCAN_STOPPED, NULL, 0);
}

int blc_bassc_writeRemoteScanStarted(u16 connHandle)
{
    return blt_bassc_writeCtrlPoint(connHandle, BASS_OPCODE_REMOTE_SCAN_STARTED, NULL, 0);
}

int blc_bassc_writeAddSource(u16 connHandle, u8* param, u16 paramLen)
{
    return blt_bassc_writeCtrlPoint(connHandle, BASS_OPCODE_ADD_SOURCE, param, paramLen);
}

int blc_bassc_writeModifySource(u16 connHandle, u8* param, u16 paramLen)
{
    return blt_bassc_writeCtrlPoint(connHandle, BASS_OPCODE_MODIFY_SOURCE, (u8*)param, paramLen);
}

int blc_bassc_writeSetBroadcastCode(u16 connHandle, u8 sourceID, u8 bcstCode[16])
{
    bass_setBcstCodeParam_t param = {.srcId = sourceID};
    memcpy(param.BcstCode, bcstCode, 16);
    return blt_bassc_writeCtrlPoint(connHandle, BASS_OPCODE_SET_BROADCAST_CODE, (u8*)&param, sizeof(bass_setBcstCodeParam_t));
}

int blc_bassc_writeRemoveSource(u16 connHandle, u8 sourceID)
{
    return blt_bassc_writeCtrlPoint(connHandle, BASS_OPCODE_REMOVE_SOURCE, &sourceID, 1);
}




