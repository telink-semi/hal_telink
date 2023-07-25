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

#ifndef OS_SUP_H_
#define OS_SUP_H_

#include "stack/ble/B92/ble_config.h"


typedef void (*os_give_sem_t)(void);

/* User API */
void blc_setOsSupEnable(bool en);
bool blc_isOsSupEnable(void);
bool blc_isBleSchedulerBusy(void);
void blc_ll_registerGiveSemCb(os_give_sem_t give_sem_from_isr, os_give_sem_t give_sem);
extern int blc_pm_handler(void);
extern void blc_pm_setAppWakeupLowPower(u32 wakeup_tick, u8 enable);

/* Stack API. !!! user can't use. */
extern void blt_ll_sem_give(void);

#endif /* OS_SUP_H_ */
