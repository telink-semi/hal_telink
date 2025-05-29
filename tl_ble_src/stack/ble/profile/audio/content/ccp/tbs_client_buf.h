/********************************************************************************************************
 * @file    tbs_client_buf.h
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
    u8 callIndex;
    u8 info[31];
} blc_tbs_incoming_call_target_bearer_uri_t, blc_tbs_incoming_call_t, blc_tbs_call_friendly_name_t;

typedef union
{
    u16 statusFlags;

    struct
    {
        u16 inbandRingtone : 1;
        u16 silentMode     : 1;
        u16 RFU            : 14;
    };
} blc_tbs_status_flags_t;

typedef struct
{
    u8 callIndex;
    u8 state;

    union
    {
        u8 callFlags;

        struct
        {
            u8 incomingOutgoing      : 1;
            u8 infoWithheldByServer  : 1;
            u8 infoWithheldByNetwork : 1;
            u8 RFU                   : 5;
        };
    };
} blc_tbs_call_state_t;

/* Termination Reason characteristic format */
typedef union
{
    u16 termRsn;

    struct
    {
        u8 callIndex;
        u8 reasonCode;
    };
} blc_gtbs_term_rsn_t;

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;

    /* Characteristic value handle */
    u16 bearerProviderNameHdl;          /* Bearer Provider Name */
    u16 bearerUCIHdl;                   /* Bearer Uniform Caller Identifier */
    u16 bearerTechnologyHdl;            /* Bearer Technology */
    u16 bearerURISchemesSuppListHdl;    /* Bearer URI Schemes Supported List */
    u16 bearerSignalStrengthHdl;        /* Bearer Signal Strength */
    u16 reportingIntervalHdl;           /* Bearer Signal Strength Reporting Interval */
    u16 bearerListCurrentCallsHdl;      /* Bearer List Current Calls */
    u16 ccidHdl;                        /* Content Control ID */
    u16 statusFlagsHdl;                 /* Status Flags */
    u16 incomingCallTargetBearerURIHdl; /* Incoming Call Target Bearer URI */
    u16 callStateHdl;                   /* Call State */
    u16 callControlPointHdl;            /* Call Control Point */
    u16 callControlPointOptionalOpHdl;  /* Call Control Point Optional Opcodes */
    u16 terminationReasonHdl;           /* Termination Reason */
    u16 incomingCallHdl;                /* Incoming Call*/
    u16 callFriendlyNameHdl;            /* Call Friendly Name */

    u16 connHandle;

    u16 providerNameLen;
    u8  providerName[30];

    u16 UCILen;
    u8  UCI[12];

    u16 URISchemesSupportedListLen;
    u8  URISchemesSupportedList[30];

    u16 listCurrCallsLen;
    u8  listCurrCalls[STACK_AUDIO_CALL_MEMBERS_MAX_NUM * 40];

    u8  technology;
    u8  signalStrength;
    u8  signalStrengthReportingInterval;
    u8  ccid;
    u16 uriLen;
    u16 callStateLen;
    u16 incomingCallLen;
    u16 callFriendlyNameLen;

    blc_tbs_incoming_call_target_bearer_uri_t uri;
    blc_tbs_call_state_t                      callState[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];
    blc_tbs_incoming_call_t                   incomingCall;
    blc_tbs_call_friendly_name_t              callFriendlyName;

    blc_tbs_status_flags_t statusFlags;
    blc_gtbs_term_rsn_t    termRsn;
    u16                    callCtrlPointOptionalOp;
    u8                     reserved[2];
} blc_tbs_client_t, blc_gtbs_client_t;

typedef struct
{
    blc_gtbs_client_t gtbs;

    //Not supported tbs
    u8                tbsClientCount;
    u8                reserved[3];
    blc_tbs_client_t *tbs[0];

} blc_ccp_client_t;

typedef struct blc_ccp_client_ctrl
{
    blc_prf_proc_t    process;
    blc_ccp_client_t *pCcpClient[STACK_PRF_ACL_CONN_MAX_NUM];
} blc_ccp_client_ctrl_t;

typedef struct
{
} blc_ccpc_regParam_t;
