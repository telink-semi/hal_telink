/********************************************************************************************************
 * @file    aics_server.c
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

#define blc_aicss_getAudioInputState(connHandle, server)        (blc_aics_audio_input_state_t*)blc_gatts_getAttributeValueByHandle(connHandle, server->inputStateHdl)
#define blc_aicss_getGainSettingProperties(connHandle, server)  (blc_aics_gain_setting_properties_t*)blc_gatts_getAttributeValueByHandle(connHandle, server->gainPropeHdl)
#define blc_aicss_getInputType(connHandle, server)              (u8*)blc_gatts_getAttributeValueByHandle(connHandle, server->inputTypeHdl)
#define blc_aicss_getInputStatus(connHandle, server)            (u8*)blc_gatts_getAttributeValueByHandle(connHandle, server->inputStatusHdl)


void blt_aicss_setGainSetting(blc_aics_server_t* aics, s8 gain, u8 mute, u8 mode)
{
    blc_aics_audio_input_state_t * inputState = blc_aicss_getAudioInputState(0xFFFF, aics);
    if(!inputState)
        return ;
    inputState->gainSetting = gain;
    if(mute <= AICS_MUTE_VALUE_DISABLED)    inputState->mute = mute;
    if(mode <= AICS_GAIN_MODE_VALUE_AUTOMATIC)  inputState->gainMode = mode;
}

void blt_aicss_setGainUnits(blc_aics_server_t* aics, u8 units, s8 minGain, s8 maxGain)
{
    //s8: -128~127
    blc_aics_gain_setting_properties_t* gainPrope = blc_aicss_getGainSettingProperties(0xFFFF, aics);
    if(!gainPrope || maxGain < minGain)
        return ;

    gainPrope->gainSettingUnits = units;
    gainPrope->gainSettingMinimum = minGain;
    gainPrope->gainSettingMaximum = maxGain;
}

void blt_aicss_setAudioInputType(blc_aics_server_t* aics, u8 type)
{
    u8* inputType = blc_aicss_getInputType(0xFFFF, aics);
    if(!inputType || type >= AICS_INPUT_TYPE_RFU)
        return ;
    *inputType = type;
}

void blt_aicss_setAudioInputStatus(blc_aics_server_t* aics, u8 status)
{
    u8* inputStatus = blc_aicss_getInputStatus(0xFFFF, aics);
    if(!inputStatus || status >= AICS_INPUT_STATUS_RFU)
        return ;
    *inputStatus = status;
}

void blt_aicss_setAudioInputDescription(blc_aics_server_t* aics, u16 descLen, void* desc)
{
    extern const u16 gAicsInDescMaxSize;
    u16 len = min(gAicsInDescMaxSize, descLen);

    u8* inDesc = NULL;
    u16* inDescLen = NULL;
    if(blc_gatts_getAttributeInformationByHandle(0xFFFF, aics->inputDescHdl, &inDesc, &inDescLen) != BLE_SUCCESS)
    {
        return ;
    }

    *inDescLen = len;
    memcpy(inDesc, desc, len);
}

void blt_aicss_initParam(blc_aics_server_t* aics, void* param)
{
    blc_aicss_regParam_t * aicsParam = (blc_aicss_regParam_t*)param;

    blt_aicss_setGainSetting(aics, aicsParam->gainSetting, aicsParam->mute, aicsParam->gainMode);
    blt_aicss_setGainUnits(aics, aicsParam->units, aicsParam->minGain, aicsParam->maxGain);
    blt_aicss_setAudioInputType(aics, aicsParam->inputType);
    blt_aicss_setAudioInputStatus(aics, aicsParam->inputStatus);
    blt_aicss_setAudioInputDescription(aics, strlen(aicsParam->desc), aicsParam->desc);

    BLT_AICS_LOG("Handle information, inputState:0x%x, GainSetting:0x%x, inputType:0x%x, inputStatus:0x%x inputCtrl:0x%x inputDesc:0x%x",
                    aics->inputStateHdl, aics->gainPropeHdl, aics->inputTypeHdl, aics->inputStatusHdl, aics->inputCtrlHdl, aics->inputDescHdl);
}


static bool blt_aicss_initCharStart(void* input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;
    if(vcp->aicsServerCnt >= gAppVcsSvrInstNum)
    {
        return false;
    }

    vcp->aicsServer[vcp->aicsServerCnt] = blt_aicss_getServerControlBuffer(vcp->aicsServerCnt);
    memset(vcp->aicsServer[vcp->aicsServerCnt], 0, sizeof(blc_aics_server_t));

    vcp->aicsServerCnt ++;
    return true;
}

static void blt_aicss_initInputStateChar(atts_foundCharParam_t * p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;

    blc_aics_server_t* server = vcp->aicsServer[vcp->aicsServerCnt-1];
    if(p->num > 0)
    {
        BLT_AICS_LOG("ERR: Audio Input State char too many");
        return ;
    }
    server->inputStateHdl = p->charHandle;
}

static void blt_aicss_initGainSettingChar(atts_foundCharParam_t * p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;

    blc_aics_server_t* server = vcp->aicsServer[vcp->aicsServerCnt-1];
    if(p->num > 0)
    {
        BLT_AICS_LOG("ERR: Gain Setting Properties char too many");
        return ;
    }
    server->gainPropeHdl = p->charHandle;
}

static void blt_aicss_initInputTypeChar(atts_foundCharParam_t * p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;

    blc_aics_server_t* server = vcp->aicsServer[vcp->aicsServerCnt-1];
    if(p->num > 0)
    {
        BLT_AICS_LOG("ERR: Audio Input Type char too many");
        return ;
    }
    server->inputTypeHdl = p->charHandle;
}

static void blt_aicss_initInputStatusChar(atts_foundCharParam_t * p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;

    blc_aics_server_t* server = vcp->aicsServer[vcp->aicsServerCnt-1];
    if(p->num > 0)
    {
        BLT_AICS_LOG("ERR: Audio Input Status char too many");
        return ;
    }
    server->inputStatusHdl = p->charHandle;
}

static void blt_aicss_initInputControlChar(atts_foundCharParam_t * p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;

    blc_aics_server_t* server = vcp->aicsServer[vcp->aicsServerCnt-1];
    if(p->num > 0)
    {
        BLT_AICS_LOG("ERR: Audio Input Control Point char too many");
        return ;
    }
    server->inputCtrlHdl = p->charHandle;
}

static void blt_aicss_initInputDescChar(atts_foundCharParam_t * p, void *input)
{
    blc_vcp_server_t *vcp = (blc_vcp_server_t*)input;

    blc_aics_server_t* server = vcp->aicsServer[vcp->aicsServerCnt-1];

    if(p->num > 0)
    {
        BLT_AICS_LOG("ERR: Audio Input Description char too many");
        return ;
    }
    server->inputDescHdl = p->charHandle;
}

static const atts_findCharList_t aicsChar[] = {
    {
        .charUuid = characteristicAudioInputStateUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_aicss_initInputStateChar,
    },
    {
        .charUuid = characteristicGainSettingsAttributeUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_aicss_initGainSettingChar,
    },
    {
        .charUuid = characteristicAudioInputTypeUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_aicss_initInputTypeChar,
    },
    {
        .charUuid = characteristicAudioInputStatusUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_aicss_initInputStatusChar,
    },
    {
        .charUuid = characteristicAudioInputControlPointUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_aicss_initInputControlChar,
    },
    {
        .charUuid = characteristicAudioInputDescriptionUuid,
        .charUuidLen = ATT_16_UUID_LEN,
        .foundCback = blt_aicss_initInputDescChar,
    },
};

const atts_findInclList_t aicsService = {
    .inclUuidLen = ATT_16_UUID_LEN,
    .inclUuid = serviceAudioInputControlUuid,
    .charSize = ARRAY_SIZE(aicsChar),
    .charList = aicsChar,
    .foundCback = blt_aicss_initCharStart,
};


typedef int (*aicsCtrlCb_fun)(blc_aics_server_t *server, s8 operand);

typedef struct{
    u8 opcode;
    u8 size;
    aicsCtrlCb_fun ctrlCb;
}blt_aicss_input_ctrl_cmds_t;

static int blt_aicss_dealSetGainSetting(blc_aics_server_t *server, s8 operand)
{
    blc_aics_gain_setting_properties_t* gainPrope = blc_aicss_getGainSettingProperties(0xFFFF, server);

    if(operand < gainPrope->gainSettingMinimum || operand > gainPrope->gainSettingMaximum)
    {
        return AICS_ERRCODE_VALUE_OUT_OF_RANGE;
    }
    blc_aics_audio_input_state_t* inputState = blc_aicss_getAudioInputState(0xFFFF, server);

    inputState->gainSetting = operand;
    return ATT_SUCCESS;
}

static int blt_aicss_dealNotMuted(blc_aics_server_t *server, s8 operand)
{
    (void)operand;
    blc_aics_audio_input_state_t* inputState = blc_aicss_getAudioInputState(0xFFFF, server);

    if(inputState->mute == AICS_MUTE_VALUE_DISABLED)
    {
        return AICS_ERRCODE_MUTE_DISABLE;
    }

    inputState->mute = AICS_MUTE_VALUE_NOT_MUTED;
    return ATT_SUCCESS;
}

static int blt_aicss_dealMute(blc_aics_server_t *server, s8 operand)
{
    (void)operand;
    blc_aics_audio_input_state_t* inputState = blc_aicss_getAudioInputState(0xFFFF, server);

    if(inputState->mute == AICS_MUTE_VALUE_DISABLED)
    {
        return AICS_ERRCODE_MUTE_DISABLE;
    }

    inputState->mute = AICS_MUTE_VALUE_MUTED;
    return ATT_SUCCESS;
}

static int blt_aicss_dealSetManualGainMode(blc_aics_server_t *server, s8 operand)
{
    (void)operand;
    blc_aics_audio_input_state_t* inputState = blc_aicss_getAudioInputState(0xFFFF, server);

    if(inputState->gainMode == AICS_GAIN_MODE_VALUE_MANUAL_ONLY || inputState->gainMode == AICS_GAIN_MODE_VALUE_AUTOMATIC_ONLY)
    {
        return AICS_ERRCODE_GAIN_MODE_CHANGE_NOT_ALLOWED;
    }

    inputState->gainMode = AICS_GAIN_MODE_VALUE_MANUAL;
    return ATT_SUCCESS;
}

static int blt_aicss_dealSetAutomaticGainMode(blc_aics_server_t *server, s8 operand)
{
    (void)operand;
    blc_aics_audio_input_state_t* inputState = blc_aicss_getAudioInputState(0xFFFF, server);

    if(inputState->gainMode == AICS_GAIN_MODE_VALUE_MANUAL_ONLY || inputState->gainMode == AICS_GAIN_MODE_VALUE_AUTOMATIC_ONLY)
    {
        return AICS_ERRCODE_GAIN_MODE_CHANGE_NOT_ALLOWED;
    }

    inputState->gainMode = AICS_GAIN_MODE_VALUE_AUTOMATIC;
    return ATT_SUCCESS;
}

static const blt_aicss_input_ctrl_cmds_t aicsInputCtrl[] = {
    {AICS_OPCODE_SET_GAIN_SETTING,          3, blt_aicss_dealSetGainSetting},
    {AICS_OPCODE_NOT_MUTED,                 2, blt_aicss_dealNotMuted},
    {AICS_OPCODE_MUTE,                      2, blt_aicss_dealMute},
    {AICS_OPCODE_SET_MANUAL_GAIN_MODE,      2, blt_aicss_dealSetManualGainMode},
    {AICS_OPCODE_SET_AUTOMATIC_GAIN_MODE,   2, blt_aicss_dealSetAutomaticGainMode},
};

static int blt_aicss_writeAudioInputCtrl(u16 connHandle, blc_aics_server_t* server, u8* writeValue, u16 valueLen)
{
    if(valueLen != 2 && valueLen != 3)
    {
        return ATT_ERR_INVALID_PDU;
    }

    blc_aics_audio_input_state_t * inputState = blc_aicss_getAudioInputState(connHandle, server);

    u8 opcode = writeValue[0];
    u8 changeCnt = writeValue[1];

    if(inputState->changeCnt != changeCnt)
    {
        BLT_AICS_LOG("aisc write change counter error, write[0x%x] local[0x%x]", changeCnt, inputState->changeCnt);
        return AICS_ERRCODE_INVALID_CHANGE_COUNTER;
    }

    for(size_t i=0; i<ARRAY_SIZE(aicsInputCtrl); i++)
    {
        if(opcode == aicsInputCtrl[i].opcode)
        {
            if(valueLen == aicsInputCtrl[i].size)
            {
                int err = aicsInputCtrl[i].ctrlCb(server, (s8)writeValue[2]);
                if(!err)
                {
                    inputState->changeCnt ++;
                    blc_gatts_notifyAttr(connHandle, server->inputStateHdl);
                }
                return err;
            }
            else
            {
                return ATT_ERR_INVALID_PDU;
            }
        }
    }
    return AICS_ERRCODE_OPCODE_NOT_SUPPORTED;
}


static int blt_aicss_writeAudioInputDesc(u16 connHandle, blc_aics_server_t* server, u8* writeValue, u16 valueLen)
{
    blt_aicss_setAudioInputDescription(server, valueLen, writeValue);
    blc_gatts_notifyAttr(connHandle, server->inputDescHdl);

    return ATT_SUCCESS;
}

int blt_aicss_writeCback(u16 connHandle, u16 attrHandle, u8* writeValue, u16 valueLen)
{
    blc_vcp_server_t* vcp = blt_vcp_getServerInst(connHandle);

    for(int i=0; i<vcp->aicsServerCnt; i++) {
        blc_aics_server_t* server = vcp->aicsServer[i];
        if(attrHandle == server->inputCtrlHdl){
            return blt_aicss_writeAudioInputCtrl(connHandle, server, writeValue, valueLen);
        }
        else if(attrHandle == server->inputDescHdl){
            return blt_aicss_writeAudioInputDesc(connHandle, server, writeValue, valueLen);
        }
    }

    return ATT_ERR_INVALID_HANDLE;
}






