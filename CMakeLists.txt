#
# Copyright (c) 2021, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_HAS_TELINK_DRIVERS)

add_subdirectory(tlsr9)

if (CONFIG_SOC_RISCV_TELINK_B91)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_zephyr_b91.a)
elseif (CONFIG_SOC_RISCV_TELINK_B92)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_zephyr_b92.a)
elseif (CONFIG_SOC_RISCV_TELINK_B95)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_zephyr_b95.a)
elseif (CONFIG_SOC_RISCV_TELINK_TL321X)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/lib_zephyr_tl321x.a)
else()
	message( FATAL_ERROR "Not supported SOC" )
endif()

if(CONFIG_PM)
zephyr_ld_options(-mtune=rocket)
endif() # PM

endif() # CONFIG_HAS_TELINK_DRIVERS
