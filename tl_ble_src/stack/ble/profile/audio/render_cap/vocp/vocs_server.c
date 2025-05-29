/********************************************************************************************************
 * @file    vocs_server.c
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
#include <stdarg.h>


#define blc_vocss_getVolOffsetState(server) (blc_vocs_volume_offset_state_t *)blc_gatts_getAttributeValueByHandle(0xFFFF, server->volumeOffsetStateHdl)
#define blc_vocss_getAudioLocation(server)  (u32 *)blc_gatts_getAttributeValueByHandle(0xFFFF, server->audioLocationHdl)

static void blt_vocss_setVolumeOffset(blc_vocs_server_t *vocs, s16 volume)
{
    blc_vocs_volume_offset_state_t *volumeOffset = blc_vocss_getVolOffsetState(vocs);
    if (!volumeOffset || volume < MIN_VOLUME_OFFSET || volume > MAX_VOLUME_OFFSET) {
        return;
    }

    volumeOffset->volOffset = volume;
}

static bool blt_vocss_setAudioLocation(blc_vocs_server_t *vocs, s16 location)
{
    u32 *pLocation = blc_vocss_getAudioLocation(vocs);
    if (!pLocation || BLC_AUDIO_CHANNEL_ALLOCATION_RFU(location)) {
        return false;
    }

    *pLocation = location;
    return true;
}

extern const u16 gVocsOutDescMaxSize;

static void blt_vocss_setAudioOutputDescription(blc_vocs_server_t *vocs, u16 descLen, void *desc)
{
    u16 len = min(gVocsOutDescMaxSize, descLen);

    u8  *outDesc    = NULL;
    u16 *outDescLen = NULL;
    if (blc_gatts_getAttributeInformationByHandle(0xFFFF, vocs->audioOutDescHdl, &outDesc, &outDescLen) != BLE_SUCCESS) {
        return;
    }

    *outDescLen = len;
    memcpy(outDesc, desc, len);
}

void blt_vocss_initParam(blc_vocs_server_t *vocs, void *param)
{
    blc_vocss_regParam_t *vocsParam = (blc_vocss_regParam_t *)param;
    blt_vocss_setVolumeOffset(vocs, vocsParam->volumeOffset);
    blt_vocss_setAudioLocation(vocs, vocsParam->location);
    blt_vocss_setAudioOutputDescription(vocs, strlen(vocsParam->desc), vocsParam->desc);

    BLT_VOCS_LOG("Handle information, volumeOffsetState:0x%x location:0x%x controlPoint:0x%x outDesc:0x%x", vocs->volumeOffsetStateHdl, vocs->audioLocationHdl, vocs->volumeOffsetCtrlPoint, vocs->audioOutDescHdl);
}

static bool blt_vocss_initCharStart(void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t *)input;
    if (vcp->vocsServerCnt >= gAppVcsSvrInclVocsInstNum) {
        return false;
    }

    vcp->vocsServer[vcp->vocsServerCnt] = blt_vocss_getServerBuf(vcp->vocsServerCnt);
    memset(vcp->vocsServer[vcp->vocsServerCnt], 0, sizeof(blc_vocs_server_t));

    vcp->vocsServerCnt++;
    return true;
}

static void blt_vocss_initStateChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t *)input;

    blc_vocs_server_t *server = vcp->vocsServer[vcp->vocsServerCnt - 1];
    if (p->num > 0) {
        BLT_VOCS_LOG("ERR: State char too many");
        return;
    }
    server->volumeOffsetStateHdl = p->charHandle;
}

static void blt_vocss_initLocationChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t *)input;

    blc_vocs_server_t *server = vcp->vocsServer[vcp->vocsServerCnt - 1];
    if (p->num > 0) {
        BLT_VOCS_LOG("ERR: Location char too many");
        return;
    }
    server->audioLocationHdl = p->charHandle;
}

static void blt_vocss_initControlChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t *)input;

    blc_vocs_server_t *server = vcp->vocsServer[vcp->vocsServerCnt - 1];
    if (p->num > 0) {
        BLT_VOCS_LOG("ERR: Control char too many");
        return;
    }
    server->volumeOffsetCtrlPoint = p->charHandle;
}

static void blt_vocss_initDescriptChar(atts_foundCharParam_t *p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t *)input;

    blc_vocs_server_t *server = vcp->vocsServer[vcp->vocsServerCnt - 1];
    if (p->num > 0) {
        BLT_VOCS_LOG("ERR: Description char too many");
        return;
    }
    server->audioOutDescHdl = p->charHandle;
}

static const atts_findCharList_t vocsChar[] = {
    {
     .charUuid    = characteristicVolumeOffsetStateUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vocss_initStateChar,
     },
    {
     .charUuid    = characteristicAudioLocationUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vocss_initLocationChar,
     },
    {
     .charUuid    = characteristicVolumeOffsetControlPointUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vocss_initControlChar,
     },
    {
     .charUuid    = characteristicAudioOutputDescriptionUuid,
     .charUuidLen = ATT_16_UUID_LEN,
     .foundCback  = blt_vocss_initDescriptChar,
     },
};

const atts_findInclList_t vocsService = {
    .inclUuidLen = ATT_16_UUID_LEN,
    .inclUuid    = serviceVolumeOffsetControlUuid,
    .charSize    = ARRAY_SIZE(vocsChar),
    .charList    = vocsChar,
    .foundCback  = blt_vocss_initCharStart,
};

static int blt_vocss_dealSetVolumeOffset(blc_vocs_server_t *vocs, blc_vocs_volume_offset_state_t *state, s16 operand)
{
    (void)vocs;
    (void)state;
    if (operand > 255 || operand < -255) {
        return VOCS_ERRCODE_VALUE_OUT_OF_RANGE;
    }
    state->volOffset = operand;
    return ATT_SUCCESS;
}

typedef int (*volOffsetCtrlCb_fun)(blc_vocs_server_t *vocs, blc_vocs_volume_offset_state_t *state, s16 operand);

typedef struct
{
    u8                  opcode;
    u8                  size;
    volOffsetCtrlCb_fun ctrlCb;
} blt_vocss_vol_offset_ctrl_cmds_t;

static const blt_vocss_vol_offset_ctrl_cmds_t vocssVolOffsetCtrl[] = {
    {VOCS_OPCODE_SET_VOLUME_OFFSET, 4, blt_vocss_dealSetVolumeOffset},
};

static int blt_vocss_writeVolOffsetCtrl(u16 connHandle, int index, blc_vocs_server_t *server, u8 *writeValue, u16 valueLen)
{
    if (valueLen <= 2) {
        return ATT_ERR_INVALID_PDU;
    }

    blc_vocs_volume_offset_state_t *offsetState = blc_vocss_getVolOffsetState(server);

    u8 opcode    = writeValue[0];
    u8 changeCnt = writeValue[1];

    if (offsetState->changeCnt != changeCnt) {
        BLT_VOCS_LOG("vocs write change counter error, write[0x%x] local[0x%x]", changeCnt, offsetState->changeCnt);
        return VOCS_ERRCODE_INVALID_CHANGE_COUNTER;
    }

    for (size_t i = 0; i < ARRAY_SIZE(vocssVolOffsetCtrl); i++) {
        if (opcode == vocssVolOffsetCtrl[i].opcode) {
            if (valueLen == vocssVolOffsetCtrl[i].size) {
                int err = vocssVolOffsetCtrl[i].ctrlCb(server, offsetState, *(s16 *)(writeValue + 2));
                if (err) {
                    return err;
                }
                offsetState->changeCnt++;
                blc_gatts_notifyAttr(connHandle, server->volumeOffsetStateHdl);
                blc_vocss_volumeOffsetStateChangeEvt_t evt = {
                    .vocsIndex    = index,
                    .volumeOffset = offsetState->volOffset};
                blt_prf_sendEvent(connHandle, AUDIO_EVT_VOCSS_CHANGED_VOLUME_OFFSET, (u8 *)&evt, sizeof(blc_vocss_volumeOffsetStateChangeEvt_t));

                return ATT_SUCCESS;
            } else {
                return ATT_ERR_INVALID_PDU;
            }
        }
    }
    return VOCS_ERRCODE_OPCODE_NOT_SUPPORTED;
}

static int blt_vocss_writeLocation(u16 connHandle, int index, blc_vocs_server_t *server, u8 *writeValue, u16 valueLen)
{
    if (valueLen != 4) {
        return ATT_ERR_INVALID_PDU;
    }

    u32 setLocation = *(u32 *)writeValue;

    if (!blt_vocss_setAudioLocation(server, setLocation)) {
        return ATT_ERR_INVALID_PDU;
    }
    blc_gatts_notifyAttr(connHandle, server->audioLocationHdl);

    blc_vocss_locationChangeEvt_t evt = {
        .vocsIndex = index,
        .location  = setLocation,
    };
    blt_prf_sendEvent(connHandle, AUDIO_EVT_VOCSS_CHANGED_LOCATION, (u8 *)&evt, sizeof(blc_vocss_locationChangeEvt_t));
    return ATT_SUCCESS;
}

static int blt_vocss_writeOutDesc(u16 connHandle, int index, blc_vocs_server_t *server, u8 *writeValue, u16 valueLen)
{
    blt_vocss_setAudioOutputDescription(server, valueLen, writeValue);
    blc_gatts_notifyAttr(connHandle, server->audioOutDescHdl);

    blc_vocss_outputDescChangeEvt_t evt = {
        .vocsIndex = index,
        .descLen   = min(gVocsOutDescMaxSize, valueLen),
        .desc      = writeValue,
    };

    blt_prf_sendEvent(connHandle, AUDIO_EVT_VOCSS_CHANGED_OUTPUT_DESCRIPTION, (u8 *)&evt, sizeof(blc_vocss_outputDescChangeEvt_t));
    return ATT_SUCCESS;
}

int blt_vocss_writeCback(u16 connHandle, u16 attrHandle, u8 *writeValue, u16 valueLen)
{
    blc_vcp_server_t *vcp = blt_vcp_getServerInst(connHandle);

    for (int i = 0; i < vcp->vocsServerCnt; i++) {
        blc_vocs_server_t *server = vcp->vocsServer[i];
        if (attrHandle == server->volumeOffsetCtrlPoint) {
            return blt_vocss_writeVolOffsetCtrl(connHandle, i, server, writeValue, valueLen);
        } else if (attrHandle == server->audioLocationHdl) {
            return blt_vocss_writeLocation(connHandle, i, server, writeValue, valueLen);
        } else if (attrHandle == server->audioOutDescHdl) {
            return blt_vocss_writeOutDesc(connHandle, i, server, writeValue, valueLen);
        }
    }

    return ATT_ERR_INVALID_HANDLE;
}
