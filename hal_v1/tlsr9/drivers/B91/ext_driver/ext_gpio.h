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

#ifndef DRIVERS_B91_EXT_GPIO_H_
#define DRIVERS_B91_EXT_GPIO_H_

#include "compiler.h"

#include "../analog.h"
#include "../dma.h"
#include "../gpio.h"
#include "../pm.h"
#include "../timer.h"
#include "../flash.h"
#include "../mdec.h"
#include "../trng.h"
#include "../sys.h"
#include "../plic.h"
#include "../stimer.h"
#include "../clock.h"
#include "../compatibility_pack/cmpt.h"

/**
 * @brief     This function read a pin's cache from the buffer.
 * @param[in] pin - the pin needs to read.
 * @param[in] p - the buffer from which to read the pin's level.
 * @return    the state of the pin.
 */
static inline unsigned int gpio_read_cache(gpio_pin_e pin, unsigned char *p)
{
	return p[pin>>8] & (pin & 0xff);
}

/**
 * @brief      This function read all the pins' input level.
 * @param[out] p - the buffer used to store all the pins' input level
 * @return     none
 */
static inline void gpio_read_all(unsigned char *p)
{
	p[0] = REG_ADDR8(0x140300);
	p[1] = REG_ADDR8(0x140308);
	p[2] = REG_ADDR8(0x140310);
	p[3] = REG_ADDR8(0x140318);
	p[4] = REG_ADDR8(0x140320);
}

/**
 *  @brief  Define pull up or down types
 */
typedef enum {
	PM_PIN_UP_DOWN_FLOAT    = 0,
	PM_PIN_PULLUP_1M     	= 1,
	PM_PIN_PULLDOWN_100K  	= 2,
	PM_PIN_PULLUP_10K 		= 3,
}gpio_pull_type;

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] gpio - the pin needs to set its pull-up/down resistor
 * @param[in] up_down - the type of the pull-up/down resistor
 * @return    none
 */
void gpio_setup_up_down_resistor(gpio_pin_e gpio, gpio_pull_type up_down);



#endif /* DRIVERS_B91_EXT_GPIO_H_ */
