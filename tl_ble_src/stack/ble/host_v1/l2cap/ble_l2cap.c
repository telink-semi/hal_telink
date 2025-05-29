#include <string.h>
#include <sys/queue.h>

#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host.h"
#include "../inc/ble_host_sal.h"

#include "../hci/inc/ble_acl_data.h"

#include "inc/ble_l2cap.h"
#include "inc/ble_l2cap_log.h"
#include "inc/ble_l2cap_interface.h"

static void ble_host_l2cap_receive_l2cap_data(struct ble_host_conn *conn, bool is_start, const uint8_t *p_data, uint16_t data_len);
static void ble_host_l2cap_acl_connected(struct ble_host_conn *conn);
static void ble_host_l2cap_acl_disconnected(struct ble_host_conn *conn, uint8_t reason);

#define BLE_L2CAP_RX_MALLOC(size)        ble_host_l2cap_malloc(size, BLE_HOST_L2CAP_MALLOC_RX_BUFFER)
#define BLE_L2CAP_RX_FREE(ptr)           ble_host_l2cap_free(ptr)

#define BLE_L2CAP_TX_MALLOC(size)        ble_host_l2cap_malloc(size, BLE_HOST_L2CAP_MALLOC_TX_BUFFER)
#define BLE_L2CAP_TX_FREE(ptr)           ble_host_l2cap_free(ptr)

#define BLE_L2CAP_CONN_INFO_MALLOC(size) ble_host_l2cap_malloc(size, BLE_HOST_L2CAP_MALLOC_CONN_INFO)
#define BLE_L2CAP_CONN_INFO_FREE(ptr)    ble_host_l2cap_free(ptr)

struct ble_host_l2cap_tx_fifo {
    STAILQ_ENTRY(ble_host_l2cap_tx_fifo) next;
    uint16_t conn_handle;
    struct ble_host_l2cap_tx_packet tx_packet;
};

struct ble_host_l2cap_common_info {
    uint8_t *memory_addr;
    uint32_t curr_tx_fifo;
    STAILQ_HEAD(, ble_host_l2cap_tx_fifo) tx_fifo;
};

struct l2cap_callbacks {
    // ATT Layer callback
    l2cap_ctrl_callback_t     att_ctrl_callback;
    l2cap_data_callback_t     att_data_callback;
    // SMP Layer callback
    l2cap_ctrl_callback_t     smp_ctrl_callback;
    l2cap_data_callback_t     smp_data_callback;
    // Signaling Layer callback
    l2cap_ctrl_callback_t     signaling_ctrl_callback;
    l2cap_data_callback_t     signaling_data_callback;
    // CoC Layer callback
    l2cap_cid_ctrl_callback_t cid_ctrl_callback;
    l2cap_cid_data_callback_t cid_data_callback;
};

static struct ble_host_l2cap_common_info s_l2cap_common_info = {
    .memory_addr = NULL,
    .tx_fifo = STAILQ_HEAD_INITIALIZER(s_l2cap_common_info.tx_fifo),
};

static struct l2cap_callbacks s_l2cap_callbacks = {
    // ATT Layer callback
    .att_ctrl_callback = NULL,
    .att_data_callback = NULL,
    // SMP Layer callback
    .smp_ctrl_callback = NULL,
    .smp_data_callback = NULL,
    // Signaling Layer callback
    .signaling_ctrl_callback = NULL,
    .signaling_data_callback = NULL,
    // CoC Layer callback
    .cid_ctrl_callback = NULL,
    .cid_data_callback = NULL,
};

static const struct ble_host_acl_conn_callbacks s_l2cap_acl_conn_callbacks = {
    .connected = ble_host_l2cap_acl_connected,
    .disconnected = ble_host_l2cap_acl_disconnected,
};

/**
 *   @brief this function is used to initialize the L2CAP module.
 *
 *   @param[in] p_l2cap_memory: pointer to the L2CAP memory pool.
 *   @param[in] size: size of the L2CAP memory pool.
 *
 *   @return none.
 */
void ble_host_l2cap_init(uint8_t *p_l2cap_memory, uint32_t size)
{
    ble_host_sal_memory_pool_init(p_l2cap_memory, size);
    s_l2cap_common_info.memory_addr = p_l2cap_memory;

    ble_host_acl_data_init(ble_host_l2cap_receive_l2cap_data);

    ble_host_acl_conn_register_user_data(BLE_HOST_L2CAP_USER_ID, &s_l2cap_acl_conn_callbacks);
}

/**
 *   @brief this function is used to allocate memory from L2CAP memory pool.
 *
 *   @param[in] size: size of the memory to be allocated.
 *   @param[in] type_id: type ID of the memory to be allocated.
 *
 *   @return pointer to the allocated memory.
 */
void *ble_host_l2cap_malloc(uint32_t size, uint16_t type_id)
{
    return ble_host_sal_memory_malloc(s_l2cap_common_info.memory_addr, size, type_id);
}

/**
 *   @brief this function is used to free memory from L2CAP memory pool.
 *
 *   @param[in] ptr: pointer to the memory to be freed.
 *
 *   @return none.
 */
void ble_host_l2cap_free(void *ptr)
{
    ble_host_sal_memory_free(s_l2cap_common_info.memory_addr, ptr);
}

/**
 *   @brief this function is used to register ATT, SMP, signaling and callbacks.
 *
 *   @param[in] cid: channel ID, LE_L2CAP_CID_ATT, LE_L2CAP_CID_SIGNALING, LE_L2CAP_CID_SMP.
 *   @param[in] ctrl_callback: control callback function.
 *   @param[in] data_callback: data callback function.
 *
 *   @return none.
 */
void ble_host_l2cap_register_callbacks(uint8_t cid, l2cap_ctrl_callback_t ctrl_callback, l2cap_data_callback_t data_callback)
{
    switch (cid) {
    case LE_L2CAP_CID_ATT:
        s_l2cap_callbacks.att_ctrl_callback = ctrl_callback;
        s_l2cap_callbacks.att_data_callback = data_callback;
        break;
    case LE_L2CAP_CID_SMP:
        s_l2cap_callbacks.smp_ctrl_callback = ctrl_callback;
        s_l2cap_callbacks.smp_data_callback = data_callback;
        break;
    case LE_L2CAP_CID_SIGNALING:
        s_l2cap_callbacks.signaling_ctrl_callback = ctrl_callback;
        s_l2cap_callbacks.signaling_data_callback = data_callback;
        break;
    default:
        break;
    }
}

/**
 *   @brief this function is used to register CoC data callback.
 *
 *   @param[in] ctrl_callback: control callback function.
 *   @param[in] data_callback: data callback function.
 *
 *   @return none.
 */
void ble_host_l2cap_register_cid_data_callback(l2cap_cid_ctrl_callback_t ctrl_callback, l2cap_cid_data_callback_t data_callback)
{
    s_l2cap_callbacks.cid_ctrl_callback = ctrl_callback;
    s_l2cap_callbacks.cid_data_callback = data_callback;
}

static void ble_host_l2cap_report_event(struct ble_host_conn *conn, uint8_t event, const void *param)
{
    if (s_l2cap_callbacks.att_ctrl_callback != NULL) {  // if register ATT callback, call ATT callback.
        s_l2cap_callbacks.att_ctrl_callback(conn, event, param);
    }

    if (s_l2cap_callbacks.smp_ctrl_callback != NULL) {   // if register SMP callback, call SMP callback.
        s_l2cap_callbacks.smp_ctrl_callback(conn, event, param);
    }

    if (s_l2cap_callbacks.signaling_ctrl_callback != NULL) {    // if register signaling callback, call signaling callback.
        s_l2cap_callbacks.signaling_ctrl_callback(conn, event, param);
    }

    if (s_l2cap_callbacks.cid_ctrl_callback != NULL) {  // if register CoC callback, call CoC callback.
        s_l2cap_callbacks.cid_ctrl_callback(conn, 0xFFFF, event, param);
    }
}

struct ble_l2cap_conn_info *ble_host_l2cap_find_conn_info_by_handle(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    return conn == NULL ? NULL : conn->user_data[BLE_HOST_L2CAP_USER_ID];
}

static void ble_host_l2cap_acl_connected(struct ble_host_conn *conn)
{
    BLE_HOST_L2CAP_COMMON_INFO("L2CAP ACL connected, conn handle:0x%03x", conn->conn_handle);

    struct ble_l2cap_conn_info *p_l2cap_conn_info = BLE_L2CAP_CONN_INFO_MALLOC(sizeof(struct ble_l2cap_conn_info));
    if (p_l2cap_conn_info == NULL) {
        BLE_HOST_L2CAP_COMMON_ERROR("L2CAP malloc connection info failed.");
        return;
    }

    memset(p_l2cap_conn_info, 0, sizeof(struct ble_l2cap_conn_info));
    conn->user_data[BLE_HOST_L2CAP_USER_ID] = p_l2cap_conn_info;

    ble_host_l2cap_report_event(conn, L2CAP_EVT_ACL_CONNECTED, NULL);
}

static void ble_host_l2cap_acl_disconnected(struct ble_host_conn *conn, uint8_t reason)
{
    BLE_HOST_L2CAP_COMMON_INFO("L2CAP ACL disconnected, conn handle:0x%03x, reason:%d", conn->conn_handle, reason);

    struct ble_l2cap_conn_info *p_l2cap_conn_info = conn->user_data[BLE_HOST_L2CAP_USER_ID];

    if (p_l2cap_conn_info == NULL) {
        BLE_HOST_L2CAP_COMMON_ERROR("L2CAP connection info not found, conn handle:0x%03x", conn->conn_handle);
        return;
    }

    ble_host_l2cap_report_event(conn, L2CAP_EVT_ACL_DISCONNECTED, &reason);

    BLE_L2CAP_CONN_INFO_FREE(p_l2cap_conn_info);
}

static void ble_host_clear_reassembled_data(struct ble_host_l2cap_rx_packet *p_rx_packet)
{
    BLE_L2CAP_RX_FREE(p_rx_packet->packet);
    p_rx_packet->packet = NULL;
    p_rx_packet->offset = 0;
    p_rx_packet->total_length = 0;
}

/**
 *   @brief this function is used to reassemble L2CAP data(only for BLE ACL connection).
 *
 *   @param[in] conn: pointer to the connection object.
 *   @param[in] is_start: indicate if the packet is start or continue.
 *   @param[in] p_data: pointer to the data buffer.
 *   @param[in] data_len: length of the data buffer.
 *
 *   @return true if the data is reassembled finish, otherwise false.
 */
static bool ble_host_reassemble_l2cap_data(struct ble_host_l2cap_rx_packet *p_rx_packet, bool is_start, const uint8_t *p_data, uint16_t data_len)
{
    uint32_t pdu_length = 0;
    if (is_start) {
        // start a new L2CAP packet.
        if (p_rx_packet->offset != 0) {
            // if previous packet is not complete, free it.
            BLE_HOST_L2CAP_COMMON_WARN("L2CAP packet start while previous packet is not complete");
            ble_host_clear_reassembled_data(p_rx_packet);
        }

        const struct ble_l2cap_pdu_format *pPdu = (const struct ble_l2cap_pdu_format *) p_data;

        if (pPdu->pdu_length > 512) {
            BLE_HOST_L2CAP_COMMON_WARN("L2CAP PDU length maybe too long, length is %d.", pPdu->pdu_length);
        }

        pdu_length = pPdu->pdu_length + sizeof(struct ble_l2cap_pdu_format);

        if (data_len > pdu_length || data_len < sizeof(struct ble_l2cap_pdu_format)) {
            BLE_HOST_L2CAP_COMMON_ERROR("L2CAP packet length error, data_len:%d, pdu_length:%d", data_len, pdu_length);
            return false;
        }

        p_rx_packet->packet = BLE_L2CAP_RX_MALLOC(pdu_length);

        if (p_rx_packet->packet == NULL) {
            BLE_HOST_L2CAP_COMMON_ERROR("L2CAP malloc receive acl data buffer failed.");
            return false;
        }

        p_rx_packet->total_length = pdu_length;
        p_rx_packet->offset = data_len;
        memcpy(p_rx_packet->packet, p_data, data_len);
    } else {
        // continue a L2CAP packet.
        if (p_rx_packet->offset == 0) {
            BLE_HOST_L2CAP_COMMON_ERROR("L2CAP continue packet while no previous packet");
            return false;
        }

        pdu_length = p_rx_packet->total_length;

        if (data_len + p_rx_packet->offset > pdu_length) {
            BLE_HOST_L2CAP_COMMON_ERROR("L2CAP continue packet length error, received length:%d, current packet length:%d, PDU length:%d",
                p_rx_packet->offset,
                data_len,
                pdu_length);
            ble_host_clear_reassembled_data(p_rx_packet);
            return false;
        }

        memcpy(p_rx_packet->packet + p_rx_packet->offset, p_data, data_len);
        p_rx_packet->offset += data_len;
    }

    return p_rx_packet->offset == p_rx_packet->total_length;
}

static void ble_host_l2cap_receive_l2cap_data(struct ble_host_conn *conn, bool is_start, const uint8_t *p_data, uint16_t data_len)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);
    BLE_HOST_SAL_ASSERT(p_data != NULL);
    BLE_HOST_SAL_ASSERT(data_len != 0);
    BLE_HOST_SAL_ASSERT(conn->user_data[BLE_HOST_L2CAP_USER_ID] != NULL);

    if (conn->user_data[BLE_HOST_L2CAP_USER_ID] == NULL) {
        BLE_HOST_L2CAP_COMMON_ERROR("L2CAP Layer mempool size not enough");
        return;
    }

    struct ble_host_l2cap_rx_packet *p_rx_packet = &((struct ble_l2cap_conn_info *) conn->user_data[BLE_HOST_L2CAP_USER_ID])->rx_packet;

    if (ble_host_reassemble_l2cap_data(p_rx_packet, is_start, p_data, data_len) == false) {
        return;
    }

    struct ble_l2cap_pdu_format *p_l2cap_packet = (struct ble_l2cap_pdu_format *) p_rx_packet->packet;

    BLE_HOST_L2CAP_COMMON_INFO("L2CAP packet received, conn handle:0x%03x, CID:0x%04x, PDU length:%d",
        conn->conn_handle,
        p_l2cap_packet->cid,
        p_l2cap_packet->pdu_length);

    BLE_HOST_L2CAP_COMMON_DEBUG("L2CAP packet:%s", hex_to_str(p_l2cap_packet->info_payload, p_l2cap_packet->pdu_length));

    if (p_l2cap_packet->cid == LE_L2CAP_CID_ATT) {
        if (s_l2cap_callbacks.att_data_callback != NULL) {
            s_l2cap_callbacks.att_data_callback(conn, p_l2cap_packet->pdu_length, p_l2cap_packet->info_payload);
        }
    } else if (p_l2cap_packet->cid == LE_L2CAP_CID_SMP) {
        if (s_l2cap_callbacks.smp_data_callback != NULL) {
            s_l2cap_callbacks.smp_data_callback(conn, p_l2cap_packet->pdu_length, p_l2cap_packet->info_payload);
        }
    } else if (p_l2cap_packet->cid == LE_L2CAP_CID_SIGNALING) {
        if (s_l2cap_callbacks.signaling_data_callback != NULL) {
            s_l2cap_callbacks.signaling_data_callback(conn, p_l2cap_packet->pdu_length, p_l2cap_packet->info_payload);
        }
    } else {
        //if (p_l2cap_packet->cid >= LE_L2CAP_CID_DYN_START && p_l2cap_packet->cid <= LE_L2CAP_CID_DYN_END)
        if (s_l2cap_callbacks.cid_data_callback != NULL) {
            s_l2cap_callbacks.cid_data_callback(conn, p_l2cap_packet->cid, p_l2cap_packet->pdu_length, p_l2cap_packet->info_payload);
        }
    }

    ble_host_clear_reassembled_data(p_rx_packet);
}

static void ble_host_l2cap_send_l2cap_data_sync_callback(uint16_t conn_handle, void *cb_arg, uint8_t status)
{
    BLE_HOST_L2CAP_COMMON_DEBUG("L2CAP packet sent synchronous callback, conn handle:0x%03x, status:%d",
        conn_handle, status);

    struct ble_host_l2cap_tx_packet *tx_packet = cb_arg;

    if (tx_packet->tx_complete_cb != NULL) {
        tx_packet->tx_complete_cb(conn_handle, tx_packet->cb_arg, status);
    }

    BLE_L2CAP_TX_FREE(tx_packet);
}


int ble_host_l2cap_send_l2cap_data_sync(struct ble_host_conn *conn, struct ble_host_l2cap_tx_packet *tx_packet)
{
    if (conn == NULL || tx_packet == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (tx_packet->p_data == NULL || tx_packet->data_length == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    struct ble_host_l2cap_tx_packet *p_packet = BLE_L2CAP_TX_MALLOC(
        sizeof(struct ble_host_l2cap_tx_packet) + tx_packet->data_length);

    if (p_packet == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INSUFFICIENT_RESOURCES);
    }

    p_packet->tx_complete_cb = tx_packet->tx_complete_cb;
    p_packet->cb_arg = tx_packet->cb_arg;
    struct ble_host_l2cap_packet l2cap_packet = {
        .channel_id = tx_packet->channel_id,
        .data_length = tx_packet->data_length,
        .p_data = (uint8_t *) (p_packet + 1),
        .cb = ble_host_l2cap_send_l2cap_data_sync_callback,
        .cb_arg = p_packet,
    };

    memcpy(p_packet + 1, tx_packet->p_data, tx_packet->data_length);

    int ret = ble_host_hci_send_acl_data(conn, &l2cap_packet);

    BLE_HOST_L2CAP_COMMON_DEBUG("L2CAP packet send, conn handle:0x%03x, channel id:%d, data length:%d, result:0x%x",
        conn->conn_handle, tx_packet->channel_id, tx_packet->data_length, ret);

    if (ret != BLE_HOST_ERR_SUCC) {
        BLE_L2CAP_TX_FREE(p_packet); // free the packet if send failed.
    }

    return ret;
}

int ble_host_l2cap_send_l2cap_data_sync_by_conn_handle(uint16_t conn_handle, struct ble_host_l2cap_tx_packet *tx_packet)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    if (conn == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    return ble_host_l2cap_send_l2cap_data_sync(conn, tx_packet);
}

static void ble_host_l2cap_send_l2cap_data_async_callback(uint16_t conn_handle, void *cb_arg, uint8_t status)
{
    BLE_HOST_L2CAP_COMMON_DEBUG("L2CAP packet sent asynchronous callback, conn handle:0x%03x, status:%d",
        conn_handle, status);

    struct ble_host_l2cap_tx_fifo *tx_node = cb_arg;
    BLE_HOST_L2CAP_COMMON_DEBUG("tx node is %p %p", tx_node, tx_node->tx_packet.cb_arg);
    if (tx_node->tx_packet.tx_complete_cb != NULL) {
        tx_node->tx_packet.tx_complete_cb(conn_handle, tx_node->tx_packet.cb_arg, status);
    }

    BLE_L2CAP_TX_FREE(tx_node);
}

static int ble_host_l2cap_send_l2cap_data_async(struct ble_host_conn *conn, struct ble_host_l2cap_tx_fifo *tx_node)
{
    struct ble_host_l2cap_packet l2cap_packet = {
        .channel_id = tx_node->tx_packet.channel_id,
        .data_length = tx_node->tx_packet.data_length,
        .p_data = tx_node->tx_packet.p_data,
        .cb = ble_host_l2cap_send_l2cap_data_async_callback,
        .cb_arg = tx_node,
    };

    int ret = ble_host_hci_send_acl_data(conn, &l2cap_packet);

    BLE_HOST_L2CAP_COMMON_DEBUG("L2CAP packet send asynchronous, conn handle:0x%03x, channel id:%d, data length:%d, result:0x%x",
        conn->conn_handle, tx_node->tx_packet.channel_id, tx_node->tx_packet.data_length, ret);
    return ret;
}

int ble_host_l2cap_send_l2cap_data(struct ble_host_conn *conn, struct ble_host_l2cap_tx_packet *tx_packet)
{
    if (conn == NULL || tx_packet == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (tx_packet->p_data == NULL || tx_packet->data_length == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    struct ble_host_l2cap_tx_fifo *p_tx_node = BLE_L2CAP_TX_MALLOC(
        sizeof(struct ble_host_l2cap_tx_fifo) + tx_packet->data_length
    );

    if (p_tx_node == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INSUFFICIENT_RESOURCES);
    }

    p_tx_node->conn_handle = conn->conn_handle;
    p_tx_node->tx_packet = *tx_packet;
    p_tx_node->tx_packet.p_data = (uint8_t *) (p_tx_node + 1);
    memcpy(p_tx_node + 1, tx_packet->p_data, tx_packet->data_length);
    STAILQ_INSERT_TAIL(&s_l2cap_common_info.tx_fifo, p_tx_node, next);
    s_l2cap_common_info.curr_tx_fifo++;
    BLE_HOST_L2CAP_COMMON_DEBUG("L2CAP packet enqueue, conn handle:0x%03x, channel id:%d, data length:%d",
        conn->conn_handle, tx_packet->channel_id, tx_packet->data_length);
    return BLE_HOST_ERR_SUCC;
}

/**
 *   @brief this function is used to send L2CAP data by connection handle.
 *
 *   @param[in] conn_handle: connection handle.
 *   @param[in] tx_packet: pointer to the L2CAP TX packet.
 *
 *   @return BLE_L2CAP_ERR_INVALID_PARAMS if the input parameter is invalid.
 *           BLE_HOST_ERR_SUCC if the data is sent successfully.
 */
int ble_host_l2cap_send_l2cap_data_by_conn_handle(uint16_t conn_handle, struct ble_host_l2cap_tx_packet *tx_packet)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    if (conn == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    return ble_host_l2cap_send_l2cap_data(conn, tx_packet);
}

/**
 *   @brief this function is used to process the L2CAP TX FIFO.
 *
 *   @return none.
 */
void ble_host_l2cap_tx_task(void)
{
    struct ble_host_l2cap_tx_fifo *p_tx_node = STAILQ_FIRST(&s_l2cap_common_info.tx_fifo);

    if (p_tx_node != NULL) {
        struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(p_tx_node->conn_handle);
        int ret = BLE_HOST_ERR_SUCC;

        if (conn != NULL) {
            ret = ble_host_l2cap_send_l2cap_data_async(conn, p_tx_node);
        }

        if (ret == BLE_HOST_ERR_SUCC) {
            s_l2cap_common_info.curr_tx_fifo--;
            STAILQ_REMOVE_HEAD(&s_l2cap_common_info.tx_fifo, next);
        }

        if (conn == NULL) { // if connect is lost, free the tx packet.
            if (p_tx_node->tx_packet.tx_complete_cb != NULL) {
                p_tx_node->tx_packet.tx_complete_cb(p_tx_node->conn_handle, p_tx_node->tx_packet.cb_arg, BLE_HOST_TX_L2CAP_ERROR_DISCONNECTED);
            }
            BLE_L2CAP_TX_FREE(p_tx_node);
        }
    }
}

