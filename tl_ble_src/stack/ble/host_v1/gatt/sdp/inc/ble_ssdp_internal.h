
struct gatt_ssdp_wait_service {
    STAILQ_ENTRY(gatt_ssdp_wait_service) next;
    bool include_service;
    const void *list;
};

struct disc_last_characteristic_info {
    union characteristic_properties properties;
    uint16_t handle;
    const struct gatt_ssdp_characteristic *characteristic;
};

struct gatt_ssdp_param {
    bool include_service;
    uint8_t found_service_count;
    uint16_t end_group_handle;
    union {
        const struct ble_gatt_ssdp_list *ssdp_list;
        const struct ble_gatt_ssdp_no_include_list *no_include_list;
        const void *list;
    };

    struct disc_last_characteristic_info last_char_info;
    SLIST_HEAD(, gatt_ssdp_message) message_list;
};

/** @brief GATT request message queue */
struct gatt_ssdp_context {
    uint16_t channel_id;                     /** < Channel ID. */
    struct gatt_ssdp_param *disc_param;
};

/** @brief GATT client connection information */
struct gatt_ssdp_conn_info {
    STAILQ_HEAD(, gatt_ssdp_wait_service) wait_list;  /** < Wait discover service list. */
    struct gatt_ssdp_context att_ctx;  /** < ATT request context */
    /** < EATT request context array. */
    struct gatt_ssdp_context eatt_ctx[BLE_GATT_MAX_EATT_CHANNELS];
};

/********** GATT SSDP message type */
enum gatt_ssdp_message_type {
    SSDP_TYPE_DISC_PRIMARY = 0,
    SSDP_TYPE_FIND_INCLUDE,
    SSDP_TYPE_DISC_INCLUDE_ALL_CHAR,
    SSDP_TYPE_DISC_INCLUDE_CHAR_DESC,
    SSDP_TYPE_SUBSCRIBE_INCLUDE_CCC,
    SSDP_TYPE_READ_INCLUDE_CHAR_VALUE,

    SSDP_TYPE_DISC_PRIMARY_ALL_CHAR,
    SSDP_TYPE_DISC_PRIMARY_CHAR_DESC,
    SSDP_TYPE_SUBSCRIBE_PRIMARY_CCC,
    SSDP_TYPE_READ_PRIMARY_CHAR_VALUE,

    SSDP_TYPE_DONE,
};

struct gatt_ssdp_disc_primary {
    const struct att_uuid *uuid;
    void *user_data;
};

struct gatt_ssdp_find_include {
    uint8_t primary_service_count;
    struct gattc_find_incl_service_param find_incl;
};

struct gatt_ssdp_disc_all_char {
    uint8_t service_count;
    struct gattc_disc_all_characteristics disc_char;
    const struct gatt_ssdp_characteristic_info *char_info;
};

struct gatt_ssdp_disc_char_desc {
    uint8_t service_count;
    union characteristic_properties properties;
    const struct gatt_ssdp_characteristic *characteristic;
    struct gattc_disc_characteristic_desc_param disc_desc;
};

struct gatt_ssdp_subscribe_ccc {
    uint8_t service_count;
    const struct gatt_ssdp_characteristic *characteristic;
    uint16_t ccc_handle;
    bool enable_notification;
    bool enable_indication;
};

struct gatt_ssdp_read_value {
    uint16_t handle;
    uint8_t *write_buffer;
    uint16_t *write_buffer_len;
    uint16_t max_buffer_len;
    const struct gatt_ssdp_characteristic *characteristic;
};

struct gatt_ssdp_message {
    SLIST_ENTRY(gatt_ssdp_message) next;
    uint8_t type;
    uint8_t service_count;
    union {
        struct gatt_ssdp_disc_primary disc_primary;
        struct gatt_ssdp_find_include find_include;
        struct gatt_ssdp_disc_all_char disc_all_char;
        struct gatt_ssdp_disc_char_desc disc_char_desc;
        struct gatt_ssdp_subscribe_ccc subscribe_ccc;
        struct gatt_ssdp_read_value read_value;
    };
};

