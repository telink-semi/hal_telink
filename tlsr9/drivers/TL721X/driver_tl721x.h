/******************************************************************************
 * Copyright (c) 2024 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "lib/include/analog.h"
#include "lib/include/clock.h"
#include "lib/include/core.h"
#include "lib/include/emi.h"
#include "lib/include/flash_base.h"
#include "lib/include/mspi.h"
#include "lib/include/plic.h"
#include "lib/include/stimer.h"
#include "lib/include/sys.h"
#include "lib/include/pke/pke.h"
#include "lib/include/pm/pm.h"
#include "lib/include/rf/rf_common.h"
#include "lib/include/trng/trng.h"

#include "adc.h"
#include "audio.h"
#include "dma.h"
#include "flash.h"
#include "gpio.h"
#include "i2c.h"
#include "lpc.h"
#include "pwm.h"
#include "qdec.h"
#include "s7816.h"
#include "spi.h"
#include "timer.h"
#include "uart.h"
#include "usbhw.h"
#include "watchdog.h"
