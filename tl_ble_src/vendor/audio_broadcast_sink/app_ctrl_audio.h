/********************************************************************************************************
 * @file    app_ctrl_audio.h
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
#pragma once

///////////////////////audio control LC3 packet and push to DMA///////////////////////

//broadcast sink extend advertising parameter
#define BROADCAST_SINK_LOCATION         (BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR)

#define APP_AUDIO_FRAME_BYTES           960 //max 48KHz

#define APP_SINK_RECV_SPEAK_FRAME_COUNT 8

/**
 *  @brief  app sink sync BIS information parameter
 */
typedef struct
{
    u16   bisHandle;
    u16   codecFrameSize;
    u8    lc3Count;
    u8    lc3Index[2];
    u8    allocation; //only support FL/FR
    void *popSdu;
} appSinkSyncBisInfo_t;

/**
 *  @brief  app sink information parameter
 */
typedef struct
{
    u8                   bigSyncState;
    u8                   bisSyncNum;
    u16                  frameDataLen;
    u32                  presentationDelay; //unit us
    u32                  syncLocation;
    appSinkSyncBisInfo_t bisInfo[2];        //only supported sync 2 bis

    u8 spkState;
} appSinkInfo_t;

/**
 *  @brief  app sink sync receive speak parameter
 */
typedef struct
{
    u32 seqNum;
    u32 renderPoint;
    u16 rxBuff[APP_AUDIO_FRAME_BYTES];
} appSinkRecvSpeak_t;

/**
 * @brief       codec initial function
 * @param[in]   none.
 * return       none.
 */
void app_codec_init(void);

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
void app_codec_setBigInformation(blc_bapbs_bisSinkInitCodecEvt_t *codecEvt);

/**
 * @brief       broadcast sink audio receive BIS SDU Handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_receiveHandler(void);
