/********************************************************************************************************
 * @file    bass_server_buf.h
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

#ifndef DEFAULT_BROADCAST_ID
#define DEFAULT_BROADCAST_ID            0x123456
#endif

typedef struct{
    /* Service handle range */
    u16 bassCtrlHandle;
    u16 pastConnHandle;
    u8 sourceId;
    u8 bcstRcvStateCnt;
    u16 pastTimer;
    u16 recvStateHandle[STACK_AUDIO_BASS_RECV_STATE_NUM];
    u32 pastStartTimer;

} blc_bass_server_t;

typedef struct blc_bass_server_ctrl{
    blc_prf_proc_t process;
    blc_bass_server_t bassServer;
} blc_bass_server_ctrl_t;

typedef struct{
    u16 pastTimer;  //unit ms
} blc_basss_regParam_t;

typedef struct{
    blc_adv_ltv_t ltv;
    u16 baasUuid;
    u8 broadcastId[3];
} blc_adv_broadcastId_t;

typedef struct{
    blc_adv_ltv_t ltv;
    u16 bassUuid;
} blc_adv_broadcastSink_t;

extern const blc_adv_broadcastId_t advDefBroadcastId;
extern const blc_adv_broadcastSink_t advDefBroadcastSink;

