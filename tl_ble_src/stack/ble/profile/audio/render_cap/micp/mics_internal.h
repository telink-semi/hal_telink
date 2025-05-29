/********************************************************************************************************
 * @file    mics_internal.h
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

#define BLT_MICS_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_MICS_LOG, "[MICS]" fmt, ##__VA_ARGS__)

typedef enum
{
    MICS_ERRCODE_MUTE_DISABLED = 0x80,
} blt_mics_error_code_enum;

/*
 * MICS: ATT handle information: 41byte
 * MICS service entity supports a maximum of 4 AICS: refer to STACK_AUDIO_MICS_CLIENT_INCLUDE_AICS_INSTANCE_NUM
 */
typedef struct
{
    u16 baseHandle;
    u8  endHdl;
    u8  muteHdl; //NTF
} blt_mics_att_hdl_t;

typedef struct
{
    blt_mics_att_hdl_t att;
    u8                 aicsClientCnt;
} blt_mics_nv_info_t;

int blt_micsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);

int blt_micsc_init(u8 initType, const void *param);
int blt_micsc_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_micsc_discovery(u16 connHandle);


blc_micp_client_t *blt_micsc_getClientBuf(u8 instIdx);
blc_micp_client_t *blt_micp_getClientInst(u16 connHandle);
void               blt_micp_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);


int blt_micss_init(u8 initType, const void *param);
int blt_micss_connect(u16 connHandle, prf_acl_state_enum connState);
