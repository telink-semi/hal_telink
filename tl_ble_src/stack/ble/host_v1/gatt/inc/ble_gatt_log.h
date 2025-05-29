#pragma once

extern const uint8_t g_ble_host_gatt_log_enable;
extern const uint8_t g_ble_host_gatt_client_log_enable;
extern const uint8_t g_ble_host_gatt_server_log_enable;
extern const uint8_t g_ble_host_gatt_sdp_log_enable;

#define BLE_GATT_LOG_OUTPUT(log, en, str, ...) \
    do { \
        if (en && g_ble_host_gatt_log_enable) { \
            log("[GATT]" str, ##__VA_ARGS__); \
        } \
    } while(0)


#define BLE_HOST_GATT_COMMON_ERROR(str, ...)    BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_gatt_log_enable, str, ##__VA_ARGS__)
#define BLE_HOST_GATT_COMMON_WARN(str, ...)     BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_gatt_log_enable, str, ##__VA_ARGS__)
#define BLE_HOST_GATT_COMMON_INFO(str, ...)     BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_gatt_log_enable, str, ##__VA_ARGS__)
#define BLE_HOST_GATT_COMMON_DEBUG(str, ...)    BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_gatt_log_enable, str, ##__VA_ARGS__)

#define BLE_HOST_GATT_CLIENT_ERROR(str, ...)    BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_gatt_client_log_enable,"[CLIENT]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_CLIENT_WARN(str, ...)     BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_gatt_client_log_enable,"[CLIENT]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_CLIENT_INFO(str, ...)     BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_gatt_client_log_enable,"[CLIENT]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_CLIENT_DEBUG(str, ...)    BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_gatt_client_log_enable,"[CLIENT]"str, ##__VA_ARGS__)

#define BLE_HOST_GATT_SERVER_ERROR(str, ...)    BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_gatt_server_log_enable,"[SERVER]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_SERVER_WARN(str, ...)     BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_gatt_server_log_enable,"[SERVER]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_SERVER_INFO(str, ...)     BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_gatt_server_log_enable,"[SERVER]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_SERVER_DEBUG(str, ...)    BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_gatt_server_log_enable,"[SERVER]"str, ##__VA_ARGS__)

#define BLE_HOST_GATT_SDP_ERROR(str, ...)       BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_gatt_sdp_log_enable,"[SDP]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_SDP_WARN(str, ...)        BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_gatt_sdp_log_enable,"[SDP]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_SDP_INFO(str, ...)        BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_gatt_sdp_log_enable,"[SDP]"str, ##__VA_ARGS__)
#define BLE_HOST_GATT_SDP_DEBUG(str, ...)       BLE_GATT_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_gatt_sdp_log_enable,"[SDP]"str, ##__VA_ARGS__)
