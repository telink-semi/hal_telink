/******************************************************************************
 * Copyright (c) 2023-2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef B9X_BT_RF_POWER_H_
#define B9X_BT_RF_POWER_H_

#include "stdint.h"

#define B9X_TX_POWER_MIN                    (-30)
#define B9X_TX_POWER_MAX                    (9)
#define TLX_TX_POWER_MIN                    (-30)
#define TLX_TX_POWER_MAX                    (9)

extern const uint8_t b9x_tx_pwr_lt[];
extern const uint8_t tlx_tx_pwr_lt[];

#endif /* B9X_BT_RF_POWER_H_ */
