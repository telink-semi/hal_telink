#
# Copyright (c) 2021, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory_ifdef(CONFIG_HAS_TELINK_DRIVERS tlsr9)

# PM and BLE external dependency
if(CONFIG_PM OR CONFIG_BT_B91)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/zephyr/blobs/liblt_9518_zephyr.a)

if(CONFIG_PM)
zephyr_ld_options(-mtune=rocket)
endif() # PM

endif() # PM or BLE external dependency
