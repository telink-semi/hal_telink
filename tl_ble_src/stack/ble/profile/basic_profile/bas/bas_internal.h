/********************************************************************************************************
 * @file    bas_internal.h
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


#define BATTERY_LEVEL_MIN        0
#define BATTERY_LEVEL_MAX        100

#define CHECK_BATTERY_LEVEL(val) ((val) > BATTERY_LEVEL_MIN && (val) < BATTERY_LEVEL_MAX)

/*
 * BAS: ATT handle information: 7byte
 */
struct blt_bas_att_hdl
{
    u16 baseHandle;
    u8  endHdl;
    u8  batteryLevelHdl;      //NTF
    u8  batteryPowerStateHdl; //NTF
} __attribute__((packed));

struct blt_bas_nv_info
{
    struct blt_bas_att_hdl att;
};

#define BLT_BAS_LOG(fmt, ...) BLC_BASIC_PRF_LOG(DBG_PRF_MASK_BAS_LOG, "[BAS]" fmt, ##__VA_ARGS__)

#define BAS_MALLOC(size)      malloc_nonreten((size))
#define BAS_FREE(ptr)         free_nonreten(ptr)

///Client
#define BLT_DEFINE_BAS_DISCOVERY_FOUND_CHAR(characteristic)         BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR(bas, BAS, characteristic)
#define BLT_DEFINE_BAS_DISCOVERY_START_READ_FIX_LEN(characteristic) BLT_DEFINE_PRF_DISCOVERY_START_READ_FIX_LEN(bas, BAS, characteristic)
#define BLT_BAS_RECONNECT_GET_INFO_READ(characteristic)             BLT_DEFINE_PRF_RECONNECT_GET_INFO(bas, CHAR_PROP_READ | CHAR_PROP_NOTIFY, characteristic)
#define BLT_BAS_RECONNECT_CHAR(characteristic)                      BLT_PRF_RECONNECT_READ_CHAR(bas, characteristic)
#define BLT_BAS_DISCOVERY_READ_NOTIFY_CHAR(uuid, characteristic)    BLT_PRF_DISCOVERY_READ_NOTIFY_CHAR(bas, uuid, characteristic)

#define BLT_BAS_READ_ATTR_VALUE_FIX_LEN(charName)                   BLT_PRF_READ_ATTR_VALUE_FIX_LEN(bas, BAS, charName##Hdl, charName)
#define BLT_BAS_GET_ATTR_VALUE_FIX_LEN(characteristic)              BLT_PRF_GET_ATTR_VALUE_FIX_LEN(bas, characteristic)

///server
#define BLT_BAS_SERVER_INIT_HANDLE(characteristic)     BLT_PRF_SERVER_INIT_HANDLE(bas, BAS, characteristic)
#define BLT_BAS_SERVER_FIND_CHAR(characteristic, uuid) BLT_PRF_SERVER_FIND_CHAR(bas, characteristic, uuid)
