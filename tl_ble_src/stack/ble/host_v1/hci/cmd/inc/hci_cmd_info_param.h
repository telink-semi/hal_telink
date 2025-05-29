#pragma once
/**
 * @brief Reads the local version information.
 *
 * @param[out] p_ver_info Pointer to the structure to store the local version information.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_read_local_version_info(struct ble_hci_ip_rd_local_ver_rp *p_ver_info);

/**
 * @brief Reads the Bluetooth device address.
 *
 * @param[out] p_bd_addr Pointer to the structure to store the Bluetooth device address.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_read_bd_address(struct ble_hci_ip_rd_bd_addr_rp *p_bd_addr);

/**
 * @brief Reads the local supported features.
 *
 * @param[out] p_local_supp_feat Pointer to the structure to store the local supported features.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_read_local_supported_features(struct ble_hci_ip_rd_loc_supp_feat_rp *p_local_supp_feat);

/**
 * @brief Reads the local supported commands.
 *
 * @param[out] p_local_supp_cmd Pointer to the structure to store the local supported commands.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_read_local_supported_commands(struct ble_hci_ip_rd_loc_supp_cmd_rp *p_local_supp_cmd);