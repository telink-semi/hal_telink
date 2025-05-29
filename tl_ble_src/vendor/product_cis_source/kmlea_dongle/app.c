/********************************************************************************************************
 * @file    app.c
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
#include "app_audio.h"
#include "app.h"
#include "app_buffer.h"
#include "app_ui.h"


#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)


//dev_cus_info_t    dev_custom_list[DEVICE_CHAR_INFO_MAX_NUM];

int central_smp_pending = 0; // SMP: security & encryption;


/* matrix index   0: mouse, 1:keyboard, 2:CIS
 *     connected:  value is connection handle(none zero)
 * not connected:  value is 0*/
u16 acl_cen_connnected[ACL_CENTRAL_MAX_NUM] = {0};

    #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
u8 app_cis_established;
;
u16 app_cisConnHandle;
    #endif


/**
 * @brief      LE Extended Advertising report event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_ext_adv_report_event_handle(u8 *p, int evt_data_len)
{
    #if (ACL_CENTRAL_SMP_ENABLE)
    if (central_smp_pending) { //if previous connection SMP not finish, can not create a new connection
        return 1;
    }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    if (central_sdp_pending) { //if previous connection SDP not finish, can not create a new connection
        return 1;
    }
    #endif

    if (central_disconnect_connhandle) { //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return 1;
    }


    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int offset = 0;

    extAdvEvt_info_t *pExtAdvInfo = NULL;


    for (int i = 0; i < pExtAdvRpt->num_reports; i++) {
        pExtAdvInfo = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
        s8 rssi = pExtAdvInfo->rssi;
        (void)rssi; //remove compiler warning
        u8 ext_evtType = pExtAdvInfo->event_type & EXTADV_RPT_EVTTYPE_MASK;

    #if 0           //debug, print ADV report
            static u32 AA_dbg_adv_rpt = 0;
            static u32 tick_adv_rpt = 0;
            if(1 || clock_time_exceed(tick_adv_rpt, 2000000)){  //2000000
                tick_adv_rpt = clock_time();
                AA_dbg_adv_rpt ++;
                if(ext_evtType & EXTADV_RPT_EVT_MASK_LEGACY){
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "Adv report legacy", pExtAdvInfo->address, 6);
                }
                else{
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "Adv report extend", pExtAdvInfo->address, 6);
                }
            }
    #endif


    #if 0 //for SiHui debug, please remove at your application code
            u8 temp_mac[6] = {0x33, 0x33, 0x33, 0x33, 0x33, 0x33};
            if(!smemcmp(pExtAdvInfo->address + 3, temp_mac + 3, 3)){
                //tlkapi_send_string_data(APP_PAIR_LOG_EN, "Adv report legacy", pExtAdvInfo->address, 6);
            }
            else{
                return 0;
            }
    #endif


        if (blc_ll_isInitiationBusy()) {
            return 0;
        }

        /* legacy ADV  */
        if (ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_IND || ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_DIRECT_IND) {
            if (rssi < -66) { //short distance limitation for debug
                return 0;
            }

            int adv_appearance_match = 0, pair_devive_match = 0;
            u8  cur_paring_acl_cen_idx = central_pairing_aclCen_index;

    #if 1
            if (ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_DIRECT_IND) {
                //tlkapi_send_string_data();
                u8         temp_buff[sizeof(dev_att_t)];
                dev_att_t *pdev_att = (dev_att_t *)temp_buff;
                if (dev_char_info_search_peer_att_handle_by_peer_mac(pExtAdvInfo->address_type, pExtAdvInfo->address, pdev_att)) {
                    cur_paring_acl_cen_idx = pdev_att->rsvd[0];
                    if (cur_paring_acl_cen_idx <= ACLCEN_IDX_KEYBOARD) {
                        adv_appearance_match = 1;
                        if (cur_paring_acl_cen_idx == ACLCEN_IDX_MOUSE) {
                            tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[UI][PAIR] mouse direct ADV", pExtAdvInfo->address, 6);
                        } else {
                            tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[UI][PAIR] keyboard direct ADV", pExtAdvInfo->address, 6);
                        }

                    } else {
                        tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[APP] direct ADV not match", pExtAdvInfo->address, 6);
                        return 0;
                    }
                }
            } else { // EXTADV_RPT_EVTTYPE_LEGACY_ADV_IND
                u8 index = 0;
                while (index < pExtAdvInfo->data_length) {
                    advData_str_t *advDataStr = (advData_str_t *)(&pExtAdvInfo->data[0] + index);
                    if (advDataStr->type == DT_APPEARANCE) {
                        u16 appearance_value = advDataStr->data[0] | (advDataStr->data[1] << 8);
                        if (appearance_value == APPEAR_HID_MOUSE) {
        #if 0
                                        u8 mac_mouse[6] = {0x87, 0xF0, 0x70, 0x30, 0x04, 0xD1}; //D1 05 30    D1 07 30
                                        if(memcmp(pExtAdvInfo->address, mac_mouse, 3)){
                                            return 0;
                                        }
        #endif

                            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Mouse ADV", pExtAdvInfo->address, 6);
                            adv_appearance_match = 1;
                            if (central_pairing_enable) {
                                pair_devive_match = central_pairing_aclCen_index == ACLCEN_IDX_MOUSE;
                            } else { //maybe auto connect
                                cur_paring_acl_cen_idx = ACLCEN_IDX_MOUSE;
                            }
                        } else if (appearance_value == APPEAR_HID_KEYBOARD) {
        #if 0
                                        u8 mac_keyboard[6] = {0x1B, 0x25, 0x15, 0x8D, 0x03, 0xD1};
                                        if(memcmp(pExtAdvInfo->address, mac_keyboard, 3)){
                                            return 0;
                                        }
        #endif

                            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] keyboard ADV", pExtAdvInfo->address, 6);
                            adv_appearance_match = 1;
                            if (central_pairing_enable) {
                                pair_devive_match = central_pairing_aclCen_index == ACLCEN_IDX_KEYBOARD;
                            } else { //maybe auto connect
                                cur_paring_acl_cen_idx = ACLCEN_IDX_KEYBOARD;
                            }
                        }

                        break;
                    }

                    index += (advDataStr->length + 1);
                }
            }

        #if 0 //for SiHui debug, please remove at your application code
                        if(1){
                            adv_appearance_match = 1;
                            if(central_pairing_enable){
                                pair_devive_match = 1;
                            }
                            else{
                                cur_paring_acl_cen_idx = ACLCEN_IDX_CIS;
                                //tlkapi_send_string_data(1, "no paring", pExtAdvInfo->address, 6);
                                u8  temp_buff[sizeof(dev_att_t)];
                                dev_att_t *pdev_att = (dev_att_t *)temp_buff;
                                if( dev_char_info_search_peer_att_handle_by_peer_mac(pExtAdvInfo->address_type, pExtAdvInfo->address, pdev_att) ){
                                    cur_paring_acl_cen_idx = pdev_att->rsvd[0];
                                    tlkapi_send_string_data(1, "auto connect", &cur_paring_acl_cen_idx, 1);
                                }
                            }
                        }
        #endif
    #else     //debug code
            u8  mac_keyboard[6] = {0x1B, 0x25, 0x15, 0x8D, 0x03, 0xD1};
            int kb              = 0;
            if (!memcmp(pExtAdvInfo->address, mac_keyboard, 3)) {
                kb = 1;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] keyboard ADV", pExtAdvInfo->address, 6);
            }

            u8  mac_mouse[6] = {0x87, 0xF0, 0x70, 0x30, 0x04, 0xD1}; //D1 05 30    D1 07 30
            int mouse        = 0;
            if (!memcmp(pExtAdvInfo->address, mac_mouse, 3)) {
                mouse = 1;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Mouse ADV", pExtAdvInfo->address, 6);
            }

            if (!kb && !mouse) {
                return 0;
            }
    #endif


            if (adv_appearance_match) {
                int central_auto_connect = 0;
                int user_manual_pairing  = 0;

                /* manual pairing methods 1: key press triggers.  set RSSI threshold, short distance */
                user_manual_pairing = central_pairing_enable && pair_devive_match && (rssi > -66);
    #if (ACL_CENTRAL_SMP_ENABLE)
                central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pExtAdvInfo->address_type, pExtAdvInfo->address);
                if (central_auto_connect) {
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Auto connect match", pExtAdvInfo->address, 6);
                }
    #endif

                if (user_manual_pairing || central_auto_connect) {
                    blc_ll_setAclCentralIndex_arrangedTaskTiming_diffMode(cur_paring_acl_cen_idx);

                    u16 conn_interval = CONN_INTERVAL_20MS;
                    if (cur_paring_acl_cen_idx == ACLCEN_IDX_MOUSE) {
                        conn_interval = CONN_INTERVAL_10MS;
                    }

                    u8 status = blc_ll_extended_createConnection(INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, conn_interval, conn_interval, CONN_TIMEOUT_4S, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

                    if (status == BLE_SUCCESS) { //create connection success
                        tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] Extended INIT on legacy ADV OK", pExtAdvInfo->address, 6);
                    } else {
                        tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] Extended INIT on legacy ADV Fail", &status, 1);
                    }
                }
            }

        }
        /* Extended ADV */
        else if (ext_evtType == EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_DIRECTED) {
            /* Extended, Non_Connectable Non_Scannable Undirected */
        } else if (ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_DIRECTED) {
    #if 0 //for SiHui debug, please remove at your application code. filter 0x333333xxyyzz
                u8 mac_high_byte[3] = {0x33, 0x33, 0x33};
                if(memcmp(pExtAdvInfo->address + 3, mac_high_byte, 3)){
                    return 0;
                }
    #endif

            u8 conn_adv_flag  = 0;
            u8 conn_auto_conn = 0;


            conn_adv_flag          = 2; //Extended
            u8               index = 0;
            app_advdata_LTV *adv_data;
            while (index < pExtAdvInfo->data_length) {
                adv_data = (app_advdata_LTV *)(&pExtAdvInfo->data[0] + index);
                if (adv_data->type == DT_SERVICE_DATA) {
                    u16 ServiceUUID = adv_data->data[0] | (adv_data->data[1] << 8);
                    //                  tlkapi_send_string_data(USER_DUMP_EN, "Service UUID", (u8 *)&ServiceUUID, 2);
                    if (ServiceUUID == SERVICE_UUID_AUDIO_STREAM_CONTROL) {
                        app_adv_announcement_t *p = (app_adv_announcement_t *)&adv_data->data[2];
                        if (p->announcement_type == 1 && p->available_audio_context | (BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA)) {
                            //tlkapi_send_string_data(USER_DUMP_EN, "context check pass", (u8 *)&ServiceUUID, 2);
                            conn_auto_conn = 1;
                            break;
                        } else {
                            continue;
                        }
                    }
                }

                index += (adv_data->length + 1);
            }

            if (conn_auto_conn) {
                int trigger_conn = 0;
                u8  cur_paring_acl_cen_idx;
    #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
                cur_paring_acl_cen_idx = central_pairing_aclCen_index;
                trigger_conn           = central_pairing_enable && central_pairing_aclCen_index == ACLCEN_IDX_CIS && (rssi > -66);
    #else
                if (!acl_cen_connnected[ACLCEN_IDX_CIS]) { //connect ACL for CIS
                    trigger_conn           = 1;
                    cur_paring_acl_cen_idx = ACLCEN_IDX_CIS;
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Connect ACL for CIS", 0, 0);
                } else {
                    tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] ACL for CIS ERROR", 0, 0);
                }
    #endif

                if (trigger_conn) {
                    blc_ll_setAclCentralIndex_arrangedTaskTiming_diffMode(cur_paring_acl_cen_idx);
                    u8 status = blc_ll_extended_createConnection(INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_20MS, CONN_TIMEOUT_1S, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_20MS, CONN_TIMEOUT_1S, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_20MS, CONN_TIMEOUT_1S);


                    if (status != BLE_SUCCESS) {
                        tlkapi_send_string_data(APP_LOG_EN, "[APP] Extended INIT on EXT ADV Fail", pExtAdvInfo->address, 6); //&status, 1
                    } else {
                        tlkapi_send_string_data(APP_LOG_EN, "[APP] Extended INIT on EXT ADV OK", pExtAdvInfo->address, 6);
                    }
                }
            }
        }
    }

    return 0;
}

/**
 * @brief      BLE enhanced connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_enhanced_connection_complete_event_handle(u8 *p)
{
    hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

    if (pConnEvt->status == BLE_SUCCESS) {
        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP] LE Enhanced Connection complete", &pConnEvt->connHandle, 28);

        dev_char_info_insert_by_enhanced_conn_event(pConnEvt);

        if (pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
        {
            /* attention: here ACL CEN index can not use "central_pairing_aclCen_index" */
            u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pConnEvt->connHandle);

            if (cur_aclCentral_idx <= ACLCEN_IDX_MAX) {
                acl_cen_connnected[cur_aclCentral_idx] = pConnEvt->connHandle; //mark ACL connected and ACL handle value

    #if (ACL_CENTRAL_SMP_ENABLE)
                central_smp_pending = pConnEvt->connHandle;                    // this connection need SMP
    #endif

                if (cur_aclCentral_idx <= ACLCEN_IDX_KEYBOARD) {               // ACLCEN_IDX_MOUSE  ACLCEN_IDX_KEYBOARD
                    if (cur_aclCentral_idx == ACLCEN_IDX_MOUSE) {
    #if (UI_LED_ENABLE)
                        gpio_write(GPIO_LED_RED, 1);
    #endif
                        tlkapi_send_string_data(APP_LOG_EN, "[UI] Mouse connect", &cur_aclCentral_idx, 1);
                    } else { // ACLCEN_IDX_KEYBOARD
    #if (UI_LED_ENABLE)
                        gpio_write(GPIO_LED_GREEN, 1);
    #endif
                        tlkapi_send_string_data(APP_LOG_EN, "[UI] Keyboard connect", &cur_aclCentral_idx, 1);
                    }

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
                    cur_sdp_device.conn_handle  = pConnEvt->connHandle;
                    cur_sdp_device.peer_adrType = pConnEvt->PeerAddrType;
                    memcpy(cur_sdp_device.peer_addr, pConnEvt->PeerAddr, 6);

                    u8         temp_buff[sizeof(dev_att_t)];
                    dev_att_t *pdev_att = (dev_att_t *)temp_buff;

                    /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
                    if (dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->PeerAddrType, pConnEvt->PeerAddr, pdev_att)) {
                        cur_sdp_device.char_handle[2] = pdev_att->char_handle[2]; //OTA
                        cur_sdp_device.char_handle[3] = pdev_att->char_handle[3]; //consume report
                        cur_sdp_device.char_handle[4] = pdev_att->char_handle[4]; //keyboard report
                        cur_sdp_device.char_handle[5] = pdev_att->char_handle[5]; //mouse report

                        /* add the peer device att_handle value to conn_dev_list */
                        dev_char_info_add_peer_att_handle(&cur_sdp_device);
                        tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[APP][SDP] read old SDP infor", &pConnEvt->connHandle, 2);
                    } else {
                        central_sdp_pending = pConnEvt->connHandle; // mark this connection need SDP

        #if (ACL_CENTRAL_SMP_ENABLE)
                            //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
                        tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[APP][SDP] new SDP pending", &pConnEvt->connHandle, 2);
        #else
                        tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[APP][SDP] new SDP start", &pConnEvt->connHandle, 2);
                        app_register_service(&app_service_discovery); //No SMP, service discovery can initiated now
        #endif
                    }
    #endif
                } else { // ACLCEN_IDX_CIS
                    blc_ll_setExtendedScanSecondaryChannelRxDataProcessEnable(0);
                    tlkapi_send_string_data(APP_LOG_EN, "[APP] disable ext_scan secondary channel process", 0, 0);

    #if (UI_LED_ENABLE)
                    gpio_write(GPIO_LED_BLUE, 1);
    #endif
                    tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] ACL of CIS connect", &cur_aclCentral_idx, 1);
                }
            } else {
                tlkapi_send_string_data(APP_LOG_EN, "[UI] connection aclCentral_idx Get ERROR !!!", &cur_aclCentral_idx, 1);
            }
        }


        if (acl_conn_central_num == ACL_CENTRAL_MAX_NUM) {
            tlkapi_send_string_data(APP_LOG_EN, "[UI] ACL CEN number max, disable Scan", &acl_conn_central_num, 1);
            blc_ll_setExtScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        }
    }

    return 0;
}

/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_disconnect_event_handle(u8 *p)
{
    hci_disconnectionCompleteEvt_t *pDisConn = (hci_disconnectionCompleteEvt_t *)p;
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] disconnect event", &pDisConn->connHandle, 3);

    //terminate reason
    if (pDisConn->reason == HCI_ERR_CONN_TIMEOUT) {                 //connection timeout

    } else if (pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN) { //peer device send terminate command on link layer

    }
    //central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
    else if (pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST) {
    } else {
    }


    dev_char_info_t *dev_char_info = dev_char_info_search_by_connhandle(pDisConn->connHandle);
    if (dev_char_info)                                      //ACL connection
    {
        if (dev_char_info->conn_role == ACL_ROLE_CENTRAL) { //ACL central

            u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pDisConn->connHandle);

            if (cur_aclCentral_idx <= ACLCEN_IDX_MAX) {
                acl_cen_connnected[cur_aclCentral_idx] = 0;

    #if (ACL_CENTRAL_SMP_ENABLE)
                if (central_smp_pending == pDisConn->connHandle) { //if previous connection SMP not finished, clear flag
                    central_smp_pending = 0;
                }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                if (central_sdp_pending == pDisConn->connHandle) {
                    central_sdp_pending = 0;
                }
    #endif

                if (central_disconnect_connhandle == pDisConn->connHandle) { //un_pair disconnection flow finish, clear flag
                    central_disconnect_connhandle = 0;
                }

                if (cur_aclCentral_idx <= ACLCEN_IDX_KEYBOARD) { // ACLCEN_IDX_MOUSE  ACLCEN_IDX_KEYBOARD
                    if (cur_aclCentral_idx == ACLCEN_IDX_MOUSE) {
    #if (UI_LED_ENABLE)
                        gpio_write(GPIO_LED_RED, 0);
    #endif
                        tlkapi_send_string_data(APP_LOG_EN, "[UI] Mouse disconnect", &cur_aclCentral_idx, 1);
                    } else { //ACLCEN_IDX_KEYBOARD
    #if (UI_LED_ENABLE)
                        gpio_write(GPIO_LED_GREEN, 0);
    #endif
                        tlkapi_send_string_data(APP_LOG_EN, "[UI] keyboard disconnect", &cur_aclCentral_idx, 1);
                    }

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    if (central_sdp_pending == pDisConn->connHandle) {
                        central_sdp_pending = 0;
                    }
    #endif
                } else { // ACLCEN_IDX_CIS
                    blc_ll_setExtendedScanSecondaryChannelRxDataProcessEnable(1);
                    tlkapi_send_string_data(APP_LOG_EN, "[APP] restore ext_scan secondary channel process", 0, 0);

    #if (UI_LED_ENABLE)
                    gpio_write(GPIO_LED_BLUE, 0);
    #endif
                    tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] ACL of CIS disconnect", &cur_aclCentral_idx, 1);
                }
            } else {
                tlkapi_send_string_data(APP_LOG_EN, "[UI] disconnection aclCentral_idx Get ERROR !!!", &cur_aclCentral_idx, 1);
            }
        }

        dev_char_info_delete_by_connhandle(pDisConn->connHandle);


        if (acl_conn_central_num == (ACL_CENTRAL_MAX_NUM - 1)) {
            tlkapi_send_string_data(APP_LOG_EN, "[UI] ACL CEN number not max, enable Scan", &acl_conn_central_num, 1);
            blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
        }
    } else {
    #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
        if (pDisConn->connHandle == app_cisConnHandle) { //CIS handle
            app_cis_established = 0;
            tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] CIS disconnect ", &pDisConn->connHandle, 2);
        }
    #endif
    }


    return 0;
}

/**
 * @brief      BLE Connection update complete event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_update_complete_event_handle(u8 *p)
{
    return 0;
}

/**
 * @brief      LE CIS Established event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_cis_establish_event_handle(u8 *p)
{
    #if (PEER_DEVICE_USE_CIS_PERIPHERAL_TEST)
    hci_le_cisEstablishedEvt_t *pCisEstbEvent = (hci_le_cisEstablishedEvt_t *)p;
    if (pCisEstbEvent->status == BLE_SUCCESS) {
        if (pCisEstbEvent->cisHandle == app_cisConnHandle) {
            app_cis_established = 1;
            tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] CIS establish ", &pCisEstbEvent->cisHandle, 2);
        } else {
            tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] ERROR CIS establish !!!", &pCisEstbEvent->cisHandle, 2);
        }
    } else {
        tlkapi_send_string_data(APP_CIS_LOG_EN, "[UI][CIS] CIS establish event ERROR", &pCisEstbEvent->status, 1)
    }
    #endif

    return 0;
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h - event type
 * @param[in]  p - Pointer point to event parameter buffer.
 * @param[in]  n - the length of event parameter.
 * @return
 */
int app_controller_event_callback(u32 h, u8 *p, int n)
{
    if (h & HCI_FLAG_EVENT_BT_STD) //Controller HCI event
    {
        u8 evtCode = h & 0xff;

        //------------ disconnect -------------------------------------
        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE) //connection terminate
        {
            app_disconnect_event_handle(p);
        }
        //------------ encryption change event -------------------------------
        else if (evtCode == HCI_EVT_ENCRYPTION_CHANGE) //encryption change
        {
            //hci_le_encryptEnableEvt_t* pEnc = (hci_le_encryptEnableEvt_t*)p;
            //tlkapi_send_string_data(APP_LOG_EN, "[APP] Controller encryption change event ", &pEnc->connHandle, 3)
        } else if (evtCode == HCI_EVT_LE_META) //LE Event
        {
            u8 subEvt_code = p[0];
            //------hci le event: le enhanced_connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE) // connection complete
            {
                app_le_enhanced_connection_complete_event_handle(p);
            }
            //------hci le event: LE extended advertising report event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) // ADV packet
            {
                app_le_ext_adv_report_event_handle(p, n);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE) // connection update
            {
                app_le_connection_update_complete_event_handle(p);
            }
            //------HCI LE event: LE CIS Established event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CIS_ESTABLISHED) {
                app_le_cis_establish_event_handle(p);
            }
        }
    }
    return 0;
}

/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback(u32 h, u8 *para, int n)
{
    u8 event = h & 0xFF;

    switch (event) {
    case GAP_EVT_SMP_PAIRING_BEGIN:
    {
    } break;

    case GAP_EVT_SMP_PAIRING_SUCCESS:
    {
    } break;

    case GAP_EVT_SMP_PAIRING_FAIL:
    {
    #if (ACL_CENTRAL_SMP_ENABLE)
        gap_smp_pairingFailEvt_t *pEvt = (gap_smp_pairingFailEvt_t *)para;

        if (dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL) {
            if (central_smp_pending == pEvt->connHandle) {
                tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring fail", &pEvt->connHandle, sizeof(gap_smp_pairingFailEvt_t));
                central_smp_pending = 0;
            }
        }
    #endif
    } break;

    case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
    {
        gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Connection encryption done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));
    } break;

    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
        gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Security process done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));

    #if (ACL_CENTRAL_SMP_ENABLE)
        if (dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL) {
            if (central_smp_pending == pEvt->connHandle) {
                central_smp_pending = 0;
            }
        }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pEvt->connHandle);
        if (cur_aclCentral_idx <= ACLCEN_IDX_KEYBOARD) {
            if (central_sdp_pending == pEvt->connHandle) {    //SDP is pending
                tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[APP][SDP] start SDP after SMP done", &pEvt->connHandle, 2);
                app_register_service(&app_service_discovery); //start SDP now
            }
        }
    #endif
    } break;
    case GAP_EVT_ATT_EXCHANGE_MTU:
    {
    } break;

    case GAP_EVT_L2CAP_CONN_PARAM_UPDATE_REQ:
    {
        gap_l2cap_connParamUpdateReqEvt_t *pEvt = (gap_l2cap_connParamUpdateReqEvt_t *)para;

        tlkapi_send_string_data(APP_LOG_EN, "[APP] Conn Update Request, not accept", pEvt, sizeof(gap_l2cap_connParamUpdateReqEvt_t));
        //blc_l2cap_SendConnParamUpdateResponse(pEvt->connHandle, pEvt->id, CONN_PARAM_UPDATE_REJECT);

    } break;

    default:
        break;
    }

    return 0;
}


    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
int app_simple_sdp_infor_store_device_acl_cen_index(u32 current_flash_adr, void *p)
{
    dev_char_info_t *pdev_char = (dev_char_info_t *)p;

    u8 device_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(pdev_char->conn_handle);

    //  if(device_idx <= ACLCEN_IDX_KEYBOARD){
    //      if(current_flash_adr >= FLASH_SDP_ATT_ADRRESS && current_flash_adr < (FLASH_SDP_ATT_ADRRESS + FLASH_SDP_ATT_MAX_SIZE)){
    //          flash_write_page( current_flash_adr + OFFSETOF(dev_att_t, rsvd),  1, &device_idx);
    //          //tlkapi_send_string_data(APP_LOG_EN, "[APP] save device_idx in flash", &device_idx, 1);
    //      }
    //  }
    //  else{
    //      tlkapi_send_string_data(APP_LOG_EN, "[APP] device_idx ERROR when store SDP information", &device_idx, 1);
    //  }

    return 0;
}
    #endif

//#define           HID_HANDLE_KEYBOARD_REPORT          29
//#define           HID_HANDLE_MOUSE_REPORT
/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt)
{
    if (dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL) //GATT data for ACL Central
    {
        u8 cur_aclCentral_idx = blc_ll_getAclCentralIndex_arrangedTaskTiming_diffMode(connHandle);
        if (cur_aclCentral_idx <= ACLCEN_IDX_KEYBOARD) {
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
            if (central_sdp_pending == connHandle) { //ATT service discovery is ongoing on this conn_handle
                //when service discovery function is running, all the ATT data from peripheral
                //will be processed by it,  user can only send your own att cmd after  service discovery is over
                ble_att_readByTypeRsp_t *pData = (ble_att_readByTypeRsp_t *)pkt;
                tlkapi_send_string_data(APP_SIMPLE_SDP_LOG_EN, "[APP][SDP] SDP RX data", &pData->l2capLen, pData->l2capLen + 4);

                host_att_client_handler(connHandle, pkt); //handle this ATT data by service discovery process
            }
    #endif

            rf_packet_att_t *pAtt = (rf_packet_att_t *)pkt;

            //so any ATT data before service discovery will be dropped
            dev_char_info_t *dev_info = dev_char_info_search_by_connhandle(connHandle);
            if (dev_info) {
                //-------   user process ------------------------------------------------
                u16 attHandle = pAtt->handle;

                if (pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI) {
                    //---------------   keyboard key --------------------------
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    if (attHandle == dev_info->char_handle[3]) {
                        tlkapi_send_string_data(APP_LOG_EN, "[UI] consumer report", pAtt->dat, 2);
                    } else if (attHandle == dev_info->char_handle[4]) // Key Report In
                    {
                        att_keyboard(connHandle, pAtt->dat);
                    } else if (attHandle == dev_info->char_handle[5]) // mouse Report In
                    {
                        att_mouse(connHandle, pAtt->dat);
                    }
    #endif
                } else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND) {
                }
            }

            /* The Central does not support GATT Server by default */
            if (!(pAtt->opcode & 0x01)) {
                switch (pAtt->opcode) {
                case ATT_OP_FIND_INFO_REQ:
                case ATT_OP_FIND_BY_TYPE_VALUE_REQ:
                case ATT_OP_READ_BY_TYPE_REQ:
                case ATT_OP_READ_BY_GROUP_TYPE_REQ:
                    blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_ATTR_NOT_FOUND);
                    break;
                case ATT_OP_READ_REQ:
                case ATT_OP_READ_BLOB_REQ:
                case ATT_OP_READ_MULTI_REQ:
                case ATT_OP_WRITE_REQ:
                case ATT_OP_PREPARE_WRITE_REQ:
                    blc_gatt_pushErrResponse(connHandle, pAtt->opcode, pAtt->handle, ATT_ERR_INVALID_HANDLE);
                    break;
                case ATT_OP_EXECUTE_WRITE_REQ:
                case ATT_OP_HANDLE_VALUE_CFM:
                case ATT_OP_WRITE_CMD:
                case ATT_OP_SIGNED_WRITE_CMD:
                    //ignore
                    break;
                default: //no action
                    break;
                }
            }
        }
    } else { //GATT data for ACL Peripheral
    }


    return 0;
}

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
    //////////////////////////// basic hardware Initialization  Begin //////////////////////////////////
    /* random number generator must be initiated here( in the beginning of user_init_normal).
     * When deepSleep retention wakeUp, no need initialize again */
    random_generator_init();

    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_init();
    blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();
    //////////////////////////// basic hardware Initialization  End /////////////////////////////////

    //////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8 mac_public[6];
    u8 mac_random_static[6];

    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public); //mandatory

    blc_ll_initExtendedScanning_module();
    blc_ll_initExtendedInitiating_module();

    blc_ll_initAclConnection_module();
    blc_ll_initAclCentralRole_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);


    /******* special design for ACL Central timing customization *******/
    blc_ll_setAclCentralTaskTimingArrangement(ACLC_SET_ARNG_ENABLE, ACLC_SLOT_DURN_DIFF, 0);

    blc_ll_setAclCentralTimingPositionSlotNumber_for_diffMode(ACLCEN_IDX_MOUSE, ACLC_TIMPOSN_IDX0, 3);
    blc_ll_setAclCentralTimingPositionSlotNumber_for_diffMode(ACLCEN_IDX_KEYBOARD, ACLC_TIMPOSN_IDX1, 3);
    blc_ll_setAclCentralTimingPositionSlotNumber_for_diffMode(ACLCEN_IDX_CIS, ACLC_TIMPOSN_IDX1, 3);
    /*******************************************************************/


    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);

    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_10MS);

    rf_set_power_level_index(RF_POWER_P9dBm);

    //////////// LinkLayer Initialization  End /////////////////////////

    ///// CIS Central initialization //////////////
    blc_ll_initCisCentralModule_initCigParametersBuffer(app_cig_param, APP_CIG_NUMBER);

    blc_ll_initCisConnModule_initCisConnParametersBuffer(app_cis_conn_param, APP_CIS_CENTRAL_NUMBER, APP_CIS_PERIPHR_NUMBER);


    /******* special design for CIS Central timing customization *******/
    blc_ll_setCigTimingOffsetOfAclCentral(ACLCEN_IDX_CIS, 1875);
    /*******************************************************************/

    /* CIS RX PDU buffer initialization */
    blc_ll_initCisRxFifo(app_cis_rxPduFifo, CIS_RX_PDU_FIFO_SIZE, CIS_RX_PDU_FIFO_NUM);
    /* CIS TX PDU buffer initialization */
    blc_ll_initCisTxFifo(app_cis_txPduFifo, CIS_TX_PDU_FIFO_SIZE, CIS_TX_PDU_FIFO_NUM);

    /* CIS SDU in & out buffer initialization */
    blc_ll_initCisSduBuffer(app_cis_sdu_in_fifo, CIS_SDU_IN_FIFO_SIZE, CIS_SDU_IN_FIFO_NUM, app_cis_sdu_out_fifo, CIS_SDU_OUT_FIFO_SIZE, CIS_SDU_OUT_FIFO_NUM);

    blc_ll_setCisSupplementPDUStrategy(CIS_PDU_STRATEGY0);

    blc_iso_enableSduToHostTimestamp(1);
    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE |
                             HCI_EVT_MASK_ENCRYPTION_CHANGE);
    blc_hci_setEventMask_2_cmd(HCI_EVT_MASK_ENCRYPTION_KEY_REFRESH_COMPLETE);
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE | HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CIS_ESTABLISHED | HCI_LE_EVT_MASK_CIS_REQUESTED);

    u8 error_code = blc_contr_checkControllerInitialization();
    if (error_code != INIT_SUCCESS) {
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
    #endif
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_send_string_data(APP_LOG_EN, "Controller Init ERROR:", &error_code, 1);
        while (1) {
            tlkapi_debug_handler();
        }
    #else
        while (1)
            ;
    #endif
    }
    //////////// HCI Initialization  End /////////////////////////


    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, app_cen_l2cap_tx_buf, CENTRAL_L2CAP_BUFF_SIZE);
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize(64);

    /* GATT Initialization */
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    host_att_register_idle_func(main_idle_loop);
    simple_sdp_register_store_info_callback(app_simple_sdp_infor_store_device_acl_cen_index);
    #endif

    blc_gatt_register_data_handler(app_gatt_data_handler);

    /* SMP Initialization */
    #if (ACL_CENTRAL_SMP_ENABLE)

    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif
    #if (ACL_CENTRAL_SMP_ENABLE)
    blc_smp_setSecurityLevel_central(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
    #else
    blc_smp_setSecurityLevel(No_Security);
    #endif

    blc_smp_smpParamInit();


    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler(app_host_event_callback);
    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN |
                         GAP_EVT_MASK_SMP_PAIRING_SUCCESS |
                         GAP_EVT_MASK_SMP_PAIRING_FAIL |
                         GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE |
                         GAP_EVT_MASK_L2CAP_CONN_PARAM_UPDATE_REQ);
    //////////// Host Initialization  End /////////////////////////

    //////////////////////////// BLE stack Initialization  End //////////////////////////////////


    //////////////////////////// User Configuration for BLE application ////////////////////////////
    blc_ll_setExtScanParam(OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M_CODED, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_WINDOW_90MS, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_WINDOW_90MS);

    blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);

    ////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////

    app_audio_init();

    tlkapi_send_string_data(APP_LOG_EN, "[APP] kmlea dongle init", NULL, 0);
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{
}

/////////////////////////////////////////////////////////////////////
// main loop flow
/////////////////////////////////////////////////////////////////////
u32 ledToggleTick = 0;

/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop(void)
{
    #if 0
    static u32 loop_cnt = 0;
    if(clock_time_exceed(ledToggleTick, 2000 * 1000))
    {  //led toggle interval: 1000mS
        ledToggleTick = clock_time();
        loop_cnt ++;
        //gpio_toggle(GPIO_LED_RED);
        tlkapi_send_string_data(APP_LOG_EN, "[APP] mainloop", &loop_cnt, 4);
    }
    #endif

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    app_audio_handler();

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
    #endif

    proc_central_role_unpair();

    return 0; //must return 0 due to SDP flow
}

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
    APP_DBG_CHN_0_HIGH;
    main_idle_loop();

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    simple_sdp_loop();
    #endif

    APP_DBG_CHN_0_LOW;
}


#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)
