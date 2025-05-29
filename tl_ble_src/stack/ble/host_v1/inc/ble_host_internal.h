
struct ble_acl_conn_complete {
    /** ACL connect handle */
    uint16_t conn_handle;
    /** Connected role, 0x00 is central, 0x01 is peripheral */
    uint8_t role;
    /** peer device address type, 0x00 public, 0x01 random */
    uint8_t peer_addr_type;
    /** peer device address */
    uint8_t peer_addr[6];
    /** local rpa address */
    uint8_t local_rpa[6];
    /** peer rpa address */
    uint8_t peer_rpa[6];
    /** connection interval , range 0x0006 to 0x0C80, unit 1.25ms */
    uint16_t conn_interval;
    /** connection latency , range 0x0000 to 0x01F3 */
    uint16_t conn_latency;
    /** supervision timeout, range 0x000A to 0x0C80, unit 10ms */
    uint16_t supervision_timeout;
    /** master clock accuracy, 0x00: 500ppm, 0x01: 250ppm, 0x02: 150ppm, 0x03: 100ppm
                               0x04: 75ppm, 0x05: 50ppm, 0x06: 30ppm, 0x07: 20ppm */
    uint8_t master_clock_acc;
};

/**
 *   @brief this function is used to allocate memory from Host memory pool.
 *
 *   @param[in] size: size of the memory to be allocated.
 *   @param[in] type_id: type ID of the memory to be allocated.
 *
 *   @return pointer to the allocated memory.
 */
void *ble_host_malloc(uint32_t size, uint16_t type_id);

/**
 *   @brief this function is used to free memory from L2CAP memory pool.
 *
 *   @param[in] ptr: pointer to the memory to be freed.
 *
 *   @return none.
 */
void ble_host_free(void *ptr);


/**
 *   @brief this function is used to lock the BLE host.
 *
 *   @return none.
 */
void ble_host_lock(void);

/**
 *   @brief this function is used to unlock the BLE host.
 *
 *   @return none.
 */
void ble_host_unlock(void);

/**
 *   @brief this function is used to reset the BLE host.
 *
 *   @param[in] reason: reason for the reset.
 *
 *   @return none.
 */
void ble_host_reset(int reason);

/**
 *   @brief this function is used to insert a connection complete event into the list of active connections.
 *
 *   @param[in] conn_complete: pointer to the connection complete event.
 *
 *   @return none.
 */
void ble_host_conn_insert_conn_complete(struct ble_acl_conn_complete *conn_complete);

/**
 *   @brief this function is used to remove a connection from the list of active connections.
 *
 *   @param[in] conn_handle: connection handle of the connection to be removed.
 *   @param[in] reason: reason for the disconnection.
 *
 *   @return none.
 */
void ble_host_conn_remove_conn(uint16_t conn_handle, uint8_t reason);
