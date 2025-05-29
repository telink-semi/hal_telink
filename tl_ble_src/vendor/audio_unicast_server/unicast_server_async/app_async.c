/*
 * app_async.c
 *
 *  Created on: 2023.9.20
 *      Author: ADmin
 */
#include "tl_common.h"
#include "drivers.h"
#include "ext_driver/ext_audio.h"
#include "stack/ble/ble.h"
#include "app_config.h"
#if (UNICAST_SERVER_SELECT == UNICAST_SERVER_ASYNC)
    #include "app_async.h"
    #include "app_audio.h"
    #include "app_audio_ctrl.h"
    #include "app_led.h"
    #include "app_key.h"
    #include "app.h"

struct adv_manufacturer
{
    blc_adv_ltv_t ltv;
    u16           company_id;
    u8            configuration;
} manDataField = {
    .ltv.len       = 0x04,
    .ltv.type      = DT_MANUFACTURER_SPECIFIC_DATA,
    .company_id    = 0x0211,
    .configuration = 0xAA,
};

struct adv_dev_name
{
    blc_adv_ltv_t ltv;
    u8            devName[sizeof("async_lea")];
} advDevName = {
    .ltv.len  = sizeof("async_lea"),
    .ltv.type = DT_COMPLETE_LOCAL_NAME,
    .devName  = "async_lea",
};

extern app_csis_Rsi_t   advCsisRsi;
extern app_audio_ctrl_t appCtrl;

u8               appAsyncState;
u32              appAsyncBootTick = 0;
app_async_node_t appAsyncNode;

    #define APP_ASYNC_BOOT_TIME 30

int app_async_tx_cb(u32 syncTick, blc_async_message_t *pMessage);
int app_async_rx_cb(u32 syncTick, blc_async_message_t *pMessage);

void app_async_init(void)
{
    if (appCtrl.leaRole == ACL_ROLE_PERIPHERAL) {
        blc_adv_ltv_t *adv_ltvs1[] = {
            (blc_adv_ltv_t *)&advCsisRsi,
            (blc_adv_ltv_t *)&advDevName,
            (blc_adv_ltv_t *)&manDataField,
        };
        u8 advData[255];
        u8 advDataLen = blc_adv_buildAdvData(adv_ltvs1, ARRAY_SIZE(adv_ltvs1), advData);
        //Legacy ADV initialization
        blc_ll_setExtAdvParam(ADV_HANDLE1, ADV_EVT_PROP_EXTENDED_CONNECTABLE_UNDIRECTED, ADV_INTERVAL_30MS, ADV_INTERVAL_30MS, BLT_ENABLE_ADV_ALL, OWN_ADDRESS_PUBLIC, BLE_ADDR_PUBLIC, NULL, ADV_FP_NONE, TX_POWER_3dBm, BLE_PHY_1M, 0, BLE_PHY_1M, ADV_SID_0, 0);
        blc_ll_setExtAdvData(ADV_HANDLE1, advDataLen, advData);
        blc_ll_setExtAdvEnable(BLC_ADV_ENABLE, ADV_HANDLE1, 0, 0);
        extern void blc_ll_asyncSetPrivateAdvChannel();
        blc_ll_asyncSetPrivateAdvChannel(ADV_HANDLE1, 6, 15, 31);
        tlkapi_printf(APP_LOG_EN, "[APP][LEA] ADV Init");

    } else if (appCtrl.leaRole == ACL_ROLE_CENTRAL) {
        //Legacy SCAN initialization
        blc_ll_initExtendedScanning_module();
        blc_ll_initExtendedInitiating_module();
        blc_ll_setAclCentralBaseConnectionInterval(CONN_INTERVAL_10MS);
        extern void blc_ll_asyncSetPrivateScanChannel();
        blc_ll_asyncSetPrivateScanChannel(6, 15, 31);
        blc_ll_setExtScanParam(OWN_ADDRESS_PUBLIC, SCAN_FP_ALLOW_ADV_ANY, SCAN_PHY_1M_CODED, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_WINDOW_90MS, SCAN_TYPE_PASSIVE, SCAN_INTERVAL_90MS, SCAN_WINDOW_90MS);

        blc_ll_setExtScanEnable(BLC_SCAN_ENABLE, DUPE_FLTR_DISABLE, SCAN_DURATION_CONTINUOUS, SCAN_WINDOW_CONTINUOUS);

        tlkapi_printf(APP_LOG_EN, "[APP][LEA] Scan Init");
    } else {
        tlkapi_printf(APP_LOG_EN, "[APP][LEA]###error lea role###");
    }
    blc_async_registerDataHandler(app_async_tx_cb, app_async_rx_cb);
    ble_audio_timer_init(TIMER1);
    appAsyncBootTick = clock_time() | 1;
}

/**
 * @brief       BLE controller event LE extend advertising report callback.
 * @param[in]   p       Pointer point to event parameter buffer.
 * @param[in]   n       the length of event parameter.
 * @return      none.
 */
static __attribute__((unused)) bool is_telink_async_adv(u8 *configuration, u8 *advData, u32 len)
{
    u8 adLen;

    u8 *buff = blc_adv_getManufacturerDataInformationByCompanyId(advData, len, VENDOR_ID, &adLen);

    if (buff && adLen == 1) {
        *configuration = *buff;
        return true;
    }

    return false;
}

/**
 * @brief      BLE Adv report event handler
 * @param[in]  p         Pointer point to event parameter buffer.
 * @return
 */
int app_le_ext_adv_report_event_handle(u8 *p)
{
    if (appCtrl.leaRole != ACL_ROLE_CENTRAL) {
        return 0;
    }

    hci_le_extAdvReportEvt_t *pExtAdvRpt = (hci_le_extAdvReportEvt_t *)p;

    int               offset      = 0;
    extAdvEvt_info_t *pExtAdvInfo = NULL;

    //  tlkapi_send_string_data(1, "[APP][LEA] Adv report", 0, 0);

    for (int i = 0; i < pExtAdvRpt->num_reports; i++) {
        pExtAdvInfo = (extAdvEvt_info_t *)(pExtAdvRpt->advEvtInfo + offset);
        offset += (EXTADV_INFO_LENGTH + pExtAdvInfo->data_length);
        //      tlkapi_send_string_data(1, "ADV", pExtAdvInfo->data, pExtAdvInfo->data_length);
        u8 connIndex = 0;
        s8 rssi      = pExtAdvInfo->rssi;
        (void)rssi; //unused, remove warning
        u8 ext_evtType = pExtAdvInfo->event_type & EXTADV_RPT_EVTTYPE_MASK;
        if (ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_UNDIRECTED || ext_evtType == EXTADV_RPT_EVTTYPE_EXT_CONNECTABLE_DIRECTED) {
            u8               index = 0;
            app_advdata_LTV *adv_data;
            while (index < pExtAdvInfo->data_length) {
                adv_data = (app_advdata_LTV *)(&pExtAdvInfo->data[0] + index);
                if (adv_data->type == DT_CSIP_RSI) {
                    if (blc_csis_resolveRSI(appCtrl.sirkCfg, adv_data->data)) {
                        connIndex = 1;
                        tlkapi_send_string_data(1, "rsi resolved", 0, 0);
                    }
                }
                //              tlkapi_printf(APP_LOG_EN,"adv len:0x%x\n", adv_data->length);
                //              tlkapi_printf(APP_LOG_EN,"adv type:0x%x\n", adv_data->type);
                //              BLT_APP_STR_LOG("[APP]adv para\n", adv_data->data, adv_data->length-1);
                index += (adv_data->length + 1);
            }
        }
        if (connIndex) {
            u32 status = blc_ll_extended_createConnection(INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_47P5MS, CONN_INTERVAL_50MS, CONN_TIMEOUT_1S, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_47P5MS, CONN_INTERVAL_50MS, CONN_TIMEOUT_1S, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_47P5MS, CONN_INTERVAL_50MS, CONN_TIMEOUT_1S);
            if (status == BLE_SUCCESS) { //create connection success
                tlkapi_send_string_data(APP_LOG_EN, "[APP][LEA] create connection success", 0, 0);
            } else {
                tlkapi_send_string_data(APP_LOG_EN, "[APP][LEA] create connection fail", (u8 *)&status, 4);
            }
        }
    }
    return 0;
}

void app_async_switchExecute(void)
{
    #ifdef __BOOT_SWITCH_APP1__
    analog_write_reg8(0x3c, 0x02);
    start_reboot();
    #endif
}

void app_key_switch_boot(void)
{
    if (appAsyncState == APP_ASYNC_STATE_CONNECT) {
        blc_async_message_t message;
        message.type   = TYPE_SYNC;
        message.opcode = OPCODE_SWITCH;
        blc_async_push_message(500 * 1000, &message);
        appAsyncBootTick = clock_time();
    } else {
        app_async_switchExecute();
    }
}

void app_tone_play(tlk_tone_type_e type)
{
    if (appAsyncState == APP_ASYNC_STATE_CONNECT) {
        blc_async_message_t message;
        message.type    = TYPE_SYNC;
        message.opcode  = OPCODE_TONE;
        message.data[0] = type;
        blc_async_push_message(500 * 1000, &message);
    } else {
        tlk_tone_play(type);
    }
}

int app_async_tx_cb(u32 syncTick, blc_async_message_t *pMessage)
{
    APP_DBG_CHN_14_HIGH;
    if (pMessage->type == TYPE_ASYNC) {
        switch (pMessage->opcode) {
        case OPCODE_LED:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] TX LED ASYNC");
            app_led_async(pMessage->data);
        } break;

        case OPCODE_TONE:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] TX TONE ASYNC");
        } break;

        case OPCODE_KEY:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] TX KEY ASYNC");
            app_key_async(pMessage->data);
        } break;

        case OPCODE_SWITCH:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] TX SWITCH ASYNC");
            app_async_switchExecute();
        } break;

        default:
            break;
        }
    } else if (pMessage->type == TYPE_SYNC) {
        appAsyncNode.syncTick = syncTick;
        memcpy(&appAsyncNode.message.type, &pMessage->type, sizeof(blc_async_message_t));
        u32 capture_tick_stimer = syncTick - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER1, 0, capture_tick_timer);
    }
    APP_DBG_CHN_14_LOW;

    return 0;
}

int app_async_rx_cb(u32 syncTick, blc_async_message_t *pMessage)
{
    APP_DBG_CHN_14_HIGH;
    if (pMessage->type == TYPE_ASYNC) {
        switch (pMessage->opcode) {
        case OPCODE_LED:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] RX LED ASYNC");
            app_led_async(pMessage->data);
        } break;

        case OPCODE_TONE:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] RX TONE ASYNC");
        } break;

        case OPCODE_KEY:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] RX KEY ASYNC");
            app_key_async(pMessage->data);
        } break;

        case OPCODE_SWITCH:
        {
            tlkapi_printf(APP_LOG_EN, "[APP][ASYNC] RX SWITCH ASYNC");
            app_async_switchExecute();
        } break;

        default:
            break;
        }
    } else if (pMessage->type == TYPE_SYNC) {
        appAsyncNode.syncTick = syncTick;
        memcpy(&appAsyncNode.message.type, &pMessage->type, sizeof(blc_async_message_t));
        u32 capture_tick_stimer = syncTick - clock_time();
        u32 capture_tick_timer  = (capture_tick_stimer * sys_clk.pclk) / SYSTEM_TIMER_TICK_1US;
        ble_audio_timer_set_capture(TIMER1, 0, capture_tick_timer);
    }
    APP_DBG_CHN_14_LOW;

    return 0;
}

void app_async_switchCheckProcess(void)
{
    if (appAsyncBootTick && clock_time_exceed(appAsyncBootTick, APP_ASYNC_BOOT_TIME * 1000 * 1000)) {
        if (appAsyncState == APP_ASYNC_STATE_CONNECT) {
            blc_async_message_t message;
            message.type   = TYPE_SYNC;
            message.opcode = OPCODE_SWITCH;
            blc_async_push_message(200 * 1000, &message);
            appAsyncBootTick = clock_time();
        } else {
            app_async_switchExecute();
        }
    }
}

void app_async_task(void)
{
    blc_async_loopProcess();
    app_async_switchCheckProcess();
}

void app_timer1_irq_proc(void)
{
    timer_stop(TIMER1);
    APP_DBG_CHN_15_HIGH;
    APP_DBG_CHN_15_LOW;
    tlkapi_printf(APP_LOG_EN, "### timer1 irq process ###");
    switch (appAsyncNode.message.opcode) {
    case OPCODE_LED:
    {
        tlkapi_printf(APP_LOG_EN, "[APP][TIMER] RX LED SYNC");
        app_led_sync(appAsyncNode.message.data);
    } break;

    case OPCODE_TONE:
    {
        tlkapi_printf(APP_LOG_EN, "[APP][TIMER] RX TONE SYNC");
        if (appAsyncNode.message.data[0] < TLK_TONE_MODE_MAX) {
            tlk_tone_play(appAsyncNode.message.data[0]);
        } else {
            tlkapi_printf(APP_LOG_EN, "[APP][TIMER] Tone Type exceed");
        }
    } break;

    case OPCODE_KEY:
    {
        tlkapi_printf(APP_LOG_EN, "[APP][TIMER] RX KEY SYNC");
        app_key_sync(appAsyncNode.message.data);
    } break;

    case OPCODE_SWITCH:
    {
        tlkapi_printf(APP_LOG_EN, "[APP][TIMER] RX SWITCH SYNC");
        app_async_switchExecute();
    } break;

    default:
        break;
    }
}

#endif
