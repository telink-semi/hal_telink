#
# Copyright (c) 2022-2023, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

if (CONFIG_SOC_RISCV_TELINK_B91)
	set(SOC "B91")
elseif (CONFIG_SOC_RISCV_TELINK_B92)
	set(SOC "B92")
else()
	message( FATAL_ERROR "Not supported SOC" )
endif()

zephyr_library()

zephyr_include_directories(common)
zephyr_include_directories(drivers/${SOC}/compatibility_pack)
zephyr_include_directories(drivers/${SOC}/reg_include)
zephyr_include_directories(drivers/${SOC})

# soc.c reference sources
zephyr_library_sources(drivers/${SOC}/clock.c)
zephyr_library_sources(drivers/${SOC}/analog.c)

# driver lib (sys.c, pm.c, flash_base.c, plic.c, pke.c, rf.c, trng.c)
if (CONFIG_SOC_RISCV_TELINK_B92)
zephyr_link_libraries(${CMAKE_CURRENT_SOURCE_DIR}/drivers/${SOC}/lib/libdriver.a)
endif()

# MbedTLS HW acceleration
if(CONFIG_MBEDTLS AND CONFIG_TELINK_B91_MBEDTLS_HW_ACCELERATION)
	add_subdirectory( crypto/mbedtls )
	zephyr_library_sources(
		drivers/${SOC}/aes.c
	)
endif() # MbedTLS HW acceleration

# Flash driver reference sources
if(CONFIG_SOC_FLASH_TELINK_B91 OR CONFIG_USB_TELINK_B91)
	zephyr_library_sources(
		drivers/${SOC}/flash.c
		drivers/${SOC}/stimer.c
	)
endif()

# PWM driver reference sources
zephyr_library_sources_ifdef(CONFIG_PWM_TELINK_B91 drivers/${SOC}/pwm.c)

# SPI driver reference sources
zephyr_library_sources_ifdef(CONFIG_SPI_TELINK_B91 drivers/${SOC}/gpio.c)

# I2C driver reference sources
zephyr_library_sources_ifdef(CONFIG_I2C_TELINK_B91 drivers/${SOC}/i2c.c)

# RF driver reference sources
if(CONFIG_BT_B91 OR CONFIG_IEEE802154_TELINK_B91)
	zephyr_library_sources(
		drivers/${SOC}/stimer.c
	)
endif()

# Entropy driver reference sources
zephyr_library_sources_ifdef(CONFIG_ENTROPY_TELINK_B91_TRNG drivers/${SOC}/trng.c)

# ADC driver reference sources
zephyr_library_sources_ifdef(CONFIG_ADC_TELINK_B91 drivers/${SOC}/adc.c)
zephyr_library_sources_ifdef(CONFIG_ADC_TELINK_B91 drivers/${SOC}/gpio.c)

# USB driver reference sources
zephyr_library_sources_ifdef(CONFIG_USB_TELINK_B91 drivers/${SOC}/gpio.c)
zephyr_library_sources_ifdef(CONFIG_USB_TELINK_B91 drivers/${SOC}/usbhw.c)

#PM driver dependency
zephyr_library_sources_ifdef(CONFIG_PM drivers/${SOC}/stimer.c)
zephyr_library_sources_ifdef(CONFIG_PM common/${SOC}_sleep.c)

# BLE reference sources
if (CONFIG_BT_B91)

zephyr_include_directories(
	ble
	ble/vendor/controller
)

zephyr_library_sources(
	drivers/${SOC}/aes.c
	drivers/${SOC}/flash.c
	drivers/${SOC}/stimer.c
	ble/common/utility.c
	ble/vendor/controller/b91_bt.c
	ble/vendor/controller/b91_bt_init.c
	ble/vendor/controller/b91_bt_buffer.c
)

endif() # CONFIG_BT_B91

# Retention feature
if (CONFIG_BOARD_TLSR9518ADK80D_RETENTION)

zephyr_library_sources(common/b91_context.S)

endif() # CONFIG_BOARD_TLSR9518ADK80D_RETENTION
