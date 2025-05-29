#include "common/types.h"
#include "common/utility.h"


#ifndef BLE_HOST_GAP_LOG_ENABLE
    #define BLE_HOST_GAP_LOG_ENABLE 1
#endif

const uint8_t g_ble_host_gap_log_enable            = IS_ENABLED(BLE_HOST_GAP_LOG_ENABLE);
