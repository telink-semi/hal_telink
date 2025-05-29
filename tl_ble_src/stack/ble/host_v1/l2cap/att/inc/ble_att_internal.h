
struct atts_group **ble_host_att_get_atts_header(void);

void ble_host_att_set_remote_mtu(uint16_t conn_handle, uint16_t mtu);

uint16_t ble_host_att_get_local_mtu(uint16_t conn_handle);

/**
 *  @brief packs an error response PDU with the given parameters.
 *
 *  @param[in] opcode The ATT opcode of the request that caused the error.
 *  @param[in] handle The handle of the attribute that caused the error.
 *  @param[in] reason The reason for the error refer to enum attribute_error_code.
 *  @param[out] txBuff The buffer to pack the error response into.
 *
 *  @return The length of the packed error response PDU.
*/
uint16_t ble_att_package_error_rsp(uint8_t opcode, uint16_t handle, uint8_t reason, void *txBuff);