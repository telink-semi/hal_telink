#include <string.h>
#include <sys/queue.h>

#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host.h"
#include "../inc/ble_host_sal.h"

#include "../l2cap/inc/ble_l2cap_interface.h"

#include "inc/ble_acl_data.h"
#include "inc/ble_hci_log.h"
#include "inc/ble_hci.h"

#define BLE_ACL_DATA_MALLOC(size) ble_host_hci_malloc(size, BLE_HOST_HCI_MALLOC_ACL_DATA_PACKET);
#define BLE_ACL_DATA_FREE(ptr)    ble_host_hci_free(ptr);

enum ble_l2cap_packet_type {
    L2CAP_TYPE_START, /** < L2CAP packet start or complete */
    L2CAP_TYPE_CONT,  /** < L2CAP packet continue */
};

static const char *s_broadcastFlagStr[] = {
    "Point-to-point (ACL-U or LE-U)",
    "BR/EDR broadcast (APB-U)",
    "Reserved for future use",
    "Reserved for future use" };

static recv_ble_acl_data_cb_t s_recv_ble_acl_data_cb = NULL;

void ble_host_acl_data_init(const recv_ble_acl_data_cb_t cb)
{
    s_recv_ble_acl_data_cb = cb;
}

void ble_host_send_ble_acl_data(struct ble_host_conn *p_conn, struct ble_acl_data_pkt *p_data_pkt)
{
    // TODO: implement this function
    (void) p_conn;
    (void) p_data_pkt;
}

void ble_host_receive_ble_acl_data(void *p_data)
{
    const struct ble_host_hci_acl_data *p_ble_acl_data = p_data;
    uint16_t                            conn_handle = p_ble_acl_data->handle;

    // this code only supported Bluetooth LE ACL data.
    if ((conn_handle < 0x0EFF) && (p_ble_acl_data->bcFlag == HCI_ACL_PC_FLAG_POINT_TO_POINT)) {
        bool is_start = false;
        switch (p_ble_acl_data->pbFlag) {
        case HCI_LE_ACL_PB_FLAG_CONTINUE_L2CAP:
            is_start = false;
            break;
        case HCI_LE_ACL_PB_FLAG_C_TO_H_START_L2CAP:
            is_start = true;
            break;
        default:
            BLE_HOST_HCI_ACL_DATA_ERROR("LE error packet boundary flag:%d", p_ble_acl_data->pbFlag);
            return; //error PB flag exit function.
        }

        struct ble_host_conn *p_conn = ble_host_conn_find_by_conn_handle(conn_handle);
        if (p_conn == NULL) {
            BLE_HOST_HCI_ACL_DATA_ERROR("LE ACL data for unknown connection handle:0x%03x", conn_handle);
            return;
        }

        BLE_HOST_HCI_ACL_DATA_DEBUG("LE Connect handle:0x%03x, type:%s, packet is %s", conn_handle, (is_start ? "start" : "continue"), hex_to_str(p_ble_acl_data->data, p_ble_acl_data->dataTotalLength));
        if (s_recv_ble_acl_data_cb != NULL) {
            // ble_host_l2cap_receive_l2cap_data
            s_recv_ble_acl_data_cb(p_conn, is_start, p_ble_acl_data->data, p_ble_acl_data->dataTotalLength);
        } else {
            BLE_HOST_HCI_ACL_DATA_ERROR("No callback for receive ble acl data");
        }
        return;
    }
    BLE_HOST_HCI_ACL_DATA_ERROR("unknown acl handle :0x%03x, BroadcastFlag:%s", conn_handle, s_broadcastFlagStr[p_ble_acl_data->bcFlag]);
}

struct ble_host_hci_acl_data_msg {
    STAILQ_ENTRY(ble_host_hci_acl_data_msg) next;
    tx_l2cap_finish_callback_t callback;
    uint16_t conn_handle;
    void *cb_arg;
};

static struct ble_host_hci_acl_data_msg s_acl_data_msg[16] = { 0 };

static STAILQ_HEAD(ble_host_hci_acl_data_msg_list, ble_host_hci_acl_data_msg) s_header = STAILQ_HEAD_INITIALIZER(s_header);

void ble_host_hci_send_acl_data_sync(void)
{
    if (STAILQ_FIRST(&s_header)) {
        struct ble_host_hci_acl_data_msg *p_msg = STAILQ_FIRST(&s_header);
        STAILQ_REMOVE_HEAD(&s_header, next);
        p_msg->callback(p_msg->conn_handle, p_msg->cb_arg, BLE_HOST_TX_L2CAP_SUCCESS);
        p_msg->callback = NULL;
    }
}

int ble_host_hci_send_acl_data(struct ble_host_conn *conn, struct ble_host_l2cap_packet *l2cap_packet)
{
    BLE_HOST_SAL_ASSERT(conn != NULL && l2cap_packet != NULL);
    BLE_HOST_SAL_ASSERT(l2cap_packet->p_data != NULL && l2cap_packet->data_length > 0);

    // 4 is L2cap header, channel id + pdu length
    int buffer_size = l2cap_packet->data_length + 4 + sizeof(struct ble_host_hci_acl_data_h4);
    uint8_t *hci_acl_data_buff = BLE_ACL_DATA_MALLOC(buffer_size);

    struct ble_host_hci_acl_data_h4 *p_acl_data = (struct ble_host_hci_acl_data_h4 *) hci_acl_data_buff;

    BLE_HOST_HCI_COMMON_CMD_INFO("HCI ACL send BC=0 PB=0, %s", hex_to_str(l2cap_packet->p_data, l2cap_packet->data_length));

    p_acl_data->type = BLE_HCI_H4_ACL;
    p_acl_data->handle = conn->conn_handle;
    p_acl_data->bcFlag = HCI_ACL_PC_FLAG_POINT_TO_POINT;
    p_acl_data->pbFlag = HCI_LE_ACL_PB_FLAG_H_TO_C_START_L2CAP;
    p_acl_data->dataTotalLength = l2cap_packet->data_length + 4;

    uint8_t *pData = p_acl_data->data;
    U16_TO_STREAM(pData, l2cap_packet->data_length);
    U16_TO_STREAM(pData, l2cap_packet->channel_id);

    memcpy(pData, l2cap_packet->p_data, l2cap_packet->data_length);

    ble_host_hci_send_packet(hci_acl_data_buff, buffer_size);

    BLE_ACL_DATA_FREE(hci_acl_data_buff);

    if (l2cap_packet->cb == NULL) {
        return 0;
    }

    for (int i = 0; i < 16; i++) {
        if (s_acl_data_msg[i].callback == NULL) {
            s_acl_data_msg[i].callback = l2cap_packet->cb;
            s_acl_data_msg[i].conn_handle = conn->conn_handle;
            s_acl_data_msg[i].cb_arg = l2cap_packet->cb_arg;
            STAILQ_INSERT_TAIL(&s_header, &s_acl_data_msg[i], next);
            break;
        }
    }

    return 0;
}
