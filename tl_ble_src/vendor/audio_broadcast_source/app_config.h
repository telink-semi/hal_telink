/********************************************************************************************************
 * @file    app_config.h
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


#include "source_config.h"

#define TLKAPI_DEBUG_FIFO_SIZE 144
#define TLKAPI_DEBUG_FIFO_NUM  32


#if (SOURCE_VERSION == SOURCE_ONLY_VERSION)
    #include "source_only/app_config.h"
#elif (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)
    #include "source_with_assistant/app_config.h"
#else
    #error "need include one app_config.h at least"
#endif
