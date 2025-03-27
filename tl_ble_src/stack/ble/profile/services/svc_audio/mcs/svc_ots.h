/********************************************************************************************************
 * @file    svc_ots.h
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

//OTS: Object Transfer Service

struct svc_ots_feature{
    u32 OACP_feature;
    u32 OLCP_feature;
};

struct svc_ots_object_size{
    u32 currentSize;
    u32 allocatedSize;
};

struct svc_ots_universal_time{
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
}__attribute__((packed));

/**
 * @brief      for user add default OTS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addOtsGroup(void);

/**
 * @brief      for user remove default OTS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeOtsGroup(void);

/**
 * @brief      for user register read or write attribute value callback function in OTS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_otsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback);
