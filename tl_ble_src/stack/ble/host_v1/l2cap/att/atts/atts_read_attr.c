#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../../inc/ble_host.h"

#include "../inc/ble_att.h"
#include "../inc/ble_att_uuid.h"
#include "../inc/ble_att_service.h"
#include "../inc/ble_att_internal.h"
#include "../inc/uuid16bit.h"

#include "inc/atts_internal.h"



// ATT_OPCODE_READ_BY_TYPE_REQ
uint16_t ble_host_att_deal_read_by_type_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    const uint8_t *pAttrData = pdu->param;
    const struct atts_attribute *pAttr = NULL;
    struct atts_group *pGroup = NULL;
    uint16_t startHandle, endHandle;
    int err = ATT_SUCCESS;
    struct ble_att_pdu_format *tx_buff = att_deal_info->tx_buffer;
    uint8_t *rsp = tx_buff->param;

    uint16_t conn_handle = att_deal_info->conn_handle;
    uint16_t mtu = att_deal_info->mtu;
    uint16_t attLen;

    if (mtu < ATT_MINIMUM_MTU) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);

    uint32_t uuid_len = pdu_len - ATT_READ_TYPE_REQ_LEN;

    struct atts_group *pAttrGroup = ble_host_get_attribute_service_group_by_conn_handle(conn_handle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    } else if (!((uuid_len == ATT_16_UUID_LEN) || (uuid_len == ATT_128_UUID_LEN))) {
        err = ATT_ERR_INVALID_PDU;
    } else if ((startHandle == ATTR_HANDLE_NONE) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    }

    if (!err) {
        uint16_t handle = ble_atts_find_uuid_in_range(conn_handle, startHandle, endHandle, uuid_len, pAttrData, &pAttr, &pGroup);
        startHandle = handle;

        if (handle == ATTR_HANDLE_NONE) {
            err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
        } else if ((err = ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ,
            handle, pAttr->perm)) != ATT_SUCCESS) {

        } else {
            attLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
            uint8_t *attValue = pAttr->attrValue;

            if ((pAttr->settings & ATTS_SET_READ_CALLBACK) && (pGroup->readCallback != NULL)) {
                uint8_t *outValue = NULL;
                uint16_t outValueLen = 0;
                //read callback return is ATT_SUCCESS
                err = pGroup->readCallback(conn_handle, ATT_OPCODE_READ_BY_TYPE_REQ, startHandle, &outValue, &outValueLen);

                //if return error code out, stack cannot reply.
                if (err < ATT_SUCCESS || err >= 0x100) {
                    return 0;
                }
                if (err != ATT_SUCCESS) {
                    goto attsPushReadByTypeRsp;
                }
                if (outValueLen) {
                    attLen = outValueLen;
                    attValue = outValue;
                }
            }

            if (pAttr->settings & ATTS_SET_ATTR_VALUE_PROPERTIES) {
                const struct atts_attribute *pNextAttr = pAttr + 1;
                attLen = pNextAttr->uuidLen + 3;
                U8_TO_STREAM(rsp, attLen + 2);      //pair Length
                U16_TO_STREAM(rsp, handle);         //Attribute Handle
                U8_TO_STREAM(rsp, *pAttr->attrValue);   //Characteristic Properties
                U16_TO_STREAM(rsp, handle + 1); //Characteristic Value Handle
                STR_TO_STREAM(rsp, pNextAttr->uuid, pNextAttr->uuidLen);    //Characteristic UUID

            } else {
                U8_TO_STREAM(rsp, attLen + 2);
                U16_TO_STREAM(rsp, handle);
                STR_TO_STREAM(rsp, attValue, attLen);
            }

            attLen = min(attLen, mtu - ATT_READ_TYPE_RSP_LEN - 2);  //attrValueLen > MTU-2-2, attLen use min value.
            uint8_t maxListCnt = (mtu - ATT_READ_TYPE_RSP_LEN) / (attLen + 2);

            maxListCnt--;

            handle++;
            while (((handle = ble_atts_find_uuid_in_range(conn_handle, handle, endHandle, uuid_len,
                pAttrData, &pAttr, &pGroup)) != ATTR_HANDLE_NONE) && maxListCnt) {
                if (ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ,
                    handle, pAttr->perm) != ATT_SUCCESS) {
                    break;
                }
                uint16_t newAttLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;;
                attValue = pAttr->attrValue;
                if ((pAttr->settings & ATTS_SET_READ_CALLBACK) && (pGroup->readCallback != NULL)) {
                    uint8_t *outValue = NULL;
                    uint16_t outValueLen = 0;
                    //read callback return is ATT_SUCCESS
                    int err2 = pGroup->readCallback(conn_handle, ATT_OPCODE_READ_BY_TYPE_REQ, startHandle, &outValue, &outValueLen);

                    //if return error code out, stack cannot reply.
                    if (err2 != ATT_SUCCESS) {
                        break;
                    }

                    if (outValueLen) {
                        newAttLen = outValueLen;
                        attValue = outValue;
                    }
                }

                if (pAttr->settings & ATTS_SET_ATTR_VALUE_PROPERTIES) {
                    const struct atts_attribute *pNextAttr = pAttr + 1;
                    newAttLen = pNextAttr->uuidLen + 3;

                    if (newAttLen != attLen) {
                        break;
                    }
                    U16_TO_STREAM(rsp, handle);         //Attribute Handle
                    U8_TO_STREAM(rsp, *pAttr->attrValue);   //Characteristic Properties
                    U16_TO_STREAM(rsp, handle + 1); //Characteristic Value Handle
                    STR_TO_STREAM(rsp, pNextAttr->uuid, pNextAttr->uuidLen);    //Characteristic UUID
                    maxListCnt--;
                } else if (newAttLen == attLen) {
                    U16_TO_STREAM(rsp, handle);
                    STR_TO_STREAM(rsp, attValue, newAttLen);
                    maxListCnt--;
                } else {
                    break;
                }

                if (handle == ATTR_HANDLE_END_MAX) {
                    break;
                }

                if (++handle > endHandle) {
                    break;
                }
            }
        }
    }

attsPushReadByTypeRsp:
    if (err) {
        return ble_att_package_error_rsp(ATT_OPCODE_READ_BY_TYPE_REQ, startHandle, err, tx_buff);
    }
    tx_buff->opcode = ATT_OPCODE_READ_BY_TYPE_RSP;
    return rsp - &tx_buff->opcode;
}

static uint16_t ble_host_att_deal_read(const struct ble_host_att_deal_info *att_deal_info,
    uint16_t attrHandle, uint16_t valueOffset, uint8_t opcode)
{
    const struct atts_attribute *pAttr = NULL;
    struct atts_group *pGroup = NULL;
    int err = ATT_SUCCESS;
    uint16_t conn_handle = att_deal_info->conn_handle;
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;
    uint8_t *rsp = tx_buffer->param;
    uint8_t *outValue = NULL;
    uint16_t outValueLen = 0x0000;

    uint16_t mtu = att_deal_info->mtu;

    pAttr = ble_atts_get_service_group_by_handle(conn_handle, attrHandle, &pGroup);

    if (pAttr != NULL) {
        if ((err = ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ,
            attrHandle, pAttr->perm)) == ATT_SUCCESS) {

            if ((pAttr->settings & ATTS_SET_READ_CALLBACK) &&
                (pGroup->readCallback != NULL)) {
                //read callback return is ATT_SUCCESS
                err = pGroup->readCallback(conn_handle, valueOffset ? ATT_OPCODE_READ_BLOB_REQ : ATT_OPCODE_READ_REQ, attrHandle, &outValue, &outValueLen);
                //if return error code out, stack cannot reply.
                if (err < ATT_SUCCESS || err >= 0x100) {
                    return 0;
                }
                if (err != ATT_SUCCESS) {
                    goto attsPushReadRsp;
                }
            }

            if (err == ATT_SUCCESS && outValueLen == 0x0000) {
                outValue = pAttr->attrValue;
                outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;
            }

            if (valueOffset > outValueLen) {
                err = ATT_ERR_INVALID_OFFSET;
            }

            outValueLen = min(outValueLen - valueOffset, mtu - 1);
            outValue += valueOffset;
        }
    } else {
        err = ATT_ERR_INVALID_HANDLE;
    }

attsPushReadRsp:
    if (err) {
        return ble_att_package_error_rsp(opcode, attrHandle, err, tx_buffer);
    }

    tx_buffer->opcode = opcode == ATT_OPCODE_READ_REQ ? ATT_OPCODE_READ_RSP : ATT_OPCODE_READ_BLOB_RSP;
    STR_TO_STREAM(rsp, outValue, outValueLen);
    return rsp - &tx_buffer->opcode;
}

// ATT_OPCODE_READ_REQ
uint16_t ble_host_att_deal_read_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    (void) pdu_len;
    const uint8_t *pAttrData = pdu->param;
    uint16_t attrHandle = ATTR_HANDLE_NONE;
    STREAM_TO_U16(attrHandle, pAttrData);
    return ble_host_att_deal_read(att_deal_info, attrHandle, 0x0000, ATT_OPCODE_READ_REQ);
}

// ATT_OPCODE_READ_BLOB_REQ
uint16_t ble_host_att_deal_read_blob_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    (void) pdu_len;
    const uint8_t *pAttrData = pdu->param;
    uint16_t attrHandle = ATTR_HANDLE_NONE;
    uint16_t valueOffset = 0x0000;
    STREAM_TO_U16(attrHandle, pAttrData);
    STREAM_TO_U16(valueOffset, pAttrData);
    return ble_host_att_deal_read(att_deal_info, attrHandle, valueOffset, ATT_OPCODE_READ_BLOB_REQ);
}

// ATT_OPCODE_READ_MULTIPLE_REQ
uint16_t ble_host_att_deal_read_multiple_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    const struct atts_attribute *pAttr = NULL;
    struct atts_group *pGroup = NULL;
    int err = ATT_SUCCESS;
    uint16_t conn_handle = att_deal_info->conn_handle;
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;
    uint8_t *rsp = tx_buffer->param;
    uint16_t attHandle = 0;
    const uint8_t *pAttVal = pdu->param;
    uint8_t setOfHandles = (pdu_len - 1) >> 1;
    uint16_t mtu = att_deal_info->mtu;
    if (mtu < ATT_MINIMUM_MTU) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    } else if (setOfHandles == 0 || pdu_len > mtu) {
        setOfHandles = 0;
        err = ATT_ERR_INVALID_HANDLE;
    }

    int maxRspValLen = mtu - 1;

    while (setOfHandles--) {
        STREAM_TO_U16(attHandle, pAttVal);
        pAttr = ble_atts_get_service_group_by_handle(conn_handle, attHandle, &pGroup);
        if (pAttr == NULL) {
            err = ATT_ERR_INVALID_HANDLE;
            break;
        }

        if ((err = ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ, attHandle, pAttr->perm)) != ATT_SUCCESS) {
            break;
        }

        uint8_t *outValue = pAttr->attrValue;
        uint16_t outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;;
        if ((pAttr->settings & ATTS_SET_READ_CALLBACK) && (pGroup->readCallback != NULL)) {
            //read callback return is ATT_SUCCESS
            err = pGroup->readCallback(conn_handle, ATT_OPCODE_READ_MULTIPLE_REQ, attHandle, &outValue, &outValueLen);
            if (err != ATT_SUCCESS) {
                break;
            }

            if (outValueLen == 0x0000) {
                outValue = pAttr->attrValue;
                outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;;
            }
        }

        /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
        maxRspValLen -= outValueLen;
        if (maxRspValLen < 0) {
            outValueLen += maxRspValLen;
            STR_TO_STREAM(rsp, outValue, outValueLen);
            break;
        }
        STR_TO_STREAM(rsp, outValue, outValueLen);
    }

    if (err) {
        return ble_att_package_error_rsp(ATT_OPCODE_READ_MULTIPLE_REQ, attHandle, err, tx_buffer);
    }

    tx_buffer->opcode = ATT_OPCODE_READ_MULTIPLE_RSP;

    return min(mtu, rsp - &tx_buffer->opcode);
}


// ATT_OPCODE_READ_BY_GROUP_TYPE_REQ
uint16_t ble_host_att_deal_read_by_group_type_req(const struct ble_host_att_deal_info *att_deal_info,
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
    uint16_t attLen = 0;

    if (mtu < ATT_MINIMUM_MTU) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    }

    STREAM_TO_U16(startHandle, pAttrData);
    STREAM_TO_U16(endHandle, pAttrData);

    uint8_t uuid_len = pdu_len - ATT_READ_GROUP_TYPE_REQ_LEN;
    struct atts_group *pAttrGroup = ble_host_get_attribute_service_group_by_conn_handle(conn_handle);
    if (pAttrGroup == NULL) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    } else if (!((uuid_len == ATT_16_UUID_LEN) || (uuid_len == ATT_128_UUID_LEN))) {
        err = ATT_ERR_INVALID_PDU;
    } else if ((startHandle == ATTR_HANDLE_NONE) || (startHandle > endHandle)) {
        err = ATT_ERR_INVALID_HANDLE;
    } else if (!ble_uuid_cmp_uuid16_uuid(declarationsPrimaryServiceUuid, uuid_len, pAttrData)) {
        err = ATT_ERR_UNSUPPORTED_GROUP_TYPE;
    }

    if (!err) {
        uint16_t handle = ble_atts_find_uuid_in_range(conn_handle, startHandle, endHandle, uuid_len, pAttrData, &pAttr, &pGroup);
        attLen = *pAttr->attrValueLen;
        if (!((attLen == ATT_16_UUID_LEN) || (attLen == ATT_128_UUID_LEN))) {
            err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
        } else if (handle == ATTR_HANDLE_NONE) {
            err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
        } else if ((err = ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ,
            handle, pAttr->perm)) != ATT_SUCCESS) {
            startHandle = handle;
        } else {
            U8_TO_STREAM(rsp, attLen + 4);  //4: Attribute Handle and End Group Handle

            uint8_t maxListCnt = (mtu - ATT_READ_GROUP_TYPE_RSP_LEN) / (attLen + 4);
            U16_TO_STREAM(rsp, handle);
            handle = ble_atts_find_service_group_end_handle(conn_handle, handle);
            U16_TO_STREAM(rsp, handle);
            STR_TO_STREAM(rsp, pAttr->attrValue, attLen);
            maxListCnt--;

            while (maxListCnt) {
                if (handle == ATTR_HANDLE_END_MAX) {
                    break;
                }
                if (++handle > endHandle) {
                    break;
                }
                if ((handle = ble_atts_find_uuid_in_range(conn_handle, handle, endHandle, uuid_len,
                    pAttrData, &pAttr, &pGroup)) == ATTR_HANDLE_NONE) {
                    break;
                }
                if ((*pAttr->attrValueLen == attLen) &&
                    (ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ,
                        handle, pAttr->perm) == ATT_SUCCESS)) {
                    U16_TO_STREAM(rsp, handle);
                    handle = ble_atts_find_service_group_end_handle(conn_handle, handle);
                    U16_TO_STREAM(rsp, handle);
                    STR_TO_STREAM(rsp, pAttr->attrValue, attLen);
                    maxListCnt--;
                } else {
                    break;
                }
            }
        }
    }

    if (err) {
        return ble_att_package_error_rsp(ATT_OPCODE_READ_BY_GROUP_TYPE_REQ, startHandle, err, tx_buffer);
    }
    tx_buffer->opcode = ATT_OPCODE_READ_BY_GROUP_TYPE_RSP;
    return rsp - &tx_buffer->opcode;
}

// ATT_OPCODE_READ_MULTIPLE_VARIABLE_REQ TODO:
uint16_t ble_host_att_deal_read_multiple_variable_req(const struct ble_host_att_deal_info *att_deal_info,
    const struct ble_att_pdu_format *pdu, uint16_t pdu_len)
{
    const struct atts_attribute *pAttr = NULL;
    struct atts_group *pGroup = NULL;
    int err = ATT_SUCCESS;
    uint16_t conn_handle = att_deal_info->conn_handle;
    struct ble_att_pdu_format *tx_buffer = att_deal_info->tx_buffer;
    uint8_t *rsp = tx_buffer->param;
    uint16_t attHandle = 0;
    const uint8_t *pAttVal = pdu->param;
    uint8_t setOfHandles = (pdu_len - 1) >> 1;
    uint16_t mtu = att_deal_info->mtu;
    if (mtu < ATT_MINIMUM_MTU) {
        err = ATT_ERR_ATTRIBUTE_NOT_FOUND;
    } else if (setOfHandles == 0 || pdu_len > mtu) {
        setOfHandles = 0;
        err = ATT_ERR_INVALID_HANDLE;
    }

    int maxRspValLen = mtu - 1;

    while (setOfHandles--) {
        STREAM_TO_U16(attHandle, pAttVal);
        pAttr = ble_atts_get_service_group_by_handle(conn_handle, attHandle, &pGroup);
        if (pAttr == NULL) {
            err = ATT_ERR_INVALID_HANDLE;
            break;
        }

        if ((err = ble_atts_check_permissions(conn_handle, ATT_PERMISSIONS_READ, attHandle, pAttr->perm)) != ATT_SUCCESS) {
            break;
        }

        uint8_t *outValue = pAttr->attrValue;
        uint16_t outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;;
        if ((pAttr->settings & ATTS_SET_READ_CALLBACK) && (pGroup->readCallback != NULL)) {
            //read callback return is ATT_SUCCESS
            err = pGroup->readCallback(conn_handle, ATT_OPCODE_READ_MULTIPLE_VARIABLE_REQ, attHandle, &outValue, &outValueLen);
            if (err != ATT_SUCCESS) {
                break;
            }

            if (outValueLen == 0x0000) {
                outValue = pAttr->attrValue;
                outValueLen = pAttr->attrValueLen ? *pAttr->attrValueLen : 0;;
            }
        }

        /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
        maxRspValLen -= (outValueLen + 2);
        if (maxRspValLen < 0) {
            outValueLen += maxRspValLen;
            U16_TO_STREAM(rsp, *pAttr->attrValueLen);
            STR_TO_STREAM(rsp, outValue, outValueLen);
            break;
        }

        U16_TO_STREAM(rsp, *pAttr->attrValueLen);
        STR_TO_STREAM(rsp, pAttr->attrValue, *pAttr->attrValueLen);
    }

    if (err) {
        return ble_att_package_error_rsp(ATT_OPCODE_READ_MULTIPLE_VARIABLE_REQ, attHandle, err, tx_buffer);
    }

    tx_buffer->opcode = ATT_OPCODE_READ_MULTIPLE_VARIABLE_RSP;

    /* If the Set Of Values parameter is longer than (ATT_MTU-1) then only the first (ATT_MTU-1) octets shall be included in this response. */
    return min(mtu, rsp - &tx_buffer->opcode);
}

