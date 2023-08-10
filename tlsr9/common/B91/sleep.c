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
#include "b9x_sleep.h"
#include <ext_driver/ext_pm.h>

#if CONFIG_BT_B91
#include <stack/ble/controller/os_sup.h>
#include <b91_bt.h>
#endif /* CONFIG_BT_B91 */

/**
 * @brief     This function sets B91 MCU to suspend mode
 * @param[in] wake_stimer_tick - wake-up stimer tick
 * @return    true if suspend mode entered otherwise false
 */
bool b9x_suspend(uint32_t wake_stimer_tick)
{
	bool result = false;

#if CONFIG_BT_B91
	enum b91_bt_controller_state state = b91_bt_controller_state();

	if (state == B91_BT_CONTROLLER_STATE_ACTIVE ||
		state == B91_BT_CONTROLLER_STATE_STOPPING) {
		blc_pm_setAppWakeupLowPower(wake_stimer_tick, 1);
		if (!blc_pm_handler()) {
			rf_set_power_level_index(CONFIG_B91_BLE_CTRL_RF_POWER);
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
#endif /* CONFIG_BT_B91 */

	return result;
}

#ifdef CONFIG_BOARD_TLSR9518ADK80D_RETENTION

bool b91_deep_sleep(uint32_t wake_stimer_tick)
{
	bool result = false;
	static volatile bool tl_sleep_retention
	__attribute__ ((section (".retention_data"))) = false;

	extern void tl_context_save(void);
	extern void soc_b9x_restore(void);

#if CONFIG_BT_B91
	enum b91_bt_controller_state state = b91_bt_controller_state();

	if (state == B91_BT_CONTROLLER_STATE_ACTIVE ||
		state == B91_BT_CONTROLLER_STATE_STOPPING) {
		blc_pm_setAppWakeupLowPower(wake_stimer_tick, 1);
		if (!blc_pm_handler()) {
			rf_set_power_level_index(CONFIG_B91_BLE_CTRL_RF_POWER);
			result = true;
		}
		blc_pm_setAppWakeupLowPower(0, 0);
	} else {
		tl_context_save();
		if (!tl_sleep_retention) {
			tl_sleep_retention = true;
			(void)cpu_sleep_wakeup_32k_rc(DEEPSLEEP_MODE_RET_SRAM_LOW64K,
				PM_WAKEUP_TIMER | PM_WAKEUP_PAD,
				wake_stimer_tick);
			tl_sleep_retention = false;
		} else {
			soc_b9x_restore();
			tl_sleep_retention = false;
			result = true;
		}
	}
#else
	tl_context_save();
	if (!tl_sleep_retention) {
		tl_sleep_retention = true;
		(void)cpu_sleep_wakeup_32k_rc(DEEPSLEEP_MODE_RET_SRAM_LOW64K,
			PM_WAKEUP_TIMER | PM_WAKEUP_PAD,
			wake_stimer_tick);
		tl_sleep_retention = false;
	} else {
		soc_b9x_restore();
		tl_sleep_retention = false;
		result = true;
	}
#endif /* CONFIG_BT_B91 */

	return result;
}

#endif /* CONFIG_BOARD_TLSR9518ADK80D_RETENTION */
