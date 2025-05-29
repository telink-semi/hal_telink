#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"

#include "../inc/ble_l2cap.h"
#include "../inc/ble_l2cap_log.h"

#include "inc/ble_att.h"
#include "inc/ble_att_pdu_format.h"
#include "inc/ble_att_service.h"
#include "inc/ble_att_internal.h"

#include "atts/inc/atts_packet.h"
#include "attc/inc/attc_packet.h"

#define BLE_ATT_CONN_INFO_MALLOC(size)      ble_host_l2cap_malloc(size, BLE_HOST_L2CAP_MALLOC_ATT_CONN_INFO)
#define BLE_ATT_CONN_INFO_FREE(ptr)         ble_host_l2cap_free(ptr)

#define GET_ATT_CONN_INFO(conn)             ((struct ble_att_conn_info *)(GET_L2CAP_CONN_INFO(conn)->att_info))
#define ATT_CONN_INFO_CHECK(conn)           (L2CAP_CONN_INFO_CHECK(conn) || GET_ATT_CONN_INFO(conn) == NULL)


struct att_common_param {
    uint16_t mtu;
    struct atts_group *atts_header;
};

struct ble_att_conn_info {
    uint16_t mtu;       /** < maximum transmission unit,  */
    uint16_t curr_mtu;
};

static struct att_common_param s_att_common_param = {
    .mtu = 23,  // BLE_ATT_DEFAULT_MTU
};

static const struct ble_host_att_process_handle s_att_process_handle[] = {
    [ATT_OPCODE_EXCHANGE_MTU_REQ] =
                {sizeof(struct att_exchange_mtu_req_format), ble_host_att_deal_exchange_mtu_req },
    [ATT_OPCODE_FIND_INFORMATION_REQ] =
                {sizeof(struct att_find_information_req_format), ble_host_att_deal_find_information_req },
    [ATT_OPCODE_FIND_BY_TYPE_VALUE_REQ] =
                {sizeof(struct att_find_by_type_value_req_format), ble_host_att_deal_find_by_type_value_req },
    [ATT_OPCODE_READ_BY_TYPE_REQ] =
                {sizeof(struct att_read_by_type_req_format), ble_host_att_deal_read_by_type_req },
    [ATT_OPCODE_READ_REQ] =
                {sizeof(struct att_read_req_format), ble_host_att_deal_read_req },
    [ATT_OPCODE_READ_BLOB_REQ] =
                {sizeof(struct att_read_blob_req_format), ble_host_att_deal_read_blob_req },
    [ATT_OPCODE_READ_MULTIPLE_REQ] =
                {sizeof(struct att_read_multiple_req_format), ble_host_att_deal_read_multiple_req },
    [ATT_OPCODE_READ_BY_GROUP_TYPE_REQ] =
                {sizeof(struct att_read_by_group_type_req_format), ble_host_att_deal_read_by_group_type_req },
    [ATT_OPCODE_READ_MULTIPLE_VARIABLE_REQ] =
                {sizeof(struct att_read_multiple_variable_req_format), ble_host_att_deal_read_multiple_variable_req },
    [ATT_OPCODE_WRITE_REQ] =
                {sizeof(struct att_write_req_format), ble_host_att_deal_write_req },

    [ATT_OPCODE_ERROR_RSP] =
                {sizeof(struct att_error_rsp_format), ble_host_att_deal_error_rsp },
    [ATT_OPCODE_EXCHANGE_MTU_RSP] =
                {sizeof(struct att_exchange_mtu_rsp_format), ble_host_att_deal_exchange_mtu_rsp },
    [ATT_OPCODE_FIND_INFORMATION_RSP] =
                {sizeof(struct att_find_information_rsp_format), ble_host_att_deal_find_information_rsp },
    [ATT_OPCODE_FIND_BY_TYPE_VALUE_RSP] =
                {sizeof(struct att_find_by_type_value_rsp_format), ble_host_att_deal_find_by_type_value_rsp },
    [ATT_OPCODE_READ_BY_TYPE_RSP] =
                {sizeof(struct att_read_by_type_rsp_format), ble_host_att_deal_read_by_type_rsp },
    [ATT_OPCODE_READ_RSP] =
                {sizeof(struct att_read_rsp_format), ble_host_att_deal_read_rsp },
    [ATT_OPCODE_READ_BLOB_RSP] =
                {sizeof(struct att_read_blob_rsp_format), ble_host_att_deal_read_blob_rsp },
    [ATT_OPCODE_READ_MULTIPLE_RSP] =
                {sizeof(struct att_read_multiple_rsp_format), ble_host_att_deal_read_multiple_rsp },
    [ATT_OPCODE_READ_BY_GROUP_TYPE_RSP] =
                {sizeof(struct att_read_by_group_type_rsp_format), ble_host_att_deal_read_by_group_type_rsp },
    [ATT_OPCODE_WRITE_RSP] =
                {sizeof(struct att_write_rsp_format), ble_host_att_deal_write_rsp },
    [ATT_OPCODE_PREPARE_WRITE_RSP] =
                {sizeof(struct att_prepare_write_rsp_format), ble_host_att_deal_prepare_write_rsp },
    [ATT_OPCODE_EXECUTE_WRITE_RSP] =
                {sizeof(struct att_execute_write_rsp_format), ble_host_att_deal_execute_write_rsp },
    [ATT_OPCODE_READ_MULTIPLE_VARIABLE_RSP] =
                {sizeof(struct att_read_multiple_variable_rsp_format), ble_host_att_deal_read_multiple_variable_rsp },

    [ATT_OPCODE_HANDLE_VALUE_NTF] =
                {sizeof(struct att_handle_value_ntf_format), ble_host_att_deal_handle_value_ntf },
    [ATT_OPCODE_HANDLE_VALUE_IND] =
                {sizeof(struct att_handle_value_ind_format), ble_host_att_deal_handle_value_ind },
    [ATT_OPCODE_MULTIPLE_HANDLE_VALUE_NTF] =
                {sizeof(struct att_multiple_handle_value_ntf_format), ble_host_att_deal_multiple_handle_value_ntf },
};

static const struct ble_host_att_process_handle s_att_write_cmd_handle = {
    sizeof(struct att_write_cmd_format), ble_host_att_deal_write_cmd
};

static const struct ble_host_att_process_handle s_att_signed_write_cmd_handle = {
    sizeof(struct att_signed_write_cmd_format), ble_host_att_deal_signed_write_cmd
};

/**
 *   @brief Initialize ATT layer.
 *
 *   @param[in] init_param Pointer to the ATT initialization parameters.
 *
 *   @return BLE_HOST_ERR_SUCC on success.
 *          -  BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS) if init_param is NULL.
 *          -  BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED) if the ATT MTU is exceeded.
 */
int ble_host_att_init(struct att_initial_param *init_param)
{
    if (init_param == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (!ATT_MTU_CHECK(init_param->mtu)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    s_att_common_param.mtu = init_param->mtu;

    return BLE_HOST_ERR_SUCC;
}

struct atts_group **ble_host_att_get_atts_header(void)
{
    return &s_att_common_param.atts_header;
}

/**
 *   @brief Send ATT packet immediately, only used for att layer ack request packet.
 *
 *   @param[in] conn Connection context pointer.
 *   @param[in] p_data Pointer to the data to be sent.
 *   @param[in] data_len Length of the data to be sent.
 *
 *   @return BLE_HOST_ERR_SUCC on success or error code on failure.
 */
int ble_host_att_send_packet_sync(struct ble_host_conn *conn, const uint8_t *p_data, uint16_t data_len)
{
    struct ble_host_l2cap_tx_packet tx_packet = {
        .channel_id = LE_L2CAP_CID_ATT,
        .data_length = data_len,
        .p_data = p_data,
        .tx_complete_cb = NULL,
        .cb_arg = NULL,
    };

    return ble_host_l2cap_send_l2cap_data_sync(conn, &tx_packet);
}

int ble_host_att_send_packet(uint16_t conn_handle, const uint8_t *p_data, uint16_t data_len,
    ble_l2cap_tx_complete_cb tx_complete_cb, void *cb_arg)
{
    struct ble_host_l2cap_tx_packet tx_packet = {
        .channel_id = LE_L2CAP_CID_ATT,
        .data_length = data_len,
        .p_data = p_data,
        .tx_complete_cb = tx_complete_cb,
        .cb_arg = cb_arg,
    };

    return ble_host_l2cap_send_l2cap_data_by_conn_handle(conn_handle, &tx_packet);
}

static uint16_t ble_host_att_get_mtu_by_conn(struct ble_host_conn *conn)
{
    if (ATT_CONN_INFO_CHECK(conn)) {
        return 0;
    }

    return GET_ATT_CONN_INFO(conn)->curr_mtu;
}

uint16_t ble_host_att_get_mtu(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    return ble_host_att_get_mtu_by_conn(conn);
}

int ble_host_att_set_mtu(uint16_t conn_handle, uint16_t mtu)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    if (ATT_CONN_INFO_CHECK(conn)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_CONN_HANDLE);
    }

    if (!ATT_MTU_CHECK(mtu)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    GET_ATT_CONN_INFO(conn)->mtu = mtu;

    return BLE_HOST_ERR_SUCC;
}

void ble_host_att_set_remote_mtu(uint16_t conn_handle, uint16_t mtu)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    GET_ATT_CONN_INFO(conn)->curr_mtu = min(mtu, GET_ATT_CONN_INFO(conn)->mtu);
}

uint16_t ble_host_att_get_local_mtu(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    return GET_ATT_CONN_INFO(conn)->mtu;
}

static uint16_t ble_host_att_rx_handler(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    uint8_t opcode = pdu->opcode;
    BLE_HOST_L2CAP_ATT_DEBUG("connHandle:0x%03x, receive ATT opcode:%d, param is %s",
        att_deal_info->conn_handle, opcode, hex_to_str(pdu->param, pdu_len - 1));
    struct ble_att_pdu_format *p_tx_buffer = att_deal_info->tx_buffer;

    const struct ble_host_att_process_handle *p_handle = NULL;

    if (opcode >= ARRAY_SIZE(s_att_process_handle)) {
        if (opcode == ATT_OPCODE_WRITE_CMD) {
            p_handle = &s_att_write_cmd_handle;
        } else if (opcode == ATT_OPCODE_SIGNED_WRITE_CMD) {
            p_handle = &s_att_signed_write_cmd_handle;
        }
    } else {
        p_handle = &s_att_process_handle[opcode];
    }

    if (p_handle != NULL && p_handle->expect_length != 0) {
        if (p_handle->expect_length > pdu_len) {
            BLE_HOST_L2CAP_ATT_ERROR("RX length check error. opcode:0x%02x, receive length:%d, expect min length:%d",
                opcode, pdu_len, p_handle->expect_length);
            return ble_att_package_error_rsp(opcode, ATTR_HANDLE_NONE, ATT_ERR_INVALID_PDU, p_tx_buffer);
        }

        BLE_HOST_L2CAP_ATT_DEBUG("deal function callback opcode:0x%02x", opcode);

        if (p_handle->att_rx_handler != NULL) {
            return p_handle->att_rx_handler(att_deal_info, pdu, pdu_len);
        }
    }

    if (opcode & ATTR_OP_FLAG_COMMAND) {
        return 0;   //Command Flag, don't send response.
    }

    BLE_HOST_L2CAP_ATT_INFO("stack not support ATT request, opcode:0x%02x", opcode); //debug
    return ble_att_package_error_rsp(opcode, ATTR_HANDLE_NONE, ATT_ERR_REQ_NOT_SUPPORTED, p_tx_buffer);
}

void ble_host_att_rx_packet_handler(struct ble_host_conn *conn, uint16_t len, uint8_t *att_packet)
{
    BLE_HOST_SAL_ASSERT(conn != NULL && att_packet != NULL);
    BLE_HOST_L2CAP_ATT_INFO("ATT received packet, conn handle:0x%03x, packet opcode:0x%02x, len:%d",
        conn->conn_handle, att_packet[0], len);

    uint16_t mtu = ble_host_att_get_mtu_by_conn(conn);
    uint8_t tx_buffer[mtu];
    struct ble_host_att_deal_info att_deal_info = {
        .conn_handle = conn->conn_handle,
        .cid = LE_L2CAP_CID_ATT,
        .mtu = mtu,
        .tx_buffer = (struct ble_att_pdu_format *) tx_buffer,
    };

    uint16_t tx_pdu_len = ble_host_att_rx_handler(&att_deal_info, (const struct ble_att_pdu_format *) att_packet, len);

    if (tx_pdu_len > 0) {
        BLE_HOST_L2CAP_ATT_INFO("connHandle:0x%03x, send ATT opcode:0x%x, len:%d",
            conn->conn_handle, att_deal_info.tx_buffer->opcode, tx_pdu_len);

        int err = ble_host_att_send_packet_sync(conn, tx_buffer, tx_pdu_len);
        if (err != BLE_HOST_ERR_SUCC) {
            BLE_HOST_L2CAP_ATT_ERROR("ATT send packet failed, err:0x%02x", err);
        }
        BLE_HOST_SAL_ASSERT(err == BLE_HOST_ERR_SUCC);
    }
}

static void ble_host_att_acl_connected(struct ble_host_conn *conn)
{
    BLE_HOST_L2CAP_ATT_DEBUG("ATT ACL connected, conn handle:0x%03x", conn->conn_handle);

    BLE_HOST_SAL_ASSERT(GET_L2CAP_CONN_INFO(conn) != NULL);

    struct ble_att_conn_info *att_conn_info = BLE_ATT_CONN_INFO_MALLOC(sizeof(struct ble_att_conn_info));

    if (att_conn_info == NULL) {
        BLE_HOST_L2CAP_ATT_ERROR("ATT connection info malloc failed");
        return;
    }

    GET_L2CAP_CONN_INFO(conn)->att_info = att_conn_info;
    att_conn_info->mtu = s_att_common_param.mtu;
    att_conn_info->curr_mtu = BLE_ATT_DEFAULT_MTU;
}

static void ble_host_att_acl_disconnected(struct ble_host_conn *conn)
{
    BLE_HOST_L2CAP_ATT_DEBUG("ATT ACL disconnected, conn handle:0x%03x", conn->conn_handle);
    BLE_HOST_SAL_ASSERT(!ATT_CONN_INFO_CHECK(conn));
    if (GET_ATT_CONN_INFO(conn) != NULL) {
        BLE_ATT_CONN_INFO_FREE(GET_ATT_CONN_INFO(conn));
    }
}

void ble_host_att_ctrl_handler(struct ble_host_conn *conn, uint8_t event, const void *param)
{
    BLE_HOST_SAL_ASSERT(conn != NULL);

    (void) param;
    switch (event) {
    case L2CAP_EVT_ACL_CONNECTED:
        ble_host_att_acl_connected(conn);
        break;

    case L2CAP_EVT_ACL_DISCONNECTED:
        ble_host_att_acl_disconnected(conn);
        break;

    default:
        break;
    }
}
