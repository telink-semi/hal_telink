#include "common/types.h"
#include "common/utility.h"


#ifndef BLE_HOST_HCI_LOG_ENABLE
    #define BLE_HOST_HCI_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_HCI_COMMON_CMD_LOG_ENABLE
    #define BLE_HOST_HCI_COMMON_CMD_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_HCI_COMMON_EVT_LOG_ENABLE
    #define BLE_HOST_HCI_COMMON_EVT_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_HCI_LE_CMD_LOG_ENABLE
    #define BLE_HOST_HCI_LE_CMD_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_HCI_LE_EVT_LOG_ENABLE
    #define BLE_HOST_HCI_LE_EVT_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_HCI_ACL_DATA_LOG_ENABLE
    #define BLE_HOST_HCI_ACL_DATA_LOG_ENABLE 1
#endif

#ifndef BLE_HOST_HCI_ISO_DATA_LOG_ENABLE
    #define BLE_HOST_HCI_ISO_DATA_LOG_ENABLE 1
#endif

const uint8_t g_ble_host_hci_log_enable            = IS_ENABLED(BLE_HOST_HCI_LOG_ENABLE);
const uint8_t g_ble_host_hci_common_cmd_log_enable = IS_ENABLED(BLE_HOST_HCI_COMMON_CMD_LOG_ENABLE);
const uint8_t g_ble_host_hci_common_evt_log_enable = IS_ENABLED(BLE_HOST_HCI_COMMON_EVT_LOG_ENABLE);
const uint8_t g_ble_host_hci_le_cmd_log_enable     = IS_ENABLED(BLE_HOST_HCI_LE_CMD_LOG_ENABLE);
const uint8_t g_ble_host_hci_le_evt_log_enable     = IS_ENABLED(BLE_HOST_HCI_LE_EVT_LOG_ENABLE);
const uint8_t g_ble_host_hci_acl_data_log_enable   = IS_ENABLED(BLE_HOST_HCI_ACL_DATA_LOG_ENABLE);
const uint8_t g_ble_host_hci_iso_data_log_enable   = IS_ENABLED(BLE_HOST_HCI_ISO_DATA_LOG_ENABLE);
