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
#include "../sink_config.h"
#if (SINK_VERSION == SINK_ONLY_VERSION)

    #pragma once

    #define VOLUME_INITIAL_VALUE           20
    #define MUTE_INITIAL_VALUE             false
    #define VLOUME_STEP_INITIAL_VALUE      20
    #define LEFT_VOL_OFFSET_INITIAL_VALUE  0
    #define RIGHT_VOL_OFFSET_INITIAL_VALUE 0

/**
 *  @brief  app audio event callback parameter.
 */
typedef struct
{
    audio_event_enum id;
    int (*evtCb)(u16 connHandle, u8 *pData, u16 dataLen);
} app_audio_evtCb_t;

/**
 *  @brief  app sink VCP state parameter.
 */
typedef struct
{
    u8   volume;
    bool mute;
    s16  volOffset[APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM];
} appSinkVcpState_t;

/**
 * @brief       audio initial function.
 * @param[in]   none.
 * @return      none
 */
void app_audio_init(void);

/**
 * @brief      audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void);

/**
 * @brief       set volume and mute state.
 * @param[in]   volume: volume value.
 * @param[in]   mute: mute state.
 * @return      none
 */
void app_setVolState(u8 volume, bool mute);

/**
 * @brief       set volume up and send volume state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_volUp(void);

/**
 * @brief       set volume down and send volume state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_volDown(void);

/**
 * @brief       send mute state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_mute(void);

/**
 * @brief       send unmute state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_unmute(void);

/**
 * @brief       change mute state and send mute/unmute state to remote device.
 * @param[in]   none
 * @return      none
 */
void app_send_changeMuteState(void);

#endif //SINK_VERSION == SINK_ONLY_VERSION
