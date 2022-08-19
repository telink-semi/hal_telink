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

#ifndef ECP_ALT_H
#define ECP_ALT_H

#if defined(MBEDTLS_ECP_ALT)

typedef struct mbedtls_ecp_group {
	mbedtls_ecp_group_id id;
	mbedtls_mpi P;
	mbedtls_mpi A;
	mbedtls_mpi B;
	mbedtls_ecp_point G;
	mbedtls_mpi N;
	size_t pbits;
	size_t nbits;
	/* End of public fields */
	unsigned int MBEDTLS_PRIVATE(h);
	int (*MBEDTLS_PRIVATE(modp))(
		mbedtls_mpi *);
	int (*MBEDTLS_PRIVATE(t_pre))(mbedtls_ecp_point *, void *);
	int (*MBEDTLS_PRIVATE(t_post))(mbedtls_ecp_point *, void *);
	void *MBEDTLS_PRIVATE(t_data);
	mbedtls_ecp_point *MBEDTLS_PRIVATE(T);
	size_t MBEDTLS_PRIVATE(T_size);
} mbedtls_ecp_group;

#if !defined(MBEDTLS_ECP_WINDOW_SIZE)
/*
 * Maximum "window" size used for point multiplication.
 * Default: a point where higher memory usage yields disminishing performance
 *          returns.
 * Minimum value: 2. Maximum value: 7.
 *
 * Result is an array of at most ( 1 << ( MBEDTLS_ECP_WINDOW_SIZE - 1 ) )
 * points used for point multiplication. This value is directly tied to EC
 * peak memory usage, so decreasing it by one should roughly cut memory usage
 * by two (if large curves are in use).
 *
 * Reduction in size may reduce speed, but larger curves are impacted first.
 * Sample performances (in ECDHE handshakes/s, with FIXED_POINT_OPTIM = 1):
 *      w-size:     6       5       4       3       2
 *      521       145     141     135     120      97
 *      384       214     209     198     177     146
 *      256       320     320     303     262     226
 *      224       475     475     453     398     342
 *      192       640     640     633     587     476
 */
#define MBEDTLS_ECP_WINDOW_SIZE 4
#endif				  /* MBEDTLS_ECP_WINDOW_SIZE */

#if !defined(MBEDTLS_ECP_FIXED_POINT_OPTIM)
/*
 * Trade code size for speed on fixed-point multiplication.
 *
 * This speeds up repeated multiplication of the generator (that is, the
 * multiplication in ECDSA signatures, and half of the multiplications in
 * ECDSA verification and ECDHE) by a factor roughly 3 to 4.
 *
 * For each n-bit Short Weierstrass curve that is enabled, this adds 4n bytes
 * of code size if n < 384 and 8n otherwise.
 *
 * Change this value to 0 to reduce code size.
 */
#define MBEDTLS_ECP_FIXED_POINT_OPTIM 1
#endif					/* MBEDTLS_ECP_FIXED_POINT_OPTIM */

/* \} name SECTION: Module settings */

#endif /* MBEDTLS_ECP_ALT */

#endif /* ecp_alt.h */
