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

#if (INTER_TEST_MODE == TEST_ONCA)

_attribute_ble_data_retention_ u8 ota_is_working = 0;


/**
 * @brief   BLE Advertising data
 */
const u8 tbl_advData[] = {
    13,
    DT_COMPLETE_LOCAL_NAME,
    'p',
    'e',
    'r',
    'i',
    'p',
    'h',
    'r',
    '_',
    'd',
    'e',
    'm',
    'o',
    2,
    DT_FLAGS,
    0x05, // BLE limited discoverable mode and BR/EDR not supported
    3,
    DT_APPEARANCE,
    0x80,
    0x01, // 384, Generic Remote Control, Generic category
    5,
    DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    0x12,
    0x18,
    0x0F,
    0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8 tbl_scanRsp[] = {
    13,
    DT_COMPLETE_LOCAL_NAME,
    'p',
    'e',
    'r',
    'i',
    'p',
    'h',
    'r',
    '_',
    'd',
    'e',
    'm',
    'o',
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
            //bls_l2cap_requestConnParamUpdate(pConnEvt->connHandle, CONN_INTERVAL_20MS, CONN_INTERVAL_20MS, 49, CONN_TIMEOUT_4S);  // 1 second
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
    if (pDisConn->reason == HCI_ERR_CONN_TIMEOUT) {                 //connection timeout

    } else if (pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN) { //peer device send terminate command on link layer

    }
    //central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
    else if (pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST) {
    } else {
    }

    dev_char_info_delete_by_connhandle(pDisConn->connHandle);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
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
            u8 subEvt_code = p[0];

            //------hci le event: le connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_COMPLETE) // connection complete
            {
                //              blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
                app_le_connection_complete_event_handle(p);
            }
            //--------hci le event: le adv report event ----------------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT) // ADV packet
            {
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE) // connection update
            {
                //              blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
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
    (void)para;
    (void)n;
    u8 event = h & 0xFF;

    tlkapi_send_string_data(APP_HOST_EVT_LOG_EN, "[APP][EVT] host event", &event, 1);

    return 0;
}

/**
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt)
{
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
}

void gpio_test_initial(void)
{
    gpio_function_en(GPIO_CHN0);
    gpio_function_en(GPIO_CHN1);
    gpio_function_en(GPIO_CHN2);
    gpio_function_en(GPIO_CHN3);
    gpio_function_en(GPIO_CHN4);
    gpio_function_en(GPIO_CHN5);
    gpio_function_en(GPIO_CHN6);
    gpio_function_en(GPIO_CHN7);
    gpio_function_en(GPIO_CHN8);
    gpio_function_en(GPIO_CHN9);
    gpio_function_en(GPIO_CHN10);
    gpio_function_en(GPIO_CHN11);
    gpio_function_en(GPIO_CHN12);
    gpio_function_en(GPIO_CHN13);
    gpio_function_en(GPIO_CHN14);
    gpio_function_en(GPIO_CHN15);

    gpio_output_en(GPIO_CHN0);
    gpio_output_en(GPIO_CHN1);
    gpio_output_en(GPIO_CHN2);
    gpio_output_en(GPIO_CHN3);
    gpio_output_en(GPIO_CHN4);
    gpio_output_en(GPIO_CHN5);
    gpio_output_en(GPIO_CHN6);
    gpio_output_en(GPIO_CHN7);
    gpio_output_en(GPIO_CHN8);
    gpio_output_en(GPIO_CHN9);
    gpio_output_en(GPIO_CHN10);
    gpio_output_en(GPIO_CHN11);
    gpio_output_en(GPIO_CHN12);
    gpio_output_en(GPIO_CHN13);
    gpio_output_en(GPIO_CHN14);
    gpio_output_en(GPIO_CHN15);
}

u8  advData[255];
u16 advLen;

void ble_legacy_adv_ind_test(void)
{
    blc_ll_setAdvData(advData, advLen);
    blc_ll_setScanRspData(advData, advLen);
    blc_ll_setAdvParam(ADV_INTERVAL_40MS, ADV_INTERVAL_45MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
}

void ble_legacy_nonconn_ind_test(void)
{
    blc_ll_setAdvData(advData, advLen);
    blc_ll_setScanRspData(advData, advLen);
    blc_ll_setAdvParam(ADV_INTERVAL_40MS, ADV_INTERVAL_45MS, ADV_TYPE_NONCONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
}

void ble_legacy_adv_scan_ind_test(void)
{
    blc_ll_setAdvData(advData, advLen);
    blc_ll_setScanRspData(advData, advLen);
    blc_ll_setAdvParam(ADV_INTERVAL_40MS, ADV_INTERVAL_45MS, ADV_TYPE_SCANNABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
}

void ble_legacy_adv_indirect_ind_test(void)
{
    blc_ll_setAdvData(advData, advLen);
    blc_ll_setScanRspData(advData, advLen);
    blc_ll_setAdvParam(ADV_INTERVAL_40MS, ADV_INTERVAL_45MS, ADV_TYPE_CONNECTABLE_DIRECTED_LOW_DUTY, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
}

void ble_legacy_adv_test(void)
{
    blc_ll_initLegacyAdvertising_module();

    ble_legacy_adv_ind_test();
    //  ble_legacy_nonconn_ind_test();
}

void ble_extend_adv_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_adv_direct_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_LEGACY_CONNECTABLE_DIRECTED_LOW_DUTY, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_adv_scan_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_LEGACY_SCANNABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_adv_nonconn_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_LEGACY_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_nonconn_nonscan_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_conn_undirected_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_scan_undirected_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_EXTENDED_SCANNABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_nonconn_nonscan_undirected_ind_test(adv_handle_t handle)
{
    //Legacy, Connectable_Scannable, Undirected
    blc_ll_setExtAdvParam(handle, ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_DIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_35MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
    blc_ll_setExtAdvData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtScanRspData(handle, advLen, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, handle, 0, 0);
}

void ble_extend_adv_test(void)
{
    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);
    blc_ll_initExtendedScanRspDataBuffer(app_extScanRspData_buf, APP_EXT_SCANRSP_DATA_LENGTH);
    u8 adv_handle = ADV_HANDLE0;
    ble_extend_adv_ind_test(adv_handle++);
    //  ble_extend_adv_direct_ind_test(adv_handle++);
    //  ble_extend_adv_scan_ind_test(adv_handle++);
    ble_extend_adv_nonconn_ind_test(adv_handle++);
    //  ble_extend_nonconn_nonscan_ind_test(adv_handle++);
    //  ble_extend_conn_undirected_ind_test(adv_handle++);
    //  ble_extend_scan_undirected_ind_test(adv_handle++);
    //  ble_extend_nonconn_nonscan_undirected_ind_test(adv_handle++);
}

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
_attribute_no_inline_ void user_init_normal(void)
{
    gpio_test_initial();
    //////////////////////////// basic hardware Initialization  Begin //////////////////////////////////
    /* random number generator must be initiated here( in the beginning of user_init_normal).
     * When deepSleep retention wakeUp, no need initialize again */
    random_generator_init();

    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_init();
    blc_debug_enableStackLog(STK_LOG_NONE);
    #endif

    //////////////////////////// basic hardware Initialization  End /////////////////////////////////


    //////////////////////////// BLE stack Initialization  Begin //////////////////////////////////
    u8 mac_public[6];
    u8 mac_random_static[6];

    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);

    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

    blc_ll_initAclConnection_module();

    blc_ll_initAclPeriphrRole_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_init2MPhyCodedPhy_feature();
    //////////// LinkLayer Initialization  End /////////////////////////


    //////////// HCI Initialization  Begin /////////////////////////

    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU); ///must be placed after "blc_gap_init"

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (ACL_PERIPHR_SMP_ENABLE)                                               //Peripheral SMP Enable
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
    #else
    blc_smp_setSecurityLevel_periphr(No_Security);
    #endif
    blc_smp_setSecurityParameters(Bondable_Mode, 0, LE_Legacy_Pairing, 0, 0, IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    blc_smp_smpParamInit();
    blc_smp_configSecurityRequestSending(SecReq_IMM_SEND, SecReq_PEND_SEND, 1000); //if not set, default is:  send "security request" immediately after link layer connection established(regardless of new connection or reconnection)

    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler(app_host_event_callback);
    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN |
                         GAP_EVT_MASK_SMP_PAIRING_SUCCESS |
                         GAP_EVT_MASK_SMP_PAIRING_FAIL |
                         GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
    //////////// Host Initialization  End /////////////////////////

    blt_host_init();
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE);
    //////////// HCI Initialization  End /////////////////////////
    blc_ll_setDefaultTxPowerLevel(RF_POWER_P0dBm);

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
    tlkapi_printf(1, "onca demo init.\n");
    blc_svc_addCoreGroup();
    blc_svc_addBasGroup();
    blc_svc_addDisGroup();
    blc_svc_addScpsGroup();
    blc_svc_addHidGroup();

    const blc_adv_flags_t advDefFlags = {
        .ltv.len  = 0x02,
        .ltv.type = DT_FLAGS,
        .flags    = 0x05};

    const blc_adv_completeName_t advDefCompleteName = {
        .ltv.len      = sizeof(DEFAULT_DEV_NAME),
        .ltv.type     = DT_COMPLETE_LOCAL_NAME,
        .completeName = DEFAULT_DEV_NAME,
    };

    blc_adv_ltv_t *adv_ltvs[] = {
        (blc_adv_ltv_t *)&advDefFlags,
        (blc_adv_ltv_t *)&advDefCompleteName,
    };
    advLen = blc_adv_buildAdvData(adv_ltvs, ARRAY_SIZE(adv_ltvs), advData);

    #if (MCU_CORE_TYPE == MCU_CORE_TL751X)
    pke_dig_en();

    /*This demo uses the trng module and must call the trng_dig_en function interface.*/
    trng_dig_en();

    ske_dig_en();
        #ifdef SKE_LP_DMA_FUNCTION
    ske_set_tx_dma_config(DMA14);
    ske_set_rx_dma_config(DMA15, DMA_BURST_1_WORD);
        #endif /* SKE_LP_DMA_FUNCTION */

    #endif

    #if (FREERTOS_ENABLE)
    return;
    #endif

    ble_legacy_adv_test();
    //
    //  return ;

    //    ble_extend_adv_test();


    return;


    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* L2CAP data buffer Initialization */
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU); ///must be placed after "blc_gap_init"

    blc_gatt_register_data_handler(app_gatt_data_handler);


    //////////////////////////// BLE stack Initialization  End //////////////////////////////////

    //////////////////////////// User Configuration for BLE application ////////////////////////////


    //blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);


    #if (BLE_APP_PM_ENABLE)
    blc_ll_initPowerManagement_module();
    blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_PERIPHR);

        #if (PM_DEEPSLEEP_RETENTION_ENABLE)
    blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Enable);
    blc_pm_setDeepsleepRetentionThreshold(95);

            #if (MCU_CORE_TYPE == MCU_CORE_B91)
    blc_pm_setDeepsleepRetentionEarlyWakeupTiming(300);
            #elif (MCU_CORE_TYPE == MCU_CORE_B92)
    blc_pm_setDeepsleepRetentionEarlyWakeupTiming(560); //todo ronglu
            #endif
        #else
    blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Disable);
        #endif

    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &user_set_flag_suspend_exit);
    #endif

    #if (UI_KEYBOARD_ENABLE)
    keyboard_init();
    #endif

    #if (BLE_OTA_SERVER_ENABLE)
        #if (TLKAPI_DEBUG_ENABLE)
            /* user can enable OTA flow log in BLE stack */
            //blc_debug_addStackLog(STK_LOG_OTA_FLOW);
        #endif

    blc_ota_initOtaServer_module();
    blc_ota_setOtaProcessTimeout(30);
    #endif
    ////////////////////////////////////////////////////////////////////////////////////////////////

    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] acl peripheral demo init", 0, 0);
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
    #if (PM_DEEPSLEEP_RETENTION_ENABLE)
    blc_app_loadCustomizedParameters_deepRetn();

    blc_ll_initBasicMCU(); //mandatory

    blc_ll_recoverDeepRetention();

    DBG_CHN0_HIGH; //debug
    irq_enable();

        #if (UI_KEYBOARD_ENABLE)
    /////////// keyboard GPIO wakeup init ////////
    u32 pin[] = KB_DRIVE_PINS;
    for (unsigned int i = 0; i < (sizeof(pin) / sizeof(*pin)); i++) {
        cpu_set_gpio_wakeup(pin[i], WAKEUP_LEVEL_HIGH, 1); //drive pin pad high level wakeup deepsleep
            #if FREERTOS_ENABLE
        gpio_set_irq(pin[i], INTR_HIGH_LEVEL);
            #endif
    }
            #if FREERTOS_ENABLE
    gpio_set_irq_mask(GPIO_IRQ_MASK_GPIO);
            #endif
        #endif

        #if (BATT_CHECK_ENABLE)
    adc_hw_initialized = 0;
        #endif

        #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_deepRetn_init();
        #endif
    #endif
}

void app_process_power_management(void)
{
    #if (BLE_APP_PM_ENABLE)
    //Log needs to be output ASAP, and UART invalid after suspend. So Log disable sleep.
    //User tasks can go into suspend, but no deep sleep. So we use manual latency.
    if (tlkapi_debug_isBusy()) {
        blc_pm_setSleepMask(PM_SLEEP_DISABLE);
    } else {
        int user_task_flg = 0;

        blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_PERIPHR);

        #if (BLE_OTA_SERVER_ENABLE)
        user_task_flg |= ota_is_working;
        #endif

        #if (UI_KEYBOARD_ENABLE)
        user_task_flg |= user_task_flg || scan_pin_need || key_not_released;
        #endif

        if (user_task_flg) {
            bls_pm_setManualLatency(0);
        }
    }
    #endif
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

    #if (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
    #endif

    ////////////////////////////////////// PM entry /////////////////////////////////
    app_process_power_management();

    return 0; //must return 0 due to SDP flow
}

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
    main_idle_loop();
}

#endif
