

/**
 *   @brief Function for calculating the database hash.
 *
 *   @param[out] database_hash Pointer to a buffer where the hash will be stored.
 *
 *   @note The hash is calculated based on the content of the database.
 */
void ble_gatts_calculate_database_hash(uint8_t database_hash[16]);

/**
 *   @brief Function for calculating the database hash for a specific connection handle.
 *
 *   @param[in] conn_handle Connection handle for which the hash will be calculated.
 *   @param[out] database_hash Pointer to a buffer where the hash will be stored.
 *
 *   @note The hash is calculated based on the content of the database for the specified connection handle.
 */
void ble_gatts_calculate_database_hash_by_conn_handle(uint16_t conn_handle, uint8_t database_hash[16]);
