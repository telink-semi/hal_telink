/********************************************************************************************************
 * @file    pacs_client_buf.c
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

const u8 gAppPacsCltSinkPacNum = APP_AUDIO_PACS_SNK_PAC_RECORD_NUM;
const u8 gAppPacsCltSrcPacNum = APP_AUDIO_PACS_SRC_PAC_RECORD_NUM;
const u16 gAppPacsCltPacMaxSize = APP_AUDIO_PACS_CLIENT_READ_PAC_MAX_SIZE;

_attribute_ble_data_retention_
blc_audio_pacRecordParamEntity_t gPacscSinkPacRcd[ACL_CENTRAL_MAX_NUM][APP_AUDIO_PACS_SNK_PAC_RECORD_NUM];

_attribute_ble_data_retention_
blc_audio_pacRecordParamEntity_t gPacscSrcPacRcd[ACL_CENTRAL_MAX_NUM][APP_AUDIO_PACS_SRC_PAC_RECORD_NUM];

_attribute_ble_data_retention_
blc_pacs_client_t gPacsClient[ACL_CENTRAL_MAX_NUM];




blc_pacs_client_t *blt_pacsc_getClientBuf(u8 instIdx)
{
    assert(instIdx < gAppAudioAclCentralNum);

    return &gPacsClient[instIdx];
}

blt_audio_pac_record_param_t *blt_pacsc_getSinkPacBuf(u8 aclIdx, u8 instIdx)
{
    assert(aclIdx < gAppAudioAclCentralNum);
    assert(instIdx < gAppPacsCltSinkPacNum);

    return (blt_audio_pac_record_param_t*)&gPacscSinkPacRcd[aclIdx][instIdx];
}

blt_audio_pac_record_param_t *blt_pacsc_getSrcPacBuf(u8 aclIdx, u8 instIdx)
{
    assert(aclIdx < gAppAudioAclCentralNum);
    assert(instIdx < gAppPacsCltSrcPacNum);

    return (blt_audio_pac_record_param_t*)&gPacscSrcPacRcd[aclIdx][instIdx];
}




