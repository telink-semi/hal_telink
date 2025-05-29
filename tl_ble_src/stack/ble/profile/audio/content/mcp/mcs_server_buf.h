/********************************************************************************************************
 * @file    mcs_server_buf.h
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

typedef struct
{
    u16  connHandle;
    bool active                 : 1;
    bool mediaPlayerNameChanged : 1;
    bool trackTitleChanged      : 1;
} mcs_value_changed_conn_t;

typedef struct
{
    u16                      mediaPlayerNameHandle;
    u16                      mediaPlayerIconObjectIDHandle;
    u16                      mediaPlayerIconURLHandle;
    u16                      trackChangedHandle;
    u16                      trackTitleHandle;
    u16                      trackDurationHandle;
    u16                      trackPositionHandle;
    u16                      playbackSpeedHandle;
    u16                      seekingSpeedHandle;
    u16                      currentTrackSegmentsObjectIDHandle;
    u16                      currentTrackObjectIDHandle;
    u16                      nextTrackObjectIDHandle;
    u16                      parentGroupObjectIDHandle;
    u16                      currentGroupObjectIDHandle;
    u16                      playingOrderHandle;
    u16                      playingOrdersSupportedHandle;
    u16                      mediaStateHandle;
    u16                      mediaControlPointHandle;
    u16                      mediaControlPointOpcodesSupportedHandle;
    u16                      searchControlPointHandle;
    u16                      searchResultsObjectIDHandle;
    u16                      contentControlIDHandle;
    mcs_value_changed_conn_t valueChanged[STACK_PRF_ACL_CONN_MAX_NUM];
} blc_mcs_server_t, blc_gmcs_server_t;

typedef struct
{
    blc_gmcs_server_t gmcs;
    //TODO: not supported mcs
    u8                mcsServerCount;
    u8                reserved[3];
    blc_mcs_server_t *mcs[0];

} blc_mcp_server_t;

typedef struct blc_mcp_server_ctrl
{
    blc_prf_proc_t   process;
    blc_mcp_server_t mcpServer;
} blc_mcp_server_ctrl_t;

typedef struct
{
    u8             *mediaPlayerName;
    u16             mediaPlayerNameLen;
    bool            mediaPlayerIconObjectIdPresent      : 1;
    bool            currentTrackSegmentsObjectIdPresent : 1;
    bool            currentTrackObjectIdPresent         : 1;
    bool            nextTrackObjectIdPresent            : 1;
    bool            parentGroupObjectIdPresent          : 1;
    bool            currentGroupObjectIdPresent         : 1;
    blc_object_id_t mediaPlayerIconObjectId;
    blc_object_id_t currentTrackSegmentsObjectId;
    blc_object_id_t currentTrackObjectId;
    blc_object_id_t nextTrackObjectId;
    blc_object_id_t parentGroupObjectId;
    blc_object_id_t currentGroupObjectId;
    u8             *mediaPlayerIconUrl;
    u16             mediaPlayerIconUrlLen;
    u8             *trackTitle;
    u16             trackTitleLen;
    s32             trackDuration;
    s32             trackPosition;
    u8              mediaState;
    u8              CCID;
    u32             mediaControlPointOpcodesSupported;
    s8              playbackSpeed;
    s8              seekingSpeed;
    u16             playingOrdersSupported;
    u8              playingOrder;
} blc_gmcss_regParam_t, blc_mcss_regParam_t;

typedef struct
{
    blc_gmcss_regParam_t gmcsParam;
} blc_mcps_regParam_t;
