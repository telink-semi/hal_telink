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
#include "app_ui.h"
#include "app_sub_node.h"

#if (MONITOR_ROLE_SELECT == MONITOR_CENTRAL_PERIPHERAL)

#define MY_RF_POWER_INDEX                   RF_POWER_INDEX_P9p90dBm

#define MY_ADV_INTERVAL_MIN                 ADV_INTERVAL_100MS
#define MY_ADV_INTERVAL_MAX                 ADV_INTERVAL_100MS


/**
 * @brief   BLE Advertising data
 */
const u8    tbl_advData[] = {
    12, DT_COMPLETE_LOCAL_NAME,                 's','n','i','f','f','e', 'r', '_', 'S', 'C', 'P',
    2,  DT_FLAGS,                               0x05,                   // BLE limited discoverable mode and BR/EDR not supported
    3,  DT_APPEARANCE,                          0x80, 0x01,             // 384, Generic Remote Control, Generic category
    5,  DT_INCOMPLETE_LIST_16BIT_SERVICE_UUID,  0x12, 0x18, 0x0F, 0x18, // incomplete list of service class UUIDs (0x1812, 0x180F)
};

/**
 * @brief   BLE Scan Response Packet data
 */
const u8    tbl_scanRsp [] = {
    12,  DT_COMPLETE_LOCAL_NAME,                's','n','i','f','f','e', 'r', '_', 'S', 'C', 'P',
};





/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int AA_dbg_adv_rpt = 0;
u32 tick_adv_rpt = 0;

int app_le_adv_report_event_handle(u8 *p)
{
#if (APP_LE_LEGACY_SCAN_EN)  //debug, print ADV report every 500ms
    event_adv_report_t *pa = (event_adv_report_t *)p;
    s8 rssi = pa->data[pa->len];
    if(clock_time_exceed(tick_adv_rpt, 500000)){
        tick_adv_rpt = clock_time();

        tlkapi_printf(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] Adv RSSI:%d\n", rssi);
        tlkapi_send_string_data(APP_CONTR_EVENT_LOG_EN, "[APP][EVT] Adv report", &pa->mac[0], pa->len + 7);
    }
#endif

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

        if(evtCode == HCI_EVT_LE_META)  //LE Event
        {
            u8 subEvt_code = p[0];

            //--------hci le event: le adv report event ----------------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_ADVERTISING_REPORT)   // ADV packet
            {
                //after controller is set to scan state, it will report all the adv packet it received by this event

                app_le_adv_report_event_handle(p);
            }
        }
    }

    return 0;
}


#if (BATT_CHECK_ENABLE)  //battery check must do before OTA relative operation

_attribute_data_retention_  u32 lowBattDet_tick   = 0;

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
    u8 battery_check_returnValue=0;
    if(analog_read(USED_DEEP_ANA_REG) & LOW_BATT_FLG){
        battery_check_returnValue=app_battery_power_check(alarm_vol_mv+200);
    }
    else{
        battery_check_returnValue=app_battery_power_check(alarm_vol_mv);
    }
    if(battery_check_returnValue)
    {
        analog_write_reg8(USED_DEEP_ANA_REG,  analog_read_reg8(USED_DEEP_ANA_REG) &(~LOW_BATT_FLG));  //clr
    }
    else
    {
        #if (UI_LED_ENABLE)  //led indicate
            for(int k=0;k<3;k++){
                gpio_write(GPIO_LED_BLUE, LED_ON_LEVEL);
                sleep_us(200000);
                gpio_write(GPIO_LED_BLUE, !LED_ON_LEVEL);
                sleep_us(200000);
            }
        #endif
        analog_write_reg8(USED_DEEP_ANA_REG,  analog_read_reg8(USED_DEEP_ANA_REG) | LOW_BATT_FLG);  //mark

        #if (UI_KEYBOARD_ENABLE)
        u32 pin[] = KB_DRIVE_PINS;
        for (u32 i=0; i<(sizeof (pin)/sizeof(*pin)); i++)
        {
            cpu_set_gpio_wakeup (pin[i], 1, 1);  //drive pin pad high wakeup deepsleep
        }

        cpu_sleep_wakeup(DEEPSLEEP_MODE, PM_WAKEUP_PAD, 0);  //deepsleep
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
_attribute_data_retention_ u32  app_lockBlock = FLASH_LOCK_ALL_AREA;
_attribute_data_retention_ u16  flash_lockBlock_cmd;
void app_flash_protection_operation(u8 flash_op_evt, u32 op_addr_begin, u32 op_addr_end)
{
    (void)op_addr_begin;
    (void)op_addr_end;
    if(flash_op_evt == FLASH_OP_EVT_APP_INITIALIZATION)
    {
        /* ignore "op addr_begin" and "op addr_end" for initialization event
         * must call "flash protection_init" first, will choose correct flash protection relative API according to current internal flash type in MCU */
        flash_protection_init();

        /* just sample code here, protect all flash area for old firmware and OTA new firmware.
         * user can change this design if have other consideration */
        app_lockBlock = FLASH_LOCK_ALL_AREA; //lock all Flash

        flash_lockBlock_cmd = flash_change_app_lock_block_to_flash_lock_block(app_lockBlock);

        if(blc_flashProt.init_err){
            tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] flash protection initialization error!!!\n");
        }

        tlkapi_printf(APP_FLASH_PROT_LOG_EN, "[FLASH][PROT] initialization, lock flash\n");
        flash_lock(flash_lockBlock_cmd);
    }
    /* add more flash protection operation for your application if needed */
}

#endif


#if (BLE_APP_PM_ENABLE)
/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SLEEP_ENTER"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_sleep_enter(u8 e, u8 *p, int n)
{
    (void)e;
    (void)p;
    (void)n;

    #if (UI_KEYBOARD_ENABLE)
        app_set_kb_wakeup (e, p, n);
    #endif
}

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_SUSPEND_EXIT"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_suspend_exit(u8 e, u8 *p, int n)
{
    (void)e;
    (void)p;
    (void)n;
}

/**
 * @brief      callBack function of LinkLayer Event "BLT_EV_FLAG_GPIO_EARLY_WAKEUP"
 * @param[in]  e - LinkLayer Event type
 * @param[in]  p - data pointer of event
 * @param[in]  n - data length of event
 * @return     none
 */
_attribute_ram_code_ void user_gpio_early_wakeup(u8 e, u8 *p, int n)
{
    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard(e, p, n);
    #endif
}

#endif


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

    /* attention that this function must be called after "blc readFlashSize_autoConfigCustomFlashSector" !!!*/
    blc_app_loadCustomizedParameters_normal();

    #if (APP_FLASH_PROTECTION_ENABLE)
        app_flash_protection_operation(FLASH_OP_EVT_APP_INITIALIZATION, 0, 0);
        blc_appRegisterStackFlashOperationCallback(app_flash_protection_operation); //register flash operation callback for stack
    #endif

//////////////////////////// basic hardware Initialization  End /////////////////////////////////


//////////////////////////// BLE stack Initialization  Begin //////////////////////////////////

    u8  mac_public[6];
    u8  mac_random_static[6];
    /* Note: If change IC type, need to confirm the FLASH_SIZE_CONFIG */
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);

#if (APP_LE_LEGACY_ADV_EN)
    blc_ll_initLegacyAdvertising_module();  //adv module:        mandatory for BLE slave,
#endif

#if (APP_LE_LEGACY_SCAN_EN)
    blc_ll_initLegacyScanning_module();     //scan module:       mandatory for BLE master
    blc_ll_configScanEnableStrategy(SCAN_STRATEGY_1);
#endif

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
    /* Attention: sniffer support PHY switch (1M ,2M, coded) */
    #if (APP_PHY_SWITCHING_ENABLE)
        blc_ll_init2MPhyCodedPhy_feature();
    #endif
    //////////// LinkLayer Initialization  End /////////////////////////



    //////////// HCI Initialization  Begin /////////////////////////
    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth event
    blc_hci_setEventMask_cmd (HCI_EVT_MASK_NONE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_ADVERTISING_REPORT);
    //////////// HCI Initialization  End /////////////////////////

    /* Check if any Stack(Controller & Host) Initialization error after all BLE initialization done.
     * attention: user can not delete !!! */
    u32 error_code1 = blc_contr_checkControllerInitialization();
    u32 error_code2 = blc_host_checkHostInitialization();
    if(error_code1 != INIT_SUCCESS || error_code2 != INIT_SUCCESS){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        #if (UI_LED_ENABLE)
            gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif

        #if (TLKAPI_DEBUG_ENABLE)
            tlkapi_printf(APP_LOG_EN, "[APP][INI] Stack INIT ERROR 0x%04x, 0x%04x", error_code1, error_code2);
            while(1){
                tlkapi_debug_handler();
            }
        #else
            while(1);
        #endif
    }
    //////////////////////////// BLE stack Initialization  End /////////////////




    //////////////////////////// User Configuration for BLE application ////////
#if (APP_LE_LEGACY_ADV_EN)
    blc_ll_setAdvData( (u8 *)tbl_advData, sizeof(tbl_advData) );
    blc_ll_setScanRspData( (u8 *)tbl_scanRsp, sizeof(tbl_scanRsp));
    blc_ll_setAdvParam(MY_ADV_INTERVAL_MIN, MY_ADV_INTERVAL_MAX, ADV_TYPE_CONNECTABLE_UNDIRECTED, OWN_ADDRESS_PUBLIC, 0, NULL, BLT_ENABLE_ADV_ALL, ADV_FP_NONE);
    blc_ll_setAdvEnable(BLC_ADV_ENABLE);  //ADV enable
    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] Legacy Advertising Enable", 0, 0);
#endif

#if (APP_LE_LEGACY_SCAN_EN)
    blc_ll_setScanParameter(SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_WINDOW_50MS, OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY);
    blc_ll_setScanEnable (BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] Legacy Scanning Enable", 0, 0);
#endif

    rf_set_power_level_index(MY_RF_POWER_INDEX);

#if (UI_KEYBOARD_ENABLE)
    keyboard_init();
#endif

#if (BLE_APP_PM_ENABLE)
    blc_ll_initPowerManagement_module();
    blc_pm_setSleepMask(PM_SLEEP_DISABLE);

    blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_SLEEP_ENTER, &user_sleep_enter);
    blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_SUSPEND_EXIT, &user_suspend_exit);
    blc_ll_registerTelinkControllerEventCallback (BLT_EV_FLAG_GPIO_EARLY_WAKEUP, &user_gpio_early_wakeup);
#endif

    tlkapi_send_string_data(APP_LOG_EN, "[APP][INI] user_init_normal", 0, 0);
    ////////////////////////////////////////////////////////////////////////////
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

        blc_pm_setSleepMask(PM_SLEEP_DISABLE);

        #if (UI_KEYBOARD_ENABLE)
            user_task_flg = user_task_flg || scan_pin_need || key_not_released;
        #endif

        if (user_task_flg){
            blc_pm_setSleepMask(PM_SLEEP_DISABLE);
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
int main_idle_loop (void)
{

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    ////////////////////////////////////// main_node entry ///////////////////////////
    snif_sub_node_control_process();

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    #if (BATT_CHECK_ENABLE)
        /*The frequency of low battery detect is controlled by the variable lowBattDet_tick, which is executed every
         500ms in the demo. Users can modify this time according to their needs.*/
        if(battery_get_detect_enable() && clock_time_exceed(lowBattDet_tick, 500000) ){
            lowBattDet_tick = clock_time();
            user_battery_power_check(BAT_DEEP_THRESHOLD_MV);
        }
    #endif

    #if (UI_KEYBOARD_ENABLE)
        proc_keyboard (0, 0, 0);
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
_attribute_no_inline_ void main_loop (void)
{
    main_idle_loop ();
}
#endif



