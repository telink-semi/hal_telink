/********************************************************************************************************
 * @file    audio.h
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

/*
 * bluetooth LE Audio profile debug log.
 */
#define BLC_AUDIO_PRF_DBG(en, fmt, ...)     if(DBG_PRF_AUD_LOG){BLC_PROFILE_DEBUG(en, fmt, ##__VA_ARGS__);}

typedef enum{
    AUDIO_ESUCC = 0x00, /* Success */
    AUDIO_EFAIL,
    AUDIO_EMPTY,
    AUDIO_EBUSY,
    AUDIO_EUNREG, /* Unregister */
    AUDIO_EALIGN,
    AUDIO_EPARAM,
    AUDIO_EREPEAT,
    AUDIO_EHANDLE,
    AUDIO_EDIR,
    AUDIO_EROLE,
    AUDIO_EASEID,
    AUDIO_ESTATUS,
    AUDIO_ENOREADY,
    AUDIO_ELENGTH,
    AUDIO_ENOSUPP,
    AUDIO_EPUSH_SDU,
    AUDIO_ERR_PARAM_INVALID,
    AUDIO_ERR_NULL_POINTER,

    AUDIO_ERR_LTV_STRUCT_INVALID,
    AUDIO_ERR_RFU_SUPP_FREQ,
    AUDIO_ERR_RFU_SUPP_DURATIONS,
    AUDIO_ERR_RFU_CHANNEL_ALLOCATION,
    AUDIO_ERR_RFU_SUPP_CHANNEL_COUNTS,
    AUDIO_ERR_RFU_SUPP_PER_CODEC_FRAME,
    AUDIO_ERR_RFU_SUPP_MAX_CODEC_FRAME_PER_SDU,

    AUDIO_ERR_OPCODE_RFU,
    AUDIO_ERR_OPCODE_NOT_SUPP,
    AUDIO_ERR_NOT_MEDIA_CTRL_POINT_HANDLE,
    AUDIO_ERR_NOT_SEARCH_CTRL_POINT_HANDLE,
    AUDIO_ERR_PARAM_SIZE_ERR,

    AUDIO_ERR_CONNHANDLE_INVALID,
    AUDIO_ERR_GET_ATTR_HANDLE_NOT_FOUND,
    AUDIO_ERR_INPUT_NULL_PTR,
} audio_error_enum;


/**
 * profile client/server ID enumeration.
 */
typedef enum{
    AUDIO_CLIENT_START = PRF_LE_AUDIO_CLIENT_START-1,
    AUDIO_CSIS_CLIENT,      /* Coordinated Set Identification Service Client */
    AUDIO_PACS_CLIENT,      /* Published Audio Capabilities Service Client */
    AUDIO_ASCS_CLIENT,      /* Audio Stream Control Service Client */
    AUDIO_BASS_CLIENT,      /* Broadcast Audio Scan Service Client */
    AUDIO_GMCS_CLIENT,      /* Generic Media Control Service Client */
    AUDIO_GTBS_CLIENT,      /* Generic Telephone Bearer Service Client */
    AUDIO_VCP_CLIENT,       /* Volume Controller Profile Client, include VOCS+AISC*/
    AUDIO_MICS_CLIENT,      /* Microphone Control Service Client include AICS*/
    AUDIO_TMAS_CLIENT,      /* Telephony And Media Audio Service Client */
    AUDIO_HAS_CLIENT,       /* Hearing Access Service Client */

    AUDIO_SERVER_START = AUDIO_CLIENT_START + PRF_SERVER_OFFSET,
    AUDIO_CSIS_SERVER,      /* Coordinated Set Identification Service Server */
    AUDIO_PACS_SERVER,      /* Published Audio Capabilities Service Server */
    AUDIO_ASCS_SERVER,      /* Audio Stream Control Service Server */
    AUDIO_BASS_SERVER,      /* Broadcast Audio Scan Service Server */
    AUDIO_GMCS_SERVER,      /* Generic Media Control Service Server */
    AUDIO_GTBS_SERVER,      /* Generic Telephone Bearer Service Server */
    AUDIO_VCP_SERVER,       /* Volume Controller Profile Server, include VOCS+AISC*/
    AUDIO_MICS_SERVER,      /* Microphone Control Service Server include AICS*/
    AUDIO_TMAS_SERVER,      /* Telephony And Media Audio Service Server */
    AUDIO_HAS_SERVER,       /* Hearing Access Service Server */
    AUDIO_SERVER_MAX,
} le_audio_service_role_enum;

typedef enum{
    /*********** Event for Client *************/
    AUDIO_EVT_TYPE_CLIENT_START = PRF_EVTID_LE_AUDIO_START, //refer to each XXXP modules XXXS.h
    AUDIO_EVT_TYPE_CSISC = AUDIO_EVT_TYPE_CLIENT_START,
    AUDIO_EVT_TYPE_PACSC = AUDIO_EVT_TYPE_CSISC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_ASCSC = AUDIO_EVT_TYPE_PACSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_BASSC = AUDIO_EVT_TYPE_ASCSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_BAPUC = AUDIO_EVT_TYPE_BASSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_BAPBA = AUDIO_EVT_TYPE_BAPUC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_GMCSC = AUDIO_EVT_TYPE_BAPBA + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_GTBSC = AUDIO_EVT_TYPE_GMCSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_VCSC  = AUDIO_EVT_TYPE_GTBSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_VOCSC = AUDIO_EVT_TYPE_VCSC  + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_AICSC = AUDIO_EVT_TYPE_VOCSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_MICSC = AUDIO_EVT_TYPE_AICSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_TMASC = AUDIO_EVT_TYPE_MICSC + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_HASC  = AUDIO_EVT_TYPE_TMASC + PRF_EVENT_ID_SIZE,

    /*********** Event for Server *************/
    AUDIO_EVT_TYPE_SERVER_START = PRF_EVTID_LE_AUDIO_START + PRF_EVENT_ID_SIZE*PRF_SERVER_OFFSET, //refer to each XXXP modules XXXS.h
    AUDIO_EVT_TYPE_CSISS = AUDIO_EVT_TYPE_SERVER_START,
    AUDIO_EVT_TYPE_PACSS = AUDIO_EVT_TYPE_CSISS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_ASCSS = AUDIO_EVT_TYPE_PACSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_BASSS = AUDIO_EVT_TYPE_ASCSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_BAPUS = AUDIO_EVT_TYPE_BASSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_BAPBS = AUDIO_EVT_TYPE_BAPUS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_GMCSS = AUDIO_EVT_TYPE_BAPBS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_GTBSS = AUDIO_EVT_TYPE_GMCSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_VCSS  = AUDIO_EVT_TYPE_GTBSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_VOCSS = AUDIO_EVT_TYPE_VCSS  + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_AICSS = AUDIO_EVT_TYPE_VOCSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_MICSS = AUDIO_EVT_TYPE_AICSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_TMASS = AUDIO_EVT_TYPE_MICSS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_TYPE_HASS  = AUDIO_EVT_TYPE_TMASS + PRF_EVENT_ID_SIZE,

    /*********** Event for controller *************/
    AUDIO_EVT_TYPE_CONTROLLER_START = AUDIO_EVT_TYPE_HASS + PRF_EVENT_ID_SIZE,
    AUDIO_EVT_CIS_CONNECT,          //refer to 'blc_audio_cisConnEvt_t'
    AUDIO_EVT_CIS_DISCONNECT,       //refer to 'blc_audio_cisDisconnEvt_t'
    AUDIO_EVT_CIS_REQUEST,          //refer to 'blc_audio_cisReqEvt_t'


} audio_event_enum;



/**
 *  @brief  Event Parameters for "AUDIO_EVT_CIS_CONNECT"
 */
typedef struct{
    u16 cisHandle;
    u8  cigSyncDly[3];
    u8  cisSyncDly[3];
    u8  transLaty_m2s[3];
    u8  transLaty_s2m[3];
    u8  phy_m2s; //le_phy_type_t: 0x01/0x02/0x03
    u8  phy_s2m; //le_phy_type_t: 0x01/0x02/0x03
    u8  nse;
    u8  bn_m2s;
    u8  bn_s2m;
    u8  ft_m2s;
    u8  ft_s2m;
    u16 maxPDU_m2s;
    u16 maxPDU_s2m;
    u16 isoIntvl;
} blc_audio_cisConnEvt_t;

/**
 *  @brief  Event Parameters for "PRF_EVTID_ACL_DISCONNECT"
 */
typedef struct{
    u16 cisHandle;
    u8  reason;
} blc_audio_cisDisconnEvt_t;

/**
 *  @brief  Event Parameters for "AUDIO_EVT_CIS_REQUEST"
 */
typedef struct{
    u16 aclHandle;
    u16 cisHandle;
    u8  cigId;
    u8  cisId;
} blc_audio_cisReqEvt_t;






/////////////////////////////////////only for add new profile used///////////////////////////////////////////////




void blc_audio_initialModule(prf_evt_cb_t evtCb);



//used for KMA dongle
void blc_audio_setAclCentralIndexForCIS(u8 aclIdx1,u8 aclIdx2,u8 aclIdx3,u8 aclIdx4);


