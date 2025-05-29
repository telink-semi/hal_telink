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
#include "app_buffer.h"
#include "application/keyboard/keyboard.h"
#include "application/usbstd/usbkeycode.h"


#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)


int central_pairing_enable       = 0;
u8  central_pairing_aclCen_index = 0;

u16 central_unpair_enable = 0;
u16 central_disconnect_connhandle; //mark the central connection which is in un_pair disconnection flow

int app_setCigParam_success;


    #if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_ int key_not_released;
_attribute_ble_data_retention_ int key_type;

        #define CONSUMER_KEY    1
        #define KEYBOARD_KEY    2
        #define PAIR_UNPAIR_KEY 3

/**
 * @brief   Check changed key value.
 * @param   none.
 * @return  none.
 */
void key_change_proc(void)
{
    u8 key0 = kb_event.keycode[0];
    u8 key1 = kb_event.keycode[1];

    key_not_released = 1;
    if (kb_event.cnt == 2) //two key press
    {
        if ((key0 == BTN_KEY1 && key1 == BTN_KEY4) ||
            (key0 == BTN_KEY4 && key1 == BTN_KEY1)) {
            if (!central_disconnect_connhandle) { //if one central un_pair disconnection flow not finish, here new un_pair not accepted
                if (acl_cen_connnected[ACLCEN_IDX_MOUSE]) {
                    central_unpair_enable = acl_cen_connnected[ACLCEN_IDX_MOUSE];
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] UnPair Mouse", 0, 0);
                }
            } else {
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] previous UnPair not finish", 0, 0);
            }
        } else if ((key0 == BTN_KEY2 && key1 == BTN_KEY4) ||
                   (key0 == BTN_KEY4 && key1 == BTN_KEY2)) {
            if (!central_disconnect_connhandle) { //if one central un_pair disconnection flow not finish, here new un_pair not accepted
                if (acl_cen_connnected[ACLCEN_IDX_KEYBOARD]) {
                    central_unpair_enable = acl_cen_connnected[ACLCEN_IDX_KEYBOARD];
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] UnPair Keyboard", 0, 0);
                }
            } else {
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] previous UnPair not finish", 0, 0);
            }
        }
        #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
        else if ((key0 == BTN_KEY3 && key1 == BTN_KEY4) ||
                 (key0 == BTN_KEY4 && key1 == BTN_KEY3)) {

            if (!central_disconnect_connhandle) { //if one central un_pair disconnection flow not finish, here new un_pair not accepted
                if (acl_cen_connnected[ACLCEN_IDX_CIS]) {
                    central_unpair_enable = acl_cen_connnected[ACLCEN_IDX_CIS];
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] UnPair CIS", 0, 0);
                }
            } else {
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] previous UnPair not finish", 0, 0);
            }
        }
        #endif
    } else if (kb_event.cnt == 1) {
        if (key0 == BTN_KEY1) {
            if (!acl_cen_connnected[ACLCEN_IDX_MOUSE]) {
                central_pairing_enable       = 1;
                central_pairing_aclCen_index = ACLCEN_IDX_MOUSE;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair mouse begin", 0, 0);
            }
        } else if (key0 == BTN_KEY2) {
            if (!acl_cen_connnected[ACLCEN_IDX_KEYBOARD]) {
                central_pairing_enable       = 1;
                central_pairing_aclCen_index = ACLCEN_IDX_KEYBOARD;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair keyboard begin", 0, 0);
            }
        } else if (key0 == BTN_KEY3) {
        #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
            central_pairing_enable = 1;
            if (!acl_cen_connnected[ACLCEN_IDX_CIS]) { //connect ACL for CIS1
                central_pairing_aclCen_index = ACLCEN_IDX_CIS;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair CIS begin", 0, 0);
            }
        #endif
        } else if (key0 == BTN_KEY4) {
        #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
            if (!acl_cen_connnected[ACLCEN_IDX_CIS]) { //no ACL for CIS
                tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] Can not create CIS before corresponding ACL establish", 0, 0);
            } else {
                if (app_setCigParam_success) {         //set CIG Param OK

                    u8                        cis_create_buffer[12];
                    hci_le_CreateCisParams_t *pCisParam = (hci_le_CreateCisParams_t *)cis_create_buffer;

                    int create_cis_cmd = 0;
                    if (app_cis_established) {
                        tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] CIS already created", 0, 0);
                    } else {
                        pCisParam->cis_count = 1;

                        //ACL connected, CIS not established
                        if (acl_cen_connnected[ACLCEN_IDX_CIS] && !app_cis_established) { //create CIS
                            create_cis_cmd                   = 1;
                            pCisParam->cisConn[0].cis_handle = app_cisConnHandle;
                            pCisParam->cisConn[0].acl_handle = acl_cen_connnected[ACLCEN_IDX_CIS];
                            tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] create CIS", &pCisParam->cisConn[0].cis_handle, 4);
                        } else {
                            tlkapi_send_string_u8s(APP_CIS_LOG_EN, "[APP][CIS] not create CIS", acl_cen_connnected[ACLCEN_IDX_CIS], app_cis_established, 0, 0);
                        }
                    }

                    if (create_cis_cmd) {
                        u8 status = blc_hci_le_createCis(pCisParam);

                        if (status == BLE_SUCCESS) {
                            tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] create CIS command success", 0, 0);
                        } else {
                            tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] create CIS command Fail", &status, 1);
                        }
                    }
                } else {
                    u8                             cig_ret_buffer[3 + APP_CIS_CENTRAL_NUMBER * 1];
                    hci_le_setCigParam_retParam_t *pCigRetParam = (hci_le_setCigParam_retParam_t *)cig_ret_buffer;

                    u8                                 cig_cmd_buffer[15 + APP_CIS_CENTRAL_NUMBER * sizeof(cigParamTest_cisCfg_t)];
                    hci_le_setCigParamTest_cmdParam_t *pCigCmdParam = (hci_le_setCigParamTest_cmdParam_t *)cig_cmd_buffer;

                    pCigCmdParam->cig_id = CIG_ID_0;
                    u32 sdu_interval_m2s = 10000;
                    u32 sdu_interval_s2m = 10000;
                    memcpy(pCigCmdParam->sdu_int_m2s, &sdu_interval_m2s, 3);
                    memcpy(pCigCmdParam->sdu_int_s2m, &sdu_interval_s2m, 3);
                    pCigCmdParam->ft_m2s    = 1;
                    pCigCmdParam->ft_s2m    = 1;
                    pCigCmdParam->iso_intvl = ISO_INTERVAL_10MS;
                    pCigCmdParam->sca       = PPM_251_500;
                    pCigCmdParam->packing   = PACK_INTERLEAVED;
                    pCigCmdParam->framing   = CIS_UNFRAMED;
                    pCigCmdParam->cis_count = APP_CIS_CENTRAL_NUMBER;

                    for (int i = 0; i < pCigCmdParam->cis_count; i++) {
                        pCigCmdParam->cisCfg[i].cis_id      = i;
                        pCigCmdParam->cisCfg[i].nse         = 4;
                        pCigCmdParam->cisCfg[i].max_sdu_m2s = 200;
                        pCigCmdParam->cisCfg[i].max_sdu_s2m = 40;
                        pCigCmdParam->cisCfg[i].max_pdu_m2s = 200;
                        pCigCmdParam->cisCfg[i].max_pdu_s2m = 40;
                        pCigCmdParam->cisCfg[i].phy_m2s     = PHY_PREFER_2M;
                        pCigCmdParam->cisCfg[i].phy_s2m     = PHY_PREFER_2M;
                        pCigCmdParam->cisCfg[i].bn_m2s      = 1;
                        pCigCmdParam->cisCfg[i].bn_s2m      = 1;
                    }

                    u8 status = blc_hci_le_setCigParamsTest(pCigCmdParam, pCigRetParam);
                    if (status == BLE_SUCCESS) {
                        tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] Set CIG Param Success", 0, 0);
                        app_setCigParam_success = 1;
                        app_cisConnHandle       = pCigRetParam->cis_connHandle[0];
                    } else {
                        tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] Set CIG Param Fail", &status, 1);
                    }
                }
            }
        #endif
        }
    } else //kb_event.cnt == 0,  key release
    {
        key_not_released = 0;

        if (central_pairing_enable) {
            central_pairing_enable = 0;
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][Pair] Pair end", 0, 0);
        }

        if (central_unpair_enable) {
            central_unpair_enable = 0;
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][Pair] UnPair end", 0, 0);
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
            }

        } else {                       //un_pair device can not find in device list, it's not connected now

            central_unpair_enable = 0; //every "un_pair" key can only triggers one connection disconnect
        }
    }
}

kb_data_t kb_dat_report = {
    1,
    0,
    {0, 0, 0, 0, 0, 0}
};
int keyboard_not_release = 0;

/**
 * @brief       This function is used to send HID keyboard report by USB.
 * @param[in]   conn     - connection handle
 * @param[in]   p        - Pointer point to data buffer.
 * @return
 */
void att_keyboard(u16 conn, u8 *p)
{
    tlkapi_send_string_data(APP_KEYBOARD_LOG_EN, "[UI][KB] keyboard report", p, 8);


    #if (USB_KEYBOARD_ENABLE)
    memcpy(&kb_dat_report, p, sizeof(kb_data_t));

    if (kb_dat_report.keycode[0]) {
        #if 0
            kb_dat_report.cnt = 1;
        #else
        for (int i = 0; i < KB_RETURN_KEY_MAX; i++) {
            if (kb_dat_report.keycode[i]) {
                kb_dat_report.cnt = i + 1;
            } else {
                break;
            }
        }
        #endif

        keyboard_not_release = 1;
    } else {
        kb_dat_report.cnt    = 0; //key release
        keyboard_not_release = 0;
    }


    //tlkapi_send_string_data(APP_KEYBOARD_LOG_EN, "[UI][KB] keyboard USB", (u8*)&kb_dat_report, 8);

    usbkb_hid_report(&kb_dat_report);
    #endif
}

/**
 * @brief       This function is used to send HID mouse report by USB.
 * @param[in]   conn - connect handle
 * @param[in]   p - pointer of l2cap data packet
 * @return      none
 */
void att_mouse(u16 conn, u8 *p)
{
    tlkapi_send_string_data(APP_MOUSE_LOG_EN, "[UI][MOUSE] mouse report", p, 6);

    #if (USB_KEYBOARD_ENABLE)
    mouse_data_t mouse_dat_report;
    mouse_dat_report.btn = *p++;
    mouse_dat_report.x   = *p++;
    p++;
    mouse_dat_report.y = *p++;
    p++;
    mouse_dat_report.wheel = *p;

    //tlkapi_send_string_data(APP_MOUSE_LOG_EN, "[UI][MOUSE] mouse USB", (u8*)&mouse_dat_report, 4);

    extern void usbmouse_add_frame(mouse_data_t * packet_mouse, int packet_num);
    usbmouse_add_frame(&mouse_dat_report, 1);
    #endif
}


#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)
