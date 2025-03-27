/********************************************************************************************************
 * @file    app_audio_media.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_BASE)
static void app_audio_media_control_result(blc_mcsc_mediaCtrlResultEvt_t *pEvt);

/**
 * @brief       Media event callback in APP layer,used to inform user about 'media state' and 'media information'
 * @param[in]   connHandle - ACL connect handle.
 * @param[in]   evtID      - Media event ID.
 * @param[in]   pData      - Additional data.
 * @param[in]   dataLen    - Additional data length.
 * @return      none
 */
void app_media_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen)
{
    switch(evtID)
    {

        case AUDIO_EVT_MCSC_MEDIA_PLAYER_NAME:
        {
            blc_mcsc_mediaPlayerNameEvt_t *pEvt = (blc_mcsc_mediaPlayerNameEvt_t*)pData;

            tlkapi_printf(APP_LOG_EN,"media name[%.*s]", pEvt->mediaNameLen,pEvt->mediaName);
        }
        break;

        case AUDIO_EVT_MCSC_MEDIA_TRACK_CHANGED:
        {
//          blc_mcsc_mediaTrackChangedEvt_t *pEvt = (blc_mcsc_mediaTrackChangedEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: track changed");
        }
        break;

        case AUDIO_EVT_MCSC_MEDIA_TRACK_TITLE:
        {
            blc_mcsc_mediaTrackTitleEvt_t *pEvt = (blc_mcsc_mediaTrackTitleEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media track title[%.*s]", pEvt->trackTitleLen,pEvt->trackTitle);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_TRACK_DURATION:
        {
            blc_mcsc_mediaTrackDurationEvt_t *pEvt = (blc_mcsc_mediaTrackDurationEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: track duration - %d",pEvt->trackDuration);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_TRACK_POSITION:
        {
            blc_mcsc_mediaTrackPositionEvt_t *pEvt = (blc_mcsc_mediaTrackPositionEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: track position - %d",pEvt->trackPosition);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_PLAYBACK_SPEED:
        {
            blc_mcsc_mediaPlaybackSpeedEvt_t *pEvt = (blc_mcsc_mediaPlaybackSpeedEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: playback speed - %d",pEvt->playbackSpeed);
        }
        break;

        case AUDIO_EVT_MCSC_MEDIA_SEEKING_SPEED:
        {
            blc_mcsc_mediaSeekingSpeedEvt_t *pEvt = (blc_mcsc_mediaSeekingSpeedEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: seeking speed - %d",pEvt->seekingSpeed);
        }
        break;

        case AUDIO_EVT_MCSC_MEDIA_CURRENT_TRACK_OBJECT_ID:
        {
            blc_mcsc_mediaCurrentTrackObjectIdEvt_t *pEvt = (blc_mcsc_mediaCurrentTrackObjectIdEvt_t*)pData;
            BLT_APP_STR_LOG("media: current track object ID - %s",pEvt->object.objectId,6);
        }
        break;

        case AUDIO_EVT_MCSC_MEDIA_NEXT_TRACK_OBJECT_ID:
        {
            blc_mcsc_mediaNextTrackObjectIdEvt_t *pEvt = (blc_mcsc_mediaNextTrackObjectIdEvt_t*)pData;
            BLT_APP_STR_LOG("media: next track object ID - %s", pEvt->object.objectId,6);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_PARENT_GROUP_OBJECT_ID:
        {
            blc_mcsc_mediaParentGroupObjectIdEvt_t *pEvt = (blc_mcsc_mediaParentGroupObjectIdEvt_t*)pData;
            BLT_APP_STR_LOG("media: parent group object ID - %s", pEvt->object.objectId,6);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_CURRENT_GROUP_OBJECT_ID:
        {
            blc_mcsc_mediaCurrentGroupObjectIdEvt_t *pEvt = (blc_mcsc_mediaCurrentGroupObjectIdEvt_t*)pData;
            BLT_APP_STR_LOG("media: current group object ID - %s", pEvt->object.objectId,6);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_PLAYING_ORDER:
        {
            blc_mcsc_mediaPlayingOrderEvt_t *pEvt = (blc_mcsc_mediaPlayingOrderEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: playing order - %x",pEvt->order);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_STATE:
        {
            blc_mcsc_mediaStateEvt_t *pEvt = (blc_mcsc_mediaStateEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: media state - %d",pEvt->state);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_CTRL_RESULT:
        {
            app_audio_media_control_result((blc_mcsc_mediaCtrlResultEvt_t*)pData);
        }
        break;
        case AUDIO_EVT_MCSC_MEDIA_CTRL_OPCODE_SUPPORT:
        {
            blc_mcsc_mediaCtrlOpcodeSupportEvt_t *pEvt = (blc_mcsc_mediaCtrlOpcodeSupportEvt_t*)pData;
            tlkapi_printf(APP_LOG_EN,"media: control opcode support - %x",pEvt->supportOpcode);
        }
        break;

        case AUDIO_EVT_MCSC_SEARCH_CTRL_RESULT:
        {
            blc_mcsc_searchCtrlResultEvt_t *pEvt = (blc_mcsc_searchCtrlResultEvt_t*)pData;
            if(pEvt->result != BLC_MCS_SEARCH_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"object search fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"object search success");
            }
        }
        break;

        case AUDIO_EVT_MCSC_SEARCH_RESULT_OBJECT_ID:
        {
            blc_mcsc_searchResultObjectIdEvt_t *pEvt = (blc_mcsc_searchResultObjectIdEvt_t*)pData;
            BLT_APP_STR_LOG("media: serach result object ID - %s", pEvt->object.objectId,6);
        }
        break;

        default:
        break;
    }
}

/**
 * @brief       This function serves to control the remote media,if success the media will convert to playing state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_play(u16 connHandle)
{
    blc_mcs_mediaState_enum mediaState = 0xff;
    if(blc_gmcsc_getMediaState(connHandle,&mediaState)!=AUDIO_ESUCC)
    {
        tlkapi_printf(APP_LOG_EN,"get media state error");
        return;
    }
    if(mediaState == GMCS_MEDIA_STATE_INACTIVE)
    {
        tlkapi_printf(APP_LOG_EN,"media player inactive");
    }
    else if(mediaState == GMCS_MEDIA_STATE_PLAYING)
    {
        tlkapi_printf(APP_LOG_EN,"media player already playing");
    }
    else if(mediaState == GMCS_MEDIA_STATE_PAUSED)
    {
        if(blc_gmcsc_writeStartPlayingCurrentTrack(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert paused to play");
    }
    else if(mediaState == GMCS_MEDIA_STATE_SEEKING)
    {
        if(blc_gmcsc_writeStartPlayingCurrentTrack(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert seeking to play");
    }
}

/**
 * @brief       This function serves to control the remote media,if success the media will convert to pause state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_pause(u16 connHandle)
{
    blc_mcs_mediaState_enum mediaState = 0xff;
    if(blc_gmcsc_getMediaState(connHandle,&mediaState)!=AUDIO_ESUCC)
    {
        tlkapi_printf(APP_LOG_EN,"get media state error");
        return;
    }
    if(mediaState == GMCS_MEDIA_STATE_INACTIVE)
    {
        tlkapi_printf(APP_LOG_EN,"media player inactive");
    }
    else if(mediaState == GMCS_MEDIA_STATE_PLAYING)
    {
        if(blc_gmcsc_writePauseCurrentTrack(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert playing to pause");
    }
    else if(mediaState == GMCS_MEDIA_STATE_PAUSED)
    {
        tlkapi_printf(APP_LOG_EN,"media already paused");
    }
    else if(mediaState == GMCS_MEDIA_STATE_SEEKING)
    {
        if(blc_gmcsc_writePauseCurrentTrack(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert seeking to paused");
    }
}

/**
 * @brief       This function serves to control the remote media,if success the media will convert to stop state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_stop(u16 connHandle)
{
    blc_mcs_mediaState_enum mediaState = 0xff;
    if(blc_gmcsc_getMediaState(connHandle,&mediaState)!=AUDIO_ESUCC)
    {
        tlkapi_printf(APP_LOG_EN,"get media state error");
        return;
    }
    if(mediaState == GMCS_MEDIA_STATE_INACTIVE)
    {
        tlkapi_printf(APP_LOG_EN,"media player inactive");
    }
    else
    {
        if(blc_gmcsc_writeStopActivity(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert to paused");
    }
}

/**
 * @brief       This function serves to control the remote media,if success the media will convert current truck to previous truck.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_previous_track(u16 connHandle)
{
    blc_mcs_mediaState_enum mediaState = 0xff;
    if(blc_gmcsc_getMediaState(connHandle,&mediaState)!=AUDIO_ESUCC)
    {
        tlkapi_printf(APP_LOG_EN,"get media state error");
        return;
    }
    if(mediaState == GMCS_MEDIA_STATE_INACTIVE)
    {
        tlkapi_printf(APP_LOG_EN,"media player inactive");
    }
    else
    {
        if(blc_gmcsc_writePreviousTrack(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert to paused");
    }
}

/**
 * @brief       This function serves to control the remote media,if success the media will convert current truck to next truck.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_next_track(u16 connHandle)
{
    blc_mcs_mediaState_enum mediaState = 0xff;
    if(blc_gmcsc_getMediaState(connHandle,&mediaState)!=AUDIO_ESUCC)
    {
        tlkapi_printf(APP_LOG_EN,"get media state error");
        return;
    }
    if(mediaState == GMCS_MEDIA_STATE_INACTIVE)
    {
        tlkapi_printf(APP_LOG_EN,"media player inactive");
    }
    else
    {
        if(blc_gmcsc_writeNextTrack(connHandle)!=AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"media write ctrl error");
        }
        tlkapi_printf(APP_LOG_EN,"media convert to paused");
    }
}

static void app_audio_media_control_result(blc_mcsc_mediaCtrlResultEvt_t *pEvt)
{
    switch(pEvt->op)
    {
        case BLC_MCS_OPCODE_PLAY:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PLAY fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PLAY success");
            }
        }
        break;

        case BLC_MCS_OPCODE_PAUSE:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PAUSE fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PAUSE success");
            }
        }
        break;

        case BLC_MCS_OPCODE_FAST_REWIND:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FAST_REWIND fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FAST_REWIND success");
            }
        }
        break;

        case BLC_MCS_OPCODE_FAST_FORWARD:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FAST_FORWARD fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FAST_FORWARD success");
            }
        }
        break;

        case BLC_MCS_OPCODE_STOP:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_STOP fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_STOP success");
            }
        }
        break;

        case BLC_MCS_OPCODE_MOVE_RELATIVE:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_MOVE_RELATIVE fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_MOVE_RELATIVE success");
            }
        }
        break;

        case BLC_MCS_OPCODE_PREVIOUS_SEGMENT:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PREVIOUS_SEGMENT fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PREVIOUS_SEGMENT success");
            }
        }
        break;

        case BLC_MCS_OPCODE_NEXT_SEGMENT:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_NEXT_SEGMENT fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_NEXT_SEGMENT success");
            }
        }
        break;

        case BLC_MCS_OPCODE_FIRST_SEGMENT:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FIRST_SEGMENT fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FIRST_SEGMENT success");
            }
        }
        break;

        case BLC_MCS_OPCODE_LAST_SEGMENT:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_LAST_SEGMENT fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_LAST_SEGMENT success");
            }
        }
        break;

        case BLC_MCS_OPCODE_GOTO_SEGMENT:
         {
             if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
             {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_GOTO_SEGMENT fail,reason - %d!",pEvt->result);
             }
             else
             {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_GOTO_SEGMENT success");
             }
         }
         break;

        case BLC_MCS_OPCODE_PREVIOUS_TRACK:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PREVIOUS_TRACK fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PREVIOUS_TRACK operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_NEXT_TRACK:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_NEXT_TRACK fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_NEXT_TRACK operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_FIRST_TRACK:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FIRST_TRACK fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FIRST_TRACK operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_LAST_TRACK:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_LAST_TRACK fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_LAST_TRACK operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_GOTO_TRACK:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_GOTO_TRACK fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_GOTO_TRACK operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_PREVIOUS_GROUP:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PREVIOUS_GROUP fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_PREVIOUS_GROUP operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_NEXT_GROUP:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_NEXT_GROUP fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_NEXT_GROUP operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_FIRST_GROUP:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FIRST_GROUP fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_FIRST_GROUP operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_LAST_GROUP:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_LAST_GROUP fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_LAST_GROUP operation success");
            }
        }
        break;

        case BLC_MCS_OPCODE_GOTO_GROUP:
        {
            if(pEvt->result!=BLC_MCS_MEDIA_CTRL_RESULT_SUCCESS)
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_GOTO_GROUP fail,reason - %d!",pEvt->result);
            }
            else
            {
                tlkapi_printf(APP_LOG_EN,"BLC_MCS_OPCODE_GOTO_GROUP operation success");
            }
        }
        break;

        default:
        break;
        //...search for 'blc_mcs_mediaCtrlPointOpcode_enum' for other command
    }
}


#endif
