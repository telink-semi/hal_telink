/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/init.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sha_tlx, CONFIG_TL_TELINK_CRYPTO_BACKEND_LOG_LEVEL);

#include <hash/hash.h>
#include <hash/hash_portable.h>

extern void telink_tlx_soc_sha_lock(void);
extern void telink_tlx_soc_sha_unlock(void);

static struct {
	void *lib_ctx;
	HASH_CTX hw_ctx;
} telink_tlx_sha_data[CONFIG_TL_TELINK_TLX_SHA_HW_SLOTS];

static int telink_tlx_sha_get_slot(const void *ctx, bool exist)
{
	int result = -1;

	for (size_t i = 0; i < ARRAY_SIZE(telink_tlx_sha_data); ++i) {
		if ((exist && telink_tlx_sha_data[i].lib_ctx == ctx) ||
		    (!exist && telink_tlx_sha_data[i].lib_ctx == NULL)) {
			result = i;
			break;
		}
	}
	return result;
}

int telink_tlx_sha_init(void *ctx, HASH_ALG hash_alg)
{
	int result = -1;

	telink_tlx_soc_sha_lock();
	int id = telink_tlx_sha_get_slot(ctx, false);

	if (id >= 0) {
		if (!hash_init(&telink_tlx_sha_data[id].hw_ctx, hash_alg)) {
			telink_tlx_sha_data[id].lib_ctx = ctx;
			result = 0;
		}
	}
	telink_tlx_soc_sha_unlock();
	if (result) {
		LOG_DBG("telink SHA acceleration failed failed (%s)", __func__);
	}
	return result;
}

int telink_tlx_sha_update(void *ctx, const void *data, size_t data_len)
{
	int result = -1;

	telink_tlx_soc_sha_lock();
	int id = telink_tlx_sha_get_slot(ctx, true);

	if (id >= 0) {
		if (!hash_update(&telink_tlx_sha_data[id].hw_ctx, data, data_len)) {
			result = 0;
		}
	}
	telink_tlx_soc_sha_unlock();
	if (result) {
		LOG_DBG("telink SHA acceleration failed failed (%s)", __func__);
	}
	return result;
}

int telink_tlx_sha_final(void *ctx, uint8_t *digest)
{
	int result = -1;

	telink_tlx_soc_sha_lock();
	int id = telink_tlx_sha_get_slot(ctx, true);

	if (id >= 0) {
		if (!hash_final(&telink_tlx_sha_data[id].hw_ctx, digest)) {
			telink_tlx_sha_data[id].lib_ctx = NULL;
			result = 0;
		}
	}
	telink_tlx_soc_sha_unlock();
	if (result) {
		LOG_DBG("telink SHA acceleration failed failed (%s)", __func__);
	}
	return result;
}

int telink_tlx_sha_clone(void *dst, const void *src)
{
	int result = -1;

	telink_tlx_soc_sha_lock();
	int src_id = telink_tlx_sha_get_slot(src, true);
	int dst_id = telink_tlx_sha_get_slot(dst, false);

	if (src_id >= 0 && dst_id >= 0) {
		telink_tlx_sha_data[dst_id].hw_ctx = telink_tlx_sha_data[src_id].hw_ctx;
		telink_tlx_sha_data[dst_id].lib_ctx = dst;
		result = 0;
	}
	telink_tlx_soc_sha_unlock();
	if (result) {
		LOG_DBG("telink SHA acceleration failed failed (%s)", __func__);
	}
	return result;
}

void telink_tlx_sha_cleanup(void *ctx)
{
	telink_tlx_soc_sha_lock();
	int id = telink_tlx_sha_get_slot(ctx, true);

	if (id >= 0) {
		telink_tlx_sha_data[id].lib_ctx = NULL;
	}
	telink_tlx_soc_sha_unlock();
}

static void telink_tlx_sha_pm_cb(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		hash_dig_en();
	}
}

static int telink_tlx_sha_startup(void)
{
	static struct pm_notifier telink_tlx_sha_pm_notifier = {
		.state_exit = telink_tlx_sha_pm_cb,
	};

	pm_notifier_register(&telink_tlx_sha_pm_notifier);
	hash_dig_en();
	LOG_DBG("telink SHA acceleration inited");
	return 0;
}

SYS_INIT(telink_tlx_sha_startup, POST_KERNEL, 0);
