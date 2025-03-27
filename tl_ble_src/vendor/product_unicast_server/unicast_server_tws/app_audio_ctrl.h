/********************************************************************************************************
 * @file    app_audio_ctrl.h
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
#ifndef VENDOR_AUDIO_UNICAST_SERVER_APP_AUDIO_CTRL_H_
#define VENDOR_AUDIO_UNICAST_SERVER_APP_AUDIO_CTRL_H_

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_TWS)

/**
 * @brief       This function serves to initialize the acceptor module.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_acceptor_init(void);

/**
 * @brief       Call event callback in APP layer,used to inform user about 'call state' and 'call information'
 * @param[in]   connHandle - ACL connect handle.
 * @param[in]   evtID      - Call event ID.
 * @param[in]   pData      - Additional data.
 * @param[in]   dataLen    - Additional data length.
 * @return      none
 */
void app_call_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen);

/**
 * @brief       Media event callback in APP layer,used to inform user about 'media state' and 'media information'
 * @param[in]   connHandle - ACL connect handle.
 * @param[in]   evtID      - Media event ID.
 * @param[in]   pData      - Additional data.
 * @param[in]   dataLen    - Additional data length.
 * @return      none
 */
void app_media_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen);

/**
 * @brief       Volume event callback in APP layer,used to inform user about 'Volume level'
 * @param[in]   connHandle - ACL connect handle.
 * @param[in]   evtID      - Volume event ID.
 * @param[in]   pData      - Additional data.
 * @param[in]   dataLen    - Additional data length.
 * @return      none
 */
void app_volume_event_callback(u16 connHandle, int evtID, u8 *pData, u16 dataLen);

/*************************media control*************************/

/**
 * @brief       This function serves to control the remote media,if success the media will convert to playing state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_play(u16 connHandle);

/**
 * @brief       This function serves to control the remote media,if success the media will convert to pause state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_pause(u16 connHandle);

/**
 * @brief       This function serves to control the remote media,if success the media will convert to stop state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_stop(u16 connHandle);

/**
 * @brief       This function serves to control the remote media,if success the media will convert current truck to previous truck.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_previous_track(u16 connHandle);

/**
 * @brief       This function serves to control the remote media,if success the media will convert current truck to next truck.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_media_next_track(u16 connHandle);

/*************************call control*************************/

/**
 * @brief       This function serves to excute the accept operaiton,if success the call will convert to active state.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_call_accept(u16 connHandle,u8 callIndex);

/**
 * @brief       This function serves to terminate the call.
 * @param[in]   connHandle - ACL connect handle.
 * @return      none
 */
void app_audio_call_termiante(u16 connHandle,u8 callIndex);

#endif

#endif /* VENDOR_AUDIO_UNICAST_SERVER_APP_AUDIO_CTRL_H_ */
