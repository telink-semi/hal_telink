#pragma once

#define BLE_HCI_LE_MAX_PERIODIC_ADV_DATA_LEN 252 // Maximum length of periodic advertising data

struct ble_hci_le_set_periodic_adv_data_full_cp
{
    uint8_t adv_handle;
    uint8_t operation;
    uint8_t adv_data_len;
    uint8_t adv_data[BLE_HCI_LE_MAX_PERIODIC_ADV_DATA_LEN];
} __attribute__((packed));

/**
 * @brief HCI LE Set Periodic Advertising Parameters command parameters.
 * @details The HCI LE Set Periodic Advertising Parameters command is used to set the
 *          periodic advertising parameters for a given advertising set.
 *
 * @param[in] p_periodic_adv_params Pointer to the HCI LE Set Periodic Advertising Parameters command parameters.
 *
 * @return Zero on success or error code on failure.
 */
int ble_host_hci_le_set_periodic_adv_params(const struct ble_hci_le_set_periodic_adv_params_cp *p_periodic_adv_params);

/**
 * @brief HCI LE Set Periodic Advertising Data command parameters.
 * @details The HCI LE Set Periodic Advertising Data command is used to set the
 *          periodic advertising data for a given advertising set.
 *
 * @param[in] p_periodic_adv_data Pointer to the HCI LE Set Periodic Advertising Data command parameters.
 *
 * @return Zero on success or error code on failure.
 */
int ble_host_hci_le_set_periodic_adv_data(const struct ble_hci_le_set_periodic_adv_data_full_cp *p_periodic_adv_data);

/**
 * @brief HCI LE Set Periodic Advertising Enable command parameters.
 * @details The HCI LE Set Periodic Advertising Enable command is used to enable or disable
 *          the periodic advertising for a given advertising set.
 *
 * @param[in] p_periodic_adv_enable Pointer to the HCI LE Set Periodic Advertising Enable command parameters.
 *
 * @return Zero on success or error code on failure.
 */
int ble_host_hci_le_set_periodic_adv_enable(const struct ble_hci_le_set_periodic_adv_enable_cp *p_periodic_adv_enable);
