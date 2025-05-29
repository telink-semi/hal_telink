/********************************************************************************************************
 * @file    vcs.h
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

/******************************* VCS Common Start **********************************************************************/

/*  Volume State characteristic value format */
typedef struct
{
    u8 volSetting;
    u8 mute;
    u8 changeCnt;
} blc_vcs_volume_state_t;

typedef struct
{
    u8 volSettingPersisted : 1;
    u8 RFU                 : 7;
} blc_vcs_volume_flags_t;

#include "vcs_client_buf.h"
#include "vcs_server_buf.h"

#include "stack/ble/profile/audio/render_cap/aicp/aics.h"
#include "stack/ble/profile/audio/render_cap/vocp/vocs.h"

/* Volume Control Point  request opcodes */
typedef enum
{
    VCS_OPCODE_RELATIVE_VOLUME_DOWN        = 0x00,
    VCS_OPCODE_RELATIVE_VOLUME_UP          = 0x01,
    VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_DOWN = 0x02,
    VCS_OPCODE_UNMUTE_RELATIVE_VOLUME_UP   = 0x03,
    VCS_OPCODE_SET_ABSOLUTE_VOLUME         = 0x04,
    VCS_OPCODE_UNMUTE                      = 0x05,
    VCS_OPCODE_MUTE                        = 0x06,
    VCS_OPCODE_MAX,
    VCS_OPCODE_BQB_TEST = 0xEE,
} blt_vcs_opcode_enum;

typedef struct
{
    u8   volumeSetting;
    bool mute;
} blc_audio_volumeState;

/******************************* VCS Common End **********************************************************************/


/******************************* VCS Client Start **********************************************************************/

extern const int gAppVcsSvrInclAicsInstNum;
extern const int gAppVcsSvrInclVocsInstNum;

typedef struct
{
    s16 volumeOffset;
    u32 location;
    u16 outDescLen; //audio output description length
    u8 *outDesc;
} blc_audio_volumeOffsetState_t;

typedef struct
{
    s8  gainSetting;
    u8  mute;     //blc_aics_mute_value_enum
    u8  gainMode; //blc_aics_gain_mode_value_enum
    u8  gainSettingUnits;
    s8  minGainSetting;
    s8  maxGainSetting;
    u8  inputType; //blc_aics_audio_input_type_def_enum
    u8  inputStatus;
    u16 inDescLen; //Audio Input Description Length
    u8 *inDesc;
} blc_audio_inputState_t;

typedef struct
{
    blc_audio_volumeState         volState;
    u8                            vosCnt; //volume Offset State count;
    blc_audio_volumeOffsetState_t voc[STACK_AUDIO_VCS_CLIENT_INCLUDE_VOCS_INSTANCE_NUM];
    u8                            aisCnt; //audio input state count;
    blc_audio_inputState_t        ais[STACK_AUDIO_VCS_CLIENT_INCLUDE_AICS_INSTANCE_NUM];
} blc_audio_vcpState_t;

//VCS Client Event ID
typedef enum
{
    AUDIO_EVT_VCSC_START = AUDIO_EVT_TYPE_VCSC,
    AUDIO_EVT_VCSC_CHANGED_VOLUME_STATE, //refer to 'blc_vcsc_volumeStateChangeEvt_t'
} audio_vcsc_evt_enum;

typedef struct
{ // Event ID: AUDIO_EVT_VCSC_CHANGED_VOLUME_STATE
    u8   volumeSetting;
    bool mute;
} blc_vcsc_volumeStateChangeEvt_t;

/**
 * @brief       This function serves to register VCP volume controller
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerVCSControlClient(const blc_vcsc_regParam_t *param);


//VCS Client Read Characteristic Value Operation API
int blc_vcsc_readVolState(u16 connHandle, prf_read_cb_t readCb);
int blc_vcsc_readVolFlags(u16 connHandle, prf_read_cb_t readCb);

//VCS Client Get Characteristic Value Operation API
int blc_vcsc_getVolState(u16 connHandle, blc_vcs_volume_state_t *state);
int blc_vcsc_getVolFlags(u16 connHandle, blc_vcs_volume_flags_t *flags);
int blc_vcpc_getState(u16 connHandle, blc_audio_vcpState_t *vcpState);

//VCS Client Write Characteristic Value Operation API
int blc_vcsc_writeCtrlPoint(u16 connHandle, blt_vcs_opcode_enum opcode, u8 volumeSetting, prf_write_cb_t writeCb);
int blc_vcsc_writeRelativeVolDown(u16 connHandle);
int blc_vcsc_writeRelativeVolUp(u16 connHandle);
int blc_vcsc_writeUnmuteOrRelativeVolDown(u16 connHandle);
int blc_vcsc_writeUnmuteOrRelativeVolUp(u16 connHandle);
int blc_vcsc_writeSetAbsoluteVol(u16 connHandle, u8 volSetting);
int blc_vcsc_writeUnmute(u16 connHandle);
int blc_vcsc_writeMute(u16 connHandle);

/******************************* VCS Client End **********************************************************************/


/******************************* VCS Server Start **********************************************************************/

//Default VCP Volume Render parameters
extern const blc_vcss_regParam_t defaultVcpRendererParam;

//VCS Server Event ID
typedef enum
{
    AUDIO_EVT_VCSS_START = AUDIO_EVT_TYPE_VCSS,
    AUDIO_EVT_VCSS_CHANGED_VOLUME_STATE, //refer to 'blc_vcss_volumeStateChangeEvt_t'
} audio_vcss_evt_enum;

typedef struct
{ // Event ID: AUDIO_EVT_VCSS_CHANGED_VOLUME_STATE
    u8   volumeSetting;
    bool mute;
} blc_vcss_volumeStateChangeEvt_t;

/**
 * @brief       This function serves to register VCP volume renderer
 * @param[in]   refer to 'blc_vcss_regParam_t'
 * @return      none.
 */
void blc_audio_registerVCSControlServer(const blc_vcss_regParam_t *param);

/**
 * @brief       This function server to get volume state.
 * @param[in]   connHandle: ACL connection handle.
 * @return      volume state pointer.
 */
blc_vcs_volume_state_t *blc_vcss_getVolState(u16 connHandle);

/**
 * @brief       This function server to get volume flag.
 * @param[in]   connHandle: ACL connection handle.
 * @return      volume flag pointer.
 */
blc_vcs_volume_flags_t *blc_vcss_getVolFlags(u16 connHandle);


//VCS Server Update Characteristic Value Operation API
int blc_vcss_updateVolSetting(u16 connHandle, u8 volSetting);
int blc_vcss_updateMuteState(u16 connHandle, bool mute);
int blc_vcss_updateVolState(u16 connHandle, u8 volSetting, bool mute);
int blc_vcss_updateVolFlags(u16 connHandle, blc_vcs_volume_flags_t flags);

/******************************* VCS Server End **********************************************************************/
