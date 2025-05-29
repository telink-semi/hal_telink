/********************************************************************************************************
 * @file    vocs_client_buf.c
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

const int gAppVocsCltInstNum = ACL_CENTRAL_MAX_NUM * APP_AUDIO_VOCS_CLIENT_MAX_INSTANCE_NUM;


_attribute_ble_data_retention_
    blc_vocs_client_t gVocsClient[ACL_CENTRAL_MAX_NUM * APP_AUDIO_VOCS_CLIENT_MAX_INSTANCE_NUM];

blc_vocs_client_t *blt_vocsc_getClientBuf(void)
{
    for (int i = 0; i < gAppVocsCltInstNum; i++) {
        if (gVocsClient[i].useFlag) {
            continue;
        }
        gVocsClient[i].useFlag = true;
        return &gVocsClient[i];
    }
    return NULL;
}

void blt_vocsc_cleanBuf(void)
{
    memset(gVocsClient, 0, sizeof(blc_vocs_client_t) * gAppVocsCltInstNum);
}
