/********************************************************************************************************
 * @file    aics_client.c
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

int blt_aicsc_init(u8 initType)
{
#if (BLT_STRUCT_4B_ALIGN_CHECK_EN)
    STATIC_ASSERT_FILE(IS_4BYTE_ALIGN(sizeof(blc_aics_client_t)), blc_aics_client_t);
#endif

    if (initType == PRF_PROC_INIT) {
        BLT_AICS_LOG("Client init");
        blt_aicsc_cleanAllClientControlBuffer();
    }
    //  else if (initType == PRF_PROC_DEINIT) {
    //      BLT_AICS_LOG("Client deinit");
    //  }
    return 0;
}

static int blt_aicsc_sendInfoEvt(blc_aics_client_t *client, u16 attHandle)
{
    u16 connHandle  = client->connHandle;
    u8  vcpInclIdx  = 0xFF;
    u8  micpInclIdx = 0xFF;
    if (client->vcpInclFlag) {
        blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);
        for (int i = 0; i < vcp->aicsClientCnt; i++) {
            if (vcp->pAicsClient[i] == client) {
                vcpInclIdx = i;
                break;
            }
        }
    }

    if (client->micpInclFlag) {
        blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);
        for (int i = 0; i < micp->aicsClientCnt; i++) {
            if (micp->pAicsClient[i] == client) {
                micpInclIdx = i;
                break;
            }
        }
    }

    if (attHandle == client->audioInStateHdl) {
        blc_aicsc_inStateChangeEvt_t evt;
        evt.vcpInclIdx  = vcpInclIdx;
        evt.micpInclIdx = micpInclIdx;
        evt.gainSetting = client->audioInState.gainSetting;
        evt.mute        = client->audioInState.mute;
        evt.gainMode    = client->audioInState.gainMode;
        blt_prf_sendEvent(connHandle, AUDIO_EVT_AICSC_CHANGED_INPUT_STATE, (u8 *)&evt, sizeof(blc_aicsc_inStateChangeEvt_t));
        return ATT_SUCCESS;
    }

    return ATT_ERR_INVALID_HANDLE;
}

static int blt_aicsc_dataInput(blc_aics_client_t *client, u16 attHdl, u8 *val, u16 valLen)
{
    if (client->audioInStateHdl == attHdl) {
        if (valLen != sizeof(blc_aics_audio_input_state_t)) {
            BLT_AICS_LOG("NTF ERR: Audio Input State[0x%x] is %s", attHdl, hex_to_str(val, valLen));
            return ATT_SUCCESS;
        }
        memcpy(&client->audioInState, val, valLen);

        BLT_AICS_LOG("INFO: Audio Input State, handle[0x%x], gainSetting[%d], Mute[%d], GainMode[%d], changeCount[%d]", attHdl, client->audioInState.gainSetting, client->audioInState.mute, client->audioInState.gainMode, client->audioInState.changeCnt);
        return blt_aicsc_sendInfoEvt(client, attHdl);
    }
    return ATT_ERR_INVALID_HANDLE;
}

int blt_aicsc_vcpDataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);
    for (int i = 0; i < vcp->aicsClientCnt; i++) {
        blc_aics_client_t *client = vcp->pAicsClient[i];
        if (blt_aicsc_dataInput(client, attHdl, val, valLen) == ATT_SUCCESS) {
            return ATT_SUCCESS;
        }
    }
    return ATT_ERR_INVALID_HANDLE;
}

int blt_aicsc_micpDataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen)
{
    blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);
    for (int i = 0; i < micp->aicsClientCnt; i++) {
        blc_aics_client_t *client = micp->pAicsClient[i];
        if (blt_aicsc_dataInput(client, attHdl, val, valLen) == ATT_SUCCESS) {
            return ATT_SUCCESS;
        }
    }
    return ATT_ERR_INVALID_HANDLE;
}

///////////////////////AICS include micp volume controller////////////////////////////

void blt_aicsc_micpFoundServiceEnd(u16 connHandle)
{
    blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);
    BLT_AICS_LOG("sdp over connHandle[0x%x], found aics count is %d", connHandle, micp->aicsClientCnt);
    for (int i = 0; i < micp->aicsClientCnt; i++) {
        blc_aics_client_t *client = micp->pAicsClient[i];
        BLT_AICS_LOG("  index[%d], info is", i);
        BLT_AICS_LOG("  INFO: audio input state Handle[0x%x] gainSet[%d] mute[%d] gainMode[%d] changeCnt[%d]", client->audioInStateHdl, client->audioInState.gainSetting, client->audioInState.mute, client->audioInState.gainMode, client->audioInState.changeCnt);
        BLT_AICS_LOG("  INFO: gain setting properties Handle[0x%x] gainSettingUnits[%d] gainSettingMinimum[%d] gainSettingMaximum[%d]", client->gainSettingPropertiesHdl, client->gainSettingProperties.gainSettingUnits, client->gainSettingProperties.gainSettingMinimum, client->gainSettingProperties.gainSettingMaximum);

        BLT_AICS_LOG("  INFO: audio input type Handle[0x%x] Type[0x%x]", client->audioInTypeHdl, client->audioInType);
        BLT_AICS_LOG("  INFO: audio input status Handle[0x%x] status[0x%x]", client->audioInStatusHdl, client->audioInStatus);
        BLT_AICS_LOG("  INFO: audio input control point Handle[0x%x]", client->audioInCtrlHdl);
        BLT_AICS_LOG("  INFO: audio input description Handle[0x%x] flags[%s]", client->audioInDescHdl, client->audioInDesc);

        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
    }
}

static bool blt_aicsc_micpFoundService(u16 connHandle, u16 startHandle, u16 endHandle)
{
    blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);

    if (micp->aicsClientCnt >= STACK_AUDIO_MICS_CLIENT_INCLUDE_AICS_INSTANCE_NUM) {
        BLT_AICS_LOG("micp found service too many");
        return false;
    }

    blc_aics_client_t *aicsc = blt_aicsc_getClientControlBuffer(connHandle, startHandle, endHandle);

    if (!aicsc) {
        BLT_AICS_LOG("micp not found new aicsc client");
        return false;
    }

    micp->pAicsClient[micp->aicsClientCnt] = aicsc;
    micp->aicsClientCnt++;
    micp->aicsClientIdx++;
    aicsc->micpInclFlag = true;

    aicsc->ntfInput.startHdl     = startHandle;
    aicsc->ntfInput.endHdl       = endHandle;
    aicsc->ntfInput.ntfOrIndFunc = blt_micp_dataInput;
    BLT_AICS_LOG("micp found include service, connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x", connHandle, startHandle, endHandle);
    return true;
}

void blt_aicsc_micpLoad(u16 connHandle, prf_nv_param_t *param)
{
    blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);

    //  BLT_AICS_LOG("  INFO: sdp over connHandle[0x%x], micp found aics count is %d", connHandle, micp->aicsClientCnt);
    for (int i = 0; i < micp->aicsClientCnt; i++) {
        blc_aics_att_hdl_t *pAicsHdlInfo = (blc_aics_att_hdl_t *)param->dataPtr;

        u16                startHandle = pAicsHdlInfo->baseHandle;
        u16                endHandle   = AUD_PARAM_ATT_RESTORE(pAicsHdlInfo->endHdl, pAicsHdlInfo->baseHandle);
        blc_aics_client_t *aicsc       = blt_aicsc_getClientControlBuffer(connHandle, startHandle, endHandle);
        aicsc->micpInclFlag            = true;
        micp->pAicsClient[i]           = aicsc;

        blt_prf_loadClientHdl(aicsc, param->dataPtr, &aicsc->audioInDescHdl);
        param->dataPtr += sizeof(blc_aics_att_hdl_t);

        //      BLT_AICS_LOG("      INFO: index[%d], info is", i);
        //      BLT_AICS_LOG("      INFO: audioInStateHdl[0x%x]", aicsc->audioInStateHdl);
        //      BLT_AICS_LOG("      INFO: gainSettingPropertiesHdl[0x%x]", aicsc->gainSettingPropertiesHdl);
        //      BLT_AICS_LOG("      INFO: audioInTypeHdl[0x%x]", aicsc->audioInTypeHdl);
        //      BLT_AICS_LOG("      INFO: audioInStatusHdl[0x%x]", aicsc->audioInStatusHdl);
        //      BLT_AICS_LOG("      INFO: audioInCtrlHdl[0x%x]", aicsc->audioInCtrlHdl);
        //      BLT_AICS_LOG("      INFO: audioInDescHdl[0x%x]", aicsc->audioInDescHdl);
        aicsc->ntfInput.ntfOrIndFunc = blt_micp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &aicsc->ntfInput);
        //      BLT_AICS_LOG("      INFO: AICS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, aicsc->ntfInput.startHdl, aicsc->ntfInput.endHdl);
    }
}

void blt_aicsc_micpStore(u16 connHandle, prf_nv_param_t *param)
{
    blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);

    //  BLT_AICS_LOG("  INFO: sdp over connHandle[0x%x], micp found aics count is %d", connHandle, micp->aicsClientCnt);
    for (int i = 0; i < micp->aicsClientCnt; i++) {
        blc_aics_client_t *client = micp->pAicsClient[i];

        blt_prf_storeClientHdl(param->dataPtr, client, &client->audioInDescHdl);
        param->dataPtr += sizeof(blc_aics_att_hdl_t);

        //      BLT_AICS_LOG("      INFO: index[%d], info is", i);
        //      BLT_AICS_LOG("      INFO: audioInStateHdl[0x%x]", client->audioInStateHdl);
        //      BLT_AICS_LOG("      INFO: gainSettingPropertiesHdl[0x%x]", client->gainSettingPropertiesHdl);
        //      BLT_AICS_LOG("      INFO: audioInTypeHdl[0x%x]", client->audioInTypeHdl);
        //      BLT_AICS_LOG("      INFO: audioInStatusHdl[0x%x]", client->audioInStatusHdl);
        //      BLT_AICS_LOG("      INFO: audioInCtrlHdl[0x%x]", client->audioInCtrlHdl);
        //      BLT_AICS_LOG("      INFO: audioInDescHdl[0x%x]", client->audioInDescHdl);
        client->ntfInput.ntfOrIndFunc = blt_micp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        //      BLT_AICS_LOG("      INFO: AICS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
    }
}

///////////////////////AICS include VCP volume controller////////////////////////////

void blt_aicsc_vcpFoundServiceEnd(u16 connHandle)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);
    BLT_AICS_LOG(" sdp over connHandle[0x%x], found aics count is %d", connHandle, vcp->aicsClientCnt);
    for (int i = 0; i < vcp->aicsClientCnt; i++) {
        blc_aics_client_t *client = vcp->pAicsClient[i];
        BLT_AICS_LOG("  index[%d], info is", i);
        BLT_AICS_LOG("  INFO: audio input state Handle[0x%x] gainSet[%d] mute[%d] gainMode[%d] changeCnt[%d]", client->audioInStateHdl, client->audioInState.gainSetting, client->audioInState.mute, client->audioInState.gainMode, client->audioInState.changeCnt);
        BLT_AICS_LOG("  INFO: gain setting properties Handle[0x%x] gainSettingUnits[%d] gainSettingMinimum[%d] gainSettingMaximum[%d]", client->gainSettingPropertiesHdl, client->gainSettingProperties.gainSettingUnits, client->gainSettingProperties.gainSettingMinimum, client->gainSettingProperties.gainSettingMaximum);

        BLT_AICS_LOG("  INFO: audio input type Handle[0x%x] Type[0x%x]", client->audioInTypeHdl, client->audioInType);
        BLT_AICS_LOG("  INFO: audio input status Handle[0x%x] Type[0x%x]", client->audioInStatusHdl, client->audioInStatus);
        BLT_AICS_LOG("  INFO: audio input control point Handle[0x%x]", client->audioInCtrlHdl);
        BLT_AICS_LOG("  INFO: audio input description Handle[0x%x] flags[%s]", client->audioInDescHdl, client->audioInDesc);

        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
    }
}

static bool blt_aicsc_vcpFoundService(u16 connHandle, u16 startHandle, u16 endHandle)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);

    if (vcp->aicsClientCnt >= STACK_AUDIO_VCS_CLIENT_INCLUDE_AICS_INSTANCE_NUM) {
        BLT_AICS_LOG("VCP found service too many");
        return false;
    }

    blc_aics_client_t *aicsc = blt_aicsc_getClientControlBuffer(connHandle, startHandle, endHandle);

    if (!aicsc) {
        BLT_AICS_LOG("VCP not found new aicsc client");
        return false;
    }

    vcp->pAicsClient[vcp->aicsClientCnt] = aicsc;
    vcp->aicsClientCnt++;
    vcp->aicsClientIdx++;

    aicsc->vcpInclFlag = true;

    aicsc->ntfInput.startHdl     = startHandle;
    aicsc->ntfInput.endHdl       = endHandle;
    aicsc->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
    BLT_AICS_LOG("VCP found include service, connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x", connHandle, startHandle, endHandle);
    return true;
}

void blt_aicsc_vcpLoad(u16 connHandle, prf_nv_param_t *param)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);

    //  BLT_AICS_LOG("  INFO: sdp over connHandle[0x%x], micp found aics count is %d", connHandle, micp->aicsClientCnt);
    for (int i = 0; i < vcp->aicsClientCnt; i++) {
        blc_aics_att_hdl_t *pAicsHdlInfo = (blc_aics_att_hdl_t *)param->dataPtr;

        u16                startHandle = pAicsHdlInfo->baseHandle;
        u16                endHandle   = AUD_PARAM_ATT_RESTORE(pAicsHdlInfo->endHdl, pAicsHdlInfo->baseHandle);
        blc_aics_client_t *aicsc       = blt_aicsc_getClientControlBuffer(connHandle, startHandle, endHandle);
        aicsc->vcpInclFlag             = true;
        vcp->pAicsClient[i]            = aicsc;

        blt_prf_loadClientHdl(aicsc, pAicsHdlInfo, &aicsc->audioInDescHdl);
        param->dataPtr += sizeof(blc_aics_att_hdl_t);

        //      BLT_AICS_LOG("      INFO: index[%d], info is", i);
        //      BLT_AICS_LOG("      INFO: audioInStateHdl[0x%x]", aicsc->audioInStateHdl);
        //      BLT_AICS_LOG("      INFO: gainSettingPropertiesHdl[0x%x]", aicsc->gainSettingPropertiesHdl);
        //      BLT_AICS_LOG("      INFO: audioInTypeHdl[0x%x]", aicsc->audioInTypeHdl);
        //      BLT_AICS_LOG("      INFO: audioInStatusHdl[0x%x]", aicsc->audioInStatusHdl);
        //      BLT_AICS_LOG("      INFO: audioInCtrlHdl[0x%x]", aicsc->audioInCtrlHdl);
        //      BLT_AICS_LOG("      INFO: audioInDescHdl[0x%x]", aicsc->audioInDescHdl);
        aicsc->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &aicsc->ntfInput);
        //      BLT_AICS_LOG("      INFO: AICS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, aicsc->ntfInput.startHdl, aicsc->ntfInput.endHdl);
    }
}

void blt_aicsc_vcpStore(u16 connHandle, prf_nv_param_t *param)
{
    blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);

    //  BLT_AICS_LOG("  INFO: sdp over connHandle[0x%x], vcp found aics count is %d", connHandle, vcp->aicsClientCnt);
    for (int i = 0; i < vcp->aicsClientCnt; i++) {
        blc_aics_client_t *client = vcp->pAicsClient[i];

        blt_prf_storeClientHdl(param->dataPtr, client, &client->audioInDescHdl);
        param->dataPtr += sizeof(blc_aics_att_hdl_t);

        //      BLT_AICS_LOG("      INFO: index[%d], info is", i);
        //      BLT_AICS_LOG("      INFO: audioInStateHdl[0x%x]", client->audioInStateHdl);
        //      BLT_AICS_LOG("      INFO: gainSettingPropertiesHdl[0x%x]", client->gainSettingPropertiesHdl);
        //      BLT_AICS_LOG("      INFO: audioInTypeHdl[0x%x]", client->audioInTypeHdl);
        //      BLT_AICS_LOG("      INFO: audioInStatusHdl[0x%x]", client->audioInStatusHdl);
        //      BLT_AICS_LOG("      INFO: audioInCtrlHdl[0x%x]", client->audioInCtrlHdl);
        //      BLT_AICS_LOG("      INFO: audioInDescHdl[0x%x]", client->audioInDescHdl);
        client->ntfInput.ntfOrIndFunc = blt_vcp_dataInput;
        blc_gattc_addSubscribeCCCNode(connHandle, &client->ntfInput);
        //      BLT_AICS_LOG("      INFO: AICS connHandle: 0x%x startHandle: 0x%x EndHandle:0x%x ", connHandle, client->ntfInput.startHdl, client->ntfInput.endHdl);
    }
}

static bool blt_aicsc_foundService(u16 connHandle, u16 startHandle, u16 endHandle)
{
    const uuid_t *uuid = blc_gapc_getDiscoveryServiceUUID(connHandle);

    uuid_t vcpUuid  = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL);
    uuid_t micpUuid = UUID16_INIT(SERVICE_UUID_MICROPHONE_CONTROL);
    if (blc_uuid_cmp(uuid, (const uuid_t *)&vcpUuid) == 0) {
        return blt_aicsc_vcpFoundService(connHandle, startHandle, endHandle);
    } else if (blc_uuid_cmp(uuid, (const uuid_t *)&micpUuid) == 0) {
        return blt_aicsc_micpFoundService(connHandle, startHandle, endHandle);
    }
    return false;
}

static blc_aics_client_t *blt_aicsc_getSdpInst(u16 connHandle)
{
    const uuid_t *uuid = blc_gapc_getDiscoveryServiceUUID(connHandle);

    uuid_t vcpUuid  = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL);
    uuid_t micpUuid = UUID16_INIT(SERVICE_UUID_MICROPHONE_CONTROL);
    if (blc_uuid_cmp(uuid, (const uuid_t *)&vcpUuid) == 0) {
        blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);
        return vcp->pAicsClient[vcp->aicsClientIdx - 1];
    } else if (blc_uuid_cmp(uuid, (const uuid_t *)&micpUuid) == 0) {
        blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);
        return micp->pAicsClient[micp->aicsClientIdx - 1];
    }
    return NULL;
}

static blc_aics_client_t *blt_aicsc_getInstByIdx(u16 connHandle, int serviceUUID, int index)
{
    if (serviceUUID == SERVICE_UUID_VOLUME_CONTROL) {
        blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);
        return index < vcp->aicsClientCnt ? vcp->pAicsClient[index] : NULL;
    } else if (serviceUUID == SERVICE_UUID_MICROPHONE_CONTROL) {
        blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);
        return index < micp->aicsClientCnt ? micp->pAicsClient[index] : NULL;
    }

    return NULL;
}

static void blt_aicsc_foundAudioInputStateChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    client->audioInStateHdl = valueHandle;

    BLT_AICS_LOG("VCP audio input state ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_aicsc_audioInputStateStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    *read        = (u8 *)&client->audioInState;
    *readLen     = 0;
    *readMaxSize = sizeof(blc_aics_audio_input_state_t);
    *rdCbFunc    = NULL;
    BLT_AICS_LOG("VCP will read audio input state handle[0x%x] value", attrHandle);
}

static void blt_aicsc_foundGainSettingPropertiesChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    client->gainSettingPropertiesHdl = valueHandle;

    BLT_AICS_LOG("VCP gain Setting Properties ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_aicsc_gainSettingPropertiesStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    *read        = (u8 *)&client->gainSettingProperties;
    *readLen     = 0;
    *readMaxSize = sizeof(blc_aics_gain_setting_properties_t);
    *rdCbFunc    = NULL;
    BLT_AICS_LOG("VCP will read gain Setting Properties handle value");
}

static void blt_aicsc_foundAudioInTypeChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    client->audioInTypeHdl = valueHandle;

    BLT_AICS_LOG("VCP audio Input Type ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_aicsc_audioInTypeStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    *read        = (u8 *)&client->audioInType;
    *readLen     = 0;
    *readMaxSize = 1;
    *rdCbFunc    = NULL;
    BLT_AICS_LOG("VCP will read audio Input Type handle value");
}

static void blt_aicsc_foundAudioInStatusChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    client->audioInStatusHdl = valueHandle;

    BLT_AICS_LOG("VCP audio Input Status ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_aicsc_audioInStatusStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    *read        = (u8 *)&client->audioInStatus;
    *readLen     = 0;
    *readMaxSize = 1;
    *rdCbFunc    = NULL;
    BLT_AICS_LOG("VCP will read audio Input Status handle value");
}

static void blt_aicsc_foundAudioInCtrlChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    client->audioInCtrlHdl = valueHandle;

    BLT_AICS_LOG("VCP audio Input control point ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_aicsc_foundAudioInDescChar(u16 connHandle, u8 serviceCount, u8 properties, u16 valueHandle)
{
    (void)serviceCount;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    client->audioInDescHdl = valueHandle;

    BLT_AICS_LOG("VCP audio Input Description ConnHandle[0x%x] properties[0x%x] handle[0x%x]", connHandle, properties, valueHandle);
}

static void blt_aicsc_audioInDescStartRead(u16 connHandle, u16 attrHandle, u8 **read, u16 **readLen, u16 *readMaxSize, gapc_read_func_t *rdCbFunc)
{
    (void)attrHandle;
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    *read        = (u8 *)&client->audioInDesc[0];
    *readLen     = &client->audioInDescLen;
    *readMaxSize = AICS_READ_INPUT_DESC_MAX_SIZE;
    *rdCbFunc    = NULL;
    BLT_AICS_LOG("VCP will read audio Input Description handle value");
}

static blc_gapc_discChar_t aicsChar[] = {
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_AUDIO_INPUT_STATE),
     .cfun         = blt_aicsc_foundAudioInputStateChar,
     .rfun         = blt_aicsc_audioInputStateStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_GAIN_SETTINGS_ATTRIBUTE),
     .cfun      = blt_aicsc_foundGainSettingPropertiesChar,
     .rfun      = blt_aicsc_gainSettingPropertiesStartRead,
     },
    {
     .readValue = true,
     .uuid      = UUID16_INIT(CHARACTERISTIC_UUID_AUDIO_INPUT_TYPE),
     .cfun      = blt_aicsc_foundAudioInTypeChar,
     .rfun      = blt_aicsc_audioInTypeStartRead,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_AICS_INPUT_STATUS),
     .cfun         = blt_aicsc_foundAudioInStatusChar,
     .rfun         = blt_aicsc_audioInStatusStartRead,
     },
    {
     .setting = 0,
     .uuid    = UUID16_INIT(CHARACTERISTIC_UUID_AUDIO_INPUT_CONTROL_POINT),
     .cfun    = blt_aicsc_foundAudioInCtrlChar,
     },
    {
     .subscribeNtf = true,
     .readValue    = true,
     .uuid         = UUID16_INIT(CHARACTERISTIC_UUID_AUDIO_INPUT_DESCRIPTION),
     .cfun         = blt_aicsc_foundAudioInDescChar,
     .rfun         = blt_aicsc_audioInDescStartRead,
     },
};

const blc_gapc_discInclude_t discAics = {
    .uuid           = UUID16_INIT(SERVICE_UUID_AUDIO_INPUT_CONTROL),
    .characteristic = {
                       .size           = ARRAY_SIZE(aicsChar),
                       .characteristic = aicsChar,
                       },
    .ifun = blt_aicsc_foundService,
};

/**********reconnect function start*********/

static bool blt_aicsc_reconnService(u16 connHandle, int count)
{
    const uuid_t *uuid = blc_gapc_getDiscoveryServiceUUID(connHandle);

    uuid_t vcpUuid  = UUID16_INIT(SERVICE_UUID_VOLUME_CONTROL);
    uuid_t micpUuid = UUID16_INIT(SERVICE_UUID_MICROPHONE_CONTROL);
    if (blc_uuid_cmp(uuid, (const uuid_t *)&vcpUuid) == 0) {
        blc_vcp_client_t *vcp = blt_vcp_getClientInst(connHandle);
        vcp->aicsClientIdx++;
        return count <= vcp->aicsClientCnt;
    } else if (blc_uuid_cmp(uuid, (const uuid_t *)&micpUuid) == 0) {
        blc_micp_client_t *micp = blt_micp_getClientInst(connHandle);
        micp->aicsClientIdx++;
        return count <= micp->aicsClientCnt;
    }
    return false;
}

static int blt_aicsc_audioInputStateGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->audioInStateHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_aicsc_gainSettingPropertiesGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->gainSettingPropertiesHdl;

    return 1;
}

static int blt_aicsc_audioInTypeGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ;
    charInfo->valueHandle = client->audioInTypeHdl;

    return 1;
}

static int blt_aicsc_audioInStatusGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->audioInStatusHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static int blt_aicsc_audioInDescGetInfo(u16 connHandle, blc_gapc_charInfo_t *charInfo)
{
    blc_aics_client_t *client = blt_aicsc_getSdpInst(connHandle);

    charInfo->properties  = CHAR_PROP_READ | CHAR_PROP_NOTIFY;
    charInfo->valueHandle = client->audioInDescHdl;
    charInfo->cccHandle   = 0;

    return 1;
}

static const blc_gapc_reconnChar_t reAicsChar[] = {

    {
     .ifun = blt_aicsc_audioInputStateGetInfo,
     .rfun = blt_aicsc_audioInputStateStartRead,
     },

    {
     .ifun = blt_aicsc_gainSettingPropertiesGetInfo,
     .rfun = blt_aicsc_gainSettingPropertiesStartRead,
     },

    {
     .ifun = blt_aicsc_audioInTypeGetInfo,
     .rfun = blt_aicsc_audioInTypeStartRead,
     },

    {
     .ifun = blt_aicsc_audioInStatusGetInfo,
     .rfun = blt_aicsc_audioInStatusStartRead,
     },

    {
     .ifun = blt_aicsc_audioInDescGetInfo,
     .rfun = blt_aicsc_audioInDescStartRead,
     },
};

const blc_gapc_reconnInclTable_t reconnAics = {
    .reifun = blt_aicsc_reconnService,
    .charTb = {
               .size           = ARRAY_SIZE(reAicsChar),
               .characteristic = reAicsChar,
               },
};

/**********reconnect function ending********/


static void blt_aicsc_readAttributeValueCallback(u16 connHandle, u8 err, gattc_read_cfg_t *pRdCfg)
{
    blc_prf_readAttributeValueCallback(connHandle, err);
    if (err == ATT_SUCCESS) {
        for (int i = 0; i < gAppAicsCltInstNum; i++) {
            blc_aics_client_t *client = &gAicsClient[i];
            if (client->connHandle == connHandle) {
                if (blt_aicsc_sendInfoEvt(client, pRdCfg->single.handle) == ATT_SUCCESS) {
                    return;
                }
            }
        }
    }
}

static int blt_aicsc_readAttributeValue(u16 connHandle, blc_aics_client_t *aicsc, blt_aics_read_enum rdType, prf_read_cb_t readCb)
{
    BLT_AICS_LOG("Read Attribute Value:%d", rdType);
    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (rdType >= AICS_READ_MAX || !aicsc) {
        BLT_AICS_LOG("ERR: Invalid read type %d, 0x%x", rdType, aicsc);
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_read_cfg_t pGapReCfg;

    pGapReCfg.func = blt_aicsc_readAttributeValueCallback;
    if (rdType == AICS_READ_AUDIO_INPUT_STATE) {
        pGapReCfg.handle   = aicsc->audioInStateHdl;
        pGapReCfg.wBuff    = (u8 *)&aicsc->audioInState;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(blc_aics_audio_input_state_t);
    } else if (rdType == AICS_READ_GAIN_SETTING_PROPERTIES) {
        pGapReCfg.handle   = aicsc->gainSettingPropertiesHdl;
        pGapReCfg.wBuff    = (u8 *)&aicsc->gainSettingProperties;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = sizeof(blc_aics_gain_setting_properties_t);
    } else if (rdType == AICS_READ_AUDIO_INPUT_TYPE) {
        pGapReCfg.handle   = aicsc->audioInTypeHdl;
        pGapReCfg.wBuff    = (u8 *)&aicsc->audioInType;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 1;
    } else if (rdType == AICS_READ_AUDIO_INPUT_STATUS) {
        pGapReCfg.handle   = aicsc->audioInStatusHdl;
        pGapReCfg.wBuff    = (u8 *)&aicsc->audioInStatus;
        pGapReCfg.wBuffLen = NULL;
        pGapReCfg.maxLen   = 1;
    } else if (rdType == AICS_READ_AUDIO_INPUT_DESCRIPTION) {
        pGapReCfg.handle   = aicsc->audioInDescHdl;
        pGapReCfg.wBuff    = (u8 *)&aicsc->audioInDesc[0];
        pGapReCfg.wBuffLen = &aicsc->audioInDescLen;
        pGapReCfg.maxLen   = AICS_READ_INPUT_DESC_MAX_SIZE;
    }
    return blc_prf_readAttributeValue(connHandle, &pGapReCfg, readCb);
}

int blc_aiscc_readAudioInputState(u16 connHandle, blc_aics_client_t *aicsc, prf_read_cb_t readCb)
{
    return blt_aicsc_readAttributeValue(connHandle, aicsc, AICS_READ_AUDIO_INPUT_STATE, readCb);
}

int blc_aiscc_readGainSetProperties(u16 connHandle, blc_aics_client_t *aicsc, prf_read_cb_t readCb)
{
    return blt_aicsc_readAttributeValue(connHandle, aicsc, AICS_READ_GAIN_SETTING_PROPERTIES, readCb);
}

int blc_aiscc_readAudioInputType(u16 connHandle, blc_aics_client_t *aicsc, prf_read_cb_t readCb)
{
    return blt_aicsc_readAttributeValue(connHandle, aicsc, AICS_READ_AUDIO_INPUT_TYPE, readCb);
}

int blc_aiscc_readAudioInputStatus(u16 connHandle, blc_aics_client_t *aicsc, prf_read_cb_t readCb)
{
    return blt_aicsc_readAttributeValue(connHandle, aicsc, AICS_READ_AUDIO_INPUT_STATUS, readCb);
}

int blc_aiscc_readAudioInputDescription(u16 connHandle, blc_aics_client_t *aicsc, prf_read_cb_t readCb)
{
    return blt_aicsc_readAttributeValue(connHandle, aicsc, AICS_READ_AUDIO_INPUT_DESCRIPTION, readCb);
}

int blc_aiscc_readAudioInputStateByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputState(connHandle,
                                         blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_VOLUME_CONTROL, index),
                                         readCb);
}

int blc_aiscc_readAudioInputStateByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputState(connHandle,
                                         blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_MICROPHONE_CONTROL, index),
                                         readCb);
}

int blc_aiscc_readGainSetPropertiesByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readGainSetProperties(connHandle,
                                           blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_VOLUME_CONTROL, index),
                                           readCb);
}

int blc_aiscc_readGainSetPropertiesByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readGainSetProperties(connHandle,
                                           blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_MICROPHONE_CONTROL, index),
                                           readCb);
}

int blc_aiscc_readAudioInputTypeByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputType(connHandle,
                                        blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_VOLUME_CONTROL, index),
                                        readCb);
}

int blc_aiscc_readAudioInputTypeByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputType(connHandle,
                                        blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_MICROPHONE_CONTROL, index),
                                        readCb);
}

int blc_aiscc_readAudioInputStatusByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputStatus(connHandle,
                                          blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_VOLUME_CONTROL, index),
                                          readCb);
}

int blc_aiscc_readAudioInputStatusByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputStatus(connHandle,
                                          blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_MICROPHONE_CONTROL, index),
                                          readCb);
}

int blc_aiscc_readAudioInputDescriptionByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputDescription(connHandle,
                                               blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_VOLUME_CONTROL, index),
                                               readCb);
}

int blc_aiscc_readAudioInputDescriptionByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb)
{
    return blc_aiscc_readAudioInputDescription(connHandle,
                                               blt_aicsc_getInstByIdx(connHandle, SERVICE_UUID_MICROPHONE_CONTROL, index),
                                               readCb);
}

static void blt_aicsc_writeCtrlPntCb(u16 connHandle, u8 err, void *data)
{
    assert(blt_ll_isAclhdlInvalid(connHandle) == BLE_SUCCESS);

    if (err >= AICS_ERRCODE_INVALID_CHANGE_COUNTER && err <= AICS_ERRCODE_GAIN_MODE_CHANGE_NOT_ALLOWED) {
        if (err == AICS_ERRCODE_VALUE_OUT_OF_RANGE) {
            blc_aiscc_readGainSetProperties(connHandle, (blc_aics_client_t *)data, NULL);
        } else {
            blc_aiscc_readAudioInputState(connHandle, (blc_aics_client_t *)data, NULL);
        }
    }

    if (err) {
        BLT_AICS_LOG("WR_CB INFO: ERR: 0x%x", err);
    } else {
        BLT_AICS_LOG("WR_CB INFO: SUCC");
    }
}

int blc_aicsc_writeAudioInputControlPoint(u16 connHandle, blc_aics_client_t *aicsc, int opcode, s8 gainSetting, prf_write_cb_t writeCb)
{
    (void)writeCb;

    if (blt_ll_isAclhdlInvalid(connHandle) != BLE_SUCCESS) {
        return HCI_ERR_UNKNOWN_CONN_ID;
    } else if (opcode >= AICS_OPCODE_MAX || !aicsc) {
        BLT_AICS_LOG("ERR: Invalid read type %d, 0x%x", opcode, aicsc);
        return AUDIO_ERR_INVALID_PARAMETER;
    } else if (aicsc->audioInCtrlHdl == 0) {
        BLT_AICS_LOG("ERR: handle not set");
        return AUDIO_ERR_INVALID_PARAMETER;
    }

    gapc_write_cfg_t pGapWrCfg;

    blt_aics_set_gain_setting_op_t cmd = {
        .cp.opcode    = opcode,
        .cp.changeCnt = aicsc->audioInState.changeCnt,
        .gainSetting  = gainSetting,
    };

    pGapWrCfg.func       = blt_aicsc_writeCtrlPntCb;
    pGapWrCfg.handle     = aicsc->audioInCtrlHdl;
    pGapWrCfg.data       = &cmd;
    pGapWrCfg.length     = opcode == AICS_OPCODE_SET_GAIN_SETTING ? sizeof(blt_aics_set_gain_setting_op_t) : sizeof(blt_aics_input_cp_t);
    pGapWrCfg.withoutRsp = false;
    pGapWrCfg.cbData     = aicsc;
    return blc_gapc_writeAttributeValue(connHandle, &pGapWrCfg);
}

#define blt_aicsc_writeCtrlPoint(connHandle, serviceUuid, index, opcode, gainSetting) \
    blc_aicsc_writeAudioInputControlPoint(connHandle, blt_aicsc_getInstByIdx(connHandle, serviceUuid, index), opcode, gainSetting, NULL)

int blc_aicsc_writeSetGainSettingByVcpIndex(u16 connHandle, int index, s8 gainSetting)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_VOLUME_CONTROL,
        index,
        AICS_OPCODE_SET_GAIN_SETTING,
        gainSetting);
}

int blc_aicsc_writeSetGainSettingByMicpIndex(u16 connHandle, int index, s8 gainSetting)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_MICROPHONE_CONTROL,
        index,
        AICS_OPCODE_SET_GAIN_SETTING,
        gainSetting);
}

int blc_aicsc_writeUnmuteByVcpIndex(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_VOLUME_CONTROL,
        index,
        AICS_OPCODE_NOT_MUTED,
        0);
}

int blc_aicsc_writeUnmuteByMicpIndex(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_MICROPHONE_CONTROL,
        index,
        AICS_OPCODE_NOT_MUTED,
        0);
}

int blc_aicsc_vcpMute(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_VOLUME_CONTROL,
        index,
        AICS_OPCODE_MUTE,
        0);
}

int blc_aicsc_micpMute(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_MICROPHONE_CONTROL,
        index,
        AICS_OPCODE_MUTE,
        0);
}

int blc_aicsc_vcpSetManualGainMode(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_VOLUME_CONTROL,
        index,
        AICS_OPCODE_SET_MANUAL_GAIN_MODE,
        0);
}

int blc_aicsc_micpSetManualGainMode(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_MICROPHONE_CONTROL,
        index,
        AICS_OPCODE_SET_MANUAL_GAIN_MODE,
        0);
}

int blc_aicsc_vcpSetAutoGainMode(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_VOLUME_CONTROL,
        index,
        AICS_OPCODE_SET_AUTOMATIC_GAIN_MODE,
        0);
}

int blc_aicsc_micpSetAutoGainMode(u16 connHandle, int index)
{
    return blt_aicsc_writeCtrlPoint(
        connHandle,
        SERVICE_UUID_MICROPHONE_CONTROL,
        index,
        AICS_OPCODE_SET_AUTOMATIC_GAIN_MODE,
        0);
}
