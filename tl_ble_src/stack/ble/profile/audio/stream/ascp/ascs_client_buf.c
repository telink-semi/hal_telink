/********************************************************************************************************
 * @file    ascs_client_buf.c
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


const u8 gAppAscsCltSinkAseNum = APP_AUDIO_ASCS_ASE_SNK_NUM;
const u8 gAppAscsCltSrcAseNum = APP_AUDIO_ASCS_ASE_SRC_NUM;
const u16 gAppAscsMetadataLen = DATA_LENGTH_ALIGN4(AUDIO_MAX_METADATA_BUFF_LEN);

_attribute_ble_data_retention_
u8 gAscscMetadataBuff[ACL_CENTRAL_MAX_NUM*APP_AUDIO_ASCS_ASE_NUM][DATA_LENGTH_ALIGN4(AUDIO_MAX_METADATA_BUFF_LEN)];


_attribute_ble_data_retention_
blt_ascsc_ase_t gAscscSinkAse[ACL_CENTRAL_MAX_NUM][APP_AUDIO_ASCS_ASE_SNK_NUM];

_attribute_ble_data_retention_
blt_ascsc_ase_t gAscscSrcAse[ACL_CENTRAL_MAX_NUM][APP_AUDIO_ASCS_ASE_SRC_NUM];

_attribute_ble_data_retention_
blc_ascs_client_t gAscsClient[ACL_CENTRAL_MAX_NUM];




blc_ascs_client_t *blt_ascsc_getClientBuf(u8 instIdx)
{
    assert(instIdx < gAppAudioAclCentralNum);

    return &gAscsClient[instIdx];
}

blt_ascsc_ase_t *blt_ascsc_getSinkAseBuf(u8 aclIdx, u8 instIdx)
{
    assert(aclIdx < gAppAudioAclCentralNum);
    assert(instIdx < gAppAscsCltSinkAseNum);

    return &gAscscSinkAse[aclIdx][instIdx];
}

blt_ascsc_ase_t *blt_ascsc_getSrcAseBuf(u8 aclIdx, u8 instIdx)
{
    assert(aclIdx < gAppAudioAclCentralNum);
    assert(instIdx < gAppAscsCltSrcAseNum);

    return &gAscscSrcAse[aclIdx][instIdx];
}

u8 *blt_ascsc_getMetadataBuf(u8 aclIdx, u8 instIdx)
{
    assert(aclIdx < gAppAudioAclCentralNum);
    assert(instIdx < gAppAscsCltSrcAseNum+gAppAscsCltSinkAseNum);

    return &gAscscMetadataBuff[aclIdx+gAppAscsMetadataLen*instIdx][0];
}


