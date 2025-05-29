#pragma once

#define BLE_HCI_LE_MAX_SUPPORTED_EXT_SCAN_PARAMS_COUNT 3

struct ble_hci_le_set_ext_scan_params_full_cp
{
    uint8_t            own_addr_type;
    uint8_t            filter_policy;
    uint8_t            phys;
    struct scan_params scans[BLE_HCI_LE_MAX_SUPPORTED_EXT_SCAN_PARAMS_COUNT];
} __attribute__((packed));

/**
 * @brief Sets up the extended scan parameters.
 *
 * @param[in] p_ext_scan_param Pointer to the structure containing the extended scan parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_ext_scan_param(const struct ble_hci_le_set_ext_scan_params_full_cp *p_ext_scan_param);

/**
 * @brief Enables the extended scan.
 *
 * @param[in] p_ext_scan_enable Pointer to the structure containing the extended scan enable parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_ext_scan_enable(const struct ble_hci_le_set_ext_scan_enable_cp *p_ext_scan_enable);