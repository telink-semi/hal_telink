/********************************************************************************************************
 * @file    ascs_server_buf.c
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


const u8 gAscssSinkAseCnt = APP_AUDIO_ASCSS_SINK_ASE_CNT;
const u8 gAscssSrcAseCnt = APP_AUDIO_ASCSS_SRC_ASE_CNT;


_attribute_ble_data_retention_
blt_ascss_ase_state_t gAseState[APP_AUDIO_ASCSS_ASE_CNT * ACL_PERIPHR_MAX_NUM];

_attribute_ble_data_retention_
blc_ascs_server_t gAscss[ACL_PERIPHR_MAX_NUM];

blt_ascss_ase_state_t* blc_ascss_getAseStateInfo(u8 index)
{
    return index>(APP_AUDIO_ASCSS_ASE_CNT * ACL_PERIPHR_MAX_NUM) ? NULL : &gAseState[index];
}

blc_ascs_server_t* blc_ascss_getAscssInfo(u8 index)
{
    return index>ACL_PERIPHR_MAX_NUM ? NULL : &gAscss[index];
}

void blc_ascss_initAseParam(blt_ascss_ase_state_t* aseState)
{
    u8* codecCfg = &aseState->codecState.framing;
    U8_TO_STREAM(codecCfg,  AUDIO_UNICAST_SERVER_SUPPORT_FRAMING);
    U8_TO_STREAM(codecCfg,  AUDIO_UNICAST_SERVER_PREFERRED_PHY);
    U8_TO_STREAM(codecCfg,  AUDIO_UNICAST_SERVER_PREFERRED_RTN);
    U16_TO_STREAM(codecCfg, AUDIO_UNICAST_SERVER_MAX_TRANSPORT_LATENCY);
    U24_TO_STREAM(codecCfg, AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MIN);
    U24_TO_STREAM(codecCfg, AUDIO_UNICAST_SERVER_PRESENTATION_DELAY_MAX);
    U24_TO_STREAM(codecCfg, AUDIO_UNICAST_SERVER_PREFERRED_PRESENTATION_DELAY_MIN);
    U24_TO_STREAM(codecCfg, AUDIO_UNICAST_SERVER_PREFERRED_PRESENTATION_DELAY_MAX);
}

const blc_bapAnnouncement_t bapTargetDefAnnouncement = {
        .ltv.len = 17,
        .ltv.type = DT_SERVICE_DATA_16BIT_UUID,
        .ascsUuid = SERVICE_UUID_AUDIO_STREAM_CONTROL,
        .type = BLC_AUDIO_TARGETED_ANNOUNCEMENT,
        .availableContext = AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT|AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT<<16,
        .metadataLen = 8,
        .metadata = {// length  --  type  --  value
                0x03, BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS,(AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF), (AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF00) >> 8,
                0x03, BLC_AUDIO_METATYPE_STREAMING_CONTEXTS,(AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF), (AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF00) >> 8,
        },
};

const blc_bapAnnouncement_t bapGeneralDefAnnouncement = {
        .ltv.len = 17,
        .ltv.type = DT_SERVICE_DATA_16BIT_UUID,
        .ascsUuid = SERVICE_UUID_AUDIO_STREAM_CONTROL,
        .type = BLC_AUDIO_GENERAL_ANNOUNCEMENT,
        .availableContext = AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT,
        .metadataLen = 8,
        .metadata = {// length  --  type  --  value
                0x03, BLC_AUDIO_METATYPE_PREFERRED_CONTEXTS,(AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF), (AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF00) >> 8,
                0x03, BLC_AUDIO_METATYPE_STREAMING_CONTEXTS,(AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF), (AUDIO_UNICAST_SERVER_DEFAULT_CONTEXT & 0xFF00) >> 8,
        },
};



