#include "common/types.h"

#include "../../inc/ble_host.h"

#include "../inc/ble_l2cap.h"

int ble_host_signaling_send_data(struct ble_host_conn *conn, const uint8_t *p_data, uint16_t data_len)
{
    struct ble_host_l2cap_tx_packet tx_packet = {
        .channel_id = LE_L2CAP_CID_SIGNALING,
        .data_length = data_len,
        .p_data = p_data,
        .tx_complete_cb = NULL,
        .cb_arg = NULL,
    };

    return ble_host_l2cap_send_l2cap_data_sync(conn, &tx_packet);
}
