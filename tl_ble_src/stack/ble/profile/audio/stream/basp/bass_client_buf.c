/********************************************************************************************************
 * @file    bass_client_buf.c
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




const u8 gAppBasscRecvStateNum = APP_AUDIO_BASS_CLIENT_RECV_STATE_CNT;

const u8 gAppBasscRecvStateMaxSize = APP_AUDIO_BASS_RECV_STATE_MAX_SIZE;

_attribute_ble_data_retention_
blc_bassc_recvStateParamEntity_t gBassCRecvState[ACL_CENTRAL_MAX_NUM * APP_AUDIO_BASS_CLIENT_RECV_STATE_CNT];

_attribute_ble_data_retention_
blc_bass_client_t gBassClient[ACL_CENTRAL_MAX_NUM];


blc_bass_client_t* blc_bassc_getClientBuf(u8 index)
{
    return &gBassClient[index];
}

blc_bassc_recv_state_param_t* blc_bassc_getRecvStateBuf(u8 index)
{
    return (blc_bassc_recv_state_param_t*)&gBassCRecvState[index];
}




