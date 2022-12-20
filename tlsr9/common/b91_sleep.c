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
