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
#include "app_audio.h"
#include "app_buffer.h"
#include "app_config.h"
#include "app_key.h"
#include "app_led.h"

#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_BASE)

/**
 * @brief      BLE enhanced connection complete event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_enhanced_connection_complete_event_handle(u8 *p)
{
    hci_le_enhancedConnCompleteEvt_t *pConnEvt = (hci_le_enhancedConnCompleteEvt_t *)p;

    if (pConnEvt->status == BLE_SUCCESS) {
        dev_char_info_insert_by_enhanced_conn_event(pConnEvt);

    #if (TLK_LED_ENABLE)
        if (dev_char_get_conn_role_by_connhandle(pConnEvt->connHandle) == ACL_ROLE_PERIPHERAL) {
            app_led_set_all_ctrl_pattern(LED_PATTERN_LED_FLASH_FAST, DEFAULT_PATTERN); //peripheral device
        } else {
            app_led_set_all_ctrl_pattern(LED_PATTERN_LED_FLASH_FAST, DEFAULT_PATTERN); //central device
        }
    #endif
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


    if (dev_char_get_conn_index_by_connhandle(pDisConn->connHandle) != 0xFF) { //ACL handle
        //terminate reason
        if (pDisConn->reason == HCI_ERR_CONN_TIMEOUT) {                 //connection timeout

        } else if (pDisConn->reason == HCI_ERR_REMOTE_USER_TERM_CONN) { //peer device send terminate command on link layer

        }
        //central host disconnect( blm_ll_disconnect(current_connHandle, HCI_ERR_REMOTE_USER_TERM_CONN) )
        else if (pDisConn->reason == HCI_ERR_CONN_TERM_BY_LOCAL_HOST) {
        } else {
        }
    #if (TLK_LED_ENABLE)
        if (dev_char_get_conn_role_by_connhandle(pDisConn->connHandle) == ACL_ROLE_PERIPHERAL) {
            app_led_set_all_ctrl_pattern(LED_PATTERN_LED_FLASH_SLOW, DEFAULT_PATTERN); //peripheral device
        } else {
            app_led_set_all_ctrl_pattern(LED_PATTERN_LED_FLASH_SLOW, DEFAULT_PATTERN); //central device
        }
    #endif
        dev_char_info_delete_by_connhandle(pDisConn->connHandle);
    } else {                                                                       //CIS Handle
    #if (TLK_LED_ENABLE)
        app_led_set_all_ctrl_pattern(LED_PATTERN_LED_FLASH_FAST, DEFAULT_PATTERN); // cis disconnect
    #endif
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
            }
            //------HCI LE event: LE CIS Established event-------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CIS_ESTABLISHED) {
                hci_le_cisEstablishedEvt_t *pCisEstbEvent = (hci_le_cisEstablishedEvt_t *)p;
                (void)pCisEstbEvent; //remove compiler warning
    #if (TLK_LED_ENABLE)
                app_led_set_all_ctrl_pattern(LED_PATTERN_LED_BREATH, DEFAULT_PATTERN);
    #endif
            }
            //------HCI LE event: LE CIS Request event -------------------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CIS_REQUEST) // connection update
            {
                hci_le_cisReqEvt_t *pCisReqEvt = (hci_le_cisReqEvt_t *)p;
                (void)pCisReqEvt;                               //remove compiler warning
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
        gap_smp_connEncDoneEvt_t *p = (gap_smp_connEncDoneEvt_t *)para;
        (void)p; //remove compiler warning
    } break;

    case GAP_EVT_SMP_SECURITY_PROCESS_DONE:
    {
        gap_smp_securityProcessDoneEvt_t *p = (gap_smp_securityProcessDoneEvt_t *)para;
        (void)p; //remove compiler warning

    } break;

    case GAP_EVT_ATT_EXCHANGE_MTU:
    {
        gap_gatt_mtuSizeExchangeEvt_t *p = (gap_gatt_mtuSizeExchangeEvt_t *)para;
        (void)p; //remove compiler warning

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
        #if (SIHUI_DEBUG_AUDIO)
    uint8_t  flag   = 0;
    uint16_t usb_id = 0;
    flash_read_page(0x1F1000, 1, (u8 *)&flag);

    usb_id = (0xff == flag) ? 0x120 : (0x0100 + flag);

    tlkapi_debug_customize_usb_id(usb_id);
        #else
    tlkapi_debug_customize_usb_id(0x121);
        #endif
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
    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] mac address", mac_public, 6);

    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

    blc_ll_initLegacyInitiating_module();

    blc_ll_initAclConnection_module();
    blc_ll_initAclPeriphrRole_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);

    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);

    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

    /* Default TX Power Initialization */
    rf_set_power_level_index(RF_POWER_P9dBm);

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

    blc_iso_enableSduToHostTimestamp(1);
    blc_ll_setCisSupplementPDUStrategy(CIS_PDU_STRATEGY0);
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
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_printf(APP_LOG_EN, "Controller Init ERROR:0x%x", error_code);
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
    blc_l2cap_initAclCentralBuffer(app_cen_l2cap_rx_buf, CENTRAL_L2CAP_BUFF_SIZE, NULL, 0);
    blc_l2cap_initAclPeripheralBuffer(app_per_l2cap_rx_buf, PERIPHR_L2CAP_BUFF_SIZE, app_per_l2cap_tx_buf, PERIPHR_L2CAP_BUFF_SIZE);

    blc_att_setCentralRxMtuSize(CENTRAL_ATT_RX_MTU);    ///must be placed after "blc_gap_init"
    blc_att_setPeripheralRxMtuSize(PERIPHR_ATT_RX_MTU); ///must be placed after "blc_gap_init"

    /* SMP Initialization */
    #if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)

    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
    #endif

    #if (ACL_PERIPHR_SMP_ENABLE)                                               //Slave SMP Enable
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption); //LE_Security_Mode_1_Level_2
        //blc_smp_configSecurityRequestSending(SecReq_PEND_SEND, SecReq_NOT_SEND, 100);
    #else
    blc_smp_setSecurityLevel(No_Security);
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
    /* Audio unicast server Initialization */
    app_audio_init();

    #if (TLK_KEY_ENABLE)
    app_key_init();
    #endif

    #if (TLK_LED_ENABLE)
    app_led_init();
    #endif

    #if EQ_AUDIO_EN
    #endif

    #if (HARDWARE_TYPE == TLSR9517CDK56D)
    gpio_write(GPIO_PD4, 1);
    #endif
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
_attribute_ble_data_retention_ u32 ledScanTick = 0;

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop(void)
{
    APP_DBG_CHN_0_HIGH;
    if (clock_time_exceed(ledScanTick, 1000 * 1000)) { //keyScan interval: 1000mS
        ledScanTick = clock_time();
        //      app_led_insert_all_ctrl_pattern(LED_PATTERN_LED_FAST_FLASH_1_TIME, DEFAULT_PATTERN);
    }

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    app_audio_handler();

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (TLK_KEY_ENABLE)
    tlk_key_scan();
    tlk_key_process();
    #endif

    #if (TLK_LED_ENABLE)
    app_led_task();
    #endif
    APP_DBG_CHN_0_LOW;
}


#endif
