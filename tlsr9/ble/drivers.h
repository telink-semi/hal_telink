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
#pragma once

#if CONFIG_SOC_RISCV_TELINK_B91
#include "../drivers/B91/driver_b91.h"
#include "../drivers/B91/ext_driver/driver_ext.h"
#elif CONFIG_SOC_RISCV_TELINK_B92
#include "../drivers/B92/driver_b92.h"
#include "../drivers/B92/ext_driver/driver_ext.h"
#endif
#include "../common/types.h"
