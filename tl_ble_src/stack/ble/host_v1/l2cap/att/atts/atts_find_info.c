#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../../inc/ble_host.h"

#include "../inc/ble_att.h"
#include "../inc/ble_att_uuid.h"
#include "../inc/ble_att_service.h"
#include "../inc/ble_att_pdu_format.h"
#include "../inc/ble_att_internal.h"
#include "../inc/uuid16bit.h"

#include "inc/atts_internal.h"

// ATT_OPCODE_FIND_INFORMATION_REQ
uint16_t ble_host_att_deal_find_information_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    (void) pdu_len;
    const uint8_t *pAttrData = pdu->param;
    const struct atts_attribute *pAttr = NULL;
    uint16_t startHandle, endHandle;
    uint8_t err = ATT_SUCCESS;
    uint16_t conn_handle = att_deal_info->conn_handle;
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;
    uint8_t *rsp = tx_buffer->param;

    uint16_t mtu = att_deal_info->mtu;

    if (mtu < ATT_MINIMUM_MTU) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);

    struct atts_group *pAttrGroup = ble_host_get_attribute_service_group_by_conn_handle(conn_handle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    } else if ((startHandle == 0) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    uint16_t handle = ble_atts_get_attribute_range_of_handle(conn_handle, startHandle, endHandle, &pAttr);
    if (handle == ATTR_HANDLE_NONE) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    }

    if (!err) {
        uint16_t uuidLen = pAttr->uuidLen;
        U8_TO_STREAM(rsp, uuidLen == ATT_16_UUID_LEN ? ATT_INFO_FORMAT_16BIT_UUID : ATT_INFO_FORMAT_128BIT_UUID);

        uint8_t maxListCnt = (mtu - ATT_FIND_INFO_RSP_LEN) / (uuidLen + 2);

        U16_TO_STREAM(rsp, handle);
        STR_TO_STREAM(rsp, pAttr->uuid, uuidLen);
        maxListCnt--;

        handle++;
        while (handle <= endHandle && maxListCnt) {
            handle = ble_atts_get_attribute_range_of_handle(conn_handle, handle, endHandle, &pAttr);
            if (handle == ATTR_HANDLE_NONE) {
                break;
            }

            if (pAttr->uuidLen != uuidLen) {
                break;
            }

            U16_TO_STREAM(rsp, handle);
            STR_TO_STREAM(rsp, pAttr->uuid, uuidLen);
            maxListCnt--;

            if (handle == ATTR_HANDLE_END_MAX) {
                break;
            }

            if (++handle > endHandle) {
                break;
            }
        }

    }

    if (err) {
        return ble_att_package_error_rsp(ATT_OPCODE_FIND_INFORMATION_REQ, startHandle, err, tx_buffer);
    }
    tx_buffer->opcode = ATT_OPCODE_FIND_INFORMATION_RSP;
    return rsp - &tx_buffer->opcode;
}

// ATT_OPCODE_FIND_BY_TYPE_VALUE_REQ
uint16_t ble_host_att_deal_find_by_type_value_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    const uint8_t *pAttrData = pdu->param;
    const struct atts_attribute *pAttr = NULL;
    struct atts_group *pGroup = NULL;
    uint16_t startHandle, endHandle;
    uint8_t err = ATT_SUCCESS;
    uint16_t conn_handle = att_deal_info->conn_handle;
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;
    uint8_t *rsp = tx_buffer->param;

    uint16_t mtu = att_deal_info->mtu;

    if (mtu < ATT_MINIMUM_MTU) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);
    const uint8_t *attrType = pAttrData;
    pAttrData += 2;

    struct atts_group *pAttrGroup = ble_host_get_attribute_service_group_by_conn_handle(conn_handle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    } else if ((startHandle == ATTR_HANDLE_NONE) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    uint8_t len = pdu_len - ATT_FIND_TYPE_REQ_LEN;

    if (!err) {
        uint16_t handle = startHandle;
        uint16_t nextHandle;
        uint8_t maxListCnt = (mtu - ATT_FIND_TYPE_RSP_LEN) >> 2;
        while (((handle = ble_atts_find_uuid_in_range(conn_handle, handle, endHandle, ATT_16_UUID_LEN,
            attrType, &pAttr, &pGroup)) != ATTR_HANDLE_NONE) && maxListCnt) {
            if ((pAttr->perm & ATT_PERMISSIONS_READ) &&
                ((len == 0) || ((len == *pAttr->attrValueLen) && (memcmp(pAttrData, pAttr->attrValue, len) == 0)))) {
                if (ble_uuid_cmp_uuid16_uuid(declarationsPrimaryServiceUuid, ATT_16_UUID_LEN, attrType)) {
                    nextHandle = ble_atts_find_service_group_end_handle(conn_handle, handle);
                } else {
                    nextHandle = handle;
                }
                U16_TO_STREAM(rsp, handle);
                U16_TO_STREAM(rsp, nextHandle);
                maxListCnt--;
            } else {
                nextHandle = handle;
            }

            if ((nextHandle >= endHandle) || (nextHandle == ATTR_HANDLE_END_MAX)) {
                break;
            }

            handle = nextHandle + 1;
        }

        if (tx_buffer->param == rsp) {
            err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
        }
    }

    if (err) {
        return ble_att_package_error_rsp(ATT_OPCODE_FIND_BY_TYPE_VALUE_REQ, startHandle, err, tx_buffer);
    }
    tx_buffer->opcode = ATT_OPCODE_FIND_BY_TYPE_VALUE_RSP;
    return rsp - &tx_buffer->opcode;
}
