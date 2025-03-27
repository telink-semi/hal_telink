/********************************************************************************************************
 * @file    bass_server_buf.c
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




const u8 gAppBasssRecvStateCnt = APP_AUDIO_BASS_SERVER_RECV_STATE_CNT;

const blc_adv_broadcastId_t advDefBroadcastId = {
    .ltv.len = sizeof(blc_adv_broadcastId_t) -1,
    .ltv.type = DT_SERVICE_DATA_16BIT_UUID,
    .baasUuid = SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT,
    .broadcastId = {U24_TO_BYTES(DEFAULT_BROADCAST_ID)},
};

const blc_adv_broadcastSink_t advDefBroadcastSink = {
    .ltv.len = sizeof(blc_adv_broadcastSink_t) -1,
    .ltv.type = DT_SERVICE_DATA_16BIT_UUID,
    .bassUuid = SERVICE_UUID_BROADCAST_AUDIO_SCAN,
};




