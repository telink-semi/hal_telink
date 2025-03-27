/********************************************************************************************************
 * @file    svc_gtbs.c
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

#ifndef LE_AUDIO_GTBS_BEARER_SIGNAL_STRENGTH
#define LE_AUDIO_GTBS_BEARER_SIGNAL_STRENGTH                        1
#endif

#ifndef LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI
#define LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI               1
#endif

#ifndef LE_AUDIO_GTBS_CALL_FRIENDLY_NAME
#define LE_AUDIO_GTBS_CALL_FRIENDLY_NAME                            1
#endif

#ifndef LE_AUDIO_GTBS_BEARER_PROVIDER_NAME_MAX_SIZE
#define LE_AUDIO_GTBS_BEARER_PROVIDER_NAME_MAX_SIZE                 64
#endif

#ifndef LE_AUDIO_GTBS_BEARER_UCI_MAX_SIZE
#define LE_AUDIO_GTBS_BEARER_UCI_MAX_SIZE                           16
#endif

#ifndef LE_AUDIO_GTBS_BEARER_URI_SCHEMES_SUPPORTED_LIST_MAX_SIZE
#define LE_AUDIO_GTBS_BEARER_URI_SCHEMES_SUPPORTED_LIST_MAX_SIZE    64
#endif

#ifndef LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE
#define LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE      64
#endif

#ifndef LE_AUDIO_GTBS_INCOMING_CALL_MAX_SIZE
#define LE_AUDIO_GTBS_INCOMING_CALL_MAX_SIZE                        64
#endif

#ifndef LE_AUDIO_GTBS_FRIENDLY_NAME_MAX_SIZE
#define LE_AUDIO_GTBS_FRIENDLY_NAME_MAX_SIZE                        64
#endif

#ifndef LE_AUDIO_GTBS_BEARER_LIST_CURRENT_CALLS_MAX_SIZE
#define LE_AUDIO_GTBS_BEARER_LIST_CURRENT_CALLS_MAX_SIZE            256
#endif

#ifndef LE_AUDIO_GTBS_CALL_STATE_MAX_SIZE
#define LE_AUDIO_GTBS_CALL_STATE_MAX_SIZE                           64
#endif

#define GTBS_START_HDL                  SERVICE_GENERIC_TELEPHONE_BEARER_HDL

_attribute_ble_data_retention_ u8 gtbsBearerProviderNameValue[LE_AUDIO_GTBS_BEARER_PROVIDER_NAME_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsBearerProviderNameValueLen;
const u16 gtbsBearerProviderNameMaxSize = sizeof(gtbsBearerProviderNameValue);

_attribute_ble_data_retention_ u8 gtbsBearerUCIValue[LE_AUDIO_GTBS_BEARER_UCI_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsBearerUCIValueLen;
const u16 gtbsBearerUCIMaxSize = sizeof(gtbsBearerUCIValue);

_attribute_ble_data_retention_ u8 gtbsBearerTechnologyValue = 0x01;
static const u16 gtbsBearerTechnologyValueLen = sizeof(gtbsBearerTechnologyValue);

_attribute_ble_data_retention_ u8 gtbsBearerURISchemesSupportedListValue[LE_AUDIO_GTBS_BEARER_URI_SCHEMES_SUPPORTED_LIST_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsBearerURISchemesSupportedListValueLen;
const u16 gtbsBearerURISchemesSupportedListMaxSize = sizeof(gtbsBearerURISchemesSupportedListValue);

#if LE_AUDIO_GTBS_BEARER_SIGNAL_STRENGTH
_attribute_ble_data_retention_ u8 gtbsBearerSignalStrengthValue;
static const u16 gtbsBearerSignalStrengthValueLen = sizeof(gtbsBearerSignalStrengthValue);

_attribute_ble_data_retention_ u8 gtbsBearerSignalStrengthReportingIntervalValue;
static const u16 gtbsBearerSignalStrengthReportingIntervalValueLen = sizeof(gtbsBearerSignalStrengthReportingIntervalValue);
#endif

_attribute_ble_data_retention_ u8 gtbsBearerListCurrentCallsValue[LE_AUDIO_GTBS_BEARER_LIST_CURRENT_CALLS_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsBearerListCurrentCallsValueLen;
const u16 gtbsBearerListCurrentCallsMaxSize = sizeof(gtbsBearerListCurrentCallsValue);

_attribute_ble_data_retention_ u8 gtbsCCIDValue;
static const u16 gtbsCCIDValueLen = sizeof(gtbsCCIDValue);

_attribute_ble_data_retention_ u8 gtbsStatusFlagsValue[2];
static const u16 gtbsStatusFlagsValueLen = sizeof(gtbsStatusFlagsValue);

const u16 gtbsIncomingCallTargetBearerURIMaxSize = LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE;
#if LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI
_attribute_ble_data_retention_ u8 gtbsIncomingCallTargetBearerURIValue[LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsIncomingCallTargetBearerURIValueLen;
#endif

_attribute_ble_data_retention_ u8 gtbsCallStateValue[LE_AUDIO_GTBS_CALL_STATE_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsCallStateValueLen;
const u16 gtbsCallStateMaxSize = sizeof(gtbsCallStateValue);

_attribute_ble_data_retention_ u8 gtbsCallControlPointValue[3];
static const u16 gtbsCallControlPointValueLen = sizeof(gtbsCallControlPointValue);

_attribute_ble_data_retention_ u16 gtbsCallControlPointOptionalOpcodesValue;
static const u16 gtbsCallControlPointOptionalOpcodesValueLen = sizeof(gtbsCallControlPointOptionalOpcodesValue);

const u16 gtbsIncomingCallMaxSize = LE_AUDIO_GTBS_INCOMING_CALL_MAX_SIZE;
_attribute_ble_data_retention_ u8 gtbsIncomingCallValue[LE_AUDIO_GTBS_INCOMING_CALL_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsIncomingCallValueLen;

const u16 gtbsFriendlyNameMaxSize = LE_AUDIO_GTBS_FRIENDLY_NAME_MAX_SIZE;
#if LE_AUDIO_GTBS_CALL_FRIENDLY_NAME
_attribute_ble_data_retention_ u8 gtbsCallFriendlyNameValue[LE_AUDIO_GTBS_FRIENDLY_NAME_MAX_SIZE];
_attribute_ble_data_retention_ u16 gtbsCallFriendlyNameValueLen;
#endif

/*
 * @brief the structure for default GTBS service List.
 */
static const atts_attribute_t gtbsList[] =
{
    ATTS_PRIMARY_SERVICE(serviceGenericTelephoneBearerUuid),

    //Bearer Provider Name
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicBearerProviderNameUuid, gtbsBearerProviderNameValue),
    ATTS_COMMON_CCC_DEFINE,

    //Bearer Uniform Caller Identifier(UCI)
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicBearerUciUuid, gtbsBearerUCIValue),

    //Bearer Technology
    ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicBearerTechnologyUuid, gtbsBearerTechnologyValue),
    ATTS_COMMON_CCC_DEFINE,

    //Bearer URI Schemes Supported List
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropRead, characteristicBearerUriSchemesSuppListUuid, gtbsBearerURISchemesSupportedListValue),
    ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_GTBS_BEARER_SIGNAL_STRENGTH
    //Bearer Signal Strength
    ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicBearerSsUuid, gtbsBearerSignalStrengthValue),
    ATTS_COMMON_CCC_DEFINE,

    //Bearer Signal Strength Reporting Interval
    ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithout, characteristicBearerSsReportingIntervalUuid, gtbsBearerSignalStrengthReportingIntervalValue),
#endif

    //Bearer List Current Calls
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicBearerListCurrentCallsUuid,gtbsBearerListCurrentCallsValue),
    ATTS_COMMON_CCC_DEFINE,

    //Content Control ID (CCID)
    ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicContentControlIdUuid, gtbsCCIDValue),

    //Status Flags
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicStatusFlagsUuid, gtbsStatusFlagsValue),
    ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_GTBS_INCOMING_CALL_TARGET_BEARER_URI
    //Incoming Call Target Bearer URI
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicIncomingCallTargetBearerUriUuid, gtbsIncomingCallTargetBearerURIValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

    //Call State
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicCallStateUuid, gtbsCallStateValue),
    ATTS_COMMON_CCC_DEFINE,

    //Call Control Point
    ATTS_CHAR_UUID_ENCR_WRITE_POINT_CB(charPropWriteWriteWithoutNotify, characteristicCallCtrlPointUuid, gtbsCallControlPointValue),
    ATTS_COMMON_CCC_DEFINE,

    //Call Control Point Optional Opcodes
    ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicCallCtrlPointOptionalOpcodesUuid, gtbsCallControlPointOptionalOpcodesValue),

    //Termination Reason
    ATTS_CHAR_UUID_NOTIF_ONLY(characteristicTerminationReasonUuid),
    ATTS_COMMON_CCC_DEFINE,

    //Incoming Call
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicIncomingCallUuid, gtbsIncomingCallValue),
    ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_GTBS_CALL_FRIENDLY_NAME
    //Call Friendly Name
    ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicCallFriendlyNameUuid, gtbsCallFriendlyNameValue),
    ATTS_COMMON_CCC_DEFINE,
#endif
};

/*
 * @brief the structure for default GTBS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcGtbsGroup =
{
    NULL,
    gtbsList,
    NULL,
    NULL,
    GTBS_START_HDL,
    0,
};

/**
 * @brief      for user add default GTBS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addGtbsGroup(void)
{
    svcGtbsGroup.endHandle = svcGtbsGroup.startHandle+ARRAY_SIZE(gtbsList)-1;
    blc_gatts_addAttributeServiceGroup(&svcGtbsGroup);
}

/**
 * @brief      for user remove default GTBS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeGtbsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(GTBS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in GTBS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_gtbsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcGtbsGroup.readCback = readCback;
    svcGtbsGroup.writeCback = writeCback;
}
