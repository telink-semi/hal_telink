/********************************************************************************************************
 * @file    pacs_client_buf.h
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

/* PACS characteristics read */
typedef enum
{
    PACS_READ_SINK_PAC,
    PACS_READ_SINK_AUDIO_LOC,
    PACS_READ_SRC_PAC,
    PACS_READ_SRC_AUDIO_LOC,
    PACS_READ_AVA_AUDIO_CTX,
    PACS_READ_SUP_AUDIO_CTX,
    PACS_READ_MAX
} blt_pacs_read_enum;

/* PACS characteristics write */
typedef enum
{
    PACS_WRITE_SINK_AUDIO_LOC = 0x00,
    PACS_WRITE_SRC_AUDIO_LOC  = 0x01,
    PACS_WRITE_MAX
} blt_pacs_write_enum;

typedef struct
{
    u16 pacLen;
    u8  pac[APP_AUDIO_PACS_CLIENT_READ_PAC_MAX_SIZE];
} blc_audio_pacRecordParamEntity_t;

typedef struct
{
    u16 pacLen;
    u8  pac[];
} blt_audio_pac_record_param_t;

/* Read PAC used */
typedef ble_sts_t (*pacs_read_pac_cb_t)(u16 connHdl, blt_pacs_read_enum rdType);

typedef struct
{
    pacs_read_pac_cb_t func;
    gattc_read_cfg_t   rdCfg;
    u8                 rdTypeCfg; //blt_pacs_read_enum  rdTypeCfg;
    u8                 currentIdx;
    u8                 reserved1[2];
} blt_pacs_read_pac_t;

typedef struct
{
    gattc_sub_ccc_msg_t ntfInput;
    /* Characteristic value handle */
    u16 sinkAudioLcaHdl;                                 /* Sink Audio Locations */
    u16 srcAudioLcaHdl;                                  /* Source Audio Locations */
    u16 avaAudioCtxHdl;                                  /* Available Contexts */
    u16 supAudioCtxHdl;                                  /* Supported Context */
    u16 sinkPacHdl[STACK_AUDIO_PACS_SNK_PAC_RECORD_NUM]; /* Sink PAC */
    u16 srcPacHdl[STACK_AUDIO_PACS_SRC_PAC_RECORD_NUM];  /* Source PAC */

    /* ACL connection handle */
    u16 connHandle;
    u16 reserved;

    /* Characteristic value */
    u32 sinkAudioLca; /* Sink Audio Locations*/
    u32 srcAudioLca;  /* Source Audio Locations */

    struct
    {
        u16 avaSinkCtx; /* Available Sink Contexts */
        u16 avaSrcCtx;  /* Available Source Context */
    } avaAudioCtx;

    struct
    {
        u16 supSinkCtx; /* Supported Sink Contexts */
        u16 supSrcCtx;  /* Supported Source Contexts */
    } supAudioCtx;

    /* PAC record numbers */
    u8 sinkPacRcdNum;
    u8 srcPacRcdNum;
    u8 rdSinkPacRcdIdx;
    u8 rdSrcPacRcdIdx;

    blt_audio_pac_record_param_t *pSinkPacRcd[STACK_AUDIO_PACS_SNK_PAC_RECORD_NUM];
    blt_audio_pac_record_param_t *pSrcPacRcd[STACK_AUDIO_PACS_SRC_PAC_RECORD_NUM];


} blc_pacs_client_t;

typedef struct blc_pacs_client_ctrl
{
    blc_prf_proc_t     process;
    blc_pacs_client_t *pPacsClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
} blc_pacs_client_ctrl_t;

typedef struct
{
} blc_pacsc_regParam_t;
