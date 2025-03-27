/********************************************************************************************************
 * @file    mics_client_buf.h
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


typedef struct {

    gattc_sub_ccc_msg_t ntfInput;

    /* Characteristic value handle */
    u16 muteHdl;

    /* Characteristic value */
    u8  muteValue;
    u8 reserved[1];

} blc_mics_client_t;

typedef struct {
    blc_mics_client_t micsClient;

    /* AICS instances number */
    u8 aicsClientCnt;
    u8 aicsClientIdx;
    u8 reserved[2];
    /* MICP_MIC_CTRL may include zero or more instances of AICS Client */
    blc_aics_client_t *pAicsClient[STACK_AUDIO_MICS_CLIENT_INCLUDE_AICS_INSTANCE_NUM];
} blc_micp_client_t;

typedef struct blc_micp_client_ctrl{
    blc_prf_proc_t process;
    blc_micp_client_t *pMicpClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
} blc_micp_client_ctrl_t;

typedef struct {

} blc_micsc_regParam_t;
