#pragma once
#define BLE_HCI_LE_MAX_SUPPORTED_CIS_COUNT 4 // Maximum number of supported CISes is 4,spec says 0-0x1F

struct ble_hci_le_set_cig_params_full_cp
{
    uint8_t                      cig_id;
    uint8_t                      sdu_interval_c_to_p[3];
    uint8_t                      sdu_interval_p_to_c[3];
    uint8_t                      worst_sca;
    uint8_t                      packing;
    uint8_t                      framing;
    uint16_t                     max_latency_c_to_p;
    uint16_t                     max_latency_p_to_c;
    uint8_t                      cis_count;
    struct ble_hci_le_cis_params cis[BLE_HCI_LE_MAX_SUPPORTED_CIS_COUNT];
} __attribute__((packed));

struct ble_hci_le_set_cig_params_full_rp
{
    uint8_t  cig_id;
    uint8_t  cis_count;
    uint16_t conn_handle[BLE_HCI_LE_MAX_SUPPORTED_CIS_COUNT];
} __attribute__((packed));

struct ble_hci_le_set_cig_params_test_full_cp
{
    uint8_t                           cig_id;
    uint8_t                           sdu_interval_c_to_p[3];
    uint8_t                           sdu_interval_p_to_c[3];
    uint8_t                           ft_c_to_p;
    uint8_t                           ft_p_to_c;
    uint16_t                          iso_interval;
    uint8_t                           worst_sca;
    uint8_t                           packing;
    uint8_t                           framing;
    uint8_t                           cis_count;
    struct ble_hci_le_cis_params_test cis[BLE_HCI_LE_MAX_SUPPORTED_CIS_COUNT];
} __attribute__((packed));

struct ble_hci_le_set_cig_params_test_full_rp
{
    uint8_t  cig_id;
    uint8_t  cis_count;
    uint16_t conn_handle[BLE_HCI_LE_MAX_SUPPORTED_CIS_COUNT];
} __attribute__((packed));

struct ble_hci_le_create_cis_full_cp
{
    uint8_t                             cis_count;
    struct ble_hci_le_create_cis_params cis[BLE_HCI_LE_MAX_SUPPORTED_CIS_COUNT];
} __attribute__((packed));

/**
 * @brief Sets up an ISO data path for the host.
 *
 * @param[in] p_cig_params Pointer to the structure containing the CIG parameters.
 * @param[out] p_cig_params_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_cig_params(const struct ble_hci_le_set_cig_params_full_cp *p_cig_params,
                                   struct ble_hci_le_set_cig_params_full_rp       *p_cig_params_rp);

/**
 * @brief Sets up an ISO data path for the host in test mode.
 *
 * @param[in] p_cig_params_test Pointer to the structure containing the CIG parameters for test mode.
 * @param[out] p_cig_params_test_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_cig_params_test(const struct ble_hci_le_set_cig_params_test_full_cp *p_cig_params_test,
                                        struct ble_hci_le_set_cig_params_test_full_rp       *p_cig_params_test_rp);

/**
 * @brief Creates a new CIS (Connection Information Structure).
 *
 * @param[in] p_create_cis Pointer to the structure containing the CIS parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_create_cis(const struct ble_hci_le_create_cis_full_cp *p_create_cis);

/**
 * @brief Removes a CIG (Connection Information Group).
 *
 * @param[in] p_remove_cig Pointer to the structure containing the CIG parameters to be removed.
 * @param[out] p_remove_cig_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_remove_cig(const struct ble_hci_le_remove_cig_cp *p_remove_cig,
                               struct ble_hci_le_remove_cig_rp       *p_remove_cig_rp);

/**
 * @brief Accepts a CIS (Connection Information Structure) request.
 *
 * @param[in] p_accept_cis_request Pointer to the structure containing the CIS request parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_accept_cis_request(const struct ble_hci_le_accept_cis_request_cp *p_accept_cis_request);

/**
 * @brief Rejects a CIS (Connection Information Structure) request.
 *
 * @param[in] p_reject_cis_request Pointer to the structure containing the CIS request parameters to be rejected.
 * @param[out] p_reject_cis_request_rp Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_reject_cis_request(const struct ble_hci_le_reject_cis_request_cp *p_reject_cis_request,
                                       struct ble_hci_le_reject_cis_request_rp       *p_reject_cis_request_rp);
