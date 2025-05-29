/********************************************************************************************************
 * @file    ble_host.c
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
#include <sys/queue.h>

#include "common/types.h"

#include "../ble_common.h"

#include "inc/ble_host.h"
#include "inc/ble_host_sal.h"
#include "inc/ble_host_log.h"
#include "inc/ble_host_internal.h"

#include "hci/inc/ble_hci.h"

#include "hci/inc/ble_hci_cmd.h"
#include "hci/inc/ble_hci_evt.h"
#include "hci/le_cmd/inc/hci_cmd_le_misc.h"

#include "hci/cmd/inc/hci_cmd_bb.h"
#include "hci/cmd/inc/hci_cmd_info_param.h"
#include "hci/cmd/inc/hci_cmd_vendor.h"
struct ble_host_acl_conn {
    TAILQ_ENTRY(ble_host_acl_conn) next;
    struct ble_host_conn conn_env;
};

static TAILQ_HEAD(, ble_host_acl_conn) s_ble_host_acl_conns_head;

static struct ble_host_info s_ble_host_info;

static const struct ble_host_acl_conn_callbacks *s_acl_conn_callbacks[BLE_HOST_USER_DATA_ID_MAX] = { NULL };

/********** BLE Host stack initialization and memory management APIs. **********/
static void ble_host_init_all_sal_module(void)
{
    ble_host_sal_platform_time_init();
    ble_host_sal_timer_init();
    ble_host_sal_log_init(BLE_HOST_LOG_LEVEL_NONE);
    ble_host_sal_hci_init();
}

/**
 *   @brief this function is used to initialize the BLE Host stack.
 *
 *   @param[in] p_host_memory: pointer to the Host memory pool.
 *   @param[in] size: size of the Host memory pool.
 *
 *   @return None.
 */
void ble_host_init(uint8_t *p_host_memory, uint32_t size)
{
    ble_host_init_all_sal_module();

    ble_host_sal_memory_pool_init(p_host_memory, size);
    s_ble_host_info.hs_mpool = p_host_memory;
    TAILQ_INIT(&s_ble_host_acl_conns_head);

    /* Not support SMP feature */
//    s_ble_host_info.sm_dft_settings.IO_capability = SMP_IO_CAP_NO_INPUT_NO_OUTPUT;
//    s_ble_host_info.sm_dft_settings.passKeyEntryDftTK = 123456;
//    s_ble_host_info.sm_dft_settings.MITM_protection = 0;
//    s_ble_host_info.sm_dft_settings.bonding_mode = 0;
//    s_ble_host_info.sm_dft_settings.ecdh_debug_mode = 0;
//    s_ble_host_info.sm_dft_settings.keypress_ntf_enable = 0;
//    s_ble_host_info.sm_dft_settings.oob_enable = 0;
//    s_ble_host_info.sm_dft_settings.security_level = 0;
}

/**
 *   @brief this function is used to allocate memory from Host memory pool.
 *
 *   @param[in] size: size of the memory to be allocated.
 *   @param[in] type_id: type ID of the memory to be allocated.
 *
 *   @return pointer to the allocated memory.
 */
void *ble_host_malloc(uint32_t size, uint16_t type_id)
{
    return ble_host_sal_memory_malloc(s_ble_host_info.hs_mpool, size, type_id);
}

/**
 *   @brief this function is used to free memory from L2CAP memory pool.
 *
 *   @param[in] ptr: pointer to the memory to be freed.
 *
 *   @return none.
 */
void ble_host_free(void *ptr)
{
    ble_host_sal_memory_free(s_ble_host_info.hs_mpool, ptr);
}
/********************* BLE Host stack OS APIs. *******************/
/**
 *   @brief this function is used to lock the BLE host.
 *
 *   @return none.
 */
void ble_host_lock(void)
{
}

/**
 *   @brief this function is used to unlock the BLE host.
 *
 *   @return none.
 */
void ble_host_unlock(void)
{
}

/**
 *   @brief this function is used to reset the BLE host.
 *
 *   @param[in] reason: reason for the reset.
 *
 *   @return none.
 */
void ble_host_reset(int reason)
{
    (void) reason;
}

/****************BLE Host stack ACL connection management APIs. *****************/
/** BLE Host connection device management APIs.
 * These APIs are used to manage the list of active BLE Host connections.
 */
static struct ble_host_acl_conn *ble_host_acl_conn_find_by_conn_handle(uint16_t conn_handle)
{
    struct ble_host_acl_conn *ble_conn;

    TAILQ_FOREACH(ble_conn, &s_ble_host_acl_conns_head, next)
    {
        if (ble_conn->conn_env.conn_handle == conn_handle) {
            return ble_conn;
        }
    }

    return NULL;
}

/**
 *   @brief this function is used to register the callbacks for ACL connection events(BLE Host).
 *
 *   @param[in] id: Host ACL connection user data ID.
 *   @param[in] cb: pointer to the callback structure.
 *
 *   @return none.
 */
void ble_host_acl_conn_register_user_data(enum ble_host_user_data_id id, const struct ble_host_acl_conn_callbacks *cb)
{
    if (id < BLE_HOST_USER_DATA_ID_MAX && cb != NULL) {
        s_acl_conn_callbacks[id] = cb;
    }
}

/**
 *   @brief this function is used to register the callbacks for ACL connection events(BLE Host) by index.
 *
 *   @param[in] id: Host ACL connection user data ID.
 *
 *   @return none.
 */
void ble_host_acl_conn_unregister_user_data(enum ble_host_user_data_id id)
{
    if (id < BLE_HOST_USER_DATA_ID_MAX) {
        s_acl_conn_callbacks[id] = NULL;
    }
}

static int ble_host_get_id_addr(uint8_t id_addr_type, const uint8_t **out_id_addr, int *out_is_nrpa)
{
    const uint8_t *id_addr;
    int nrpa;

    switch (id_addr_type) {
    case BLE_ADDR_PUBLIC:
        id_addr = s_ble_host_info.hs_id_pub;
        nrpa = 0;
        break;

    case BLE_ADDR_RANDOM:
        id_addr = s_ble_host_info.hs_id_rnd;
        nrpa = (id_addr[5] & 0xc0) == 0;
        break;

    default:
        return BLE_HOST_ERR_PARM;
    }

    uint8_t empty_addr[6] = { 0, 0, 0, 0, 0, 0 };
    if (memcmp(id_addr, empty_addr, 6) == 0) {
        return BLE_HOST_ERR_NOADDR;
    }

    if (out_id_addr != NULL) {
        *out_id_addr = id_addr;
    }
    if (out_is_nrpa != NULL) {
        *out_is_nrpa = nrpa;
    }

    return BLE_HOST_ERR_SUCC;
}

static void ble_host_update_conn_addrs(struct ble_host_conn *conn)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    /* Determine our address information. */
    conn->own_id_addr.type = (conn->own_addr_type & BLE_ADDR_TYPE_RANDOM_MASK) ? BLE_ADDR_RANDOM : BLE_ADDR_PUBLIC;

    const uint8_t *own_id_addr_val;

    /* With Extended Advertising, the enabled random address for peripheral connection is per advertising instance,
     * so we need to use the address from the connection object instead of the default address.
     */
    if ((conn->role == BLE_HCI_CONN_ROLE_PERIPHERAL) && conn->own_id_addr.type == BLE_ADDR_RANDOM) {
        own_id_addr_val = conn->own_rnd_addr;
    } else {
        int rc = ble_host_get_id_addr(conn->own_id_addr.type, &own_id_addr_val, NULL);
        BLE_HOST_SAL_ASSERT(rc == 0);
    }

    memcpy(conn->own_id_addr.val, (void *) own_id_addr_val, 6);

    uint8_t empty_addr[6] = { 0, 0, 0, 0, 0, 0 };

    if (memcmp(conn->own_rpa_addr.val, empty_addr, 6) == 0) {
        conn->own_ota_addr = conn->own_id_addr;
    } else {
        conn->own_ota_addr = conn->own_rpa_addr;
    }


    /* Determine peer address information. */
    conn->peer_id_addr = conn->peer_addr; //peer_ota_addr's value copy from connection complete event
    conn->peer_ota_addr = conn->peer_addr;
    switch (conn->peer_ota_addr.type) {
    case BLE_ADDR_PUBLIC:
    case BLE_ADDR_RANDOM:
        break;
    case BLE_ADDR_PUBLIC_ID:
        conn->peer_id_addr.type = BLE_ADDR_PUBLIC;
        conn->peer_ota_addr = conn->peer_rpa_addr;
        break;

    case BLE_ADDR_RANDOM_ID:
        conn->peer_id_addr.type = BLE_ADDR_RANDOM;
        conn->peer_ota_addr = conn->peer_rpa_addr;
        break;
    }
}
static void ble_host_conn_insert_new_conn(struct ble_host_acl_conn *ble_conn)
{
    TAILQ_INSERT_TAIL(&s_ble_host_acl_conns_head, ble_conn, next);

    for (int i = 0; i < BLE_HOST_USER_DATA_ID_MAX; i++) {
        if (s_acl_conn_callbacks[i] != NULL) {
            s_acl_conn_callbacks[i]->connected(&ble_conn->conn_env);
        }
    }
}

static void ble_host_conn_remove_old_conn(struct ble_host_acl_conn *ble_conn, uint8_t reason)
{
    for (int i = 0; i < BLE_HOST_USER_DATA_ID_MAX; i++) {
        if (s_acl_conn_callbacks[i] != NULL) {
            s_acl_conn_callbacks[i]->disconnected(&ble_conn->conn_env, reason);
        }
    }

    TAILQ_REMOVE(&s_ble_host_acl_conns_head, ble_conn, next);

    ble_host_free(ble_conn);
}

/**
 *   @brief this function is used to insert a connection complete event into the list of active connections.
 *
 *   @param[in] conn_complete: pointer to the connection complete event.
 *
 *   @return none.
 */
void ble_host_conn_insert_conn_complete(struct ble_acl_conn_complete *conn_complete)
{
    struct ble_host_acl_conn *ble_conn = NULL;
    if ((ble_conn = ble_host_acl_conn_find_by_conn_handle(conn_complete->conn_handle)) != NULL) {
        BLE_HOST_ACL_DEV_ERROR("conn complete: already exists");
        return;
    }

    ble_conn = ble_host_malloc(sizeof(struct ble_host_acl_conn), BLE_HOST_MALLOC_ACL_MANAGER);
    if (ble_conn == NULL) {
        BLE_HOST_ACL_DEV_ERROR("conn complete: malloc failed");
        return;
    }
    ble_host_lock();
    struct ble_host_conn *conn = &ble_conn->conn_env;

    memset(conn, 0, sizeof(struct ble_host_conn));

    conn->conn_handle = conn_complete->conn_handle;
    conn->role = conn_complete->role;
    conn->peer_addr.type = conn_complete->peer_addr_type;
    memcpy(conn->peer_addr.val, conn_complete->peer_addr, 6);
    conn->conn_interval = conn_complete->conn_interval;
    conn->conn_latency = conn_complete->conn_latency;
    conn->supervision_timeout = conn_complete->supervision_timeout;
    conn->master_clock_acc = conn_complete->master_clock_acc;

    conn->own_rpa_addr.type = BLE_ADDR_RANDOM;
    memcpy(conn->own_rpa_addr.val, conn_complete->local_rpa, 6);

    /* If peer RPA is not set in the event and peer address
     * is RPA then store the peer RPA address so when the peer
     * address is resolved, the RPA is not forgotten.
     */
    if (memcmp(BLE_ADDR_EMPTY->val, conn_complete->peer_rpa, 6) == 0) {
        if (IS_RESOLVABLE_PRIVATE_ADDR(conn->peer_addr.type, conn->peer_addr.val)) {
            conn->peer_rpa_addr = conn->peer_addr;
        }
    } else {
        conn->peer_rpa_addr.type = BLE_ADDR_RANDOM;
        memcpy(conn->peer_rpa_addr.val, conn_complete->peer_rpa, 6);
    }

    /* Set default security settings, but can be updated later by the application.
     * TODO: SM and ACL structure control block cross-layer coupling, using the QiHang said
     * usr_data pointer registration mechanism to solve the problem of cross-layer coupling. */
    conn->sm_settings = s_ble_host_info.sm_dft_settings;

    /* Update the connection addresses: id, ota address */
    ble_host_update_conn_addrs(conn);

    ble_host_conn_insert_new_conn(ble_conn);

    ble_host_unlock();
}

/**
 *   @brief this function is used to remove a connection from the list of active connections.
 *
 *   @param[in] conn_handle: connection handle of the connection to be removed.
 *   @param[in] reason: reason for the disconnection.
 *
 *   @return none.
 */
void ble_host_conn_remove_conn(uint16_t conn_handle, uint8_t reason)
{
    struct ble_host_acl_conn *ble_conn = NULL;

    if ((ble_conn = ble_host_acl_conn_find_by_conn_handle(conn_handle)) == NULL) {
        BLE_HOST_ACL_DEV_ERROR("conn disconnect: not found");
        return;
    }
    ble_host_lock();
    ble_host_conn_remove_old_conn(ble_conn, reason);
    ble_host_unlock();
}

/** Finds a connection by connection handle.*/
struct ble_host_conn *ble_host_conn_find_by_conn_handle(uint16_t conn_handle)
{
    struct ble_host_acl_conn *ble_conn = ble_host_acl_conn_find_by_conn_handle(conn_handle);

    return (ble_conn != NULL) ? &ble_conn->conn_env : NULL;
}

bool ble_host_conn_is_connected(uint16_t conn_handle)
{
    return ble_host_conn_find_by_conn_handle(conn_handle) != NULL;
}

bool ble_host_conn_is_peripheral(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    // 0x01 is acl peripheral role.
    return (conn != NULL) ? (conn->role == 0x01) : false;
}

bool ble_host_conn_is_central(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    // 0x00 is acl central role.
    return (conn != NULL) ? (conn->role == 0x00) : false;
}

/************** BLE Host config and query controller information APIs. *****************/
void ble_host_hci_add_acl_avail_pkts(int16_t delta)
{
    s_ble_host_info.hs_hci_acl_avail_pkts += delta;
}

void ble_host_hci_add_iso_avail_pkts(int16_t delta)
{
    s_ble_host_info.hs_hci_iso_avail_pkts += delta;
}

void ble_host_hci_get_bd_addr(uint8_t *addr)
{
    if (addr) {
        memcpy(addr, s_ble_host_info.hs_id_pub, 6);
    }
}

uint16_t ble_host_hci_max_acl_payload_sz(void)
{
    return s_ble_host_info.hs_hci_acl_buf_sz;
}

uint16_t ble_host_hci_max_iso_payload_sz(void)
{
    return s_ble_host_info.hs_hci_iso_buf_sz;
}

bool ble_host_hci_get_le_supported_controller(void)
{
    return s_ble_host_info.le_supported_controller;
}

static void ble_host_read_local_info(void)
{
    // read local version information.
    struct ble_hci_ip_rd_local_ver_rp     loc_ver_info;
    ble_host_hci_read_local_version_info(&loc_ver_info);
    s_ble_host_info.hs_hci_version = loc_ver_info.hci_ver;

    // read local supported Features(LMP features)
    struct ble_hci_ip_rd_loc_supp_feat_rp local_supp_feat;
    ble_host_hci_read_local_supported_features(&local_supp_feat);
    s_ble_host_info.le_supported_controller = (local_supp_feat.features & (1ULL << 38)) != 0;

    // read local supported commands
    struct ble_hci_ip_rd_loc_supp_cmd_rp  local_supp_cmd;
    ble_host_hci_read_local_supported_commands(&local_supp_cmd);
    // memcpy(s_ble_host_info.supported_commands, local_supp_cmd.commands, 64);

    // read local BD address
    struct ble_hci_ip_rd_bd_addr_rp bd_addr;
    ble_host_hci_read_bd_address(&bd_addr);
    memcpy(s_ble_host_info.hs_id_pub, bd_addr.addr, 6);
}

static void ble_host_read_le_info(void)
{
    // LE set event mask.

    /**
     * Enable the following events:
     *     0x0000000000000001 HCI_LE_EVT_MASK_CONNECTION_COMPLETE
     *     0x0000000000000002 HCI_LE_EVT_MASK_ADVERTISING_REPORT
     *     0x0000000000000004 HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE
     *     0x0000000000000008 HCI_LE_EVT_MASK_READ_REMOTE_FEATURES_COMPLETE
     *     0x0000000000000010 HCI_LE_EVT_MASK_LONG_TERM_KEY_REQUEST
     *     0x0000000000000080 HCI_LE_EVT_MASK_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE
     *     0x0000000000000100 HCI_LE_EVT_MASK_GENERATE_DHKEY_COMPLETE
     */
    struct ble_hci_le_set_event_mask_cp le_event_mask = {
        //.event_mask = 0x3FFFFFFFF,  //bit0 to bit34
        .event_mask = 0x000000000000019F,
    };
    ble_host_hci_le_set_event_mask(&le_event_mask);

    // LE read local supported features.
    struct ble_hci_le_rd_loc_supp_feat_rp le_local_supp_feat;
    ble_host_hci_le_read_local_supported_features(&le_local_supp_feat);
    s_ble_host_info.le_supported_feature = le_local_supp_feat.features;

    // LE read buffer size.
    struct ble_hci_le_rd_buf_size_v2_rp   le_buf_size_v2;
    ble_host_hci_le_read_buffer_size_v2(&le_buf_size_v2);
    s_ble_host_info.hs_hci_max_pkts = le_buf_size_v2.data_packets;
    s_ble_host_info.hs_hci_acl_avail_pkts = le_buf_size_v2.data_packets;
    s_ble_host_info.hs_hci_acl_buf_sz = le_buf_size_v2.data_len;
    s_ble_host_info.hs_hci_iso_avail_pkts = le_buf_size_v2.iso_data_packets;
    s_ble_host_info.hs_hci_iso_buf_sz = le_buf_size_v2.iso_data_len;

    struct ble_hci_le_set_host_feature_cp le_host_feature;
    /**
     * Bit number iso stream host: 32
     */
    le_host_feature.bit_num = 0x20;
    le_host_feature.bit_val = 0x01;
    ble_host_hci_le_set_host_feature(&le_host_feature);
}



void ble_host_hci_read_controller_basic_info(void)
{
    // Call the functions with appropriate parameters

    /**
     * Enable the following events:
     *     0x0000000000000010 Disconnection Complete Event
     *     0x0000000000000080 Encryption Change Event
     *     0x0000000000000800 Read Remote Version Information Complete Event
     *     0x0000800000000000 Encryption Key Refresh Complete Event
     *     0x2000000000000000 LE Meta-Event
     */
    struct ble_hci_cb_set_event_mask_cp event_mask = {
        .event_mask = 0x2000800000000890
    };
    ble_host_hci_set_event_mask(&event_mask);

    /**
     * Enable the following events:
     *     0x0000000000800000 Authenticated Payload Timeout Event
     *     0x0000000002000000 Encryption Change event
     */
    struct ble_hci_cb_set_event_mask2_cp event_mask2 = {
        .event_mask2 = 0x0000000002800000,
    };
    ble_host_hci_set_event_mask2(&event_mask2);

#if 0
    /**
     * Enable the following events:
     *     0x0000000000000001 HCI_LE_EVT_MASK_CONNECTION_COMPLETE
     *     0x0000000000000002 HCI_LE_EVT_MASK_ADVERTISING_REPORT
     *     0x0000000000000004 HCI_LE_EVT_MASK_CONNECTION_UPDATE_COMPLETE
     *     0x0000000000000008 HCI_LE_EVT_MASK_READ_REMOTE_FEATURES_COMPLETE
     *     0x0000000000000010 HCI_LE_EVT_MASK_LONG_TERM_KEY_REQUEST
     *     0x0000000000000080 HCI_LE_EVT_MASK_READ_LOCAL_P256_PUBLIC_KEY_COMPLETE
     *     0x0000000000000100 HCI_LE_EVT_MASK_GENERATE_DHKEY_COMPLETE
     */
    struct ble_hci_le_set_event_mask_cp   le_event_mask = {
        //.event_mask = 0x7FFFFFFFFFFFFFFF,
        .event_mask = 0x000000000000019F,
    };
    ble_host_hci_le_set_event_mask(&le_event_mask);
#endif
    // read local information version features, commands, and BD address.
    ble_host_read_local_info();

    // read LE information, buffer size, supported features, and supported commands.
    ble_host_read_le_info();
}

uint8_t ble_host_buffer[1024]; //TODO:
uint8_t ble_l2cap_buffer[1024];
uint8_t ble_hci_buffer[2048];
uint8_t ble_gatt_buffer[1024 * 2];
uint8_t ble_prf_buffer[1024];

// todo :it will be deleted later.
#include "l2cap/inc/ble_l2cap.h"
#include "l2cap/att/inc/ble_att.h"
#include "l2cap/att/inc/ble_att_uuid.h"
#include "hci/inc/ble_hci.h"
#include "hci/inc/ble_hci_evt.h"
#include "hci/inc/ble_acl_data.h"
#include "hci/inc/ble_iso_data.h"
#include "hci/inc/ble_hci_cmd.h"
#include "gatt/inc/gatt.h"
#include "gatt/gattc/inc/gattc.h"
#include "gatt/sdp/inc/ble_sdp.h"
#include "gatt/sdp/inc/ble_ssdp.h"
#include "profile/inc/profile.h"

#include "common/utility.h"
static void ble_prf_event_callback(uint16_t conn_handle, uint8_t event_id, const void *event_msg)
{
//    extern void tlk_printf(const char *format, ...);
//    tlk_printf("conn:%d, event:%d, msg:%s", conn_handle, event_id, hex_to_str(event_msg, 16));
}

void ble_host_v1_init(const uint8_t *mac, const uint8_t *mac_random)
{
    ble_host_init(ble_host_buffer, sizeof(ble_host_buffer));
    ble_host_l2cap_init(ble_l2cap_buffer, sizeof(ble_l2cap_buffer));

    ble_host_hci_init(ble_hci_buffer, sizeof(ble_hci_buffer));
    ble_host_gatt_init(ble_gatt_buffer, sizeof(ble_gatt_buffer));

    ble_host_gattc_init();
    ble_host_gatt_ssdp_init();

    struct ble_prf_init_param prf_init_param = {
        .event_cb = ble_prf_event_callback,
        .p_prf_memory = ble_prf_buffer,
        .prf_memory_size = sizeof(ble_prf_buffer),
    };
    ble_prf_initial(&prf_init_param);

    ble_host_hci_register_rx_event_callback(ble_host_hci_evt_dispatch_process);
    ble_host_hci_register_rx_acl_data_callback(ble_host_receive_ble_acl_data);
    //ble_host_hci_register_rx_iso_data_callback(ble_host_receive_ble_iso_data);


    extern void blt_att_l2capAttRxHandler1(struct ble_host_conn *p_conn, uint16_t len, uint8_t * p_packet);
    ble_host_l2cap_register_callbacks(LE_L2CAP_CID_ATT, NULL, blt_att_l2capAttRxHandler1);

    extern void blt_att_l2capSmpRxHandler(struct ble_host_conn *p_conn, uint16_t len, uint8_t * p_packet);
    ble_host_l2cap_register_callbacks(LE_L2CAP_CID_SMP, NULL, blt_att_l2capSmpRxHandler);

    struct att_initial_param att_init_param = {
        .mtu = 64
    };
    ble_host_att_init(&att_init_param);

    /* Controller Reset first */
    // ble_host_hci_send_reset();

    /* Set BLE PUBLIC MAC address (use vendor HCI CMD) */
    if (mac != NULL) {
        ble_host_hci_set_le_addr(mac);
    }
    /* Set BLE RANDOM MAC address */
    if (mac_random != NULL) {
        struct ble_hci_le_set_rand_addr_cp le_set_rand_addr;
        memcpy(le_set_rand_addr.addr, mac_random, 6);
        ble_host_hci_le_set_random_address(&le_set_rand_addr);
    }

    /* get controller information */
    ble_host_hci_read_controller_basic_info();


}

void ble_host_hci_set_le_addr(const uint8_t *mac)
{
    const struct ble_hci_ip_rd_bd_addr_rp *p_bd_addr = (const struct ble_hci_ip_rd_bd_addr_rp *) mac;
    ble_host_hci_vendor_set_bd_address(p_bd_addr);
}


extern void ble_host_hci_send_acl_data_sync(void);

void ble_host_v1_main_loop(void)
{
    ble_host_hci_rx_task();
    ble_host_l2cap_tx_task();
    ble_host_hci_send_acl_data_sync();
}

