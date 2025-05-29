

/**
 *   @brief  ATT client receive PDU callback function type.
 *
 *   This function is called by the ATT client when a PDU is received from the server.
 *
 *   @param[in] conn_handle Connection handle.
 *   @param[in] cid ATT channel ID.
 *   @param[in] opcode ATT operation code.
 *   @param[in] pdu Pointer to the PDU data.
 *   @param[in] pdu_len Length of the PDU data.
 *
 *   @return None.
 */
typedef void(*ble_host_attc_recv_pdu_callback)(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len);

/**
 *   @brief  Register ATT client receive PDU callback function.
 *
 *   @param[in] callback Pointer to the callback function.
 *
 *   @return None.
 *
 *   @note If Att client module receive response from server, it will call this callback function.
 */
void ble_host_attc_register_rsp_recv_pdu_callback(ble_host_attc_recv_pdu_callback callback);

/**
 *   @brief  Register ATT client server initiated receive PDU callback function.
 *
 *   @param[in] callback Pointer to the callback function.
 *
 *   @return None.
 *
 *   @note If Att client module receive Notification, Indication or multiple Notifications from server,
 *          it will call this callback function.
 */
void ble_host_attc_register_server_initiated_recv_pdu_callback(ble_host_attc_recv_pdu_callback callback);

