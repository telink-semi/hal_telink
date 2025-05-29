/********************************************************************************************************
 * @file    vocs_client_buf.h
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

#define VOCS_READ_OUTPUT_DESC_MAX_SIZE DATA_LENGTH_ALIGN4(50)

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;

    /* Characteristic value handle */
    u16 volOffStateHdl;   /* Volume Offset State */
    u16 audioLocationHdl; /* Audio Location */
    u16 volOffCtrlpntHdl; /* Volume Offset Control Point */
    u16 audioOutDescHdl;  /* Audio Output Description */

    /* Characteristic value */
    blc_vocs_volume_offset_state_t volOffsetState;
    bool                           useFlag;
    u32                            audioLocation;
    u16                            audioOutDescLen;
    u16                            reserved2;
    u8                             audioOutDesc[VOCS_READ_OUTPUT_DESC_MAX_SIZE];

} blc_vocs_client_t;

typedef struct
{
} blc_vocsc_regParam_t;
