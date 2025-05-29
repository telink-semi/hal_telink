#include <string.h>
#include <sys/queue.h>

#include "common/types.h"
#include "common/utility.h"

#include "../../inc/ble_host_sal.h"
#include "../../inc/ble_host.h"

#include "../../l2cap/inc/ble_l2cap.h"
#include "../../l2cap/att/inc/ble_att.h"
#include "../../l2cap/att/attc/inc/ble_attc.h"

#include "../inc/gatt.h"
#include "../inc/ble_gatt_log.h"
#include "../inc/gatt_internal.h"

#include "inc/gattc_internal.h"
#include "inc/gattc.h"

#define BLE_GATTC_CONN_INFO_MALLOC(size)    ble_host_gatt_malloc(size, BLE_HOST_GATT_MALLOC_CLIENT_CONN_INFO)
#define BLE_GATTC_CONN_INFO_FREE(ptr)       ble_host_gatt_free(ptr)

#define BLE_GATTC_REQ_CONTEXT_MALLOC(size)    ble_host_gatt_malloc(size, BLE_HOST_GATT_MALLOC_CLIENT_REQ_CONTEXT)
#define BLE_GATTC_REQ_CONTEXT_FREE(ptr)       ble_host_gatt_free(ptr)

#define GET_GATTC_CONN_INFO(conn)             ((struct gattc_conn_info *)(GET_GATT_CONN_INFO(conn)->gatt_client_info))
#define GATTC_CONN_INFO_CHECK(conn)           (GATT_CONN_INFO_CHECK(conn) || GET_GATTC_CONN_INFO(conn) == NULL)

static void ble_host_gattc_ctrl_callback(struct ble_host_conn *conn, uint8_t event, const void *param);

static void ble_host_gattc_recv_att_pdu(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len);

static void ble_host_gattc_recv_server_initiated_pdu(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len);

/** @brief GATT request message queue node */
struct gattc_req_message_info {
    TAILQ_ENTRY(gattc_req_message_info) next;   /** < Pointer to the next node */
    struct gattc_req_message msg;               /** < GATT request message */
    gattc_req_message_handler_t handler;        /** < GATT request message deal with handler */
};

/** @brief GATT request message queue */
struct gattc_request_context {
    uint16_t channel_id;                     /** < Channel ID. */
    void *timer;                             /** < Timer handle. */
    TAILQ_HEAD(, gattc_req_message_info) req_queue;  /** < GATT request message queue */
};

/** @brief GATT client connection information */
struct gattc_conn_info {
    struct gattc_request_context att_req_ctx;  /** < ATT request context */
    /** < EATT request context array. */
    struct gattc_request_context eatt_req_ctx[BLE_GATT_MAX_EATT_CHANNELS];
    SLIST_HEAD(, ble_gattc_ccc_message) ccc_msg_list; /** < CCC message list */
};

void ble_host_gattc_init(void)
{
    ble_host_gatt_register_ctrl_callback(GATT_CLIENT_USER_ID, ble_host_gattc_ctrl_callback);
    ble_host_attc_register_rsp_recv_pdu_callback(ble_host_gattc_recv_att_pdu);
    ble_host_attc_register_server_initiated_recv_pdu_callback(ble_host_gattc_recv_server_initiated_pdu);
}

static inline void ble_host_gattc_initial_request_context(struct gattc_request_context *req_ctx)
{
    req_ctx->channel_id = LE_L2CAP_CID_ATT;
    req_ctx->timer = NULL;
    TAILQ_INIT(&req_ctx->req_queue);
}

static void ble_host_gattc_acl_connected_event(struct ble_host_conn *conn, const void *param)
{
    (void) param;
    BLE_HOST_GATT_CLIENT_DEBUG("GATTC ACL connected event, acl0x%03x", conn->conn_handle);
    BLE_HOST_SAL_ASSERT(GET_GATT_CONN_INFO(conn) != NULL);

    struct gattc_conn_info *gattc_conn_info = BLE_GATTC_CONN_INFO_MALLOC(sizeof(struct gattc_conn_info));

    if (gattc_conn_info == NULL) {
        BLE_HOST_GATT_CLIENT_ERROR("GATTC connection info malloc failed");
        return;
    }

    GET_GATT_CONN_INFO(conn)->gatt_client_info = gattc_conn_info;
    ble_host_gattc_initial_request_context(&gattc_conn_info->att_req_ctx);
    for (int i = 0; i < BLE_GATT_MAX_EATT_CHANNELS; i++) {
        ble_host_gattc_initial_request_context(&gattc_conn_info->eatt_req_ctx[i]);
    }
    SLIST_INIT(&gattc_conn_info->ccc_msg_list);
    // maybe do something else here
}

static void ble_host_gattc_acl_disconnected_event(struct ble_host_conn *conn, const void *param)
{
    uint8_t reason = *(uint8_t *) param;
    BLE_HOST_GATT_CLIENT_DEBUG("GATTC ACL disconnected event, acl0x%03x, reason:%d", conn->conn_handle, reason);

    BLE_HOST_SAL_ASSERT(!GATTC_CONN_INFO_CHECK(conn));
    if (GET_GATTC_CONN_INFO(conn) != NULL) {
        // maybe do something else here
        BLE_GATTC_CONN_INFO_FREE(GET_GATTC_CONN_INFO(conn));
    }
}

static void ble_host_gattc_eatt_connected_event(struct ble_host_conn *conn, const void *param)
{
    // no support for EATT
    (void) conn;
    (void) param;
}

static void ble_host_gattc_eatt_disconnected_event(struct ble_host_conn *conn, const void *param)
{
    // no support for EATT
    (void) conn;
    (void) param;
}

static void ble_host_gattc_ctrl_callback(struct ble_host_conn *conn, uint8_t event, const void *param)
{
    switch (event) {
    case GATT_EVT_ACL_CONNECTED: {
        ble_host_gattc_acl_connected_event(conn, param);
    }break;
    case GATT_EVT_ACL_DISCONNECTED: {
        ble_host_gattc_acl_disconnected_event(conn, param);
    }break;
    case GATT_EVT_EATT_CONNECTTED: {
        ble_host_gattc_eatt_connected_event(conn, param);
    }break;
    case GATT_EVT_EATT_DISCONNECTED: {
        ble_host_gattc_eatt_disconnected_event(conn, param);
    }break;
    default:
        break;
    }
}

static struct gattc_conn_info *ble_host_gattc_get_conn_info(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    if (GATTC_CONN_INFO_CHECK(conn)) {
        return NULL;
    }

    return GET_GATTC_CONN_INFO(conn);
}

static struct gattc_request_context *ble_host_gattc_get_request_context(uint16_t conn_handle, uint16_t cid)
{
    struct gattc_conn_info *gattc_conn_info = ble_host_gattc_get_conn_info(conn_handle);

    if (gattc_conn_info == NULL) {
        return NULL;
    }

    if (cid == LE_L2CAP_CID_ATT) {
        return &gattc_conn_info->att_req_ctx;
    }

    for (int i = 0; i < BLE_GATT_MAX_EATT_CHANNELS; i++) {
        if (gattc_conn_info->eatt_req_ctx[i].channel_id == cid) {
            return &gattc_conn_info->eatt_req_ctx[i];
        }
    }

    return NULL;
}

static void ble_host_gattc_run_handler_and_update_time(uint16_t conn_handle, uint16_t cid,
    struct gattc_req_message_info *msg_info, void *timer)
{
    int ret = msg_info->handler(conn_handle, cid, msg_info->msg.user_data);
    if (ret != BLE_HOST_ERR_SUCC) {
        // maybe todo, if error need callback handler again.
    }
    // update timer and start.
    ble_host_sal_timer_stop(timer);
    ble_host_sal_timer_update_timeout(timer, GATTC_REQUEST_TIMEOUT_MS);
    ble_host_sal_timer_start(timer);
}

static void ble_host_gattc_delete_request_message(uint16_t conn_handle, uint16_t cid,
    struct gattc_request_context *req_ctx, struct gattc_req_message_info *msg_info)
{
    // stop timer first.
    ble_host_sal_timer_stop(req_ctx->timer);
    // remove first message.
    TAILQ_REMOVE(&req_ctx->req_queue, msg_info, next);
    ble_host_gatt_free(msg_info->msg.user_data);
    BLE_GATTC_REQ_CONTEXT_FREE(msg_info);

    msg_info = TAILQ_FIRST(&req_ctx->req_queue);
    if (msg_info == NULL) {
        ble_host_sal_timer_delete(req_ctx->timer);
        req_ctx->timer = NULL;
    } else {
        ble_host_gattc_run_handler_and_update_time(conn_handle, cid, msg_info, req_ctx->timer);
    }
}

static void ble_host_gattc_continue_run_node(uint16_t conn_handle, uint16_t cid,
    struct gattc_request_context *req_ctx, struct gattc_req_message_info *msg_info)
{
    ble_host_gattc_run_handler_and_update_time(conn_handle, cid, msg_info, req_ctx->timer);
}

static void ble_host_gattc_recv_att_pdu(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len)
{
    struct gattc_request_context *req_ctx = ble_host_gattc_get_request_context(conn_handle, cid);

    if (req_ctx == NULL) {
        return;
    }

    struct gattc_req_message_info *msg_info = TAILQ_FIRST(&req_ctx->req_queue);

    if (msg_info == NULL) {
        return;
    }

    struct gattc_req_message *msg = &msg_info->msg;

    if (msg == NULL) {
        BLE_HOST_GATT_CLIENT_ERROR("request message not found, acl:0x%03x, cid:0x%04x, opcode:%d, len:%d",
            conn_handle, cid, opcode, pdu_len);
        return;
    }

    bool msg_finished = true;
    if (opcode == ATT_OPCODE_ERROR_RSP) {
        uint8_t request_opcode = pdu[0];
        uint16_t handle = (pdu[1] << 8) | pdu[2];
        uint8_t error_code = pdu[3];
        if (msg->request_opcode != request_opcode) {
            BLE_HOST_GATT_CLIENT_ERROR("GATTC recv error rsp, acl0x%03x, cid:0x%04x, req:%d, handle:0x%04x, error:%d",
                conn_handle, cid, request_opcode, handle, error_code);
            return;
        }

        if (msg->error_callback != NULL) {
            msg->error_callback(conn_handle, cid, handle, error_code, msg->user_data);
        }
    } else {
        if (msg->expect_opcode != opcode) {
            BLE_HOST_GATT_CLIENT_ERROR("request message not found, acl:0x%03x, cid:0x%04x, opcode:%d, len:%d",
                conn_handle, cid, opcode, pdu_len);
            return;
        }

        if (msg->rsp_callback) {
            msg_finished = msg->rsp_callback(conn_handle, cid, pdu, pdu_len, msg->user_data);
        }
    }

    if (msg_finished) {
        ble_host_gattc_delete_request_message(conn_handle, cid, req_ctx, msg_info);
    } else {
        ble_host_gattc_continue_run_node(conn_handle, cid, req_ctx, msg_info);
    }
}

static void ble_host_gattc_request_timeout_handler(void *arg)
{
    // todo: handle timeout event
    (void) arg;
}

int ble_host_gattc_send_request(uint16_t conn_handle, uint16_t cid, const struct gattc_req_message *msg,
    gattc_req_message_handler_t handler)
{
    struct gattc_request_context *req_ctx = ble_host_gattc_get_request_context(conn_handle, cid);

    if (req_ctx == NULL) {
        BLE_HOST_GATT_CLIENT_ERROR("get request context failed, acl:0x%03x, cid:0x%04x, req:%d",
            conn_handle, cid, msg->request_opcode);
        ble_host_gatt_free(msg->user_data);     // if user_data is not NULL, free it
        return BLE_GATT_ERR(BLE_GATT_ERR_INVALID_CONN_HANDLE);
    }

    struct gattc_req_message_info *msg_node = BLE_GATTC_REQ_CONTEXT_MALLOC(sizeof(struct gattc_req_message_info));

    if (msg_node == NULL) {
        BLE_HOST_GATT_CLIENT_ERROR("malloc failed for request, acl:0x%03x, cid:0x%04x, req:%d",
            conn_handle, cid, msg->request_opcode);
        ble_host_gatt_free(msg->user_data);     // if user_data is not NULL, free it
        return BLE_GATT_ERR(BLE_GATT_ERR_INSUFFICIENT_RESOURCES);
    }

    if (TAILQ_EMPTY(&req_ctx->req_queue)) {
        int ret = handler(conn_handle, cid, msg->user_data);

        if (ret != BLE_HOST_ERR_SUCC) {
            BLE_GATTC_REQ_CONTEXT_FREE(msg_node);
            ble_host_gatt_free(msg->user_data);     // if user_data is not NULL, free it
            return ret;
        }
        // start timer
        req_ctx->timer = ble_host_sal_timer_create(ble_host_gattc_request_timeout_handler, req_ctx, GATTC_REQUEST_TIMEOUT_MS);
        ble_host_sal_timer_start(req_ctx->timer);
    }

    memcpy(&msg_node->msg, msg, sizeof(struct gattc_req_message));
    msg_node->handler = handler;
    TAILQ_INSERT_TAIL(&req_ctx->req_queue, msg_node, next);

    return BLE_HOST_ERR_SUCC;
}



void ble_host_gattc_update_laster_message_info(uint16_t conn_handle, uint16_t cid,
    const struct gattc_req_message *msg, gattc_req_message_handler_t handler)
{
    struct gattc_request_context *req_ctx = ble_host_gattc_get_request_context(conn_handle, cid);
    struct gattc_req_message_info *msg_info = TAILQ_FIRST(&req_ctx->req_queue);

    memcpy(&msg_info->msg, msg, sizeof(struct gattc_req_message));
    msg_info->handler = handler;
}

bool ble_host_gattc_is_connected(uint16_t conn_handle, uint16_t cid)
{
    // todo : check if connection is connected
    (void) conn_handle;
    (void) cid;
    return true;
}

bool ble_host_gattc_add_subscribe_ccc_message(uint16_t conn_handle, struct ble_gattc_ccc_message *ccc_msg)
{
    struct gattc_conn_info *gattc_conn_info = ble_host_gattc_get_conn_info(conn_handle);

    if (ccc_msg == NULL || gattc_conn_info == NULL) {
        return false;
    }

    struct ble_gattc_ccc_message *cur_msg = NULL;
    SLIST_FOREACH(cur_msg, &gattc_conn_info->ccc_msg_list, next)
    {
        if (cur_msg == ccc_msg) {
            return false;
        }
    }

    SLIST_INSERT_HEAD(&gattc_conn_info->ccc_msg_list, ccc_msg, next);
    return true;
}

bool ble_host_gattc_remove_subscribe_ccc_message(uint16_t conn_handle, struct ble_gattc_ccc_message *ccc_msg)
{
    struct gattc_conn_info *gattc_conn_info = ble_host_gattc_get_conn_info(conn_handle);

    if (ccc_msg == NULL || gattc_conn_info == NULL) {
        return false;
    }

    struct ble_gattc_ccc_message *cur_msg = NULL;
    SLIST_FOREACH(cur_msg, &gattc_conn_info->ccc_msg_list, next)
    {
        if (cur_msg == ccc_msg) {
            SLIST_REMOVE(&gattc_conn_info->ccc_msg_list, cur_msg, ble_gattc_ccc_message, next);
            return true;
        }
    }

    return false;
}

void ble_host_gattc_clean_subscribe_ccc_message(uint16_t conn_handle)
{
    struct gattc_conn_info *gattc_conn_info = ble_host_gattc_get_conn_info(conn_handle);

    if (gattc_conn_info == NULL) {
        return;
    }

    SLIST_INIT(&gattc_conn_info->ccc_msg_list);
}

static void ble_host_gattc_recv_server_initiated_pdu(uint16_t conn_handle, uint16_t cid, uint8_t opcode,
    const uint8_t *pdu, uint16_t pdu_len)
{
    struct gattc_conn_info *gattc_conn_info = ble_host_gattc_get_conn_info(conn_handle);

    if (gattc_conn_info == NULL) {
        return;
    }

    if (opcode == ATT_OPCODE_MULTIPLE_HANDLE_VALUE_NTF) {
        // todo: handle multiple handle value ntf
        return;
    }

    uint16_t handle;
    STREAM_TO_U16(handle, pdu);
    pdu_len -= 2;

    struct ble_gattc_ccc_value ccc_value = {
        .attr_handle = handle,
        .cid = cid,
        .opcode = opcode,
        .value = pdu,
        .value_length = pdu_len,
    };

    struct ble_gattc_ccc_message *cur_msg = NULL;
    SLIST_FOREACH(cur_msg, &gattc_conn_info->ccc_msg_list, next)
    {
        if (handle >= cur_msg->start_handle && handle <= cur_msg->end_handle) {
            if (cur_msg->report_callback != NULL) {
                cur_msg->report_callback(conn_handle, &ccc_value);
            }
            break;  // message only handle one handle, so break here.
        }
    }
}


