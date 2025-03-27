/********************************************************************************************************
 * @file    vocs.h
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



/******************************* VOCS Common Start **********************************************************************/

/* Volume Offset State characteristic value format */
typedef struct {
    s16 volOffset;
    u8 changeCnt;
} blc_vocs_volume_offset_state_t;

typedef enum{
    VOCS_OPCODE_SET_VOLUME_OFFSET           =   0x01,
}blt_vocs_volume_offset_control_opcode_enum;

#include "vocs_client_buf.h"
#include "vocs_server_buf.h"

/******************************* VOCS Common End **********************************************************************/




/******************************* VOCS Client Start **********************************************************************/

//VOCS Client Event ID
typedef enum{
    AUDIO_EVT_VOCSC_START = AUDIO_EVT_TYPE_VOCSC,
    AUDIO_EVT_VOCSC_CHANGED_VOLUME_OFFSET,      //refer to 'blc_vocsc_volumeOffsetStateChangeEvt_t'
    AUDIO_EVT_VOCSC_CHANGED_LOCATION,           //refer to 'blc_vocsc_locationChangeEvt_t'
    AUDIO_EVT_VOCSC_CHANGED_OUTPUT_DESCRIPTION, //refer to 'blc_vocsc_outputDescChangeEvt_t'
} audio_vocsc_evt_enum;

typedef struct{ //Event ID: AUDIO_EVT_VOCSC_CHANGED_VOLUME_OFFSET
    u8 vocsIndex;
    s16 volumeOffset;
} blc_vocsc_volumeOffsetStateChangeEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_VOCSC_CHANGED_LOCATION
    u8 vocsIndex;
    u32 location;
} blc_vocsc_locationChangeEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_VOCSC_CHANGED_OUTPUT_DESCRIPTION
    u8 vocsIndex;
    u16 descLen;
    u8* desc;
} blc_vocsc_outputDescChangeEvt_t;


/**
 * @brief       This function VOCS client get client controller module.
 * @param[in]   connHandle: ACL connection handle.
 * @param[in]   index: VOCS index in VCP.
 * @return      VOCS client controller value.
 */
blc_vocs_client_t* blc_vocsc_vcpGetClientByIndex(u16 connHandle, int index);


//VOCS Client Read Characteristic Value Operation API
int blc_vocsc_readVolOffsetState(u16 connHandle, blc_vocs_client_t* vocsc, prf_read_cb_t readCb);
int blc_vocsc_readAudioLoc(u16 connHandle, blc_vocs_client_t* vocsc, prf_read_cb_t readCb);
int blc_vocsc_readOutputDesc(u16 connHandle, blc_vocs_client_t* vocsc, prf_read_cb_t readCb);
int blc_vocsc_readVcpVolOffsetState(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_vocsc_readVcpAudioLoc(u16 connHandle, int index, prf_read_cb_t readCb);
int blc_vocsc_readVcpOutputDesc(u16 connHandle, int index, prf_read_cb_t readCb);

//VOCS Client Get Characteristic Value Operation API
int blc_vocsc_getVolOffsetState(u16 connHandle, blc_vocs_client_t* vocsc, blc_vocs_volume_offset_state_t* state);
int blc_vocsc_getAudioLoc(u16 connHandle, blc_vocs_client_t* vocsc, u32* location);
int blc_vocsc_getOutputDesc(u16 connHandle, blc_vocs_client_t* vocsc, u8* desc, u16* descLen);
int blc_vocsc_getVcpVolOffsetState(u16 connHandle, int index, blc_vocs_volume_offset_state_t* state);
int blc_vocsc_getVcpAudioLoc(u16 connHandle, int index, u32* location);
int blc_vocsc_getVcpOutputDesc(u16 connHandle, int index, u8* desc, u16* descLen);

//VOCS Client Write Characteristic Value Operation API
int blc_vocsc_writeAudioLoc(u16 connHandle, blc_vocs_client_t* vocsc, u32 location);
int blc_vocsc_writeVolOffsetCtrlPoint(u16 connHandle, blc_vocs_client_t* vocsc, blt_vocs_volume_offset_control_opcode_enum opcode, s16 volOffset, prf_write_cb_t writeCb);
int blc_vocsc_writeOutputDesc(u16 connHandle, blc_vocs_client_t* vocsc, u8* desc, u16 descLen);
int blc_vocsc_writeVcpAudioLoc(u16 connHandle, int index, u32 location);
int blc_vocsc_writeVcpOutputDesc(u16 connHandle, int index, u8* desc, u16 descLen);
int blc_vocsc_writeVcpVolOffsetCtrlPoint(u16 connHandle, int index, blt_vocs_volume_offset_control_opcode_enum opcode, s16 volOffset, prf_write_cb_t writeCb);
int blc_vocsc_writeSetVolOffset(u16 connHandle, blc_vocs_client_t* vocsc, s16 volOffset, prf_write_cb_t writeCb);
int blc_vocsc_vcpWriteSetVolOffset(u16 connHandle, int index, s16 volOffset, prf_write_cb_t writeCb);

/******************************* VOCS Client End **********************************************************************/




/******************************* VOCS Server Start **********************************************************************/

//VOCS Server Event ID
typedef enum{
    AUDIO_EVT_VOCSS_START = AUDIO_EVT_TYPE_VOCSS,
    AUDIO_EVT_VOCSS_CHANGED_VOLUME_OFFSET,      //refer to 'blc_vocss_volumeOffsetStateChangeEvt_t'
    AUDIO_EVT_VOCSS_CHANGED_LOCATION,           //refer to 'blc_vocss_locationChangeEvt_t'
    AUDIO_EVT_VOCSS_CHANGED_OUTPUT_DESCRIPTION, //refer to 'blc_vocss_outputDescChangeEvt_t'
} audio_vocss_evt_enum;

typedef struct{ //Event ID: AUDIO_EVT_VOCSS_CHANGED_VOLUME_OFFSET
    u8 vocsIndex;
    s16 volumeOffset;
} blc_vocss_volumeOffsetStateChangeEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_VOCSS_CHANGED_LOCATION
    u8 vocsIndex;
    u32 location;
} blc_vocss_locationChangeEvt_t;

typedef struct{ //Event ID: AUDIO_EVT_VOCSS_CHANGED_OUTPUT_DESCRIPTION
    u8 vocsIndex;
    u16 descLen;
    u8* desc;
} blc_vocss_outputDescChangeEvt_t;
/******************************* VOCS Server End **********************************************************************/
