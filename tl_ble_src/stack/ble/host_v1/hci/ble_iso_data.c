#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host.h"
#include "../inc/ble_host_sal.h"

#include "../l2cap/inc/ble_l2cap_interface.h"

#include "inc/ble_iso_data.h"
#include "inc/ble_hci_log.h"
#include "inc/ble_hci.h"

#define BLE_ISO_DATA_MALLOC(size) ble_host_hci_malloc(size, BLE_HOST_HCI_MALLOC_ISO_DATA_PACKET);
#define BLE_ISO_DATA_FREE(ptr)    ble_host_hci_free(ptr);

struct ble_host_iso_conn s_ble_host_iso_conn = {
    .conn_handle          = 0,
    .cfc_outstanding_pkts = 0,
    .hfc_completed_pkts   = 0,
};

static bool ble_host_reassemble_iso_data(struct ble_host_iso_conn *p_conn, uint8_t pb_flag, const uint8_t *p_data, uint16_t data_len)
{
    (void)p_conn;
    (void)pb_flag;
    (void)p_data;
    (void)data_len;
    return false;
}

void ble_host_receive_ble_iso_data(void *p_data)
{
    const struct ble_host_hci_iso *p_ble_iso_data = p_data;
    uint16_t                       conn_handle    = p_ble_iso_data->hdr.handle;

    // this code only supported Bluetooth LE ISO data.
    if ((conn_handle <= 0x0EFF)) {
        //        uint8_t pkt_sts_flag;
        uint8_t pb_flag = p_ble_iso_data->hdr.pbFlag;
        //        uint16_t data_total_len = p_ble_iso_data->hdr.dataTotalLen;
        //        uint8_t *p_iso_sdu_frag = p_ble_iso_data->data;
        //        uint16_t packet_seq_num, sdu_len = 0;
        //        uint32_t timestamp = 0;
        //
        //        if (pb_flag == BLE_HCI_ISO_PB_FIRST) {
        //            if (p_ble_iso_data->hdr.tsFlag) {
        //                struct hci_iso_data_ts *p_iso_data_ts = (struct hci_iso_data_ts *) p_iso_sdu_frag;
        //                timestamp = p_iso_data_ts->timestamp;
        //                packet_seq_num = p_iso_data_ts->packet_seq_num;
        //                sdu_len = p_iso_data_ts->sdu_len;
        //                pkt_sts_flag = p_iso_data_ts->pkt_sts_flag;
        //                p_iso_sdu_frag += sizeof(struct hci_iso_data_ts);
        //            } else {
        //                struct hci_iso_data *p_iso_data = (struct hci_iso_data *) p_iso_sdu_frag;
        //                packet_seq_num = p_iso_data->packet_seq_num;
        //                sdu_len = p_iso_data->sdu_len;
        //                pkt_sts_flag = p_iso_data->pkt_sts_flag;
        //                p_iso_sdu_frag += sizeof(struct hci_iso_data);
        //            }
        //
        //            BLE_HOST_HCI_ISO_DATA_INFO("HCI ISO receive first data: %s", hex_to_str(p_iso_sdu_frag, sdu_len));
        //        } else if (pb_flag == BLE_HCI_ISO_PB_CONTINUATION) {
        //            BLE_HOST_HCI_ISO_DATA_INFO("HCI ISO receive continue data: %s", hex_to_str(p_iso_sdu_frag, sdu_len));
        //        } else if (pb_flag == BLE_HCI_ISO_PB_COMPLETE) {
        //            BLE_HOST_HCI_ISO_DATA_INFO("HCI ISO receive complete data: %s", hex_to_str(p_iso_sdu_frag, sdu_len));
        //        } else if (pb_flag == BLE_HCI_ISO_PB_LAST) {
        //            BLE_HOST_HCI_ISO_DATA_INFO("HCI ISO receive last data: %s", hex_to_str(p_iso_sdu_frag, sdu_len));
        //        } else {
        //            BLE_HOST_HCI_ISO_DATA_ERROR("unknown pb_flag :0x%02x", pb_flag);
        //        }
        //
        //        //TODO: implement reassemble ISO data
        ble_host_reassemble_iso_data(NULL, pb_flag, NULL, 0);

        return;
    }

    BLE_HOST_HCI_ISO_DATA_ERROR("unknown iso handle :0x%03x", conn_handle);
}

static int ble_host_hci_iso_tx_segmented(struct ble_host_iso_conn *p_conn, struct ble_iso_fragment_pkt *p_send_data, uint32_t timestamp)
{
    /* Send the ISO data packet segmented */

    /* Consider for Time_Stamp if needed, Packet_sequence_number and ISO_SDU_Length fields */
    uint16_t add_info_len           = timestamp ? sizeof(struct hci_iso_data_ts) : sizeof(struct hci_iso_data); //  8 or 4 Bytes
    uint16_t data_total_len         = add_info_len + p_send_data->data_length;
    uint16_t max_hci_iso_payload_sz = ble_host_hci_max_iso_payload_sz();

    struct hci_iso_data        *p_iso_data;
    struct ble_host_hci_iso_h4 *p_iso_data_h4;
    uint8_t                    *p_iso_data_buff;
    uint16_t                    data_left = data_total_len;
    uint16_t                    packet_len;
    uint16_t                    offset = sizeof(struct ble_host_hci_iso_h4); //first packet offset
    uint8_t                     pb;

    while (data_left) {
        packet_len = min(max_hci_iso_payload_sz, data_left);
        if (data_left == data_total_len) {
            pb = BLE_HCI_ISO_PB_FIRST;
        } else if (packet_len == data_left) {
            pb = BLE_HCI_ISO_PB_LAST;
        } else {
            pb = BLE_HCI_ISO_PB_CONTINUATION;
        }

        /* fragements of ISO data packet */
        if (pb == BLE_HCI_ISO_PB_FIRST) {
            p_iso_data_buff = p_send_data->p_data;
        } else {
            p_iso_data_buff = p_send_data->p_data + offset - sizeof(struct ble_host_hci_iso_h4);
        }

        p_iso_data_h4                   = (struct ble_host_hci_iso_h4 *)p_iso_data_buff;
        p_iso_data_h4->type             = BLE_HCI_H4_ISO;
        p_iso_data_h4->hdr.handle       = p_conn->conn_handle;
        p_iso_data_h4->hdr.pbFlag       = pb;
        p_iso_data_h4->hdr.tsFlag       = 0; //clear it first, reset it in the first packet later
        p_iso_data_h4->hdr.dataTotalLen = packet_len;

        /* If PB_Flag equals 0b00 or 0b10, then the Packet_Sequence_Number, ISO_SDU_Length, and Packet_Status_Flag
           fields (plus the intermediate RFU field) shall all be present in the packet and the Time_Stamp field may
           be present. If PB_Flag equals 0b01 or 0b11, then none of these fields shall be included in the packet. */
        if (pb == BLE_HCI_ISO_PB_FIRST) {
            if (timestamp != 0) {
                p_iso_data_h4->hdr.tsFlag = 1;
                u32_to_bstream_le(timestamp, p_iso_data_h4->data);
                p_iso_data = (struct hci_iso_data *)(p_iso_data_h4->data + 4); //4 Bytes offset for timestamp
            } else {
                p_iso_data = (struct hci_iso_data *)p_iso_data_h4->data;
            }

            p_iso_data->packet_seq_num = 0;                        //TODO: implement packet sequence number
            p_iso_data->sdu_len        = p_send_data->data_length; //ISO_SDU_Length
        }

        /* ISO Data Payload already in the buffer, no need to copy it again */

        const char *dbg_str[] = {"first", "continue", "complete", "last"};
        BLE_HOST_HCI_ISO_DATA_INFO("HCI ISO send %s data: %s", dbg_str[pb], hex_to_str(p_iso_data_buff, packet_len + sizeof(struct ble_host_hci_iso_h4)));

        ble_host_hci_send_packet(p_iso_data_buff, packet_len + sizeof(struct ble_host_hci_iso_h4));

        offset += packet_len;
        data_left -= packet_len;
    }

    return BLE_HOST_ERR_SUCC;
}

static int ble_host_hci_iso_tx_complete(struct ble_host_iso_conn *p_conn, struct ble_iso_fragment_pkt *p_send_data, uint32_t timestamp)
{
    /* Send the ISO data packet directly */
    /* Consider for Time_Stamp if needed, Packet_sequence_number and ISO_SDU_Length fields */
    uint16_t add_info_len   = timestamp ? sizeof(struct hci_iso_data_ts) : sizeof(struct hci_iso_data); //  8 or 4 Bytes
    uint16_t data_total_len = add_info_len + p_send_data->data_length;

    /* The first (add_info_len+struct ble_host_hci_iso_h4) Bytes are reserved for the H4 ISO Data Header, and the remaining bytes are used for the ISO Data Payload.  */
    uint8_t                    *p_iso_data_buff = p_send_data->p_data;
    struct ble_host_hci_iso_h4 *p_iso_data_h4   = (struct ble_host_hci_iso_h4 *)p_iso_data_buff;

    p_iso_data_h4->type             = BLE_HCI_H4_ISO;
    p_iso_data_h4->hdr.handle       = p_conn->conn_handle;
    p_iso_data_h4->hdr.pbFlag       = BLE_HCI_ISO_PB_COMPLETE;
    p_iso_data_h4->hdr.tsFlag       = timestamp ? 1 : 0;
    p_iso_data_h4->hdr.dataTotalLen = data_total_len;
    struct hci_iso_data *p_iso_data;

    if (timestamp != 0) {
        u32_to_bstream_le(timestamp, p_iso_data_h4->data);
        p_iso_data = (struct hci_iso_data *)(p_iso_data_h4->data + 4); //4 Bytes offset for timestamp
    } else {
        p_iso_data = (struct hci_iso_data *)p_iso_data_h4->data;
    }

    p_iso_data->packet_seq_num = 0;                        //TODO: implement packet sequence number
    p_iso_data->sdu_len        = p_send_data->data_length; //ISO_SDU_Length

    /* ISO Data Payload already in the buffer, no need to copy it again */

    // BLE_HOST_HCI_ISO_DATA_INFO("HCI ISO send cpmplete data: %s", hex_to_str(p_iso_data_buff, data_total_len + sizeof(struct ble_host_hci_iso_h4)));

    ble_host_hci_send_packet(p_iso_data_buff, data_total_len + sizeof(struct ble_host_hci_iso_h4));

    return BLE_HOST_ERR_SUCC;
}

static void ble_host_send_iso_data_cb(struct ble_host_iso_conn *p_conn, struct ble_iso_fragment_pkt *p_packet, uint8_t status)
{
    BLE_HOST_HCI_ISO_DATA_DEBUG("ISO packet sent to controller callback, conn handle:0x%03x, status:%d", p_conn->conn_handle, status);

    if (status == BLE_HOST_TX_ISO_SUCCESS) {
        BLE_ISO_DATA_FREE(p_packet->p_data);
        BLE_ISO_DATA_FREE(p_packet);
    }
}

/**
 * @brief   Send HCI ISO data.
 *
 * @param[in]  p_conn - Pointer to the connection object.
 * @param[in]  p_data - Pointer to the data buffer.
 * @param[in]  data_len - Length of the data buffer.
 * @param[in]  timestamp - Timestamp of the data, optional, if timestamp is not needed, set it to 0.
 *
 * @return     0 on success, negative error code on failure.
 */
int ble_host_send_ble_iso_data(struct ble_host_iso_conn *p_conn, const uint8_t *p_data, uint16_t data_len, uint32_t timestamp)
{
    if (p_conn == NULL || p_data == NULL) {
        return BLE_HOST_ERR_BADDATA;
    }

    uint16_t                     add_info_len = timestamp ? sizeof(struct hci_iso_data_ts) : sizeof(struct hci_iso_data); //  8 or 4 Bytes
    struct ble_host_hci_iso_h4  *p_tx_pdu     = (struct ble_host_hci_iso_h4 *)BLE_ISO_DATA_MALLOC(add_info_len + data_len + sizeof(struct ble_host_hci_iso_h4));
    struct ble_iso_fragment_pkt *p_data_buff  = BLE_ISO_DATA_MALLOC(sizeof(struct ble_iso_fragment_pkt));

    if (p_data_buff == NULL || p_tx_pdu == NULL) {
        if (p_data_buff != NULL) {
            BLE_ISO_DATA_FREE(p_data_buff);
        }

        if (p_tx_pdu != NULL) {
            BLE_ISO_DATA_FREE(p_tx_pdu);
        }

        BLE_HOST_HCI_ISO_DATA_ERROR("send ISO data buffer malloc failed");
        return BLE_HOST_ERR_NOMEM;
    }

    /* clear the buffer */
    memset(p_data_buff, 0, sizeof(struct ble_iso_fragment_pkt));
    p_data_buff->p_data      = (uint8_t *)p_tx_pdu;
    p_data_buff->data_length = data_len; //!!!Special: here use ISO_SDU_Length, not plus the add_info_len
    p_data_buff->offset      = 0;
    p_data_buff->cb          = ble_host_send_iso_data_cb;

    /* !!!Special: here offset (add_info_len + sizeof(struct ble_host_hci_iso_h4)) bytes space  */
    /* copy ISO Data Payload in the buffer, no need to copy it again */
    memcpy(p_data_buff->p_data + add_info_len + sizeof(struct ble_host_hci_iso_h4), p_data, data_len);

    /* Consider for Time_Stamp if needed, Packet_sequence_number and ISO_SDU_Length fields */
    uint16_t data_total_len         = add_info_len + data_len;
    uint16_t max_hci_iso_payload_sz = ble_host_hci_max_iso_payload_sz();
    int      rc;

    /* In the Host to Controller direction, Data_Total_Length shall be less than or equal to
    the maximum ISO_Data_Packet_Length (returned by the LE Read Buffer Size command) */
    if (data_total_len > max_hci_iso_payload_sz) {
        rc = ble_host_hci_iso_tx_segmented(p_conn, p_data_buff, timestamp);
    } else {
        rc = ble_host_hci_iso_tx_complete(p_conn, p_data_buff, timestamp);
    }

    if (p_data_buff->cb != NULL) {
        /*  erroe code switch from ble_host_err_code to ble_host_tx_iso_status */
        p_data_buff->cb(p_conn, p_data_buff, rc == BLE_HOST_ERR_SUCC ? BLE_HOST_TX_ISO_SUCCESS : BLE_HOST_TX_ISO_ERROR_NO_MEMORY);
    }

    return rc;
}
