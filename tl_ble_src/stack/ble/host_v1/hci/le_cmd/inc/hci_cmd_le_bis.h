#pragma once

#define BLE_HCI_LE_MAX_SUPPORTED_BIS 4 // Maximum number of BIS
struct ble_hci_le_big_create_sync_full_cp
{
    uint8_t  big_handle;
    uint16_t sync_handle;
    uint8_t  encryption;
    uint8_t  broadcast_code[16];
    uint8_t  mse;
    uint16_t sync_timeout;
    uint8_t  num_bis;
    uint8_t  bis[BLE_HCI_LE_MAX_SUPPORTED_BIS];
} __attribute__((packed));


/**
 * @brief Creates a BIG (Broadcast Isochronous Group).
 *
 * @param[in] p_create_big Pointer to the structure containing the BIG parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_create_big(const struct ble_hci_le_create_big_cp *p_create_big);

/**
 * @brief Creates a BIG test.
 *
 * @param[in] p_create_big_test Pointer to the structure containing the BIG test parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_create_big_test(const struct ble_hci_le_create_big_test_cp *p_create_big_test);

/**
 * @brief Terminates a BIG.
 *
 * @param[in] p_terminate_big Pointer to the structure containing the BIG termination parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_terminate_big(const struct ble_hci_le_terminate_big_cp *p_terminate_big);

/**
 * @brief Creates a BIG sync.
 *
 * @param[in] p_create_sync Pointer to the structure containing the BIG sync parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_big_create_sync(const struct ble_hci_le_big_create_sync_full_cp *p_create_sync);

/**
 * @brief Terminates a BIG sync.
 *
 * @param[in] p_terminate_sync Pointer to the structure containing the BIG sync termination parameters.
 * @param[out] p_terminate_sync_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_big_terminate_sync(const struct ble_hci_le_big_terminate_sync_cp *p_terminate_sync, struct ble_hci_le_big_terminate_sync_rp *p_terminate_sync_rp);