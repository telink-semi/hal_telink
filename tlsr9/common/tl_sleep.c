/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/
#include <tl_sleep.h>
#include <ext_driver/ext_pm.h>

#if (CONFIG_BT_B9X || CONFIG_BT_TLX)
#if CONFIG_SOC_RISCV_TELINK_B91
#include <stack/ble/B91/controller/os_sup.h>
#elif CONFIG_SOC_RISCV_TELINK_B92
#include <stack/ble/B92/controller/os_sup.h>
#elif CONFIG_SOC_RISCV_TELINK_B95
#include <stack/ble/B95/controller/os_sup.h>
#elif CONFIG_SOC_RISCV_TELINK_TL321X
#include <stack/ble/TL321X/controller/os_sup.h>
#endif
#include <tl_rf_power.h>
#endif /* CONFIG_BT_B9X || CONFIG_BT_TLX */

#if CONFIG_BT_B9X
#include <b9x_bt.h>
#elif CONFIG_BT_TLX
#include <tlx_bt.h>
#endif /* CONFIG_BT_B9X */

/**
 * @brief     This function sets Telink MCU to suspend mode
 * @param[in] wake_stimer_tick - wake-up stimer tick
 * @return    true if suspend mode entered otherwise false
 */
bool tl_suspend(uint32_t wake_stimer_tick)
{
	bool result = false;

#if (CONFIG_BT_B9X || CONFIG_BT_TLX)
	enum tl_bt_controller_state state = tl_bt_controller_state();

	if (state == TL_BT_CONTROLLER_STATE_ACTIVE ||
		state == TL_BT_CONTROLLER_STATE_STOPPING) {
		blc_pm_setAppWakeupLowPower(wake_stimer_tick, 1);
		if (!blc_pm_handler()) {
			rf_set_power_level(tl_tx_pwr_lt[CONFIG_TL_BLE_CTRL_RF_POWER
			- TL_TX_POWER_MIN]);
			result = true;
		}
		blc_pm_setAppWakeupLowPower(0, 0);
	} else {
		if (cpu_sleep_wakeup_32k_rc(SUSPEND_MODE, PM_WAKEUP_TIMER | PM_WAKEUP_PAD,
			wake_stimer_tick) != STATUS_GPIO_ERR_NO_ENTER_PM) {
			result = true;
		}
	}
#else
	if (cpu_sleep_wakeup_32k_rc(SUSPEND_MODE, PM_WAKEUP_TIMER | PM_WAKEUP_PAD,
		wake_stimer_tick) != STATUS_GPIO_ERR_NO_ENTER_PM) {
		result = true;
	}
#endif /* CONFIG_BT_B9X || CONFIG_BT_TL */

	return result;
}

#if (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION || \
CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION)

#if CONFIG_SOC_RISCV_TELINK_B91
#define DEEPSLEEP_MODE_RET_SRAM DEEPSLEEP_MODE_RET_SRAM_LOW64K
#elif CONFIG_SOC_RISCV_TELINK_B92
#define DEEPSLEEP_MODE_RET_SRAM DEEPSLEEP_MODE_RET_SRAM_LOW96K
#elif CONFIG_SOC_RISCV_TELINK_B95
#define DEEPSLEEP_MODE_RET_SRAM DEEPSLEEP_MODE_RET_SRAM_LOW96K
#elif CONFIG_SOC_RISCV_TELINK_TL321X
#define DEEPSLEEP_MODE_RET_SRAM DEEPSLEEP_MODE_RET_SRAM_LOW96K
#endif

bool tl_deep_sleep(uint32_t wake_stimer_tick)
{
	bool result = false;
	static volatile bool tl_sleep_retention
	__attribute__ ((section (".retention_data"))) = false;

	extern void tl_context_save(void);
#if CONFIG_BT_B9X
	extern void soc_b9x_restore(void);
#elif CONFIG_BT_TLX
	extern void soc_tlx_restore(void);
#endif

#if (CONFIG_BT_B9X || CONFIG_BT_TLX)
	enum tl_bt_controller_state state = tl_bt_controller_state();

	if (state == TL_BT_CONTROLLER_STATE_ACTIVE ||
		state == TL_BT_CONTROLLER_STATE_STOPPING) {
		blc_pm_setAppWakeupLowPower(wake_stimer_tick, 1);
		if (!blc_pm_handler()) {
			rf_set_power_level(tl_tx_pwr_lt[CONFIG_TL_BLE_CTRL_RF_POWER
			- TL_TX_POWER_MIN]);
			result = true;
		}
		blc_pm_setAppWakeupLowPower(0, 0);
	} else {
		tl_context_save();
		if (!tl_sleep_retention) {
			tl_sleep_retention = true;
			(void)cpu_sleep_wakeup_32k_rc(DEEPSLEEP_MODE_RET_SRAM,
				PM_WAKEUP_TIMER | PM_WAKEUP_PAD,
				wake_stimer_tick);
			tl_sleep_retention = false;
		} else {
#if CONFIG_BT_B9X
			soc_b9x_restore();
#elif CONFIG_BT_TLX
			soc_tlx_restore();
#endif
			tl_sleep_retention = false;
			result = true;
		}
	}
#else
	tl_context_save();
	if (!tl_sleep_retention) {
		tl_sleep_retention = true;
		(void)cpu_sleep_wakeup_32k_rc(DEEPSLEEP_MODE_RET_SRAM,
			PM_WAKEUP_TIMER | PM_WAKEUP_PAD,
			wake_stimer_tick);
		tl_sleep_retention = false;
	} else {
#if CONFIG_BT_B9X
		soc_b9x_restore();
#elif CONFIG_BT_TLX
		soc_tlx_restore();
#endif
		tl_sleep_retention = false;
		result = true;
	}
#endif /* CONFIG_BT_B9X || CONFIG_BT_TLX */

	return result;
}

#endif /* (CONFIG_SOC_SERIES_RISCV_TELINK_B9X_RETENTION || \ */
/* CONFIG_SOC_SERIES_RISCV_TELINK_TLX_RETENTION) */
