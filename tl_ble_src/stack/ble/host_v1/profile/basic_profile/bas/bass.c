
#include "common/types.h"

#include "../../../inc/ble_host.h"
#include "../../../inc/ble_host_sal.h"
#include "../inc/prf_basic_log.h"

#include "../../../gatt/gatts/inc/gatts_sdp.h"
#include "../../../gatt/gatts/inc/gatts_req.h"

#include "../../../l2cap/att/inc/ble_att.h"
#include "../../../l2cap/att/inc/ble_att_service.h"
#include "../../../l2cap/att/inc/uuid_def.h"
#include "../../../services/svc_gatt/bas/svc_battery.h"

#include "../../inc/profile.h"
#include "../../inc/profile_internal.h"

#include "../inc/prf_basic.h"

#include "inc/bas.h"
#include "inc/bass.h"

#include "inc/bass_internal.h"

static void ble_bass_init(enum prf_process_type type, const void *param);
static int ble_bass_read_callback(uint16_t conn_handle, uint8_t opcode, uint16_t attr_handle, uint8_t **out_value, uint16_t *out_value_len);
static void ble_bass_server_init_param(const struct ble_bass_register_param *param);

static const struct ble_prf_param s_bas_server_param = {
    .client = 0,
    .used_acl_role = PRF_USED_ACL_ROLE_CONNECT,
    .service_id = SERVICE_ID_BAS,
    .sec_flag = 0,
    .init = ble_bass_init,
};

static struct ble_bas_server_control s_bas_server_ctrl = {
    .prf_process = {
        .prf_params = &s_bas_server_param,
    },
};

void ble_basic_register_BAS_control_server(const struct ble_bass_register_param *param)
{
    blc_prf_register_service_module(&s_bas_server_ctrl.prf_process, param);
}

static void ble_bass_init(enum prf_process_type type, const void *param)
{
    if (type == PRF_PROCESS_INIT) {
        BLE_BAS_INFO("server initialization");
        blc_svc_addBasGroup();
        blc_svc_basCbackRegister(ble_bass_read_callback);
        ble_bass_server_init_param(param);
    }
}

static struct ble_bas_server *ble_bass_get_server_env(void)
{
    return &s_bas_server_ctrl.bas_server;
}

static int ble_bass_read_callback(uint16_t conn_handle, uint8_t opcode, uint16_t attr_handle, uint8_t **out_value, uint16_t *out_value_len)
{
    (void) conn_handle;
    (void) opcode;

    struct ble_bas_server *server = ble_bass_get_server_env();

    if (attr_handle == server->battery_level_handle) {
        *out_value = &server->battery_level;
        *out_value_len = sizeof(server->battery_level);
    } else if (attr_handle == server->battery_power_state_handle) {
        *out_value = &server->battery_power_state;
        *out_value_len = sizeof(server->battery_power_state);
    }

    return ATT_SUCCESS;
}

BLE_BAS_SERVER_INIT_HANDLE(battery_level)
BLE_BAS_SERVER_INIT_HANDLE(battery_power_state)

static const struct gatts_discover_char_info bass_char[] = {
    BLE_BAS_SERVER_FIND_CHAR(battery_level, characteristicBatteryLevelAttUuid),
    BLE_BAS_SERVER_FIND_CHAR(battery_power_state, characteristicBatteryPowerStateAttUuid),
    GATTS_DISCOVER_CHAR_END,
};

static const struct ble_bass_register_param default_bass_param = {
    .battery_level = 100,
    .power_state = DEVICE_NO_CHARGING,
};

static void ble_bass_server_init_param(const struct ble_bass_register_param *param)
{
    struct ble_bas_server *server = ble_bass_get_server_env();
    ble_gatts_discover_by_service_uuid(&serviceBatteryAttUuid, bass_char, server);

    const struct ble_bass_register_param *bass_param = param != NULL ? param : &default_bass_param;

    ble_bass_set_battery_level(bass_param->battery_level);
    ble_bass_set_power_state(bass_param->power_state);

    BLE_BAS_DEBUG("Handle Info: level:[Hdl:0x%x, value:%d], power state:[Hdl:0x%x, value:%d]",
        server->battery_level_handle, server->battery_level,
        server->battery_power_state_handle, server->battery_power_state);
}

uint8_t ble_bass_get_battery_level(void)
{
    return ble_bass_get_server_env()->battery_level;
}

uint8_t ble_bass_get_power_state(void)
{
    return ble_bass_get_server_env()->battery_power_state;
}

void ble_bass_set_battery_level(uint8_t level)
{
    struct ble_bas_server *server = ble_bass_get_server_env();
    if (!CHECK_BATTERY_LEVEL(level)) {
        level = BATTERY_MAX_LEVEL;
    }
    server->battery_level = level;
}

void ble_bass_set_power_state(uint8_t power_state)
{
    struct ble_bas_server *server = ble_bass_get_server_env();
    server->battery_power_state = power_state;
}

int ble_bass_update_battery_level(uint16_t conn_handle, uint8_t level)
{
    BLE_BAS_INFO("update battery level, conn_handle:0x%x, level:%d", conn_handle, level);

    struct ble_bas_server *server = ble_bass_get_server_env();
    ble_bass_set_battery_level(level);
    return ble_gatts_notify(conn_handle, server->battery_level_handle, &server->battery_level, 1);
}

int ble_bass_update_power_state(uint16_t conn_handle, uint8_t power_state)
{
    BLE_BAS_INFO("update battery power state, conn_handle:0x%x, power_state:%d", conn_handle, power_state);

    struct ble_bas_server *server = ble_bass_get_server_env();
    server->battery_power_state = power_state;
    return ble_gatts_notify(conn_handle, server->battery_power_state_handle, &power_state, 1);
}
