/********************************************************************************************************
 * @file    aics_client_buf.h
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
#pragma once

#include "common/utility.h"

#define AICS_READ_INPUT_DESC_MAX_SIZE DATA_LENGTH_ALIGN4(50)

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;

    /* Characteristic value handle */
    u16 audioInStateHdl;          /* Audio Input State */
    u16 gainSettingPropertiesHdl; /* Gain Setting Properties */
    u16 audioInTypeHdl;           /* Audio Input Type */
    u16 audioInStatusHdl;         /* Audio Input Status */
    u16 audioInCtrlHdl;           /* Audio Input Control Point */
    u16 audioInDescHdl;           /* Audio Input Description */

    u16  connHandle;
    bool vcpInclFlag;
    bool micpInclFlag;

    /* ATT Characteristic values */
    blc_aics_audio_input_state_t       audioInState;
    blc_aics_gain_setting_properties_t gainSettingProperties;
    bool                               useFlag;
    u8                                 audioInType;
    u8                                 audioInStatus;
    u16                                audioInDescLen;
    u8                                 audioInDesc[AICS_READ_INPUT_DESC_MAX_SIZE];
} blc_aics_client_t;
