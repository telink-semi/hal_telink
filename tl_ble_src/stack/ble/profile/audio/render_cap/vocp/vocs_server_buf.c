/********************************************************************************************************
 * @file    vocs_server_buf.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


const int gAppVocsSvrInstNum = APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM;


_attribute_ble_data_retention_
blc_vocs_server_t vocs_server[APP_AUDIO_VOCS_SERVER_MAX_INSTANCE_NUM];


blc_vocs_server_t *blt_vocss_getServerBuf(u8 instIdx)
{
    assert(instIdx < gAppVocsSvrInstNum);

    return &vocs_server[instIdx];
}


