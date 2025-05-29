/********************************************************************************************************
 * @file    svc_gmcs.c
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

#ifndef LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_OBJECT_ID
    #define LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_OBJECT_ID 1
#endif

#ifndef LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_URL
    #define LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_URL 1
#endif

#ifndef LE_AUDIO_GMCS_PLAYBACK_SPEED
    #define LE_AUDIO_GMCS_PLAYBACK_SPEED 1
#endif

#ifndef LE_AUDIO_GMCS_SEEKING_SPEED
    #define LE_AUDIO_GMCS_SEEKING_SPEED 1
#endif

#ifndef LE_AUDIO_GMCS_CURRENT_TRACK_OBJECT_ID
    #define LE_AUDIO_GMCS_CURRENT_TRACK_OBJECT_ID 1
#endif

#ifndef LE_AUDIO_GMCS_PLAYING_ORDER
    #define LE_AUDIO_GMCS_PLAYING_ORDER 1
#endif

#ifndef LE_AUDIO_GMCS_PLAYING_ORDERS_SUPPORTED
    #define LE_AUDIO_GMCS_PLAYING_ORDERS_SUPPORTED 1
#endif

#ifndef LE_AUDIO_GMCS_MEDIA_CONTROL_POINT
    #define LE_AUDIO_GMCS_MEDIA_CONTROL_POINT 1
#endif

#ifndef LE_AUDIO_GMCS_SEARCH_RESULTS_OBJECT_ID
    #define LE_AUDIO_GMCS_SEARCH_RESULTS_OBJECT_ID 1
#endif

#ifndef LE_AUDIO_GMCS_PMEDIA_PLAYER_NAME_MAX_SIZE
    #define LE_AUDIO_GMCS_PMEDIA_PLAYER_NAME_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_GMCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE
    #define LE_AUDIO_GMCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE 64
#endif

#ifndef LE_AUDIO_GMCS_TRACK_TITLE_MAX_SIZE
    #define LE_AUDIO_GMCS_TRACK_TITLE_MAX_SIZE 64
#endif

#define GMCS_START_HDL SERVICE_GENERIC_MEDIA_CONTROL_HDL

_attribute_ble_data_retention_ u8  gmcsMediaPlayerNameValue[LE_AUDIO_GMCS_PMEDIA_PLAYER_NAME_MAX_SIZE];
_attribute_ble_data_retention_ u16 gmcsMediaPlayerNameValueLen;
const u16                          gmcsMediaPlayerNameMaxSize = sizeof(gmcsMediaPlayerNameValue);

#if LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_OBJECT_ID
_attribute_ble_data_retention_ u8  gmcsMediaPlayerIconObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsMediaPlayerIconObjectIDValueLen;
#endif

const u16 gmcsMediaPlayerIconURLMaxSize = LE_AUDIO_GMCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE;
#if LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_URL
_attribute_ble_data_retention_ u8  gmcsMediaPlayerIconURLValue[LE_AUDIO_GMCS_PMEDIA_PLAYER_ICON_URL_MAX_SIZE];
_attribute_ble_data_retention_ u16 gmcsMediaPlayerIconURLValueLen;
#endif

static const u16 gMcsTrackChangedLen = 0;

_attribute_ble_data_retention_ u8  gmcsTrackTitleValue[LE_AUDIO_GMCS_TRACK_TITLE_MAX_SIZE];
_attribute_ble_data_retention_ u16 gmcsTrackTitleValueLen;
const u16                          gmcsTrackTitleMaxSize = sizeof(gmcsTrackTitleValue);

_attribute_ble_data_retention_ u32 gmcsTrackDurationValue    = 0xFFFFFFFF;
static const u16                   gmcsTrackDurationValueLen = 4;

_attribute_ble_data_retention_ u32 gmcsTrackPositionValue    = 0xFFFFFFFF;
static const u16                   gmcsTrackPositionValueLen = 4;

#if LE_AUDIO_GMCS_PLAYBACK_SPEED
_attribute_ble_data_retention_ char gmcsPlaybackSpeedValue    = 0;
static const u16                    gmcsPlaybackSpeedValueLen = 1;
#endif

#if LE_AUDIO_GMCS_SEEKING_SPEED
_attribute_ble_data_retention_ char gmcsSeekingSpeedValue    = 0;
static const u16                    gmcsSeekingSpeedValueLen = 1;
#endif

#if LE_AUDIO_GMCS_CURRENT_TRACK_OBJECT_ID
_attribute_ble_data_retention_ u8  gmcsCurrentTrackSegmentsObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsCurrentTrackSegmentsObjectIDValueLen;

_attribute_ble_data_retention_ u8  gmcsCurrentTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsCurrentTrackObjectIDValueLen;

_attribute_ble_data_retention_ u8  gmcsNextTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsNextTrackObjectIDValueLen;

_attribute_ble_data_retention_ u8  gmcsParentGroupTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsParentGroupTrackObjectIDValueLen;

_attribute_ble_data_retention_ u8  gmcsCurrentGroupTrackObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsCurrentGroupTrackObjectIDValueLen;
#endif

#if LE_AUDIO_GMCS_PLAYING_ORDER
_attribute_ble_data_retention_ u8 gmcsPlayingOrderValue    = 0x01;
static const u16                  gmcsPlayingOrderValueLen = 1;
#endif

#if LE_AUDIO_GMCS_PLAYING_ORDERS_SUPPORTED
_attribute_ble_data_retention_ u16 gmcsPlayingOrderSupportedValue;
static const u16                   gmcsPlayingOrderSupportedValueLen = 2;
#endif

_attribute_ble_data_retention_ u8 gmcsMediaStateValue    = 0x00;
static const u16                  gmcsMediaStateValueLen = 1;

#if LE_AUDIO_GMCS_MEDIA_CONTROL_POINT
_attribute_ble_data_retention_ u8  gmcsMediaControlPointValue[5];
_attribute_ble_data_retention_ u16 gmcsMediaControlPointValueLen;

_attribute_ble_data_retention_ u32 gmcsMediaControlPointSupportedValue;
static const u16                   gmcsMediaControlPointSupportedValueLen = 4;
#endif

#if LE_AUDIO_GMCS_SEARCH_RESULTS_OBJECT_ID
_attribute_ble_data_retention_ u8  gmcsSearchResultsObjectIDValue[6];
_attribute_ble_data_retention_ u16 gmcsSearchResultsObjectIDValueLen;

_attribute_ble_data_retention_ u8 gmcsSearchControlPointValue;
static const u16                  gmcsSearchControlPointValueLen = sizeof(gmcsSearchControlPointValue);
#endif

_attribute_ble_data_retention_ u8 gmcsCCIDValue;
static const u16                  gmcsCCIDValueLen = sizeof(gmcsCCIDValue);

extern const u16 otsIncludeValue[3];

/*
 * @brief the structure for default GMCS service List.
 */
static const atts_attribute_t gmcsList[] =
    {
        ATTS_PRIMARY_SERVICE(serviceGenericMediaControlUuid),

        ATTS_INCLUDE_DEFINE(&otsIncludeValue),

        //Media Player Name
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicMediaPlayerNameUuid, gmcsMediaPlayerNameValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_OBJECT_ID
        //Media Player Icon Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicMediaPlayerIconObjectIdUuid, gmcsMediaPlayerIconObjectIDValue),
#endif

#if LE_AUDIO_GMCS_MEDIA_PLAYER_ICON_URL
        //Media Player Icon URL
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicMediaPlayerIconUrlUuid, gmcsMediaPlayerIconURLValue),
#endif

        //Track Changed
        ATTS_CHARACTERISTIC_DECLARATIONS(charPropNotify),
        {0, ATT_16_UUID_LEN, (u8 *)(size_t)characteristicTrackChangedUuid, (u16 *)(size_t)&gMcsTrackChangedLen, 0, NULL, 0},
        ATTS_COMMON_CCC_DEFINE,

        //Track Title
        ATTS_CHAR_UUID_ENCR_READ_POINT_CB(charPropReadNotfiy, characteristicTrackTitleUuid, gmcsTrackTitleValue),
        ATTS_COMMON_CCC_DEFINE,

        //Track Duration
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicTrackDurationUuid, gmcsTrackDurationValue),
        ATTS_COMMON_CCC_DEFINE,

        //Track Position
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithout, characteristicTrackPositionUuid, gmcsTrackPositionValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_GMCS_PLAYBACK_SPEED
        //Playback Speed
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithoutNotify, characteristicPlaybackSpeedUuid, gmcsPlaybackSpeedValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_GMCS_SEEKING_SPEED
        //Seeking Speed
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicSeekingSpeedUuid, gmcsSeekingSpeedValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_GMCS_CURRENT_TRACK_OBJECT_ID
        //Current Track Segments Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicCurrentTrackSegmentsObjectIdUuid, gmcsCurrentTrackSegmentsObjectIDValue),

        //Current Track Object ID
        ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteWriteWithoutNotify, characteristicCurrentTrackObjectIdUuid, gmcsCurrentTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Next Track Object ID
        ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteWriteWithoutNotify, characteristicNextTrackObjectIdUuid, gmcsNextTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Parent Group Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropReadNotfiy, characteristicParentGroupObjectIdUuid, gmcsParentGroupTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Current Group Object ID
        ATTS_CHAR_UUID_ENCR_RDWR_POINT_WCB(charPropReadWriteWriteWithoutNotify, characteristicCurrentGroupObjectIdUuid, gmcsCurrentGroupTrackObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_GMCS_PLAYING_ORDER
        //Playing Order
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_WCB(charPropReadWriteWriteWithoutNotify, characteristicPlayingOrderUuid, gmcsPlayingOrderValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_GMCS_PLAYING_ORDERS_SUPPORTED
        //Playing Order Supported
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicPlayingOrdersSupportedUuid, gmcsPlayingOrderSupportedValue),
#endif

        //Media State
        ATTS_CHAR_UUID_ENCR_RDWR_ENTITY_NOCB(charPropReadNotfiy, characteristicMediaStateUuid, gmcsMediaStateValue),
        ATTS_COMMON_CCC_DEFINE,

#if LE_AUDIO_GMCS_MEDIA_CONTROL_POINT
        //Media Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_POINT_CB(charPropWriteWriteWithoutNotify, characteristicMediaControlPointUuid, gmcsMediaControlPointValue),
        ATTS_COMMON_CCC_DEFINE,

        //Media Control Point Supported
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicMediaCtrlPointOpSupportedUuid, gmcsMediaControlPointSupportedValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

#if LE_AUDIO_GMCS_SEARCH_RESULTS_OBJECT_ID
        //Search Results Object ID
        ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropWriteWriteWithoutNotify, characteristicSearchResultsObjectIdUuid, gmcsSearchResultsObjectIDValue),
        ATTS_COMMON_CCC_DEFINE,

        //Search Control Point
        ATTS_CHAR_UUID_ENCR_WRITE_ENTITY_CB(charPropReadNotfiy, characteristicSearchControlPointUuid, gmcsSearchControlPointValue),
        ATTS_COMMON_CCC_DEFINE,
#endif

        //Content Control ID(CCID)
        ATTS_CHAR_UUID_ENCR_READ_ENTITY_NOCB(charPropRead, characteristicContentControlIdUuid, gmcsCCIDValue),
};

/*
 * @brief the structure for default GMCS service group.
 */
_attribute_ble_data_retention_ static atts_group_t svcGmcsGroup =
    {
        NULL,
        gmcsList,
        NULL,
        NULL,
        GMCS_START_HDL,
        0,
};

/**
 * @brief      for user add default GMCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addGmcsGroup(void)
{
    blc_svc_addOtsGroup();
    svcGmcsGroup.endHandle = svcGmcsGroup.startHandle + ARRAY_SIZE(gmcsList) - 1;
    blc_gatts_addAttributeServiceGroup(&svcGmcsGroup);
}

/**
 * @brief      for user remove default GMCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeGmcsGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(GMCS_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in GMCS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_gmcsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    blc_svc_otsCbackRegister(readCback, writeCback);
    svcGmcsGroup.readCback  = readCback;
    svcGmcsGroup.writeCback = writeCback;
}
