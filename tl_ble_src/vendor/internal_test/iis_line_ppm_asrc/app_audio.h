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

#include "tl_common.h"
#include "app_config.h"


#if (INTER_TEST_MODE == TEST_PPM_ASRC_WITH_IIS_LINEIN)

#define APP_AUDIO_INPUT_BUFFER_SIZE                 2048
#define APP_AUDIO_INPUT_FRAME_SAMPLE_MAX             480
#define APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX       155

#define APP_AUDIO_OUTPUT_BUFFER_SIZE                2048
#define APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX            480
#define APP_AUDIO_OUTPUT_FRAME_ENCODE_BYTES_MAX       40


#define APP_AUDIO_FRAME_SAMPLE                      480
#define APP_AUDIO_FRAME_SAMPLE_BYTES                APP_AUDIO_FRAME_SAMPLE<<1


#define ASRC_OFFSET0_TICK                           500//us
#define ASRC_OFFSET1_TICK                           9500//us

typedef struct app_ble_sync_st
{
    // 00
    u32 task_tick;
    u32 input_wptr;

    s16 mic_num;
    u8  ppm_set;
    u8  st;

    int ppm;

    int ppm_i2s;
    u32 t_sample;


    s16 playback[APP_AUDIO_INPUT_FRAME_SAMPLE_MAX*2];

} __attribute__ ((aligned (4))) app_ble_sync_st_t;

extern app_ble_sync_st_t    async;

void app_audio_init(void);
void app_audio_input_task(void);
void app_audio_output_task(void);
int asrc_i2s_48k_ppm (void);
#endif

#endif /* APP_AUDIO_H_ */
