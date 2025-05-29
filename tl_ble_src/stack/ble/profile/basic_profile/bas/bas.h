/********************************************************************************************************
 * @file    bas.h
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

// BAS: Battery Service
// BASC: Battery Service Client.
// BASS: Battery Service

/******************************* BAS Common Start **********************************************************************/

/******************************* BAS Common End **********************************************************************/

/******************************* BAS Client Start **********************************************************************/
//BAS Client Event ID
enum
{
    BASIC_EVT_BASC_START = BASIC_EVT_TYPE_BAS_CLIENT,
    BASC_EVT_BATTERY_LEVEL_CHANGE,       //refer to 'struct blc_basc_batteryLevelChangeEvt'
    BASC_EVT_BATTERY_POWER_STATE_CHANGE, //refer to 'struct blc_basc_batteryPowerStateChangeEvt'
};

struct blc_basc_regParam
{
};

struct blc_basc_batteryLevelChangeEvt
{ //Event ID: BASC_EVT_BATTERY_LEVEL_CHANGE
    u8 batteryLevel;
};

struct blc_basc_batteryPowerStateChangeEvt
{ //Event ID: BASC_EVT_BATTERY_POWER_STATE_CHANGE
    u8 batteryPowerState;
};

/**
 * @brief       for user to register battery service control client module.
 * @param[in]   param - currently not used, fixed NULL.
 * @return      none.
 */
void blc_basic_registerBASControlClient(const struct blc_basc_regParam *param);

//BAS Client Read Characteristic Value Operation API
int blc_basc_readBatteryLevel(u16 connHandle, prf_read_cb_t readCb);
int blc_basc_readBatteryPowerState(u16 connHandle, prf_read_cb_t readCb);

//BAS Client Get Characteristic Value Operation API
int blc_basc_getBatteryLevel(u16 connHandle, u8 *batteryLevel);
int blc_basc_getBatteryPowerState(u16 connHandle, u8 *batteryPowerState);

/******************************* BAS Client End **********************************************************************/


/******************************* BAS Server Start **********************************************************************/
//BAS Server Event ID
enum
{
    BASIC_EVT_BASS_START = BASIC_EVT_TYPE_BAS_SERVER,
};

struct blc_bass_regParam
{
    u8 batteryLevel; //range in 0-100.
    u8 powerState;   //value is blc_bas_battery_power_state_enum.
};

/**
 * @brief       for user to register battery service control server module.
 * @param[in]   param - server initial parameter.
 * @return      none.
 */
void blc_basic_registerBASControlServer(const struct blc_bass_regParam *param);

//BAS Server Get Characteristic Value Operation API
u8 blc_bass_getBatteryLevel(void);

//BAS Server Update Characteristic Value Operation API
int blc_bass_updateBatteryLevel(u16 connHandle, u8 batteryLevel);
int blc_bass_updateBatteryPowerState(u16 connHandle, u8 batteryPowerState);

/******************************* BAS Server End **********************************************************************/
