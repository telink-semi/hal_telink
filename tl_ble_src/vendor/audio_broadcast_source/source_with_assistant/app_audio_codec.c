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
#include "../source_config.h"
#if (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)

#include "app_config.h"
#include "app_audio.h"
#include "stack/ble/ble.h"
#include "vendor/common/tlk_api/tlk_codec.h"
#if APP_AUDIO_INPUT_MODE <= APP_AUDIO_INPUT_CODEC_ENDING

unsigned short gAppAudioBuffer[APP_AUDIO_FRAME_BYTES *2];

/**
 * @brief       audio initial codec function
 * @param[in]   none
 * @return      none
 */
void app_audio_initCodec(void)
{
    blc_audio_codecSpecCfgParam_t *codecCfg = &bisSource.BASE.BIG_param[0].codecCfg;

    if(APP_AUDIO_INPUT_MODE <= APP_AUDIO_INPUT_CODEC_ENDING)
    {
        tlk_codec_init();

#if(APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_AMIC)
        tlk_codec_config(TLK_CODEC_INPUT, codecCfg->samplingFreq, TLK_CODEC_2_CHANNEL, TLK_CODEC_MIC, (u8*)gAppAudioBuffer, sizeof(gAppAudioBuffer));
#elif(APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_LINEIN)
        tlk_codec_config(TLK_CODEC_INPUT, codecCfg->samplingFreq, TLK_CODEC_2_CHANNEL, TLK_CODEC_LINE, (u8*)gAppAudioBuffer, sizeof(gAppAudioBuffer));
#endif

    }
}

/**
 * @brief       codec audio clean rx Buffer.
 * @param[in]   none
 * @return      none
 */
void app_audio_cleanCodecRxBuffer(void)
{
    tlk_codec_start(TLK_CODEC_INPUT);
}

/**
 * @brief       codec audio get pcm data.
 * @param[in]   none
 * @return      none
 */
void app_audio_getCodecData(u16* pcm)
{
    if(tlk_codec_input_dataPop((u8*)pcm, 4*codecFrameDataLen) != TLK_CODEC_SUCCESS)
    {
        memset(pcm, 0, 4*codecFrameDataLen);
    }
}


#endif

#endif      //SOURCE_VERSION == SOURCE_WITH_ASSISTANT

