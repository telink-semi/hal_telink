#pragma once


void ble_store_init(void);
void ble_store_deinit(void);
void ble_store_read(const char *key, const void *value_buf, size_t buf_len);
void ble_store_write(const char *key, const void *value_buf, size_t buf_len);
void ble_store_delete(const char *key);

