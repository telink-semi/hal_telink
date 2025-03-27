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
#include "../sink_config.h"
#if (SINK_VERSION == SINK_WITH_ASSISTANT_VERSION)

#include "app.h"
#include "app_config.h"
#include "app_ui.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app_audio.h"

#if (UI_KEYBOARD_ENABLE)


/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{
    u8 key0 = kb_event.keycode[0];

    if (kb_event.cnt == 2)   //two key press
    {

    }
    else if(kb_event.cnt == 1)
    {
        if(key0 == SW3_K2_9518EVK){     //volume up
            app_audio_selectBroadcastSource(true);
        }
        else if(key0 == SW2_K1_9518EVK){ //volume down
            app_audio_selectBroadcastSource(true);

        }
        else if(key0 == SW4_K3_9518EVK)   //Manual pair triggered by Key Press
        {
            app_audio_selectBroadcastSource(false);
        }
        else if(key0 == SW5_K4_9518EVK) //Manual un_pair triggered by Key Press
        {
            app_audio_selectBroadcastSource(false);
        }

    }
    else   //kb_event.cnt == 0,  key release
    {

    }
}


_attribute_ble_data_retention_      static u32 keyScanTick = 0;

/**
 * @brief      keyboard task handler
 * @param[in]  e    - event type
 * @param[in]  p    - Pointer point to event parameter.
 * @param[in]  n    - the length of event parameter.
 * @return     none.
 */
void proc_keyboard (u8 e, u8 *p, int n)
{
    if(clock_time_exceed(keyScanTick, 10 * 1000)){  //keyScan interval: 10mS
        keyScanTick = clock_time();
    }
    else{
        return;
    }

    #if(!DEBUG_GPIO_ENABLE && UI_9517C)

    static u8 keyPress[3] = {0, 0, 0};
    if(!gpio_read(SW9_KEY_9517C))
    {
        if(keyPress[0] == 0)
        {
            keyPress[0] = 1;
            app_audio_selectBroadcastSource(true);
        }
    }
    else
    {
        keyPress[0] = 0;
    }
    if(!gpio_read(SW8_KEY_9517C))
    {
        if(keyPress[1] == 0)
        {
            keyPress[1] = 1;
            app_audio_selectBroadcastSource(false);
        }
    }
    else
    {
        keyPress[1] = 0;
    }
    if(!gpio_read(SW10_KEY_9517C))
    {
        if(keyPress[2] == 0)
        {
            keyPress[2] = 1;
        }
    }
    else
    {
        keyPress[2] = 0;
    }
    #endif  //!DEBUG_GPIO_ENABLE && UI_9517C

    kb_event.keycode[0] = 0;
    int det_key = kb_scan_key (0, 1);

    if (det_key){
        key_change_proc();
    }
}




#endif   //end of UI_KEYBOARD_ENABLE


#endif      //SINK_VERSION == SINK_WITH_ASSISTANT_VERSION

