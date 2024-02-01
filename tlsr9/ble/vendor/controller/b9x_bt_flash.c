/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include "b9x_bt_flash.h"

#if CONFIG_SOC_RISCV_TELINK_B91
#include "stack/ble/B91/ble_common.h"
#elif CONFIG_SOC_RISCV_TELINK_B92
#include "stack/ble/B92/ble_common.h"
#elif CONFIG_SOC_RISCV_TELINK_B95
#include "stack/ble/B95/ble_common.h"
#endif

#include "flash.h"
#include "string.h"
#include <zephyr/storage/flash_map.h>
#include <zephyr/drivers/flash.h>

#if CONFIG_B9X_BLE_CTRL_MAC_TYPE_PUBLIC || CONFIG_B9X_BLE_CTRL_MAC_FLASH
static const struct device *flash_device =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
#endif

static int get_bytes_from_str(uint8_t *buf, int buf_len, const char *src)
{
	unsigned int i;

	for (i = 0U; i < strlen(src); i++) {
		if (!(src[i] >= '0' && src[i] <= '9') &&
		    !(src[i] >= 'A' && src[i] <= 'F') &&
		    !(src[i] >= 'a' && src[i] <= 'f') &&
		    src[i] != ':') {
			return -EINVAL;
		}
	}

	(void)memset(buf, 0, buf_len);

	for (i = 0U; i < buf_len; i++) {
		uint8_t temp;

		char2hex(*src, &temp);
		buf[i] = temp;
		char2hex(*(src + 1), &temp);
		buf[i] |= temp << 4;
		src = src + 3;
	}

	return 0;
}

/**
 * @brief		This function is used to initialize the MAC address
 * @param[in]	bt_mac - BT MAC address
 * @return      Status - 0: command succeeded
 */
_attribute_no_inline_ int b9x_bt_blc_mac_init(uint8_t *bt_mac)
{
	int err = 0;
#if CONFIG_B9X_BLE_CTRL_MAC_TYPE_PUBLIC
	uint8_t dummy_mac[BLE_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	err = flash_read(flash_device, FIXED_PARTITION_OFFSET(vendor_partition)
			+ B9X_BT_MAC_ADDR_OFFSET, bt_mac, BLE_ADDR_LEN);
	if (err < 0)
		return err;

	if (memcmp(bt_mac, dummy_mac, BLE_ADDR_LEN))
		return 0;

	err = get_bytes_from_str(bt_mac, BLE_ADDR_LEN,
			CONFIG_B9X_BLE_PUBLIC_MAC_ADDR);
	if (err)
		return err;

#elif CONFIG_B9X_BLE_CTRL_MAC_TYPE_RANDOM_STATIC
#if CONFIG_B9X_BLE_CTRL_MAC_FLASH
	uint8_t temp_mac[BLE_ADDR_LEN + 3];

	err = flash_read(flash_device, FIXED_PARTITION_OFFSET(vendor_partition)
			+ B9X_BT_MAC_ADDR_OFFSET, temp_mac, BLE_ADDR_LEN + 3);
	if (err < 0)
		return err;

	if (temp_mac[8] == 0) {
		memcpy(bt_mac, temp_mac, BLE_ADDR_LEN);
		return 0;
	}
	generateRandomNum(BLE_ADDR_LEN, bt_mac);
	/* The random static address will be generated,
	 * if stored random static MAC is empty
	 */
	bt_mac[5] |= 0xC0; /* random static by default */
	memcpy(temp_mac, bt_mac, BLE_ADDR_LEN);
	temp_mac[8] = 0;
	err = flash_write(flash_device, FIXED_PARTITION_OFFSET(vendor_partition)
			+ B9X_BT_MAC_ADDR_OFFSET, temp_mac, BLE_ADDR_LEN + 3);
#else
	generateRandomNum(BLE_ADDR_LEN, bt_mac);
	/* The random static address will be generated on every reboot */
	bt_mac[5] |= 0xC0; /* random static by default */
#endif
#else
#error "Other address types are not supported or need to be set via HCI"
#endif
	return err;
}
