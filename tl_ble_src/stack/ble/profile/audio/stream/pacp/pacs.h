/********************************************************************************************************
 * @file    pacs.h
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

#include "pacs_client_buf.h"
#include "pacs_server_buf.h"


/******************************* PACS Common Start **********************************************************************/

/******************************* PACS Common End **********************************************************************/


/******************************* PACS Client Start **********************************************************************/

extern const u8  gAppPacsCltSinkPacNum;
extern const u8  gAppPacsCltSrcPacNum;
extern const u16 gAppPacsCltPacMaxSize;

//PACS Client Event ID
typedef enum
{
    AUDIO_EVT_PACSC_START = AUDIO_EVT_TYPE_PACSC,
    //NONE:
} audio_pacsc_evt_enum;

/**
 * @brief       This function serves to register PACS Client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerPACSControlClient(const blc_pacsc_regParam_t *param);


//PACS Client Read Characteristic Value Operation API
int blc_pacsc_readSinkPac(u16 connHandle, prf_read_cb_t readCb);
int blc_pacsc_readSinkAudioLoc(u16 connHandle, prf_read_cb_t readCb);
int blc_pacsc_readSourcePac(u16 connHandle, prf_read_cb_t readCb);
int blc_pacsc_readSourceAudioLoc(u16 connHandle, prf_read_cb_t readCb);
int blc_pacsc_readAvaAudioCtx(u16 connHandle, prf_read_cb_t readCb);
int blc_pacsc_readSupAudioCtx(u16 connHandle, prf_read_cb_t readCb);

//PACS Client Get Characteristic Value Operation API
int blc_pacsc_getSinkPac(u16 connHandle, u8 *sinkPac, u16 *sinkPacLen);
int blc_pacsc_getSinkAudioLoc(u16 connHandle, u32 *sinkLoc);
int blc_pacsc_getSourcePac(u16 connHandle, u8 *srcPac, u16 *srcPacLen);
int blc_pacsc_getSourceAudioLoc(u16 connHandle, u32 *srcLoc);
int blc_pacsc_getAvaAudioCtx(u16 connHandle, u16 *avaSinkCtx, u16 *avaSrcCtx);
int blc_pacsc_getSupAudioCtx(u16 connHandle, u16 *supSinkCtx, u16 *supSrcCtx);

//PACS Client Write Characteristic Value Operation API
int blc_pacsc_writeSinkAudioLoc(u16 connHandle, u32 audioLoc, prf_write_cb_t writeCb);
int blc_pacsc_writeSrcAudioLoc(u16 connHandle, u32 audioLoc, prf_write_cb_t writeCb);

int blc_pacsc_checkPACLegal(blc_audio_codec_id_t *codec, blc_audio_codecSpecCfgParsed_t *codecCfg, blc_audio_metadata_parsed_t *metadata, u8 *pac, u32 locations);
int blc_pacsc_checkSinkPAC(u16 connHandle, blc_audio_codec_id_t *codec, blc_audio_codecSpecCfgParsed_t *codecCfg, blc_audio_metadata_parsed_t *metadata);

/*******************************  PACS Client End  **********************************************************************/


/*******************************  PACS Server Start  **********************************************************************/

//Default PACS server parameters
extern const blc_pacss_regParam_t defaultPacsParam;

//PACS Server Event ID
typedef enum
{
    AUDIO_EVT_PACSS_START = AUDIO_EVT_TYPE_PACSS,
    //NONE:
} audio_pacss_evt_enum;

/**
 * @brief       This function serves to register PACS Server function
 * @param[in]   refer to 'blc_pacss_regParam_t'
 * @return      none.
 */
void blc_audio_registerPACSControlServer(const blc_pacss_regParam_t *param);

//PACS Server Update Characteristic Value Operation API
int blc_pacss_updateSinkAudioLocations(u16 connHandle, u32 locations);
int blc_pacss_updateSourceAudioLocations(u16 connHandle, u32 locations);


/******************************* PACS Server End **********************************************************************/
