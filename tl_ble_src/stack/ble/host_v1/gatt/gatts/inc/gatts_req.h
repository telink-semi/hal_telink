

/**
 *   @brief  Send a notification message to a client.
 *
 *   @param[in]   conn_handle  ACL Connection handle.
 *   @param[in]   attr_handle  Attribute handle.
 *   @param[in]   value        Pointer to the value to be notified.
 *   @param[in]   len          Length of the value to be notified.
 *
 *   @return BLE_HOST_ERR_SUCC if the handle value notification is sent successfully.
 */
int ble_gatts_notify(uint16_t conn_handle, uint16_t attr_handle, const uint8_t *value, uint16_t len);
