#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../../inc/ble_host.h"

#include "../inc/ble_att.h"
#include "../inc/ble_att_service.h"
#include "../inc/ble_att_internal.h"

#include "inc/atts_internal.h"

static uint16_t ble_host_att_deal_write(const struct ble_host_att_deal_info *att_deal_info,
    uint8_t opcode, const uint8_t *pAttrData, uint16_t attLen)
{
    uint16_t attrHandle = ATTR_HANDLE_NONE;
    STREAM_TO_U16(attrHandle, pAttrData);
    attLen -= 2;    //attribute handle;
    const struct atts_attribute *pAttr;
    struct atts_group *pGroup;
    uint16_t conn_handle = att_deal_info->conn_handle;
    int err = ATT_SUCCESS;
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;

    pAttr = ble_atts_get_service_group_by_handle(conn_handle, attrHandle, &pGroup);

    if (pAttr == NULL) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    if (!err && ((err = ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_WRITE,
        attrHandle, pAttr->perm)) == ATT_SUCCESS)) {
        if ((pAttr->settings & ATTS_SET_WRITE_CALLBACK) && (pGroup->writeCallback != NULL)) {
            err = pGroup->writeCallback(conn_handle, opcode, attrHandle, (uint8_t *) (size_t) pAttrData, attLen);
            if (err < ATT_SUCCESS || err >= 0x100) {
                return 0;
            }
            if (err != ATT_SUCCESS) {
                goto attsPushWriteRsp;
            }
        }

        if ((pAttr->settings & ATTS_SET_ALLOW_WRITE) != 0) {
            if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) == 0) && (attLen != pAttr->maxAttrLen)) {
                err = ATT_ERR_INVALID_ATTRIBUTE_VALUE_LEN;
            } else if (((pAttr->settings & ATTS_SET_VARIABLE_LEN) != 0) && (attLen > pAttr->maxAttrLen)) {
                err = ATT_ERR_INVALID_ATTRIBUTE_VALUE_LEN;
            } else {
                memcpy(pAttr->attrValue, pAttrData, attLen);
                if (*(pAttr->attrValueLen) != attLen)
                    *(pAttr->attrValueLen) = attLen;
            }
        }
    }

attsPushWriteRsp:

    if (err) {
        return ble_att_package_error_rsp(opcode, attrHandle, err, tx_buffer);
    }

    tx_buffer->opcode = ATT_OPCODE_WRITE_RSP;
    return ATT_WRITE_RSP_LEN;
}

uint16_t ble_host_att_deal_write_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    return ble_host_att_deal_write(att_deal_info, pdu->opcode, pdu->param, pdu_len - sizeof(struct ble_att_pdu_format));
}

uint16_t ble_host_att_deal_write_cmd(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    ble_host_att_deal_write(att_deal_info, pdu->opcode, pdu->param, pdu_len - sizeof(struct ble_att_pdu_format));
    return 0;       //command not need response
}

uint16_t ble_host_att_deal_signed_write_cmd(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    (void) att_deal_info;(void) pdu;(void) pdu_len;
    return 0;       //command not need response
}
