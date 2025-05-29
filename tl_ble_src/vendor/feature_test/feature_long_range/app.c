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
#include "app.h"
#include "../default_buffer.h"
#include "../default_att.h"
#include "app_ui.h"
#include "oled.h"

#if (FEATURE_TEST_MODE == TEST_LONG_RANGE)

#if (ACL_CENTRAL_SMP_ENABLE)
_attribute_ble_data_retention_      int central_smp_pending = 0;        // SMP: security & encryption
#endif

typedef enum{
    TEST_PHY_1M,
    TEST_PHY_2M,
    TEST_PHY_S2,
    TEST_PHY_S8,
}test_phy_t;

static char *test_phy_name[] = {
    "1M",
    "2M",
    "S2",
    "S8",
};

static char *test_mode_name[] = {
    "UNCONN",
    "CONN"
};

/**
 * @brief   BLE Advertising data
 */
u8    tbl_advData[] = {
    5,  DT_SERVICE_DATA, 0x00, 0x00, 0x00, 0x00
};

#if TEST_ROLE ==  TEST_ROLE_PERIPHR
_attribute_ble_data_retention_  u8  app_extAdvSetParam_buf[ADV_SET_PARAM_LENGTH * APP_EXT_ADV_SETS_NUMBER];
_attribute_iram_noinit_data_    u8  app_extAdvData_buf[APP_EXT_ADV_DATA_LENGTH * APP_EXT_ADV_SETS_NUMBER];

typedef struct{
    uint8_t mode : 1;   /*!< 0-ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED, 1-ADV_EVT_PROP_EXTENDED_CONNECTABLE_DIRECTED*/
    uint8_t phy  : 7;   /*!< phy: 1M, 2M, S2, S8 */
    uint32_t pkt_cnt;   /*!< adv packet counter */
    uint8_t restart_times; /*!< adv restart_times */
}adv_info_t;
adv_info_t adv_info = {
    0,
    TEST_PHY_S8,
    0,
    0
};

void app_switch_test_phy(void)
{
    adv_info.phy++;
    if(adv_info.phy > TEST_PHY_S8)
    {
        adv_info.phy = TEST_PHY_1M;
    }
}

void app_switch_test_mode(void)
{
    adv_info.mode = adv_info.mode == 0 ? 1 : 0;
}

void app_switch_adv_info(void)
{
    blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);

    adv_info.pkt_cnt = 0;
    adv_info.restart_times = 0;

    u16 event_prop = ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED;
    if(adv_info.mode == 0)
        event_prop = ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED;
    else if(adv_info.mode == 1)
        event_prop = ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED;

    le_phy_type_t pri_chn_phy = BLE_PHY_1M;
    le_phy_type_t scn_chn_phy = BLE_PHY_1M;
    le_ci_prefer_t prefer_ci = CODED_PHY_PREFER_NONE;
    switch(adv_info.phy)
    {
    case TEST_PHY_1M:           pri_chn_phy = BLE_PHY_1M;
                                scn_chn_phy = BLE_PHY_1M;
                                prefer_ci = CODED_PHY_PREFER_NONE;
                                break;
    case TEST_PHY_2M:           pri_chn_phy = BLE_PHY_1M;
                                scn_chn_phy = BLE_PHY_2M;
                                prefer_ci = CODED_PHY_PREFER_NONE;
                                break;
    case TEST_PHY_S2:           pri_chn_phy = BLE_PHY_CODED;
                                scn_chn_phy = BLE_PHY_CODED;
                                prefer_ci = CODED_PHY_PREFER_S2;
                                blc_ll_setDefaultConnCodingIndication(prefer_ci);
                                break;
    case TEST_PHY_S8:           pri_chn_phy = BLE_PHY_CODED;
                                scn_chn_phy = BLE_PHY_CODED;
                                prefer_ci = CODED_PHY_PREFER_S8;
                                blc_ll_setDefaultConnCodingIndication(prefer_ci);
                                break;
    default:                    pri_chn_phy = BLE_PHY_1M;
                                scn_chn_phy = BLE_PHY_1M;
                                prefer_ci = CODED_PHY_PREFER_NONE;
                                break;
    }

    blc_ll_setExtAdvParam( ADV_HANDLE0,         event_prop,                                                    ADV_INTERVAL_20MS,               ADV_INTERVAL_20MS,
                           BLT_ENABLE_ADV_ALL,  OWN_ADDRESS_PUBLIC,                                            BLE_ADDR_PUBLIC,                 NULL,
                           ADV_FP_NONE,         TX_POWER_0dBm, /*invaild paras */                               pri_chn_phy,                      0,
                           scn_chn_phy,          ADV_SID_0,                                                     0); 

    smemcpy(tbl_advData + sizeof(tbl_advData) - 4, (u8 *)&adv_info.pkt_cnt, 4);
    blc_ll_setExtAdvData( ADV_HANDLE0, sizeof(tbl_advData), (const u8 *)tbl_advData);

    if(adv_info.mode == 0)
        blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, TEST_PACKET_PER_TIMES);
    else if(adv_info.mode == 1)
        blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);

    snprintf(oled_show_buff, 32, "%-31s", "Telink_Per_Test");
    OLED_ShowStr(2, 0, (uint8_t *)oled_show_buff);
    snprintf(oled_show_buff, 32, "TX Mode: %-22s", test_mode_name[adv_info.mode]);
    OLED_ShowStr(2, 2, (uint8_t *)oled_show_buff);
    snprintf(oled_show_buff, 32, "Phy: %-26s", test_phy_name[adv_info.phy]);
    OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);

    if(adv_info.mode == 0)
    {
        snprintf(oled_show_buff, 32, "Cnt: %ld        ", adv_info.pkt_cnt);
        OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
    }
    else
    {
        snprintf(oled_show_buff, 32, "Advertising");
        OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
    }
}

#elif TEST_ROLE == TEST_ROLE_CENTRAL
typedef struct
{
    uint8_t mode : 1;   /*!< 0-unconn, 1-conn peer device */
    uint8_t phy  : 7;   /*!< phy: 1M, 2M, S2, S8 */
    uint16_t cur_cnt;       /*!< current pkt received */

    uint32_t pkt_cnt;   /*!< current pkt counter */
    uint32_t timeout_ticks;
    uint8_t oled_color_turn_flag;
    uint8_t conn_flag;  /*!< update after conn update evt, used to begin writecmd */
}scan_info_t;
scan_info_t scan_info = {
    0,
    TEST_PHY_S8,
    0,
    0,
    0,
    0,
    0,
};

void app_switch_test_phy(void)
{
    scan_info.phy++;
    if(scan_info.phy > TEST_PHY_S8)
    {
        scan_info.phy = TEST_PHY_1M;
    }
}

void app_switch_test_mode(void)
{
    scan_info.mode = scan_info.mode == 0 ? 1 : 0;
}

void app_cal_pre(void)
{
    double pre = (double)(TEST_PACKET_SUM - scan_info.cur_cnt) * 100 / (double)TEST_PACKET_SUM;
    tlk_printf("Per Value: %3.2f%%\r\n", pre);

    snprintf(oled_show_buff, 32, "Per: %3.2f%% ", pre);
    OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);

    scan_info.cur_cnt = 0;
}

void app_switch_scan_info(void)
{
    scan_info.cur_cnt = 0;
    scan_info.pkt_cnt = 0;
    scan_info.timeout_ticks = 0;

    le_phy_prefer_type_t prefer_phy = PHY_PREFER_1M;
    scan_phy_t scan_phy = SCAN_PHY_1M;

    switch(scan_info.phy)
    {
    case TEST_PHY_1M:   prefer_phy = PHY_PREFER_1M;
                        scan_phy = SCAN_PHY_1M;
                        break;
    case TEST_PHY_2M:   prefer_phy = PHY_PREFER_2M;
                        scan_phy = SCAN_PHY_1M;
                        break;
    case TEST_PHY_S2:
    case TEST_PHY_S8:   prefer_phy = PHY_PREFER_CODED;
                        scan_phy = SCAN_PHY_CODED;
                        break;
    default:            break;
    }
    blc_ll_setExtScanParam(OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, scan_phy,
                               SCAN_TYPE_PASSIVE, SCAN_INTERVAL_1000MS, SCAN_WINDOW_1000MS,
                               SCAN_TYPE_PASSIVE, SCAN_INTERVAL_1000MS, SCAN_WINDOW_1000MS);

    blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);

    snprintf(oled_show_buff, 32, "RX Mode: %-22s", test_mode_name[scan_info.mode]);
    OLED_ShowStr(2, 0, (uint8_t *)oled_show_buff);
    snprintf(oled_show_buff, 32, "Phy: %-26s", test_phy_name[scan_info.phy]);
    OLED_ShowStr(2, 2, (uint8_t *)oled_show_buff);

    if(scan_info.mode == 0)
    {
        snprintf(oled_show_buff, 32, "Cur: %d      ", scan_info.cur_cnt);
        OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);
        snprintf(oled_show_buff, 32, "Per: 0.0%%   ");
        OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
    }
    else if(scan_info.mode == 1)
    {
        snprintf(oled_show_buff, 32, "Scanning     ");
        OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);
        snprintf(oled_show_buff, 32, "%-15s", " ");
        OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
    }
}
#endif

/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int     app_disconnect_event_handle(u8 *p)
{
    hci_disconnectionCompleteEvt_t  *pDisConn = (hci_disconnectionCompleteEvt_t *)p;
    tlkapi_printf(APP_CONTR_EVT_LOG_EN,"app Disconnect event connHandle:%04X reason:%02X",pDisConn->connHandle,pDisConn->reason);

    //terminate reason
    if(pDisConn->reason == HCI_ERR_CONN_TIMEOUT){   //connection timeout

    }
    else if(pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN){     //peer device send terminate command on link layer

    }
    //central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
    else if(pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST){

    }
    else{

    }

    /* if previous connection SMP & SDP not finished, clear flag */
    #if (ACL_CENTRAL_SMP_ENABLE)
        if(central_smp_pending == pDisConn->connHandle){
            central_smp_pending = 0;
        }
    #endif
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        if(central_sdp_pending == pDisConn->connHandle){
            central_sdp_pending = 0;
        }
    #endif

    if(central_disconnect_connhandle == pDisConn->connHandle){  //un_pair disconnection flow finish, clear flag
        central_disconnect_connhandle = 0;
    }

    dev_char_info_delete_by_connhandle(pDisConn->connHandle);

#if TEST_ROLE == TEST_ROLE_PERIPHR
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, blc_ll_getExtendedAdvHandleForAclConnection(pDisConn->connHandle), 0, 0);
    gpio_set_low_level(GPIO_LED_RED);
    snprintf(oled_show_buff, 32, "Advertising  ");
    OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
#elif TEST_ROLE == TEST_ROLE_CENTRAL
    gpio_set_low_level(GPIO_LED_RED);
    snprintf(oled_show_buff, 32, "Scanning     ");
    OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);
#endif

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
    tlkapi_printf(APP_CONTR_EVT_LOG_EN,"app connect update complete handle:%04X  connInterval:%04X  connLatency::%04X Timeout:%04X",
            pUpt->connHandle,pUpt->connInterval,pUpt->connLatency,pUpt->supervisionTimeout);

    if(pUpt->status == BLE_SUCCESS){
        #if TEST_ROLE == TEST_ROLE_PERIPHR
            bls_l2cap_requestConnParamUpdate(pUpt->connHandle, CONN_INTERVAL_20MS, CONN_INTERVAL_20MS, 49, CONN_TIMEOUT_4S);  // 1 second
        #elif TEST_ROLE == TEST_ROLE_CENTRAL
            scan_info.conn_flag = 1;
        #endif
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
    hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;
    tlkapi_printf(APP_CONTR_EVT_LOG_EN,"le_enhanced_connection_complete connHandle:%04X mac:%02X %02X %02X %02X %02X %02X",\
            pConnEvt->connHandle,pConnEvt->peerAddr[0],pConnEvt->peerAddr[1],pConnEvt->peerAddr[2],\
            pConnEvt->peerAddr[3],pConnEvt->peerAddr[4],pConnEvt->peerAddr[5]);

    if(pConnEvt->status == BLE_SUCCESS){

        dev_char_info_insert_by_conn_event(pConnEvt);

        if(pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
        {
            #if (ACL_CENTRAL_SMP_ENABLE)
                central_smp_pending = pConnEvt->connHandle; // this connection need SMP
            #endif

            #if TEST_ROLE == TEST_ROLE_CENTRAL
                gpio_set_high_level(GPIO_LED_RED);
                snprintf(oled_show_buff, 32, "CONN Complete");
                OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);
            #endif

            #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                memset(&cur_sdp_device, 0, sizeof(dev_char_info_t));
                cur_sdp_device.conn_handle = pConnEvt->connHandle;
                cur_sdp_device.peer_adrType = pConnEvt->peerAddrType;
                memcpy(cur_sdp_device.peer_addr, pConnEvt->peerAddr, 6);

                u8  temp_buff[sizeof(dev_att_t)];
                dev_att_t *pdev_att = (dev_att_t *)temp_buff;

                /* att_handle search in flash, if success, load char_handle directly from flash, no need SDP again */
                if( dev_char_info_search_peer_att_handle_by_peer_mac(pConnEvt->peerAddrType, pConnEvt->peerAddr, pdev_att) ){
                    //cur_sdp_device.char_handle[1] =                                   //Speaker
                    cur_sdp_device.char_handle[2] = pdev_att->char_handle[2];           //OTA
                    cur_sdp_device.char_handle[3] = pdev_att->char_handle[3];           //consume report
                    cur_sdp_device.char_handle[4] = pdev_att->char_handle[4];           //normal key report
                    //cur_sdp_device.char_handle[6] =                                   //BLE Module, SPP Server to Client
                    //cur_sdp_device.char_handle[7] =                                   //BLE Module, SPP Client to Server

                    /* add the peer device att_handle value to conn_dev_list */
                    dev_char_info_add_peer_att_handle(&cur_sdp_device);
                }
                else
                {
                    central_sdp_pending = pConnEvt->connHandle;  // mark this connection need SDP

                    #if (ACL_CENTRAL_SMP_ENABLE)
                         //service discovery initiated after SMP done, trigger it in "GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE" event callBack.
                    #else
                         app_register_service(&app_service_discovery);  //No SMP, service discovery can initiated now
                    #endif
                }
            #endif
        }
        else if(pConnEvt->role == ACL_ROLE_PERIPHERAL)
        {
            gpio_set_high_level(GPIO_LED_RED);
            snprintf(oled_show_buff, 32, "CONN Complete");
            OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
            bls_l2cap_requestConnParamUpdate(pConnEvt->connHandle, CONN_INTERVAL_50MS, CONN_INTERVAL_50MS, 19, CONN_TIMEOUT_4S);  // 1 second
        }
    }


    return 0;
}

/**
 * @brief      LE Extended Advertising report event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_ext_adv_report_event_handle(u8 *p, int evt_data_len)
{
    (void)evt_data_len;

    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int offset = 0;

    extAdvEvt_info_t *pExtAdvInfo = NULL;
    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
    #if (ACL_CENTRAL_SMP_ENABLE)
        if(central_smp_pending){     //if previous connection SMP not finish, can not create a new connection
            return 1;
        }
    #endif

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        if(central_sdp_pending){     //if previous connection SDP not finish, can not create a new connection
            return 1;
        }
    #endif

    if (central_disconnect_connhandle){ //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return 1;
    }

    if(pExtAdvRpt->num_reports != 1){
      tlkapi_printf(APP_LOG_EN,"rpt evt combine %d", &pExtAdvRpt->num_reports);
    }

    for(int i=0; i<pExtAdvRpt->num_reports ; i++)
    {
        pExtAdvInfo = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
        s8 rssi = pExtAdvInfo->rssi;
        //TODO: add a function process data combine

        u8 ext_evtType = pExtAdvInfo->event_type & EXTADV_RPT_EVTTYPE_MASK;
        //u8 data_status = pExtAdvInfo->event_type & EXTADV_RPT_DATA_STATUS_MASK;

        int conn_adv_flag = 0;
        /* Legacy ADV */
        if(ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_IND || ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_DIRECT_IND){
             /* Legacy connectable */
            conn_adv_flag = 1;  //legacy
        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_SCAN_IND){

        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_ADV_NONCONN_IND){
#if TEST_ROLE == TEST_ROLE_CENTRAL
            u8 mac[6] = {0x11, 0x01, 0x01, 0x12, 0x12, 0x12};

            if(smemcmp(pExtAdvInfo->address + 0, mac, 6) == 0)
            {
                uint32_t pkt_cnt = 0;

                scan_info.timeout_ticks = 0;

                smemcpy((void *)&pkt_cnt, pExtAdvInfo->data + sizeof(tbl_advData) - 4, 4);
                if(pkt_cnt == scan_info.pkt_cnt)
                {
                    scan_info.cur_cnt ++;
                    snprintf(oled_show_buff, 32, "Cur: %d      ", scan_info.cur_cnt);
                    OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);
                }
                else
                {
                    scan_info.pkt_cnt = pkt_cnt;
                    app_cal_pre();

                    OLED_ColorTurn(scan_info.oled_color_turn_flag ^= 1);
                }
            }
#endif
        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_SCAN_RSP_2_ADV_IND || ext_evtType == EXTADV_RPT_EVTTYPE_LEGACY_SCAN_RSP_2_ADV_SCAN_IND){

        }
        /* Extended ADV */
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_DIRECTED){
        /* Extended, Non_Connectable Non_Scannable Undirected */
        #if TEST_ROLE == TEST_ROLE_CENTRAL
            u8 mac[6] = {0x11, 0x01, 0x01, 0x12, 0x12, 0x12};

            if(scan_info.mode == 1 ||
                smemcmp(pExtAdvInfo->address + 0, mac, 6) ||
                pExtAdvInfo->data_length != sizeof(tbl_advData)) 
                return 0;

            if((scan_info.phy == TEST_PHY_1M && pExtAdvInfo->secondary_phy == 0x01) ||
                (scan_info.phy == TEST_PHY_2M && pExtAdvInfo->secondary_phy == 0x02) ||
                (scan_info.phy == TEST_PHY_S8 && pExtAdvInfo->primary_phy == 0x03) ||
                (scan_info.phy == TEST_PHY_S2 && pExtAdvInfo->primary_phy == 0x03))
            {
                uint32_t pkt_cnt = 0;

                scan_info.timeout_ticks = 0;

                smemcpy((void *)&pkt_cnt, pExtAdvInfo->data + sizeof(tbl_advData) - 4, 4);
                if(pkt_cnt == scan_info.pkt_cnt)
                {
                    scan_info.cur_cnt ++;
                    snprintf(oled_show_buff, 32, "Cur: %d      ", scan_info.cur_cnt);
                    OLED_ShowStr(2, 4, (uint8_t *)oled_show_buff);
                }
                else
                {
                    scan_info.pkt_cnt = pkt_cnt;
                    app_cal_pre();

                    OLED_ColorTurn(scan_info.oled_color_turn_flag ^= 1);
                }
            }
        #endif
        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_DIRECTED){
            /* Extended, Connectable Undirected */
            conn_adv_flag = 2;  //Extended
        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_SCANNABLE_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_SCANNABLE_DIRECTED){
            /* Extended, Scannable Undirected */
        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_SCAN_RESPONSE){

        }

        if(conn_adv_flag)
        {
                /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
                #if (ACL_CENTRAL_SMP_ENABLE)
                    if(central_smp_pending ){    //if previous connection SMP not finish, can not create a new connection
                        return 1;
                    }
                #endif

                #if TEST_ROLE == TEST_ROLE_CENTRAL
                    if(scan_info.mode == 0) return 0;
                #endif

                int central_auto_connect = 0;
                int user_manual_pairing = 0;
                //manual pairing methods 1: key press triggers
                user_manual_pairing = central_pairing_enable && (rssi > -50);  //button trigger pairing(RSSI threshold, short distance)

                #if (ACL_CENTRAL_SMP_ENABLE)
                    central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pExtAdvInfo->address_type, pExtAdvInfo->address);
                #endif

                u8 mac[6] = {0x11, 0x01, 0x01, 0x12, 0x12, 0x12};
                if((central_auto_connect || user_manual_pairing) && !memcmp(pExtAdvInfo->address, mac, 6)){
                    /* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
                     * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
                     * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
                     * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
                    ble_sts_t status = 0xff;

                    if(conn_adv_flag == 1){ //legacy
                        /* only 1M used */
                        status = blc_ll_extended_createConnection( INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M,
                                                                   SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, CONN_TIMEOUT_4S,
                                                                   SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, CONN_TIMEOUT_4S,
                                                                   SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, CONN_TIMEOUT_4S);
                        tlkapi_printf(APP_LOG_EN,"extended_createConnection 1m Status:%02X ext_evtType%04X",status,ext_evtType);
                    }
                    else if(conn_adv_flag == 2){ //ext
                        status = blc_ll_extended_createConnection( INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M_2M_CODED, \
                                                                       SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, CONN_TIMEOUT_4S, \
                                                                       SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, CONN_TIMEOUT_4S, \
                                                                       SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, CONN_TIMEOUT_4S);
                        tlkapi_printf(APP_LOG_EN,"extended_createConnection INIT_PHY_1M_2M_CODED Status:%02X ext_evtType%04X",status,ext_evtType);
                    }
                    else{

                    }


                    if(status == BLE_SUCCESS){ //create connection success

                    }
                    else
                    {
                      tlkapi_printf(APP_LOG_EN,"blc_ll_createConnection error code :%02X",status);
                    }
                }
        }


    }
    return 0;
}


int app_le_adv_report_event_handle(u8 *p)
{
    event_adv_report_t *pa = (event_adv_report_t *)p;
    s8 rssi = pa->data[pa->len];

    #if 0  //debug, print ADV report number every 5 seconds
        AA_dbg_adv_rpt ++;
        if(clock_time_exceed(tick_adv_rpt, 5000000)){
            tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Adv report", pa->mac, 6);
            tick_adv_rpt = clock_time();
        }
    #endif

    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
#if (ACL_CENTRAL_SMP_ENABLE)
    if(central_smp_pending){     //if previous connection SMP not finish, can not create a new connection
        return 1;
    }
#endif

#if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
    if(central_sdp_pending){     //if previous connection SDP not finish, can not create a new connection
        return 1;
    }
#endif

    if (central_disconnect_connhandle){ //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return 1;
    }

    u8 test_peer_mac[6] = {0x11,0x01,0x01, 0x12,0x12,0x12};
    if(smemcmp(pa->mac, test_peer_mac, 6))
    {
        tlk_printf("rev\n");
    }
    /*********************** Central Create connection demo code end  *******************************************************/


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
int app_controller_event_callback (u32 h, u8 *p, int n)
{
    (void)n;
    if (h &HCI_FLAG_EVENT_BT_STD)       //Controller HCI event
    {
        u8 evtCode = h & 0xff;

        //------------ disconnect -------------------------------------
        if(evtCode == HCI_EVT_DISCONNECTION_COMPLETE)  //connection terminate
        {
            app_disconnect_event_handle(p);
        }
        else if(evtCode == HCI_EVT_LE_META)  //LE Event
        {
            u8 subEvt_code = p[0];

            //------hci le event: le connection update complete event-------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)  // connection update
            {
                app_le_connection_update_complete_event_handle(p);
            }
            //------hci le event: le enhanced_connection complete event---------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE) // connection complete
            {
                app_le_enhanced_connection_complete_event_handle(p);
            }
            //------hci le event: LE extended advertising report event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) // ADV packet
            {
                app_le_ext_adv_report_event_handle(p, n);
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)  // ADV packet
            {
                //after controller is set to scan state, it will report all the adv packet it received by this event

                app_le_adv_report_event_handle(p);
            }
            #if TEST_ROLE == TEST_ROLE_PERIPHR
            else if(subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_SET_TERMINATED)
            {
                hci_le_advSetTerminatedEvt_t *pEvt = (hci_le_advSetTerminatedEvt_t *) p;
                if(pEvt->status == HCI_ERR_LIMIT_REACHED)
                {
                    adv_info.restart_times++;
                    if(adv_info.restart_times >= TEST_ADV_RESTART_TIMES)
                    {
                        adv_info.restart_times = 0;
                        adv_info.pkt_cnt++;
                        smemcpy(tbl_advData + sizeof(tbl_advData) - 4, (u8 *)&adv_info.pkt_cnt, 4);
                        blc_ll_setExtAdvData( ADV_HANDLE0, sizeof(tbl_advData), (const u8 *)tbl_advData);
                        snprintf(oled_show_buff, 32, "Cnt: %ld        ", adv_info.pkt_cnt);
                        OLED_ShowStr(2, 6, (uint8_t *)oled_show_buff);
                    }
                   
                    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, TEST_PACKET_PER_TIMES);
                }
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
int app_host_event_callback (u32 h, u8 *para, int n)
{
    (void)n;
    u8 event = h & 0xFF;

    switch(event)
    {
        case GAP_EVT_SMP_PAIRING_BEGIN:
        {

        }
        break;

        case GAP_EVT_SMP_PAIRING_SUCCESS:
        {

        }
        break;

        case GAP_EVT_SMP_PAIRING_FAIL:
        {
            #if (ACL_CENTRAL_SMP_ENABLE)
                gap_smp_pairingFailEvt_t *pEvt = (gap_smp_pairingFailEvt_t *)para;

                if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL){
                    if(central_smp_pending == pEvt->connHandle){
                        central_smp_pending = 0;
                        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] paring fail", &pEvt->connHandle, sizeof(gap_smp_pairingFailEvt_t));
                    }
                }
            #endif
        }
        break;

        case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
        {
            gap_smp_connEncDoneEvt_t *pEvt = (gap_smp_connEncDoneEvt_t *)para;
            tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Connection encryption done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));
        }
        break;

        case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
        {
            gap_smp_connEncDoneEvt_t* pEvt = (gap_smp_connEncDoneEvt_t*)para;
            tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][SMP] Security process done event", &pEvt->connHandle, sizeof(gap_smp_connEncDoneEvt_t));

            if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL){

                #if (ACL_CENTRAL_SMP_ENABLE)
                    if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL){
                        if(central_smp_pending == pEvt->connHandle){
                            central_smp_pending = 0;
                        }
                    }
                #endif

                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)  //SMP finish
                    if(central_sdp_pending == pEvt->connHandle){  //SDP is pending
                        app_register_service(&app_service_discovery);  //start SDP now
                    }
                #endif
            }
        }
        break;

        case GAP_EVT_SMP_TK_DISPLAY:
        {

        }
        break;

        case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
        {

        }
        break;

        case GAP_EVT_SMP_TK_REQUEST_OOB:
        {

        }
        break;

        case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
        {

        }
        break;

        case GAP_EVT_ATT_EXCHANGE_MTU:
        {

        }
        break;

        case GAP_EVT_GATT_HANDLE_VALUE_CONFIRM:
        {

        }
        break;

        default:
        break;
    }

    return 0;
}



#define         HID_HANDLE_CONSUME_REPORT           25
#define         HID_HANDLE_KEYBOARD_REPORT          29
#define         AUDIO_HANDLE_MIC                    52
#define         OTA_HANDLE_DATA                     48

/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler (u16 connHandle, u8 *pkt)
{
    if( dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL )   //GATT data for Central
    {
        #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
            if(central_sdp_pending == connHandle ){  //ATT service discovery is ongoing on this conn_handle
                //when service discovery function is running, all the ATT data from peripheral
                //will be processed by it,  user can only send your own att cmd after  service discovery is over
                host_att_client_handler (connHandle, pkt); //handle this ATT data by service discovery process
            }
        #endif

        rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

        //so any ATT data before service discovery will be dropped
        dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
        if(dev_info)
        {
            //-------   user process ------------------------------------------------
            u16 attHandle = pAtt->handle;

            if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //peripheral handle notify
            {
                #if TEST_ROLE == TEST_ROLE_CENTRAL
                    scan_info.timeout_ticks = 0;
                #endif
                    //---------------   consumer key --------------------------
                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    if(attHandle == dev_info->char_handle[3])  // Consume Report In (Media Key)
                #else
                    if(attHandle == HID_HANDLE_CONSUME_REPORT)   //Demo device(825x_ble_sample) Consume Report AttHandle value is 25
                #endif
                    {
                        att_keyboard_media (connHandle, pAtt->dat);
                    }
                    //---------------   keyboard key --------------------------
                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    else if(attHandle == dev_info->char_handle[4])     // Key Report In
                #else
                    else if(attHandle == HID_HANDLE_KEYBOARD_REPORT)   // Demo device(825x_ble_sample) Key Report AttHandle value is 29
                #endif
                    {
                        att_keyboard (connHandle, pAtt->dat);
                    }
                #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
                    else if(attHandle == dev_info->char_handle[0])     // AUDIO Notify
                #else
                    else if(attHandle == AUDIO_HANDLE_MIC)   // Demo device(825x_ble_remote) Key Report AttHandle value is 52
                #endif
                    {

                    }
                    else
                    {

                    }
            }
            else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
            {

            }
        }

        if(!(pAtt->opcode & 0x01)){
            switch(pAtt->opcode){
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
                default://no action
                    break;
            }
        }
    }
    else{   //GATT data for Peripheral

    }


    return 0;
}

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
        blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    blc_readFlashSize_autoConfigCustomFlashSector();

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();
//////////////////////////// basic hardware Initialization  End /////////////////////////////////


//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8  mac_public[6];
    u8  mac_random_static[6];
    
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

#if TEST_ROLE == TEST_ROLE_PERIPHR
    /*!< limit periphr mac to 12:12:12:01:01:01 */
    u8 mac[6] = {0x11,0x01,0x01, 0x12,0x12,0x12};
    memcpy(mac_public, mac, 6);
#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
    /*!< limit central mac to 12:12:12:01:01:10 */
    u8 mac[6] = {0x10,0x01,0x01, 0x12,0x12,0x12};
    memcpy(mac_public, mac, 6);
#endif

    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();
    blc_ll_initStandby_module(mac_public);

#if TEST_ROLE == TEST_ROLE_PERIPHR
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);

#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
    blc_ll_initExtendedScanning_module();
    blc_ll_initExtendedInitiating_module();
#endif

    blc_ll_initAclConnection_module();

#if TEST_ROLE == TEST_ROLE_PERIPHR
    blc_ll_initAclPeriphrRole_module();
#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
    blc_ll_initAclCentralRole_module();
#endif

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);

#if TEST_ROLE ==  TEST_ROLE_CENTRAL
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);
    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_50MS);
#elif TEST_ROLE == TEST_ROLE_PERIPHR
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);
#endif
    //////////// LinkLayer Initialization  End /////////////////////////

    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd( HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE\
                                |HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE\
                                |HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT
                                |HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_SET_TERMINATED
                                | HCI_LE_EVT_MASK_ADVERTISING_REPORT);

    u8 error_code = blc_contr_checkControllerInitialization();
    if(error_code != INIT_SUCCESS){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] Controller INIT ERROR", &error_code, 1);
            while(1){
                tlkapi_debug_handler();
            }
        #else
            while(1);
        #endif
    }
    //////////// HCI Initialization  End /////////////////////////


    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
#if TEST_ROLE == TEST_ROLE_PERIPHR
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);
    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU);   ///must be placed after "blc_gap_init"
#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, NULL, 0);
    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU); ///must be placed after "blc_gap_init"
#endif

    /* GATT Initialization */
    my_gatt_init();
    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        host_att_register_idle_func (main_idle_loop);
    #endif
    blc_gatt_register_data_handler(app_gatt_data_handler);

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
        
        blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (ACL_PERIPHR_SMP_ENABLE)  //Peripheral SMP Enable
        blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
    #else
        blc_smp_setSecurityLevel_periphr(No_Security);
    #endif

    #if (ACL_CENTRAL_SMP_ENABLE)
        blc_smp_setSecurityLevel_central(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
    #else
        blc_smp_setSecurityLevel_central(No_Security);
    #endif

    blc_smp_smpParamInit();


    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler( app_host_event_callback );
    blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN            |  \
                          GAP_EVT_MASK_SMP_PAIRING_SUCCESS          |  \
                          GAP_EVT_MASK_SMP_PAIRING_FAIL             |  \
                          GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
    //////////// Host Initialization  End /////////////////////////

//////////////////////////// BLE stack Initialization  End //////////////////////////////////




//////////////////////////// User Configuration for BLE application ////////////////////////////
    rf_set_power_level_index(TEST_RF_POWER);

    // limit the channel to TEST_CHANNEL_NUM, to avoid pri channel interference
#if TEST_ROLE ==  TEST_ROLE_PERIPHR
     blc_ll_setCustomizedAdvertisingScanningChannel(TEST_CHANNEL_NUM, 38, 39);
#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
     blc_ll_setCustomizedAdvertisingScanningChannel(TEST_CHANNEL_NUM, TEST_CHANNEL_NUM, TEST_CHANNEL_NUM);
#endif

    /*!< oled init */
    i2c_set_pin(I2C_GPIO_SDA_PIN, I2C_GPIO_SCL_PIN);
    i2c_master_init();
    i2c_set_master_clk((unsigned char)(sys_clk.pclk * 1000 * 1000 / (4 * I2C_CLK_SPEED)));
    OLED_Init();

    /*!< led init */
    gpio_set_high_level(GPIO_LED_WHITE);
    gpio_set_low_level(GPIO_LED_GREEN);
    gpio_set_low_level(GPIO_LED_BLUE);
#if TEST_ROLE ==  TEST_ROLE_PERIPHR
    app_switch_adv_info();
    gpio_set_low_level(GPIO_LED_RED);
#elif TEST_ROLE ==  TEST_ROLE_CENTRAL
    app_switch_scan_info();
    gpio_set_low_level(GPIO_LED_RED);
#endif


    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] TEST Long Range init", 0, 0);
////////////////////////////////////////////////////////////////////////////////////////////////
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

/**
 * @brief     BLE main idle loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop (void)
{
    #if TEST_ROLE == TEST_ROLE_PERIPHR
        static u32 tick = 0;
        static u8 app_test_data[4] = {0x00, 0x00, 0x00, 0x00};

        if(adv_info.mode &&
            conn_dev_list[0].conn_state &&
            clock_time_exceed(tick, 100 * 1000))
        {
            blc_gatt_pushHandleValueNotify(conn_dev_list[0].conn_handle,  SPP_CLIENT_TO_SERVER_DP_H, app_test_data, 4);
            tick = clock_time();
        }
    #endif

    #if TEST_ROLE == TEST_ROLE_CENTRAL
        static u32 ticks = 0;
        if(clock_time_exceed(ticks, 100 * 1000))   // check per 100 ms
        {
            scan_info.timeout_ticks += 100;         // will be cleared after receiving a pkt
            if(scan_info.timeout_ticks > 1000)
            {
                if(!scan_info.mode) gpio_set_low_level(GPIO_LED_RED);
                else    gpio_set_low_level(GPIO_LED_BLUE);
            }
            else
            {
                if(!scan_info.mode) gpio_set_high_level(GPIO_LED_RED);
                else    gpio_set_high_level(GPIO_LED_BLUE);
            }
            ticks = clock_time();
        }
    #endif

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    ///////////////////////////////// SOFTWARE_TIMER entry ////////////////////////////
    #if (BLT_SOFTWARE_TIMER_ENABLE)
    blt_soft_timer_process(MAINLOOP_ENTRY);
    #endif

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard (0, 0, 0);
    #endif


    proc_central_role_unpair();


    return 0; //must return 0 due to SDP flow
}



/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop (void)
{
    main_idle_loop ();

    #if (ACL_CENTRAL_SIMPLE_SDP_ENABLE)
        simple_sdp_loop ();
    #endif
}

#endif //end of (FEATURE_TEST_MODE == ...)



