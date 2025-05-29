/********************************************************************************************************
 * @file    ascs_internal.h
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

#include "../bap.h"

#define BLT_ASCS_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_ASCS_LOG, "[ASCS]" fmt, ##__VA_ARGS__)

typedef enum
{
    BLT_ASCS_ASE_STATE_IDLE      = 0x00,
    BLT_ASCS_ASE_STATE_CODEC_CFG = 0x01,
    BLT_ASCS_ASE_STATE_QOS_CFG   = 0x02,
    BLT_ASCS_ASE_STATE_ENABLING  = 0x03,
    BLT_ASCS_ASE_STATE_STREAMING = 0x04,
    BLT_ASCS_ASE_STATE_DISABLING = 0x05,
    BLT_ASCS_ASE_STATE_RELEASING = 0x06,
} blt_ascs_ase_state_enum;

/* <<ASCS_v1.0.pdf>> Page 24, Table 4.6: ASE Control operations */
typedef enum
{
    BLT_ASCS_OPCODE_CONFIG_CODEC           = 0x01,
    BLT_ASCS_OPCODE_CONFIG_QOS             = 0x02,
    BLT_ASCS_OPCODE_CONFIG_ENABLE          = 0x03,
    BLT_ASCS_OPCODE_CONFIG_RECV_START      = 0x04,
    BLT_ASCS_OPCODE_CONFIG_DISABLE         = 0x05,
    BLT_ASCS_OPCODE_CONFIG_RECV_STOP       = 0x06,
    BLT_ASCS_OPCODE_CONFIG_UPDATE_METADATA = 0x07,
    BLT_ASCS_OPCODE_CONFIG_RELEASE         = 0x08,
    /* BLT_AUDIO_ASCS_OPCODE_CONFIG_RELEASED = "-": Releasing to Idle or Codec_cfg, allowed only when ASE_State = 0x06 */
} blt_ascs_ase_ctrl_opcode_enum;

/* <<ASCS_v1.0.pdf>> Page 28, Table 5.1: ASE Control Point characteristic Response_Code and Reason values when notified */
typedef enum
{
    BLT_ASCS_RSP_CODE_SUCCESS                 = 0x0000,
    BLT_ASCS_RSP_CODE_UNSUPPORTED_OPCODE      = 0x0001,
    BLT_ASCS_RSP_CODE_INVALID_LENGTH          = 0x0002,
    BLT_ASCS_RSP_CODE_INVALID_ASE_ID          = 0x0003,
    BLT_ASCS_RSP_CODE_INVALID_ASE_STATE       = 0x0004,
    BLT_ASCS_RSP_CODE_INVALID_ASE_DIRECTION   = 0x0005,
    BLT_ASCS_RSP_CODE_UNSUPP_AUDIO_CAPABILITY = 0x0006,

    BLT_ASCS_RSP_CODE_UNSUPP_CONFIG_PARAM   = 0x0007, // need  BLT_AUDIO_ASCS_REASON_ENUM
    BLT_ASCS_RSP_CODE_REJECTED_CONFIG_PARAM = 0x0008, // need  BLT_AUDIO_ASCS_REASON_ENUM
    BLT_ASCS_RSP_CODE_INVALID_CONFIG_PARAM  = 0x0009, // need  BLT_AUDIO_ASCS_REASON_ENUM

    BLT_ASCS_RSP_CODE_UNSUPP_METADATA   = 0x000A,     // need  0xXX Value of the Metadata Type field in error
    BLT_ASCS_RSP_CODE_REJECTED_METADATA = 0x000B,     // need  0xXX Value of the Metadata Type field in error
    BLT_ASCS_RSP_CODE_INVALID_METADATA  = 0x000C,     // need  0xXX Value of the Metadata Type field in error

    BLT_ASCS_RSP_CODE_INSUFFICIENT_RESOURCE = 0x000D,
    BLT_ASCS_RSP_CODE_UNSPECIFIED_ERROR     = 0x000E,
} blt_ascs_rsp_code_enum;

typedef enum
{
    BLT_ASCS_REASON_SUCCESS                 = 0x00,
    BLT_ASCS_REASON_CODEC_ID                = 0x01,
    BLT_ASCS_REASON_CODEC_SEPC_CONFIG       = 0x02,
    BLT_ASCS_REASON_SDU_INTERVAL            = 0x03,
    BLT_ASCS_REASON_FRAMING                 = 0x04,
    BLT_ASCS_REASON_PHY                     = 0x05,
    BLT_ASCS_REASON_MAX_SDU_SIZE            = 0x06,
    BLT_ASCS_REASON_RETRANS_NUMBER          = 0x07,
    BLT_ASCS_REASON_MAX_LATENCY             = 0x08,
    BLT_ASCS_REASON_PRESENT_DELAY           = 0x09,
    BLT_ASCS_REASON_INVALID_AES_CIS_MAPPING = 0x0A,
} blt_ascs_reason_enum;

typedef struct
{
    u8 opcode;
    u8 fixSize;
    u8 variableSize;
} aseCtrlLega_t;

/* <<ASCS_v1.0.pdf>> Page 24, Table 4.7: Format of ASE Control Point characteristic value when notified */
typedef struct
{
    u8 aseID;
    u8 responseCode;
    u8 reason;
} aseCtrlNtfPayload_t;

typedef struct
{
    u8                  opcode;
    u8                  numOfAses;
    aseCtrlNtfPayload_t payload[0];
} blt_ascs_aseCtrlPointCharNtf_t;

/* <<ASCS_v1.0.pdf>> Page 29, Table 5.2: Config Codec operation format */
typedef struct
{
    u8                   aseID;
    u8                   tgtLatency;
    u8                   tgtPhy;
    blc_audio_codec_id_t codecID;
    u8                   codecSpecCfgLen;
    u8                   codecSpecCfg[0];
} blt_ascs_cfgCodec_t;

/* <<ASCS_v1.0.pdf>> Page 31, Table 5.3: Config QoS operation format */
typedef struct
{
    u8  aseID;
    u8  cigID;
    u8  cisID;
    u8  sduInterval[3];
    u8  framing;
    u8  PHY;
    u16 maxSDU;
    u8  RTN;
    u16 maxTranLatency;
    u8  presentationDelay[3];
} blt_ascs_cfgQos_t;

/* <<ASCS_v1.0.pdf>> Page 32, Table 5.4: Enable operation format */

typedef struct
{
    u8 aseID;
    u8 metadataLen;
    u8 metadataCfg[0];
} blt_ascs_enable_t, blt_ascs_updateMetadata_t, blt_ascs_metadata_t;

typedef struct
{
    u8  SDUMinInterval[3];       /*<! Range: 0x0000FF - 0xFFFFFF */
    u8  SDUMaxInterval[3];       /*<! Range: 0x0000FF - 0xFFFFFF */
    u8  framing;                 /*<! Preferred Frame            */
    u8  PHY;                     /*<! Preferred PHY              */
    u16 maxSDU;
    u8  retransmitNum;           /*<! Range: 0x00 - 0x0F         */
    u16 maxTransportLatency;     /*<! Range: 0x0005C0x0FA0      */
    u8  minPresentationDelay[3]; /*<! Unit: us                   */
    u8  maxPresentationDelay[3]; /*<! Unit: us                   */
} blt_ascs_preferParam_t;

typedef enum
{
    BLT_ASCSC_ASE_FLAG_NONE = 0x0000,

    BLT_ASCSC_ASE_FLAG_ENABLE     = 0x8000,
    BLT_ASCSC_ASE_FLAG_DISABLE    = 0x4000,
    BLT_ASCSC_ASE_FLAG_SEND_WAIT  = 0x2000,
    BLT_ASCSC_ASE_FLAG_SEND_CODEC = 0x1000,

    BLT_ASCSC_ASE_FLAG_SEND_QOS     = 0x0800,
    BLT_ASCSC_ASE_FLAG_SEND_ENABLE  = 0x0400,
    BLT_ASCSC_ASE_FLAG_SEND_DISABLE = 0x0200,
    BLT_ASCSC_ASE_FLAG_SEND_START   = 0x0100, //Receiver Start Ready operation

    BLT_ASCSC_ASE_FLAG_SEND_STOP    = 0x0080, //Receiver Stop Ready operation
    BLT_ASCSC_ASE_FLAG_SEND_RELEASE = 0x0040,
    BLT_ASCSC_ASE_FLAG_SEND_UPDATE  = 0x0020,
    BLT_ASCSC_ASE_FLAG_CREATE_CIS   = 0x0010,

    BLT_ASCSC_ASE_FLAG_DESTROY_CIS      = 0x0008,
    BLT_ASCSC_ASE_FLAG_CREATE_CIS_WAIT  = 0x0004,
    BLT_ASCSC_ASE_FLAG_DESTROY_CIS_WAIT = 0x0002,

    BLT_ASCSC_ASE_FLAG_ENABLE_MASK = BLT_ASCSC_ASE_FLAG_SEND_CODEC |
                                     BLT_ASCSC_ASE_FLAG_SEND_QOS |
                                     BLT_ASCSC_ASE_FLAG_SEND_ENABLE |
                                     BLT_ASCSC_ASE_FLAG_SEND_START |
                                     BLT_ASCSC_ASE_FLAG_ENABLE,

    BLT_ASCSC_ASE_FLAG_DISABLE_MASK = BLT_ASCSC_ASE_FLAG_SEND_DISABLE |
                                      BLT_ASCSC_ASE_FLAG_SEND_STOP |
                                      BLT_ASCSC_ASE_FLAG_SEND_RELEASE |
                                      BLT_ASCSC_ASE_FLAG_DISABLE,
} blt_ascs_ase_flags_enum;

typedef enum
{
    BLT_ASCSC_ASE_READY_NONE  = 0x0000,
    BLT_ASCSC_ASE_PARAM_READY = 0x8000,
    BLT_ASCSC_ASE_CODEC_READY = 0x0001,
} blt_ascs_ase_ready_enum;

typedef struct
{
    u8               cisID;
    u8               cisNum;
    u16              reserved;
    blt_ascsc_ase_t *pAseList[STACK_AUDIO_ASCS_ASE_NUM];
} blt_ascsc_cissInfo_t;

/*
 * ASCS: ATT handle information: 10byte
 * sinAseHdl: Support up to 2: refer to STACK_AUDIO_ASCS_ASE_SNK_NUM
 * sourceAseHdl: Support up to 2: refer to STACK_AUDIO_ASCS_ASE_SRC_NUM
 */
typedef struct
{
    u16 baseHandle;
    u8  endHdl;
    u8  aseCtrlHdl;                                 //NTF
    u8  sinkAseHdl[STACK_AUDIO_ASCS_ASE_SNK_NUM];   //NTF
    u8  sourceAseHdl[STACK_AUDIO_ASCS_ASE_SRC_NUM]; //NTF
} blt_ascs_att_hdl_t;

typedef struct
{
    blt_ascs_att_hdl_t att;
    u8                 sinkAseCnt;
    u8                 srcAseCnt;
} blt_ascs_nv_info_t;

int blt_ascsc_init(u8 initType, const void *param);
int blt_ascsc_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_ascsc_discovery(u16 connHandle);
int blt_ascsc_loop(u16 connHandle);
int blt_ascsc_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);


blc_ascs_client_t *blt_ascsc_getClientBuf(u8 instIdx);
blt_ascsc_ase_t   *blt_ascsc_getSinkAseBuf(u8 aclIdx, u8 instIdx);
blt_ascsc_ase_t   *blt_ascsc_getSrcAseBuf(u8 aclIdx, u8 instIdx);
blt_ascsc_ase_t   *blt_ascsc_getAsePtrByAseId(u16 connHandle, u8 aseID);
u8                *blt_ascsc_getMetadataBuf(u8 aclIdx, u8 instIdx);


int blt_ascss_init(u8 initType, const void *param);
int blt_ascss_connect(u16 connHandle, prf_acl_state_enum connState);

blc_ascs_client_t *blt_ascsc_getClientInst(u16 connHandle);

int blt_ascsc_enableAse(u16 aclHandle, u8 aseID);
int blt_ascsc_disableAse(u16 aclHandle, u8 aseID);
int blt_ascsc_releaseAse(u16 aclHandle, u8 aseID);
int blt_ascsc_setMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen);
int blt_ascsc_updateMetadata(u16 aclHandle, u8 aseID, u8 *pMetadata, u8 metadataLen);

int blt_ascsc_cisConnectEvt(u16 cisHandle, u8 *pPkt);
int blt_ascsc_cisDisconnEvt(u16 cisHandle, u8 *pPkt);
int blt_ascsc_termCisByAse(u16 connHandle, u8 aseID, u8 reason);
int blt_ascsc_removeCIGByAse(u16 connHandle, u8 aseID);                  //not used now
int blt_ascsc_setAseCfg(u16 connHandle, u8 aseID, blc_ascsc_aseConfig_t *pCfg);
int blt_ascsc_setAllAseCfg(u16 connHandle, blc_ascsc_aseConfig_t *pCfg); //not used now
