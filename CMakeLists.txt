#
# Copyright (c) 2021, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_HAS_TELINK_DRIVERS tlsr9)

if (CONFIG_SOC_RISCV_TELINK_B91)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/liblt_9518_zephyr.a)
elseif (CONFIG_SOC_RISCV_TELINK_B92)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/liblt_9528_zephyr.a)
else()
	message( FATAL_ERROR "Not supported SOC" )
endif()

if(CONFIG_PM)
zephyr_ld_options(-mtune=rocket)
endif() # PM
