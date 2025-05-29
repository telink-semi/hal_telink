
#include "common/types.h"

#include "../../l2cap/att/inc/ble_att_uuid.h"
#include "../../l2cap/att/inc/ble_att_pdu_format.h"
#include "../../l2cap/att/inc/ble_att_package.h"

int ble_gatts_notify(uint16_t conn_handle, uint16_t attr_handle, const uint8_t *value, uint16_t len)
{
    return ble_host_att_send_handle_value_notification(conn_handle, attr_handle, value, len);
}
