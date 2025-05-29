#pragma once
/**
 * @brief  HCI Link Control command, Disconnect command
 * 
 * @param[in] p_lc_disconnect Pointer to the HCI Disconnect command parameters
 * 
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_disconnect(struct ble_hci_lc_disconnect_cp *p_lc_disconnect);

/**
 * @brief  HCI Link Control command opcodes
 * 
 * @param[in] p_rd_rem_ver_info Pointer to the HCI Read Remote Version Information command parameters
 * 
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_read_rem_ver_info(struct ble_hci_rd_rem_ver_info_cp *p_rd_rem_ver_info);
