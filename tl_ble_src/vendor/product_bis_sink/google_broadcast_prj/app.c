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
#include "../bis_sink_config.h"
#if (PRODUCT_BIS_SINK_SELECT == PRODUCT_GOOGLE_BROADCAST_SINK)

#include "app_receiver.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

#include "app.h"
#include "app_buffer.h"
#include "app_config.h"

/**
 * @brief      BLE host event handler call-back.
 * @param[in]  h       event type
 * @param[in]  para    Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_host_event_callback (u32 h, u8 *para, int n)
{
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
    blc_ll_initBasicMCU();                  //mandatory
    blc_ll_initStandby_module(mac_public);  //mandatory


    blc_ll_initAclConnection_module();
    blc_ll_initAclPeriphrRole_module();

    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);

    //Extend Scan initialization
    blc_ll_initExtendedScanning_module();
    blc_ll_configScanEnableStrategy(SCAN_STRATEGY_1);

    //PDA Sync initialization
    blc_ll_initPeriodicAdvertisingSynchronization_module();

    blc_ll_setMaxConnectionNumber(ACL_CENTRAL_MAX_NUM, ACL_PERIPHR_MAX_NUM);
    blc_ll_setAclConnMaxOctetsNumber(ACL_CONN_MAX_RX_OCTETS, ACL_CENTRAL_MAX_TX_OCTETS, ACL_PERIPHR_MAX_TX_OCTETS);

    /* all ACL connection share same RX FIFO */
    blc_ll_initAclConnRxFifo(app_acl_rx_fifo, ACL_RX_FIFO_SIZE, ACL_RX_FIFO_NUM);
    /* ACL Peripheral TX FIFO */
    blc_ll_initAclPeriphrTxFifo(app_acl_per_tx_fifo, ACL_PERIPHR_TX_FIFO_SIZE, ACL_PERIPHR_TX_FIFO_NUM, ACL_PERIPHR_MAX_NUM);

    rf_set_power_level_index(RF_POWER_P9dBm);
    //////////// LinkLayer Initialization  End /////////////////////////


    /////////////////// BIS SYNC initialization ////////////////////////
    blc_ll_initBigSyncModule_initBigSyncParametersBuffer(app_bigSyncParam, APP_BIG_SYNC_NUMBER);

    blc_ll_InitBisParametersBuffer(app_bisToatlParam, APP_BIS_NUM_IN_ALL_BIG_BCST, APP_BIS_NUM_IN_ALL_BIG_SYNC);

    /* BIS RX buffer init */
    blc_ll_initBisRxFifo(app_bisSyncRxfifo, BIS_RX_PDU_FIFO_SIZE, BIS_RX_PDU_FIFO_NUM, APP_BIS_NUM_IN_ALL_BIG_SYNC);//APP_BIG_SYNC_NUMBER

    /* IAL SDU buff init */
    blc_ll_initBisSyncSduOutBuffer(app_bis_sdu_out_fifo, BIS_SDU_OUT_FIFO_SIZE, BIS_SDU_OUT_FIFO_NUM);
    //////////////// BIS SYNC Initialization End ///////////////////////


    //////////// HCI Initialization  Begin /////////////////////////

    blc_hci_setEventMask_cmd(HCI_EVT_MASK_DISCONNECTION_COMPLETE);

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(  HCI_LE_EVT_MASK_EXTENDED_ADVERTISING_REPORT
                                | HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_ESTABLISHED
                                | HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_REPORT
                                | HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_LOST
                                | HCI_LE_EVT_MASK_BIG_SYNC_ESTABLISHED
                                | HCI_LE_EVT_MASK_BIG_SYNC_LOST
                                | HCI_LE_EVT_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECEIVED
                                | HCI_LE_EVT_MASK_ENHANCED_CONNECTION_COMPLETE
                                | HCI_LE_EVT_MASK_CONNECTION_COMPLETE
                                | HCI_LE_EVT_MASK_TERMINATE_BIG_COMPLETE);

    blc_hci_le_setEventMask_2_cmd(  HCI_LE_EVT_MASK_2_BIGINFO_ADVERTISING_REPORT );
    //////////// HCI Initialization  End /////////////////////////

    u8 error_code = blc_contr_checkControllerInitialization();
    if(error_code != INIT_SUCCESS){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
        #if(UI_LED_ENABLE)
            gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif
        #if (TLKAPI_DEBUG_ENABLE)
            BLT_APP_LOG("Controller Init ERROR:0x%x", error_code);
            while(1){
                tlkapi_debug_handler();
            }
        #else
            while(1);
        #endif
    }

    //////////// Host Initialization  Begin /////////////////////////
    /* Host Initialization */
    /* GAP initialization must be done before any other host feature initialization !!! */
    blc_gap_init();

    /* SMP Initialization */
#if (ACL_PERIPHR_SMP_ENABLE || ACL_CENTRAL_SMP_ENABLE)
    
    blc_smp_configPairingSecurityInfoStorageAddressAndSize(flash_sector_smp_storage, FLASH_SMP_PAIRING_MAX_SIZE);
#endif

#if (ACL_PERIPHR_SMP_ENABLE)  //Peripheral SMP Enable
    blc_smp_setSecurityLevel_periphr(Unauthenticated_Pairing_with_Encryption);
    blc_smp_smpParamInit();
#else
    blc_smp_setSecurityLevel(No_Security);
#endif

    //host(GAP/SMP/GATT/ATT) event process: register host event callback and set event mask
    blc_gap_registerHostEventHandler( app_host_event_callback );
    blc_gap_setEventMask( GAP_EVT_MASK_SMP_PAIRING_BEGIN            |
                          GAP_EVT_MASK_SMP_PAIRING_SUCCESS          |
                          GAP_EVT_MASK_SMP_PAIRING_FAIL             |
                          GAP_EVT_MASK_SMP_SECURITY_PROCESS_DONE    |
                          GAP_EVT_MASK_SMP_TK_NUMERIC_COMPARE);
    //////////// Host Initialization  End /////////////////////////

    app_bis_receiver_init();

    BLT_APP_LOG("user_init end");

#if RUNNING_BOARD == TLSR9517CDK56D
    gpio_write(TL_SHDN_GPIO, 1);
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

/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
_attribute_no_inline_ void main_loop (void)
{
    static u32 tick = 1;
    if(clock_time_exceed(tick, 500*1000)){
        tick = clock_time();
        gpio_toggle(GPIO_LED_WHITE);
    }

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

    app_bis_receiver_handler();
}

#endif

