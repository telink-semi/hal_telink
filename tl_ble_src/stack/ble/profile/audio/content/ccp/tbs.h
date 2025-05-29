/********************************************************************************************************
 * @file    tbs.h
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

#include "tbs_client_buf.h"
#include "tbs_server_buf.h"


/******************************* (G)TBS Common Start **********************************************************************/

/* Status_Flags bit definitions */
#define GTBS_STATUS_FLAG_INBAND_RINGTONE_DISABLE 0      //inband ringtone disabled
#define GTBS_STATUS_FLAG_INBAND_RINGTONE_ENABLE  BIT(0) //inband ringtone disabled

#define GTBS_STATUS_FLAG_SVR_NOT_IN_SILENT_MODE  0      //Server is not in silent mode
#define GTBS_STATUS_FLAG_SVR_IN_SILENT_MODE      BIT(1) //Server is in silent mode


/* Call_Flags bit definitions */
#define GTBS_CALL_FLAG_INCOMING_CALL  0      //Call is an incoming call
#define GTBS_CALL_FLAG_OUTGOING_CALL  BIT(0) //Call is an outgoing call

#define GTBS_CALL_NOT_WITHHELD_BY_SVR 0      //Not withheld by server
#define GTBS_CALL_WITHHELD_BY_SVR     BIT(1) //Withheld by server

#define GTBS_CALL_PROVIDED_BY_NETWORK 0      //Provided by network
#define GTBS_CALL_WITHHELD_BY_NETWORK BIT(2) //Withheld by network

/* Call Control Point Optional Opcodes bit definitions */
#define GTBS_CCP_OPT_OPCODE_NOT_SUPP_LOCAL_HOLD_AND_RETRIEVE 0
#define GTBS_CCP_OPT_OPCODE_SUPP_LOCAL_HOLD_AND_RETRIEVE     BIT(0)
#define GTBS_CCP_OPT_OPCODE_NOT_SUPP_JION_CALL               0
#define GTBS_CCP_OPT_OPCODE_SUPP_JION_CALL                   BIT(1)

/* Call State possible states */
typedef enum
{
    GTBS_CALL_STATE_INCOMING                  = 0x00,
    GTBS_CALL_STATE_DIALING                   = 0x01,
    GTBS_CALL_STATE_ALERTING                  = 0x02,
    GTBS_CALL_STATE_ACTIVE                    = 0x03,
    GTBS_CALL_STATE_LOCALLY_HELD              = 0x04,
    GTBS_CALL_STATE_REMOTELY_HELD             = 0x05,
    GTBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD = 0x06,
} blc_gtbs_callState_enum;

/* Call Control Point characteristic opcodes */
typedef enum
{
    GTBS_OPCODE_ACCEPT,
    GTBS_OPCODE_TERMINATE,
    GTBS_OPCODE_LOCAL_HOLD,
    GTBS_OPCODE_LOCAL_RETRIEVE,
    GTBS_OPCODE_ORIGINATE,
    GTBS_OPCODE_JOIN,
    GTBS_OPCODE_MAX,
} blt_gtbs_opcode_enum;

/* Call Control Point Notification Result Codes */
typedef enum
{
    GTBS_NTF_RESULT_CODE_SUCCESS                = 0x00,
    GTBS_NTF_RESULT_CODE_OPCODE_NOT_SUPP        = 0x01,
    GTBS_NTF_RESULT_CODE_OPERATION_NOT_POSSIBLE = 0x02,
    GTBS_NTF_RESULT_CODE_INVALID_CALL_INDEX     = 0x03,
    GTBS_NTF_RESULT_CODE_STATE_MISMATCH         = 0x04,
    GTBS_NTF_RESULT_CODE_LACK_OF_RESOURCES      = 0x05,
    GTBS_NTF_RESULT_CODE_INVALID_OUTGOING_URI   = 0x06,
} blc_gtbs_ntfResultCode_enum;

/* Termination Reason Codes */
typedef enum
{
    GTBS_TERM_REASON_URI_ERROR          = 0x00,
    GTBS_TERM_REASON_CALL_FAILED        = 0x01,
    GTBS_TERM_REASON_REMOTE_ENDED_CALL  = 0x02,
    GTBS_TERM_REASON_SERVER_ENDED_CALL  = 0x03,
    GTBS_TERM_REASON_LINE_BUSY          = 0x04,
    GTBS_TERM_REASON_NETWORK_CONGESTION = 0x05,
    GTBS_TERM_REASON_CLIENT_TERM_CALL   = 0x06,
    GTBS_TERM_REASON_NO_SERVICE         = 0x07,
    GTBS_TERM_REASON_NO_ANSWER          = 0x08,
    GTBS_TERM_REASON_UNSPECIFIED        = 0x09,
} blc_gtbs_termReason_enum;

/* Bearer Technology */
typedef enum
{
    GTBS_TECHNOLOGY_3G    = 0x01,
    GTBS_TECHNOLOGY_4G    = 0x02,
    GTBS_TECHNOLOGY_LTE   = 0x03,
    GTBS_TECHNOLOGY_WIFI  = 0x04,
    GTBS_TECHNOLOGY_5G    = 0x05,
    GTBS_TECHNOLOGY_GSM   = 0x06,
    GTBS_TECHNOLOGY_CDMA  = 0x07,
    GTBS_TECHNOLOGY_2G    = 0x08,
    GTBS_TECHNOLOGY_WCDMA = 0x09,
    GTBS_TECHNOLOGY_IP    = 0x0a,
} blc_gtbs_technology_enum;

/* Signal Strength */
#define GTBS_SIGNAL_STRENGTH_UNAVAILABLE 0xff

//GTBS get some string API.
/**
 * @brief       This function use get bearer technology name string.
 * @param[in]   tech    - blc_gtbs_technology_enum.
 * @return      name string.
 */
const char *blc_gtbs_getBearerTechnologyName(blc_gtbs_technology_enum tech);

/**
 * @brief       This function use get status flags description string.
 * @param[in]   statusFlags - status flags.
 * @return      description string.
 */
const char *blc_gtbs_getStatusFlagsDescription(u16 statusFlags);

/**
 * @brief       This function use get call state name string.
 * @param[in]   state   - blc_gtbs_callState_enum.
 * @return      name string.
 */
const char *blc_gtbs_getCallStateName(blc_gtbs_callState_enum state);

/**
 * @brief       This function use get call flags description string.
 * @param[in]   statusFlags - status flags.
 * @return      description string.
 */
const char *blc_gtbs_getCallFlagsDescription(u8 callFlags);

/**
 * @brief       This function use get call control point opcode name string.
 * @param[in]   opcode  - blt_gtbs_opcode_enum.
 * @return      name string.
 */
const char *blc_gtbs_getCallControlPointOpcodeName(blt_gtbs_opcode_enum opcode);

/**
 * @brief       This function use get termination reason name string.
 * @param[in]   status  - blc_gtbs_termReason_enum.
 * @return      name string.
 */
const char *blt_gtbs_getTerminationReasonName(blc_gtbs_termReason_enum status);

/******************************* (G)TBS Common End **********************************************************************/


/******************************* (G)TBS Client Start **********************************************************************/

//GTBS Client Event ID
typedef enum
{
    AUDIO_EVT_GTBSC_START = AUDIO_EVT_TYPE_GTBSC,
    AUDIO_EVT_GTBS_BEARER_PROVIDER_NAME,         //refer to 'blc_gtbsc_bearerProviderName_t'
    AUDIO_EVT_GTBS_BEARER_TECHNOLOGY,            //refer to 'blc_gtbsc_technology_t'
    AUDIO_EVT_GTBS_BEARER_URI_SCHEMES_SUPP_LIST, //refer to 'blc_gtbsc_uriSchemeSuppList_t'
    AUDIO_EVT_GTBS_BEARER_SIGNAL_STRENGTH,       //refer to 'blc_gtbsc_signalStrength_t'
    AUDIO_EVT_GTBS_BEARER_LIST_CURRENT_CALL,     //refer to 'blc_gtbsc_listCurrentCallsEvt_t'
    AUDIO_EVT_GTBS_STATUS_FLAGS,                 //refer to 'blc_gtbsc_statusFlagsEvt_t'
    AUDIO_EVT_GTBS_INCOMING_CALL_TGT_URI,        //refer to 'blc_gtbsc_incomingCallTgtUriEvt_t'
    AUDIO_EVT_GTBS_CALL_STATE,                   //refer to 'blc_gtbsc_listCallStateEvt_t'
    AUDIO_EVT_GTBS_TERM_REASON,                  //refer to 'blc_gtbsc_termRsnEvt_t'
    AUDIO_EVT_GTBS_INCOMING_CALL,                //refer to 'blc_gtbsc_incomingCallEvt_t'
    AUDIO_EVT_GTBS_CALL_FRIENDLY_NAME,           //refer to 'blc_gtbsc_friendlyNameEvt_t'
    AUDIO_EVT_GTBS_CCP_NTF_RESULT_CODE,          //refer to 'blc_gtbsc_ccpNtfResultCodesEvt_t'
} audio_gtbsc_evt_enum;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_BEARER_PROVIDER_NAME"
 */
typedef struct
{
    u8 nameLen;
    u8 providerName[30]; //e.g. "CMCC"
} blc_gtbsc_bearerProviderName_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_BEARER_TECHNOLOGY"
 */
typedef struct
{
    int technology; //blc_gtbs_technology_enum
} blc_gtbsc_technology_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_BEARER_URI_SCHEMES_SUPP_LIST"
 */
typedef struct
{
    u8 suppLen;
    u8 uriSchemeSuppList[30]; //e.g.: "tel,sip,skype"
} blc_gtbsc_uriSchemeSuppList_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_BEARER_SIGNAL_STRENGTH"
 */
typedef struct
{
    u8 signalStrength;
} blc_gtbsc_signalStrength_t;

typedef struct
{
    u8  listItemLen;
    u8  callIndex;
    u8  state;
    u8  callFlags;
    u8 *pCallUri;
} blc_gtbsc_list_curr_call_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_BEARER_LIST_CURRENT_CALL"
 */
typedef struct
{
    u8 listLen;
    u8 currentListCall[STACK_AUDIO_CALL_MEMBERS_MAX_NUM * 40]; //structure:List_Item_Length[i]-Call_Index[i]-Call_Flags[i]-Call_URI[i]
} blc_gtbsc_listCurrentCallsEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_STATUS_FLAGS"
 */
typedef struct
{
    u16 statusFlags;
} blc_gtbsc_statusFlagsEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_TERM_REASON"
 */
typedef struct
{
    u8 callIndex;
    u8 termRsn;
} blc_gtbsc_termRsnEvt_t, blc_gtbss_terminationReasonNtf_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_INCOMING_CALL_TGT_URI"
 */
typedef struct
{
    u8                                        uriLen;
    blc_tbs_incoming_call_target_bearer_uri_t uri;
} blc_gtbsc_incomingCallTgtUriEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_INCOMING_CALL"
 */
typedef struct
{
    u8                      callLen;
    blc_tbs_incoming_call_t call;
} blc_gtbsc_incomingCallEvt_t;

typedef struct
{
    u8 callIndex;
    u8 state;
    u8 callFlags;
} blc_gtbs_call_state_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_CALL_STATE"
 */
typedef struct
{
    u8                    stateLen;
    blc_gtbs_call_state_t state[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];
} blc_gtbsc_listCallStateEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_CALL_FRIENDLY_NAME"
 */
typedef struct
{
    u8                           nameLen;
    blc_tbs_call_friendly_name_t name;
} blc_gtbsc_friendlyNameEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_GTBS_CCP_NTF_RESULT_CODE"
 */
typedef struct
{
    u8 reqOpcode;
    u8 callIndex;
    u8 resultCode;
} blc_gtbsc_ccpNtfResultCodesEvt_t, blc_gtbss_callCtrlPointNtf_t;

/**
 * @brief       This function serves to register Call Control Client include GTBS and TBS client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerCallControlClient(const blc_ccpc_regParam_t *param);


//GTBS Client Read Characteristic Value Operation API
int blc_gtbsc_readBearerProviderName(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readBearerUCI(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readBearerTechnology(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readBearerURISchemesSuppList(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readBearerSignalStrength(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readBearerSignalStrengthRptItvl(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readBearerListCurrentCalls(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readContentControlID(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readIncomingCallTargetBearerURI(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readStatusFlags(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readCallState(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readCallCtrlPntOptOpcodes(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readIncomingCall(u16 connHandle, prf_read_cb_t readCb);
int blc_gtbsc_readCallFriendlyName(u16 connHandle, prf_read_cb_t readCb);

//GTBS Client Get Characteristic Value Operation API
int blc_gtbsc_getBearerProviderName(u16 connHandle, u8 *pOutPrName, u16 *OutPrNameLen);
int blc_gtbsc_getBearerUCI(u16 connHandle, u8 *pOutUCI, u16 *OutUCILen);
int blc_gtbsc_getBearerTechnology(u16 connHandle, u8 outTechnology[1]);
int blc_gtbsc_getBearerURISchemesSuppList(u16 connHandle, u8 *outURISchemesSuppList, u16 *outURISchemesSuppListLen);
int blc_gtbsc_getBearerSignalStrength(u16 connHandle, u8 outSignalStrength[1]);
int blc_gtbsc_getBearerSignalStrengthItvl(u16 connHandle, u8 outSignalStrengthItvl[1]);
int blc_gtbsc_getBearerListCurrentCalls(u16 connHandle, u8 *listCurrCalls, u16 *listCurrCallsLen);
int blc_gtbsc_getCCID(u16 connHandle, u8 outCCID[1]);
int blc_gtbsc_getStatusFlags(u16 connHandle, blc_tbs_status_flags_t *statusFlags);
int blc_gtbsc_getIncomingCallTargetBearerURI(u16 connHandle, blc_tbs_incoming_call_target_bearer_uri_t *uri, u16 *uriLen);
int blc_gtbsc_getCallState(u16 connHandle, u16 *callMembersCnt, blc_gtbs_call_state_t *callState);
int blc_gtbsc_getCcpOptionalOp(u16 connHandle, u16 outCcpOptionalOp[1]);
int blc_gtbsc_getIncomingCall(u16 connHandle, blc_tbs_incoming_call_t *incomingCall, u16 *incomingCallLen);
int blc_gtbsc_getCallFriendlyName(u16 connHandle, blc_tbs_call_friendly_name_t *callFriendlyName, u16 *callFriendlyNameLen);

//GTBS Client Write Characteristic Value Operation API
int blc_gtbsc_writeBearerSignalStrengthItvl(u16 connHandle, u8 outSignalStrengthItvl, prf_write_cb_t writeCb);
int blc_gtbsc_writeBearerSignalStrengthItvlWithoutRsp(u16 connHandle, u8 outSignalStrengthItvl);
int blc_gtbsc_writeCallCtrlPoint(u16 connHandle, blt_gtbs_opcode_enum opcode, u8 *param, u16 paramLen, prf_write_cb_t writeCb);
int blc_gtbsc_writeCallCtrlPointWithoutRsp(u16 connHandle, blt_gtbs_opcode_enum opcode, u8 *param, u16 paramLen);

//GTBS client Call Control Point API
/**
 * @brief       This function use answer the incoming call identified by Call_Index.
 * @param[in]   connHandle  - ACL connect handle.
 * @param[in]   callIndex   - incoming call index.
 * @return      ble_sts_t
 */
int blc_gtbsc_writeAcceptIncomingCall(u16 connHandle, u8 callIndex);

/**
 * @brief       This function use end the active, alerting(outgoing), dialing(outgoing), incoming, or held (locally or remotely) call identified by Call_Index.
 * @param[in]   connHandle  - ACL connect handle.
 * @param[in]   callIndex   - incoming call index.
 * @return      ble_sts_t
 */
int blc_gtbsc_writeTerminateCall(u16 connHandle, u8 callIndex);

/**
 * @brief       This function use place the active or incoming call identified by Call_Index on local hold.
 * @param[in]   connHandle  - ACL connect handle.
 * @param[in]   callIndex   - incoming call index.
 * @return      ble_sts_t
 */
int blc_gtbsc_writeLocalHoldActiveOrImcomingCall(u16 connHandle, u8 callIndex);

/**
 * @brief       This function use Move a locally held call identified by Call_Index to an active call,
 *                      Move a locally and remotely held call to a remotely held call.
 * @param[in]   connHandle  - ACL connect handle.
 * @param[in]   callIndex   - incoming call index.
 * @return      ble_sts_t
 */
int blc_gtbsc_writeLocalRetrieve(u16 connHandle, u8 callIndex);

/**
 * @brief       This function use initiate a call to the remote party identified by the URI.
 * @param[in]   connHandle  - ACL connect handle.
 * @param[in]   callIndex   - incoming call index.
 * @return      ble_sts_t
 */
int blc_gtbsc_writeOriginate(u16 connHandle, u8 *pUriofOutgoingCall, u8 uriLen);

/**
 * @brief       This function use Put calls in the list that are not in the Remotely Held state to the active state and join the calls.
 *                  Any calls in the Remotely Held or Locally and Remotely Held states move to the Remotely Held state and are joined with the other calls.
 * @param[in]   connHandle  - ACL connect handle.
 * @param[in]   callIndex   - incoming call index.
 * @return      ble_sts_t
 */
int blc_gtbsc_writeJoinCallList(u16 connHandle, u8 *pCallList, u8 callListLen);


/******************************* (G)TBS Client End **********************************************************************/


/******************************* (G)TBS Server Start **********************************************************************/

extern const blc_ccps_regParam_t defaultCppsParam;

//GTBS Server Event ID
typedef enum
{
    AUDIO_EVT_GTBSS_START = AUDIO_EVT_TYPE_GTBSS,
    AUDIO_EVT_GTBSS_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL,
    AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_ACCEPT,
    AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_TERMINATE,
    AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_LOCAL_HOLD,
    AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_LOCAL_RETRIEVE,
    AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_ORIGINATE,
    AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_JOIN,
    //NONE:
} audio_gtbss_evt_enum;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_BEARER_SIGNAL_STRENGTH_REPORTING_INTERVAL
    u8 interval;
} blc_gtbss_bearerSignalStrengthReportingIntervalEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_ACCEPT
    u8 callIndex;
} blc_gtbss_callControlPointAcceptEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_TERMINATE
    u8 callIndex;
} blc_gtbss_callControlPointTerminateEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_LOCAL_HOLD
    u8 callIndex;
} blc_gtbss_callControlPointLocalHoldEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_LOCAL_RETRIEVE
    u8 callIndex;
} blc_gtbss_callControlPointLocalRetrieveEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_ORIGINATE
    u16 uriLen;
    u8  uri[];
} blc_gtbss_callControlPointOriginateEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GTBSS_CALL_CONTROL_POINT_JOIN
    u16 callIndexNum;
    u8  callIndexes[];
} blc_gtbss_callControlPointJoinEvt_t;

typedef struct
{
    blc_gtbs_call_state_t state;
    u16                   uriLen;
    const u8             *uri;
} blc_gtbs_bearer_list_item_t;

/**
 * @brief       This function serves to register call control Server include TBS and GTBS function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerCallControlServer(const blc_ccps_regParam_t *param);


//GTBS Server Update Characteristic Value Operation API
int blc_gtbss_updateBearerProviderName(u16 connHandle, const u8 *pPrName, u16 prNameLen);
// TBS spec 3.2.1 Bearer UCI behavior "The server shall not change this value once instantiated."
int blc_gtbss_updateBearerUniformCallerIdentifier(u16 connHandle, const u8 *uci, u16 uciLen);
int blc_gtbss_updateBearerTechnology(u16 connHandle, u8 Technology);
int blc_gtbss_updateBearerURISchemesSupportedList(u16 connHandle, u8 uriSchemesNum, const blc_tbss_uri_scheme_t *schemes);
int blc_gtbss_updateBearerSignalStrength(u16 connHandle, u8 signalStrength);
int blc_gtbss_updateBearerListCurrentCalls(u16 connHandle, blc_gtbs_bearer_list_item_t *listCurrCalls, u16 listCurrCallsLen);
int blc_gtbss_updateContentCtrlID(u16 connHandle, u8 ccid);
int blc_gtbss_updateStatusFlags(u16 connHandle, blc_tbs_status_flags_t statusFlags);
int blc_gtbss_updateIncomingCallTargetBearerURI(u16 connHandle, u8 callIndex, const u8 *uri, u16 uriLen);
int blc_gtbss_updateCallState(u16 connHandle, u16 callMembersCnt, blc_gtbs_call_state_t *callState);
int blc_gtbss_updateCallCtrlPoint(u16 connHandle, blc_gtbss_callCtrlPointNtf_t val);
int blc_gtbss_updateTerminationReason(u16 connHandle, blc_gtbss_terminationReasonNtf_t val);
int blc_gtbss_updateCallControlPointOptionalOpcodes(u16 connHandle, u16 optionalOpcodes);
int blc_gtbss_updateIncomingCall(u16 connHandle, u8 callIndex, const u8 *uri, u16 uriLen);
int blc_gtbss_updateCallFriendlyName(u16 connHandle, u8 callIndex, const u8 *callFriendlyName, u16 callFriendlyNameLen);

/******************************* (G)TBS Server End **********************************************************************/
