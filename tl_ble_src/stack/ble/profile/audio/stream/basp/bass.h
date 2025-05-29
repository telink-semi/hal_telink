/********************************************************************************************************
 * @file    bass.h
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

#include "bass_client_buf.h"
#include "bass_server_buf.h"


/******************************* BASS Common Start **********************************************************************/

/******************************* BASS Common End **********************************************************************/


/******************************* BASS Client Start **********************************************************************/

extern const u8 gAppBasscRecvStateNum;
extern const u8 gAppBasscRecvStateMaxSize;

//BASS Client Event ID
typedef enum
{
    AUDIO_EVT_BASSC_START = AUDIO_EVT_TYPE_BASSC,
    AUDIO_EVT_BASSC_RECV_SYNCINFO_REQ,  //NULL event data, only Event ID
    AUDIO_EVT_BASSC_RECV_SINK_STATE,    //refer to 'blc_bassc_recvSinkStateEvt_t'
    AUDIO_EVT_BASSC_BROADCAST_CODE_REQ, //refer to 'blc_bassc_bcstCodeReq_t'
    AUDIO_EVT_BASSC_BAD_BROADCAST_CODE, //refer to 'blc_bassc_badBroadcastCodeEvt_t'
} audio_bassc_evt_enum;

typedef struct
{ //Event ID: AUDIO_EVT_RECV_SINK_STATE
    u8   sourceID;
    bool paState;
    u8   numSubgroups;
    u32  bisSyncState;
    u8   metadataLen;
    u8  *metadata;
} blc_bassc_recvSinkStateEvt_t;

typedef struct
{ //Event ID: AUDIO_EVT_BASSC_BROADCAST_CODE_REQ
    u8 sourceID;
} blc_bassc_bcstCodeReq_t;

typedef struct
{ //Event ID: AUDIO_EVT_BASSC_BAD_BROADCAST_CODE
    u8   sourceID;
    bool paState;
    u16  broadcastCode[16];
} blc_bassc_badBroadcastCodeEvt_t;

/**
 * @brief       This function serves to register BASS Client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerBASSControlClient(const blc_bassc_regParam_t *param);


//BASS Client Read Characteristic Value Operation API
int blc_bassc_readBcstRecvState(u16 connHandle, int index, prf_read_cb_t readCb);

//BASS Client Get Characteristic Value Operation API
int blc_bassc_getBcstRecvState(u16 connHandle, int index, u8 *state, u16 *len);

//BASS Client Write Characteristic Value Operation API
int blc_bassc_writeBcstScanCtrlPoint(u16 connHandle, u8 *cmd, u16 cmdLen, prf_write_cb_t writeCb);
int blc_bassc_writeBcstScanCtrlPointWithoutRsp(u16 connHandle, u8 *cmd, u16 cmdLen);

//default use write without response
int blc_bassc_writeRemoteScanStopped(u16 connHandle);
int blc_bassc_writeRemoteScanStarted(u16 connHandle);
int blc_bassc_writeAddSource(u16 connHandle, u8 *param, u16 paramLen);
int blc_bassc_writeModifySource(u16 connHandle, u8 *param, u16 paramLen);
int blc_bassc_writeSetBroadcastCode(u16 connHandle, u8 sourceID, u8 bcstCode[16]);
int blc_bassc_writeRemoveSource(u16 connHandle, u8 sourceID);

/******************************* BASS Client End **********************************************************************/


/******************************* BASS Server Start **********************************************************************/

extern const u8 gAppBasssRecvStateCntl;

//BASS Server Event ID
typedef enum
{
    AUDIO_EVT_BASSS_START = AUDIO_EVT_TYPE_BASSS,
    AUDIO_EVT_BASSS_REMOTE_SCAN_STOPPED,     //NULL event data, only Event ID
    AUDIO_EVT_BASSS_REMOTE_SCAN_STARTED,     //NULL event data, only Event ID
    AUDIO_EVT_BASSS_REMOVE_SOURCE,           //internal used
    AUDIO_EVT_BASSS_DONOT_SYNC_TO_PA,        //internal used
    AUDIO_EVT_BASSS_NO_PAST,                 //wait PAST timeout,need clean PA sync state.
                                             //internal used
    AUDIO_EVT_BASSS_SYNC_TO_PA,              //internal used
    AUDIO_EVT_BASSS_SYNC_TO_BIS,             //internal used
    AUDIO_EVT_BASSS_RECV_SET_BROADCAST_CODE, //internal used
} audio_basss_evt_enum;

/**
 * @brief       This function serves to register BASS Server function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerBASSControServer(const blc_basss_regParam_t *param);


//BASS Server Update Characteristic Value Operation API
ble_sts_t blc_basss_updateBcstRecvState(u16 connHandle, u8 *state, u16 len);

/******************************* BASS Server End **********************************************************************/
