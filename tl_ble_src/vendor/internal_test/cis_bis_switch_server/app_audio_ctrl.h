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

#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_SERVER)

/*************************** CIS audio function, Begin *************************************/

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

/*************************** CIS audio function, end *************************************/


/*************************** BIS audio function, Begin *************************************/



/*************************** BIS audio function, end *************************************/

///////////////////////audio control LC3 packet and push to DMA///////////////////////

//broadcast sink extend advertising parameter
#define BROADCAST_SINK_LOCATION         (BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR)

#define APP_AUDIO_FRAME_BYTES           960     //max 48KHz

#define APP_SINK_RECV_SPEAK_FRAME_COUNT         8

#define APP_AUDIO_IIS_OUT                           1
#define APP_AUDIO_LINE_OUT                          2
#define APP_AUDIO_OUTPUT_TYPE                       APP_AUDIO_LINE_OUT


/**
 *  @brief  app sink sync BIS information parameter
 */
typedef struct __attribute__((packed)) {
    u16 bisHandle;
    u16 codecFrameSize;
    u8 lc3Count;
    u8 lc3Index[2];
    u8 allocation;      //only support FL/FR
    void* popSdu;
} appSinkSyncBisInfo_t;

/**
 *  @brief  app sink information parameter
 */
typedef struct __attribute__((packed)) {
    u8 bigSyncState;
    u8 bisSyncNum;
    u16 frameDataLen;
    u32 presentationDelay;      //unit us
    u32 syncLocation;
    appSinkSyncBisInfo_t bisInfo[2];    //only supported sync 2 bis

    u8 spkState;
} appSinkInfo_t;

/**
 *  @brief  app sink sync receive speak parameter
 */
typedef struct __attribute__((packed)) {
    u32 seqNum;
    u32 renderPoint;
    u16 rxBuff[APP_AUDIO_FRAME_BYTES];
} appSinkRecvSpeak_t;

extern appSinkInfo_t appSinkInfo;

/**
 * @brief       set BIG sync state into control model.
 * @param[in]   syncState: refer to blc_audio_bigSyncState_enum.
 * @param[in]   numBis: the number of sync BIS.
 * @param[in]   bisHandles: sync BIS handles.
 * @return      none
 */
void app_codec_setBigSyncState(u8 syncState, u8 numBis, u16 bisHandles[0]);

/**
 * @brief       set BIG information into control model.
 * @param[in]   codecEvt: broadcast sink initial codec event.
 * @return      none
 */
void app_codec_setBigInformation(blc_bapbs_bisSinkInitCodecEvt_t* codecEvt);

/**
 * @brief       broadcast sink audio receive BIS SDU Handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_receiveHandler(void);

#endif

#endif /* VENDOR_AUDIO_UNICAST_SERVER_APP_AUDIO_CTRL_H_ */
