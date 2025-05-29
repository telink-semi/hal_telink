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
#include "algorithm/hadm/gcc10/cs_cal.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "app_buffer.h"
#include "app_ui.h"
#include "app_parse_ui.h"
#include "application/usbstd/usb.h"
#include "app_parse_char.h"
#include "app_cs.h"

#include "types.h"

#if (INTER_TEST_MODE == TEST_RAS_CLIENT)

#if (RAS_OOB)
u16 pinCodeReq_handle;
_attribute_ble_data_retention_  AppScOobCtrl_t  appScOobCtrl;
smp_sc_oob_data_t remoteOob;
#endif

int central_smp_pending      = 0; // SMP: security & encryption;
int central_connected_led_on = 0;

const char *hex_to_str_nospace(const void *buf, u8 len)
{
    static const char hex[] = "0123456789abcdef";
    static char       str[301];
    const uint8_t    *b = buf;
    u8                i;

    len = min(len, (sizeof(str) - 1) / 2);

    for (i = 0; i < len; i++) {
        str[i * 2]     = hex[b[i] >> 4];
        str[i * 2 + 1] = hex[b[i] & 0xf];
    }

    str[i * 2] = '\0';

    return str;
}

/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int AA_dbg_adv_rpt = 0;
u32 tick_adv_rpt   = 0;

int app_le_adv_report_event_handle(u8 *p)
{
    event_adv_report_t *pa   = (event_adv_report_t *)p;
    s8                  rssi = pa->data[pa->len];

#if 0 //debug, print ADV report number every 5 seconds
        AA_dbg_adv_rpt ++;
        if(clock_time_exceed(tick_adv_rpt, 5000000)){
            tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Adv report", pa->mac, 6);
            tick_adv_rpt = clock_time();
        }
#endif

    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
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

    int central_auto_connect = 0;
    int user_manual_pairing  = 0;

    //manual pairing methods 1: key press triggers
    user_manual_pairing = central_pairing_enable && (rssi > -66); //button trigger pairing(RSSI threshold, short distance)

#if (ACL_CENTRAL_SMP_ENABLE)
    central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
#elif (ACL_CENTRAL_CUSTOM_PAIR_ENABLE)
        //search in peripheral mac_address table to find whether this device is an old device which has already paired with central
    central_auto_connect = user_tbl_peripheral_mac_search(pa->adr_type, pa->mac);
#endif

#if (UI_CONTROL_ENABLE && CS_PROCEDURE_CMD_TRIG)
    if (reconn_en == 0) {
        central_auto_connect = 0;
    }
#endif

    if (central_auto_connect || user_manual_pairing) {
        /* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
         * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
         * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
         * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
        u8 status = blc_ll_createConnection(SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY, pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, 0, 0xFFFF);

        if (status == BLE_SUCCESS) { //create connection success
#if (ACL_CENTRAL_CUSTOM_PAIR_ENABLE)
            // for Telink referenced pair&bonding,
            if (user_manual_pairing && !central_auto_connect) { //manual pair but not auto connect
                blm_manPair.manual_pair = 1;
                blm_manPair.mac_type    = pa->adr_type;
                memcpy(blm_manPair.mac, pa->mac, 6);
                blm_manPair.pair_tick = clock_time();
            }
#endif
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] create connection success", pa->mac, 6);
        } else {
            //tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] create connection fail", &status, 1);
        }
    }
    /*********************** Central Create connection demo code end  *******************************************************/


    return 0;
}

static int app_le_ext_adv_report_event_handle(u8 *p, int evt_data_len)
{
    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;
    int offset = 0;
    extAdvEvt_info_t *pExtAdvInfo = NULL;
    u8 status;

    for (int i = 0; i<pExtAdvRpt->num_reports; i++)
    {
        pExtAdvInfo = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
        s8 rssi = pExtAdvInfo->rssi;
        u8 ext_evtType = pExtAdvInfo->event_type & EXTADV_RPT_EVTTYPE_MASK;
        u8 conn_adv_flag = 0;
        /* Extended ADV */

        app_parse_foundExtAdv(pExtAdvInfo);

        if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_DIRECTED)
        {
            conn_adv_flag = 1;  //Extended
            /* Extended, Connectable Undirected */
        } else if (ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_IND || ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_DIRECT_IND) {
            conn_adv_flag = 2;  //Legacy
        }

        if(conn_adv_flag)
        {
            /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
            if (central_smp_pending) {      //if previous connection SMP not finish, can not create a new connection
                return 1;
            }

            if (central_disconnect_connhandle) { //one ACL connection central role is in un_pair disconnection flow, do not create a new one
                return 1;
            }
        }
    }
    return 0;
}

/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_complete_event_handle(u8 *p)
{
    hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

    if (pConnEvt->status == BLE_SUCCESS) {
        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection complete event", &pConnEvt->connHandle, sizeof(hci_le_connectionCompleteEvt_t) - 2);
#if (UI_LED_ENABLE)
        //led show connection state
        central_connected_led_on = 1;
        gpio_write(GPIO_LED_RED, LED_ON_LEVEL);    //red on
        gpio_write(GPIO_LED_WHITE, !LED_ON_LEVEL); //white off
#endif

        dev_char_info_insert_by_conn_event(pConnEvt);

        /*
         * For CS tool show version.
         */
        app_parse_printf("CS version_%s\r\n", blc_get_cs_lib_version());

        if (pConnEvt->role == ACL_ROLE_CENTRAL)         // central role, process SMP and SDP if necessary
        {
#if (ACL_CENTRAL_SMP_ENABLE)
            central_smp_pending = pConnEvt->connHandle; // this connection need SMP
#elif (ACL_CENTRAL_CUSTOM_PAIR_ENABLE)
            //manual pairing, device match, add this device to peripheral mac table
            if (blm_manPair.manual_pair && blm_manPair.mac_type == pConnEvt->peerAddrType && !memcmp(blm_manPair.mac, pConnEvt->peerAddr, 6)) {
                blm_manPair.manual_pair = 0;
                user_tbl_peripheral_mac_add(pConnEvt->peerAddrType, pConnEvt->peerAddr);
            }
#endif

#if CS_PROCEDURE_EXCHANGE
            u8 index = 0;
            if (user_addCsCtrlByHadle(pConnEvt->connHandle)) {
                index = user_getCsCtrlByHadle(pConnEvt->connHandle);
                if (index == 0) {
                    //err
                    tlkapi_printf(APP_CS_LOG_EN, "[APP]CS] get cs ctrl idx error.\r\n");
                } else {
                    index = index - 1;
                }
            } else {
                //error,buffer not enough
                tlkapi_printf(APP_CS_LOG_EN, "[APP]CS] add cs ctrl error.\r\n");
            }
            cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time() | 1;
            user_setCsProcStartStatus(index, CAP_EXCH);
            cs_app_ctrl.cs_ctrl[index].acl_role = pConnEvt->role;
    #if UI_CONTROL_ENABLE
            app_parse_printf("wait cs capability exchange\r\n");
    #endif
#endif
#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
            memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
            cur_sdp_device.conn_handle  = pConnEvt->connHandle;
            cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
            memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

            u8         temp_buff[sizeof(dev_att_t)];
            dev_att_t *pdev_att = (dev_att_t *)temp_buff;

            /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
            if (dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att)) {
                //cur_sdp_device.char_handle[1] =                                   //Speaker
                cur_sdp_device.char_handle[2] = pdev_att->char_handle[2]; //OTA
                cur_sdp_device.char_handle[3] = pdev_att->char_handle[3]; //consume report
                cur_sdp_device.char_handle[4] = pdev_att->char_handle[4]; //normal key report
                //cur_sdp_device.char_handle[6] =                                   //BLE Module, SPP Server to Client
                //cur_sdp_device.char_handle[7] =                                   //BLE Module, SPP Client to Server

                /* add the peer device att_handle value to conn_dev_list */
                dev_char_info_add_peer_att_handle(&cur_sdp_device);
            } else {
                central_sdp_pending = pConnEvt->connHandle; // mark this connection need SDP

    #if (ACL_CENTRAL_SMP_ENABLE)
                    //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
    #else
                app_register_service(&app_service_discovery); //No SMP, service discovery can initiated now
    #endif
            }
#endif
        }
    }

    return 0;
}

static void app_le_enhanced_connection_complete_event_handle(u8 *p)
{
    hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

    tlkapi_printf(APP_LOG_EN,"[APP][EVT] le_connection_complete connHandle:%04X mac:%02X %02X %02X %02X %02X %02X",\
            pConnEvt->connHandle,pConnEvt->PeerAddr[0],pConnEvt->PeerAddr[1],pConnEvt->PeerAddr[2],\
            pConnEvt->PeerAddr[3],pConnEvt->PeerAddr[4],pConnEvt->PeerAddr[5]);

    if (pConnEvt->status != BLE_SUCCESS) {
        return;
    }

#if (UI_LED_ENABLE)
    //led show connection state
    central_connected_led_on = 1;
    gpio_write(GPIO_LED_RED, LED_ON_LEVEL);    //red on
    gpio_write(GPIO_LED_WHITE, !LED_ON_LEVEL); //white off
#endif

    dev_char_info_insert_by_enhanced_conn_event(pConnEvt);

    if (pConnEvt->role == ACL_ROLE_CENTRAL)         // central role, process SMP and SDP if necessary
    {
#if (ACL_CENTRAL_SMP_ENABLE)
        central_smp_pending = pConnEvt->connHandle; // this connection need SMP
#endif

#if CS_PROCEDURE_EXCHANGE
        u8 index = 0;
        if (user_addCsCtrlByHadle(pConnEvt->connHandle)) {
            index = user_getCsCtrlByHadle(pConnEvt->connHandle);
            if (index == 0) {
                //err
                tlkapi_printf(APP_CS_LOG_EN, "[APP]CS] get cs ctrl idx error.\r\n");
            } else {
                index = index - 1;
            }
        } else {
            //error,buffer not enough
            tlkapi_printf(APP_CS_LOG_EN, "[APP]CS] add cs ctrl error.\r\n");
        }
        cs_app_ctrl.cs_ctrl[index].exchange_tick = clock_time() | 1;
        user_setCsProcStartStatus(index, CAP_EXCH);
        cs_app_ctrl.cs_ctrl[index].acl_role = pConnEvt->role;
    #if UI_CONTROL_ENABLE
        app_parse_printf("wait cs capability exchange\r\n");
    #endif
#endif
#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
        cur_sdp_device.conn_handle  = pConnEvt->connHandle;
        cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
        memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

        u8         temp_buff[sizeof(dev_att_t)];
        dev_att_t *pdev_att = (dev_att_t *)temp_buff;

        /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
        if (dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att)) {
            //cur_sdp_device.char_handle[1] =                                   //Speaker
            cur_sdp_device.char_handle[2] = pdev_att->char_handle[2]; //OTA
            cur_sdp_device.char_handle[3] = pdev_att->char_handle[3]; //consume report
            cur_sdp_device.char_handle[4] = pdev_att->char_handle[4]; //normal key report
            //cur_sdp_device.char_handle[6] =                                   //BLE Module, SPP Server to Client
            //cur_sdp_device.char_handle[7] =                                   //BLE Module, SPP Client to Server

            /* add the peer device att_handle value to conn_dev_list */
            dev_char_info_add_peer_att_handle(&cur_sdp_device);
        } else {
            central_sdp_pending = pConnEvt->connHandle; // mark this connection need SDP

#if (ACL_CENTRAL_SMP_ENABLE)
                //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
#else
            app_register_service(&app_service_discovery); //No SMP, service discovery can initiated now
#endif
        }
#endif
    }
}

/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
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

#if (UI_LED_ENABLE)
    //led show none connection state
    if (central_connected_led_on) {
        central_connected_led_on = 0;
        gpio_write(GPIO_LED_WHITE, LED_ON_LEVEL); //white on
        gpio_write(GPIO_LED_RED, !LED_ON_LEVEL);  //red off
    }
#endif

    //if previous connection SMP & SDP not finished, clear flag
#if (ACL_CENTRAL_SMP_ENABLE)
    if (central_smp_pending == pDisConn->connHandle) {
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
#if CS_PROCEDURE_EXCHANGE
    blc_cs_resetByHandle(pDisConn->connHandle);
    user_clrCsCtrlByHadle(pDisConn->connHandle);

    #if(!CS_PROCEDURE_CMD_TRIG)
    if (blc_smp_param_getCurrentBondingDeviceNumber(1, 0)) {
        blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    } else {
        blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
    }
    #endif
        #if UI_CONTROL_ENABLE
    reconn_en = 0;
        #endif
#endif
    dev_char_info_delete_by_connhandle(pDisConn->connHandle);


    return 0;
}

/**
 * @brief      BLE Connection update complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_connection_update_complete_event_handle(u8 *p)
{
    hci_le_connectionUpdateCompleteEvt_t *pUpt = (hci_le_connectionUpdateCompleteEvt_t *)p;
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection Update Event", &pUpt->connHandle, 8);

    if (pUpt->status == BLE_SUCCESS) {
    }

    return 0;
}

//////////////////////////////////////////////////////////
// event call back
//////////////////////////////////////////////////////////
/**
 * @brief      BLE controller event handler call-back.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_controller_event_callback(u32 h, u8 *p, int n)
{
    (void)n;
    if (h & HCI_FLAG_EVENT_BT_STD) //Controller HCI event
    {
        u8 evtCode = h & 0xff;

        //------------ disconnect -------------------------------------
        if (evtCode == HCI_EVT_DISCONNECTION_COMPLETE) //connection terminate
        {
            app_disconnect_event_handle(p);
        } else if (evtCode == HCI_EVT_LE_META)         //LE Event
        {
            u8  subEvt_code = p[0];
            u16 handle      = p[1] | (p[2] << 8);
#if CS_PROCEDURE_EXCHANGE
            u8 index = user_getCsCtrlByHadle(handle);
            if (index == 0) {
                //err
            } else {
                index = index - 1;
            }
#endif
            //------hci le event: le connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE) // connection complete
            {
                app_le_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT) // ADV packet
            {
//after controller is set to scan state, it will report all the adv packet it received by this event
#if UI_CONTROL_ENABLE
                app_parse_foundAdv(p);
#endif
                app_le_adv_report_event_handle(p);
            }
            //--------hci le event: le ext adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT)
            {
                // app_parse_printf("ext adv rep\r\n");
                // tlkapi_send_string_data(APP_LOG_EN, "ext adv", p, sizeof(extAdvEvt_info_t));
                app_le_ext_adv_report_event_handle(p, n);
            }
            //------hci le event: le enhanced_connection complete event---------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE)  // connection complete
            {
                app_le_enhanced_connection_complete_event_handle(p);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE) // connection update
            {
                app_le_connection_update_complete_event_handle(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_DATA_LENGTH_CHANGE) {
                hci_le_dataLengthChangeEvt_t *pDle = (hci_le_dataLengthChangeEvt_t *)p;
                tlkapi_send_string_data(APP_LOG_EN, "dle change", pDle, sizeof(hci_le_dataLengthChangeEvt_t));
            }
#if CS_PROCEDURE_EXCHANGE
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE) {
                user_setCsProcCmpltStatus(index, CAP_EXCH_CMPLT);
    #if UI_CONTROL_ENABLE
                app_parse_printf("cs capability exchange success\r\n");
    #endif
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE) {
                user_setCsProcCmpltStatus(index, FAE_EXCH_CMPLT);
    #if UI_CONTROL_ENABLE
                app_parse_printf("cs fae exchange success\r\n");
    #endif
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE) {
                user_setCsProcCmpltStatus(index, CFG_EXCH_CMPLT);
    #if UI_CONTROL_ENABLE
                app_parse_printf("cs cfg exchange success\r\n");
    #endif

                app_le_cs_config_complete_event_handle(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE) {
                user_setCsProcCmpltStatus(index, SEC_EXCH_CMPLT);
    #if UI_CONTROL_ENABLE
                app_parse_printf("cs sec exchange success\r\n");
    #endif
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE) {
                user_setCsProcCmpltStatus(index, CS_PROC_EN_CMPLT);

    #if UI_CONTROL_ENABLE
                app_parse_printf("cs measurement start\r\n");

                app_le_cs_procedure_enable_complete_event_handle(p);
    #endif
            }

            //------HCI LE event: LE CS Subevent Result event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT) {
    #if (APP_CS_SUBEVENT_LOG_EN)
                hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;

                /* print subevent data to txt file to calculate distance with other company */
                u8 tempBuff[258];
                tempBuff[0] = 0x04; //type
                tempBuff[1] = 0x3E; //event_code
                tempBuff[2] = n;    // total_len
                smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);

            #if(GOOGLE_CS_INIT_ROLE_EN)
                app_parse_printf("Procedure count:[%d], subevent len:[%d]\r\n",pCsSubevent->Procedure_Counter, n);
                app_parse_printf("cs subevent:\n");
                for(int i = 0; i< n+3; i++){
                    if(tempBuff[i] < 0x10){
                        app_parse_printf("0%x ", tempBuff[i]);
                   }else{
                        app_parse_printf("%2x ", tempBuff[i]);
                    }
                }
                app_parse_printf("\n");
            #else
                tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "subevent len***:%d\r\n", n);
                tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs subevent", tempBuff, n + 3);
            #endif
    #endif
                app_le_cs_subevent_result_event_handle(p);
            }
            //------HCI LE event: LE CS Subevent Result Continue event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE) {
    #if (APP_CS_SUBEVENT_LOG_EN)
                hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;

                /* print subevent continue data to txt file to calculate distance with other company */
                u8 tempBuff[258];
                tempBuff[0] = 0x04;
                tempBuff[1] = 0x3E; //event_code
                tempBuff[2] = n;    // total_len
                smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);

            #if(GOOGLE_CS_INIT_ROLE_EN)
                app_parse_printf("continue subevent len:%d\r\n", n);
                app_parse_printf("cs continue subevent:\r\n");
                for(int i = 0; i< n+3; i++){
                    if(tempBuff[i] < 0x10){
                        app_parse_printf("0%x ", tempBuff[i]);
                    }else{
                        app_parse_printf("%2x ", tempBuff[i]);
                    }
                }
                app_parse_printf("\n");
            #else
                tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "continue subevent len***:%d\r\n", n);
                tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs continue subevent", tempBuff, n + 3);
            #endif
    #endif
                app_le_cs_subevent_result_continue_event_handle(p);
            }
#endif
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
    (void)n;

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
        gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;

        if (dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL) {
            if (central_smp_pending == p->connHandle) {
                central_smp_pending = 0;
            }
        }
#endif
    } break;

    case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
    {
    } break;

    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
        gap_smp_connEncDoneEvt_t *p = (gap_smp_connEncDoneEvt_t *)para;

#if (ACL_CENTRAL_SMP_ENABLE)
        if (dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL) {
            if (central_smp_pending == p->connHandle) {
                central_smp_pending = 0;
            }
        }
#endif
#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)                       //SMP finish
        if (central_sdp_pending == p->connHandle) {       //SDP is pending
            app_register_service(&app_service_discovery); //start SDP now
        }
#endif
    } break;

    case GAP_EVT_SMP_TK_DISPLAY:
    {
    } break;

    case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
    {
    } break;

    case GAP_EVT_SMP_TK_REQUEST_OOB:
    {
    } break;
#if (RAS_OOB)
    case GAP_EVT_SMP_REQUEST_SCOOB_DATA:
    {
        gap_smp_requestScOobDataEvt_t* pEvt = (gap_smp_requestScOobDataEvt_t*)para;
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] SC OOB request data", &pEvt->connHandle, sizeof(gap_smp_requestScOobDataEvt_t));

        pinCodeReq_handle = pEvt->connHandle;
        tlkapi_printf(APP_SMP_LOG_EN, "[APP][SMP] SC OOB scOobLocalUsed %d", pEvt->scOobLocalUsed);
        tlkapi_printf(APP_SMP_LOG_EN, "[APP][SMP] SC OOB scOobRemoteUsed %d", pEvt->scOobRemoteUsed);

        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Send Local SC OOB data-c(be) (by UART simulate OOB)", appScOobCtrl.scoob_local.confirm, 16);
        tlkapi_send_string_data(APP_SMP_LOG_EN, "                      SC OOB data-r(be) (by UART simulate OOB)", appScOobCtrl.scoob_local.random, 16);

        tlkapi_printf(APP_SMP_LOG_EN, "[APP][SMP] SC OOB not receive remote data, waiting...");

        memcpy(appScOobCtrl.scoob_remote.random, remoteOob.random, 16);
        memcpy(appScOobCtrl.scoob_remote.confirm, remoteOob.confirm, 16);

        tlkapi_send_string_data(APP_SMP_LOG_EN, "Remote data-c(be)", appScOobCtrl.scoob_remote.confirm, 16);
        tlkapi_send_string_data(APP_SMP_LOG_EN, "Remote data-r(be)", appScOobCtrl.scoob_remote.random, 16);

        tlkapi_printf(APP_SMP_LOG_EN, "our pubkey= %s", hex_to_str(appScOobCtrl.scoob_local_key.public_key, 64));
        tlkapi_printf(APP_SMP_LOG_EN, "our privkey=%s", hex_to_str(appScOobCtrl.scoob_local_key.private_key, 32));

        blc_smp_setScOobData(pinCodeReq_handle, &appScOobCtrl.scoob_local, &appScOobCtrl.scoob_local_key, &appScOobCtrl.scoob_remote);
    }
    break;
#endif

    case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
    {
    } break;

    case GAP_EVT_ATT_EXCHANGE_MTU:
    {
    } break;

    case GAP_EVT_GATT_HANDLE_VALUE_CONFIRM:
    {
    } break;

    default:
        break;
    }

    return 0;
}


#if (AUTO_CALIB_CHIP_INTERNAL_DELAY_EN)
    #define CFO_CALIB_REFLECTOR_COMPLETE 69
#endif


#if (BATT_CHECK_ENABLE) //battery check must do before OTA relative operation

_attribute_data_retention_ u32 lowBattDet_tick = 0;

/**
 * @brief       this function is used to process battery power.
 *              The low voltage protection threshold 2.0V is an example and reference value. Customers should
 *              evaluate and modify these thresholds according to the actual situation. If users have unreasonable designs
 *              in the hardware circuit, which leads to a decrease in the stability of the power supply network, the
 *              safety thresholds must be increased as appropriate.
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void user_battery_power_check(u16 alarm_vol_mv)
{
    /*For battery-powered products, as the battery power will gradually drop, when the voltage is low to a certain
      value, it will cause many problems.
        a) When the voltage is lower than operating voltage range of chip, chip can no longer guarantee stable operation.
        b) When the battery voltage is low, due to the unstable power supply, the write and erase operations
            of Flash may have the risk of error, causing the program firmware and user data to be modified abnormally,
            and eventually causing the product to fail. */
    u8 battery_check_returnValue = 0;
    if (analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG) {
        battery_check_returnValue = app_battery_power_check(alarm_vol_mv + 200);
    } else {
        battery_check_returnValue = app_battery_power_check(alarm_vol_mv);
    }
    if (battery_check_returnValue) {
        analog_write_reg8(USED_DEEP_ANA_REG, analog_read_reg8(USED_DEEP_ANA_REG) & (~LOW_BATT_FLG)); //clr
    } else {
    #if (UI_LED_ENABLE)                                                                              //led indicate
        for (int k = 0; k < 3; k++) {
            gpio_write(GPIO_LED_BLUE, LED_ON_LEVEL);
            sleep_us(200000);
            gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
            sleep_us(200000);
        }
    #endif
        analog_write_reg8(USED_DEEP_ANA_REG, analog_read_reg8(USED_DEEP_ANA_REG) | LOW_BATT_FLG); //mark

    #if (UI_KEYBOARD_ENABLE)
        u32 pin[] = KB_DRIVE_PINS;
        for (unsigned int i = 0; i < (sizeof(pin) / sizeof(*pin)); i++) {
            cpu_set_gpio_wakeup(pin[i], 1, 1);              //drive pin pad high wakeup deepsleep
        }

        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0); //deepsleep
    #endif
    }
}

#endif


#if (APP_FLASH_PROTECTION_ENABLE)

/**
 * @brief      flash protection operation, including all locking & unlocking for application
 *             handle all flash write & erase action for this demo code. use should add more more if they have more flash operation.
 * @param[in]  flash_op_evt - flash operation event, including application layer action and stack layer action event(OTA write & erase)
 *             attention 1: if you have more flash write or erase action, you should should add more type and process them
 *             attention 2: for "end" event, no need to pay attention on op_addr_begin & op_addr_end, we set them to 0 for
 *                          stack event, such as stack OTA write new firmware end event
 * @param[in]  op_addr_begin - operating flash address range begin value
 * @param[in]  op_addr_end - operating flash address range end value
 *             attention that, we use: [op_addr_begin, op_addr_end)
 *             e.g. if we write flash sector from 0x10000 to 0x20000, actual operating flash address is 0x10000 ~ 0x1FFFF
 *                  but we use [0x10000, 0x20000):  op_addr_begin = 0x10000, op_addr_end = 0x20000
 * @return     none
 */
_attribute_data_retention_ u16 flash_lockBlock_cmd;

void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end)
{
    (void)op_addr_begin;
    (void)op_addr_end;
    if (flash_op_evt == FLASH_OP_EVT_APP_INITIALIZATION) {
        /* ignore "op addr_begin" and "op addr_end" for initialization event
         * must call "flash protection_init" first, will choose correct flash protection relative API according to current internal flash type in MCU */
        flash_protection_init();

        /* just sample code here, protect all flash area for old firmware and OTA new firmware.
         * user can change this design if have other consideration */
        u32 app_lockBlock = FLASH_LOCK_FW_LOW_512K; //just demo value, user can change this value according to application

        flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);

        if (blc_flashProt.init_err) {
            tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] flash protection initialization error!!!\n");
        }

        tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] initialization, lock flash\n");
        flash_lock(flash_lockBlock_cmd);
    }
    /* add more flash protection operation for your application if needed */
}

#endif


///////////////////////////////////////////

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
    blc_debug_enableStackLog(STK_LOG_SMP_LTK | STK_LOG_HCI_CS | STK_LOG_LL_CS);
#endif

#if (BATT_CHECK_ENABLE)
    /*The SDK must do a quick low battery detect during user initialization instead of waiting
      until the main_loop. The reason for this process is to avoid application errors that the device
      has already working at low power.
      Considering the working voltage of MCU and the working voltage of flash, if the Demo is set below 2.0V,
      the chip will alarm and deep sleep (Due to PM does not work in the current version of B92, it does not go
      into deepsleep), and once the chip is detected to be lower than 2.0V, it needs to wait until the voltage rises to 2.2V,
      the chip will resume normal operation. Consider the following points in this design:
        At 2.0V, when other modules are operated, the voltage may be pulled down and the flash will not
        work normally. Therefore, it is necessary to enter deepsleep below 2.0V to ensure that the chip no
        longer runs related modules;
        When there is a low voltage situation, need to restore to 2.2V in order to make other functions normal,
        this is to ensure that the power supply voltage is confirmed in the charge and has a certain amount of
        power, then start to restore the function can be safer.*/

    user_battery_power_check(2000);
#endif

    blc_readFlashSize_autoConfigCustomFlashSector();

#if (FLASH_4LINE_MODE_ENABLE)
    /* enable flash mode (cmd:1x, addr:4x, data:4x, dummy:6), need parameter flash mid, which is got in blc_readFlashSize_autoConfigCustomFlashSector,
         * so ble_flash_4line_enable must be called after blc_readFlashSize_autoConfigCustomFlashSector()*/
    ble_flash_4line_enable();
#endif

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();

#if (APP_FLASH_PROTECTION_ENABLE)
    app_flash_protection_operation(FLASH_OP_EVT_APP_INITIALIZATION, 0, 0);
    blc_appRegisterStackFlashOperationCallback(app_flash_protection_operation); //register flash operation callback for stack
#endif

    //////////////////////////// basic hardware Initialization  End //////////////////////////////////


    //////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8 mac_public[6];
    u8 mac_random_static[6];
    /* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

    //////////// LinkLayer Initialization  Begin /////////////////////////

    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

    blc_ll_setRandomAddr(mac_random_static);

    blc_ll_initExtendedScanning_module();
    blc_ll_initExtendedInitiating_module();
    // blc_ll_initLegacyScanning_module();
    // blc_ll_initLegacyInitiating_module();


    blc_ll_initAclConnection_module();

    blc_ll_initAclCentralRole_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

#if (RAS_PERSISTENT_FILTER)
    blc_prf_initPairingInfoStoreModule();
#endif

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);

    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_15MS);

    rf_set_power_level_index(RF_POWER_P9dBm);

    blc_ll_configScanEnableStrategy(SCAN_STRATEGY_0);
    //////////// LinkLayer Initialization  End /////////////////////////


    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE | HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT | HCI_LE_EVT_MASK_DATA_LENGTH_CHANGE | HCI_LE_EVT_MASK_2_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE | HCI_LE_EVT_MASK_2_CS_READ_REMOTE_FAE_TABLE_COMPLETE | HCI_LE_EVT_MASK_2_CS_SECURITY_ENABLE_COMPLETE | HCI_LE_EVT_MASK_2_CS_CONFIG_COMPLETE | HCI_LE_EVT_MASK_2_CS_PROCEDURE_ENABLE_COMPLETE);
    blc_hci_le_setEventMask_2_cmd(HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT | HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT_CONTINUE);

    //////////// HCI Initialization  End /////////////////////////


    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, app_cen_l2cap_tx_buf, CENTRAL_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU); ///must be placed after "blc_gap_init"

/* GATT Initialization */
#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    host_att_register_idle_func(main_idle_loop);
#endif
    //  blc_gatt_register_data_handler(app_gatt_data_handler);

/* SMP Initialization */
#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Configure the storage address and size for SMP pairing security information */
    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);

    /* Set the security level for the central role */
    #if (ACL_CENTRAL_SMP_ENABLE)
        #if (RAS_OOB)
            blc_smp_setSecurityLevel_central(Authenticated_LE_Secure_Connection_Pairing_with_Encryption);
            blc_smp_setSecurityParameters_central(Bondable_Mode, 1, LE_Secure_Connection, 1, 0, IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
            blc_smp_setEcdhDebugMode_central(debug_mode);
        #else
            /* Enable unauthenticated pairing with encryption (LE Security Mode 1, Level 2) */
            blc_smp_setSecurityLevel_central(Unauthenticated_Pairing_with_Encryption); // Equivalent to LE_Security_Mode_1_Level_2
        #endif
    #else
    /* Disable security for the central role */
    blc_smp_setSecurityLevel_central(No_Security);

        #if (ACL_CENTRAL_CUSTOM_PAIR_ENABLE)
    user_central_host_pairing_management_init(); //Telink referenced pairing&bonding without standard pairing in BLE Spec
        #endif
    #endif
    /* Set the security level for the peripheral role */
    #if (ACL_PERIPHR_SMP_ENABLE)
    /* Enable unauthenticated pairing with encryption (LE Security Mode 1, Level 2) */
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption); // Equivalent to LE_Security_Mode_1_Level_2
    #else
    /* Disable security for the peripheral role */
    blc_smp_setSecurityLevel_periphr(No_Security);
    #endif

#if (RAS_OOB)
    blc_smp_enableOobAuthentication(1);
    if(!blc_smp_generateScOobData(&appScOobCtrl.scoob_local, &appScOobCtrl.scoob_local_key)){
        tlkapi_printf(APP_SMP_LOG_EN, "Generate local SC OOB data failed");
    } else{
        // 081B // 0-1
        // 3CCFB403A86B 00 //2-7 (6) //8
        // 021C 02 //9-10 //11
        // 1122 //12-13
        // 1473C3537566CBC60E0803DD0521A045 - confirm //14-29
        // 1123 //30-31
        // F0B8AA4A01F3DAF70000000000000000 - random //32-47

        u8 reverse_confirm[16];
        u8 reverse_random[16];
        for(u8 i = 0; i < 16; i++) {
            //reverse
            reverse_confirm[i] = appScOobCtrl.scoob_local.confirm[15-i];
            reverse_random[i] = appScOobCtrl.scoob_local.random[15-i];
        }
        u8 reverse_confirm_str[50];
        u8 reverse_random_str[50];
        memcpy(reverse_confirm_str, hex_to_str_nospace(reverse_confirm, 16), 40);
        memcpy(reverse_random_str, hex_to_str_nospace(reverse_random, 16), 40);

        //tlkapi_printf(APP_SMP_LOG_EN, "081B3CCFB403A86B00021C021122%s1123%s", hex_to_str(appScOobCtrl.scoob_local.confirm, 16), hex_to_str(appScOobCtrl.scoob_local.random, 16));
        tlkapi_printf(APP_SMP_LOG_EN, "081B6BA803B4CF3C00021C021122%s1123%s", reverse_confirm_str, reverse_random_str);
    }
#endif //#if (RAS_OOB)

    /* Initialize SMP parameters */
    blc_smp_smpParamInit();
#endif //#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)

    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler(app_host_event_callback);
#if(RAS_OOB)
    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN |
                         GAP_EVT_MASK_SMP_PAIRING_SUCCESS |
                         GAP_EVT_MASK_SMP_PAIRING_FAIL |
                         GAP_EVT_MASK_SMP_REQUEST_SCOOB_DATA |
                         GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE | GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE);
#else
    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN |
                         GAP_EVT_MASK_SMP_PAIRING_SUCCESS |
                         GAP_EVT_MASK_SMP_PAIRING_FAIL |
                         GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
#endif

    /* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
     * attention: user can not delete !!! */
    u32 error_code1 = blc_contr_checkControllerInitialization();
    u32 error_code2 = blc_host_checkHostInitialization();
    if (error_code1 != INIT_SUCCESS || error_code2 != INIT_SUCCESS) {
/* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
#if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
#endif

#if (TLKAPI_DEBUG_ENABLE)
        tlkapi_printf(APP_LOG_EN, "[APP][INI] Stack INIT ERROR 0x%04x, 0x%04x", error_code1, error_code2);
        while (1) {
            tlkapi_debug_handler();
        }
#else
        while (1)
            ;
#endif
    }

    //////////// Host Initialization  End /////////////////////////

    //////////// Channel Sounding Initialization Start/////////////////////////
    app_channel_sounding_init();
    //////////// Channel Sounding Initialization End /////////////////////////
    //////////////////////////// BLE stack Initialization  End //////////////////////////////////


    //////////////////////////// User Configuration for BLE application ////////////////////////////

    blc_ll_setExtScanParam(OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M_CODED, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS,
        SCAN_WINDOW_100MS, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS);
    // blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);


#if UI_CONTROL_ENABLE
    app_parse_ui_init();
    #if (!CS_PROCEDURE_CMD_TRIG)
    if (blc_smp_param_getCurrentBondingDeviceNumber(1, 0)) {
        // blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
        blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
    }
    #endif
#else
    // blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
#endif

    tlkapi_printf(APP_LOG_EN, "[APP][INI] Initiator demo init finished");
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{
}


#if (AUTO_CALIB_CHIP_INTERNAL_DELAY_EN)

extern void chipDly_trigger_cs_procedure(u8 mode0_cnt, u8 cfo_calFlg);

void chipDly_handle_mainloop(void)
{
    for (int i = 0; i < CS_MAX_NUM; i++) {
        if (!cs_app_ctrl.cs_ctrl[i].connhandle || !cs_app_ctrl.cs_ctrl[i].chipDly_calib_en) {
            continue;
        }

        if (cs_app_ctrl.cs_ctrl[i].chipDly_calib_CFO_cmplt) {
    #define CALIB_TRIGGER_HANDLEER 0x66 //reflector will use that to set CFO value.
            u8 sentToReflCfo[4];

            tlkapi_printf(1, "calibration cfo value=%f\n", cs_app_ctrl.cs_ctrl[i].chipDly_initCalcCfoVal);
            //U32_TO_STREAM(sentToReflCfo, cs_app_ctrl.cs_ctrl[i].chipDly_initCalcCfoVal);
            int calcCfo      = (int)cs_app_ctrl.cs_ctrl[i].chipDly_initCalcCfoVal;
            sentToReflCfo[0] = calcCfo & 0xff;
            sentToReflCfo[1] = (calcCfo >> 8) & 0xff;
            sentToReflCfo[2] = (calcCfo >> 16) & 0xff;
            sentToReflCfo[3] = (calcCfo >> 24) & 0xff;

            if (BLE_SUCCESS != blc_gatt_pushWriteCommand(cs_app_ctrl.cs_ctrl[i].connhandle, CALIB_TRIGGER_HANDLEER, sentToReflCfo, 4)) {
                cs_app_ctrl.cs_ctrl[i].chipDly_calib_CFO_cmplt = 0;
                tlkapi_printf(1, "write cfo fail because fifo is full \n");
            }
        }
        ///step 2:///
        if (cs_app_ctrl.cs_ctrl[i].chipDly_refl_cfo_cmplt) {
            cs_app_ctrl.cs_ctrl[i].chipDly_refl_cfo_cmplt = 0; //clear and avoid to run again.
            cs_app_ctrl.cs_ctrl[i].chipDly_max_proc_cnt   = 100;

            u8 mode0_num = 2;
            u8 isCfoCalc = 0;

            chipDly_trigger_cs_procedure(mode0_num, isCfoCalc);
        }
    }
}
#endif
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
    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();
    blc_prf_main_loop();

////////////////////////////////////// Debug entry /////////////////////////////////
#if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
#endif

////////////////////////////////////// UI entry /////////////////////////////////
#if (BATT_CHECK_ENABLE)
    /*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
         500ms in the demo. Users can modify this time according to their needs.*/
    if (battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000)) {
        lowBattDet_tick = clock_time();
        user_battery_power_check(BAT_DEEP_THRESHOLD_MV);
    }
#endif

#if (UI_BUTTON_ENABLE)
    static u8 button_detect_en = 0;
    if (!button_detect_en && clock_time_exceed(0, 1000000)) { // process button 1 second later after power on
        button_detect_en = 1;
    }
    if (button_detect_en) {
        proc_button(); //button triggers pair & unpair  and OTA
    }
#elif (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
#endif


    proc_central_role_unpair();
#if UI_CONTROL_ENABLE
    app_parse_ui_loop();
#endif
#if CS_PROCEDURE_EXCHANGE
    cs_procedure_ctrl();
#endif

#if (AUTO_CALIB_CHIP_INTERNAL_DELAY_EN)
    chipDly_handle_mainloop();
#endif

    return 0; //must return 0 due to SDP flow
}

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
#if UI_LED_ENABLE
    if (clock_time_exceed(ledToggleTick, 1000 * 1000)) { //led toggle interval: 1000mS
        ledToggleTick = clock_time();
        gpio_toggle(GPIO_LED_BLUE);
    }
#endif
    main_idle_loop();

#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    simple_sdp_loop();
#endif
}
#endif //#if (INTER_TEST_MODE == TEST_RAS_CLIENT)