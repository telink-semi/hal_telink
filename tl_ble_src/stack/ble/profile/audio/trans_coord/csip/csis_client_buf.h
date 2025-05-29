/********************************************************************************************************
 * @file    csis_client_buf.h
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

#include "stack/ble/profile/services/svc_audio/csis/svc_csis.h"

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;

    /* Characteristic value handle */
    u16 setIdentityResolvingKeyHdl; /* set Identity Resolving Key */
    u16 coordinatedSetSizeHdl;      /* Coordinated Set Size */
    u16 setMemberLockHdl;           /* SetMember Lock */
    u16 setMemberRankHdl;           /* SetMember Rank */

    svc_csis_SIRK_t sirk;
    u8              coordinatedSetSize;
    u8              lock;
    u8              rank;

} blc_csis_client_t;

typedef struct blc_csis_client_ctrl
{
    blc_prf_proc_t     process;
    blc_csis_client_t *pCsisClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
} blc_csis_client_ctrl_t;

typedef struct
{
} blc_csisc_regParam_t;
