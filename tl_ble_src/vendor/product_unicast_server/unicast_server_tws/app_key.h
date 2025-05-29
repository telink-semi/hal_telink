/********************************************************************************************************
 * @file    app_ui.h
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
#ifndef APP_UI_H_
#define APP_UI_H_
#include "app_config.h"
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_TWS)


typedef enum
{
    KEY_MODE_NULL = 0,
    KEY_MODE_TEST1,
    KEY_MODE_TEST2,
    KEY_MODE_TEST3,
    KEY_MODE_TEST4,
    KEY_MODE_TEST5,
    KEY_MODE_MAX
} key_evt_mode_e;

/**
 * @brief     This function serves to init the key function.
 * @param[in] None.
 * @returns   None.
 */
void app_key_init(void);

#endif
#endif
