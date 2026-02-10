/*
 * Copyright (c) 2025 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file sha256_tlx_backend.c
 * @brief TLX hardware-accelerated SHA256 backend for mbedTLS
 */

#include <string.h>
#include <mbedtls/sha256.h>
#include <mbedtls/error.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>

#include "hash/hash_portable.h"
#include "hash/sha256.h"

/* Max concurrent SHA256 contexts */
#define MAX_SHA256_CONTEXTS    8

typedef struct {
    mbedtls_sha256_context *owner;
    SHA256_CTX hw_ctx;
    uint32_t iterator[HASH_ITERATOR_MAX_WORD_LEN];
    bool in_use;
    bool started;
} sha256_slot_t;

static sha256_slot_t g_slots[MAX_SHA256_CONTEXTS];

/* Protects slot table + g_hw_active_slot only */
static struct k_spinlock g_slots_lock;

/* Serializes access to the HW SHA engine */
static struct k_mutex g_hw_mutex;

static sha256_slot_t *g_hw_active_slot;

/* ---------- HW context save / restore ---------- */

static void save_hw_state(sha256_slot_t *slot)
{
    if (slot && slot->started) {
        hash_get_iterator((unsigned char *)slot->iterator,
                          HASH_ITERATOR_MAX_WORD_LEN);
    }
}

static void restore_hw_state(sha256_slot_t *slot)
{
    if (slot && slot->started) {
        hash_set_iterator(slot->iterator,
                          HASH_ITERATOR_MAX_WORD_LEN);
    }
}

/*
 * Must be called with g_slots_lock held
 */
static void acquire_hw_locked(sha256_slot_t *slot)
{
    if (g_hw_active_slot == slot) {
        return;
    }

    if (g_hw_active_slot) {
        save_hw_state(g_hw_active_slot);
    }

    restore_hw_state(slot);
    g_hw_active_slot = slot;
}

/* ---------- Slot management ---------- */

static sha256_slot_t *get_slot(mbedtls_sha256_context *ctx, bool allocate)
{
    k_spinlock_key_t key = k_spin_lock(&g_slots_lock);
    sha256_slot_t *free_slot = NULL;

    for (int i = 0; i < MAX_SHA256_CONTEXTS; i++) {
        if (g_slots[i].in_use) {
            if (g_slots[i].owner == ctx) {
                k_spin_unlock(&g_slots_lock, key);
                return &g_slots[i];
            }
        } else if (!free_slot) {
            free_slot = &g_slots[i];
        }
    }

    if (allocate && free_slot) {
        memset(free_slot, 0, sizeof(*free_slot));
        free_slot->in_use = true;
        free_slot->owner = ctx;
        k_spin_unlock(&g_slots_lock, key);
        return free_slot;
    }

    k_spin_unlock(&g_slots_lock, key);
    return NULL;
}

static void release_slot(mbedtls_sha256_context *ctx)
{
    k_spinlock_key_t key = k_spin_lock(&g_slots_lock);

    for (int i = 0; i < MAX_SHA256_CONTEXTS; i++) {
        if (g_slots[i].in_use && g_slots[i].owner == ctx) {
            if (g_hw_active_slot == &g_slots[i]) {
                g_hw_active_slot = NULL;
            }
            memset(&g_slots[i], 0, sizeof(g_slots[i]));
            break;
        }
    }

    k_spin_unlock(&g_slots_lock, key);
}

/* ---------- mbedTLS backend API ---------- */

void telink_tlx_sha256_init(mbedtls_sha256_context *ctx)
{
    release_slot(ctx);
}

int telink_tlx_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
    if (is224) {
        return MBEDTLS_ERR_SHA256_BAD_INPUT_DATA;
    }

    sha256_slot_t *slot = get_slot(ctx, false);
    if (!slot) {
        slot = get_slot(ctx, true);
        if (!slot) {
            return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
        }
    }

    /* Serialize HW access */
    k_mutex_lock(&g_hw_mutex, K_FOREVER);

    hash_dig_en();

    if (sha256_init(&slot->hw_ctx) != 0) {
        k_mutex_unlock(&g_hw_mutex);
        release_slot(ctx);
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

    slot->started = true;

    /* Switch HW context */
    k_spinlock_key_t key = k_spin_lock(&g_slots_lock);
    acquire_hw_locked(slot);
    k_spin_unlock(&g_slots_lock, key);

    k_mutex_unlock(&g_hw_mutex);
    return 0;
}

int telink_tlx_sha256_update(mbedtls_sha256_context *ctx,
                            const unsigned char *input,
                            size_t ilen)
{
    if (ilen == 0) {
        return 0;
    }

    sha256_slot_t *slot = get_slot(ctx, false);
    if (!slot || !slot->started) {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

    k_mutex_lock(&g_hw_mutex, K_FOREVER);

    k_spinlock_key_t key = k_spin_lock(&g_slots_lock);
    acquire_hw_locked(slot);
    k_spin_unlock(&g_slots_lock, key);

    int rc = sha256_update(&slot->hw_ctx,
                           (unsigned char *)input,
                           ilen);

    key = k_spin_lock(&g_slots_lock);
    save_hw_state(slot);
    k_spin_unlock(&g_slots_lock, key);

    k_mutex_unlock(&g_hw_mutex);

    return rc ? MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED : 0;
}

int telink_tlx_sha256_finish(mbedtls_sha256_context *ctx,
                            unsigned char output[32])
{
    sha256_slot_t *slot = get_slot(ctx, false);
    if (!slot || !slot->started) {
        return MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED;
    }

    k_mutex_lock(&g_hw_mutex, K_FOREVER);

    k_spinlock_key_t key = k_spin_lock(&g_slots_lock);
    acquire_hw_locked(slot);
    k_spin_unlock(&g_slots_lock, key);

    int rc = sha256_final(&slot->hw_ctx, output);

    key = k_spin_lock(&g_slots_lock);
    slot->started = false;
    if (g_hw_active_slot == slot) {
        g_hw_active_slot = NULL;
    }
    k_spin_unlock(&g_slots_lock, key);

    k_mutex_unlock(&g_hw_mutex);

    return rc ? MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED : 0;
}

void telink_tlx_sha256_free(mbedtls_sha256_context *ctx)
{
    release_slot(ctx);
}

void telink_tlx_sha256_clone(mbedtls_sha256_context *dst,
                            const mbedtls_sha256_context *src)
{
    sha256_slot_t *src_slot =
        get_slot((mbedtls_sha256_context *)src, false);
    if (!src_slot || !src_slot->started) {
        return;
    }

    sha256_slot_t *dst_slot = get_slot(dst, true);
    if (!dst_slot) {
        return;
    }

    k_mutex_lock(&g_hw_mutex, K_FOREVER);

    k_spinlock_key_t key = k_spin_lock(&g_slots_lock);
    if (g_hw_active_slot == src_slot) {
        save_hw_state(src_slot);
    }

    memcpy(&dst_slot->hw_ctx,
           &src_slot->hw_ctx,
           sizeof(SHA256_CTX));
    memcpy(dst_slot->iterator,
           src_slot->iterator,
           sizeof(dst_slot->iterator));
    dst_slot->started = true;
    k_spin_unlock(&g_slots_lock, key);

    k_mutex_unlock(&g_hw_mutex);
}
