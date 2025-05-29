/********************************************************************************************************
 * @file    app_audio.c
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
#include "tl_common.h"
#include "drivers.h"

#include "app_config.h"


#include "stack/ble/ble.h"


#if (INTER_TEST_MODE == TEST_PPM_ASRC_WITH_IIS_LINEIN)
#include "vendor/common/tlk_api/tlk_codec.h"
#include "app_audio.h"

    #include "algorithm/audio_alg/ppm_asrc/gcc7/tlka_ppm_asrc_api.h"
tlka_ppm_asrc_16_bit_param my_ppm_data = {0};

    #define ENCODE_EN                   0 //only demo,not enable
    #define PPM_ASRC_EN                 1 //can configure,0 or 1


    #define ASRC_PPM_BUFFER_LEN         16
    #define ASRC_PPM_BUFFER_LEN_TWO_CHN (ASRC_PPM_BUFFER_LEN << 1)
    #define APP_AUDIO_FRAME_SAMPLE_HALF (APP_AUDIO_FRAME_SAMPLE >> 1) //5ms frame sample


    #define MONO_MODE                   1
    #define STEREO_MODE                 2

    #define CODEC_MODE_SELECT           STEREO_MODE //can configure


    #define IIS_IN                      1
    #define LINE_IN                     2
    #define MIC_IN                      3

    #define INPUT_MODE_SELECT           LINE_IN //can configure


    #if INPUT_MODE_SELECT == LINE_IN
        #define CODEC_INPUT_PATH  TLK_CODEC_LINE
        #define CODEC_OUTPUT_PATH TLK_CODEC_LINE

    #elif INPUT_MODE_SELECT == MIC_IN
        #define CODEC_INPUT_PATH  TLK_CODEC_MIC
        #define CODEC_OUTPUT_PATH TLK_CODEC_LINE

    #elif INPUT_MODE_SELECT == IIS_IN //i2s only configure stereo demo
        #undef CODEC_MODE_SELECT
        #define CODEC_MODE_SELECT STEREO_MODE

    #endif


    #if CODEC_MODE_SELECT == STEREO_MODE
        #define CODEC_INPUT_CHN_NUM   TLK_CODEC_2_CHANNEL
        #define CODEC_OUTPUT_CHN_NUM  TLK_CODEC_2_CHANNEL
        #define CODEC_INPUT_BUFF_LEN  2 * APP_AUDIO_OUTPUT_BUFFER_SIZE
        #define CODEC_OUTPUT_BUFF_LEN 2 * APP_AUDIO_OUTPUT_BUFFER_SIZE
        #define PPM_ASRC_CHN          TLKA_PPM_ASRC_STEREO
        #define AUDIO_SAMPLE_SIZE     4
typedef unsigned int tcodec_type;
    #elif CODEC_MODE_SELECT == MONO_MODE
        #define CODEC_INPUT_CHN_NUM   TLK_CODEC_1_CHANNEL
        #define CODEC_OUTPUT_CHN_NUM  TLK_CODEC_1_CHANNEL
        #define CODEC_INPUT_BUFF_LEN  APP_AUDIO_OUTPUT_BUFFER_SIZE
        #define CODEC_OUTPUT_BUFF_LEN APP_AUDIO_OUTPUT_BUFFER_SIZE
        #define PPM_ASRC_CHN          TLKA_PPM_ASRC_SINGLE
        #define AUDIO_SAMPLE_SIZE     2
typedef unsigned short tcodec_type;
    #endif


unsigned short app_audio_output_buffer[APP_AUDIO_OUTPUT_BUFFER_SIZE]; //48k,10ms,
unsigned short app_audio_input_buffer[APP_AUDIO_INPUT_BUFFER_SIZE];   //48k,10ms,

app_ble_sync_st_t async = {0};

void app_audio_init(void)
{
    #if INPUT_MODE_SELECT == LINE_IN || INPUT_MODE_SELECT == MIC_IN
    //base init
    tlk_codec_init();
    //codec configure
    //output
    tlk_codec_config(TLK_CODEC_OUTPUT, TLK_CODEC_FREQ_48000, CODEC_OUTPUT_CHN_NUM, CODEC_OUTPUT_PATH, (u8 *)app_audio_output_buffer, CODEC_OUTPUT_BUFF_LEN);
    //input
    tlk_codec_config(TLK_CODEC_INPUT, TLK_CODEC_FREQ_48000, CODEC_INPUT_CHN_NUM, CODEC_INPUT_PATH, (u8 *)app_audio_input_buffer, CODEC_INPUT_BUFF_LEN);


    //codec start
    //input
    tlk_codec_sts_e codecRet = tlk_codec_start(TLK_CODEC_INPUT);
    if (codecRet == TLK_CODEC_STATE_ERROR || codecRet == TLK_CODEC_OPERATION_REPEAT) {
        BLT_APP_LOG("error codec state %d", codecRet);
    }
    //output
    codecRet = tlk_codec_start(TLK_CODEC_OUTPUT);
    if (codecRet == TLK_CODEC_STATE_ERROR || codecRet == TLK_CODEC_OPERATION_REPEAT) {
        BLT_APP_LOG("error codec state %d", codecRet);
    }

    #else //i2s only configure stereo demo
    iis_base_init((u8 *)app_audio_input_buffer, 2 * APP_AUDIO_INPUT_BUFFER_SIZE, (u8 *)app_audio_input_buffer, 2 * APP_AUDIO_INPUT_BUFFER_SIZE);
    #endif

          ///////////////////////////////////////////////////////////////////////////////////////////////
    //PPM ASRC INIT
    #if PPM_ASRC_EN
    tlka_ppm_asrc_16_bit_init(&my_ppm_data, PPM_ASRC_CHN, 0);
    async.mic_num = ASRC_PPM_BUFFER_LEN;
    BLT_APP_LOG("ppm asrc demo init end");
    #endif

    ////////////////////////////////////////////////////////////////////////////////////////////////
    //LC3 init
    #if ENCODE_EN
    //encode chn0
    int lc3Ret = lc3enc_encode_init_test(0, BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 100);
    if (lc3Ret != LC3ENC_OK) {
        BLT_APP_LOG("lc3 encode chn0 init fail:0x%x", lc3Ret);
    }
    //encode chn1
    lc3Ret = lc3enc_encode_init_test(1, BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 100);
    if (lc3Ret != LC3ENC_OK) {
        BLT_APP_LOG("lc3 encode chn1 init fail:0x%x", lc3Ret);
    }
    //decode chn0
    lc3Ret = lc3enc_decode_init_test(0, BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 100);
    if (lc3Ret != LC3DEC_OK) {
        BLT_APP_LOG("lc3 decode chn0 init fail:0x%x", lc3Ret);
    }
    //decode chn1
    lc3Ret = lc3enc_decode_init_test(1, BLC_AUDIO_FREQ_CFG_48000, BLC_AUDIO_DURATION_CFG_10, 100);
    if (lc3Ret != LC3DEC_OK) {
        BLT_APP_LOG("lc3 decode chn1 init fail:0x%x", lc3Ret);
    }
    #endif

    //only debug
    //  u32 wptr = codec_get_InputWriteOffset();
    //  while(wptr == codec_get_InputWriteOffset()){
    //      gpio_toggle(GPIO_LED_GREEN);
    //      tlkapi_debug_handler();
    //      sleep_us(50000);
    //  }
}

_attribute_ram_code_ int asrc_i2s_48k_ppm(void)
{
    #define REF_SAMPLES 480

    /////////////////  I2S sampling rate PPM calculation ////////////////////
    static u32 t_1s       = 0;
    static int as_samples = 0;
    static u32 as_wptr    = 0;

    if (!t_1s || clock_time_exceed(t_1s, 100000)) {
        u32 t    = clock_time();
        int wptr = codec_get_InputWriteOffset();

        while (!clock_time_exceed(t, 30)) {
            int wptr1 = codec_get_InputWriteOffset();
            if (wptr != wptr1) {
                u32 tc = clock_time() | 1;
                if (t_1s) {
                    int tp0 = tc - t_1s;
                    int tp  = tp0 * 3;
                    as_samples += ((wptr1 - as_wptr) & (CODEC_INPUT_BUFF_LEN - 1)) / AUDIO_SAMPLE_SIZE;
                    int td = as_samples * 1000 - tp;

                    if (!td) {
                        td = 1;
                    }

                    tp      = tp / 8;
                    int ppm = (int)(td * (1000000 / 8) + tp / 2) / (int)tp;

                    if (async.ppm_i2s != ppm) {
                        async.ppm_i2s += ((ppm - async.ppm_i2s) * 15) >> 4;
                    }
                }
                t    = tc;
                wptr = wptr1;
                break;
            }
        }
        t_1s       = t;
        as_wptr    = wptr;
        as_samples = 0;


    } else {
        int wptr = codec_get_InputWriteOffset();
        as_samples += ((wptr - as_wptr) & (CODEC_INPUT_BUFF_LEN - 1)) / AUDIO_SAMPLE_SIZE;
        as_wptr = wptr;
    }

    if (async.st < 1) {
        async.st++;
        async.mic_num = ASRC_PPM_BUFFER_LEN;
        return 0;
    }

    int ppm_s = async.ppm_i2s;
    if (async.mic_num > ASRC_PPM_BUFFER_LEN) {
        ppm_s = async.ppm_i2s + (15);
    } else if (async.mic_num < ASRC_PPM_BUFFER_LEN) {
        ppm_s = async.ppm_i2s - (15);
    }
    if (ppm_s != async.ppm) {
        async.ppm     = ppm_s;
        async.ppm_set = 1;
    }
    //  my_dump_str_u32s (1, "Bar_A_diff1", async.ppm,async.ppm_i2s,ppm_s, async.mic_num);

    return 0;
}

_attribute_ram_code_ void app_audio_input_task(void)
{
    //////////codec input//////////////
    async.input_wptr = codec_get_InputWriteOffset();
    u32 input_rptr   = codec_get_InputReadOffset();

    u16 lenPcm1 = 0;

    if (async.input_wptr >= input_rptr) {
        lenPcm1 = async.input_wptr - input_rptr;
    } else {
        lenPcm1 = codec_get_InputBuffMaxlen() + async.input_wptr - input_rptr;
    }
    #if PPM_ASRC_EN
        //40*4 //The goal is to reduce the time consumed by the ASRC algorithm
        #if CODEC_MODE_SELECT == STEREO_MODE
            #define PRE_SAMPLE 40
        #else
            #define PRE_SAMPLE 30
        #endif

    #else
        #define PRE_SAMPLE 0
    #endif

    u16 lenPcm = lenPcm1 + PRE_SAMPLE * AUDIO_SAMPLE_SIZE;

    if ((abs(lenPcm - (APP_AUDIO_FRAME_SAMPLE * AUDIO_SAMPLE_SIZE))) > 32) {
        codec_set_InputReadOffset(async.input_wptr + PRE_SAMPLE * AUDIO_SAMPLE_SIZE);
        BLT_APP_LOG("error len:0x%x", async.input_wptr);
        return;
    }

    //get input data
    tcodec_type pcm32[APP_AUDIO_FRAME_SAMPLE + ASRC_PPM_BUFFER_LEN] = {0};

    #if PPM_ASRC_EN
    static tcodec_type asrc_buff[ASRC_PPM_BUFFER_LEN_TWO_CHN] = {0};
    tcodec_type        pcmData[APP_AUDIO_FRAME_SAMPLE + 64]   = {0};

    if (async.ppm_set) {
        async.ppm_set = 0;
        tlka_ppm_asrc_16_bit_init(&my_ppm_data, PPM_ASRC_CHN, async.ppm);
    }

    u32 asrc_pcm_len = lenPcm / AUDIO_SAMPLE_SIZE; //stereo mode
    for (int k = 0; k < 2; k++) {                  //900us loop one time
        u8  offset = k * APP_AUDIO_FRAME_SAMPLE_HALF;
        int nu     = APP_AUDIO_FRAME_SAMPLE_HALF;
        u32 ns     = (k == 0) ? APP_AUDIO_FRAME_SAMPLE_HALF : (asrc_pcm_len - APP_AUDIO_FRAME_SAMPLE_HALF);
        u32 n      = APP_AUDIO_FRAME_SAMPLE_HALF;

        tcodec_type pcm_ppm_data[APP_AUDIO_FRAME_SAMPLE_HALF + ASRC_PPM_BUFFER_LEN_TWO_CHN] = {0};
        //read input data
        log_task_end_irq(1, SL01_dbug0);
        codec_input_readData1((u8 *)(pcmData) + offset * AUDIO_SAMPLE_SIZE, ns * AUDIO_SAMPLE_SIZE, async.input_wptr + offset * AUDIO_SAMPLE_SIZE);
        //asrc
        nu = tlka_ppm_asrc_16_bit_frame_process(&my_ppm_data, (tcodec_type *)pcmData + offset, ns, (tcodec_type *)pcm_ppm_data);

        log_task_begin_irq(1, SL01_dbug0);
        for (int i = 0; i < async.mic_num; i++) {
            pcm32[i + offset] = asrc_buff[i];
        }

        tcodec_type *pd = pcm_ppm_data;

        for (int i = 0; i < (n - async.mic_num); i++) {
            pcm32[i + async.mic_num + offset] = *pd++;
        }

        int diff = nu - n;
        async.mic_num += diff;
        if ((async.mic_num > ASRC_PPM_BUFFER_LEN_TWO_CHN) || (async.mic_num < 0)) {
            async.mic_num = ASRC_PPM_BUFFER_LEN;
            u32 len       = (lenPcm / AUDIO_SAMPLE_SIZE - 480);
            my_dump_str_u32s(1, "mic num", async.mic_num, len, nu, ns);
        }
        for (int i = 0; i < async.mic_num; i++) {
            asrc_buff[i] = *pd++;
        }
    }
    #else
    codec_input_readData1((u8 *)pcm32, APP_AUDIO_FRAME_SAMPLE * AUDIO_SAMPLE_SIZE, async.input_wptr);
    #endif

    //encode
    #if ENCODE_EN
    u8  audio_enc[100]                    = {0};
    s16 audio_pcm[APP_AUDIO_FRAME_SAMPLE] = {0};

    for (u8 i = 0; i < 2; i++) {
        s16 *pdata = (s16 *)pcm32;
        for (u16 j = 0; j < APP_AUDIO_FRAME_SAMPLE; j++) {
            audio_pcm[j] = *(pdata + i);
            pdata += 2;
        }
        LC3ENC_Error ret_lc3 = lc3enc_encode_pkt(i, (u8 *)audio_pcm, audio_enc[i]);
        if (ret_lc3 != LC3ENC_OK) {
            BLT_APP_LOG("lc3 encode error:0x%x", ret_lc3);
        }
    }
    #endif

    memcpy((u8 *)async.playback, (u8 *)pcm32, APP_AUDIO_FRAME_SAMPLE * AUDIO_SAMPLE_SIZE);
}

_attribute_ram_code_ void app_audio_output_task(void)
{
    //////////codec output//////////////
    u32 output_rptr = codec_output_getReadOffset();
    u32 output_wptr = codec_output_getWriteOffset();
    u32 offset      = 0;
    if (output_wptr >= output_rptr) {
        offset = (output_wptr - output_rptr);
    } else {
        offset = (output_wptr + codec_get_OutputBufferLen() - output_rptr);
    }

    if ((offset < 4) || (offset > 20)) { //6 sample
        BLT_APP_LOG("buff reset:0x%x", offset);
        codec_output_setWriteOffset(output_rptr + 12);
    }

    u32 setBufferRet = codec_output_writeData((u8 *)async.playback, APP_AUDIO_FRAME_SAMPLE * AUDIO_SAMPLE_SIZE);

    if (setBufferRet != true) {
        BLT_APP_LOG("audio state error-fill buffer");
        return;
    }
}

#endif
