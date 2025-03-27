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
#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_GOOGLE_BROADCAST_SOURCE)

#include "app.h"
#include "app_buffer.h"
#include "app_uart.h"
#include "app_config.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"

bool gAppAudioIsSend = false;
u8  app_bisBcstNum = 0;
u16 app_bisBcstHandle = 0;

static bisSourceAdvData_t tbl_advData = {
        .localNameLen = sizeof(DEFAULT_LOCAL_NAME),
        .localNameType = DT_COMPLETE_LOCAL_NAME,
        .localName = DEFAULT_LOCAL_NAME,
        .flagsLen = 0x02,
        .flagsType = DT_FLAGS,
        .flags = 0x05,
        .broadcastIDLen = 0x06,
        .broadcastIDType = DT_SERVICE_DATA,
        .BAAS_UUID = SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT,
        .broadcastID = {0x00, 0x00, 0x00},
        .PBAFeatureLen = 5,
        .PBAFeatureType = DT_SERVICE_DATA,
        .PBAS_UUID = SERVICE_UUID_PUBLIC_BROADCAST_ANNOUNCEMENT,
        .PBAFeature = BLC_AUDIO_PBA_FEATURE_HIGH_AUDIO,
        .metadataLen = DEFAULT_METADATA_LENGTH,
        .broadcastNameLen = sizeof(DEFAULT_BROADCAST_NAME),
        .broadcastNameType = DT_BROADCAST_NAME,
        .broadcastName = DEFAULT_BROADCAST_NAME,
};

static void blc_ll_updateBcstID(int advData_len, u8 *advData)
{
    //Generate random Broadcast_ID for Broadcast Audio Announcement, Ref: BAP_v1.0 3.7.2.1.1
    for(u8 i=0; i<advData_len;){
        u8 ad_length = advData[i];
        if(advData[i+1] == DT_SERVICE_DATA){
            if(*(u16*)&advData[i+2] == SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT) {//Broadcast Audio Announcement UUID
                //Broadcast_ID generator
                generateRandomNum(3, advData+i+4);
                break;
            }
        }
        i += ad_length+1;
    }
}

static void app_user_initExtAdv(void)
{
    u32  my_adv_interval_min = ADV_INTERVAL_100MS;
    u32  my_adv_interval_max = ADV_INTERVAL_100MS;
    // Extended, None_Connectable_None_Scannable undirected, with auxiliary packet
    blc_ll_setExtAdvParam( ADV_HANDLE0,         ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, my_adv_interval_min,    my_adv_interval_max,
                            BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC,                                             BLE_ADDR_PUBLIC,        NULL,
                           ADV_FP_NONE,         TX_POWER_3dBm,                                                  BLE_PHY_1M,             0,
                           BLE_PHY_2M,          PRIVATE_EXT_FILTER_SPECIFIC_SID,                                0);

    //Generate Broadcast_ID if there is Broadcast Audio Announcement in advData.
    blc_ll_updateBcstID(sizeof(tbl_advData), (u8 *)&tbl_advData);
    blc_ll_setExtAdvData(ADV_HANDLE0, sizeof(tbl_advData), (u8 *)&tbl_advData);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
}


const bisSourcPdaeAdvData_t pdaAdvData = {
    .length = sizeof(bisSourcPdaeAdvData_t) - 1,
    .type = DT_SERVICE_DATA,
    .BASS_UUID = SERVICE_UUID_BASIC_AUDIO_ANNOUNCEMENT,
    .presentationDelay = {U24_TO_BYTES(APP_PRESENTATION_DELAY)},
    .numSubgroups = 1,
    .groupInfo = {
        .numBis = 1,
        .codecId = {
            .id = 0xFF,
            .companyID = 0x0001,
            .vendorID = 0x0001,
        },
        .CodecSpecificConfigLen = sizeof(bisPdaCodecSpecificConfig_t),
        .metadataLen = sizeof(bisPdaMetadata_t),
        .addiInfo = {
            .bisIndex = 1,
            .size = 0,
        },
    },
};

static void app_user_initPeriodicAdv(void)
{
    blc_ll_initPeriodicAdvModule_initPeriodicdAdvSetParamBuffer(app_peridAdvSet_buffer, APP_PERID_ADV_SETS_NUMBER);
    blc_ll_initPeriodicAdvDataBuffer(app_peridAdvData_buffer, APP_PERID_ADV_DATA_LENGTH);
    u32  my_per_adv_itvl_min = PERADV_INTERVAL_100MS;
    u32  my_per_adv_itvl_max = PERADV_INTERVAL_100MS;
    blc_ll_setPeriodicAdvParam( ADV_HANDLE0, my_per_adv_itvl_min, my_per_adv_itvl_max, PERD_ADV_PROP_MASK_TX_POWER_INCLUDE);


    blc_ll_setPeriodicAdvData( ADV_HANDLE0, sizeof(bisSourcPdaeAdvData_t), (u8 *)&pdaAdvData);
    blc_ll_setPeriodicAdvEnable( BLC_ADV_ENABLE, ADV_HANDLE0);
}


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

        if(evtCode == HCI_EVT_LE_META)  //LE Event
        {
            u8 subEvt_code = p[0];

            //------hci le event: le create BIG complete event-------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CREATE_BIG_COMPLETE)  // create BIG complete
            {
                hci_le_createBigCompleteEvt_t* pEvt = (hci_le_createBigCompleteEvt_t*)p;

                if(pEvt->status == BLE_SUCCESS){
                    BLT_APP_LOG("Create BIG complete succeed");

                    app_bisBcstNum = pEvt->numBis;
                    BLT_APP_LOG("app_bisBcstNum:0x%x", app_bisBcstNum);

                    for(int i = 0; i < pEvt->numBis; i++){
                        app_bisBcstHandle = pEvt->bisHandles[i];
                        BLT_APP_LOG("app_bisBcstHandle:0x%x", app_bisBcstHandle);
                    }

                    gAppAudioIsSend = true;
                }
                else{
                    BLT_APP_LOG("Create BIG complete failed:0x%x", pEvt->status);
                }
            }
        }
    }
    return 0;
}


static void app_user_createBig(void)
{

    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CREATE_BIG_COMPLETE);


    blc_ll_initBigBcstModule_initBigBcstParametersBuffer(app_bigBcstParam, APP_BIG_BCST_NUMBER);

    blc_ll_InitBisParametersBuffer(app_bisToatlParam, APP_BIS_NUM_IN_ALL_BIG_BCST, APP_BIS_NUM_IN_ALL_BIG_SYNC);

    /* BIS TX buffer init */
    blc_ll_initBisTxFifo(app_bisBcstTxfifo, BIS_TX_PDU_FIFO_SIZE, BIS_TX_PDU_FIFO_NUM);

    /* IAL SDU buff init */
    blc_ll_initBisBcstSduInBuffer(app_bis_sdu_in_fifo, BIS_SDU_IN_FIFO_SIZE, BIS_SDU_IN_FIFO_NUM);
    blc_ll_closeBigCtrlPdu();

    hci_le_createBigParamsTest_t pBigCreateTstParam = {
        .big_handle = BIG_HANDLE_0,                             /* Used to identify the BIG */
        .adv_handle = ADV_HANDLE0,                          /* Used to identify the periodic advertising train */
        .num_bis = 1,                           /* Total number of BISes in the BIG */
        .sdu_intvl = {U24_TO_BYTES(APP_BIS_PARAM_SDU_INTERVAL)},        /* The interval, in microseconds, of periodic SDUs */
        .iso_intvl = APP_BIS_PARAM_ISO_INTERVAL/1250,                   /* The time between consecutive BIG anchor points */
        .nse = APP_BIS_PARAM_NSE,                                           /* The total number of subevents in each interval of each BIS in the BIG */
        .max_pdu = APP_BIS_PARAM_SDU_SIZE,          /* Maximum size of an SDU, in octets */
        .max_sdu = APP_BIS_PARAM_SDU_SIZE,          /* Maximum size, in octets, of payload */
        .phy = PHY_PREFER_2M,                               /* The transmitter PHY of packets */
        .packing = PACK_SEQUENTIAL,
        .framing = BIS_UNFRAMED,
        .bn = APP_BIS_PARAM_BN,                             /* The number of new payloads in each interval for each BIS */
        .irc = APP_BIS_PARAM_IRC,                           /* The number of times the scheduled payload(s) are transmitted in a given event*/
        .pto = APP_BIS_PARAM_PTO,                           /* Offset used for pre-transmissions */
        .enc = 0,                                           /* Encryption flag */
        /* TK: all zeros, just like JustWorks TODO: LE security mode 3, here use LE security mode 3 level2 */
        .broadcast_code = {0},                              /* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
    };
//  memset(pBigCreateTstParam.broadcast_code, 0x11, 16);

    ble_sts_t status = blc_hci_le_createBigParamsTest(&pBigCreateTstParam);


    BLT_APP_LOG("BIG create parameter status:0x%x", status);
}


void app_user_init(void)
{
    app_uart_init();
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


    //////////// BLE stack Initialization  Begin /////////////////////////

    u8  mac_public[6];
    u8  mac_random_static[6];
    
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


    //////////// LinkLayer Initialization  Begin /////////////////////////
    blc_ll_initBasicMCU();

    blc_ll_initStandby_module(mac_public);                         //mandatory


    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);

    rf_set_power_level_index(RF_POWER_P9dBm);

    //////////////////////////// User Configuration for BLE application ////////////////////////////

    app_user_initExtAdv();
    app_user_initPeriodicAdv();
    app_user_createBig();

    app_user_init();

    u8 error_code = blc_contr_checkControllerInitialization();
    if(error_code != INIT_SUCCESS){
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
        #if(UI_LED_ENABLE)
            gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
        #endif
        #if (TLKAPI_DEBUG_ENABLE)
            BLT_APP_LOG( "Controller Init ERROR:0x%x", error_code);
            while(1){
                tlkapi_debug_handler();
            }
        #else
            while(1);
        #endif
    }

    BLT_APP_LOG( "audio broadcast source init");
}



/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{

}

void app_user_send_handler(void);

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

    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

#if (UI_LED_ENABLE)
    static u32 tick=0;
    if(clock_time_exceed(tick, 500*1000))
    {
        tick = clock_time();
        gpio_toggle(GPIO_LED_GREEN);
    }
#endif
    app_uart_loop();
    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////
    app_user_send_handler();
}

#if APP_BIS_SEND_DATA_MODE == APP_BIS_SEND_DATA_FLOW_CTRL

bool app_user_check_send_flag(void)
{
    if(!blc_ll_getBisSduInBufferFreeNum(app_bisBcstHandle))
        return false;

    static u32 popDataTimer = 0;
    int dataLen = app_uart_getRecvDataLen();

    if(dataLen >= APP_BIS_PARAM_SDU_SIZE ||(dataLen && clock_time_exceed(popDataTimer, APP_BIS_PARAM_SDU_INTERVAL)))
    {
        popDataTimer = clock_time();
        return true;
    }
    return false;
}

void app_user_send_handler(void)
{

    if(!gAppAudioIsSend)
    {
        return ;
    }

    if(app_user_check_send_flag())
    {
        u8 coded_raw[APP_BIS_PARAM_SDU_SIZE];
        int size = app_uart_getRecvData(coded_raw, APP_BIS_PARAM_SDU_SIZE);

        blc_iso_sendData(app_bisBcstHandle, coded_raw, size);
    }
}

#else
void app_user_send_handler(void)
{
    static u32 popDataTimer = 0;
    if(!gAppAudioIsSend)
    {
        return ;
    }

    if(!clock_time_exceed(popDataTimer, APP_BIS_PARAM_SDU_INTERVAL))
    {
        return ;
    }

    if(popDataTimer == 0)
        popDataTimer = clock_time();
    else
        popDataTimer += APP_BIS_PARAM_SDU_INTERVAL*SYSTEM_TIMER_TICK_1US;

    u8 coded_raw[APP_BIS_PARAM_SDU_SIZE];
    int size = app_uart_getRecvData(coded_raw, APP_BIS_PARAM_SDU_SIZE);

    blc_iso_sendData(app_bisBcstHandle, coded_raw, size);
}

#endif

#endif

