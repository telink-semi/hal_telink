/********************************************************************************************************
 * @file    ras_server_buf.h
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
#if (defined(HOST_V2_ENABLE))
#include "stack/ble/host_v1/services/svc_cs/ras/svc_ras.h"
#endif
/**
 * @brief currently set data exchange mechanism - bases on ondemand / realtime ccc configuration
 */
typedef enum
{
    PROC_DATA_EXCHG_NULL,
    PROC_DATA_EXCHG_LOCAL,
    PROC_DATA_EXCHG_REALTIME_NOTIFICATIONS,
    PROC_DATA_EXCHG_ONDEMAND_NOTIFICATIONS,
    PROC_DATA_EXCHG_REALTIME_INDICATIONS,
    PROC_DATA_EXCHG_ONDEMAND_INDICATIONS,
} blc_rass_procedure_data_exchange_mechanism_enum;

/**
 * @brief the data structure of register RAS server parameter.
 */
typedef struct __attribute__((packed))
{
    svc_ras_feature_t ras_feature;
} blc_rass_regParam_t;
