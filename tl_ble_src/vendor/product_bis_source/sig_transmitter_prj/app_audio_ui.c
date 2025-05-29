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
#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER)

    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_parse_char.h"
    #include "app_audio.h"

extern u8                     mac_public[6];
extern app_auracastCfgParam_t auracastCfg;

void app_source_show_param(char *argv[], int argc, void *user_data)
{
    app_parse_printf("Source Information.\r\n");
    app_parse_printf("the Address is %s\r\n", addr_to_str(mac_public));
    app_parse_printf("complete name is %s.\r\n", DEFAULT_DEV_NAME);
    app_parse_printf("broadcast ID is 0x%02x%02x%02x.\r\n", auracastCfg.broadcastID[2], auracastCfg.broadcastID[1], auracastCfg.broadcastID[0]);
    app_parse_printf("broadcast name is %.*s.\r\n", auracastCfg.broadcastNameLen, auracastCfg.broadcastName);
    app_parse_printf("BIG is %s.\r\n", auracastCfg.encryptionFlag ? "Encrypted" : "Unencrypted");
    if (auracastCfg.encryptionFlag) {
        app_parse_printf("broadcast code is %s.\r\n", auracastCfg.broadcastCode);
        app_parse_printf("broadcast code ASCII is %s.\r\n", hex_to_str(auracastCfg.broadcastCode, 16));
    }
    if (auracastCfg.audioMode == 2) {
        app_parse_printf("current audio mode is stereo.\r\n");
    } else {
        app_parse_printf("current audio mode is mono.\r\n");
    }
}

/**
 * @brief       Assistant UI send volume down command to sink.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
void app_source_set_param(char *argv[], int argc, void *user_data)
{
    if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
        app_parse_printf("Broadcast state not idle, should send \"bcast stop\" command to stop broadcast.\r\n");
        return;
    }

    if (!strcasecmp("Broadcast-name", argv[0])) {
        if (argc == 2 && app_audio_setBroadcastName(argv[1], strlen(argv[1]))) {
            app_parse_printf("set Broadcast name successful, new name is %.*s", auracastCfg.broadcastNameLen, auracastCfg.broadcastName);
        } else {
            app_parse_printf("set Broadcast-name <name>, name length must less than 32 bytes.\r\n");
        }
    } else if (!strcasecmp("Broadcast-ID", argv[0])) {
        if (argc == 2) {
            int bcstId = app_parse_str2n(argv[1]);
            app_audio_setBroadcastID(bcstId);

            app_parse_printf("new broadcast ID is 0x%02x%02x%02x.\r\n", auracastCfg.broadcastID[2], auracastCfg.broadcastID[1], auracastCfg.broadcastID[0]);
        } else {
            app_parse_printf("set Broadcast-ID <id>, id is 24bit value.\r\n");
        }
    } else if (!strcasecmp("broadcast-code", argv[0])) {
        if (argc == 2) {
            app_audio_setBroadcastCode(argv[1]);
            app_parse_printf("open broadcast encrypted, broadcast code is %s.\r\n", auracastCfg.broadcastCode);
            app_parse_printf("broadcast code ASCII is %s.\r\n", hex_to_str(auracastCfg.broadcastCode, 16));
        } else {
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
    } else if (!strcasecmp("stereo", argv[0])) {
        if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
            app_parse_printf("broadcasting active\r\n");
            return;
        }
        app_audio_setStereoAudio();
        app_parse_printf("audio mode is stereo.\r\n");
    } else if (!strcasecmp("mono", argv[0])) {
        if (app_audio_getBroadcastState() != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
            app_parse_printf("broadcasting active\r\n");
            return;
        }
        app_audio_setMonoAudio();
        app_parse_printf("audio mode is mono.\r\n");
    }
    app_audio_storeInformation();
}

static const parse_fun_list_t sourceParse[] = {
    {"show",  app_source_show_param},
    {"set",   app_source_set_param },
    {"bcast", app_audio_bcast      },
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
    app_parse_init(sourceParse, ARRAY_SIZE(sourceParse));
    app_parse_printf("Auracast source initial\r\n");
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

#endif //PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER
