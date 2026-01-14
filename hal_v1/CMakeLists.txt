#
# Copyright (c) 2021-2025, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(tlsr9)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
	set(TOOLCHAIN_LIB "zephyr")
elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "andes-gcc")
	set(TOOLCHAIN_LIB "nds")
else()
	message( FATAL_ERROR "Not supported toolchain: '${ZEPHYR_TOOLCHAIN_VARIANT}'" )
endif()

if (CONFIG_SOC_RISCV_TELINK_B91)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../zephyr/blobs/lib_${TOOLCHAIN_LIB}_b91.a)
elseif (CONFIG_SOC_RISCV_TELINK_B92)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../zephyr/blobs/lib_${TOOLCHAIN_LIB}_b92.a)
elseif (CONFIG_SOC_RISCV_TELINK_TL321X)
	if(CONFIG_PM)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl321x_pm.a)
	else()
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl321x.a)
	endif()
elseif (CONFIG_SOC_RISCV_TELINK_TL721X)
	if(CONFIG_PM)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl721x_pm.a)
	else()
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/../zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl721x.a)
	endif()
else()
	message( FATAL_ERROR "Not supported SOC" )
endif()

zephyr_compile_options(
	-DBLC_ZEPHYR_BLE_INTEGRATION=1
)

if(CONFIG_PM)
zephyr_ld_options(-mtune=rocket)
endif() # PM

