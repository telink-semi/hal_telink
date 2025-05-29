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
#include "stack/ble/ble.h"

#include "app_buffer.h"
#include "app_config.h"
#include "app_audio.h"

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)

/*
 * typedef struct{
    u32   frequencyBitField;  //bit fields
    u8    frequencyValue;
    u8    durationBitField;   //bit fields
    u8    durationValue;
    u16   frameOctets;

    u8    codecFreq;
    u8    codecChannel;
    u8    codecSample;
    u8    codecSampleBytes;
}blc_audio_codec_settings_t;
 */
//{BLC_AUDIO_SUPP_FREQ_FLAG_16000,BLC_AUDIO_FREQ_CFG_16000, BLC_AUDIO_SUPP_DURATION_FLAG_10, BLC_AUDIO_DURATION_CFG_10,  40}
//u8  gbisSamplingFreq  = codecSettings[BIS_COMMON_CODEC_CONFIG].frequencyValue;
//u8  gbisFrameDuration = codecSettings[BIS_COMMON_CODEC_CONFIG].durationValue;
//u16 gbisOctetPer      = codecSettings[BIS_COMMON_CODEC_CONFIG].frameOctets;
u8  gbisSamplingFreq;
u8  gbisFrameDuration;
u16 gbisOctetPer;

u16  gAppAudioBuffer[APP_AUDIO_FRAME_BYTES * 2];
bool gAppAudioIsSend = false;

    #if (MCU_CORE_TYPE == MCU_CORE_B92)
audio_codec_stream0_input_t audio_codec_input =
    {
        .input_src     = DMIC_STREAM0_STEREO,
        .sample_rate   = AUDIO_48K,
        .fifo_num      = FIFO0,
        .data_width    = CODEC_BIT_16_DATA,
        .dma_num       = DMA2,
        .data_buf      = gAppAudioBuffer,
        .data_buf_size = sizeof(gAppAudioBuffer),
};
audio_codec_output_t audio_codec_output =
    {
        .output_src    = CODEC_DAC_STEREO,
        .sample_rate   = AUDIO_48K,
        .fifo_num      = FIFO0,
        .data_width    = CODEC_BIT_16_DATA,
        .dma_num       = DMA3,
        .mode          = HP_MODE,
        .data_buf      = gAppAudioBuffer,
        .data_buf_size = sizeof(gAppAudioBuffer),
};
    #endif


int app_audio_init(void)
{
    lc3enc_encode_init_bap(0, gbisSamplingFreq, gbisFrameDuration, gbisOctetPer);
    lc3enc_encode_init_bap(1, gbisSamplingFreq, gbisFrameDuration, gbisOctetPer);

    #if (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_USB_MIC)
    app_audio_usb_speak_init();
    #else
    app_audio_codec_init();
    #endif

    return 0;
}

void app_audio_handler(void)
{
    app_audio_send_handler();
}

bool app_audio_check_send_flag(void)
{
    return gAppAudioIsSend;
}

void app_audio_send_handler(void)
{
    #if (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_USB_MIC)
    app_audio_usb_speaker_handle();
    #else
    app_audio_buffer_handle();
    #endif
}


#endif /* INTER_TEST_MODE */
