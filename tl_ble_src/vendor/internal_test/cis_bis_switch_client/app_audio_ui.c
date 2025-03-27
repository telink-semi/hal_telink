/********************************************************************************************************
 * @file    app_audio_ui.c
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
#include "../intest_config.h"

#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_CLIENT)

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_parse_char.h"
#include "app_audio_ui.h"
#include "app_audio.h"
#include "tlk_api/tlk_mem.h"

u16 scanSourceHandle = 0x00;
u8 scanSourceIndex = 0x00;

extern const char locationStr[32][30];
extern bool muteFlag;
u32 cis_connection_tick = 0;
s8  cig_creat_flag = 0;
extern app_ctrl_t appCtrl;
static void app_audio_ui_conn(char *argv[], int argc, void *user_data)
{
    u16 aclHandle = appCtrl.aclParam[0].acl_handle;

    s8 acl_index = app_audio_getHandleIndex(aclHandle);
    if(acl_index<0)
    {
        tlkapi_printf(APP_LOG_EN,"error-get acl handle:0x%x\n", aclHandle);
        return;
    }

    if(cig_creat_flag)
    {
        app_parse_printf("create cis fail: cis already exists");
        return ;
    }
    else
    {
        tlk_mem_pool_desc_t poolDesc[] =
        {
            { 340,  8},//receive data use,16k,1channel,10ms,320 byte each frame.
        };
        const u8 numPools = sizeof(poolDesc) / sizeof(poolDesc[0]);
        //blocks malloc
        if(tlk_mempool_init(numPools, poolDesc)!=0)
        {
            tlkapi_printf(APP_LOG_EN,"error-mempool init failed!");
        }


        blc_audio_ase_cfg_info_t audChnInfo;
        int audioRet = blc_bapuc_checkAudioConfigures(aclHandle, APP_AUDIO_CONFIGURATION_PREFER, &audChnInfo);
        if(audioRet!= AUDIO_ESUCC)
        {
            tlkapi_printf(APP_LOG_EN,"error-audio configurations:0x%x\n", audioRet);
            return;
        }

        ////////////////////// SVR: SINK; CLT: SOURCE //////////////////////////////////////
        for(int i = 0; i<audChnInfo.sinkASEsPerSvr; i++)
        {
            audioRet = blc_bapuc_setAseConfigCodec(aclHandle,audChnInfo.sinkASEId[i], APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER, &audChnInfo);
            if(audioRet!= AUDIO_ESUCC)
            {
                tlkapi_printf(APP_LOG_EN,"error-unicast config audio:0x%x\n", audioRet);
            }
            else
            {
                appCtrl.aclParam[acl_index].source.codecParam = APP_AUDIO_CODEC_INPUT_PARAMETER_PREFER;
                appCtrl.aclParam[acl_index].source.blocks = audChnInfo.sinkCodecFrameBlksPerSDU;
                appCtrl.aclParam[acl_index].source.codecOp = APP_CONFIG_CODEC;
                tlkapi_printf(APP_LOG_EN,"source config codec-blocks %d\n",appCtrl.aclParam[acl_index].source.blocks);
            }
        }

        ////////////////////// SVR: SOURCE;  CLT: SINK //////////////////////////////////////
        for(int i = 0; i<audChnInfo.srcASEsPerSvr; i++)
        {
            audioRet = blc_bapuc_setAseConfigCodec(aclHandle, audChnInfo.srcASEId[i], APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER, &audChnInfo);
            if(audioRet!= AUDIO_ESUCC)
            {
                tlkapi_printf(APP_LOG_EN,"error-unicast config audio:0x%x\n", audioRet);
            }
            else
            {
                appCtrl.aclParam[acl_index].sink.codecParam = APP_AUDIO_CODEC_OUTPUT_PARAMETER_PREFER;
                appCtrl.aclParam[acl_index].sink.blocks = audChnInfo.srcCodecFrameBlksPerSDU;
                appCtrl.aclParam[acl_index].sink.codecOp = APP_CONFIG_CODEC;
                tlkapi_printf(APP_LOG_EN,"sink config codec-blocks %d\n",appCtrl.aclParam[acl_index].sink.blocks);
            }
        }
        cig_creat_flag = 1;
        app_parse_printf("cig connection\r\n");

        cis_connection_tick = clock_time()|1;
    }

}

static void app_audio_ui_disconn(char *argv[], int argc, void *user_data)
{
    if(!cig_creat_flag)
    {
        app_parse_printf("cis already disconnect");
        return ;
    }
    else
    {
        u16 aclHandle = appCtrl.aclParam[0].acl_handle;
        blc_ascs_client_t *pAscsClt = blt_ascsc_getClientInst(aclHandle);
        if (pAscsClt == NULL) {
            app_parse_printf("Invalid ACL handle: 0x%x", aclHandle);
            return;
        }

        blt_ascsc_ase_t *pAse = pAscsClt->pSrcAse[0];

        audio_error_enum errorCode;
        errorCode = blc_bapuc_setAseRelease(aclHandle, pAse->aseID);
        if(errorCode != AUDIO_ESUCC)
        {
            app_parse_printf("Ase release error: %x\r\n", errorCode);
        }
        cig_creat_flag = 0;
        app_parse_printf("cig connection\r\n");
    }
}

/**
 * @brief       Assistant UI scan sink device.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_scan_sink(char *argv[], int argc, void *user_data)
{
    if (argc == 0) {
        app_parse_printf("scan-sink <start|stop|clear>\r\n");
        return;
    }

    if(strcasecmp(argv[0], "start") == 0)
    {
        app_parse_printf("assistant start scan sink\r\n");
        app_audio_initSinkInfoBuf();
        app_audio_openScanSink();
    }
    else if(strcasecmp(argv[0], "stop") == 0)
    {
        app_parse_printf("assistant stop scan sink\r\n");
        app_audio_closeScanSink();
    }
    else if(strcasecmp(argv[0], "clear") == 0)
    {
        app_audio_initSinkInfoBuf();
        app_parse_printf("assistant clear sink info\r\n");
    }
    else
    {
        app_parse_printf("scan-sink not support [%s]", argv[0]);
        app_parse_printf("scan-sink <start|stop|clear>\r\n");
    }
}

/**
 * @brief       Assistant UI create connect to sink device.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_conn_sink(char *argv[], int argc, void *user_data)
{
    if(argc != 1)
    {
        app_parse_printf("conn-sink <dev_idx>\r\n");
        return ;
    }

    if(app_audio_aclConnFull())
    {
        app_parse_printf("connect sink fail: Connection already exists");
        return ;
    }

    if(app_audio_createSinkConn(app_parse_str2n(argv[0])))
    {
        app_parse_printf("assistant start connect sink\r\n");
    }
    else
    {
        app_parse_printf("connect sink index error\r\n");
        app_parse_printf("show sink to view sink index\r\n");
    }
}

/**
 * @brief       Assistant UI scan source for connected sink device.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_scan_source(char *argv[], int argc, void *user_data)
{
    if(argc != 2)
    {
        app_parse_printf("scan-bcast <start|stop|clear> <conn_idx>\r\n");
        return ;
    }
    int index = app_parse_str2n(argv[1]);

    u16 connHandle = app_audio_getConnHandle(index);

    if(!connHandle)
    {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(pConn->sdp_over == 0)
    {
        return app_parse_printf("wait sdp discovery ending\r\n");
    }

    if(pConn->BASS_server == 0)
    {
        return app_parse_printf("connect device not supported BASS Server \r\n");
    }

    if(pConn->PACS_server == 0)
    {
        return app_parse_printf("connect device not supported PACS Server \r\n");
    }

    if(strcasecmp(argv[0], "start") == 0)
    {
        app_audio_initSourceInfoBuf();
        scanSourceIndex = index;
        scanSourceHandle = connHandle;
        blc_bapba_writeRemoteScanStarted(connHandle);
        app_parse_printf("assistant start scan broadcast source\r\n");
        app_parse_printf("pConn sinkstate = %d\r\n", pConn->sinkState);
    }
    else if(strcasecmp(argv[0], "stop") == 0)
    {
        blc_bapba_writeRemoteScanStopped(connHandle);
        app_parse_printf("assistant stop scan broadcast source\r\n");
    }
    else if(strcasecmp(argv[0], "clear") == 0)
    {
        scanSourceIndex = 0;
        scanSourceHandle = 0;
        app_audio_initSourceInfoBuf();
        blc_bapba_writeRemoteScanStopped(connHandle);
        app_parse_printf("assistant clear broadcast source info\r\n");
    }
    else
    {
        app_parse_printf("scan-sink not support [%s]", argv[0]);
    }
}

/**
 * @brief       Assistant UI add source into sink device.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_add_source(char *argv[], int argc, void *user_data)
{
    if(argc < 3)
    {
        app_parse_printf("add-source <conn_idx> <source_idx> <bis_sync> [broadcast_key]\r\n");
        return ;
    }

    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));
    if(!connHandle)
    {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    int sourceIndex = app_parse_str2n(argv[1]);
    source_info_t* sourceInfo = app_audio_getSourceInfo(sourceIndex);
    if(!sourceInfo)
    {
        return app_parse_printf("index error.You can run the \"show source\" command to view source information. \r\n");
    }

    int bisSync = app_parse_str2n(argv[2]);

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(sourceInfo->enc)
    {
        if(argc != 4){
            return app_parse_printf("source is Encrypted, please enter broadcast code\r\n");
        }
        strncpy(pConn->broadcastCode, argv[3], 16);
    }

    int bisIndex = 0;
    for(int i=0; i<sourceInfo->bisCnt; i++)
    {
        bisIndex |= BIT(sourceInfo->bisIndex[i]-1);
    }

    if(bisSync & (~bisIndex))
    {
        return app_parse_printf("Bis sync is error, only set 0x%08x\r\n", bisIndex);
    }

    pConn->pastFlag = 0;
    app_parse_printf("pConn sinkstate = %d\r\n", pConn->sinkState);
    if(pConn->sinkState == SINK_STATE_NO_SOURCE) {
        pConn->sinkState = SINK_STATE_ADD_SOURCE;
        blc_bapba_writeAddSourceNoPast(connHandle, (blc_audio_source_head_t *)sourceInfo, bisSync);
    }
    else
    {
        if(pConn->sinkState == SINK_STATE_HAD_SOURCE) {
            pConn->sinkState = SINK_STATE_MODIFY_SOURCE;
            blc_bapba_writeModifySourceNotSyncPA(connHandle, pConn->remoteSourceId, 0);

            pConn->sourceIndex = sourceIndex;
            pConn->bisSync = bisSync;

        }
        else{
            app_parse_printf("please use clean-source command, reset sink state\r\n"); return ;
        }
    }

    app_parse_printf("started add source\r\n");

}

static void app_audio_ui_remvoe_source(char *argv[], int argc, void *user_data)
{
    if(argc != 2)
    {
        app_parse_printf("remove_source <dev_idx> <source_idx>\r\n");
        return ;
    }

    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));
    if(!connHandle)
    {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    int sourceIndex = app_parse_str2n(argv[1]);
    connect_info_t* pConn = app_audio_getConn(connHandle);
    app_parse_printf("pConn sinkstate = %d\r\n", pConn->sinkState);
    if((pConn->sinkState == SINK_STATE_HAD_SOURCE)||(pConn->sinkState == SINK_STATE_ADD_SOURCE))
    {
        pConn->sinkState = SINK_STATE_MODIFY_SOURCE;
        blc_bapba_writeModifySourceNotSyncPA(connHandle, pConn->remoteSourceId, 0);
        pConn->sourceIndex = sourceIndex;
    }
    else{
        app_parse_printf("please use clean-source command, reset sink state\r\n"); return ;
    }


    if(pConn->sinkState != SINK_STATE_MODIFY_SOURCE)
    {
        pConn->sinkState = SINK_STATE_REMOVE_SOURCE;
        blc_bapba_writeRemoveSource(connHandle, pConn->remoteSourceId);
        pConn->sinkState = SINK_STATE_NO_SOURCE;
        app_parse_printf("assistant remove broadcast source\r\n");
    }
    else
    {
        app_parse_printf("assistant remove broadcast source success\r\n");
    }

}

/**
 * @brief       Assistant UI send mute/unmute command sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_mute_sink(char *argv[], int argc, void *user_data)
{
    if(argc < 1)
    {
        return app_parse_printf("mute <conn_idx>\r\n");
    }
    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if(!connHandle)
    {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(pConn->VCS_server == 0)
    {
        return app_parse_printf("connect device not supported VCS Server \r\n");
    }

    muteFlag = !muteFlag;

    app_parse_printf("mute flag is %d", muteFlag);

    if(muteFlag){
        blc_vcsc_writeMute(connHandle);
    }
    else{
        blc_vcsc_writeUnmute(connHandle);
    }
}

/**
 * @brief       Assistant UI send volume up command to sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_vol_up(char *argv[], int argc, void *user_data)
{
    if(argc < 1)
    {
        return app_parse_printf("vol+ <conn_idx>\r\n");
    }
    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if(!connHandle)
    {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(pConn->VCS_server == 0)
    {
        return app_parse_printf("connect device not supported VCS Server \r\n");
    }

    blc_vcsc_writeUnmuteOrRelativeVolUp(connHandle);
}

/**
 * @brief       Assistant UI send volume down command to sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_vol_down(char *argv[], int argc, void *user_data)
{
    if(argc < 1)
    {
        return app_parse_printf("vol- <conn_idx>\r\n");
    }
    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if(!connHandle)
    {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(pConn->VCS_server == 0)
    {
        return app_parse_printf("connect device not supported VCS Server \r\n");
    }

    blc_vcsc_writeUnmuteOrRelativeVolDown(connHandle);
}

static const parse_fun_list_t cisUiControl[] = {
    {"cis-conn", app_audio_ui_conn},
    {"cis-disconn", app_audio_ui_disconn},
    {"scan-sink", app_audio_ui_scan_sink},
    {"conn-sink", app_audio_ui_conn_sink},
    {"scan-bcast", app_audio_ui_scan_source},
    {"add-source", app_audio_ui_add_source},
    {"remove-source", app_audio_ui_remvoe_source},
    {"mute", app_audio_ui_mute_sink},
    {"vol+", app_audio_ui_vol_up},
    {"vol-", app_audio_ui_vol_down}
};

static void app_audio_ui_unicast_state_changed(app_audio_unicast_state_enum state)
{
    switch (state) {
        case APP_AUDIO_UNICAST_CLIENT_STATE_CONN:
            app_parse_printf("unicast CIG connect\r\n");
            break;
        case APP_AUDIO_UNICAST_CLIENT_STATE_DISCONN:
            app_parse_printf("unicast CIG disconnect\r\n");
            break;
        default:
            app_parse_printf("unicast state unknown: %d\r\n", state);
            break;
    }
}

static unicast_state_changed_cb bcast_state_changed = NULL;

static void unicast_state_set(app_audio_unicast_state_enum state)
{
    CIG_status = state;
    if (bcast_state_changed) {
        bcast_state_changed(CIG_status);
    }
}

void unicast_set_state_changed_cb(unicast_state_changed_cb cb)
{
    bcast_state_changed = cb;
}

/**
 * @brief       broadcast assistant UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_ui_init(void)
{
    app_parse_init(cisUiControl, ARRAY_SIZE(cisUiControl));
    unicast_set_state_changed_cb(app_audio_ui_unicast_state_changed);
}

/**
 * @brief       broadcast assistant UI loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_ui_loop(void)
{
    app_parse_loop();
}

#endif  //SOURCE_VERSION == SOURCE_WITH_ASSISTANT


