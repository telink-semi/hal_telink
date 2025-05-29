/********************************************************************************************************
 * @file    otas.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    03,2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "otas_client_buf.h"

/******************************* OTAS Common Start **********************************************************************/

/******************************* OTAS Common End **********************************************************************/

/******************************* OTAS Client Start **********************************************************************/
//OTAS Client Event ID
enum{
    OTASC_EVT_BASC_START = OTA_EVT_TYPE_OTASC,
    OTASC_EVT_NOTIF,          //refer to 'struct blc_otasc_notifEvt'
};

struct blc_otasc_regParam{

};

struct blc_otasc_notifEvt{      //Event ID: OTASC_EVT_NOTIF
    u8 data[0];
};

#ifndef BLC_OTASC_OTA_DATA_MAX_SIZE
#define BLC_OTASC_OTA_DATA_MAX_SIZE    0x20
#endif

void blc_otas_registerOTASControlClient(const struct blc_otasc_regParam *param);
ble_sts_t blc_otasc_readOtaData(u16 connHandle, prf_read_cb_t readCb);
ble_sts_t blc_otasc_writeOtaData(u16 connHandle, u16 length, u8 *data);
ble_sts_t blc_otasc_getOtaData(u16 connHandle, u16 *length, u8* buffer);

/******************************* OTAS Client End **********************************************************************/

/******************************* OTAS Server Start **********************************************************************/

/******************************* OTAS Server End **********************************************************************/
