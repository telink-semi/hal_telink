/********************************************************************************************************
 * @file    ble_hci_le_event.c
 *
 * @brief   This is the source file for TLSR/TL
 *
 * @author  Bluetooth Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"
#include "../../inc/ble_host_internal.h"

#include "../inc/ble_hci.h"
#include "../inc/ble_hci_cmd.h"
#include "../inc/ble_hci_evt.h"

#include "stack/ble/ble_stack.h"
#include "../inc/ble_hci_log.h"
#include "../../gap/acl/inc/ble_gap_reader.h"

//TODO:
#include "stack/ble/ble_stack.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"


typedef int ble_host_hci_evt_le_fn(uint8_t subevent, const void *data, uint32_t len);

static ble_host_hci_evt_le_fn ble_host_hci_evt_le_adv_rpt;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_conn_complete;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_conn_upd_complete;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_lt_key_req;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_conn_parm_req;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_phy_update_complete;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_enh_conn_complete;

static ble_host_hci_evt_le_fn ble_host_hci_evt_le_dir_adv_rpt;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_ext_adv_rpt;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_rd_rem_used_feat_complete;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_scan_timeout;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_adv_set_terminated;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_periodic_adv_sync_estab;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_periodic_adv_rpt;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_periodic_adv_sync_lost;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_scan_req_rcvd;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_periodic_adv_sync_transfer;
#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_create_big_complete;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_terminate_big_complete;
#endif
#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_big_sync_established;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_big_sync_lost;
#endif
#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_biginfo_adv_report;
#endif
#if (LL_FEATURE_ENABLE_POWER_CONTROL)
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_pathloss_threshold;
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_transmit_power_report;
#endif
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
static ble_host_hci_evt_le_fn ble_host_hci_evt_le_subrate_change;
#endif

/** Dispatch table for incoming HCI LE events.  Sorted by event code field. */
static ble_host_hci_evt_le_fn *const ble_host_hci_evt_le_dispatch[] = {

    [BLE_HCI_LE_SUBEV_CONN_COMPLETE] = ble_host_hci_evt_le_conn_complete,


    [BLE_HCI_LE_SUBEV_ADV_RPT] = ble_host_hci_evt_le_adv_rpt,

    [BLE_HCI_LE_SUBEV_CONN_UPD_COMPLETE] = ble_host_hci_evt_le_conn_upd_complete,
    [BLE_HCI_LE_SUBEV_LT_KEY_REQ] = ble_host_hci_evt_le_lt_key_req,
    [BLE_HCI_LE_SUBEV_REM_CONN_PARM_REQ] = ble_host_hci_evt_le_conn_parm_req,
    [BLE_HCI_LE_SUBEV_ENH_CONN_COMPLETE] = ble_host_hci_evt_le_enh_conn_complete,

    [BLE_HCI_LE_SUBEV_DIRECT_ADV_RPT] = ble_host_hci_evt_le_dir_adv_rpt,

    [BLE_HCI_LE_SUBEV_PHY_UPDATE_COMPLETE] = ble_host_hci_evt_le_phy_update_complete,

    [BLE_HCI_LE_SUBEV_EXT_ADV_RPT] = ble_host_hci_evt_le_ext_adv_rpt,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_ESTAB] = ble_host_hci_evt_le_periodic_adv_sync_estab,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_RPT] = ble_host_hci_evt_le_periodic_adv_rpt,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_LOST] = ble_host_hci_evt_le_periodic_adv_sync_lost,
    [BLE_HCI_LE_SUBEV_RD_REM_USED_FEAT] = ble_host_hci_evt_le_rd_rem_used_feat_complete,
    [BLE_HCI_LE_SUBEV_SCAN_TIMEOUT] = ble_host_hci_evt_le_scan_timeout,
    [BLE_HCI_LE_SUBEV_ADV_SET_TERMINATED] = ble_host_hci_evt_le_adv_set_terminated,
    [BLE_HCI_LE_SUBEV_SCAN_REQ_RCVD] = ble_host_hci_evt_le_scan_req_rcvd,
    [BLE_HCI_LE_SUBEV_PERIODIC_ADV_SYNC_TRANSFER] = ble_host_hci_evt_le_periodic_adv_sync_transfer,
#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)
    [BLE_HCI_LE_SUBEV_CREATE_BIG_COMPLETE] = ble_host_hci_evt_le_create_big_complete,
    [BLE_HCI_LE_SUBEV_TERMINATE_BIG_COMPLETE] = ble_host_hci_evt_le_terminate_big_complete,
#endif
#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)
    [BLE_HCI_LE_SUBEV_BIG_SYNC_ESTABLISHED] = ble_host_hci_evt_le_big_sync_established,
    [BLE_HCI_LE_SUBEV_BIG_SYNC_LOST] = ble_host_hci_evt_le_big_sync_lost,
#endif
#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)
    [BLE_HCI_LE_SUBEV_BIGINFO_ADV_REPORT] = ble_host_hci_evt_le_biginfo_adv_report,
#endif
#if (LL_FEATURE_ENABLE_POWER_CONTROL)
    [BLE_HCI_LE_SUBEV_PATH_LOSS_THRESHOLD] = ble_host_hci_evt_le_pathloss_threshold,
    [BLE_HCI_LE_SUBEV_TRANSMIT_POWER_REPORT] = ble_host_hci_evt_le_transmit_power_report,
#endif
#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
    [BLE_HCI_LE_SUBEV_SUBRATE_CHANGE] = ble_host_hci_evt_le_subrate_change,
#endif

};

#define BLE_HS_HCI_EVT_LE_DISPATCH_SZ (sizeof ble_host_hci_evt_le_dispatch / sizeof ble_host_hci_evt_le_dispatch[0])

ble_host_hci_evt_le_fn *ble_host_hci_evt_le_dispatch_find(uint8_t event_code)
{
    if (event_code >= BLE_HS_HCI_EVT_LE_DISPATCH_SZ) {
        return NULL;
    }

    return ble_host_hci_evt_le_dispatch[event_code];
}

int ble_host_hci_evt_le_meta(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_meta *ev = data;
    ble_host_hci_evt_le_fn *fn;

    if (len < sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    fn = ble_host_hci_evt_le_dispatch_find(ev->subevent);
    if (fn) {
        return fn(ev->subevent, data, len);
    } else {
        return BLE_HOST_ERR_NOTSUP;
    }

    return 0;
}

static int ble_host_hci_evt_le_enh_conn_complete(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_enh_conn_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    // todo: remove later.
    struct rf_connect_hdr_t {
        u8 type : 4;
        u8 rfu1 : 1;
        u8 chan_sel : 1;
        u8 txAddr : 1;
        u8 rxAddr : 1;

        u8 rf_len;   //LEN(6)_RFU(2)
        u8 initA[6]; //scanA
        u8 advA[6];  //
    } __attribute__((packed)) con_req_info = {
        .type = 0,
        .rfu1 = 0,
        .chan_sel = 0,
        .rf_len = 34,
    };

    u8 ownAddr[6];
    ble_host_hci_get_bd_addr(ownAddr);

    if (ev->role == BLE_HCI_CONN_ROLE_PERIPHERAL) {
        con_req_info.txAddr = ev->peer_addr_type ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        memcpy(con_req_info.initA, ev->peer_addr, 6);
        con_req_info.rxAddr = BLE_ADDR_PUBLIC;
        memcpy(con_req_info.advA, ownAddr, 6);
        extern void blt_ll_record_identity_address(u8 type, u8 * addr);
        blt_ll_record_identity_address(BLE_ADDR_PUBLIC, ownAddr);

        BLE_HOST_HCI_COMMON_INFO("*****S Con_Req_Info: %s ******");
        BLE_HOST_HCI_COMMON_INFO("initA: %s, advA: %s", hex_to_str(con_req_info.initA, 6), hex_to_str(con_req_info.advA, 6));
    } else {
        con_req_info.rxAddr = ev->peer_addr_type ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        memcpy(con_req_info.advA, ev->peer_addr, 6);
        con_req_info.txAddr = BLE_ADDR_PUBLIC;
        memcpy(con_req_info.initA, ownAddr, 6);
        extern void blt_ll_record_identity_address(u8 type, u8 * addr);
        blt_ll_record_identity_address(BLE_ADDR_PUBLIC, ownAddr);

        BLE_HOST_HCI_COMMON_INFO("*****M Con_Req_Info ******");
    }

    extern int blt_gap_conn_complete_handler(u16 connHandle, u8 * p);
    blt_gap_conn_complete_handler(ev->conn_handle, (uint8_t *) ((&con_req_info.rf_len) - 1));

    /* We verified that there is a free connection when the procedure began. */
    struct ble_acl_conn_complete acl_conn = {
        .conn_handle = ev->conn_handle,
        .role = ev->role,
        .peer_addr_type = ev->peer_addr_type,
        .conn_interval = ev->conn_itvl,
        .conn_latency = ev->conn_latency,
        .supervision_timeout = ev->supervision_timeout,
        .master_clock_acc = ev->mca,
    };

    memcpy(acl_conn.peer_addr, ev->peer_addr, sizeof(acl_conn.peer_addr));

    ble_host_conn_insert_conn_complete(&acl_conn);

    return 0;
}

static int ble_host_hci_evt_le_conn_complete(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_conn_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    struct rf_connect_hdr_t {
        u8 type : 4;
        u8 rfu1 : 1;
        u8 chan_sel : 1;
        u8 txAddr : 1;
        u8 rxAddr : 1;

        u8 rf_len;   //LEN(6)_RFU(2)
        u8 initA[6]; //scanA
        u8 advA[6];  //
    } __attribute__((packed)) con_req_info = {
        .type = 0,
        .rfu1 = 0,
        .chan_sel = 0,
        .rf_len = 34,
    };

    u8 ownAddr[6];
    ble_host_hci_get_bd_addr(ownAddr);

    if (ev->role == BLE_HCI_CONN_ROLE_PERIPHERAL) {
        con_req_info.txAddr = ev->peer_addr_type ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        memcpy(con_req_info.initA, ev->peer_addr, 6);
        con_req_info.rxAddr = BLE_ADDR_PUBLIC;
        memcpy(con_req_info.advA, ownAddr, 6);
        extern void blt_ll_record_identity_address(u8 type, u8 * addr);
        blt_ll_record_identity_address(BLE_ADDR_PUBLIC, ownAddr);

        BLE_HOST_HCI_COMMON_INFO("*****SE Con_Req_Info: %s ******", hex_to_str(&con_req_info, sizeof(struct rf_connect_hdr_t)));
    } else {
        con_req_info.rxAddr = ev->peer_addr_type ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;
        memcpy(con_req_info.advA, ev->peer_addr, 6);
        con_req_info.txAddr = BLE_ADDR_PUBLIC;
        memcpy(con_req_info.initA, ownAddr, 6);
        extern void blt_ll_record_identity_address(u8 type, u8 * addr);
        blt_ll_record_identity_address(BLE_ADDR_PUBLIC, ownAddr);

        BLE_HOST_HCI_COMMON_INFO("*****ME Con_Req_Info ******");
    }

    extern int blt_gap_conn_complete_handler(u16 connHandle, u8 * p);
    blt_gap_conn_complete_handler(ev->conn_handle, (uint8_t *) ((&con_req_info.rf_len) - 1));


    /* We verified that there is a free connection when the procedure began. */
    struct ble_acl_conn_complete acl_conn = {
        .conn_handle = ev->conn_handle,
        .role = ev->role,
        .peer_addr_type = ev->peer_addr_type,
        .conn_interval = ev->conn_itvl,
        .conn_latency = ev->conn_latency,
        .supervision_timeout = ev->supervision_timeout,
        .master_clock_acc = ev->mca,
    };

    memcpy(acl_conn.peer_addr, ev->peer_addr, sizeof(acl_conn.peer_addr));

    ble_host_conn_insert_conn_complete(&acl_conn);

    return 0;
}

static int ble_host_hci_evt_le_adv_rpt_first_pass(const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_adv_rpt *ev = data;
    const struct adv_reports *rpt;
    int                                       i;

    if (len < sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    len -= sizeof(*ev);
    data += sizeof(*ev);

    if (ev->num_reports < BLE_HCI_LE_ADV_RPT_NUM_RPTS_MIN ||
        ev->num_reports > BLE_HCI_LE_ADV_RPT_NUM_RPTS_MAX) {
        return BLE_HOST_ERR_BADDATA;
    }

    for (i = 0; i < ev->num_reports; i++) {
        /* extra byte for RSSI after adv data */
        if (len < sizeof(*rpt) + 1) {
            return BLE_HOST_ERR_CONTROLLER;
        }

        rpt = data;

        len -= sizeof(*rpt) + 1;
        data += sizeof(rpt) + 1;

        if (rpt->data_len > len) {
            return BLE_HOST_ERR_CONTROLLER;
        }

        len -= rpt->data_len;
        data += rpt->data_len;
    }

    /* Make sure length was correct */
    if (len) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    return 0;
}

static int ble_host_hci_evt_le_adv_rpt(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_adv_rpt *ev = data;
    const struct adv_reports *rpt;
    int                                       rc;
    int                                       i;

    /* Validate the event is formatted correctly */
    rc = ble_host_hci_evt_le_adv_rpt_first_pass(data, len);
    if (rc != 0) {
        return rc;
    }

    data += sizeof(*ev);

    for (i = 0; i < ev->num_reports; i++) {
        rpt = data;

        data += sizeof(rpt) + rpt->data_len + 1;

        /* TODO: Notify the BLE GAP layer of the advertising report event.  */
    }

    return 0;
}

static int ble_host_hci_evt_le_dir_adv_rpt(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_direct_adv_rpt *ev = data;

    int i;

    if (len < sizeof(*ev) || len != ev->num_reports * sizeof(ev->reports[0])) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    for (i = 0; i < ev->num_reports; i++) {
        /* TODO: Notify the BLE GAP layer of the directed advertising report event.  */
    }

    return 0;
}

static int ble_host_hci_evt_le_rd_rem_used_feat_complete(uint8_t subevent, const void *data, uint32_t len)
{
    (void) subevent;
    const struct ble_hci_ev_le_subev_rd_rem_used_feat *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the remote used features complete event.  */
    ble_host_gap_read_remote_feature(ev->conn_handle, *(uint64_t *) ev->features);
    return 0;
}

#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)

static int ble_host_hci_decode_legacy_type(uint16_t evt_type)
{
    switch (evt_type) {
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_ADV_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_DIRECT_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_DIR_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_SCAN_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_SCAN_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_ADV_NONCON_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_NONCONN_IND;
    case BLE_HCI_LEGACY_ADV_EVTYPE_SCAN_RSP_ADV_IND:
    case BLE_HCI_LEGACY_ADV_EVTYPE_SCAN_RSP_ADV_SCAN_IND:
        return BLE_HCI_ADV_RPT_EVTYPE_SCAN_RSP;
    default:
        return -1;
    }
}
#endif

static int ble_host_hci_evt_le_ext_adv_rpt(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
    const struct ble_hci_ev_le_subev_ext_adv_rpt *ev = data;
    const struct ext_adv_reports *report;
    int                                           i;
    int                                           legacy_event_type;

    if (len < sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    if (ev->num_reports < BLE_HCI_LE_ADV_RPT_NUM_RPTS_MIN ||
        ev->num_reports > BLE_HCI_LE_ADV_RPT_NUM_RPTS_MAX) {
        return BLE_HOST_ERR_BADDATA;
    }

    report = &ev->reports[0];
    for (i = 0; i < ev->num_reports; i++) {
        uint8_t props = (report->evt_type) & 0x1F;

///////temporary for btble dongle, need to optimize and remove this code/////////
#if DUAL_CORE_MODE_ENABLED//(TLK_CFG_UART_TOOL_ENABLE) //must report needed adv to pc
#if (TLK_CFG_UART_TOOL_ENABLE && TLKAPP_LEMGR_ENABLE)
#include "tlkapp/tlkapp.h"
            uint08  name_len;
            uint08 *name;

            /* report LE Audio device to UART tool */
            if (blc_adv_get16BitServiceUuid(report->data, report->data_len, 0x184E)) {
                name = blc_adv_getCompleteNameInformation(report->data, report->data_len,&name_len);
                if (tlkapp_lemgr_adv_sameCheck(report->addr, report->addr_type)) {
                    tlkapp_lemgr_sendExtScanDataEvt(report->addr_type, report->addr, name, name_len);
                }
            }

#if (ACL_CENTRAL_GENERAL_NUM)
            /* Demo code: report only HID REmote Control device to UART tool, user to change this according to requirement */
            if (blc_adv_get16BitAppearanceUuid(pExtAdvInfo->data, pExtAdvInfo->data_length, 0x0180)) //0x180 = 384, Generic Remote Control, Generic category
            {
                name = blc_adv_getCompleteNameInformation(pExtAdvInfo->data, pExtAdvInfo->data_length, &name_len);
                if (tlkapp_lemgr_adv_sameCheck(pExtAdvInfo->address, pExtAdvInfo->address_type)) {
                    tlkapp_lemgr_sendExtScanDataEvt(pExtAdvInfo->address_type, pExtAdvInfo->address, name, name_len);
                }
            }
#endif
            int central_auto_connect = 0;
            int user_manual_pairing  = 0;
            s8 rssi = report->rssi;
            if (SetConnectParam.tool_create_connect) {

                user_manual_pairing = SetConnectParam.address_type == report->addr_type && !memcmp(SetConnectParam.address, report->addr, 6) && (rssi > -50);
                tlkapi_printf(APP_LOG_EN, "user_manual_pairing = %d",user_manual_pairing);
            } else {
                //user_manual_pairing = central_pairing_enable && (rssi > -50);
            }

            #if (ACL_CENTRAL_SMP_ENABLE)
//            central_auto_connect = le_auto_connect_flag && blc_smp_searchBondingPeripheralDevice_by_PeerMacAddress(pExtAdvInfo->address_type, pExtAdvInfo->address);
            #endif

             if (central_auto_connect || user_manual_pairing) { //create connection
                struct ble_hci_le_ext_create_conn_cp p_create_conn = {
                    .filter_policy = INITIATE_FP_ADV_SPECIFY,
                    .own_addr_type = OWN_ADDRESS_PUBLIC,
                    .peer_addr_type = report->addr_type,
                    .init_phy_mask = INIT_PHY_1M_2M,
                };
                memcpy(p_create_conn.peer_addr, report->addr, 6);
                p_create_conn.conn_params[0].scan_itvl = SCAN_INTERVAL_100MS;
                p_create_conn.conn_params[0].scan_window = SCAN_WINDOW_100MS;
                p_create_conn.conn_params[0].conn_min_itvl = CONN_INTERVAL_40MS;
                p_create_conn.conn_params[0].conn_max_itvl = CONN_INTERVAL_40MS;
                p_create_conn.conn_params[0].conn_latency = 0;
                p_create_conn.conn_params[0].supervision_timeout = CONN_TIMEOUT_1S;
                p_create_conn.conn_params[0].min_ce = 0;
                p_create_conn.conn_params[0].max_ce = 0;

                p_create_conn.conn_params[1].scan_itvl = SCAN_INTERVAL_100MS;
                p_create_conn.conn_params[1].scan_window = SCAN_WINDOW_100MS;
                p_create_conn.conn_params[1].conn_min_itvl = CONN_INTERVAL_40MS;
                p_create_conn.conn_params[1].conn_max_itvl = CONN_INTERVAL_40MS;
                p_create_conn.conn_params[1].conn_latency = 0;
                p_create_conn.conn_params[1].supervision_timeout = CONN_TIMEOUT_1S;
                p_create_conn.conn_params[1].min_ce = 0;
                p_create_conn.conn_params[1].max_ce = 0;

                ble_sts_t status = BLE_SUCCESS;
                //status = blc_ll_extended_createConnection(INITIATE_FP_ADV_SPECIFY, OWN_ADDRESS_PUBLIC, pExtAdvInfo->address_type, pExtAdvInfo->address, INIT_PHY_1M_2M, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_40MS, CONN_INTERVAL_40MS, CONN_TIMEOUT_1S, SCAN_INTERVAL_100MS, SCAN_WINDOW_100MS, CONN_INTERVAL_40MS, CONN_INTERVAL_40MS, CONN_TIMEOUT_1S, 0, 0, 0, 0, 0);
                status = ble_host_hci_le_ext_create_connection(&p_create_conn);
//                tlkapi_printf(APP_LOG_EN, "p_create_conn->dir_addr %s", report->addr);
//                tlkapi_printf(APP_LOG_EN, "p_create_conn->name %s", name);

                if (status == BLE_SUCCESS) { //create connection success
                    tlkapi_printf(APP_LOG_EN, "[APP][CMD] Ext Create connection success", report->dir_addr, 6);

                if (SetConnectParam.tool_create_connect) {
                    SetConnectParam.tool_create_connect = 0;
                    if (status == BLE_SUCCESS) {
                        tlkapp_lemgr_sendCommRsp(TLKSYS_LE_MSGID_CONNECT, TLKPRT_COMM_RSP_STATUE_SUCCESS, 0x00, (uint08 *)&status, 1);
                    } else {
                        tlkapp_lemgr_sendCommRsp(TLKSYS_LE_MSGID_CONNECT, TLKPRT_COMM_RSP_STATUE_FAILURE, 0x01, (uint08 *)&status, 1);
                    }
                }
             }
        }
#endif
#endif

        if (props & BLE_HCI_ADV_LEGACY_MASK) {
            legacy_event_type = ble_host_hci_decode_legacy_type(report->evt_type);
            (void) legacy_event_type;
        } else {
            /** TOOD */
        }
    }
#endif
    return 0;
}

static int ble_host_hci_evt_le_periodic_adv_sync_estab(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
    const struct ble_hci_ev_le_subev_periodic_adv_sync_estab *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the periodic advertising sync established event.  */

#endif
    return 0;
}

static int ble_host_hci_evt_le_periodic_adv_rpt(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
    const struct ble_hci_ev_le_subev_periodic_adv_rpt *ev = data;

    if (len < sizeof(*ev) || len != (sizeof(*ev) + ev->data_len)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the periodic advertising report event.  */

#endif
    return 0;
}

static int ble_host_hci_evt_le_periodic_adv_sync_lost(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING)
    const struct ble_hci_ev_le_subev_periodic_adv_sync_lost *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the periodic advertising sync lost event.  */

#endif
    return 0;
}

#if (LL_FEATURE_ENABLE_POWER_CONTROL)

static int ble_host_hci_evt_le_pathloss_threshold(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_path_loss_threshold *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the path loss threshold event.  */

    return 0;
}

static int ble_host_hci_evt_le_transmit_power_report(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_transmit_power_report *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the transmit power report event.  */

    return 0;
}
#endif


static int ble_host_hci_evt_le_periodic_adv_sync_transfer(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_PAST)
    const struct ble_hci_ev_le_subev_periodic_adv_sync_transfer *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the periodic advertising sync transfer event.  */

#endif
    return 0;
}

#if (LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER)

static int ble_host_hci_evt_le_create_big_complete(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_create_big_complete *ev = data;

    if (len != sizeof(*ev) + (ev->num_bis * sizeof(ev->conn_handle[0]))) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the create big complete event.  */

    return 0;
}

static int ble_host_hci_evt_le_terminate_big_complete(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_terminate_big_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the terminate big complete event.  */

    return 0;
}
#endif

#if (LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER)

static int ble_host_hci_evt_le_big_sync_established(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_big_sync_established *ev = data;

    if (len < sizeof(*ev) ||
        len != (sizeof(*ev) + ev->num_bis * sizeof(ev->conn_handle[0]))) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the big sync established event.  */

    return 0;
}

static int ble_host_hci_evt_le_big_sync_lost(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_big_sync_lost *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the big sync lost event.  */

    return 0;
}
#endif

#if (LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC)

static int ble_host_hci_evt_le_biginfo_adv_report(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_biginfo_adv_report *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the biginfo advertising report event.  */

    return 0;
}
#endif

static int ble_host_hci_evt_le_scan_timeout(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_EXTENDED_SCAN)
    const struct ble_hci_ev_le_subev_scan_timeout *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_BADDATA;
    }

    /* TODO: Notify the BLE GAP layer of the scan timeout event.  */

#endif
    return 0;
}

static int ble_host_hci_evt_le_adv_set_terminated(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
    const struct ble_hci_ev_le_subev_adv_set_terminated *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the adv set terminated event.  */

#endif

    return 0;
}

static int ble_host_hci_evt_le_scan_req_rcvd(uint8_t subevent, const void *data, uint32_t len)
{
#if (LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING)
    const struct ble_hci_ev_le_subev_scan_req_rcvd *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the scan request received event.  */

#endif

    return 0;
}

#if (LL_FEATURE_ENABLE_CONNECTION_SUBRATING)
static int ble_host_hci_evt_le_subrate_change(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_subrate_change *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the subrate change event.  */


    return 0;
}
#endif

static int ble_host_hci_evt_le_conn_upd_complete(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_conn_upd_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the connection update complete event.  */

    return 0;
}

static int ble_host_hci_evt_le_lt_key_req(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_lt_key_req *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE SMP layer of the long term key request event.  */
#if DUAL_CORE_MODE_ENABLED
    extern int bls_smp_llGetLtkReq(u16 connHandle, u8 * random, u16 ediv);

    BLE_HOST_HCI_COMMON_CMD_INFO("bls_smp_llGetLtkReq: %s", hex_to_str(data, len));

    bls_smp_llGetLtkReq(ev->conn_handle, (uint8_t *) &ev->rand, ev->div);
#endif

    return 0;
}

static int ble_host_hci_evt_le_conn_parm_req(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_rem_conn_param_req *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the remote connection parameter request event.  */

    return 0;
}

static int ble_host_hci_evt_le_phy_update_complete(uint8_t subevent, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_subev_phy_update_complete *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the PHY update complete event.  */

    return 0;
}


int ble_host_hci_le_evt_dispatch_process(const struct ble_hci_ev_le_meta *ev, uint32_t len)
{
    int rc;

    ble_host_hci_evt_le_fn *fn;

    BLE_HOST_HCI_COMMON_EVT_INFO("dispatch le event*: subevent 0x%02x, %s", ev->subevent, hex_to_str(ev->data, len));

    extern int blc_hci_send_event1(u32 h, u8 * para, int n);
    // HCI_FLAG_EVENT_BT_STD (1 << 25)  HCI_EVT_LE_META 0x3E
    blc_hci_send_event1((1 << 25) | 0x3e, (uint8_t *) ev, len);

    if (len < sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    BLE_HOST_HCI_COMMON_EVT_INFO("dispatch le event: subevent 0x%02x", ev->subevent);

    fn = ble_host_hci_evt_le_dispatch_find(ev->subevent);
    if (fn) {
        return fn(ev->subevent, (const void *) ev, len);
    } else {
        return BLE_HOST_ERR_NOTSUP;
    }

    return rc;
}

#pragma GCC diagnostic pop
