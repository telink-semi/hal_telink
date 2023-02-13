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
#include "b91_sleep.h"
#include <ext_driver/ext_pm.h>
#include <stack/ble/controller/os_sup.h>
#include <stack/ble/controller/ll/ll.h>
#include <stack/ble/controller/ll/ll_stack.h>
#include <stack/ble/controller/ll/ll_pm.h>

extern bool b91_bt_controller_started;

/**
 * @brief     This function sets B91 MCU to suspend mode
 * @param[in] wake_stimer_tick - wake-up stimer tick
 * @return    true if suspend mode entered otherwise false
 */
bool b91_suspend(uint32_t wake_stimer_tick)
{
	bool result = false;

#if CONFIG_BT
	blc_pm_setAppWakeupLowPower(wake_stimer_tick, 1);

	if (!b91_bt_controller_started) {
		blmsPm.sleep_allowed = 1;
		blmsParam.sdk_mainloop_flg = 1;
		blmsPm.next_task_tick = wake_stimer_tick;
		
		if (blmsPm.sleep_mask == PM_SLEEP_DISABLE) {	
			blc_pm_setSleepMask(PM_SLEEP_LEG_ADV | PM_SLEEP_LEG_SCAN | PM_SLEEP_ACL_SLAVE | PM_SLEEP_ACL_MASTER);
		}

		if (ll_module_pm_cb == NULL) {
			u8  mac_public[6];
			blc_ll_initPowerManagement_module();
			blc_ll_initStandby_module(mac_public);
		}
	}

	if (!blc_pm_handler()) {
		result = true;
	}
	blc_pm_setAppWakeupLowPower(0, 0);
#else
	cpu_sleep_wakeup_32k_rc(SUSPEND_MODE, PM_WAKEUP_TIMER, wake_stimer_tick);
	result = true;
#endif /* CONFIG_BT */

	return result;
}
