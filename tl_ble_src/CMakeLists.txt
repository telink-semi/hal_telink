string(REGEX REPLACE "^telink_" "" SOC_FAMILY ${CONFIG_SOC_SERIES})

if(CONFIG_SOC_RISCV_TELINK_B91)
	set(SOC "B91")
	message(STATUS "Configuring for SOC: TELINK_B91")
elseif(CONFIG_SOC_RISCV_TELINK_B92)
	set(SOC "B92")
	message(STATUS "Configuring for SOC: TELINK_B92")
elseif(CONFIG_SOC_RISCV_TELINK_TL321X)
	set(SOC "TL321X")
	message(STATUS "Configuring for SOC: TELINK_TL321X")
elseif(CONFIG_SOC_RISCV_TELINK_TL721X)
	set(SOC "TL721X")
	message(STATUS "Configuring for SOC: TELINK_TL721X")
else()
	message(FATAL_ERROR "Not supported SOC")
endif()

zephyr_library()

if(CONFIG_PM)
zephyr_ld_options(-mtune=rocket)
endif() # PM

zephyr_compile_options(
	-O2
	-DSTD_GCC=1
	# -flto=auto		// maybe cannot be used in zephyr
	-fmessage-length=0
	-ffunction-sections
	-fdata-sections
	-mno-relax
	-fno-builtin
	-fomit-frame-pointer
	-fno-strict-aliasing
	-fshort-wchar
	-fuse-ld=bfd
	-fno-jump-tables
	# -Wno-address-of-packed-member
	# -Wall
	# -Wextra
	# -Wshadow
	# -Wimplicit-fallthrough
	# -Wpointer-arith
	# -Wredundant-decls
	# -Wcast-qual
	# -Wsign-compare
	# -Wunused-parameter
	# -Wunused-variable
	# -Wswitch
	# -Wstrict-prototypes
	# -Wmissing-field-initializers
	# -Wdeprecated-declarations
	# -Wenum-conversion
	# -Wpacked-not-aligned
	# -Wundef
	-DTLKAPI_USE_INTERNAL_SPECIAL_UART_TOOL=0
	-DSDK_RELEASE_CHECK_EN=1
	-D__PROJECT_ACL_CONN_DEMO__=1
	-DFIX_AUX_CONN_SLOT_IDX_CAL=1
	-DVCD_EN=0
	-DCHIP_TYPE=CHIP_TYPE_${SOC}
	-DBLC_ZEPHYR_BLE_INTEGRATION=1
	-DLL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION=1
	-DBLUETOOTH_VER=BLUETOOTH_VER_5_0
	-DLL_FEATURE_SUPPORT_PRIVACY=1
	-DCONTROLLER_GEN_P256KEY_ENABLE=0
	-DCONN_MAX_NUM_CONFIG=CONN_MAX_NUM_C0_P1
	-DOPTIMIZE_RAM_FOR_MATTER=1
	-DLL_FEATURE_SUPPORT_LOCAL_RPA=0
	-DLL_FEATURE_SUPPORT_PHY_TEST_MODE=0
	-DLL_FEATURE_SUPPORT_LE_ENCRYPTION=0
	-DLL_FEATURE_SUPPORT_LE_LEGACY_SCANNING=0
	-DLL_FEATURE_SUPPORT_EXTENDED_SCANNER_FILTER_POLICIES=0
	-DLL_FEATURE_SUPPORT_LE_2M_PHY=1
	-DLL_FEATURE_SUPPORT_LE_CODED_PHY=0
	-DLL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING=0
	-DLL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING=0
	-DLL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2=1
	-DLL_FEATURE_SUPPORT_MIN_USED_OF_USED_CHANNELS=0
	-DLL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING=0
	-DLL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE=0
	-DLL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC=0
	-DLL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER=0
	-DTIFS_VARIATION_WORKAROUND_MLP_CODE_IN_RAM=0
	-DLL_FEATURE_SUPPORT_LE_PING=0
	-DLE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN=0
	-DDEBUG_GPIO_ENABLE=0
	-DSCH_TASK_PRIORITY_IN_CB_EN=0
	-DSTACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION=0
	-DSTACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION_V2=0
	-DBLC_PM_RETENTION_DRIVERS_ONLY=1
	-DBLC_PM_DEEP_RETENTION_MODE_EN=0
	-DPM_DEEPSLEEP_RETENTION_ENABLE=0
    
    -DBLMS_PM_ENABLE=0
    -DBLC_PM_EN=0
)

file(GLOB_RECURSE SOC_SOURCES
    ./drivers/${SOC}/error_handler/*.c
    ./drivers/${SOC}/ext_driver/*.c
    ./drivers/${SOC}/ext_driver/driver_internal/*.c
    ./drivers/${SOC}/flash/*.c
    ./drivers/${SOC}/lib/src/*.c
    ./drivers/${SOC}/lib/src/crypto_common/*.c
    ./drivers/${SOC}/lib/src/hash/*.c
    ./drivers/${SOC}/lib/src/pke/*.c
    ./drivers/${SOC}/lib/src/pm/*.c
    ./drivers/${SOC}/lib/src/rf/*.c
    ./drivers/${SOC}/lib/src/ske/*.c
    ./drivers/${SOC}/lib/src/trng/*.c

    ./drivers/${SOC}/lib/src/core.c
    ./drivers/${SOC}/lib/src/mspi.c
    ./drivers/${SOC}/watchdog.c
    ./drivers/${SOC}/dma.c
    ./vendor/common/flash_prot.c
    ./vendor/common/ble_flash.c
)

zephyr_sources(${SOC_SOURCES})

if(CONFIG_IEEE802154_TELINK_B9X OR CONFIG_IEEE802154_TELINK_TLX OR CONFIG_BT_B9X OR CONFIG_BT_TLX)
    file(GLOB BLE_SOURCES
        "./stack/ble/*.c"
        "./stack/ble/bqb/*.c"
        "./stack/ble/darwin/*.c"
        "./stack/ble/debug/*.c"
        "./stack/ble/device/*.c"
        "./stack/ble/hal/hal_${SOC}/*.c"
        "./stack/ble/hci/*.c"
        "./stack/ble/os_sup/*.c"
        "./stack/ble/secureboot/*.c"
        "./stack/ble/controller/*.c"
        # "./stack/ble/controller/cs_drbg/*.c"
        "./stack/ble/controller/csa/*.c"
        "./stack/ble/controller/ial/*.c"
        "./stack/ble/controller/ll/*.c"
        "./stack/ble/controller/phy/*.c"
        "./stack/ble/controller/whitelist/*.c"
        "./stack/ble/controller/ll/subrate/*.c"
        "./stack/ble/controller/ll/scan/*.c"
        "./stack/ble/controller/ll/prdadv/*.c"
        "./stack/ble/controller/ll/pcl/*.c"
        "./stack/ble/controller/ll/past/*.c"
        "./stack/ble/controller/ll/iso/*.c"
        "./stack/ble/controller/ll/init/*.c"
        "./stack/ble/controller/ll/chn_class/*.c"
        "./stack/ble/controller/ll/adv/*.c"
        "./stack/ble/controller/ll/acl_conn/*.c"
    )

    zephyr_library_sources(${BLE_SOURCES})
endif()


zephyr_include_directories(
  ./
  ./drivers/${SOC}
  ./drivers/${SOC}/compatibility_pack
  ./drivers/${SOC}/reg_include
  ./drivers/${SOC}/ext_driver
  ./stack/ble/hal/hal_${SOC}
  ./common
  ./hal_zephyr/common
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

# add for TLX headers
if (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
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
	add_subdirectory( hal_zephyr/crypto/mbedtls )
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
if(CONFIG_PWM_TELINK_B9X OR CONFIG_PWM_TELINK_TLX 
	OR CONFIG_SOC_RISCV_TELINK_TLX)
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
		hal_zephyr/common/tl_rf_power.c
	)

	if(NOT ${SOC} STREQUAL "TL721X")
		zephyr_library_sources(drivers/${SOC}/aes.c)
	endif()

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
	zephyr_library_sources(hal_zephyr/common/tl_sleep.c)
	zephyr_library_sources_ifdef(NOT (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
		drivers/${SOC}/stimer.c
	)
endif()

#BLE flash reference sources
if (CONFIG_IEEE802154_TELINK_B9X OR CONFIG_IEEE802154_TELINK_TLX)
	zephyr_include_directories(
		hal_zephyr/controller/${SOC_FAMILY}
	)
endif()

# BLE stack reference sources
if (CONFIG_BT_B9X OR CONFIG_BT_TLX)
	zephyr_include_directories(
		hal_zephyr/controller/${SOC_FAMILY}
	)
	zephyr_library_sources(
		common/utility.c
		hal_zephyr/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt.c
		hal_zephyr/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_init.c
		hal_zephyr/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_buffer.c
		hal_zephyr/controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_flash.c
	)
endif() # CONFIG_BT_B9X OR CONFIG_BT_TLX

if (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
	add_definitions(-DSTD_GCC)
endif()

# Retention feature
if (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION OR CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION)
	zephyr_library_sources(hal_zephyr/common/tl_context.S)
endif()

if (CONFIG_SOC_SERIES_RISCV_TELINK_TLX OR CONFIG_SOC_RISCV_TELINK_TLX)
	target_compile_options(${ZEPHYR_CURRENT_LIBRARY} PRIVATE -flto=auto)
endif()