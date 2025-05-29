
// BASS: Battery Service Server.

struct ble_bass_register_param {
    uint8_t battery_level;        /** < range 0-100 */
    uint8_t power_state;
};

/**
 *   @brief Register the battery service server control.
 *
 *   @param[in] param Pointer to the structure containing the initial values for the service.
 *
 *   @note This function should be called before any other function in the BASS module.
 *
 *   @return None.
 */
void ble_basic_register_BAS_control_server(const struct ble_bass_register_param *param);

/**
 *   @brief Get the current battery level.
 *
 *   @return The current battery level.
 */
uint8_t ble_bass_get_battery_level(void);

/**
 *   @brief Set the current battery level.
 *
 *   @param[in] level The new battery level, range 0-100.
 *
 *   @return None.
 */
void ble_bass_set_battery_level(uint8_t level);

/**
 *   @brief Update the battery level for a given connection handle.
 *
 *   @param[in] conn_handle The connection handle for which to update the battery level.
 *   @param[in] level The new battery level, range 0-100.
 *
 *   @return BLE_HOST_ERR_SUCC if the update was successful, or an error code otherwise.
 *
 *   @note Even if the connection handle is invalid, the battery level is still updated.
 */
int ble_bass_update_battery_level(uint16_t conn_handle, uint8_t level);

/**
 *   @brief Get the current power state.
 *
 *   @return The current power state.
 */
uint8_t ble_bass_get_power_state(void);

/**
 *   @brief Set the current power state.
 *
 *   @param[in] state The new power state.
 *
 *   @return None.
 */
void ble_bass_set_power_state(uint8_t state);

/**
 *   @brief Update the power state for a given connection handle.
 *
 *   @param[in] conn_handle The connection handle for which to update the power state.
 *   @param[in] power_state The new power state.
 *
 *   @return BLE_HOST_ERR_SUCC if the update was successful, or an error code otherwise.
 *
 *   @note Even if the connection handle is invalid, the power state is still updated.
 */
int ble_bass_update_power_state(uint16_t conn_handle, uint8_t power_state);
