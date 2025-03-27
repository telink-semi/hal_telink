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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app_config.h"
#include "app_ui.h"

#include "app_audio.h"

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)

#if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_  int     key_not_released;
_attribute_ble_data_retention_  int     key_type;
_attribute_ble_data_retention_  int     key_toggle_cnt;
_attribute_ble_data_retention_  u8      volSetting;

#define CONSUMER_KEY                1
#define KEYBOARD_KEY                2
#define PAIR_UNPAIR_KEY             3

extern app_audio_ctrl_t appCtrl;

/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{
    u8 key0 = kb_event.keycode[0];

    key_not_released = 1;
    if (kb_event.cnt == 2)   //two key press
    {

    }
    else if(kb_event.cnt == 1)
    {
        if(key0 >= CR_VOL_UP )  //volume up/down
        {
            key_type = CONSUMER_KEY;
            u16 consumer_key;
            (void)consumer_key;
            u8 mediaState;
            blc_gmcsc_getMediaState(appCtrl.aclHandle, &mediaState);

            if(key0 == CR_VOL_UP)
            { //volume up
                BLT_APP_LOG("UI: volume up (mediaState:0x%x)", mediaState);

                switch (mediaState) {
                    case GMCS_MEDIA_STATE_INACTIVE:
                    case GMCS_MEDIA_STATE_PAUSED:
                        blc_gmcsc_writeStartPlayingCurrentTrack(appCtrl.aclHandle);
                        BLT_APP_LOG("UI:Start Media Play[aclHdl:0x%x]", appCtrl.aclHandle);
                        break;
                    case GMCS_MEDIA_STATE_PLAYING:
                        blc_gmcsc_writePauseCurrentTrack(appCtrl.aclHandle);
                        BLT_APP_LOG("UI:Pause Media Play[aclHdl:0x%x]", appCtrl.aclHandle);
                        break;
                    case GMCS_MEDIA_STATE_SEEKING:
                        break;
                    default:
                        break;
                }
            }
            else if(key0 == CR_VOL_DN)
            { //volume down
                BLT_APP_LOG("UI: volume down (mediaState:0x%x)", mediaState);

                switch (mediaState) {
                    case GMCS_MEDIA_STATE_PLAYING:
                    {
                        if(++key_toggle_cnt&1){
                            blc_gmcsc_writeNextTrack(appCtrl.aclHandle);
                            BLT_APP_LOG("UI:Next Track[aclHdl:0x%x]", appCtrl.aclHandle);
                        } else{
                            blc_gmcsc_writePreviousTrack(appCtrl.aclHandle);
                            BLT_APP_LOG("UI:Previous Track[aclHdl:0x%x]", appCtrl.aclHandle);
                        }
                    }
                        break;
                    case GMCS_MEDIA_STATE_INACTIVE:
                    case GMCS_MEDIA_STATE_PAUSED:
                    case GMCS_MEDIA_STATE_SEEKING:
                        break;
                    default:
                        break;
                }
            }
        }
        else
        {
            key_type = PAIR_UNPAIR_KEY;

            if(key0 == BTN_PAIR)   //Manual pair triggered by Key Press
            {
                BLT_APP_LOG("UI: Manual pair");

                u8 callIndex, state, callFlags, callMembersCnt;
                blc_gtbs_call_state_t calls[STACK_AUDIO_CALL_MEMBERS_MAX_NUM];
                blc_gtbsc_getCallState(appCtrl.aclHandle, &callMembersCnt, calls);

                for(int i = 0; i < callMembersCnt; i++){
                    callIndex = calls[i].callIndex;
                    state = calls[i].state;
                    callFlags = calls[i].callFlags;
                    (void)callFlags;
                    BLT_APP_LOG("UI: [%d] Call State:0x%x", i, state);

                    switch (state) {
                        case GTBS_CALL_STATE_INCOMING:
                            blc_gtbsc_writeAcceptIncomingCall(appCtrl.aclHandle, callIndex);
                            BLT_APP_LOG("UI:Accept Call: Call_Index:0x%x", callIndex);
                            break;
                        case GTBS_CALL_STATE_DIALING:
                        case GTBS_CALL_STATE_ALERTING:
                        case GTBS_CALL_STATE_ACTIVE:
                            blc_gtbsc_writeTerminateCall(appCtrl.aclHandle, callIndex);
                            BLT_APP_LOG("UI:Terminate Call: Call_Index:0x%x", callIndex);
                            break;
                        case GTBS_CALL_STATE_LOCALLY_HELD:
                        case GTBS_CALL_STATE_REMOTELY_HELD:
                        case GTBS_CALL_STATE_LOCALLY_AND_REMOTELY_HELD:
                            break;
                        default:
                            break;
                    }
                }
            }
            else if(key0 == BTN_UNPAIR) //Manual un_pair triggered by Key Press
            {
                BLT_APP_LOG("UI: Manual unpair");
                volSetting += 10;
                blc_vcss_updateVolSetting(appCtrl.aclHandle, volSetting);
                BLT_APP_LOG("UI:Volume Setting NTF[aclHdl:0x%x]", appCtrl.aclHandle);


                if(volSetting == 50){
                    blc_vcss_updateMuteState(appCtrl.aclHandle, 1);
                    BLT_APP_LOG("UI:Mute State NTF[aclHdl:0x%x]", appCtrl.aclHandle);
                }else if(volSetting == 60){
                    blc_vcss_updateMuteState(appCtrl.aclHandle, 0);
                    BLT_APP_LOG("UI:Mute State NTF[aclHdl:0x%x]", appCtrl.aclHandle);
                }
            }
        }
    }
    else   //kb_event.cnt == 0,  key release
    {
        key_not_released = 0;
        if(key_type == CONSUMER_KEY)
        {
            u16 consumer_key = 0;
            (void)consumer_key;
        }
        else if(key_type == KEYBOARD_KEY)
        {

        }
        else if(key_type == PAIR_UNPAIR_KEY)
        {

        }
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
    if(clock_time_exceed(keyScanTick, 10 * 1000))
    {  //keyScan interval: 10mS
        keyScanTick = clock_time();
    }
    else
    {
        return;
    }

    kb_event.keycode[0] = 0;
    int det_key = kb_scan_key (0, 1);

    if (det_key)
    {
        key_change_proc();
    }
}




#endif   //end of UI_KEYBOARD_ENABLE

#endif /* INTER_TEST_MODE */
