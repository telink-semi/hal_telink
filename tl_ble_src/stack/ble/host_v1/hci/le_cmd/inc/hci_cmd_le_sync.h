#pragma once
/**
 * @brief Sets up an ISO data path for the host.
 *
 * @param[in] p_create_sync Pointer to the structure containing the create sync parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_periodic_adv_create_sync(const struct ble_hci_le_periodic_adv_create_sync_cp *p_create_sync);

/**
 * @brief Cancels the periodic advertising create sync.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_periodic_adv_create_sync_cancel(void);

/**
 * @brief Terminates the periodic advertising sync.
 *
 * @param[in] p_terminate_sync Pointer to the structure containing the terminate sync parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_periodic_adv_terminate_sync(const struct ble_hci_le_periodic_adv_term_sync_cp *p_terminate_sync);
