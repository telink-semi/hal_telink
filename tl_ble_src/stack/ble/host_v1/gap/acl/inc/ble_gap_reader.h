#pragma once

/**
 * @brief Reads the remote feature of a BLE connection.
 *
 * This function initiates a read of the remote feature for the specified
 * connection handle.
 *
 * @param conn_handle The connection handle of the BLE connection.
 * @param features The features to be read from the remote device.
 */
void ble_host_gap_read_remote_feature(uint8_t conn_handle, const uint64_t features);

/**
 * @brief Reads the remote version of a BLE connection.
 *
 * This function initiates a read of the remote version for the specified
 * connection handle.
 *
 * @param conn_handle The connection handle of the BLE connection.
 * @param version The version to be read from the remote device.
 */
void ble_host_gap_read_remote_version(uint8_t conn_handle, const uint8_t version);