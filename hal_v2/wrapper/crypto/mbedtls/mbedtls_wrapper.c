/*
 * Copyright (c) 2024-2026 Telink Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mbedtls/build_info.h>
#include <mbedtls/error.h>

#ifdef MBEDTLS_ECP_C

#include <mbedtls/ecp.h>

/*********************************************************************
 * ECP HW accelerated functions
 *********************************************************************/

#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX || CONFIG_SOC_RISCV_TELINK_TLX
extern int telink_tlx_ecp_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt);
extern int telink_tlx_ecp_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx);
extern int telink_tlx_ecp_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx);
#elif CONFIG_SOC_SERIES_RISCV_TELINK_B9X || CONFIG_SOC_RISCV_TELINK_B9X
extern int telink_b9x_ecp_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt);
extern int telink_b9x_ecp_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx);
extern int telink_b9x_ecp_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx);
#endif
#ifdef MBEDTLS_SELF_TEST
extern int telink_soc_ecp_self_test(int verbose);
#endif /* MBEDTLS_SELF_TEST */

#endif /* MBEDTLS_ECP_C */

/*********************************************************************
 * SHA256 HW accelerated functions
 *********************************************************************/

#ifdef MBEDTLS_SHA256_C

#include <mbedtls/sha256.h>

#if CONFIG_TELINK_TLX_MBEDTLS_HW_SHA_ACCELERATION
extern void telink_tlx_sha256_init(mbedtls_sha256_context *ctx);
extern int telink_tlx_sha256_starts(mbedtls_sha256_context *ctx, int is224);
extern int telink_tlx_sha256_update(mbedtls_sha256_context *ctx,
                                     const unsigned char *input,
                                     size_t ilen);
extern int telink_tlx_sha256_finish(mbedtls_sha256_context *ctx,
                                     unsigned char output[32]);
extern void telink_tlx_sha256_free(mbedtls_sha256_context *ctx);

extern void telink_tlx_sha256_clone(mbedtls_sha256_context *dst,
                                  const mbedtls_sha256_context *src);
#endif /* CONFIG_TELINK_TLX_MBEDTLS_HW_SHA_ACCELERATION */

#endif /* MBEDTLS_SHA256_C */

/*********************************************************************
 * LD transformed software functions
 *********************************************************************/

#ifdef MBEDTLS_ECP_C
extern int __real_mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt);
extern int __real_mbedtls_ecp_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx);
extern int __real_mbedtls_ecp_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx);
#ifdef MBEDTLS_SELF_TEST
extern int __real_mbedtls_ecp_self_test(int verbose);
#endif /* MBEDTLS_SELF_TEST */
#endif /* MBEDTLS_ECP_C */

/*********************************************************************
 * Call HW accelerated functionality if fails use software
 *********************************************************************/

#ifdef MBEDTLS_ECP_C

int __wrap_mbedtls_ecp_check_pubkey(const mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *pt)
{
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX || CONFIG_SOC_RISCV_TELINK_TLX
	int result = telink_tlx_ecp_check_pubkey(grp, pt);
#elif CONFIG_SOC_SERIES_RISCV_TELINK_B9X || CONFIG_SOC_RISCV_TELINK_B9X
	int result = telink_b9x_ecp_check_pubkey(grp, pt);
#endif

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_check_pubkey(grp, pt);
	}
	return result;
}

int __wrap_mbedtls_ecp_mul_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng, mbedtls_ecp_restart_ctx *rs_ctx)
{
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX || CONFIG_SOC_RISCV_TELINK_TLX
	int result = telink_tlx_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, rs_ctx);
#elif CONFIG_SOC_SERIES_RISCV_TELINK_B9X || CONFIG_SOC_RISCV_TELINK_B9X
	int result = telink_b9x_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, rs_ctx);
#endif

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, rs_ctx);
	}
	return result;
}

int __wrap_mbedtls_ecp_mul(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R, const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
	return __wrap_mbedtls_ecp_mul_restartable(grp, R, m, P, f_rng, p_rng, NULL);
}

int __wrap_mbedtls_ecp_muladd_restartable(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q,
	mbedtls_ecp_restart_ctx *rs_ctx)
{
#if CONFIG_SOC_SERIES_RISCV_TELINK_TLX || CONFIG_SOC_RISCV_TELINK_TLX
	int result = telink_tlx_ecp_muladd_restartable(grp, R, m, P, n, Q, rs_ctx);
#elif CONFIG_SOC_SERIES_RISCV_TELINK_B9X || CONFIG_SOC_RISCV_TELINK_B9X
	int result = telink_b9x_ecp_muladd_restartable(grp, R, m, P, n, Q, rs_ctx);
#endif

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_muladd_restartable(grp, R, m, P, n, Q, rs_ctx);
	}
	return result;
}

int __wrap_mbedtls_ecp_muladd(mbedtls_ecp_group *grp,
	mbedtls_ecp_point *R,
	const mbedtls_mpi *m, const mbedtls_ecp_point *P,
	const mbedtls_mpi *n, const mbedtls_ecp_point *Q)
{
	return __wrap_mbedtls_ecp_muladd_restartable(grp, R, m, P, n, Q, NULL);
}

int __wrap_mbedtls_ecp_gen_keypair_base(mbedtls_ecp_group *grp,
	const mbedtls_ecp_point *G,
	mbedtls_mpi *d, mbedtls_ecp_point *Q,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng)
{
	int ret = mbedtls_ecp_gen_privkey(grp, d, f_rng, p_rng);

	if (!ret) {
		__wrap_mbedtls_ecp_mul(grp, Q, d, G, f_rng, p_rng);
	}

	return ret;
}

int __wrap_mbedtls_ecp_gen_keypair(mbedtls_ecp_group *grp,
	mbedtls_mpi *d, mbedtls_ecp_point *Q,
	int (*f_rng)(void *, unsigned char *, size_t),
	void *p_rng)
{
	return __wrap_mbedtls_ecp_gen_keypair_base(grp, &grp->G, d, Q, f_rng, p_rng);
}

int __wrap_mbedtls_ecp_gen_key(mbedtls_ecp_group_id grp_id, mbedtls_ecp_keypair *key,
	int (*f_rng)(void *, unsigned char *, size_t), void *p_rng)
{
	int ret = MBEDTLS_ERR_ERROR_CORRUPTION_DETECTED;

	ret = mbedtls_ecp_group_load(&key->MBEDTLS_PRIVATE(grp), grp_id);
	if (ret != 0) {
		return ret;
	}

	return __wrap_mbedtls_ecp_gen_keypair(&key->MBEDTLS_PRIVATE(grp), &key->MBEDTLS_PRIVATE(d),
		&key->MBEDTLS_PRIVATE(Q), f_rng, p_rng);
}

#ifdef MBEDTLS_SELF_TEST
int __wrap_mbedtls_ecp_self_test(int verbose)
{
	int result = telink_soc_ecp_self_test(verbose);

	if (result == MBEDTLS_ERR_PLATFORM_HW_ACCEL_FAILED) {
		result = __real_mbedtls_ecp_self_test(verbose);
	}
	return result;
}
#endif /* MBEDTLS_SELF_TEST */

#endif /* MBEDTLS_ECP_C */

/*********************************************************************
 * SHA256 wrapper functions
 *********************************************************************/

#ifdef MBEDTLS_SHA256_C

#ifdef CONFIG_TELINK_TLX_MBEDTLS_HW_SHA_ACCELERATION
void __wrap_mbedtls_sha256_init(mbedtls_sha256_context *ctx)
{
	telink_tlx_sha256_init(ctx);
}

int __wrap_mbedtls_sha256_starts(mbedtls_sha256_context *ctx, int is224)
{
	return telink_tlx_sha256_starts(ctx, is224);
}

int __wrap_mbedtls_sha256_update(mbedtls_sha256_context *ctx,
                                  const unsigned char *input,
                                  size_t ilen)
{
	return telink_tlx_sha256_update(ctx, input, ilen);;
}

int __wrap_mbedtls_sha256_finish(mbedtls_sha256_context *ctx,
                                  unsigned char output[32])
{
	return telink_tlx_sha256_finish(ctx, output);
}

void __wrap_mbedtls_sha256_free(mbedtls_sha256_context *ctx)
{
	telink_tlx_sha256_free(ctx);
}

void __wrap_mbedtls_sha256_clone(mbedtls_sha256_context *dst,
                                  const mbedtls_sha256_context *src)
{
    telink_tlx_sha256_clone(dst, src);
}

int __wrap_mbedtls_sha256(const unsigned char *input,
                           size_t ilen,
                           unsigned char output[32],
                           int is224)
{
    mbedtls_sha256_context ctx;
    int ret;

    __wrap_mbedtls_sha256_init(&ctx);

    ret = __wrap_mbedtls_sha256_starts(&ctx, is224);
    if (ret != 0) {
        goto exit;
    }

    ret = __wrap_mbedtls_sha256_update(&ctx, input, ilen);
    if (ret != 0) {
        goto exit;
    }

    ret = __wrap_mbedtls_sha256_finish(&ctx, output);

exit:
    __wrap_mbedtls_sha256_free(&ctx);
    return ret;
}

#endif /* CONFIG_TELINK_TLX_MBEDTLS_HW_SHA_ACCELERATION */

#endif /* MBEDTLS_SHA256_C */
