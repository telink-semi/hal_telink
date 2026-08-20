#
# Copyright (c) 2026, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

# SOC configuration for Telink RISC-V series
if(CONFIG_SOC_FAMILY_TELINK_TL)
    set(SOC_FAMILY "tlx")
else()
    string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})
endif()

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
elseif(CONFIG_SOC_RISCV_TELINK_TL521X)
	set(SOC "TL521X")
	set(LIB_SOC_NAME "tl521x")
	message(STATUS "Configuring for SOC: TELINK_TL521X")
elseif(CONFIG_SOC_RISCV_TELINK_TL523X)
	set(SOC "TL523X")
	set(LIB_SOC_NAME "tl523x")
	message(STATUS "Configuring for SOC: TELINK_TL523X")
else()
	message(FATAL_ERROR "Not supported SOC")
endif()

# Default to precompiled BLE SDK unless Kconfig disables it.
# A disabled bool Kconfig option is undefined in CMake, so use
# CONFIG_SOC_SERIES as a sentinel to detect that Kconfig has run.
if(NOT DEFINED CONFIG_COMPILE_SDK)
    if(DEFINED CONFIG_SOC_SERIES)
        set(CONFIG_COMPILE_SDK 0)   # Kconfig processed; COMPILE_SDK explicitly disabled.
    else()
        set(CONFIG_COMPILE_SDK 1)   # Kconfig not loaded yet -> default to precompiled SDK.
    endif()
endif()

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

	add_subdirectory(${TELINK_BLE_SDK_DIR}/tools ${CMAKE_CURRENT_BINARY_DIR}/telink_ble_sdk/tools)

	set(lib_dir_path "${TELINK_BLE_SDK_DIR}/tl_ble_sdk/proj_lib/zephyr/${LIB_SOC_NAME}/lib_zephyr_${LIB_SOC_NAME}")

	if(CONFIG_SOC_RISCV_TELINK_TL322X)
		if(CONFIG_TELINK_TL322X_ENABLE_N22)
			set(lib_dir_path ${lib_dir_path}_dual_core)
		else()
			set(lib_dir_path ${lib_dir_path}_single_core)
		endif()
	endif()

	if(CONFIG_TELINK_TL322X_ENABLE_N22)
		set(lib_dir_path ${lib_dir_path}.a)
	else()
		if(CONFIG_TL_BLE_CTRL_PERIPHERAL)
			set(lib_dir_path ${lib_dir_path}_peripheral.a)
			# set(lib_dir_path ${lib_dir_path}_multirole.a)
		elseif(CONFIG_TL_BLE_CTRL_CENTRAL)
			set(lib_dir_path ${lib_dir_path}_central.a)
		elseif(CONFIG_TL_BLE_CTRL_MULTIROLE)
			set(lib_dir_path ${lib_dir_path}_multirole.a)
		endif()
	endif()

	message(STATUS "HAL_TELINK LINK ${lib_dir_path}")
	zephyr_link_libraries(${lib_dir_path})
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
