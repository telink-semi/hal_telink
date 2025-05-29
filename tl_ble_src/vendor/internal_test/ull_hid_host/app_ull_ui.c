#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_HOST)


    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_ull_hid.h"
    #include "app_ull_cis.h"
    #include "app_parse_char.h"
    #include "app.h"

struct device_info
{
    u8 addressType;
    u8 address[6];
    u8 name[32];
};

    #define MAX_SCAN_HID_DEVICE_NUM 10

static struct device_info g_device[MAX_SCAN_HID_DEVICE_NUM] = {0};
static u8                 g_deviceNum                       = 0;
static u8                 g_scanEn                          = 0;

void app_uiihid_receive_device(u8 addressType, u8 address[6], u8 *name, u16 nameLen)
{
    if (g_deviceNum == MAX_SCAN_HID_DEVICE_NUM || g_scanEn == 0) {
        return;
    }

    for (int i = 0; i < g_deviceNum; i++) {
        if (g_device[i].addressType == addressType && memcmp(g_device[i].address, address, 6) == 0) {
            return;
        }
    }
    g_device[g_deviceNum].addressType = addressType;
    memcpy(g_device[g_deviceNum].address, address, 6);
    memcpy(g_device[g_deviceNum].name, name, nameLen);
    app_parse_printf("[%d]:Address[%s]:%s, name:%s\r\n", g_deviceNum + 1, addressType ? "random" : "public", addr_to_str(address), g_device[g_deviceNum].name);
    g_deviceNum++;
}

static void app_ullhid_scan_device(char *argv[], int argc, void *user_data)
{
    memset(g_device, 0, sizeof(g_device));
    g_deviceNum = 0;
    g_scanEn    = 1;
    app_parse_printf("start/restart scan hid device\r\n");
}

static const char *temp[] = {"NoSupp", "Supp  "};

static void app_ull_display_remote_ull_hid(void)
{
    struct blc_ullhid_properties_format s_properties;
    u16                                 propertiesLen;
    int                                 res = blc_ullhidc_getUllhidProperties(app_ull_hid_get_acl_handle(), &s_properties, &propertiesLen);
    if (res != PRF_COMMON_SUCC) {
        app_parse_printf(" connected peripheral not supported ULL-HID Service.\n");
    } else {
        struct blc_ullhid_properties_format *properties = &s_properties;
        app_parse_printf("Hybrid Mode ULL Reports information\n");
        app_parse_printf("Features: \n\t Device Mode Change:%s\n", temp[properties->deviceModeChange]);
        app_parse_printf("Supported Report Interval: \n\t 1ms:%s, 2ms:%s, 3ms:%s, 4ms:%s, \n\t 5ms:%s, 1.25ms:%s, 2.5ms:%s, 3.75ms:%s\n",
                         temp[properties->suppInterval.interval_1ms],
                         temp[properties->suppInterval.interval_2ms],
                         temp[properties->suppInterval.interval_3ms],
                         temp[properties->suppInterval.interval_4ms],
                         temp[properties->suppInterval.interval_5ms],
                         temp[properties->suppInterval.interval_1_25ms],
                         temp[properties->suppInterval.interval_2_5ms],
                         temp[properties->suppInterval.interval_3_75ms]);

        app_parse_printf("max SDU size , Device to Host:%d, Host to Device:%d\n", properties->deviceToHostMaxSduSize, properties->hostToDeviceMaxSduSize);
        ullhidParam.cisSduS2M = properties->deviceToHostMaxSduSize;
        ullhidParam.cisSduM2S = properties->hostToDeviceMaxSduSize;

        for (int i = 0; i < ((propertiesLen - ULLHID_PROPERTIES_HEAD_SIZE) >> 1); i++) {
            app_parse_printf("[%d] ", i);
            app_parse_printf("Report ID:%02d, ", properties->hybridModeReport[i].reportID);
            app_parse_printf("Type:%s,", properties->hybridModeReport[i].reportType == 0x00 ? "Input " : "Output");
            app_parse_printf("PowerSavingConfirmation:%s,", temp[properties->hybridModeReport[i].powerSavingCfm]);
            app_parse_printf("Repetition:%s. \n", temp[properties->hybridModeReport[i].repetition]);
        }
    }
}

static void app_ullhid_show_device(char *argv[], int argc, void *user_data)
{
    if (g_deviceNum == 0) {
        app_parse_printf("no scan device.\r\n");
    } else {
        if (app_ull_hid_get_acl_handle()) {
            app_ull_display_remote_ull_hid();
        } else {
            for (int i = 0; i < g_deviceNum; i++) {
                app_parse_printf("[%d]:Address[%s]:%s, name:%s\r\n", i + 1, g_device[i].addressType ? "random" : "public", addr_to_str(g_device[i].address), g_device[i].name);
            }
        }
    }
}

static void app_ullhid_conn_device(char *argv[], int argc, void *user_data)
{
    if (argc != 1 || g_deviceNum == 0) {
        app_parse_printf("must input, conn-device [index]\r\n");
        app_parse_printf("you can input, show-device to Query index.\r\n");
        return;
    }

    int index = app_parse_str2n(argv[0]);
    if (index > g_deviceNum || index <= 0) {
        app_parse_printf("index range must is 1 to %d", g_deviceNum);
        return;
    }

    g_scanEn  = 0;
    u8 status = blc_ll_extended_createConnection(INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, g_device[index - 1].addressType, g_device[index - 1].address, INIT_PHY_1M, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_62P5MS, CONN_INTERVAL_80MS, CONN_TIMEOUT_4S, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

    app_parse_printf("send create connection request, address:%s\r\n", addr_to_str(g_device[index - 1].address));

    if (status == BLE_SUCCESS) { //create connection success
        app_parse_printf("Create connection success\r\n");
    } else {
        app_parse_printf("Create connection fail, status is %d\r\n", status);
    }
}

static void app_ullhid_disconnect_device(char *argv[], int argc, void *user_data)
{
    u16 connHandle = app_ull_hid_get_acl_handle();

    if (connHandle == 0) {
        app_parse_printf("No ACL connected.\r\n");
    } else {
        ble_sts_t sts = blc_ll_disconnect(connHandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        app_parse_printf("send ACL disconnect command, status is %d.\r\n", sts);
    }
}

static void app_ullhid_selectHybridModeCb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err == ATT_SUCCESS) {
        app_ullhid_initCigParam();
    }
}

struct ull_param_check suppParam[] = {
    {1000, 5 },
    {1000, 10},
    {1000, 15},
    {1250, 4 },
    {1250, 8 },
    {1250, 12},
    {2000, 5 },
    {2000, 10},
    {2500, 4 },
    {2500, 8 },
    {2500, 12},
    {3000, 5 },
    {3750, 4 },
    {4000, 5 },
    {5000, 1 },
    {5000, 2 },
    {5000, 4 },
};

const int suppParamLen = ARRAY_SIZE(suppParam);

static void app_ullhid_selectHybridMode(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_acl_handle() == 0 || app_ull_hid_get_cis_handle()) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return;
    }

    if (argc != 3) {
        app_parse_printf("must input, selectHybridMode [report-interval] [NSE] [report index]\r\n");
        return;
    }

    int reportInterval = app_parse_str2n(argv[0]);
    int NSE            = app_parse_str2n(argv[1]);

    int i = 0;
    for (; i < ARRAY_SIZE(suppParam); i++) {
        if (reportInterval == suppParam[i].reportInterval && NSE == suppParam[i].NSE) {
            break;
        }
    }

    if (i == ARRAY_SIZE(suppParam)) {
        app_parse_printf("please enter the correct parameters.\r\n");
        return;
    }

    ullhidParam.reportInterval = reportInterval;
    ullhidParam.nse            = NSE;

    struct ullhid_select_hybrid_param param = {
        .CIG_ID                 = 0x00,
        .CIS_ID                 = 0x00,
        .suppInterval.intervals = BIT(blc_ullhid_convertReportInterval(ullhidParam.reportInterval)),
        .indicesCnt             = 1,
        .indices[0]             = app_parse_str2n(argv[2]),
    };

    struct blc_ullhid_properties_format s_properties;
    u16                                 propertiesLen;
    blc_ullhidc_getUllhidProperties(app_ull_hid_get_acl_handle(), &s_properties, &propertiesLen);

    ullhidParam.reportID       = s_properties.hybridModeReport[param.indices[0]].reportID;
    ullhidParam.reportType     = s_properties.hybridModeReport[param.indices[0]].reportType;
    ullhidParam.powerSavingCfm = s_properties.hybridModeReport[param.indices[0]].powerSavingCfm;
    ullhidParam.repetition     = s_properties.hybridModeReport[param.indices[0]].repetition;

    int err = blc_ullhidc_writeSelectHybridMode(app_ull_hid_get_acl_handle(), &param, app_ullhid_selectHybridModeCb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static void app_ullhid_selectDefaultModeCb(u16 connHandle, att_err_t err)
{
    app_parse_printf("select default mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    u8 status = blc_ll_cis_disconnect(app_ull_hid_get_cis_handle(), HCI_ERR_CONN_TERM_BY_LOCAL_HOST);
    app_parse_printf("send CIS disconnect command %s\r\n", status == BLE_SUCCESS ? "successful" : "failed");
}

static void app_ullhid_selectDefaultMode(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() == 0x00 || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS connect\r\n");
        return;
    }

    int err = blc_ullhidc_writeSelectDefaultMode(app_ull_hid_get_acl_handle(), app_ullhid_selectDefaultModeCb);
    app_parse_printf("write select default mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static void app_ullhid_set_sdu(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected\r\n");
        return;
    }

    if (argc != 2) {
        app_parse_printf("should input set-SDU [D2S] [S2D] \r\n");
        return;
    }

    ullhidParam.cisSduS2M = app_parse_str2n(argv[0]);
    ullhidParam.cisSduM2S = app_parse_str2n(argv[1]);

    app_parse_printf("device to host SDU is %d, host to device SDU is %d\r\n", app_parse_str2n(argv[0]), app_parse_str2n(argv[1]));
}

extern void app_ullhid_report_data(char *argv[], int argc, void *user_data);

extern void app_ullhid_hd_hguom_bi_01_c(char *argv[], int argc, void *user_data);
extern void app_ullhid_hd_hguom_bi_02_c(char *argv[], int argc, void *user_data);
extern void app_ullhid_hd_hguom_bi_03_c(char *argv[], int argc, void *user_data);
extern void app_ullhid_hd_hguom_bi_04_c(char *argv[], int argc, void *user_data);

void app_ullhid_update_phy(char *argv[], int argc, void *user_data)
{
    blc_ll_setPhy(app_ull_hid_get_acl_handle(), PHY_TRX_PREFER, PHY_PREFER_2M, PHY_PREFER_2M, CODED_PHY_PREFER_NONE);
}

static const parse_fun_list_t ullHidParse[] = {
    {"scan-device",       app_ullhid_scan_device      },
    {"show-device",       app_ullhid_show_device      },
    {"conn-device",       app_ullhid_conn_device      },
    {"dis-device",        app_ullhid_disconnect_device},
    {"selectHybridMode",  app_ullhid_selectHybridMode },
    {"selectDefaultMode", app_ullhid_selectDefaultMode},
    {"set-SDU",           app_ullhid_set_sdu          },
    {"reportData",        app_ullhid_report_data      },
    {"HD-HGUOM-BI-01-C",  app_ullhid_hd_hguom_bi_01_c },
    {"HD-HGUOM-BI-02-C",  app_ullhid_hd_hguom_bi_02_c },
    {"HD-HGUOM-BI-03-C",  app_ullhid_hd_hguom_bi_03_c },
    {"HD-HGUOM-BI-04-C",  app_ullhid_hd_hguom_bi_04_c },
    {"PHY-Update",        app_ullhid_update_phy       },
};

static int app_ullhid_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen);
#define HOST_MALLOC_BUFF_SIZE      (4 * 1024)

static u8 hostMallocBuffer[HOST_MALLOC_BUFF_SIZE];
void blc_app_ull_ui_init(void)
{
    blc_prf_initialModule(app_prf_eventCb,hostMallocBuffer,HOST_MALLOC_BUFF_SIZE);

    app_parse_init(ullHidParse, ARRAY_SIZE(ullHidParse));
    app_parse_printf("ULL HID Host initial.\r\n");
}

/*****************************ULL HID Host Profile event**********************************/

static int app_ullhid_recvInputReportData(u16 connHandle, u8 *pData, u16 dataLen)
{
    struct blc_hidc_recvInputReportData *pEvt = (struct blc_hidc_recvInputReportData *)pData;
    ULL_HID_LOG("receive input report data event, connHandle:0x%x, report id:%d, value is %s", connHandle, pEvt->reportId, hex_to_str(pEvt->value, pEvt->len));

    if (pEvt->reportId == HID_REPORT_ID_MOUSE_INPUT) {
    #if (USB_MOUSE_ENABLE)
        extern void usbmouse_add_frame(mouse_data_t * packet_mouse, int packet_num);
        usbmouse_add_frame((mouse_data_t *)pEvt->value, 1);
    #else
        ULL_HID_LOG("receive mouse data by GATT, value is %s", hex_to_str(pEvt->value, pEvt->len));
    #endif
    } else if (pEvt->reportId == HID_REPORT_ID_KEYBOARD_INPUT) {
    #if (USB_KEYBOARD_ENABLE)
        kb_data_t data = {
            .ctrl_key   = 0,
            .cnt        = 1,
            .keycode[0] = pEvt->value[2],
        };
        usbkb_hid_report(&data);
    #else
        ULL_HID_LOG("receive keyboard data by GATT, value is %s", hex_to_str(pEvt->value, pEvt->len));
    #endif
    }


    return 0;
}

static int app_ullhid_prfSdpOver(u16 connHandle, u8 *pData, u16 dataLen)
{
    app_parse_printf("ACL connected complete.\r\n");
    app_ull_display_remote_ull_hid();
    return 0;
}

static int app_ullhid_selectHybridModeEvt(u16 connHandle, u8 *pData, u16 dataLen)
{
    struct ullhidc_selectHybridModeEvt *pEvt = (struct ullhidc_selectHybridModeEvt *)pData;
    app_parse_printf("receive select hybrid mode, report interval is %dus\r\n", pEvt->reportInterval

    );
    app_parse_printf("Report ID:%d, ", pEvt->reportInfo[0].reportID);
    app_parse_printf("Type:%s,", pEvt->reportInfo[0].reportType == 0x00 ? "Input" : "Output");
    app_parse_printf("Power Saving Confirmation:%s,", temp[pEvt->reportInfo[0].powerSavingCfm]);
    app_parse_printf("Repetition:%s. \n", temp[pEvt->reportInfo[0].repetition]);

    if (app_ull_hid_get_cis_handle() || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return 0;
    }

    ullhidParam.reportInterval = pEvt->reportInterval;
    if (pEvt->reportInterval == 5000) {
        ullhidParam.nse = 1;
    } else if (pEvt->reportInterval == 1250 || pEvt->reportInterval == 2500 || pEvt->reportInterval == 3750) {
        ullhidParam.nse = 4;
    } else {
        ullhidParam.nse = 5;
    }

    struct ullhid_select_hybrid_param param = {
        .CIG_ID                 = 0x00,
        .CIS_ID                 = 0x00,
        .suppInterval.intervals = BIT(blc_ullhid_convertReportInterval(ullhidParam.reportInterval)),
        .indicesCnt             = 1,
        .indices[0]             = pEvt->reportIndex[0],
    };

    ullhidParam.reportID       = pEvt->reportInfo[0].reportID;
    ullhidParam.reportType     = pEvt->reportInfo[0].reportType;
    ullhidParam.powerSavingCfm = pEvt->reportInfo[0].powerSavingCfm;
    ullhidParam.repetition     = pEvt->reportInfo[0].repetition;

    int err = blc_ullhidc_writeSelectHybridMode(app_ull_hid_get_acl_handle(), &param, app_ullhid_selectHybridModeCb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);

    return 0;
}

static int app_ullhid_selectDefaultModeEvt(u16 connHandle, u8 *pData, u16 dataLen)
{
    app_parse_printf("receive select default mode\r\n");

    if (app_ull_hid_get_cis_handle() == 0 || app_ull_hid_get_acl_handle() == 0x00) {
        return 0;
    }

    int err = blc_ullhidc_writeSelectDefaultMode(app_ull_hid_get_acl_handle(), app_ullhid_selectDefaultModeCb);
    app_parse_printf("write select default mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);

    return 0;
}

static const app_prf_evtCb_t ullhidPrfCb[] = {
    {PRF_EVTID_CLIENT_ALL_SDP_OVER,   app_ullhid_prfSdpOver          },
    {HIDC_EVT_RECV_INPUT_REPORT_DATA, app_ullhid_recvInputReportData },
    {ULLHIDC_EVT_SELECT_HYBRID_MODE,  app_ullhid_selectHybridModeEvt },
    {ULLHIDC_EVT_SELECT_DEFAULT_MODE, app_ullhid_selectDefaultModeEvt},
};

/**
 * @brief       ULL-HID profile event callback function.
 * @param[in]   aclHandle: ACL connect handle.
 * @param[in]   evtID: audio event ID, refer audio_event_enum.
 * @param[in]   pData: Data carried by the event.
 * @param[in]   dataLen: data length.
 * @return      0/1.
 */
static int app_ullhid_prfEvtCb(u16 aclHandle, int evtID, u8 *pData, u16 dataLen)
{
    for (int i = 0; i < ARRAY_SIZE(ullhidPrfCb); i++) {
        if (ullhidPrfCb[i].evtId == evtID) {
            return ullhidPrfCb[i].evtCb(aclHandle, pData, dataLen);
        }
    }
    return 0;
}

#endif
