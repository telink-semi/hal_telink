/********************************************************************************************************
 * @file    audio_internal.h
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


#define BLT_GTBS_PTS_BQB_EN   0 //PTS test used only, SDK release need close
#define BLT_GMCS_PTS_BQB_EN   0 //PTS test used only, SDK release need close

#define BLT_AUD_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_AUD_LOG, "[AUD]" fmt, ##__VA_ARGS__)

#define BLT_AUD_CHECK_NULL_PTR1(ptr1) \
    if (CHECK_PTR_NUL(ptr1))          \
    return AUDIO_ERR_INPUT_NULL_PTR
#define BLT_AUD_CHECK_NULL_PTR2(ptr1, ptr2)         \
    if (CHECK_PTR_NUL(ptr1) || CHECK_PTR_NUL(ptr2)) \
    return AUDIO_ERR_INPUT_NULL_PTR
#define BLT_AUD_CHECK_NULL_PTR3(ptr1, ptr2, ptr3)                          \
    if (CHECK_PTR_NUL(ptr1) || CHECK_PTR_NUL(ptr2) || CHECK_PTR_NUL(ptr3)) \
    return AUDIO_ERR_INPUT_NULL_PTR

#define BLT_AUD_CHECK_NULL_PTR(...)                           VARARG(BLT_AUD_CHECK_NULL_PTR, __VA_ARGS__)

#define BLC_COMMON_NV_STORE(connHandle, PRF, prf, attrHandle) BLC_PRF_NV_STORE(connHandle, AUDIO_##PRF##_CLIENT, prf, attrHandle)
#define BLC_COMMON_SDP_DISCOVERY(connHandle, PRF, prf)        BLC_PRF_SDP_DISCOVERY(connHandle, PRF, prf, AUDIO_##PRF##_CLIENT)

/* LE stack event callback for audio profile */
typedef void (*audio_le_evt_func_t)(u32 h, u8 *para, int n);

extern audio_le_evt_func_t bap_unicast_clt_cb;
extern audio_le_evt_func_t bap_unicast_svr_cb;
extern audio_le_evt_func_t bap_bcst_sink_cb;
extern audio_le_evt_func_t bap_bcst_assistant_cb;

typedef enum
{
    AUDIO_CLIENT = 0x00,
    AUDIO_SERVER = 0x01,
} audio_role_enum;

typedef struct
{
    u16                  fieldExistFlg;
    blc_audio_codec_id_t codecId;   // Add this to mark the Codec
    u16                  frequency; // Sampling_Frequencies, Bitfield

    u8 counts;                      // Audio_Channel_Counts, Bitfield
    u8 duration;                    // Frame_Durations, Bitfield
    u8 maxCodecFramesPerSDU;        // Max_Codec_Frames_Per_SDU

    u16 minOctets;                  // Min Octets_Per_Codec_Frame
    u16 maxOctets;                  // Max Octets_Per_Codec_Frame
} blt_audio_codecSpecCapParam_t;

typedef struct
{
    blt_audio_codecSpecCapParam_t codecSpecCapParam;
    blc_audio_metadata_parsed_t   metadataParam;
} blt_audio_pacParam_t;

typedef struct
{
    bool kmaMark;
    u8   rsvd;
    u16  rsvd1;

    u8 aclIdx1;
    u8 aclIdx2;
    u8 aclIdx3;
    u8 aclIdx4;
} blt_audio_cap_ctrl_t;

extern blt_audio_cap_ctrl_t blt_audio_cap_ctrl;


u16 blt_audio_getFrameOctsBySampleAndDuration(u8 audioSampleIdx, u8 durationType);

int blt_audio_getMetadataParams(u8 metaLen, u8 *pMeta, blc_audio_metadata_parsed_t *pParam);
int blt_audio_getCodecSpecCapParam(u8 *pSpecCap, blt_audio_codecSpecCapParam_t *pParam);
int blt_audio_getCodecSpecCfgParam(u8 *pSpecCfg, blc_audio_codecSpecCfgParsed_t *pParam);
int blt_audio_checkCodecParamValid(blt_audio_codecSpecCapParam_t *codecCap, blc_audio_codecSpecCfgParsed_t *codecCfg);

u8 blt_audio_setMetadata(blc_audio_metadata_parsed_t *pParam, u8 *pMeta);

int blt_audio_getAclHdlByCisHdl(u16 cisHandle);
