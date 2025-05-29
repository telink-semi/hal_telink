/********************************************************************************************************
 * @file    app_audio.c
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

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER)

    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_buffer.h"
    #include "app_config.h"
    #include "app_audio.h"
    #include "app_audio_ui.h"
    #include "app_usb_desc.h"

typedef struct
{
    int advSid;
    u16 extAdvIntervalMin;
    u16 extAdvIntervalMax;
    u16 perAdvIntervalMin;
    u16 perAdvIntervalMax;
} audio_mode_t;

audio_mode_t audio_mode = {
    .advSid            = 0x06,
    .extAdvIntervalMin = 0x0040,
    .extAdvIntervalMax = 0x0046,
    .perAdvIntervalMin = 0x0150,
    .perAdvIntervalMax = 0x0160,
};

    // BASE control
    #define SOURCE_PRESENTATION_DELAY 20000
    #define BASE_SUBGROUPS_NUM        1

    // BIS Parameter set
    #define BIG_INFO_ISO_INTERVAL      2  //iso interval = sdu interval*n
    #define BIG_INFO_TRANSPORT_LATENCY 20 //unit ms

    #define BIS_INDEX_1_CHANNEL        BLC_AUDIO_LOCATION_FLAG_FL
    #define BIS_INDEX_2_CHANNEL        BLC_AUDIO_LOCATION_FLAG_FR

/**
 * @brief       Broadcast Source initial parameter.
 */
app_bisSource_param_t bisSource = {
    .BASE = {
             .presentation_delay = SOURCE_PRESENTATION_DELAY,
             .subGroupNum        = BASE_SUBGROUPS_NUM,
             .BIG_param[0]       = {
                  .BIS_num = 2,
            LC3_CFG_24_2,
                  .BIS_param[0] = {
                      .BIS_index = 0x01,
                      .codecCfg  = {.channelAllocation = BIS_INDEX_1_CHANNEL, .perSduFrameBlocks = 1},
            },
                  .BIS_param[1] = {
                      .BIS_index = 0x02,
                      .codecCfg  = {.channelAllocation = BIS_INDEX_2_CHANNEL, .perSduFrameBlocks = 1},
            },
        },
             },
};

bool gAppAudioIsSend   = false;
u32  popDataTimer      = 0;
u32  sduInterval       = 10000; //unit us
int  codecFrameDataLen = 0;

u16 app_bisBcstHandle[APP_BIS_NUM_IN_PER_BIG_BCST] = {0};

app_auracastCfgParam_t auracastCfg = {
    .broadcastID      = {U24_TO_BYTES(DEFAULT_BROADCAST_ID)},
    .broadcastName    = DEFAULT_BROADCAST_NAME,
    .broadcastNameLen = sizeof(DEFAULT_BROADCAST_NAME) - 1,
    .encryptionFlag   = 0,
    .audioMode        = 2,
};

bool app_audio_setBroadcastID(int bcstID)
{
    auracastCfg.broadcastID[2] = (bcstID >> 16) & 0xFF;
    auracastCfg.broadcastID[1] = (bcstID >> 8) & 0xFF;
    auracastCfg.broadcastID[0] = (bcstID >> 0) & 0xFF;

    return true;
}

bool app_audio_setBroadcastName(char *bcstName, u8 bcstNameLen)
{
    if (bcstNameLen == 0 || bcstNameLen > 31) {
        return false;
    }

    memcpy(auracastCfg.broadcastName, bcstName, bcstNameLen);
    auracastCfg.broadcastNameLen = bcstNameLen;
    return true;
}

bool app_audio_setBroadcastCode(char bcstCode[16])
{
    strncpy((char *)auracastCfg.broadcastCode, bcstCode, 6);
    auracastCfg.encryptionFlag = 1;
    return true;
}

bool app_audio_closeEncryptBig(void)
{
    auracastCfg.encryptionFlag = 0;
    return true;
}

u8 app_audio_getBroadcastState(void)
{
    return bisSource.state;
}

void app_usb_changeDesc(vendor_usbDesc_t *newDesc);

static const vendor_usbDesc_t stereoAudio = {
    .vendorId        = 0x248a,
    .productId       = 0x6103,
    .speakSampleRate = 24000,
    .speakNum        = 2,
};

static const vendor_usbDesc_t monoAudio = {
    .vendorId        = 0x248a,
    .productId       = 0x6104,
    .speakSampleRate = 24000,
    .speakNum        = 1,
};

void app_audio_setStereoAudio(void)
{
    app_usb_changeDesc((vendor_usbDesc_t *)&stereoAudio);
    auracastCfg.audioMode               = 2;
    audio_mode.advSid                   = 0x06;
    audio_mode.extAdvIntervalMin        = 0x0040;
    audio_mode.extAdvIntervalMax        = 0x0046;
    audio_mode.perAdvIntervalMin        = 0x0150;
    audio_mode.perAdvIntervalMax        = 0x0160;
    bisSource.BASE.BIG_param[0].BIS_num = 2;
}

void app_audio_setMonoAudio(void)
{
    app_usb_changeDesc((vendor_usbDesc_t *)&monoAudio);
    auracastCfg.audioMode               = 1;
    audio_mode.advSid                   = 0x07;
    audio_mode.extAdvIntervalMin        = 0x0050;
    audio_mode.extAdvIntervalMax        = 0x0056;
    audio_mode.perAdvIntervalMin        = 0x0150;
    audio_mode.perAdvIntervalMax        = 0x0160;
    bisSource.BASE.BIG_param[0].BIS_num = 1;
}

static bcast_state_changed_cb bcast_state_changed = NULL;

static void bcast_state_set(app_audio_brodcast_state_enum state)
{
    bisSource.state = state;
    if (bcast_state_changed) {
        bcast_state_changed(bisSource.state);
    }
}

void bcast_set_state_changed_cb(bcast_state_changed_cb cb)
{
    bcast_state_changed = cb;
}

/**
 * @brief       user initialization extend advertise parameter.
 * @param[in]   none
 * @return      none
 */
static void app_audio_initExtAdv(void)
{
    /* Extended ADV module and ADV Set Parameters buffer initialization */
    blc_ll_initExtendedAdvModule_initExtendedAdvSetParamBuffer(app_extAdvSetParam_buf, APP_EXT_ADV_SETS_NUMBER);
    blc_ll_initExtendedAdvDataBuffer(app_extAdvData_buf, APP_EXT_ADV_DATA_LENGTH);


    // Extended, None_Connectable_None_Scannable undirected, with auxiliary packet
    blc_ll_setExtAdvParam(AURACAST_SOURCE_ADV_HANDLE, ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, audio_mode.extAdvIntervalMin, audio_mode.extAdvIntervalMax, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_2M, audio_mode.advSid, 0);
}

/**
 * @brief       user set extend advertise data.
 * @param[in]   none
 * @return      none
 */
static void app_audio_setExtAdvData(void)
{
    blc_adv_broadcastId_t advBroadcastId = {
        .ltv.len  = sizeof(blc_adv_broadcastId_t) - 1,
        .ltv.type = DT_SERVICE_DATA_16BIT_UUID,
        .baasUuid = SERVICE_UUID_BROADCAST_AUDIO_ANNOUNCEMENT,
    };

    blc_adv_broadcastName_t advBcastName = {
        .ltv.len  = auracastCfg.broadcastNameLen + 1,
        .ltv.type = DT_BROADCAST_NAME,
    };

    memcpy(advBroadcastId.broadcastId, auracastCfg.broadcastID, 3);
    memcpy(advBcastName.bcastName, auracastCfg.broadcastName, auracastCfg.broadcastNameLen);

    u8 advData[255];

    blc_adv_ltv_t *adv_ltvs[] = {
        (blc_adv_ltv_t *)&advDefFlags,        //ADType: flags.
        (blc_adv_ltv_t *)&advDefCompleteName, //ADType: Complete Name.
        (blc_adv_ltv_t *)&advBroadcastId,     //ADType: Broadcast ID.
        (blc_adv_ltv_t *)&advDefPbpFeature,   //ADType: PBP Feature.
        (blc_adv_ltv_t *)&advBcastName,       //ADType: Broadcast Name.
    };

    advDefPbpFeature.feature = BLC_AUDIO_PBA_FEATURE_STANDARD_AUDIO;
    advDefPbpFeature.feature |= auracastCfg.encryptionFlag ? BLC_AUDIO_PBA_FEATURE_ENCRYPTION : 0;

    u8 adv_ext_len = blc_adv_buildAdvData(adv_ltvs, ARRAY_SIZE(adv_ltvs), advData);

    //Generate Broadcast_ID if there is Broadcast Audio Announcement in advData.
    blc_ll_setExtAdvData(AURACAST_SOURCE_ADV_HANDLE, adv_ext_len, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, AURACAST_SOURCE_ADV_HANDLE, 0, 0);
}

/**
 * @brief       user initialization periodic advertise parameter.
 * @param[in]   none
 * @return      none
 */
static void app_audio_initPeriodicAdv(void)
{
    blc_ll_initPeriodicAdvModule_initPeriodicdAdvSetParamBuffer(app_peridAdvSet_buffer, APP_PERID_ADV_SETS_NUMBER);
    blc_ll_initPeriodicAdvDataBuffer(app_peridAdvData_buffer, APP_PERID_ADV_DATA_LENGTH);

    blc_ll_setPeriodicAdvParam(AURACAST_SOURCE_ADV_HANDLE, audio_mode.perAdvIntervalMin, audio_mode.perAdvIntervalMax, PERD_ADV_PROP_MASK_TX_POWER_INCLUDE);
}

/**
 * @brief       user set periodic advertise data.
 * @param[in]   none
 * @return      none
 */
static void app_audio_setPeriodicAdvData(void)
{
    u8 pdaAdvData[256];

    blc_bap_setBASEToAddress(&bisSource.BASE, pdaAdvData);

    blc_ll_setPeriodicAdvData(AURACAST_SOURCE_ADV_HANDLE, blc_bap_calculateBASELength(&bisSource.BASE), &pdaAdvData[0]);
    blc_ll_setPeriodicAdvEnable(BLC_ADV_ENABLE, AURACAST_SOURCE_ADV_HANDLE);
}

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
                    tlkapi_printf(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Create BIG complete succeed, Bis_num is %d \r\n", pEvt->numBis);

                    for (int i = 0; i < pEvt->numBis; i++) {
                        app_bisBcstHandle[i] = pEvt->bisHandles[i];
                    }
                    gAppAudioIsSend = true;
                    popDataTimer    = (clock_time() - 2000 * SYSTEM_TIMER_TICK_1US) | 1;
    #if APP_AUDIO_INPUT_MODE <= APP_AUDIO_INPUT_CODEC_ENDING
                    app_audio_cleanCodecRxBuffer();
    #elif APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
                    usb_audio_cleanUsbRxBuffer();
    #endif
                    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_ACTIVE);
                } else {
                    tlkapi_printf(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Create BIG complete failed, status is 0x%x \r\n", pEvt->status);
                    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE);
                }
            } else if (subEvt_code == HCI_SUB_EVT_LE_TERMINATE_BIG_COMPLETE) {
                memset(app_bisBcstHandle, 0, sizeof(app_bisBcstHandle));
                gAppAudioIsSend = false;
                bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE);
            }
        }
    }
    return 0;
}

/**
 * @brief       user initialization Big parameter.
 * @param[in]   none
 * @return      none
 */
static void app_audio_initBig(void)
{
    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func

    //bluetooth low energy(LE) event
    blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CREATE_BIG_COMPLETE | HCI_LE_EVT_MASK_TERMINATE_BIG_COMPLETE);

    blc_ll_initBigBcstModule_initBigBcstParametersBuffer(app_bigBcstParam, APP_BIG_BCST_NUMBER);

    blc_ll_InitBisParametersBuffer(app_bisToatlParam, APP_BIS_NUM_IN_ALL_BIG_BCST, APP_BIS_NUM_IN_ALL_BIG_SYNC);

    /* BIS TX buffer init */
    blc_ll_initBisTxFifo(app_bisBcstTxfifo, BIS_TX_PDU_FIFO_SIZE, BIS_TX_PDU_FIFO_NUM);

    /* IAL SDU buff init */
    blc_ll_initBisBcstSduInBuffer(app_bis_sdu_in_fifo, BIS_SDU_IN_FIFO_SIZE, BIS_SDU_IN_FIFO_NUM);
}

/**
 * @brief       user Create BIG Broadcast.
 * @param[in]   none
 * @return      true: create BIG Successful.
 *              false: create BIG Failed.
 */
static bool app_audio_createBig(void)
{
    u16 sduSize = 60;

    u16 isoInterval = sduInterval * BIG_INFO_ISO_INTERVAL / 1250;

    u8 nse, bn, irc, pto;

    u8 rtn = (BIG_INFO_TRANSPORT_LATENCY * 1000) / (isoInterval * 1250);

    if (rtn < 4) {
        pto = rtn ? rtn - 1 : 0;
        bn  = BIG_INFO_ISO_INTERVAL;
        nse = 4 * bn;
        irc = 4 - pto;
    } else {
        pto = 4;
        bn  = BIG_INFO_ISO_INTERVAL;
        nse = 4 * bn;
        irc = 3;
    }

    hci_le_createBigParamsTest_t pBigCreateTstParam = {
        .big_handle = AURACAST_SOURCE_BIG_HANDLE,   /* Used to identify the BIG */
        .adv_handle = AURACAST_SOURCE_ADV_HANDLE,   /* Used to identify the periodic advertising train */
        .num_bis    = auracastCfg.audioMode,        /* Total number of BISes in the BIG */
        .sdu_intvl  = {U24_TO_BYTES(sduInterval)},  /* The interval, in microseconds, of periodic SDUs */
        .iso_intvl  = isoInterval,                  /* The time between consecutive BIG anchor points */
        .nse        = nse,                          /* The total number of subevents in each interval of each BIS in the BIG */
        .max_pdu    = min(BIS_TX_MAX_PDU, sduSize), /* Maximum size of an SDU, in octets */
        .max_sdu    = sduSize,                      /* Maximum size, in octets, of payload */
        .phy        = PHY_PREFER_2M,                /* The transmitter PHY of packets */
        .packing    = PACK_INTERLEAVED,
        .framing    = BIS_UNFRAMED,
        .bn         = bn,                           /* The number of new payloads in each interval for each BIS */
        .irc        = irc,                          /* The number of times the scheduled payload(s) are transmitted in a given event*/
        .pto        = pto,                          /* Offset used for pre-transmissions */
        .enc        = auracastCfg.encryptionFlag,   /* Encryption flag */
        /* TK: all zeros, just like JustWorks TODO: LE security mode 3, here use LE security mode 3 level2 */
        .broadcast_code = {0}, /* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
    };

    memcpy(pBigCreateTstParam.broadcast_code, auracastCfg.broadcastCode, 16);

    ble_sts_t status = blc_hci_le_createBigParamsTest(&pBigCreateTstParam);

    tlkapi_printf(APP_LOG_EN, "BIG create parameter status:0x%x \r\n", status);
    return status == BLE_SUCCESS;
}

bool app_audio_terminateBig(void)
{
    hci_le_terminateBigParams_t terminateBig = {
        .big_handle = AURACAST_SOURCE_BIG_HANDLE,
        .reason     = HCI_ERR_OP_CANCELLED_BY_HOST,
    };

    return blc_hci_le_terminateBig(&terminateBig) == BLE_SUCCESS;
}

bool app_audio_broadcastStart(void)
{
    if (bisSource.state != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
        return false;
    }

    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_ENABLING);

    app_audio_setExtAdvData();
    app_audio_setPeriodicAdvData();
    if (!app_audio_createBig()) {
        // Failed to create BIG - stop periodic advertising
        blc_ll_setPeriodicAdvEnable(BLC_ADV_DISABLE, AURACAST_SOURCE_ADV_HANDLE);
        blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, AURACAST_SOURCE_ADV_HANDLE, 0, 0);
        bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE);
        return false;
    }
    //LC3_24_2
    lc3enc_encode_init_bap(0, BLC_AUDIO_FREQ_CFG_24000, BLC_AUDIO_DURATION_CFG_10, 60);
    if (auracastCfg.audioMode == 2) {
        lc3enc_encode_init_bap(1, BLC_AUDIO_FREQ_CFG_24000, BLC_AUDIO_DURATION_CFG_10, 60);
    }
    return true;
}

bool app_audio_broadcastStop(void)
{
    if (bisSource.state != APP_AUDIO_BRODCAST_SOURCE_STATE_ACTIVE) {
        return false;
    }

    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_DISABLING);
    gAppAudioIsSend = false;
    app_audio_terminateBig();
    blc_ll_setPeriodicAdvEnable(BLC_ADV_DISABLE, AURACAST_SOURCE_ADV_HANDLE);
    blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, AURACAST_SOURCE_ADV_HANDLE, 0, 0);

    lc3enc_free_init(0);
    if (auracastCfg.audioMode == 2) {
        lc3enc_free_init(1);
    }
    return true;
}

void app_audio_loadInformation(void)
{
    app_auracastCfgParam_t param;
    flash_read_page(APP_DATA_STORE_FLASH_ADDR, sizeof(app_auracastCfgParam_t), (u8 *)&param);
    if (param.head == APP_DATA_HEAD_VALUE) {
        memcpy(&auracastCfg, &param, sizeof(app_auracastCfgParam_t));
        if (auracastCfg.audioMode == 2) {
            app_audio_setStereoAudio();
        } else {
            app_audio_setMonoAudio();
        }
    }
}

void app_audio_storeInformation(void)
{
    auracastCfg.head = APP_DATA_HEAD_VALUE;
    flash_write_page(APP_DATA_STORE_FLASH_ADDR, sizeof(app_auracastCfgParam_t), (u8 *)&auracastCfg);
}

/**
 * @brief       audio initial function.
 * @param[in]   none.
 * @return      true: initial successful, fail: initial failed.
 */
bool app_audio_init(void)
{
    codecFrameDataLen = 240;

    app_audio_initExtAdv();
    app_audio_initPeriodicAdv();
    app_audio_initBig();

    app_audio_ui_init();
    app_audio_initUsbMic();

    app_audio_loadInformation();

    return app_audio_broadcastStart();
}

/**
 * @brief       bis send sdu packet to Controller.
 * @param[in]   sdu: the sdu value pointer, default sdu include [left audio, right audio].
 *              sduSize: per sdu size.
 * @return      none.
 */
static void app_audio_sendSdu(u8 *sdu, u8 sduSize)
{
    blc_iso_sendData(app_bisBcstHandle[0], sdu, sduSize);
    if (auracastCfg.audioMode == 2) {
        blc_iso_sendData(app_bisBcstHandle[1], sdu + sduSize, sduSize);
    }
}

/**
 * @brief       app audio handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_handler(void)
{
    #if APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
    app_audio_usbMicHandler();
    #endif
    app_audio_ui_loop();

    if (!gAppAudioIsSend || !clock_time_exceed(popDataTimer, sduInterval)) {
        return;
    }

    popDataTimer += sduInterval * SYSTEM_TIMER_TICK_1US;
    u16 audioData[APP_AUDIO_FRAME_BYTES];

    app_audio_getUsbMicData(audioData);

    u16 audioBuff[APP_AUDIO_FRAME_SAMPLE];
    u8  codecSdu[120]; //Max LC3 encode data
    for (int i = 0; i < codecFrameDataLen; i++) {
        audioBuff[i] = audioData[2 * i];
    }
    lc3enc_encode_pkt(0, (u8 *)audioBuff, codecSdu);

    if (auracastCfg.audioMode == 2) {
        for (int i = 0; i < codecFrameDataLen; i++) {
            audioBuff[i] = audioData[2 * i + 1];
        }
        lc3enc_encode_pkt(1, (u8 *)audioBuff, codecSdu + 60);
    }

    app_audio_sendSdu(codecSdu, 60);
}


#endif //PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER
