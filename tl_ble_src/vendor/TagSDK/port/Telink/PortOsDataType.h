/* ***************************************************************************
 *
 * Copyright 2021 Samsung Electronics All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific
 * language governing permissions and limitations under the License.
 *
 ****************************************************************************/

#ifndef TAGSDK_PORT_OS_DATATYPE_H_
#define TAGSDK_PORT_OS_DATATYPE_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Timer handle
 */
typedef void* PortTimerHandle_t;

/**
 * @brief Queue handle
 */
typedef void* PortQueueHandle_t;

/**
 * @brief Event group handle
 */
typedef void* PortEventGroupHandle_t;

/**
 * @brief Task Handle
 */
typedef void* PortTaskHandle_t;

/**
 * @brief Tick type
 */
typedef uint32_t PortTickType_t;

/**
 * @brief Convert milliseconds to ticks
 */
#define CONV_MS_TO_TICKS(x)    (x/2)

/**
 * @brief Maximum delay in ticks
 */
#define PORT_MAX_DELAY  ((uint32_t)0xffffffffUL)

/**
 * @brief Tick frequency
 */
#define PORT_TICK_RATE_HZ       (500)

#endif /* TAGSDK_PORT_OS_DATATYPE_H_ */
