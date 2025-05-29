
#define BLE_BAS_CLIENT_SUPPORT_MAX_COUNT        PRF_CONNECT_MAX_COUNT

/***** BAS Client defines *****/
struct ble_bas_client {
    struct ble_gattc_ccc_message ccc_msg;
    uint16_t battery_level_handle;
    uint16_t battery_power_state_handle;

    uint16_t conn_handle;

    uint8_t battery_level;
    uint8_t battery_power_state;
};

struct ble_bas_client_control {
    struct ble_prf_process prf_process;
    struct ble_bas_client *p_bas_client[BLE_BAS_CLIENT_SUPPORT_MAX_COUNT];
};

//client
#define BLE_BAS_DISC_READ_NOTIFY_CHAR(uuid, characteristic)    BLE_PRF_DISC_READ_NOTIFY_CHAR(bas, uuid, characteristic)

#define BLE_FUNC_BAS_DISC_FOUND_CHAR(characteristic)         BLE_FUNC_PRF_DISC_FOUND_CHAR(bas, BAS, characteristic)
#define BLE_FUNC_BAS_DISC_START_READ_FIX_LEN(characteristic) BLE_FUNC_PRF_DISC_START_READ_FIX_LEN(bas, BAS, characteristic)


#define BLE_BAS_READ_ATTR_VALUE_FIX_LEN(charName)                   BLE_PRF_READ_ATTR_VALUE_FIX_LEN(bas, charName)

#define BLE_BAS_GET_ATTR_VALUE_FIX_LEN(characteristic)              BLE_PRF_GET_ATTR_VALUE_FIX_LEN(bas, characteristic)

