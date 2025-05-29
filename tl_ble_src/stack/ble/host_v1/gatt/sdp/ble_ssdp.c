#include <sys/queue.h>
#include <string.h>

#include "common/types.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"

#include "../../l2cap/inc/ble_l2cap.h"
#include "../../l2cap/att/inc/ble_att.h"
#include "../../l2cap/att/inc/ble_att_uuid.h"
#include "../../l2cap/att/inc/uuid_def.h"

#include "../inc/gatt.h"
#include "../inc/gatt_internal.h"
#include "../inc/ble_gatt_log.h"
#include "../gattc/inc/gattc.h"
#include "../gattc/inc/gattc_req.h"

#include "inc/ble_ssdp.h"
#include "inc/ble_ssdp_internal.h"

// note: ssdp is only used in this file, ssdp means single service discovery.

#define BLE_GATT_SSDP_INFO_MALLOC(size)    ble_host_gatt_malloc(size, BLE_HOST_GATT_MALLOC_SSDP_INFO)
#define BLE_GATT_SSDP_INFO_FREE(ptr)       ble_host_gatt_free(ptr)

#define GET_GATT_SSDP_CONN_INFO(conn)          ((struct gatt_ssdp_conn_info *)(GET_GATT_CONN_INFO(conn)->gatt_ssdp_info))
#define GATT_SSDP_CONN_INFO_CHECK(conn)        (GATT_CONN_INFO_CHECK(conn) || GET_GATT_SSDP_CONN_INFO(conn) == NULL)

static void ble_host_gatt_ssdp_ctrl_callback(struct ble_host_conn *conn, uint8_t event, const void *param);
static void ble_host_gatt_ssdp_create_disc_primary_service_by_uuid_message(uint16_t conn_handle, struct gatt_ssdp_context *ssdp_ctx);

typedef int (*gatt_ssdp_message_handler_t)(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);

void ble_host_gatt_ssdp_init(void)
{
    ble_host_gattc_init();  // must init, because ssdp use gattc.
    ble_host_gatt_register_ctrl_callback(GATT_SSDP_USER_ID, ble_host_gatt_ssdp_ctrl_callback);
}

static inline void ble_host_gatt_ssdp_initial_request_context(struct gatt_ssdp_context *req_ctx)
{
    req_ctx->channel_id = LE_L2CAP_CID_ATT;
    // TAILQ_INIT(&req_ctx->req_queue);
}

static void ble_host_gatt_ssdp_acl_connected_event(struct ble_host_conn *conn, const void *param)
{
    (void) param;
    BLE_HOST_GATT_SDP_DEBUG("gatt ssdp ACL connected event, acl0x%03x", conn->conn_handle);
    BLE_HOST_SAL_ASSERT(GET_GATT_CONN_INFO(conn) != NULL);

    struct gatt_ssdp_conn_info *gatt_ssdp_conn_info = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_conn_info));

    if (gatt_ssdp_conn_info == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_ssdp connection info malloc failed");
        return;
    }

    GET_GATT_CONN_INFO(conn)->gatt_ssdp_info = gatt_ssdp_conn_info;
    STAILQ_INIT(&gatt_ssdp_conn_info->wait_list);
    ble_host_gatt_ssdp_initial_request_context(&gatt_ssdp_conn_info->att_ctx);
    for (int i = 0; i < BLE_GATT_MAX_EATT_CHANNELS; i++) {
        ble_host_gatt_ssdp_initial_request_context(&gatt_ssdp_conn_info->eatt_ctx[i]);
    }
    // maybe do something else here
}

static void ble_host_gatt_ssdp_acl_disconnected_event(struct ble_host_conn *conn, const void *param)
{
    uint8_t reason = *(uint8_t *) param;
    BLE_HOST_GATT_SDP_DEBUG("gatt_ssdp ACL disconnected event, acl0x%03x, reason:%d", conn->conn_handle, reason);

    BLE_HOST_SAL_ASSERT(!GATT_SSDP_CONN_INFO_CHECK(conn));
    if (GET_GATT_SSDP_CONN_INFO(conn) != NULL) {
        // maybe do something else here
        BLE_GATT_SSDP_INFO_FREE(GET_GATT_SSDP_CONN_INFO(conn));
    }
}

static void ble_host_gatt_ssdp_eatt_connected_event(struct ble_host_conn *conn, const void *param)
{
    // no support for EATT
    (void) conn;
    (void) param;
}

static void ble_host_gatt_ssdp_eatt_disconnected_event(struct ble_host_conn *conn, const void *param)
{
    // no support for EATT
    (void) conn;
    (void) param;
}

static void ble_host_gatt_ssdp_ctrl_callback(struct ble_host_conn *conn, uint8_t event, const void *param)
{
    switch (event) {
    case GATT_EVT_ACL_CONNECTED: {
        ble_host_gatt_ssdp_acl_connected_event(conn, param);
    }break;
    case GATT_EVT_ACL_DISCONNECTED: {
        ble_host_gatt_ssdp_acl_disconnected_event(conn, param);
    }break;
    case GATT_EVT_EATT_CONNECTTED: {
        ble_host_gatt_ssdp_eatt_connected_event(conn, param);
    }break;
    case GATT_EVT_EATT_DISCONNECTED: {
        ble_host_gatt_ssdp_eatt_disconnected_event(conn, param);
    }break;
    default:
        break;
    }
}

static struct gatt_ssdp_context *ble_host_gatt_ssdp_get_context_by_cid(struct gatt_ssdp_conn_info *gatt_ssdp_conn_info, uint16_t cid)
{
    if (cid == LE_L2CAP_CID_ATT) {
        return &gatt_ssdp_conn_info->att_ctx;
    } else {
        for (int i = 0; i < BLE_GATT_MAX_EATT_CHANNELS; i++) {
            if (gatt_ssdp_conn_info->eatt_ctx[i].channel_id != cid) {
                return &gatt_ssdp_conn_info->eatt_ctx[i];
            }
        }
    }

    return NULL;
}

static struct gatt_ssdp_context *ble_host_gatt_ssdp_get_idle_context(struct gatt_ssdp_conn_info *gatt_ssdp_conn_info)
{
    // check ATT state is idle.
    if (gatt_ssdp_conn_info->att_ctx.disc_param == NULL) {
        return &gatt_ssdp_conn_info->att_ctx;
    }

    // check EATT state is idle.
    for (int i = 0; i < BLE_GATT_MAX_EATT_CHANNELS; i++) {
        if (gatt_ssdp_conn_info->eatt_ctx[i].channel_id != LE_L2CAP_CID_ATT &&
            gatt_ssdp_conn_info->eatt_ctx[i].disc_param == NULL) {
            return &gatt_ssdp_conn_info->eatt_ctx[i];
        }
    }

    // no idle context
    return NULL;
}

/************************** GATT Client Single Service Discovery procedure **********/
static int ble_host_gatt_ssdp_disc_primary_service_by_uuid(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);
static int ble_host_gatt_ssdp_find_include(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);
static int ble_host_gatt_ssdp_disc_all_char(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);
static int ble_host_gatt_ssdp_disc_char_desc(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);
static int ble_host_gatt_ssdp_subscribe_ccc(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);
static int ble_host_gatt_ssdp_read_char_value(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param);

static gatt_ssdp_message_handler_t s_message_handler[] = {
    [SSDP_TYPE_DISC_PRIMARY] = ble_host_gatt_ssdp_disc_primary_service_by_uuid,
    [SSDP_TYPE_FIND_INCLUDE] = ble_host_gatt_ssdp_find_include,

    [SSDP_TYPE_DISC_INCLUDE_ALL_CHAR] = ble_host_gatt_ssdp_disc_all_char,
    [SSDP_TYPE_DISC_PRIMARY_ALL_CHAR] = ble_host_gatt_ssdp_disc_all_char,

    [SSDP_TYPE_DISC_INCLUDE_CHAR_DESC] = ble_host_gatt_ssdp_disc_char_desc,
    [SSDP_TYPE_DISC_PRIMARY_CHAR_DESC] = ble_host_gatt_ssdp_disc_char_desc,

    [SSDP_TYPE_SUBSCRIBE_INCLUDE_CCC] = ble_host_gatt_ssdp_subscribe_ccc,
    [SSDP_TYPE_SUBSCRIBE_PRIMARY_CCC] = ble_host_gatt_ssdp_subscribe_ccc,

    [SSDP_TYPE_READ_INCLUDE_CHAR_VALUE] = ble_host_gatt_ssdp_read_char_value,
    [SSDP_TYPE_READ_PRIMARY_CHAR_VALUE] = ble_host_gatt_ssdp_read_char_value,

    [SSDP_TYPE_DONE] = NULL,
};

static void ble_host_gatt_ssdp_insert_new_message(struct gatt_ssdp_param *disc_param, struct gatt_ssdp_message *msg)
{
    BLE_HOST_GATT_SDP_DEBUG("insert new message, type:%d", msg->type);

    struct gatt_ssdp_message *cur_msg = NULL, *prev_msg = NULL;
    SLIST_FOREACH(cur_msg, &disc_param->message_list, next)
    {
        if (cur_msg->type != SSDP_TYPE_DONE && cur_msg->type > msg->type) {
            break;
        }

        prev_msg = cur_msg;
    }

    if (prev_msg == NULL) {
        SLIST_INSERT_HEAD(&disc_param->message_list, msg, next);
    } else {
        SLIST_INSERT_AFTER(prev_msg, msg, next);
    }

}

static void ble_host_gatt_ssdp_finish_procedure(uint16_t conn_handle, uint16_t cid, struct gatt_ssdp_param *disc_param)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    struct gatt_ssdp_conn_info *gatt_ssdp_conn_info = GET_GATT_SSDP_CONN_INFO(conn);

    struct gatt_ssdp_context *ctx = ble_host_gatt_ssdp_get_context_by_cid(gatt_ssdp_conn_info, cid);

    if (STAILQ_EMPTY(&gatt_ssdp_conn_info->wait_list)) {
        BLE_HOST_GATT_SDP_DEBUG("wait list is empty, finish procedure acl:0x%03x, cid:0x%04x", conn_handle, cid);
        BLE_GATT_SSDP_INFO_FREE(ctx->disc_param);
        ctx->disc_param = NULL;
        ctx->channel_id = LE_L2CAP_CID_ATT;
    } else {
        struct gatt_ssdp_wait_service *wait_node = STAILQ_FIRST(&gatt_ssdp_conn_info->wait_list);
        BLE_HOST_GATT_SDP_DEBUG("wait list is not empty, start next procedure, acl:0x%03x, cid:0x%04x",
            conn_handle, cid);

        // start next procedure
        memset(disc_param, 0, sizeof(struct gatt_ssdp_param));
        disc_param->list = wait_node->list;
        disc_param->include_service = wait_node->include_service;

        STAILQ_REMOVE_HEAD(&gatt_ssdp_conn_info->wait_list, next);
        BLE_GATT_SSDP_INFO_FREE(wait_node);
        ble_host_gatt_ssdp_create_disc_primary_service_by_uuid_message(conn_handle, ctx);
    }
}

static void ble_host_gatt_ssdp_run_new_message(uint16_t conn_handle, uint16_t cid, struct gatt_ssdp_param *disc_param)
{
    struct gatt_ssdp_message *msg = SLIST_FIRST(&disc_param->message_list);

    if (msg != NULL && msg->type == SSDP_TYPE_DONE) {
        // message is done, remove it from list.
        SLIST_REMOVE_HEAD(&disc_param->message_list, next);
        BLE_GATT_SSDP_INFO_FREE(msg);
    }

    if (SLIST_EMPTY(&disc_param->message_list)) {
        // No new message, finish discovery procedure.
        BLE_HOST_GATT_SDP_DEBUG("No new message, finish discovery procedure");

        uint8_t service_count = disc_param->found_service_count == 0x00 ? GATT_SSDP_SERVICE_NOT_FOUND : GATT_SSDP_SERVICE_FOUND_END;
        if (disc_param->ssdp_list->service.service_callback != NULL) {
            disc_param->ssdp_list->service.service_callback(conn_handle, service_count, GATT_ATTR_HANDLE_NONE, GATT_ATTR_HANDLE_NONE);
        }

        ble_host_gatt_ssdp_finish_procedure(conn_handle, cid, disc_param);
        return;
    }

    msg = SLIST_FIRST(&disc_param->message_list);

    if (s_message_handler[msg->type] != NULL) {
        int ret = s_message_handler[msg->type](conn_handle, cid, msg, disc_param);
        BLE_HOST_GATT_SDP_DEBUG("run new message, type:%d, ret:0x%x", msg->type, ret);
        msg->type = SSDP_TYPE_DONE;  // mark message as done.
    }
}

static struct gatt_ssdp_message *ble_host_gatt_ssdp_get_current_message(struct gatt_ssdp_param *disc_param)
{
    return SLIST_FIRST(&disc_param->message_list);
}

static const struct gatt_ssdp_characteristic *ble_host_gatt_ssdp_check_characteristic_exist(
    const struct gatt_ssdp_characteristic_info *info, const struct att_uuid *characteristic_uuid)
{
    for (int i = 0; i < info->characteristic_size; i++) {
        // compare uuid
        if (ble_att_uuid_cmp(info->characteristic[i].characteristic_uuid, characteristic_uuid) == 0) {
            return &info->characteristic[i];
        }
    }

    return NULL;
}

/*************** Sixth step: Read Characteristic Value ***************/
static bool ble_host_gatt_ssdp_read_char_value_callback(uint16_t conn_handle, uint16_t cid, uint32_t err,
    const struct gattc_read_characteristic_value *param)
{
    struct gatt_ssdp_param *disc_param = param->user_data;

    if (err != GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        BLE_HOST_GATT_SDP_DEBUG("read characteristic value finished, acl:0x%03x, cid:0x%04x, err:0x%x",
            conn_handle, cid, err);

        ble_host_gatt_ssdp_run_new_message(conn_handle, cid, disc_param);
    }
    return true;
}

static int ble_host_gatt_ssdp_read_char_value(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param)
{
    struct gatt_ssdp_read_value *read_value = &msg->read_value;

    struct gattc_characteristic_value_write_info read_info = {
        .write_buffer = read_value->write_buffer,
        .write_buffer_len = read_value->write_buffer_len,
        .max_buffer_len = read_value->max_buffer_len,
        .user_data = disc_param,
        .callback = ble_host_gatt_ssdp_read_char_value_callback,
    };

    BLE_HOST_GATT_SDP_DEBUG("read characteristic, acl:0x%03x, cid:0x%04x, handle:0x%04x",
        conn_handle, cid, read_value->handle);
    return ble_host_gattc_read_characteristic_value_write(conn_handle, cid, read_value->handle, &read_info);
}

static void ble_host_gatt_ssdp_create_read_char_value_message(uint16_t conn_handle, uint8_t msg_type, uint16_t handle,
    const struct gatt_ssdp_characteristic *characteristic, struct gatt_ssdp_param *disc_param)
{
    struct gatt_ssdp_message *read_value_msg = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_message));
    if (read_value_msg == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_ssdp malloc read characteristic value message failed");
    } else {
        memset(read_value_msg, 0, sizeof(struct gatt_ssdp_message));
        read_value_msg->type = msg_type == SSDP_TYPE_DISC_PRIMARY ? SSDP_TYPE_READ_PRIMARY_CHAR_VALUE : SSDP_TYPE_READ_INCLUDE_CHAR_VALUE;
        struct gatt_ssdp_read_value *read_value = &read_value_msg->read_value;

        characteristic->read_value_callback(conn_handle, handle,
            &read_value->write_buffer, &read_value->write_buffer_len, &read_value->max_buffer_len);
        read_value->handle = handle;
        read_value->characteristic = characteristic;

        ble_host_gatt_ssdp_insert_new_message(disc_param, read_value_msg);
    }
}

/*************** Fifth step: Subscribe or Indicate or Notify ***************/
static void ble_host_gatt_ssdp_subscribe_ccc_callback(uint16_t conn_handle, uint16_t cid, uint32_t err, void *user_data)
{
    struct gatt_ssdp_param *disc_param = user_data;

    BLE_HOST_GATT_SDP_DEBUG("subscribe ccc callback, acl:0x%03x, cid:0x%04x, err:0x%x", conn_handle, cid, err);

    uint8_t att_err = ATT_SUCCESS;
    if (err == GATT_REQUEST_PROCEDURE_COMPLETE) {
        att_err = ATT_SUCCESS;
    } else if (err < 0x100) {
        att_err = err;
    } else {
        att_err = ATT_ERR_INVALID_HANDLE;
    }

    struct gatt_ssdp_message *p_msg = ble_host_gatt_ssdp_get_current_message(disc_param);

    const struct gatt_ssdp_characteristic *characteristic = p_msg->subscribe_ccc.characteristic;

    if (characteristic->subscribe_ccc_callback != NULL) {
        characteristic->subscribe_ccc_callback(conn_handle, p_msg->subscribe_ccc.ccc_handle, att_err);
    }

    ble_host_gatt_ssdp_run_new_message(conn_handle, cid, disc_param);
}

static int ble_host_gatt_ssdp_subscribe_ccc(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param)
{
    (void) disc_param;

    struct gatt_ssdp_subscribe_ccc *subscribe_ccc = &msg->subscribe_ccc;

    BLE_HOST_GATT_SDP_DEBUG("subscribe ccc, acl:0x%03x, cid:0x%04x, handle:0x%04x, notif:%d, indicate:%d",
        conn_handle, cid, subscribe_ccc->ccc_handle, subscribe_ccc->enable_notification, subscribe_ccc->enable_indication);
    return ble_host_gattc_write_ccc_value(conn_handle, cid, subscribe_ccc->ccc_handle, subscribe_ccc->enable_notification,
        subscribe_ccc->enable_indication, ble_host_gatt_ssdp_subscribe_ccc_callback, disc_param);
}

static void ble_host_gatt_ssdp_create_subscribe_ccc_message(uint8_t msg_type, uint8_t service_count, uint16_t handle,
    union characteristic_properties properties, const struct gatt_ssdp_characteristic *characteristic,
    struct gatt_ssdp_param *disc_param)
{
    struct gatt_ssdp_message *subscribe_msg = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_message));
    if (subscribe_msg == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_ssdp malloc subscribe CCC message failed");
    } else {
        subscribe_msg->type = msg_type == SSDP_TYPE_DISC_PRIMARY_CHAR_DESC ?
            SSDP_TYPE_SUBSCRIBE_PRIMARY_CCC : SSDP_TYPE_SUBSCRIBE_INCLUDE_CCC;

        struct gatt_ssdp_subscribe_ccc *subscribe_ccc = &subscribe_msg->subscribe_ccc;
        subscribe_ccc->service_count = service_count;
        subscribe_ccc->characteristic = characteristic;
        subscribe_ccc->ccc_handle = handle;
        subscribe_ccc->enable_indication = characteristic->subscribe_indicate && properties.indicate;
        subscribe_ccc->enable_notification = characteristic->subscribe_notify && properties.notify;

        ble_host_gatt_ssdp_insert_new_message(disc_param, subscribe_msg);
    }
}

/*************** Fourth step: Discover Primary Service's All Characteristics Descriptor ***************/
static bool ble_host_gatt_ssdp_disc_char_desc_found_callback(uint16_t conn_handle, uint16_t cid, uint32_t err,
    const struct gattc_disc_characteristic_desc *descriptor)
{
    struct gatt_ssdp_param *disc_param = descriptor->user_data;

    if (err == GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        BLE_HOST_GATT_SDP_DEBUG("descriptor found, acl:0x%03x, cid:0x%04x, attr0x%04x, uuid:%s",
            conn_handle, cid, descriptor->handle, ble_att_uuid_format(&descriptor->descriptor_uuid));

        struct gatt_ssdp_message *p_msg = ble_host_gatt_ssdp_get_current_message(disc_param);

        const struct gatt_ssdp_characteristic *characteristic = p_msg->disc_char_desc.characteristic;
        if (characteristic->desc_callback != NULL) {
            characteristic->desc_callback(conn_handle, &descriptor->descriptor_uuid, descriptor->handle);
        }

        union characteristic_properties properties = p_msg->disc_char_desc.properties;
        if (ble_att_uuid_cmp(&descriptor->descriptor_uuid, &descriptorClientCharacteristicConfigurationAttUuid) == 0 &&
            ((characteristic->subscribe_indicate && properties.indicate) ||
                (characteristic->subscribe_notify && properties.notify))
            ) {
                // create a new ssdp message to subscribe or indicate or notify.
            ble_host_gatt_ssdp_create_subscribe_ccc_message(p_msg->type, p_msg->disc_char_desc.service_count, descriptor->handle, properties, characteristic, disc_param);
        }
    } else {
        BLE_HOST_GATT_SDP_DEBUG("descriptor not found, acl:0x%03x, cid:0x%04x, err:0x%x",
            conn_handle, cid, err);

        ble_host_gatt_ssdp_run_new_message(conn_handle, cid, disc_param);
    }
    return true;
}

static int ble_host_gatt_ssdp_disc_char_desc(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param)
{
    (void) disc_param;

    struct gattc_disc_characteristic_desc_param *disc_desc = &msg->disc_char_desc.disc_desc;

    BLE_HOST_GATT_SDP_DEBUG("disc descriptor, acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x",
        conn_handle, cid, disc_desc->start_handle, disc_desc->end_handle);
    return ble_host_gattc_discover_characteristic_desc(conn_handle, cid, disc_desc);
}

static void ble_host_gatt_ssdp_create_disc_primary_char_desc_message(uint8_t msg_type, uint8_t service_count, uint16_t handle,
    struct disc_last_characteristic_info *p_last_char_info, struct gatt_ssdp_param *disc_param)
{
    // last characteristic is not the same as current characteristic, call last characteristic callback.
    if (p_last_char_info->characteristic->subscribe_indicate ||
        p_last_char_info->characteristic->subscribe_notify ||
        p_last_char_info->characteristic->found_descriptor) {
        // create a new ssdp message to discover descriptor of last characteristic.
        struct gatt_ssdp_message *disc_desc_msg = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_message));
        if (disc_desc_msg == NULL) {
            BLE_HOST_GATT_SDP_ERROR("gatt_ssdp malloc discover descriptor message failed");
        } else {
            disc_desc_msg->type = msg_type == SSDP_TYPE_DISC_PRIMARY ? SSDP_TYPE_DISC_PRIMARY_CHAR_DESC : SSDP_TYPE_DISC_INCLUDE_CHAR_DESC;
            struct gatt_ssdp_disc_char_desc *disc_desc = &disc_desc_msg->disc_char_desc;
            disc_desc->service_count = service_count;
            disc_desc->characteristic = p_last_char_info->characteristic;
            disc_desc->properties = p_last_char_info->properties;
            // set start_handle and end_handle of descriptor discovery procedure.
            disc_desc->disc_desc.start_handle = p_last_char_info->handle;
            disc_desc->disc_desc.end_handle = handle;
            disc_desc->disc_desc.user_data = disc_param;
            disc_desc->disc_desc.callback = ble_host_gatt_ssdp_disc_char_desc_found_callback;
            ble_host_gatt_ssdp_insert_new_message(disc_param, disc_desc_msg);
        }
    }
}

/*************** Third step: Discover Primary or Included Service's All Characteristics ***************/
static bool ble_host_gatt_ssdp_disc_all_char_found_callback(uint16_t conn_handle, uint16_t cid, uint32_t err,
    const struct gattc_disc_characteristic *characteristic)
{
    struct gatt_ssdp_param *disc_param = characteristic->user_data;
    struct disc_last_characteristic_info *p_last_char_info = &disc_param->last_char_info;
    struct gatt_ssdp_message *p_msg = ble_host_gatt_ssdp_get_current_message(disc_param);

    if (err == GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        BLE_HOST_GATT_SDP_DEBUG("characteristic found, acl:0x%03x, cid:0x%04x, attr0x%04x, properties:0x%02x, uuid:%s",
            conn_handle, cid, characteristic->value_handle, characteristic->properties,
            ble_att_uuid_format(&characteristic->characteristic_uuid));

        const struct gatt_ssdp_characteristic_info *p_char_info = p_msg->disc_all_char.char_info;
        const struct gatt_ssdp_characteristic *p_ssdp_char = ble_host_gatt_ssdp_check_characteristic_exist(
            p_char_info, &characteristic->characteristic_uuid);

        if (p_ssdp_char == NULL) {
            // characteristic not found in ssdp list, call unknown characteristic callback.
            if (p_char_info->unknown_characteristic_callback != NULL) {
                p_char_info->unknown_characteristic_callback(conn_handle,
                    &characteristic->characteristic_uuid, characteristic->properties.all, characteristic->value_handle);
            }
        } else {
            // characteristic found in ssdp list, characteristic callback found
            if (p_ssdp_char->characteristic_callback != NULL) {
                p_ssdp_char->characteristic_callback(conn_handle, p_msg->disc_all_char.service_count,
                    characteristic->properties.all, characteristic->value_handle);
            }

            // if characteristic has read permission, read characteristic value flag is true, callback function is not null,
            // create a new ssdp message to read characteristic value.
            if (characteristic->properties.read && p_ssdp_char->read_value && p_ssdp_char->read_value_callback != NULL) {
                BLE_HOST_GATT_CLIENT_DEBUG("create a new ssdp message to read characteristic value, acl:0x%03x, cid:0x%04x, attr0x%04x",
                    conn_handle, cid, characteristic->value_handle);

                // create a new ssdp message to read characteristic value.
                ble_host_gatt_ssdp_create_read_char_value_message(conn_handle, p_msg->type, characteristic->value_handle,
                    p_ssdp_char, disc_param);
            }
        }

        if (p_last_char_info->handle != GATT_ATTR_HANDLE_NONE &&
            p_last_char_info->characteristic != NULL &&
            characteristic->handle != p_last_char_info->handle) {

            ble_host_gatt_ssdp_create_disc_primary_char_desc_message(p_msg->type, p_msg->disc_all_char.service_count,
                characteristic->handle - 1, p_last_char_info, disc_param);
        }
        p_last_char_info->handle = characteristic->value_handle + 1;
        p_last_char_info->properties.all = characteristic->properties.all;
        p_last_char_info->characteristic = p_ssdp_char;
    } else {
        // GATT_REQUEST_PROCEDURE_COMPLETE or other error, mean discover all characteristic procedure failed or finished.
        BLE_HOST_GATT_SDP_DEBUG("discover all characteristic procedure complete, acl:0x%03x, cid:0x%04x, err:0x%x",
            conn_handle, cid, err);

        if (p_last_char_info->handle != GATT_ATTR_HANDLE_NONE &&
            p_last_char_info->handle <= disc_param->end_group_handle) {
            ble_host_gatt_ssdp_create_disc_primary_char_desc_message(p_msg->type, p_msg->disc_all_char.service_count,
                disc_param->end_group_handle, p_last_char_info, disc_param);
        }

        p_last_char_info->handle = GATT_ATTR_HANDLE_NONE;
        p_last_char_info->properties.all = 0;
        p_last_char_info->characteristic = NULL;
        ble_host_gatt_ssdp_run_new_message(conn_handle, cid, disc_param);
    }
    return true;
}

static int ble_host_gatt_ssdp_disc_all_char(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param)
{
    const struct gattc_disc_all_characteristics *disc_char = &msg->disc_all_char.disc_char;

    BLE_HOST_GATT_SDP_DEBUG("discovery all characteristics, acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x",
        conn_handle, cid, disc_char->start_handle, disc_char->end_handle);

    disc_param->end_group_handle = disc_char->end_handle;
    return ble_host_gattc_discover_all_characteristics_of_service(conn_handle, cid, disc_char);
}

static void ble_host_gatt_ssdp_create_disc_all_char_message(uint8_t msg_type, uint8_t service_count,
    uint16_t start_handle, uint16_t end_handle, const struct gatt_ssdp_characteristic_info *char_info,
    struct gatt_ssdp_param *disc_param)
{
    // create a new ssdp message to find  service's All Characteristics.
    struct gatt_ssdp_message *p_disc_char_msg = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_message));
    if (p_disc_char_msg == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_ssdp malloc discover service's all characteristics message failed");
    } else {

        p_disc_char_msg->type = msg_type;
        struct gatt_ssdp_disc_all_char *p_disc_char_info = &p_disc_char_msg->disc_all_char;
        p_disc_char_info->service_count = service_count;
        p_disc_char_info->char_info = char_info;
        p_disc_char_info->disc_char.start_handle = start_handle;
        p_disc_char_info->disc_char.end_handle = end_handle;
        p_disc_char_info->disc_char.user_data = disc_param;
        p_disc_char_info->disc_char.callback = ble_host_gatt_ssdp_disc_all_char_found_callback;
        ble_host_gatt_ssdp_insert_new_message(disc_param, p_disc_char_msg);
    }
}

/*************** Second step: Find Include Services ***************/
static bool ble_host_gatt_ssdp_find_include_found_callback(uint16_t conn_handle, uint16_t cid, uint32_t err,
    const struct gattc_find_incl_service *incl_service)
{
    struct gatt_ssdp_param *disc_param = incl_service->user_data;

    if (err == GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        BLE_HOST_GATT_SDP_DEBUG("found include service, conn:0x%03x, cid:0x%04x, start handle:0x%04x, end handle:0x%04x, uuid:%s",
            conn_handle, cid, incl_service->start_handle, incl_service->end_handle,
            ble_att_uuid_format(&incl_service->incl_service_uuid));
        const struct ble_gatt_ssdp_include_info *incl_info = &disc_param->ssdp_list->include_table;
        int i = 0;
        for (; i < incl_info->include_size; i++) {
            if (incl_info->include[i]->include_uuid != NULL &&
                ble_att_uuid_cmp(incl_info->include[i]->include_uuid, &incl_service->incl_service_uuid) == 0) {
                // include service found, call include service callback.
                if (incl_info->include[i]->include_callback != NULL) {
                    incl_info->include[i]->include_callback(conn_handle, incl_service->start_handle, incl_service->end_handle);
                }

                if (incl_info->include[i]->characteristic.characteristic_size > 0) {
                    struct gatt_ssdp_message *p_msg = ble_host_gatt_ssdp_get_current_message(disc_param);
                    // create a new ssdp message to find include service's All Characteristics.
                    ble_host_gatt_ssdp_create_disc_all_char_message(SSDP_TYPE_DISC_INCLUDE_ALL_CHAR, p_msg->find_include.primary_service_count,
                        incl_service->start_handle, incl_service->end_handle, &incl_info->include[i]->characteristic, disc_param);
                }
                break;
            }
        }
        if (i == incl_info->include_size) {
            // include service not found, call unknown include service callback.
            if (disc_param->ssdp_list->include_table.unknown_include_callback != NULL) {
                disc_param->ssdp_list->include_table.unknown_include_callback(conn_handle,
                    &incl_service->incl_service_uuid, incl_service->start_handle, incl_service->end_handle);
            }
        }
    } else {
        // GATT_REQUEST_PROCEDURE_COMPLETE or other error, mean find include procedure failed or finished.
        ble_host_gatt_ssdp_run_new_message(conn_handle, cid, disc_param);
    }

    return true;
}

static int ble_host_gatt_ssdp_find_include(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param)
{
    (void) disc_param;
    const struct gattc_find_incl_service_param *find_incl = &msg->find_include.find_incl;

    BLE_HOST_GATT_SDP_DEBUG("find include, acl:0x%03x, cid:0x%04x, start_handle:0x%04x, end_handle:0x%04x",
        conn_handle, cid, find_incl->start_handle, find_incl->end_handle);

    return ble_host_gattc_find_included_services(conn_handle, cid, find_incl);
}

static void ble_host_gatt_ssdp_create_find_include_message(uint16_t start_handle, uint16_t end_handle,
    struct gatt_ssdp_param *disc_param)
{
    // create a new ssdp message to find include services
    struct gatt_ssdp_message *find_incl_msg = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_message));
    if (find_incl_msg == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_ssdp malloc find include services message failed");
    } else {
        find_incl_msg->type = SSDP_TYPE_FIND_INCLUDE;
        struct gatt_ssdp_find_include *find_incl = &find_incl_msg->find_include;
        find_incl->primary_service_count = disc_param->found_service_count;
        find_incl->find_incl.start_handle = start_handle;
        find_incl->find_incl.end_handle = end_handle;
        find_incl->find_incl.user_data = disc_param;
        find_incl->find_incl.callback = ble_host_gatt_ssdp_find_include_found_callback;
        ble_host_gatt_ssdp_insert_new_message(disc_param, find_incl_msg);
    }
}

/*************** First step: Discover Primary Service by Service UUID ***************/
static bool ble_host_gatt_ssdp_primary_service_found_callback(uint16_t conn_handle, uint16_t cid, uint32_t err,
    const struct gattc_disc_services *service)
{
    bool ret = true;

    struct gatt_ssdp_param *disc_param = service->user_data;

    if (err == GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        const struct ble_gatt_ssdp_list *list = disc_param->ssdp_list;

        disc_param->found_service_count++;
        BLE_HOST_GATT_SDP_DEBUG("acl:0x%03x, cid:0x%04x, service count:%d, start_handle:0x%04x, end_handle:0x%04x",
            conn_handle, cid, disc_param->found_service_count, service->start_handle, service->end_handle);

        if (list->service.service_callback != NULL) {
            list->service.service_callback(conn_handle, disc_param->found_service_count,
                service->start_handle, service->end_handle);
        }

        if (disc_param->include_service && list->include_table.include_size > 0) {
            ble_host_gatt_ssdp_create_find_include_message(service->start_handle, service->end_handle, disc_param);
        }

        if (list->characteristic_table.characteristic_size > 0) {
            // create a new ssdp message to discover primary service's all characteristics
            ble_host_gatt_ssdp_create_disc_all_char_message(SSDP_TYPE_DISC_PRIMARY_ALL_CHAR, disc_param->found_service_count,
                service->start_handle, service->end_handle, &list->characteristic_table, disc_param);
        }

        if (list->max_service_count <= disc_param->found_service_count) {
            // all services are found, stop discover primary service by uuid procedure.
            ret = false;
        }
    } else {
        // GATT_REQUEST_PROCEDURE_COMPLETE or other error, mean discover primary service by uuid procedure failed or finished.
        BLE_HOST_GATT_SDP_DEBUG("discover primary service by uuid finished, acl:0x%03x, cid:0x%04x",
            conn_handle, cid);

        ret = false;
    }

    if (ret == false) {
        ble_host_gatt_ssdp_run_new_message(conn_handle, cid, disc_param);
    }

    return ret;
}

static int ble_host_gatt_ssdp_disc_primary_service_by_uuid(uint16_t conn_handle, uint16_t cid,
    struct gatt_ssdp_message *msg, struct gatt_ssdp_param *disc_param)
{
    (void) disc_param;
    BLE_HOST_GATT_SDP_DEBUG("discover primary service by uuid[%s], acl:0x%03x, cid:0x%04x",
        ble_att_uuid_format(msg->disc_primary.uuid), conn_handle, cid);

    return ble_host_gattc_discover_primary_service_by_uuid(conn_handle, cid,
        msg->disc_primary.uuid, ble_host_gatt_ssdp_primary_service_found_callback,
        msg->disc_primary.user_data);
}

static void ble_host_gatt_ssdp_create_disc_primary_service_by_uuid_message(uint16_t conn_handle, struct gatt_ssdp_context *ssdp_ctx)
{
    // create a new ssdp message to discover primary service by uuid.
    struct gatt_ssdp_message *disc_primary_service = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_message));
    if (disc_primary_service == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_ssdp malloc discover primary service's all characteristics message failed");
    } else {
        disc_primary_service->type = SSDP_TYPE_DISC_PRIMARY;
        struct gatt_ssdp_disc_primary *disc_primary = &disc_primary_service->disc_primary;
        disc_primary->uuid = ssdp_ctx->disc_param->ssdp_list->service.service_uuid;
        disc_primary->user_data = ssdp_ctx->disc_param;
        ble_host_gatt_ssdp_insert_new_message(ssdp_ctx->disc_param, disc_primary_service);
    }
    ble_host_gatt_ssdp_run_new_message(conn_handle, ssdp_ctx->channel_id, ssdp_ctx->disc_param);
}

static int ble_host_gatt_ssdp_start_common(uint16_t conn_handle, bool include_service, const void *list)
{
    if (list == NULL) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_PARAMS);
    }

    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    if (GATT_SSDP_CONN_INFO_CHECK(conn)) {
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    struct gatt_ssdp_wait_service *wait_node = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_wait_service));
    if (wait_node == NULL) {
        // maybe not used, but simplify the code design and malloc first.
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    struct gatt_ssdp_conn_info *gatt_ssdp_conn_info = GET_GATT_SSDP_CONN_INFO(conn);

    struct gatt_ssdp_context *idle_ctx = ble_host_gatt_ssdp_get_idle_context(gatt_ssdp_conn_info);

    if (idle_ctx == NULL) {
        // insert to wait list
        wait_node->include_service = include_service;
        wait_node->list = list;
        STAILQ_INSERT_TAIL(&gatt_ssdp_conn_info->wait_list, wait_node, next);
    } else {
        // release 
        BLE_GATT_SSDP_INFO_FREE(wait_node);

        struct gatt_ssdp_param *disc_param = BLE_GATT_SSDP_INFO_MALLOC(sizeof(struct gatt_ssdp_param));
        if (disc_param == NULL) {
            return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
        }
        memset(disc_param, 0, sizeof(struct gatt_ssdp_param));
        SLIST_INIT(&disc_param->message_list);
        disc_param->include_service = include_service;
        disc_param->list = list;
        idle_ctx->disc_param = disc_param;
        ble_host_gatt_ssdp_create_disc_primary_service_by_uuid_message(conn_handle, idle_ctx);
    }

    return BLE_HOST_ERR_SUCC;
}

int ble_host_gatt_ssdp_start(uint16_t conn_handle, const struct ble_gatt_ssdp_list *ssdp_list)
{
    return ble_host_gatt_ssdp_start_common(conn_handle, true, ssdp_list);
}

int ble_host_gatt_ssdp_start_no_include(uint16_t conn_handle, const struct ble_gatt_ssdp_no_include_list *ssdp_list)
{
    return ble_host_gatt_ssdp_start_common(conn_handle, false, ssdp_list);
}

