/********************************************************************************************************
 * @file    ots_internal.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    10,2023
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
    u8  otsFeatureHdl;
    u8  otsObjectNameHdl;
    u8  otsObjectTypeHdl;
    u8  otsObjectSizeHdl;
    u8  otsObjectFirstCreatedHdl;
    u8  otsObjectLastModifiedHdl;
    u8  otsObjectIdHdl;
    u8  otsObjectPropertiesHdl;
    u8  otsObjectActionControlPointHdl;
    u8  otsObjectActionControlPointCccHdl;
    u8  otsObjectListControlPointHdl;
    u8  otsObjectListControlPointCccHdl;
    u8  otsObjectListFilterHdl[3];
    u8  otsObjectChangedHdl;
    u8  otsObjectChangedCccHdl;
} blt_ots_att_hdl_t;

typedef struct
{
    blt_ots_att_hdl_t att;
    u8                otsObjectNameProperties;
    u8                otsObjectFirstCreatedProperties;
    u8                otsObjectLastModifiedProperties;
    u8                otsObjectPropertiesProperties;
    u8                objectListFilterCnt;
} blt_ots_nv_info_t;

typedef struct
{
    u8 olcpCccVal[sizeof(u16)];
    u8 oacpCccVal[sizeof(u16)];
} blt_otss_nv_info_t;
