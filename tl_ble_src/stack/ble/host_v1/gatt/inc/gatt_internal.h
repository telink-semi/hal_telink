
#define BLE_GATT_MAX_EATT_CHANNELS          (0)

#define GATT_ATTR_HANDLE_NONE               0x0000   /** refer to ATTR_HANDLE_NONE in att.h */
#define GATT_ATTR_HANDLE_START              0x0001   /** refer to ATTR_HANDLE_START in att.h */
#define GATT_ATTR_HANDLE_END                0xFFFF   /** refer to ATTR_HANDLE_END_MAX in att.h */

#define GET_GATT_CONN_INFO(conn)           ((struct ble_gatt_conn_info *)(conn->user_data[BLE_HOST_GATT_USER_ID]))
#define GATT_CONN_INFO_CHECK(conn)         ((conn) == NULL || GET_GATT_CONN_INFO(conn) == NULL)

enum {
    BLE_HOST_GATT_MALLOC_TYPE = 0x100,
    BLE_HOST_GATT_MALLOC_CONN_INFO,
    BLE_HOST_GATT_MALLOC_CLIENT_CONN_INFO,
    BLE_HOST_GATT_MALLOC_SERVER_CONN_INFO,
    BLE_HOST_GATT_MALLOC_SDP_INFO,      // SDP is service discovery protocol.
    BLE_HOST_GATT_MALLOC_SSDP_INFO,      // SSDP is single service discovery.
    BLE_HOST_GATT_MALLOC_CLIENT_REQ_PARAMS,
    BLE_HOST_GATT_MALLOC_CLIENT_REQ_CONTEXT,
};

enum ble_gatt_event {
    GATT_EVT_ACL_CONNECTED,        /** < param is NULL */
    GATT_EVT_ACL_DISCONNECTED,     /** < param is disconnected reason, uint8_t */
    GATT_EVT_EATT_CONNECTTED,      /** < param: refer to struct ble_gatt_evt_eatt_conn */
    GATT_EVT_EATT_DISCONNECTED,    /** < param: refer to struct ble_gatt_evt_eatt_conn */
};

// 
struct ble_gatt_evt_eatt_conn {
    uint16_t conn_handle;
    uint16_t channel_id;            /** < channel ID for EATT connection */
};

enum ble_gatt_user_id {
    GATT_CLIENT_USER_ID = 0,
    GATT_SERVER_USER_ID,
    GATT_SDP_USER_ID,   // SDP is service discovery protocol.
    GATT_SSDP_USER_ID = GATT_SDP_USER_ID,   // SSDP is single service discovery.
    GATT_MAX_USER_ID
};

typedef void (*gatt_ctrl_callback_t)(struct ble_host_conn *conn, uint8_t event, const void *param);

struct ble_gatt_conn_info {
    void *gatt_client_info;
    void *gatt_server_info;
    union {
        void *gatt_sdp_info;        // SDP is service discovery protocol.
        void *gatt_ssdp_info;        // SSDP is single service discovery.
    };
};

/**
 *   @brief this function is used to allocate memory from GATT memory pool.
 *
 *   @param[in] size: size of the memory to be allocated.
 *   @param[in] type_id: type ID of the memory to be allocated.
 *
 *   @return pointer to the allocated memory.
 */
void *ble_host_gatt_malloc(uint32_t size, uint16_t type_id);

/**
 *   @brief this function is used to free memory from GATT memory pool.
 *
 *   @param[in] ptr: pointer to the memory to be freed.
 *
 *   @return none.
 */
void ble_host_gatt_free(void *ptr);

void ble_host_gatt_register_ctrl_callback(enum ble_gatt_user_id user_id, gatt_ctrl_callback_t callback);

void ble_host_gatt_unregister_ctrl_callback(enum ble_gatt_user_id user_id);
