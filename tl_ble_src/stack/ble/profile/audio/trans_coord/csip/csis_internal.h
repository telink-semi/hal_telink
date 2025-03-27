/********************************************************************************************************
 * @file    csis_internal.h
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

#define BLT_CSIS_LOG(fmt, ...)          BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_CSIS_LOG, "[CSIS]"fmt, ##__VA_ARGS__)

typedef enum{
    BLT_CSIS_ERROR_CODE_LOCK_DENIED                 = 0x80, //The lock cannot be granted because the server is already locked.
    BLT_CSIS_ERROR_CODE_LOCK_RELEASE_NOT_ALLOWED    = 0x81,
    BLT_CSIS_ERROR_CODE_INVALID_LOCK_VALUE          = 0x82,
    BLT_CSIS_ERROR_CODE_OOB_SIRK_ONLY               = 0x83,
    BLT_CSIS_ERROR_CODE_LOCK_ALREADY_GRANTED        = 0x84, //The client that made the request is the current owner of the lock.
} blt_csis_error_code_enum;


/*
 * CSIS: ATT handle information: 7byte
 */
typedef struct {
    u16 baseHandle;
    u8 endHdl;
    u8 setIdentityResolvingKeyHdl; //NTF
    u8 coordinatedSetSizeHdl; //NTF
    u8 setMemberLockHdl; //NTF
    u8 setMemberRankHdl;
} blt_csis_att_hdl_t;

typedef struct {
    blt_csis_att_hdl_t att;
} blt_csis_nv_info_t;

blc_csis_client_t *blt_csisc_getClientBuf(u8 instIdx);
int blt_csisc_init(u8 initType, const void* param);
int blt_csisc_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_csisc_discovery(u16 connHandle);
int blt_csisc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);

int blt_csiss_init(u8 initType, const void* param);

int blt_csiss_connect(u16 connHandle, prf_acl_state_enum connState);

int blt_csiss_loop(u16 connHandle);

void blt_csis_cryptoSIRKEncDec(u16 connHandle, u8* in, u8* out);
