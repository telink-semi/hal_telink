/********************************************************************************************************
 * @file    bap_internal.h
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

#include "ascp/ascs_internal.h"
#include "basp/bass_internal.h"
#include "pacp/pacs_internal.h"


#define BLT_BAP_LOG(fmt, ...)           BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_BAP_LOG, "[BAP]" fmt, ##__VA_ARGS__)

#define BLT_BIS_LOG(fmt, ...)           BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_BCST_LOG, fmt, ##__VA_ARGS__)
#define BLT_BIS_SINK_LOG(fmt, ...)      BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_BCST_LOG, "[SINK]" fmt, ##__VA_ARGS__)
#define BLT_BIS_ASSISTANT_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_BCST_LOG, "[ASST]" fmt, ##__VA_ARGS__)

u8 *blt_bap_getCodecCfgTarget(u8 *pSpecCfg, u8 tgtType);

#define blt_bap_getCodecCfgSamplingFreq(cfg)           *blt_bap_getCodecCfgTarget(cfg, BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY)
#define blt_bap_getCodecCfgFrameDuration(cfg)          *blt_bap_getCodecCfgTarget(cfg, BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION)
#define blt_bap_getCodecCfgAudioChannelAllocation(cfg) *(u32 *)blt_bap_getCodecCfgTarget(cfg, BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION)
#define blt_bap_getCodecCfgOctetsPerFrame(cfg)         *(u16 *)blt_bap_getCodecCfgTarget(cfg, BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME)
#define blt_bap_getCodecCfgFrameBlocksPerSdu(cfg)      *blt_bap_getCodecCfgTarget(cfg, BLC_AUDIO_CAPTYPE_CFG_CODEC_FRAME_BLCKS_PER_SDU)

// Audio Codec_Specific_Capabilities Field Valid mask
enum
{
    BLC_AUDIO_SAMPLE_FREQUENCY_MASK       = BIT(0),
    BLC_AUDIO_FRAME_DURATION_MASK         = BIT(1),
    BLC_AUDIO_AUDIO_CHANNEL_MASK          = BIT(2),
    BLC_AUDIO_OCTETS_PER_CODEC_FRAME_MASK = BIT(3),
    BLC_AUDIO_CODEC_FRAMES_PER_SDU_MASK   = BIT(4),
};

// Audio Codec_Specific_Capabilities type
enum
{
    BLC_AUDIO_CAPTYPE_SUP_SAMPLE_FREQUENCY         = 0x01, // Supported_Sampling_Frequencies
    BLC_AUDIO_CAPTYPE_SUP_FRAME_DURATION           = 0x02, // Supported_Frame_Durations
    BLC_AUDIO_CAPTYPE_SUP_AUDIO_CHN_COUNTS         = 0x03, // Audio_Channel_Counts
    BLC_AUDIO_CAPTYPE_SUP_OCTETS_PER_CODEC_FRAME   = 0x04, // Supported_Octets_Per_Codec_Frame
    BLC_AUDIO_CAPTYPE_SUP_MAX_CODEC_FRAMES_PER_SDU = 0x05, // Supported_Max_Codec_Frames_Per_SDU
};

// Audio Codec_Specific_Configuration parameter
enum
{
    BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY          = 0x01, // Sampling_Frequency
    BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION            = 0x02, // Frame_Duration
    BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION       = 0x03, // Audio_Channel_Allocation
    BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME    = 0x04, // Octets_Per_Codec_Frame
    BLC_AUDIO_CAPTYPE_CFG_CODEC_FRAME_BLCKS_PER_SDU = 0x05, // Codec_Frame_Blocks_Per_SDU
};

enum
{
    BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS_MASK = BIT(0),
    BLC_AUDIO_METATYPE_STREAMING_CONTEXTS_MASK = BIT(1),
    BLC_AUDIO_METATYPE_PROGRAM_INFO_MASK       = BIT(2),
    BLC_AUDIO_METATYPE_LANGUAGE_MASK           = BIT(3),
    BLC_AUDIO_METATYPE_CCID_LIST_MASK          = BIT(4),
    BLC_AUDIO_METATYPE_PARENTAL_RATING_MASK    = BIT(5),
    BLC_AUDIO_METATYPE_PROGRAM_INFO_URI_MASK   = BIT(6),
    BLC_AUDIO_METATYPE_EXTENDED_METADATA_MASK  = BIT(7),
    BLC_AUDIO_METATYPE_VENDOR_SPECIFIC_MASK    = BIT(8),
};

// Audio Direction
enum
{
    BLT_ASE_DIRECTION_SINK  = BIT(0),
    BLT_ASE_DIRECTION_SRC   = BIT(1),
    BLT_ASE_DIRECTION_BIDIR = BITS(0, 1),
};

void blt_audio_broadcastSinkRecvEvt(u16 connHandle, int evtID, u8 *pData, u16 dataLen);
bool blt_audio_sinkGetBroadcastId(u8 bcstId[3], u8 *pAdvDat, u32 len);

enum
{
    PA_SYNC_STATE_NONE,
    PA_SYNC_STATE_START,
    PA_SYNC_STATE_SUCCESS,
};

typedef struct
{
    u16 syncHandle;
    u16 dataLength; // 0 to 247 Length of the Data field
    u16 dataOffset;
    u8  data[240];
} pda_recombination_t;

extern pda_recombination_t gPdaPkt;

//common function
blc_ascs_server_t *blt_ascss_getCtrl(u16 connHandle);

int blt_audio_unicastDataPathSetup(blt_ascss_ase_state_t *pAse, u8 type, u8 role);

void blt_ascss_ntfAllAseState(u16 connHandle);

int blt_audio_bcstAssistantRecvEvt(u16 connHandle, int evtID, u8 *pData, u16 dataLen);

int blt_pda_recombination_handler(u8 *p);

//app event
void blt_audio_sendCisConnEvt(hci_le_cisEstablishedEvt_t *pData);

void blt_audio_sendCisDisconnEvt(hci_disconnectionCompleteEvt_t *pData);

void blt_audio_sendCisReqEvt(hci_le_cisReqEvt_t *pData);


//Unicast client event
void blt_audio_unicastCltSetCigParamsEvt(u16 connHandle);

void blt_audio_unicastCltUpdateEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltRcvStreamEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltSendStreamEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltDisablingEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltReleasingEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltStreamStopEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltCodecCfgEvt(u16 connHandle, blt_ascsc_ase_t *pAse, blt_ascsc_aseStateCodecCfg_t *codecParam);

void blt_audio_unicastCltQosCfgEvt(u16 connHandle, blt_ascsc_ase_t *pAse);

void blt_audio_unicastCltEnablingEvt(u16 connHandle, blt_ascsc_ase_t *pAse);


//Unicast Client operation

int blc_bapuc_setASEOperationEnable(u16 aclHandle, u8 aseID);

//Unicast server event
void blt_audio_unicastSvrCodecCfgEvt(u16 connHandle, blt_ascss_ase_state_t *pAse, blc_audio_codecSpecCfgParsed_t *pCodecCfg);

void blt_audio_unicastSvrQosCfgEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrEnablingEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrUpdateEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrDisablingEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrReleasingEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrSendStreamEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrRcvStreamEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);

void blt_audio_unicastSvrRcvStopRdyEvt(u16 connHandle, blt_ascss_ase_state_t *pAse);
