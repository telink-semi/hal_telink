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
#if (SINK_VERSION == SINK_WITH_ASSISTANT_VERSION)

#pragma once

/**
 *  @brief  app audio event callback parameter.
 */
typedef struct{
    int evtCode;
    void (*evtCb)(u8 *p, int n);
} app_audio_controllerEvtCb_t;

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
 * @brief      BLE controller event handler call-back in audio.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_audio_controllerEventCallBack(u32 h, u8 *p, int n);


void app_audio_selectBroadcastSource(bool up);



#endif      //SINK_VERSION == SINK_WITH_ASSISTANT_VERSION
