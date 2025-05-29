/********************************************************************************************************
 * @file    svc_tbs.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "stack/ble/ble.h"

#ifndef LE_AUDIO_TBS_BEARER_SIGNAL_STRENGTH
    #define LE_AUDIO_TBS_BEARER_SIGNAL_STRENGTH 1
#endif

#ifndef LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI
    #define LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI 1
#endif

#ifndef LE_AUDIO_TBS_CALL_FRIENDLY_NAME
    #define LE_AUDIO_TBS_CALL_FRIENDLY_NAME 1
#endif

#ifndef LE_AUDIO_TBS_BEARER_PROVIDER_NAME_MAX_SIZE
    #define LE_AUDIO_TBS_BEARER_PROVIDER_NAME_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_TBS_BEARER_UCI_MAX_SIZE
    #define LE_AUDIO_TBS_BEARER_UCI_MAX_SIZE 16
#endif

#ifndef LE_AUDIO_TBS_BEARER_URI_SCHEMES_SUPPORTED_LIST_MAX_SIZE
    #define LE_AUDIO_TBS_BEARER_URI_SCHEMES_SUPPORTED_LIST_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE
    #define LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_TBS_INCOMING_CALL_MAX_SIZE
    #define LE_AUDIO_TBS_INCOMING_CALL_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_TBS_FRIENDLY_NAME_MAX_SIZE
    #define LE_AUDIO_TBS_FRIENDLY_NAME_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_TBS_BEARER_LIST_CURRENT_CALLS_MAX_SIZE
    #define LE_AUDIO_TBS_BEARER_LIST_CURRENT_CALLS_MAX_SIZE 256
#endif

#ifndef LE_AUDIO_TBS_CALL_STATE_MAX_SIZE
    #define LE_AUDIO_TBS_CALL_STATE_MAX_SIZE 64
#endif

#define TBS_START_HDL SERVICE_TELEPHONE_BEARER_HDL

_attribute_ble_data_retention_ u8  tbsBearerProviderNameValue[LE_AUDIO_TBS_BEARER_PROVIDER_NAME_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsBearerProviderNameValueLen;
const u16                          tbsBearerProviderNameMaxSize = sizeof(tbsBearerProviderNameValue);

_attribute_ble_data_retention_ u8  tbsBearerUCIValue[LE_AUDIO_TBS_BEARER_UCI_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsBearerUCIValueLen;
const u16                          tbsBearerUCIMaxSize = sizeof(tbsBearerUCIValue);

_attribute_ble_data_retention_ u8 tbsBearerTechnologyValue    = 0x01;
static const u16                  tbsBearerTechnologyValueLen = sizeof(tbsBearerTechnologyValue);

_attribute_ble_data_retention_ u8  tbsBearerURISchemesSupportedListValue[LE_AUDIO_TBS_BEARER_URI_SCHEMES_SUPPORTED_LIST_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsBearerURISchemesSupportedListValueLen;
const u16                          tbsBearerURISchemesSupportedListMaxSize = sizeof(tbsBearerURISchemesSupportedListValue);

#if LE_AUDIO_TBS_BEARER_SIGNAL_STRENGTH
_attribute_ble_data_retention_ u8 tbsBearerSignalStrengthValue;
static const u16                  tbsBearerSignalStrengthValueLen = sizeof(tbsBearerSignalStrengthValue);

_attribute_ble_data_retention_ u8 tbsBearerSignalStrengthReportingIntervalValue;
static const u16                  tbsBearerSignalStrengthReportingIntervalValueLen = sizeof(tbsBearerSignalStrengthReportingIntervalValue);
#endif

_attribute_ble_data_retention_ u8  tbsBearerListCurrentCallsValue[LE_AUDIO_TBS_BEARER_LIST_CURRENT_CALLS_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsBearerListCurrentCallsValueLen;
const u16                          tbsBearerListCurrentCallsMaxSize = sizeof(tbsBearerListCurrentCallsValue);

_attribute_ble_data_retention_ u8 tbsCCIDValue;
static const u16                  tbsCCIDValueLen = sizeof(tbsCCIDValue);

_attribute_ble_data_retention_ u8 tbsStatusFlagsValue[2];
static const u16                  tbsStatusFlagsValueLen = sizeof(tbsStatusFlagsValue);

const u16 tbsIncomingCallTargetBearerURIMaxSize = LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE;
#if LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI
_attribute_ble_data_retention_ u8  tbsIncomingCallTargetBearerURIValue[LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsIncomingCallTargetBearerURIValueLen;
#endif

_attribute_ble_data_retention_ u8  tbsCallStateValue[LE_AUDIO_TBS_CALL_STATE_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsCallStateValueLen;
const u16                          tbsCallStateMaxSize = sizeof(tbsCallStateValue);

_attribute_ble_data_retention_ u8 tbsCallControlPointValue[3];
static const u16                  tbsCallControlPointValueLen = sizeof(tbsCallControlPointValue);

_attribute_ble_data_retention_ u16 tbsCallControlPointOptionalOpcodesValue;
static const u16                   tbsCallControlPointOptionalOpcodesValueLen = sizeof(tbsCallControlPointOptionalOpcodesValue);

const u16                          tbsIncomingCallMaxSize = LE_AUDIO_TBS_INCOMING_CALL_MAX_SIZE;
_attribute_ble_data_retention_ u8  tbsIncomingCallValue[LE_AUDIO_TBS_INCOMING_CALL_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsIncomingCallValueLen;

const u16 tbsFriendlyNameMaxSize = LE_AUDIO_TBS_FRIENDLY_NAME_MAX_SIZE;
#if LE_AUDIO_TBS_CALL_FRIENDLY_NAME
_attribute_ble_data_retention_ u8  tbsCallFriendlyNameValue[LE_AUDIO_TBS_FRIENDLY_NAME_MAX_SIZE];
_attribute_ble_data_retention_ u16 tbsCallFriendlyNameValueLen;
#endif

/*
 * @brief the structure for default TBS service List.
 */
static const atts_attribute_t tbsList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceTelephoneBearerUuid),

        //Bearer Provider Name
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicBearerProviderNameUuid, tbsBearerProviderNameValue),
        ATTS_COMMON_CCC_DEFINE,

        //Bearer Uniform Caller Identifier(UCI)
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicBearerUciUuid, tbsBearerUCIValue),

        //Bearer Technology
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicBearerTechnologyUuid, tbsBearerTechnologyValue),
        ATTS_COMMON_CCC_DEFINE,

        //Bearer URI Schemes Supported List
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropRead, characteristicBearerUriSchemesSuppListUuid, tbsBearerURISchemesSupportedListValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_TBS_BEARER_SIGNAL_STRENGTH
        //Bearer Signal Strength
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicBearerSsUuid, tbsBearerSignalStrengthValue),
        ATTS_COMMON_CCC_DEFINE,

        //Bearer Signal Strength Reporting Interval
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithout, characteristicBearerSsReportingIntervalUuid, tbsBearerSignalStrengthReportingIntervalValue),
#endif

        //Bearer List Current Calls
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicBearerListCurrentCallsUuid, tbsBearerListCurrentCallsValue),
        ATTS_COMMON_CCC_DEFINE,

        //Content Control ID (CCID)
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicContentControlIdUuid, tbsCCIDValue),

        //Status Flags
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicStatusFlagsUuid, tbsStatusFlagsValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_TBS_INCOMING_CALL_TARGET_BEARER_URI
        //Incoming Call Target Bearer URI
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicIncomingCallTargetBearerUriUuid, tbsIncomingCallTargetBearerURIValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

        //Call State
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicCallStateUuid, tbsCallStateValue),
        ATTS_COMMON_CCC_DEFINE,

        //Call Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_POINT_CB(charPropWriteWriteWithoutNotify, characteristicCallCtrlPointUuid, tbsCallControlPointValue),
        ATTS_COMMON_CCC_DEFINE,

        //Call Control Point Optional Opcodes
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicCallCtrlPointOptionalOpcodesUuid, tbsCallControlPointOptionalOpcodesValue),

        //Termination Reason
        ATTS_CHAR_UUID_NOTIF_ONLY(characteristicTerminationReasonUuid),
        ATTS_COMMON_CCC_DEFINE,

        //Incoming Call
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicIncomingCallUuid, tbsIncomingCallValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_TBS_CALL_FRIENDLY_NAME
        //Call Friendly Name
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicCallFriendlyNameUuid, tbsCallFriendlyNameValue),
        ATTS_COMMON_CCC_DEFINE,
#endif
};

/*
 * @brief the structure for default TBS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcTbsGroup =
    {
        NULL,
        tbsList,
        NULL,
        NULL,
        TBS_START_HDL,
        0,
};

/**
 * @brief      for user add default TBS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addTbsGroup(void)
{
    svcTbsGroup.endHandle = svcTbsGroup.startHandle + ARRAY_SIZE(tbsList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcTbsGroup);
}

/**
 * @brief      for user remove default TBS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeTbsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(TBS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in TBS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_tbsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcTbsGroup.readCback  = readCback;
    svcTbsGroup.writeCback = writeCback;
}
