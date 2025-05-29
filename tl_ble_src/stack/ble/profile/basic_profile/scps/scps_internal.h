/********************************************************************************************************
 * @file    scps_internal.h
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


#define BLT_SCPS_LOG(fmt, ...)                               BLC_BASIC_PRF_LOG(DBG_PRF_MASK_SCPS_LOG, "[ScPS]" fmt, ##__VA_ARGS__)

#define BLT_DEFINE_SCPS_DISCOVERY_FOUND_CHAR(characteristic) BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR(scps, SCPS, characteristic)

#define BLT_SCPS_SERVER_INIT_HANDLE(characteristic)          BLT_PRF_SERVER_INIT_HANDLE(scps, SCPS, characteristic)
#define BLT_SCPS_SERVER_FIND_CHAR(characteristic, uuid)      BLT_PRF_SERVER_FIND_CHAR(scps, characteristic, uuid)

#define LE_SCAN_INTERVAL_MIN                                 0x0004
#define LE_SCAN_INTERVAL_MAX                                 0x4000
#define CHECK_LE_SCAN_INTERVAL(interval)                     ((interval) >= LE_SCAN_INTERVAL_MIN && (interval) <= LE_SCAN_INTERVAL_MAX)


#define LE_SCAN_WINDOW_MIN                                   0x0004
#define LE_SCAN_WINDOW_MAX                                   0x4000
#define CHECK_LE_SCAN_WINDOW(window)                         ((window) >= LE_SCAN_WINDOW_MIN && (window) <= LE_SCAN_WINDOW_MAX)

#define SCPS_MALLOC(size)                                    malloc_nonreten((size))
#define SCPS_FREE(ptr)                                       free_nonreten(ptr)

enum
{
    SERVER_REQUIRES_REFRESH = 0x00,
};

/*
 * ScPS: ATT handle information: 5byte
 */
struct blt_scps_att_hdl
{
    u16 baseHandle;
    u8  endHdl;
    u8  scanIntervalWindowHdl;
    u8  scanRefreshHdl;
} __attribute__((packed));

struct blt_scps_nv_info
{
    struct blt_scps_att_hdl att;
};

#define BLT_SCPS_WRITE_ATTR_VALUE_WITHOUT_RSP(charName) BLT_PRF_WRITE_ATTR_VALUE_WITHOUT_RSP_WITH_LEN(scps, SCPS, charName##Hdl, charName, charName##Len)
