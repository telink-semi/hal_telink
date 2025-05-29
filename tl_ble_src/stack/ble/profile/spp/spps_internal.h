/********************************************************************************************************
 * @file    spps_internal.h
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

/*
 * BAS: ATT handle information: 7byte
 */
typedef struct
{
    u16 baseHandle;
    u8  endHdl;
    u8  sppDataHdl;    //NTF
    u8  sppDataCccHdl; //NTF
} blt_spps_att_hdl_t;

typedef struct
{
    blt_spps_att_hdl_t att;
}blt_spps_nv_info_t;


#define BLT_SPPS_LOG(fmt, ...) BLC_PROFILE_DEBUG(1, "[SPPS]" fmt, ##__VA_ARGS__)
