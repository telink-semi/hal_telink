/********************************************************************************************************
 * @file    bass_client_buf.h
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

typedef struct
{
    u16 len;
    u8  recvState[APP_AUDIO_BASS_RECV_STATE_MAX_SIZE];
} blc_bassc_recvStateParamEntity_t;

typedef struct
{
    u16 len;
    u8  recvState[];
} blc_bassc_recv_state_param_t;

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;
    u16                 bassCtrlHandle;
    u16                 recvStateHandle[STACK_AUDIO_BASS_RECV_STATE_NUM];

    /* Service handle range */
    u16 connHandle;
    u8  bcstRcvStateCnt;
    u8  bcstRcvStateIdx;
    u8  reserved[2];

    blc_bassc_recv_state_param_t *pRecvState[STACK_AUDIO_BASS_RECV_STATE_NUM];
} blc_bass_client_t;

typedef struct blc_bass_client_ctrl
{
    blc_prf_proc_t     process;
    blc_bass_client_t *pBassClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
} blc_bass_client_ctrl_t;

typedef struct
{
} blc_bassc_regParam_t;

blc_bassc_recv_state_param_t *blc_bassc_getRecvStateBuf(u8 index);
blc_bass_client_t            *blc_bassc_getClientBuf(u8 index);
