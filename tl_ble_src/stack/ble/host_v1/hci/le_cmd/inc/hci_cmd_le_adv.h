#pragma once

#define BLE_HCI_LE_MAX_SUPPORTED_ADV_DATA_LEN      31 // Maximum length of advertising data
#define BLE_HCI_LE_MAX_SUPPORTED_SCAN_RSP_DATA_LEN 31 // Maximum length of scan response data

struct ble_hci_le_set_adv_data_full_cp
{
    uint8_t adv_data_len;
    uint8_t adv_data[BLE_HCI_LE_MAX_SUPPORTED_ADV_DATA_LEN];
} __attribute__((packed));

struct ble_hci_le_set_scan_rsp_data_full_cp
{
    uint8_t scan_rsp_len;
    uint8_t scan_rsp[BLE_HCI_LE_MAX_SUPPORTED_SCAN_RSP_DATA_LEN];
} __attribute__((packed));

/**
 * @brief Sets the advertising parameters for the BLE host.
 *
 * @param[in] p_set_adv_param Pointer to the structure containing the advertising parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_adv_param(const struct ble_hci_le_set_adv_params_cp *p_adv_param);

/**
 * @brief read the advertising channel transmit power for the BLE host.
 * 
 * @param[out] p_tx_power Pointer to the structure containing the transmit power.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_read_adv_chnl_tx_power(struct ble_hci_le_rd_adv_chan_txpwr_rp *p_tx_power);

/**
 * @brief Sets the advertising data for the host.
 *
 * @param[in] p_set_adv_data Pointer to the structure containing the advertising data.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_adv_data(const struct ble_hci_le_set_adv_data_full_cp *p_adv_data);

/**
 * @brief Sets the scan response data for the host.
 *
 * @param[in] p_set_scan_rsp_data Pointer to the structure containing the scan response data.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_scan_rsp_data(const struct ble_hci_le_set_scan_rsp_data_full_cp *p_scan_rsp_data);

/**
 * @brief Enables or disables advertising for the BLE host.
 *
 * @param[in] p_set_adv_enable Pointer to the structure containing the enable parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_adv_enable(const struct ble_hci_le_set_adv_enable_cp *p_adv_enable);