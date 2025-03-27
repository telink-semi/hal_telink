/********************************************************************************************************
 * @file    pacs_internal.h
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

#define BLT_PACS_LOG(fmt, ...)          BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_PACS_LOG, "[PACS]"fmt, ##__VA_ARGS__)


typedef enum{
    BLT_PAC_SINK,
    BLT_PAC_SOURCE
}blt_pac_e;


/*
 * PACS: ATT handle information: 17byte
 * sinkPacRcdHdl: Support up to 4: refer to STACK_AUDIO_PACS_SNK_PAC_RECORD_NUM
 * sourcePacRcdHdl: Support up to 4: refer to STACK_AUDIO_PACS_SRC_PAC_RECORD_NUM
 */
typedef struct{
    u16 baseHandle;
    u8 endHdl;
    u8 sinkAudioLcaHdl; //NTF
    u8 srcAudioLcaHdl; //NTF
    u8 avaAudioCtxHdl; //NTF
    u8 supAudioCtxHdl; //NTF
    u8 sinkPacRcdHdl[STACK_AUDIO_PACS_SNK_PAC_RECORD_NUM]; //NTF
    u8 sourcePacRcdHdl[STACK_AUDIO_PACS_SRC_PAC_RECORD_NUM]; //NTF
} blt_pacs_att_hdl_t;

typedef struct {
    blt_pacs_att_hdl_t att;
    u8 sinkPacRcdNum; //NTF
    u8 srcPacRcdNum; //NTF
} blt_pacs_nv_info_t;

int blt_pacsc_init(u8 initType, const void* param);
int blt_pacsc_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_pacsc_discovery(u16 connHandle);
int blt_pacs_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t* param);


blc_pacs_client_t *blt_pacsc_getClientBuf(u8 instIdx);
blt_audio_pac_record_param_t *blt_pacsc_getSinkPacBuf(u8 aclIdx, u8 instIdx);
blt_audio_pac_record_param_t *blt_pacsc_getSrcPacBuf(u8 aclIdx, u8 instIdx);



blc_pacs_client_t *blt_pacsc_getClientInst(u16 connHandle);
int blt_pacsc_svcDiscovery(u16 connHandle);










int blt_pacss_init(u8 initType, const void* param);
int blt_pacss_connect(u16 connHandle, prf_acl_state_enum connState);



u8 blt_pacss_getRecordParam(u16 connHandle, u8 type, u8 *pCodecId, blt_audio_pacParam_t *pParam);
u16 blt_pacss_getAvailableContext(u16 connHandle, u8 type);

int blt_pacss_checkCodecCfgParam(u16 connHandle,blt_pac_e type,blc_audio_codecSpecCfgParsed_t *codecCfg);






