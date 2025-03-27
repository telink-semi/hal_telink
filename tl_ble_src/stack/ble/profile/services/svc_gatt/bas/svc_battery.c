/********************************************************************************************************
 * @file    svc_battery.c
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
#include "stack/ble/ble.h"

#ifndef BAS_BATTERY_POWER_STATE
#define BAS_BATTERY_POWER_STATE             1
#endif

#define BAS_START_HDL                       SERVICE_BATTERY_HDL

_attribute_ble_data_retention_
static u8 basBatteryLevelValue = 0x64;
static const u16 basBatteryLevelValueLen = sizeof(basBatteryLevelValue);

#if BAS_BATTERY_POWER_STATE
_attribute_ble_data_retention_
static u8 basBatteryPowerStateValue = DEVICE_IN_CHARGING;
static const u16 basBatteryPowerStateValueLen = sizeof(basBatteryPowerStateValue);
#endif

/*
 * @brief the structure for default BAS service List.
 */
static const atts_attribute_t basList[] =
{
    ATTS_PRIMARY_SERVICE(serviceBatteryUuid),

    //Battery level
    ATTS_CHAR_UUID_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicBatteryLevelUuid, basBatteryLevelValue),
    ATTS_COMMON_CCC_DEFINE,

#if BAS_BATTERY_POWER_STATE
    //Battery Power State
    ATTS_CHAR_UUID_READ_ENTITY_NOCB(charPropReadNotfiy, characteristicBatteryPowerStateUuid, basBatteryPowerStateValue),
    ATTS_COMMON_CCC_DEFINE,
#endif
};

/*
 * @brief the structure for default BAS service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcBasGroup =
{
    NULL,
    basList,
    NULL,
    NULL,
    BAS_START_HDL,
    0,
};

const u16 basIncludeVal[3] = {SERVICE_BATTERY_HDL, SERVICE_BATTERY_HDL+ARRAY_SIZE(basList)-1, SERVICE_UUID_BATTERY};

/**
 * @brief      for user add default BAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addBasGroup(void)
{
    svcBasGroup.endHandle = svcBasGroup.startHandle+ARRAY_SIZE(basList)-1;
    blc_gatts_addAttributeServiceGroup(&svcBasGroup);
}

/**
 * @brief      for user remove default BAS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeBasGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(BAS_START_HDL);
}

/**
 * @brief      for use set battery level value.
 * @param[in]  batterylevel - the value that battery level.
 * @return     none.
 */
void blc_svc_basSetBatteryLevel(u8 batteryLevel)
{
    basBatteryLevelValue = batteryLevel;
}


/**
 * @brief      for use set battery power state value.
 * @param[in]  powerState - the value that battery power state.
 * @return     none.
 */
void blc_svc_basSetPowerState(blc_bas_battery_power_state_enum powerState)
{
#if BAS_BATTERY_POWER_STATE
    basBatteryPowerStateValue = powerState;
#else
    (void)powerState;
#endif
}

