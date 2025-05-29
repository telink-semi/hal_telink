
#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"

#include "../inc/ble_l2cap.h"
#include "../inc/ble_l2cap_log.h"

#include "inc/ble_att.h"
#include "inc/ble_att_pdu_format.h"
#include "inc/ble_att_uuid.h"
#include "inc/ble_att_package.h"

/**
 * Format of the ATT_ERROR_RSP PDU
 * | Parameter                          | Size (octets)     |
 * +------------------------------------+-------------------+
 * | Attribute Opcode                   | 1                 |
 * | Request Opcode In Error            | 1                 |
 * | Attribute Handle In Error          | 2                 |
 * | Error Code                         | 1                 |
 */
uint16_t ble_att_package_error_rsp(uint8_t opcode, uint16_t handle, uint8_t reason, void *txBuff)
{
    uint8_t *buffer = txBuff; //skip dataLen field

    U8_TO_STREAM(buffer, ATT_OPCODE_ERROR_RSP);
    U8_TO_STREAM(buffer, opcode);
    U16_TO_STREAM(buffer, handle);
    U8_TO_STREAM(buffer, reason);

    return buffer - (uint8_t *) txBuff;
}

static void tx_complete_cb(uint16_t conn_handle, void *arg, uint16_t status)
{
    BLE_HOST_L2CAP_ATT_INFO("tx complete, conn_handle:0x%03x, status:%d", conn_handle, status);
    BLE_HOST_L2CAP_ATT_INFO("arg:%p", arg);
}

int ble_host_send_att_package(uint16_t conn_handle, uint16_t cid, const uint8_t *data, uint16_t data_len)
{
    struct ble_host_l2cap_tx_packet tx_packet = {
        .channel_id = cid,
        .data_length = data_len,
        .p_data = data,
        .tx_complete_cb = tx_complete_cb,
        .cb_arg = NULL,
    };
    return ble_host_l2cap_send_l2cap_data_by_conn_handle(conn_handle, &tx_packet);
}

uint16_t ble_host_get_att_mtu(uint16_t conn_handle, uint16_t cid)
{
    if (cid == LE_L2CAP_CID_ATT) {
        return ble_host_att_get_mtu(conn_handle);
    }

    return BLE_ATT_DEFAULT_MTU;
}

int ble_host_send_att_error_rsp(uint16_t conn_handle, uint16_t cid, uint8_t opcode, uint16_t handle, uint8_t reason)
{
    uint8_t buffer[] = { ATT_OPCODE_ERROR_RSP, opcode, U16_TO_BYTES(handle), reason };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Error Response: acl:0x%03x, cid:0x%04x, opcode=0x%02x, handle=0x%04x, reason=0x%02x",
        conn_handle, cid, opcode, handle, reason);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_error_rsp(uint16_t conn_handle, uint8_t opcode, uint16_t handle, uint8_t reason)
{
    return ble_host_send_att_error_rsp(conn_handle, LE_L2CAP_CID_ATT, opcode, handle, reason);
}

int ble_host_att_send_exchange_mtu_req(uint16_t conn_handle, uint16_t mtu)
{
    if (!ATT_MTU_CHECK(mtu)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[] = { ATT_OPCODE_EXCHANGE_MTU_REQ, U16_TO_BYTES(mtu) };
    BLE_HOST_L2CAP_ATT_INFO("Send ATT Exchange MTU Request: acl:0x%03x, mtu:0x%04x", conn_handle, mtu);
    int ret = ble_host_send_att_package(conn_handle, LE_L2CAP_CID_ATT, buffer, sizeof(buffer));

    if (ret == BLE_HOST_ERR_SUCC) {
        ble_host_att_set_mtu(conn_handle, mtu);
    }

    return ret;
}

int ble_host_att_send_exchange_mtu_rsp(uint16_t conn_handle, uint16_t mtu)
{
    if (!ATT_MTU_CHECK(mtu)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[] = { ATT_OPCODE_EXCHANGE_MTU_RSP, U16_TO_BYTES(mtu) };
    BLE_HOST_L2CAP_ATT_INFO("Send ATT Exchange MTU Response: acl:0x%03x, mtu:0x%04x", conn_handle, mtu);
    int ret = ble_host_send_att_package(conn_handle, LE_L2CAP_CID_ATT, buffer, sizeof(buffer));

    if (ret == BLE_HOST_ERR_SUCC) {
        ble_host_att_set_mtu(conn_handle, mtu);
    }

    return ret;
}

int ble_host_send_att_find_info_req(uint16_t conn_handle, uint16_t cid, uint16_t start_handle, uint16_t end_handle)
{
    if (start_handle > end_handle || start_handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }
    uint8_t buffer[] = { ATT_OPCODE_FIND_INFORMATION_REQ, U16_TO_BYTES(start_handle), U16_TO_BYTES(end_handle) };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Find Information Request: acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x",
        conn_handle, cid, start_handle, end_handle);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_find_info_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle)
{
    return ble_host_send_att_find_info_req(conn_handle, LE_L2CAP_CID_ATT, start_handle, end_handle);
}

int ble_host_send_att_find_info_rsp_16bit_uuid(uint16_t conn_handle, uint16_t cid,
    const struct att_info_16bit_uuid *info, uint16_t info_count)
{
    if (info == NULL || info_count == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }
    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    if (mtu < (info_count * sizeof(struct att_info_16bit_uuid) + ATT_FIND_INFO_RSP_LEN)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_FIND_INFO_RSP_LEN + (info_count * sizeof(struct att_info_16bit_uuid))];

    uint8_t *p = buffer;

    U8_TO_STREAM(p, ATT_OPCODE_FIND_INFORMATION_RSP);
    U8_TO_STREAM(p, ATT_INFO_FORMAT_16BIT_UUID);

    for (int i = 0; i < info_count; i++) {
        U16_TO_STREAM(p, info[i].handle);
        U16_TO_STREAM(p, info[i].uuid);
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Find Information Response: acl:0x%03x, cid:0x%04x, format:0x%02x, count:%d",
        conn_handle, cid, ATT_INFO_FORMAT_16BIT_UUID, info_count);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_send_att_find_info_rsp_128bit_uuid(uint16_t conn_handle, uint16_t cid,
    const struct att_info_128bit_uuid *info, uint16_t info_count)
{
    if (info == NULL || info_count == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }
    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    if (mtu < (info_count * sizeof(struct att_info_128bit_uuid) + ATT_FIND_INFO_RSP_LEN)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_FIND_INFO_RSP_LEN + (info_count * sizeof(struct att_info_128bit_uuid))];

    uint8_t *p = buffer;

    U8_TO_STREAM(p, ATT_OPCODE_FIND_INFORMATION_RSP);
    U8_TO_STREAM(p, ATT_INFO_FORMAT_128BIT_UUID);

    for (int i = 0; i < info_count; i++) {
        U16_TO_STREAM(p, info[i].handle);
        STR_TO_STREAM(p, info[i].uuid, sizeof(info[i].uuid));
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Find Information Response: acl:0x%03x, cid:0x%04x, format:0x%02x, count:%d",
        conn_handle, cid, ATT_INFO_FORMAT_128BIT_UUID, info_count);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_find_info_rsp_16bit_uuid(uint16_t conn_handle,
    const struct att_info_16bit_uuid *info, uint16_t info_count)
{
    return ble_host_send_att_find_info_rsp_16bit_uuid(conn_handle, LE_L2CAP_CID_ATT, info, info_count);
}

int ble_host_att_send_find_info_rsp_128bit_uuid(uint16_t conn_handle,
    const struct att_info_128bit_uuid *info, uint16_t info_count)
{
    return ble_host_send_att_find_info_rsp_128bit_uuid(conn_handle, LE_L2CAP_CID_ATT, info, info_count);
}

int ble_host_send_att_find_by_type_value_req(uint16_t conn_handle, uint16_t cid, uint16_t start_handle, uint16_t end_handle,
    uint16_t type, const uint8_t *value, uint16_t value_len)
{
    if (start_handle > end_handle || start_handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    if (mtu < ATT_FIND_TYPE_REQ_LEN + value_len) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    if (value_len != 0 && value == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint8_t buffer[ATT_FIND_TYPE_REQ_LEN + value_len];

    uint8_t *p = buffer;
    U8_TO_STREAM(p, ATT_OPCODE_FIND_BY_TYPE_VALUE_REQ);
    U16_TO_STREAM(p, start_handle);
    U16_TO_STREAM(p, end_handle);
    U16_TO_STREAM(p, type);
    if (value_len != 0) {
        memcpy(p, value, value_len);
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Find By Type Value Request: acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x, type:0x%04x, value_len:%d",
        conn_handle, cid, start_handle, end_handle, type, value_len);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_find_by_type_value_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
    uint16_t type, const uint8_t *value, uint16_t value_len)
{
    return ble_host_send_att_find_by_type_value_req(conn_handle, LE_L2CAP_CID_ATT, start_handle, end_handle,
        type, value, value_len);
}

int ble_host_send_att_find_by_type_value_rsp(uint16_t conn_handle, uint16_t cid,
    const struct attr_handle_group *handles_info, uint16_t info_count)
{
    if (handles_info == NULL || info_count == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    if (mtu < (info_count * sizeof(struct attr_handle_group) + ATT_FIND_TYPE_RSP_LEN)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_FIND_TYPE_RSP_LEN + (info_count * sizeof(struct attr_handle_group))];

    uint8_t *p = buffer;

    U8_TO_STREAM(p, ATT_OPCODE_FIND_BY_TYPE_VALUE_RSP);

    for (int i = 0; i < info_count; i++) {
        U16_TO_STREAM(p, handles_info[i].groupStartHandle);
        U16_TO_STREAM(p, handles_info[i].groupEndHandle);
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Find By Type Value Response: acl:0x%03x, cid:0x%04x, count:%d",
        conn_handle, cid, info_count);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_find_by_type_value_rsp(uint16_t conn_handle,
    const struct attr_handle_group *handles_info, uint16_t info_count)
{
    return ble_host_send_att_find_by_type_value_rsp(conn_handle, LE_L2CAP_CID_ATT, handles_info, info_count);
}

int ble_host_send_att_read_by_type_req(uint16_t conn_handle, uint16_t cid, uint16_t start_handle, uint16_t end_handle,
    const struct att_uuid *uuid)
{
    if (start_handle > end_handle || start_handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    if (CHECK_ATT_UUID(uuid)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_UUID);
    }

    uint8_t buffer[21] = { ATT_OPCODE_READ_BY_TYPE_REQ, U16_TO_BYTES(start_handle), U16_TO_BYTES(end_handle) };

    memcpy(buffer + ATT_READ_TYPE_REQ_LEN, uuid->uuid, uuid->uuidLength);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read By Type Request: acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x",
        conn_handle, cid, start_handle, end_handle);
    BLE_HOST_L2CAP_ATT_DEBUG("uuid is %s", ble_att_uuid_format(uuid));
    return ble_host_send_att_package(conn_handle, cid, buffer, uuid->uuidLength + ATT_READ_TYPE_REQ_LEN);
}

int ble_host_att_send_read_by_type_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
    const struct att_uuid *uuid)
{
    return ble_host_send_att_read_by_type_req(conn_handle, LE_L2CAP_CID_ATT, start_handle, end_handle, uuid);
}


// no used api, no better api define.
// int ble_host_send_att_read_by_type_rsp();
// int ble_host_att_send_read_by_type_rsp();

int ble_host_send_att_read_req(uint16_t conn_handle, uint16_t cid, uint16_t handle)
{
    if (handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    uint8_t buffer[3] = { ATT_OPCODE_READ_REQ, U16_TO_BYTES(handle) };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Request: acl:0x%03x, cid:0x%04x, handle:0x%04x",
        conn_handle, cid, handle);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_read_req(uint16_t conn_handle, uint16_t handle)
{
    return ble_host_send_att_read_req(conn_handle, LE_L2CAP_CID_ATT, handle);
}

int ble_host_send_att_read_rsp(uint16_t conn_handle, uint16_t cid, uint8_t *value, uint16_t value_len)
{
    if (value == NULL && value_len == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    value_len = min(mtu - ATT_READ_RSP_LEN, value_len);

    uint8_t buffer[ATT_READ_RSP_LEN + value_len];

    buffer[0] = ATT_OPCODE_READ_RSP;
    memcpy(buffer + ATT_READ_RSP_LEN, value, value_len);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Response: acl:0x%03x, cid:0x%04x, value_len:%d",
        conn_handle, cid, value_len);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_RSP_LEN + value_len);
}

int ble_host_att_send_read_rsp(uint16_t conn_handle, uint8_t *value, uint16_t value_len)
{
    return ble_host_send_att_read_rsp(conn_handle, LE_L2CAP_CID_ATT, value, value_len);
}

int ble_host_send_att_read_blob_req(uint16_t conn_handle, uint16_t cid, uint16_t handle, uint16_t offset)
{
    if (handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    uint8_t buffer[5] = { ATT_OPCODE_READ_BLOB_REQ, U16_TO_BYTES(handle), U16_TO_BYTES(offset) };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Blob Request: acl:0x%03x, cid:0x%04x, handle:0x%04x, offset:0x%04x",
        conn_handle, cid, handle, offset);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_read_blob_req(uint16_t conn_handle, uint16_t handle, uint16_t offset)
{
    return ble_host_send_att_read_blob_req(conn_handle, LE_L2CAP_CID_ATT, handle, offset);
}

int ble_host_send_att_read_blob_rsp(uint16_t conn_handle, uint16_t cid, uint8_t *value, uint16_t value_len)
{
    if (value == NULL && value_len == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    value_len = min(mtu - ATT_READ_BLOB_RSP_LEN, value_len);

    uint8_t buffer[ATT_READ_BLOB_RSP_LEN + value_len];

    buffer[0] = ATT_OPCODE_READ_BLOB_RSP;
    memcpy(buffer + ATT_READ_BLOB_RSP_LEN, value, value_len);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Blob Response: acl:0x%03x, cid:0x%04x, value_len:%d",
        conn_handle, cid, value_len);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_BLOB_RSP_LEN + value_len);
}

int ble_host_att_send_read_blob_rsp(uint16_t conn_handle, uint8_t *value, uint16_t value_len)
{
    return ble_host_send_att_read_blob_rsp(conn_handle, LE_L2CAP_CID_ATT, value, value_len);
}

int ble_host_send_att_read_mult_req(uint16_t conn_handle, uint16_t cid, uint16_t *handles, uint16_t count)
{
    if (count < 2 || handles == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    if (mtu < ATT_READ_MULT_REQ_LEN + count * 2) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_READ_MULT_REQ_LEN + count * 2];

    buffer[0] = ATT_OPCODE_READ_MULTIPLE_REQ;
    uint8_t *p = buffer + ATT_READ_MULT_REQ_LEN;
    for (int i = 0; i < count; i++) {
        U16_TO_STREAM(p, handles[i]);
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Multiple Request: acl:0x%03x, cid:0x%04x, count:%d",
        conn_handle, cid, count);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_MULT_REQ_LEN + count * 2);
}

int ble_host_att_send_read_mult_req(uint16_t conn_handle, uint16_t *handles, uint16_t count)
{
    return ble_host_send_att_read_mult_req(conn_handle, LE_L2CAP_CID_ATT, handles, count);
}

int ble_host_send_att_read_mult_rsp(uint16_t conn_handle, uint16_t cid, uint8_t *values, uint16_t values_len)
{
    if (values == NULL && values_len == 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    if (mtu < ATT_READ_MULT_RSP_LEN + values_len) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_READ_MULT_RSP_LEN + values_len];

    buffer[0] = ATT_OPCODE_READ_MULTIPLE_RSP;
    memcpy(buffer + ATT_READ_MULT_RSP_LEN, values, values_len);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Multiple Response: acl:0x%03x, cid:0x%04x, values_len:%d",
        conn_handle, cid, values_len);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_MULT_RSP_LEN + values_len);
}

int ble_host_att_send_read_mult_rsp(uint16_t conn_handle, uint8_t *values, uint16_t values_len)
{
    return ble_host_send_att_read_mult_rsp(conn_handle, LE_L2CAP_CID_ATT, values, values_len);
}

int ble_host_send_att_read_by_group_type_req(uint16_t conn_handle, uint16_t cid, uint16_t start_handle, uint16_t end_handle,
    const struct att_uuid *uuid)
{
    if (start_handle == ATTR_HANDLE_NONE || end_handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (CHECK_ATT_UUID(uuid)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_UUID);
    }

    uint8_t buffer[21] = { ATT_OPCODE_READ_BY_GROUP_TYPE_REQ, U16_TO_BYTES(start_handle), U16_TO_BYTES(end_handle) };

    memcpy(buffer + ATT_READ_GROUP_TYPE_REQ_LEN, uuid->uuid, uuid->uuidLength);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read By Group Type Request: acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x",
        conn_handle, cid, start_handle, end_handle);
    BLE_HOST_L2CAP_ATT_DEBUG("uuid is %s", ble_att_uuid_format(uuid));
    return ble_host_send_att_package(conn_handle, cid, buffer, uuid->uuidLength + ATT_READ_GROUP_TYPE_REQ_LEN);
}

int ble_host_att_send_read_by_group_type_req(uint16_t conn_handle, uint16_t start_handle, uint16_t end_handle,
    const struct att_uuid *uuid)
{
    return ble_host_send_att_read_by_group_type_req(conn_handle, LE_L2CAP_CID_ATT, start_handle, end_handle, uuid);
}

int ble_host_send_att_read_by_group_type_rsp(uint16_t conn_handle, uint16_t cid, uint8_t length,
    const struct attr_group_type_data *data, uint16_t count)
{
    if (data == NULL || count == 0 || length <= sizeof(struct attr_group_type_data)) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    uint8_t max_count = (mtu - ATT_READ_GROUP_TYPE_RSP_LEN) / length;
    count = min(count, max_count);

    uint8_t buffer[ATT_READ_GROUP_TYPE_RSP_LEN + count * length];

    buffer[0] = ATT_OPCODE_READ_BY_GROUP_TYPE_RSP;
    uint8_t *p = buffer + ATT_READ_GROUP_TYPE_RSP_LEN;
    for (int i = 0; i < count; i++) {
        memcpy(p, data + i, length);
        p += length;
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read By Group Type Response: acl:0x%03x, cid:0x%04x, count:%d",
        conn_handle, cid, count);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_GROUP_TYPE_RSP_LEN + count * length);
}

int ble_host_att_send_read_by_group_type_rsp(uint16_t conn_handle, uint8_t length,
    const struct attr_group_type_data *data, uint16_t count)
{
    return ble_host_send_att_read_by_group_type_rsp(conn_handle, LE_L2CAP_CID_ATT, length, data, count);
}

int ble_host_send_att_read_mult_variable_req(uint16_t conn_handle, uint16_t cid, uint16_t *handles, uint16_t count)
{
    if (count < 2 || handles == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }
    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    if (mtu < ATT_READ_MULT_VAR_REQ_LEN + count * 2) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_READ_MULT_VAR_REQ_LEN + count * 2];

    buffer[0] = ATT_OPCODE_READ_MULTIPLE_VARIABLE_REQ;
    uint8_t *p = buffer + ATT_READ_MULT_VAR_REQ_LEN;
    for (int i = 0; i < count; i++) {
        U16_TO_STREAM(p, handles[i]);
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Multiple Variable Request: acl:0x%03x, cid:0x%04x, count:%d",
        conn_handle, cid, count);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_MULT_VAR_REQ_LEN + count * 2);
}

int ble_host_att_send_read_mult_variable_req(uint16_t conn_handle, uint16_t *handles, uint16_t count)
{
    return ble_host_send_att_read_mult_variable_req(conn_handle, LE_L2CAP_CID_ATT, handles, count);
}

int ble_host_send_att_read_mult_variable_rsp(uint16_t conn_handle, uint16_t cid,
    const struct attr_value_tuple_send *values_tuple, uint16_t count)
{
    if (count < 2 || values_tuple == NULL) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint32_t total_length = 0;

    for (int i = 0; i < count; i++) {
        total_length += values_tuple[i].length + sizeof(struct attr_value_tuple);

        if (values_tuple[i].length != 0 && values_tuple[i].value == NULL) {
            return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
        }
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    if (mtu < ATT_READ_MULT_VAR_RSP_LEN + total_length) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }
    uint8_t buffer[ATT_READ_MULT_VAR_RSP_LEN + total_length];

    buffer[0] = ATT_OPCODE_READ_MULTIPLE_VARIABLE_RSP;
    uint8_t *p = buffer + ATT_READ_MULT_VAR_RSP_LEN;

    for (int i = 0; i < count; i++) {
        U16_TO_STREAM(p, values_tuple[i].length);
        STR_TO_STREAM(p, values_tuple[i].value, values_tuple[i].length);
    }

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Read Multiple Variable Response: acl:0x%03x, cid:0x%04x, count:%d",
        conn_handle, cid, count);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_READ_MULT_VAR_RSP_LEN + total_length);
}

int ble_host_att_send_read_mult_variable_rsp(uint16_t conn_handle,
    const struct attr_value_tuple_send *values_tuple, uint16_t count)
{
    return ble_host_send_att_read_mult_variable_rsp(conn_handle, LE_L2CAP_CID_ATT, values_tuple, count);
}

static int ble_host_att_send_write(uint16_t conn_handle, uint16_t cid, uint8_t opcode, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    if (value == NULL && length != 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    length = min(length, mtu - ATT_WRITE_CMD_LEN);

    uint8_t buffer[ATT_WRITE_CMD_LEN + length];
    uint8_t *p = buffer;
    U8_TO_STREAM(p, opcode);
    U16_TO_STREAM(p, handle);
    memcpy(p, value, length);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT %s: acl:0x%0ATT_WRITE_CMD_LENx, cid:0x%04x, handle:0x%04x, length:%d",
        opcode == ATT_OPCODE_WRITE_CMD ? "Write Command" : "Write Request", conn_handle, cid, handle, length);
    return ble_host_send_att_package(conn_handle, cid, buffer, 3 + length);
}

int ble_host_send_att_write_req(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    return ble_host_att_send_write(conn_handle, cid, ATT_OPCODE_WRITE_REQ, handle, value, length);
}

int ble_host_att_send_write_req(uint16_t conn_handle, uint16_t handle, const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_write_req(conn_handle, LE_L2CAP_CID_ATT, handle, value, length);
}

int ble_host_send_att_write_rsp(uint16_t conn_handle, uint16_t cid)
{
    uint8_t buffer[] = { ATT_OPCODE_WRITE_RSP };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Write Response: acl:0x%03x, cid:0x%04x", conn_handle, cid);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_write_rsp(uint16_t conn_handle)
{
    return ble_host_send_att_write_rsp(conn_handle, LE_L2CAP_CID_ATT);
}

int ble_host_send_att_write_cmd(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    return ble_host_att_send_write(conn_handle, cid, ATT_OPCODE_WRITE_CMD, handle, value, length);
}

int ble_host_att_send_write_cmd(uint16_t conn_handle, uint16_t handle, const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_write_cmd(conn_handle, LE_L2CAP_CID_ATT, handle, value, length);
}

int ble_host_att_send_signed_write_cmd(uint16_t conn_handle, uint16_t handle,
    const uint8_t *value, uint16_t length, const uint8_t signature[12])
{
    if (value == NULL && length != 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, LE_L2CAP_CID_ATT);
    length = min(length, mtu - ATT_SIGNED_WRITE_CMD_LEN);

    uint8_t buffer[ATT_SIGNED_WRITE_CMD_LEN + length];
    uint8_t *p = buffer;
    U8_TO_STREAM(p, ATT_OPCODE_SIGNED_WRITE_CMD);
    U16_TO_STREAM(p, handle);
    STR_TO_STREAM(p, value, length);
    memcpy(p, signature, 12);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Signed Write Command: acl:0x%03x, handle:0x%04x, length:%d",
        conn_handle, handle, length);
    return ble_host_send_att_package(conn_handle, LE_L2CAP_CID_ATT, buffer, ATT_SIGNED_WRITE_CMD_LEN + length);
}

static int ble_host_send_att_prepare_write(uint16_t conn_handle, uint16_t cid, bool is_req, uint16_t handle,
    uint16_t offset, const uint8_t *value, uint16_t length)
{
    if (value == NULL && length != 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    if (handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    if (mtu < ATT_PREP_WRITE_REQ_LEN + length) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_MTU_EXCEEDED);
    }

    uint8_t buffer[ATT_PREP_WRITE_REQ_LEN + length];
    uint8_t *p = buffer;
    U8_TO_STREAM(p, is_req ? ATT_OPCODE_PREPARE_WRITE_REQ : ATT_OPCODE_PREPARE_WRITE_RSP);
    U16_TO_STREAM(p, handle);
    U16_TO_STREAM(p, offset);
    memcpy(p, value, length);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Prepare Write %s: acl:0x%03x, cid:0x%04x, handle:0x%04x, offset:%d, length:%d",
        is_req ? "Request" : "Response", conn_handle, cid, handle, offset, length);
    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_PREP_WRITE_REQ_LEN + length);
}

int ble_host_send_att_prepare_write_req(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    uint16_t offset, const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_prepare_write(conn_handle, cid, true, handle, offset, value, length);
}

int ble_host_att_send_prepare_write_req(uint16_t conn_handle, uint16_t handle,
    uint16_t offset, const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_prepare_write_req(conn_handle, LE_L2CAP_CID_ATT, handle, offset, value, length);
}

int ble_host_send_att_prepare_write_rsp(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    uint16_t offset, const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_prepare_write(conn_handle, cid, false, handle, offset, value, length);
}

int ble_host_att_send_prepare_write_rsp(uint16_t conn_handle, uint16_t handle,
    uint16_t offset, const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_prepare_write_rsp(conn_handle, LE_L2CAP_CID_ATT, handle, offset, value, length);
}

int ble_host_send_att_execute_write_req(uint16_t conn_handle, uint16_t cid, uint8_t flags)
{
    if (flags != 0x00 && flags != 0x01) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint8_t buffer[] = { ATT_OPCODE_EXECUTE_WRITE_REQ, flags };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Execute Write Request: acl:0x%03x, cid:0x%04x, flags:0x%02x",
        conn_handle, cid, flags);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_execute_write_req(uint16_t conn_handle, uint8_t flags)
{
    return ble_host_send_att_execute_write_req(conn_handle, LE_L2CAP_CID_ATT, flags);
}

int ble_host_send_att_execute_write_rsp(uint16_t conn_handle, uint16_t cid)
{
    uint8_t buffer[] = { ATT_OPCODE_EXECUTE_WRITE_RSP };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Execute Write Response: acl:0x%03x, cid:0x%04x",
        conn_handle, cid);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_execute_write_rsp(uint16_t conn_handle)
{
    return ble_host_send_att_execute_write_rsp(conn_handle, LE_L2CAP_CID_ATT);
}

static int ble_host_send_att_server_initiated(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    uint16_t handle, const uint8_t *value, uint16_t length)
{
    if (handle == ATTR_HANDLE_NONE) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_ATT_INVALID_HANDLE);
    }

    if (value == NULL && length != 0) {
        return BLE_L2CAP_ERR(BLE_L2CAP_ERR_INVALID_PARAMS);
    }

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);
    length = min(length, mtu - ATT_VALUE_NTF_LEN);

    uint8_t buffer[ATT_VALUE_NTF_LEN + length];
    uint8_t *p = buffer;
    U8_TO_STREAM(p, opcode);
    U16_TO_STREAM(p, handle);
    memcpy(p, value, length);

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Server Initiated %s: acl:0x%03x, cid:0x%04x, handle:0x%04x, length:%d",
        opcode == ATT_OPCODE_HANDLE_VALUE_NTF ? "Notification" : "Indication", conn_handle, cid, handle, length);

    return ble_host_send_att_package(conn_handle, cid, buffer, ATT_VALUE_NTF_LEN + length);
}

int ble_host_send_att_handle_value_notification(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_server_initiated(conn_handle, cid, ATT_OPCODE_HANDLE_VALUE_NTF, handle, value, length);
}

int ble_host_att_send_handle_value_notification(uint16_t conn_handle, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_handle_value_notification(conn_handle, LE_L2CAP_CID_ATT, handle, value, length);
}

int ble_host_send_att_handle_value_indication(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_server_initiated(conn_handle, cid, ATT_OPCODE_HANDLE_VALUE_IND, handle, value, length);
}

int ble_host_att_send_handle_value_indication(uint16_t conn_handle, uint16_t handle,
    const uint8_t *value, uint16_t length)
{
    return ble_host_send_att_handle_value_indication(conn_handle, LE_L2CAP_CID_ATT, handle, value, length);
}

int ble_host_send_att_handle_value_confirmation(uint16_t conn_handle, uint16_t cid)
{
    uint8_t buffer[] = { ATT_OPCODE_HANDLE_VALUE_CFM };

    BLE_HOST_L2CAP_ATT_INFO("Send ATT Handle Value Confirmation: acl:0x%03x, cid:0x%04x",
        conn_handle, cid);
    return ble_host_send_att_package(conn_handle, cid, buffer, sizeof(buffer));
}

int ble_host_att_send_handle_value_confirmation(uint16_t conn_handle)
{
    return ble_host_send_att_handle_value_confirmation(conn_handle, LE_L2CAP_CID_ATT);
}
