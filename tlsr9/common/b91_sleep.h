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

/********************************************************************************************************
 * @file	b91_sleep.h
 *
 * @brief	This is the header file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include <stdint.h>
#include <stdbool.h>

#ifndef __B91_SLEEP_H
#define __B91_SLEEP_H

bool b91_suspend(uint32_t wake_stimer_tick);

#endif /* __B91_SLEEP_H */
