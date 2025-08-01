#
# Copyright (c) 2025, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_TELINK_DRIVERS)

# SOC configuration for Telink RISC-V series
string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})

if(CONFIG_SOC_RISCV_TELINK_B91)
	set(SOC "B91")
	set(LIB_SOC_NAME "b91")
	message(STATUS "Configuring for SOC: TELINK_B91")
elseif(CONFIG_SOC_RISCV_TELINK_B92)
	set(SOC "B92")
	set(LIB_SOC_NAME "b92")
	message(STATUS "Configuring for SOC: TELINK_B92")
elseif(CONFIG_SOC_RISCV_TELINK_TL321X)
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
else()
	message(FATAL_ERROR "Not supported SOC")
endif()

add_subdirectory(tl_ble_sdk/tools)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/tl_ble_sdk/tl_ble_sdk/proj_lib/lib_zephyr_${LIB_SOC_NAME}.a)
# add_subdirectory(tl_ble_src/tools)

add_subdirectory(wrapper)

if(CONFIG_MBEDTLS)
	add_subdirectory(wrapper/crypto/mbedtls)
endif()

if(CONFIG_PM)
	zephyr_ld_options(-mtune=rocket)
endif()

endif() # CONFIG_HAS_TELINK_DRIVERS
