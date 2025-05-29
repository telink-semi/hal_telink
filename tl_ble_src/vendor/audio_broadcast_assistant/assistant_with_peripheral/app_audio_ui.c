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
#include "../assistant_config.h"

#if (ASSISTANT_VERSION == ASSISTANT_WITH_PERIPHERAL_VERSION)

    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_parse_char.h"
    #include "app_audio.h"

u16 scanSourceHandle = 0x00;
u8  scanSourceIndex  = 0x00;

extern const char locationStr[32][30];
extern bool       muteFlag;

/**
 * @brief       Assistant UI show information(scan sink device/connected sink device/scan source/sink vcp).
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_show_info(char *argv[], int argc, void *user_data)
{
    if (strcasecmp(argv[0], "conn") == 0) {
        app_parse_printf("connect sink info\r\n");
        app_audio_showConnInfo();
    } else if (strcasecmp(argv[0], "sink") == 0) {
        app_parse_printf("scan sink info\r\n");
        app_audio_showSinkInfo();
    } else if (strcasecmp(argv[0], "source") == 0) {
        if (!scanSourceHandle) {
            app_parse_printf("No Source Scanning\r\n");
        } else {
            app_parse_printf("Connected Index:%x is scanning source info\r\n", scanSourceIndex);
            app_audio_showSourceInfo();
        }
    } else if (strcasecmp(argv[0], "vcp") == 0) {
        int index = app_parse_str2n(argv[1]);

        u16 connHandle = app_audio_getConnHandle(index);

        if (!connHandle) {
            return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
        }

        blc_audio_vcpState_t vcpState;

        audio_error_enum state = blc_vcpc_getState(connHandle, &vcpState);

        if (state != AUDIO_ESUCC) {
            return app_parse_printf("get vcp state error\r\n");
        }

        app_parse_printf("Remote Volume Control State, connHandle: 0x%02x\r\n", connHandle);

        app_parse_printf("volume(min:0, max:255) is %d, muteSate:%s\r\n", vcpState.volState.volumeSetting, vcpState.volState.mute ? "Mute" : "Unmute");
        if (vcpState.vosCnt) {
            for (int i = 0; i < vcpState.vosCnt; i++) {
                app_parse_printf("VOCS index is %d \r\n", i);
                blc_audio_volumeOffsetState_t *vocs = &vcpState.voc[i];
                app_parse_printf("Location:");
                for (int j = 0; j < 32; j++) {
                    if (vocs->location & BIT(j)) {
                        app_parse_printf("%s ", &locationStr[j][0]);
                    }
                }
                app_parse_printf("\r\nAudio Description is %.*s\r\n", vocs->outDescLen, vocs->outDesc);
                app_parse_printf("Volume Offset(min:-255, max:255) is %d\r\n", vocs->volumeOffset);
            }
        } else {
            app_parse_printf("Not Had VOCS\r\n");
        }
    } else {
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

    if (strcasecmp(argv[0], "start") == 0) {
        app_parse_printf("assistant start scan sink\r\n");
        app_audio_initSinkInfoBuf();
        app_audio_openScanSink();
    } else if (strcasecmp(argv[0], "stop") == 0) {
        app_parse_printf("assistant stop scan sink\r\n");
        app_audio_closeScanSink();
    } else if (strcasecmp(argv[0], "clear") == 0) {
        app_audio_initSinkInfoBuf();
        app_parse_printf("assistant clear sink info\r\n");
    } else {
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
    if (argc != 1) {
        app_parse_printf("conn-sink <dev_idx>\r\n");
        return;
    }

    if (app_audio_aclConnFull()) {
        app_parse_printf("connect sink fail: Connection already exists");
        return;
    }

    if (app_audio_createSinkConn(app_parse_str2n(argv[0]))) {
        app_parse_printf("assistant start connect sink\r\n");
    } else {
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
    if (argc != 2) {
        app_parse_printf("scan-bcast <start|stop|clear> <conn_idx>\r\n");
        return;
    }
    int index = app_parse_str2n(argv[1]);

    u16 connHandle = app_audio_getConnHandle(index);

    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t *pConn = app_audio_getConn(connHandle);

    if (pConn->sdp_over == 0) {
        return app_parse_printf("wait sdp discovery ending\r\n");
    }

    if (pConn->BASS_server == 0) {
        return app_parse_printf("connect device not supported BASS Server \r\n");
    }

    if (pConn->PACS_server == 0) {
        return app_parse_printf("connect device not supported PACS Server \r\n");
    }

    if (strcasecmp(argv[0], "start") == 0) {
        app_audio_initSourceInfoBuf();
        scanSourceIndex  = index;
        scanSourceHandle = connHandle;
        blc_bapba_writeRemoteScanStarted(connHandle);
        app_parse_printf("assistant start scan broadcast source\r\n");
    } else if (strcasecmp(argv[0], "stop") == 0) {
        blc_bapba_writeRemoteScanStopped(connHandle);
        app_parse_printf("assistant stop scan broadcast source\r\n");
    } else if (strcasecmp(argv[0], "clear") == 0) {
        scanSourceIndex  = 0;
        scanSourceHandle = 0;
        app_audio_initSourceInfoBuf();
        blc_bapba_writeRemoteScanStopped(connHandle);
        app_parse_printf("assistant clear broadcast source info\r\n");
    } else {
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
    if (argc < 3) {
        app_parse_printf("add-source <conn_idx> <source_idx> <bis_sync> [broadcast_key]\r\n");
        return;
    }

    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));
    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    int            sourceIndex = app_parse_str2n(argv[1]);
    source_info_t *sourceInfo  = app_audio_getSourceInfo(sourceIndex);
    if (!sourceInfo) {
        return app_parse_printf("index error.You can run the \"show source\" command to view source information. \r\n");
    }

    int bisSync = app_parse_str2n(argv[2]);

    connect_info_t *pConn = app_audio_getConn(connHandle);

    if (sourceInfo->enc) {
        if (argc != 4) {
            return app_parse_printf("source is Encrypted, please enter broadcast code\r\n");
        }
        strncpy(pConn->broadcastCode, argv[3], 16);
    }

    int bisIndex = 0;
    for (int i = 0; i < sourceInfo->bisCnt; i++) {
        bisIndex |= BIT(sourceInfo->bisIndex[i] - 1);
    }

    if (bisSync & (~bisIndex)) {
        return app_parse_printf("Bis sync is error, only set 0x%08x", bisIndex);
    }

    if (pConn->sinkState == SINK_STATE_NO_SOURCE) {
        blc_bapba_writeAddSourceNoPast(connHandle, (blc_audio_source_head_t *)sourceInfo, bisSync);
    } else {
        if (pConn->sinkState == SINK_STATE_HAD_SOURCE) {
            pConn->sinkState = SINK_STATE_MODIFY_SOURCE;
            blc_bapba_writeModifySourceNotSyncPA(connHandle, pConn->remoteSourceId, 0);

            pConn->sourceIndex = sourceIndex;
            pConn->bisSync     = bisSync;

        } else {
            app_parse_printf("please use clean-source command, reset sink state\r\n");
            return;
        }
    }

    app_parse_printf("started add source\r\n");
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
    if (argc < 1) {
        return app_parse_printf("mute <conn_idx>\r\n");
    }
    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t *pConn = app_audio_getConn(connHandle);

    if (pConn->VCS_server == 0) {
        return app_parse_printf("connect device not supported VCS Server \r\n");
    }

    muteFlag = !muteFlag;

    app_parse_printf("mute flag is %d", muteFlag);

    if (muteFlag) {
        blc_vcsc_writeMute(connHandle);
    } else {
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
    if (argc < 1) {
        return app_parse_printf("vol+ <conn_idx>\r\n");
    }
    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t *pConn = app_audio_getConn(connHandle);

    if (pConn->VCS_server == 0) {
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
    if (argc < 1) {
        return app_parse_printf("vol- <conn_idx>\r\n");
    }
    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t *pConn = app_audio_getConn(connHandle);

    if (pConn->VCS_server == 0) {
        return app_parse_printf("connect device not supported VCS Server \r\n");
    }

    blc_vcsc_writeUnmuteOrRelativeVolDown(connHandle);
}

/**
 * @brief       Assistant UI send set volume value command to sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_vol(char *argv[], int argc, void *user_data)
{
    if (argc < 2) {
        return app_parse_printf("set-vol <conn_idx> <volume value>\r\n");
    }

    u16 connHandle = app_audio_getConnHandle(app_parse_str2n(argv[0]));

    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    connect_info_t *pConn = app_audio_getConn(connHandle);

    if (pConn->VCS_server == 0) {
        return app_parse_printf("connect device not supported VCS Server \r\n");
    }

    blc_vcsc_writeSetAbsoluteVol(connHandle, app_parse_str2n(argv[1]));
}

/**
 * @brief       Assistant UI set volume offset value command callback.
 * @param[in]   connHandle: ACL connection.
 * @param[in]   err: ATT layer error code.
 * @return      none.
 */
static void app_audio_set_vol_offset_cb(u16 connHandle, att_err_t err)
{
    if (err == ATT_SUCCESS) {
        app_parse_printf("write volume offset value is success\r\n");
    } else {
        app_parse_printf("write volume offset error, ATT error code is 0x%x", err);
    }
}

/**
 * @brief       Assistant UI set volume offset value command to sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_audio_ui_set_vol_offset(char *argv[], int argc, void *user_data)
{
    if (argc != 3) {
        return app_parse_printf("set-vol-offset <conn_idx> <vocs_idx> <vol_offset>\r\n");
    }

    int index = app_parse_str2n(argv[0]);

    u16 connHandle = app_audio_getConnHandle(index);

    if (!connHandle) {
        return app_parse_printf("index error.You can run the \"show conn\" command to view connection information. \r\n");
    }

    blc_audio_vcpState_t vcpState;

    int state = blc_vcpc_getState(connHandle, &vcpState);

    if (state != AUDIO_ESUCC) {
        return app_parse_printf("get vcp state error\r\n");
    }

    int vocsIdx    = app_parse_str2n(argv[1]);
    int vol_offset = app_parse_str2n(argv[2]);

    if (vocsIdx >= vcpState.vosCnt) {
        return app_parse_printf("vocs index is error, \"show vcp <conn_idx> \" command to view vcp information. \r\n");
    }

    if (vol_offset > 255 || vol_offset < -255) {
        return app_parse_printf("volume offset value is error, must in -255~255.");
    }

    state = blc_vocsc_vcpWriteSetVolOffset(connHandle, vocsIdx, vol_offset, app_audio_set_vol_offset_cb);

    if (state != BLE_SUCCESS) {
        return app_parse_printf("write Volume Offset command error, BLE error state is 0x%x", state);
    }
}

static const parse_fun_list_t assistantParse[] = {
    {"show",           app_audio_ui_show_info     },
    {"scan-sink",      app_audio_ui_scan_sink     },
    {"conn-sink",      app_audio_ui_conn_sink     },
    {"scan-bcast",     app_audio_ui_scan_source   },
    {"add-source",     app_audio_ui_add_source    },
    {"mute",           app_audio_ui_mute_sink     },
    {"vol+",           app_audio_ui_vol_up        },
    {"vol-",           app_audio_ui_vol_down      },
    {"set-vol",        app_audio_ui_vol           },
    {"set-vol-offset", app_audio_ui_set_vol_offset},
};

/**
 * @brief       broadcast assistant UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_ui_init(void)
{
    app_parse_init(assistantParse, ARRAY_SIZE(assistantParse));
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

#endif //ASSISTANT_VERSION == ASSISTANT_WITH_PERIPHERAL_VERSION
