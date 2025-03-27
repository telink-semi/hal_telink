/********************************************************************************************************
 * @file    pacs_server_buf.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

const blc_audio_pacParam_t defaultPac[] = {
    {
        LC3_CAP_16_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS_MEDIA,
    },
    {
        LC3_CAP_24_2(BLC_AUDIO_CHANNEL_COUNTS_1, 1),
        METADATA_CONTEXTS_MEDIA,
    },
};

const blc_pacss_regParam_t defaultPacsParam = {
    .sinkPacNum = 1,
    .sinkPac = defaultPac,
    .sinkAudioLocations = BLC_AUDIO_LOCATION_FLAG_FL,
    .sourcePacNum = 1,
    .sourcePac = defaultPac,
    .sourceAudioLocations = BLC_AUDIO_LOCATION_FLAG_FL,
    .availableSinkContexts = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED|BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA,
    .availableSourceContexts = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED|BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA,
    .supportedSinkContexts = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED|BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA,
    .supportedSourceContexts = BLC_AUDIO_CONTEXT_TYPE_UNSPECIFIED|BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL|BLC_AUDIO_CONTEXT_TYPE_MEDIA,
};




