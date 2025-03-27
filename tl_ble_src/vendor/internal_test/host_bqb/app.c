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
#include "app_ui.h"
#include "app_buffer.h"
#include "bqb_profile.h"
#include "../default_att.h"



#if (INTER_TEST_MODE == TEST_HOST_BQB)


int central_smp_pending = 0;        // SMP: security & encryption;
int central_auto_connect = 0;
int user_manual_pairing = 0;




const u8    tbl_advData[] = {
     7,  DT_COMPLETE_LOCAL_NAME,                'H','o','s','t','_','B',
     2,  DT_FLAGS,                              0x05,                   // BLE limited discoverable mode and BR/EDR not supported
     3,  DT_APPEARANCE,                         0x80, 0x01,             // 384, Generic Remote Control, Generic category
     5,  DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID, 0x12, 0x18, 0x0F, 0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

const u8    tbl_scanRsp [] = {
         7,  DT_COMPLETE_LOCAL_NAME,            'H','o','s','t','_','B',
};

/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_adv_report_event_handle(u8 *p)
{
    event_adv_report_t *pa = (event_adv_report_t *)p;
    s8 rssi = pa->data[pa->len];


    /*********************** Central Create connection demo: Key press or ADV pair packet triggers pair  ********************/
    if(central_smp_pending ){    //if previous connection SMP not finish, can not create a new connection
        return 1;
    }
    if (central_disconnect_connhandle){ //one ACL connection central role is in un_pair disconnection flow, do not create a new one
        return 1;
    }

    int central_auto_connect = 0;
    int user_manual_pairing = 0;
    int pts_filter_connect = 0;

    u8 PTS_Addr[6] = { 0x26, 0x39, 0x34, 0xE8, 0x07, 0xC0 };
    pts_filter_connect = !memcmp(PTS_Addr, pa->mac, 6);

    //manual pairing methods 1: key press triggers
    user_manual_pairing = pts_filter_connect && central_pairing_enable && (rssi > -66);  //button trigger pairing(RSSI threshold, short distance)

    #if (ACL_CENTRAL_SMP_ENABLE)
        central_auto_connect = blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pa->adr_type, pa->mac);
    #endif


    if(central_auto_connect || user_manual_pairing){

        /* send create connection command to Controller, trigger it switch to initiating state. After this command, Controller
         * will scan all the ADV packets it received but not report to host, to find the specified device(mac_adr_type & mac_adr),
         * then send a "CONN_REQ" packet, enter to connection state and send a connection complete event
         * (HCI_SUB_EVT_LE_CONNECTION_COMPLETE) to Host*/
        u8 status = blc_ll_createConnection( SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY,  \
                                 pa->adr_type, pa->mac, OWN_ADDRESS_PUBLIC, \
                                 CONN_INTERVAL_31P25MS, CONN_INTERVAL_31P25MS, 0, CONN_TIMEOUT_4S, \
                                 0, 0xFFFF);


        if(status == BLE_SUCCESS){ //create connection success
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] create connection success", pa->mac, 6);
            if(central_pairing_enable){
                central_pairing_enable = 0;
                tlkapi_send_string_data(APP_PAIR_LOG_EN, "[UI][PAIR] Pair end", 0, 0);
            }
        }
        else{
            tlkapi_send_string_data(APP_PAIR_LOG_EN, "[APP] create connection fail", &status, 1);
        }
    }
    /*********************** Central Create connection demo code end  *******************************************************/


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

    if(pConnEvt->status == BLE_SUCCESS){

        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection complete event", &pConnEvt->connHandle, sizeof(hci_le_connectionCompleteEvt_t) - 2);

        dev_char_info_insert_by_conn_event(pConnEvt);

        if(pConnEvt->role == ACL_ROLE_CENTRAL) // central role, process SMP and SDP if necessary
        {
            #if (ACL_CENTRAL_SMP_ENABLE)
                central_smp_pending = pConnEvt->connHandle; // this connection need SMP
            #endif
        }

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
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] disconnect event", &pDisConn->connHandle, 3);

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

    app_bqb_disconn(pDisConn->connHandle);

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
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE)  // connection complete
            {
                app_le_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)  // ADV packet
            {
                //after controller is set to scan state, it will report all the adv packet it received by this event

                app_le_adv_report_event_handle(p);
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
    //tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] host event", &event, 1);

    switch(event)
    {
        case GAP_EVT_SMP_PAIRING_BEGIN:
        {
            gap_smp_pairingBeginEvt_t *pEvt = (gap_smp_pairingBeginEvt_t *)para;
            tlkapi_printf(APP_HOST_EVT_LOG_EN, "[GAP][EVT] pairing begin[SC:%d][Method:%d]", pEvt->secure_conn, pEvt->tk_method);
        }
        break;

        case GAP_EVT_SMP_PAIRING_SUCCESS:
        {
            tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] pairing success", 0, 0);
        }
        break;

        case GAP_EVT_SMP_PAIRING_FAIL:
        {
            gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;

        #if (ACL_CENTRAL_SMP_ENABLE)
            if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){
                if(central_smp_pending == p->connHandle){
                    central_smp_pending = 0;
                }
            }
        #endif

            switch (p->reason) {
                case PAIRING_FAIL_REASON_PAIRING_NOT_SUPPORTED: //0x05
                    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "SMP Pair Fail: Pair not supported", 0, 0);
                    break;

                case PAIRING_FAIL_REASON_UNSPECIFIED_REASON:    //0x08
                    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "SMP Pair Fail: Unspecified reason", 0, 0);
                    break;

                case PAIRING_FAIL_REASON_INVALID_PARAMETER: //0x0A. Cause: MTU<65 under SC
                    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "SMP Pair Fail: Invalid param", 0, 0);
                    break;

                case PAIRING_FAIL_REASON_PAIRING_TIMEOUT:   //0x80. Cause: Not input pinCode in 30s.
                    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "SMP Pair Fail: Timeout", 0, 0);
                    break;

                default:
                    tlkapi_printf(APP_HOST_EVT_LOG_EN, "SMP Pair Fail: Unknown reason: 0x%02X", p->reason);
                    break;
            }

            //blc_ll_disconnect(p->connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        }
        break;

        case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
        {
            tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] encryption done", 0, 0);
        }
        break;

        case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
        {
            tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] security done", 0, 0);

        #if (ACL_CENTRAL_SMP_ENABLE)
            gap_smp_connEncDoneEvt_t* p = (gap_smp_connEncDoneEvt_t*)para;
            //app_discovery_init(p->connHandle);
            if( dev_char_get_conn_role_by_connhandle(p->connHandle) == ACL_ROLE_CENTRAL){

                if(central_smp_pending == p->connHandle){
                    central_smp_pending = 0;
                }

            }
        #endif
        }
        break;

        case GAP_EVT_SMP_TK_DISPLAY:
        {
            gap_smp_TkDisplayEvt_t* p = (gap_smp_TkDisplayEvt_t*)para;
            tlkapi_printf(APP_HOST_EVT_LOG_EN, "[GAP][EVT] TK display: %06d.\n", p->tk_pincode);
        }
        break;

        case GAP_EVT_SMP_TK_REQUEST_PASSKEY:
        {
            gap_smp_TkReqPassKeyEvt_t* pEvt = (gap_smp_TkReqPassKeyEvt_t*)para;
            tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "Request PK", &pEvt->connHandle, 2);

//          blc_smp_sendKeypressNotify(pEvt->connHandle, KEYPRESS_NTF_PKE_START);

            if( dev_char_get_conn_role_by_connhandle(pEvt->connHandle) == ACL_ROLE_CENTRAL) //for Central
            {
                if(blc_smp_setTK_by_PasskeyEntry(pEvt->connHandle, 123456)){
                    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] Set TK SUCC: <<123456>>", 0, 0);
//                  blc_smp_sendKeypressNotify(pEvt->connHandle, KEYPRESS_NTF_PKE_COMPLETED);
                }
                else{
                    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] Set TK FAIL", 0, 0);
                }
            }
        }
        break;

        case GAP_EVT_SMP_TK_REQUEST_OOB:
        {
            gap_smp_TkRequestOOBEvt_t* pEvt = (gap_smp_TkRequestOOBEvt_t*)para;
            tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "Request OOB", &pEvt->connHandle, 2);
//          tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "scOobUsed", &pEvt->scOobUsed, 1);
//          if(pEvt->scOobUsed)
//          {
//              tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "addr", pEvt->addr, 6);
//              tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "ra", pEvt->ra, 16);
//              tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "comfirm", pEvt->comfirm, 16);
//
//              if(blc_smp_setTK_by_scOOB(pEvt->connHandle, pEvt->ra, pEvt->comfirm))
//              {
//                  tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] OOB Communication SUCC", 0, 0);
//              }
//              else
//              {
//                  tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] OOB Communication FAIL", 0, 0);
//              }
//          }
//          else
//          {
//              /*
//               * ******************* Attention ******************
//               * ICS/IXIT: TSPX_OOB_Data: 00112233445566778899AABBCCDDEEFF
//               */
//              u8 oobData[16] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
//              swapN(oobData, 16);
//              if(blc_smp_setTK_by_OOB(pEvt->connHandle, oobData))
//              {
//                  tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] Set OOB TK SUCC", oobData, 16);
//              }
//              else
//              {
//                  tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[GAP][EVT] Set OOB TK FAIL", oobData, 16);
//              }
//          }
        }
        break;

        case GAP_EVT_SMP_TK_NUMERIC_COMPARE:
        {
            gap_smp_TkDisplayEvt_t* pEvt = (gap_smp_TkDisplayEvt_t*)para;
            tlkapi_printf(APP_HOST_EVT_LOG_EN, "Numeric Comparison Pincode = %06d.\n", pEvt->tk_pincode);
        }
        break;

//      case GAP_EVT_SMP_KEYPRESS_NOTIFY:
//      {
//          gap_smp_keypressNotifyEvt_t* pEvt = (gap_smp_keypressNotifyEvt_t*)para;
//          tlkapi_printf(APP_HOST_EVT_LOG_EN, "Keypress notify type %01d.\n", pEvt->ntfType);
//      }
//      break;

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



/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler (u16 connHandle, u8 *pkt)
{
    if( dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL)   //GATT data for Central
    {
        rf_packet_att_t *pAtt = (rf_packet_att_t*)pkt;

        dev_char_info_t* dev_info = dev_char_info_search_by_connhandle (connHandle);
        if(dev_info)
        {
            //-------   user process ------------------------------------------------
            if(pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI)  //peripheral handle notify
            {

            }
            else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND)
            {

            }
        }

        /* The Central does not support GATT Server by default */
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


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);                         //mandatory

    blc_ll_initLegacyAdvertising_module();

    blc_ll_initLegacyScanning_module();

    blc_ll_initLegacyInitiating_module();

    blc_ll_initAclConnection_module();
    blc_ll_initAclCentralRole_module();
    blc_ll_initAclPeriphrRole_module();



    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Central TX FIFO */
    blc_ll_initAclCentralTxFifo(app_acl_cen_tx_fifo, ACL_CENTRAL_TX_FIFO_SIZE, ACL_CENTRAL_TX_FIFO_NUM, ACL_CENTRAL_MAX_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);


    blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_31P25MS);

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
                                    |   HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);


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
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, NULL, 0);
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU); ///must be placed after "blc_gap_init"
    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU);   ///must be placed after "blc_gap_init"

    /* GATT Initialization */
    my_gatt_init();
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
    blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN            |
                          GAP_EVT_MASK_SMP_PAIRING_SUCCESS          |
                          GAP_EVT_MASK_SMP_PAIRING_FAIL             |
                          GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE    |
                          GAP_EVT_MASK_SMP_TK_DISPLAY               |
                          GAP_EVT_MASK_SMP_TK_REQUEST_PASSKEY       |
                          GAP_EVT_MASK_SMP_TK_REQUEST_OOB           |
                          GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE       |
                          GAP_EVT_MASK_SMP_BONDING_INFO_FULL        |
//                        GAP_EVT_MASK_SMP_KEYPRESS_NOTIFY          |
                          GAP_EVT_MASK_ATT_EXCHANGE_MTU);
    //////////// Host Initialization  End /////////////////////////

//////////////////////////// BLE stack Initialization  End //////////////////////////////////



//////////////////////////// User Configuration for BLE application ////////////////////////////
    blc_ll_setAdvData(tbl_advData, sizeof(tbl_advData));
    blc_ll_setScanRspData(tbl_scanRsp, sizeof(tbl_scanRsp));
    blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable

    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
    blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);

    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] host bqb test init", 0, 0);

    blc_svc_addCoreGroup();
    blc_svc_gattCbackRegister();
    blc_svc_addBqbGattGroup();
    blc_svc_calculateDatabaseHash();
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

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
int main_idle_loop (void)
{

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif
    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard (0, 0, 0);
    #endif

    proc_central_role_unpair();
    //app_discovery_loop();
    return 0; //must return 0 due to SDP flow
}



_attribute_no_inline_ void main_loop (void)
{
    main_idle_loop ();
}

#endif
