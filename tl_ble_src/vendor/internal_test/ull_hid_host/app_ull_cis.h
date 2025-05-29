/********************************************************************************************************
 * @file    app_att.h
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

#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_HOST)

    #define ULL_CIS_LOG(fmt, ...) tlkapi_printf(APP_ULL_HID_LOG_EN, "[ULL-CIS]" fmt "\n", ##__VA_ARGS__)


/**
 * @brief   initial Ultra Low Latency HID Host.
 * @param   none.
 * @return  none.
 */
void app_initial_ull_cis_host(void);

void app_ullhid_initCigParam(void);

#endif //INTER_TEST_MODE == TEST_ULL_HID_HOST
