#
# Copyright (c) 2022, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

zephyr_library()

zephyr_include_directories(common)
zephyr_include_directories(drivers/B91/compatibility_pack)
zephyr_include_directories(drivers/B91/reg_include)
zephyr_include_directories(drivers/B91)

# soc.c reference sources
if (NOT CONFIG_PM AND NOT CONFIG_BT_B91)
zephyr_library_sources(drivers/B91/sys.c)
endif() # no PM and no BLE external dependency
zephyr_library_sources(drivers/B91/pm.c)
zephyr_library_sources(drivers/B91/clock.c)
zephyr_library_sources(drivers/B91/analog.c)

# MbedTLS HW acceleration
if(CONFIG_MBEDTLS AND CONFIG_TELINK_B91_MBEDTLS_HW_ACCELERATION)
	add_subdirectory( crypto/mbedtls )
	zephyr_library_sources(
		drivers/B91/pke.c
		drivers/B91/aes.c
		drivers/B91/trng.c
	)
endif() # MbedTLS HW acceleration

# Flash driver reference sources
if(CONFIG_SOC_FLASH_TELINK_B91 OR CONFIG_USB_TELINK_B91)
	zephyr_library_sources(
		drivers/B91/flash.c
		drivers/B91/plic.c
		drivers/B91/stimer.c
	)
endif()

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

# ADC driver reference sources
zephyr_library_sources_ifdef(CONFIG_ADC_TELINK_B91 drivers/B91/adc.c)
zephyr_library_sources_ifdef(CONFIG_ADC_TELINK_B91 drivers/B91/gpio.c)

# USB driver reference sources
zephyr_library_sources_ifdef(CONFIG_USB_TELINK_B91 drivers/B91/gpio.c)
zephyr_library_sources_ifdef(CONFIG_USB_TELINK_B91 drivers/B91/usbhw.c)

#PM driver dependency
zephyr_library_sources_ifdef(CONFIG_PM drivers/B91/stimer.c)

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

endif() # CONFIG_BT_B91

# PM and BLE external dependency
if(CONFIG_PM OR CONFIG_BT_B91)

if(${ZEPHYR_TOOLCHAIN_VARIANT} STREQUAL "zephyr")
	zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/ble/proj_lib/lib/liblt_9518_zephyr.a)
else()
	zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/ble/proj_lib/lib/liblt_9518.a)
endif()

if(CONFIG_PM)
zephyr_ld_options(-mtune=rocket)
endif() # PM

endif() # PM or BLE external dependency
