/********************************************************************************************************
 * @file    app_audio.h
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

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_RECEIVER)


    #define DEFALUT_NUM_SUBGROUP 1
    #define SUBGROUP_NUM_BIS     2

/*
 * Broadcast source PDA Advertise parameter
 * must have: local Name/ Flags
 * BAP: broadcast ID
 * PBP: Broadcast Name/Public Broadcast Announcement
 */
typedef struct
{
    u8 samplingFreqLen;
    u8 samplingFreqType;
    u8 samplingFreq;

    u8 frameDurationLen;
    u8 frameDurationType;
    u8 frameDuration;

    u8  octetsPerFrameLen;
    u8  octetsPerFrameType;
    u16 octetsPerFrame;
} bisPdaCodecSpecificConfig_t;

typedef struct
{
    u8  streamingAudioContextsLen;
    u8  streamingAudioContextsType;
    u16 streamingAudioContexts;
    u8  languageLen;
    u8  languageType;
    u8  language[3];
} bisPdaMetadata_t;

typedef struct
{
    u8  bisIndex;
    u8  size;
    u8  audioChannelAllocationLen;
    u8  audioChannelAllocationType;
    u32 audioChannelAllocation;
} bisPdaEachAddiInfo_t;

typedef struct
{
    u8 numBis;
    // codec id
    u8  codecFormat;
    u16 companyID;
    u16 vendorSpecificCodecID;
    //codec specific configuration
    u8                          CodecSpecificConfigLen;
    bisPdaCodecSpecificConfig_t CodecSpecificConfig;
    u8                          metadataLen;
    bisPdaMetadata_t            metadata;
    bisPdaEachAddiInfo_t        addiInfo[SUBGROUP_NUM_BIS];
} bisGroupInfo_t;

typedef struct
{
    u8             length;
    u8             type;
    u16            BASS_UUID;
    u8             presentationDelay[3];
    u8             numSubgroups;
    bisGroupInfo_t groupInfo[DEFALUT_NUM_SUBGROUP];
} bisSourcPdaeAdvData_t;

    #define APP_AUDIO_FRAME_BYTES           960 //max 48KHz
    #define APP_SINK_RECV_SPEAK_FRAME_COUNT 16

typedef struct
{
    u16 aclHandle;
    u16 bisHandle;
    u16 bisHandle1;

    u16 decodeSize;
    u16 frameDataLen;

    u32 recvPckCnt;
    u32 spkState;
    u8  recvSpeakBuff[APP_AUDIO_FRAME_BYTES * APP_SINK_RECV_SPEAK_FRAME_COUNT];
} appSinkInfo_t;

extern appSinkInfo_t appSinkInfo;

extern u8 codecSpeakBuff[APP_AUDIO_FRAME_BYTES * APP_SINK_RECV_SPEAK_FRAME_COUNT];

void app_audio_handler(void);


#endif /* INTER_TEST_MODE */
