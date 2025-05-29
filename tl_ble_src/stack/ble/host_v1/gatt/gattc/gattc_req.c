#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../inc/ble_host_sal.h"
#include "../../inc/ble_host.h"

#include "../../l2cap/inc/ble_l2cap.h"
#include "../../l2cap/att/inc/ble_att.h"
#include "../../l2cap/att/inc/ble_att_uuid.h"
#include "../../l2cap/att/inc/uuid_def.h"
#include "../../l2cap/att/inc/uuid16bit.h"
#include "../../l2cap/att/inc/ble_att_pdu_format.h"
#include "../../l2cap/att/inc/ble_att_package.h"


#include "../inc/gatt.h"
#include "../inc/ble_gatt_log.h"
#include "../inc/gatt_internal.h"

#include "inc/gattc_internal.h"
#include "inc/gattc_req.h"
#include "inc/gattc_req_internal.h"


static bool ble_host_gattc_recv_exchange_mtu_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    (void) cid;
    (void) len;

    uint16_t mtu;
    STREAM_TO_U16(mtu, pdu);
    BLE_HOST_GATT_CLIENT_DEBUG("receive MTU exchange response, acl:0x%03x, mtu:%d", conn_handle, mtu);

    const struct gattc_exchange_mtu_param *param = user_data;
    if (param->callback != NULL) {
        param->callback(conn_handle, 0, mtu);
    }

    return true;
}

static void ble_host_gattc_exchange_mtu_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    BLE_HOST_GATT_CLIENT_WARN("MTU exchange timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    const struct gattc_exchange_mtu_param *param = user_data;

    if (param->callback != NULL) {
        param->callback(conn_handle, GATT_REQUEST_ERR_TIMEOUT, 0);
    }
}

static void ble_host_gattc_recv_exchange_mtu_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    (void) cid;
    BLE_HOST_GATT_CLIENT_ERROR("MTU exchange error, acl:0x%03x, handle:0x%04x, error:0x%02x",
        conn_handle, handle, error);

    const struct gattc_exchange_mtu_param *param = user_data;

    if (param != NULL && param->callback != NULL) {
        param->callback(conn_handle, error, 0);
    }
}

static int ble_host_gattc_deal_exchange_mtu_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    (void) cid;
    const struct gattc_exchange_mtu_param *param = user_data;
    BLE_HOST_GATT_CLIENT_DEBUG("Send MTU exchange request, acl:0x%03x, MTU:%d", conn_handle, param->MTU);

    return ble_host_att_send_exchange_mtu_req(conn_handle, param->MTU);
}

int ble_host_gattc_send_exchange_mtu_req(uint16_t conn_handle, uint16_t mtu, gattc_exchange_mtu_callback callback)
{
    if (!ATT_MTU_CHECK(mtu)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (!ble_host_gattc_is_connected(conn_handle, LE_L2CAP_CID_ATT)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    struct gattc_exchange_mtu_param *param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_exchange_mtu_param));

    if (param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    param->MTU = mtu;
    param->callback = callback;

    struct gattc_req_message exchange_mtu_req = {
        .request_opcode = ATT_OPCODE_EXCHANGE_MTU_REQ,
        .expect_opcode = ATT_OPCODE_EXCHANGE_MTU_RSP,
        .rsp_callback = ble_host_gattc_recv_exchange_mtu_rsp,
        .error_callback = ble_host_gattc_recv_exchange_mtu_error,
        .timeout_callback = ble_host_gattc_exchange_mtu_timeout,
        .user_data = param,
    };

    return ble_host_gattc_send_request(conn_handle, LE_L2CAP_CID_ATT, &exchange_mtu_req, ble_host_gattc_deal_exchange_mtu_req);
}

static bool ble_host_gattc_discover_all_services_continue(uint16_t conn_handle, uint16_t cid, uint16_t start_handle,
    struct gattc_disc_all_services *disc_services)
{
    if (start_handle == GATT_ATTR_HANDLE_END) {
        BLE_HOST_GATT_CLIENT_DEBUG("discovery complete");
    } else {
        disc_services->start_handle = start_handle + 1;
        if (disc_services->start_handle <= disc_services->end_handle) {
            return false;
        }
    }

    struct gattc_disc_services val = {
        .user_data = disc_services->user_data,
    };
    disc_services->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static bool ble_host_gattc_recv_read_by_group_type_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    BLE_HOST_GATT_CLIENT_DEBUG("receive read by group type response, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    const struct gattc_read_by_group_type_param *param = user_data;

    uint8_t pair_len;

    STREAM_TO_U8(pair_len, pdu);
    len--;

    struct gattc_disc_services val = {
        .user_data = param->user_data,
    };

    switch (pair_len) {
        case ATT_16_UUID_LEN + sizeof(struct attr_group_type_data) : {
            val.service_uuid.uuidLength = ATT_16_UUID_LEN;
        }break;
        case ATT_128_UUID_LEN + sizeof(struct attr_group_type_data) : {
            val.service_uuid.uuidLength = ATT_128_UUID_LEN;
        }break;
        default: {
            BLE_HOST_GATT_CLIENT_ERROR("invalid pair length: %d", pair_len);
            goto done;
        }break;
    }

    int32_t pair_count = len / pair_len;

    for (int i = 0; i < pair_count; i++) {
        STREAM_TO_U16(val.start_handle, pdu);
        STREAM_TO_U16(val.end_handle, pdu);

        if (val.start_handle == GATT_ATTR_HANDLE_NONE || val.start_handle > val.end_handle) {
            goto done;
        }

        BLE_HOST_GATT_CLIENT_INFO("Group:%d, Attribute Handle:0x%04x, End Group Handle:0x%04x",
            i, val.start_handle, val.end_handle);
        STREAM_TO_STR(val.service_uuid.uuid, pdu, val.service_uuid.uuidLength);
        BLE_HOST_GATT_CLIENT_INFO("UUID:%s", ble_att_uuid_format(&val.service_uuid));

        if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
            return true;
        }
    }

    return ble_host_gattc_discover_all_services_continue(conn_handle, cid, val.end_handle, user_data);
done:
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);
    return true;
}

static void ble_host_gattc_read_by_group_type_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    BLE_HOST_GATT_CLIENT_WARN("read by group type timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);
    const struct gattc_read_by_group_type_param *param = user_data;

    struct gattc_disc_services val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_read_by_group_type_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    BLE_HOST_GATT_CLIENT_DEBUG("read by group type error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);
    const struct gattc_read_by_group_type_param *param = user_data;

    struct gattc_disc_services val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_read_by_group_type_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_read_by_group_type_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send read by group type request, acl:0x%03x, cid:0x%04x, hdl[0x%04x, 0x%04x]",
        conn_handle, cid, param->start_handle, param->end_handle);
    return ble_host_send_att_read_by_group_type_req(conn_handle, cid, param->start_handle, param->end_handle,
        &declarationsPrimaryServiceAttUuid);
}

int ble_host_gattc_discover_all_primary_services_general(uint16_t conn_handle, uint16_t cid, const struct gattc_disc_all_services *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle > param->end_handle || param->start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_disc_all_services *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_disc_all_services));

    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    memcpy(p_param, param, sizeof(struct gattc_disc_all_services));
    struct gattc_req_message read_by_group_type_req = {
        .request_opcode = ATT_OPCODE_READ_BY_GROUP_TYPE_REQ,
        .expect_opcode = ATT_OPCODE_READ_BY_GROUP_TYPE_RSP,
        .rsp_callback = ble_host_gattc_recv_read_by_group_type_rsp,
        .error_callback = ble_host_gattc_recv_read_by_group_type_error,
        .timeout_callback = ble_host_gattc_read_by_group_type_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &read_by_group_type_req, ble_host_gattc_deal_read_by_group_type_req);
}

int ble_host_gattc_discover_all_primary_services(uint16_t conn_handle, uint16_t cid,
    gattc_disc_service_callback callback, void *user_data)
{
    struct gattc_disc_all_services param = {
        .start_handle = GATT_ATTR_HANDLE_START,
        .end_handle = GATT_ATTR_HANDLE_END,
        .user_data = user_data,
        .callback = callback,
    };

    return ble_host_gattc_discover_all_primary_services_general(conn_handle, cid, &param);
}

static bool ble_host_gattc_discover_all_services_by_uuid_continue(uint16_t conn_handle, uint16_t cid, uint16_t start_handle,
    struct gattc_disc_service_by_uuid *disc_services)
{
    if (start_handle == GATT_ATTR_HANDLE_END) {
        BLE_HOST_GATT_CLIENT_DEBUG("discovery complete");
    } else {
        disc_services->start_handle = start_handle + 1;
        if (disc_services->start_handle <= disc_services->end_handle) {
            return false;
        }
    }

    struct gattc_disc_services val = {
        .user_data = disc_services->user_data,
    };
    disc_services->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static bool ble_host_gattc_recv_find_by_type_value_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    const struct gattc_find_by_type_value_param *param = user_data;

    int32_t pair_count = len / sizeof(struct attr_handle_group);
    BLE_HOST_GATT_CLIENT_DEBUG("receive find by type value response, acl:0x%03x, cid:0x%04x, pair count: %d",
        conn_handle, cid, pair_count);

    struct gattc_disc_services val = {
        .user_data = param->user_data,
    };

    for (int i = 0; i < pair_count; i++) {
        STREAM_TO_U16(val.start_handle, pdu);
        STREAM_TO_U16(val.end_handle, pdu);

        BLE_HOST_GATT_CLIENT_DEBUG(" pair index: %d, Attribute Handle: 0x%04x, End Group Handle: 0x%04x",
            i, val.start_handle, val.end_handle);
        if (val.start_handle == GATT_ATTR_HANDLE_NONE || val.start_handle > val.end_handle) {
            goto done;
        }

        if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
            return true;
        }
    }

    return ble_host_gattc_discover_all_services_by_uuid_continue(conn_handle, cid, val.end_handle, user_data);
done:
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);
    return true;
}

static void ble_host_gattc_find_by_type_value_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_find_by_type_value_param *param = user_data;
    BLE_HOST_GATT_CLIENT_WARN("find by type value timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    struct gattc_disc_services val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_find_by_type_value_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    const struct gattc_find_by_type_value_param *param = user_data;
    BLE_HOST_GATT_CLIENT_DEBUG("find by type value error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);

    struct gattc_disc_services val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_find_by_type_value_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_find_by_type_value_param *param = user_data;
    BLE_HOST_GATT_CLIENT_DEBUG("send find by type value request, acl:0x%03x, cid:0x%04x, hdl[0x%04x, 0x%04x], uuid:%s",
        conn_handle, cid, param->start_handle, param->end_handle, ble_att_uuid_format(param->service_uuid));
    return ble_host_send_att_find_by_type_value_req(conn_handle, cid, param->start_handle, param->end_handle,
        DECLARATIONS_UUID_PRIMARY_SERVICE, param->service_uuid->uuid, param->service_uuid->uuidLength);
}

int ble_host_gattc_discover_primary_service_by_uuid_general(uint16_t conn_handle, uint16_t cid,
    const struct gattc_disc_service_by_uuid *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL || CHECK_ATT_UUID(param->service_uuid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle > param->end_handle || param->start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_disc_service_by_uuid *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_disc_service_by_uuid));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    memcpy(p_param, param, sizeof(struct gattc_disc_service_by_uuid));
    struct gattc_req_message find_by_type_value_req = {
        .request_opcode = ATT_OPCODE_FIND_BY_TYPE_VALUE_REQ,
        .expect_opcode = ATT_OPCODE_FIND_BY_TYPE_VALUE_RSP,
        .rsp_callback = ble_host_gattc_recv_find_by_type_value_rsp,
        .error_callback = ble_host_gattc_recv_find_by_type_value_error,
        .timeout_callback = ble_host_gattc_find_by_type_value_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &find_by_type_value_req, ble_host_gattc_deal_find_by_type_value_req);
}

int ble_host_gattc_discover_primary_service_by_uuid(uint16_t conn_handle, uint16_t cid,
    const struct att_uuid *service_uuid, gattc_disc_service_callback callback, void *user_data)
{
    struct gattc_disc_service_by_uuid param = {
        .service_uuid = service_uuid,
        .start_handle = GATT_ATTR_HANDLE_START,
        .end_handle = GATT_ATTR_HANDLE_END,
        .user_data = user_data,
        .callback = callback,
    };

    return ble_host_gattc_discover_primary_service_by_uuid_general(conn_handle, cid, &param);
}

static int ble_host_gattc_deal_find_included_services_req(uint16_t conn_handle, uint16_t cid, void *user_data);
static void ble_host_gattc_find_included_services_timeout(uint16_t conn_handle, uint16_t cid, void *user_data);
static void ble_host_gattc_recv_find_included_services_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data);
static bool ble_host_gattc_recv_find_included_services_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data);
static bool ble_host_gattc_recv_find_included_services_continue(uint16_t conn_handle, uint16_t cid,
    uint16_t start_handle, struct gattc_disc_incl_service_info *find_included);

static bool ble_host_gattc_recv_find_included_128bit_services_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    struct gattc_disc_incl_service_info *param = user_data;

    struct gattc_disc_128bit_uuid_incl_service *p_incl_128bit = &param->incl_128bit_uuid[param->incl_128bit_uuid_index];
    param->incl_128bit_uuid_index++;

    struct gattc_find_incl_service val = {
        .user_data = param->user_data,
        .handle = p_incl_128bit->handle,
        .start_handle = p_incl_128bit->start_handle,
        .end_handle = p_incl_128bit->end_handle,
        .incl_service_uuid.uuidLength = ATT_128_UUID_LEN,
    };

    if (len != ATT_128_UUID_LEN) {
        BLE_HOST_GATT_CLIENT_WARN("invalid length: %d", len);
        goto done;
    }

    memcpy(val.incl_service_uuid.uuid, pdu, ATT_128_UUID_LEN);

    if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
        return true;
    }

    if (param->incl_128bit_uuid_index < param->incl_128bit_uuid_count) {
        // if there are more 128-bit UUIDs to find included services, continue to send read request
        return false;
    }

    // if there are no more 128-bit UUIDs to find included services, continue to find included services
    struct gattc_req_message find_included_services_req = {
        .request_opcode = ATT_OPCODE_READ_BY_TYPE_REQ,
        .expect_opcode = ATT_OPCODE_READ_BY_TYPE_RSP,
        .rsp_callback = ble_host_gattc_recv_find_included_services_rsp,
        .error_callback = ble_host_gattc_recv_find_included_services_error,
        .timeout_callback = ble_host_gattc_find_included_services_timeout,
        .user_data = user_data,
    };

    ble_host_gattc_update_laster_message_info(conn_handle, cid, &find_included_services_req,
        ble_host_gattc_deal_find_included_services_req);

    return ble_host_gattc_recv_find_included_services_continue(conn_handle, cid, param->incl_128bit_uuid_handle, param);
done:
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);
    return true;
}

static int ble_host_gattc_deal_find_included_128bit_services_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    struct gattc_disc_incl_service_info *param = user_data;

    struct gattc_disc_128bit_uuid_incl_service *p_incl_128bit = &param->incl_128bit_uuid[param->incl_128bit_uuid_index];

    BLE_HOST_GATT_CLIENT_DEBUG("send read request, acl:0x%03x, cid:0x%04x, hdl:0x%04x",
        conn_handle, cid, p_incl_128bit->start_handle);

    return ble_host_send_att_read_req(conn_handle, cid, p_incl_128bit->start_handle);
}

static void ble_host_gattc_find_included_services_update_128bit_uuid(uint16_t conn_handle, uint16_t cid,
    struct gattc_disc_incl_service_info *find_included)
{
    struct gattc_req_message find_included_128bit_services_req = {
        .request_opcode = ATT_OPCODE_READ_REQ,
        .expect_opcode = ATT_OPCODE_READ_RSP,
        .rsp_callback = ble_host_gattc_recv_find_included_128bit_services_rsp,
        .error_callback = ble_host_gattc_recv_find_included_128bit_services_error,
        .timeout_callback = ble_host_gattc_find_included_128bit_services_timeout,
        .user_data = find_included,
    };

    ble_host_gattc_update_laster_message_info(conn_handle, cid, &find_included_128bit_services_req,
        ble_host_gattc_deal_find_included_128bit_services_req);
}

static bool ble_host_gattc_recv_find_included_services_continue(uint16_t conn_handle, uint16_t cid,
    uint16_t start_handle, struct gattc_disc_incl_service_info *find_included)
{
    if (start_handle == GATT_ATTR_HANDLE_END) {
        BLE_HOST_GATT_CLIENT_DEBUG("discovery complete");
    } else {
        find_included->start_handle = start_handle + 1;
        if (find_included->start_handle <= find_included->end_handle) {
            return false;
        }
    }

    struct gattc_find_incl_service val = {
        .user_data = find_included->user_data,
    };
    find_included->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static bool ble_host_gattc_recv_find_included_services_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    struct gattc_disc_incl_service_info *param = user_data;

    struct gattc_find_incl_service val = {
        .user_data = param->user_data,
    };

    uint8_t pair_len;
    STREAM_TO_U8(pair_len, pdu);
    len--;

    int32_t pair_count = len / pair_len;

    BLE_HOST_GATT_CLIENT_DEBUG("receive read by type response, acl:0x%03x, cid:0x%04x, pair count: %d",
        conn_handle, cid, pair_count);

    if (pair_count <= 0) {
        goto done;
    }

    if (pair_len == (sizeof(struct attr_type_data) + sizeof(struct included_16bit_uuid_attr_value))) {
        val.incl_service_uuid.uuidLength = ATT_16_UUID_LEN;
        // add included 16-bit UUID 
        for (int i = 0; i < pair_count; i++) {
            STREAM_TO_U16(val.handle, pdu);
            STREAM_TO_U16(val.start_handle, pdu);
            STREAM_TO_U16(val.end_handle, pdu);
            // 16-bit UUID fixed.
            STREAM_TO_U16(val.incl_service_uuid.uuid16, pdu);

            if (val.start_handle == GATT_ATTR_HANDLE_NONE || val.start_handle > val.end_handle) {
                BLE_HOST_GATT_CLIENT_WARN("invalid included 16-bit UUID range: 0x%04x - 0x%04x",
                    val.start_handle, val.end_handle);
                goto done;
            }

            if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
                return true;
            }
        }

        return ble_host_gattc_recv_find_included_services_continue(conn_handle, cid, val.handle, user_data);

    } else if (pair_len == (sizeof(struct attr_type_data) + sizeof(struct included_128bit_uuid_attr_value))) {
        param->incl_128bit_uuid_count = min(pair_count, GATT_FIND_INCLUDED_SERVICE_128BIT_UUID_MAX_COUNT);
        param->incl_128bit_uuid_index = 0;

        struct gattc_disc_128bit_uuid_incl_service *p_incl_128bit = &param->incl_128bit_uuid[0];
        for (int i = 0; i < param->incl_128bit_uuid_count; i++) {
            STREAM_TO_U16(p_incl_128bit->handle, pdu);
            STREAM_TO_U16(p_incl_128bit->start_handle, pdu);
            STREAM_TO_U16(p_incl_128bit->end_handle, pdu);

            if (p_incl_128bit->start_handle == GATT_ATTR_HANDLE_NONE ||
                p_incl_128bit->start_handle > p_incl_128bit->end_handle) {
                BLE_HOST_GATT_CLIENT_WARN("invalid included 128-bit UUID range: 0x%04x - 0x%04x",
                    p_incl_128bit->start_handle, p_incl_128bit->end_handle);
                goto done;
            }

            param->incl_128bit_uuid_handle = p_incl_128bit->handle;
            p_incl_128bit++;
        }
        // add update read request to find included services of the 128-bit UUIDs.
        ble_host_gattc_find_included_services_update_128bit_uuid(conn_handle, cid, param);

        return false;
    }

    BLE_HOST_GATT_CLIENT_WARN("invalid pair length: %d", pair_len);
done:
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);
    return true;
}

static void ble_host_gattc_find_included_services_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_disc_incl_service_info *param = user_data;
    BLE_HOST_GATT_CLIENT_WARN("read by type timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    struct gattc_find_incl_service val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_find_included_services_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    const struct gattc_disc_incl_service_info *param = user_data;
    BLE_HOST_GATT_CLIENT_DEBUG("read by type error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);


    struct gattc_find_incl_service val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_find_included_services_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_disc_incl_service_info *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send read by type request, acl:0x%03x, cid:0x%04x, hdl[0x%04x, 0x%04x], uuid:%s",
        conn_handle, cid, param->start_handle, param->end_handle, ble_att_uuid_format(&declarationsIncludeAttUuid));

    return ble_host_send_att_read_by_type_req(conn_handle, cid, param->start_handle, param->end_handle,
        &declarationsIncludeAttUuid);
}

int ble_host_gattc_find_included_services(uint16_t conn_handle, uint16_t cid,
    const struct gattc_find_incl_service_param *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle > param->end_handle || param->start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_disc_incl_service_info *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_disc_incl_service_info));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    memset(p_param->incl_128bit_uuid, 0, sizeof(p_param->incl_128bit_uuid));
    p_param->start_handle = param->start_handle;
    p_param->end_handle = param->end_handle;
    p_param->user_data = param->user_data;
    p_param->callback = param->callback;

    struct gattc_req_message find_included_services_req = {
        .request_opcode = ATT_OPCODE_READ_BY_TYPE_REQ,
        .expect_opcode = ATT_OPCODE_READ_BY_TYPE_RSP,
        .rsp_callback = ble_host_gattc_recv_find_included_services_rsp,
        .error_callback = ble_host_gattc_recv_find_included_services_error,
        .timeout_callback = ble_host_gattc_find_included_services_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &find_included_services_req,
        ble_host_gattc_deal_find_included_services_req);
}

static bool ble_host_gattc_discover_all_characteristics_of_service_continue(uint16_t conn_handle, uint16_t cid,
    uint16_t start_handle, struct gattc_discover_characteristic_param *disc_characteristic)
{
    if (start_handle == GATT_ATTR_HANDLE_END) {
        BLE_HOST_GATT_CLIENT_DEBUG("discovery complete");
    } else {
        disc_characteristic->start_handle = start_handle + 1;
        if (disc_characteristic->start_handle <= disc_characteristic->end_handle) {
            return false;
        }
    }

    struct gattc_disc_characteristic val = {
        .user_data = disc_characteristic->user_data,
    };
    disc_characteristic->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static bool ble_host_gattc_recv_read_by_type_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    const struct gattc_discover_characteristic_param *param = user_data;

    struct gattc_disc_characteristic val = {
        .user_data = param->user_data,
    };

    uint8_t pair_len;
    STREAM_TO_U8(pair_len, pdu);
    len--;

    if (pair_len == (sizeof(struct attr_type_data) + sizeof(struct characteristic_16bit_uuid_attr_value))) {
        val.characteristic_uuid.uuidLength = ATT_16_UUID_LEN;
    } else if (pair_len == (sizeof(struct attr_type_data) + sizeof(struct characteristic_128bit_uuid_attr_value))) {
        val.characteristic_uuid.uuidLength = ATT_128_UUID_LEN;
    } else {
        BLE_HOST_GATT_CLIENT_WARN("invalid pair length: %d", pair_len);
        goto done;
    }

    int32_t pair_count = len / pair_len;

    BLE_HOST_GATT_CLIENT_DEBUG("receive read by type response, acl:0x%03x, cid:0x%04x, pair count: %d",
        conn_handle, cid, pair_count);

    for (int i = 0; i < pair_count; i++) {
        STREAM_TO_U16(val.handle, pdu);
        STREAM_TO_U8(val.properties.all, pdu);
        STREAM_TO_U16(val.value_handle, pdu);
        STREAM_TO_STR(val.characteristic_uuid.uuid, pdu, val.characteristic_uuid.uuidLength);

        if (val.handle == GATT_ATTR_HANDLE_NONE || val.handle > val.value_handle) {
            BLE_HOST_GATT_CLIENT_WARN("invalid handle: 0x%04x, value handle: 0x%04x", val.handle, val.value_handle);
            goto done;
        }

        if (param->characteristic_uuid == NULL ||
            ble_att_uuid_cmp(&val.characteristic_uuid, param->characteristic_uuid) == 0) {
            if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
                return true;
            }
        }
    }

    return ble_host_gattc_discover_all_characteristics_of_service_continue(conn_handle, cid, val.value_handle, user_data);
done:
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);
    return true;
}

static void ble_host_gattc_read_by_type_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_discover_characteristic_param *param = user_data;
    BLE_HOST_GATT_CLIENT_WARN("read by type timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    struct gattc_disc_characteristic val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_read_by_type_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    const struct gattc_discover_characteristic_param *param = user_data;
    BLE_HOST_GATT_CLIENT_DEBUG("read by type error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);


    struct gattc_disc_characteristic val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_read_by_type_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_discover_characteristic_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send read by type request, acl:0x%03x, cid:0x%04x, hdl[0x%04x, 0x%04x], uuid:%s",
        conn_handle, cid, param->start_handle, param->end_handle, ble_att_uuid_format(&declarationsCharacteristicAttUuid));

    return ble_host_send_att_read_by_type_req(conn_handle, cid, param->start_handle, param->end_handle,
        &declarationsCharacteristicAttUuid);
}

int ble_host_gattc_discover_all_characteristics_of_service(uint16_t conn_handle, uint16_t cid,
    const struct gattc_disc_all_characteristics *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle > param->end_handle || param->start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_discover_characteristic_param *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_discover_characteristic_param));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    p_param->characteristic_uuid = NULL;
    p_param->start_handle = param->start_handle;
    p_param->end_handle = param->end_handle;
    p_param->user_data = param->user_data;
    p_param->callback = param->callback;

    struct gattc_req_message read_by_type_req = {
        .request_opcode = ATT_OPCODE_READ_BY_TYPE_REQ,
        .expect_opcode = ATT_OPCODE_READ_BY_TYPE_RSP,
        .rsp_callback = ble_host_gattc_recv_read_by_type_rsp,
        .error_callback = ble_host_gattc_recv_read_by_type_error,
        .timeout_callback = ble_host_gattc_read_by_type_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &read_by_type_req, ble_host_gattc_deal_read_by_type_req);
}

int ble_host_gattc_discover_all_characteristics_of_service_by_uuid(uint16_t conn_handle, uint16_t cid,
    const struct gattc_disc_all_characteristics_by_uuid *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL || CHECK_ATT_UUID(param->characteristic_uuid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle > param->end_handle || param->start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_discover_characteristic_param *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_discover_characteristic_param));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    p_param->characteristic_uuid = param->characteristic_uuid;
    p_param->start_handle = param->start_handle;
    p_param->end_handle = param->end_handle;
    p_param->user_data = param->user_data;
    p_param->callback = param->callback;

    struct gattc_req_message read_by_type_req = {
        .request_opcode = ATT_OPCODE_READ_BY_TYPE_REQ,
        .expect_opcode = ATT_OPCODE_READ_BY_TYPE_RSP,
        .rsp_callback = ble_host_gattc_recv_read_by_type_rsp,
        .error_callback = ble_host_gattc_recv_read_by_type_error,
        .timeout_callback = ble_host_gattc_read_by_type_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &read_by_type_req, ble_host_gattc_deal_read_by_type_req);
}

static bool ble_host_gattc_discover_characteristic_desc_continue(uint16_t conn_handle, uint16_t cid,
    uint16_t start_handle, struct gattc_find_info_param *disc_characteristic_desc)
{
    if (start_handle == GATT_ATTR_HANDLE_END) {
        BLE_HOST_GATT_CLIENT_DEBUG("discovery complete");
    } else {
        disc_characteristic_desc->start_handle = start_handle + 1;
        if (disc_characteristic_desc->start_handle <= disc_characteristic_desc->end_handle) {
            return false;
        }
    }

    struct gattc_disc_characteristic_desc val = {
        .user_data = disc_characteristic_desc->user_data,
    };
    disc_characteristic_desc->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static bool ble_host_gattc_recv_find_info_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    const struct gattc_find_info_param *param = user_data;

    uint8_t format;
    STREAM_TO_U8(format, pdu);
    len--;

    struct gattc_disc_characteristic_desc val = {
        .user_data = param->user_data,
    };

    int32_t pair_count;

    if (format == ATT_INFO_FORMAT_16BIT_UUID) {
        pair_count = len / sizeof(struct att_info_16bit_uuid);
        val.descriptor_uuid.uuidLength = ATT_16_UUID_LEN;
    } else if (format == ATT_INFO_FORMAT_128BIT_UUID) {
        pair_count = len / sizeof(struct att_info_128bit_uuid);
        val.descriptor_uuid.uuidLength = ATT_128_UUID_LEN;
    } else {
        BLE_HOST_GATT_CLIENT_WARN("invalid format: %d", format);
        goto done;
    }

    for (int i = 0; i < pair_count; i++) {
        STREAM_TO_U16(val.handle, pdu);
        STREAM_TO_STR(val.descriptor_uuid.uuid, pdu, val.descriptor_uuid.uuidLength);

        BLE_HOST_GATT_CLIENT_DEBUG(" triple:%d, Attribute Handle: 0x%04x, Descriptor UUID: %s",
            i, val.handle, ble_att_uuid_format(&val.descriptor_uuid));

        if (val.handle == GATT_ATTR_HANDLE_NONE) {
            BLE_HOST_GATT_CLIENT_WARN("invalid handle: 0x%04x", val.handle);
            goto done;
        }

        if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
            return true;
        }
    }

    return ble_host_gattc_discover_characteristic_desc_continue(conn_handle, cid, val.handle, user_data);
done:
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);
    return true;
}

static void ble_host_gattc_find_info_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_find_info_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("find information timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    struct gattc_disc_characteristic_desc val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_find_info_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    (void) handle;
    const struct gattc_find_info_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("find information error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);

    struct gattc_disc_characteristic_desc val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_find_info_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_find_info_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send find info request, acl:0x%03x, cid:0x%04x, hdl[0x%04x, 0x%04x]",
        conn_handle, cid, param->start_handle, param->end_handle);
    return ble_host_send_att_find_info_req(conn_handle, cid, param->start_handle, param->end_handle);
}

int ble_host_gattc_discover_characteristic_desc(uint16_t conn_handle, uint16_t cid,
    const struct gattc_disc_characteristic_desc_param *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle > param->end_handle || param->start_handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_disc_characteristic_desc_param *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_disc_characteristic_desc_param));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    memcpy(p_param, param, sizeof(struct gattc_disc_characteristic_desc_param));

    struct gattc_req_message find_info_req = {
        .request_opcode = ATT_OPCODE_FIND_INFORMATION_REQ,
        .expect_opcode = ATT_OPCODE_FIND_INFORMATION_RSP,
        .rsp_callback = ble_host_gattc_recv_find_info_rsp,
        .error_callback = ble_host_gattc_recv_find_info_error,
        .timeout_callback = ble_host_gattc_find_info_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &find_info_req, ble_host_gattc_deal_find_info_req);
}

static void ble_host_gattc_recv_read_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data);
static void ble_host_gattc_read_timeout(uint16_t conn_handle, uint16_t cid, void *user_data);
static int ble_host_gattc_deal_read_blob_req(uint16_t conn_handle, uint16_t cid, void *user_data);
static bool ble_host_gattc_recv_read_blob_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data);

static bool ble_host_gattc_read_characteristic_value_continue(uint16_t conn_handle, uint16_t cid,
    struct gattc_read_common_param *user_data)
{
    BLE_HOST_GATT_CLIENT_DEBUG("update read blob request, acl:0x%03x, cid:0x%04x, handle:0x%04x, offset:0x%04x",
        conn_handle, cid, user_data->handle, user_data->offset);

    struct gattc_req_message read_blob_req = {
        .request_opcode = ATT_OPCODE_READ_BLOB_REQ,
        .expect_opcode = ATT_OPCODE_READ_BLOB_RSP,
        .rsp_callback = ble_host_gattc_recv_read_blob_rsp,
        .error_callback = ble_host_gattc_recv_read_blob_error,
        .timeout_callback = ble_host_gattc_read_blob_timeout,
        .user_data = user_data,
    };
    ble_host_gattc_update_laster_message_info(conn_handle, cid, &read_blob_req, ble_host_gattc_deal_read_blob_req);
    return false;
}

static bool ble_host_gattc_recv_read_rsp_common(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    struct gattc_read_common_param *param = user_data;

    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    struct gattc_read_characteristic_value val = {
        .state = GATTC_READ_VALUE_COMPLETE,
        .handle = param->handle,
        .offset = param->offset,
        .length = len,
        .buffer = pdu,
        .user_data = param->user_data,
    };

    BLE_HOST_GATT_CLIENT_DEBUG("read response, acl:0x%03x, cid:0x%04x, handle:0x%04x, offset:0x%04x, length:0x%04x",
        conn_handle, cid, param->handle, param->offset, len);
    if (len < mtu - 1) {    // 1 is read response opcode size.
        // Characteristic value is complete, not need to read long, it will be called later.
    } else if (len == mtu - 1) {
        if (param->read_long) {
            // continue read long
            val.state = GATTC_READ_VALUE_CONTINUE;
            if (param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val) == false) {
                return true;
            }

            param->offset += len;
            return ble_host_gattc_read_characteristic_value_continue(conn_handle, cid, user_data);
        }
    } else {
        BLE_HOST_GATT_CLIENT_WARN("invalid read response length, mtu:0x%04x, length:0x%04x", mtu, len);
        param->callback(conn_handle, cid, GATT_REQUEST_ERR_OTHER_REASON, &val);

        return true;
    }
    param->callback(conn_handle, cid, GATT_REQUEST_SUB_PROCEDURE_COMPLETE, &val);
    param->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static bool ble_host_gattc_recv_read_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    return ble_host_gattc_recv_read_rsp_common(conn_handle, cid, pdu, len, user_data);
}

static void ble_host_gattc_read_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_read_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("read timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    struct gattc_read_characteristic_value val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_read_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    const struct gattc_read_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("read error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);

    struct gattc_read_characteristic_value val = {
        .user_data = param->user_data,
    };

    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_read_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_read_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send read request, acl:0x%03x, cid:0x%04x, handle:0x%04x",
        conn_handle, cid, param->handle);

    return ble_host_send_att_read_req(conn_handle, cid, param->handle);
}

static bool ble_host_gattc_recv_read_blob_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    return ble_host_gattc_recv_read_rsp_common(conn_handle, cid, pdu, len, user_data);
}

static int ble_host_gattc_deal_read_blob_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_read_blob_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send read blob request, acl:0x%03x, cid:0x%04x, handle:0x%04x, offset:0x%04x",
        conn_handle, cid, param->handle, param->offset);
    return ble_host_send_att_read_blob_req(conn_handle, cid, param->handle, param->offset);
}

static int ble_host_gattc_read_characteristic_value_common(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param, bool long_read)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_read_param *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_read_param));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    p_param->read_long = long_read;
    p_param->handle = param->handle;
    p_param->offset = 0;
    p_param->user_data = param->user_data;
    p_param->callback = param->callback;

    struct gattc_req_message read_req = {
        .request_opcode = ATT_OPCODE_READ_REQ,
        .expect_opcode = ATT_OPCODE_READ_RSP,
        .rsp_callback = ble_host_gattc_recv_read_rsp,
        .error_callback = ble_host_gattc_recv_read_error,
        .timeout_callback = ble_host_gattc_read_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &read_req, ble_host_gattc_deal_read_req);
}

int ble_host_gattc_read_short_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param)
{
    return ble_host_gattc_read_characteristic_value_common(conn_handle, cid, param, false);
}

int ble_host_gattc_read_long_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param)
{
    return ble_host_gattc_read_characteristic_value_common(conn_handle, cid, param, true);
}

int ble_host_gattc_read_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param)
{
    return ble_host_gattc_read_characteristic_value_common(conn_handle, cid, param, true);
}

int ble_host_gattc_read_blob_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_blob_characteristic_value_param *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_read_param *p_param = GATT_CLIENT_REQ_MALLOC(sizeof(struct gattc_read_param));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    p_param->read_long = true;
    p_param->handle = param->handle;
    p_param->offset = param->offset;
    p_param->user_data = param->user_data;
    p_param->callback = param->callback;

    struct gattc_req_message read_req = {
        .request_opcode = ATT_OPCODE_READ_BLOB_REQ,
        .expect_opcode = ATT_OPCODE_READ_BLOB_RSP,
        .rsp_callback = ble_host_gattc_recv_read_blob_rsp,
        .error_callback = ble_host_gattc_recv_read_blob_error,
        .timeout_callback = ble_host_gattc_read_blob_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &read_req, ble_host_gattc_deal_read_blob_req);
}

static bool ble_host_gattc_read_characteristic_value_write_callback(uint16_t conn_handle, uint16_t cid,
    uint32_t err, const struct gattc_read_characteristic_value *param)
{
    struct gattc_characteristic_value_write_info *p_user_data = param->user_data;

    if (err == GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        if (p_user_data->write_buffer != NULL) {
            int16_t left_space = p_user_data->max_buffer_len - param->offset;
            if (left_space > 0) {
                uint16_t write_len = min(left_space, param->length);
                memcpy(p_user_data->write_buffer + param->offset, param->buffer, write_len);
            }
        }

        if (p_user_data->write_buffer_len != NULL) {
            *p_user_data->write_buffer_len = param->length + param->offset;
        }
    } else {
        GATT_CLIENT_REQ_FREE(p_user_data);
    }

    if (p_user_data->callback != NULL) {
        struct gattc_read_characteristic_value val = *param;
        val.user_data = p_user_data->user_data;
        if (p_user_data->callback(conn_handle, cid, err, &val) == false) {
            GATT_CLIENT_REQ_FREE(p_user_data);
            return false;
        }
    }

    return true;
}

int ble_host_gattc_read_characteristic_value_write(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    const struct gattc_characteristic_value_write_info *param)
{
    if (param->write_buffer == NULL || param->max_buffer_len == 0) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_characteristic_value_write_info *p_user_data = GATT_CLIENT_REQ_MALLOC(
        sizeof(struct gattc_characteristic_value_write_info));
    if (p_user_data == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    memcpy(p_user_data, param, sizeof(struct gattc_characteristic_value_write_info));

    struct gattc_read_characteristic_value_param read_value_param = {
        .handle = handle,
        .user_data = p_user_data,
        .callback = ble_host_gattc_read_characteristic_value_write_callback,
    };

    int ret = ble_host_gattc_read_characteristic_value(conn_handle, cid, &read_value_param);

    if (ret != BLE_HOST_ERR_SUCC) {
        GATT_CLIENT_REQ_FREE(p_user_data);
    }

    return ret;
}

static bool ble_host_gattc_recv_read_using_characteristic_uuid_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    const struct gattc_read_using_characteristic_uuid_param *param = user_data;

    uint8_t pair_len;
    STREAM_TO_U8(pair_len, pdu);
    len--;

    uint16_t handle;
    STREAM_TO_U16(handle, pdu);

    struct gattc_read_characteristic_value val = {
        .state = GATTC_READ_VALUE_COMPLETE,
        .length = pair_len - sizeof(uint16_t),  // Attribute Handle.
        .offset = 0,
        .handle = handle,
        .buffer = pdu,
        .user_data = param->user_data,
    };

    int32_t pair_count = len / pair_len;

    BLE_HOST_GATT_CLIENT_DEBUG("receive read by type response, acl:0x%03x, cid:0x%04x, pair count: %d",
        conn_handle, cid, pair_count);

    param->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, &val);
    return true;
}

static void ble_host_gattc_read_using_characteristic_uuid_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_read_using_characteristic_uuid_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("read timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    struct gattc_read_characteristic_value val = {
        .user_data = param->user_data,
    };
    param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, &val);
}

static void ble_host_gattc_recv_read_using_characteristic_uuid_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    const struct gattc_read_using_characteristic_uuid_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("read error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);

    struct gattc_read_characteristic_value val = {
        .user_data = param->user_data,
    };

    param->callback(conn_handle, cid, error, &val);
}

static int ble_host_gattc_deal_read_using_characteristic_uuid_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_read_using_characteristic_uuid_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send read by type request, acl:0x%03x, cid:0x%04x, hdl[0x%04x, 0x%04x], uuid:%s",
        conn_handle, cid, param->start_handle, param->end_handle, ble_att_uuid_format(param->characteristic_uuid));

    return ble_host_send_att_read_by_type_req(conn_handle, cid, param->start_handle, param->end_handle,
        param->characteristic_uuid);
}

int ble_host_gattc_read_using_characteristic_uuid_general(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_using_characteristic_uuid_param *param)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || param->callback == NULL || CHECK_ATT_UUID(param->characteristic_uuid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->start_handle == GATT_ATTR_HANDLE_NONE || param->start_handle > param->end_handle) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    struct gattc_read_using_characteristic_uuid_param *p_param = GATT_CLIENT_REQ_MALLOC(
        sizeof(struct gattc_read_using_characteristic_uuid_param));
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    memcpy(p_param, param, sizeof(struct gattc_read_using_characteristic_uuid_param));
    struct gattc_req_message read_req = {
        .request_opcode = ATT_OPCODE_READ_BY_TYPE_REQ,
        .expect_opcode = ATT_OPCODE_READ_BY_TYPE_RSP,
        .rsp_callback = ble_host_gattc_recv_read_using_characteristic_uuid_rsp,
        .error_callback = ble_host_gattc_recv_read_using_characteristic_uuid_error,
        .timeout_callback = ble_host_gattc_read_using_characteristic_uuid_timeout,
        .user_data = p_param,
    };

    return ble_host_gattc_send_request(conn_handle, cid, &read_req, ble_host_gattc_deal_read_using_characteristic_uuid_req);
}

int ble_host_gattc_read_using_characteristic_uuid(uint16_t conn_handle, uint16_t cid,
    const struct att_uuid *characteristic_uuid, gattc_read_characteristic_value_callback callback, void *user_data)
{
    struct gattc_read_using_characteristic_uuid_param param = {
        .start_handle = GATT_ATTR_HANDLE_START,
        .end_handle = GATT_ATTR_HANDLE_END,
        .characteristic_uuid = characteristic_uuid,
        .callback = callback,
        .user_data = user_data,
    };

    return ble_host_gattc_read_using_characteristic_uuid_general(conn_handle, cid, &param);
}

int ble_host_gattc_write_characteristic_value_without_response(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, const uint8_t *buffer, uint16_t length)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    if (buffer == NULL && length != 0) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    BLE_HOST_GATT_CLIENT_DEBUG("send write command, acl:0x%03x, cid:0x%04x, hdl:0x%04x, len:%d",
        conn_handle, cid, handle, length);

    return ble_host_send_att_write_cmd(conn_handle, cid, handle, buffer, length);
}

int ble_host_gattc_signed_write_characteristic_value_without_response(uint16_t conn_handle,
    uint16_t handle, const uint8_t *buffer, uint16_t length, const uint8_t signature[12])
{
    if (!ble_host_gattc_is_connected(conn_handle, LE_L2CAP_CID_ATT)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    if ((buffer == NULL && length != 0) || signature == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    BLE_HOST_GATT_CLIENT_DEBUG("send signed write command, acl:0x%03x, hdl:0x%04x, len:%d",
        conn_handle, handle, length);

    return ble_host_att_send_signed_write_cmd(conn_handle, handle, buffer, length, signature);
}

static bool ble_host_gattc_recv_write_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    (void) pdu;
    (void) len;
    const struct gattc_write_param *param = user_data;

    if (param->callback != NULL) {
        param->callback(conn_handle, cid, GATT_REQUEST_PROCEDURE_COMPLETE, param->user_data);
    }

    return true;
}

static void ble_host_gattc_write_timeout(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_write_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("write timeout, acl:0x%03x, cid:0x%04x", conn_handle, cid);

    if (param->callback != NULL) {
        param->callback(conn_handle, cid, GATT_REQUEST_ERR_TIMEOUT, param->user_data);
    }
}

static void ble_host_gattc_recv_write_error(uint16_t conn_handle, uint16_t cid,
    uint16_t handle, uint32_t error, void *user_data)
{
    const struct gattc_write_param *param = user_data;

    BLE_HOST_GATT_CLIENT_WARN("write error, acl:0x%03x, cid:0x%04x, error:0x%02x, handle:0x%04x",
        conn_handle, cid, error, handle);

    if (param->callback != NULL) {
        param->callback(conn_handle, cid, error, param->user_data);
    }
}

static int ble_host_gattc_deal_write_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    const struct gattc_write_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send write request, acl:0x%03x, cid:0x%04x, hdl:0x%04x, len:%d",
        conn_handle, cid, param->handle, param->total_length);

    const uint8_t *buffer = (param->global_buffer != NULL) ? param->global_buffer : param->buffer;

    return ble_host_send_att_write_req(conn_handle, cid, param->handle, buffer, param->total_length);
}

static int ble_host_gattc_write_value_common(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param, bool global_buffer,
    struct gattc_req_message *write_req, gattc_req_message_handler_t handler)
{
    if (!ble_host_gattc_is_connected(conn_handle, cid)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    if (param == NULL || (param->length != 0 && param->buffer == NULL)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    if (param->handle == GATT_ATTR_HANDLE_NONE) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_ATTR_HANDLE);
    }

    int32_t param_size = sizeof(struct gattc_write_common_param) + (global_buffer ? 0 : param->length);
    struct gattc_write_common_param *p_param = GATT_CLIENT_REQ_MALLOC(param_size);
    if (p_param == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    p_param->handle = param->handle;
    p_param->offset = 0;
    p_param->total_length = param->length;
    p_param->user_data = param->user_data;
    p_param->callback = param->callback;
    p_param->global_buffer = NULL;
    p_param->mtu = ble_host_get_att_mtu(conn_handle, cid);
    if (global_buffer) {
        p_param->global_buffer = param->buffer;
    } else {
        memcpy(p_param->buffer, param->buffer, param->length);
    }

    write_req->user_data = p_param;
    return ble_host_gattc_send_request(conn_handle, cid, write_req, handler);
}

static int ble_host_gattc_write_characteristic_value_common(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param, bool global_buffer)
{
    struct gattc_req_message write_req = {
        .request_opcode = ATT_OPCODE_WRITE_REQ,
        .expect_opcode = ATT_OPCODE_WRITE_RSP,
        .rsp_callback = ble_host_gattc_recv_write_rsp,
        .error_callback = ble_host_gattc_recv_write_error,
        .timeout_callback = ble_host_gattc_write_timeout,
    };

    return ble_host_gattc_write_value_common(conn_handle, cid, param, global_buffer,
        &write_req, ble_host_gattc_deal_write_req);
}

int ble_host_gattc_write_short_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_characteristic_value_common(conn_handle, cid, param, false);
}

int ble_host_gattc_write_short_characteristic_value_global(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_characteristic_value_common(conn_handle, cid, param, true);
}

static bool ble_host_gattc_recv_execute_write_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    (void) pdu;
    (void) len;
    const struct gattc_write_param *param = user_data;

    if (param->callback != NULL) {
        param->callback(conn_handle, cid,
            param->execute_flag == GATTC_EXECUTE_WRITE_FLAG_IMMED_WRITE ?
            GATT_REQUEST_PROCEDURE_COMPLETE : GATT_REQUEST_ERR_OTHER_REASON,
            param->user_data);
    }

    return true;
}

static int ble_host_gattc_deal_execute_write_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    struct gattc_execute_write_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send execute write request, acl:0x%03x, cid:0x%04x, flag:%s",
        conn_handle, cid, param->execute_flag == GATTC_EXECUTE_WRITE_FLAG_IMMED_WRITE ? "IMMED_WRITE" : "CANCEL");

    return ble_host_send_att_execute_write_req(conn_handle, cid, param->execute_flag);
}

static void ble_host_gattc_update_execute_write_req(uint16_t conn_handle, uint16_t cid, uint8_t flag, void *user_data)
{
    struct gattc_execute_write_param *param = user_data;

    param->execute_flag = flag;

    struct gattc_req_message execute_write_req = {
        .request_opcode = ATT_OPCODE_EXECUTE_WRITE_REQ,
        .expect_opcode = ATT_OPCODE_EXECUTE_WRITE_RSP,
        .rsp_callback = ble_host_gattc_recv_execute_write_rsp,
        .error_callback = ble_host_gattc_recv_execute_write_error,
        .timeout_callback = ble_host_gattc_execute_write_timeout,
        .user_data = user_data,
    };
    ble_host_gattc_update_laster_message_info(conn_handle, cid, &execute_write_req, ble_host_gattc_deal_execute_write_req);
}

static bool ble_host_gattc_recv_prepare_write_rsp(uint16_t conn_handle, uint16_t cid,
    const uint8_t *pdu, uint16_t len, void *user_data)
{
    struct gattc_prepare_write_param *param = user_data;

    uint16_t recv_handle, recv_offset;
    STREAM_TO_U16(recv_handle, pdu);
    STREAM_TO_U16(recv_offset, pdu);
    len -= 4;

    if (recv_handle != param->handle || recv_offset != param->offset ||
        len != param->curr_packet_length || memcmp(pdu, param->buffer + param->offset, len) != 0) {
        BLE_HOST_GATT_CLIENT_WARN("prepare write response error, acl:0x%03x, cid:0x%04x, handle:0x%04x, offset:0x%04x",
            conn_handle, cid, recv_handle, recv_offset);

        ble_host_gattc_update_execute_write_req(conn_handle, cid, GATTC_EXECUTE_WRITE_FLAG_CANCEL, user_data);
    }

    param->offset += param->curr_packet_length;

    if (param->offset == param->total_length) {
        ble_host_gattc_update_execute_write_req(conn_handle, cid, GATTC_EXECUTE_WRITE_FLAG_IMMED_WRITE, user_data);
    }
    return false;
}

static int ble_host_gattc_deal_prepare_write_req(uint16_t conn_handle, uint16_t cid, void *user_data)
{
    struct gattc_prepare_write_param *param = user_data;

    BLE_HOST_GATT_CLIENT_DEBUG("send prepare write request, acl:0x%03x, cid:0x%04x, hdl:0x%04x, offset:%d, len:%d",
        conn_handle, cid, param->handle, param->offset, param->total_length);

    if (param->total_length - param->offset + ATT_PREP_WRITE_REQ_LEN > param->mtu) {
        param->curr_packet_length = param->mtu - ATT_PREP_WRITE_REQ_LEN;
    } else {
        param->curr_packet_length = param->total_length - param->offset;
    }

    const uint8_t *buffer = (param->global_buffer != NULL) ? param->global_buffer : param->buffer;

    return ble_host_send_att_prepare_write_req(conn_handle, cid, param->handle,
        param->offset, buffer + param->offset, param->curr_packet_length);
}

static int ble_host_gattc_write_long_characteristic_value_common(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param, bool global_buffer)
{
    struct gattc_req_message prepare_write_req = {
        .request_opcode = ATT_OPCODE_PREPARE_WRITE_REQ,
        .expect_opcode = ATT_OPCODE_PREPARE_WRITE_RSP,
        .rsp_callback = ble_host_gattc_recv_prepare_write_rsp,
        .error_callback = ble_host_gattc_recv_prepare_write_error,
        .timeout_callback = ble_host_gattc_prepare_write_timeout,
    };

    return ble_host_gattc_write_value_common(conn_handle, cid, param, global_buffer,
        &prepare_write_req, ble_host_gattc_deal_prepare_write_req);
}

int ble_host_gattc_write_long_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_long_characteristic_value_common(conn_handle, cid, param, false);
}

int ble_host_gattc_write_long_characteristic_value_global(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_long_characteristic_value_common(conn_handle, cid, param, true);
}

int ble_host_gattc_write_characteristic_value(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    if (param->length <= mtu - ATT_WRITE_REQ_LEN) {
        return ble_host_gattc_write_short_characteristic_value(conn_handle, cid, param);
    } else {
        return ble_host_gattc_write_long_characteristic_value(conn_handle, cid, param);
    }
}

int ble_host_gattc_write_characteristic_value_global(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    uint16_t mtu = ble_host_get_att_mtu(conn_handle, cid);

    if (param->length <= mtu - ATT_WRITE_REQ_LEN) {
        return ble_host_gattc_write_short_characteristic_value_global(conn_handle, cid, param);
    } else {
        return ble_host_gattc_write_long_characteristic_value_global(conn_handle, cid, param);
    }
}

int ble_host_gattc_read_short_characteristic_descriptor(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param)
{
    return ble_host_gattc_read_short_characteristic_value(conn_handle, cid, param);
}

int ble_host_gattc_read_long_characteristic_descriptor(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param)
{
    return ble_host_gattc_read_long_characteristic_value(conn_handle, cid, param);
}

int ble_host_gattc_read_characteristic_descriptor(uint16_t conn_handle, uint16_t cid,
    const struct gattc_read_characteristic_value_param *param)
{
    return ble_host_gattc_read_characteristic_value(conn_handle, cid, param);
}

int ble_host_gattc_write_short_characteristic_descriptor(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_short_characteristic_value(conn_handle, cid, param);
}

int ble_host_gattc_write_long_characteristic_descriptor(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_long_characteristic_value(conn_handle, cid, param);
}

int ble_host_gattc_write_characteristic_descriptor(uint16_t conn_handle, uint16_t cid,
    const struct gattc_write_characteristic_value_param *param)
{
    return ble_host_gattc_write_characteristic_value(conn_handle, cid, param);
}

int ble_host_gattc_write_ccc_value(uint16_t conn_handle, uint16_t cid, uint16_t handle, bool notify_enable, bool indicate_enable,
    gattc_write_characteristic_value_callback callback, void *user_data)
{
    uint8_t value[2] = { 0x00, 0x00 };

    struct gattc_write_characteristic_value_param write_param = {
        .handle = handle,
        .length = sizeof(value),
        .buffer = value,
        .callback = callback,
        .user_data = user_data,
    };

    if (notify_enable) {
        value[0] |= 0x01;
    }

    if (indicate_enable) {
        value[0] |= 0x02;
    }

    return ble_host_gattc_write_short_characteristic_value(conn_handle, cid, &write_param);
}

int ble_host_gattc_write_ccc_value_enable_notify_indicate(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    gattc_write_characteristic_value_callback callback, void *user_data)
{
    return ble_host_gattc_write_ccc_value(conn_handle, cid, handle, true, true, callback, user_data);
}

int ble_host_gattc_write_ccc_value_disable_notify_indicate(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    gattc_write_characteristic_value_callback callback, void *user_data)
{
    return ble_host_gattc_write_ccc_value(conn_handle, cid, handle, false, false, callback, user_data);
}

int ble_host_gattc_write_ccc_value_enable_notify(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    gattc_write_characteristic_value_callback callback, void *user_data)
{
    return ble_host_gattc_write_ccc_value(conn_handle, cid, handle, true, false, callback, user_data);
}

int ble_host_gattc_write_ccc_value_enable_indicate(uint16_t conn_handle, uint16_t cid, uint16_t handle,
    gattc_write_characteristic_value_callback callback, void *user_data)
{
    return ble_host_gattc_write_ccc_value(conn_handle, cid, handle, false, true, callback, user_data);
}
