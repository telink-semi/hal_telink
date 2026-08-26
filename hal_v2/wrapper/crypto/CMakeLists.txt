#
# Copyright (c) 2026, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if (CONFIG_MBEDTLS OR CONFIG_BOOT_USE_MBEDTLS)
	add_subdirectory(mbedtls)
endif()

if (CONFIG_TINYCRYPT OR CONFIG_BOOT_USE_TINYCRYPT)
	add_subdirectory(tinycrypt)
endif()


if (CONFIG_TELINK_TLX_MBEDTLS_HW_ACCELERATION OR CONFIG_TELINK_TLX_TINYCRYPT_HW_ACCELERATION)

zephyr_library()

zephyr_library_compile_definitions(
	CONFIG_HASH_SUPPORT_MUL_THREAD=1
)

zephyr_library_sources(
	sha256_wrapper.c
	multithread_tlx.c
	sha_tlx_backend.c
)

if (CONFIG_TELINK_TLX_MBEDTLS_HW_ACCELERATION)

	zephyr_link_libraries(
		-Wl,--wrap,mbedtls_sha256_starts
		-Wl,--wrap,mbedtls_sha256_update
		-Wl,--wrap,mbedtls_sha256_finish
		-Wl,--wrap=mbedtls_sha256
		-Wl,--wrap=mbedtls_sha256_clone
		-Wl,--wrap=mbedtls_sha256_free
	)

endif()

if (CONFIG_TELINK_TLX_TINYCRYPT_HW_ACCELERATION)

	zephyr_link_libraries(
		-Wl,--wrap,tc_sha256_init
		-Wl,--wrap,tc_sha256_update
		-Wl,--wrap,tc_sha256_final
	)

endif()

endif()
