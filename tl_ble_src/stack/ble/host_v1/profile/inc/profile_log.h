#pragma once

extern const uint8_t g_ble_profile_log_enable;
extern const uint8_t g_ble_profile_storage_enable;

#define BLE_PROFILE_LOG_OUTPUT(log, en, str, ...)  \
    do {                                         \
        if (en && g_ble_profile_log_enable) { \
            log("[PRF]" str, ##__VA_ARGS__);   \
        }                                        \
    } while (0)

#define BLE_PRF_COMMON_ERROR(str, ...)    BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_profile_log_enable, str, ##__VA_ARGS__)
#define BLE_PRF_COMMON_WARN(str, ...)     BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_profile_log_enable, str, ##__VA_ARGS__)
#define BLE_PRF_COMMON_INFO(str, ...)     BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_profile_log_enable, str, ##__VA_ARGS__)
#define BLE_PRF_COMMON_DEBUG(str, ...)    BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_profile_log_enable, str, ##__VA_ARGS__)

#define BLE_PRF_STORAGE_ERROR(str, ...)    BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_profile_storage_enable, "[store]"str, ##__VA_ARGS__)
#define BLE_PRF_STORAGE_WARN(str, ...)     BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_profile_storage_enable, "[store]"str, ##__VA_ARGS__)
#define BLE_PRF_STORAGE_INFO(str, ...)     BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_profile_storage_enable, "[store]"str, ##__VA_ARGS__)
#define BLE_PRF_STORAGE_DEBUG(str, ...)    BLE_PROFILE_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_profile_storage_enable, "[store]"str, ##__VA_ARGS__)
