/*
 * Copyright (c) 2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <stddef.h>
#include <hash/hash.h>

int telink_tlx_sha_init(void *ctx, HASH_ALG hash_alg);
int telink_tlx_sha_update(void *ctx, const void *data, size_t data_len);
int telink_tlx_sha_final(void *ctx, uint8_t *digest);
int telink_tlx_sha_clone(void *dst, const void *src);
void telink_tlx_sha_cleanup(void *ctx);

#if CONFIG_TELINK_TLX_MBEDTLS_HW_ACCELERATION

#include <mbedtls/sha256.h>

int __real_mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224);
int __real_mbedtls_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *input,
				 size_t ilen);
int __real_mbedtls_sha256_finish(mbedtls_sha256_context *ctx, unsigned char *output);
void __real_mbedtls_sha256_clone(mbedtls_sha256_context *dst, const mbedtls_sha256_context *src);
void __real_mbedtls_sha256_free(mbedtls_sha256_context *ctx);

int __wrap_mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
	int ret = telink_tlx_sha_init(ctx, is224 ? HASH_SHA224 : HASH_SHA256);

	if (ret) {
		ret = __real_mbedtls_sha256_starts(ctx, is224);
	}
	return ret;
}

int __wrap_mbedtls_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *input,
				 size_t ilen)
{
	int ret = telink_tlx_sha_update(ctx, input, ilen);

	if (ret) {
		ret = __real_mbedtls_sha256_update(ctx, input, ilen);
	}
	return ret;
}

int __wrap_mbedtls_sha256_finish(mbedtls_sha256_context *ctx, unsigned char *output)
{
	int ret = telink_tlx_sha_final(ctx, output);

	if (ret) {
		ret = __real_mbedtls_sha256_finish(ctx, output);
	}
	return ret;
}

int __wrap_mbedtls_sha256(const unsigned char *input, size_t ilen, unsigned char *output, int is224)
{
	int ret;
	mbedtls_sha256_context ctx;

	mbedtls_sha256_init(&ctx);
	do {
		ret = __wrap_mbedtls_sha256_starts(&ctx, is224);
		if (ret) {
			break;
		}
		ret = __wrap_mbedtls_sha256_update(&ctx, input, ilen);
		if (ret) {
			break;
		}
		ret = __wrap_mbedtls_sha256_finish(&ctx, output);
	} while (0);
	mbedtls_sha256_free(&ctx);
	return ret;
}

void __wrap_mbedtls_sha256_clone(mbedtls_sha256_context *dst, const mbedtls_sha256_context *src)
{
	if (telink_tlx_sha_clone(dst, src)) {
		__real_mbedtls_sha256_clone(dst, src);
	}
}

void __wrap_mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
	telink_tlx_sha_cleanup(ctx);
	__real_mbedtls_sha256_free(ctx);
}

#endif /* CONFIG_TELINK_TLX_MBEDTLS_HW_ACCELERATION */

#if CONFIG_TELINK_TLX_TINYCRYPT_HW_ACCELERATION

#include <tinycrypt/sha256.h>

extern int __real_tc_sha256_init(TCSha256State_t s);
extern int __real_tc_sha256_update(TCSha256State_t s, const uint8_t *data, size_t datalen);
extern int __real_tc_sha256_final(uint8_t *digest, TCSha256State_t s);

int __wrap_tc_sha256_init(TCSha256State_t s)
{
	int ret = telink_tlx_sha_init(s, HASH_SHA256);

	if (ret) {
		ret = __real_tc_sha256_init(s);
	}
	return ret;
}

int __wrap_tc_sha256_update(TCSha256State_t s, const uint8_t *data, size_t datalen)
{
	int ret = telink_tlx_sha_update(s, data, datalen);

	if (ret) {
		ret = __real_tc_sha256_update(s, data, datalen);
	}
	return ret;
}

int __wrap_tc_sha256_final(uint8_t *digest, TCSha256State_t s)
{
	int ret = telink_tlx_sha_final(s, digest);

	if (ret) {
		ret = __real_tc_sha256_final(digest, s);
	}
	return ret;
}

#endif /* CONFIG_TELINK_TLX_TINYCRYPT_HW_ACCELERATION */
