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

#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)

    #define APP_AUDIO_CONFIGURATION_PREFER          BLC_AUDIO_5_SVR_1_SINK_1_CHN_2_SRC_1_CHN_1_CISES_1_STREAMS_2
    #define APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER  BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_100BYTES
    #define APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES

    #define APP_AUDIO_QOS_INPUT_PARAMETER_PREFER    BLC_AUDIO_STD_QOS_HIGH_RELIABILITY
    #define APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER   BLC_AUDIO_STD_QOS_HIGH_RELIABILITY

typedef enum
{
    APP_CODEC_IDLE,
    APP_CONFIG_CODEC,
    APP_RELEASE_CODEC,
} app_audio_codec_e;

typedef struct
{
    bool                              sS; //stream start
    u32                               wT; //write tick
    u32                               rT; //read  tick
    u32                               pD; //presentation delay
    u8                                blocks;
    u16                               rsvd;
    app_audio_codec_e                 codecOp;
    blc_audio_std_codec_settings_enum codecParam;
} app_audio_param_t;

typedef struct
{
    u16               acl_handle;
    u16               rsvd;
    app_audio_param_t sink;
    app_audio_param_t source;
} app_acl_param_t;

typedef struct
{
    u8  cC;      //channel counts
    u16 fSample; //sample each frame
    u16 fOctets; //octets each frame
    u16 frameUs; //time each frame,us conut
    u8  rsvd;
} app_codec_desc_t;

typedef struct
{
    u8               acl_max_num; //support max acl number
    u8               acl_count;   //current acl number
    app_codec_desc_t codecI;
    app_codec_desc_t codecO;
    app_acl_param_t  aclParam;
} app_ctrl_t;

/**
 * @brief      App audio init.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_init(void);

/**
 * @brief      App audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void);

#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)

#endif
