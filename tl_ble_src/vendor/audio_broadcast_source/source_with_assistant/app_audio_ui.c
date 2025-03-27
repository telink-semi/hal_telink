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
#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_parse_char.h"
#include "app_assistant.h"
#include "app_audio.h"


extern u8  mac_public[6];
extern app_auracastCfgParam_t auracastCfg;


/**
 * @brief       Assistant UI show information(scan sink device/connected sink device/scan source/sink vcp).
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_show_info(char *argv[], int argc, void *user_data)
{
    if(strcasecmp(argv[0], "conn") == 0)
    {
        app_parse_printf("connect sink info\r\n");
        app_audio_showConnInfo();
    }
    else if(strcasecmp(argv[0], "sink") == 0)
    {
        app_parse_printf("scan sink info\r\n");
        app_audio_showSinkInfo();
    }
    else if(strcasecmp(argv[0], "vcp") == 0)
    {
        int index = app_parse_str2n(argv[1]);

        u16 connHandle = app_audio_getConnHandle(index);

        if(!connHandle)
        {
            return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
        }

        blc_audio_vcpState_t vcpState;

        audio_error_enum state = blc_vcpc_getState(connHandle, &vcpState);

        if(state != AUDIO_ESUCC)
            return app_parse_printf("get vcp state error\r\n");

        app_parse_printf("Remote Volume Control State, connHandle: 0x%02x\r\n", connHandle);

        app_parse_printf("volume(min:0, max:255) is %d, muteSate:%s\r\n", vcpState.volState.volumeSetting, vcpState.volState.mute?"Mute":"Unmute");
        if(vcpState.vosCnt)
        {
            for(int i=0; i<vcpState.vosCnt; i++)
            {
                app_parse_printf("VOCS index is %d \r\n", i);
                blc_audio_volumeOffsetState_t* vocs = &vcpState.voc[i];
                app_parse_printf("Location:");
                for(int j=0; j<32; j++)
                {
                    if(vocs->location & BIT(j))
                    {
                        app_parse_printf("%s ", &locationStr[j][0]);
                    }
                }
                app_parse_printf("\r\nAudio Description is %.*s\r\n", vocs->outDescLen, vocs->outDesc);
                app_parse_printf("Volume Offset(min:-255, max:255) is %d\r\n", vocs->volumeOffset);
            }
        }
        else
        {
            app_parse_printf("Not Had VOCS\r\n");
        }
    }
    else if(strcasecmp(argv[0], "param") == 0)
    {
        app_parse_printf("Source Information.\r\n");
        app_parse_printf("the Address is %s\r\n", addr_to_str(mac_public));
        app_parse_printf("complete name is %s.\r\n", DEFAULT_DEV_NAME);
        app_parse_printf("broadcast ID is 0x%02x%02x%02x.\r\n", auracastCfg.broadcastID[2], auracastCfg.broadcastID[1], auracastCfg.broadcastID[0]);
        app_parse_printf("broadcast name is %.*s.\r\n", auracastCfg.broadcastNameLen, auracastCfg.broadcastName);
        app_parse_printf("BIG is %s.\r\n", auracastCfg.encryptionFlag? "Encrypted": "Unencrypted");
        if(auracastCfg.encryptionFlag)
        {
            app_parse_printf("broadcast code is %s.\r\n", auracastCfg.broadcastCode);
            app_parse_printf("broadcast code ASCII is %s.\r\n", hex_to_str(auracastCfg.broadcastCode, 16));
        }
        if(auracastCfg.audioMode == 2)
        {
            app_parse_printf("current audio mode is stereo.\r\n");
        }
        else
        {
            app_parse_printf("current audio mode is mono.\r\n");
        }
    }
    else
    {
        app_parse_printf("show <conn|sink|source|vcp>\r\n");
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
 * @brief       Assistant UI add local source info to sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_add_local(char *argv[], int argc, void *user_data)
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

    connect_info_t* pConn = app_audio_getConn(connHandle);

    if(pConn->sinkState == SINK_STATE_NO_SOURCE) {
        blc_audio_source_head_t head = {
            .addrType = 0x00,
            .addr = {},
            .sid = PRIVATE_EXT_FILTER_SPECIFIC_SID,
            .broadcastId = {U24_TO_BYTES(DEFAULT_BROADCAST_ID)},
        };
        blc_ll_readBDAddr(head.addr);
        blc_bapba_writeAddSourcePast(connHandle, &head, 0x03);

        blc_bapba_setLocalSourceInfo(ADV_HANDLE0, &head);
    }
    else
    {
        if(pConn->sinkState == SINK_STATE_HAD_SOURCE) {
            pConn->sinkState = SINK_STATE_MODIFY_SOURCE;
            blc_bapba_writeModifySourceNotSyncPA(connHandle, pConn->remoteSourceId, 0);

        }
        else{
            app_parse_printf("please use clean-source command, reset sink state\r\n"); return ;
        }
    }

    app_parse_printf("started add source\r\n");
}

/**
 * @brief       Assistant UI setting broadcast parameters.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_source_set_param(char *argv[], int argc, void *user_data)
{
    if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
        app_parse_printf("Broadcast state not idle, should send \"bcast stop\" command to stop broadcast.\r\n");
        return;
    }

    if (!strcasecmp("Broadcast-name", argv[0])) {
        if(argc == 2 && app_audio_setBroadcastName(argv[1], strlen(argv[1])))
        {
            app_parse_printf("set Broadcast name successful, new name is %.*s", auracastCfg.broadcastNameLen, auracastCfg.broadcastName);
        }
        else
        {
            app_parse_printf("set Broadcast-name <name>, name length must less than 32 bytes.\r\n");
        }
    } else if(!strcasecmp("Broadcast-ID", argv[0])) {
        if(argc == 2)
        {
            int bcstId = app_parse_str2n(argv[1]);
            app_audio_setBroadcastID(bcstId);

            app_parse_printf("new broadcast ID is 0x%02x%02x%02x.\r\n", auracastCfg.broadcastID[2], auracastCfg.broadcastID[1], auracastCfg.broadcastID[0]);
        }
        else
        {
            app_parse_printf("set Broadcast-ID <id>, id is 24bit value.\r\n");
        }
    } else if(!strcasecmp("broadcast-code", argv[0])) {
        if(argc == 2)
        {
            app_audio_setBroadcastCode(argv[1]);
            app_parse_printf("open broadcast encrypted, broadcast code is %s.\r\n", auracastCfg.broadcastCode);
            app_parse_printf("broadcast code ASCII is %s.\r\n", hex_to_str(auracastCfg.broadcastCode, 16));
        }
        else
        {
            app_parse_printf("set broadcast-code <code>, name length must less than 17 bytes.\r\n");
        }
    }

    app_audio_storeInformation();

}

void app_audio_bcast(char *argv[], int argc, void *user_data)
{
    if (argc == 0) {
        app_parse_printf("usage: bcast <start|stop>\r\n");
        return;
    }

    app_parse_printf("status: %d\r\n", app_audio_getBroadcastState());

    if (!strcasecmp("start", argv[0])) {
        if (app_audio_getBroadcastState() == APP_AUDIO_BRODCAST_SOURCE_STATE_ACTIVE) {
            app_parse_printf("Already broadcasting\r\n");
            return;
        }

        if (!app_audio_broadcastStart()) {
            app_parse_printf("Failed to start broadcast\r\n");
        }
    } else if (!strcasecmp("stop", argv[0])) {
        if (app_audio_getBroadcastState() == APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
            app_parse_printf("Broadcasting already stopped\r\n");
            return;
        }

        if (!app_audio_broadcastStop()) {
            app_parse_printf("Failed to stop broadcast\r\n");
        }
    } else if (!strcasecmp("encrypt-close", argv[0])) {
        if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
            app_parse_printf("broadcasting active\r\n");
            return;
        }
        app_audio_closeEncryptBig();
        app_parse_printf("close broadcast encrypted.\r\n");
    } else if(!strcasecmp("stereo", argv[0]))
    {
        if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
            app_parse_printf("broadcasting active\r\n");
            return;
        }
        app_audio_setStereoAudio();
        app_parse_printf("audio mode is stereo.\r\n");
    } else if(!strcasecmp("mono", argv[0]))
    {
        if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
            app_parse_printf("broadcasting active\r\n");
            return;
        }
        app_audio_setMonoAudio();
        app_parse_printf("audio mode is mono.\r\n");
    }
    app_audio_storeInformation();
}

static const parse_fun_list_t assistantParse[] = {
    {"show", app_audio_ui_show_info},
    {"scan-sink", app_audio_ui_scan_sink},
    {"conn-sink", app_audio_ui_conn_sink},
    {"add-local", app_audio_ui_add_local},
    {"set", app_source_set_param},
    {"bcast", app_audio_bcast},
};

static void app_audio_ui_bcast_state_changed(app_audio_brodcast_state_enum state)
{
    switch (state) {
        case APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE:
            app_parse_printf("Broadcast disabled\r\n");
            break;
        case APP_AUDIO_BRODCAST_SOURCE_STATE_ENABLING:
            app_parse_printf("Broadcast enabling\r\n");
            break;
        case APP_AUDIO_BRODCAST_SOURCE_STATE_ACTIVE:
            app_parse_printf("Broadcast active\r\n");
            break;
        case APP_AUDIO_BRODCAST_SOURCE_STATE_DISABLING:
            app_parse_printf("Broadcast disabling\r\n");
            break;
        default:
            app_parse_printf("Broadcast state unknown: %d\r\n", state);
            break;
    }
}

/**
 * @brief       broadcast assistant UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_ui_init(void)
{
    app_parse_init(assistantParse, ARRAY_SIZE(assistantParse));
    bcast_set_state_changed_cb(app_audio_ui_bcast_state_changed);
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


