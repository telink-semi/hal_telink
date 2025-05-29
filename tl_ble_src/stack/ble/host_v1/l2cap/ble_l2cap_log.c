#include "common/types.h"
#include "common/utility.h"


#ifndef BLE_HOST_L2CAP_LOG_ENABLE
    #define BLE_HOST_L2CAP_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_L2CAP_ATT_LOG_ENABLE
    #define BLE_HOST_L2CAP_ATT_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_L2CAP_COC_LOG_ENABLE
    #define BLE_HOST_L2CAP_COC_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_L2CAP_EATT_LOG_ENABLE
    #define BLE_HOST_L2CAP_EATT_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_L2CAP_SIGNALING_LOG_ENABLE
    #define BLE_HOST_L2CAP_SIGNALING_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_L2CAP_SMP_LOG_ENABLE
    #define BLE_HOST_L2CAP_SMP_LOG_ENABLE 1
#endif

const uint8_t g_ble_host_l2cap_log_enable           = IS_ENABLED(BLE_HOST_L2CAP_LOG_ENABLE);
const uint8_t g_ble_host_l2cap_att_log_enable       = IS_ENABLED(BLE_HOST_L2CAP_ATT_LOG_ENABLE);
const uint8_t g_ble_host_l2cap_coc_log_enable       = IS_ENABLED(BLE_HOST_L2CAP_COC_LOG_ENABLE);
const uint8_t g_ble_host_l2cap_eatt_log_enable      = IS_ENABLED(BLE_HOST_L2CAP_EATT_LOG_ENABLE);
const uint8_t g_ble_host_l2cap_signaling_log_enable = IS_ENABLED(BLE_HOST_L2CAP_SIGNALING_LOG_ENABLE);
const uint8_t g_ble_host_l2cap_smp_log_enable       = IS_ENABLED(BLE_HOST_L2CAP_SMP_LOG_ENABLE);
