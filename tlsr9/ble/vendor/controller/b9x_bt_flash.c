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
#endif

#include "flash.h"
#include "string.h"
#include <zephyr/storage/flash_map.h>

/**
 * @brief		This function is used to initialize the MAC address
 * @param[in]	bt_mac - BT MAC address
 * @return      none
 */
_attribute_no_inline_ void b9x_bt_blc_mac_init(uint8_t *bt_mac)
{
	uint8_t dummy_mac[BLE_ADDR_LEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

	flash_read_page(FIXED_PARTITION_OFFSET(vendor_partition) +
	B9X_BT_MAC_ADDR_OFFSET, BLE_ADDR_LEN, bt_mac);
	if (!memcmp(bt_mac, dummy_mac, BLE_ADDR_LEN))
    {
		generateRandomNum(BLE_ADDR_LEN, bt_mac);
#if CONFIG_B9X_BLE_CTRL_MAC_TYPE_PUBLIC && CONFIG_B9X_BLE_CTRL_MAC_PUBLIC_DEBUG
	/* Debug purpose only. Public address must be set by vendor only */
		bt_mac[3] = 0x38; /* company id: 0xA4C138 */
 		bt_mac[4] = 0xC1;
 		bt_mac[5] = 0xA4;
#elif CONFIG_B9X_BLE_CTRL_MAC_TYPE_RANDOM_STATIC || CONFIG_B9X_BLE_CTRL_MAC_TYPE_PUBLIC
	/* Enters this condition, when configured public address is empty */
	/* Generally we are not allowed to generate public address in mass production device */
	/* The random static address will be generated, if stored public MAC is empty */
		bt_mac[5] |= 0xC0; /* random static by default */
#else
	#error "Other address types are not supported or need to be set via HCI"
#endif
		flash_write_page(FIXED_PARTITION_OFFSET(vendor_partition) +
		B9X_BT_MAC_ADDR_OFFSET, BLE_ADDR_LEN, bt_mac);
	}
}
