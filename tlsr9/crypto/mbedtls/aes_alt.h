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

#ifndef AES_ALT_H
#define AES_ALT_H

#if defined(MBEDTLS_AES_ALT)

typedef struct mbedtls_aes_context {
	int MBEDTLS_PRIVATE(nr);
	uint32_t *MBEDTLS_PRIVATE(rk);
	uint32_t MBEDTLS_PRIVATE(buf)[68];
} mbedtls_aes_context;

#if defined(MBEDTLS_CIPHER_MODE_XTS)

typedef struct mbedtls_aes_xts_context {
	mbedtls_aes_context MBEDTLS_PRIVATE(crypt);
	mbedtls_aes_context
		MBEDTLS_PRIVATE(tweak);
} mbedtls_aes_xts_context;

#endif /* MBEDTLS_CIPHER_MODE_XTS */

#endif /* MBEDTLS_AES_ALT */

#endif /* aes_alt.h */
