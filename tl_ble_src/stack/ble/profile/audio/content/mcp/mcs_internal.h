/********************************************************************************************************
 * @file    mcs_internal.h
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

#define BLT_MCS_LOG(fmt, ...) BLC_AUDIO_PRF_DBG(DBG_PRF_MASK_MCS_LOG, "[(G)MCS]" fmt, ##__VA_ARGS__)

/* Application error codes(MCS/GMCS) */
enum
{
    GMCS_ERRCODE_VALUE_CHANGED_DURING_READ_LONG = 0x80,
};

/*
 * GMCS: ATT handle information: 25byte
 */
typedef struct
{
    u16 baseHandle;
    u8  endHdl;
    u8  mediaPlayerNameHdl;
    u8  mediaPlayerIconObjectIDHdl;
    u8  mediaPlayerIconURLHdl;
    u8  trackChangedHdl;            //NTF
    u8  trackTitleHdl;              //NTF
    u8  trackDurationHdl;           //NTF
    u8  trackPositionHdl;           //NTF
    u8  playbackSpeedHdl;           //NTF
    u8  seekingSpeedHdl;            //NTF
    u8  currentTrackSegObjectIDHdl;
    u8  currentTrackObjectIDHdl;    //NTF
    u8  nextTrackObjectIDHdl;       //NTF
    u8  parentGroupObjectIDHdl;     //NTF
    u8  currentGroupObjectIDHd;     //NTF
    u8  playingOrderHdl;            //NTF
    u8  playingOrdersSupportedHdl;
    u8  mediaStateHdl;              //NTF
    u8  mediaControlPointHdl;       //NTF
    u8  mediaControlPointOpSuppHdl; //NTF
    u8  searchControlPointHdl;      //NTF
    u8  searchResultsObjectIDHdl;   //NTF
    u8  contentControlIDHdl;
} blt_gmcs_att_hdl_t;

typedef struct
{
    blt_gmcs_att_hdl_t att;
} blt_gmcs_nv_info_t;

int blt_mcp_nv_store(u16 connHandle, prf_nv_state_enum nvState, prf_nv_param_t *param);


int blt_mcp_init(u8 initType, const void *param);
int blt_mcp_connect(u16 connHandle, prf_acl_state_enum connState);
int blt_mcp_discovery(u16 connHandle);

blc_mcp_client_t *blt_mcp_getClientcontrolBuffer(u8 instIdx);

int blt_mcss_init(u8 initType, const void *param);
