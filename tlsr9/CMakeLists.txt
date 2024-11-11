#
# Copyright (c) 2022-2023, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})

if (CONFIG_SOC_RISCV_TELINK_B91)
	set(SOC "B91")
elseif (CONFIG_SOC_RISCV_TELINK_B92)
	set(SOC "B92")
elseif (CONFIG_SOC_RISCV_TELINK_B95)
	set(SOC "B95")
elseif (CONFIG_SOC_RISCV_TELINK_TL321X)
	set(SOC "TL321X")
elseif (CONFIG_SOC_RISCV_TELINK_TL721X)
	set(SOC "TL721X")
else()
	message( FATAL_ERROR "Not supported SOC" )
endif()

zephyr_library()

# common headers for all 
zephyr_include_directories(
	common
	drivers/${SOC}
	drivers/${SOC}/compatibility_pack
	drivers/${SOC}/reg_include
	drivers/${SOC}/ext_driver
)

if (NOT CONFIG_SOC_RISCV_TELINK_B91)

# common headers excluse B91
zephyr_include_directories(
	drivers/${SOC}/flash
	drivers/${SOC}/lib/include
	drivers/${SOC}/lib/include/pke
	)

# B92 headers
if (CONFIG_SOC_RISCV_TELINK_B92)
	zephyr_include_directories(
		drivers/${SOC}/ext_peripherals/codec_0581
		)
endif()

# add for B95/TLX headers
if (CONFIG_SOC_RISCV_TELINK_B95 OR CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
	zephyr_include_directories(
		drivers/${SOC}/lib/include/rf
		drivers/${SOC}/lib/include/ske
		drivers/${SOC}/lib/include/trng
	)
endif()

# add for TLX or B9X headers
if (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
	zephyr_include_directories(drivers/${SOC}/ext_driver/driver_internal)
elseif(CONFIG_SOC_SERIES_RISCV_TELINK_B9X)
	zephyr_include_directories(drivers/${SOC}/ext_driver/driver_lib)
endif()

endif() # NOT CONFIG_SOC_RISCV_TELINK_B91

# soc.c reference sources for B9X
if (NOT (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX))
	zephyr_library_sources(drivers/${SOC}/clock.c)
	zephyr_library_sources(drivers/${SOC}/analog.c)
endif()

# MbedTLS HW acceleration
if(CONFIG_MBEDTLS)
	add_subdirectory( crypto/mbedtls )
endif() # MbedTLS HW acceleration

# Flash driver reference sources
if(CONFIG_SOC_FLASH_TELINK_B9X OR CONFIG_USB_TELINK_B9X)
	zephyr_library_sources(
		drivers/${SOC}/flash.c
		drivers/${SOC}/stimer.c
	)
endif()
if(CONFIG_SOC_FLASH_TELINK_TLX OR CONFIG_USB_TELINK_TLX)
	zephyr_library_sources( drivers/${SOC}/flash.c )
endif()

# GPIO driver reference sources
if(CONFIG_GPIO_TELINK_B9X OR CONFIG_GPIO_TELINK_TLX)
	zephyr_library_sources(drivers/${SOC}/gpio.c)
endif()

# PWM driver reference sources
if(CONFIG_PWM_TELINK_B9X OR CONFIG_PWM_TELINK_TLX)
	zephyr_library_sources(drivers/${SOC}/pwm.c)
endif()

# SPI driver reference sources
if(CONFIG_SPI_TELINK_B9X OR CONFIG_SPI_TELINK_TLX)
	zephyr_library_sources(drivers/${SOC}/gpio.c)
endif()

# I2C driver reference sources
if(CONFIG_I2C_TELINK_B9X OR CONFIG_I2C_TELINK_TLX)
	zephyr_library_sources(drivers/${SOC}/i2c.c)
endif()

# RF driver reference sources
if(CONFIG_BT OR CONFIG_IEEE802154)
	zephyr_library_sources(
		drivers/${SOC}/aes.c
		common/tl_rf_power.c
	)
if(CONFIG_BT_B9X OR CONFIG_IEEE802154_TELINK_B9X)
	zephyr_library_sources(drivers/${SOC}/stimer.c)
endif() # CONFIG_BT_B9X OR CONFIG_IEEE802154_TELINK_B9X
endif() # CONFIG_BT OR CONFIG_IEEE802154

# ADC driver reference sources
if(CONFIG_ADC_TELINK_B9X OR CONFIG_ADC_TELINK_TLX)
	zephyr_library_sources(drivers/${SOC}/gpio.c)
	zephyr_library_sources(drivers/${SOC}/adc.c)
endif()

# USB driver reference sources
if(CONFIG_USB_TELINK_B9X OR CONFIG_USB_TELINK_TLX)
	zephyr_library_sources(drivers/${SOC}/gpio.c)
	zephyr_library_sources(drivers/${SOC}/usbhw.c)
endif()

#PM driver dependency
if(CONFIG_PM)
	zephyr_library_sources(common/tl_sleep.c)
	zephyr_library_sources_ifdef(NOT (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
		drivers/${SOC}/stimer.c
	)
endif()

#BLE flash reference sources
if (CONFIG_IEEE802154_TELINK_B9X OR CONFIG_IEEE802154_TELINK_TLX)
	zephyr_include_directories(
		ble
		ble/vendor/controller/${SOC_FAMILY}
	)
endif()

# BLE stack reference sources
if (CONFIG_BT_B9X OR CONFIG_BT_TLX)
	zephyr_include_directories(
		ble
		ble/vendor/controller/${SOC_FAMILY}
	)
	zephyr_library_sources(
		ble/common/utility.c
		ble/vendor/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt.c
		ble/vendor/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_init.c
		ble/vendor/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_buffer.c
		ble/vendor/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_flash.c
	)
endif() # CONFIG_BT_B9X OR CONFIG_BT_TLX

if (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
	add_definitions(-DSTD_GCC)
endif()

# Retention feature
if (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION OR CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION)
	zephyr_library_sources(common/tl_context.S)
endif()

if (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
	target_compile_options(${ZEPHYR_CURRENT_LIBRARY} PRIVATE -flto=auto)
endif()
