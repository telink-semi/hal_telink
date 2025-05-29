/********************************************************************************************************
 * @file    svc_vocs.h
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

//VOCS: Volume Offset Control Service

typedef struct
{
    s16 volumeOffset;
    u8  changeCount;
} svc_vocsVolOffState_t;

/**
 * @brief      for user add default VOCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addVocsGroup(void);

/**
 * @brief      for user remove default VOCS service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeVocsGroup(void);

/**
 * @brief      for user register read or write attribute value callback function in VOCS service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_vocsCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback);
