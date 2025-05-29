#pragma once

enum ble_host_tx_l2cap_status
{
    BLE_HOST_TX_L2CAP_SUCCESS = 0,
    BLE_HOST_TX_L2CAP_ERROR_DISCONNECTED,
};


// status is only valid if send_finish_callback is not NULL
typedef void (*tx_l2cap_finish_callback_t)(uint16_t conn_handle, void *cb_arg, uint8_t status);

struct ble_host_l2cap_packet
{
    uint16_t data_length;
    uint16_t channel_id;
    const uint8_t *p_data;
    tx_l2cap_finish_callback_t cb;
    void *cb_arg;
};

int ble_host_hci_send_acl_data(struct ble_host_conn *conn, struct ble_host_l2cap_packet *l2cap_packet);
