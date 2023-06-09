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

/**
 * @brief     This function sets B91 MCU to suspend mode
 * @param[in] wake_stimer_tick - wake-up stimer tick
 * @return    true if suspend mode entered otherwise false
 */
bool b9x_suspend(uint32_t wake_stimer_tick)
{
	bool result = false;
	if (pm_sleep_wakeup(SUSPEND_MODE, PM_WAKEUP_TIMER | PM_WAKEUP_PAD, PM_TICK_STIMER, wake_stimer_tick) != STATUS_GPIO_ERR_NO_ENTER_PM) {
		result = true;
	}
	return result;
}
