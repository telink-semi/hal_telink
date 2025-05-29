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

#include "app.h"
#include "app_buffer.h"
#include "app_hdt.h"

#if (INTER_TEST_MODE == TEST_HDT_RECIPIENT)

/**
 * @brief   BLE Advertising data
 */
const u8 tbl_advData[] = {
    8,       DT_COMPLETE_LOCAL_NAME,
    'H',      'D',
    'T',      '-',
    'T',      'L',
    'K',      2,
    DT_FLAGS, 0x05, // BLE limited discoverable mode and BR/EDR not supported
    3,        DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    0x5B,     0x18, // incomplete list of 0x185B(Ranging))
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8 tbl_scanRsp[] = {
    8, DT_COMPLETE_LOCAL_NAME, 'H', 'D', 'T', '-', 'T', 'L', 'K',
};


/**
 * @brief      BLE Connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
_attribute_ble_data_retention_ u8 dbg_req_cnt = 0;

int app_le_connection_complete_event_handle(u8 *p)
{
    hci_le_connectionCompleteEvt_t *pConnEvt = (hci_le_connectionCompleteEvt_t *)p;

    if (pConnEvt->status == BLE_SUCCESS) {
        tlkapi_send_string_data(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Connection complete event", &pConnEvt->connHandle, sizeof(hci_le_connectionCompleteEvt_t) - 2);

#if (UI_LED_ENABLE)
        //led show connection state
        gpio_write(GPIO_LED_RED, 1);
#endif

        dev_char_info_insert_by_conn_event(pConnEvt);

        if (pConnEvt->role == ACL_ROLE_PERIPHERAL) {
            bls_l2cap_requestConnParamUpdate(pConnEvt->connHandle, CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 20, CONN_TIMEOUT_4S); // 1 second
        }
    }

    return 0;
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

#if (UI_LED_ENABLE)
    //led show connection state
    gpio_write(GPIO_LED_RED, 0);
#endif

    //terminate reason
    if (pDisConn->reason == HCI_ERR_CONN_TIMEOUT) { //connection timeout

    } else if (pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN) { //peer device send terminate command on link layer

    }
    //central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
    else if (pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST) {
    } else {
    }

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

    if (pUpt->status == BLE_SUCCESS) {}

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
        } else if (evtCode == HCI_EVT_LE_META) //LE Event
        {
            u8 subEvt_code = p[0];
            //------hci le event: le connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE) // connection complete
            {
                app_le_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT) // ADV packet
            {
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE) // connection update
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
int app_host_event_callback(u32 h, u8 *para, int n)
{
    (void)n;
    (void)para;

    u8 event = h & 0xFF;

    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[APP][EVT] host event", &event, 1);

    switch (event) {
    case GAP_EVT_SMP_PAIRING_BEGIN:
    {
    } break;

    case GAP_EVT_SMP_PAIRING_SUCCESS:
    {
    } break;

    case GAP_EVT_SMP_PAIRING_FAIL:
    {
    } break;

    case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
    {
    } break;

    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
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

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_set_flag_suspend_exit(u8 e, u8 *p, int n)
{
    (void)e;
    (void)p;
    (void)n;
    rf_set_power_level_index(RF_POWER_P0dBm);
}



_attribute_ram_code_ void app_process_power_management(u8 e, u8 *p, int n)
{
    (void)e;
    (void)n;
    (void)p;

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
    tlkapi_debug_customize_usb_id(0x123);
    tlkapi_debug_init();
    blc_debug_enableStackLog(STK_LOG_SMP_LTK);
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

    //////////////////////////// basic hardware Initialization  End /////////////////////////////////


    //////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8 mac_public[6];
    u8 mac_random_static[6];
    /* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

    blc_ll_initLegacyAdvertising_module();

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
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);

    //////////// HCI Initialization  End /////////////////////////

    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU); ///must be placed after "blc_gap_init"

/* SMP Initialization */
#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    /* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
#endif

#if (ACL_PERIPHR_SMP_ENABLE)                                                   //Peripheral SMP Enable
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
#else
    blc_smp_setSecurityLevel_periphr(No_Security);
#endif

    blc_smp_smpParamInit();
    blc_smp_configSecurityRequestSending(
        SecReq_IMM_SEND, SecReq_PEND_SEND,
        1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection)

    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler(app_host_event_callback);

    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN | GAP_EVT_MASK_SMP_PAIRING_SUCCESS | GAP_EVT_MASK_SMP_PAIRING_FAIL | GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE |
                         GAP_EVT_MASK_SMP_CONN_ENCRYPTION_DONE);

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
        while (1);
#endif
    }

    //////////// Host Initialization  End /////////////////////////
//////////////////////////// BLE stack Initialization  End //////////////////////////////////

//////////////////////////// User Configuration for BLE application ////////////////////////////

    blc_ll_setAdvData(tbl_advData, sizeof(tbl_advData));
    blc_ll_setScanRspData(tbl_scanRsp, sizeof(tbl_scanRsp));
    blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
    //blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);

    rf_set_power_level_index(RF_POWER_P9dBm);


#if (UI_KEYBOARD_ENABLE)
    keyboard_init();
#endif

    app_higher_data_throughput_init();

    tlkapi_printf(APP_LOG_EN, "[APP][INI] Recipient demo init finished");
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void user_init_deepRetn(void)
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


#if (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
#endif

    ////////////////////////////////////// PM entry /////////////////////////////////

    return 0; //must return 0 due to SDP flow
}

u32 ledToggleTick = 0;

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
        gpio_toggle(GPIO_LED_GREEN);
    }
#endif
    main_idle_loop();
}

#endif
