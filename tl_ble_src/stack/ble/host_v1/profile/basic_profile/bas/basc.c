#include <string.h>

#include "common/types.h"
#include "common/utility.h"
#include "common/bit.h"

#include "../../inc/profile.h"
#include "../../inc/profile_internal.h"

#include "../../../gatt/gattc/inc/gattc.h"

#include "../inc/prf_basic.h"

#include "inc/bas.h"
#include "inc/basc.h"

#include "inc/basc_internal.h"

// SSDP module
#include "../../../l2cap/att/inc/ble_att_uuid.h"
#include "../../../gatt/sdp/inc/ble_ssdp.h"

// ATT UUID define 
#include "../../../l2cap/att/inc/uuid_def.h"

// LOG module
#include "../../../inc/ble_host_sal.h"
#include "../inc/prf_basic_log.h"

// BLE Host
#include "../../../inc/ble_host.h"

#define BLE_BAS_CLIENT_MALLOC(size)         BLE_PRF_MALLOC_SPEC_PRF_CLIENT(size, SERVICE_ID_BAS)
#define BLE_BAS_CLIENT_FREE(ptr)            ble_prf_free(ptr)


BLE_FUNC_DEFINE_INIT_CONNECT_DISC(bas)

static const struct ble_prf_param s_bas_client_params = {
    .client = 1,
    .used_acl_role = PRF_USED_ACL_ROLE_CONNECT,
    .service_id = SERVICE_ID_BAS,
    .sec_flag = 0,
    .init = ble_basc_init,
    .connect = ble_basc_connect,
    .discovery = ble_basc_discovery,
};

static struct ble_bas_client_control s_bas_client_ctrl = {
    .prf_process = {
        .prf_params = &s_bas_client_params,
    },
};

void ble_basic_register_BAS_control_client(const struct ble_basc_register_param *param)
{
    blc_prf_register_service_module(&s_bas_client_ctrl.prf_process, param);
}

BLE_FUNC_PRF_CLIENT_INIT_CONNECT_DISC(bas, BAS)

static void ble_basc_report_event(uint16_t conn_handle, enum ble_basc_event_id event_id, const void *event_msg)
{
    ble_basc_event_callback p_callback = s_bas_client_ctrl.prf_process.event_cb;
    if (p_callback != NULL) {
        p_callback(conn_handle, event_id, event_msg);
    }
}

static void ble_basc_data_input(uint16_t conn_handle, struct ble_gattc_ccc_value *ntf_value)
{
    BLE_BAS_DEBUG("Data input, acl:0x%03x, handle:0x%04x, value:%s", conn_handle, ntf_value->attr_handle,
        hex_to_str(ntf_value->value, ntf_value->value_length));

    struct ble_bas_client *p_bas_client = ble_basc_get_client_context(conn_handle);
    if (p_bas_client == NULL) {
        return;
    }

    uint16_t attr_handle = ntf_value->attr_handle;

    if (attr_handle == p_bas_client->battery_level_handle) {
        uint8_t battery_level = ntf_value->value[0];
        p_bas_client->battery_level = battery_level;
        ble_basc_report_event(conn_handle, BASC_EVT_ID_BATTERY_LEVEL_UPDATE, &battery_level);
    } else if (attr_handle == p_bas_client->battery_power_state_handle) {
        uint8_t battery_power_state = ntf_value->value[0];
        p_bas_client->battery_power_state = battery_power_state;
        ble_basc_report_event(conn_handle, BASC_EVT_ID_BATTERY_POWER_STATE_UPDATE, &battery_power_state);
    }
}

static void ble_basc_display_information(struct ble_bas_client *p_bas_client)
{
    BLE_BAS_DEBUG("BAS sdp over, acl:0x%03x, Battery Level[Hdl:0x%x, val:%d], power state[Hdl:0x%x, val:%d]",
        p_bas_client->conn_handle, p_bas_client->battery_level_handle, p_bas_client->battery_level,
        p_bas_client->battery_power_state_handle, p_bas_client->battery_power_state);
}

BLE_FUNC_PRF_SDP_DISCOVERY_SERVICE(bas, BAS)
BLE_FUNC_BAS_DISC_FOUND_CHAR(battery_level)
BLE_FUNC_BAS_DISC_START_READ_FIX_LEN(battery_level)
BLE_FUNC_BAS_DISC_FOUND_CHAR(battery_power_state)
BLE_FUNC_BAS_DISC_START_READ_FIX_LEN(battery_power_state)

BLE_SSDP_CHARACTERISTIC_NAME(bas) = {
    BLE_BAS_DISC_READ_NOTIFY_CHAR(characteristicBatteryLevelAttUuid, battery_level),
    BLE_BAS_DISC_READ_NOTIFY_CHAR(characteristicBatteryPowerStateAttUuid, battery_power_state)
};

BLE_PRF_DEFINE_SSDP_NO_INCLUDE_LIST(bas, serviceBatteryAttUuid);

int ble_basc_read_battery_level(uint16_t conn_handle, prf_read_callback callback)
{
    BLE_BAS_READ_ATTR_VALUE_FIX_LEN(battery_level);
}

int ble_basc_read_power_sate(uint16_t conn_handle, prf_read_callback callback)
{
    BLE_BAS_READ_ATTR_VALUE_FIX_LEN(battery_power_state);
}

int ble_basc_get_battery_level(uint16_t conn_handle, uint8_t *battery_level)
{
    BLE_BAS_GET_ATTR_VALUE_FIX_LEN(battery_level);
}

int ble_basc_get_power_state(uint16_t conn_handle, uint8_t *battery_power_state)
{
    BLE_BAS_GET_ATTR_VALUE_FIX_LEN(battery_power_state);
}
