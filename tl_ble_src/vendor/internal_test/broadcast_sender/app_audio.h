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

#include "tl_common.h"
#include "app_config.h"
#include "app_buffer.h"
#include "stack/ble/ble.h"
#include "stack/ble/profile/audio/stream/bap.h"

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)


    //broadcast source extend advertising parameter
    #define DEFAULT_LOCAL_NAME                 "Telink-SOURCE"
    #define DEFALUT_NUM_SUBGROUP               1
    #define SUBGROUP_NUM_BIS                   2

    #define BIS_STREAM_CONFIG_LOW_LATENCY      0
    #define BIS_STREAM_CONFIG_HIGH_RELIABILITY 1
    #define BIS_STREAM_CONFIG                  BIS_STREAM_CONFIG_HIGH_RELIABILITY


    #if (SUBGROUP_NUM_BIS > APP_BIS_NUM_IN_PER_BIG_BCST)
        #error "bis num large"
    #endif


    #define DEFAULT_METADATA_LENGTH 0

    #define BIS_CONTEXTS_TYPE       BLC_AUDIO_CONTEXT_TYPE_MEDIA
    #define BIS_LANGUAGE            0x0003A8 //

    #define BIS_1_AUDIO_LOCATION    BLC_AUDIO_LOCATION_FLAG_FL
    #define BIS_2_AUDIO_LOCATION    BLC_AUDIO_LOCATION_FLAG_FR

    /* Audio configuration */
    #define APP_AUDIO_FRAME_SAMPLE (480)                         // 48K => 10ms * 48sample
    #define APP_AUDIO_FRAME_BYTES  (APP_AUDIO_FRAME_SAMPLE << 1) // 1 sample 16bits

    #define APP_USB_1MS_BUFF_LEN   (APP_AUDIO_FRAME_BYTES / 10)  //AUDIO_SAMPLE_RATE //bytes per ms

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

/*
 * Broadcast source extend Advertise parameter
 * must have: local Name/ Flags
 * BAP: broadcast ID
 * PBP: Broadcast Name/Public Broadcast Announcement
 */
typedef struct
{
    //adv local name
    u8 localNameLen;
    u8 localNameType;
    u8 localName[sizeof(DEFAULT_LOCAL_NAME) - 1];
    //adv flags
    u8 flagsLen;
    u8 flagsType;
    u8 flags;
    //adv BAAS UUID (BAP)
    u8  broadcastIDLen;
    u8  broadcastIDType;
    u16 BAAS_UUID;
    u8  broadcastID[3];
    //adv PBAS UUID (PBP)
    u8  PBAFeatureLen;
    u8  PBAFeatureType;
    u16 PBAS_UUID;
    u8  PBAFeature;
    u8  metadataLen;
    u8  metadata[DEFAULT_METADATA_LENGTH];
    //adv Broadcast Name (PBP)
    u8 broadcastNameLen;
    u8 broadcastNameType;
    u8 broadcastName[sizeof(DEFAULT_BROADCAST_NAME) - 1];

} bisSourceAdvData_t;

extern unsigned short gAppAudioBuffer[APP_AUDIO_FRAME_BYTES * 2];
extern u8             gbisSamplingFreq;
extern u8             gbisFrameDuration;
extern u16            gbisOctetPer;
extern bool           gAppAudioIsSend;

int  app_audio_init(void);
void app_audio_handler(void);
void app_audio_send_handler(void);
bool app_audio_check_send_flag(void);

void app_audio_usb_speak_init(void);
void app_audio_usb_speaker_handle(void);
void app_audio_codec_init(void);
void app_audio_buffer_handle(void);

#endif /* INTER_TEST_MODE */
