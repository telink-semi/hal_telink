#include "common/types.h"
#include "common/utility.h"

#ifndef BLE_BASIC_PRF_LOG_ENABLE
#define BLE_BASIC_PRF_LOG_ENABLE 1
#endif

#ifndef BLE_BAS_LOG_ENABLE
#define BLE_BAS_LOG_ENABLE 0
#endif

#ifndef BLE_DIS_LOG_ENABLE
#define BLE_DIS_LOG_ENABLE 1
#endif

#ifndef BLE_SCPS_LOG_ENABLE
#define BLE_SCPS_LOG_ENABLE 1
#endif

const uint8_t g_ble_basic_prf_log_enable = IS_ENABLED(BLE_BASIC_PRF_LOG_ENABLE);
const uint8_t g_ble_bas_log_enable = IS_ENABLED(BLE_BAS_LOG_ENABLE);
const uint8_t g_ble_dis_log_enable = IS_ENABLED(BLE_DIS_LOG_ENABLE);
const uint8_t g_ble_scps_log_enable = IS_ENABLED(BLE_SCPS_LOG_ENABLE);
