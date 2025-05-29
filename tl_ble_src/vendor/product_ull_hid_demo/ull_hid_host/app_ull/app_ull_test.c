#include "../../ull_hid_config.h"
#if (ULL_HID_DEMO_SLECT == ULL_HID_HOST)


    #include "tl_common.h"
    #include "drivers.h"
    #include "stack/ble/ble.h"

    #include "app_ull_hid.h"
    #include "app_ull_cis.h"
    #include "../../app_parse/app_parse_char.h"
    #include "../app.h"
    #include "stack/ble/host/gatt/tlk_timer_stack.h"

    #include "stack/ble/profile/prf_hid/ullhid/ullhid_internal.h"

extern struct ull_param_check suppParam[];
extern const int              suppParamLen;

static struct soft_timer testTimer;

struct ullhid_select_hybrid_mode_format_test
{
    u8                                 opcode; //blt_ullhids_opcode_enum
    u8                                 CIG_ID;
    u8                                 CIS_ID;
    union ullhid_supp_report_intervals suppInterval;
    u8                                 indices[9];
} __attribute__((packed));

struct ullhid_select_hybrid_param_test
{
    u8                                 CIG_ID;
    u8                                 CIS_ID;
    union ullhid_supp_report_intervals suppInterval;
    u8                                 indicesCnt;
    u8                                 indices[9];
};

static void app_ullhid_getSelectHybridCmd(struct ullhid_select_hybrid_param *param)
{
    param->CIG_ID     = CIG_ID_0;
    param->CIS_ID     = CIS_ID_0;
    param->indicesCnt = 1;
    param->indices[0] = 0x00;

    struct blc_ullhid_properties_format s_properties;
    u16                                 propertiesLen;
    blc_ullhidc_getUllhidProperties(app_ull_hid_get_acl_handle(), &s_properties, &propertiesLen);

    for (int i = 7; i >= 0; i--) {
        if (s_properties.suppInterval.intervals & BIT(i)) {
            param->suppInterval.intervals = BIT(i);
            ullhidParam.reportInterval    = blc_ullhid_convertReportIntervalBit(param->suppInterval.intervals);
            for (; i < suppParamLen; i++) {
                if (ullhidParam.reportInterval == suppParam[i].reportInterval) {
                    ullhidParam.nse = suppParam[i].NSE;
                    break;
                }
            }
            return;
        }
    }
}

static void app_ullhid_hd_hguom_bi_01_c_cb_finish(u16 connHandle, att_err_t err)
{
    app_parse_printf("select default mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    u8 status = blc_ll_cis_disconnect(app_ull_hid_get_cis_handle(), HCI_ERR_CONN_TERM_BY_LOCAL_HOST);
    app_parse_printf("send CIS disconnect command %s\r\n", status == BLE_SUCCESS ? "successful" : "failed");

    app_parse_printf("HD-HGUOM-BI-01-C test successful.\r\n");
}

static void app_ullhid_hd_hguom_bi_01_c_cb_3(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_DEVICE_ALREADY_IN_REQUESTED_STATE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-01-C test error.\r\n");
    } else {
        int err = blc_ullhidc_writeSelectDefaultMode(app_ull_hid_get_acl_handle(), app_ullhid_hd_hguom_bi_01_c_cb_finish);
        app_parse_printf("send disable hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
    }
}

static int app_ullhid_hd_hguom_bi_01_c_timeout(void *arg)
{
    struct ullhid_select_hybrid_param param;
    app_ullhid_getSelectHybridCmd(&param);
    int err = blc_ullhidc_writeSelectHybridMode(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_01_c_cb_3);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
    return 0;
}

static void app_ullhid_hd_hguom_bi_01_c_cb_2(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err == ATT_SUCCESS) {
        app_ullhid_initCigParam();
        testTimer.cb    = app_ullhid_hd_hguom_bi_01_c_timeout;
        testTimer.timer = 1000;
        soft_timer_add(&testTimer);
    } else {
        app_parse_printf("HOGP/HD/HGUOM/BI-01-C test error.\r\n");
    }
}

static void app_ullhid_hd_hguom_bi_01_c_cb_1(u16 connHandle, att_err_t err)
{
    app_parse_printf("select default mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_DEVICE_ALREADY_IN_REQUESTED_STATE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-01-C test error.\r\n");
    } else {
        struct ullhid_select_hybrid_param param;
        app_ullhid_getSelectHybridCmd(&param);
        int err = blc_ullhidc_writeSelectHybridMode(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_01_c_cb_2);

        app_parse_printf("write select default mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
    }
}

void app_ullhid_hd_hguom_bi_01_c(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return;
    }

    int err = blc_ullhidc_writeSelectDefaultMode(app_ull_hid_get_acl_handle(), app_ullhid_hd_hguom_bi_01_c_cb_1);
    app_parse_printf("send write select default mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static u8 testOperation[32];

static void app_ullhid_hd_hguom_bi_02_c_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write LE HID Operation Mode callback, opcode is 0x%02x.\r\n", testOperation[0]);

    if (err != ULL_HID_ERR_OPCODE_OUTSIDE_RANGE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-02-C test error.\r\n");
    } else {
        testOperation[0]++;
        if (testOperation[0] == 0) {
            app_parse_printf("HOGP/HD/HGUOM/BI-02-C test successful.\r\n");
            return;
        }
        blc_ullhidc_writeLeHidOperationMode(app_ull_hid_get_acl_handle(), testOperation, (rand() & 0x1F) + 1, app_ullhid_hd_hguom_bi_02_c_cb);
        app_parse_printf("write LE HID Operation Mode, opcode is 0x%02x.\r\n", testOperation[0]);
    }
}

void app_ullhid_hd_hguom_bi_02_c(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return;
    }
    testOperation[0] = 2;
    blc_ullhidc_writeLeHidOperationMode(app_ull_hid_get_acl_handle(), testOperation, (rand() & 0x1F) + 1, app_ullhid_hd_hguom_bi_02_c_cb);
    app_parse_printf("write LE HID Operation Mode, opcode is 0x%02x.\r\n", testOperation[0]);
}

union ullhid_supp_report_intervals app_ullhid_get_remote_intervals(void)
{
    struct blc_ullhid_properties_format s_properties;
    u16                                 propertiesLen;
    blc_ullhidc_getUllhidProperties(app_ull_hid_get_acl_handle(), &s_properties, &propertiesLen);

    return s_properties.suppInterval;
}

int blc_ullhidc_writeSelectHybridMode_test(u16 connHandle, struct ullhid_select_hybrid_param_test *param, prf_write_cb_t writeCb)
{
    struct ullhid_select_hybrid_mode_format_test pHybrid = {
        .opcode                 = ULL_HID_OPCODE_SELECT_HYBRID_MODE,
        .CIG_ID                 = param->CIG_ID,
        .CIS_ID                 = param->CIS_ID,
        .suppInterval.intervals = param->suppInterval.intervals};

    memcpy(pHybrid.indices, param->indices, param->indicesCnt);
    return blc_ullhidc_writeLeHidOperationMode(connHandle, (u8 *)&pHybrid, ULLHID_SELECT_HYBRID_MODE_HEAD_SIZE + param->indicesCnt, writeCb);
}

static void app_ullhid_hd_hguom_bi_03_c_3_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_UNSUPPORTED_FEATURE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-03-C test error.\r\n");
    } else {
        app_parse_printf("HOGP/HD/HGUOM/BI-03-C test successful.\r\n");
    }
}

void app_ullhid_hd_hguom_bi_03_c_3(void)
{
    union ullhid_supp_report_intervals intervals = app_ullhid_get_remote_intervals();

    struct ullhid_select_hybrid_param_test param;
    param.CIG_ID     = CIG_ID_0;
    param.CIS_ID     = CIS_ID_0;
    param.indicesCnt = 1;
    param.indices[0] = 0x00;

    if ((intervals.intervals & 0xFF) == 0xFF) {
        param.suppInterval.interval_5ms = 1;
        param.suppInterval.intervalRFU  = 1;
    } else if (blt_calBit1Number_16bit(intervals.intervals) == 1) {
        param.suppInterval.intervals   = intervals.intervals;
        param.suppInterval.intervalRFU = 1;
    } else {
        u8 count = 0;
        for (int i = 7; i >= 0; i--) {
            if ((intervals.intervals & BIT(i))) {
                param.suppInterval.intervals |= BIT(i);
                if (count == 1) {
                    break;
                }
                count++;
            }
        }
    }
    int err = blc_ullhidc_writeSelectHybridMode_test(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_03_c_3_cb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static void app_ullhid_hd_hguom_bi_03_c_2_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_UNSUPPORTED_FEATURE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-03-C test error.\r\n");
    } else {
        app_ullhid_hd_hguom_bi_03_c_3();
    }
}

void app_ullhid_hd_hguom_bi_03_c_2(void)
{
    struct ullhid_select_hybrid_param_test param;
    param.CIG_ID                   = CIG_ID_0;
    param.CIS_ID                   = CIS_ID_0;
    param.suppInterval.intervals   = 0;
    param.suppInterval.intervalRFU = 1;
    param.indicesCnt               = 1;
    param.indices[0]               = 0x00;

    int err = blc_ullhidc_writeSelectHybridMode_test(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_03_c_2_cb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static void app_ullhid_hd_hguom_bi_03_c_1_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_UNSUPPORTED_FEATURE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-03-C test error.\r\n");
    } else {
        app_ullhid_hd_hguom_bi_03_c_2();
    }
}

void app_ullhid_hd_hguom_bi_03_c_1(void)
{
    union ullhid_supp_report_intervals intervals = app_ullhid_get_remote_intervals();

    if ((intervals.intervals & 0xFF) == 0xFF) {
        app_parse_printf("skip round 1. HID device supports all report intervals.\r\n");
        app_ullhid_hd_hguom_bi_03_c_2();
    } else {
        struct ullhid_select_hybrid_param_test param;
        param.CIG_ID     = CIG_ID_0;
        param.CIS_ID     = CIS_ID_0;
        param.indicesCnt = 1;
        param.indices[0] = 0x00;

        for (int i = 7; i >= 0; i--) {
            if ((intervals.intervals & BIT(i)) == 0) {
                param.suppInterval.intervals = BIT(i);
                break;
            }
        }
        int err = blc_ullhidc_writeSelectHybridMode_test(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_03_c_1_cb);

        app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
    }
}

void app_ullhid_hd_hguom_bi_03_c(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return;
    }
    app_ullhid_hd_hguom_bi_03_c_1();
}

static void app_ullhid_hd_hguom_bi_04_c_3_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_UNSUPPORTED_FEATURE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-04-C test error.\r\n");
    } else {
        app_parse_printf("HOGP/HD/HGUOM/BI-04-C test successful.\r\n");
    }
}

void app_ullhid_hd_hguom_bi_04_c_3(void)
{
    struct blc_ullhid_properties_format s_properties;
    u16                                 propertiesLen;
    blc_ullhidc_getUllhidProperties(app_ull_hid_get_acl_handle(), &s_properties, &propertiesLen);

    if (propertiesLen == sizeof(struct blc_ullhid_properties_format)) {
        app_parse_printf("HOGP/HD/HGUOM/BI-04-C test skip round 3.\r\n");
        app_parse_printf("HOGP/HD/HGUOM/BI-04-C test successful.\r\n");
        return;
    }

    union ullhid_supp_report_intervals intervals = s_properties.suppInterval;

    struct ullhid_select_hybrid_param_test param;
    param.CIG_ID     = CIG_ID_0;
    param.CIS_ID     = CIS_ID_0;
    param.indicesCnt = 1;
    param.indices[0] = (propertiesLen - ULLHID_PROPERTIES_HEAD_SIZE) / 2;

    for (int i = 7; i >= 0; i--) {
        if (intervals.intervals & BIT(i)) {
            param.suppInterval.intervals = BIT(i);
            break;
        }
    }
    int err = blc_ullhidc_writeSelectHybridMode_test(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_04_c_3_cb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static void app_ullhid_hd_hguom_bi_04_c_2_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_UNSUPPORTED_FEATURE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-04-C test error.\r\n");
    } else {
        app_ullhid_hd_hguom_bi_04_c_3();
    }
}

void app_ullhid_hd_hguom_bi_04_c_2(void)
{
    union ullhid_supp_report_intervals intervals = app_ullhid_get_remote_intervals();

    struct ullhid_select_hybrid_param_test param;
    param.CIG_ID     = CIG_ID_0;
    param.CIS_ID     = CIS_ID_0;
    param.indicesCnt = 9;
    for (int i = 0; i < 9; i++) {
        param.indices[i] = i;
    }

    for (int i = 7; i >= 0; i--) {
        if (intervals.intervals & BIT(i)) {
            param.suppInterval.intervals = BIT(i);
            break;
        }
    }
    int err = blc_ullhidc_writeSelectHybridMode_test(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_04_c_2_cb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

static void app_ullhid_hd_hguom_bi_04_c_1_cb(u16 connHandle, att_err_t err)
{
    app_parse_printf("write select hybrid mode Callback, connHandle:0x%x, att_err:0x%x\r\n", connHandle, err);

    if (err != ULL_HID_ERR_UNSUPPORTED_FEATURE) {
        app_parse_printf("HOGP/HD/HGUOM/BI-04-C test error.\r\n");
    } else {
        app_ullhid_hd_hguom_bi_04_c_2();
    }
}

void app_ullhid_hd_hguom_bi_04_c_1(void)
{
    union ullhid_supp_report_intervals intervals = app_ullhid_get_remote_intervals();

    struct ullhid_select_hybrid_param_test param;
    param.CIG_ID     = CIG_ID_0;
    param.CIS_ID     = CIS_ID_0;
    param.indicesCnt = 1;
    param.indices[0] = 0xFF;

    for (int i = 7; i >= 0; i--) {
        if (intervals.intervals & BIT(i)) {
            param.suppInterval.intervals = BIT(i);
            break;
        }
    }
    int err = blc_ullhidc_writeSelectHybridMode_test(app_ull_hid_get_acl_handle(), &param, app_ullhid_hd_hguom_bi_04_c_1_cb);

    app_parse_printf("write select hybrid mode, connHandle:0x%x, state:0x%x\r\n", app_ull_hid_get_acl_handle(), err);
}

void app_ullhid_hd_hguom_bi_04_c(char *argv[], int argc, void *user_data)
{
    if (app_ull_hid_get_cis_handle() || app_ull_hid_get_acl_handle() == 0x00) {
        app_parse_printf("must ACL connected, CIS disconnect\r\n");
        return;
    }
    app_ullhid_hd_hguom_bi_04_c_1();
}

#endif
