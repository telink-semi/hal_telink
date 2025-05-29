#pragma once

/**
 *   @brief  Initialize the service discovery produce(SDP) module.
 *
 *   This function initializes the SDP module. It should be called once at the
 *   beginning of the application.
 *
 *   @return None.
 */
void ble_host_gatt_sdp_init(void);

/**
 *   @brief  Start the SDP service on a connection.
 *
 *   @param[in] conn_handle  Connection handle.
 *
 *   @return BLE_HOST_ERR_SUCC if the request is sent successfully, otherwise an error code.
 *
 *   @note   If SDP starts successfully, it can not stop by user.
 */
int ble_host_gatt_sdp_start(uint16_t conn_handle);
