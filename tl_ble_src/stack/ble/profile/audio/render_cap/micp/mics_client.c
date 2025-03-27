/********************************************************************************************************
 * @file    mics_client.c
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




static const blc_gapc_discList_t discMicp;
#define BLC_MICS_START_SDP(connHandle)          blc_gapc_registerDiscoveryService(connHandle, &discMicp)

static const blc_gapc_reconnList_t reconnMicp;
#define BLC_MICS_START_RECONN(connHandle)       blc_gapc_registerReconnectService(connHandle, &reconnMicp)

_attribute_ble_data_retention_
blc_micp_client_ctrl_t micp_client_ctrl = {
    .process = {
        .pNext = NULL,
        .id = AUDIO_MICS_CLIENT,
        .usedAclRole = 0,
        .init = blt_micsc_init,
        .connect = blt_micsc_connect,
        .discov = blt_micsc_discovery,
        .loop = NULL,
        .store = blt_micsc_nv_store,
    },
};

blc_micp_client_t *blt_micp_getClientInst(u16 connHandle)
{
    int ret = blt_prf_getAclRole(connHandle);
    if(ret < 0 || ret == ACL_ROLE_PERIPHERAL) {
        BLT_MICS_LOG("ERR: ACL role, unlikely: 0x%x", ret);

        if(ret >= 0){
            /* MICP Microphone Controller GAP Central */
            blt_prf_sendSvrGapRoleErrEvt(connHandle, AUDIO_MICS_CLIENT, ret);
        }

        return NULL;
    }

    int idx = blc_prf_getAclConnectIndex(connHandle); //already checked aclHandle
    return micp_client_ctrl.pMicpClient[idx];
}

static blc_mics_client_t *blt_micsc_getClientInst(u16 connHandle)
{
    blc_micp_client_t *client = blt_micp_getClientInst(connHandle);
    if(client == NULL){
        return NULL;
    }

    return &client->micsClient;
}

void blc_audio_registerMICSControlClient(const blc_micsc_regParam_t *param)
{
    blc_prf_registerServiceModule(PRF_GAP_ACL_CENTRAL, (blc_prf_proc_t*)&micp_client_ctrl, param);
}

int blt_micsc_init(u8 initType, const void* param)
{
#if(BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_micp_client_ctrl_t)), gMicscCtrl);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_micp_client_t)), blc_micp_client_t);
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_mics_client_t)), blc_mics_client_t);
#endif
    (void)param;
    if(initType == PRF_PROC_INIT) {
        BLT_MICS_LOG("Client init");

        for(int i=0; i<gAppAudioAclCentralNum; i++) {
            micp_client_ctrl.pMicpClient[i] = blt_micsc_getClientBuf(i);
            /* Clear VCS Client parameters  */
            memset(micp_client_ctrl.pMicpClient[i], 0, sizeof(blc_micp_client_t));
            /* Initialize Pointer buffer */
            //MICS Client Discovery will do this (Initialize Include SVC Pointer buffer: pAicsClient)
            blt_aicsc_init(initType);
        }
    }
//  else if (initType == PRF_PROC_DEINIT) {
//  }
    return 0;
}

static int blt_micp_disconnect(u16 connHandle)
{
    if (blt_ll_isAclHandleOutOfRange(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_micp_client_t* micp = blt_micp_getClientInst(connHandle);

    for(int i=0; i<micp->aicsClientCnt; i++)
    {
        memset(micp->pAicsClient[i], 0, sizeof(blc_aics_client_t));
    }

    memset(micp, 0, sizeof(blc_micp_client_t));

    return BLE_SUCCESS;
}

int blt_micsc_connect(u16 connHandle, prf_acl_state_enum connState)
{
    if(connState == PRF_ACL_STATE_DISCONN) {
        BLT_MICS_LOG("Disconnect: 0x%x", connHandle);
        blt_micp_disconnect(connHandle);
    } else {
        BLT_MICS_LOG("Connect: 0x%x", connHandle);
    }
    return 0;
}

int blt_micsc_discovery(u16 connHandle)
{
    BLC_COMMON_SDP_DISCOVERY(connHandle, MICS, mics);
}

int blt_micsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param)
{
    blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);

    if(nvState == PRF_NV_STATE_STORE)   {
        if(micp->micsClient.ntfInput.startHdl) {
            u8* nvDataPtrTemp = param->dataPtr;
            param->dataPtr ++;

            U8_TO_STREAM(param->dataPtr, AUDIO_MICS_CLIENT);
            blt_prf_storeClientHdl(param->dataPtr, &micp->micsClient, &micp->micsClient.muteHdl);
            param->dataPtr+=sizeof(blt_mics_nv_info_t);
            U8_TO_STREAM(param->dataPtr, micp->aicsClientCnt);
            blt_aicsc_micpStore(connHandle, param);

            u8 paramLen = param->dataPtr - nvDataPtrTemp - 2;
            *nvDataPtrTemp = paramLen;
            param->currentTotalLen += 2 + paramLen;
        }

    }
    else if(nvState == PRF_NV_STATE_LOAD) {
        blt_prf_loadClientHdl(&micp->micsClient, param->dataPtr, &micp->micsClient.muteHdl);
        param->dataPtr+=sizeof(blt_mics_nv_info_t);
        STREAM_TO_U8(micp->aicsClientCnt, param->dataPtr);
        if(micp->aicsClientCnt)
        {
            blt_aicsc_micpLoad(connHandle, param);
        }
    }
    return 0;
}


int blt_micsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);
    if(attHdl == client->muteHdl)
    {
        if(valLen != 1 || *val >= MICS_MUTE_VALUE_RFU)
        {
            return ATT_SUCCESS;
        }

        client->muteValue = *val;

        blc_micsc_muteChangeEvt_t evt;
        evt.mute = client->muteValue;
        blt_prf_sendEvent(connHandle, AUDIO_EVT_MICSC_CHANGE_MUTE, (u8*)&evt, sizeof(blc_micsc_muteChangeEvt_t));
        return ATT_SUCCESS;
    }
    return ATT_ERR_INVALID_HANDLE;
}

void blt_micp_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    if(blt_micsc_dataInput(connHandle, attHdl, val, valLen) == ATT_SUCCESS)
    {
        return ;
    }
    blt_aicsc_micpDataInput(connHandle, attHdl, val, valLen);
}

static void blt_micsc_displayInfo(u16 connHandle, blc_mics_client_t* client)
{
    BLT_MICS_LOG("[MICS] sdp over connHandle[0x%x]", connHandle);
    BLT_MICS_LOG("  INFO: Mute Handle[0x%x] mute[%d]", client->muteHdl, client->muteValue);
    blt_aicsc_micpFoundServiceEnd(connHandle);
}

static void blt_micsc_foundService(u16 connHandle, u8 count, u16 startHandle, u16 endHandle)
{
    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);

    if(count == 0xFF)
    {
        blc_prf_sendServiceDiscoveryFailEvent(connHandle, AUDIO_MICS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        BLT_MICS_LOG("ERR:not found MICS");
        return ;
    }

    if(count == 0)
    {
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_MICS_CLIENT);
        blt_micsc_displayInfo(connHandle, client);
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return ;
    }

    client->ntfInput.startHdl = startHandle;
    client->ntfInput.endHdl = endHandle;
    client->ntfInput.ntfOrIndFunc = blt_micp_dataInput;
    BLT_MICS_LOG("  INFO: MICS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, startHandle, endHandle);
    blc_prf_sendServiceDiscoveryFoundEvent(connHandle, AUDIO_MICS_CLIENT, startHandle, endHandle);
}

static void blt_micsc_foundMuteChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);
    client->muteHdl = valueHandle;
    BLT_MICS_LOG("[MICS] mute ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_micsc_muteStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);
    *read = (u8*)&client->muteValue;
    *readLen = NULL;
    *readMaxSize = 1;
    *rdCbFunc = NULL;
}


static const blc_gapc_discService_t micsService = {
    .uuid = UUID16_INIT(SERVICE_UUID_MICROPHONE_CONTROL),
    .sfun = blt_micsc_foundService,
};

static const blc_gapc_discChar_t micsChar[] = {
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_MUTE),
        .cfun = blt_micsc_foundMuteChar,
        .rfun = blt_micsc_muteStartRead,
    },
};

extern const blc_gapc_discInclude_t discAics;

static const blc_gapc_discList_t discMicp = {
    .maxServiceCount = 1,
    .service = &micsService,
    .includeTable = {
        .size = 1,
        .include[0] = &discAics,
    },
    .characteristicTable = {
        .size = ARRAY_SIZE(micsChar),
        .characteristic = micsChar,
    },
};


/**********reconnect function start*********/
static bool blt_micsc_reconnService(u16 connHandle, int count)
{
    if(count == 0)
    {
        blc_mics_client_t *client = blt_micsc_getClientInst(connHandle);

        blt_micsc_displayInfo(connHandle, client);
        BLT_MICS_LOG("  INFO: MICS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
        blc_prf_sendSingleServiceDiscoveryFinishEvent(connHandle, AUDIO_MICS_CLIENT);
        blc_prf_setDiscoveryStatusFinish(connHandle);
        return true;
    }

    if(count > 1)
        return false;
    return true;
}

static int blt_micsc_muteGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->muteHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static const blc_gapc_reconnChar_t reMicsChar[] = {

    {
        .ifun = blt_micsc_muteGetInfo,
        .rfun = blt_micsc_muteStartRead,
    },
};

extern const blc_gapc_reconnInclTable_t reconnAics;

static const blc_gapc_reconnList_t reconnMicp = {
    .serviceUuid = UUID16_INIT(SERVICE_UUID_MICROPHONE_CONTROL),
    .resfun = blt_micsc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reMicsChar),
        .characteristic = reMicsChar,
    },
    .inclSize = 1,
    .includeCharTb[0] = &reconnAics,
};

/**********reconnect function ending********/




/*************************************************************************
 *  GATTC Write Characteristics
 *  CHARACTERISTIC_UUID_VOLUME_CONTROL_POINT,
 *************************************************************************/
static void blc_micsc_writeMuteCb(u16 connHandle, u8 err, void* data)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);
    blc_prf_writeAttributeValueCallback(connHandle, err);

    if (err) {
        BLT_MICS_LOG("WR_CB INFO: ERR: 0x%x", err);
    } else {
        BLT_MICS_LOG("WR_CB INFO: SUCC");
    }
    (void)data;
}

int blc_micsc_writeMute(u16 connHandle, blc_mics_mute_value_enum mute, prf_write_cb_t writeCb)
{

    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        BLT_MICS_LOG("ERR: ACL handle invalid");
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);

    if (mute >= MICS_MUTE_VALUE_RFU) {
        BLT_MICS_LOG("ERR: mute state[0x%x] invalid", mute);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;
    u8 muteState = mute;

    pGapWrCfg.func = blc_micsc_writeMuteCb;
    pGapWrCfg.handle = client->muteHdl;
    pGapWrCfg.data = &muteState;
    pGapWrCfg.length = sizeof(muteState);
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData = NULL;
    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
}


static void blc_micsc_readMuteCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    (void)pRdCfg;
    blc_prf_readAttributeValueCallback(connHandle, err);

    if(err == ATT_SUCCESS)
    {
        blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);
        blc_micsc_muteChangeEvt_t evt;
        evt.mute = client->muteValue;
        blt_prf_sendEvent(connHandle, AUDIO_EVT_MICSC_CHANGE_MUTE, (u8*)&evt, sizeof(blc_micsc_muteChangeEvt_t));
    }

}

int blc_micsc_readMute(u16 connHandle, prf_read_cb_t readCb)
{
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    blc_mics_client_t* client = blt_micsc_getClientInst(connHandle);

    if(!client)
    {
        return HCI_ERR_UNKNOWN_CONN_ID;
    }

    gapc_read_cfg_t pGapReCfg;

    pGapReCfg.func = blc_micsc_readMuteCb;
    pGapReCfg.handle = client->muteHdl;
    pGapReCfg.wBuff = (u8*)&client->muteValue;
    pGapReCfg.wBuffLen = NULL;
    pGapReCfg.maxLen = 1;

    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

#define BLT_MICSC_CHECK_PARAM(connHandle, attrHandle)       if(blt_ll_isAclHandleOutOfRange(connHandle)) return AUDIO_ERR_CONNHANDLE_INVALID;   \
                                                            blc_mics_client_t* client = blt_micsc_getClientInst(connHandle); \
                                                            if(client == NULL)      return AUDIO_ERR_CONNHANDLE_INVALID;    \
                                                            if(client->attrHandle == 0) return AUDIO_ERR_GET_ATTR_HANDLE_NOT_FOUND

int blc_micsc_getMute(u16 connHandle, u8* mute)
{
    BLT_AUD_CHECK_NULL_PTR(mute);
    BLT_MICSC_CHECK_PARAM(connHandle, muteHdl);

    *mute = client->muteValue;

    return AUDIO_ESUCC;
}







