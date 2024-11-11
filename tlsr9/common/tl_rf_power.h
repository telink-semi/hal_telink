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

#ifndef TL_RF_POWER_H_
#define TL_RF_POWER_H_

#include "stdint.h"

#if CONFIG_SOC_RISCV_TELINK_TL321X
#define TL_TX_POWER_MIN                    (-19)
#define TL_TX_POWER_MAX                    (31)
#elif CONFIG_SOC_RISCV_TELINK_TL721X
#define TL_TX_POWER_MIN                    (-23)
#define TL_TX_POWER_MAX                    (26)
#else
#define TL_TX_POWER_MIN                    (-30)
#define TL_TX_POWER_MAX                    (9)
#endif

extern const uint8_t tl_tx_pwr_lt[];

#endif /* TL_RF_POWER_H_ */
