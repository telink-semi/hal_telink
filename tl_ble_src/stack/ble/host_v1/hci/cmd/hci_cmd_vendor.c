#include <string.h>

#include "common/types.h"

#include "../../inc/ble_host.h"
#include "../../inc/ble_host_sal.h"
#include "../inc/ble_hci.h"
#include "../inc/ble_hci_cmd.h"
#include "../inc/ble_hci_log.h"
#include "inc/hci_cmd_vendor.h"

#include "stack/ble/hci/hci_vendor.h"

struct ble_hci_vendor_cmd {
    uint8_t  data[0];
} __attribute__((packed));

int ble_host_hci_vendor_set_bd_address(const struct ble_hci_ip_rd_bd_addr_rp *p_bd_addr)
{
    uint8_t data[sizeof(struct ble_hci_ip_rd_bd_addr_rp)];

    if (p_bd_addr == NULL) {
        BLE_HOST_HCI_COMMON_CMD_ERROR("set le address null pointer");
        return BLE_HCI_ERR(BLE_HOST_HCI_ERR_NULL_PIONTER);
    }

    struct ble_hci_vendor_cmd *p_vendor_cmd = (struct ble_hci_vendor_cmd *)data;
    memcpy(p_vendor_cmd->data,p_bd_addr->addr,6);

    int rc = ble_host_hci_cmd_tx(BLE_HCI_OP(BLE_HCI_OGF_VENDOR|HCI_VENDOR_CMD_CBC_OPCODE_OGF, HCI_TELINK_SET_BD_ADDR),
                                 data,
                                 sizeof(data),
                                 NULL,
                                 0);

    BLE_HOST_HCI_COMMON_CMD_INFO("set le address return error code 0x%x", rc);

    return rc;
}
