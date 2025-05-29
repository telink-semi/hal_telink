
enum ble_prf_error_code {
    BLE_PRF_SUCCESS = 0x00,
    BLE_PRF_ERR_INVALID_CONN_HANDLE,
    BLE_PRF_ERR_INPUT_NULL,
    BLE_PRF_ERR_GET_ATTR_VALUE_NOT_FOUND,
    BLE_PRF_ERR_SPEC_START = 0x80,
};

/** < Profile process type enumeration. */
enum prf_process_type {
    PRF_PROCESS_INIT,   /* < when register profile node */
    PRF_PROCESS_DEINIT, /* < when unregister profile node */
};

/** < profile bound ACL role enumeration. */
enum prf_used_acl_role {
    PRF_USED_ACL_ROLE_CENTRAL = 0x01,       /** < only used in ACL central. */
    PRF_USED_ACL_ROLE_PERIPHERAL = 0x02,    /** < only used in ACL peripheral. */
    PRF_USED_ACL_ROLE_CONNECT = PRF_USED_ACL_ROLE_CENTRAL | PRF_USED_ACL_ROLE_PERIPHERAL,
};

/** < profile ACL change state enumeration. */
enum prf_acl_state_change {
    PRF_ACL_CONNECT,        /** < When ACL connected. */
    PRF_ACL_DISCONNECT,     /** < When ACL disconnected. */
    PRF_ACL_RECONNECT,      /** < When ACL reconnect. */
};

/** < profile discovery type enumeration. */
enum prf_disc_type {
    // if first connect or remote change service(read database hash value changed), need discover all service.
    PRF_DISC_TYPE_SVC,          /** < First Discover service, need discover service and characteristic. */
    // if connected paired device or read database hash value is same, only need read characteristic value.
    PRF_DISC_TYPE_RECONNECT,    /** < Reconnect service, only need read characteristic value. */
    // current not supported.
    PRF_DISC_TYPE_SERVICE_CHANGED,/** < Received service changed indication, check if service changed. */
};

/** < profile NV state change enumeration. */
enum prf_nv_state_change {
    PRF_NV_STORE,            /** < When store NV. */
    PRF_NV_LOAD,             /** < When load NV. */
};

/** < Profile service id enumeration. */
enum prf_service_id {
    PRF_SERVICE_ID_NULL = 0x00,
    PRF_BASIC_SERVICE_ID_START,                                                 //for GATT, GAP, BAS, DIS.
    PRF_BASIC_SERVICE_ID_END = PRF_BASIC_SERVICE_ID_START + 0x0E,                  //used:4, other reserved
    PRF_LE_AUDIO_SERVICE_ID_START,                                              //for LE AUDIO,.
    PRF_LE_AUDIO_SERVICE_ID_END = PRF_LE_AUDIO_SERVICE_ID_START + 0x1F,             //used:10, other reserved.
    // PRF_CHANNEL_SOUNDING_CLIENT_START,
    // PRF_CHANNEL_SOUNDING_CLIENT_END = PRF_CHANNEL_SOUNDING_CLIENT_START + 0x03,             //used:1 other reserved.
    // PRF_HUMAN_INTERFACE_DEVICE_CLIENT_START,
    // PRF_HUMAN_INTERFACE_DEVICE_CLIENT_ENC = PRF_HUMAN_INTERFACE_DEVICE_CLIENT_START + 0x03, //used:2 other reserved
    // PRF_TEST_PROFILE_CLIENT_START = 0x70,                                           //only user 1
    // PRF_USE_DEFINE_CLIENT_START = 0x71,
    // PRF_SERVER_OFFSET = 0x80,
};

/** < Profile common event ID enumeration. */
enum prf_common_event_id {
    PRF_EVT_ID_ACL_CONNECT,
    PRF_EVT_ID_ACL_DISCONNECT,
    PRF_EVT_ID_ACL_CONNECT_UPDATE_INTERVAL,
    PRF_EVT_ID_SECURITY_DONE,

    PRF_EVT_ID_SDP_FOUND,           /** < message for SDP found, refer to struct ble_prf_sdp_info */
    PRF_EVT_ID_SDP_NOT_FOUND,       /** < message for SDP not found, refer to struct ble_prf_sdp_info */
    PRF_EVT_ID_SDP_FOUND_END,       /** < message for SDP found end, refer to struct ble_prf_sdp_info */
    PRF_EVT_ID_SDP_FINISH,          /** < all service discover finish */
};

/** < Profile NV parameter structure. */
struct prf_nv_param {
    //if load nv data, ptr is NV block start address.
    //if store nv data, ptr is current write NV data address.
    uint8_t *dataPtr;

    union {
        uint16_t currentTotalLen; //only user in store mode.
        uint16_t nvItemLen;       //only used in load mode.
    };
};

typedef void (*prf_event_callback)(uint16_t conn_handle, uint8_t event_id, const void *event_msg);

/** < BLE profile parameters structure. */
struct ble_prf_param {
    uint8_t service_id;                /** < refer to enum prf_service_id */
    uint8_t used_acl_role;              /** < refer to enum prf_used_acl_role */
    uint8_t client;                    /** < 0: server, 1: client */
    uint8_t sec_flag;                  /** < security flag */
    void (*init)(enum prf_process_type type, const void *param);
    void (*connect)(uint16_t conn_handle, enum prf_acl_state_change state);
    void (*discovery)(uint16_t conn_handle, enum prf_disc_type type);
    int (*store)(uint16_t conn_handle, enum prf_nv_state_change state, struct prf_nv_param *param);
};

struct ble_prf_process {
    struct {
        struct ble_prf_process *sle_next;  /* next element */
    }next;          // SLIST_ENTRY(blc_prf_process) next;
    void *event_cb;
    const struct ble_prf_param *prf_params;
};

/** < BLE profile initialization parameter structure. */
struct ble_prf_init_param {
    prf_event_callback event_cb;    /** < event callback function */
    uint8_t *p_prf_memory;          /** < pointer to profile memory */
    uint32_t prf_memory_size;       /** < profile memory size */
};

struct ble_prf_sdp_info {
    uint8_t service_id;
    const char *service_name;
    uint16_t start_handle;      /** < start handle of service only used event PRF_EVT_ID_SDP_FOUND */
    uint16_t end_handle;        /** < end handle of service only used event PRF_EVT_ID_SDP_FOUND */
};

typedef void (*prf_read_callback)(uint16_t conn_handle, uint32_t err);
typedef void (*prf_write_callback)(uint16_t conn_handle, uint32_t err);

/**
 *   @brief  Initialize BLE profile module.
 *
 *   @param[in]  param  Pointer to BLE profile initialization parameter, refer to struct ble_prf_init_param.
 *
 *   @return none.
 */
void ble_prf_initial(struct ble_prf_init_param *param);

/**
 *   @brief  Register BLE service module.
 *
 *   @param[in]  p_module  Pointer to service module, refer to struct ble_prf_process.
 *   @param[in]  param     Pointer to service parameter.
 *
 *   @return none.
 */
void blc_prf_register_service_module(struct ble_prf_process *p_module, const void *param);

/**
 *   @brief  Unregister BLE service module.
 *
 *   @param[in]  p_module  Pointer to service module, refer to struct ble_prf_process.
 *
 *   @return none.
 */
void blc_prf_unregister_service_module(struct ble_prf_process *p_module, const void *param);

void ble_prf_report_sdp_found_end_event(uint16_t conn_handle, uint8_t service_id, const char *service_name);

void ble_prf_report_sdp_not_found_event(uint16_t conn_handle, uint8_t service_id, const char *service_name);

void ble_prf_report_sdp_found_event(uint16_t conn_handle, uint8_t service_id, const char *service_name,
    uint16_t start_handle, uint16_t end_handle);

// remove later.
void ble_prf_discovery_start(uint16_t conn_handle, enum prf_disc_type type, uint8_t sec_flag);

struct ble_prf_read_value {
    uint8_t *data;
    uint16_t *length;
    prf_read_callback callback;
    uint16_t max_data_len;
};

int ble_prf_read_attribute_value(uint16_t conn_handle, uint16_t handle, struct ble_prf_read_value *prf_read_param);
