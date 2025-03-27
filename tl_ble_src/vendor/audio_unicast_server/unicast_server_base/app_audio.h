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
#ifndef APP_AUDIO_H_
#define APP_AUDIO_H_
#include "app_config.h"
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_BASE)

#define VOLUME_INITIAL_VALUE                150
#define MUTE_INITIAL_VALUE                  false
#define VLOUME_STEP_INITIAL_VALUE           15

/**
 *  @brief  app audio codec parameter
 */
typedef struct{
    bool  paramReady;
    u8    frequency;
    u8    duration;
    u32   frameOcts;
    u32   location;
    u8    blocks;
    blc_audio_codec_id_t  codecId;
    u8    rsvd;
}app_codec_param_t;

/**
 *  @brief  app audio parameter
 */
typedef struct{
    u8    epId;            //endpoint ID
    bool  sS;              //stream start
    u16   rsvd;
    u32   sT;              //stream tick
    u32   pD;              //presentation delay
    app_codec_param_t cP;  //codec Param
}app_audio_param_t;

/**
 *  @brief  app audio control parameter
 */
typedef struct{
    u16    aclHandle;
    u8     volume;
    bool   mute;
    bool   configCodecIdx;
    u8     rsvd;
    u8     mic_reset;
    u8     spk_reset;
    app_audio_param_t   source[APP_AUDIO_MAX_SOURCE_EP];
    app_audio_param_t   sink[APP_AUDIO_MAX_SINK_EP];
#if (TLK_TONE_ENABLE)
    u32 is_tone_codec_cfg;  // currently configuration is tone codec
    u32 tone_len;
    s16 tone_buff[480];
#endif
}app_audio_ctrl_t;

/**
 *  @brief  app audio event callback parameter
 */
typedef struct{
    audio_event_enum id;
    int (*evtCb)(u16 connHandle, void *pAudEvt);
} app_audio_evtCb_t;

/**
 * @brief      Audio module init.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_init(void);


/**
 * @brief      Audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void);

void app_audio_volume_up(void);
void app_audio_volume_down(void);
void app_audio_mute(void);
void app_audio_unmute(void);

/**
 * @brief  set audio volume
 *
 * @param[in]  volSetting: vol setting
 * @param[in]  sample: audio data sample number
 *
 * @returns none
 */
_attribute_ram_code_ void app_audio_volume_set(u8 volSetting);

/**
 * @brief  set audio mute
 *
 * @param[in]  mute: mute setting
 * TRUE: mute, FALSE: unmute
 *
 * @returns none
 */
_attribute_ram_code_ void app_audio_mute_set(bool mute);

/**
 * @brief  control audio volume
 *
 * @param[in]  p: audio data
 * @param[in]  sample: audio data sample number
 *
 * @returns none
 */
_attribute_ram_code_ void app_audio_control_volume(int16_t *p, uint16_t sample);

#if (TLK_TONE_ENABLE)
_attribute_ram_code_ void app_audio_tone_handle_task(void);
#endif

#endif
#endif
