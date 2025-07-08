#
# Copyright (c) 2021-2025, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_TELINK_DRIVERS)

add_subdirectory(tlsr9)

# Enabled by default from Zephyr 3.7 version
if (CMAKE_PROJECT_VERSION_MAJOR GREATER_EQUAL 3 AND CMAKE_PROJECT_VERSION_MINOR GREATER_EQUAL 7)
	set(OPT_FLAGS "_Os")
endif()

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
	set(TOOLCHAIN_LIB "zephyr")
elseif(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "andes-gcc")
	set(TOOLCHAIN_LIB "nds")
else()
	message( FATAL_ERROR "Not supported toolchain: '${ZEPHYR_TOOLCHAIN_VARIANT}'" )
endif()

if (CONFIG_SOC_RISCV_TELINK_B91)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_${TOOLCHAIN_LIB}_b91${OPT_FLAGS}.a)
elseif (CONFIG_SOC_RISCV_TELINK_B92)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_${TOOLCHAIN_LIB}_b92${OPT_FLAGS}.a)
elseif (CONFIG_SOC_RISCV_TELINK_TL321X)
	if(CONFIG_PM)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl321x${OPT_FLAGS}_pm.a)
	else()
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl321x${OPT_FLAGS}.a)
	endif()
elseif (CONFIG_SOC_RISCV_TELINK_TL721X)
	if(CONFIG_PM)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl721x${OPT_FLAGS}_pm.a)
	else()
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_${TOOLCHAIN_LIB}_tl721x${OPT_FLAGS}.a)
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

endif() # CONFIG_HAS_TELINK_DRIVERS
