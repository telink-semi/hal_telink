#pragma once

extern const uint8_t g_ble_basic_prf_log_enable;
extern const uint8_t g_ble_bas_log_enable;
extern const uint8_t g_ble_dis_log_enable;
extern const uint8_t g_ble_scps_log_enable;

#define BLE_BASIC_PRF_LOG(log, en, str, ...)  \
    do {                                         \
        if (en && g_ble_basic_prf_log_enable) { \
            log("[B-PRF]" str, ##__VA_ARGS__);   \
        }                                        \
    } while (0)

#define BLE_BAS_ERROR(str, ...)    BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_ERROR, g_ble_bas_log_enable, "[BAS]"str, ##__VA_ARGS__)
#define BLE_BAS_WARN(str, ...)     BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_WARN, g_ble_bas_log_enable, "[BAS]"str, ##__VA_ARGS__)
#define BLE_BAS_INFO(str, ...)     BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_INFO, g_ble_bas_log_enable, "[BAS]"str, ##__VA_ARGS__)
#define BLE_BAS_DEBUG(str, ...)    BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_DEBUG, g_ble_bas_log_enable, "[BAS]"str, ##__VA_ARGS__)

#define BLE_DIS_ERROR(str, ...)    BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_ERROR, g_ble_dis_log_enable, "[DIS]"str, ##__VA_ARGS__)
#define BLE_DIS_WARN(str, ...)     BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_WARN, g_ble_dis_log_enable, "[DIS]"str, ##__VA_ARGS__)
#define BLE_DIS_INFO(str, ...)     BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_INFO, g_ble_dis_log_enable, "[DIS]"str, ##__VA_ARGS__)
#define BLE_DIS_DEBUG(str, ...)    BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_DEBUG, g_ble_dis_log_enable, "[DIS]"str, ##__VA_ARGS__)

#define BLE_SCPS_ERROR(str, ...)   BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_ERROR, g_ble_scps_log_enable, "[SCPS]"str, ##__VA_ARGS__)
#define BLE_SCPS_WARN(str, ...)    BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_WARN, g_ble_scps_log_enable, "[SCPS]"str, ##__VA_ARGS__)
#define BLE_SCPS_INFO(str, ...)    BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_INFO, g_ble_scps_log_enable, "[SCPS]"str, ##__VA_ARGS__)
#define BLE_SCPS_DEBUG(str, ...)   BLE_BASIC_PRF_LOG(BLE_HOST_SAL_LOG_DEBUG, g_ble_scps_log_enable, "[SCPS]"str, ##__VA_ARGS__)
