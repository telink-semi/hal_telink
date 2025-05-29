
#include "common/types.h"

#include "../../inc/ble_host_internal.h"
#include "../../inc/ble_host_sal.h"
#include "../../inc/ble_host.h"
#include "../inc/ble_gap_log.h"
enum gap_reader_event
{
    EVENT_CONNECT,
    EVENT_FEATURES_EXCHANGE_COMPLETE,
    EVENT_VERSION_EXCHANGE_COMPLETE,
    EVENT_DISCONNECT,
};
enum gap_reader_state
{
    STATE_IDLE,
    STATE_FEATURES_EXCHANGE,
    STATE_VERSION_EXCHANGE,
};
#define FSM_STATE_INVALID 0xFF
#define FSM_END_STATE     {FSM_STATE_INVALID, 0, FSM_STATE_INVALID, NULL}

typedef bool (*event_handler_t)(uint8_t conn_handle);
struct gap_reader_fsm
{
    uint8_t         current_state;
    uint8_t         event;
    uint8_t         next_state;
    event_handler_t callback;
};

static void ble_host_reader_acl_connected(struct ble_host_conn *conn);
static void ble_host_reader_acl_disconnected(struct ble_host_conn *conn, uint8_t reason);
static bool ble_host_gap_read_version(uint8_t conn_handle);
static bool ble_host_gap_read_feature(uint8_t conn_handle);
static bool ble_host_gap_reader_handler(uint8_t *state, const uint8_t event, const uint8_t data);

static const struct ble_host_acl_conn_callbacks s_host_reader_conn_callbacks = {
    .connected = ble_host_reader_acl_connected,
    .disconnected = ble_host_reader_acl_disconnected,
};

static const char *s_gap_state_str[] = {
    "IDLE",
    "FEATURES_EXCHANGE",
    "VERSION_EXCHANGE",
};

static const struct gap_reader_fsm s_gap_reader_fsm[] = {
    {STATE_IDLE, EVENT_CONNECT, STATE_FEATURES_EXCHANGE, ble_host_gap_read_feature},
    {STATE_FEATURES_EXCHANGE, EVENT_FEATURES_EXCHANGE_COMPLETE, STATE_VERSION_EXCHANGE, ble_host_gap_read_version},
    {STATE_VERSION_EXCHANGE, EVENT_VERSION_EXCHANGE_COMPLETE, STATE_IDLE, NULL},
    FSM_END_STATE,
};

static void app_ble_printf_all_state(uint8_t state)
{
    BLE_HOST_GAP_COMMON_INFO("gap reader state: %s, PA Sync State: %s, BIG Sync State: %s", s_gap_state_str[state]);
}

static bool ble_host_gap_reader_handler(uint8_t *state, const uint8_t event, const uint8_t data)
{
    if (state == NULL) {
        BLE_HOST_GAP_COMMON_INFO("Error: Invalid input parameters.\n");
        return false;
    }
    bool ret         = true;
    bool match_found = false;
    for (int i = 0;; i++) {
        if (s_gap_reader_fsm[i].current_state == FSM_STATE_INVALID) {
            break;
        }

        if (s_gap_reader_fsm[i].event == event && s_gap_reader_fsm[i].current_state == *state) {
            match_found = true;

            *state = s_gap_reader_fsm[i].next_state;
            app_ble_printf_all_state(*state );
            if (s_gap_reader_fsm[i].callback != NULL) {
                ret = s_gap_reader_fsm[i].callback(data);
            }
            break;
        }
    }
    if (!match_found) {
        BLE_HOST_GAP_COMMON_ERROR("Error: No matching rule for event %d in state %d \n", event, *state);
        ret = false;
    }

    return ret;
}

static bool ble_host_gap_read_version(uint8_t conn_handle) {
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    (void)conn;
    ble_host_lock();
    //TODO hci send read remote version
    ble_host_unlock();
    return 0;
}

static bool ble_host_gap_read_feature(uint8_t conn_handle) {
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    (void)conn;
    ble_host_lock();
    //TODO hci send read remote feature
    ble_host_unlock();
    return 0;
}

static void ble_host_reader_acl_connected(struct ble_host_conn *conn) {
    ble_host_lock();
    conn->version = 0;
    conn->supported_feat = 0;
    conn->remote_version = 0;
    conn->remote_feat = 0;
    conn->reader_status = STATE_IDLE;
    ble_host_gap_reader_handler(&conn->reader_status, EVENT_CONNECT, conn->conn_handle);
    ble_host_unlock();
}

static void ble_host_reader_acl_disconnected(struct ble_host_conn *conn, uint8_t reason) {
    (void)reason;
    conn->reader_status = STATE_IDLE;
}

void ble_host_gap_read_remote_feature(uint8_t conn_handle, const uint64_t features) {
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    ble_host_lock();
    conn->remote_feat = features;
    ble_host_gap_reader_handler(&conn->reader_status, EVENT_FEATURES_EXCHANGE_COMPLETE, conn->conn_handle);
    ble_host_unlock();
    BLE_HOST_GAP_COMMON_ERROR("remote feature: %d", features);
}

void ble_host_gap_read_remote_version(uint8_t conn_handle, const uint8_t version) {
    struct ble_host_conn *conn = ble_host_conn_find_by_conn_handle(conn_handle);
    ble_host_lock();
    conn->remote_version = version;
    ble_host_gap_reader_handler(&conn->reader_status, EVENT_VERSION_EXCHANGE_COMPLETE, conn->conn_handle);
    ble_host_unlock();
    BLE_HOST_GAP_COMMON_ERROR("remote version: %d", version);
}

void ble_host_reader_init(void)
{
    ble_host_acl_conn_register_user_data(BLE_HOST_READER_USER_ID, &s_host_reader_conn_callbacks);
}
