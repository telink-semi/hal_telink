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
#include "../default_att.h"
#include "app_ui.h"


#if (FEATURE_TEST_MODE == TEST_PAWR_ADV)

typedef struct
{
    u8 set;
    u8 type;
    u8 address[BLE_ADDR_LEN];
} dev_info_t;

int central_pairing_enable = 0;
u16 central_unpair_enable  = 0;

u16 central_disconnect_connhandle; //mark the central connection which is in un_pair disconnection flow


    #if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_ int key_not_released;


        #define CONSUMER_KEY    1
        #define KEYBOARD_KEY    2
        #define PAIR_UNPAIR_KEY 3

_attribute_ble_data_retention_ u8 key_type;

_attribute_ble_data_retention_ u8 subevent_create_conn = 0;

dev_info_t ESL_deviceDB[4] = {
    {
     .set     = 0,
     .type    = BLE_ADDR_PUBLIC,
     .address = {0x99, 0x99, 0x99, 0x99, 0x99, 0x99},
     },
    {
     .set     = 1,
     .type    = BLE_ADDR_PUBLIC,
     .address = {0x01, 0x99, 0x99, 0x99, 0x99, 0x99},
     },
    {
     .set     = 2,
     .type    = BLE_ADDR_PUBLIC,
     .address = {0x02, 0x99, 0x99, 0x99, 0x99, 0x99},
     },
    {
     .set     = 3,
     .type    = BLE_ADDR_PUBLIC,
     .address = {0x03, 0x99, 0x99, 0x99, 0x99, 0x99},
     },
};

/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{
    u8 key0 = kb_event.keycode[0];
    //  u8 key_buf[8] = {0,0,0,0,0,0,0,0};

    key_not_released = 1;
    if (kb_event.cnt == 2)     //two key press
    {
    } else if (kb_event.cnt == 1) {
        if (key0 >= CR_VOL_UP) //volume up/down
        {
            key_type = CONSUMER_KEY;
            u16 consumer_key;
            if (key0 == CR_VOL_UP) {        //volume up
                consumer_key = MKEY_VOL_UP;
                tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY] send Vol +", 0, 0);
            } else if (key0 == CR_VOL_DN) { //volume down
                consumer_key = MKEY_VOL_DN;
                tlkapi_send_string_data(APP_KEY_LOG_EN, "[UI][KEY] send Vol -", 0, 0);
            }


            /*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, when Vol+/Vol- key pressed, we
            send media key "Vol+" or "Vol-" to central for all peripheral role in connection.
            For users, you should known that this is not a good method, you should manage your device and GATT data transfer
            according to  conn_dev_list[]
             * */
            for (int i = ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++) { //peripheral index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
                if (conn_dev_list[i].conn_state) {
                    blc_gatt_pushHandleValueNotify(conn_dev_list[i].conn_handle, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
                }
            }

            tlkapi_printf(APP_LOG_EN, "le_createConnection_v2 mac:%02X %02X %02X %02X %02X %02X", ESL_deviceDB[subevent_create_conn].address[0], ESL_deviceDB[subevent_create_conn].address[1], ESL_deviceDB[subevent_create_conn].address[2], ESL_deviceDB[subevent_create_conn].address[3], ESL_deviceDB[subevent_create_conn].address[4], ESL_deviceDB[subevent_create_conn].address[5]);

            ble_sts_t status =
                blc_ll_extended_createConnection_v2(ADV_HANDLE0, ESL_deviceDB[subevent_create_conn].set, /* adv_handle, subevent */
                                                    INITIATE_FP_ADV_SPECIFY,                             /* filter_policy */
                                                    OWN_ADDRESS_PUBLIC,                                  /* ownAdrType */
                                                    ESL_deviceDB[subevent_create_conn].type,
                                                    ESL_deviceDB[subevent_create_conn].address,          /* peerAdrType, *peerAddr */
                                                    0,                                                   /* init_phys */
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0, /* scanInter_0, scanWindow_0, conn_min_0, conn_max_0, timeout_0 */
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0, /* scanInter_1, scanWindow_1, conn_min_1, conn_max_1, timeout_1 */
                                                    0,
                                                    0,
                                                    0,
                                                    0,
                                                    0 /* scanInter_2, scanWindow_2, conn_min_2, conn_max_2, timeout_2 */);

            if (status == BLE_SUCCESS) { //create connection success
                tlkapi_printf(APP_LOG_EN, "blc_ll_createConnection [devid:%d][subevent:%d] SUCCESS", subevent_create_conn, ESL_deviceDB[subevent_create_conn].set);
            } else {
                tlkapi_printf(APP_LOG_EN, "blc_ll_createConnection [devid:%d][subevent:%d][error code:%02X]", subevent_create_conn, ESL_deviceDB[subevent_create_conn].set, status);
            }

            if (++subevent_create_conn >= ARRAY_SIZE(ESL_deviceDB)) {
                subevent_create_conn = 0;
            }
        } else {
            key_type = PAIR_UNPAIR_KEY;

            if (key0 == BTN_PAIR) //Manual pair triggered by Key Press
            {
                if (blc_ll_isInitiationBusy()) {
                    blc_ll_createConnectionCancel();
                    tlkapi_printf(APP_LOG_EN, "[UI][PAIR] le initiation busy, cancel it");
                } else {
                    central_pairing_enable = 1;
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair begin", 0, 0);
                }
            } else if (key0 == BTN_UNPAIR) //Manual un_pair triggered by Key Press
            {
                /*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
                 available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */

                //              if(acl_conn_central_num){ //at least 1 central connection exist
                //
                //                  if(!central_disconnect_connhandle){  //if one central un_pair disconnection flow not finish, here new un_pair not accepted
                //
                //                      /* choose one central connection to disconnect */
                //                      for(int i=0; i < ACL_CENTRAL_MAX_NUM; i++){ //peripheral index is from 0 to "ACL_CENTRAL_MAX_NUM - 1"
                //                          if(conn_dev_list[i].conn_state){
                //                              central_unpair_enable = conn_dev_list[i].conn_handle;  //mark connHandle on central_unpair_enable
                //                              tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Unpair", &central_unpair_enable, 2);
                //                              break;
                //                          }
                //                      }
                //                  }
                //              }
                for (int i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {
                    if (conn_dev_list[i].conn_state) {
                        ble_sts_t s = blc_ll_periodicAdvSetInfoTransfer(conn_dev_list[i].conn_handle, 0xff, ADV_HANDLE0);
                        tlkapi_send_string_data(1 || APP_SMP_LOG_EN, "[UI][PAST] PAST Pkt transfer status:'", &s, 1);
                        if (s == BLE_SUCCESS) {
                            gpio_write(GPIO_LED_GREEN, 1);
                        }
                    }
                }
            }
        }

    } else //kb_event.cnt == 0,  key release
    {
        key_not_released = 0;
        if (key_type == CONSUMER_KEY) {
            u16 consumer_key = 0;
            //Here is just Telink Demonstration effect. for all peripheral in connection, send release for previous "Vol+" or "Vol-" to central
            for (int i = ACL_CENTRAL_MAX_NUM; i < (ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM); i++) { //peripheral index is from "ACL_CENTRAL_MAX_NUM" to "ACL_CENTRAL_MAX_NUM + ACL_PERIPHR_MAX_NUM - 1"
                if (conn_dev_list[i].conn_state) {
                    blc_gatt_pushHandleValueNotify(conn_dev_list[i].conn_handle, HID_CONSUME_REPORT_INPUT_DP_H, (u8 *)&consumer_key, 2);
                }
            }
        } else if (key_type == KEYBOARD_KEY) {
        } else if (key_type == PAIR_UNPAIR_KEY) {
            if (central_pairing_enable) {
                central_pairing_enable = 0;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair end", 0, 0);
            }

            if (central_unpair_enable) {
                central_unpair_enable = 0;
            }
        }
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

    kb_event.keycode[0] = 0;
    int det_key         = kb_scan_key(0, 1);

    if (det_key) {
        key_change_proc();
    }
}


    #endif //end of UI_KEYBOARD_ENABLE


/**
 * @brief   BLE Unpair handle for central
 * @param   none.
 * @return  none.
 */
void proc_central_role_unpair(void)
{
    //terminate and un_pair process, Telink demonstration effect: triggered by "un_pair" key press
    if (central_unpair_enable) {
        dev_char_info_t *dev_char_info = dev_char_info_search_by_connhandle(central_unpair_enable); //connHandle has marked on on central_unpair_enable

        if (dev_char_info) {                                                                        //un_pair device in still in connection state

            if (blc_ll_disconnect(central_unpair_enable, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS) {
                central_disconnect_connhandle = central_unpair_enable;                              //mark conn_handle

                central_unpair_enable = 0;                                                          //every "un_pair" key can only triggers one connection disconnect


    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    // delete ATT handle storage on flash
                dev_char_info_delete_peer_att_handle_by_peer_mac(dev_char_info->peer_adrType, dev_char_info->peer_addr);
    #endif


    // delete this device information(mac_address and distributed keys...) on FLash
    #if (ACL_CENTRAL_SMP_ENABLE)
                blc_smp_deleteBondingPeripheralInfo_by_PeerMacAddress(dev_char_info->peer_adrType, dev_char_info->peer_addr);
    #endif

                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] delete peer device", &central_disconnect_connhandle, 2);
            }

        } else {                       //un_pair device can not find in device list, it's not connected now

            central_unpair_enable = 0; //every "un_pair" key can only triggers one connection disconnect
        }
    }
}


#endif //end of (FEATURE_TEST_MODE == ...)
