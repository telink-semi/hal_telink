
#pragma once

void ble_host_write_hci_tx_packet_to_btsnoop(const uint8_t *packet, uint16_t length);

void ble_host_write_hci_rx_packet_to_btsnoop(const uint8_t *packet, uint16_t length);
