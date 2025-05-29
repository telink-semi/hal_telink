#include "../intest_config.h"
#if (INTER_TEST_MODE == TEST_ULL_HID_DEVICE)


    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_ull_hid.h"
    #include "app_parse_char.h"
    #include "stack/ble/host/gatt/tlk_timer_stack.h"
    #include "app.h"

static int suppInterval[] = {1000, 1250, 2000, 2500, 3000, 3750, 4000, 5000};

static void app_ullhid_select_hybrid_mode_cb(u16 connHandle, u16 scid)
{
    app_parse_printf("indicate select hybrid mode finish\r\n");
}

static void app_ullhid_select_hybrid_mode(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return;
    }

    if (argc != 3) {
        app_parse_printf("must input, selectHybridMode [report-interval] [NSE] [report index]\r\n");
        app_parse_printf("NSE is an invalid parameter.\r\n");
        return;
    }

    int reportInterval = app_parse_str2n(argv[0]);

    int i = 0;
    for (; i < sizeof(suppInterval) / sizeof(int); i++) {
        if (reportInterval == suppInterval[i]) {
            break;
        }
    }

    if (i == sizeof(suppInterval) / sizeof(int)) {
        app_parse_printf("please enter the correct parameters.\r\n");
        return;
    }

    struct ullhid_select_hybrid_param param = {
        .CIG_ID                 = 0x00,
        .CIS_ID                 = 0x00,
        .suppInterval.intervals = BIT(blc_ullhid_convertReportInterval(reportInterval)),
        .indicesCnt             = 1,
        .indices[0]             = app_parse_str2n(argv[2]),
    };

    ble_sts_t state = blc_ullhids_indSelectHybridMode(app_ull_hid_get_acl_handle(), &param, app_ullhid_select_hybrid_mode_cb);

    app_parse_printf("send indicate select hybrid mode, connHandle:0x%x, result is %d\r\n", app_ull_hid_get_acl_handle(), state);
}

static void app_ullhid_select_default_mode_cb(u16 connHandle, u16 scid)
{
    app_parse_printf("indicate select default mode finish\r\n");
}

static void app_ullhid_select_default_mode(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() == 0 || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS connect\r\n");
        return;
    }

    ble_sts_t state = blc_ullhids_indSelectDefaultMode(app_ull_hid_get_acl_handle(), app_ullhid_select_default_mode_cb);

    app_parse_printf("send indicate select default mode, connHandle:0x%x, result is %d\r\n", app_ull_hid_get_acl_handle(), state);
}

extern void app_ullhid_report_data(char *argv[], int argc, void *user_data);

static const parse_fun_list_t ullHidParse[] = {
    {"selectHybridMode",  app_ullhid_select_hybrid_mode },
    {"selectDefaultMode", app_ullhid_select_default_mode},
    {"reportData",        app_ullhid_report_data        },
};

void blc_app_ull_ui_init(void)
{
    app_parse_init(ullHidParse, ARRAY_SIZE(ullHidParse));
    app_parse_printf("ULL HID Device initial.\r\n");
}

#endif
