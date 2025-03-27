/********************************************************************************************************
 * @file    vocs_internal.h
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


#define BLT_VOCS_LOG(fmt, ...)          BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_VOCS_LOG, "[VOCS]"fmt, ##__VA_ARGS__)

typedef enum{
    MIN_VOLUME_OFFSET = -255,
    MAX_VOLUME_OFFSET = 255,
}blt_vocs_volume_offset_val_range_enum;

typedef enum{
    VOCS_ERRCODE_INVALID_CHANGE_COUNTER     =   0x80,
    VOCS_ERRCODE_OPCODE_NOT_SUPPORTED       =   0x81,
    VOCS_ERRCODE_VALUE_OUT_OF_RANGE         =   0x82,
}blt_vocs_error_code_enum;

typedef enum  {
    VOCS_READ_VOL_OFFSET_STATE,
    VOCS_READ_AUDIO_LOC,
    VOCS_READ_OUT_DESC,
    VOCS_READ_MAX,
} blt_vocs_read_enum;


/* Volume Offset Control Point */
typedef struct  {
    u8 opcode;
    u8 changeCnt;
} blt_vocs_vol_offset_cp_t;

typedef struct  {
    blt_vocs_vol_offset_cp_t cp;
    s16 volSetting;
} blt_vocs_set_volume_offset_op_t;

/*
 * VOCS: ATT handle information: 7byte
 * VOCS service entity supports a maximum of 8: refer to STACK_AUDIO_VOCS_CLIENT_MAX_INSTANCE_NUM
 */
typedef struct{
    u16 baseHandle;
    u8 endHdl;
    u8 volOffStateHdl; //NTF
    u8 audioLocationHdl; //NTF
    u8 volOffCtrlpntHdl;
    u8 audioOutDescHdl; //NTF
} blc_vocs_att_hdl_t;

extern const int gAppVocsSvrInstNum;

int blt_vocsc_init(u8 initType);


blc_vocs_client_t *blt_vocsc_getClientBuf(void);
void blt_vocsc_cleanBuf(void);

int blt_vocsc_dataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
blc_vocs_client_t *blt_vocsc_getFreeClientInst(u16 connHandle);

void blt_vocsc_store(u16 connHandle, prf_nv_param_t* param);
void blt_vocsc_load(u16 connHandle, prf_nv_param_t* param);

void blt_vocss_initParam(blc_vocs_server_t* vocs, void* param);

void blt_vocsc_foundServiceEnd(u16 connHandle);
