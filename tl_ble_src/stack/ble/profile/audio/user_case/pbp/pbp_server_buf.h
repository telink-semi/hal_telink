/********************************************************************************************************
 * @file    pbp_server_buf.h
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

#include "stack/ble/profile/services/svc_adv.h"

#ifndef DEFAULT_BROADCAST_NAME
#define DEFAULT_BROADCAST_NAME                      "B91M_BROADCAST_NAME"
#endif

typedef struct{
    blc_adv_ltv_t ltv;
    u8 bcastName[32];
} blc_adv_broadcastName_t;

typedef struct{
    blc_adv_ltv_t ltv;
    u16 pbasUuid;
    u8 feature;
    u8 metadataLen;
    u8 metadata[0];
} blc_adv_pbpFeature_t;

extern const blc_adv_broadcastName_t advDefBcastName;
extern blc_adv_pbpFeature_t advDefPbpFeature;
