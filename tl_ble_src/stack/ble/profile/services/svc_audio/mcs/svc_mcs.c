/********************************************************************************************************
 * @file    svc_mcs.c
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


#ifndef LE_AUDIO_MCS_MEDIA_PLAYER_ICON_OBJECT_ID
    #define LE_AUDIO_MCS_MEDIA_PLAYER_ICON_OBJECT_ID 1
#endif

#ifndef LE_AUDIO_MCS_MEDIA_PLAYER_ICON_URL
    #define LE_AUDIO_MCS_MEDIA_PLAYER_ICON_URL 1
#endif

#ifndef LE_AUDIO_MCS_PLAYBACK_SPEED
    #define LE_AUDIO_MCS_PLAYBACK_SPEED 1
#endif

#ifndef LE_AUDIO_MCS_SEEKING_SPEED
    #define LE_AUDIO_MCS_SEEKING_SPEED 1
#endif

#ifndef LE_AUDIO_MCS_CURRENT_TRACK_OBJECT_ID
    #define LE_AUDIO_MCS_CURRENT_TRACK_OBJECT_ID 1
#endif

#ifndef LE_AUDIO_MCS_PLAYING_ORDER
    #define LE_AUDIO_MCS_PLAYING_ORDER 1
#endif

#ifndef LE_AUDIO_MCS_PLAYING_ORDERS_SUPPORTED
    #define LE_AUDIO_MCS_PLAYING_ORDERS_SUPPORTED 1
#endif

#ifndef LE_AUDIO_MCS_MEDIA_CONTROL_POINT
    #define LE_AUDIO_MCS_MEDIA_CONTROL_POINT 1
#endif

#ifndef LE_AUDIO_MCS_SEARCH_RESULTS_OBJECT_ID
    #define LE_AUDIO_MCS_SEARCH_RESULTS_OBJECT_ID 1
#endif

#ifndef LE_AUDIO_MCS_PMEDIA_PLAYER_NAME_MAX_SIZE
    #define LE_AUDIO_MCS_PMEDIA_PLAYER_NAME_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_MCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE
    #define LE_AUDIO_MCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_MCS_TRACK_TITLE_MAX_SIZE
    #define LE_AUDIO_MCS_TRACK_TITLE_MAX_SIZE 64
#endif

#define MCS_START_HDL SERVICE_MEDIA_CONTROL_HDL

_attribute_ble_data_retention_ u8  mcsMediaPlayerNameValue[LE_AUDIO_MCS_PMEDIA_PLAYER_NAME_MAX_SIZE];
_attribute_ble_data_retention_ u16 mcsMediaPlayerNameValueLen;
const u16                          mcsMediaPlayerNameMaxSize = sizeof(mcsMediaPlayerNameValue);

#if LE_AUDIO_MCS_MEDIA_PLAYER_ICON_OBJECT_ID
_attribute_ble_data_retention_ u8  mcsMediaPlayerIconObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsMediaPlayerIconObjectIDValueLen;
#endif

const u16 mcsMediaPlayerIconURLMaxSize = LE_AUDIO_MCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE;
#if LE_AUDIO_MCS_MEDIA_PLAYER_ICON_URL
_attribute_ble_data_retention_ u8  mcsMediaPlayerIconURLValue[LE_AUDIO_MCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE];
_attribute_ble_data_retention_ u16 mcsMediaPlayerIconURLValueLen;
#endif

static const u16 gMcsTrackChangedLen = 0;

_attribute_ble_data_retention_ u8  mcsTrackTitleValue[LE_AUDIO_MCS_TRACK_TITLE_MAX_SIZE];
_attribute_ble_data_retention_ u16 mcsTrackTitleValueLen;
const u16                          mcsTrackTitleMaxSize = sizeof(mcsTrackTitleValue);

_attribute_ble_data_retention_ u32 mcsTrackDurationValue    = 0xFFFFFFFF;
static const u16                   mcsTrackDurationValueLen = 4;

_attribute_ble_data_retention_ u32 mcsTrackPositionValue    = 0xFFFFFFFF;
static const u16                   mcsTrackPositionValueLen = 4;

#if LE_AUDIO_MCS_PLAYBACK_SPEED
_attribute_ble_data_retention_ char mcsPlaybackSpeedValue    = 0;
static const u16                    mcsPlaybackSpeedValueLen = 1;
#endif

#if LE_AUDIO_MCS_SEEKING_SPEED
_attribute_ble_data_retention_ char mcsSeekingSpeedValue    = 0;
static const u16                    mcsSeekingSpeedValueLen = 1;
#endif

#if LE_AUDIO_MCS_CURRENT_TRACK_OBJECT_ID
_attribute_ble_data_retention_ u8  mcsCurrentTrackSegmentsObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsCurrentTrackSegmentsObjectIDValueLen;

_attribute_ble_data_retention_ u8  mcsCurrentTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsCurrentTrackObjectIDValueLen;

_attribute_ble_data_retention_ u8  mcsNextTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsNextTrackObjectIDValueLen;

_attribute_ble_data_retention_ u8  mcsParentGroupTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsParentGroupTrackObjectIDValueLen;

_attribute_ble_data_retention_ u8  mcsCurrentGroupTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsCurrentGroupTrackObjectIDValueLen;
#endif

#if LE_AUDIO_MCS_PLAYING_ORDER
_attribute_ble_data_retention_ u8 mcsPlayingOrderValue    = 0x01;
static const u16                  mcsPlayingOrderValueLen = 1;
#endif

#if LE_AUDIO_MCS_PLAYING_ORDERS_SUPPORTED
_attribute_ble_data_retention_ u16 mcsPlayingOrderSupportedValue;
static const u16                   mcsPlayingOrderSupportedValueLen = 2;
#endif

_attribute_ble_data_retention_ u8 mcsMediaStateValue    = 0x00;
static const u16                  mcsMediaStateValueLen = 1;

#if LE_AUDIO_MCS_MEDIA_CONTROL_POINT
_attribute_ble_data_retention_ u8  mcsMediaControlPointValue[5];
_attribute_ble_data_retention_ u16 mcsMediaControlPointValueLen;

_attribute_ble_data_retention_ u32 mcsMediaControlPointSupportedValue;
static const u16                   mcsMediaControlPointSupportedValueLen = 4;
#endif

#if LE_AUDIO_MCS_SEARCH_RESULTS_OBJECT_ID
_attribute_ble_data_retention_ u8  mcsSearchResultsObjectIDValue[6];
_attribute_ble_data_retention_ u16 mcsSearchResultsObjectIDValueLen;

_attribute_ble_data_retention_ u8 mcsSearchControlPointValue;
static const u16                  mcsSearchControlPointValueLen = sizeof(mcsSearchControlPointValue);
#endif

_attribute_ble_data_retention_ u8 mcsCCIDValue;
static const u16                  mcsCCIDValueLen = sizeof(mcsCCIDValue);

extern const u16 otsIncludeValue[3];

/*
 * @brief the structure for default MCS service List.
 */
static const atts_attribute_t mcsList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceGenericMediaControlUuid),

        ATTS_INCLUDE_DEFINE(&otsIncludeValue),

        //Media Player Name
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicMediaPlayerNameUuid, mcsMediaPlayerNameValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_MCS_MEDIA_PLAYER_ICON_OBJECT_ID
        //Media Player Icon Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicMediaPlayerIconObjectIdUuid, mcsMediaPlayerIconObjectIDValue),
#endif

#if LE_AUDIO_MCS_MEDIA_PLAYER_ICON_URL
        //Media Player Icon URL
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicMediaPlayerIconUrlUuid, mcsMediaPlayerIconURLValue),
#endif

        //Track Changed
        ATTS_CHARACTERISTIC_DECLARATIONS(charPropNotify),
        {0, ATT_16_UUID_LEN, (u8 *)(size_t)characteristicTrackChangedUuid, (u16 *)(size_t)&gMcsTrackChangedLen, 0, NULL, 0},
        ATTS_COMMON_CCC_DEFINE,

        //Track Title
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicTrackTitleUuid, mcsTrackTitleValue),
        ATTS_COMMON_CCC_DEFINE,

        //Track Duration
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicTrackDurationUuid, mcsTrackDurationValue),
        ATTS_COMMON_CCC_DEFINE,

        //Track Position
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithout, characteristicTrackPositionUuid, mcsTrackPositionValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_MCS_PLAYBACK_SPEED
        //Playback Speed
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithoutNotify, characteristicPlaybackSpeedUuid, mcsPlaybackSpeedValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_MCS_SEEKING_SPEED
        //Seeking Speed
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicSeekingSpeedUuid, mcsSeekingSpeedValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_MCS_CURRENT_TRACK_OBJECT_ID
        //Current Track Segments Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicCurrentTrackSegmentsObjectIdUuid, mcsCurrentTrackSegmentsObjectIDValue),

        //Current Track Object ID
        ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteWriteWithoutNotify, characteristicCurrentTrackObjectIdUuid, mcsCurrentTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Next Track Object ID
        ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteWriteWithoutNotify, characteristicNextTrackObjectIdUuid, mcsNextTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Parent Group Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicParentGroupObjectIdUuid, mcsParentGroupTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Current Group Object ID
        ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteWriteWithoutNotify, characteristicCurrentGroupObjectIdUuid, mcsCurrentGroupTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_MCS_PLAYING_ORDER
        //Playing Order
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithoutNotify, characteristicPlayingOrderUuid, mcsPlayingOrderValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_MCS_PLAYING_ORDERS_SUPPORTED
        //Playing Order Supported
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicPlayingOrdersSupportedUuid, mcsPlayingOrderSupportedValue),
#endif

        //Media State
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_NOCB(charPropReadNotfiy, characteristicMediaStateUuid, mcsMediaStateValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_MCS_MEDIA_CONTROL_POINT
        //Media Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_POINT_CB(charPropWriteWriteWithoutNotify, characteristicMediaControlPointUuid, mcsMediaControlPointValue),
        ATTS_COMMON_CCC_DEFINE,

        //Media Control Point Supported
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicMediaCtrlPointOpSupportedUuid, mcsMediaControlPointSupportedValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_MCS_SEARCH_RESULTS_OBJECT_ID
        //Search Results Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropWriteWriteWithoutNotify, characteristicSearchResultsObjectIdUuid, mcsSearchResultsObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Search Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_ENTITY_CB(charPropReadNotfiy, characteristicSearchControlPointUuid, mcsSearchControlPointValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

        //Content Control ID(CCID)
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicContentControlIdUuid, mcsCCIDValue),
};

/*
 * @brief the structure for default MCS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcMcsGroup =
    {
        NULL,
        mcsList,
        NULL,
        NULL,
        MCS_START_HDL,
        0,
};

/**
 * @brief      for user add default MCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addMcsGroup(void)
{
    blc_svc_addOtsGroup();
    svcMcsGroup.endHandle = svcMcsGroup.startHandle + ARRAY_SIZE(mcsList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcMcsGroup);
}

/**
 * @brief      for user remove default MCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeMcsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(MCS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in MCS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_mcsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    blc_svc_otsCbackRegister(readCback, writeCback);
    svcMcsGroup.readCback  = readCback;
    svcMcsGroup.writeCback = writeCback;
}
