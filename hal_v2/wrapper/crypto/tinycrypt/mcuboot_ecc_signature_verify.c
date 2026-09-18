/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/ecc_dsa.h>
#include <lib/include/pke/ecdsa.h>

extern int __real_uECC_verify(const uint8_t *public_key, const uint8_t *message_hash,
                 unsigned hash_size, const uint8_t *signature, uECC_Curve curve);

int __wrap_uECC_verify(const uint8_t *public_key, const uint8_t *message_hash,
                 unsigned hash_size, const uint8_t *signature, uECC_Curve curve)
{
    /* Current implementation only supports secp256r1 and SHA256 */
    if(curve == uECC_secp256r1() && hash_size == 32U) {

        if(public_key == NULL || message_hash == NULL || signature == NULL) {
        return TC_CRYPTO_FAIL;
    }

        unsigned int ret = ecdsa_verify(secp256r1, message_hash, hash_size,
                                        public_key, signature);

        /* SDK success is 0; TinyCrypt success is 1. */
        if(ret == ECDSA_SUCCESS) {
            return TC_CRYPTO_SUCCESS;
        }
    }

    return __real_uECC_verify(public_key, message_hash, hash_size, signature, curve);
}