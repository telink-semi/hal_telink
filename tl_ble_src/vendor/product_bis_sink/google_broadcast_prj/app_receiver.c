/********************************************************************************************************
 * @file    app_receiver.c
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

    #include "app_buffer.h"
    #include "app.h"
    #include "app_uart.h"

typedef struct
{
    u16 pdaSyncHandle;
    u8  bisState;
    u8  bigHandle;
    u8  bisHandle;
} receiver_state_t;

receiver_state_t appState;

u8 *app_receiver_getAdvTypeInfo(u8 *pAdvDat, u32 len, data_type_t advType, u8 *outLen)
{
    u8  adLen = 0;
    u8 *p     = pAdvDat;

    while (len) {
        adLen = p[0];
        if (p[1] == advType) {
            *outLen = adLen - 1;
            return p + 2;
        }

        if (len > (adLen + 1)) {
            len -= (adLen + 1);
            p += (adLen + 1);
        } else {
            len = 0;
        }
    }
    return NULL;
}

u8 *app_receiver_getCompleteNameInfo(u8 *pAdvDat, u32 len, u8 *outLen)
{
    return app_receiver_getAdvTypeInfo(pAdvDat, len, DT_COMPLETE_LOCAL_NAME, outLen);
}

u8 *app_receiver_getBroadcastNameInfo(u8 *pAdvDat, u32 len, u8 *outLen)
{
    return app_receiver_getAdvTypeInfo(pAdvDat, len, DT_BROADCAST_NAME, outLen);
}

void app_receiver_closeScanNewTrans(void)
{
    blc_ll_setExtScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
}

void app_receiver_openScanNewTrans(void)
{
    blc_ll_periodicAdvertisingTerminateSync(appState.pdaSyncHandle);
    appState.pdaSyncHandle = 0x00;
    appState.bisHandle     = 0x00;
    appState.bisHandle     = 0x00;
    appState.bisState      = 0x00;
    blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);
}

void app_bis_receiver_advReportEvt(u8 *p)
{
    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int offset = 0;

    extAdvEvt_info_t *pExtAdv = NULL;

    for (int i = 0; i < pExtAdvRpt->num_reports; i++) {
        pExtAdv = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdv->data_length);

        u8  completeNameLen = 0, broadcastNameLen = 0;
        u8 *completeName = app_receiver_getCompleteNameInfo(pExtAdv->data, pExtAdv->data_length, &completeNameLen);

        u8 *broadcastName = app_receiver_getBroadcastNameInfo(pExtAdv->data, pExtAdv->data_length, &broadcastNameLen);

        if ((sizeof(FILTER_COMPLETE_NAME) - 1 == completeNameLen) &&
            (sizeof(FILTER_BROADCAST_NAME) - 1 == broadcastNameLen) &&
            (memcmp(completeName, FILTER_COMPLETE_NAME, completeNameLen) == 0) &&
            (memcmp(broadcastName, FILTER_BROADCAST_NAME, broadcastNameLen) == 0)) {
            APP_EVENT_LOG("    extend advertise found");

            u8 status = blc_ll_periodicAdvertisingCreateSync(SYNC_ADV_SPECIFY | REPORTING_INITIALLY_EN,
                                                             pExtAdv->advertising_sid,
                                                             pExtAdv->address_type,
                                                             pExtAdv->address,
                                                             0,
                                                             SYNC_TIMEOUT_2S,
                                                             0);
            if (status != BLE_SUCCESS) {
                APP_EVENT_LOG("PA sync create start -- Failed(status is %d)", status);
                break;
            } else {
                APP_EVENT_LOG("PA sync create start -- OK");
                app_receiver_closeScanNewTrans();
            }
        }
    }
}

void app_bis_receiver_periodicAdvSync(u8 *p)
{
    hci_le_periodicAdvSyncEstablishedEvt_t *pEvt = (hci_le_periodicAdvSyncEstablishedEvt_t *)p;

    APP_EVENT_LOG("PDA sync status is 0x%x", pEvt->status);
    if (pEvt->status == BLE_SUCCESS) {
        appState.pdaSyncHandle = pEvt->syncHandle;
    } else {
        app_receiver_openScanNewTrans();
    }
}

void app_bis_receiver_biginfoAdvReport(u8 *p)
{
    if (appState.bisState) {
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
        APP_EVENT_LOG("    BIG encryption");
    } else {
        memset(pBigCreateSyncParam->broadcast_code, 0, 16);
        APP_EVENT_LOG("    BIG no encryption");
    }

    pBigCreateSyncParam->mse              = pEvt->nse;                         /* The Controller can schedule reception of any number of subevents up to NSE */
    pBigCreateSyncParam->big_sync_timeout = 10 * pEvt->IsoItvl * 1250 / 10000; /* Synchronization timeout for the BIG */
    APP_EVENT_LOG("    BIS NSE[%d] numberOfBis[%d]", pEvt->nse, pEvt->numBis);

    pBigCreateSyncParam->bis[0]  = 1;
    pBigCreateSyncParam->num_bis = 1;

    ble_sts_t status = blc_hci_le_bigCreateSync(pBigCreateSyncParam);
    if (status != BLE_SUCCESS) {
        APP_EVENT_LOG("ERR: BIG create sync error 0x%x", status);
        app_receiver_openScanNewTrans();
    } else {
        appState.bisState = 0x01;
    }
}

void app_bis_receiver_bigSync(u8 *p)
{
    hci_le_bigSyncEstablishedEvt_t *pEvt = (hci_le_bigSyncEstablishedEvt_t *)p;

    APP_EVENT_LOG("BIG Sync, reason is 0x%x", pEvt->status);

    if (pEvt->status == BLE_SUCCESS) {
        appState.bigHandle = pEvt->bigHandle;
        for (int i = 0; i < pEvt->numBis; i++) {
            appState.bisHandle = pEvt->bisHandles[i];
            //BLT_BIS_SINK_LOG("set data path handle is 0x%x, index = %d", pEvt->bisHandles[i], i);
            blc_ll_setupIsoDataPath(
                pEvt->bisHandles[i],
                Data_Dir_Output,
                Data_Path_HCI,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0,
                0);
        }

        APP_EVENT_LOG("    BIG sync successfully");
    } else {
        app_receiver_openScanNewTrans();
    }
}

void app_bis_receiver_periodicAdvLost(u8 *p)
{
    APP_EVENT_LOG("PDA Sync Lost");
}

void app_bis_receiver_bigSyncLost(u8 *p)
{
    hci_le_bigSyncLostEvt_t *pEvt = (hci_le_bigSyncLostEvt_t *)p;
    app_receiver_openScanNewTrans();
    APP_EVENT_LOG("BIG Sync Lost, reason is 0x%x", pEvt->reason);
}

void app_bis_receiver_terminateBigComplete(u8 *p)
{
    hci_le_terminateBigCompleteEvt_t *pEvt = (hci_le_terminateBigCompleteEvt_t *)p;

    APP_EVENT_LOG("BIG terminate complete, reason is 0x%x", pEvt->reason);
    app_receiver_openScanNewTrans();
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
        if (evtCode == HCI_EVT_LE_META) {
            u8 subEvt_code = p[0];

            if (subEvt_code != HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) {
                //              APP_EVENT_LOG("subEvt code is 0x%x", subEvt_code);
            }

            if (subEvt_code == HCI_SUB_EVT_LE_EXTENDED_ADVERTISING_REPORT) // Report Ext ADV packet
            {
                //Obtain AUX_SYNC_IND PDU, Step1: HCI_LE_Periodic_Advertising_Create_Sync
                app_bis_receiver_advReportEvt(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED) {
                //Step2: After receiving HCI_LE_Periodic_Advertising_Sync_Established Event(With Sync_Handle)
                app_bis_receiver_periodicAdvSync(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_REPORT) //PDA report
            {
                //Step3: After receiving HCI_LE_Periodic_Advertising_Report Event
            } else if (subEvt_code == HCI_SUB_EVT_LE_BIGINFO_ADVERTISING_REPORT) {
                //Setp4:
                app_bis_receiver_biginfoAdvReport(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_BIG_SYNC_ESTABLISHED) // create BIG complete
            {
                //Step5:big sync established successful
                app_bis_receiver_bigSync(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_PERIODIC_ADVERTISING_SYNC_LOST) {
                app_bis_receiver_periodicAdvLost(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_BIG_SYNC_LOST) {
                app_bis_receiver_bigSyncLost(p);
            } else if (subEvt_code == HCI_SUB_EVT_LE_TERMINATE_BIG_COMPLETE) {
                app_bis_receiver_terminateBigComplete(p);
            }
        }
    }

    return 0;
}

void app_bis_receiver_init(void)
{
    blc_hci_registerControllerEventHandler(app_controller_event_callback); //controller hci event to host all processed in this func


    blc_ll_setExtScanParam(OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_100MS, SCAN_INTERVAL_100MS, 0, 0, 0);

    blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);

    app_uart_init();
}

void app_bis_receiver_handler(void)
{
    if (appState.bisHandle) {
        sdu_packet_t *pPkt;
        pPkt = blc_ll_popBisSyncRxSduData(appState.bisHandle);
        if (pPkt != NULL && pPkt->iso_sdu_len) {
            tlkapi_send_string_data(APP_BIS_SYNC_VALUE_LOG_EN, "bis sync value is ", &pPkt->iso_sdu_len, 2);
            app_uart_send_value(pPkt->data, pPkt->iso_sdu_len);
        }
    }
}

#endif
