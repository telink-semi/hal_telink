#
# Copyright (c) 2026, Telink
#
# SPDX-License-Identifier: Apache-2.0
#

string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})

if (CONFIG_SOC_RISCV_TELINK_TL711X)
	set(SOC "tl711x")
else()
	message(FATAL_ERROR "Not supported SOC")
endif()

# Include source files
file(GLOB SOC_SOURCES
	tl_platform_src/chip/${SOC}/drivers/error_handler/error_handler.c

	tl_platform_src/chip/${SOC}/drivers/flash/*.c

	tl_platform_src/chip/${SOC}/drivers/lib/src/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/crypto_common/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/hash/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/pke/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/pm/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/rf/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/ske/*.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/trng/*.c

	tl_platform_src/chip/${SOC}/drivers/lib/src/rf/rf_common.c

	tl_platform_src/chip/${SOC}/drivers/lib/src/pm/pm_internal.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/pm/pm.c

	tl_platform_src/chip/${SOC}/drivers/lib/src/analog.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/clock.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/core.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/efuse.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/flash_base.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/mspi.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/nvm.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/plic.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/stimer.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/swire.c
	tl_platform_src/chip/${SOC}/drivers/lib/src/sys.c

	tl_platform_src/chip/${SOC}/drivers/flash.c
	tl_platform_src/chip/${SOC}/drivers/gpio.c
	tl_platform_src/chip/${SOC}/drivers/lpc.c
	tl_platform_src/chip/${SOC}/drivers/spi.c
	tl_platform_src/chip/${SOC}/drivers/watchdog.c
)

zephyr_library()

zephyr_library_sources(${SOC_SOURCES})

zephyr_library_compile_options(-Wno-macro-redefined)

zephyr_compile_definitions(
	-DSTD_GCC=1
	-DBLMS_PM_ENABLE=0
	-DBLC_PM_EN=0
	-DMAILBOX_PM_VALUE_SYNC=0
	-DBLC_PM_DEEP_RETENTION_MODE_EN=0
	-DPM_DEEPSLEEP_RETENTION_ENABLE=0
)

# Include header files
zephyr_include_directories(
	tl_platform_src/common

	tl_platform_src/chip/${SOC}/drivers

	tl_platform_src/chip/${SOC}/drivers/compatibility_pack
	tl_platform_src/chip/${SOC}/drivers/error_handler
	tl_platform_src/chip/${SOC}/drivers/flash
	tl_platform_src/chip/${SOC}/drivers/reg_include

	tl_platform_src/chip/${SOC}/drivers/lib/include
	tl_platform_src/chip/${SOC}/drivers/lib/include/crypto_common
	tl_platform_src/chip/${SOC}/drivers/lib/include/hash
	tl_platform_src/chip/${SOC}/drivers/lib/include/pke
	tl_platform_src/chip/${SOC}/drivers/lib/include/pm
	tl_platform_src/chip/${SOC}/drivers/lib/include/rf
	tl_platform_src/chip/${SOC}/drivers/lib/include/ske
	tl_platform_src/chip/${SOC}/drivers/lib/include/trng
)
