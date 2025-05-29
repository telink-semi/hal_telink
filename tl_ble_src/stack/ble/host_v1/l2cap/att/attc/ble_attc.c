#include "common/types.h"
#include "common/utility.h"

#include "../../../inc/ble_host.h"
#include "../../../inc/ble_host_sal.h"

#include "../../inc/ble_l2cap.h"
#include "../../inc/ble_l2cap_log.h"

#include "../inc/ble_att.h"
#include "../inc/ble_att_internal.h"

#include "inc/ble_attc.h"

static ble_host_attc_recv_pdu_callback s_attc_recv_rsp_callback = NULL;
static ble_host_attc_recv_pdu_callback s_attc_recv_server_initiated_callback = NULL;

void ble_host_attc_register_rsp_recv_pdu_callback(ble_host_attc_recv_pdu_callback callback)
{
    s_attc_recv_rsp_callback = callback;
}

void ble_host_attc_register_server_initiated_recv_pdu_callback(ble_host_attc_recv_pdu_callback callback)
{
    s_attc_recv_server_initiated_callback = callback;
}

static void ble_host_attc_report_rsp_pdu(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len)
{
    if (s_attc_recv_rsp_callback) {
        s_attc_recv_rsp_callback(conn_handle, cid, opcode, pdu, pdu_len);
    }
}

static void ble_host_attc_report_server_initiated_pdu(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len)
{
    if (s_attc_recv_server_initiated_callback) {
        s_attc_recv_server_initiated_callback(conn_handle, cid, opcode, pdu, pdu_len);
    }
}

uint16_t ble_host_att_deal_error_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_exchange_mtu_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    if (att_deal_info->cid != LE_L2CAP_CID_ATT || pdu_len != ATT_MTU_RSP_LEN) {
        if (att_deal_info->cid != LE_L2CAP_CID_ATT) {
            BLE_HOST_L2CAP_ATT_ERROR("deal exchange MTU response failed, invalid CID(0x%04x)", att_deal_info->cid);
        }
        if (pdu_len != ATT_MTU_RSP_LEN) {
            BLE_HOST_L2CAP_ATT_ERROR("deal exchange MTU response failed, invalid ATT PDU length(%d)", pdu_len);
        }
        return 0;
    }
    const uint8_t *pAttrData = pdu->param;
    uint16_t MTU;
    STREAM_TO_U16(MTU, pAttrData);

    BLE_HOST_L2CAP_ATT_INFO("receive exchange MTU size is %d", MTU);
    if (!ATT_MTU_CHECK(MTU)) {
        BLE_HOST_L2CAP_ATT_INFO("mtu size is out of range, MTU=%d, range from 23 to 517", MTU);
        return 0;
    }

    ble_host_att_set_remote_mtu(att_deal_info->conn_handle, MTU);
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_find_information_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_find_by_type_value_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_read_by_type_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_read_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_read_blob_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_read_multiple_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_read_by_group_type_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_read_multiple_variable_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_write_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_prepare_write_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_execute_write_rsp(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_rsp_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode, pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_handle_value_ntf(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_server_initiated_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode,
        pdu->param, pdu_len - 1);
    return 0;
}

uint16_t ble_host_att_deal_handle_value_ind(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_server_initiated_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode,
        pdu->param, pdu_len - 1);

    att_deal_info->tx_buffer->opcode = ATT_OPCODE_HANDLE_VALUE_CFM;
    return ATT_VALUE_CFM_LEN;
}

uint16_t ble_host_att_deal_multiple_handle_value_ntf(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_attc_report_server_initiated_pdu(att_deal_info->conn_handle, att_deal_info->cid, pdu->opcode,
        pdu->param, pdu_len - 1);
    return 0;
}
