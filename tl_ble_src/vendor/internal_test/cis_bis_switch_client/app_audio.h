/********************************************************************************************************
 * @file    app_audio.h
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
#ifndef APP_AUDIO_H_
#define APP_AUDIO_H_
#include "app_config.h"


#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_CLIENT)

#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
#define APP_AUDIO_CONFIGURATION_PREFER           BLC_AUDIO_11II_SVR_2_SINK_2_CHN_1_SRC_2_CHN_1_CISES_2_STREAMS_4
#define APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER   BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES
#define APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER  BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
#define APP_AUDIO_CONFIGURATION_PREFER           BLC_AUDIO_5_SVR_1_SINK_1_CHN_2_SRC_1_CHN_1_CISES_1_STREAMS_2
#define APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER   BLC_AUDIO_STD_FREQ_48K_DURATION_10MS_FRAME_100BYTES
#define APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER  BLC_AUDIO_STD_FREQ_16K_DURATION_10MS_FRAME_40BYTES
#endif

#define APP_AUDIO_QOS_INPUT_PARAMETER_PREFER     BLC_AUDIO_STD_QOS_HIGH_RELIABILITY
#define APP_AUDIO_QOS_OUTPUT_PARAMETER_PREFER    BLC_AUDIO_STD_QOS_HIGH_RELIABILITY


/**
 *  @brief  codec(lc3)process enum,such as configure,release.
 */
typedef enum{
    APP_CODEC_IDLE,
    APP_CONFIG_CODEC,
    APP_RELEASE_CODEC,
}app_audio_codec_e;

/**
 *  @brief  app audio parameter
 */
typedef struct __attribute__((packed)) {
    bool  sS;//stream start
    u32   sT;//stream tick
    u32   pD;//presentation delay
    u8    blocks;
    u16   rsvd;
    app_audio_codec_e codecOp;
    blc_audio_std_codec_settings_enum codecParam;
}app_audio_param_t;

/**
 *  @brief  app acl parameter
 */
typedef struct __attribute__((packed)) {
    u16 acl_handle;
    u16 rsvd;
    app_audio_param_t sink;
    app_audio_param_t source;
}app_acl_param_t;

/**
 *  @brief  app control parameter
 */
typedef struct __attribute__((packed)) {
    u8    acl_max_num;    //support max acl number
    u8    acl_count;      //current acl number
    bool  acl_csis_exist;
    u8    acl_csis_size;
    u8    acl_csis_sirk[16];
    app_acl_param_t aclParam[ACL_CENTRAL_MAX_NUM];
}app_ctrl_t;

/**
 *  @brief  app audio event callback parameter
 */
typedef struct __attribute__((packed)) {
    audio_event_enum id;
    int (*evtCb)(u16 connHandle, u8 *pAudEvt, u16 dataLen);
} app_audio_evtCb_t;

/**
 * @brief      Audio module init.
 * @param[in]  none.
 * @return     none.
 */
void  app_audio_init(void);

/**
 * @brief      Audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void);
s8   app_audio_getHandleIndex(u16 connHandle);


/************************************** BIS Assistant Start ****************************************/

#define MAX_LINK_NUM                            1
#define MAX_SOURCE_INFO_NUM                     10
#define MAX_SINK_INFO_NUM                       5

#define CONNECT_SINK_NO_HAD_SOURCE              1
#define CONNECT_SINK_HAD_SOURCE                 2

typedef enum{
    SINK_STATE_DISCONNECT = 0,
    SINK_STATE_CONNECT,
    SINK_STATE_ADD_SOURCE,
    SINK_STATE_HAD_SOURCE,
    SINK_STATE_MODIFY_SOURCE,
    SINK_STATE_REMOVE_SOURCE,
    SINK_STATE_BAD_CODE,
    SINK_STATE_NO_SOURCE,
} sink_state_enum;

typedef struct __attribute__((packed)) {
    u8 isUsed;
    u16 connHandle;
    u8 addrType;
    u8 address[6];
    struct __attribute__((packed))  {
        bool PACS_server    :1;
        bool BASS_server    :1;
        bool VCS_server     :1;
        bool Vocs_server    :1;

        bool sdp_over       :1;
    };
    u8 sinkState;   //Sync BIS or no Sync BIS, sink_state_enum
    u8 remoteSourceId;
    int sourceIndex;
    int bisSync;
    u8 pastFlag;
    char broadcastCode[17];
    char deviceName[50];
} connect_info_t;

typedef struct __attribute__((packed))  {
    u8 connCnt;
    connect_info_t conn[MAX_LINK_NUM];
} app_connect_info_t;

typedef struct __attribute__((packed)) {
    u8 addrType;
    u8 address[6];
    char deviceName[50];
} sink_info_t;

typedef struct __attribute__((packed)) {
    u8  advAddrType;
    u8  advAddr[6];
    u8  advSID;
    u8 broadcastId[3];

    u8 usedFlag;
    u8 enc;

    u8 bisCnt;
    u8 bisIndex[2];
    bisSyncInfo_t bisInfo[2];

    char completeName[50];
    char broadcastName[50];

}source_info_t;

typedef struct __attribute__((packed))  {
    u8 completeNameLen;
    char completeName[50];
    u8 broadcastNameLen;
    char broadcastName[50];

    blc_audio_source_head_t head;
}SourceInfo_t;


/******************************************************************************/
/* Broadcast Assistant                                                        */
/******************************************************************************/

/**
 * @brief       assistant initial/clean source information buffer.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_initSourceInfoBuf(void);

/**
 * @brief       assistant get source information by index.
 * @param[in]   index: source information index.
 * @return      source information pointer/NULL.
 */
source_info_t *app_audio_getSourceInfo(u8 index);

/**
 * @brief       app display all source information to uart/usb-cdc.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_showSourceInfo(void);

/******************************************************************************/
/* ACL connection  handle                                                     */
/******************************************************************************/

connect_info_t * app_audio_getConn(u16 connHandle);

/**
 * @brief       check acl connect count full.
 * @param[in]   none.
 * @return      true:ACL connect full.
 */
bool app_audio_aclConnFull(void);

/**
 * @brief       get connHandle by index.
 * @param[in]   index: ACL connect index.
 * @return      connHandle.
 */
u16 app_audio_getConnHandle(u8 index);

/**
 * @brief       app display all connect information to uart/usb-cdc.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_showConnInfo(void);

/**
 * @brief       assistant initial/clean connect information buffer.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_initSinkInfoBuf(void);

/**
 * @brief       open scan sink switch.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_openScanSink(void);

/**
 * @brief       close scan sink switch.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_closeScanSink(void);

/**
 * @brief       app display all scan sink information to uart/usb-cdc.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_showSinkInfo(void);

/**
 * @brief       create sink ACL connect by index.
 * @param[in]   index: scan sink index.
 * @return      0: index error.
 *              1: index true.
 */
int app_audio_createSinkConn(int index);

/******************************************************************************/
/* Broadcast Assistant profile event callback                                 */
/******************************************************************************/


/************************************** BIS Assistant End ****************************************/

#endif

#endif
