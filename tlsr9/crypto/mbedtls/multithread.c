/*
 * Copyright (c) 2024 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

K_MUTEX_DEFINE(telink_b9x_ecp_mutex);

void telink_b9x_ecp_lock(void)
{
	(void) k_mutex_lock(&telink_b9x_ecp_mutex, K_FOREVER);
}

void telink_b9x_ecp_unlock(void)
{
	(void) k_mutex_unlock(&telink_b9x_ecp_mutex);
}
