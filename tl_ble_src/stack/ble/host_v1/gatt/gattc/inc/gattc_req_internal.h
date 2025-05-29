#define GATT_CLIENT_REQ_MALLOC(size) ble_host_gatt_malloc(size, BLE_HOST_GATT_MALLOC_CLIENT_REQ_PARAMS)
#define GATT_CLIENT_REQ_FREE(ptr)    ble_host_gatt_free(ptr)

/**************** GATT Client Exchange MTU ****************/
struct gattc_exchange_mtu_param {
    uint16_t MTU;
    gattc_exchange_mtu_callback callback;
};

/***************** GATT Client Discover All Services ****************/
#define gattc_read_by_group_type_param gattc_disc_all_services

/***************** GATT Client Discover All Characteristics ****************/
#define gattc_find_by_type_value_param gattc_disc_service_by_uuid
#define gattc_find_info_param   gattc_disc_characteristic_desc_param

struct gattc_discover_characteristic_param {
    const struct att_uuid *characteristic_uuid;     /** < characteristic uuid */
    uint16_t start_handle;                   /** < start handle of the characteristic range */
    uint16_t end_handle;                     /** < end handle of the characteristic range */
    void *user_data;                   /** < user data to be passed to the callback */
    disc_characteristics_callback callback;  /** < callback function to be called when the procedure is updated */
};

struct characteristic_16bit_uuid_attr_value {
    union characteristic_properties properties;  /** characteristic properties */
    uint16_t valueHandle;     /** characteristic value handle */
    uint16_t uuid;            /** characteristic uuid , 2 bytes UUID */
}__attribute__((packed));

struct characteristic_128bit_uuid_attr_value {
    union characteristic_properties properties;  /** characteristic properties */
    uint16_t valueHandle;     /** characteristic value handle */
    uint8_t uuid[ATT_128_UUID_LEN];         /** characteristic uuid , 16 bytes UUID */
}__attribute__((packed));

/***************** GATT Client Find Included Services ****************/
#define ble_host_gattc_recv_find_included_128bit_services_error     ble_host_gattc_recv_find_included_services_error
#define ble_host_gattc_find_included_128bit_services_timeout        ble_host_gattc_find_included_services_timeout

#define GATT_FIND_INCLUDED_SERVICE_128BIT_UUID_MAX_COUNT   4

struct included_16bit_uuid_attr_value {
    uint16_t start_handle;    /** start handle of the included service range */
    uint16_t end_handle;      /** end handle of the included service range */
    uint8_t uuid[ATT_16_UUID_LEN];  /** included service 16-bit UUID */
}__attribute__((packed));

struct included_128bit_uuid_attr_value {
    uint16_t start_handle;    /** start handle of the included service range */
    uint16_t end_handle;      /** end handle of the included service range */
}__attribute__((packed));

struct gattc_disc_128bit_uuid_incl_service {
    uint16_t handle;                         /** < handle of the included service */
    uint16_t start_handle;                   /** < start handle of the include service range */
    uint16_t end_handle;                     /** < end handle of the include service range */
};

struct gattc_disc_incl_service_info {
    uint16_t start_handle;                   /** < start handle of the service range */
    uint16_t end_handle;                     /** < end handle of the service range */
    void *user_data;                         /** < user data to be passed to the callback */
    gattc_find_incl_service_callback callback; /** < callback function to be called when the procedure is updated */
    uint16_t incl_128bit_uuid_handle;
    uint8_t incl_128bit_uuid_count;         /** < number of 128-bit UUIDs to find included services */
    uint8_t incl_128bit_uuid_index;         /** < index of the 128-bit UUID to find included services */
    /** < list of 128-bit UUIDs to find included services */
    struct gattc_disc_128bit_uuid_incl_service incl_128bit_uuid[GATT_FIND_INCLUDED_SERVICE_128BIT_UUID_MAX_COUNT];
};

/***************** GATT Client Read Characteristic Value ****************/
#define ble_host_gattc_recv_read_blob_error        ble_host_gattc_recv_read_error
#define ble_host_gattc_read_blob_timeout           ble_host_gattc_read_timeout

struct gattc_read_common_param {
    bool read_long;                            /** < true if long read, false if short read */
    uint16_t handle;                           /** < attribute handle of the characteristic value to be read */
    uint16_t offset;                           /** < offset of the value to be read */
    void *user_data;                     /** < user data to be passed to the callback */
    gattc_read_characteristic_value_callback callback; /** < callback function to be called when the procedure is updated */
};

#define gattc_read_param        gattc_read_common_param
#define gattc_read_blob_param   gattc_read_common_param

/***************** GATT Client Write Characteristic Value ****************/
struct gattc_write_common_param {
    uint16_t handle;                           /** < attribute handle of the characteristic value to be write */
    uint16_t total_length;                     /** < total length of the data to be written */
    uint16_t offset;                           /** < offset of the data to be written */
    uint16_t mtu;                              /** < MTU of the connection */
    uint16_t curr_packet_length;               /** < length of the current packet being sent */
    uint16_t execute_flag;                     /** < flag to indicate if the procedure include att execute write request */
    void *user_data;                     /** < user data to be passed to the callback */
    gattc_write_characteristic_value_callback callback; /** < callback function to be called when the procedure is updated */
    const uint8_t *global_buffer;              /** < global buffer to store the write data */
    uint8_t buffer[0];                        /** < buffer to store the write data */
};

#define gattc_write_param           gattc_write_common_param
#define gattc_prepare_write_param   gattc_write_common_param
#define gattc_execute_write_param   gattc_write_common_param

#define GATTC_EXECUTE_WRITE_FLAG_CANCEL         0x00
#define GATTC_EXECUTE_WRITE_FLAG_IMMED_WRITE    0x01

#define ble_host_gattc_recv_execute_write_error        ble_host_gattc_recv_write_error
#define ble_host_gattc_execute_write_timeout           ble_host_gattc_write_timeout

#define ble_host_gattc_recv_prepare_write_error        ble_host_gattc_recv_write_error
#define ble_host_gattc_prepare_write_timeout           ble_host_gattc_write_timeout

