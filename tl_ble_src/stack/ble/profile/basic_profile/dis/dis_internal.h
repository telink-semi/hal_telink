/********************************************************************************************************
 * @file    dis_internal.h
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


/*
 * DIS: ATT handle information: 13byte
 */
struct blt_dis_att_hdl{
    u16 baseHandle;
    u8 endHdl;
    u8 manufacturerNameHdl;
    u8 modelNumberHdl;
    u8 serialNumberHdl;
    u8 hardwareRevisionHdl;
    u8 firmwareRevisionHdl;
    u8 softwareRevisionHdl;
    u8 systemIdHdl;
    u8 IEEEDataListHdl;
    u8 PnPIDHdl;
    u8 udiForMedicalDevicesHdl;
}__attribute__((packed));

struct blt_dis_nv_info{
    struct blt_dis_att_hdl att;
};

#define BLT_DIS_LOG(fmt, ...)           BLC_BASIC_PRF_LOG(DBG_PRF_MASK_DIS_LOG, "[BAS]"fmt, ##__VA_ARGS__)

#define DIS_MALLOC(size)                malloc_nonreten((size))
#define DIS_FREE(ptr)                   free_nonreten(ptr)

#define BLT_DEFINE_DIS_DISCOVERY_FOUND_CHAR(characteristic)             BLT_DEFINE_PRF_DISCOVERY_FOUND_CHAR(dis, DIS, characteristic)
#define BLT_DEFINE_DIS_DISCOVERY_START_READ(characteristic)             BLT_DEFINE_PRF_DISCOVERY_START_READ(dis, DIS, characteristic)
#define BLT_DEFINE_DIS_DISCOVERY_START_READ_FIX_LEN(characteristic)     BLT_DEFINE_PRF_DISCOVERY_START_READ_FIX_LEN(dis, DIS, characteristic)

#define BLT_DIS_RECONNECT_GET_INFO_READ(characteristic)                 BLT_DEFINE_PRF_RECONNECT_GET_INFO(dis, CHAR_PROP_READ, characteristic)
#define BLT_DIS_RECONNECT_CHAR(characteristic)                          BLT_PRF_RECONNECT_READ_CHAR(dis, characteristic)
#define BLT_DIS_DISCOVERY_READ_CHAR(uuid, characteristic)               BLT_PRF_DISCOVERY_READ_CHAR(dis, uuid, characteristic)

#define BLT_DIS_READ_ATTR_VALUE(charName)                               BLT_PRF_READ_ATTR_VALUE(dis, DIS, charName##Hdl, charName, charName##Len)
#define BLT_DIS_READ_ATTR_VALUE_FIX_LEN(charName)                       BLT_PRF_READ_ATTR_VALUE_FIX_LEN(dis, DIS, charName##Hdl, charName)

#define BLT_DIS_GET_ATTR_VALUE(characteristic)                          BLT_PRF_GET_ATTR_VALUE(dis, characteristic)
#define BLT_DIS_GET_ATTR_VALUE_FIX_LEN(characteristic)                  BLT_PRF_GET_ATTR_VALUE_FIX_LEN(dis, characteristic)



