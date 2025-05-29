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
#include "app.h"
#include "app_audio.h"
#include "app_buffer.h"
#include "app_config.h"
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app_ui.h"
#include "stack/ble/profile/audio/stream/bap.h"


#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)

static bisSourceAdvData_t tbl_advData = {
    .localNameLen      = sizeof(DEFAULT_LOCAL_NAME),
    .localNameType     = DT_COMPLETE_LOCAL_NAME,
    .localName         = DEFAULT_LOCAL_NAME,
    .flagsLen          = 0x02,
    .flagsType         = DT_FLAGS,
    .flags             = 0x05,
    .broadcastIDLen    = 0x06,
    .broadcastIDType   = DT_SERVICE_DATA,
    .BAAS_UUID         = SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT,
    .broadcastID       = {0x00, 0x00, 0x00},
    .broadcastNameLen  = sizeof(DEFAULT_BROADCAST_NAME),
    .broadcastNameType = DT_BROADCAST_NAME,
    .broadcastName     = DEFAULT_BROADCAST_NAME,
    .PBAFeatureLen     = 5,
    .PBAFeatureType    = DT_SERVICE_DATA,
    .PBAS_UUID         = SERVICE_UUID_PUBLIC_BROADCAST_ANNOUNCEMENT,
    .PBAFeature        = DEFAULT_PBA_FEATURE,
    .metadataLen       = DEFAULT_METADATA_LENGTH,
};


static bisSourcPdaeAdvData_t pdaAdvData;

static void app_modify_pdaAdvData(void)
{
    gbisSamplingFreq  = codecSettings[BIS_COMMON_CODEC_CONFIG].frequencyValue;
    gbisFrameDuration = codecSettings[BIS_COMMON_CODEC_CONFIG].durationValue;
    gbisOctetPer      = codecSettings[BIS_COMMON_CODEC_CONFIG].frameOctets;

    bisSourcPdaeAdvData_t tpdaAdvData = {
        .length                              = sizeof(bisSourcPdaeAdvData_t) - 1,
        .type                                = DT_SERVICE_DATA,
        .BASS_UUID                           = SERVICE_UUID_BASIC_AUDIO_ANNOUNCEMENT,
        .presentationDelay                   = {U24_TO_BYTES(APP_PRESENTATION_DELAY)},
        .numSubgroups                        = DEFALUT_NUM_SUBGROUP,
        .groupInfo[0].numBis                 = SUBGROUP_NUM_BIS,
        .groupInfo[0].codecFormat            = BLC_AUDIO_CODING_FORMAT_LC3,
        .groupInfo[0].companyID              = 0x0000,
        .groupInfo[0].vendorSpecificCodecID  = 0x000,
        .groupInfo[0].CodecSpecificConfigLen = sizeof(bisPdaCodecSpecificConfig_t),

        .groupInfo[0].CodecSpecificConfig.samplingFreqLen    = 0x02,
        .groupInfo[0].CodecSpecificConfig.samplingFreqType   = BLC_AUDIO_CAPTYPE_CFG_SAMPLE_FREQUENCY,
        .groupInfo[0].CodecSpecificConfig.samplingFreq       = gbisSamplingFreq,
        .groupInfo[0].CodecSpecificConfig.frameDurationLen   = 0x02,
        .groupInfo[0].CodecSpecificConfig.frameDurationType  = BLC_AUDIO_CAPTYPE_CFG_FRAME_DURATION,
        .groupInfo[0].CodecSpecificConfig.frameDuration      = gbisFrameDuration,
        .groupInfo[0].CodecSpecificConfig.octetsPerFrameLen  = 0x03,
        .groupInfo[0].CodecSpecificConfig.octetsPerFrameType = BLC_AUDIO_CAPTYPE_CFG_OCTETS_PER_CODEC_FRAME,
        .groupInfo[0].CodecSpecificConfig.octetsPerFrame     = gbisOctetPer,
        .groupInfo[0].metadataLen                            = sizeof(bisPdaMetadata_t),
        .groupInfo[0].metadata.streamingAudioContextsLen     = 0x03,
        .groupInfo[0].metadata.streamingAudioContextsType    = BLC_AUDIO_METATYPE_STREAMING_CONTEXTS,
        .groupInfo[0].metadata.streamingAudioContexts        = BIS_CONTEXTS_TYPE,
        .groupInfo[0].metadata.languageLen                   = 0x04,
        .groupInfo[0].metadata.languageType                  = BLC_AUDIO_METATYPE_LANGUAGE,
        .groupInfo[0].metadata.language                      = {U24_TO_BYTES(BIS_LANGUAGE)},

        .groupInfo[0].addiInfo[0].bisIndex                   = 0x01,
        .groupInfo[0].addiInfo[0].size                       = 0x06,
        .groupInfo[0].addiInfo[0].audioChannelAllocationLen  = 0x05,
        .groupInfo[0].addiInfo[0].audioChannelAllocationType = BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION,
        .groupInfo[0].addiInfo[0].audioChannelAllocation     = BIS_1_AUDIO_LOCATION,

    #if SUBGROUP_NUM_BIS > 1
        .groupInfo[0].addiInfo[1].bisIndex                   = 0x02,
        .groupInfo[0].addiInfo[1].size                       = 0x06,
        .groupInfo[0].addiInfo[1].audioChannelAllocationLen  = 0x05,
        .groupInfo[0].addiInfo[1].audioChannelAllocationType = BLC_AUDIO_CAPTYPE_CFG_CHANNELS_ALLOCATION,
        .groupInfo[0].addiInfo[1].audioChannelAllocation     = BIS_2_AUDIO_LOCATION,
    #endif
    };

    memcpy(&pdaAdvData, &tpdaAdvData, sizeof(bisSourcPdaeAdvData_t));
}

//////////////////////////////////////////////////////////

hci_le_createBigParamsTest_t gBigCreateTstParam;

static void app_set_createBigParamsTest(void)
{
    u32 bisInterval = gbisFrameDuration ? 10000 : 7500;

    hci_le_createBigParamsTest_t tBigCreateTstParam = {
        .big_handle = BIG_HANDLE_0,                /* Used to identify the BIG */
        .adv_handle = ADV_HANDLE0,                 /* Used to identify the periodic advertising train */
        .num_bis    = min(0x1F, SUBGROUP_NUM_BIS), /* Total number of BISes in the BIG */
        .sdu_intvl  = {U24_TO_BYTES(bisInterval)}, /* The interval, in microseconds, of periodic SDUs */
        .iso_intvl  = bisInterval / 1250,          /* The time between consecutive BIG anchor points */
        .nse        = 8 / SUBGROUP_NUM_BIS,        /* The total number of subevents in each interval of each BIS in the BIG */
        .max_pdu    = gbisOctetPer,                /* Maximum size of an SDU, in octets */
        .max_sdu    = gbisOctetPer,                /* Maximum size, in octets, of payload */
        .phy        = PHY_PREFER_2M,               /* The transmitter PHY of packets */
        .packing    = PACK_SEQUENTIAL,
        .framing    = BIS_UNFRAMED,
        .bn         = 1,                           /* The number of new payloads in each interval for each BIS */
        .irc        = 1,                           /* The number of times the scheduled payload(s) are transmitted in a given event*/
        .pto        = 2,                           /* Offset used for pre-transmissions */
        .enc        = 0,                           /* Encryption flag */
        /* TK: all zeros, just like JustWorks TODO: LE security mode 3, here use LE security mode 3 level2 */
        .broadcast_code = {0}, /* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
    };

    memcpy(&gBigCreateTstParam, &tBigCreateTstParam, sizeof(hci_le_createBigParamsTest_t));
}

///////////////////////////////////////////


u16 app_bisBcstHandle[APP_BIS_NUM_IN_PER_BIG_BCST] = {0};

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

        if (evtCode == HCI_EVT_LE_META) //LE Event
        {
            u8 subEvt_code = p[0];

            //------hci le event: le create BIG complete event-------------------------------
            if (subEvt_code == HCI_SUB_EVT_LE_CREATE_BIG_COMPLETE) // create BIG complete
            {
                hci_le_createBigCompleteEvt_t *pEvt = (hci_le_createBigCompleteEvt_t *)p;

                if (pEvt->status == BLE_SUCCESS) {
                    BLT_APP_LOG("Create BIG complete succeed");
                    BLT_APP_LOG("app_bisBcstNum:0x%x", pEvt->numBis);

                    for (int i = 0; i < pEvt->numBis; i++) {
                        app_bisBcstHandle[i] = pEvt->bisHandles[i];
                        BLT_APP_LOG("app_bisBcstHandle:0x%x", app_bisBcstHandle[i]);
                    }

                    BLT_APP_LOG("handle:0x%x", app_bisBcstHandle);
                    gAppAudioIsSend = true;
                } else {
                    BLT_APP_LOG("Create BIG complete failed:0x%x", pEvt->status);
                }
            }
        }
    }
    return 0;
}

static void blc_ll_updateBcstID(int advData_len, u8 *advData)
{
    //Generate random Broadcast_ID for Broadcast Audio Announcement, Ref: BAP_v1.0 3.7.2.1.1
    for (u8 i = 0; i < advData_len;) {
        u8 ad_length = advData[i];
        if (advData[i + 1] == DT_SERVICE_DATA) {
            if (*(u16 *)&advData[i + 2] == SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT) { //Broadcast Audio Announcement UUID
                //Broadcast_ID generator
                generateRandomNum(3, advData + i + 4);
                break;
            }
        }
        i += ad_length + 1;
    }
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


    /*************** BLE stack Initialization  Begin *********************************/

    u8 mac_public[6];
    u8 mac_random_static[6];
    blc_initMacAddress(flash_sector_mac_address, mac_public, mac_random_static);


    /*************** LinkLayer Initialization  Begin ***********************/
    blc_ll_initBasicMCU();
    blc_ll_initStandby_module(mac_public); //mandatory


    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);

    rf_set_power_level_index(RF_POWER_P3dBm);
    /*************** LinkLayer Initialization  End ***********************/


    /*************** HCI Initialization  Begin ***************************/
    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func
    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CREATE_BIG_COMPLETE | HCI_LE_EVT_MASK_TERMINATE_BIG_COMPLETE);
    /*************** HCI Initialization  End *****************************/

    /*************************** BLE stack Initialization  End *********************************/


    /*************** Extended ADV Initialization  Begin *******************/
    // Extended, None_Connectable_None_Scannable undirected, with auxiliary packet
    blc_ll_setExtAdvParam(ADV_HANDLE0, ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, ADV_INTERVAL_100MS, ADV_INTERVAL_100MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_2M, PRIVATE_EXT_FILTER_SPECIFIC_SID, 0);

    //Generate Broadcast_ID if there is Broadcast Audio Announcement in advData.
    blc_ll_updateBcstID(sizeof(tbl_advData), (u8 *)&tbl_advData);
    blc_ll_setExtAdvData(ADV_HANDLE0, sizeof(tbl_advData), (u8 *)&tbl_advData);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
    /*************** Extended ADV Initialization End **********************/


    /*************** Periodic Adv Initialization  Begin *******************/
    blc_ll_initPeriodicAdvModule_initPeriodicdAdvSetParamBuffer(app_peridAdvSet_buffer, APP_PERID_ADV_SETS_NUMBER);
    blc_ll_initPeriodicAdvDataBuffer(app_peridAdvData_buffer, APP_PERID_ADV_DATA_LENGTH);

    blc_ll_setPeriodicAdvParam(ADV_HANDLE0, PERADV_INTERVAL_100MS, PERADV_INTERVAL_100MS, PERD_ADV_PROP_MASK_TX_POWER_INCLUDE);

    app_modify_pdaAdvData();
    blc_ll_setPeriodicAdvData(ADV_HANDLE0, sizeof(bisSourcPdaeAdvData_t), (u8 *)&pdaAdvData);
    blc_ll_setPeriodicAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0);
    /*************** Periodic Adv Initialization End **********************/


    /*************** BIS BCST initialization ******************************/
    blc_ll_initBigBcstModule_initBigBcstParametersBuffer(app_bigBcstParam, APP_BIG_BCST_NUMBER);
    blc_ll_InitBisParametersBuffer(app_bisToatlParam, APP_BIS_NUM_IN_ALL_BIG_BCST, APP_BIS_NUM_IN_ALL_BIG_SYNC);

    blc_ll_initBisTxFifo(app_bisBcstTxfifo, BIS_TX_PDU_FIFO_SIZE, BIS_TX_PDU_FIFO_NUM);            /* BIS TX buffer init */
    blc_ll_initBisBcstSduInBuffer(app_bis_sdu_in_fifo, BIS_SDU_IN_FIFO_SIZE, BIS_SDU_IN_FIFO_NUM); /* IAL SDU buff init */

    app_set_createBigParamsTest();
    ble_sts_t status = blc_hci_le_createBigParamsTest(&gBigCreateTstParam);
    if (status != BLE_SUCCESS) {
        BLT_APP_LOG("big create fail");
    }
    /*************** BIS BCST Initialization End ******************************/


    u8 error_code = blc_contr_checkControllerInitialization();
    if (error_code != INIT_SUCCESS) {
        /* It's recommended that user set some UI alarm to know the exact error, e.g. LED shine, print log */
        write_log32(0x88880000 | error_code);
    #if (UI_LED_ENABLE)
        gpio_write(GPIO_LED_RED, LED_ON_LEVEL);
    #endif
    #if (TLKAPI_DEBUG_ENABLE)
        BLT_APP_LOG("Controller Init ERROR:0x%x", error_code);
        while (1) {
            tlkapi_debug_handler();
        }
    #else
        while (1)
            ;
    #endif
    }


    /*************** audio initial *******************/
    app_audio_init();
    /*************************************************/
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
_attribute_no_inline_ void main_loop(void)
{
    ////////////////////////////////////// BLE entry /////////////////////////////////
    blc_sdk_main_loop();

    #if (UI_LED_ENABLE)
    static u32 tick = 0;
    if (clock_time_exceed(tick, 500 * 1000)) {
        tick = clock_time();
        gpio_toggle(GPIO_LED_WHITE);
    }
    #endif

    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_handler();
    #endif

    ////////////////////////////////////// UI entry /////////////////////////////////

    #if (UI_KEYBOARD_ENABLE)
    proc_keyboard(0, 0, 0);
    #endif

    app_audio_handler();
}


#endif /* INTER_TEST_MODE */
