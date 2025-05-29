/********************************************************************************************************
 * @file    csis.h
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

#include "csis_client_buf.h"
#include "csis_server_buf.h"

/******************************* CSIS Common Start **********************************************************************/

typedef enum
{
    BLC_CSIS_UNLOCKED = 0x01,
    BLC_CSIS_LOCKED   = 0x02,
} blc_csis_memberLockState_enum;

typedef enum
{
    BLT_CSIS_ENCRYPTED_SIRK = 0x00,
    BLT_CSIS_PLAIN_TEXT_SIRK,
} blc_csis_sirk_type_enum;

/**
 * @brief       This function is used to generate RSI based on SIRK.
 * @param[in]   SIRK    - is the 128-bit key, little--endian.
 * @param[out]  outRSI  - is the 48-bit key, little--endian..
 * @return      none.
 */
void blc_csis_cryptoGenerateRSI(const u8 SIRK[16], u8 outRSI[6]);

/**
 * @brief       This function is used to resolving RSI based on SIRK.
 * @param[in]   SIRK    - is the 128-bit key, little--endian.
 * @param[in]   outRSI  - is the 48-bit key, little--endian..
 * @return      0x01: resolved; 0x00: not resolved.
 */
bool blc_csis_resolveRSI(const u8 sirk[16], u8 rsi[6]);

/******************************* CSIS Common End **********************************************************************/


/******************************* CSIS Client Start **********************************************************************/

//CSIS Client Event ID
typedef enum
{
    AUDIO_EVT_CSISC_START = AUDIO_EVT_TYPE_CSISC,
    //NONE:
} audio_csisc_evt_enum;

/**
 * @brief       This function serves to register CSIS Client function
 * @param[in]   currently not used, input NULL
 * @return      none.
 */
void blc_audio_registerCSISControlClient(const blc_csisc_regParam_t *param);


//CSIS Client Read Characteristic Value Operation API
int blc_csisc_readSIRK(u16 connHandle, prf_read_cb_t readCb);
int blc_csisc_readSSetSize(u16 connHandle, prf_read_cb_t readCb);
int blc_csisc_readLock(u16 connHandle, prf_read_cb_t readCb);
int blc_csisc_readRank(u16 connHandle, prf_read_cb_t readCb);

//CSIS Client Set Characteristic Value Operation API
int blc_csisc_getSetIdentityResolvingKey(u16 connHandle, u8 outSIRK[16]);
int blc_csisc_getCoordinatedSetSize(u16 connHandle, u8 outSetSize[1]);
int blc_csisc_getSetMemberLock(u16 connHandle, u8 outLock[1]);
int blc_csisc_getSetMemberRank(u16 connHandle, u8 outRank[1]);

//CSIS Client Write Characteristic Value Operation API
int blc_csisc_writeLock(u16 connHandle, u8 lock, prf_write_cb_t writeCb);

/******************************* CSIS Client End **********************************************************************/


/******************************* CSIS Server Start **********************************************************************/

extern const blc_csiss_regParam_t defaultCsipSetMemberParam;

//CSIS Server Event ID
typedef enum
{
    AUDIO_EVT_CSISS_START = AUDIO_EVT_TYPE_CSISS,
    //NONE:
} audio_csiss_evt_enum;

/**
 * @brief       This function serves to register CSIS Server function
 * @param[in]   refer to 'blc_csiss_regParam_t'
 * @return      none.
 */
void blc_audio_registerCSISControlServer(const blc_csiss_regParam_t *param);

/**
 * @brief       This function CSIS server to get resolvable set identifier.
 * @param[out]  outRSI: RSI value.
 * @return      none.
 */
void blc_csiss_getResolvableSetIdentifier(u8 outRSI[6]);

/**
 * @brief       This function serves to get the SIRK
 * @param[in]   SIRK - csip SIRK
 * @return      none.
 */
int blc_csiss_initSIRK(u8 type, const u8 SIRK[16]);

/**
 * @brief       This function serves to get the coordinate set size and rank
 * @param[in]   coordinatedSetSize - coordinate size
 * @param[in]   memberRank         - coordinate rank
 * @return      none.
 */
int blc_csiss_initSizeRank(u8 coordinatedSetSize, u8 memberRank);

/**
 * @brief       This function serves to init the lock tomer.
 * @param[in]   timer - the timer need to init
 * @return      none.
 */
int blc_csiss_initLockTimer(u8 timer);


//CSIS Client Set Characteristic Value Operation API
int blc_csiss_updateSIRK(u16 connHandle, u8 outSIRK[16]);
int blc_csiss_updateSetSize(u16 connHandle, u8 outSetSize[1]);
int blc_csiss_updateLock(u16 connHandle, u8 outLock[1]);

/******************************* CSIS Server End **********************************************************************/
