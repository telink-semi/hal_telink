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
#include "app_buffer.h"
#include "app_att.h"
#include "app_ui.h"
#include "bqb_profile.h"

#if (FEATURE_TEST_MODE == TEST_GATTC_SDP)


int central_smp_pending = 0;        // SMP: security & encryption;






const u8    tbl_advData[] = {
     8,  DT_COMPLETE_LOCAL_NAME,                'f','e','a','t','u','r','e',
     2,  DT_FLAGS,                              0x05,                   // BLE limited discoverable mode and BR/EDR not supported
     3,  DT_APPEARANCE,                         0x80, 0x01,             // 384, Generic Remote Control, Generic category
     5,  DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID, 0x12, 0x18, 0x0F, 0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8    tbl_scanRsp [] = {
     8, DT_COMPLETE_LOCAL_NAME,                 'f','e','a','t','u','r','e',
};



/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
//int app_le_adv_report_event_handle(u8 *p)
//{
//  event_adv_report_t *pa = (event_adv_report_t *)p;
//  s8 rssi = pa->data[pa->len];
//
//
//  /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
//  if(central_smp_pending ){    //if previous connection SMP not finish, can not create a new connection
//      return 1;
//  }
//  if (central_disconnect_connhandle){ //one ACL connection central role is in un_pair disconnection flow, do not create a new one
//      return 1;
//  }
//
//  int central_auto_connect = 0;
//  int user_manual_pairing = 0;
//
//  //manual pairing methods 1: key press triggers
//  user_manual_pairing = central_pairing_enable && (1 || rssi > -66);  //button trigger pairing(RSSI threshold, short distance)
//
//  #if (ACL_CENTRAL_SMP_ENABLE)
//      central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
//  #else
//      //search in peripheral mac_address table to find whether this device is an old device which has already paired with central
//  #endif
//
//
//  if(central_auto_connect || user_manual_pairing){
//
//
        /* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
         * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
         * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
         * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
//      u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY,
//                               pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC,
//                               CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, 0, CONN_TIMEOUT_4S,
//                               0, 0xFFFF);
//
//      (void)status;
//  }
//  /*********************** Central Create connection demo code end  *******************************************************/
//
//
//  return 0;
//}

/**
 * @brief      LE Extended Advertising report event handler
 * @param[in]  p - Pointer point to event parameter buffer.
 * @return
 */
int app_le_ext_adv_report_event_handle(u8 *p, int evt_data_len)
{

    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int offset = 0;

    extAdvEvt_info_t *pExtAdvInfo = NULL;


    for(int i=0; i<pExtAdvRpt->num_reports ; i++)
    {
        pExtAdvInfo = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
        s8 rssi = pExtAdvInfo->rssi;
        BLT_APP_STR_LOG("[APP]adv mac", pExtAdvInfo->address, 6);
        u8 ext_evtType = pExtAdvInfo->event_type & EXTADV_RPT_EVTTYPE_MASK;
        u8 conn_adv_flag = 0;
        /* Extended ADV */
        if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_NON_CONN_NON_SCAN_DIRECTED)
        {
            /* Extended, Non_Connectable Non_Scannable Undirected */
        }
        else if(ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_DIRECTED)
        {
            conn_adv_flag = 2;  //Extended
            /* Extended, Connectable Undirected */
        }
        if(conn_adv_flag)
        {
            /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
            if(central_smp_pending )
            {    //if previous connection SMP not finish, can not create a new connection
                return 1;
            }
            if (central_disconnect_connhandle)
            { //one ACL connection central role is in un_pair disconnection flow, do not create a new one
                return 1;
            }

            int central_auto_connect = 0;
            int user_manual_pairing = 0;

            //manual pairing methods 1: key press triggers
            user_manual_pairing = central_pairing_enable && (0 || rssi > -66);  //button trigger pairing(RSSI threshold, short distance)

            #if (ACL_CENTRAL_SMP_ENABLE)
                central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pExtAdvInfo->address_type, pExtAdvInfo->address);
            #else
                //search in peripheral mac_address table to find whether this device is an old device which has already paired with central
            #endif


            if(central_auto_connect || user_manual_pairing)
            {
                u8 status;
                if(conn_adv_flag == 1)
                { //legacy
                    /* only 1M used */
                    BLT_APP_LOG("adv 1");
                    status = blc_ll_extended_createConnection( INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M, \
                                                               SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_40MS, CONN_TIMEOUT_4S, \
                                                               0, 0, 0, 0, 0, \
                                                               0, 0, 0, 0, 0);
                }
                else
                {
                    BLT_APP_LOG("adv 2");
                    status = blc_ll_extended_createConnection( INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M, \
                                                               SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_40MS, CONN_TIMEOUT_4S, \
                                                               SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_40MS, CONN_TIMEOUT_4S, \
                                                               SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_20MS, CONN_INTERVAL_40MS, CONN_TIMEOUT_4S);
                }

                if(status != BLE_SUCCESS)
                {
                    if(status != HCI_ERR_CONN_REJ_LIMITED_RESOURCES)
                    {
                        BLT_APP_LOG("ext init fail:%d", status);
                    }
                }
                else
                {
                    BLT_APP_LOG("ext init ok:%d", status);
                }
            }
        }
    }
    return 0;
}


uuid_t uuid = UUID16_INIT(0);
gattc_sdp_cfg_t  app_gatt_sdp_cfg[ACL_CENTRAL_MAX_NUM];
u16 svcStartHdl, svcEndHdl;

struct gattc_sub_ccc_tbl {
    u16 startHdl;
    u16 endHdl;
    u16 cccHdl;
    u16 cccHdl_value;
} ccc_subscribe[10];
u8 ccc_subscribe_idx = 0;

u8 discovery_func(u16 connHandle, gatt_attr_t *attr, gattc_sdp_cfg_t *params)
{
    gattc_sdp_cfg_t *pSdpCfg = &app_gatt_sdp_cfg[connHandle&0x0F];

    if (attr == NULL) {
        if (params->type == GATT_DISCOVER_PRIMARY) {
            BLT_APP_LOG("///////// Services Discovery complete /////////");

            BLT_APP_LOG("XXXXXXXXX Characteristic Discovery start /////////");
            pSdpCfg->uuid = NULL;
            pSdpCfg->startHdl = svcStartHdl;
            pSdpCfg->endHdl = svcEndHdl;
            pSdpCfg->type = GATT_DISCOVER_CHARACTERISTIC;

            ble_sts_t status = blc_gattc_discovery(connHandle, pSdpCfg);
            if (status != BLE_SUCCESS) {
                BLT_APP_LOG("Characteristic Discovery failed:%d", status);
            }
        } else if (params->type == GATT_DISCOVER_CHARACTERISTIC) {
            BLT_APP_LOG("///////// Characteristic Discovery complete /////////");

            BLT_APP_LOG("XXXXXXXXX Include Service Discovery start /////////");
            pSdpCfg->uuid = NULL;
            pSdpCfg->startHdl = svcStartHdl;
            pSdpCfg->endHdl = svcEndHdl;
            pSdpCfg->type = GATT_DISCOVER_INCLUDE;

            ble_sts_t status = blc_gattc_discovery(connHandle, pSdpCfg);
            if (status != BLE_SUCCESS) {
                BLT_APP_LOG("Include Service Discovery failed:%d", status);
            }
        } else if (params->type == GATT_DISCOVER_INCLUDE) {
            BLT_APP_LOG("///////// Include Service complete /////////");

            BLT_APP_LOG("XXXXXXXXX Descriptor Discovery start:%d /////////", ccc_subscribe_idx);

            if (ccc_subscribe_idx) {
                struct gattc_sub_ccc_tbl * pCccSubscribe = &ccc_subscribe[--ccc_subscribe_idx];

                UUID16_CPY(uuid, DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION);
                pSdpCfg->uuid = &uuid;
                pSdpCfg->startHdl = pCccSubscribe->startHdl;
                pSdpCfg->endHdl = pCccSubscribe->endHdl;
                pSdpCfg->type = GATT_DISCOVER_DESCRIPTOR;

                ble_sts_t status = blc_gattc_discovery(connHandle, pSdpCfg);
                if (status != BLE_SUCCESS) {
                    BLT_APP_LOG("GATTC CCC Discovery failed:%d", status);
                }
            } else {
                /* Clear GATT Discovery procedure parameters configuration */
                memset(pSdpCfg, 0, sizeof(gattc_sdp_cfg_t));
                return GATT_PROC_END;
            }


        } else if (params->type == GATT_DISCOVER_DESCRIPTOR) {

            if (ccc_subscribe_idx) {
                struct gattc_sub_ccc_tbl * pCccSubscribe = &ccc_subscribe[--ccc_subscribe_idx];

                UUID16_CPY(uuid, DESCRIPTOR_UUID_CLIENT_CHARACTERISTIC_CONFIGURATION);
                pSdpCfg->uuid = &uuid;
                pSdpCfg->startHdl = pCccSubscribe->startHdl;
                pSdpCfg->endHdl = pCccSubscribe->endHdl;
                pSdpCfg->type = GATT_DISCOVER_DESCRIPTOR;

                ble_sts_t status = blc_gattc_discovery(connHandle, pSdpCfg);
                if (status != BLE_SUCCESS) {
                    BLT_APP_LOG("GATTC CCC Discovery failed:%d", status);
                }
            } else {
                BLT_APP_LOG("///////// Descriptor Discovery complete /////////");
                /* Clear GATT Discovery procedure parameters configuration */
                memset(pSdpCfg, 0, sizeof(gattc_sdp_cfg_t));
                return GATT_PROC_END;
            }
        }

        return GATT_PROC_END;
    }

    BLT_APP_LOG("     [ATTRIBUTE] handle:0x%x", attr->handle);

    if (params->type == GATT_DISCOVER_PRIMARY) {
        gatt_service_val_t *prim_service;
        prim_service = (gatt_service_val_t *)attr->user_data;
        u16 startHdl = attr->handle + 1;
        u16 endHdl = prim_service->endHdl;

    #if (1)
        svcStartHdl = startHdl;
        svcEndHdl = endHdl;
    #else
        svcStartHdl = ATT_HANDLE_START;
        svcEndHdl = ATT_HANDLE_MAX;
    #endif

        BLT_APP_LOG("     [ Service ] startHdl:0x%x", startHdl);
        BLT_APP_LOG("                 endHdl:0x%x", endHdl);
        BLT_APP_STR_LOG("[APP]        << uuid >>",  prim_service->uuid->uuidVal.u, prim_service->uuid->uuidLen);

        return GATT_PROC_CONT;
    } else if (params->type == GATT_DISCOVER_CHARACTERISTIC) {
        gatt_chrc_t *chrc = (gatt_chrc_t*)attr->user_data;

        BLT_APP_LOG("     [ Char... ] properties:%d", chrc->properties);
        BLT_APP_LOG("                 valueHdl:0x%x", chrc->valueHdl);
        BLT_APP_STR_LOG("[APP]        << uuid >>",  chrc->uuid.uuidVal.u, chrc->uuid.uuidLen);

        /* CCC discovery needed */
        if(chrc->properties & (CHAR_PROP_NOTIFY | CHAR_PROP_INDICATE)){
            ccc_subscribe[ccc_subscribe_idx].startHdl = chrc->valueHdl + 1;
            ccc_subscribe[ccc_subscribe_idx].endHdl = chrc->valueHdl + 1; /*  */
            BLT_APP_LOG("              sub_startHdl:0x%x", ccc_subscribe[ccc_subscribe_idx].startHdl);
            BLT_APP_LOG("              sub_endHdl:0x%x", ccc_subscribe[ccc_subscribe_idx].endHdl);
            ccc_subscribe_idx++;
        }

        return GATT_PROC_CONT;
    } else if (params->type == GATT_DISCOVER_INCLUDE) {
        gatt_include_t *inc = (gatt_include_t*)attr->user_data;

        BLT_APP_LOG("     [ Include ] startHdl:0x%x", inc->startHdl);
        BLT_APP_LOG("                 endHdl:0x%x", inc->endHdl);
        BLT_APP_STR_LOG("[APP]          << uuid >>",  inc->uuid.uuidVal.u, inc->uuid.uuidLen);

        return GATT_PROC_CONT;
    } else if (params->type == GATT_DISCOVER_DESCRIPTOR) {
        /* Get Client Characteristic Configuration: Attribute Handle */
        u16 cccHdl = attr->handle;
        BLT_APP_LOG("     [ cccHdl ] handle_value:0x%x", cccHdl);

        struct gattc_sub_ccc_tbl * pCccSubscribe = &ccc_subscribe[ccc_subscribe_idx];
        pCccSubscribe->cccHdl = cccHdl;

        return GATT_PROC_CONT;
    }

    return GATT_PROC_END;
}

/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
//int app_le_connection_complete_event_handle(u8 *p)
//{
//  hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;
//
//  if(pConnEvt->status == BLE_SUCCESS){
//
//      dev_char_info_insert_by_conn_event(pConnEvt);
//
//      if(pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
//      {
//          #if (ACL_CENTRAL_SMP_ENABLE)
//              central_smp_pending = pConnEvt->connHandle; // this connection need SMP
//          #else
//
//          #endif
//      }
//  }
//
//  return 0;
//}


/**
 * @brief      BLE enhanced connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_enhanced_connection_complete_event_handle(u8 *p)
{
    hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

    if(pConnEvt->status == BLE_SUCCESS)
    {
        dev_char_info_insert_by_enhanced_conn_event(pConnEvt);

        if(pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
        {
            BLT_APP_LOG("LE Enhanced Connection complete:0x%x", pConnEvt->connHandle);

            #if (ACL_CENTRAL_SMP_ENABLE)
                central_smp_pending = pConnEvt->connHandle; // this connection need SMP
            #else

            #endif
        }

        blc_audio_connect(pConnEvt->connHandle);
        gpio_toggle(GPIO_LED_BLUE);
        app_bqb_connect(pConnEvt->connHandle);
    }

    return 0;
}



/**
 * @brief      BLE Disconnection event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int     app_disconnect_event_handle(u8 *p)
{
    hci_disconnectionCompleteEvt_t  *pDisConn = (hci_disconnectionCompleteEvt_t *)p;

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



    //if previous connection SMP & SDP not finished, clear flag
#if (ACL_CENTRAL_SMP_ENABLE)
    if(central_smp_pending == pDisConn->connHandle){
        central_smp_pending = 0;
    }
#endif

    if(central_disconnect_connhandle == pDisConn->connHandle){  //un_pair disconnection flow finish, clear flag
        central_disconnect_connhandle = 0;
    }

    dev_char_info_delete_by_connhandle(pDisConn->connHandle);

    blc_audio_disconnect(pDisConn->connHandle);

    app_bqb_disconn(pDisConn->connHandle);
    gpio_toggle(GPIO_LED_BLUE);
//  blc_audio_vcpVolumeCtrlDisconnect(pDisConn->connHandle);
//  blc_audio_micpMicCtrlDisconnect(pDisConn->connHandle);
//  blc_audio_aicsClientDisconnect(pDisConn->connHandle);
//  blc_audio_vocsClientDisconnect(pDisConn->connHandle);
//  blc_audio_clientDiscReset(pDisConn->connHandle);

    return 0;
}

/**
 * @brief      BLE GATTC indicate/notify data handle call-back
 * @param[in]  connHandle - connection handle.
 * @param[in]  attr - 'attr_pkt_t'  Pointer point to the packet buffer.
 * @param[in]  attrLen - the length of the packet.
 * @return
 */
int app_gattc_subscribe_data_callback (u16 connHandle, attr_pkt_t *attr, u16 attrLen)
{
    if (attr->opcode == ATT_OP_HANDLE_VALUE_NOTI) { //handle notify
        blt_attr_handleValueNtf_t *pHdlValNtf = (blt_attr_handleValueNtf_t*)attr;

        (void)pHdlValNtf;
    } else if (attr->opcode == ATT_OP_HANDLE_VALUE_IND) { //handle indicate
        blt_attr_handleValueInd_t *pHdlValInd = (blt_attr_handleValueInd_t*)attr;

        (void)pHdlValInd;

        blc_attc_sendAttHdlValueCfm(connHandle);
    } else if(attr->opcode == ATT_OP_MULTIPLE_HANDLE_VALUE_NTF) { //handle multiple notify
        blt_attr_multiHandleValueNtf_t *pHdlMultValInd = (blt_attr_multiHandleValueNtf_t*)attr;

        (void)pHdlMultValInd;
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
int app_controller_event_callback (u32 h, u8 *p, int n)
{
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

            //------hci le event: le connection complete event---------------------------------
//          if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)  // connection complete
//          {
//              app_le_connection_complete_event_handle(p);
//          }
            //------hci le event: le enhanced_connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE)  // connection complete
            {
                app_le_enhanced_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
//          else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)  // ADV packet
//          {
//              //after controller is set to scan state, it will report all the adv packet it received by this event
//
//              app_le_adv_report_event_handle(p);
//          }
            //------hci le event: LE extended advertising report event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) // ADV packet
            {
                app_le_ext_adv_report_event_handle(p, n);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)  // connection update
            {

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
int app_host_event_callback (u32 h, u8 *para, int n)
{
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
            gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;

            if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){
                if(central_smp_pending == p->connHandle){
                    central_smp_pending = 0;
                }
            }
        #endif
        }
        break;

        case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
        {

        }
        break;

        case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
        {
        #if (ACL_CENTRAL_SMP_ENABLE)
            gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;

            if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){

                if(central_smp_pending == p->connHandle){
                    central_smp_pending = 0;
#if (0)
                    gattc_sdp_cfg_t *pSdpCfg = &app_gatt_sdp_cfg[p->connHandle&0x0F];

                #if (1)
                    UUID16_CPY(uuid, SERVICE_UUID_HUMAN_INTERFACE_DEVICE);
                    pSdpCfg->uuid = &uuid;
                #else
                    pSdpCfg->uuid = NULL;
                #endif

                    pSdpCfg->type = GATT_DISCOVER_PRIMARY;
                    pSdpCfg->startHdl = ATT_HANDLE_START;
                    pSdpCfg->endHdl = ATT_HANDLE_MAX;
                    pSdpCfg->func = discovery_func;

                    BLT_APP_LOG("XXXXXXXXX Service Discovery start /////////");

                    ble_sts_t status = blc_gattc_discovery(p->connHandle, pSdpCfg);
                    if (status != BLE_SUCCESS) {
                        BLT_APP_LOG("Service discovery err:%d", status);
                    }
#else
                    blc_audio_discoveryStart(p->connHandle);
#endif
                }
            }
        #endif
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


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);                         //mandatory

//  blc_ll_initLegacyAdvertising_module();
//  blc_ll_initLegacyScanning_module();
//  blc_ll_initLegacyInitiating_module();

    blc_ll_initExtendedScanning_module();
    blc_ll_initExtendedInitiating_module();


    blc_ll_initAclConnection_module();
    blc_ll_initAclCentralRole_module();
    blc_ll_initAclPeriphrRole_module();

    // ISO features must support CSA#2
    blc_ll_initChannelSelectionAlgorithm_2_feature();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_10MS);

    rf_set_power_level_index(RF_POWER_P3dBm);


    //////////// LinkLayer Initialization  End /////////////////////////



    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(        HCI_LE_EVT_MASK_CONNECTION_COMPLETE  \
                                    |   HCI_LE_EVT_MASK_ADVERTISING_REPORT \
                                    |   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE \
                                    |   HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE \
                                    |   HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT );


    u8 error_code = blc_contr_checkControllerInitialization();
    if(error_code != INIT_SUCCESS){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
        #if (TLKAPI_DEBUG_ENABLE)
            BLT_APP_LOG("Controller Init ERROR:0x%x", error_code);
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
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, app_cen_l2cap_tx_buf, CENTRAL_L2CAP_BUFF_SIZE);
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize (128);
    blc_att_setPeripheralRxMtuSize (128);
    //blc_att_setMtureqSendingTime_after_connCreate(1);

    /* GATT Server Initialization */
    gatt_init_service();

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

//  blc_audio_proc_t *vcpVolCtrl[] = {
//      (blc_audio_proc_t*)&vcp_client_ctrl,
//      (blc_audio_proc_t*)&vocs_client_ctrl,
//      (blc_audio_proc_t*)&aics_client_ctrl,
//  };
//
//  blc_audio_proc_t *micpMicCtrl[] = {
//      (blc_audio_proc_t*)&micp_client_ctrl,
//      (blc_audio_proc_t*)&aics_client_ctrl,
//  };
//
//  blc_audio_proc_t *bapUnicastClt[] = {
//      (blc_audio_proc_t*)&pacs_client_ctrl,
//      (blc_audio_proc_t*)&ascs_client_ctrl,
//  };
//
//  blc_audio_svcTableInit(PRF_GAP_ACL_UNSPECIF, bapUnicastClt, ARRAY_SIZE(bapUnicastClt), NULL);
//  blc_audio_svcTableInit(PRF_GAP_ACL_UNSPECIF, vcpVolCtrl, ARRAY_SIZE(vcpVolCtrl), NULL);
//  blc_audio_svcTableInit(PRF_GAP_ACL_UNSPECIF, micpMicCtrl, ARRAY_SIZE(micpMicCtrl), NULL);

//////////////////////////// BLE stack Initialization  End //////////////////////////////////



//////////////////////////// User Configuration for BLE application ////////////////////////////
//  blc_ll_setAdvData(tbl_advData, sizeof(tbl_advData));
//  blc_ll_setScanRspData(tbl_scanRsp, sizeof(tbl_scanRsp));
//  blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
//  blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable
//
//  blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
//  blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

    blc_ll_setExtScanParam( OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M_CODED, \
                            SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_90MS,   SCAN_WINDOW_90MS, \
                            SCAN_TYPE_PASSIVE,  SCAN_INTERVAL_90MS,   SCAN_WINDOW_90MS);

    blc_ll_setExtScanEnable( BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);



    app_bqb_init();
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
u32 ledScanTick = 0;
/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop (void)
{

    if(clock_time_exceed(ledScanTick, 1000 * 1000))
    {  //keyScan interval: 1000mS
        ledScanTick = clock_time();
        gpio_toggle(GPIO_LED_GREEN);

    }
    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    blc_prf_main_loop();

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard (0, 0, 0);
    #endif

    proc_central_role_unpair();

    return 0; //must return 0 due to SDP flow
}



_attribute_no_inline_ void main_loop (void)
{
    main_idle_loop ();
}

#endif



