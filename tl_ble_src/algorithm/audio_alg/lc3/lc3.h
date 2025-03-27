/********************************************************************************************************
 * @file    lc3.h
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

#include "vendor/common/user_config.h"
#include "types.h"
#include "tlka_lc3_api.h"

#ifndef LC3_DUMP_EN
#define LC3_DUMP_EN                     0
#endif

#ifndef LC3_ENCODE_CHANNEL_COUNT
#define LC3_ENCODE_CHANNEL_COUNT        0
#endif

#ifndef LC3_DECODE_CHANNEL_COUNT
#define LC3_DECODE_CHANNEL_COUNT        0
#endif

typedef struct{
    u32 freqHz;

    u8  freqCodec;
    u16 duraUs;
    u8  duralc3;

    u16 frameSample;
    u16 frameOctets;
}lc3ParamIndex;

//gLc3Index[frequencyCfg][durationCFG]
extern  const lc3ParamIndex gLc3Index[13][2];


/**
 * @brief       This function applicable initial lc3 encode.
 * @param[in]   index       - The index of encode, [0, LC3_ENCODE_CHANNEL_COUNT-1]
 * @param[in]   nSamplerate - original audio sampling frequency. 8kHz---8000
 * @param[in]   nBitrate    - Compressed audio bit rate.
 * @param[in]   nMs_mode    - audio compression cycle. 1 mean 7.5ms, 0 mean 10ms.
 * @return      -1          - index range error.
 *              other       - LC3ENC_Error
 */
int lc3enc_encode_init(u8 index, u32 nSamplerate, u32 nBitrate, u16 nMs_mode);

/**
 * @brief       This function BAP initial lc3 encode.
 * @param[in]   index           - The index of encode, [0, LC3_ENCODE_CHANNEL_COUNT-1]
 * @param[in]   samplingFreq    - Sampling_Frequency.
 * @param[in]   frameDuration   - Frame_Duration.
 * @param[in]   perCodecFram    - Octets_Per_Codec_Frame.
 * @return      -1              - index range error.
 *              -2              - parameter not supported.
 *              other           - LC3ENC_Error
 */
int lc3enc_encode_init_bap(u8 index, u8 samplingFreq, u8 frameDuration, u16 perCodecFrame);

/**
 * @brief       This function encode audio data to LC3 data.
 * @param[in]   index   - The index of encode, [0, LC3_ENCODE_CHANNEL_COUNT-1]
 * @param[in]   rawData - One frame of raw audio data.
 * @param[out]  encData - One frame of compressed audio data.
 * @return      -1      - index range error.
 *              other   - LC3ENC_Error
 */
int lc3enc_encode_pkt(u8 index, u8* rawData, u8* encData);

/**
 * @brief       This function free lc3 encode.
 * @param[in]   index   - The index of encode, [0, LC3_ENCODE_CHANNEL_COUNT-1]
 * @return      -1      - index range error.
 *              other   - LC3ENC_Error
 */
int lc3enc_free_init(u8 index);

/**
 * @brief       This function applicable initial lc3 decode.
 * @param[in]   index       - The index of decode, [0, LC3_DECODE_CHANNEL_COUNT-1]
 * @param[in]   nSamplerate - original audio sampling frequency. 8kHz---8000
 * @param[in]   nBitrate    - Compressed audio bit rate.
 * @param[in]   nMs_mode    - audio compression cycle. 1 mean 7.5ms, 0 mean 10ms.
 * @return      -1          - index range error.
 *              other       - LC3ENC_Error
 */
int lc3dec_decode_init(u8 index, u32 nSamplerate, u32 nBitrate, u16 nMs_mode);

/**
 * @brief       This function BAP initial lc3 decode.
 * @param[in]   index           - The index of decode, [0, LC3_DECODE_CHANNEL_COUNT-1]
 * @param[in]   samplingFreq    - Sampling_Frequency.
 * @param[in]   frameDuration   - Frame_Duration.
 * @param[in]   perCodecFram    - Octets_Per_Codec_Frame.
 * @return      -1              - index range error.
 *              -2              - parameter not supported.
 *              other           - LC3ENC_Error
 */
int lc3dec_decode_init_bap(u8 index, u8 samplingFreq, u8 frameDuration, u16 perCodecFrame);

/**
 * @brief       This function decode LC3 data to audio data.
 * @param[in]   index   - The index of encode, [0, LC3_DECODE_CHANNEL_COUNT-1]
 * @param[in]   encData - compressed audio data.
 * @param[in]   encDataLen  - The length of compressed audio data.
 * @param[out]  rawData - raw audio data.
 * @return      -1      - index range error.
 *              other   - LC3ENC_Error
 */
int lc3dec_decode_pkt(u8 index, u8* encData, u16 encDataLen, u8* rawData);

/**
 * @brief       This function sever to set lc3 decode parameter.
 * @param[in]   index   - The index of decode, [0, LC3_DECODE_CHANNEL_COUNT-1]
 * @param[in]   para    - The parameter which to set.
 * @param[in]   val     - The value of parameter.
 * @return      -1      - index range error.
 *              other   - LC3ENC_Error
 */
int lc3dec_set_parameter(u8 index, LC3_PARAMETER para, u32* val);

/**
 * @brief       This function free lc3 decode.
 * @param[in]   index   - The index of decode, [0, LC3_DECODE_CHANNEL_COUNT-1]
 * @return      -1      - index range error.
 *              other   - LC3ENC_Error
 */
int lc3dec_free_init(u8 index);
