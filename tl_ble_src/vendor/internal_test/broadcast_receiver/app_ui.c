/********************************************************************************************************
 * @file    app_ui.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "app.h"
#include "app_config.h"
#include "app_ui.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"


#if (INTER_TEST_MODE == TEST_BIS_AUDIO_RECEIVER)

int central_pairing_enable = 0;
u16 central_unpair_enable  = 0;

u16 central_disconnect_connhandle; //mark the central connection which is in un_pair disconnection flow


int central_auto_connect = 0;
int user_manual_pairing;


    #if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_ int key_not_released;
_attribute_ble_data_retention_ int key_type;

        #define CONSUMER_KEY    1
        #define KEYBOARD_KEY    2
        #define PAIR_UNPAIR_KEY 3

unsigned char volume   = 0;
unsigned char muteFlag = 0;

/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{
    u8 key0 = kb_event.keycode[0];

    key_not_released = 1;
    if (kb_event.cnt == 2)           //two key press
    {
    } else if (kb_event.cnt == 1) {
        if (key0 >= CR_VOL_UP)       //volume up/down
        {
            if (key0 == CR_VOL_UP) { //volume up
                if (volume < 12) {
                    volume += 2;
                }
                BLT_APP_LOG("UI send Vol+");
            } else if (key0 == CR_VOL_DN) { //volume down
                if (volume >= 2) {
                    volume -= 2;
                }
                BLT_APP_LOG("UI send Vol-");
            }
            //          audio_set_output_gain_test(12-volume);
            //          blc_audio_vcsServerUpdateVolSetting(app_audio_get_acl_conn_handle(), volume);
        } else {
            if (key0 == BTN_MUTE) //Manual pair triggered by Key Press
            {
                BLT_APP_LOG("UI mute");
                //              audio_set_codec_dac_mute();
                //blc_audio_vcsServerUpdateMute(app_audio_get_acl_conn_handle(), 1);
            } else if (key0 == BTN_UNMUTE) //Manual un_pair triggered by Key Press
            {
                BLT_APP_LOG("UI numute");
                //              audio_set_codec_dac_unmute();
                //blc_audio_vcsServerUpdateMute(app_audio_get_acl_conn_handle(), 0);
            }
        }

    } else //kb_event.cnt == 0,  key release
    {
    }
}

_attribute_ble_data_retention_ static u32 keyScanTick = 0;

/**
 * @brief      keyboard task handler
 * @param[in]  e    - event type
 * @param[in]  p    - Pointer point to event parameter.
 * @param[in]  n    - the length of event parameter.
 * @return     none.
 */
void proc_keyboard(u8 e, u8 *p, int n)
{
    if (clock_time_exceed(keyScanTick, 10 * 1000)) { //keyScan interval: 10mS
        keyScanTick = clock_time();
    } else {
        return;
    }

    static u8 keyPress[3] = {0, 0, 0};
    if (!gpio_read(GPIO_PB1)) {
        if (keyPress[0] == 0) {
            keyPress[0] = 1;
            if (volume < 12) {
                volume += 2;
            }
            //          audio_set_output_gain_test(12-volume);
            BLT_APP_LOG("UI send Vol+");
            //blc_audio_vcsServerUpdateVolSetting(app_audio_get_acl_conn_handle(), volume);
        }
    } else {
        keyPress[0] = 0;
    }
    if (!gpio_read(GPIO_PB2)) {
        if (keyPress[1] == 0) {
            keyPress[1] = 1;
            if (volume >= 2) {
                volume -= 2;
            }
            //          audio_set_output_gain_test(12-volume);
            BLT_APP_LOG("UI send Vol-");
            //blc_audio_vcsServerUpdateVolSetting(app_audio_get_acl_conn_handle(), volume);
        }
    } else {
        keyPress[1] = 0;
    }
    if (!gpio_read(GPIO_PB0)) {
        if (keyPress[2] == 0) {
            keyPress[2] = 1;
            if (muteFlag) {
                BLT_APP_LOG("UI mute");
                //              audio_set_codec_dac_unmute();
            } else {
                BLT_APP_LOG("UI unmute");
                //              audio_set_codec_dac_mute();
            }
            muteFlag = ~muteFlag;
            //blc_audio_vcsServerUpdateMute(app_audio_get_acl_conn_handle(), muteFlag);
        }
    } else {
        keyPress[2] = 0;
    }

    kb_event.keycode[0] = 0;
    int det_key         = kb_scan_key(0, 1);

    if (det_key) {
        key_change_proc();
    }
}


    #endif //end of UI_KEYBOARD_ENABLE


#endif     /* INTER_TEST_MODE */
