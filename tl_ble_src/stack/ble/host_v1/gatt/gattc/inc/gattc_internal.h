
#define GATTC_REQUEST_TIMEOUT_MS  (30000)  /** GATT client request timeout in milliseconds. */

/**
 *  GATT client when not receive expected response or error response, wait for timeout(GATTC_REQUEST_TIMEOUT_MS).
 *  call this callback function.
*/
typedef void(*gattc_req_timeout_func_t)(uint16_t conn_handle, uint16_t cid, void *user_data);

/**
 *  GATT client when receive error response, call this callback function.
*/
typedef void(*gattc_req_error_rsp_func_t)(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data);

/**
 *   GATT client when receive expected response, call this callback function.
 *
 *   @return true if the package is finished, otherwise false.
*/
typedef bool(*gattc_req_expect_rsp_func_t)(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data);

struct gattc_req_message {
    uint8_t request_opcode;                  /** ATT opcode to send. */
    uint8_t expect_opcode;                   /** Expected ATT opcode. */
    gattc_req_expect_rsp_func_t rsp_callback;/** Pointer to the expected response callback function. */
    gattc_req_error_rsp_func_t error_callback;  /** Pointer to the error response callback function. */
    gattc_req_timeout_func_t timeout_callback;     /** Pointer to the timeout callback function. */
    void *user_data;                         /** Pointer to the user data. */
};

typedef int(*gattc_req_message_handler_t)(uint16_t conn_handle, uint16_t cid, void *user_data);

/**
 *  @brief Send a GATT request message to the server.
 *
 *  @param[in] conn_handle Connection handle.
 *  @param[in] cid        Channel ID.
 *  @param[in] msg        Pointer to the request message.
 *  @param[in] handler    Pointer to the request message send handler.
 *
 *  @return refer to enum ble_err_code.
 *      - BLE_SUCCESS: Operation successful.
 *      - BLE_ERR_GATTC_ERR_HEAD(GATTC_ERR_INSUFFICIENT_RESOURCES) : Insufficient resources to start the procedure(malloc failed).
 *      - others: handler return value.
*/
int ble_host_gattc_send_request(uint16_t conn_handle, uint16_t cid, const struct gattc_req_message *msg,
    gattc_req_message_handler_t handler);

void ble_host_gattc_update_laster_message_info(uint16_t conn_handle, uint16_t cid,
    const struct gattc_req_message *msg, gattc_req_message_handler_t handler);
