/********************************************************************************************************
 * @file    mics.h
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

/******************************* MICS Common Start **********************************************************************/

typedef enum
{
    MICS_MUTE_VALUE_NOT_MUTED = 0x00,
    MICS_MUTE_VALUE_MUTED,
    MICS_MUTE_VALUE_DISABLED,
    MICS_MUTE_VALUE_RFU,
} blc_mics_mute_value_enum;

/******************************* MICS Common End **********************************************************************/

#include "stack/ble/profile/audio/render_cap/aicp/aics.h"
#include "mics_client_buf.h"
#include "mics_server_buf.h"

/******************************* MICS Client Start **********************************************************************/

//MICS Client Event ID
typedef enum
{
    AUDIO_EVT_MICSC_START = AUDIO_EVT_TYPE_MICSC,
    AUDIO_EVT_MICSC_CHANGE_MUTE, //refer to 'blc_micsc_muteChangeEvt_t'
} audio_micsc_evt_enum;

typedef struct
{            //Event ID: AUDIO_EVT_MICSC_CHANGE_MUTE
    u8 mute; //blc_mics_mute_value_enum
} blc_micsc_muteChangeEvt_t;

/**
 * @brief       This function serves to register MICP Client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerMICSControlClient(const blc_micsc_regParam_t *param);


//MICP Client Read Characteristic Value Operation API
int blc_micsc_readMute(u16 connHandle, prf_read_cb_t readCb);

//PACS Client Get Characteristic Value Operation API
int blc_micsc_getMute(u16 connHandle, u8 *mute);

//PACS Client Write Characteristic Value Operation API
int blc_micsc_writeMute(u16 connHandle, blc_mics_mute_value_enum mute, prf_write_cb_t writeCb);

/******************************* MICS Client End **********************************************************************/


/******************************* MICS Server Start **********************************************************************/

//MICS Server Event ID
typedef enum
{
    AUDIO_EVT_MICSS_START = AUDIO_EVT_TYPE_MICSS,
    AUDIO_EVT_MICSS_CHANGE_MUTE, //refer to 'blc_micss_muteChangeEvt_t'
} audio_micss_evt_enum;

typedef struct
{            //Event ID: AUDIO_EVT_MICSS_CHANGE_MUTE
    u8 mute; //blc_mics_mute_value_enum
} blc_micss_muteChangeEvt_t;

extern const blc_micss_regParam_t defaultMicpParam;

/**
 * @brief       This function serves to register MICP Server function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerMICSControlServer(const blc_micss_regParam_t *param);


//PACS Server Update Characteristic Value Operation API
int blc_micss_initMute(blc_mics_mute_value_enum mute);
int blc_micss_updateMute(u16 connHandle, blc_mics_mute_value_enum mute);

/******************************* MICS Server End **********************************************************************/
