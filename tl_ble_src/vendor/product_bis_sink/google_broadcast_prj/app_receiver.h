/********************************************************************************************************
 * @file    app_receiver.h
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

#include "../bis_sink_config.h"
#if (PRODUCT_BIS_SINK_SELECT == PRODUCT_GOOGLE_BROADCAST_SINK)


    #define FILTER_COMPLETE_NAME   "Telink-BIS"
    #define FILTER_BROADCAST_NAME  "Telink-trans-value"
    #define DEFAULT_BROADCAST_CODE "Telink 9518 EVK"

void app_bis_receiver_init(void);

void app_bis_receiver_handler(void);


#endif
