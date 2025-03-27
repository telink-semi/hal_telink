/********************************************************************************************************
 * @file    tmas_internal.h
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


#define BLT_TMAS_LOG(fmt, ...)          BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_TMAS_LOG, "[TMAS]"fmt, ##__VA_ARGS__)

/*
 * TMAS: ATT handle information: 4byte
 */
typedef struct {
    u16 tmasRoleHdl;
} blt_tmas_nv_info_t;

int blt_tmasc_init(u8 initType, const void* param);
int blt_tmasc_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_tmasc_discovery(u16 connHandle);
int blt_tmasc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);

blc_tmas_client_t *blt_tmasc_getClientBuf(u8 instIdx);

int blt_tmass_init(u8 initType, const void* param);

