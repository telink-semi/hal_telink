#pragma once
/**
 * @brief Reset hci.
 *
 * @param None
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_send_reset(void);

/**
 * @brief Sets the event mask for the HCI callback.
 *
 * @param[in] p_event_mask Pointer to the structure containing the event mask.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_set_event_mask(const struct ble_hci_cb_set_event_mask_cp *p_event_mask);

/**
 * @brief Sets the event mask 2 for the HCI callback.
 *
 * @param[in] p_event_mask2 Pointer to the structure containing the event mask 2.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_set_event_mask2(const struct ble_hci_cb_set_event_mask2_cp *p_event_mask2);