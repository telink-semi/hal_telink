#include "common/types.h"
#include "common/utility.h"


#ifndef BLE_HOST_ACL_DEVICE_MANAGER_LOG_ENABLE
    #define BLE_HOST_ACL_DEVICE_MANAGER_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_CONTROLLER_INFO_LOG_ENABLE
    #define BLE_HOST_CONTROLLER_INFO_LOG_ENABLE 1
#endif

const uint8_t g_ble_host_acl_device_manager_log_enable = IS_ENABLED(BLE_HOST_ACL_DEVICE_MANAGER_LOG_ENABLE);
const uint8_t g_ble_host_controller_info_log_enable    = IS_ENABLED(BLE_HOST_CONTROLLER_INFO_LOG_ENABLE);
