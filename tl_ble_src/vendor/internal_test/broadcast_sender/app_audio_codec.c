/********************************************************************************************************
 * @file    app_audio_codec.c
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
#include "app_config.h"
#include "app.h"
#include "app_audio.h"
#include "application/usbstd/usb.h"
#include "stack/ble/ble.h"

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)


    #if (APP_AUDIO_INPUT_TYPE != APP_AUDIO_INPUT_USB_MIC)

        #if (MCU_CORE_TYPE == MCU_CORE_B92)
extern audio_codec_stream0_input_t audio_codec_input;
extern audio_codec_output_t        audio_codec_output;
        #endif

u32 gframeDataLen = 0;

void app_audio_codec_init(void)
{
    audio_sample_rate_e rate;
    switch (gbisSamplingFreq) {
    case BLC_AUDIO_FREQ_CFG_8000:
        rate          = AUDIO_8K;
        gframeDataLen = 80;
        break;
    case BLC_AUDIO_FREQ_CFG_16000:
        rate          = AUDIO_16K;
        gframeDataLen = 160;
        break;
    case BLC_AUDIO_FREQ_CFG_32000:
        rate          = AUDIO_32K;
        gframeDataLen = 320;
        break;
        //  case BLC_AUDIO_FREQ_CFG_44100:
        //      rate = AUDIO_44EP1K;
        //      break;
    case BLC_AUDIO_FREQ_CFG_48000:
        rate          = AUDIO_48K;
        gframeDataLen = 480;
        break;
    default:
        rate          = AUDIO_16K;
        gframeDataLen = 160;
        break;
    }
        #if (MCU_CORE_TYPE == MCU_CORE_B91)
            #if (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_IISIN)
    extern int gAudioIisConfig;
    gAudioIisConfig = I2S_S_CODEC_M;
    gpio_input_en(I2S_BCK_PC3 | I2S_DAC_LR_PC6 | I2S_DAC_DAT_PC7);
    ble_audio_codec_init(BLC_CODEC_SUBDEV_IIS, rate, STEREO_BIT_16);
            #else
    ble_audio_codec_init(BLC_CODEC_SUBDEV_DEF, rate, STEREO_BIT_16);
            #endif

            #if (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_AMIC)
    audio_init(AMIC_IN_TO_BUF_TO_LINE_OUT, rate, STEREO_BIT_16);
            #elif (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_LINEIN)
    audio_init(LINE_IN_TO_BUF_TO_LINE_OUT, rate, STEREO_BIT_16);
            #elif (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_DMIC)
    audio_set_dmic_pin(DMIC_GROUPD_D4_DAT_D5_D6_CLK);
    audio_init(DMIC_IN_TO_BUF_TO_LINE_OUT, rate, STEREO_BIT_16);
            #endif
        #elif (MCU_CORE_TYPE == MCU_CORE_B92)
            #if (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_AMIC)
    audio_codec_input.sample_rate  = AUDIO_16K;
    audio_codec_output.sample_rate = AUDIO_16K;
    audio_codec_input.input_src    = AMIC_STREAM0_STEREO;
    ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC_SPK, &audio_codec_input, &audio_codec_output);
            #elif (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_LINEIN)
    audio_codec_input.sample_rate  = AUDIO_16K;
    audio_codec_output.sample_rate = AUDIO_16K;
    audio_codec_input.input_src    = LINE_STREAM0_STEREO;
    ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC_SPK, &audio_codec_input, &audio_codec_output);
            #elif (APP_AUDIO_INPUT_TYPE == APP_AUDIO_INPUT_DMIC)
    audio_codec_input.sample_rate  = AUDIO_16K;
    audio_codec_output.sample_rate = AUDIO_16K;
    ble_audio_codec_init(BLC_CODEC_SUBDEV_MIC_SPK, &audio_codec_input, &audio_codec_output);
            #endif
        #endif
    ble_codec_setSpkBuffer((u8 *)gAppAudioBuffer, sizeof(gAppAudioBuffer));
    ble_codec_setMicBuffer((u8 *)gAppAudioBuffer, sizeof(gAppAudioBuffer));
    ble_codec_micOpen();
    ble_codec_spkOpen();
}

void app_audio_buffer_handle(void)
{
    u16 lenPcm = blc_codec_getMicDataLen();

    if (lenPcm < 4 * gframeDataLen) //STEREO_BIT_16 control
    {
        return;
    }

    u16 pcmData[APP_AUDIO_FRAME_BYTES];

    blc_codec_readMicBuff((u8 *)pcmData, 4 * gframeDataLen);

    if (!app_audio_check_send_flag()) {
        return;
    }

    u16 audioBuff[APP_AUDIO_FRAME_SAMPLE];
    u8  coded_raw[155]; //Max LC3 encode data
    for (int i = 0; i < gframeDataLen; i++) {
        audioBuff[i] = pcmData[2 * i];
    }
    lc3enc_encode_pkt(0, (u8 *)audioBuff, coded_raw);
    blc_iso_sendData(app_bisBcstHandle[0], coded_raw, gbisOctetPer);

        #if SUBGROUP_NUM_BIS > 1
    for (int i = 0; i < gframeDataLen; i++) {
        audioBuff[i] = pcmData[2 * i + 1];
    }
    //  lc3enc_encode_pkt(1, (u8*)audioBuff, coded_raw);
    blc_iso_sendData(app_bisBcstHandle[1], coded_raw, gbisOctetPer);
        #endif
}

    #endif


#endif /* INTER_TEST_MODE */
