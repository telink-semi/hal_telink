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
#include "app.h"
#include "app_ui.h"
#include "app_audio.h"


#if (INTER_TEST_MODE == TEST_CIS_AUDIO_CLIENT)

#if (UI_KEYBOARD_ENABLE)

int central_pairing_enable = 0;
u16 central_unpair_enable = 0;

u16 central_disconnect_connhandle;   //mark the central connection which is in un_pair disconnection flow

_attribute_ble_data_retention_  int     key_not_released;
_attribute_ble_data_retention_  int     key_type;
_attribute_ble_data_retention_  int     key_toggle_cnt;

#define CONSUMER_KEY                1
#define KEYBOARD_KEY                2
#define PAIR_UNPAIR_KEY             3

extern app_common_ctrl_t app_ctrl;

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
            if(key0 == CR_VOL_UP)
            {
                u8 volumeTogg = ++key_toggle_cnt%1;
                if(volumeTogg)
                {
                    BLT_APP_LOG("UI: volume up");
                    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
                    {
                        blc_vcsc_writeRelativeVolUp(app_ctrl.acl_param[i].acl_handle);
                    }
                }
                else
                {
                    BLT_APP_LOG("UI: volume down");
                    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
                    {
                        blc_vcsc_writeRelativeVolDown(app_ctrl.acl_param[i].acl_handle);
                    }
                }
            }
            else if(key0 == CR_VOL_DN)
            {
                u8 muteTogg = ++key_toggle_cnt%1;
                if(muteTogg) {
                    BLT_APP_LOG("UI: mute");
                    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
                    {
                        blc_vcsc_writeMute(app_ctrl.acl_param[i].acl_handle);
                    }

                }else{
                    BLT_APP_LOG("UI: unmute");
                    for(u8 i=0;i<app_ctrl.acl_max_num;i++)
                    {
                        blc_vcsc_writeUnmute(app_ctrl.acl_param[i].acl_handle);
                    }
                }
            }
        }
        else
        {
            if(key0 == BTN_PAIR)
            {
                central_pairing_enable = 1;
                BLT_APP_LOG("UI: Pair begin");
            }
            else if(key0 == BTN_UNPAIR)
            {
                /*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
                 available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */
                if(acl_conn_central_num){ //at least 1 central connection exist

                    if(!central_disconnect_connhandle && !central_unpair_enable){  //if one central un_pair disconnection flow not finish, here new un_pair not accepted

                        /* choose one central connection to disconnect */
                        for(int i=0; i < ACL_CENTRAL_MAX_NUM; i++){ //peripheral index is from 0 to "ACL_CENTRAL_MAX_NUM - 1"
                            if(conn_dev_list[i].conn_state){
                                central_unpair_enable = conn_dev_list[i].conn_handle;  //mark connHandle on central_unpair_enable
                                BLT_APP_LOG("[0x%x] UI: Unpair", central_unpair_enable);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    else   //kb_event.cnt == 0,  key release
    {
        if(central_pairing_enable){
            central_pairing_enable = 0;
            BLT_APP_LOG("UI: Pair end", 0, 0);
        }

        if(central_unpair_enable){
            central_unpair_enable = 0;
        }
        key_not_released = 0;
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

    kb_event.keycode[0] = 0;
    int det_key = kb_scan_key (0, 1);

    if (det_key){
        key_change_proc();
    }
}


/**
 * @brief   BLE Unpair handle for central
 * @param   none.
 * @return  none.
 */
void proc_central_role_unpair(void)
{
    //terminate and un_pair process, Telink demonstration effect: triggered by "un_pair" key press
    if(central_unpair_enable)
    {
        dev_char_info_t* dev_char_info = dev_char_info_search_by_connhandle(central_unpair_enable); //connHandle has marked on on central_unpair_enable

        if( dev_char_info ){ //un_pair device in still in connection state

            if(blc_ll_disconnect(central_unpair_enable, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS){

                central_disconnect_connhandle = central_unpair_enable; //mark conn_handle

                central_unpair_enable = 0;  //every "un_pair" key can only triggers one connection disconnect

                // delete this device information(mac_address and distributed keys...) on FLash
                #if (ACL_CENTRAL_SMP_ENABLE)
                    u32 flash_addr = blc_smp_deleteBondingPeripheralInfo_by_PeerMacAddress(dev_char_info->peer_adrType, dev_char_info->peer_addr);
                #endif

                if(app_ctrl.acl_csis_size > 1){
                    u8 rank = 0;
                    int r = blc_csisc_getSetMemberRank(central_disconnect_connhandle, &rank);
                    if(AUDIO_ESUCC == r){
                        BLT_APP_LOG("[0x%x]CSIS: member%d, unpair:flash_addr 0x%x", central_disconnect_connhandle, rank, flash_addr);
                    }
                }
            }
        }
        else{ //un_pair device can not find in device list, it's not connected now

            central_unpair_enable = 0;  //every "un_pair" key can only triggers one connection disconnect
        }
    }
}


#endif   //end of UI_KEYBOARD_ENABLE

#endif /* INTER_TEST_MODE */
