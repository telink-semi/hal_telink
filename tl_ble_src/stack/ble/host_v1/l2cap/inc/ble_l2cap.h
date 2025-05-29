#pragma once

#define GET_L2CAP_CONN_INFO(conn)           ((struct ble_l2cap_conn_info *)(conn->user_data[BLE_HOST_L2CAP_USER_ID]))
#define L2CAP_CONN_INFO_CHECK(conn)         ((conn) == NULL || GET_L2CAP_CONN_INFO(conn) == NULL)

enum {
    BLE_HOST_L2CAP_MALLOC_TYPE = 0x100,
    BLE_HOST_L2CAP_MALLOC_RX_BUFFER,
    BLE_HOST_L2CAP_MALLOC_TX_BUFFER,
    BLE_HOST_L2CAP_MALLOC_CONN_INFO,
    BLE_HOST_L2CAP_MALLOC_ATT_CONN_INFO,
};

/**
 *  @brief  Definition for L2CAP CID value for the LE-U
 */
enum ble_l2cap_channel_id {
    LE_L2CAP_CID_ATT = 0x0004,
    LE_L2CAP_CID_SIGNALING = 0x0005,
    LE_L2CAP_CID_SMP = 0x0006,
};

enum ble_l2cap_event {
    L2CAP_EVT_ACL_CONNECTED,        /** < param is NULL */
    L2CAP_EVT_ACL_DISCONNECTED,     /** < param is disconnected reason, uint8_t */
};

enum ble_host_l2cap_error_code {
    BLE_L2CAP_SUCCESS = 0,
    BLE_L2CAP_ERR_INVALID_CONN_HANDLE,
    BLE_L2CAP_ERR_INVALID_PARAMS,
    BLE_L2CAP_ERR_INSUFFICIENT_RESOURCES,
    BLE_L2CAP_ERR_ATT_MTU_EXCEEDED,
    BLE_L2CAP_ERR_ATT_INVALID_HANDLE,
    BLE_L2CAP_ERR_ATT_INVALID_UUID,
};

typedef void (*l2cap_ctrl_callback_t)(struct ble_host_conn *conn, uint8_t event, const void *param);
typedef void (*l2cap_data_callback_t)(struct ble_host_conn *conn, uint16_t len, uint8_t *p_packet);

typedef void (*l2cap_cid_ctrl_callback_t)(struct ble_host_conn *conn, uint16_t cid, uint8_t event, const void *param);
typedef void (*l2cap_cid_data_callback_t)(struct ble_host_conn *conn, uint16_t cid, uint16_t len, uint8_t *p_packet);

struct ble_l2cap_pdu_format {
    uint16_t pdu_length;      /** < PDU Length*/
    uint16_t cid;             /** < Channel ID */
    uint8_t  info_payload[0]; /** < Information payload range 0 to 65535 octets */
};

struct ble_host_l2cap_rx_packet {
    uint32_t total_length;
    uint32_t offset;
    uint8_t *packet;
};

struct ble_l2cap_conn_info {
    struct ble_host_l2cap_rx_packet rx_packet;
    void *att_info;
    void *sig_info;
    void *smp_info;
    void *coc_info;
};

typedef void (*ble_l2cap_tx_complete_cb)(uint16_t conn_handle, void *arg, uint16_t status);

struct ble_host_l2cap_tx_packet {
    uint16_t channel_id;
    uint16_t data_length;
    const uint8_t *p_data;    // pointer to the data buffer, global memory.
    ble_l2cap_tx_complete_cb tx_complete_cb;
    void *cb_arg;
};

/**
 *   @brief this function is used to initialize the L2CAP module.
 *
 *   @param[in] p_l2cap_memory: pointer to the L2CAP memory pool.
 *   @param[in] size: size of the L2CAP memory pool.
 *
 *   @return none.
 */
void ble_host_l2cap_init(uint8_t *p_l2cap_memory, uint32_t size);

/**
 *   @brief this function is used to allocate memory from L2CAP memory pool.
 *
 *   @param[in] size: size of the memory to be allocated.
 *   @param[in] type_id: type ID of the memory to be allocated.
 *
 *   @return pointer to the allocated memory.
 */
void *ble_host_l2cap_malloc(uint32_t size, uint16_t type_id);

/**
 *   @brief this function is used to free memory from L2CAP memory pool.
 *
 *   @param[in] ptr: pointer to the memory to be freed.
 *
 *   @return none.
 */
void ble_host_l2cap_free(void *ptr);

/**
 *   @brief this function is used to register ATT, SMP, signaling and callbacks.
 *
 *   @param[in] cid: channel ID, LE_L2CAP_CID_ATT, LE_L2CAP_CID_SIGNALING, LE_L2CAP_CID_SMP.
 *   @param[in] ctrl_callback: control callback function.
 *   @param[in] data_callback: data callback function.
 *
 *   @return none.
 */
void ble_host_l2cap_register_callbacks(uint8_t cid, l2cap_ctrl_callback_t ctrl_callback, l2cap_data_callback_t data_callback);

/**
 *   @brief this function is used to register CoC data callback.
 *
 *   @param[in] ctrl_callback: control callback function.
 *   @param[in] data_callback: data callback function.
 *
 *   @return none.
 */
void ble_host_l2cap_register_cid_data_callback(l2cap_cid_ctrl_callback_t ctrl_callback, l2cap_cid_data_callback_t data_callback);

/**
 *   @brief this function is used to process the L2CAP TX FIFO.
 *
 *   @return none.
 */
void ble_host_l2cap_tx_task(void);

/**
 *   @brief this function is used to send L2CAP data.
 *
 *   @param[in] conn: pointer to the connection object.
 *   @param[in] tx_packet: pointer to the L2CAP TX packet.
 *
 *   @return BLE_L2CAP_ERR_INVALID_PARAMS if the input parameter is invalid.
 *           BLE_L2CAP_ERR_INSUFFICIENT_RESOURCES if there is no memory to allocate the TX node.
 *           BLE_HOST_ERR_SUCC if the data is sent successfully.
 */
int ble_host_l2cap_send_l2cap_data(struct ble_host_conn *conn, struct ble_host_l2cap_tx_packet *tx_packet);

/**
 *   @brief this function is used to send L2CAP data by connection handle.
 *
 *   @param[in] conn_handle: connection handle.
 *   @param[in] tx_packet: pointer to the L2CAP TX packet.
 *
 *   @return BLE_L2CAP_ERR_INVALID_PARAMS if the input parameter is invalid.
 *           BLE_HOST_ERR_SUCC if the data is sent successfully.
 */
int ble_host_l2cap_send_l2cap_data_by_conn_handle(uint16_t conn_handle, struct ble_host_l2cap_tx_packet *tx_packet);

/**
 *   @brief this function is used to send L2CAP data synchronously.
 *
 *   @param[in] conn: pointer to the connection object.
 *   @param[in] tx_packet: pointer to the L2CAP TX packet.
 *
 *   @return BLE_L2CAP_ERR_INVALID_PARAMS if the input parameter is invalid.
 *           BLE_L2CAP_ERR_INSUFFICIENT_RESOURCES if there is no memory to allocate the TX node.
 *           BLE_HOST_ERR_SUCC if the data is sent successfully.
 */
int ble_host_l2cap_send_l2cap_data_sync(struct ble_host_conn *conn, struct ble_host_l2cap_tx_packet *tx_packet);

/**
 *   @brief this function is used to send L2CAP data synchronously by connection handle.
 *
 *   @param[in] conn_handle: connection handle.
 *   @param[in] tx_packet: pointer to the L2CAP TX packet.
 *
 *   @return BLE_L2CAP_ERR_INVALID_PARAMS if the input parameter is invalid.
 *           BLE_HOST_ERR_SUCC if the data is sent successfully.
 */
int ble_host_l2cap_send_l2cap_data_sync_by_conn_handle(uint16_t conn_handle, struct ble_host_l2cap_tx_packet *tx_packet);

