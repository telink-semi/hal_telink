/********************************************************************************************************
 * @file    mcs.h
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

#include "mcs_client_buf.h"
#include "mcs_server_buf.h"

/******************************* (G)MCS Common Start **********************************************************************/

/* Media State values */
typedef enum
{
    GMCS_MEDIA_STATE_INACTIVE = 0x00,
    GMCS_MEDIA_STATE_PLAYING  = 0x01,
    GMCS_MEDIA_STATE_PAUSED   = 0x02,
    GMCS_MEDIA_STATE_SEEKING  = 0x03,
    GMCS_MEDIA_STATE_RFU      = 0xff,
} blc_mcs_mediaState_enum;

//media control opcode
typedef enum
{
    BLC_MCS_OPCODE_NONE             = 0x00,
    BLC_MCS_OPCODE_PLAY             = 0x01,
    BLC_MCS_OPCODE_PAUSE            = 0x02,
    BLC_MCS_OPCODE_FAST_REWIND      = 0x03,
    BLC_MCS_OPCODE_FAST_FORWARD     = 0x04,
    BLC_MCS_OPCODE_STOP             = 0x05,
    BLC_MCS_OPCODE_MOVE_RELATIVE    = 0x10,
    BLC_MCS_OPCODE_PREVIOUS_SEGMENT = 0x20,
    BLC_MCS_OPCODE_NEXT_SEGMENT     = 0x21,
    BLC_MCS_OPCODE_FIRST_SEGMENT    = 0x22,
    BLC_MCS_OPCODE_LAST_SEGMENT     = 0x23,
    BLC_MCS_OPCODE_GOTO_SEGMENT     = 0x24,
    BLC_MCS_OPCODE_PREVIOUS_TRACK   = 0x30,
    BLC_MCS_OPCODE_NEXT_TRACK       = 0x31,
    BLC_MCS_OPCODE_FIRST_TRACK      = 0x32,
    BLC_MCS_OPCODE_LAST_TRACK       = 0x33,
    BLC_MCS_OPCODE_GOTO_TRACK       = 0x34,
    BLC_MCS_OPCODE_PREVIOUS_GROUP   = 0x40,
    BLC_MCS_OPCODE_NEXT_GROUP       = 0x41,
    BLC_MCS_OPCODE_FIRST_GROUP      = 0x42,
    BLC_MCS_OPCODE_LAST_GROUP       = 0x43,
    BLC_MCS_OPCODE_GOTO_GROUP       = 0x44,
} blc_mcs_mediaCtrlPointOpcode_enum;

//media support control opcode
typedef enum
{
    BLC_MCS_OPCODE_SUPPORT_PLAY             = BIT(0),
    BLC_MCS_OPCODE_SUPPORT_PAUSE            = BIT(1),
    BLC_MCS_OPCODE_SUPPORT_FAST_REWIND      = BIT(2),
    BLC_MCS_OPCODE_SUPPORT_FAST_FORWARD     = BIT(3),
    BLC_MCS_OPCODE_SUPPORT_STOP             = BIT(4),
    BLC_MCS_OPCODE_SUPPORT_MOVE_RELATIVE    = BIT(5),
    BLC_MCS_OPCODE_SUPPORT_PREVIOUS_SEGMENT = BIT(6),
    BLC_MCS_OPCODE_SUPPORT_NEXT_SEGMENT     = BIT(7),
    BLC_MCS_OPCODE_SUPPORT_FIRST_SEGMENT    = BIT(8),
    BLC_MCS_OPCODE_SUPPORT_LAST_SEGMENT     = BIT(9),
    BLC_MCS_OPCODE_SUPPORT_GOTO_SEGMENT     = BIT(10),
    BLC_MCS_OPCODE_SUPPORT_PREVIOUS_TRACK   = BIT(11),
    BLC_MCS_OPCODE_SUPPORT_NEXT_TRACK       = BIT(12),
    BLC_MCS_OPCODE_SUPPORT_FIRST_TRACK      = BIT(13),
    BLC_MCS_OPCODE_SUPPORT_LAST_TRACK       = BIT(14),
    BLC_MCS_OPCODE_SUPPORT_GOTO_TRACK       = BIT(15),
    BLC_MCS_OPCODE_SUPPORT_PREVIOUS_GROUP   = BIT(16),
    BLC_MCS_OPCODE_SUPPORT_NEXT_GROUP       = BIT(17),
    BLC_MCS_OPCODE_SUPPORT_FIRST_GROUP      = BIT(18),
    BLC_MCS_OPCODE_SUPPORT_LAST_GROUP       = BIT(19),
    BLC_MCS_OPCODE_SUPPORT_GOTO_GROUP       = BIT(20),
    BLC_MCS_OPCODE_SUPPORT_RFU              = 0xFFFFFFFF,
} blc_mcs_mediaCtrlPointOpcodeSupport_enum;

//media control result
typedef enum
{
    //Action requested by the opcode write was completed successfully.
    BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS = 0x01,
    //An invalid or unsupported opcode was used for the Media Control Point write.
    BLC_MCS_MEDIA_CTRL_RESULT_OP_NOT_SUPP,
    //The Media Player State characteristic value is Inactive when the opcode is
    //received or the result of the requested action of the opcode results in the Media
    //Player State characteristic being set to Inactive.
    BLC_MCS_MEDIA_CTRL_RESULT_MEDIA_PLAYER_INACTIVE,
    //The requested action of any Media Control Point write cannot be completed
    //successfully because of a condition within the player.
    BLC_MCS_MEDIA_CTRL_RESULT_CMD_CANNOT_BE_COMPLETED
} blc_mcs_mediaCtrlPointResult_enum;

//search control items
typedef enum
{
    BLC_MCS_SEARCH_TYPE_TRACK_NAME = 0x01,
    BLC_MCS_SEARCH_TYPE_ARTIST_NAME,
    BLC_MCS_SEARCH_TYPE_ALBUM_NAME,
    BLC_MCS_SEARCH_TYPE_GROUP_NAME,
    BLC_MCS_SEARCH_TYPE_EARLIEST_YEAR,
    BLC_MCS_SEARCH_TYPE_LATEST_YEAR,
    BLC_MCS_SEARCH_TYPE_GENRE,
    BLC_MCS_SEARCH_TYPE_ONLY_TRACKS,
    BLC_MCS_SEARCH_TYPE_ONLY_GROUPS,
    BLC_MCS_SEARCH_TYPE_RFU,
} blc_mcs_searchCtrlPointType_enum;

typedef enum
{
    //Search request was accepted; search has started.
    BLC_MCS_SEARCH_CTRL_RESULT_SUCCESS,
    //Search request was invalid; no search started.
    BLC_MCS_SEARCH_CTRL_RESULT_FAILURE,
} blc_mcs_searchCtrlPointResult_enum;

typedef enum
{
    BLC_MCS_PLAYING_ORDER_SINGLE_ONCE = 0x01,
    BLC_MCS_PLAYING_ORDER_SINGLE_REPEAT,
    BLC_MCS_PLAYING_ORDER_IN_ORDER_ONCE,
    BLC_MCS_PLAYING_ORDER_IN_ORDER_REPEAT,
    BLC_MCS_PLAYING_ORDER_OLDEST_ONCE,
    BLC_MCS_PLAYING_ORDER_OLDEST_REPEAT,
    BLC_MCS_PLAYING_ORDER_NEWEST_ONCE,
    BLC_MCS_PLAYING_ORDER_NEWEST_REPEAT,
    BLC_MCS_PLAYING_ORDER_SHUFFLE_ONCE,
    BLC_MCS_PLAYING_ORDER_SHUFFLE_REPEAT,
    BLC_MCS_PLAYING_ORDER_RFU = 0XFF,
} blc_mcs_playingOrder_enum;

typedef enum
{
    BLC_MCS_PLAYING_ORDER_SUPPORT_SINGLE_ONCE     = 0x0001,
    BLC_MCS_PLAYING_ORDER_SUPPORT_SINGLE_REPEAT   = 0x0002,
    BLC_MCS_PLAYING_ORDER_SUPPORT_IN_ORDER_ONCE   = 0x0004,
    BLC_MCS_PLAYING_ORDER_SUPPORT_IN_ORDER_REPEAT = 0x0008,
    BLC_MCS_PLAYING_ORDER_SUPPORT_OLDEST_ONCE     = 0x0010,
    BLC_MCS_PLAYING_ORDER_SUPPORT_OLDEST_REPEAT   = 0x0020,
    BLC_MCS_PLAYING_ORDER_SUPPORT_NEWEST_ONCE     = 0x0040,
    BLC_MCS_PLAYING_ORDER_SUPPORT_NEWEST_REPEAT   = 0x0080,
    BLC_MCS_PLAYING_ORDER_SUPPORT_SHUFFLE_ONCE    = 0x0100,
    BLC_MCS_PLAYING_ORDER_SUPPORT_SHUFFLE_REPEAT  = 0x0200,
    BLC_MCS_PLAYING_ORDER_SUPPORT_RFU             = 0XFFFF,
} blc_mcs_playingOrderSupported_enum;

/******************************* (G)MCS Common End **********************************************************************/


/******************************* (G)MCS Client Start **********************************************************************/

//GMCS Client Event ID
typedef enum
{
    AUDIO_EVT_GMCSC_START = AUDIO_EVT_TYPE_GMCSC,
    AUDIO_EVT_MCSC_MEDIA_PLAYER_NAME,             //refer to 'blc_mcsc_mediaPlayerNameEvt_t'
    AUDIO_EVT_MCSC_MEDIA_TRACK_CHANGED,           //refer to 'blc_mcsc_mediaTrackChangedEvt_t'
    AUDIO_EVT_MCSC_MEDIA_TRACK_TITLE,             //refer to 'blc_mcsc_mediaTrackTitleEvt_t'
    AUDIO_EVT_MCSC_MEDIA_TRACK_DURATION,          //refer to 'blc_mcsc_mediaTrackDurationEvt_t'
    AUDIO_EVT_MCSC_MEDIA_TRACK_POSITION,          //refer to 'blc_mcsc_mediaTrackPositionEvt_t'
    AUDIO_EVT_MCSC_MEDIA_PLAYBACK_SPEED,          //refer to 'blc_mcsc_mediaPlaybackSpeedEvt_t'
    AUDIO_EVT_MCSC_MEDIA_SEEKING_SPEED,           //refer to 'blc_mcsc_mediaSeekingSpeedEvt_t'
    AUDIO_EVT_MCSC_MEDIA_CURRENT_TRACK_OBJECT_ID, //refer to 'blc_mcsc_mediaCurrentTrackObjectIdEvt_t'
    AUDIO_EVT_MCSC_MEDIA_NEXT_TRACK_OBJECT_ID,    //refer to 'blc_mcsc_mediaNextTrackObjectIdEvt_t'
    AUDIO_EVT_MCSC_MEDIA_PARENT_GROUP_OBJECT_ID,  //refer to 'blc_mcsc_mediaParentGroupObjectIdEvt_t'
    AUDIO_EVT_MCSC_MEDIA_CURRENT_GROUP_OBJECT_ID, //refer to 'blc_mcsc_mediaCurrentGroupObjectIdEvt_t'
    AUDIO_EVT_MCSC_MEDIA_PLAYING_ORDER,           //refer to 'blc_mcsc_mediaPlayingOrderEvt_t'
    AUDIO_EVT_MCSC_MEDIA_STATE,                   //refer to 'blc_mcsc_mediaStateEvt_t'
    AUDIO_EVT_MCSC_MEDIA_CTRL_RESULT,             //refer to 'blc_mcsc_mediaCtrlResultEvt_t'
    AUDIO_EVT_MCSC_MEDIA_CTRL_OPCODE_SUPPORT,     //refer to 'blc_mcsc_mediaCtrlOpcodeSupportEvt_t'
    AUDIO_EVT_MCSC_SEARCH_CTRL_RESULT,            //refer to 'blc_mcsc_searchCtrlResultEvt_t'
    AUDIO_EVT_MCSC_SEARCH_RESULT_OBJECT_ID,       //refer to 'blc_mcsc_searchResultObjectIdEvt_t'
} audio_gmcsc_evt_enum;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_PLAYER_NAME"
 */
typedef struct
{
    u16 connHandle;
    u8  mediaNameLen;
    u8  mediaName[50];
} blc_mcsc_mediaPlayerNameEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_TRACK_CHANGED"
 */
typedef struct
{
    u16 connHandle;
} blc_mcsc_mediaTrackChangedEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_TRACK_TITLE"
 */
typedef struct
{
    u16 connHandle;
    u8  trackTitleLen;
    u8  trackTitle[50];
} blc_mcsc_mediaTrackTitleEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_TRACK_DURATION"
 */
typedef struct
{
    u16 connHandle;
    s32 trackDuration;
} blc_mcsc_mediaTrackDurationEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_TRACK_POSITION"
 */
typedef struct
{
    u16 connHandle;
    s32 trackPosition;
} blc_mcsc_mediaTrackPositionEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_PLAYBACK_SPEED"
 */
typedef struct
{
    u16 connHandle;
    s8  playbackSpeed;
} blc_mcsc_mediaPlaybackSpeedEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_SEEKING_SPEED"
 */
typedef struct
{
    u16 connHandle;
    s8  seekingSpeed;
} blc_mcsc_mediaSeekingSpeedEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_CURRENT_TRACK_OBJECT_ID", "AUDIO_EVT_MCSC_MEDIA_NEXT_TRACK_OBJECT_ID"
 *                              "AUDIO_EVT_MCSC_MEDIA_PARENT_GROUP_OBJECT_ID", "AUDIO_EVT_MCSC_MEDIA_CURRENT_GROUP_OBJECT_ID",
 *                              "AUDIO_EVT_MCSC_SEARCH_RESULT_OBJECT_ID"
 */
typedef struct
{
    u16             connHandle;
    blc_object_id_t object;
} blc_mcsc_mediaCurrentTrackObjectIdEvt_t, blc_mcsc_mediaNextTrackObjectIdEvt_t,
    blc_mcsc_mediaParentGroupObjectIdEvt_t, blc_mcsc_mediaCurrentGroupObjectIdEvt_t,
    blc_mcsc_searchResultObjectIdEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_PLAYING_ORDER"
 */
typedef struct
{
    u16 connHandle;
    int order; //blc_mcs_playingOrder_enum
} blc_mcsc_mediaPlayingOrderEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_STATE"
 */
typedef struct
{
    u16 connHandle;
    int state; //blc_mcs_mediaState_enum
} blc_mcsc_mediaStateEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_CTRL_RESULT"
 */
typedef struct
{
    u16 connHandle;
    int op;     //blc_mcs_mediaCtrlPointOpcode_enum
    int result; //blc_mcs_mediaCtrlPointResult_enum
} blc_mcsc_mediaCtrlResultEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_MEDIA_CTRL_OPCODE_SUPPORT"
 */
typedef struct
{
    u16 connHandle;
    u32 supportOpcode; //search for 'blc_mcs_mediaCtrlPointOpcodeSupport_enum'
} blc_mcsc_mediaCtrlOpcodeSupportEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_MCSC_SEARCH_CTRL_RESULT"
 */
typedef struct
{
    u16 connHandle;
    int result; //blc_mcs_searchCtrlPointResult_enum
} blc_mcsc_searchCtrlResultEvt_t;

/**
 * @brief       This function serves to register media control Client include MCS, GMCS and OTS.
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerMediaControlClient(const blc_mcpc_regParam_t *param);


//GMCS Client Read Characteristic Value Operation API
int blc_gmcsc_readMediaPlayerName(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readMediaPlayerIconObjectID(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readMediaPlayerIconURL(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readTrackTitle(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readTrackDuration(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readTrackPosition(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readPlaybackSpeed(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readSeekingSpeed(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readCurrentTrackSegmentsObjectID(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readCurrentTrackObjectID(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readNextTrackObjectID(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readParentGroupObjectID(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readCurrentGroupObjectID(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readPlayingOrder(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readPlayingOrdersSupp(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readMediaState(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readMediaCtrlPointOpSupp(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readSearchResObjectId(u16 connHandle, prf_read_cb_t readCb);
int blc_gmcsc_readContentCtrlID(u16 connHandle, prf_read_cb_t readCb);

//GMCS Client Get Characteristic Value Operation API
int blc_gmcsc_getMediaPlayerName(u16 connHandle, u8 *mediaPlayerName, u16 *len);
int blc_gmcsc_getMediaPlayerIconObjectID(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getMediaPlayerIconURL(u16 connHandle, u8 *url, u16 *len);
int blc_gmcsc_getTrackTitle(u16 connHandle, u8 *title, u16 *len);
int blc_gmcsc_getTrackDuration(u16 connHandle, u32 *duration);
int blc_gmcsc_getTrackPosition(u16 connHandle, u32 *position);
int blc_gmcsc_getPlaybackSpeed(u16 connHandle, s8 *playbackSpeed);
int blc_gmcsc_getSeekingSpeed(u16 connHandle, s8 *seekingSpeed);
int blc_gmcsc_getCurrentTrackSegmentsObjectID(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getCurrentTrackObjectID(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getNextTrackObjectID(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getParentGroupObjectID(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getCurrentGroupObjectID(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getPlayingOrder(u16 connHandle, u8 *playingOrder);
int blc_gmcsc_getPlayingOrdersSupp(u16 connHandle, u16 *playingOrderSupp);
int blc_gmcsc_getMediaState(u16 connHandle, blc_mcs_mediaState_enum *mediaState);
int blc_gmcsc_getMediaCtrlPointOpSupp(u16 connHandle, u32 *mediaCtrlPointOpSupp);
int blc_gmcsc_getSearchResObjectId(u16 connHandle, blc_object_id_t *id);
int blc_gmcsc_getContentCtrlID(u16 connHandle, u8 *ccid);

//GMCS Client Write Characteristic Value Operation API
int blc_gmcsc_writeTrackPosition(u16 connHandle, u32 trackPosition, prf_write_cb_t writeCb);
int blc_gmcsc_writePlaybackSpeed(u16 connHandle, u8 playbackSpeed, prf_write_cb_t writeCb);
int blc_gmcsc_writeTrackPositionWithoutRsp(u16 connHandle, u32 trackPosition);
int blc_gmcsc_writePlaybackSpeedWithoutRsp(u16 connHandle, u8 playbackSpeed);
int blc_gmcsc_writeCurrentTrackObjectID(u16 connHandle, blc_object_id_t id, prf_write_cb_t writeCb);
int blc_gmcsc_writeCurrentTrackObjectIDWithoutRsp(u16 connHandle, blc_object_id_t id);
int blc_gmcsc_writeNextTrackObjectID(u16 connHandle, blc_object_id_t id, prf_write_cb_t writeCb);
int blc_gmcsc_writeNextTrackObjectIDWithoutRsp(u16 connHandle, blc_object_id_t id);
int blc_gmcsc_writeCurrentGroupObjectID(u16 connHandle, blc_object_id_t id, prf_write_cb_t writeCb);
int blc_gmcsc_writeCurrentGroupObjectIDWithoutRsp(u16 connHandle, blc_object_id_t id);
int blc_gmcsc_writePlayingOrderObjectID(u16 connHandle, blc_object_id_t id, prf_write_cb_t writeCb);
int blc_gmcsc_writePlayingOrderObjectIDWithoutRsp(u16 connHandle, blc_object_id_t id);
int blc_gmcsc_writeMediaCtrlPoint(u16 connHandle, blc_mcs_mediaCtrlPointOpcode_enum opcode, u8 *param, u16 paramLen, prf_write_cb_t writeCb);
int blc_gmcsc_writeMediaCtrlPointWithoutRsp(u16 connHandle, blc_mcs_mediaCtrlPointOpcode_enum opcode, u8 *param, u16 paramLen);
int blc_gmcsc_writeSearchCtrlPoint(u16 connHandle, blc_mcs_searchCtrlPointType_enum opcode, u8 *param, u16 len, prf_write_cb_t writeCb);
int blc_gmcsc_writeSearchCtrlPointWithoutRsp(u16 connHandle, blc_mcs_searchCtrlPointType_enum opcode, u8 *param, u16 len);

//GMCS client Media Control Point API
int blc_gmcsc_writeMediaControl(u16 connHandle, blc_mcs_mediaCtrlPointOpcode_enum opcode, u8 *param, u16 paramLen);

/**
 * @brief       This function use send play command to start playing the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeStartPlayingCurrentTrack(u16 connHandle);

/**
 * @brief       This function use send pause command to pause the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writePauseCurrentTrack(u16 connHandle);

/**
 * @brief       This function use send fast rewind command to fast rewind the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeFastRewindCurrentTrack(u16 connHandle);

/**
 * @brief       This function use send fast forward command to fast forward the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeFastForwardCurrentTrack(u16 connHandle);

/**
 * @brief       This function use send stop command to Stop current activity and
 *                  return to the paused state and set the current track position to the start of the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeStopActivity(u16 connHandle);

/**
 * @brief       This function use send move relative command to set a new current track position relative to the current track position.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeMoveRelative(u16 connHandle, int offset);

/**
 * @brief       This function use send previous segment command to set the current track position to the starting position of the previous segment of the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writePreviousSegment(u16 connHandle);

/**
 * @brief       This function use send next segment command to set the current track position to the starting position of the next segment of the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeNextSegment(u16 connHandle);

/**
 * @brief       This function use send first segment command to set the current track position to the starting position of the first segment of the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeFirstSegment(u16 connHandle);

/**
 * @brief       This function use send last segment command to set the current track position to the starting position of the last segment of the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeLastSegment(u16 connHandle);

/**
 * @brief       This function use send goto segment command to set the current track position to the starting position of the NTH segment of the current track.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeGotoSegment(u16 connHandle, int n);

/**
 * @brief       This function use send previous track command to set the current track to the previous track based on the playing order.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writePreviousTrack(u16 connHandle);

/**
 * @brief       This function use send next track command to set the current track to the next track based on the playing order.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeNextTrack(u16 connHandle);

/**
 * @brief       This function use send first track command to set the current track to the first track based on the playing order.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeFirstTrack(u16 connHandle);

/**
 * @brief       This function use send last track command to set the current track to the last track based on the playing order.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeLastTrack(u16 connHandle);

/**
 * @brief       This function use send goto track command to set the current track to the NTH track based on the playing order.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeGotoTrack(u16 connHandle, int n);

/**
 * @brief       This function use send previous group command to set the current group to the previous group in the sequence of groups.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writePreviousGroup(u16 connHandle);

/**
 * @brief       This function use send next group command to set the current group to the next group in the sequence of groups.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeNextGroup(u16 connHandle);

/**
 * @brief       This function use send first group command to set the current group to the first group in the sequence of groups.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeFirstGroup(u16 connHandle);

/**
 * @brief       This function use send last group command to set the current group to the last group in the sequence of groups.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeLastGroup(u16 connHandle);

/**
 * @brief       This function use send NTH group command to set the current group to the NTH group in the sequence of groups.
 * @param[in]   connHandle  - ACL connect handle.
 * @return      audio_error_enum
 */
int blc_gmcsc_writeGotoGroup(u16 connHandle, int n);


//GMCS client Search Control Point API
int blc_gmcsc_writeSearchControl(u16 connHandle, blc_mcs_searchCtrlPointType_enum opcode, char *param);
int blc_gmcsc_writeSearchCtrlTrackName(u16 connHandle, char *trackName);
int blc_gmcsc_writeSearchCtrlArtistName(u16 connHandle, char *artistName);
int blc_gmcsc_writeSearchCtrlAlbumName(u16 connHandle, char *albumName);
int blc_gmcsc_writeSearchCtrlGroupName(u16 connHandle, char *groupName);
int blc_gmcsc_writeSearchCtrlEarliestYear(u16 connHandle, char *earliestYear);
int blc_gmcsc_writeSearchCtrlLatestYear(u16 connHandle, char *latestYear);
int blc_gmcsc_writeSearchCtrlGenre(u16 connHandle, char *genre);
int blc_gmcsc_writeSearchCtrlOnlyTracks(u16 connHandle);
int blc_gmcsc_writeSearchCtrlOnlyGroups(u16 connHandle);

/******************************* (G)MCS Client End **********************************************************************/


/******************************* (G)MCS Server Start **********************************************************************/

// Default MCS server par
extern const blc_mcps_regParam_t defaultMcpsParam;

typedef struct
{
    u8 type;
    u8 paramLength;
    u8 param[];
} blc_mcs_search_control_item_t;

//GMCS Server Event ID
typedef enum
{
    AUDIO_EVT_GMCSS_START = AUDIO_EVT_TYPE_GMCSS,
    AUDIO_EVT_GMCSS_TRACK_POSITION,
    AUDIO_EVT_GMCSS_PLAYBACK_SPEED,
    AUDIO_EVT_GMCSS_PLAYING_ORDER,
    AUDIO_EVT_GMCSS_CURRENT_TRACK_OBJECT_ID,
    AUDIO_EVT_GMCSS_NEXT_TRACK_OBJECT_ID,
    AUDIO_EVT_GMCSS_CURRENT_GROUP_OBJECT_ID,
    AUDIO_EVT_GMCSS_SEARCH_CONTROL_POINT,
    AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT,
} audio_gmcss_evt_enum;

typedef struct
{ //Event ID: AUDIO_EVT_GMCSS_TRACK_POSITION
    s32 position;
} blc_mcss_mediaTrackPositionEvt_t;

typedef struct
{              //Event ID: AUDIO_EVT_GMCSS_PLAYING_ORDER
    int order; //blc_mcs_playingOrder_enum
} blc_mcss_mediaPlayingOrderEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GMCSS_PLAYBACK_SPEED
    s8 speed;
} blc_mcss_mediaPlaybackSpeedEvt_t;

typedef struct
{              //Event ID: AUDIO_EVT_GMCSS_MEDIA_CONTROL_POINT
    u8 opcode; //blc_mcs_mediaCtrlPointOpcode_enum
} blc_mcss_mediaControlPointEvt_t;

typedef struct
{
    u8  opcode; //blc_mcs_mediaCtrlPointOpcode_enum
    s32 offset;
} blc_mcss_mediaControlPointMoveRelativeEvt_t;

typedef struct
{
    u8  opcode; //blc_mcs_mediaCtrlPointOpcode_enum
    s32 n;
} blc_mcss_mediaControlPointGotoSegmentEvt_t;

typedef struct
{
    u8  opcode; //blc_mcs_mediaCtrlPointOpcode_enum
    s32 n;
} blc_mcss_mediaControlPointGotoTrackEvt_t;

typedef struct
{
    u8  opcode; //blc_mcs_mediaCtrlPointOpcode_enum
    s32 n;
} blc_mcss_mediaControlPointGotoGroupEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GMCSS_CURRENT_TRACK_OBJECT_ID
    blc_object_id_t id;
} blc_mcss_currentTrackObjectIdEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GMCSS_NEXT_TRACK_OBJECT_ID
    blc_object_id_t id;
} blc_mcss_nextTrackObjectIdEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GMCSS_CURRENT_GROUP_OBJECT_ID
    blc_object_id_t id;
} blc_mcss_currentGroupObjectIdEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_GMCSS_SEARCH_CONTROL_POINT
    u8                            numItems;
    blc_mcs_search_control_item_t items[];
} blc_mcss_currentSearchControlPointEvt_t;

/**
 * @brief       This function serves to register media control Server include MCS, GMCS and OTS.
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerMediaControlServer(const blc_mcps_regParam_t *param);


//GMCS Server Update Characteristic Value Operation API
int blc_gmcss_updateMediaPlayerName(u16 connHandle, u8 *mediaPlayerName, u16 len);
int blc_gmcss_updateMediaPlayerIconObjectId(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updateMediaPlayerIconURL(u16 connHandle, u8 *iconUrl, u8 iconUrlLen);
int blc_gmcss_updateTrackChanged(u16 connHandle);
int blc_gmcss_updateTrackTitle(u16 connHandle, u8 *title, u16 len);
int blc_gmcss_updateTrackDuration(u16 connHandle, s32 duration);
int blc_gmcss_updateTrackPosition(u16 connHandle, s32 position);
int blc_gmcss_updatePlaybackSpeed(u16 connHandle, s8 playbackSpeed);
int blc_gmcss_updateSeekingSpeed(u16 connHandle, s8 seekingSpeed);
int blc_gmcss_updateCurrentTrackSegmentsObjectId(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updateCurrentTrackObjectID(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updateNextTrackObjectID(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updateParentGroupObjectID(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updateCurrentGroupObjectID(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updatePlayingOrder(u16 connHandle, u8 playingOrder);
int blc_gmcss_updatePlayingOrdersSupported(u16 connHandle, u16 playingOrdersSupported);
int blc_gmcss_updateMediaState(u16 connHandle, u8 mediaState);
int blc_gmcss_updateMediaCtrlPoint(u16 connHandle, blc_mcs_mediaCtrlPointOpcode_enum op, u8 resultCode);
int blc_gmcss_updateMediaCtrlPointOpSupp(u16 connHandle, u32 mediaCtrlPointOpSupp);
int blc_gmcss_updateSearchCtrlPoint(u16 connHandle, u8 resultCode);
int blc_gmcss_updateSearchResObjectId(u16 connHandle, const blc_object_id_t *id);
int blc_gmcss_updateContentCtrlID(u16 connHandle, u8 ccid);

/******************************* (G)MCS Server End **********************************************************************/
