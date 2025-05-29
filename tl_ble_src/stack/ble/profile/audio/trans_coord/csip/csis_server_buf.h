/********************************************************************************************************
 * @file    csis_server_buf.h
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
    u16 SIRKHandle;
    u16 CSSizeHandle;
    u16 memberLockHandle;
    u16 memberRankHandle;
    u8  rsi[6];
    u16 memberLockedConnHandle;

    u8 memberLockedTimeout; //Unit time:1s
    u8 type;                //SIRK type
    u8 reserved[2];
    u8 plainSIRK[16];

    u32 memberLockedTimer;

} blc_csis_server_t;

typedef struct blc_csis_server_ctrl
{
    blc_prf_proc_t    process;
    blc_csis_server_t server;
} blc_csis_server_ctrl_t;

typedef struct
{
    u8 setSize;       //Coordinated Set Size:1-255
    u8 setRank;       //Set Member Rank, must less than or equal to setSize

    u8 SIRK_type;     //exposes SIRK type, 0:plain text, 1:Encrypted, 2:only OOB
    u8 SIRK[16];      //Set Identity Resolving Key

    u8 lockedTimeout; //Unit time:1s, Parameter suggestion range:10-200

} blc_csiss_regParam_t;
