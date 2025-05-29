/********************************************************************************************************
 * @file    prf_cs.h
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

typedef enum
{
    CS_CLIENT_START = PRF_CHANNEL_SOUNDING_CLIENT_START - 1,
    CS_RAS_CLIENT,

    CS_SERVER_START = CS_CLIENT_START + PRF_SERVER_OFFSET,
    CS_RAS_SERVER,
} channel_souding_service_role_enum;

typedef enum
{
    CS_EVT_TYPE_CLIENT_START = PRF_EVTID_CHANNEL_SOUNDING_START,
    CS_EVT_TYPE_RASC,
    CS_EVT_TYPE_SERVER_START = PRF_EVTID_CHANNEL_SOUNDING_START + PRF_EVENT_ID_SIZE * PRF_SERVER_OFFSET,
    CS_EVT_TYPE_RASS,
} channel_sounding_event_enum;

#include "rasp/ras.h"
