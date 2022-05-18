#
# Copyright (c) 2021, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_include_directories(common)
zephyr_include_directories(drivers/B91/compatibility_pack)
zephyr_include_directories(drivers/B91/reg_include)
zephyr_include_directories(drivers/B91)

# soc.c referance sources
zephyr_library_sources_ifndef(CONFIG_BT_B91 drivers/B91/sys.c)
zephyr_library_sources(drivers/B91/pm.c)
zephyr_library_sources(drivers/B91/clock.c)
zephyr_library_sources(drivers/B91/analog.c)

# Flash driver reference sources
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_TELINK_B91 drivers/B91/flash.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_TELINK_B91 drivers/B91/plic.c)
zephyr_library_sources_ifdef(CONFIG_SOC_FLASH_TELINK_B91 drivers/B91/stimer.c)

# PWM driver reference sources
zephyr_library_sources_ifdef(CONFIG_PWM_TELINK_B91 drivers/B91/pwm.c)

# SPI driver reference sources
zephyr_library_sources_ifdef(CONFIG_SPI_TELINK_B91 drivers/B91/gpio.c)

# I2C driver reference sources
zephyr_library_sources_ifdef(CONFIG_I2C_TELINK_B91 drivers/B91/i2c.c)

# RF driver reference sources
zephyr_library_sources_ifdef(CONFIG_IEEE802154_TELINK_B91 drivers/B91/stimer.c)
zephyr_library_sources_ifdef(CONFIG_IEEE802154_TELINK_B91 drivers/B91/rf.c)

# Entropy driver reference sources
zephyr_library_sources_ifdef(CONFIG_ENTROPY_TELINK_B91_TRNG drivers/B91/trng.c)

# BLE reference sources
if (CONFIG_BT_B91)

zephyr_include_directories(
	ble
	ble/vendor/controller
)

zephyr_library_sources(
	drivers/B91/pke.c
	drivers/B91/aes.c
	drivers/B91/flash.c
	drivers/B91/plic.c
	drivers/B91/stimer.c
	drivers/B91/ext_driver/software_pa.c
	ble/common/utility.c
	ble/vendor/controller/b91_bt.c
	ble/vendor/controller/b91_bt_init.c
	ble/vendor/controller/b91_bt_buffer.c
)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
	zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/ble/proj_lib/liblt_9518_zephyr.a)
else()
	zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/ble/proj_lib/liblt_9518.a)
endif()

endif() # CONFIG_BT_B91
