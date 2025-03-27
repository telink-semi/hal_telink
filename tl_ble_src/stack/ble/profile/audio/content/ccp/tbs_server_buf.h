/********************************************************************************************************
 * @file    tbs_server_buf.h
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

typedef struct{
    u16 connHandle;
    bool active : 1;
    bool bearerProviderNameChanged : 1;
    bool bearerURISchemesSupportedListChanged : 1;
    bool bearerListCurrentCallsChanged : 1;
    bool incomingCallTargetBearerURIChanged : 1;
    bool callStateChanged : 1;
    bool incomingCallChanged : 1;
    bool callFriendlyNameChanged : 1;
} value_changed_conn_t;

typedef struct{
    u16 bearerProviderNameHandle;
    u16 bearerUCIHandle;
    u16 bearerTechnologyHandle;
    u16 bearerURISchemesSupportedListHandle;
    u16 bearerSignalStrengthHandle;
    u16 bearerSignalStrengthReportingIntervalHandle;
    u16 bearerListCurrentCallsHandle;
    u16 CCIDHandle;
    u16 statusFlagsHandle;
    u16 incomingCallTargetBearerURIHandle;
    u16 callStateHandle;
    u16 callControlPointHandle;
    u16 callControlPointOptionalOpcodesHandle;
    u16 terminatingReasonHandle;
    u16 incomingCallHandle;
    u16 callFriendlyNameHandle;
    u8 terminationReasonValue[2];
    value_changed_conn_t valueChanged[STACK_PRF_ACL_CONN_MAX_NUM];
    u16 alignment;
} blc_tbs_server_t, blc_gtbs_server_t;


typedef struct{

    blc_gtbs_server_t gtbs;

    u8 tbsServerCount;
    u8 reserved[3];
    blc_tbs_server_t *tbs[0];
} blc_ccp_server_t;

typedef struct blc_ccp_server_ctrl{
    blc_prf_proc_t process;
    blc_ccp_server_t ccpServer;
} blc_ccp_server_ctrl_t;


typedef struct{
    const u8 *uri;
    u16 uriLen;
} blc_tbss_uri_scheme_t;

typedef struct{
    const u8 *bearerProviderName;
    u16 bearerProviderNameLen;
    const u8 *bearerUci;
    u16 bearerUciLen;
    u8 bearerTechnology;
    const blc_tbss_uri_scheme_t *bearerUriSchemeList;
    u8 bearerUriSchemeListLen;
    u8 signalStrength;
    u8 CCID;
    blc_tbs_status_flags_t statusFlags;
} blc_gtbss_regParam_t, blc_tbss_regParam_t;

typedef struct{
    blc_gtbss_regParam_t gtbsParam;
} blc_ccps_regParam_t;
