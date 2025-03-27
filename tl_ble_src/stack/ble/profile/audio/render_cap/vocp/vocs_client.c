/********************************************************************************************************
 * @file    vocs_client.c
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

int blt_vocsc_init(u8 initType)
{
    if(initType == PRF_PROC_INIT) {
        BLT_VOCS_LOG("Client init");
        blt_vocsc_cleanBuf();
    }
//  else if (initType == PRF_PROC_DEINIT) {
//      BLT_VOCS_LOG("Client deinit");
//  }
    return 0;
}


void blt_vocsc_foundServiceEnd(u16 connHandle)
{
    blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);
    BLT_VOCS_LOG("sdp over connHandle[0x%x], found vocs count is %d", connHandle, vcp->vocsClientCnt);
    for(int i=0; i<vcp->vocsClientCnt; i++) {
        blc_vocs_client_t* client = vcp->pVocsClient[i];
        BLT_VOCS_LOG("  index[%d], info is", i);
        BLT_VOCS_LOG("  INFO: volume offset state Handle[0x%x] volumeOffset[%d] changeCounter[%d]", client->volOffStateHdl, client->volOffsetState.volOffset,
                client->volOffsetState.changeCnt);
        BLT_VOCS_LOG("  INFO: audio location Handle[0x%x] audioLocation[0x%08x]", client->audioLocationHdl, client->audioLocation);
        BLT_VOCS_LOG("  INFO: volume offset control point Handle[0x%x]", client->volOffCtrlpntHdl);
        BLT_VOCS_LOG("  INFO: audio output description Handle[0x%x] Desc[%s]", client->audioOutDescHdl, client->audioOutDesc);

        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
    }
}

static bool blt_vocsc_foundService(u16 connHandle, u16 startHandle, u16 endHandle)
{
    blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);

    if(vcp->vocsClientCnt >= STACK_AUDIO_VCS_CLIENT_INCLUDE_VOCS_INSTANCE_NUM) {
        BLT_VOCS_LOG("found service too many");
        return false;
    }

    blc_vocs_client_t* vocsc = blt_vocsc_getClientBuf();

    if(!vocsc) {
        BLT_VOCS_LOG("not found new vocsc client");
        return false;
    }

    vcp->pVocsClient[vcp->vocsClientCnt] = vocsc;
    vcp->vocsClientCnt ++;
    vcp->vocsClientIdx ++;

    vocsc->ntfInput.startHdl = startHandle;
    vocsc->ntfInput.endHdl = endHandle;
    vocsc->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;

    BLT_VOCS_LOG("found include service, connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x", connHandle, startHandle, endHandle);
    return true;
}

void blt_vocsc_store(u16 connHandle, prf_nv_param_t *param)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);

//  BLT_VOCS_LOG("  INFO: sdp over connHandle[0x%x], vcp found vocs count is %d", connHandle, vcp->vocsClientCnt);
    for(int i=0; i<vcp->vocsClientCnt; i++) {
        blc_vocs_client_t* client = vcp->pVocsClient[i];

        blt_prf_storeClientHdl(param->dataPtr, client, &client->audioOutDescHdl);
        param->dataPtr += sizeof(blc_vocs_att_hdl_t);
//      BLT_VOCS_LOG("      INFO: index[%d], info is", i);
//      BLT_VOCS_LOG("      INFO: volOffStateHdl[0x%x]", client->volOffStateHdl);
//      BLT_VOCS_LOG("      INFO: audioLocationHdl[0x%x]", client->audioLocationHdl);
//      BLT_VOCS_LOG("      INFO: volOffCtrlpntHdl[0x%x]", client->volOffCtrlpntHdl);
//      BLT_VOCS_LOG("      INFO: audioOutDescHdl[0x%x]", client->audioOutDescHdl);
        client->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
//      BLT_VOCS_LOG("      INFO: VOCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
    }
}

void blt_vocsc_load(u16 connHandle, prf_nv_param_t* param)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);

//  BLT_VOCS_LOG("  INFO: sdp over connHandle[0x%x], vcp found vocs count is %d", connHandle, vcp->vocsClientCnt);
    for(int i=0; i<vcp->vocsClientCnt; i++) {
        blc_vocs_client_t* vocsc = blt_vocsc_getClientBuf();

        vcp->pVocsClient[i] = vocsc;

        blt_prf_loadClientHdl(vocsc, param->dataPtr, &vocsc->audioOutDescHdl);
        param->dataPtr += sizeof(blc_vocs_att_hdl_t);

//      BLT_VOCS_LOG("      INFO: index[%d], info is", i);
//      BLT_VOCS_LOG("      INFO: volOffStateHdl[0x%x]", vocsc->volOffStateHdl);
//      BLT_VOCS_LOG("      INFO: audioLocationHdl[0x%x]", vocsc->audioLocationHdl);
//      BLT_VOCS_LOG("      INFO: volOffCtrlpntHdl[0x%x]", vocsc->volOffCtrlpntHdl);
//      BLT_VOCS_LOG("      INFO: audioOutDescHdl[0x%x]", vocsc->audioOutDescHdl);
        vocsc->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &vocsc->ntfInput);
//      BLT_VOCS_LOG("      INFO: VOCS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, vocsc->ntfInput.startHdl, vocsc->ntfInput.endHdl);
    }
}

static blc_vocs_client_t* blt_vocsc_getSdpInst(u16 connHandle)
{
    blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);
    return vcp->pVocsClient[vcp->vocsClientIdx-1];
}

static void blt_vocsc_foundVolumeOffsetStateChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    vocsc->volOffStateHdl = valueHandle;

    BLT_VOCS_LOG("volume offset state ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vocsc_volumeOffsetStateStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    *read = (u8*)&vocsc->volOffsetState;
    *readLen = 0;
    *readMaxSize = sizeof(blc_vocs_volume_offset_state_t);
    *rdCbFunc = NULL;
    BLT_VOCS_LOG("will read volume offset state handle value");
}

static void blt_vocsc_foundAudioLocationChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    vocsc->audioLocationHdl = valueHandle;

    BLT_VOCS_LOG("audio location ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vocsc_audioLocationStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    *read = (u8*)&vocsc->audioLocation;
    *readLen = 0;
    *readMaxSize = sizeof(vocsc->audioLocation);
    *rdCbFunc = NULL;
    BLT_VOCS_LOG("will read audio location handle value");
}

static void blt_vocsc_foundVolOffsetCtrlPointChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    vocsc->volOffCtrlpntHdl = valueHandle;

    BLT_VOCS_LOG("volume offset control Point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vocsc_foundAudioOutputDescChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    vocsc->audioOutDescHdl = valueHandle;

    BLT_VOCS_LOG("audio output description ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_vocsc_audioOutputDescStartRead(u16 connHandle, u16 attrHandle, u8** read, u16** readLen, u16* readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_vocs_client_t* vocsc = blt_vocsc_getSdpInst(connHandle);

    *read = (u8*)&vocsc->audioOutDesc[0];
    *readLen = &vocsc->audioOutDescLen;
    *readMaxSize = VOCS_READ_OUTPUT_DESC_MAX_SIZE;
    *rdCbFunc = NULL;
    BLT_VOCS_LOG("will read audio output description handle value");
}

static blc_gapc_discChar_t vocsChar[] = {
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_OFFSET_STATE),
        .cfun = blt_vocsc_foundVolumeOffsetStateChar,
        .rfun = blt_vocsc_volumeOffsetStateStartRead,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_AUDIO_LOCATION),
        .cfun = blt_vocsc_foundAudioLocationChar,
        .rfun = blt_vocsc_audioLocationStartRead,
    },
    {
        .setting = 0,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_VOLUME_OFFSET_CONTROL_POINT),
        .cfun = blt_vocsc_foundVolOffsetCtrlPointChar,
    },
    {
        .subscribeNtf = true,
        .readValue = true,
        .uuid = UUID16_INIT(CHARACTERISTIC_UUID_AUDIO_OUTPUT_DESCRIPTION),
        .cfun = blt_vocsc_foundAudioOutputDescChar,
        .rfun = blt_vocsc_audioOutputDescStartRead,
    },
};

const blc_gapc_discInclude_t discVocs = {
    .uuid = UUID16_INIT(SERVICE_UUID_VOLUME_OFFSET_CONTROL),
    .characteristic = {
        .size = ARRAY_SIZE(vocsChar),
        .characteristic = vocsChar,
    },
    .ifun = blt_vocsc_foundService,
};


/**********reconnect function start*********/
static bool blt_vocsc_reconnService(u16 connHandle, int count)
{
    const uuid_t* uuid = blc_gapc_getDiscoveryServiceUUID(connHandle);

    uuid_t vcpUuid = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL);
    if(blc_uuid_cmp(uuid, (const uuid_t*)&vcpUuid) == 0)
    {
        blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);
        vcp->vocsClientIdx++;
        return count <= vcp->vocsClientCnt;
    }

    return false;
}

static int blt_vocsc_volumeOffsetStateGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_vocs_client_t* client = blt_vocsc_getSdpInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->volOffStateHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static int blt_vocsc_audioLocationGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_vocs_client_t* client = blt_vocsc_getSdpInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->audioLocationHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static int blt_vocsc_audioOutputDescGetInfo(u16 connHandle, blc_gapc_charInfo_t* charInfo)
{
    blc_vocs_client_t* client = blt_vocsc_getSdpInst(connHandle);

    charInfo->properties = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->audioOutDescHdl;
    charInfo->cccHandle = 0;

    return 1;
}

static const blc_gapc_reconnChar_t reVocsChar[] = {

    {
        .ifun = blt_vocsc_volumeOffsetStateGetInfo,
        .rfun = blt_vocsc_volumeOffsetStateStartRead,
    },

    {
        .ifun = blt_vocsc_audioLocationGetInfo,
        .rfun = blt_vocsc_audioLocationStartRead,
    },

    {
        .ifun = blt_vocsc_audioOutputDescGetInfo,
        .rfun = blt_vocsc_audioOutputDescStartRead,
    },
};

const blc_gapc_reconnInclTable_t reconnVocs = {
    .reifun = blt_vocsc_reconnService,
    .charTb = {
        .size = ARRAY_SIZE(reVocsChar),
        .characteristic = reVocsChar,
    },
};

/**********reconnect function ending********/

static int blt_vocsc_sendInfoEvt(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);
    for(int i=0; i<vcp->vocsClientCnt; i++) {
        blc_vocs_client_t* vocsc = vcp->pVocsClient[i];
        if(vocsc->volOffStateHdl == attHdl)
        {
            if(val)
            {
                s16 volOffset = *(s16*)val;
                u8 changeCnt = *(val+2);
                if(valLen != sizeof(blc_vocs_volume_offset_state_t) || volOffset > MAX_VOLUME_OFFSET || volOffset < MIN_VOLUME_OFFSET)
                {
                    BLT_VOCS_LOG("NTF ERR: volume offset state[0x%x] is %s", attHdl, hex_to_str(val, valLen));
                    return ATT_SUCCESS;
                }
                vocsc->volOffsetState.volOffset = volOffset;
                vocsc->volOffsetState.changeCnt = changeCnt;
            }

            BLT_VOCS_LOG("INFO: index[%d], volumeOffsetState, handle[0x%x], volume_offset[%d], changeCount[%d]", i, attHdl,
                    vocsc->volOffsetState.volOffset, vocsc->volOffsetState.changeCnt
            );

            blc_vocsc_volumeOffsetStateChangeEvt_t evt;
            evt.vocsIndex = i;
            evt.volumeOffset = vocsc->volOffsetState.volOffset;

            blt_prf_sendEvent(connHandle, AUDIO_EVT_VOCSC_CHANGED_VOLUME_OFFSET, (u8*)&evt, sizeof(blc_vocsc_volumeOffsetStateChangeEvt_t));
            return ATT_SUCCESS;
        }
        else if(vocsc->audioLocationHdl == attHdl)
        {
            if(val)
            {
                u32 location = *(u32*)val;
                if(valLen != 4 || BLC_AUDIO_CHANNEL_ALLOCATION_RFU(location))
                {
                    BLT_VOCS_LOG("NTF ERR: audio location[0x%x] is %s", attHdl, hex_to_str(val, valLen));
                    return ATT_SUCCESS;
                }
                vocsc->audioLocation = location;
            }
            BLT_VOCS_LOG("INFO: index[%d], audio_location, handle[0x%x], location[0x%x]", i, attHdl, vocsc->audioLocation);
            blc_vocsc_locationChangeEvt_t evt;
            evt.vocsIndex = i;
            evt.location = vocsc->audioLocation;
            blt_prf_sendEvent(connHandle, AUDIO_EVT_VOCSC_CHANGED_LOCATION, (u8*)&evt, sizeof(blc_vocsc_locationChangeEvt_t));
            return ATT_SUCCESS;
        }
        else if(vocsc->audioOutDescHdl == attHdl)
        {
            if(val)
            {
                vocsc->audioOutDescLen = min(VOCS_READ_OUTPUT_DESC_MAX_SIZE, valLen);
                memcpy(vocsc->audioOutDesc, val, valLen);
            }

            BLT_VOCS_LOG("INFO: index[%d], output Description, handle[0x%x], Desc:%s", i, attHdl, hex_to_str(vocsc->audioOutDesc, vocsc->audioOutDescLen));
            blc_vocsc_outputDescChangeEvt_t evt;
            evt.vocsIndex = i;
            evt.descLen = vocsc->audioOutDescLen;
            evt.desc = vocsc->audioOutDesc;
            blt_prf_sendEvent(connHandle, AUDIO_EVT_VOCSC_CHANGED_OUTPUT_DESCRIPTION, (u8*)&evt, sizeof(blc_vocsc_outputDescChangeEvt_t));
            return ATT_SUCCESS;
        }
    }
    return ATT_ERR_INVALID_HANDLE;
}

int blt_vocsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    return blt_vocsc_sendInfoEvt(connHandle, attHdl, val, valLen);
}

blc_vocs_client_t* blc_vocsc_vcpGetClientByIndex(u16 connHandle, int index)
{
    blc_vcp_client_t* vcp = blt_vcp_getClientInst(connHandle);

    return vcp->vocsClientCnt>index? vcp->pVocsClient[index]: NULL;
}

static void blt_vocsc_readAttrValueCb(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    blc_prf_readAttributeValueCallback(connHandle, err);
    if(err == ATT_SUCCESS)
    {
        blt_vocsc_sendInfoEvt(connHandle, pRdCfg->single.handle, NULL, 0);
    }
}

static int blt_vocsc_readAttrValue(u16 connHandle, blc_vocs_client_t* vocsc, blt_vocs_read_enum rdType, prf_read_cb_t readCb)
{
    BLT_VOCS_LOG("Read Attribute Value:%d", rdType);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= VOCS_READ_MAX || !vocsc) {
        BLT_VOCS_LOG("ERR: Invalid read type %d, 0x%x", rdType, vocsc);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_read_cfg_t pGapReCfg;

    pGapReCfg.func = blt_vocsc_readAttrValueCb;
    if(rdType == VOCS_READ_VOL_OFFSET_STATE)
    {
        pGapReCfg.handle = vocsc->volOffStateHdl;
        pGapReCfg.wBuff = (u8*)&vocsc->volOffsetState;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen = sizeof(blc_vocs_volume_offset_state_t);
    }
    else if(rdType == VOCS_READ_AUDIO_LOC)
    {
        pGapReCfg.handle = vocsc->audioLocationHdl;
        pGapReCfg.wBuff = (u8*)&vocsc->audioLocation;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen = sizeof(u32);
    }
    else if(rdType == VOCS_READ_OUT_DESC)
    {
        pGapReCfg.handle = vocsc->audioOutDescHdl;
        pGapReCfg.wBuff = (u8*)&vocsc->audioOutDesc;
        pGapReCfg.wBuffLen = &vocsc->audioOutDescLen;
        pGapReCfg.maxLen = VOCS_READ_OUTPUT_DESC_MAX_SIZE;
    }
    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

int blc_vocsc_readVolOffsetState(u16 connHandle, blc_vocs_client_t* vocsc, prf_read_cb_t readCb)
{
    return blt_vocsc_readAttrValue(connHandle, vocsc, VOCS_READ_VOL_OFFSET_STATE, readCb);
}

int blc_vocsc_readAudioLoc(u16 connHandle, blc_vocs_client_t* vocsc, prf_read_cb_t readCb)
{
    return blt_vocsc_readAttrValue(connHandle, vocsc, VOCS_READ_AUDIO_LOC, readCb);
}

int blc_vocsc_readOutputDesc(u16 connHandle, blc_vocs_client_t* vocsc, prf_read_cb_t readCb)
{
    return blt_vocsc_readAttrValue(connHandle, vocsc, VOCS_READ_OUT_DESC, readCb);
}

int blc_vocsc_readVcpVolOffsetState(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blt_vocsc_readAttrValue(connHandle,
            blc_vocsc_vcpGetClientByIndex(connHandle, index),
            VOCS_READ_VOL_OFFSET_STATE, readCb
            );
}

int blc_vocsc_readVcpAudioLoc(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blt_vocsc_readAttrValue(connHandle,
            blc_vocsc_vcpGetClientByIndex(connHandle, index),
            VOCS_READ_AUDIO_LOC, readCb
            );
}

int blc_vocsc_readVcpOutputDesc(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blt_vocsc_readAttrValue(connHandle,
            blc_vocsc_vcpGetClientByIndex(connHandle, index),
            VOCS_READ_OUT_DESC, readCb
            );
}

int blc_vocsc_writeAudioLoc(u16 connHandle, blc_vocs_client_t* vocsc, u32 location)
{
    BLT_VOCS_LOG("blc_vocsc_writeAudioLoc");

    if(!vocsc->audioLocationHdl || !vocsc) {
        BLT_VOCS_LOG("ERR: Audio Location handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;

    pGapWrCfg.handle = vocsc->audioLocationHdl;
    pGapWrCfg.data = &location;
    pGapWrCfg.length = sizeof(u32);
    pGapWrCfg.withoutRsp = true;
    return blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);
}

static void blc_vocsc_writeVolOffsetCtrlPointCb(u16 connHandle, u8 err, void* data)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);
    blc_prf_writeAttributeValueCallback(connHandle, err);

    if(err == VOCS_ERRCODE_INVALID_CHANGE_COUNTER || err == VOCS_ERRCODE_OPCODE_NOT_SUPPORTED) {
        blt_vocsc_readAttrValue(connHandle, (blc_vocs_client_t*)data, VOCS_READ_VOL_OFFSET_STATE, NULL);
    }

    if (err) {
        BLT_VOCS_LOG("WR_CB INFO: ERR: 0x%x", err);
    } else {
        BLT_VOCS_LOG("WR_CB INFO: SUCC");
    }

}

int blc_vocsc_writeVolOffsetCtrlPoint(u16 connHandle, blc_vocs_client_t* vocsc, blt_vocs_volume_offset_control_opcode_enum opcode, s16 volOffset, prf_write_cb_t writeCb)
{

    BLT_VOCS_LOG("blc_vocsc_writeVolOffsetCtrlPoint");
    if(volOffset > MAX_VOLUME_OFFSET || volOffset < MIN_VOLUME_OFFSET) {
        BLT_VOCS_LOG("ERR: invalid volume offset:%d", volOffset);
        return VOCS_ERRCODE_VALUE_OUT_OF_RANGE;
    }

    if(!vocsc->volOffCtrlpntHdl || !vocsc) {
        BLT_VOCS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    if(opcode != VOCS_OPCODE_SET_VOLUME_OFFSET){
        BLT_VOCS_LOG("ERR: opcode not supported, %d", opcode);
        return VOCS_ERRCODE_OPCODE_NOT_SUPPORTED;
    }

    gapc_write_cfg_t pGapWrCfg;

    blt_vocs_set_volume_offset_op_t cmd = {
        .cp.opcode = opcode,
        .cp.changeCnt = vocsc->volOffsetState.changeCnt,
        .volSetting = volOffset,
    };

    pGapWrCfg.func = blc_vocsc_writeVolOffsetCtrlPointCb;
    pGapWrCfg.handle = vocsc->volOffCtrlpntHdl;
    pGapWrCfg.data = &cmd;
    pGapWrCfg.length = sizeof(blt_vocs_set_volume_offset_op_t);
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData = vocsc;
    return blc_prf_writeAttributeValue(connHandle, &pGapWrCfg, writeCb);
}

int blc_vocsc_writeOutputDesc(u16 connHandle, blc_vocs_client_t* vocsc, u8* desc, u16 descLen)
{
    BLT_VOCS_LOG("blc_vocsc_writeOutputDesc");

    if(!vocsc->audioOutDescHdl || !vocsc) {
        BLT_VOCS_LOG("ERR: Audio Output Description handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;

    pGapWrCfg.handle = vocsc->audioOutDescHdl;
    pGapWrCfg.data = desc;
    pGapWrCfg.length = descLen;
    pGapWrCfg.withoutRsp = true;
    return blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);
}

int blc_vocsc_writeVcpAudioLoc(u16 connHandle, int index, u32 location)
{
    return blc_vocsc_writeAudioLoc(connHandle, blc_vocsc_vcpGetClientByIndex(connHandle, index), location);
}

int blc_vocsc_writeVcpOutputDesc(u16 connHandle, int index, u8* desc, u16 descLen)
{
    return blc_vocsc_writeOutputDesc(connHandle, blc_vocsc_vcpGetClientByIndex(connHandle, index), desc, descLen);
}

int blc_vocsc_writeVcpVolOffsetCtrlPoint(u16 connHandle, int index, blt_vocs_volume_offset_control_opcode_enum opcode, s16 volOffset, prf_write_cb_t writeCb)
{
    return blc_vocsc_writeVolOffsetCtrlPoint(connHandle, blc_vocsc_vcpGetClientByIndex(connHandle, index), opcode, volOffset, writeCb);
}

int blc_vocsc_writeSetVolOffset(u16 connHandle, blc_vocs_client_t* vocsc, s16 volOffset, prf_write_cb_t writeCb)
{
    return blc_vocsc_writeVolOffsetCtrlPoint(connHandle, vocsc, VOCS_OPCODE_SET_VOLUME_OFFSET, volOffset, writeCb);
}

int blc_vocsc_vcpWriteSetVolOffset(u16 connHandle, int index, s16 volOffset, prf_write_cb_t writeCb)
{
    return blc_vocsc_writeVolOffsetCtrlPoint(connHandle, blc_vocsc_vcpGetClientByIndex(connHandle, index), VOCS_OPCODE_SET_VOLUME_OFFSET, volOffset, writeCb);
}


#define BLT_VOCSC_CHECK_PARAM(connHandle, attrHandle)       if(blt_ll_isAclHandleOutOfRange(connHandle)) return AUDIO_ERR_CONNHANDLE_INVALID;   \
                                                            if(vocsc == NULL)       return AUDIO_ERR_CONNHANDLE_INVALID;    \
                                                            if(vocsc->attrHandle == 0) return AUDIO_ERR_GET_ATTR_HANDLE_NOT_FOUND

int blc_vocsc_getVolOffsetState(u16 connHandle, blc_vocs_client_t* vocsc, blc_vocs_volume_offset_state_t* state)
{
    BLT_AUD_CHECK_NULL_PTR(state);
    BLT_VOCSC_CHECK_PARAM(connHandle, volOffStateHdl);

    *state = vocsc->volOffsetState;
    return AUDIO_ESUCC;
}

int blc_vocsc_getAudioLoc(u16 connHandle, blc_vocs_client_t* vocsc, u32* location)
{
    BLT_AUD_CHECK_NULL_PTR(location);
    BLT_VOCSC_CHECK_PARAM(connHandle, audioLocationHdl);

    *location = vocsc->audioLocation;
    return AUDIO_ESUCC;
}

int blc_vocsc_getOutputDesc(u16 connHandle, blc_vocs_client_t* vocsc, u8* desc, u16* descLen)
{
    BLT_AUD_CHECK_NULL_PTR(desc, descLen);
    BLT_VOCSC_CHECK_PARAM(connHandle, audioOutDescHdl);

    *descLen = vocsc->audioOutDescLen;
    memcpy(desc, vocsc->audioOutDesc, vocsc->audioOutDescLen);
    return AUDIO_ESUCC;
}

int blc_vocsc_getVcpVolOffsetState(u16 connHandle, int index, blc_vocs_volume_offset_state_t* state)
{
    return blc_vocsc_getVolOffsetState(connHandle,
            blc_vocsc_vcpGetClientByIndex(connHandle, index),
            state
            );
}

int blc_vocsc_getVcpAudioLoc(u16 connHandle, int index, u32* location)
{
    return blc_vocsc_getAudioLoc(connHandle,
            blc_vocsc_vcpGetClientByIndex(connHandle, index),
            location
            );
}

int blc_vocsc_getVcpOutputDesc(u16 connHandle, int index, u8* desc, u16* descLen)
{
    return blc_vocsc_getOutputDesc(connHandle,
            blc_vocsc_vcpGetClientByIndex(connHandle, index),
            desc, descLen
            );
}

