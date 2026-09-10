/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

K_MUTEX_DEFINE(telink_tlx_soc_sha_mutex);

void telink_tlx_soc_sha_lock(void)
{
	(void)k_mutex_lock(&telink_tlx_soc_sha_mutex, K_FOREVER);
}

void telink_tlx_soc_sha_unlock(void)
{
	(void)k_mutex_unlock(&telink_tlx_soc_sha_mutex);
}
