#
# Copyright (c) 2022-2026, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_compile_definitions(
	MBEDTLS_CONFIG_FILE="${CONFIG_MBEDTLS_CFG_FILE}"
)

zephyr_include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}
)

if(CONFIG_TELINK_B9X_MBEDTLS_HW_ACCELERATION OR CONFIG_TELINK_TLX_MBEDTLS_HW_ACCELERATION)

zephyr_library()

zephyr_include_directories(
	${ZEPHYR_MBEDTLS_MODULE_DIR}/include
	${ZEPHYR_BASE}/modules/mbedtls/configs
)

zephyr_library_sources(
	mbedtls_wrapper.c
	ecp_backend_test.c
	multithread.c
)

zephyr_library_sources_ifdef(CONFIG_TELINK_B9X_MBEDTLS_HW_ACCELERATION 
	./ecp_b9x_backend.c
)
zephyr_library_sources_ifdef(CONFIG_TELINK_TLX_MBEDTLS_HW_ACCELERATION 
	./ecp_tlx_backend.c
)

# Include TLX drivers for accelerated SHA256, linker wraps
if(CONFIG_TELINK_TLX_MBEDTLS_HW_SHA_ACCELERATION)
	message(STATUS "HW SHA configured")

	zephyr_include_directories(
		${TELINK_BLE_SDK_DIR}/tl_ble_sdk/drivers/${SOC}/lib/include
		${TELINK_BLE_SDK_DIR}/tl_ble_sdk/drivers/${SOC}/lib/include/hash
	)

	zephyr_library_sources( 
		./sha256_tlx_backend.c
	)

	zephyr_link_libraries(
		-Wl,--wrap,mbedtls_sha256_init
		-Wl,--wrap,mbedtls_sha256_starts
		-Wl,--wrap,mbedtls_sha256_update
		-Wl,--wrap,mbedtls_sha256_finish
		-Wl,--wrap,mbedtls_sha256_free
		-Wl,--wrap=mbedtls_sha256
		-Wl,--wrap=mbedtls_sha256_clone
	)
endif() # CONFIG_TELINK_TLX_MBEDTLS_HW_SHA_ACCELERATION

zephyr_link_libraries(
	-Wl,--wrap,mbedtls_ecp_check_pubkey
	-Wl,--wrap,mbedtls_ecp_mul
	-Wl,--wrap,mbedtls_ecp_mul_restartable
	-Wl,--wrap,mbedtls_ecp_muladd
	-Wl,--wrap,mbedtls_ecp_muladd_restartable
	-Wl,--wrap,mbedtls_ecp_gen_key
	-Wl,--wrap,mbedtls_ecp_gen_keypair
	-Wl,--wrap,mbedtls_ecp_gen_keypair_base
	-Wl,--wrap,mbedtls_ecp_self_test
)

endif()
