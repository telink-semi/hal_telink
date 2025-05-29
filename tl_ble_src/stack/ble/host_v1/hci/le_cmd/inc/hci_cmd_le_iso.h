#pragma once

#define BLE_HCI_LE_MAX_SUPPORTED_CODEC_CONFIG_LEN 4 // Maximum length of codec config data

struct ble_hci_le_setup_iso_data_path_full_cp
{
    uint16_t conn_handle;
    uint8_t  data_path_dir;
    uint8_t  data_path_id;
    uint8_t  codec_id[5];
    uint8_t  controller_delay[3];
    uint8_t  codec_config_len;
    uint8_t  codec_config[BLE_HCI_LE_MAX_SUPPORTED_CODEC_CONFIG_LEN];
} __attribute__((packed));

/**
 * @brief Sets up an ISO data path for the host.
 *
 * @param[in] p_cig_params Pointer to the structure containing the CIG parameters.
 * @param[out] p_cig_params_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_setup_iso_data_path(const struct ble_hci_le_setup_iso_data_path_full_cp *p_cig_params,
                                        struct ble_hci_le_setup_iso_data_path_rp            *p_cig_params_rp);

/**
 * @brief Removes an ISO data path for the host.
 *
 * @param[in] p_cig_params Pointer to the structure containing the CIG parameters.
 * @param[out] p_cig_params_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_remove_iso_data_path(const struct ble_hci_le_remove_iso_data_path_cp *p_cig_params,
                                         struct ble_hci_le_remove_iso_data_path_rp       *p_cig_params_rp);

/**
 * @brief Performs an ISO receive test for the host.
 *
 * @param[in] p_receive_test_cp Pointer to the structure containing the receive test parameters.
 * @param[out] p_receive_test_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_iso_receive_test(const struct ble_hci_le_iso_receive_test_cp *p_receive_test_cp,
                                     struct ble_hci_le_iso_receive_test_rp       *p_receive_test_rp);

/**
 * @brief Reads the ISO test counters for the host.
 *
 * @param[in] p_read_test_counters_cp Pointer to the structure containing the read test counters parameters.
 * @param[out] p_read_test_counters_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_iso_read_test_counters(const struct ble_hci_le_iso_read_test_counters_cp *p_read_test_counters_cp,
                                           struct ble_hci_le_iso_read_test_counters_rp       *p_read_test_counters_rp);

/**
 * @brief Ends an ISO test for the host.
 *
 * @param[in] p_test_end_cp Pointer to the structure containing the test end parameters.
 * @param[out] p_test_end_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_iso_test_end(const struct ble_hci_le_iso_test_end_cp *p_test_end_cp,
                                 struct ble_hci_le_iso_test_end_rp       *p_test_end_rp);
