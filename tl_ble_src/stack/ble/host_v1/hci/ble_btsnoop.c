#include <string.h>

#include "common/types.h"
#include "common/utility.h"

#include "../inc/ble_host_sal.h"

#ifndef BLE_HOST_BTSNOOP_LOG_ENABLE
    #define BLE_HOST_BTSNOOP_LOG_ENABLE 0
#endif

#if BLE_HOST_BTSNOOP_LOG_ENABLE

extern void tlkdbg_send_str_data(char *str, u8 *pData, u32 data_len);

static void ble_host_write_to_btsnoop(char *header, const uint8_t *data, uint16_t length)
{
    tlkdbg_send_str_data(header, (u8 *)(size_t)data, length);
    tlkdbg_send_str_data("[BTSNOOP] end of packet.", NULL, 0);
}

void ble_host_write_hci_tx_packet_to_btsnoop(const uint8_t *packet, uint16_t length)
{
    ble_host_write_to_btsnoop("[BTSNOOP] HCI TX Packet", packet, length);
}

void ble_host_write_hci_rx_packet_to_btsnoop(const uint8_t *packet, uint16_t length)
{
    ble_host_write_to_btsnoop("[BTSNOOP] HCI RX Packet", packet, length);
}

#else

void ble_host_write_hci_tx_packet_to_btsnoop(const uint8_t *packet, uint16_t length)
{
    (void)packet;
    (void)length;
}

void ble_host_write_hci_rx_packet_to_btsnoop(const uint8_t *packet, uint16_t length)
{
    (void)packet;
    (void)length;
}

#endif
