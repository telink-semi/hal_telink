/********************************************************************************************************
 * @file    pacs_server_buf.h
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

#define INIT_CODEC_ID_LC3 { \
    .id        = 0x06,      \
    .companyID = 0x0000,    \
    .vendorID  = 0x0000,    \
}

#define CODEC_SPEC_CAP(freq, durations, octets, counts, perSdu) .codecId = INIT_CODEC_ID_LC3, .codecSpec = {                     \
                                                                                                  .samplingFreq     = freq,      \
                                                                                                  .frameDurations   = durations, \
                                                                                                  .minPerCodecFrame = octets,    \
                                                                                                  .maxPerCodecFrame = octets,    \
                                                                                                  .channelCounts    = counts,    \
                                                                                                  .maxPerSdu        = perSdu,    \
}

#define LC3_CAP_8_1(counts, perSdu)   CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_8000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 26, counts, perSdu)
#define LC3_CAP_8_2(counts, perSdu)   CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_8000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 30, counts, perSdu)
#define LC3_CAP_16_1(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_16000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 30, counts, perSdu)
#define LC3_CAP_16_2(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_16000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 40, counts, perSdu)
#define LC3_CAP_24_1(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_24000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 45, counts, perSdu)
#define LC3_CAP_24_2(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_24000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 60, counts, perSdu)
#define LC3_CAP_32_1(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_32000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 60, counts, perSdu)
#define LC3_CAP_32_2(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_32000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 80, counts, perSdu)
#define LC3_CAP_441_1(counts, perSdu) CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_44100, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 97, counts, perSdu)
#define LC3_CAP_441_2(counts, perSdu) CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_44100, BLC_AUDIO_SUPP_DURATION_FLAG_10, 130, counts, perSdu)
#define LC3_CAP_48_1(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 75, counts, perSdu)
#define LC3_CAP_48_2(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 100, counts, perSdu)
#define LC3_CAP_48_3(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 90, counts, perSdu)
#define LC3_CAP_48_4(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 120, counts, perSdu)
#define LC3_CAP_48_5(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_7_5, 117, counts, perSdu)
#define LC3_CAP_48_6(counts, perSdu)  CODEC_SPEC_CAP(BLC_AUDIO_SUPP_FREQ_FLAG_48000, BLC_AUDIO_SUPP_DURATION_FLAG_10, 155, counts, perSdu)

#define METADATA_CONTEXTS(contexts)   .metadata =                          \
                                        {                                  \
                                            .preferredContexts = contexts, \
                                            .StreamingContexts = contexts, \
}

#define METADATA_CONTEXTS_UNSPECIFIED      METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED)
#define METADATA_CONTEXTS_CONVERSATIONAL   METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL)
#define METADATA_CONTEXTS_MEDIA            METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_MEDIA)
#define METADATA_CONTEXTS_GAME             METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_GAME)
#define METADATA_CONTEXTS_INSTRUCTIONAL    METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_INSTRUCTIONAL)
#define METADATA_CONTEXTS_VOICE_ASSISTANTS METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_VOICE_ASSISTANTS)
#define METADATA_CONTEXTS_LIVE             METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_LIVE)
#define METADATA_CONTEXTS_SOUND_EFFECTS    METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_SOUND_EFFECTS)
#define METADATA_CONTEXTS_NOTIFICATIONS    METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_NOTIFICATIONS)
#define METADATA_CONTEXTS_RINGTONE         METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_RINGTONE)
#define METADATA_CONTEXTS_ALERT            METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_ALERT)
#define METADATA_CONTEXTS_EMERGENCY_ALARM  METADATA_CONTEXTS(BLC_AUDIO_CONTEXT_TYPE_EMERGENCY_ALARM)

typedef struct
{
    u16 sinkPacHandle;
    u16 sinkAudioLocationsHandle;
    u16 sourcePacHandle;
    u16 SourceAudioLocationsHandle;
    u16 availableAudioContextsHandle;
    u16 suppAudioContextsHandle;
} blc_pacs_server_t;

typedef struct blc_pacs_server_ctrl
{
    blc_prf_proc_t    process;
    blc_pacs_server_t pacsServer;
} blc_pacs_server_ctrl_t;

typedef struct
{
    blc_audio_codec_id_t          codecId; //Codec ID, 06 0000 0000 mean LC3 codec
    blc_audio_codecSpecCapParam_t codecSpec;
    blc_audio_metadataParam_t     metadata;
} blc_audio_pacParam_t;

typedef struct
{
    u8                          sinkPacNum;           //number of Sink PAC records
    const blc_audio_pacParam_t *sinkPac;              //Sink PAC
    u32                         sinkAudioLocations;   //Sink Audio Location, BLC_AUDIO_LOCATION_FLAG_FL

    u8                          sourcePacNum;         //number of Source PAC records
    const blc_audio_pacParam_t *sourcePac;            //Source PAC
    u32                         sourceAudioLocations; //Source Audio Location, BLC_AUDIO_LOCATION_FLAG_FL

    u16 availableSinkContexts;                        //Available Sink Contexts, BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED
    u16 availableSourceContexts;                      //Available Source Contexts, BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED
    u16 supportedSinkContexts;                        //Supported Sink Contexts, BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED
    u16 supportedSourceContexts;                      //Supported Sink Contexts, BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED
} blc_pacss_regParam_t;
