/********************************************************************************************************
 * @file    svc_battery.h
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

//BAS: Battery Level Service

typedef enum{
    DEVICE_IN_CHARGING  =   0xBB,
    DEVICE_NO_CHARGING  =   0xAE,
} blc_bas_battery_power_state_enum;

/**
 * @brief      for user add default BAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addBasGroup(void);

/**
 * @brief      for user remove default BAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeBasGroup(void);

/**
 * @brief      for use set battery level value.
 * @param[in]  batterylevel - the value that battery level.
 * @return     none.
 */
void blc_svc_basSetBatteryLevel(u8 batteryLevel);

/**
 * @brief      for use set battery power state value.
 * @param[in]  powerState - the value that battery power state.
 * @return     none.
 */
void blc_svc_basSetPowerState(blc_bas_battery_power_state_enum powerState);
