zephyr_library()

zephyr_include_directories(
	common
	controller/${SOC_FAMILY}
)

if(CONFIG_IEEE802154_TELINK_B9X OR CONFIG_IEEE802154_TELINK_TLX)
	zephyr_include_directories(controller/${SOC_FAMILY})
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

if(CONFIG_PM)
	zephyr_sources(common/tl_sleep.c)
endif()