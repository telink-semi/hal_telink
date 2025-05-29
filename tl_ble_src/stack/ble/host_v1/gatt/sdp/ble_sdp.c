#include "common/types.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"

#include "../../l2cap/inc/ble_l2cap.h"
#include "../../l2cap/att/inc/ble_att_uuid.h"

#include "../inc/gatt.h"
#include "../inc/gatt_internal.h"
#include "../inc/ble_gatt_log.h"
#include "../gattc/inc/gattc.h"
#include "../gattc/inc/gattc_req.h"

#define BLE_GATT_SDP_CONN_INFO_MALLOC(size)    ble_host_gatt_malloc(size, BLE_HOST_GATT_MALLOC_SDP_INFO)
#define BLE_GATT_SDP_CONN_INFO_FREE(ptr)       ble_host_gatt_free(ptr)

#define GET_GATT_SDP_CONN_INFO(conn)          ((struct gatt_sdp_conn_info *)(GET_GATT_CONN_INFO(conn)->gatt_sdp_info))
#define GATT_SDP_CONN_INFO_CHECK(conn)        (GATT_CONN_INFO_CHECK(conn) || GET_GATT_SDP_CONN_INFO(conn) == NULL)

static void ble_host_gatt_sdp_ctrl_callback(struct ble_host_conn *conn, uint8_t event, const void *param);

/** @brief GATT request message queue */
struct gatt_sdp_context {
    uint16_t channel_id;                     /** < Channel ID. */
};

/** @brief GATT client connection information */
struct gatt_sdp_conn_info {
    struct gatt_sdp_context att_ctx;  /** < ATT request context */
    /** < EATT request context array. */
    struct gatt_sdp_context eatt_ctx[BLE_GATT_MAX_EATT_CHANNELS];
};

void ble_host_gatt_sdp_init(void)
{
    ble_host_gattc_init();  // must init, because sdp use gattc.
    ble_host_gatt_register_ctrl_callback(GATT_SDP_USER_ID, ble_host_gatt_sdp_ctrl_callback);
}

static inline void ble_host_gatt_sdp_initial_request_context(struct gatt_sdp_context *req_ctx)
{
    req_ctx->channel_id = LE_L2CAP_CID_ATT;
    // TAILQ_INIT(&req_ctx->req_queue);
}

static void ble_host_gatt_sdp_acl_connected_event(struct ble_host_conn *conn, const void *param)
{
    (void) param;
    BLE_HOST_GATT_SDP_DEBUG("gatt sdp ACL connected event, acl0x%03x", conn->conn_handle);
    BLE_HOST_SAL_ASSERT(GET_GATT_CONN_INFO(conn) != NULL);

    struct gatt_sdp_conn_info *gatt_sdp_conn_info = BLE_GATT_SDP_CONN_INFO_MALLOC(sizeof(struct gatt_sdp_conn_info));

    if (gatt_sdp_conn_info == NULL) {
        BLE_HOST_GATT_SDP_ERROR("gatt_sdp connection info malloc failed");
        return;
    }

    GET_GATT_CONN_INFO(conn)->gatt_sdp_info = gatt_sdp_conn_info;
    ble_host_gatt_sdp_initial_request_context(&gatt_sdp_conn_info->att_ctx);
    for (int i = 0; i < BLE_GATT_MAX_EATT_CHANNELS; i++) {
        ble_host_gatt_sdp_initial_request_context(&gatt_sdp_conn_info->eatt_ctx[i]);
    }
    // maybe do something else here
}

static void ble_host_gatt_sdp_acl_disconnected_event(struct ble_host_conn *conn, const void *param)
{
    uint8_t reason = *(uint8_t *) param;
    BLE_HOST_GATT_SDP_DEBUG("gatt_sdp ACL disconnected event, acl0x%03x, reason:%d", conn->conn_handle, reason);

    BLE_HOST_SAL_ASSERT(!GATT_SDP_CONN_INFO_CHECK(conn));
    if (GET_GATT_SDP_CONN_INFO(conn) != NULL) {
        // maybe do something else here
        BLE_GATT_SDP_CONN_INFO_FREE(GET_GATT_SDP_CONN_INFO(conn));
    }
}

static void ble_host_gatt_sdp_eatt_connected_event(struct ble_host_conn *conn, const void *param)
{
    // no support for EATT
    (void) conn;
    (void) param;
}

static void ble_host_gatt_sdp_eatt_disconnected_event(struct ble_host_conn *conn, const void *param)
{
    // no support for EATT
    (void) conn;
    (void) param;
}

static void ble_host_gatt_sdp_ctrl_callback(struct ble_host_conn *conn, uint8_t event, const void *param)
{
    switch (event) {
    case GATT_EVT_ACL_CONNECTED: {
        ble_host_gatt_sdp_acl_connected_event(conn, param);
    }break;
    case GATT_EVT_ACL_DISCONNECTED: {
        ble_host_gatt_sdp_acl_disconnected_event(conn, param);
    }break;
    case GATT_EVT_EATT_CONNECTTED: {
        ble_host_gatt_sdp_eatt_connected_event(conn, param);
    }break;
    case GATT_EVT_EATT_DISCONNECTED: {
        ble_host_gatt_sdp_eatt_disconnected_event(conn, param);
    }break;
    default:
        break;
    }
}

void ble_host_gatt_sdp_start(uint16_t conn_handle)
{
    (void) conn_handle;
    // todo: start sdp service here
}
