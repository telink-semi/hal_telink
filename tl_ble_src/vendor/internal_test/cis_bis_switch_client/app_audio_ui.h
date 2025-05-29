/********************************************************************************************************
 * @file    app_audio_ui.h
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

#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_CLIENT)

typedef enum
{
    APP_AUDIO_UNICAST_CLIENT_STATE_IDLE,
    APP_AUDIO_UNICAST_CLIENT_STATE_CONN,
    APP_AUDIO_UNICAST_CLIENT_STATE_DISCONN,
} app_audio_unicast_state_enum;

app_audio_unicast_state_enum CIG_status;

typedef void (*unicast_state_changed_cb)(app_audio_unicast_state_enum state);
void unicast_set_state_changed_cb(unicast_state_changed_cb cb);

/**
 * @brief       broadcast assistant UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_ui_init(void);

/**
 * @brief       broadcast assistant UI loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_ui_loop(void);

#endif //SOURCE_VERSION == SOURCE_WITH_ASSISTANT
