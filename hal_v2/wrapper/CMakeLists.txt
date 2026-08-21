zephyr_library()

if(CONFIG_SOC_RISCV_TELINK_TL322X)

set(TL_ZIGBEE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../tl_zigbee_src/)

zephyr_compile_options(
	-DFLASH_CAP_SIZE_1M=1
	-DMCU_CORE_TL3228X=1
	-DROUTER=1
	-D__PROJECT_TL_DIMMABLE_LIGHT__=1
)

zephyr_include_directories(
	zb_proj
)

# file(GLOB ZIGBEE_SOURCES
#		zb_proj/drivers/multi_core/*.c
#)
# zephyr_library_sources(${ZIGBEE_SOURCES})


zephyr_compile_options(
	-DCHIP_TYPE=CHIP_TYPE_TL322X
	# -DTLK_MESSAGE_D25F=1
	# -DTLK_MESSAGE_N22=0
	-DMAILBOX_D25F=1
	-DMAILBOX_FALSH_WR_NOTIFY=1
	-DSHM_MBOX_TYPE_TPSLL_EN=1
	-DSHM_MBOX_TYPE_ZIGBEE_EN=1
	-DMCU_CORE_TL322X_D25F=1
)
endif()

zephyr_include_directories(
	common
	controller/${SOC_FAMILY}
)

if(CONFIG_IEEE802154_TLX_BLE_COEXIST)
	zephyr_include_directories(${ZEPHYR_BASE}/drivers/ieee802154)
endif()

if(CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION OR CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION)
	zephyr_library_sources(common/tl_context.S)
endif()

if(CONFIG_BT_B9X OR CONFIG_BT_TLX)
	zephyr_library_sources(
		controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_init.c
		controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_buffer.c
		controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_flash.c
	)

	if(CONFIG_BT_CHANNEL_SOUNDING)
		zephyr_library_sources(
			controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_cs.c
		)
	endif()

	if(CONFIG_TELINK_TL322X_ENABLE_N22)
		zephyr_library_sources(
			controller/${SOC_FAMILY}/${SOC_FAMILY}_bt_host.c
		)
	else()
		zephyr_library_sources(
			controller/${SOC_FAMILY}/${SOC_FAMILY}_bt.c
		)
	endif()
endif()

if(CONFIG_BT OR CONFIG_IEEE802154)
    zephyr_library_sources(common/tl_rf_power.c)
endif()

zephyr_library_sources(common/tl_flash.c)
if(CONFIG_SOC_RISCV_TELINK_TL323X)
zephyr_library_sources(common/tl_sd_adc.c)
endif()

if(CONFIG_PM)
	zephyr_sources(common/tl_sleep.c)
endif()