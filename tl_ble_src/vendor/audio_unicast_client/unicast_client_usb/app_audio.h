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
#if (UNICAST_CLIENT_SELECT == UNICAST_CLIENT_USB)

#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
#define APP_AUDIO_CONFIGURATION_PREFER           BLC_AUDIO_11II_SVR_2_SINK_2_CHN_1_SRC_2_CHN_1_CISES_2_STREAMS_4
#define APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER   BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_100BYTES
#define APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER  BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
#define APP_AUDIO_CONFIGURATION_PREFER           BLC_AUDIO_5_SVR_1_SINK_1_CHN_2_SRC_1_CHN_1_CISES_1_STREAMS_2
#define APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER   BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_100BYTES
#define APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER  BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES
#endif

#define APP_AUDIO_QOS_INPUT_PARAMETER_PREFER     BLC_AUDIO_STD_QOS_HIGH_RELIABILITY
#define APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER    BLC_AUDIO_STD_QOS_HIGH_RELIABILITY


/**
 *  @brief  codec(lc3)process enum,such as configure,release.
 */
typedef enum{
    APP_CODEC_IDLE,
    APP_CONFIG_CODEC,
    APP_RELEASE_CODEC,
}app_audio_codec_e;

/**
 *  @brief  app audio parameter
 */
typedef struct{
    bool  sS;//stream start
    u32   sT;//stream tick
    u32   pD;//presentation delay
    u32   location;
    u8    blocks;
    u16   rsvd;
    app_audio_codec_e codecOp;
    blc_audio_std_codec_settings_enum codecParam;
}app_audio_param_t;

/**
 *  @brief  app acl parameter
 */
typedef struct{
    u16 acl_handle;
    u16 rsvd;
    app_audio_param_t sink;
    app_audio_param_t source;
}app_acl_param_t;

/**
 *  @brief  app control parameter
 */
typedef struct{
    u8    acl_max_num;    //support max acl number
    u8    acl_count;      //current acl number
    bool  acl_csis_exist;
    u8    acl_csis_size;
    u8    acl_csis_sirk[16];
    app_acl_param_t aclParam[ACL_CENTRAL_MAX_NUM];
}app_ctrl_t;

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
void  app_audio_init(void);

/**
 * @brief      Audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void);

#endif


#endif

