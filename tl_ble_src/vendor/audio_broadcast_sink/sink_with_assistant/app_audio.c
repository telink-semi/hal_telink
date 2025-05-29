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
#include "../sink_config.h"
#if (SINK_VERSION == SINK_WITH_ASSISTANT_VERSION)

    #include "app_config.h"

    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_buffer.h"
    #include "app_audio.h"
    #include "app.h"
    #include "../app_ctrl_audio.h"

    #define TELINK_COMPANY_ID              0x0211
    #define TELINK_BCAST_SOURCE_NUM        (10)
    #define DEFAULT_BROADCAST_CODE         "Telink"


    #define FILTER_USE_ADDRESS             BIT(0)
    #define FILTER_USE_BROADCAST_NAME      BIT(1)
    #define FILTER_USE_TELINK_VID          BIT(2)
    #define FILTER_USE_ALL_TYPE            0xFFFFFFFF

    #define ASSISTANT_FILTER_SOURCE_METHOD FILTER_USE_TELINK_VID

    #if ASSISTANT_FILTER_SOURCE_METHOD & FILTER_USE_ADDRESS

    #endif

/**
 *  @brief  broadcast source information.
 */
typedef struct
{
    bool used;
    u8   addrType;
    u8   addr[6];
    u8   advSid;
    u8   configuration;
} telink_bcast_source_t;

/**
 *  @brief  application user state enumerate.
 */
typedef enum
{
    APP_STATE_IDLE,
    APP_STATE_SYNCING,
    APP_STATE_SYNCED,
    APP_STATE_TERMINATING,
} app_state_t;

/**
 *  @brief  periodic advertise sync state enumerate.
 */
typedef enum
{
    PA_SYNC_STATE_IDLE,
    PA_SYNC_STATE_CREATE,
    PA_SYNC_STATE_EST,
    PA_SYNC_STATE_RECV_BASE,
    PA_SYNC_STATE_FAILED,
    PA_SYNC_STATE_TERMINATING,
} pa_sync_state_t;

/**
 *  @brief  BIG sync state enumerate.
 */
typedef enum
{
    BIG_SYNC_STATE_IDLE,
    BIG_SYNC_STATE_CREATE,
    BIG_SYNC_STATE_EST,
    BIG_SYNC_STATE_FAILED,
    BIG_SYNC_STATE_TERMINATING,
} big_sync_state_t;

telink_bcast_source_t  telinkBcastSources[TELINK_BCAST_SOURCE_NUM];
u8                     recvBcstSourceNum = 0;
telink_bcast_source_t *currectSource_Idx = NULL;
telink_bcast_source_t *nextSource_Idx    = NULL;

typedef struct
{
    app_state_t      appState;
    u16              paSyncHandle;
    pa_sync_state_t  paState;
    big_sync_state_t bigState;
} app_all_state_t;

app_all_state_t allState = {
    .appState     = APP_STATE_IDLE,
    .paSyncHandle = 0x0000,
    .paState      = PA_SYNC_STATE_IDLE,
    .bigState     = BIG_SYNC_STATE_IDLE,
};

/**
 * @brief       Broadcast sink initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_init(void)
{
    app_codec_init();
    blc_ll_setExtScanParam(OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, 0, 0, 0);

    blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
}

/**
 * @brief       get app state string.
 * @param[in]   state: app state.
 * @return      the pointer of app state string.
 */
static const char *app_audio_getAppStateString(app_state_t state)
{
    switch (state) {
    case APP_STATE_IDLE:
        return "IDLE";
    case APP_STATE_SYNCING:
        return "SYNCING";
    case APP_STATE_SYNCED:
        return "SYNCED";
    case APP_STATE_TERMINATING:
        return "TERMINATING";
    default:
        break;
    }

    return "UNKNOWN";
}

/**
 * @brief       get periodic advertise sync state string.
 * @param[in]   state: periodic advertise sync state.
 * @return      the pointer of periodic advertise sync state string.
 */
static const char *app_audio_getPaSyncStateString(pa_sync_state_t state)
{
    switch (state) {
    case PA_SYNC_STATE_IDLE:
        return "IDLE";
    case PA_SYNC_STATE_CREATE:
        return "CREATE";
    case PA_SYNC_STATE_EST:
        return "ESTABLISHED";
    case PA_SYNC_STATE_RECV_BASE:
        return "RECEIVE_BASE";
    case PA_SYNC_STATE_FAILED:
        return "FAILED";
    case PA_SYNC_STATE_TERMINATING:
        return "TERMINATING";
    default:
        break;
    }

    return "UNKNOWN";
}

/**
 * @brief       get BIG sync state string.
 * @param[in]   state: BIG sync state.
 * @return      the pointer of BIG sync state string.
 */
static const char *app_audio_getBigSyncStateString(big_sync_state_t state)
{
    switch (state) {
    case BIG_SYNC_STATE_IDLE:
        return "IDLE";
    case BIG_SYNC_STATE_CREATE:
        return "CREATE";
    case BIG_SYNC_STATE_EST:
        return "ESTABLISHED";
    case BIG_SYNC_STATE_FAILED:
        return "FAILED";
    case BIG_SYNC_STATE_TERMINATING:
        return "TERMINATING";
    default:
        break;
    }

    return "UNKNOWN";
}

/**
 * @brief       print all state.
 * @param[in]   none.
 * @return      none.
 */
static void app_audio_printAllState(void)
{
    tlkapi_printf(APP_LOG_EN, "State: APP: %s PA: %s BIG: %s", app_audio_getAppStateString(allState.appState), app_audio_getPaSyncStateString(allState.paState), app_audio_getBigSyncStateString(allState.bigState));
}

/**
 * @brief       update app state.
 * @param[in]   state: app state.
 * @return      none.
 */
static void app_audio_updateAppState(app_state_t state)
{
    allState.appState = state;

    app_audio_printAllState();

    #if (UI_LED_ENABLE)
    if (allState.appState == APP_STATE_SYNCED) {
        gpio_write(GPIO_LED_GREEN, 1);
        #if (UI_9517C)
        gpio_write(GPIO_LED_GREEN_9517C, 1);
        #endif
    } else {
        gpio_write(GPIO_LED_GREEN, 0);
        #if (UI_9517C)
        gpio_write(GPIO_LED_GREEN_9517C, 0);
        #endif
    }
    #endif

    if (allState.appState != APP_STATE_SYNCED) {
        app_codec_setBigSyncState(BIG_LOST, 0, NULL);
    }
}

/**
 * @brief       update periodic advertise sync state.
 * @param[in]   state: periodic advertise sync state.
 * @param[in]   syncHandle: periodic advertise sync handle.
 * @return      none.
 */
static void app_audio_updatePaSyncState(pa_sync_state_t state, u16 syncHandle)
{
    allState.paState      = state;
    allState.paSyncHandle = syncHandle;
    app_audio_printAllState();
}

/**
 * @brief       update BIG sync state.
 * @param[in]   state: BIG sync state.
 * @return      none.
 */
static void app_audio_updateBigSyncState(big_sync_state_t state)
{
    allState.bigState = state;

    app_audio_printAllState();
}

/**
 * @brief       audio state change process.
 * @param[in]   none.
 * @return      none.
 */
static void app_audio_stateChangeProcess(void)
{
    app_state_t prev_app_state;
    u8          status;

    do {
        prev_app_state = allState.appState;

        if (nextSource_Idx) {
            if (allState.appState == APP_STATE_IDLE) {
                currectSource_Idx = nextSource_Idx;
                nextSource_Idx    = NULL;
                app_audio_updateAppState(APP_STATE_SYNCING);
            } else if (allState.appState == APP_STATE_SYNCING || allState.appState == APP_STATE_SYNCED) {
                app_audio_updateAppState(APP_STATE_TERMINATING);
            }
        }

        switch (allState.appState) {
        case APP_STATE_SYNCING:
            if (allState.paState == PA_SYNC_STATE_IDLE) {
                // Create PA sync
                status = blc_ll_periodicAdvertisingCreateSync(SYNC_ADV_SPECIFY | REPORTING_INITIALLY_EN,
                                                              currectSource_Idx->advSid,
                                                              currectSource_Idx->addrType,
                                                              currectSource_Idx->addr,
                                                              0,
                                                              SYNC_TIMEOUT_2S,
                                                              0);
                app_audio_updatePaSyncState(status == BLE_SUCCESS ? PA_SYNC_STATE_CREATE : PA_SYNC_STATE_IDLE, 0x0000);
                tlkapi_printf(APP_LOG_EN, "start create PDA sync, SID is %d, addr Type is %d address is %s, result is %d", currectSource_Idx->advSid, currectSource_Idx->addrType, addr_to_str(currectSource_Idx->addr), status);
                if (allState.paState == PA_SYNC_STATE_IDLE) {
                    app_audio_updateAppState(APP_STATE_TERMINATING);
                } else {
                    return;
                }
            } else if (allState.paState == PA_SYNC_STATE_FAILED) {
                app_audio_updatePaSyncState(PA_SYNC_STATE_IDLE, 0x0000);
                app_audio_updateAppState(APP_STATE_TERMINATING);
            }

            if (allState.bigState == BIG_SYNC_STATE_FAILED) {
                app_audio_updateBigSyncState(BIG_SYNC_STATE_IDLE);
                app_audio_updateAppState(APP_STATE_TERMINATING);
            }

            if (allState.paState == PA_SYNC_STATE_RECV_BASE && allState.bigState == BIG_SYNC_STATE_EST) {
                app_audio_updateAppState(APP_STATE_SYNCED);
            }
            break;
        case APP_STATE_TERMINATING:
            if (allState.paState == PA_SYNC_STATE_CREATE) {
                app_audio_updatePaSyncState(PA_SYNC_STATE_TERMINATING, 0x0000);
                blc_ll_periodicAdvertisingCreateSyncCancel();
                return;
            } else if (allState.paState == PA_SYNC_STATE_RECV_BASE) {
                blc_ll_periodicAdvertisingTerminateSync(allState.paSyncHandle);
                app_audio_updatePaSyncState(PA_SYNC_STATE_IDLE, 0x0000);
            } else if (allState.paState == PA_SYNC_STATE_FAILED) {
                app_audio_updatePaSyncState(PA_SYNC_STATE_IDLE, 0x0000);
            }

            if (allState.bigState == BIG_SYNC_STATE_CREATE || allState.bigState == BIG_SYNC_STATE_EST) {
                blc_ll_bigTerminateSync(BIG_HANDLE_0);
                app_audio_updateBigSyncState(BIG_SYNC_STATE_TERMINATING);
            } else if (allState.bigState == BIG_SYNC_STATE_FAILED) {
                app_audio_updateBigSyncState(BIG_SYNC_STATE_IDLE);
            }

            if (allState.bigState == BIG_SYNC_STATE_IDLE && allState.paState == PA_SYNC_STATE_IDLE) {
                app_audio_updateAppState(APP_STATE_IDLE);
            }
            break;
        case APP_STATE_SYNCED:
            if (allState.paState == PA_SYNC_STATE_RECV_BASE) {
                blc_ll_periodicAdvertisingTerminateSync(allState.paSyncHandle);
                app_audio_updatePaSyncState(PA_SYNC_STATE_IDLE, 0x0000);
            } else if (allState.paState == PA_SYNC_STATE_FAILED) {
                app_audio_updatePaSyncState(PA_SYNC_STATE_IDLE, 0x0000);
            }

            if (allState.bigState == BIG_SYNC_STATE_FAILED) {
                app_audio_updateAppState(APP_STATE_TERMINATING);
            }
            break;
        default:
            break;
        }
    } while (allState.appState != prev_app_state);
}

/**
 * @brief       BLE controller event LE extend advertising report callback.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static bool isTelinkSourceCapable(u8 *configuration, u8 *advData, u32 len)
{
    u8 adLen;

    u8 *buff = blc_adv_getManufacturerDataInformationByCompanyId(advData, len, TELINK_COMPANY_ID, &adLen);

    if (buff && adLen == 1) {
        *configuration = *buff;
        return true;
    }

    return false;
}

/**
 * @brief       check whether the broadcast source exists.
 * @param[in]   addr: source address.
 * @param[in]   add_type: source address type.
 * @param[in]   advSid: source advertise SID.
 * @return      true: the source already exists.
 *              false: the source not present.
 */
static bool app_audio_checkBroadcastSourceExist(u8 addr[6], u8 addr_type, u8 advSid)
{
    if (recvBcstSourceNum >= TELINK_BCAST_SOURCE_NUM) {
        return true;
    }

    for (size_t i = 0; i < TELINK_BCAST_SOURCE_NUM; i++) {
        if (telinkBcastSources[i].used && telinkBcastSources[i].addrType == addr_type &&
            telinkBcastSources[i].advSid == advSid && !memcmp(addr, telinkBcastSources[i].addr, sizeof(telinkBcastSources[i].addr))) {
            return true;
        }
    }

    return false;
}

/**
 * @brief       add a new broadcast source into buffer.
 * @param[in]   addr: source address.
 * @param[in]   add_type: source address type.
 * @param[in]   advSid: source advertise SID.
 * @return      none.
 */
static void app_audio_addNewBroadcastSource(u8 addr[6], u8 addr_type, u8 advSid)
{
    for (size_t i = 0; i < ARRAY_SIZE(telinkBcastSources); i++) {
        if (!telinkBcastSources[i].used) {
            telinkBcastSources[i].used     = true;
            telinkBcastSources[i].advSid   = advSid;
            telinkBcastSources[i].addrType = addr_type;
            memcpy(telinkBcastSources[i].addr, addr, sizeof(telinkBcastSources[i].addr));
            recvBcstSourceNum++;
            break;
        }
    }
}

/**
 * @brief       Broadcast sink LE extend advertise data report.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_leExtendAdvReport(u8 *p, int n)
{
    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;
    u8                        broadcastId[3];
    u8                        configuration;
    int                       offset = 0;
    u8                       *pAdvData;

    extAdvEvt_info_t *pExtAdv = NULL;

    for (int i = 0; i < pExtAdvRpt->num_reports; i++) {
        pExtAdv = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdv->data_length);
        pAdvData = pExtAdv->data;

        if (app_audio_checkBroadcastSourceExist(pExtAdv->address, pExtAdv->address_type, pExtAdv->advertising_sid)) {
            // we already have this info stored.
            continue;
        }


        // Check if there is a broadcast code
        if (!blc_advGetBroadcastID(pAdvData, pExtAdv->data_length, broadcastId)) {
            continue;
        }

        // Check if source transmits extra manufacturer data
        if (!isTelinkSourceCapable(&configuration, pAdvData, pExtAdv->data_length)) {
            continue;
        }

    #if (UI_LED_ENABLE)
        // Indicate that at least one device has been found
        gpio_write(GPIO_LED_BLUE, 1);
        #if (UI_9517C)
        gpio_write(GPIO_LED_BLUE_9517C, 1);
        #endif
    #endif

        app_audio_addNewBroadcastSource(pExtAdv->address, pExtAdv->address_type, pExtAdv->advertising_sid);
        tlkapi_printf(APP_LOG_EN, "[APP]Added Device Address is %s", addr_to_str(pExtAdv->address));
    #if APP_LOG_EN
        u8  completeNameLen = 0;
        u8 *completeName    = blc_adv_getCompleteNameInformation(pAdvData, pExtAdv->data_length, &completeNameLen);
        u8  bcstNameLen     = 0;
        u8 *bcstName        = blc_adv_getBroadcastNameInformation(pAdvData, pExtAdv->data_length, &bcstNameLen);
        tlkapi_printf(APP_LOG_EN, "[APP]Complete name is %.*s, Broadcast Name is %.*s", completeNameLen, completeName, bcstNameLen, bcstName);
    #endif
    }
}

/**
 * @brief       Broadcast sink LE periodic advertise sync established.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_periodicAdvSyncEst(u8 *p, int n)
{
    hci_le_periodicAdvSyncEstablishedEvt_t *pEvt = (hci_le_periodicAdvSyncEstablishedEvt_t *)p;

    tlkapi_printf(APP_LOG_EN, "PDA sync established status is %x, handle is %x", pEvt->status, pEvt->syncHandle);

    if (pEvt->status == BLE_SUCCESS) {
        app_audio_updatePaSyncState(PA_SYNC_STATE_EST, 0x0000);
    } else {
        app_audio_updatePaSyncState(PA_SYNC_STATE_FAILED, 0x0000);
    }
    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink LE periodic advertise data report.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_periodicAdvReport(u8 *p, int n)
{
    if (allState.paState == PA_SYNC_STATE_RECV_BASE) {
        return;
    }

    //TODO: recombination BASE.

    hci_le_periodicAdvReportEvt_t *pEvt = (hci_le_periodicAdvReportEvt_t *)p;

    //TODO: Generate an initial codec event based on the BASE field.

    u8 codecEvtBuf[sizeof(blc_bapbs_bisSinkInitCodecEvt_t) + 2 * sizeof(bisSyncInfo_t)];

    blc_bapbs_bisSinkInitCodecEvt_t *codecEvt = (blc_bapbs_bisSinkInitCodecEvt_t *)codecEvtBuf;

    codecEvt->presentationDelay                        = 20000;
    codecEvt->bisNum                                   = 2;
    codecEvt->bisInfo[0].CodecId.id                    = 0x06;
    codecEvt->bisInfo[0].CodecId.companyID             = 0x00;
    codecEvt->bisInfo[0].CodecId.vendorID              = 0x00;
    codecEvt->bisInfo[0].codecCfg.frequency            = 8;
    codecEvt->bisInfo[0].codecCfg.duration             = 1;
    codecEvt->bisInfo[0].codecCfg.allocation           = 0x01;
    codecEvt->bisInfo[0].codecCfg.frameOcts            = 100;
    codecEvt->bisInfo[0].codecCfg.codecFrameBlksPerSDU = 1;
    codecEvt->bisInfo[0].metadata                      = NULL;

    codecEvt->bisInfo[1].CodecId.id                    = 0x06;
    codecEvt->bisInfo[1].CodecId.companyID             = 0x00;
    codecEvt->bisInfo[1].CodecId.vendorID              = 0x00;
    codecEvt->bisInfo[1].codecCfg.frequency            = 8;
    codecEvt->bisInfo[1].codecCfg.duration             = 1;
    codecEvt->bisInfo[1].codecCfg.allocation           = 0x02;
    codecEvt->bisInfo[1].codecCfg.frameOcts            = 100;
    codecEvt->bisInfo[1].codecCfg.codecFrameBlksPerSDU = 1;
    codecEvt->bisInfo[1].metadata                      = NULL;
    app_codec_setBigInformation(codecEvt);

    app_audio_updatePaSyncState(PA_SYNC_STATE_RECV_BASE, pEvt->syncHandle);
    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink LE BIG information report.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_biginfoAdvReport(u8 *p, int n)
{
    if (allState.paState != PA_SYNC_STATE_RECV_BASE) {
        return; //need receive complete BASE value.
    }

    if (allState.bigState != BIG_SYNC_STATE_IDLE) {
        return;
    }

    hci_le_bigInfoAdvReportEvt_t *pEvt = (hci_le_bigInfoAdvReportEvt_t *)p;

    u8 bigSyncParamBuf[sizeof(hci_le_bigCreateSyncParams_t) + 32] = {0};

    hci_le_bigCreateSyncParams_t *pBigCreateSyncParam = (hci_le_bigCreateSyncParams_t *)bigSyncParamBuf;

    pBigCreateSyncParam->big_handle  = BIG_HANDLE_0;     /* Used to identify the BIG */
    pBigCreateSyncParam->sync_handle = pEvt->syncHandle; /* Identifier of the periodic advertising train */
    pBigCreateSyncParam->enc         = pEvt->enc;        /* Encryption flag */

    if (pEvt->enc) {
        strncpy((char *)pBigCreateSyncParam->broadcast_code, DEFAULT_BROADCAST_CODE, 16);
    } else {
        memset(pBigCreateSyncParam->broadcast_code, 0, 16);
    }

    pBigCreateSyncParam->mse              = pEvt->nse;                         /* The Controller can schedule reception of any number of subevents up to NSE */
    pBigCreateSyncParam->big_sync_timeout = 10 * pEvt->IsoItvl * 1250 / 10000; /* Synchronization timeout for the BIG */

    tlkapi_printf(APP_LOG_EN, "start create big sync, NSE is %d, BIS number is %d, Encrypt state is %s", pEvt->nse, pEvt->numBis, pEvt->enc ? "Encrypted" : "unencrypted");

    pBigCreateSyncParam->num_bis = pEvt->numBis;

    for (int i = 0; i < pEvt->numBis; i++) {
        pBigCreateSyncParam->bis[i] = i + 1;
    }
    ble_sts_t state = blc_hci_le_bigCreateSync(pBigCreateSyncParam);

    tlkapi_printf(APP_LOG_EN, "create BIG Sync state is %02x", state);

    if (state == BLE_SUCCESS) {
        app_audio_updateBigSyncState(BIG_SYNC_STATE_CREATE);
    } else {
        app_audio_updateBigSyncState(BIG_SYNC_STATE_FAILED);
    }
    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink LE BIG sync established.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_bigSyncEst(u8 *p, int n)
{
    hci_le_bigSyncEstablishedEvt_t *pEvt = (hci_le_bigSyncEstablishedEvt_t *)p;

    tlkapi_printf(APP_LOG_EN, "BIG sync established status: %02X", pEvt->status);

    if (pEvt->status == BLE_SUCCESS) {
        app_codec_setBigSyncState(BIG_SYNCED, pEvt->numBis, pEvt->bisHandles);
        app_audio_updateBigSyncState(BIG_SYNC_STATE_EST);

        for (int i = 0; i < pEvt->numBis; i++) {
            blc_ll_setupIsoDataPath(pEvt->bisHandles[i], Data_Dir_Output, Data_Path_HCI, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        }
    } else {
        app_audio_updateBigSyncState(BIG_SYNC_STATE_FAILED);
    }

    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink LE terminate BIG complete.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_terminateBigComplete(u8 *p, int n)
{
    hci_le_terminateBigCompleteEvt_t *pEvt = (hci_le_terminateBigCompleteEvt_t *)p;
    tlkapi_printf(APP_LOG_EN, "BIG terminate complete, reason is 0x%x", pEvt->reason);
    app_audio_updateBigSyncState(BIG_SYNC_STATE_FAILED);
    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink LE BIG sync lost.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_bigSyncLost(u8 *p, int n)
{
    tlkapi_printf(APP_LOG_EN, "BIG sync lost");

    app_audio_updateBigSyncState(BIG_SYNC_STATE_FAILED);

    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink LE periodic advertise sync lost.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static void app_audio_periodicAdvSyncLost(u8 *p, int n)
{
    tlkapi_printf(APP_LOG_EN, "Periodic advertise sync lost");

    app_audio_updatePaSyncState(PA_SYNC_STATE_FAILED, 0x0000);

    app_audio_stateChangeProcess();
}

static const app_audio_controllerEvtCb_t sinkCb[] = {
    {HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT,           app_audio_leExtendAdvReport   },
    {HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED, app_audio_periodicAdvSyncEst  },
    {HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT,           app_audio_periodicAdvReport   },
    {HCI_SUB_EVT_LE_BIGINFO_ADVERTISING_REPORT,            app_audio_biginfoAdvReport    },
    {HCI_SUB_EVT_LE_BIG_SYNC_ESTABLISHED,                  app_audio_bigSyncEst          },
    {HCI_SUB_EVT_LE_TERMINATE_BIG_COMPLETE,                app_audio_terminateBigComplete},
    {HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST,        app_audio_periodicAdvSyncLost },
    {HCI_SUB_EVT_LE_BIG_SYNC_LOST,                         app_audio_bigSyncLost         },
};

/**
 * @brief      BLE controller event handler call-back in audio.
 * @param[in]  h       event type
 * @param[in]  p       Pointer point to event parameter buffer.
 * @param[in]  n       the length of event parameter.
 * @return
 */
int app_audio_controllerEventCallBack(u32 h, u8 *p, int n)
{
    if ((h & HCI_FLAG_EVENT_BT_STD) && ((h & 0xff) == HCI_EVT_LE_META)) {
        u8 subEvt_code = p[0];
        for (int i = 0; i < ARRAY_SIZE(sinkCb); i++) {
            if (sinkCb[i].evtCode == subEvt_code) {
                sinkCb[i].evtCb(p, n);
                break;
            }
        }
    }
    return 0;
}

/**
 * @brief       select next or previous Broadcast Source.
 * @param[in]   up: true mean select next source, false mean select previous source.
 * @return      none.
 */
void app_audio_selectBroadcastSource(bool up)
{
    telink_bcast_source_t *last = nextSource_Idx ? nextSource_Idx : currectSource_Idx;

    if (!last) {
        // Start from the first device
        for (size_t i = 0; i < ARRAY_SIZE(telinkBcastSources); i++) {
            if (telinkBcastSources[i].used) {
                nextSource_Idx = &telinkBcastSources[i];
                break;
            }
        }
    } else {
        bool found = false;
        if (up) {
            for (int i = 0; i < ARRAY_SIZE(telinkBcastSources); i++) {
                if (found && telinkBcastSources[i].used) {
                    nextSource_Idx = &telinkBcastSources[i];
                    goto done;
                }

                if (&telinkBcastSources[i] == last) {
                    found = true;
                }
            }

            for (int i = 0; i < ARRAY_SIZE(telinkBcastSources); i++) {
                if (telinkBcastSources[i].used) {
                    nextSource_Idx = &telinkBcastSources[i];
                    goto done;
                }
            }
        } else {
            for (int i = ARRAY_SIZE(telinkBcastSources) - 1; i >= 0; i--) {
                if (found && telinkBcastSources[i].used) {
                    nextSource_Idx = &telinkBcastSources[i];
                    goto done;
                }

                if (&telinkBcastSources[i] == last) {
                    found = true;
                }
            }

            for (int i = ARRAY_SIZE(telinkBcastSources) - 1; i >= 0; i--) {
                if (telinkBcastSources[i].used) {
                    nextSource_Idx = &telinkBcastSources[i];
                    goto done;
                }
            }
        }
    }

done:
    if (!nextSource_Idx) {
        tlkapi_printf(APP_LOG_EN, "No source cached");
        return;
    }

    tlkapi_printf(APP_LOG_EN, "[APP]Selected source[%d] Address is %s", (nextSource_Idx - telinkBcastSources) / sizeof(*telinkBcastSources), addr_to_str(nextSource_Idx->addr));
    app_audio_stateChangeProcess();
}

/**
 * @brief       Broadcast sink audio main loop.
 * @param[in]   none.
 * @return      none.
 */
void app_audio_handler(void)
{
    app_audio_receiveHandler();
}

#endif //SINK_VERSION == SINK_WITH_ASSISTANT_VERSION
