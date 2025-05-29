#include <string.h>

#include "common/types.h"

#include "../inc/ble_host_sal.h"
#include "../inc/ble_host.h"

#include "inc/ble_gatt_log.h"
#include "inc/gatt.h"
#include "inc/gatt_internal.h"

static void ble_host_gatt_acl_connected(struct ble_host_conn *conn);
static void ble_host_gatt_acl_disconnected(struct ble_host_conn *conn, uint8_t reason);

#define BLE_GATT_CONN_INFO_MALLOC(size) ble_host_gatt_malloc(size, BLE_HOST_GATT_MALLOC_CONN_INFO)
#define BLE_GATT_CONN_INFO_FREE(ptr)    ble_host_gatt_free(ptr)

struct ble_host_gatt_common_info {
    uint8_t *memory_addr;
};

static struct ble_host_gatt_common_info s_gatt_common_info = {
    .memory_addr = NULL,
};

static const struct ble_host_acl_conn_callbacks s_gatt_acl_conn_callbacks = {
    .connected = ble_host_gatt_acl_connected,
    .disconnected = ble_host_gatt_acl_disconnected,
};

static gatt_ctrl_callback_t s_gatt_ctrl_callbacks[GATT_MAX_USER_ID] = { NULL };

/**
 *   @brief this function is used to initialize the GATT module.
 *
 *   @param[in] p_gatt_memory: pointer to the GATT memory pool.
 *   @param[in] size: size of the GATT memory pool.
 *
 *   @return none.
 */
void ble_host_gatt_init(uint8_t *p_gatt_memory, uint32_t size)
{
    ble_host_sal_memory_pool_init(p_gatt_memory, size);
    s_gatt_common_info.memory_addr = p_gatt_memory;

    ble_host_acl_conn_register_user_data(BLE_HOST_GATT_USER_ID, &s_gatt_acl_conn_callbacks);
}

/**
 *   @brief this function is used to allocate memory from GATT memory pool.
 *
 *   @param[in] size: size of the memory to be allocated.
 *   @param[in] type_id: type ID of the memory to be allocated.
 *
 *   @return pointer to the allocated memory.
 */
void *ble_host_gatt_malloc(uint32_t size, uint16_t type_id)
{
    return ble_host_sal_memory_malloc(s_gatt_common_info.memory_addr, size, type_id);
}

/**
 *   @brief this function is used to free memory from GATT memory pool.
 *
 *   @param[in] ptr: pointer to the memory to be freed.
 *
 *   @return none.
 */
void ble_host_gatt_free(void *ptr)
{
    ble_host_sal_memory_free(s_gatt_common_info.memory_addr, ptr);
}

void ble_host_gatt_register_ctrl_callback(enum ble_gatt_user_id user_id, gatt_ctrl_callback_t callback)
{
    if (user_id < GATT_MAX_USER_ID) {
        s_gatt_ctrl_callbacks[user_id] = callback;
    }
}

void ble_host_gatt_unregister_ctrl_callback(enum ble_gatt_user_id user_id)
{
    if (user_id < GATT_MAX_USER_ID) {
        s_gatt_ctrl_callbacks[user_id] = NULL;
    }
}

static void ble_host_gatt_report_event(struct ble_host_conn *conn, uint8_t event, const void *param)
{
    for (int i = 0; i < GATT_MAX_USER_ID; i++) {
        if (s_gatt_ctrl_callbacks[i] != NULL) {
            s_gatt_ctrl_callbacks[i](conn, event, param);
        }
    }
}

static void ble_host_gatt_acl_connected(struct ble_host_conn *conn)
{
    BLE_HOST_GATT_COMMON_INFO("GATT ACL connected, conn handle:0x%03x", conn->conn_handle);

    struct ble_gatt_conn_info *p_gatt_conn_info = BLE_GATT_CONN_INFO_MALLOC(sizeof(struct ble_gatt_conn_info));
    if (p_gatt_conn_info == NULL) {
        BLE_HOST_GATT_COMMON_ERROR("GATT malloc connection info failed.");
        return;
    }

    memset(p_gatt_conn_info, 0, sizeof(struct ble_gatt_conn_info));
    conn->user_data[BLE_HOST_GATT_USER_ID] = p_gatt_conn_info;

    ble_host_gatt_report_event(conn, GATT_EVT_ACL_CONNECTED, NULL);
}

static void ble_host_gatt_acl_disconnected(struct ble_host_conn *conn, uint8_t reason)
{
    BLE_HOST_GATT_COMMON_INFO("GATT ACL disconnected, conn handle:0x%03x, reason:%d", conn->conn_handle, reason);

    struct ble_gatt_conn_info *p_gatt_conn_info = conn->user_data[BLE_HOST_GATT_USER_ID];

    if (p_gatt_conn_info == NULL) {
        BLE_HOST_GATT_COMMON_ERROR("GATT connection info not found, conn handle:0x%03x", conn->conn_handle);
        return;
    }

    ble_host_gatt_report_event(conn, GATT_EVT_ACL_DISCONNECTED, &reason);

    BLE_GATT_CONN_INFO_FREE(p_gatt_conn_info);
}
