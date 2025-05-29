#include "common/types.h"
#include "common/utility.h"

#ifndef BLE_HOST_GATT_LOG_ENABLE
#define BLE_HOST_GATT_LOG_ENABLE        1
#endif

#ifndef BLE_HOST_GATT_CLIENT_LOG_ENABLE
#define BLE_HOST_GATT_CLIENT_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_GATT_SERVER_LOG_ENABLE
#define BLE_HOST_GATT_SERVER_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_GATT_SDP_LOG_ENABLE
#define BLE_HOST_GATT_SDP_LOG_ENABLE    1
#endif

const uint8_t g_ble_host_gatt_log_enable = IS_ENABLED(BLE_HOST_GATT_LOG_ENABLE);
const uint8_t g_ble_host_gatt_client_log_enable = IS_ENABLED(BLE_HOST_GATT_CLIENT_LOG_ENABLE);
const uint8_t g_ble_host_gatt_server_log_enable = IS_ENABLED(BLE_HOST_GATT_SERVER_LOG_ENABLE);
const uint8_t g_ble_host_gatt_sdp_log_enable = IS_ENABLED(BLE_HOST_GATT_SDP_LOG_ENABLE);
