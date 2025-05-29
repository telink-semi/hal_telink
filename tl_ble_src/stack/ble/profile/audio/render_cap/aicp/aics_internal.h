/********************************************************************************************************
 * @file    aics_internal.h
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


#define BLT_AICS_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_AICS_LOG, "[AICS]" fmt, ##__VA_ARGS__)

typedef enum
{
    AICS_ERRCODE_INVALID_CHANGE_COUNTER       = 0x80,
    AICS_ERRCODE_OPCODE_NOT_SUPPORTED         = 0x81,
    AICS_ERRCODE_MUTE_DISABLE                 = 0x82,
    AICS_ERRCODE_VALUE_OUT_OF_RANGE           = 0x83,
    AICS_ERRCODE_GAIN_MODE_CHANGE_NOT_ALLOWED = 0x84,
} blt_aics_error_code_enum;

typedef enum
{
    AICS_READ_AUDIO_INPUT_STATE,
    AICS_READ_GAIN_SETTING_PROPERTIES,
    AICS_READ_AUDIO_INPUT_TYPE,
    AICS_READ_AUDIO_INPUT_STATUS,
    AICS_READ_AUDIO_INPUT_DESCRIPTION,
    AICS_READ_MAX,
} blt_aics_read_enum;

typedef enum
{
    AICS_OPCODE_SET_GAIN_SETTING        = 0x01,
    AICS_OPCODE_NOT_MUTED               = 0x02,
    AICS_OPCODE_MUTE                    = 0x03,
    AICS_OPCODE_SET_MANUAL_GAIN_MODE    = 0x04,
    AICS_OPCODE_SET_AUTOMATIC_GAIN_MODE = 0x05,
    AICS_OPCODE_MAX,
} blt_aics_audio_intput_control_opcode_enum;

/* Audio Input Control Point */
typedef struct
{
    u8 opcode;
    u8 changeCnt;
} blt_aics_input_cp_t;

typedef struct
{
    blt_aics_input_cp_t cp;
    s8                  gainSetting;
} blt_aics_set_gain_setting_op_t;

/*
 * AICS: ATT handle information: 9byte
 * AICS service entity supports a maximum of 8: refer to STACK_AUDIO_AICS_CLIENT_MAX_INSTANCE_NUM
 */
typedef struct
{
    u16 baseHandle;
    u8  endHdl;
    u8  audioInStateHdl;  //NTF
    u8  gainSettingPropertiesHdl;
    u8  audioInTypeHdl;
    u8  audioInStatusHdl; //NTF
    u8  audioInCtrlHdl;
    u8  audioInDescHdl;   //NTF
} blc_aics_att_hdl_t;

extern const int         gAppAicsCltInstNum;
extern blc_aics_client_t gAicsClient[];

extern const int         gAppVcsSvrInstNum;
extern blc_aics_server_t aics_server[];

int  blt_aicsc_init(u8 initType);
void blt_aicsc_vcpFoundServiceEnd(u16 connHandle);
void blt_aicsc_micpFoundServiceEnd(u16 connHandle);
int  blt_aicsc_vcpDataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);
int  blt_aicsc_micpDataInput(u16 connHandle, u16 attHdl, u8 *val, u16 valLen);

void blt_aicsc_micpLoad(u16 connHandle, prf_nv_param_t *param);
void blt_aicsc_micpStore(u16 connHandle, prf_nv_param_t *param);
void blt_aicsc_vcpLoad(u16 connHandle, prf_nv_param_t *param);
void blt_aicsc_vcpStore(u16 connHandle, prf_nv_param_t *param);

blc_aics_client_t *blt_aicsc_getClientControlBuffer(u16 connHandle, u16 startHandle, u16 endHandle);
void               blt_aicsc_cleanAllClientControlBuffer(void);
blc_aics_server_t *blt_aicss_getServerControlBuffer(u8 instIdx);

int  blt_aicss_writeCback(u16 connHandle, u16 attrHandle, u8 *writeValue, u16 valueLen);
void blt_aicss_initParam(blc_aics_server_t *aics, void *param);
