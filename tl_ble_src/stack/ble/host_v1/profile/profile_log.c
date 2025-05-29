#include "common/types.h"
#include "common/utility.h"


#ifndef BLE_PROFILE_LOG_ENABLE
#define BLE_PROFILE_LOG_ENABLE 0
#endif

#ifndef BLE_PROFILE_STORAGE_ENABLE
#define BLE_PROFILE_STORAGE_ENABLE 1
#endif

const uint8_t g_ble_profile_log_enable = IS_ENABLED(BLE_PROFILE_LOG_ENABLE);
const uint8_t g_ble_profile_storage_enable = IS_ENABLED(BLE_PROFILE_STORAGE_ENABLE);
