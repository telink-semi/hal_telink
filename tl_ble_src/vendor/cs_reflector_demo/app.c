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
#include "app_cs.h"
#if (FREERTOS_ENABLE)
#include "app_port_freertos.h"
#endif

_attribute_ble_data_retention_ u8 ota_is_working = 0;


/**
 * @brief   BLE Advertising data
 */
const u8 tbl_advData[] = {
    10,       DT_COMPLETE_LOCAL_NAME,
    'c',      's',
    '_',      't',
    'e',      'l',
    'i',      'n',
    'k',      2,
    DT_FLAGS, 0x05, // BLE limited discoverable mode and BR/EDR not supported
    3,        DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,
    0x5B,     0x18, // incomplete list of 0x185B(Ranging))
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8 tbl_scanRsp[] = {
    10, DT_COMPLETE_LOCAL_NAME, 'c', 's', '_', 't', 'e', 'l', 'i', 'n', 'k',
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

    blc_cs_resetByHandle(pDisConn->connHandle);

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
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE) {
                app_le_cs_read_remote_support_capabilities_complete_event_handle(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_READ_REMOTE_FAE_TABLE_COMPLETE) {
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_CONFIG_COMPLETE) {
                app_le_cs_config_complete_event_handle(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_SECURITY_ENABLE_COMPLETE) {
            } else if (subEvt_code == HCI_SUB_EVT_LE_CS_PROCEDURE_ENABLE_COMPLETE) {
                app_le_cs_procedure_enable_complete_event_handle(p);
            }

            //------hci le event: LE CS Subevent Result event-------------------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT) {
#if (APP_CS_SUBEVENT_LOG_EN)
                hci_le_csSubeventResultEvt_t *pCsSubevent = (hci_le_csSubeventResultEvt_t *)p;
                /* print subevent data to txt file to calculate distance with other company */
                u8 tempBuff[258];
                tempBuff[0] = 0x04;
                tempBuff[1] = 0x3E; //event_code
                tempBuff[2] = n;    // total_len
                smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);
                tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "Procedure count:[%d]\r\n", pCsSubevent->Procedure_Counter);
                tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "subevent len:[%d]bytes\r\n", n);
                debugwait();
                tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs subevent", tempBuff, n + 3);
                debugwait();
#endif

                app_le_cs_subevent_result_event_handle(p);
#if FREERTOS_ENABLE
                os_give_sem();
#endif
            }
            //------hci le event: LE CS Subevent Result Continue event----------
            else if (subEvt_code == HCI_SUB_EVT_LE_CS_SUBEVENT_RESULT_CONTINUE) {
#if (APP_CS_SUBEVENT_LOG_EN)
                hci_le_csSubeventResultContinueEvt_t *pCsSubevent = (hci_le_csSubeventResultContinueEvt_t *)p;
                /* print subevent continue data to txt file to calculate distance with other company */
                u8 tempBuff[258];
                tempBuff[0] = 0x04;
                tempBuff[1] = 0x3E; //event_code
                tempBuff[2] = n;    // total_len
                smemcpy(tempBuff + 3, &pCsSubevent->Subevent_Code, n);
                tlkapi_printf(APP_CS_SUBEVENT_LOG_EN, "continue subevent len:[%d]bytes\r\n", n);
                tlkapi_send_string_data(APP_CS_SUBEVENT_LOG_EN, "cs continue subevent", tempBuff, n + 3);
#endif
                app_le_cs_subevent_result_continue_event_handle(p);
#if FREERTOS_ENABLE
                os_give_sem();
#endif
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
#if (UI_LED_ENABLE) //led indicate
        for (int k = 0; k < 3; k++) {
            gpio_write(GPIO_LED_BLUE, LED_ON_LEVEL);
            sleep_us(200000);
            gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
            sleep_us(200000);
        }
#endif
        analog_write_reg8(USED_DEEP_ANA_REG, analog_read_reg8(USED_DEEP_ANA_REG) | LOW_BATT_FLG); //mark
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
    if (flash_op_evt == FLASH_OP_EVT_APP_INITIALIZATION) {
        /* ignore "op addr_begin" and "op addr_end" for initialization event
         * must call "flash protection_init" first, will choose correct flash protection relative API according to current internal flash type in MCU */
        flash_protection_init();

        /* just sample code here, protect all flash area for old firmware and OTA new firmware.
         * user can change this design if have other consideration */
        u32 app_lockBlock = FLASH_LOCK_FW_NONE;
#if (BLE_OTA_SERVER_ENABLE)
        u32 multiBootAddress = blc_ota_getCurrentUsedMultipleBootAddress();
        if (multiBootAddress == MULTI_BOOT_ADDR_0x20000) {
            app_lockBlock = FLASH_LOCK_FW_LOW_256K;
        } else if (multiBootAddress == MULTI_BOOT_ADDR_0x40000) {
            app_lockBlock = FLASH_LOCK_FW_LOW_512K;
        } else if (multiBootAddress == MULTI_BOOT_ADDR_0x80000) {
            /* attention that 1M capacity flash can not lock all 1M area, should leave some upper sector for system data and user data
                 * will use a approximate value */
            app_lockBlock = FLASH_LOCK_FW_LOW_1M;
        }
#else
        app_lockBlock = FLASH_LOCK_FW_LOW_256K; //just demo value, user can change this value according to application
#endif


        flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);

        tlkapi_send_string_data(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] initialization, lock flash", 0, 0);
        flash_lock(flash_lockBlock_cmd);
    }
#if (BLE_OTA_SERVER_ENABLE)
    else if (flash_op_evt == FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_BEGIN) {
        /* OTA clear old firmware begin event is triggered by stack, in "blc ota_initOtaServer_module", rebooting from a successful OTA.
         * Software will erase whole old firmware for potential next new OTA, need unlock flash if any part of flash address from
         * "op addr_begin" to "op addr_end" is in locking block area.
         * In this sample code, we protect whole flash area for old and new firmware, so here we do not need judge "op addr_begin" and "op addr_end",
         * must unlock flash */
        tlkapi_send_string_u32s(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA clear old FW begin, un_lock flash", op_addr_begin, op_addr_end, 0, 0);
        flash_unlock();
    } else if (flash_op_evt == FLASH_OP_EVT_STACK_OTA_CLEAR_OLD_FW_END) {
        /* ignore "op addr_begin" and "op addr_end" for END event
         * OTA clear old firmware end event is triggered by stack, in "blc ota_initOtaServer_module", erasing old firmware data finished.
         * In this sample code, we need lock flash again, because we have unlocked it at the begin event of clear old firmware */
        tlkapi_send_string_data(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA clear old FW end, restore flash locking ", 0, 0);
        flash_lock(flash_lockBlock_cmd);
    } else if (flash_op_evt == FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_BEGIN) {
        /* OTA write new firmware begin event is triggered by stack, when receive first OTA data PDU.
         * Software will write data to flash on new firmware area,  need unlock flash if any part of flash address from
         * "op addr_begin" to "op addr_end" is in locking block area.
         * In this sample code, we protect whole flash area for old and new firmware, so here we do not need judge "op addr_begin" and "op addr_end",
         * must unlock flash */
        tlkapi_send_string_u32s(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA write new FW begin, un_lock flash", op_addr_begin, op_addr_end, 0, 0);
        flash_unlock();
    } else if (flash_op_evt == FLASH_OP_EVT_STACK_OTA_WRITE_NEW_FW_END) {
        /* ignore "op addr_begin" and "op addr_end" for END event
         * OTA write new firmware end event is triggered by stack, after OTA end or an OTA error happens, writing new firmware data finished.
         * In this sample code, we need lock flash again, because we have unlocked it at the begin event of write new firmware */
        tlkapi_send_string_data(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] OTA write new FW end, restore flash locking ", 0, 0);
        flash_lock(flash_lockBlock_cmd);
    }
#endif
    /* add more flash protection operation for your application if needed */
}


#endif

_attribute_ram_code_ void app_process_power_management(u8 e, u8 *p, int n)
{
    (void)e;
    (void)n;
    (void)p;
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
        user_task_flg |= user_task_flg || scan_pin_need;
#endif

        if (user_task_flg) {
            bls_pm_setManualLatency(0);
        }
    }
#endif
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
    blc_debug_enableStackLog(STK_LOG_HCI_CS | STK_LOG_SMP_LTK);
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

#if (RAS_PERSISTENT_FILTER)
    blc_prf_initPairingInfoStoreModule();
#endif

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
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CONNECTION_COMPLETE | HCI_LE_EVT_MASK_ADVERTISING_REPORT | HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE |
                                HCI_LE_EVT_MASK_2_CS_READ_REMOTE_SUPPORTED_CAPABILITIES_COMPLETE | HCI_LE_EVT_MASK_2_CS_READ_REMOTE_FAE_TABLE_COMPLETE |
                                HCI_LE_EVT_MASK_2_CS_SECURITY_ENABLE_COMPLETE | HCI_LE_EVT_MASK_2_CS_CONFIG_COMPLETE | HCI_LE_EVT_MASK_2_CS_PROCEDURE_ENABLE_COMPLETE);
    blc_hci_le_setEventMask_2_cmd(HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT | HCI_LE_EVT_MASK_2_CS_SUBEVENT_RESULT_CONTINUE);

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
#if (!FREERTOS_ENABLE)
    blc_ll_setAdvData(tbl_advData, sizeof(tbl_advData));
    blc_ll_setScanRspData(tbl_scanRsp, sizeof(tbl_scanRsp));
    blc_ll_setAdvParam(ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE); //ADV enable
    //blc_ll_setMaxAdvDelay_for_AdvEvent(MAX_DELAY_0MS);
#endif
    rf_set_power_level_index(RF_POWER_P9dBm);

#if (BLE_APP_PM_ENABLE)
    blc_ll_initPowerManagement_module();
    blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_ACL_PERIPHR);
    blc_pm_setDeepsleepRetentionEnable(PM_DeepRetn_Disable);

    blc_ll_registerTelinkControllerEventCallback(BLT_EV_FLAG_SUSPEND_EXIT, &user_set_flag_suspend_exit);

#endif

    app_channel_sounding_init();

    tlkapi_printf(APP_LOG_EN, "[APP][INI] Reflector demo init finished");
}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_ void user_init_deepRetn(void)
{
    /*
     * flash 4 line enable need to be called after deep_retn, now CS only suspend mode
     * here call this api for feature deep retn mode use, deepRetn early wake up time need to be calculated again.
     */
#if (0)
#if (FLASH_4LINE_MODE_ENABLE)
    /* enable flash mode (cmd:1x, addr:4x, data:4x, dummy:6), need parameter flash mid, which is got in blc_readFlashSize_autoConfigCustomFlashSector, and flash mid is retn param */
    ble_flash_4line_enable();
#endif
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

#if (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
#endif

    app_process_power_management(0, 0, 0);
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
