/********************************************************************************************************
 * @file    lc3.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "lc3.h"
#include "common/compiler.h"
#include "tl_common.h"


#if LC3_ENCODE_CHANNEL_COUNT + LC3_DECODE_CHANNEL_COUNT

#define LC3_ENC_SIZE                2704    //0x0A88 V1.1.7
#define LC3_ENC_SCRATCH_SIZE        10240   //0x2800 V1.1.7
#define LC3_DEC_SIZE                5310    //0x14b8 V1.1.7
#define LC3_DEC_SCRATCH_SIZE        3840    //0x0f00 V1.1.7

#if (!LC3_ENCODE_CHANNEL_COUNT && LC3_DECODE_CHANNEL_COUNT)
#define LC3_SCRATCH_SIZE            LC3_DEC_SCRATCH_SIZE

#elif (LC3_ENCODE_CHANNEL_COUNT && !LC3_DECODE_CHANNEL_COUNT)
#define LC3_SCRATCH_SIZE            LC3_ENC_SCRATCH_SIZE
#else
#define LC3_SCRATCH_SIZE            max(LC3_ENC_SCRATCH_SIZE, LC3_DEC_SCRATCH_SIZE)
#endif


_attribute_iram_noinit_data_ u8 lc3enc_buff[LC3_ENC_SIZE * LC3_ENCODE_CHANNEL_COUNT];
_attribute_iram_noinit_data_ u8 lc3dec_buff[LC3_DEC_SIZE * LC3_DECODE_CHANNEL_COUNT];

u8 lc3Scratch[LC3_SCRATCH_SIZE];

//16bit each sample
const lc3ParamIndex gLc3Index[13][2] = {
    {{0,0,0,  0,0,0},
    {0,0,0,  0,0,0}},//rsvd

    {{8000,0,7500,  1,60,120},
    {8000,0,10000,  0,80,160}},//UNFRAMED,

    {{11025,1,7500, 1,0,0},
    {11025,1,10000, 0,0,0}},//FRAMED

    {{16000,3,7500, 1,120,240},
    {16000,3,10000, 0,160,320}},//UNFRAMED

    {{22050,4,7500, 1,0,0},
    {22050,4,10000, 0, 0,0}},//FRAMED

    {{24000,5,7500, 1,160,230},
    {24000,5,10000, 0,240,480}},//UNFRAMED

    {{32000,6,7500, 1,240,480},
    {32000,6,10000, 0,320,640}},//UNFRAMED

    {{44100,7,7500, 1,0,0},
    {44100,7,10000, 0,0,0}},//FRAMED

    {{48000,8,7500, 1,360,720},
    {48000,8,10000, 0,480,960}},//UNFRAMED

    {{88200,9,7500, 1,0,0},
    {88200,9,10000, 0,0,0}},//FRAMED

    {{96000,10,7500, 1,720,1440},
    {96000,10,10000, 0,960,1960}},//UNFRAMED

    {{176400,11,7500,1,0,0},
    {176400,11,10000,0,0,0}},//FRAMED

    {{192000,12,7500,1,1440,2880},
    {192000,12,10000,0,1920,3840}},//UNFRAMED
};

static int lc3_encode_init(u8* enc, u32 nSamplerate, u32 nBitrate, u16 nMs_mode)
{
    if(enc == NULL)
    {
        return -1;
    }

    LC3ENC_Error ret = 0;

    LC3_CFG_Param lc3_enc_param = {
        .channels = 1,
        .bitrate = nBitrate,
        .frame_ms = nMs_mode,
        .samplerate = nSamplerate,
        .pScratch  = lc3Scratch,
//      .plc_fadeout_in_ms = 30,
    };
    
    ret = tlka_lc3enc_init((LC3_ENC_STRU*)enc, &lc3_enc_param);

    tlkapi_send_string_u32s(LC3_DUMP_EN, "enc_size, scratch, encode", tlka_lc3enc_get_size(), tlka_lc3enc_get_scratch_size(), ret, (u32)enc);

    if(ret != LC3ENC_OK)
    {
        return ret;
    }

    Word32 in_framesize;
    Word32 out_framesize;
    Word32 tns_enable = 1;

    ret = tlka_lc3enc_get_parameter((LC3_ENC_STRU*)enc, LC3_PARA_FRAMESIZE, &in_framesize);
    ret = tlka_lc3enc_get_parameter((LC3_ENC_STRU*)enc, LC3_PARA_OUTSIZE, &out_framesize);
    ret = tlka_lc3enc_set_parameter((LC3_ENC_STRU*)enc, LC3_PARA_TNS, &tns_enable);

    tlkapi_send_string_u32s(LC3_DUMP_EN, "lc3enc_get_parameter", ret, in_framesize,out_framesize,0);

    return LC3ENC_OK;
}


int lc3enc_encode_init(u8 index, u32 nSamplerate, u32 nBitrate, u16 nMs_mode)
{
#if LC3_ENCODE_CHANNEL_COUNT
    if(index >= LC3_ENCODE_CHANNEL_COUNT)
    {
        return -1;
    }
    return lc3_encode_init(lc3enc_buff + index*LC3_ENC_SIZE, nSamplerate, nBitrate, nMs_mode);
#else
    return lc3_encode_init(NULL, 0, 0, 0);
#endif
}


int lc3enc_encode_init_bap(u8 index, u8 samplingFreq, u8 frameDuration, u16 perCodecFrame)
{
#if LC3_ENCODE_CHANNEL_COUNT
    if(index >= LC3_ENCODE_CHANNEL_COUNT)
    {
        return -1;
    }
    if(frameDuration > 2 || samplingFreq > 12 || samplingFreq == 0)
    {
        return -2;
    }
    u32 bitRate = (perCodecFrame*8*10*1000)/(frameDuration?100:75);
    tlkapi_send_string_u32s(LC3_DUMP_EN, "Encode Initial index, freq, bitrate,dura", index, gLc3Index[samplingFreq][frameDuration].freqHz, bitRate, gLc3Index[samplingFreq][frameDuration].duralc3);
    return lc3_encode_init(lc3enc_buff + index*LC3_ENC_SIZE, gLc3Index[samplingFreq][frameDuration].freqHz,\
                                                    bitRate, gLc3Index[samplingFreq][frameDuration].duralc3);
#else
    return lc3_encode_init(NULL, 0, 0, 0);
#endif
}

static int lc3_encode_stream(u8* enc, u8* rawData, u8* encData)
{
    if(enc == NULL || rawData == NULL || encData == NULL)
    {
        return -1;
    }

    return tlka_lc3enc_encode_frame((LC3_ENC_STRU*)enc, (Word16*)rawData, (UWord8*)encData);
}

int lc3enc_encode_pkt(u8 index, u8* rawData, u8* encData)
{
#if LC3_ENCODE_CHANNEL_COUNT
    if(index >= LC3_ENCODE_CHANNEL_COUNT)
    {
        return -1;
    }
    return lc3_encode_stream(lc3enc_buff + index*LC3_ENC_SIZE, rawData, encData);
#else
    return lc3_encode_stream(NULL, NULL, NULL);
#endif
}

int lc3enc_free_init(u8 index)
{
#if LC3_ENCODE_CHANNEL_COUNT
    if(index >= LC3_ENCODE_CHANNEL_COUNT)
    {
        return -1;
    }

    return tlka_lc3enc_free((LC3_ENC_STRU*)(lc3enc_buff + index*LC3_ENC_SIZE));
#else
    return -1;
#endif
}


static int lc3_decode_init(u8* dec, u32 nSamplerate, u32 nBitrate, u16 nMs_mode)
{
    if(dec == NULL)
    {
        return -1;
    }

    LC3_CFG_Param lc3_dec_param = {
        .channels = 1,
        .bitrate = nBitrate,
        .frame_ms = nMs_mode,
        .samplerate = nSamplerate,
        .pScratch = lc3Scratch,
        .plc_fadeout_in_ms = 30,
    };

    LC3ENC_Error ret = tlka_lc3dec_init((LC3_DEC_STRU*)dec, &lc3_dec_param);

    tlkapi_send_string_u32s(LC3_DUMP_EN, "dec_size, scratch, decode", tlka_lc3dec_get_size(), tlka_lc3dec_get_scratch_size(), ret, (u32)dec);

    return ret;
}

int lc3dec_decode_init(u8 index, u32 nSamplerate, u32 nBitrate, u16 nMs_mode)
{
#if LC3_DECODE_CHANNEL_COUNT
    if(index >= LC3_DECODE_CHANNEL_COUNT)
    {
        return -1;
    }
    return lc3_decode_init(lc3dec_buff + index*LC3_DEC_SIZE, nSamplerate, nBitrate, nMs_mode);
#else
    return lc3_decode_init(NULL, 0, 0, 0);
#endif
}

int lc3dec_decode_init_bap(u8 index, u8 samplingFreq, u8 frameDuration, u16 perCodecFrame)
{
#if LC3_DECODE_CHANNEL_COUNT
    if(index >= LC3_DECODE_CHANNEL_COUNT)
    {
        return -1;
    }

    if(frameDuration > 2 || samplingFreq > 12 || samplingFreq == 0)
    {
        return -2;
    }

    u32 bitRate = (perCodecFrame*8*10*1000)/(frameDuration?100:75);
    tlkapi_send_string_u32s(LC3_DUMP_EN, "Decode Initial index, freq, bitrate,dura", index, gLc3Index[samplingFreq][frameDuration].freqHz, bitRate, gLc3Index[samplingFreq][frameDuration].duralc3);
    return lc3_decode_init(lc3dec_buff + index*LC3_DEC_SIZE, gLc3Index[samplingFreq][frameDuration].freqHz,\
                                                    bitRate, gLc3Index[samplingFreq][frameDuration].duralc3);
#else
    return lc3_decode_init(NULL, 0, 0, 0);
#endif
}

static int lc3_decode_stream(u8* dec, u8* encData, u16 encDataLen, u8* rawData)
{
    if(dec == NULL)
    {
        return -1;
    }

    return tlka_lc3dec_decode_frame((LC3_DEC_STRU*)dec, (UWord8*)encData, (UWord32)encDataLen, (Word16*)rawData);
}

int lc3dec_decode_pkt(u8 index, u8* encData, u16 encDataLen, u8* rawData)
{
#if LC3_DECODE_CHANNEL_COUNT
    if(index >= LC3_DECODE_CHANNEL_COUNT)
    {
        return -1;
    }

    return lc3_decode_stream(lc3dec_buff + index*LC3_DEC_SIZE, encData, encDataLen, rawData);
#else
    return lc3_decode_stream(NULL, encData, encDataLen, rawData);
#endif
}

int lc3dec_set_parameter(u8 index, LC3_PARAMETER para, u32* val)
{
    return tlka_lc3dec_set_parameter((LC3_DEC_STRU*)(lc3dec_buff + index*LC3_DEC_SIZE), para, (Word32 *)val);
}

int lc3dec_free_init(u8 index)
{
#if LC3_DECODE_CHANNEL_COUNT
    if(index >= LC3_DECODE_CHANNEL_COUNT)
    {
        return -1;
    }

    return tlka_lc3dec_free((LC3_DEC_STRU*)(lc3dec_buff + index*LC3_DEC_SIZE));
#else
    return -1;
#endif
}

#endif
