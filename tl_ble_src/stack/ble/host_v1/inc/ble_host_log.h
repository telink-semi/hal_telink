
#pragma once

extern const uint8_t g_ble_host_acl_device_manager_log_enable;
extern const uint8_t g_ble_host_controller_info_log_enable;

#define BLE_HOST_LOG_OUTPUT(log, en, str, ...) \
    do {                                       \
        if (en) {                              \
            log("[HOST]" str, ##__VA_ARGS__);  \
        }                                      \
    } while (0)

#define BLE_HOST_ACL_DEV_ERROR(str, ...)   BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_acl_device_manager_log_enable, "[DEV]" str, ##__VA_ARGS__)
#define BLE_HOST_ACL_DEV_WARN(str, ...)    BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_acl_device_manager_log_enable, "[DEV]" str, ##__VA_ARGS__)
#define BLE_HOST_ACL_DEV_INFO(str, ...)    BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_acl_device_manager_log_enable, "[DEV]" str, ##__VA_ARGS__)
#define BLE_HOST_ACL_DEV_DEBUG(str, ...)   BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_acl_device_manager_log_enable, "[DEV]" str, ##__VA_ARGS__)

#define BLE_HOST_CTRL_INFO_ERROR(str, ...) BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_ERROR, g_ble_host_controller_info_log_enable, "[INFO]" str, ##__VA_ARGS__)
#define BLE_HOST_CTRL_INFO_WARN(str, ...)  BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_WARN, g_ble_host_controller_info_log_enable, "[INFO]" str, ##__VA_ARGS__)
#define BLE_HOST_CTRL_INFO_INFO(str, ...)  BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_INFO, g_ble_host_controller_info_log_enable, "[INFO]" str, ##__VA_ARGS__)
#define BLE_HOST_CTRL_INFO_DEBUG(str, ...) BLE_HOST_LOG_OUTPUT(BLE_HOST_SAL_LOG_DEBUG, g_ble_host_controller_info_log_enable, "[INFO]" str, ##__VA_ARGS__)
