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
#include "app_buffer.h"
#include "app.h"
#include "app_ull/app_ull_hid.h"
#include "../ull_hid_config.h"
#if (ULL_HID_DEMO_SLECT == ULL_HID_DEVICE)

app_ullhid_param_t ullhidParam;

/**
 * @brief      BLE enhanced connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_enhanced_connection_complete_event_handle(u8 *p)
{
    hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

    if (pConnEvt->status == BLE_SUCCESS) {
        tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT] Enhanced Conn complete event", &pConnEvt->connHandle, sizeof(hci_le_enhancedConnCompleteEvt_t) - 2);

        dev_char_info_insert_by_enhanced_conn_event(pConnEvt);

        app_ull_hid_acl_connect(pConnEvt->connHandle, pConnEvt->connInterval);
        tlkapi_printf(APP_LOG_EN, "ACL connection interval is %.2fms\n", pConnEvt->connInterval * 1.25);

        if (pConnEvt->role == ACL_ROLE_PERIPHERAL) {
    #if (ADV_USE_EXT_MODE)
            blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);
    #endif
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

    tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT] disconnect event", &pDisConn->connHandle, 3);

    app_ull_hid_cis_disconnect(pDisConn->connHandle);

    if ((pDisConn->connHandle & 0xC0) == 0) {
        return 0;
    }

    app_ull_hid_acl_disconnect(pDisConn->connHandle);
    if (dev_char_get_conn_role_by_connhandle(pDisConn->connHandle) == ACL_ROLE_PERIPHERAL) {
    #if (ADV_USE_EXT_MODE)
        blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
    #endif
    }

    dev_char_info_delete_by_connhandle(pDisConn->connHandle);


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
            //------hci le event: le enhanced_connection complete event---------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_ENHANCED_CONNECTION_COMPLETE) // connection complete
            {
                app_le_enhanced_connection_complete_event_handle(p);
            }
            //------hci le event: le connection update complete event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CONNECTION_UPDATE_COMPLETE) // connection update
            {
                hci_le_connectionUpdateCompleteEvt_t *pUpdateEvt = (hci_le_connectionUpdateCompleteEvt_t *)p;
                app_ull_hid_acl_connect(pUpdateEvt->connHandle, pUpdateEvt->connInterval);
                tlkapi_printf(APP_LOG_EN, "ACL connection interval update, new interval is %.2fms\n", pUpdateEvt->connInterval * 1.25);
            }
            //------HCI LE event: LE CIS Established event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CIS_ESTABLISHED) {
                hci_le_cisEstablishedEvt_t *pCisEstbEvent = (hci_le_cisEstablishedEvt_t *)p;
                app_ull_hid_cis_connect(pCisEstbEvent->cisHandle, pCisEstbEvent->isoIntvl, pCisEstbEvent->nse, pCisEstbEvent->maxPDU_s2m);

                u8 status = blc_ll_setupCisDataPath(pCisEstbEvent->cisHandle, Data_Dir_Input, Data_Path_HCI, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                tlkapi_send_string_data(APP_LOG_EN, "[APP][EVT] Setup_ISO_Data_Path", &status, 1);
                tlkapi_printf(APP_LOG_EN, "[APP][EVT] CIS SDU interval is %dus", pCisEstbEvent->isoIntvl * 1250 / pCisEstbEvent->nse);
            }
            //------HCI LE event: LE CIS Request event -------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CIS_REQUEST) // connection update
            {
                hci_le_cisReqEvt_t *pCisReqEvt = (hci_le_cisReqEvt_t *)p;
                if (dev_char_info_is_connection_state_by_conn_handle(pCisReqEvt->aclHandle)) {
                    blc_ll_acceptCisRequest(pCisReqEvt->cisHandle);
                }
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
        gap_smp_pairingBeginEvt_t *p = (gap_smp_pairingBeginEvt_t *)para;
        (void)p; //remove compiler warning
    } break;

    case GAP_EVT_SMP_PAIRING_SUCCESS:
    {
        gap_smp_pairingSuccessEvt_t *p = (gap_smp_pairingSuccessEvt_t *)para;
        (void)p; //remove compiler warning
    } break;

    case GAP_EVT_SMP_PAIRING_FAIL:
    {
        gap_smp_pairingFailEvt_t *p = (gap_smp_pairingFailEvt_t *)para;
        (void)p; //remove compiler warning
    } break;

    case GAP_EVT_SMP_CONN_ENCRYPTION_DONE:
    {
    } break;

    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
        gap_smp_connEncDoneEvt_t *p = (gap_smp_connEncDoneEvt_t *)para;
        (void)p; //remove compiler warning

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
 * @brief      BLE GATT data handler call-back.
 * @param[in]  connHandle     connection handle.
 * @param[in]  pkt             Pointer point to data packet buffer.
 * @return
 */
int app_gatt_data_handler(u16 connHandle, u8 *pkt)
{
    if (dev_char_get_conn_role_by_connhandle(connHandle) == ACL_ROLE_CENTRAL) //GATT data for Central
    {
        rf_packet_att_t *pAtt = (rf_packet_att_t *)pkt;

        dev_char_info_t *dev_info = dev_char_info_search_by_connhandle(connHandle);
        if (dev_info) {
            //-------   user process ------------------------------------------------
            if (pAtt->opcode == ATT_OP_HANDLE_VALUE_NOTI) {
            } else if (pAtt->opcode == ATT_OP_HANDLE_VALUE_IND) {
            }
        }
    } else {
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
    tlkapi_debug_customize_usb_id(0x123);
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

    blc_ll_initStandby_module(mac_public);

    #if (ADV_USE_EXT_MODE)
    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);
        //blc_ll_initExtendedScanRspDataBuffer(app_extScanRspData_buf, APP_EXT_SCANRSP_DATA_LENGTH);
    #else
    blc_ll_initLegacyAdvertising_module();
    #endif

    blc_ll_initAclConnection_module();

    blc_ll_initAclPeriphrRole_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

    rf_set_power_level_index(RF_POWER_P3dBm);

    //////////// LinkLayer Initialization  End /////////////////////////


    ///// CIS Peripheral initialization //////////////
    blc_ll_initCisPeriphrModule_initCisPeriphrParametersBuffer(app_cis_per_param, APP_CIS_PERIPHR_NUMBER);

    blc_ll_initCisConnModule_initCisConnParametersBuffer(app_cis_conn_param, APP_CIS_CENTRAL_NUMBER, APP_CIS_PERIPHR_NUMBER);

    /* CIS RX PDU buffer initialization */
    blc_ll_initCisRxFifo(app_cis_rxPduFifo, CIS_RX_PDU_FIFO_SIZE, CIS_RX_PDU_FIFO_NUM);
    /* CIS TX PDU buffer initialization */
    blc_ll_initCisTxFifo(app_cis_txPduFifo, CIS_TX_PDU_FIFO_SIZE, CIS_TX_PDU_FIFO_NUM);

    /* CIS SDU in & out buffer initialization */
    blc_ll_initCisSduBuffer(app_cis_sdu_in_fifo, CIS_SDU_IN_FIFO_SIZE, CIS_SDU_IN_FIFO_NUM, app_cis_sdu_out_fifo, CIS_SDU_OUT_FIFO_SIZE, CIS_SDU_OUT_FIFO_NUM);

    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerDataHandler(blc_l2cap_pktHandler_5_3);

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE | HCI_LE_EVT_MASK_CIS_ESTABLISHED | HCI_LE_EVT_MASK_CIS_REQUESTED);
    u8 error_code = blc_contr_checkControllerInitialization();
    if (error_code != INIT_SUCCESS) {
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
    #endif
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] Controller INIT ERROR", &error_code, 1);
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

    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU);    ///must be placed after "blc_gap_init"
    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU); ///must be placed after "blc_gap_init"

    blc_gatt_register_data_handler(app_gatt_data_handler);

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (ACL_PERIPHR_SMP_ENABLE)                                               //Slave SMP Enable
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
        //blc_smp_configSecurityRequestSending(SecReq_PEND_SEND, SecReq_NOT_SEND, 100);
    #else
    blc_smp_setSecurityLevel_periphr(No_Security);
    #endif

    blc_smp_smpParamInit();

    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler(app_host_event_callback);
    blc_gap_setEventMask(GAP_EVT_MASK_SMP_PAIRING_BEGIN |
                         GAP_EVT_MASK_SMP_PAIRING_SUCCESS |
                         GAP_EVT_MASK_SMP_PAIRING_FAIL |
                         GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE);
    //////////// Host Initialization  End /////////////////////////

    //////////////////////////// BLE stack Initialization  End //////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////


    #if (BLE_APP_PM_ENABLE)
    blc_ll_initPowerManagement_module();
        //blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_EXT_ADV | PM_SLEEP_ACL_PERIPHR | PM_SLEEP_CIS_PERIPHR);
    #endif


    //////////////////////////// User Configuration for BLE application ////////////////////////////
    #if (ADV_USE_EXT_MODE)
    blc_ll_setExtAdvParam(ADV_HANDLE0, ADV_EVT_PROP_LEGACY_CONNECTABLE_SCANNABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);


    /**
     * @brief   BLE Advertising data
     */
    u8 tbl_advData[] = {
        13,
        DT_COMPLETE_LOCAL_NAME,
        'T',
        'L',
        'S',
        'R',
        '9',
        '2',
        '1',
        '7',
        '-',
        'U',
        'L',
        'L',
        2,
        DT_FLAGS,
        0x05, // BLE limited discoverable mode and BR/EDR not supported
        3,
        DT_COMPLETE_LIST_16BIT_SERVICE_UUID,
        U16_TO_BYTES(SERVICE_UUID_ULL_HID)};

    blc_ll_setExtAdvData(ADV_HANDLE0, sizeof(tbl_advData), (u8 *)tbl_advData);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
    #else
    u8 tbl_advData[] = {
        8,
        DT_COMPLETE_LOCAL_NAME,
        'U',
        'L',
        'L',
        '-',
        'H',
        'I',
        'D',
        2,
        DT_FLAGS,
        0x05, // BLE limited discoverable mode and BR/EDR not supported
        3,
        DT_COMPLETE_LIST_16BIT_SERVICE_UUID,
        U16_TO_BYTES(SERVICE_UUID_ULL_HID)};
    u8 tbl_scanRsp[] = {
        2,
        DT_FLAGS,
        0x05,
    };

    blc_ll_setAdvData(tbl_advData, sizeof(tbl_advData));
    blc_ll_setScanRspData(tbl_scanRsp, sizeof(tbl_scanRsp));
    blc_ll_setAdvParam(ADV_INTERVAL_50MS, ADV_INTERVAL_50MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
    #endif

    app_initial_ull_hid_device();

    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] cis peripheral init", 0, 0);
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{
}

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();
    blc_prf_main_loop();

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////

    static u32 ledScanTick = 0;
    static int loop_cnt    = 0;
    if (clock_time_exceed(ledScanTick, 1000 * 1000)) {
        loop_cnt++;
        ledScanTick = clock_time();
        gpio_toggle(GPIO_LED_WHITE);
    }
    app_ull_hid_device_main_loop();
    ////////////////////////////////////// PM entry /////////////////////////////////
    #if (BLE_APP_PM_ENABLE)
    if (tlkapi_debug_isBusy()) {
        blc_pm_setSleepMask(PM_SLEEP_DISABLE);
    } else {
        //blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_EXT_ADV | PM_SLEEP_ACL_PERIPHR | PM_SLEEP_CIS_PERIPHR);
    }
    #endif
}

#endif //INTER_TEST_MODE == TEST_ULL_HID_DEVICE
