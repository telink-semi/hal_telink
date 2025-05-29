
#pragma once

extern const uint8_t g_ble_host_gap_log_enable;

#define BLE_GAP_LOG_OUTPUT(log, en, str, ...)  \
    do {                                       \
        if (en && g_ble_host_gap_log_enable) { \
            log("[GAP]" str, ##__VA_ARGS__);   \
        }                                      \
    } while (0)

#define BLE_HOST_GAP_COMMON_ERROR(str, ...)     BLE_GAP_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_gap_log_enable, str, ##__VA_ARGS__)
#define BLE_HOST_GAP_COMMON_WARN(str, ...)      BLE_GAP_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_gap_log_enable, str, ##__VA_ARGS__)
#define BLE_HOST_GAP_COMMON_INFO(str, ...)      BLE_GAP_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_gap_log_enable, str, ##__VA_ARGS__)
#define BLE_HOST_GAP_COMMON_DEBUG(str, ...)     BLE_GAP_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_gap_log_enable, str, ##__VA_ARGS__)