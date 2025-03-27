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


#if (FEATURE_TEST_MODE == TEST_PRIVACY_PERIPHERAL)

/**
 * @brief   BLE Advertising data
 */
const u8    tbl_advData[] = {
     8,  DT_COMPLETE_LOCAL_NAME,                'p','r','i','v','a','c','y',
     2,  DT_FLAGS,                              0x05,                   // BLE limited discoverable mode and BR/EDR not supported
     3,  DT_APPEARANCE,                         0x80, 0x01,             // 384, Generic Remote Control, Generic category
     5,  DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID, 0x12, 0x18, 0x0F, 0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8    tbl_scanRsp [] = {
     8, DT_COMPLETE_LOCAL_NAME,                 'p','r','i','v','a','c','y',
};

#if(PRIVACY_TEST_MODE == EXTEND_ADV_TEST)
#define APP_EXT_ADV_DATA_LENGTH         100 //user set value
#define APP_EXT_SCANRSP_DATA_LENGTH     100 //user set value

_attribute_ble_data_retention_  u8  app_extAdvSetParam_buf[ADV_SET_PARAM_LENGTH];

_attribute_iram_noinit_data_    u8  app_extAdvData_buf[APP_EXT_ADV_DATA_LENGTH];

_attribute_iram_noinit_data_    u8  app_extScanRspData_buf[APP_EXT_SCANRSP_DATA_LENGTH];
#endif

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
        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Enhanced Conn complete event", &pConnEvt->connHandle, sizeof(hci_le_enhancedConnCompleteEvt_t) - 2);

        dev_char_info_insert_by_enhanced_conn_event(pConnEvt);


        if(pConnEvt->role == ACL_ROLE_PERIPHERAL){

        }

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

    dev_char_info_delete_by_connhandle(pDisConn->connHandle);

    #if(PRIVACY_TEST_MODE == LEGACY_ADV_TEST)
        app_configLegacyAdvParam();
    #elif(PRIVACY_TEST_MODE == EXTEND_ADV_TEST)
        app_configExtendAdvParam();
        blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
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
    tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection Update Event", &pUpt->connHandle, 8);

    if(pUpt->status == BLE_SUCCESS){

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
    (void)n; //unused, remove warning
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
            if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE) // connection complete
            {
                app_le_enhanced_connection_complete_event_handle(p);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE)  // connection update
            {
                app_le_connection_update_complete_event_handle(p);
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
    (void)n; //unused, remove warning
    u8 event = h & 0xFF;

    switch(event)
    {
        case GAP_EVT_SMP_PAIRING_BEGIN:
        {
            tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] Pairing Begin",0,0);
        }
        break;

        case GAP_EVT_SMP_PAIRING_SUCCESS:
        {
            gap_smp_pairingSuccessEvt_t* p = (gap_smp_pairingSuccessEvt_t*)para;
            tlkapi_send_string_u8s(APP_LOG_EN,"[APP][SMP] Pairing success,bond flg", p->bonding ?1:0,0,0,0);

            if(p->bonding_result){
                tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] save smp key succ",0,0);
            }
            else{
                tlkapi_send_string_data(APP_LOG_EN,"[APP][SMP] save smp key failed",0,0);
            }
        }
        break;

        case GAP_EVT_SMP_PAIRING_FAIL:
        {

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

    blc_ll_initStandby_module(mac_public);

    #if(PRIVACY_TEST_MODE == LEGACY_ADV_TEST)
        blc_ll_initLegacyAdvertising_module();
    #elif(PRIVACY_TEST_MODE == EXTEND_ADV_TEST)
        /* Extended ADV module and ADV Set Parameters buffer initialization */
        blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, 1);
        blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);
        blc_ll_initExtendedScanRspDataBuffer(app_extScanRspData_buf, APP_EXT_SCANRSP_DATA_LENGTH);
    #endif

    blc_ll_initAclConnection_module();

    blc_ll_initAclPeriphrRole_module();


    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);


    //////////// LinkLayer Initialization  End /////////////////////////



    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler (blc_l2cap_pktHandler);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd (HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(        HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE  \
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
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU);   ///must be placed after "blc_gap_init"

    /* GATT Initialization */
    my_gatt_init();

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE)
        
        blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (ACL_PERIPHR_SMP_ENABLE)
        blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption);  //LE_Security_Mode_1_Level_2
    #else
        blc_smp_setSecurityLevel(No_Security);
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
    #if(PRIVACY_TEST_MODE == LEGACY_ADV_TEST)
        blc_ll_setAdvData((const u8 *)tbl_advData, sizeof(tbl_advData));
        blc_ll_setScanRspData((const u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
        app_configLegacyAdvParam(); ///note: this API set resolve list.
    #elif(PRIVACY_TEST_MODE == EXTEND_ADV_TEST)
        app_configExtendAdvParam(); ///note: this API set resolve list.
        blc_ll_setExtAdvData(ADV_HANDLE0, sizeof(tbl_advData), (const u8 *)tbl_advData); //Attention: this API must be called after "app_configExtendAdvParam".
        blc_ll_setScanRspData((const u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));    //Attention: this API must be called after "app_configExtendAdvParam".
        blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);  //Attention: this API must be called after "blc_ll_setExtAdvData" and "blc_ll_setScanRspData".
        tlkapi_send_string_data(APP_SMP_LOG_EN, "[APP][RPA] ADV Config Done", 0, 0);
    #endif
    rf_set_power_level_index(RF_POWER_P3dBm);

    #if (BLE_APP_PM_ENABLE)
        blc_ll_initPowerManagement_module();
        blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_PERIPHR | PM_SLEEP_ACL_CENTRAL);
    #endif


    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] FEATURE_PRIVACY init", 0, 0);
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
}

#endif //end of (FEATURE_TEST_MODE == ...)



