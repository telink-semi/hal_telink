/********************************************************************************************************
 * @file    tmas.h
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

#include "tmas_client_buf.h"
#include "tmas_server_buf.h"

/******************************* TMAS Common Start **********************************************************************/

/******************************* TMAS Common End **********************************************************************/


/******************************* TMAS Client Start **********************************************************************/

//TMAS Client Event ID
typedef enum
{
    AUDIO_EVT_TMASC_START = AUDIO_EVT_TYPE_TMASC,
    //NONE:
} audio_tmasc_evt_enum;

/**
 * @brief       This function serves to register TMAS Client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerTMASControlClient(const blc_tmasc_regParam_t *param);


//TMAS Client Read Characteristic Value Operation API
int blc_tmasc_readTmapRole(u16 connHandle, prf_read_cb_t readCb);

//TMAS Client Get Characteristic Value Operation API
int blc_tmasc_getTmapRole(u16 connHandle, u16 *tmapRole);

/******************************* TMAS Client End **********************************************************************/


/******************************* TMAS Server Start **********************************************************************/

//TMAS Server Event ID
typedef enum
{
    AUDIO_EVT_TMASS_START = AUDIO_EVT_TYPE_TMASS,
    //NONE:
} audio_tmass_evt_enum;

/**
 * @brief       This function serves to register TMAS Server function
 * @param[in]   refer to 'blc_tmass_regParam_t'
 * @return      none.
 */
void blc_audio_registerTMASControlServer(const blc_tmass_regParam_t *param);

/******************************* TMAS Server End **********************************************************************/
