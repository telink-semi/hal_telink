/********************************************************************************************************
 * @file    aics.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once


/******************************* AICS Common Start **********************************************************************/

/* Audio Input State characteristic value format */
typedef struct {
    s8 gainSetting;
    u8 mute;            //blc_aics_mute_value_enum
    u8 gainMode;
    u8 changeCnt;
} blc_aics_audio_input_state_t;

/* Gain Setting Properties characteristic value format */
typedef struct {
    u8 gainSettingUnits;
    s8 gainSettingMinimum;
    s8 gainSettingMaximum;
} blc_aics_gain_setting_properties_t;

typedef enum{
    AICS_MUTE_VALUE_NOT_MUTED = 0x00,
    AICS_MUTE_VALUE_MUTED,
    AICS_MUTE_VALUE_DISABLED,
    AICS_MUTE_VALUE_RFU,
} blc_aics_mute_value_enum;

typedef enum{
    AICS_GAIN_MODE_VALUE_MANUAL_ONLY    = 0x00,
    AICS_GAIN_MODE_VALUE_AUTOMATIC_ONLY,
    AICS_GAIN_MODE_VALUE_MANUAL,
    AICS_GAIN_MODE_VALUE_AUTOMATIC,
} blc_aics_gain_mode_value_enum;

typedef enum{
    AICS_INPUT_TYPE_UNSPECIFIED = 0x00, //Unspecified Input
    AICS_INPUT_TYPE_BLUETOOTH,          //Bluetooth Audio Stream
    AICS_INPUT_TYPE_MICROPHONE,         //Microphone
    AICS_INPUT_TYPE_ANALOG,             //Analog Interface
    AICS_INPUT_TYPE_DIGITAL,            //Digital Interface
    AICS_INPUT_TYPE_RADIO,              //AM/FM/XM/etc
    AICS_INPUT_TYPE_STREAMING,          //Streaming Audio Source
    AICS_INPUT_TYPE_RFU,
} blc_aics_audio_input_type_def_enum;

typedef enum{
    AICS_INPUT_STATUS_INACTIVE = 0x00,
    AICS_INPUT_STATUS_ACTIVE,
    AICS_INPUT_STATUS_RFU,
} blc_aics_audio_input_status_enum;

#include "aics_client_buf.h"
#include "aics_server_buf.h"

/******************************* AICS Common End **********************************************************************/




/******************************* AICS Client Start **********************************************************************/

//AICS Client Event ID
typedef enum{
    AUDIO_EVT_AICSC_START = AUDIO_EVT_TYPE_AICSC,
    AUDIO_EVT_AICSC_CHANGED_INPUT_STATE,    //refer to 'blc_aicsc_inStateChangeEvt_t'
} audio_aicsc_evt_enum;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_AICSC_CHANGED_INPUT_STATE"
 */
typedef struct{
    u8 vcpInclIdx;
    u8 micpInclIdx;
    s8 gainSetting;
    u8 mute; //blc_aics_mute_value_enum
    u8 gainMode; //blc_aics_gain_mode_value_enum
} blc_aicsc_inStateChangeEvt_t;


//AICS Client Read Characteristic Value Operation API
int blc_aiscc_readAudioInputState(u16 connHandle, blc_aics_client_t* aicsc, prf_read_cb_t readCb);
int blc_aiscc_readGainSetProperties(u16 connHandle, blc_aics_client_t* aicsc, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputType(u16 connHandle, blc_aics_client_t* aicsc, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputStatus(u16 connHandle, blc_aics_client_t* aicsc, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputDescription(u16 connHandle, blc_aics_client_t* aicsc, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputStateByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readGainSetPropertiesByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputTypeByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputStatusByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputDescriptionByMicpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputStateByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readGainSetPropertiesByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputTypeByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputStatusByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_aiscc_readAudioInputDescriptionByVcpIndex(u16 connHandle, int index, prf_read_cb_t readCb);

//AICS Client Get Characteristic Value Operation API
int blc_aiscc_getAudioInputState(u16 connHandle, blc_aics_client_t* aicsc, blc_aics_audio_input_state_t* inputState);
int blc_aiscc_getGainSetProperties(u16 connHandle, blc_aics_client_t* aicsc, blc_aics_gain_setting_properties_t* gainSetProp);
int blc_aiscc_getAudioInputType(u16 connHandle, blc_aics_client_t* aicsc, u8 type[1]);
int blc_aiscc_getAudioInputStatus(u16 connHandle, blc_aics_client_t* aicsc, u8 status[1]);
int blc_aiscc_getAudioInputDescription(u16 connHandle, blc_aics_client_t* aicsc, u8* desc, u16* descLen);
int blc_aiscc_getMicpAudioInputState(u16 connHandle, int index, blc_aics_audio_input_state_t* inputState);
int blc_aiscc_getMicpGainSetProperties(u16 connHandle, int index, blc_aics_gain_setting_properties_t* gainSetProp);
int blc_aiscc_getMicpAudioInputType(u16 connHandle, int index, u8 type[1]);
int blc_aiscc_getMicpAudioInputStatus(u16 connHandle, int index, u8 status[1]);
int blc_aiscc_getMicpAudioInputDescription(u16 connHandle, int index, u8* desc, u16* descLen);
int blc_aiscc_getVcpAudioInputState(u16 connHandle, int index, blc_aics_audio_input_state_t* inputState);
int blc_aiscc_getVcpGainSetProperties(u16 connHandle, int index, blc_aics_gain_setting_properties_t* gainSetProp);
int blc_aiscc_getVcpAudioInputType(u16 connHandle, int index, u8 type[1]);
int blc_aiscc_getVcpAudioInputStatus(u16 connHandle, int index, u8 status[1]);
int blc_aiscc_getVcpAudioInputDescription(u16 connHandle, int index, u8* desc, u16* descLen);

//AICS Client Write Characteristic Value Operation API
int blc_aicsc_writeAudioInputControlPoint(u16 connHandle, blc_aics_client_t* aicsc, int opcode, s8 gainSetting, prf_write_cb_t writeCb);
int blc_aicsc_writeSetGainSettingByVcpIndex(u16 connHandle, int index, s8 gainSetting);
int blc_aicsc_writeSetGainSettingByMicpIndex(u16 connHandle, int index, s8 gainSetting);
int blc_aicsc_writeUnmuteByVcpIndex(u16 connHandle, int index);
int blc_aicsc_writeUnmuteByMicpIndex(u16 connHandle, int index);
int blc_aicsc_vcpMute(u16 connHandle, int index);
int blc_aicsc_micpMute(u16 connHandle, int index);
int blc_aicsc_vcpSetManualGainMode(u16 connHandle, int index);
int blc_aicsc_micpSetManualGainMode(u16 connHandle, int index);
int blc_aicsc_vcpSetAutoGainMode(u16 connHandle, int index);
int blc_aicsc_micpSetAutoGainMode(u16 connHandle, int index);
int blc_aiscc_writeInputDescWithoutRsp(u16 connHandle, blc_aics_client_t* aicsc, u8* desc, u16 descLen);
int blc_aiscc_writeMicpInputDesc(u16 connHandle, int index, u8* desc, u16 descLen);
int blc_aiscc_writeVcpInputDesc(u16 connHandle, int index, u8* desc, u16 descLen);

/******************************* AICS Client End **********************************************************************/




/******************************* AICS Server Start **********************************************************************/

//AICS Server Event ID
typedef enum{
    AUDIO_EVT_AICSS_START = AUDIO_EVT_TYPE_AICSS,
    AUDIO_EVT_AICSS_CHANGED_INPUT_STATE,    //refer to 'blc_aicss_inStateChangeEvt_t'
} audio_aicss_evt_enum;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_AICSS_CHANGED_INPUT_STATE"
 */
typedef struct{
    u8 vcpInclIdx;
    s8 gainSetting;
    u8 mute; //blc_aics_mute_value_enum
    u8 gainMode; //blc_aics_gain_mode_value_enum
} blc_aicss_inStateChangeEvt_t;


//AICS Server Update Characteristic Value Operation API
int blc_aicss_updateInputState(u16 connHandle, blc_aics_server_t *aicss, blc_aics_audio_input_state_t* inputState);
int blc_aicss_updateInputStatus(u16 connHandle, blc_aics_server_t *aicss, u8 status);
int blc_aicss_updateInputDesc(u16 connHandle, blc_aics_server_t *aicss, u8* desc, u16 descLen);
int blc_aicss_updateMicpInputState(u16 connHandle, int index, blc_aics_audio_input_state_t* inputState);
int blc_aicss_updateMicpInputStatus(u16 connHandle, int index, u8 status);
int blc_aicss_updateMicpInputDesc(u16 connHandle, int index, u8* desc, u16 descLen);
int blc_aicss_updateVcpInputState(u16 connHandle, int index, blc_aics_audio_input_state_t* inputState);
int blc_aicss_updateVcpInputStatus(u16 connHandle, int index, u8 status);
int blc_aicss_updateVcpInputDesc(u16 connHandle, int index, u8* desc, u16 descLen);

/******************************* AICS Server End **********************************************************************/
