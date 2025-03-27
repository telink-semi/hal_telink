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
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_ASYNC)


/**
 *  @brief  app audio codec parameter
 */
typedef struct{
    bool  paramReady;
    u8    frequency;
    u8    duration;
    u8    blocks;
    u32   frameOcts;
    u32   location;
    blc_audio_codec_id_t  codecId;
    u8    rsvd[3];
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
    bool   configCodecIdx;
    u8     leaRole;//ACL_ROLE_PERIPHERAL or ACL_ROLE_CENTRAL
    u16    asyncHandle;
    u16    rsvd;
    u8     mic_reset;
    u8     spk_reset;
    app_audio_param_t   source[APP_AUDIO_MAX_SOURCE_EP];
    app_audio_param_t   sink[APP_AUDIO_MAX_SINK_EP];
    u8     sirkCfg[16];
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

#if (TLK_TONE_ENABLE)
_attribute_ram_code_ void app_audio_tone_handle_task(void);
#endif

#endif
#endif
