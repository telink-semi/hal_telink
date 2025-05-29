/********************************************************************************************************
 * @file    ble_hci_event.c
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
#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host.h"
#include "../inc/ble_host_internal.h"
#include "../inc/ble_host_sal.h"

#include "inc/ble_hci.h"
#include "inc/ble_hci_log.h"
#include "inc/ble_hci_evt.h"
//#include "stack/ble/ble_config_internal.h"
#include "../gap/acl/inc/ble_gap_reader.h"

//TODO:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"


typedef int ble_host_hci_evt_fn(uint8_t event_code, const void *data, uint32_t len);
static ble_host_hci_evt_fn ble_host_hci_evt_remote_version;
static ble_host_hci_evt_fn ble_host_hci_evt_hw_error;
static ble_host_hci_evt_fn ble_host_hci_evt_num_completed_pkts;
#if 1//(LL_ACL_CEN_EN || LL_ACL_PER_EN)
static ble_host_hci_evt_fn ble_host_hci_evt_disconn_complete;
static ble_host_hci_evt_fn ble_host_hci_evt_encrypt_change;
static ble_host_hci_evt_fn ble_host_hci_evt_enc_key_refresh;
#endif

/* Notice XXX: le_meta event dispatch in other file. */
static ble_host_hci_evt_fn ble_host_hci_evt_le_meta;

#if (defined(BLE_HCI_VS_EVT_ENABLE))
static ble_host_hci_evt_fn ble_host_hci_evt_vs;
#endif


/** Dispatch table for incoming HCI events.  Sorted by event code field. */
struct ble_host_hci_evt_dispatch_entry {
    uint8_t              event_code;
    ble_host_hci_evt_fn *cb;
};

static const struct ble_host_hci_evt_dispatch_entry ble_host_hci_evt_dispatch[] = {
    {BLE_HCI_EVCODE_RD_REM_VER_INFO_CMP,ble_host_hci_evt_remote_version},
    {BLE_HCI_EVCODE_HW_ERROR,        ble_host_hci_evt_hw_error          },
    {BLE_HCI_EVCODE_NUM_COMP_PKTS,   ble_host_hci_evt_num_completed_pkts},
#if 1//(LL_ACL_CEN_EN || LL_ACL_PER_EN)
    {BLE_HCI_EVCODE_DISCONN_CMP,     ble_host_hci_evt_disconn_complete  },
    {BLE_HCI_EVCODE_ENCRYPT_CHG,     ble_host_hci_evt_encrypt_change    },
    {BLE_HCI_EVCODE_ENC_KEY_REFRESH, ble_host_hci_evt_enc_key_refresh   },
#endif
    {BLE_HCI_EVCODE_LE_META,         ble_host_hci_evt_le_meta           },
#if (defined(BLE_HCI_VS_EVT_ENABLE))
    {BLE_HCI_EVCODE_VS,              ble_host_hci_evt_vs                },
#endif
};

#define BLE_HS_HCI_EVT_DISPATCH_SZ (sizeof ble_host_hci_evt_dispatch / sizeof ble_host_hci_evt_dispatch[0])

static const struct ble_host_hci_evt_dispatch_entry *ble_host_hci_evt_dispatch_find(uint8_t event_code)
{
    const struct ble_host_hci_evt_dispatch_entry *entry;
    unsigned int                                  i;

    for (i = 0; i < BLE_HS_HCI_EVT_DISPATCH_SZ; i++) {
        entry = ble_host_hci_evt_dispatch + i;
        if (entry->event_code == event_code) {
            return entry;
        }
    }

    return NULL;
}

#if 1//(LL_ACL_CEN_EN || LL_ACL_PER_EN)

static int ble_host_hci_evt_disconn_complete(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_disconn_cmp *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    if (ev->conn_handle & 0xc0) {
        extern int blt_gap_conn_terminate_handler(u16 connHandle, u8 * p);
        blt_gap_conn_terminate_handler(ev->conn_handle, NULL);
        BLE_HOST_HCI_COMMON_CMD_INFO("******* blt_gap_conn_terminate_handler******* ");
    }

    extern int blc_hci_send_event1(u32 h, u8 * para, int n);
    // HCI_FLAG_EVENT_BT_STD (1 << 25)  HCI_EVT_DISCONNECTION_COMPLETE 0x05
    blc_hci_send_event1((1 << 25) | 0x05, (uint8_t *) ev, len);

    ble_host_conn_remove_conn(ev->conn_handle, ev->reason);
    return 0;
}

static int ble_host_hci_evt_encrypt_change(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_enrypt_chg *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE SMP layer of the encryption change event.  */
    extern int blt_gap_ll_enc_done_handler(u16 connHandle, u8 status, u8 enc_enable);
    blt_gap_ll_enc_done_handler(ev->connection_handle, ev->status, ev->enabled);
    //tlkapi_printf(1,"ble_host_hci_evt_encrypt_change");
    BLE_HOST_HCI_COMMON_CMD_INFO("blt_smp_llEncryptionDone");
    return 0;
}
#endif

static int ble_host_hci_evt_hw_error(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_hw_error *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Hardware error handling.  */

    return 0;
}

#if 1//(LL_ACL_CEN_EN || LL_ACL_PER_EN)

static int ble_host_hci_evt_enc_key_refresh(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_enc_key_refresh *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE SMP layer of the encryption key refresh event.  */
    BLE_HOST_HCI_COMMON_CMD_INFO("enc_key_refresh: %s", hex_to_str(data, len));
    return 0;
}
#endif


static int ble_host_hci_evt_num_completed_pkts(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_num_comp_pkts *ev = data;
    struct ble_host_conn *conn;
    uint16_t                               num_pkts;
    int                                    i;

    if (len != sizeof(*ev) + (ev->count * sizeof(ev->completed[0]))) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    for (i = 0; i < ev->count; i++) {
        num_pkts = ev->completed[i].packets;

        if (num_pkts > 0) {
            ble_host_lock();
            conn = ble_host_conn_find_by_conn_handle(ev->completed[i].handle);
            if (conn != NULL) {
                if (conn->cfc_outstanding_pkts < num_pkts) {
                    ble_host_reset(BLE_HOST_ERR_CONTROLLER);
                } else {
                    conn->cfc_outstanding_pkts -= num_pkts;
                }

                ble_host_hci_add_acl_avail_pkts(num_pkts);
            }
            ble_host_unlock();
        }
    }

    /* If any transmissions have stalled, wake them up now. */

    return 0;
}

static int ble_host_hci_evt_le_meta(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_le_meta *ev = data;

    return ble_host_hci_le_evt_dispatch_process(ev, len);
}

#if (defined(BLE_HCI_VS_EVT_ENABLE))

static int ble_host_hci_evt_vs(uint8_t event_code, const void *data, uint32_t len)
{
    const struct ble_hci_ev_vs *ev = data;

    if (len < sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Handle vendor-specific HCI events.  */
    switch (ev->id) {
    case BLE_HCI_VS_SUBEV_XXX0:
        //struct ble_hci_ev_vs_xxx0 *ev_xxx0 = (void *) ev->data;
        //(void) ev_xxx0;
        break;
    case BLE_HCI_VS_SUBEV_XXX1:
        break;
    }
    return 0;
}
#endif

static int ble_host_hci_evt_remote_version(uint8_t event_code, const void *data, uint32_t len)
{
    (void) event_code;
    const struct ble_hci_ev_rd_rem_ver_info_cmp *ev = data;

    if (len != sizeof(*ev)) {
        return BLE_HOST_ERR_CONTROLLER;
    }

    /* TODO: Notify the BLE GAP layer of the PHY update complete event.  */
    ble_host_gap_read_remote_version(ev->conn_handle, ev->version);
    return 0;
}

int ble_host_hci_evt_dispatch_process(void *ev)
{
    int                                           rc;
    struct ble_hci_evt *p_hci_event = ev;
    const struct ble_host_hci_evt_dispatch_entry *entry;

    entry = ble_host_hci_evt_dispatch_find(p_hci_event->opcode);
    if (entry == NULL) {
        rc = BLE_HOST_ERR_NOTSUP;
    } else {
        rc = entry->cb(p_hci_event->opcode, p_hci_event->data, p_hci_event->length);
    }

    BLE_HOST_HCI_COMMON_EVT_INFO("dispatch event opcode 0x%02x", p_hci_event->opcode, hex_to_str(p_hci_event->data, p_hci_event->length));

    return rc;
}

#pragma GCC diagnostic pop
