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
#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)

#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include "app_assistant.h"
#include "app_buffer.h"
#include "app_config.h"
#include "app_audio.h"
#include "app_usb_desc.h"

#define TELINK_COMPANY_ID               0x0211
//extend advertise
#define EXT_ADV_INTERVAL                ADV_INTERVAL_400MS
//periodic advertise
#define PER_ADV_INTERVAL                PERADV_INTERVAL_800MS

// BASE control
#define SOURCE_PRESENTATION_DELAY       10000
#define BASE_SUBGROUPS_NUM              1
#define BIG_INFO_BIS_NUM                2
#define AUDIO_PARAM_LC3_CFG             LC3_CFG_48_2

// BIS Parameter set
#define BIG_INFO_ISO_INTERVAL           1       //iso interval = sdu interval*n
#define BIG_INFO_TRANSPORT_LATENCY      20      //unit ms
#define BIG_INFO_ENC_FLAG               0
#define BIG_INFO_BROADCAST_CODE         "Telink"

#define BIS_INDEX_1_CHANNEL             BLC_AUDIO_LOCATION_FLAG_FL
#define BIS_INDEX_2_CHANNEL             BLC_AUDIO_LOCATION_FLAG_FR

typedef struct __attribute__((packed)) {
    int advSid;
    u16 extAdvIntervalMin;
    u16 extAdvIntervalMax;
    u16 perAdvIntervalMin;
    u16 perAdvIntervalMax;
} audio_mode_t;

audio_mode_t audio_mode = {
    .advSid = 0x08,
    .extAdvIntervalMin = ADV_INTERVAL_400MS,
    .extAdvIntervalMax = ADV_INTERVAL_400MS,
    .perAdvIntervalMin = PERADV_INTERVAL_800MS,
    .perAdvIntervalMax = PERADV_INTERVAL_800MS,
};

/**
 * @brief       Broadcast Source initial parameter.
 */
app_bisSource_param_t bisSource = {
    .dataInMode = APP_AUDIO_INPUT_AMIC,
    .BASE = {
        .presentation_delay = SOURCE_PRESENTATION_DELAY,
        .subGroupNum = BASE_SUBGROUPS_NUM,
        .BIG_param[0] = {
            .BIS_num = BIG_INFO_BIS_NUM,
            AUDIO_PARAM_LC3_CFG,
            .BIS_param[0] = {
                .BIS_index = 0x01,
                .codecCfg = { .channelAllocation = BIS_INDEX_1_CHANNEL, .perSduFrameBlocks = 1},
            },
            .BIS_param[1] = {
                .BIS_index = 0x02,
                .codecCfg = { .channelAllocation = BIS_INDEX_2_CHANNEL, .perSduFrameBlocks = 1},
            },
        },
    },
};

bool gAppAudioIsSend = false;
u32 popDataTimer = 0;
u32 sduInterval = 0;
int codecFrameDataLen = 0;

u16 app_bisBcstHandle[APP_BIS_NUM_IN_PER_BIG_BCST] = {0};


extern app_bisSource_param_t bisSource;
extern app_connect_info_t appConnInfo;

app_auracastCfgParam_t auracastCfg = {
    .broadcastID = {U24_TO_BYTES(DEFAULT_BROADCAST_ID)},
    .broadcastName = DEFAULT_BROADCAST_NAME,
    .broadcastNameLen = sizeof(DEFAULT_BROADCAST_NAME) - 1,
    .encryptionFlag = 0,
    .audioMode = 2,
};

static void app_audio_setExtAdvData(void);
static void app_audio_setPeriodicAdvData(void);
static bool app_audio_createBig(void);

bool app_audio_setBroadcastID(int bcstID)
{
    auracastCfg.broadcastID[2] = (bcstID>>16)&0xFF;
    auracastCfg.broadcastID[1] = (bcstID>>8)&0xFF;
    auracastCfg.broadcastID[0] = (bcstID>>0)&0xFF;

    return true;
}

bool app_audio_setBroadcastName(char* bcstName, u8 bcstNameLen)
{
    if(bcstNameLen == 0 || bcstNameLen>31)
        return false;

    memcpy(auracastCfg.broadcastName, bcstName, bcstNameLen);
    auracastCfg.broadcastNameLen = bcstNameLen;
    return true;
}

bool app_audio_setBroadcastCode(char bcstCode[16])
{
    strncpy((char*)auracastCfg.broadcastCode, bcstCode, 6);
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

void app_audio_setStereoAudio(void)
{
    app_usb_changeDesc(USB_SAMPLING_FREQ_48KHZ, AUDIO_TYPE_STEREO);
    auracastCfg.audioMode = 2;
    audio_mode.advSid = 0x06;
    audio_mode.extAdvIntervalMin = 0x0040;
    audio_mode.extAdvIntervalMax = 0x0046;
    audio_mode.perAdvIntervalMin = 0x0150;
    audio_mode.perAdvIntervalMax = 0x0160;
    bisSource.BASE.BIG_param[0].BIS_num = 2;
}

void app_audio_setMonoAudio(void)
{
    app_usb_changeDesc(USB_SAMPLING_FREQ_48KHZ, AUDIO_TYPE_MONO);
    auracastCfg.audioMode = 1;
    audio_mode.advSid = 0x07;
    audio_mode.extAdvIntervalMin = 0x0050;
    audio_mode.extAdvIntervalMax = 0x0056;
    audio_mode.perAdvIntervalMin = 0x0150;
    audio_mode.perAdvIntervalMax = 0x0160;
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

bool app_audio_terminateBig(void)
{
    hci_le_terminateBigParams_t terminateBig = {
        .big_handle = BIG_HANDLE_0,
        .reason = HCI_ERR_OP_CANCELLED_BY_HOST,
    };

    return blc_hci_le_terminateBig(&terminateBig) == BLE_SUCCESS;
}

bool app_audio_broadcastStart(void)
{
    blc_audio_codecSpecCfgParam_t *codecCfg = &bisSource.BASE.BIG_param[0].codecCfg;

    if (bisSource.state != APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE) {
        return false;
    }

    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_ENABLING);

    app_audio_setExtAdvData();
    app_audio_setPeriodicAdvData();
    if(!app_audio_createBig())
    {
        // Failed to create BIG - stop periodic advertising
        blc_ll_setPeriodicAdvEnable( BLC_ADV_DISABLE, ADV_HANDLE0);
        blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);
        bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE);
        return false;
    }
    //LC3 init
    lc3enc_encode_init_bap(0, codecCfg->samplingFreq, codecCfg->frameDuration, codecCfg->perCodecFrame);
    if(auracastCfg.audioMode == 2)
    {
        lc3enc_encode_init_bap(1, codecCfg->samplingFreq, codecCfg->frameDuration, codecCfg->perCodecFrame);
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
    blc_ll_setPeriodicAdvEnable( BLC_ADV_DISABLE, ADV_HANDLE0);
    blc_ll_setExtAdvEnable(BLC_ADV_DISABLE, ADV_HANDLE0, 0, 0);

    for(int i=0; i<appConnInfo.connCnt; i++)
    {
        u16 connHnadle = appConnInfo.conn[i].connHandle;
        blc_ll_disconnect(connHnadle, HCI_ERR_REMOTE_USER_TERM_CONN);
    }


    lc3enc_free_init(0);
    if(auracastCfg.audioMode == 2)
    {
        lc3enc_free_init(1);
    }
    return true;
}

void app_audio_loadInformation(void)
{
    app_auracastCfgParam_t param;
    flash_read_page(APP_DATA_STORE_FLASH_ADDR, sizeof(app_auracastCfgParam_t), (u8*)&param);
    if(param.head == APP_DATA_HEAD_VALUE)
    {
        memcpy(&auracastCfg, &param, sizeof(app_auracastCfgParam_t));
        if(auracastCfg.audioMode == 2)
        {
            app_audio_setStereoAudio();
        }
        else
        {
            app_audio_setMonoAudio();
        }
    }
}

void app_audio_storeInformation(void)
{
    if(auracastCfg.head == APP_DATA_HEAD_VALUE)
    {
        flash_erase_sector(APP_DATA_STORE_FLASH_ADDR);
    }
    flash_write_page(APP_DATA_STORE_FLASH_ADDR, sizeof(app_auracastCfgParam_t), (u8*)&auracastCfg);
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
    blc_ll_setExtAdvParam( ADV_HANDLE0,         ADV_EVT_PROP_EXTENDED_NON_CONNECTABLE_NON_SCANNABLE_UNDIRECTED, audio_mode.extAdvIntervalMin,   audio_mode.extAdvIntervalMax,
                           BLT_ENABLE_ADV_ALL,  OWN_ADDRESS_PUBLIC,                                             BLE_ADDR_PUBLIC,        NULL,
                           ADV_FP_NONE,         TX_POWER_3dBm,                                                  BLE_PHY_1M,             0,
                           BLE_PHY_2M,          audio_mode.advSid,                              0);

}

/**
 * @brief       user set extend advertise data.
 * @param[in]   none
 * @return      none
 */
static void app_audio_setExtAdvData(void)
{
    typedef struct {
        blc_adv_ltv_t ltv;
        u16 company_id;
        u8 configuration;
    } adv_manufacturer_data_field;

    adv_manufacturer_data_field manDataField = {
        .ltv.len = 0x04,
        .ltv.type = DT_MANUFACTURER_SPECIFIC_DATA,
        .company_id = TELINK_COMPANY_ID,
        .configuration = 0x00,
    };

    u8 advData[255];

    blc_adv_ltv_t *adv_ltvs[] = {
            (blc_adv_ltv_t *) &advDefFlags,         //ADType: flags.
            (blc_adv_ltv_t *) &advDefCompleteName,  //ADType: Complete Name.
            (blc_adv_ltv_t *) &advDefBroadcastId,   //ADType: Broadcast ID.
            (blc_adv_ltv_t *) &advDefPbpFeature,    //ADType: PBP Feature.
            (blc_adv_ltv_t *) &advDefBcastName,     //ADType: Broadcast Name.
            (blc_adv_ltv_t *) &manDataField,        //ADtype: Manufacturer Data.
            };

    advDefPbpFeature.feature = (BIG_INFO_ENC_FLAG? BLC_AUDIO_PBA_FEATURE_ENCRYPTION : 0x00) |
            (bisSource.BASE.BIG_param[0].codecCfg.samplingFreq >= BLC_AUDIO_FREQ_CFG_48000? BLC_AUDIO_PBA_FEATURE_HIGH_AUDIO: BLC_AUDIO_PBA_FEATURE_STANDARD_AUDIO);

    u8 adv_ext_len = blc_adv_buildAdvData(adv_ltvs, ARRAY_SIZE(adv_ltvs), advData);

    //Generate Broadcast_ID if there is Broadcast Audio Announcement in advData.
    blc_ll_setExtAdvData(ADV_HANDLE0, adv_ext_len, (u8 *)&advData[0]);
    blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE0, 0, 0);
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

    blc_ll_setPeriodicAdvParam( ADV_HANDLE0, audio_mode.perAdvIntervalMin, audio_mode.perAdvIntervalMax, PERD_ADV_PROP_MASK_TX_POWER_INCLUDE);
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

    blc_ll_setPeriodicAdvData( ADV_HANDLE0, blc_bap_calculateBASELength(&bisSource.BASE), &pdaAdvData[0]);
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
                    tlkapi_printf(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Create BIG complete succeed, Bis_num is %d \n", pEvt->numBis);

                    for(int i = 0; i < pEvt->numBis; i++){
                        app_bisBcstHandle[i] = pEvt->bisHandles[i];
                    }
                    gAppAudioIsSend = true;
                    popDataTimer = (clock_time() - 2000*SYSTEM_TIMER_TICK_1US) | 1;
                    #if APP_AUDIO_INPUT_MODE <= APP_AUDIO_INPUT_CODEC_ENDING
                    app_audio_cleanCodecRxBuffer();
                    #elif APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
                    usb_audio_cleanUsbRxBuffer();
                    #endif
                    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_ACTIVE);
                }
                else{
                    tlkapi_printf(APP_CONTR_EVT_LOG_EN, "[APP][EVT] Create BIG complete failed, status is 0x%x \n", pEvt->status);
                    bcast_state_set(APP_AUDIO_BRODCAST_SOURCE_STATE_IDLE);
                }
            }
            else if (subEvt_code == HCI_SUB_EVT_LE_TERMINATE_BIG_COMPLETE)
            {
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
//  blc_hci_le_setEventMask_cmd(HCI_LE_EVT_MASK_CREATE_BIG_COMPLETE);

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
    sduInterval = bisSource.BASE.BIG_param[0].codecCfg.frameDuration == BLC_AUDIO_DURATION_CFG_10? 10000: 7500;     //unit us

    u16 sduSize = bisSource.BASE.BIG_param[0].codecCfg.perCodecFrame;

    blc_audio_codecSpecCfgParam_t *bis0CodecCfg = &bisSource.BASE.BIG_param[0].BIS_param[0].codecCfg;

    if(bis0CodecCfg->channelAllocation == (BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR))
    {
        sduSize = sduSize * 2;
    }

    u16 isoInterval = sduInterval*BIG_INFO_ISO_INTERVAL/1250;

    u8 nse, bn, irc, pto;

    u8 rtn = (BIG_INFO_TRANSPORT_LATENCY*1000)/(isoInterval*1250);

    if(rtn < 4)
    {
        pto = rtn? rtn-1: 0;
        bn = BIG_INFO_ISO_INTERVAL;
        nse = 4*bn;
        irc = 4-pto;
    }
    else
    {
        pto = 4;
        bn = BIG_INFO_ISO_INTERVAL;
        nse = 4*bn;
        irc = 3;
    }

    hci_le_createBigParamsTest_t pBigCreateTstParam = {
        .big_handle = BIG_HANDLE_0,                         /* Used to identify the BIG */
        .adv_handle = ADV_HANDLE0,                          /* Used to identify the periodic advertising train */
        .num_bis = BIG_INFO_BIS_NUM,                        /* Total number of BISes in the BIG */
        .sdu_intvl = {U24_TO_BYTES(sduInterval)},           /* The interval, in microseconds, of periodic SDUs */
        .iso_intvl = isoInterval,                           /* The time between consecutive BIG anchor points */
        .nse = nse,                                         /* The total number of subevents in each interval of each BIS in the BIG */
        .max_pdu = min(BIS_TX_MAX_PDU, sduSize),            /* Maximum size of an SDU, in octets */
        .max_sdu = sduSize,                                 /* Maximum size, in octets, of payload */
        .phy = PHY_PREFER_2M,                               /* The transmitter PHY of packets */
        .packing = PACK_INTERLEAVED,
        .framing = BIS_UNFRAMED,
        .bn = bn,                                           /* The number of new payloads in each interval for each BIS */
        .irc = irc,                                         /* The number of times the scheduled payload(s) are transmitted in a given event*/
        .pto = pto,                                         /* Offset used for pre-transmissions */
        .enc = BIG_INFO_ENC_FLAG,                               /* Encryption flag */
        /* TK: all zeros, just like JustWorks TODO: LE security mode 3, here use LE security mode 3 level2 */
        .broadcast_code = {0},                              /* The code used to derive the session key that is used to encrypt and decrypt BIS payloads */
    };

    strncpy((char*)pBigCreateTstParam.broadcast_code, BIG_INFO_BROADCAST_CODE, 16);
    ble_sts_t status = blc_hci_le_createBigParamsTest(&pBigCreateTstParam);

    tlkapi_printf(APP_LOG_EN, "BIG create parameter status:0x%x \r\n", status);

    return status == BLE_SUCCESS;
}

/**
 * @brief       audio initial function.
 * @param[in]   none.
 * @return      true: initial successful, fail: initial failed.
 */
bool app_audio_init(void)
{
    blc_audio_codecSpecCfgParam_t *codecCfg = &bisSource.BASE.BIG_param[0].codecCfg;

    switch(codecCfg->samplingFreq)
    {
        case BLC_AUDIO_FREQ_CFG_8000:
            codecFrameDataLen = 80;
            break;
        case BLC_AUDIO_FREQ_CFG_16000:
            codecFrameDataLen = 160;
            break;
        case BLC_AUDIO_FREQ_CFG_24000:
            codecFrameDataLen = 240;
            break;
        case BLC_AUDIO_FREQ_CFG_32000:
            codecFrameDataLen = 320;
            break;
        case BLC_AUDIO_FREQ_CFG_48000:
            codecFrameDataLen = 480;
            break;
        default:
            codecFrameDataLen = 160;
            break;
    }

    app_audio_initExtAdv();
    app_audio_initPeriodicAdv();
    app_audio_initBig();

    app_audio_loadInformation();

    app_audio_setExtAdvData();
    app_audio_setPeriodicAdvData();
    if(!app_audio_createBig())
    {
        return false;
    }

    lc3enc_encode_init_bap(0, codecCfg->samplingFreq, codecCfg->frameDuration, codecCfg->perCodecFrame);
    lc3enc_encode_init_bap(1, codecCfg->samplingFreq, codecCfg->frameDuration, codecCfg->perCodecFrame);

#if APP_AUDIO_INPUT_MODE <= APP_AUDIO_INPUT_CODEC_ENDING
    app_audio_initCodec();
#elif APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
    if(codecCfg->samplingFreq != BLC_AUDIO_FREQ_CFG_48000)
    {
        tlkapi_printf(APP_LOG_EN, "usb audio sampling Frequency must 48kHZ.\r\n");
        return false;
    }
    app_audio_initUsbMic();
#endif

    reg_usb_ep1_buf_addr = 0x00;
    reg_usb_ep3_buf_addr = 0x00;
    reg_usb_ep6_buf_addr = 0x00;
    reg_usb_ep7_buf_addr = 0xc0;
    reg_usb_ep8_buf_addr = 0xc0;
    reg_usb_ep5_buf_addr = 0xc0;
    reg_usb_ep4_buf_addr = 0xe0;
    reg_usb_ep2_buf_addr = 0x00;

    app_assistant_init();

    return true;
}

/**
 * @brief       bis send sdu packet to Controller.
 * @param[in]   sdu: the sdu value pointer, default sdu include [left audio, right audio].
 *              sduSize: per sdu size.
 * @return      none.
 */
static void app_audio_sendSdu(u8* sdu, u8 sduSize)
{
    blc_audio_codecSpecCfgParam_t *codecCfg = &bisSource.BASE.BIG_param[0].codecCfg;

    blc_audio_codecSpecCfgParam_t *bis0CodecCfg = &bisSource.BASE.BIG_param[0].BIS_param[0].codecCfg;

    if(bis0CodecCfg->channelAllocation == BLC_AUDIO_LOCATION_FLAG_FL)
    {
        blc_iso_sendData(app_bisBcstHandle[0], sdu, codecCfg->perCodecFrame);
    }
    else if(bis0CodecCfg->channelAllocation == BLC_AUDIO_LOCATION_FLAG_FR)
    {
        blc_iso_sendData(app_bisBcstHandle[0], sdu + codecCfg->perCodecFrame, codecCfg->perCodecFrame);
    }
    else if(bis0CodecCfg->channelAllocation == (BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR))
    {
        blc_iso_sendData(app_bisBcstHandle[0], sdu, 2*codecCfg->perCodecFrame);
    }

    #if(BIG_INFO_BIS_NUM == 2)
    blc_audio_codecSpecCfgParam_t *bis1CodecCfg = &bisSource.BASE.BIG_param[0].BIS_param[1].codecCfg;

    if(bis1CodecCfg->channelAllocation == BLC_AUDIO_LOCATION_FLAG_FL)
    {
        blc_iso_sendData(app_bisBcstHandle[1], sdu, codecCfg->perCodecFrame);
    }
    else if(bis1CodecCfg->channelAllocation == BLC_AUDIO_LOCATION_FLAG_FR)
    {
        blc_iso_sendData(app_bisBcstHandle[1], sdu + codecCfg->perCodecFrame, codecCfg->perCodecFrame);
    }
    else if(bis1CodecCfg->channelAllocation == (BLC_AUDIO_LOCATION_FLAG_FL | BLC_AUDIO_LOCATION_FLAG_FR))
    {
        blc_iso_sendData(app_bisBcstHandle[1], sdu, 2*codecCfg->perCodecFrame);
    }
    #endif
}

/**
 * @brief       app audio handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_handler(void)
{
    app_assistant_handler();
#if APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
    app_audio_usbMicHandler();
#endif

    if(!gAppAudioIsSend || !clock_time_exceed(popDataTimer, sduInterval))
    {
        return ;
    }

    popDataTimer += sduInterval*SYSTEM_TIMER_TICK_1US;
    u16 audioData[APP_AUDIO_FRAME_BYTES];

#if APP_AUDIO_INPUT_MODE <= APP_AUDIO_INPUT_CODEC_ENDING
    app_audio_getCodecData(audioData);
#elif APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
    app_audio_getUsbMicData(audioData);
#elif APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_NONE
    memset(audioData, 0, sizeof(audioData));
#endif

    blc_audio_codecSpecCfgParam_t *codecCfg = &bisSource.BASE.BIG_param[0].codecCfg;

    u16 audioBuff[APP_AUDIO_FRAME_SAMPLE];
    u8 codecSdu[310];       //Max LC3 encode data

    for(int i = 0; i<codecFrameDataLen; i++)
    {
        audioBuff[i] = audioData[2*i];
    }
    lc3enc_encode_pkt(0, (u8*)audioBuff, codecSdu);

    for(int i = 0; i<codecFrameDataLen; i++)
    {
        audioBuff[i] = audioData[2*i + 1];
    }
    lc3enc_encode_pkt(1, (u8*)audioBuff, codecSdu+codecCfg->perCodecFrame);

#if APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_NONE
    static u8 noneInputTestCnt = 0;
    noneInputTestCnt ++;
    memset(codecSdu, noneInputTestCnt, sizeof(codecSdu));
#endif

    app_audio_sendSdu(codecSdu, codecCfg->perCodecFrame);

}



#endif      //SOURCE_VERSION == SOURCE_WITH_ASSISTANT

