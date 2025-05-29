/********************************************************************************************************
 * @file    svc_ras.h
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

/**
 * @brief the data structure of RAS Feature.
 */
typedef struct __attribute__((packed))
{
    union
    {
        u32 features;

        struct
        {
            u32 realTimeProcedureDataSupport        : 1;
            u32 getLostProcedureDataSegmentsSupport : 1;
            u32 abortOperationSupport               : 1;
            u32 filterProcedureDataSupport          : 1;
            u32 pctParseFormatSupport               : 1;
            u32 rfu                                 : 27;
        };
    };
} svc_ras_feature_t;

/**
 * @brief     for user add default RAS service in all GAP server.
 * @param[in] none.
 * @return    none.
 */
void blc_svc_addRasGroup(void);

/**
 * @brief     for user remove default RAS service in all GAP server.
 * @param[in] none.
 * @return    none.
 */
void blc_svc_removeRasGroup(void);

/**
 * @brief     for user register read or write attribute value callback function in RAS server.
 * @param[in] readCback: read attribute value callback function pointer.
 * @param[in] wrtieCback: write attribute value callback function pointer.
 * @return    none.
 */
void blc_svc_rasCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback);
