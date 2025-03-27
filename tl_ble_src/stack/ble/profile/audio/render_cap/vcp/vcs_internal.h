/********************************************************************************************************
 * @file    vcs_internal.h
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

#define BLT_VCS_LOG(fmt, ...)           BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_VCS_LOG, "[VCS]"fmt, ##__VA_ARGS__)

#define VCS_MIN_VOLUME_SETTING                  0
#define VCS_MAX_VOLUME_SETTING                  255


/* Application error codes(VCS) */
enum{
    VCS_ERRCODE_INVALID_CHANGE_COUNTER      =   0x80,
    VCS_ERRCODE_OPCODE_NOT_SUPPORTED        =   0x81,
};

typedef enum{
    VCS_MUTE_STATE_NOT_MUTED                =   0x00,
    VCS_MUTE_STATE_MUTED,
} blc_vcs_mute_value_enum;

/*
 * VCS: ATT handle information: 72byte
 * VCS service entity supports a maximum of 4 AICS: refer to STACK_AUDIO_VCS_CLIENT_INCLUDE_AICS_INSTANCE_NUM
 * VCS service entity supports a maximum of 4 VOCS: refer to STACK_AUDIO_VCS_CLIENT_INCLUDE_VOCS_INSTANCE_NUM
 */
typedef struct{
    u16 baseHandle;
    u8 endHdl;
    u8 volStateHdl; //NTF
    u8 volCtrlPntHdl;
    u8 volFlagHdl; //NTF

} blt_vcs_att_hdl_t;

typedef struct {
    blt_vcs_att_hdl_t att;
    u8 aicsClientCnt;
    u8 vocsClientCnt;
//  blc_aics_hdl_info_t aicsHdl[STACK_AUDIO_VCS_CLIENT_INCLUDE_AICS_INSTANCE_NUM];
//  blc_vocs_hdl_info_t vocsHdl[STACK_AUDIO_VCS_CLIENT_INCLUDE_VOCS_INSTANCE_NUM];

} blt_vcs_nv_info_t;

int blt_vcpVolCtrl_init(u8 initType, const void* param);
int blt_vcpVolCtrl_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_vcpVolCtrl_discovery(u16 connHandle);
int blt_vcpVolCtrl_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);

int blt_vcss_init(u8 initType, const void* param);
int blt_vcss_connect(u16 connHandle, prf_acl_state_enum connState);

blc_vcp_client_t *blt_vcsc_getClientBuf(u8 instIdx);
void blt_vcp_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
blc_vcp_client_t *blt_vcp_getClientInst(u16 connHandle);

blc_vcp_server_t* blt_vcp_getServerInst(u16 connHandle);;
