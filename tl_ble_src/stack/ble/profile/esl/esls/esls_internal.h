/********************************************************************************************************
 * @file    esls_internal.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    07,2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

typedef struct
{
    u16 baseHandle;
    u8  endHdl;
    u8  eslAddressHdl;
    u8  apSyncKetMaterialHdl;
    u8  eslResponseKeyMaterialHdl;
    u8  eslCurrentAbsoluteTimeHdl;
    u8  eslDisplayInformationHdl;
    u8  eslImageInformationHdl;
    u8  eslSensorInformationHdl;
    u8  eslLedInformationHdl;
    u8  eslControlPointHdl;
    u8  eslControlPointCccHdl;
} blt_esls_att_hdl_t;

typedef struct
{
    blt_esls_att_hdl_t att;
} blt_esls_nv_info_t;
