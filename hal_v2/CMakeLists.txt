#
# Copyright (c) 2025, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

# SOC configuration for Telink RISC-V series
string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})

if(CONFIG_SOC_RISCV_TELINK_TL321X)
	set(SOC "TL321X")
	set(LIB_SOC_NAME "tl321x")
	message(STATUS "Configuring for SOC: TELINK_TL321X")
elseif(CONFIG_SOC_RISCV_TELINK_TL721X)
	set(SOC "TL721X")
	set(LIB_SOC_NAME "tl721x")
	message(STATUS "Configuring for SOC: TELINK_TL721X")
elseif(CONFIG_SOC_RISCV_TELINK_TL322X)
	set(SOC "TL322X")
	set(LIB_SOC_NAME "tl322x")
	message(STATUS "Configuring for SOC: TELINK_TL322X")
elseif(CONFIG_SOC_RISCV_TELINK_TL323X)
	set(SOC "TL323X")
	set(LIB_SOC_NAME "tl323x")
	message(STATUS "Configuring for SOC: TELINK_TL323X")
else()
	message(FATAL_ERROR "Not supported SOC")
endif()

set(CONFIG_COMPILE_SDK 1)

if(CONFIG_COMPILE_SDK)
	get_filename_component(
		TELINK_BLE_SDK_DIR
		"${ZEPHYR_HAL_TELINK_MODULE_DIR}/../telink_ble_sdk"
		ABSOLUTE
	)

	if(NOT EXISTS "${TELINK_BLE_SDK_DIR}/tl_ble_sdk")
		message(FATAL_ERROR
			"Telink BLE SDK not found at ${TELINK_BLE_SDK_DIR}. "
		)
	endif()

	add_subdirectory(${TELINK_BLE_SDK_DIR}/tools ${CMAKE_CURRENT_BINARY_DIR}/telink_ble_sdk_tools)

	if(CONFIG_SOC_RISCV_TELINK_TL322X)
		if(CONFIG_TELINK_TL322X_ENABLE_N22)
			message(STATUS "Enabling N22 support for TL322X")
			zephyr_link_libraries(${TELINK_BLE_SDK_DIR}/tl_ble_sdk/proj_lib/zephyr/${LIB_SOC_NAME}/lib_zephyr_${LIB_SOC_NAME}_dual_core.a)
		else()
			zephyr_link_libraries(${TELINK_BLE_SDK_DIR}/tl_ble_sdk/proj_lib/zephyr/${LIB_SOC_NAME}/lib_zephyr_${LIB_SOC_NAME}_single_core.a)
		endif()
	elseif(CONFIG_SOC_RISCV_TELINK_TL323X OR CONFIG_SOC_RISCV_TELINK_TL721X)
			zephyr_link_libraries(${TELINK_BLE_SDK_DIR}/tl_ble_sdk/proj_lib/zephyr/${LIB_SOC_NAME}/lib_zephyr_${LIB_SOC_NAME}.a)
	elseif(CONFIG_SOC_RISCV_TELINK_TL321X)
		zephyr_link_libraries(${TELINK_BLE_SDK_DIR}/tl_ble_sdk/proj_lib/lib_zephyr_${LIB_SOC_NAME}.a)
	endif()
else()
	add_subdirectory(tl_ble_src/tools)
endif()

add_subdirectory(wrapper)

if(CONFIG_MBEDTLS)
	add_subdirectory(wrapper/crypto/mbedtls)
endif()

if(CONFIG_PM)
	zephyr_ld_options(-mtune=rocket)
endif()
