#pragma once

#define BLE_HCI_OCF_VENDOR_CMD (0x0050)
#define BLE_HCI_SUB_CMD_LEN (1)

enum BLE_HCI_VENDOR_SUB_CMD
{
    BLE_HCI_SUB_CMD_SET_BD_ADDR = 0x01,
}__attribute__((packed));

int ble_host_hci_vendor_set_bd_address(const struct ble_hci_ip_rd_bd_addr_rp *p_bd_addr);