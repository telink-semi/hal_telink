/********************************************************************************************************
 * @file    tl_common.h
 *
 * @brief   This is the header file for tl_common
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#pragma once


// #include "app_cfg.h"
// #include "platform.h"

#include "../proj/common/types.h"
#include "../proj/common/compiler.h"
#include "../proj/common/static_assert.h"
#include "../proj/common/assert.h"
#include "../proj/common/bit.h"
#include "../proj/common/utility.h"
#include "../proj/common/utlist.h"
#include "../proj/common/list.h"
#include "../proj/common/string.h"
#include "../proj/common/tlPrintf.h"
#include "../proj/common/mempool.h"

// #include "../proj/os/ev_poll.h"
#include "../proj/os/ev_buffer.h"
#include "../proj/os/ev_queue.h"
#include "../proj/os/ev_rtc.h"
#include "../proj/os/ev_timer.h"
#include "../proj/os/ev.h"


// #include "drivers/drv_hw.h"
// #include "drivers/drv_radio.h"
// #include "drivers/drv_gpio.h"
// #include "drivers/drv_adc.h"
// #include "./drivers/drv_flash.h"
// #include "drivers/drv_i2c.h"
// #include "drivers/drv_spi.h"
// #include "drivers/drv_pwm.h"
// #include "drivers/drv_uart.h"
// #include "drivers/drv_pm.h"
// #include "drivers/drv_timer.h"
// #include "drivers/drv_keyboard.h"
// #include "./drivers/multi_core/mailbox_service.h"
// #include "./drivers/multi_core/mailbox.h"
// #include "./drivers/multi_core/share_memory.h"
// #include "./drivers/drv_calibration.h"
// #include "./drivers/drv_nv.h"
// #include "drivers/drv_putchar.h"
// #include "drivers/drv_usb.h"
// #include "drivers/drv_security.h"
// #include "drivers/drv_console.h"
