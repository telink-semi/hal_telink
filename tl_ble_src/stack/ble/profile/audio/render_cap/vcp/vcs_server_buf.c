/********************************************************************************************************
 * @file    vcs_server_buf.c
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

const int gAppVcsSvrInclAicsInstNum = APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM;
const int gAppVcsSvrInclVocsInstNum = APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM;

#define VCS_DEFAULT_STEP                1
#define VCS_DEFAULT_VOLUME              20


#define AICS_INPUT_DESC_1               "Telink AICS Input Description 1"
#define AICS_INPUT_DESC_2               "Telink AICS Input Description 2"
#define AICS_INPUT_DESC_3               "Telink AICS Input Description 3"
#define AICS_INPUT_DESC_4               "Telink AICS Input Description 4"

#define VOCS_OUTPUT_DESC_1              "Telink VOCS Output Description 1"
#define VOCS_OUTPUT_DESC_2              "Telink VOCS Output Description 2"
#define VOCS_OUTPUT_DESC_3              "Telink VOCS Output Description 3"
#define VOCS_OUTPUT_DESC_4              "Telink VOCS Output Description 4"

static const blc_aicss_regParam_t defaultAicsParam[APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM] = {
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 0
    {
        .gainSetting = 1,
        .mute = AICS_MUTE_VALUE_MUTED,
        .gainMode = AICS_GAIN_MODE_VALUE_MANUAL,
        .units = 1,
        .minGain = -128,
        .maxGain = 127,
        .inputType = AICS_INPUT_TYPE_DIGITAL,
        .inputStatus = AICS_INPUT_STATUS_ACTIVE,
        .desc = AICS_INPUT_DESC_1,
    },
#endif
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 1
    {
        .gainSetting = 1,
        .mute = AICS_MUTE_VALUE_NOT_MUTED,
        .gainMode = AICS_GAIN_MODE_VALUE_MANUAL,
        .units = 1,
        .minGain = -128,
        .maxGain = 127,
        .inputType = AICS_INPUT_TYPE_DIGITAL,
        .inputStatus = AICS_INPUT_STATUS_ACTIVE,
        .desc = AICS_INPUT_DESC_2,
    },
#endif
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 2
    {
        .gainSetting = 1,
        .mute = AICS_MUTE_VALUE_NOT_MUTED,
        .gainMode = AICS_GAIN_MODE_VALUE_MANUAL,
        .units = 1,
        .minGain = -128,
        .maxGain = 127,
        .inputType = AICS_INPUT_TYPE_DIGITAL,
        .inputStatus = AICS_INPUT_STATUS_ACTIVE,
        .desc = AICS_INPUT_DESC_3,
    },
#endif
#if APP_AUDIO_VCS_INCLUDE_AICS_INSTANCE_NUM > 3
    {
        .gainSetting = 1,
        .mute = AICS_MUTE_VALUE_NOT_MUTED,
        .gainMode = AICS_GAIN_MODE_VALUE_MANUAL,
        .units = 1,
        .minGain = -128,
        .maxGain = 127,
        .inputType = AICS_INPUT_TYPE_DIGITAL,
        .inputStatus = AICS_INPUT_STATUS_ACTIVE,
        .desc = AICS_INPUT_DESC_4,
    },
#endif
};

static const blc_vocss_regParam_t defaultVocsParam[APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM] = {
#if APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM > 0
    {
        .location = BLC_AUDIO_LOCATION_FLAG_FL,
        .volumeOffset = 2,
        .desc = VOCS_OUTPUT_DESC_1,
    },
#endif
#if APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM > 1
    {
        .location = BLC_AUDIO_LOCATION_FLAG_FR,
        .volumeOffset = 2,
        .desc = VOCS_OUTPUT_DESC_2,
    },
#endif
#if APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM > 2
    {
        .location = BLC_AUDIO_LOCATION_FLAG_BL,
        .volumeOffset = 2,
        .desc = VOCS_OUTPUT_DESC_3,
    },
#endif
#if APP_AUDIO_VCS_INCLUDE_VOCS_INSTANCE_NUM > 3
    {
        .location = BLC_AUDIO_LOCATION_FLAG_BR,
        .volumeOffset = 2,
        .desc = VOCS_OUTPUT_DESC_4,
    },
#endif
};


const blc_vcss_regParam_t defaultVcpRendererParam = {
    .vcsParam = {
        .step = VCS_DEFAULT_STEP,
        .volume = VCS_DEFAULT_VOLUME,
        .mute = false,
    },
    .aicsParam = defaultAicsParam,
    .vocsParam = defaultVocsParam,
};






