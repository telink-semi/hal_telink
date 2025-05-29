/***** BAS Server defines *****/

struct ble_bas_server {
    uint16_t battery_level_handle;
    uint16_t battery_power_state_handle;
    uint8_t battery_level;
    uint8_t battery_power_state;
};

struct ble_bas_server_control {
    struct ble_prf_process prf_process;
    struct ble_bas_server bas_server;
};

//server
#define BLE_BAS_SERVER_INIT_HANDLE(characteristic)     BLE_PRF_SERVER_INIT_HANDLE(bas, BAS, characteristic)
#define BLE_BAS_SERVER_FIND_CHAR(characteristic, uuid) BLE_PRF_SERVER_FIND_CHAR(bas, characteristic, uuid)

