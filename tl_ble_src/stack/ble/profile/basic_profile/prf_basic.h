/********************************************************************************************************
 * @file    prf_basic.h
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

#define BLC_BASIC_PRF_LOG                       BLC_PROFILE_DEBUG

/*
 * It is used to implement profiles commonly used before Bluetooth LE Core v5.2,
 * usually these are the Basic services of LE devices, so they are also called basic profiles.
 * all profile: Generic Attribute Service(GATT), Generic Access Profile Service(GAP), Device Information Service(DIS),
 * Battery Service(BAS), Scan Parameters Service(ScPS).
 */

enum{
    BASIC_CLIENT_START = PRF_BASIC_CLIENT_START - 1,
    GATT_SERVICE_CLIENT,
    GAP_SERVICE_CLIENT,
    DIS_CLIENT,
    BAS_CLIENT,
    SCPS_CLIENT,

    BASIC_SERVER_START = BASIC_CLIENT_START + PRF_SERVER_OFFSET,
    GATT_SERVICE_SERVER,
    GAP_SERVICE_SERVER,
    DIS_SERVER,
    BAS_SERVER,
    SCPS_SERVER,
};


enum{
    BASIC_EVT_TYPE_CLIENT_START = PRF_EVTID_BASIC_START,
    BASIC_EVT_TYPE_GATT_SERVICE_CLIENT = BASIC_EVT_TYPE_CLIENT_START,
    BASIC_EVT_TYPE_GAP_SERVICE_CLIENT = BASIC_EVT_TYPE_GATT_SERVICE_CLIENT + PRF_EVENT_ID_SIZE,
    BASIC_EVT_TYPE_DIS_CLIENT = BASIC_EVT_TYPE_GAP_SERVICE_CLIENT + PRF_EVENT_ID_SIZE,
    BASIC_EVT_TYPE_BAS_CLIENT = BASIC_EVT_TYPE_DIS_CLIENT + PRF_EVENT_ID_SIZE,
    BASIC_EVT_TYPE_SCPS_CLIENT = BASIC_EVT_TYPE_BAS_CLIENT + PRF_EVENT_ID_SIZE,

    BASIC_EVT_TYPE_SERVER_START = PRF_EVTID_BASIC_START + PRF_EVENT_ID_SIZE*PRF_SERVER_OFFSET,
    BASIC_EVT_TYPE_GATT_SERVICE_SERVER = BASIC_EVT_TYPE_CLIENT_START,
    BASIC_EVT_TYPE_GAP_SERVICE_SERVER = BASIC_EVT_TYPE_GATT_SERVICE_SERVER + PRF_EVENT_ID_SIZE,
    BASIC_EVT_TYPE_DIS_SERVER = BASIC_EVT_TYPE_GAP_SERVICE_SERVER + PRF_EVENT_ID_SIZE,
    BASIC_EVT_TYPE_BAS_SERVER = BASIC_EVT_TYPE_DIS_SERVER + PRF_EVENT_ID_SIZE,
    BASIC_EVT_TYPE_SCPS_SERVER = BASIC_EVT_TYPE_BAS_SERVER + PRF_EVENT_ID_SIZE,
};


#include "bas/bas.h"
#include "dis/dis.h"
#include "scps/scps.h"

