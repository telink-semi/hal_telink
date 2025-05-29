/********************************************************************************************************
 * @file    auto_test_config.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    11,2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef AUTO_TEST_CONFIG_H_
#define AUTO_TEST_CONFIG_H_

#include "../feature_config.h"

#include "config.h"

#if (FEATURE_TEST_MODE == TEST_AUTO)
    /////////////////// AUTO TEST SELECTION /////////////////////////////////

    #define AUTO_TEST_NONE      0
    #define AUTO_TEST_CONN_DEMO 1

    #define AUTO_TEST_MODE      AUTO_TEST_CONN_DEMO

#endif

#endif /* AUTO_TEST_CONFIG_H_ */
