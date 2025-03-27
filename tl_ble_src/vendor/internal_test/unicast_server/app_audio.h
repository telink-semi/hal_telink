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

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)

typedef enum{
    APP_NONE               = 0,
    APP_EP_RECEIVE_START = BIT(1),
    APP_EP_RECEIVE_STOP  = BIT(2),
    APP_EP_RELEASE         = BIT(3),
}ep_operation_e;

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

typedef struct{
    u8    epId;            //endpoint ID
    bool  sS;              //stream start
    u16   rsvd;
    u32   sT;              //stream tick
    u32   pD;              //presentation delay
    app_codec_param_t cP;  //codec Param
    ep_operation_e epOp;   //endpoint operation
}app_audio_param_t;

typedef struct{
    u8   cC;       //channel counts
    u16  fSample;  //sample each frame
    u16  fOctets;  //octets each frame
    u16  frameUs;  //time each frame,us conut
}app_codec_desc_t;

typedef struct{
    u16    aclHandle;
    u8     rsi[6];
    bool   configCodecIdx;
    app_codec_desc_t    codecI;
    app_codec_desc_t    codecO;
    app_audio_param_t   source[APP_AUDIO_MAX_SOURCE_EP];
    app_audio_param_t   sink[APP_AUDIO_MAX_SINK_EP];
}app_audio_ctrl_t;

/**
 * @brief       Initialization audio concerned modules
 * @param[in]   none
 * @return      none
 */
void app_audio_init(void);


/**
 * @brief       Audio main loop
 * @param[in]   none
 * @return      none
 */
void app_audio_handler(void);


/**
 * @brief       Audio timer IRQ for render
 * @param[in]   none
 * @return      none
 */
void app_timer_irq_proc(void);


#endif /* INTER_TEST_MODE */

#endif
