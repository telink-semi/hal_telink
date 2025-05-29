#pragma once

struct ble_gattc_ccc_value {
    uint16_t opcode;        /** < ATT opcode */
    uint16_t cid;           /** < Channel ID, if supported EATT */
    uint16_t attr_handle;   /** < Attribute handle */
    uint16_t value_length;  /** < Length of the message value */
    const uint8_t *value;   /** < Pointer to the message value */
};

struct ble_gattc_ccc_message {
    struct {
        struct ble_gattc_ccc_message *sle_next; /* next element */
    }next;      /** < SLIST_ENTRY(ble_gattc_ccc_message) next; */
    void(*report_callback)(uint16_t conn_handle, struct ble_gattc_ccc_value *ntf_value);
    uint16_t start_handle;      /** < Start handle of the notified attribute */
    uint16_t end_handle;        /** < End handle of the notified attribute */
};

/**
 *   @brief  Initialize the GATT Client module.
 *
 *   This function initializes the GATT Client module. It should be calleed
 *   before any other GATT Client function is called.
 *
 *   @return None.
 */
void ble_host_gattc_init(void);

/**
 *   @brief  Add a subscribe CCC message to the GATT Client module.
 *
 *   @param[in] conn_handle Connection handle.
 *   @param[in] ccc_msg Pointer to the CCC message to add.
 *
 *   @return True if the message was added successfully, False otherwise.
 */
bool ble_host_gattc_add_subscribe_ccc_message(uint16_t conn_handle, struct ble_gattc_ccc_message *ccc_msg);

/**
 *   @brief  Remove a subscribe CCC message from the GATT Client module.
 *
 *   @param[in] conn_handle Connection handle.
 *   @param[in] ccc_msg Pointer to the CCC message to remove.
 *
 *   @return True if the message was removed successfully, False otherwise.
 */
bool ble_host_gattc_remove_subscribe_ccc_message(uint16_t conn_handle, struct ble_gattc_ccc_message *ccc_msg);

/**
 *   @brief  Clean all subscribe CCC messages from the GATT Client module.
 *
 *   @param[in] conn_handle Connection handle.
 *
 *   @return None.
 */
void ble_host_gattc_clean_subscribe_ccc_message(uint16_t conn_handle);
