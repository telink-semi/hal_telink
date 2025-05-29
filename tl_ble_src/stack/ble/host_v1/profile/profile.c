#include <string.h>
#include <sys/queue.h>

#include "common/types.h"

#include "inc/profile.h"
#include "inc/profile_internal.h"

#include "../inc/ble_host.h"

#include "../inc/ble_host_sal.h"
#include "inc/profile_log.h"

// SSDP module
#include "../l2cap/att/inc/ble_att_uuid.h"
#include "../gatt/sdp/inc/ble_ssdp.h"

#include "../l2cap/inc/ble_l2cap.h"
#include "../gatt/inc/gatt.h"
#include "../gatt/gattc/inc/gattc_req.h"



SLIST_HEAD(ble_prf_process_list, ble_prf_process);

struct ble_prf_common_info {
    prf_event_callback event_cb;        /** < profile common event callback */
    struct ble_prf_process_list client_list; /** < list of registered profiles, client role only */
    struct ble_prf_process_list server_list; /** < list of registered profiles, server role only */
    uint8_t *prf_memory;
};

static struct ble_prf_common_info s_prf_common_info = {
    .event_cb = NULL,
    .client_list = SLIST_HEAD_INITIALIZER(s_prf_common_info.client_list),
    .server_list = SLIST_HEAD_INITIALIZER(s_prf_common_info.server_list),
};

struct ble_prf_conn_info {
    uint8_t prf_role;       /** < refer to enum prf_used_acl_role */
    uint8_t sdp_type;       /** < refer to enum prf_disc_type */
    uint8_t sdp_sec_flag;
    struct ble_prf_process *curr_sdp_server;
};

static void ble_prf_acl_connected(struct ble_host_conn *conn);
static void ble_prf_acl_disconnected(struct ble_host_conn *conn, uint8_t reason);
static void ble_prf_start_discovery(uint16_t conn_handle, struct ble_prf_conn_info *p_prf_conn_info);
static void ble_prf_discovery_single_finish_by_conn_handle(uint16_t conn_handle);

static const struct ble_host_acl_conn_callbacks s_prf_acl_conn_callback = {
    .connected = ble_prf_acl_connected,
    .disconnected = ble_prf_acl_disconnected,
};

void ble_prf_initial(struct ble_prf_init_param *param)
{
    s_prf_common_info.event_cb = param->event_cb;
    SLIST_INIT(&s_prf_common_info.client_list);
    SLIST_INIT(&s_prf_common_info.server_list);

    ble_host_sal_memory_pool_init(param->p_prf_memory, param->prf_memory_size);
    s_prf_common_info.prf_memory = param->p_prf_memory;

    ble_host_acl_conn_register_user_data(BLE_HOST_PROFILE_USER_ID, &s_prf_acl_conn_callback);
}

void *ble_prf_malloc(uint32_t size, uint16_t type_id)
{
    return ble_host_sal_memory_malloc(s_prf_common_info.prf_memory, size, type_id);
}

void ble_prf_free(void *ptr)
{
    ble_host_sal_memory_free(s_prf_common_info.prf_memory, ptr);
}

static void ble_prf_register_service_node(struct ble_prf_process *prf_proc, const void *param)
{
    const struct ble_prf_param *prf_params = prf_proc->prf_params;
    struct ble_prf_process_list *list = NULL;
    if (prf_params->client == 1) {
        list = &s_prf_common_info.client_list;
    } else {
        list = &s_prf_common_info.server_list;
    }

    struct ble_prf_process *cur = NULL, *prev = NULL;
    SLIST_FOREACH(cur, list, next)
    {
        if (cur == prf_proc) {
            return; // same service already registered.
        }

        if (cur->prf_params->service_id > prf_params->service_id) {
            // if insert node service id is less than current node service id, insert before current node.
            break;
        }

        prev = cur;
    }

    BLE_PRF_COMMON_DEBUG("register service_id: %d, is_client: %s",
        prf_params->service_id, prf_params->client == 1 ? "true" : "false");

    // if prev is NULL, insert node at head of list.
    if (prev == NULL) {
        SLIST_INSERT_HEAD(list, prf_proc, next);
    } else {
        SLIST_INSERT_AFTER(prev, prf_proc, next);
    }

    if (prf_params->init != NULL) {
        prf_params->init(PRF_PROCESS_INIT, param);
    }
}

static void ble_prf_unregister_service_node(struct ble_prf_process *prf_proc, const void *param)
{
    const struct ble_prf_param *prf_params = prf_proc->prf_params;

    struct ble_prf_process_list *list = NULL;
    if (prf_params->client == 1) {
        list = &s_prf_common_info.client_list;
    } else {
        list = &s_prf_common_info.server_list;
    }

    struct ble_prf_process *cur = NULL;

    // first check if service is registered.
    SLIST_FOREACH(cur, list, next)
    {
        if (cur == prf_proc) {
            break;
        }
    }

    if (cur == NULL) {
        return; // service not registered.
    }

    BLE_PRF_COMMON_DEBUG("unregister  service_id: %d, is_client: %s",
        prf_params->service_id, prf_params->client == 1 ? "true" : "false");

    SLIST_REMOVE(list, prf_proc, ble_prf_process, next);

    if (prf_params->init != NULL) {
        prf_params->init(PRF_PROCESS_DEINIT, param);
    }
}

void blc_prf_register_service_module(struct ble_prf_process *p_module, const void *param)
{
    if (p_module == NULL) {
        return;
    }

    ble_prf_register_service_node(p_module, param);
}

void blc_prf_unregister_service_module(struct ble_prf_process *p_module, const void *param)
{
    if (p_module) {
        return;
    }

    ble_prf_unregister_service_node(p_module, param);
}

/**************** ACL connection callback *******************/
static void ble_prf_acl_connected_callback(uint16_t conn_handle, uint8_t prf_role, uint8_t state, struct ble_prf_process_list *list)
{
    struct ble_prf_process *cur = NULL;
    SLIST_FOREACH(cur, list, next)
    {
        const struct ble_prf_param *prf_params = cur->prf_params;
        if (prf_params != NULL && prf_params->connect != NULL) {
            if (prf_params->used_acl_role & prf_role) {
                prf_params->connect(conn_handle, state);
            }
        }
    }
}

static void ble_prf_call_acl_state_change_callback(uint16_t conn_handle, uint8_t prf_role, enum prf_acl_state_change state)
{
    ble_prf_acl_connected_callback(conn_handle, prf_role, state, &s_prf_common_info.server_list);
    ble_prf_acl_connected_callback(conn_handle, prf_role, state, &s_prf_common_info.client_list);
}

static void ble_prf_acl_connected(struct ble_host_conn *conn)
{
    BLE_PRF_COMMON_INFO("profile ACL connected, conn handle:0x%03x", conn->conn_handle);

    struct ble_prf_conn_info *p_prf_conn_info = ble_prf_malloc(sizeof(struct ble_prf_conn_info), BLE_PRF_CONN_INFO_TYPE);
    if (p_prf_conn_info == NULL) {
        BLE_PRF_COMMON_ERROR("profile malloc connection info failed");
        return;
    }
    memset(p_prf_conn_info, 0, sizeof(struct ble_prf_conn_info));
    p_prf_conn_info->prf_role = conn->role == BLE_HOST_ACL_ROLE_CENTRAL ? PRF_USED_ACL_ROLE_CENTRAL : PRF_USED_ACL_ROLE_PERIPHERAL;
    conn->user_data[BLE_HOST_PROFILE_USER_ID] = p_prf_conn_info;

    ble_prf_call_acl_state_change_callback(conn->conn_handle, p_prf_conn_info->prf_role, PRF_ACL_CONNECT);
}

static void ble_prf_acl_disconnected(struct ble_host_conn *conn, uint8_t reason)
{
    BLE_PRF_COMMON_INFO("profile ACL disconnected, conn handle:0x%03x, reason:0x%02x", conn->conn_handle, reason);

    struct ble_prf_conn_info *p_prf_conn_info = conn->user_data[BLE_HOST_PROFILE_USER_ID];
    if (p_prf_conn_info != NULL) {
        ble_prf_call_acl_state_change_callback(conn->conn_handle, p_prf_conn_info->prf_role, PRF_ACL_DISCONNECT);
        ble_prf_free(p_prf_conn_info);
    }
}

/**************** report event to high layer *******************/
static void ble_prf_report_event(uint16_t conn_handle, uint8_t event_id, const void *event_msg)
{
    if (s_prf_common_info.event_cb != NULL) {
        s_prf_common_info.event_cb(conn_handle, event_id, event_msg);
    }
}

void ble_prf_report_sdp_found_end_event(uint16_t conn_handle, uint8_t service_id, const char *service_name)
{
    struct ble_prf_sdp_info msg = {
        .service_id = service_id,
        .service_name = service_name,
        .start_handle = 0,
        .end_handle = 0,
    };

    ble_prf_report_event(conn_handle, PRF_EVT_ID_SDP_FOUND_END, &msg);
    ble_prf_discovery_single_finish_by_conn_handle(conn_handle);
}

void ble_prf_report_sdp_not_found_event(uint16_t conn_handle, uint8_t service_id, const char *service_name)
{
    struct ble_prf_sdp_info msg = {
        .service_id = service_id,
        .service_name = service_name,
        .start_handle = 0,
        .end_handle = 0,
    };

    ble_prf_report_event(conn_handle, PRF_EVT_ID_SDP_NOT_FOUND, &msg);
    ble_prf_discovery_single_finish_by_conn_handle(conn_handle);
}

void ble_prf_report_sdp_found_event(uint16_t conn_handle, uint8_t service_id, const char *service_name,
    uint16_t start_handle, uint16_t end_handle)
{
    struct ble_prf_sdp_info msg = {
        .service_id = service_id,
        .service_name = service_name,
        .start_handle = start_handle,
        .end_handle = end_handle,
    };

    ble_prf_report_event(conn_handle, PRF_EVT_ID_SDP_FOUND, &msg);
}

static void ble_prf_report_sdp_finish_event(uint16_t conn_handle)
{
    ble_prf_report_event(conn_handle, PRF_EVT_ID_SDP_FINISH, NULL);
}

/****************** profile service discovery procedure *******************/
void ble_prf_discovery_common(uint16_t conn_handle, enum prf_disc_type type, struct ble_prf_disc_svc_param *param)
{
    if (type == PRF_DISC_TYPE_SVC) {
        int ret = 0;
        if (param->included) {
            ret = ble_host_gatt_ssdp_start(conn_handle, param->disc_list);
        } else {
            ret = ble_host_gatt_ssdp_start_no_include(conn_handle, param->disc_list);
        }
        BLE_PRF_COMMON_INFO("SDP start discovery, acl:0x%03x, ret:0x%x", conn_handle, ret);

    } else if (type == PRF_DISC_TYPE_RECONNECT) {

    } else if (type == PRF_DISC_TYPE_SERVICE_CHANGED) {
        // todo: handle service changed.
    }
}



static void ble_prf_discovery_single_finish(uint16_t conn_handle, struct ble_prf_conn_info *p_prf_conn_info)
{
    p_prf_conn_info->curr_sdp_server = SLIST_NEXT(p_prf_conn_info->curr_sdp_server, next);

    if (p_prf_conn_info->curr_sdp_server != NULL) {
        ble_prf_start_discovery(conn_handle, p_prf_conn_info);
    } else {
        ble_prf_report_sdp_finish_event(conn_handle);
        // storage
    }
}

static void ble_prf_discovery_single_finish_by_conn_handle(uint16_t conn_handle)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    if (conn == NULL || conn->user_data[BLE_HOST_PROFILE_USER_ID] == NULL) {
        return;
    }

    struct ble_prf_conn_info *p_prf_conn_info = conn->user_data[BLE_HOST_PROFILE_USER_ID];

    if (p_prf_conn_info == NULL) {
        return;
    }

    ble_prf_discovery_single_finish(conn_handle, p_prf_conn_info);
}

static void ble_prf_start_discovery(uint16_t conn_handle, struct ble_prf_conn_info *p_prf_conn_info)
{
    uint8_t prf_role = p_prf_conn_info->prf_role;

    struct ble_prf_process *cur = p_prf_conn_info->curr_sdp_server;

    if (cur != NULL) {
        const struct ble_prf_param *prf_params = cur->prf_params;
        if (prf_params != NULL && (prf_params->used_acl_role & prf_role) && prf_params->discovery != NULL) {
            prf_params->discovery(conn_handle, p_prf_conn_info->sdp_type);
        } else {
            ble_prf_discovery_single_finish(conn_handle, p_prf_conn_info);
        }
    }
}

static void ble_prf_discovery_start_callback(struct ble_host_conn *conn, enum prf_disc_type type, uint8_t sec_flag)
{
    struct ble_prf_conn_info *p_prf_conn_info = conn->user_data[BLE_HOST_PROFILE_USER_ID];

    if (p_prf_conn_info == NULL || p_prf_conn_info->curr_sdp_server != NULL) {
        return;
    }

    p_prf_conn_info->curr_sdp_server = SLIST_FIRST(&s_prf_common_info.client_list);
    p_prf_conn_info->sdp_type = type;
    p_prf_conn_info->sdp_sec_flag = sec_flag;
    ble_prf_start_discovery(conn->conn_handle, p_prf_conn_info);
}

void ble_prf_discovery_start(uint16_t conn_handle, enum prf_disc_type type, uint8_t sec_flag)
{
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);

    if (conn == NULL || conn->user_data[BLE_HOST_PROFILE_USER_ID] == NULL) {
        return;
    }

    ble_prf_discovery_start_callback(conn, type, sec_flag);
}

/**************** profile read attribute value ******************/
static bool ble_prf_read_attribute_value_callback(uint16_t conn_handle, uint16_t cid, uint32_t err,
    const struct gattc_read_characteristic_value *param)
{
    (void) cid;
    if (err != GATT_REQUEST_SUB_PROCEDURE_COMPLETE) {
        prf_read_callback callback = param->user_data;
        if (callback != NULL) {
            callback(conn_handle, err);
        }
    }
    return true;
}

int ble_prf_read_attribute_value(uint16_t conn_handle, uint16_t handle, struct ble_prf_read_value *prf_read_param)
{
    struct gattc_characteristic_value_write_info gatt_read_param = {
        .write_buffer = prf_read_param->data,
        .write_buffer_len = prf_read_param->length,
        .max_buffer_len = prf_read_param->max_data_len,
        .callback = ble_prf_read_attribute_value_callback,
        .user_data = prf_read_param->callback,
    };
    return ble_host_gattc_read_characteristic_value_write(conn_handle, LE_L2CAP_CID_ATT, handle,
        &gatt_read_param);
}
