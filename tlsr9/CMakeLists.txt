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
zephyr_library_sources(drivers/B91/clock.c)
zephyr_library_sources(drivers/B91/analog.c)

# MbedTLS HW acceleration
if(CONFIG_MBEDTLS AND CONFIG_TELINK_B91_MBEDTLS_HW_ACCELERATION)
	add_subdirectory( crypto/mbedtls )
	zephyr_library_sources(
		drivers/B91/aes.c
	)
endif() # MbedTLS HW acceleration

# Flash driver reference sources
if(CONFIG_SOC_FLASH_TELINK_B91 OR CONFIG_USB_TELINK_B91)
	zephyr_library_sources(
		drivers/B91/flash.c
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
if(CONFIG_BT_B91 OR CONFIG_IEEE802154_TELINK_B91)
	zephyr_library_sources(
		drivers/B91/stimer.c
	)
endif()

# ADC driver reference sources
zephyr_library_sources_ifdef(CONFIG_ADC_TELINK_B91 drivers/B91/adc.c)
zephyr_library_sources_ifdef(CONFIG_ADC_TELINK_B91 drivers/B91/gpio.c)

# USB driver reference sources
zephyr_library_sources_ifdef(CONFIG_USB_TELINK_B91 drivers/B91/gpio.c)
zephyr_library_sources_ifdef(CONFIG_USB_TELINK_B91 drivers/B91/usbhw.c)

#PM driver dependency
zephyr_library_sources_ifdef(CONFIG_PM drivers/B91/stimer.c)
zephyr_library_sources_ifdef(CONFIG_PM common/b91_sleep.c)

# BLE reference sources
if (CONFIG_BT_B91)

zephyr_include_directories(
	ble
	ble/vendor/controller
)

zephyr_library_sources(
	drivers/B91/aes.c
	drivers/B91/flash.c
	drivers/B91/stimer.c
	ble/common/utility.c
	ble/vendor/controller/b91_bt.c
	ble/vendor/controller/b91_bt_init.c
	ble/vendor/controller/b91_bt_buffer.c
)

endif() # CONFIG_BT_B91
