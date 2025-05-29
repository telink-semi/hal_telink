
#include "common/types.h"
#include "common/utility.h"

#include "../../../inc/ble_host.h"
#include "../../../inc/ble_host_sal.h"

#include "../../inc/ble_l2cap.h"
#include "../../inc/ble_l2cap_log.h"

#include "../inc/ble_att.h"
#include "../inc/ble_att_internal.h"

uint16_t ble_host_att_deal_exchange_mtu_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;
    if (att_deal_info->cid != LE_L2CAP_CID_ATT || pdu_len != ATT_MTU_REQ_LEN) {
        if (att_deal_info->cid != LE_L2CAP_CID_ATT) {
            BLE_HOST_L2CAP_ATT_ERROR("deal exchange MTU response failed, invalid CID(0x%04x)", att_deal_info->cid);
        }
        if (pdu_len != ATT_MTU_REQ_LEN) {
            BLE_HOST_L2CAP_ATT_ERROR("deal exchange MTU response failed, invalid ATT PDU length(%d)", pdu_len);
        }
        return ble_att_package_error_rsp(ATT_OPCODE_EXCHANGE_MTU_REQ, 0x00, ATT_ERR_INVALID_PDU, tx_buffer);
    }
    const uint8_t *pAttrData = pdu->param;
    uint16_t MTU;
    STREAM_TO_U16(MTU, pAttrData);

    BLE_HOST_L2CAP_ATT_INFO("receive exchange MTU size is %d", MTU);
    if (!ATT_MTU_CHECK(MTU)) {
        BLE_HOST_L2CAP_ATT_INFO("mtu size is out of range, MTU=%d, range from 23 to 517", MTU);
        return ble_att_package_error_rsp(ATT_OPCODE_EXCHANGE_MTU_REQ, 0x00, ATT_ERR_INVALID_PDU, tx_buffer);
    }

    ble_host_att_set_remote_mtu(att_deal_info->conn_handle, MTU);
    tx_buffer->opcode = ATT_OPCODE_EXCHANGE_MTU_RSP;
    uint8_t *pParam = tx_buffer->param;
    U16_TO_STREAM(pParam, ble_host_att_get_local_mtu(att_deal_info->conn_handle));
    return ATT_MTU_RSP_LEN;
}
