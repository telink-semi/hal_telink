/********************************************************************************************************
 * @file    app_parse_ui.c
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
#include "tl_common.h"
#include "drivers.h"
#include "stack/ble/ble.h"
#include <strings.h>
#include "app_parse_ui.h"
#include "app_parse_char.h"
#include "app_hdt.h"

#if (INTER_TEST_MODE == TEST_HDT_SENDER)

#if UI_CONTROL_ENABLE
adv_info_t advInfoTable[MAX_ADV_INFO_NUM];

u8 advCnt             = 0;
u8 reconn_en          = 0;
u8 scan_filter_enable = 0;
u8 filter_scan_addr[6];

/**
 * @brief       find advertisement information and store to buffer.
 * @param[in]   addrType: advertisement address type.
 * @param[in]   address: advertisement address.
 * @return      0: advertisement Already exist/ Insufficient buffer.
 *              other: advertisement index.
 */
static u8 app_findAdvInfo(u8 addrType, u8 address[6])
{
    if (advCnt == MAX_ADV_INFO_NUM) {
        return 0;
    }

    for (int i = 0; i < advCnt; i++) {
        adv_info_t *advInfo = &advInfoTable[i];

        if (advInfo->addrType == addrType && !memcmp(advInfo->address, address, 6)) {
            return 0;
        }
    }
    advInfoTable[advCnt].addrType = addrType;
    memcpy(advInfoTable[advCnt].address, address, 6);
    advCnt++;
    return advCnt;
}

/**
 * @brief       initial/clean advertisement information buffer.
 * @param[in]   none.
 * @return      none.
 */
void app_init_AdvInfoBuf(void)
{
    advCnt = 0;
    memset(&advInfoTable[0], 0, sizeof(adv_info_t) * MAX_ADV_INFO_NUM);
}

/**
 * @brief       found advertisement device event.
 * @param[in]   p: Data carried by the event.
 * @return      0.
 */
int app_parse_foundAdv(u8 *p)
{
    event_adv_report_t *pa = (event_adv_report_t *)p;
    if (scan_filter_enable) {
        if (memcpy(pa->mac, filter_scan_addr, 6)) {
            return 0;
        }
    }
    u8 index = app_findAdvInfo(pa->adr_type, pa->mac);
    if (index) {
        u8  adLen;
        u8  completeName[32] = "{No find name}";
        u8  len              = pa->len;
        u8 *pdata            = pa->data;
        while (len) {
            adLen = pdata[0];
            if (pdata[1] == DT_COMPLETE_LOCAL_NAME) {
                if (adLen - 1 > 31) {
                    adLen = 32;
                }
                memcpy(completeName, (pdata) + 2, adLen - 1);
                completeName[adLen - 1] = '\0';
                break;
            }

            if (len > (adLen + 1)) {
                len -= (adLen + 1);
                pdata += (adLen + 1);
            } else {
                len = 0;
            }
        }
        s8 rssi = pa->data[pa->len];
        app_parse_printf("[%d] %s %s RSSI:%d name:%s\r\n", index, pa->adr_type ? "random" : "public", addr_to_str(pa->mac), rssi, completeName);
    }

    return 0;
}

/**
 * @brief       scan advertisement device.
 * @param[in]   argv: parse input parameter pointer.
 * @param[in]   argc: parse input parameter size.
 * @param[in]   user_data: command input data, default NULL.
 * @return      none.
 */
static void app_parse_ui_scan(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (argc == 0) {
        app_parse_printf("scan <start|stop|clear|filter>\r\n");
        return;
    }

    if (strcasecmp(argv[0], "start") == 0) {
        app_parse_printf("central start scan\r\n");
        app_parse_printf("If need to connect, use 'conn <dev_idx>'\r\n");
        app_init_AdvInfoBuf();
        blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
    } else if (strcasecmp(argv[0], "stop") == 0) {
        app_parse_printf("central stop scan\r\n");
        blc_ll_setScanEnable(BLC_SCAN_DISABLE, DUP_FILTER_DISABLE);
    } else if (strcasecmp(argv[0], "clear") == 0) {
        app_init_AdvInfoBuf();
        app_parse_printf("central clear adv info\r\n");
    } else if (strcasecmp(argv[0], "filter") == 0) {
        if (strcasecmp(argv[1], "disable") == 0) {
            app_parse_printf("central scan filter disable\r\n");
            scan_filter_enable = 0;
        } else if (strcasecmp(argv[1], "enable") == 0) {
            app_parse_printf("central scan filter enable\r\n");
            scan_filter_enable = 1;
        } else if (argc == 7) {
            scan_filter_enable = 1;
            blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
            app_parse_printf("central scan filter enable\r\n");
            for (unsigned char i = 0; i < 6; i++) {
                filter_scan_addr[5 - i] = app_parse_str2xn(argv[i + 1]);
            }
        } else {
            app_parse_printf("scan filter not support [%s]", argv[0]);
            app_parse_printf("scan filter <enable|disable|mac address>\r\n");
        }
    } else {
        app_parse_printf("scan not support [%s]", argv[0]);
        app_parse_printf("scan <start|stop|clear|filter>\r\n");
    }
}

/**
 * @brief       create ACL connect by index.
 * @param[in]   index: scan advertisement index.
 * @return      0: index error.
 *              1: index true.
 */
int app_createACLConn(int index)
{
    if (index > advCnt) {
        return 0;
    }

    blc_ll_createConnection(SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, INITIATE_FP_ADV_SPECIFY, advInfoTable[index - 1].addrType, advInfoTable[index - 1].address, OWN_ADDRESS_PUBLIC,
                            CONN_INTERVAL_10MS, CONN_INTERVAL_10MS, 0, CONN_TIMEOUT_4S, 0, 0xFFFF);
    return 1;
}

static void app_parse_ui_conn(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (argc != 1) {
        app_parse_printf("conn <dev_idx|unpair>\r\n");
        return;
    }

    if (strcasecmp(argv[0], "unpair") == 0) {
        if (hdt_app_ctrl.connhandle) {
            app_parse_printf("disconnection\r\n");
            blc_ll_disconnect(hdt_app_ctrl.connhandle, HCI_ERR_REMOTE_USER_TERM_CONN);
        }
        blc_smp_eraseAllBondingInfo();
        app_parse_printf("device unpair success\r\n");
    } else if (app_createACLConn(app_parse_str2n(argv[0]))) {
        app_parse_printf("central start connect peripheral\r\n");
    } else {
        app_parse_printf("connect index error\r\n");
    }
}

static void app_parse_ui_reconn(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (argc != 1) {
        app_parse_printf("reconn <enable|disable>\r\n");
        return;
    }

    if (strcasecmp(argv[0], "enable") == 0) {
        app_parse_printf("reconnection enable\r\n");
        blc_ll_setScanEnable(BLC_SCAN_ENABLE, DUP_FILTER_DISABLE);
        reconn_en = 1;
    } else if (strcasecmp(argv[0], "disable") == 0) {
        app_parse_printf("reconnection disable\r\n");
        reconn_en = 0;
    } else {
        app_parse_printf("reconnect command error\r\n");
        app_parse_printf("reconn <enable|disable>\r\n");
    }
}

static void app_parse_ui_hdt(char *argv[], int argc, void *user_data)
{
    (void)user_data;
    if (!hdt_app_ctrl.connhandle) {
        app_parse_printf("acl connection establishment fail\r\n");
        return;
    }
    if (argc < 1) {
        app_parse_printf("hdt <rst>\r\n");
        app_parse_printf("cmd-> [rst]:reset\r\n");
        return;
    }

    if (strcasecmp(argv[0], "rst") == 0) {
        blc_ll_disconnect(hdt_app_ctrl.connhandle, HCI_ERR_CONN_TERM_BY_LOCAL_HOST);
        app_parse_printf("hdt reset ok\r\n");
    }
    else {
        app_parse_printf("hdt cmd not support [%s]", argv[0]);
        app_parse_printf("hdt <rst>\r\n");
    }
}


#define READBUFSIZE 4

u8 rBuff[READBUFSIZE];
u8 rBuffLen;

u8 readCBackFun(u16 connHandle, u8 err, gatt_read_data_t *rdData, struct __attribute__((packed)) gattc_read_cfg *params)
{
    (void)connHandle;

    if (err) {
        app_parse_printf("read attr value callback, err is %d\r\n", err);
        return GATT_PROC_END;
    }
    app_parse_printf("read value state is %d, data(%d) is %s", rdData->rdState, rdData->dataLen, hex_to_str(rdData->dataVal, rdData->dataLen));
    app_parse_printf("read sing data(%d) is %s", *params->single.wBuffLen, hex_to_str(params->single.wBuff, *params->single.wBuffLen));

    return GATT_PROC_CONT;
}

static const parse_fun_list_t centralParse[] = {
    {"scan", app_parse_ui_scan, NULL},
    {"conn", app_parse_ui_conn, NULL},
    {"reconn", app_parse_ui_reconn, NULL},
    {"hdt", app_parse_ui_hdt, NULL},

};

/**
 * @brief       parse UI initial function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_init(void)
{
    app_parse_init(centralParse, ARRAY_SIZE(centralParse));
    app_parse_printf("Parse init OK\r\n");
}

/**
 * @brief       central UI loop function.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_ui_loop(void)
{
    app_parse_loop();
}
#endif
#endif
