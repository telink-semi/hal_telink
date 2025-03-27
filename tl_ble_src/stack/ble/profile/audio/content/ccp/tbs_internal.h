/********************************************************************************************************
 * @file    tbs_internal.h
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

#define BLT_TBS_LOG(fmt, ...)           BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_TBS_LOG, "[(G)TBS]"fmt, ##__VA_ARGS__)

/* Application error codes(TBS/GTBS) */
enum{
    GTBS_ERRCODE_VALUE_CHANGED_DURING_READ_LONG     =   0x80,
};

typedef enum  {
    GTBS_READ_BEARER_PROVIDER_NAME,             //Read Bearer Provider Name
    GTBS_READ_BEARER_UCI,                       //Read Bearer UCI
    GTBS_READ_BEARER_TECHNOLOGY,                //Read Bearer Technology
    GTBS_READ_BEARER_URI_SCHEMES_SUPP_LIST,     //Read Bearer URI Schemes Supported List
    GTBS_READ_BEARER_SIGNAL_STRENGTH,           //Read Bearer Signal Strength
    GTBS_READ_BEARER_SIGNAL_STRENGTH_RPT_ITVL,  //Read Bearer Signal Strength Reporting Interval
    GTBS_READ_BEARER_LIST_CURRENT_CALLS,        //Read Bearer List Current Calls
    GTBS_READ_CONTENT_CONTROL_ID,               //Read Content Control ID
    GTBS_READ_INCOMING_CALL_TARGET_BEARER_URI,  //Read Incoming Call Target Bearer URI
    GTBS_READ_STATUS_FLAGS,                     //Read Status Flags
    GTBS_READ_CALL_STATE,                       //Read Call State
    GTBS_READ_CALL_CONTROL_POINT_OPT_OPCODES,   //Read Call Control Point Optional Opcodes 4.4.14 O
    GTBS_READ_INCOMING_CALL,                    //Read Incoming Call 4.4.15 O
    GTBS_READ_CALL_FRIENDLY_NAME,               //Read Call Friendly Name 4.4.16 O
    GTBS_READ_MAX,
} blt_gtbs_read_enum;

typedef struct {
    u8 opcode;
    u8 *pData;
} blt_gtbs_ccp_op_t;

/* Call Control Point Notification format */
typedef struct {
    u8 resultOpcode;
    u8 callIndex;
    u8 resultCode;
} blt_gtbs_ccp_ntf_t;

/*
 * GTBS: ATT handle information: 19byte
 */
typedef struct{
    u16 baseHandle;
    u8 endHdl;
    u8 bearerProviderNameHdl; //NTF
    u8 bearerUCIHdl;
    u8 bearerTechnologyHdl; //NTF
    u8 bearerURISchemesSuppListHdl; //NTF
    u8 bearerSignalStrengthHdl;  //NTF
    u8 reportingIntervalHdl;
    u8 bearerListCurrentCallsHdl; //NTF
    u8 ccidHdl;
    u8 statusFlagsHdl; //NTF
    u8 incomingCallTargetBearerURIHdl; //NTF
    u8 callStateHdl; //NTF
    u8 callControlPointHdl;
    u8 callControlPointOptionalOpHdl;
    u8 terminationReasonHdl; //NTF
    u8 incomingCallHdl;  //NTF
    u8 callFriendlyNameHdl; //NTF
} blt_gtbs_att_hdl_t;

typedef struct {
    blt_gtbs_att_hdl_t att;
} blt_gtbs_nv_info_t;

int blt_ccp_init(u8 initType, const void* param);
int blt_ccp_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_ccp_discovery(u16 connHandle);

blc_ccp_client_t *blt_ccp_getClientControlBuffer(u8 instIdx);
int blt_ccp_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);

const char *blt_ccp_ntfStatusFlagsStr(blc_gtbs_ntfResultCode_enum status);

int blt_tbss_init(u8 initType, const void* param);

