#pragma once

/**
 * @brief Sets the event mask for the HCI LE.
 *
 * @param[in] p_le_event_mask Pointer to the structure containing the LE event mask.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_event_mask(const struct ble_hci_le_set_event_mask_cp *p_le_event_mask);

/**
 * @brief Reads the buffer size for the HCI LE.
 *
 * @param[out] p_le_buf_size Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_read_buffer_size(struct ble_hci_le_rd_buf_size_rp *p_le_buf_size);

/**
 * @brief Reads the buffer size v2 for the HCI LE.
 *
 * @param[out] p_le_buf_size_v2 Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_read_buffer_size_v2(struct ble_hci_le_rd_buf_size_v2_rp *p_le_buf_size_v2);

/**
 * @brief Sets the host feature for the HCI LE.
 *
 * @param[in] p_le_host_feature Pointer to the structure containing the LE host feature.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_host_feature(const struct ble_hci_le_set_host_feature_cp *p_le_host_feature);

/**
 * @brief Reads the local supported features for the HCI LE.
 *
 * @param[out] p_le_local_supp_feat Pointer to the structure to store the response parameters.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_read_local_supported_features(struct ble_hci_le_rd_loc_supp_feat_rp *p_le_local_supp_feat);

/**
 * @brief Set the random address for the HCI LE.
 * 
 * @param[in] p_le_rand_addr Pointer to the structure containing the LE random address.
 * 
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_set_random_address(const struct ble_hci_le_set_rand_addr_cp *p_le_rand_addr);

/**
 * @brief Start LE encryption for the HCI LE.
 * 
 * @param[in] p_le_start_enc Pointer to the structure containing the LE start encryption parameters.
 * 
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_start_encryption(const struct ble_hci_le_start_encrypt_cp *p_le_start_enc);

/**
 * @brief Generates a random number from the controller.
 * @param[out] dst Pointer to the buffer to store the random number.
 * @param[in] len Length of the buffer.
 *
 * @return int Returns 0 on success, or an error code on failure.
 */
int ble_host_hci_le_gen_rand(void *dst, int len);

/**
 * @brief LE Long Term Key Request Reply.
 * 
 * @param p_le_lt_key_req_reply Pointer to the structure containing the LE Long Term Key Request Reply parameters.
 * 
 * @return int Returns 0 on success, or an error code on failure.
 * @param p_le_lt_key_req_reply 
 * @return 
 */
int ble_host_hci_le_ltk_request_reply(struct ble_hci_le_lt_key_req_reply_cp *p_le_lt_key_req_reply);

/**
 * @brief LE Long Term Key Request Negative Reply.
 * 
 * @param p_le_lt_key_req_neg_reply Pointer to the structure containing the LE Long Term Key Request Negative Reply parameters.
 * 
 * @return int Returns 0 on success, or an error code on failure.   
 */
int ble_host_hci_le_ltk_request_negative_reply(struct ble_hci_le_lt_key_req_neg_reply_cp *p_le_lt_key_req_neg_reply);

int ble_host_hci_ltk_request_reply(uint16_t connHandle, uint8_t *ltk);

int ble_host_hci_ltk_request_negative_reply(uint16_t connHandle);
