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

#include "app.h"
#include "app_buffer.h"
#include "app_ui.h"


int central_pairing_enable = 0;
u16 central_unpair_enable  = 0;

u16 central_disconnect_connhandle; //mark the central connection which is in un_pair disconnection flow

int app_setCigParam_success;

extern u16 app_aclConnHandle[];
extern u16 app_cisConnHandle[];
extern int app_cis_conn_num;


#if (UI_KEYBOARD_ENABLE)

_attribute_ble_data_retention_ int key_not_released;


    #define CONSUMER_KEY    1
    #define KEYBOARD_KEY    2
    #define PAIR_UNPAIR_KEY 3

_attribute_ble_data_retention_ u8 key_type;

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
    if (kb_event.cnt == 2)    //two key press
    {
    } else if (kb_event.cnt == 1) {
        if (key0 == BTN_PAIR) //Manual pair triggered by Key Press
        {
            central_pairing_enable = 1;
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair begin", 0, 0);
        } else if (key0 == BTN_UNPAIR) //Manual un_pair triggered by Key Press
        {
            /*Here is just Telink Demonstration effect. Cause the demo board has limited key to use, only one "un_pair" key is
             available. When "un_pair" key pressed, we will choose and un_pair one device in connection state */
            if (acl_conn_central_num) {               //at least 1 central connection exist

                if (!central_disconnect_connhandle) { //if one central un_pair disconnection flow not finish, here new un_pair not accepted

                    /* choose one central connection to disconnect */
                    for (int i = 0; i < ACL_CENTRAL_MAX_NUM; i++) {               //peripheral index is from 0 to "ACL_CENTRAL_MAX_NUM - 1"
                        if (conn_dev_list[i].conn_state) {
                            central_unpair_enable = conn_dev_list[i].conn_handle; //mark connHandle on central_unpair_enable
                            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Unpair", &central_unpair_enable, 2);
                            break;
                        }
                    }
                }
            }
        }
        if (key0 == SET_CIG_PARAM) {
            if (app_setCigParam_success) {
            } else {
                if (acl_conn_central_num) { //at least on ACL Central exist
                    u8                             cig_ret_buffer[3 + APP_CIS_CENTRAL_NUMBER * 2];
                    hci_le_setCigParam_retParam_t *pCigRetParam = (hci_le_setCigParam_retParam_t *)cig_ret_buffer;

                    u8                                 cig_cmd_buffer[15 + APP_CIS_CENTRAL_NUMBER * sizeof(cigParamTest_cisCfg_t)];
                    hci_le_setCigParamTest_cmdParam_t *pCigCmdParam = (hci_le_setCigParamTest_cmdParam_t *)cig_cmd_buffer;


    #if (DBG_CREATE_FF0D_ERROR)
                    pCigCmdParam->cig_id = CIG_ID_0;
                    u32 sdu_interval     = 10000; //10mS
                    memcpy(pCigCmdParam->sdu_int_m2s, &sdu_interval, 3);
                    memcpy(pCigCmdParam->sdu_int_s2m, &sdu_interval, 3);
                    pCigCmdParam->ft_m2s    = 4;
                    pCigCmdParam->ft_s2m    = 4;
                    pCigCmdParam->iso_intvl = ISO_INTERVAL_10MS;
                    pCigCmdParam->sca       = PPM_251_500;
                    pCigCmdParam->packing   = PACK_SEQUENTIAL;
                    pCigCmdParam->framing   = CIS_UNFRAMED;
                    pCigCmdParam->cis_count = APP_CIS_CENTRAL_NUMBER;

                    for (int i = 0; i < pCigCmdParam->cis_count; i++) {
                        pCigCmdParam->cisCfg[i].cis_id      = i;
                        pCigCmdParam->cisCfg[i].nse         = 4;
                        pCigCmdParam->cisCfg[i].max_sdu_m2s = 80;
                        pCigCmdParam->cisCfg[i].max_sdu_s2m = 80;
                        pCigCmdParam->cisCfg[i].max_pdu_m2s = 80;
                        pCigCmdParam->cisCfg[i].max_pdu_s2m = 80;
                        pCigCmdParam->cisCfg[i].phy_m2s     = PHY_PREFER_2M;
                        pCigCmdParam->cisCfg[i].phy_s2m     = PHY_PREFER_2M;
                        pCigCmdParam->cisCfg[i].bn_m2s      = 1;
                        pCigCmdParam->cisCfg[i].bn_s2m      = 1;
                    }
    #else
                    pCigCmdParam->cig_id = CIG_ID_0;
                    u32 sdu_interval     = 10000; //10mS
                    memcpy(pCigCmdParam->sdu_int_m2s, &sdu_interval, 3);
                    memcpy(pCigCmdParam->sdu_int_s2m, &sdu_interval, 3);
                    pCigCmdParam->ft_m2s    = 1;
                    pCigCmdParam->ft_s2m    = 1;
                    pCigCmdParam->iso_intvl = ISO_INTERVAL_10MS;
                    pCigCmdParam->sca       = PPM_251_500;
                    pCigCmdParam->packing   = PACK_SEQUENTIAL;
                    pCigCmdParam->framing   = CIS_UNFRAMED;
                    pCigCmdParam->cis_count = APP_CIS_CENTRAL_NUMBER;

                    for (int i = 0; i < pCigCmdParam->cis_count; i++) {
                        pCigCmdParam->cisCfg[i].cis_id      = i;
                        pCigCmdParam->cisCfg[i].nse         = 2;
                        pCigCmdParam->cisCfg[i].max_sdu_m2s = 50;
                        pCigCmdParam->cisCfg[i].max_sdu_s2m = 50;
                        pCigCmdParam->cisCfg[i].max_pdu_m2s = 50;
                        pCigCmdParam->cisCfg[i].max_pdu_s2m = 50;
                        pCigCmdParam->cisCfg[i].phy_m2s     = PHY_PREFER_1M;
                        pCigCmdParam->cisCfg[i].phy_s2m     = PHY_PREFER_1M;
                        pCigCmdParam->cisCfg[i].bn_m2s      = 1;
                        pCigCmdParam->cisCfg[i].bn_s2m      = 1;
                    }
    #endif

                    u8 status = blc_hci_le_setCigParamsTest(pCigCmdParam, pCigRetParam);
                    if (status == BLE_SUCCESS) {
                        app_setCigParam_success = 1;
                        for (int i = 0; i < APP_CIS_CENTRAL_NUMBER; i++) {
                            app_cisConnHandle[i] = pCigRetParam->cis_connHandle[i];
                        }
                        tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] Set CIG Param Success", 0, 0);
                    } else {
                        tlkapi_send_string_data(APP_CIS_LOG_EN, "[APP][CIS] Set CIG Param Fail", &status, 1);
                    }
                }
            }
        } else if (key0 == CREATE_CIS) {
            if (app_setCigParam_success) {
                // app_cis_conn_num
                u8                        cis_create_buffer[12];
                hci_le_CreateCisParams_t *pCisParam = (hci_le_CreateCisParams_t *)cis_create_buffer;

                pCisParam->cis_count             = 1;
                pCisParam->cisConn[0].cis_handle = app_cisConnHandle[0];
                pCisParam->cisConn[0].acl_handle = app_aclConnHandle[0];

                //              pCisParam->cis_count = 2;
                //              pCisParam->cisConn[0].cis_handle = app_cisConnHandle[0];
                //              pCisParam->cisConn[0].acl_handle = app_aclConnHandle[0];
                //              pCisParam->cisConn[1].cis_handle = app_cisConnHandle[1];
                //              pCisParam->cisConn[1].acl_handle = app_aclConnHandle[1];

                u8 status = blc_hci_le_createCis(pCisParam);

                if (status == BLE_SUCCESS) { //Start to Send LL_CIS_REQ pkt
                    tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] create CIS command success", 0, 0);
                } else {
                    tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] create CIS command Fail", &status, 1);
                }

            } else {
                tlkapi_send_string_data(APP_CIS_LOG_EN, "Create CIS before Set CIG Param, Fail", 0, 0);
            }
        }

    } else //kb_event.cnt == 0,  key release
    {
        key_not_released = 0;

        //if(key_type == PAIR_UNPAIR_KEY)
        {
            if (central_pairing_enable) {
                central_pairing_enable = 0;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair begin", 0, 0);
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
#if (ACL_CENTRAL_CUSTOM_PAIR_ENABLE)
    if (blm_manPair.manual_pair && clock_time_exceed(blm_manPair.pair_tick, 2000000)) { //@@
        blm_manPair.manual_pair = 0;
    }
#endif


    //terminate and un_pair process, Telink demonstration effect: triggered by "un_pair" key press
    if (central_unpair_enable) {
        dev_char_info_t *dev_char_info = dev_char_info_search_by_connhandle(central_unpair_enable); //connHandle has marked on on central_unpair_enable

        if (dev_char_info) {                                                                        //un_pair device in still in connection state

            if (blc_ll_disconnect(central_unpair_enable, HCI_ERR_REMOTE_USER_TERM_CONN) == BLE_SUCCESS) {
                central_disconnect_connhandle = central_unpair_enable;                              //mark conn_handle

                central_unpair_enable = 0;                                                          //every "un_pair" key can only triggers one connection disconnect


// delete this device information(mac_address and distributed keys...) on FLash
#if (ACL_CENTRAL_SMP_ENABLE)
                blc_smp_deleteBondingPeripheralInfo_by_PeerMacAddress(dev_char_info->peer_adrType, dev_char_info->peer_addr);
#elif (ACL_CENTRAL_CUSTOM_PAIR_ENABLE)
                user_tbl_peripheral_mac_delete_by_adr(dev_char_info->peer_adrType, dev_char_info->peer_addr);
#endif
            }

        } else {                       //un_pair device can not find in device list, it's not connected now

            central_unpair_enable = 0; //every "un_pair" key can only triggers one connection disconnect
        }
    }
}
