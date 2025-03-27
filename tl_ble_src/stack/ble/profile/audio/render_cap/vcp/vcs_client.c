/********************************************************************************************************
 * @file    vcs_client.c
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





static int blt_vcsc_vcpVolCtrlDisconnect(u16 connHandle);
blc_vcs_client_t *blt_vcsc_getClientInst(u16 connHandle);

static const blc_gapc_discList_t discVcpRenderer;
#define BLC_VCP_START_SDP(connHandle)       blc_gapc_registerDiscoveryService(connHandle, &discVcpRenderer)

static const blc_gapc_reconnList_t reconnVcpRenderer;
#define BLC_VCP_START_RECONN(connHandle)        blc_gapc_registerReconnectService(connHandle, &reconnVcpRenderer)

_attribute_ble_data_retention_
blc_vcp_client_ctrl_t vcp_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_VCP_CLIENT,
        .usedAclRole = 0,
        .init = blt_vcpVolCtrl_init,
        .connect = blt_vcpVolCtrl_connect,
        .discov = blt_vcpVolCtrl_discovery,
        .loop = NULL,
        .store = blt_vcpVolCtrl_nv_store,
    },
};


void blc_audio_registerVCSControlClient(const blc_vcsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&vcp_client_ctrl, param);
}

int blt_vcpVolCtrl_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_vcp_client_t)), blc_vcp_client_t);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_vcs_client_t)), blc_vcs_client_t);
#endif
    (void)param;
    if(initType == PRF_PROC_INIT) {
        BLT_VCS_LOG("Client init");

        for(int i=0; i<gAppAudioAclCentralNum; i++) {
            vcp_client_ctrl.pVcpCtrl[i] = blt_vcsc_getClientBuf(i);
            /* Clear VCS Client parameters  */
            memset(vcp_client_ctrl.pVcpCtrl[i], 0, sizeof(blc_vcp_client_t));
            /* Initialize Pointer buffer */
            //VCS Client Discovery will do this (Initialize Include SVC Pointer buffer: pAicsClient && pVocsClient)
            blt_vocsc_init(initType);
            blt_aicsc_init(initType);
        }
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      BLT_VCS_LOG("Client deinit");
//  }
    return 0;
}

int blt_vcpVolCtrl_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_VCS_LOG("Disconnect:0x%x", connHandle);
        blt_vcsc_vcpVolCtrlDisconnect(connHandle);
    } else {
        BLT_VCS_LOG("Connect:0x%x", connHandle);
    }
    return 0;
}

int blt_vcpVolCtrl_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, VCP, vcs);
}

int blt_vcpVolCtrl_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);

    if(nvState == PRF_NV_STATE_STORE)   {
        if(vcp->vcsClient.ntfInput.startHdl) {
            u8* nvDataPtrTemp = param->dataPtr;
            param->dataPtr ++;

            U8_TO_STREAM(param->dataPtr, AUDIO_VCP_CLIENT);
            blt_prf_storeClientHdl(param->dataPtr, &vcp->vcsClient, &vcp->vcsClient.volFlagHdl);
            param->dataPtr+=sizeof(blt_vcs_att_hdl_t);
            U8_TO_STREAM(param->dataPtr, vcp->aicsClientCnt);
            U8_TO_STREAM(param->dataPtr, vcp->vocsClientCnt);
            blt_aicsc_vcpStore(connHandle, param);
            blt_vocsc_store(connHandle, param);
            u8 paramLen = param->dataPtr - nvDataPtrTemp - 2;
            *nvDataPtrTemp = paramLen;
            param->currentTotalLen += 2 + paramLen;
        }

    }
    else if(nvState == PRF_NV_STATE_LOAD) {
        blt_prf_loadClientHdl(&vcp->vcsClient, param->dataPtr, &vcp->vcsClient.volFlagHdl);
        param->dataPtr+=sizeof(blt_vcs_att_hdl_t);
        STREAM_TO_U8(vcp->aicsClientCnt, param->dataPtr);
        STREAM_TO_U8(vcp->vocsClientCnt, param->dataPtr);
        if(vcp->aicsClientCnt)
        {
            blt_aicsc_vcpLoad(connHandle, param);
        }
        if(vcp->vocsClientCnt)
        {
            blt_vocsc_load(connHandle, param);
        }
        vcp->vcsClient.ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &vcp->vcsClient.ntfInput);
    }
    return 0;
}

blc_vcp_client_t *blt_vcp_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_PERIPHERAL) {
        BLT_VCS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* VCP Volume Controller: GAP Central */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_VCP_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return vcp_client_ctrl.pVcpCtrl[idx];
}

blc_vcs_client_t *blt_vcsc_getClientInst(u16 connHandle)
{
    blc_vcp_client_t *client = blt_vcp_getClientInst(connHandle);
    if(client == NULL){
        return NULL;
    }
    return &client->vcsClient;
}

static int blt_vcsc_vcpVolCtrlDisconnect(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);

    for(int i=0; i<vcp->aicsClientCnt; i++)
        memset(vcp->pAicsClient[i], 0, sizeof(blc_aics_client_t));

    for(int i=0; i<vcp->vocsClientCnt; i++)
        memset(vcp->pVocsClient[i], 0, sizeof(blc_vocs_client_t));

    memset(vcp, 0, sizeof(blc_vcp_client_t));

    return BLE_SUCCESS;
}

int blc_vcpc_getState(u16 connHandle, blc_audio_vcpState_t* vcpState)
{
    blc_vcp_client_t *client = blt_vcp_getClientInst(connHandle);
    if(client == NULL || vcpState == NULL)
        return AUDIO_EMPTY;

    vcpState->volState.volumeSetting = client->vcsClient.volState.volSetting;
    vcpState->volState.mute = client->vcsClient.volState.mute == VCS_MUTE_STATE_NOT_MUTED? false: true;

    vcpState->vosCnt = client->vocsClientCnt;

    for(int i=0; i<client->vocsClientCnt; i++)
    {
        blc_audio_volumeOffsetState_t *vocState = &vcpState->voc[i];
        blc_vocs_client_t *vocs = client->pVocsClient[i];
        vocState->volumeOffset = vocs->volOffsetState.volOffset;
        vocState->location = vocs->audioLocation;
        vocState->outDescLen = vocs->audioOutDescLen;
        vocState->outDesc = vocs->audioOutDesc;
    }

    vcpState->aisCnt = client->aicsClientCnt;

    for(int i=0; i<client->aicsClientCnt; i++)
    {
        blc_audio_inputState_t *aisState = &vcpState->ais[i];
        blc_aics_client_t *aics = client->pAicsClient[i];
        aisState->gainSetting = aics->audioInState.gainSetting;
        aisState->mute = aics->audioInState.mute;
        aisState->gainMode = aics->audioInState.gainMode;
        aisState->gainSettingUnits = aics->gainSettingProperties.gainSettingUnits;
        aisState->minGainSetting = aics->gainSettingProperties.gainSettingMinimum;
        aisState->maxGainSetting = aics->gainSettingProperties.gainSettingMaximum;
        aisState->inputType = aics->audioInType;
        aisState->inputStatus = aics->audioInStatus;
        aisState->inDescLen = aics->audioInDescLen;
        aisState->inDesc = aics->audioInDesc;
    }
    return AUDIO_ESUCC;

}

int blt_vcsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);

    if(attHdl == client->volStateHdl)
    {
        if(valLen != sizeof(blc_vcs_volume_state_t))
            return ATT_SUCCESS;
        memcpy(&client->volState, val, valLen);
        BLT_VCS_LOG("NTF INFO: Vol state Handle[0x%x] volumeSetting[0x%x] mute[0x%x] changeCounter[0x%x]", client->volStateHdl, client->volState.volSetting,
                        client->volState.mute, client->volState.changeCnt);

        blc_vcsc_volumeStateChangeEvt_t evt;
        evt.volumeSetting = client->volState.volSetting;
        evt.mute = client->volState.mute == VCS_MUTE_STATE_NOT_MUTED? false: true;
        blt_prf_sendEvent(connHandle, AUDIO_EVT_VCSC_CHANGED_VOLUME_STATE, (u8*)&evt, sizeof(blc_vcsc_volumeStateChangeEvt_t));
        return ATT_SUCCESS;
    }
    else if(attHdl == client->volFlagHdl)
    {
        if(valLen != 1)
            return ATT_SUCCESS;
        BLT_VCS_LOG("NTF INFO: Vol Flags Handle[0x%x] flags[0x%x]", client->volFlagHdl, client->volFlag.volSettingPersisted);
        *(u8*)&client->volFlag = *val;
        return ATT_SUCCESS;
    }
    return ATT_ERR_INVALID_HANDLE;
}

void blt_vcp_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    if(blt_vcsc_dataInput(connHandle, attHdl, val, valLen) == ATT_SUCCESS)
    {
        return ;
    }
    if(blt_vocsc_dataInput(connHandle, attHdl, val, valLen) == ATT_SUCCESS)
    {
        return ;
    }
    blt_aicsc_vcpDataInput(connHandle, attHdl, val, valLen);
}

static void blt_vcsc_displayInfo(u16 connHandle, blc_vcs_client_t* client)
{
    BLT_VCS_LOG("sdp over connHandle[0x%x]", connHandle);
    BLT_VCS_LOG("   INFO: Vol state Handle[0x%x] volumeSetting[0x%x] mute[0x%x] changeCounter[0x%x]", client->volStateHdl, client->volState.volSetting,
            client->volState.mute, client->volState.changeCnt);
    BLT_VCS_LOG("   INFO: Vol Control Point Handle[0x%x]", client->volCtrlPntHdl);
    BLT_VCS_LOG("   INFO: Vol Flags Handle[0x%x] flags[%x]", client->volFlagHdl, client->volFlag.volSettingPersisted);

    blt_vocsc_foundServiceEnd(connHandle);
    blt_aicsc_vcpFoundServiceEnd(connHandle);

    blc_vcsc_volumeStateChangeEvt_t evt;
    evt.volumeSetting = client->volState.volSetting;
    evt.mute = client->volState.mute == VCS_MUTE_STATE_NOT_MUTED? false: true;
    blt_prf_sendEvent(connHandle, AUDIO_EVT_VCSC_CHANGED_VOLUME_STATE, (u8*)&evt, sizeof(blc_vcsc_volumeStateChangeEvt_t));

}

static void blt_vcsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);

    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_VCP_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_VCS_LOG("ERR:not found VCP");
        return ;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_VCP_CLIENT);
        blt_vcsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }

    client->ntfInput.startHdl = startHandle;
    client->ntfInput.endHdl = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
    BLT_VCS_LOG("   INFO: VCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_VCP_CLIENT, startHandle, endHandle);
}

static void blt_vcsc_foundCtrlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);
    if(properties & CHAR_PROP_WRITE)
        client->volCtrlPntHdl = valueHandle;
    BLT_VCS_LOG("volume control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vcsc_foundVolumeStateChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);
    client->volStateHdl = valueHandle;
    BLT_VCS_LOG("volume state ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vcsc_volumeStateStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);
    *read = (u8*)&client->volState;
    *readLen = NULL;
    *readMaxSize = sizeof(blc_vcs_volume_state_t);
    *rdCbFunc = NULL;
}

static void blt_vcsc_foundVolumeFlagsChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);
    client->volFlagHdl = valueHandle;
    BLT_VCS_LOG("volume flags ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vcsc_volumeFlagsStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);
    *read = (u8*)&client->volFlag;
    *readLen = NULL;
    *readMaxSize = sizeof(client->volFlag);
    *rdCbFunc = NULL;
}

static const blc_gapc_discService_t vcsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL),
    .sfun = blt_vcsc_foundService,
};

static const blc_gapc_discChar_t vcsChar[] = {
    {
        .setting = 0,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_CONTROL_POINT),
        .cfun = blt_vcsc_foundCtrlPointChar,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_STATE),
        .cfun = blt_vcsc_foundVolumeStateChar,
        .rfun = blt_vcsc_volumeStateStartRead,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_FLAGS),
        .cfun = blt_vcsc_foundVolumeFlagsChar,
        .rfun = blt_vcsc_volumeFlagsStartRead,
    }
};

extern const blc_gapc_discInclude_t discVocs;
extern const blc_gapc_discInclude_t discAics;

static const blc_gapc_discList_t discVcpRenderer = {
    .maxServiceCount = 1,
    .service = &vcsService,
    .includeTable = {
        .size = 2,
        .include[0] = &discVocs,
        .include[1] = &discAics,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(vcsChar),
        .characteristic = vcsChar,
    },
};


/**********reconnect function start*********/
static bool blt_vcsc_reconnService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_vcs_client_t *client = blt_vcsc_getClientInst(connHandle);
        blt_vcsc_displayInfo(connHandle, client);
        BLT_VCS_LOG("   INFO: VCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_VCP_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}

static int blt_vcsc_volumeStateGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->volStateHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static int blt_vcsc_volumeFlagsGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->volFlagHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static const blc_gapc_reconnChar_t reVcsChar[] = {

    {
        .ifun = blt_vcsc_volumeStateGetInfo,
        .rfun = blt_vcsc_volumeStateStartRead,
    },

    {
        .ifun = blt_vcsc_volumeFlagsGetInfo,
        .rfun = blt_vcsc_volumeFlagsStartRead,
    },
};

extern const blc_gapc_reconnInclTable_t reconnAics;
extern const blc_gapc_reconnInclTable_t reconnVocs;

static const blc_gapc_reconnList_t reconnVcpRenderer = {
    .serviceUuid = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL),
    .resfun = blt_vcsc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reVcsChar),
        .characteristic = reVcsChar,
    },
    .inclSize = 2,
    .includeCharTb[0] = &reconnAics,
    .includeCharTb[1] = &reconnVocs,
};

/**********reconnect function ending********/


/*************************************************************************
 *  GATTC Read Characteristics
 *  CHARACTERISTIC_UUID_VOLUME_STATE,
 *  CHARACTERISTIC_UUID_VOLUME_FLAGS,
 *************************************************************************/
static void blt_vcsc_readAttrValCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    (void)connHandle;
    (void)pRdCfg;
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    if(err) BLT_VCS_LOG("RD_CB  INFO: ERR: read failed: 0x%x", err);
}

static int blt_vcsc_readAttrVal(u16 connHandle, blt_vcs_read_enum  rdType)
{
    BLT_VCS_LOG("blt_vcsc_readAttrVal:%d", rdType);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_VCS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= VCS_READ_MAX) {
        BLT_VCS_LOG("ERR: Invalid read type %d", rdType);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);
    u16 volHdl = rdType == VCS_READ_VOL_STATE ? client->volStateHdl : client->volFlagHdl;
    if (!volHdl) {
        BLT_VCS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_read_cfg_t pGapReCfg;

    pGapReCfg.func = blt_vcsc_readAttrValCb;
    if(rdType == VCS_READ_VOL_STATE)
    {
        pGapReCfg.handle = volHdl;
        pGapReCfg.wBuff = (u8*)&client->volState;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen = sizeof(blc_vcs_volume_state_t);
    }
    else if(rdType == VCS_READ_VOL_FLAG)
    {
        pGapReCfg.handle = volHdl;
        pGapReCfg.wBuff = (u8*)&client->volFlag;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen = sizeof(client->volFlag);
    }

    return blc_gapc_readAttributeValue(connHandle, &pGapReCfg);
}

int blc_vcsc_readVolState(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_vcsc_readAttrVal(connHandle, VCS_READ_VOL_STATE);
}

int blc_vcsc_readVolFlags(u16 connHandle, prf_read_cb_t readCb)
{
    (void)readCb;
    return blt_vcsc_readAttrVal(connHandle, VCS_READ_VOL_FLAG);
}

/*************************************************************************
 *  GATTC Write Characteristics
 *  CHARACTERISTIC_UUID_VOLUME_CONTROL_POINT,
 *************************************************************************/
static void blt_vcsc_writeCtrlPntCb(u16 connHandle, u8 err, void* data)
{
    (void)data;

    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    if(err == VCS_ERRCODE_INVALID_CHANGE_COUNTER) {
        blc_vcsc_readVolState(connHandle, NULL);
    }

    if (err) {
        BLT_VCS_LOG("WR_CB INFO: ERR: 0x%x", err);
    } else {
        BLT_VCS_LOG("WR_CB INFO: SUCC");
    }

}

static int blt_vcsc_writeCtrlPnt(u16 connHandle, blt_vcs_opcode_enum opcode, u8* volumeSetting)
{
    BLT_VCS_LOG("blt_vcsc_writeCtrlPnt:%d", opcode);

    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_VCS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }else if (opcode >= VCS_OPCODE_MAX && opcode != VCS_OPCODE_BQB_TEST) {
        BLT_VCS_LOG("ERR: Invalid write opcode %d", opcode);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    blc_vcs_client_t* client = blt_vcsc_getClientInst(connHandle);

    if (!client->volCtrlPntHdl) {
        BLT_VCS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    } else if (opcode >= VCS_OPCODE_MAX) {
        BLT_VCS_LOG("ERR: opcode[0x%x] invalid", opcode);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;
    blt_vcs_vol_cp_vol_t ctrl = {
        .cp.opcode = opcode,
        .cp.changeCnt = client->volState.changeCnt,
        .volSetting = *volumeSetting,
    };

    pGapWrCfg.func = blt_vcsc_writeCtrlPntCb;
    pGapWrCfg.handle = client->volCtrlPntHdl;
    pGapWrCfg.data = &ctrl;
    pGapWrCfg.length = opcode == VCS_OPCODE_SET_ABSOLUTE_VOLUME? sizeof(blt_vcs_vol_cp_vol_t): sizeof(blt_vcs_vol_cp_t);
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData = NULL;
    return blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);

}

int blc_vcsc_writeRelativeVolDown(u16 connHandle)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_RELATIVE_VOLUME_DOWN, NULL);
}

int blc_vcsc_writeRelativeVolUp(u16 connHandle)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_RELATIVE_VOLUME_UP, NULL);
}

int blc_vcsc_writeUnmuteOrRelativeVolDown(u16 connHandle)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN, NULL);
}

int blc_vcsc_writeUnmuteOrRelativeVolUp(u16 connHandle)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_UP, NULL);
}

int blc_vcsc_writeSetAbsoluteVol(u16 connHandle, u8 volSetting)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_SET_ABSOLUTE_VOLUME, &volSetting);
}

int blc_vcsc_writeUnmute(u16 connHandle)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_UNMUTE, NULL);
}

int blc_vcsc_writeMute(u16 connHandle)
{
    return blt_vcsc_writeCtrlPnt(connHandle, VCS_OPCODE_MUTE, NULL);
}








