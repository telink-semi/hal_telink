/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#ifndef B9X_BT_FLASH_H_
#define B9X_BT_FLASH_H_

#include "stdint.h"
#include "compiler.h"

/* SOC Calibration Address Offset */
#define B9X_CALIBRATION_ADDR_OFFSET 0x0000

/* SOC BT MAC Address Offset */
#define B9X_BT_MAC_ADDR_OFFSET 0x1000

_attribute_no_inline_ int b9x_bt_blc_mac_init(unsigned char *bt_mac);

#endif
