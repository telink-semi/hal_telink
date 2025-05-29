
/**
 *  @brief Definition Characteristic Properties field, core_v5.4 Vol 3, Part G, 3.3.1.1 (table 3.5).
*/
#define CHAR_PROP_BROADCAST                 0x01
#define CHAR_PROP_READ                      0x02
#define CHAR_PROP_WRITE_WITHOUT_RSP         0x04
#define CHAR_PROP_WRITE                     0x08
#define CHAR_PROP_NOTIFY                    0x10
#define CHAR_PROP_INDICATE                  0x20
#define CHAR_PROP_AUTHEN_SIGNED_WRITES      0x40
#define CHAR_PROP_EXTENDED_PROPERTIES       0x80

/**
 *  @brief Definition Characteristic Extended Properties field, core_v5.4 Vol 3, Part G, 3.3.3.1 (table 3.8).
*/
#define CHAR_EXT_PROP_RELIABLE_WRITE        0x01
#define CHAR_EXT_PROP_WRITABLE_AUXILIARIES  0x02

union characteristic_properties {
    struct {
        uint8_t broadcast : 1;
        uint8_t read : 1;
        uint8_t writeWithoutResponse : 1;
        uint8_t write : 1;
        uint8_t notify : 1;
        uint8_t indicate : 1;
        uint8_t authenticatedSignedWrites : 1;
        uint8_t extendedProperties : 1;
    };
    uint8_t all;
};

enum ble_host_gatt_error_code {
    BLE_GATT_SUCCESS = 0,
    BLE_GATT_ERR_INVALID_CONN_HANDLE,
    BLE_GATT_ERR_INVALID_PARAMS,
    BLE_GATT_ERR_INSUFFICIENT_RESOURCES,
    BLE_GATT_ERR_INVALID_ATTR_HANDLE,
    BLE_GATT_ERR_NOT_FOUND_SERVICE,
};

/**
 *   @brief this function is used to initialize the GATT module.
 *
 *   @param[in] p_gatt_memory: pointer to the GATT memory pool.
 *   @param[in] size: size of the GATT memory pool.
 *
 *   @return none.
 */
void ble_host_gatt_init(uint8_t *p_gatt_memory, uint32_t size);
