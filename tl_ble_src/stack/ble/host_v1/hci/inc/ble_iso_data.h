#pragma once

#define BLE_HCI_ISO_CONN_HANDLE_MASK     (0x07ff)
#define BLE_HCI_ISO_PB_FLAG_MASK         (0x3000)
#define BLE_HCI_ISO_TS_FLAG_MASK         (0x4000)
#define BLE_HCI_ISO_LENGTH_MASK          (0x7fff)
#define BLE_HCI_ISO_SDU_LENGTH_MASK      (0x0fff)
#define BLE_HCI_ISO_PKT_STATUS_FLAG_MASK (0xC000)

#define BLE_HCI_ISO_HANDLE(ch, pb, ts)   ((ch) | ((pb) << 12) | ((ts) << 14))

#define BLE_HCI_ISO_CONN_HANDLE(h)       ((h) & BLE_HCI_ISO_CONN_HANDLE_MASK)
#define BLE_HCI_ISO_PB_FLAG(h)           (((h) & BLE_HCI_ISO_PB_FLAG_MASK) >> 12)
#define BLE_HCI_ISO_TS_FLAG(h)           (((h) & BLE_HCI_ISO_TS_FLAG_MASK) >> 14)
#define BLE_HCI_ISO_LENGTH(l)            ((l) & BLE_HCI_ISO_LENGTH_MASK)
#define BLE_HCI_ISO_SDU_LENGTH(l)        ((l) & BLE_HCI_ISO_SDU_LENGTH_MASK)
#define BLE_HCI_ISO_PKT_STATUS_FLAG(l)   (((l) & BLE_HCI_ISO_PKT_STATUS_FLAG_MASK) >> 14)

/* PB_Flag  */
#define BLE_HCI_ISO_PB_FIRST        (0)
#define BLE_HCI_ISO_PB_CONTINUATION (1)
#define BLE_HCI_ISO_PB_COMPLETE     (2)
#define BLE_HCI_ISO_PB_LAST         (3)

/* Packet_Status_Flag (in packets sent by the Controller) */
#define BLE_HCI_ISO_PKT_STATUS_VALID           0x00
#define BLE_HCI_ISO_PKT_STATUS_INVALID         0x01
#define BLE_HCI_ISO_PKT_STATUS_LOST            0x10

#define BLE_HCI_ISO_BIG_HANDLE_MIN             0x00
#define BLE_HCI_ISO_BIG_HANDLE_MAX             0xEF

#define BLE_HCI_ISO_BIG_ENCRYPTION_UNENCRYPTED 0x00
#define BLE_HCI_ISO_BIG_ENCRYPTION_ENCRYPTED   0x01

#define BLE_HCI_ISO_DATA_PATH_DIR_INPUT        0x00
#define BLE_HCI_ISO_DATA_PATH_DIR_OUTPUT       0x01

#define BLE_HCI_ISO_DATA_PATH_ID_HCI           0x00

#define BLE_HCI_ISO_HDR_SDU_LENGTH_MASK        (0x07ff)

/**
 * @brief Definition for HCI ISO data header format.
 * 
 */
struct ble_host_hci_iso_hdr
{
    uint16_t handle : 12;  /** < Connection handle */
    uint16_t pbFlag : 2;   /** < Packet boundary flag */
    uint16_t tsFlag : 1;   /** < Timestamp flag */
    uint16_t rfu    : 1;
    uint16_t dataTotalLen; /** < Total length of data */
} __attribute__((packed));

/**
 * @brief   Definition for HCI ISO data format.
 */
struct ble_host_hci_iso
{
    struct ble_host_hci_iso_hdr hdr;
    uint8_t                     data[0];
} __attribute__((packed));

/**
 *  @brief Definition for HCI ISO data H4 format.
 */
struct ble_host_hci_iso_h4
{
    uint8_t                     type;
    struct ble_host_hci_iso_hdr hdr;
    uint8_t                     data[0];
} __attribute__((packed));

struct hci_iso_data
{
    uint16_t packet_seq_num;
    uint16_t sdu_len      : 14;
    uint16_t pkt_sts_flag : 2; //only valid in packets sent by the Controller
    uint8_t  data[0];
} __attribute__((packed));

struct hci_iso_data_ts
{
    uint32_t timestamp;
    uint16_t packet_seq_num;
    uint16_t sdu_len      : 14;
    uint16_t pkt_sts_flag : 2; //only valid in packets sent by the Controller
    uint8_t  data[0];
} __attribute__((packed));

/* statement first */
struct ble_host_iso_conn;
struct ble_iso_fragment_pkt;

/* Status is only valid if send_finish_callback is not NULL */
typedef void (*iso_tx_finish_callback)(struct ble_host_iso_conn    *p_conn,
                                       struct ble_iso_fragment_pkt *p_data,
                                       uint8_t                      status);

struct ble_iso_fragment_pkt
{
    uint8_t               *p_data;
    uint16_t               data_length;
    uint16_t               offset;
    iso_tx_finish_callback cb;
};

struct ble_host_iso_conn
{
    /* Connection handle */
    uint16_t conn_handle;

    /* Controller flow control used*/
    uint16_t cfc_outstanding_pkts;
    /* Host flow control used */
    uint16_t hfc_completed_pkts;
};

enum ble_host_tx_iso_status
{
    BLE_HOST_TX_ISO_SUCCESS = 0,
    BLE_HOST_TX_ISO_ERROR_NO_MEMORY,
    BLE_HOST_TX_ISO_ERROR_DISCONNECTED,
};

/**
 * @brief   Definition for HCI ISO data format.
 * 
 * @param[in]  p_data - Pointer to the data buffer.
 * 
 * @return   None.
 */
void ble_host_receive_ble_iso_data(void *p_data);

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
int ble_host_send_ble_iso_data(struct ble_host_iso_conn *p_conn, const uint8_t *p_data, uint16_t data_len, uint32_t timestamp);
