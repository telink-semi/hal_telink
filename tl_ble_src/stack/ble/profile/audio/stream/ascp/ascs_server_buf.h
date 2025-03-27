/********************************************************************************************************
 * @file    ascs_server_buf.h
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

/* <<ASCS_v1.0.pdf>> Page 20, Table 4.3: Additional_ASE_Parameters format when ASE_State = 0x01 (Codec Configured) */
typedef struct{ // 39Bytes
    u8 aseID;
    u8 aseState;
    u8 framing;                      /*<! Unframed support flag      */
    u8 prefPHY;                      /*<! Preferred PHY              */
    u8 prefRetransmitNum;            /*<! Range: 0x00 - 0xFF         */
    u16 maxTransportLatency;          /*<! Unit: ms, Range: 0x0005-0x0FA0 */

    u8 presentationDelayMin[3];      /*<! Unit: us                   */
    u8 presentationDelayMax[3];      /*<! Unit: us                   */
    u8 prefPresentationDelayMin[3];  /*<! Unit: us                   */
    u8 prefPresentationDelayMax[3];  /*<! Unit: us                   */

    blc_audio_codec_id_t  codecId;
    u8  codecSpecCfgLen;
    u8  codecSpecCfg[19];         /*<! max = 19Byte. Shall exist only if the Codec_Specific_Configuration_Length field is not 0x00. */
} blt_ascss_aseStateCodecCfg_t;

typedef struct{
    u8 aseID;
    u8 aseState;
    u8 cigID;
    u8 cisID;
    u8 sduInterval[3];
    u8 framing;
    u8 phy;
    u16 maxSdu;
    u8 retranNum;
    u16 maxTranLatency;
    u8 presentationDelay[3];
} blt_ascss_aseStateQosCfg_t;

typedef struct{
    u8 aseID;
    u8 aseState;
    u8 cigID;
    u8 cisID;
    u8 metadataLen;
    u8 metadata[AUDIO_MAX_METADATA_BUFF_LEN];
} blt_ascss_aseStateEnabling_t, blt_ascss_aseStateStreaming_t, blt_ascss_aseStateDisabling_t, blt_ascss_aseStateOther_t;

typedef struct{
    u16 aseHandle;

    u8 dir;   //audio_dir_enum
    u8 aseID;
    u8 state;   //blt_ascs_ase_state_enum
    u8 notifFlag;

    u16   cisHandle;
    bool  cisEstablish;
    bool  dataPathSetup;
    blt_ascss_aseStateCodecCfg_t  codecState;
    blt_ascss_aseStateQosCfg_t    QosState;
    blt_ascss_aseStateOther_t     otherState;
    blt_ascss_aseStateStreaming_t streamingState;
    blt_ascss_aseStateDisabling_t disablingState;
} blt_ascss_ase_state_t;

typedef struct{
    u16 aseCtrlHandle;
    u16 aseCtrlNtfLen; //align 4B
    u8 aseCnt;
    u8 reserved;
    u8 aseCtrlNtf[2+3*STACK_AUDIO_ASCSS_MAX_ASE_CNT]; //Format refer to blt_audio_ascpAesCpNotify_t
    blt_ascss_ase_state_t *aseState[STACK_AUDIO_ASCSS_MAX_ASE_CNT];
} blc_ascs_server_t;

typedef struct blc_ascs_server_ctrl{
    blc_prf_proc_t process;
    blc_ascs_server_t* pAcscServer[STACK_PRF_ACL_PERIPHERAL_MAX_NUM];
} blc_ascs_server_ctrl_t;

typedef struct{

} blc_ascss_regParam_t;

/**********************Audio Announcement*************************/

typedef struct {
    blc_adv_ltv_t ltv;
    u16 ascsUuid;
    u8  type;
    u32 availableContext;
    u8  metadataLen;
    u8  metadata[64];
}blc_bapAnnouncement_t;

extern const blc_bapAnnouncement_t bapTargetDefAnnouncement;

extern const blc_bapAnnouncement_t bapGeneralDefAnnouncement;

