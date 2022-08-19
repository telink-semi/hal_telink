#
# Copyright (c) 2022, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

# MbedTLS HW accelerated alternative

zephyr_library_compile_definitions( MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}")

zephyr_include_directories(
	./
	./internal/
	${ZEPHYR_MBEDTLS_MODULE_DIR}/include/
	${ZEPHYR_BASE}/modules/mbedtls/configs/
	)

# Common
zephyr_library_sources(
	./internal/multithread.c
	)

# AES accelerator
zephyr_library_sources(
	./internal/compatibility/aes_alt.c
	)

# EC accelerator
zephyr_library_sources(
	./internal/compatibility/ecp_alt.c
	./internal/compatibility/ecp_curves_alt.c
	./internal/ecp_alt_b91_backend_test.c
	./internal/ecp_alt_b91_backend.c
	)

# HW entropy
zephyr_library_sources(
	./internal/entropy_poll_alt.c
	)
