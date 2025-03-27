/********************************************************************************************************
 * @file    ras.h
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

#include "ras_server_buf.h"
#include "ras_client_buf.h"
#include "ras_server_data.h"

/**
 * ranging profile log api.
 */
#define BLC_RAS_LOG(fmt, ...)               BLC_PROFILE_DEBUG(PRF_DBG_RAS_EN, "[RAS]"fmt, ##__VA_ARGS__)
#define BLC_RAS_DATA_LOG(fmt, ...)          BLC_PROFILE_DEBUG(DBG_CS_LOG_PRF_MASK_EN, "[RAS_DATA]"fmt, ##__VA_ARGS__)

/******************************* ranging Profile Client Start **********************************************************************/

/**
 * @brief       register ranging profile client controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_cs_registerRasProfileControlClient(const blc_rasc_regParam_t *param);

/**
 * @brief       ranging profile client write ranging in characteristic value with ATT_WRITE_REQ/PREPARE_WRITE_REQ command.
 * @param[in]   connHandle: ACL handle.
 * @param[in]   val: want write value .
 * @param[in]   writeCb: write command send callback function.
 * @return      ble_sts_t.
 */
int blc_rasc_writeControlPoint(u16 connHandle, u8* val, u16 valLen, prf_write_cb_t writeCb);

/******************************* ranging Profile Client end **********************************************************************/

/******************************* ranging Profile server Start **********************************************************************/

/**
 * @brief the enum of RAS client event.
 */
typedef enum {
    CS_EVT_RACC_START = CS_EVT_TYPE_RASC,
    CS_EVT_PROCEDURE_DATA,
    CS_EVT_RASC_START = CS_EVT_TYPE_RASS,
    CS_EVT_LOCAL_RANGING_DATA,
} cs_rasc_evt_enum;

/**
 * @brief the data structure of local Ranging Data.
 */
typedef struct {
    u16 connHandle;
    u8* dataPtr;
    u16 dataLen;
} blc_rasc_localRangingDataEvt_t;

/**
 * @brief       register ranging profile server controller.
 * @param[in]   param: initial parameter.
 * @return      none.
 */
void blc_cs_registerRasProfileControlServer(const blc_rass_regParam_t *param);

/******************************* ranging Profile server end **********************************************************************/
