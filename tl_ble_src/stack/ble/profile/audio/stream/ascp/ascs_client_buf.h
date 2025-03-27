/********************************************************************************************************
 * @file    ascs_client_buf.h
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


///////////////////////////////////////////////////////////////////////
//              ASCS Client Write ASE control point value
///////////////////////////////////////////////////////////////////////
/* ASCS write ASE control point value format */
typedef struct{
    u8 opcode;
    u8 numOfAses;
    u8 aseParam[0];
} blt_ascsc_write_ase_t;
/* <<ASCS_v1.0.pdf>> Page 29, Table 5.2: Config Codec operation format */
typedef struct {
    u8  aseID;
    u8  tgtLatency;
    u8  tgtPhy;
    blc_audio_codec_id_t codecId;
    u8  codecSpecCfgLen;
    u8  codecSpecCfg[19]; /*<! max = 19Byte. Shall exist only if the Codec_Specific_Configuration_Length field is not 0x00. */
} blt_ascsc_cfg_codec_t;
/* <<ASCS_v1.0.pdf>> Page 31, Table 5.3: Config QoS operation format */
typedef struct {
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
} blt_ascsc_config_qos_t;
/* <<ASCS_v1.0.pdf>> Page 32, Table 5.4: Enable operation format
 *                   Page 35, Table 5.8: Update Metadata operation format */
typedef struct {
    u8  aseID;
    u8  metadataLen;
    u8  metadataCfg[0];
} blt_ascsc_ase_enable_t, blt_ascsc_update_metadata_t;
/* <<ASCS_v1.0.pdf>> Page 33, Table 5.5: Receiver Start Ready operation format
 *                   Page 33, Table 5.6: Disable operation parameters
 *                   Page 34, Table 5.7: Receiver Stop Ready operation
 *                   Page 36, Table 5.9: Release operation format */
typedef struct {
    u8  aseID;
} blt_ascsc_rcv_start_rdy_t, blt_ascsc_ase_disable_t, blt_ascsc_rcv_stop_rdy_t, blt_ascsc_release_t;


///////////////////////////////////////////////////////////////////////
//              ASCS ASE characteristic: ASE state
///////////////////////////////////////////////////////////////////////
/* <<ASCS_v1.0.pdf>> Page 20, Table 4.3: Additional_ASE_Parameters format when ASE_State = 0x01 (Codec Configured) */
typedef struct{
    u8  aseID;
    u8  aseState; // refer ASCS_ASE_STATE_ENUM
    u8  framing;                      /*<! Unframed support flag      */
    u8  prefPHY;                      /*<! Preferred PHY              */
    u8  prefRetransmitNum;            /*<! Range: 0x00 - 0xFF         */
    u16 maxTransportLatency;          /*<! Unit: ms, Range: 0x0005-0x0FA0 */
    u8  presentationDelayMin[3];      /*<! Unit: us                   */
    u8  presentationDelayMax[3];      /*<! Unit: us                   */
    u8  prefPresentationDelayMin[3];  /*<! Unit: us                   */
    u8  prefPresentationDelayMax[3];  /*<! Unit: us                   */
    blc_audio_codec_id_t codecId;
    u8  codecSpecCfgLen;
    u8  codecSpecCfg[19];         /*<! max = 19Byte. Shall exist only if the Codec_Specific_Configuration_Length field is not 0x00. */
} blt_ascsc_aseStateCodecCfg_t;
/* <<ASCS_v1.0.pdf>> Page 21, Table 4.4: Additional_ASE_Parameters format when ASE_State = 0x02 (QoS Configured) */
typedef struct{
    u8  aseID;
    u8  aseState; // refer ASCS_ASE_STATE_ENUM
    u8  cigID;
    u8  cisID;
    u8  SDUInterval[3];
    u8  framing;              // refer AUDIO_FRAMING_UNFRAMED
    u8  PHY;                  // refer AUDIO_PHY_FLAG_1M
    u16 maxSDU;               /*<! Range: 0x00C0xFFF        */
    u8  retransmitNum;        /*<! Range: 0x00C0x0F          */
    u16 maxTransportLatency;  /*<! Range: 0x0005C0x0FA0      */
    u8  presentationDelay[3]; /*<! Unit: us                  */
} blt_ascsc_aseStateQosCfg_t;
/* <<ASCS_v1.0.pdf>> Page 22, Table 4.5: Format of the Additional_ASE_Parameters field when
 * ASE_State = 0x03 (Enabling), 0x04 (Streaming), or 0x05 (Disabling) */
typedef struct{ // 7Bytes
    u8  aseID;
    u8  aseState; // refer ASCS_ASE_STATE_ENUM
    u8  cigID;
    u8  cisID;
    u8  metaDataLen;
    u8  pMetaData[0];   /*<! Varies */
} blt_ascsc_aseStateEnable_t, blt_ascsc_aseStateDisable_t, blt_ascsc_aseStateStream_t;

/* <<ASCS_v1.0.pdf>> Page 18, The format of the ASE characteristic is defined in Table 4.2 */
typedef struct{
    u8 aseID;
    u8 aseState; // refer ASCS_ASE_STATE_ENUM
    u8 addAseParam[0];
} blt_ascsc_aseState_t;
typedef struct{
    union{
        blt_ascsc_aseState_t         aseVal;
        blt_ascsc_aseStateCodecCfg_t codec;
        blt_ascsc_aseStateQosCfg_t   qos;
        blt_ascsc_aseStateEnable_t   enable;
        blt_ascsc_aseStateDisable_t  disable;
        blt_ascsc_aseStateStream_t   stream;
    } u;
} blt_ascsc_aseStateVal_t;

/* <<ASCS_v1.0.pdf>> Page 24, Table 4.7: Format of ASE Control Point characteristic value when notified */
typedef struct {
    u8  aseID;
    u8  responseCode;
    u8  reason;
} blt_ascsc_ase_cp_rsp_t;

typedef struct {
    u8  opcode;
    u8  numOfAses;
    blt_ascsc_ase_cp_rsp_t rsp[0];
} blt_ascsc_ase_cp_ntf_t;

/* Read ASE used */
/* ASCS characteristics read */
typedef enum {
    ASCS_READ_SINK_ASE,
    ASCS_READ_SRC_ASE,
    ASCS_READ_MAX
} blt_ascsc_read_enum;
typedef ble_sts_t (*ascs_read_pac_cb_t) (u16 connHdl, blt_ascsc_read_enum  rdType);
typedef struct {
    ascs_read_pac_cb_t  func;
    gattc_read_cfg_t    rdCfg;
    u8 rdTypeCfg; //blt_ascsc_read_enum  rdTypeCfg;
    u8 currentIdx;
    u8 reserved1[2];
} blt_ascs_read_pac_t;


typedef struct{ // 4Bytes
    u16 cisHdl;
    u16 flags; //Refer to AUDIO_ASE_FLAGS_ENUM
    u16 ready;

    u8  aseID;
    u8  state; //Refer to AUDIO_ASE_STATE_ENUM
    u8  dir;   //Refer to Audio Direction in audio_define.h
    u8  dataPathSetup;

    u8  cisEstablished;
    u8  cisCreateRetryNum;

    u16  cfgCodecFieldExistFlg;
    u16  metaDataFieldExistFlg;

    // Codec configure parameters
    blc_audio_codec_id_t  codecId;
    u8  frequency; // Sampling_Frequency
    u8  duration; // Frame_Duration
    u8  codecFrmBlksPerSDU; // Codec_Frame_Blocks_Per_SDU
    u16 context;  // Metadata
    u16 frameOcts; //Octets_Per_Codec_Frame
    u32 location; // Audio_Channel_Allocation
    /* Refer to 'Target latency type' && 'Target PHY type' */
    u8  tgtLatency; /* Provides context for the server to return meaningful values for QoS preferences in Codec Configured state */
    u8  tgtPHY; /* PHY parameter target to achieve the Target_Latency value */

    //blt_audio_codec_qos_t qos; /** The BAP spec defined QoS values */
    //blt_audio_codec_qos_pref qosPref;
    bool unframedNotSupp; /* 0: Unframed ISOAL PDUs supported; 1:Unframed ISOAL PDUs not supported */
    u8 preferredRTN; /* Server preferred value for the Retransmission_Number parameter */
    u8 stdCodecSettingsIdx;
    u8 reserved[1];

    // Link Layer Param
    u8  cigID; // CIG_ID parameter value written by the client for this ASE
    u8  cisID; // CIS_ID parameter value written by the client for this ASE
    u8  PHY; // PHY parameter value written by the client for this ASE
    u8  RTN; // Retransmission_Number
    u16 maxSDU; // Max_SDU parameter value written by the client for this ASE
    u16 maxTransLatency; // Max_Transport_Latency parameter value written by the client for this ASE
    u8  framing; // Framing parameter value written by the client for this ASE
    u8  preferredPHY; //Formatted as a bitfield, the server may set multiple bits in the Preferred_PHY bitfield

    u32 PresentationDly; // Presentation delay parameter value being requested by the client for this ASE, Units: us
    u32 sduInterval; // SDU_Interval, Range: 0x0000FF~0x0FFFFF

    // CIG parameters decide use
    u32 prefMinPresentationDelay; //0: no preference
    u32 prefMaxPresentationDelay; //0: no preference
    u32 minPresentationDelay;
    u32 maxPresentationDelay;
    u32 cigSduInterval;

    // metadata parameter
    u32 metadataLen;
    u8 *pMetadata;

} blt_ascsc_ase_t;



typedef struct{
    gattc_sub_ccc_msg_t ntfInput;
    /* Characteristic value handle */
    u16 aseCtrlPntHdl; /* ASE Control Point */
    u16 sinkAseHdl[STACK_AUDIO_ASCS_ASE_SNK_NUM];
    u16 srcAseHdl[STACK_AUDIO_ASCS_ASE_SNK_NUM];

    /* ACL connection handle */
    u16 connHandle;

    /* ASE numbers */
    u16 aseCount;
    u16 aseFsmBusy;
    u8 sinkAseNum;
    u8 srcAseNum;
    u8 sinkAseIdx;
    u8 srcAseIdx;
    u16 reserved;
    /* Audio configurations concerned */
    u8 svrNums;
    u8 cisNums;
//  std_unicast_aud_cfg_enum audCfgIdx;

    blt_ascsc_ase_t *pSinkAse[STACK_AUDIO_ASCS_ASE_SNK_NUM];
    blt_ascsc_ase_t *pSrcAse[STACK_AUDIO_ASCS_ASE_SRC_NUM];

} blc_ascs_client_t;


typedef struct blt_ascs_client_ctrl {
    blc_prf_proc_t process;
    blc_ascs_client_t* pAscsClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
} blc_ascs_client_ctrl_t;


typedef struct{

} blc_ascsc_regParam_t;


typedef struct{
    u16 aclHandle;
    u16 cis_count;
    u16 cis_handle[3];
} blt_ascsc_cisBondingParam_t;




