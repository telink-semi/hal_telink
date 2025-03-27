/********************************************************************************************************
 * @file    gpio_default.h
 *
 * @brief   This is the header file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef DRIVERS_GPIO_DEFAULT_H_
#define DRIVERS_GPIO_DEFAULT_H_

#include "compiler.h"
#include "gpio.h"
/**********************************************************************************************************************
 *                                           GPIO   setting                                                            *
 *********************************************************************************************************************/
/************************************************************PA*******************************************************/
#ifndef PA0_INPUT_ENABLE
#define PA0_INPUT_ENABLE    0
#endif
#ifndef PA1_INPUT_ENABLE
#define PA1_INPUT_ENABLE    0
#endif
#ifndef PA2_INPUT_ENABLE
#define PA2_INPUT_ENABLE    0
#endif
#ifndef PA3_INPUT_ENABLE
#define PA3_INPUT_ENABLE    0
#endif
#ifndef PA4_INPUT_ENABLE
#define PA4_INPUT_ENABLE    0
#endif
#ifndef PA5_INPUT_ENABLE
#define PA5_INPUT_ENABLE    0
#endif
#ifndef PA6_INPUT_ENABLE
#define PA6_INPUT_ENABLE    0
#endif
#ifndef PA7_INPUT_ENABLE
#define PA7_INPUT_ENABLE    1   //SWS
#endif
#ifndef PA0_OUTPUT_ENABLE
#define PA0_OUTPUT_ENABLE   0
#endif
#ifndef PA1_OUTPUT_ENABLE
#define PA1_OUTPUT_ENABLE   0
#endif
#ifndef PA2_OUTPUT_ENABLE
#define PA2_OUTPUT_ENABLE   0
#endif
#ifndef PA3_OUTPUT_ENABLE
#define PA3_OUTPUT_ENABLE   0
#endif
#ifndef PA4_OUTPUT_ENABLE
#define PA4_OUTPUT_ENABLE   0
#endif
#ifndef PA5_OUTPUT_ENABLE
#define PA5_OUTPUT_ENABLE   0
#endif
#ifndef PA6_OUTPUT_ENABLE
#define PA6_OUTPUT_ENABLE   0
#endif
#ifndef PA7_OUTPUT_ENABLE
#define PA7_OUTPUT_ENABLE   0
#endif
#ifndef PA0_DATA_STRENGTH
#define PA0_DATA_STRENGTH   1
#endif
#ifndef PA1_DATA_STRENGTH
#define PA1_DATA_STRENGTH   1
#endif
#ifndef PA2_DATA_STRENGTH
#define PA2_DATA_STRENGTH   1
#endif
#ifndef PA3_DATA_STRENGTH
#define PA3_DATA_STRENGTH   1
#endif
#ifndef PA4_DATA_STRENGTH
#define PA4_DATA_STRENGTH   1
#endif
#ifndef PA5_DATA_STRENGTH
#define PA5_DATA_STRENGTH   1
#endif
#ifndef PA6_DATA_STRENGTH
#define PA6_DATA_STRENGTH   1
#endif
#ifndef PA7_DATA_STRENGTH
#define PA7_DATA_STRENGTH   1
#endif
#ifndef PA0_DATA_OUT
#define PA0_DATA_OUT    0
#endif
#ifndef PA1_DATA_OUT
#define PA1_DATA_OUT    0
#endif
#ifndef PA2_DATA_OUT
#define PA2_DATA_OUT    0
#endif
#ifndef PA3_DATA_OUT
#define PA3_DATA_OUT    0
#endif
#ifndef PA4_DATA_OUT
#define PA4_DATA_OUT    0
#endif
#ifndef PA5_DATA_OUT
#define PA5_DATA_OUT    0
#endif
#ifndef PA6_DATA_OUT
#define PA6_DATA_OUT    0
#endif
#ifndef PA7_DATA_OUT
#define PA7_DATA_OUT    0
#endif
#ifndef PA0_FUNC
#define PA0_FUNC    AS_GPIO
#endif
#ifndef PA1_FUNC
#define PA1_FUNC    AS_GPIO
#endif
#ifndef PA2_FUNC
#define PA2_FUNC    AS_GPIO
#endif
#ifndef PA3_FUNC
#define PA3_FUNC    AS_GPIO
#endif
#ifndef PA4_FUNC
#define PA4_FUNC    AS_GPIO
#endif
#ifndef PA5_FUNC
#define PA5_FUNC    AS_GPIO
#endif
#ifndef PA6_FUNC
#define PA6_FUNC    AS_GPIO
#endif
#ifndef PA7_FUNC
#define PA7_FUNC    AS_SWS
#endif
#ifndef PULL_WAKEUP_SRC_PA0
#define PULL_WAKEUP_SRC_PA0 0
#endif
#ifndef PULL_WAKEUP_SRC_PA1
#define PULL_WAKEUP_SRC_PA1 0
#endif
#ifndef PULL_WAKEUP_SRC_PA2
#define PULL_WAKEUP_SRC_PA2 0
#endif
#ifndef PULL_WAKEUP_SRC_PA3
#define PULL_WAKEUP_SRC_PA3 0
#endif
#ifndef PULL_WAKEUP_SRC_PA4
#define PULL_WAKEUP_SRC_PA4 0
#endif
#ifndef PULL_WAKEUP_SRC_PA5
#define PULL_WAKEUP_SRC_PA5 0
#endif
#ifndef PULL_WAKEUP_SRC_PA6
#define PULL_WAKEUP_SRC_PA6 0
#endif
#ifndef PULL_WAKEUP_SRC_PA7
#define PULL_WAKEUP_SRC_PA7     GPIO_PIN_PULLUP_1M  //SWS
#endif

/************************************************************PB*******************************************************/
#ifndef PB0_INPUT_ENABLE
#define PB0_INPUT_ENABLE    0
#endif
#ifndef PB1_INPUT_ENABLE
#define PB1_INPUT_ENABLE    0   // D25&N22 TCK
#endif
#ifndef PB2_INPUT_ENABLE
#define PB2_INPUT_ENABLE    0   // D25&N22 TMS
#endif
#ifndef PB3_INPUT_ENABLE
#define PB3_INPUT_ENABLE    0   // D25&N22 TDO
#endif
#ifndef PB4_INPUT_ENABLE
#define PB4_INPUT_ENABLE    0   // D25&N22 TDI
#endif
#ifndef PB5_INPUT_ENABLE
#define PB5_INPUT_ENABLE    0
#endif
#ifndef PB6_INPUT_ENABLE
#define PB6_INPUT_ENABLE    0
#endif
#ifndef PB7_INPUT_ENABLE
#define PB7_INPUT_ENABLE    1
#endif
#ifndef PB0_OUTPUT_ENABLE
#define PB0_OUTPUT_ENABLE   0
#endif
#ifndef PB1_OUTPUT_ENABLE
#define PB1_OUTPUT_ENABLE   0
#endif
#ifndef PB2_OUTPUT_ENABLE
#define PB2_OUTPUT_ENABLE   0
#endif
#ifndef PB3_OUTPUT_ENABLE
#define PB3_OUTPUT_ENABLE   0
#endif
#ifndef PB4_OUTPUT_ENABLE
#define PB4_OUTPUT_ENABLE   0
#endif
#ifndef PB5_OUTPUT_ENABLE
#define PB5_OUTPUT_ENABLE   0
#endif
#ifndef PB6_OUTPUT_ENABLE
#define PB6_OUTPUT_ENABLE   0
#endif
#ifndef PB7_OUTPUT_ENABLE
#define PB7_OUTPUT_ENABLE   0
#endif
#ifndef PB0_DATA_STRENGTH
#define PB0_DATA_STRENGTH   1
#endif
#ifndef PB1_DATA_STRENGTH
#define PB1_DATA_STRENGTH   1
#endif
#ifndef PB2_DATA_STRENGTH
#define PB2_DATA_STRENGTH   1
#endif
#ifndef PB3_DATA_STRENGTH
#define PB3_DATA_STRENGTH   1
#endif
#ifndef PB4_DATA_STRENGTH
#define PB4_DATA_STRENGTH   1
#endif
#ifndef PB5_DATA_STRENGTH
#define PB5_DATA_STRENGTH   1
#endif
#ifndef PB6_DATA_STRENGTH
#define PB6_DATA_STRENGTH   1
#endif
#ifndef PB7_DATA_STRENGTH
#define PB7_DATA_STRENGTH   1
#endif
#ifndef PB0_DATA_OUT
#define PB0_DATA_OUT    0
#endif
#ifndef PB1_DATA_OUT
#define PB1_DATA_OUT    0
#endif
#ifndef PB2_DATA_OUT
#define PB2_DATA_OUT    0
#endif
#ifndef PB3_DATA_OUT
#define PB3_DATA_OUT    0
#endif
#ifndef PB4_DATA_OUT
#define PB4_DATA_OUT    0
#endif
#ifndef PB5_DATA_OUT
#define PB5_DATA_OUT    0
#endif
#ifndef PB6_DATA_OUT
#define PB6_DATA_OUT    0
#endif
#ifndef PB7_DATA_OUT
#define PB7_DATA_OUT    0
#endif
#ifndef PB0_FUNC
#define PB0_FUNC    AS_GPIO
#endif
#ifndef PB1_FUNC
#define PB1_FUNC    AS_TCK  // D25&N22 TCK
#endif
#ifndef PB2_FUNC
#define PB2_FUNC    AS_TMS  // D25&N22 TMS
#endif
#ifndef PB3_FUNC
#define PB3_FUNC    AS_TDO  // D25&N22 TDO
#endif
#ifndef PB4_FUNC
#define PB4_FUNC    AS_TDI  // D25&N22 TDI
#endif
#ifndef PB5_FUNC
#define PB5_FUNC    AS_GPIO
#endif
#ifndef PB6_FUNC
#define PB6_FUNC    AS_GPIO
#endif
#ifndef PB7_FUNC
#define PB7_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PB0
#define PULL_WAKEUP_SRC_PB0 0
#endif
#ifndef PULL_WAKEUP_SRC_PB1
#define PULL_WAKEUP_SRC_PB1 GPIO_PIN_PULLUP_1M  // D25&N22 TCK
#endif
#ifndef PULL_WAKEUP_SRC_PB2
#define PULL_WAKEUP_SRC_PB2 GPIO_PIN_PULLUP_1M  // D25&N22 TMS
#endif
#ifndef PULL_WAKEUP_SRC_PB3
#define PULL_WAKEUP_SRC_PB3 GPIO_PIN_PULLUP_1M  // D25&N22 TDO
#endif
#ifndef PULL_WAKEUP_SRC_PB4
#define PULL_WAKEUP_SRC_PB4 GPIO_PIN_PULLUP_1M  // D25&N22 TDI
#endif
#ifndef PULL_WAKEUP_SRC_PB5
#define PULL_WAKEUP_SRC_PB5 0
#endif
#ifndef PULL_WAKEUP_SRC_PB6
#define PULL_WAKEUP_SRC_PB6 0
#endif
#ifndef PULL_WAKEUP_SRC_PB7
#define PULL_WAKEUP_SRC_PB7 0
#endif

/************************************************************PC*******************************************************/
#ifndef PC0_INPUT_ENABLE
#define PC0_INPUT_ENABLE    0
#endif
#ifndef PC1_INPUT_ENABLE
#define PC1_INPUT_ENABLE    0
#endif
#ifndef PC2_INPUT_ENABLE
#define PC2_INPUT_ENABLE    0
#endif
#ifndef PC3_INPUT_ENABLE
#define PC3_INPUT_ENABLE    0
#endif
#ifndef PC4_INPUT_ENABLE
#define PC4_INPUT_ENABLE    0
#endif
#ifndef PC5_INPUT_ENABLE
#define PC5_INPUT_ENABLE    0
#endif
#ifndef PC6_INPUT_ENABLE
#define PC6_INPUT_ENABLE    0
#endif
#ifndef PC7_INPUT_ENABLE
#define PC7_INPUT_ENABLE    0
#endif
#ifndef PC0_OUTPUT_ENABLE
#define PC0_OUTPUT_ENABLE   0
#endif
#ifndef PC1_OUTPUT_ENABLE
#define PC1_OUTPUT_ENABLE   0
#endif
#ifndef PC2_OUTPUT_ENABLE
#define PC2_OUTPUT_ENABLE   0
#endif
#ifndef PC3_OUTPUT_ENABLE
#define PC3_OUTPUT_ENABLE   0
#endif
#ifndef PC4_OUTPUT_ENABLE
#define PC4_OUTPUT_ENABLE   0
#endif
#ifndef PC5_OUTPUT_ENABLE
#define PC5_OUTPUT_ENABLE   0
#endif
#ifndef PC6_OUTPUT_ENABLE
#define PC6_OUTPUT_ENABLE   0
#endif
#ifndef PC7_OUTPUT_ENABLE
#define PC7_OUTPUT_ENABLE   0
#endif
#ifndef PC0_DATA_STRENGTH
#define PC0_DATA_STRENGTH   1
#endif
#ifndef PC1_DATA_STRENGTH
#define PC1_DATA_STRENGTH   1
#endif
#ifndef PC2_DATA_STRENGTH
#define PC2_DATA_STRENGTH   1
#endif
#ifndef PC3_DATA_STRENGTH
#define PC3_DATA_STRENGTH   1
#endif
#ifndef PC4_DATA_STRENGTH
#define PC4_DATA_STRENGTH   1
#endif
#ifndef PC5_DATA_STRENGTH
#define PC5_DATA_STRENGTH   1
#endif
#ifndef PC6_DATA_STRENGTH
#define PC6_DATA_STRENGTH   1
#endif
#ifndef PC7_DATA_STRENGTH
#define PC7_DATA_STRENGTH   1
#endif
#ifndef PC0_DATA_OUT
#define PC0_DATA_OUT    0
#endif
#ifndef PC1_DATA_OUT
#define PC1_DATA_OUT    0
#endif
#ifndef PC2_DATA_OUT
#define PC2_DATA_OUT    0
#endif
#ifndef PC3_DATA_OUT
#define PC3_DATA_OUT    0
#endif
#ifndef PC4_DATA_OUT
#define PC4_DATA_OUT    0
#endif
#ifndef PC5_DATA_OUT
#define PC5_DATA_OUT    0
#endif
#ifndef PC6_DATA_OUT
#define PC6_DATA_OUT    0
#endif
#ifndef PC7_DATA_OUT
#define PC7_DATA_OUT    0
#endif
#ifndef PC0_FUNC
#define PC0_FUNC    AS_GPIO
#endif
#ifndef PC1_FUNC
#define PC1_FUNC    AS_GPIO
#endif
#ifndef PC2_FUNC
#define PC2_FUNC    AS_GPIO
#endif
#ifndef PC3_FUNC
#define PC3_FUNC    AS_GPIO
#endif
#ifndef PC4_FUNC
#define PC4_FUNC    AS_GPIO
#endif
#ifndef PC5_FUNC
#define PC5_FUNC    AS_GPIO
#endif
#ifndef PC6_FUNC
#define PC6_FUNC    AS_GPIO
#endif
#ifndef PC7_FUNC
#define PC7_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PC0
#define PULL_WAKEUP_SRC_PC0 0
#endif
#ifndef PULL_WAKEUP_SRC_PC1
#define PULL_WAKEUP_SRC_PC1 0
#endif
#ifndef PULL_WAKEUP_SRC_PC2
#define PULL_WAKEUP_SRC_PC2 0
#endif
#ifndef PULL_WAKEUP_SRC_PC3
#define PULL_WAKEUP_SRC_PC3 0
#endif
#ifndef PULL_WAKEUP_SRC_PC4
#define PULL_WAKEUP_SRC_PC4 0
#endif
#ifndef PULL_WAKEUP_SRC_PC5
#define PULL_WAKEUP_SRC_PC5 0
#endif
#ifndef PULL_WAKEUP_SRC_PC6
#define PULL_WAKEUP_SRC_PC6 0
#endif
#ifndef PULL_WAKEUP_SRC_PC7
#define PULL_WAKEUP_SRC_PC7 0
#endif

/************************************************************PD*******************************************************/
#ifndef PD0_INPUT_ENABLE
#define PD0_INPUT_ENABLE    0
#endif
#ifndef PD1_INPUT_ENABLE
#define PD1_INPUT_ENABLE    0
#endif
#ifndef PD2_INPUT_ENABLE
#define PD2_INPUT_ENABLE    0
#endif
#ifndef PD3_INPUT_ENABLE
#define PD3_INPUT_ENABLE    0
#endif
#ifndef PD4_INPUT_ENABLE
#define PD4_INPUT_ENABLE    0
#endif
#ifndef PD5_INPUT_ENABLE
#define PD5_INPUT_ENABLE    0
#endif
#ifndef PD6_INPUT_ENABLE
#define PD6_INPUT_ENABLE    0
#endif
#ifndef PD7_INPUT_ENABLE
#define PD7_INPUT_ENABLE    0
#endif
#ifndef PD0_OUTPUT_ENABLE
#define PD0_OUTPUT_ENABLE   0
#endif
#ifndef PD1_OUTPUT_ENABLE
#define PD1_OUTPUT_ENABLE   0
#endif
#ifndef PD2_OUTPUT_ENABLE
#define PD2_OUTPUT_ENABLE   0
#endif
#ifndef PD3_OUTPUT_ENABLE
#define PD3_OUTPUT_ENABLE   0
#endif
#ifndef PD4_OUTPUT_ENABLE
#define PD4_OUTPUT_ENABLE   0
#endif
#ifndef PD5_OUTPUT_ENABLE
#define PD5_OUTPUT_ENABLE   0
#endif
#ifndef PD6_OUTPUT_ENABLE
#define PD6_OUTPUT_ENABLE   0
#endif
#ifndef PD7_OUTPUT_ENABLE
#define PD7_OUTPUT_ENABLE   0
#endif
#ifndef PD0_DATA_STRENGTH
#define PD0_DATA_STRENGTH   1
#endif
#ifndef PD1_DATA_STRENGTH
#define PD1_DATA_STRENGTH   1
#endif
#ifndef PD2_DATA_STRENGTH
#define PD2_DATA_STRENGTH   1
#endif
#ifndef PD3_DATA_STRENGTH
#define PD3_DATA_STRENGTH   1
#endif
#ifndef PD4_DATA_STRENGTH
#define PD4_DATA_STRENGTH   1
#endif
#ifndef PD5_DATA_STRENGTH
#define PD5_DATA_STRENGTH   1
#endif
#ifndef PD6_DATA_STRENGTH
#define PD6_DATA_STRENGTH   1
#endif
#ifndef PD7_DATA_STRENGTH
#define PD7_DATA_STRENGTH   1
#endif
#ifndef PD0_DATA_OUT
#define PD0_DATA_OUT    0
#endif
#ifndef PD1_DATA_OUT
#define PD1_DATA_OUT    0
#endif
#ifndef PD2_DATA_OUT
#define PD2_DATA_OUT    0
#endif
#ifndef PD3_DATA_OUT
#define PD3_DATA_OUT    0
#endif
#ifndef PD4_DATA_OUT
#define PD4_DATA_OUT    0
#endif
#ifndef PD5_DATA_OUT
#define PD5_DATA_OUT    0
#endif
#ifndef PD6_DATA_OUT
#define PD6_DATA_OUT    0
#endif
#ifndef PD7_DATA_OUT
#define PD7_DATA_OUT    0
#endif
#ifndef PD0_FUNC
#define PD0_FUNC    AS_GPIO
#endif
#ifndef PD1_FUNC
#define PD1_FUNC    AS_GPIO
#endif
#ifndef PD2_FUNC
#define PD2_FUNC    AS_GPIO
#endif
#ifndef PD3_FUNC
#define PD3_FUNC    AS_GPIO
#endif
#ifndef PD4_FUNC
#define PD4_FUNC    AS_GPIO
#endif
#ifndef PD5_FUNC
#define PD5_FUNC    AS_GPIO
#endif
#ifndef PD6_FUNC
#define PD6_FUNC    AS_GPIO
#endif
#ifndef PD7_FUNC
#define PD7_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PD0
#define PULL_WAKEUP_SRC_PD0 0
#endif
#ifndef PULL_WAKEUP_SRC_PD1
#define PULL_WAKEUP_SRC_PD1 0
#endif
#ifndef PULL_WAKEUP_SRC_PD2
#define PULL_WAKEUP_SRC_PD2 0
#endif
#ifndef PULL_WAKEUP_SRC_PD3
#define PULL_WAKEUP_SRC_PD3 0
#endif
#ifndef PULL_WAKEUP_SRC_PD4
#define PULL_WAKEUP_SRC_PD4 0
#endif
#ifndef PULL_WAKEUP_SRC_PD5
#define PULL_WAKEUP_SRC_PD5 0
#endif
#ifndef PULL_WAKEUP_SRC_PD6
#define PULL_WAKEUP_SRC_PD6 0
#endif
#ifndef PULL_WAKEUP_SRC_PD7
#define PULL_WAKEUP_SRC_PD7 0
#endif

/************************************************************PE*******************************************************/
#ifndef PE0_INPUT_ENABLE
#define PE0_INPUT_ENABLE    0
#endif
#ifndef PE1_INPUT_ENABLE
#define PE1_INPUT_ENABLE    0
#endif
#ifndef PE2_INPUT_ENABLE
#define PE2_INPUT_ENABLE    0
#endif
#ifndef PE3_INPUT_ENABLE
#define PE3_INPUT_ENABLE    0
#endif
#ifndef PE4_INPUT_ENABLE
#define PE4_INPUT_ENABLE    0
#endif
#ifndef PE5_INPUT_ENABLE
#define PE5_INPUT_ENABLE    0
#endif
#ifndef PE6_INPUT_ENABLE
#define PE6_INPUT_ENABLE    0
#endif
#ifndef PE7_INPUT_ENABLE
#define PE7_INPUT_ENABLE    0
#endif
#ifndef PE0_OUTPUT_ENABLE
#define PE0_OUTPUT_ENABLE   0
#endif
#ifndef PE1_OUTPUT_ENABLE
#define PE1_OUTPUT_ENABLE   0
#endif
#ifndef PE2_OUTPUT_ENABLE
#define PE2_OUTPUT_ENABLE   0
#endif
#ifndef PE3_OUTPUT_ENABLE
#define PE3_OUTPUT_ENABLE   0
#endif
#ifndef PE4_OUTPUT_ENABLE
#define PE4_OUTPUT_ENABLE   0
#endif
#ifndef PE5_OUTPUT_ENABLE
#define PE5_OUTPUT_ENABLE   0
#endif
#ifndef PE6_OUTPUT_ENABLE
#define PE6_OUTPUT_ENABLE   0
#endif
#ifndef PE7_OUTPUT_ENABLE
#define PE7_OUTPUT_ENABLE   0
#endif
#ifndef PE0_DATA_STRENGTH
#define PE0_DATA_STRENGTH   1
#endif
#ifndef PE1_DATA_STRENGTH
#define PE1_DATA_STRENGTH   1
#endif
#ifndef PE2_DATA_STRENGTH
#define PE2_DATA_STRENGTH   1
#endif
#ifndef PE3_DATA_STRENGTH
#define PE3_DATA_STRENGTH   1
#endif
#ifndef PE4_DATA_STRENGTH
#define PE4_DATA_STRENGTH   1
#endif
#ifndef PE5_DATA_STRENGTH
#define PE5_DATA_STRENGTH   1
#endif
#ifndef PE6_DATA_STRENGTH
#define PE6_DATA_STRENGTH   1
#endif
#ifndef PE7_DATA_STRENGTH
#define PE7_DATA_STRENGTH   1
#endif
#ifndef PE0_DATA_OUT
#define PE0_DATA_OUT    0
#endif
#ifndef PE1_DATA_OUT
#define PE1_DATA_OUT    0
#endif
#ifndef PE2_DATA_OUT
#define PE2_DATA_OUT    0
#endif
#ifndef PE3_DATA_OUT
#define PE3_DATA_OUT    0
#endif
#ifndef PE4_DATA_OUT
#define PE4_DATA_OUT    0
#endif
#ifndef PE5_DATA_OUT
#define PE5_DATA_OUT    0
#endif
#ifndef PE6_DATA_OUT
#define PE6_DATA_OUT    0
#endif
#ifndef PE7_DATA_OUT
#define PE7_DATA_OUT    0
#endif
#ifndef PE0_FUNC
#define PE0_FUNC    AS_GPIO
#endif
#ifndef PE1_FUNC
#define PE1_FUNC    AS_GPIO
#endif
#ifndef PE2_FUNC
#define PE2_FUNC    AS_GPIO
#endif
#ifndef PE3_FUNC
#define PE3_FUNC    AS_GPIO
#endif
#ifndef PE4_FUNC
#define PE4_FUNC    AS_GPIO
#endif
#ifndef PE5_FUNC
#define PE5_FUNC    AS_GPIO
#endif
#ifndef PE6_FUNC
#define PE6_FUNC    AS_GPIO
#endif
#ifndef PE7_FUNC
#define PE7_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PE0
#define PULL_WAKEUP_SRC_PE0 0
#endif
#ifndef PULL_WAKEUP_SRC_PE1
#define PULL_WAKEUP_SRC_PE1 0
#endif
#ifndef PULL_WAKEUP_SRC_PE2
#define PULL_WAKEUP_SRC_PE2 0
#endif
#ifndef PULL_WAKEUP_SRC_PE3
#define PULL_WAKEUP_SRC_PE3 0
#endif
#ifndef PULL_WAKEUP_SRC_PE4
#define PULL_WAKEUP_SRC_PE4 0
#endif
#ifndef PULL_WAKEUP_SRC_PE5
#define PULL_WAKEUP_SRC_PE5 0
#endif
#ifndef PULL_WAKEUP_SRC_PE6
#define PULL_WAKEUP_SRC_PE6 0
#endif
#ifndef PULL_WAKEUP_SRC_PE7
#define PULL_WAKEUP_SRC_PE7 0
#endif

/************************************************************PF*******************************************************/
#ifndef PF0_INPUT_ENABLE
#define PF0_INPUT_ENABLE    0
#endif
#ifndef PF1_INPUT_ENABLE
#define PF1_INPUT_ENABLE    0
#endif
#ifndef PF2_INPUT_ENABLE
#define PF2_INPUT_ENABLE    0
#endif
#ifndef PF3_INPUT_ENABLE
#define PF3_INPUT_ENABLE    0
#endif
#ifndef PF4_INPUT_ENABLE
#define PF4_INPUT_ENABLE    0
#endif
#ifndef PF5_INPUT_ENABLE
#define PF5_INPUT_ENABLE    0
#endif
#ifndef PF6_INPUT_ENABLE
#define PF6_INPUT_ENABLE    0
#endif
#ifndef PF7_INPUT_ENABLE
#define PF7_INPUT_ENABLE    0
#endif
#ifndef PF0_OUTPUT_ENABLE
#define PF0_OUTPUT_ENABLE   0
#endif
#ifndef PF1_OUTPUT_ENABLE
#define PF1_OUTPUT_ENABLE   0
#endif
#ifndef PF2_OUTPUT_ENABLE
#define PF2_OUTPUT_ENABLE   0
#endif
#ifndef PF3_OUTPUT_ENABLE
#define PF3_OUTPUT_ENABLE   0
#endif
#ifndef PF4_OUTPUT_ENABLE
#define PF4_OUTPUT_ENABLE   0
#endif
#ifndef PF5_OUTPUT_ENABLE
#define PF5_OUTPUT_ENABLE   0
#endif
#ifndef PF6_OUTPUT_ENABLE
#define PF6_OUTPUT_ENABLE   0
#endif
#ifndef PF7_OUTPUT_ENABLE
#define PF7_OUTPUT_ENABLE   0
#endif
#ifndef PF0_DATA_STRENGTH
#define PF0_DATA_STRENGTH   1
#endif
#ifndef PF1_DATA_STRENGTH
#define PF1_DATA_STRENGTH   1
#endif
#ifndef PF2_DATA_STRENGTH
#define PF2_DATA_STRENGTH   1
#endif
#ifndef PF3_DATA_STRENGTH
#define PF3_DATA_STRENGTH   1
#endif
#ifndef PF4_DATA_STRENGTH
#define PF4_DATA_STRENGTH   1
#endif
#ifndef PF5_DATA_STRENGTH
#define PF5_DATA_STRENGTH   1
#endif
#ifndef PF6_DATA_STRENGTH
#define PF6_DATA_STRENGTH   1
#endif
#ifndef PF7_DATA_STRENGTH
#define PF7_DATA_STRENGTH   1
#endif
#ifndef PF0_DATA_OUT
#define PF0_DATA_OUT    0
#endif
#ifndef PF1_DATA_OUT
#define PF1_DATA_OUT    0
#endif
#ifndef PF2_DATA_OUT
#define PF2_DATA_OUT    0
#endif
#ifndef PF3_DATA_OUT
#define PF3_DATA_OUT    0
#endif
#ifndef PF4_DATA_OUT
#define PF4_DATA_OUT    0
#endif
#ifndef PF5_DATA_OUT
#define PF5_DATA_OUT    0
#endif
#ifndef PF6_DATA_OUT
#define PF6_DATA_OUT    0
#endif
#ifndef PF7_DATA_OUT
#define PF7_DATA_OUT    0
#endif
#ifndef PF0_FUNC
#define PF0_FUNC    AS_GPIO
#endif
#ifndef PF1_FUNC
#define PF1_FUNC    AS_GPIO
#endif
#ifndef PF2_FUNC
#define PF2_FUNC    AS_GPIO
#endif
#ifndef PF3_FUNC
#define PF3_FUNC    AS_GPIO
#endif
#ifndef PF4_FUNC
#define PF4_FUNC    AS_GPIO
#endif
#ifndef PF5_FUNC
#define PF5_FUNC    AS_GPIO
#endif
#ifndef PF6_FUNC
#define PF6_FUNC    AS_GPIO
#endif
#ifndef PF7_FUNC
#define PF7_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PF0
#define PULL_WAKEUP_SRC_PF0 0
#endif
#ifndef PULL_WAKEUP_SRC_PF1
#define PULL_WAKEUP_SRC_PF1 0
#endif
#ifndef PULL_WAKEUP_SRC_PF2
#define PULL_WAKEUP_SRC_PF2 0
#endif
#ifndef PULL_WAKEUP_SRC_PF3
#define PULL_WAKEUP_SRC_PF3 0
#endif
#ifndef PULL_WAKEUP_SRC_PF4
#define PULL_WAKEUP_SRC_PF4 0
#endif
#ifndef PULL_WAKEUP_SRC_PF5
#define PULL_WAKEUP_SRC_PF5 0
#endif
#ifndef PULL_WAKEUP_SRC_PF6
#define PULL_WAKEUP_SRC_PF6 0
#endif
#ifndef PULL_WAKEUP_SRC_PF7
#define PULL_WAKEUP_SRC_PF7 0
#endif

/************************************************************PG*******************************************************/
#ifndef PG0_INPUT_ENABLE
#define PG0_INPUT_ENABLE    0
#endif
#ifndef PG1_INPUT_ENABLE
#define PG1_INPUT_ENABLE    0
#endif
#ifndef PG2_INPUT_ENABLE
#define PG2_INPUT_ENABLE    0
#endif
#ifndef PG3_INPUT_ENABLE
#define PG3_INPUT_ENABLE    0
#endif
#ifndef PG4_INPUT_ENABLE
#define PG4_INPUT_ENABLE    0
#endif
#ifndef PG5_INPUT_ENABLE
#define PG5_INPUT_ENABLE    0
#endif
#ifndef PG6_INPUT_ENABLE
#define PG6_INPUT_ENABLE    0
#endif
#ifndef PG7_INPUT_ENABLE
#define PG7_INPUT_ENABLE    0
#endif
#ifndef PG0_OUTPUT_ENABLE
#define PG0_OUTPUT_ENABLE   0
#endif
#ifndef PG1_OUTPUT_ENABLE
#define PG1_OUTPUT_ENABLE   0
#endif
#ifndef PG2_OUTPUT_ENABLE
#define PG2_OUTPUT_ENABLE   0
#endif
#ifndef PG3_OUTPUT_ENABLE
#define PG3_OUTPUT_ENABLE   0
#endif
#ifndef PG4_OUTPUT_ENABLE
#define PG4_OUTPUT_ENABLE   0
#endif
#ifndef PG5_OUTPUT_ENABLE
#define PG5_OUTPUT_ENABLE   0
#endif
#ifndef PG6_OUTPUT_ENABLE
#define PG6_OUTPUT_ENABLE   0
#endif
#ifndef PG7_OUTPUT_ENABLE
#define PG7_OUTPUT_ENABLE   0
#endif
#ifndef PG0_DATA_STRENGTH
#define PG0_DATA_STRENGTH   1
#endif
#ifndef PG1_DATA_STRENGTH
#define PG1_DATA_STRENGTH   1
#endif
#ifndef PG2_DATA_STRENGTH
#define PG2_DATA_STRENGTH   1
#endif
#ifndef PG3_DATA_STRENGTH
#define PG3_DATA_STRENGTH   1
#endif
#ifndef PG4_DATA_STRENGTH
#define PG4_DATA_STRENGTH   1
#endif
#ifndef PG5_DATA_STRENGTH
#define PG5_DATA_STRENGTH   1
#endif
#ifndef PG6_DATA_STRENGTH
#define PG6_DATA_STRENGTH   1
#endif
#ifndef PG7_DATA_STRENGTH
#define PG7_DATA_STRENGTH   1
#endif
#ifndef PG0_DATA_OUT
#define PG0_DATA_OUT    0
#endif
#ifndef PG1_DATA_OUT
#define PG1_DATA_OUT    0
#endif
#ifndef PG2_DATA_OUT
#define PG2_DATA_OUT    0
#endif
#ifndef PG3_DATA_OUT
#define PG3_DATA_OUT    0
#endif
#ifndef PG4_DATA_OUT
#define PG4_DATA_OUT    0
#endif
#ifndef PG5_DATA_OUT
#define PG5_DATA_OUT    0
#endif
#ifndef PG6_DATA_OUT
#define PG6_DATA_OUT    0
#endif
#ifndef PG7_DATA_OUT
#define PG7_DATA_OUT    0
#endif
#ifndef PG0_FUNC
#define PG0_FUNC    AS_GPIO
#endif
#ifndef PG1_FUNC
#define PG1_FUNC    AS_GPIO
#endif
#ifndef PG2_FUNC
#define PG2_FUNC    AS_GPIO
#endif
#ifndef PG3_FUNC
#define PG3_FUNC    AS_GPIO
#endif
#ifndef PG4_FUNC
#define PG4_FUNC    AS_GPIO
#endif
#ifndef PG5_FUNC
#define PG5_FUNC    AS_GPIO
#endif
#ifndef PG6_FUNC
#define PG6_FUNC    AS_GPIO
#endif
#ifndef PG7_FUNC
#define PG7_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PG0
#define PULL_WAKEUP_SRC_PG0 0
#endif
#ifndef PULL_WAKEUP_SRC_PG1
#define PULL_WAKEUP_SRC_PG1 0
#endif
#ifndef PULL_WAKEUP_SRC_PG2
#define PULL_WAKEUP_SRC_PG2 0
#endif
#ifndef PULL_WAKEUP_SRC_PG3
#define PULL_WAKEUP_SRC_PG3 0
#endif
#ifndef PULL_WAKEUP_SRC_PG4
#define PULL_WAKEUP_SRC_PG4 0
#endif
#ifndef PULL_WAKEUP_SRC_PG5
#define PULL_WAKEUP_SRC_PG5 0
#endif
#ifndef PULL_WAKEUP_SRC_PG6
#define PULL_WAKEUP_SRC_PG6 0
#endif
#ifndef PULL_WAKEUP_SRC_PG7
#define PULL_WAKEUP_SRC_PG7 0
#endif

/************************************************************PH*******************************************************/
#ifndef PH0_INPUT_ENABLE
#define PH0_INPUT_ENABLE    0
#endif
#ifndef PH1_INPUT_ENABLE
#define PH1_INPUT_ENABLE    0
#endif
#ifndef PH2_INPUT_ENABLE
#define PH2_INPUT_ENABLE    0
#endif
#ifndef PH3_INPUT_ENABLE
#define PH3_INPUT_ENABLE    0
#endif
#ifndef PH4_INPUT_ENABLE
#define PH4_INPUT_ENABLE    0
#endif
#ifndef PH5_INPUT_ENABLE
#define PH5_INPUT_ENABLE    0
#endif
#ifndef PH6_INPUT_ENABLE
#define PH6_INPUT_ENABLE    0
#endif
#ifndef PH0_OUTPUT_ENABLE
#define PH0_OUTPUT_ENABLE   0
#endif
#ifndef PH1_OUTPUT_ENABLE
#define PH1_OUTPUT_ENABLE   0
#endif
#ifndef PH2_OUTPUT_ENABLE
#define PH2_OUTPUT_ENABLE   0
#endif
#ifndef PH3_OUTPUT_ENABLE
#define PH3_OUTPUT_ENABLE   0
#endif
#ifndef PH4_OUTPUT_ENABLE
#define PH4_OUTPUT_ENABLE   0
#endif
#ifndef PH5_OUTPUT_ENABLE
#define PH5_OUTPUT_ENABLE   0
#endif
#ifndef PH6_OUTPUT_ENABLE
#define PH6_OUTPUT_ENABLE   0
#endif
#ifndef PH0_DATA_STRENGTH
#define PH0_DATA_STRENGTH   1
#endif
#ifndef PH1_DATA_STRENGTH
#define PH1_DATA_STRENGTH   1
#endif
#ifndef PH2_DATA_STRENGTH
#define PH2_DATA_STRENGTH   1
#endif
#ifndef PH3_DATA_STRENGTH
#define PH3_DATA_STRENGTH   1
#endif
#ifndef PH4_DATA_STRENGTH
#define PH4_DATA_STRENGTH   1
#endif
#ifndef PH5_DATA_STRENGTH
#define PH5_DATA_STRENGTH   1
#endif
#ifndef PH6_DATA_STRENGTH
#define PH6_DATA_STRENGTH   1
#endif
#ifndef PH0_DATA_OUT
#define PH0_DATA_OUT    0
#endif
#ifndef PH1_DATA_OUT
#define PH1_DATA_OUT    0
#endif
#ifndef PH2_DATA_OUT
#define PH2_DATA_OUT    0
#endif
#ifndef PH3_DATA_OUT
#define PH3_DATA_OUT    0
#endif
#ifndef PH4_DATA_OUT
#define PH4_DATA_OUT    0
#endif
#ifndef PH5_DATA_OUT
#define PH5_DATA_OUT    0
#endif
#ifndef PH6_DATA_OUT
#define PH6_DATA_OUT    0
#endif
#ifndef PH0_FUNC
#define PH0_FUNC    AS_GPIO
#endif
#ifndef PH1_FUNC
#define PH1_FUNC    AS_GPIO
#endif
#ifndef PH2_FUNC
#define PH2_FUNC    AS_GPIO
#endif
#ifndef PH3_FUNC
#define PH3_FUNC    AS_GPIO
#endif
#ifndef PH4_FUNC
#define PH4_FUNC    AS_GPIO
#endif
#ifndef PH5_FUNC
#define PH5_FUNC    AS_GPIO
#endif
#ifndef PH6_FUNC
#define PH6_FUNC    AS_GPIO
#endif
#ifndef PULL_WAKEUP_SRC_PH0
#define PULL_WAKEUP_SRC_PH0 0
#endif
#ifndef PULL_WAKEUP_SRC_PH1
#define PULL_WAKEUP_SRC_PH1 0
#endif
#ifndef PULL_WAKEUP_SRC_PH2
#define PULL_WAKEUP_SRC_PH2 0
#endif
#ifndef PULL_WAKEUP_SRC_PH3
#define PULL_WAKEUP_SRC_PH3 0
#endif
#ifndef PULL_WAKEUP_SRC_PH4
#define PULL_WAKEUP_SRC_PH4 0
#endif
#ifndef PULL_WAKEUP_SRC_PH5
#define PULL_WAKEUP_SRC_PH5 0
#endif
#ifndef PULL_WAKEUP_SRC_PH6
#define PULL_WAKEUP_SRC_PH6 0
#endif

/************************************************************PI*******************************************************/
#ifndef PI0_INPUT_ENABLE
#define PI0_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI1_INPUT_ENABLE
#define PI1_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI2_INPUT_ENABLE
#define PI2_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI3_INPUT_ENABLE
#define PI3_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI4_INPUT_ENABLE
#define PI4_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI5_INPUT_ENABLE
#define PI5_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI6_INPUT_ENABLE
#define PI6_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI7_INPUT_ENABLE
#define PI7_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PI0_OUTPUT_ENABLE
#define PI0_OUTPUT_ENABLE   0
#endif
#ifndef PI1_OUTPUT_ENABLE
#define PI1_OUTPUT_ENABLE   0
#endif
#ifndef PI2_OUTPUT_ENABLE
#define PI2_OUTPUT_ENABLE   0
#endif
#ifndef PI3_OUTPUT_ENABLE
#define PI3_OUTPUT_ENABLE   0
#endif
#ifndef PI4_OUTPUT_ENABLE
#define PI4_OUTPUT_ENABLE   0
#endif
#ifndef PI5_OUTPUT_ENABLE
#define PI5_OUTPUT_ENABLE   0
#endif
#ifndef PI6_OUTPUT_ENABLE
#define PI6_OUTPUT_ENABLE   0
#endif
#ifndef PI7_OUTPUT_ENABLE
#define PI7_OUTPUT_ENABLE   0
#endif
#ifndef PI0_DATA_STRENGTH
#define PI0_DATA_STRENGTH   1
#endif
#ifndef PI1_DATA_STRENGTH
#define PI1_DATA_STRENGTH   1
#endif
#ifndef PI2_DATA_STRENGTH
#define PI2_DATA_STRENGTH   1
#endif
#ifndef PI3_DATA_STRENGTH
#define PI3_DATA_STRENGTH   1
#endif
#ifndef PI4_DATA_STRENGTH
#define PI4_DATA_STRENGTH   1
#endif
#ifndef PI5_DATA_STRENGTH
#define PI5_DATA_STRENGTH   1
#endif
#ifndef PI6_DATA_STRENGTH
#define PI6_DATA_STRENGTH   1
#endif
#ifndef PI7_DATA_STRENGTH
#define PI7_DATA_STRENGTH   1
#endif
#ifndef PI0_DATA_OUT
#define PI0_DATA_OUT    0
#endif
#ifndef PI1_DATA_OUT
#define PI1_DATA_OUT    0
#endif
#ifndef PI2_DATA_OUT
#define PI2_DATA_OUT    0
#endif
#ifndef PI3_DATA_OUT
#define PI3_DATA_OUT    0
#endif
#ifndef PI4_DATA_OUT
#define PI4_DATA_OUT    0
#endif
#ifndef PI5_DATA_OUT
#define PI5_DATA_OUT    0
#endif
#ifndef PI6_DATA_OUT
#define PI6_DATA_OUT    0
#endif
#ifndef PI7_DATA_OUT
#define PI7_DATA_OUT    0
#endif
#ifndef PI0_FUNC
#define PI0_FUNC    AS_MSPI_MOSI
#endif
#ifndef PI1_FUNC
#define PI1_FUNC    AS_MSPI_CK
#endif
#ifndef PI2_FUNC
#define PI2_FUNC    AS_MSPI_IO3
#endif
#ifndef PI3_FUNC
#define PI3_FUNC    AS_MSPI_CN
#endif
#ifndef PI4_FUNC
#define PI4_FUNC    AS_MSPI_MISO
#endif
#ifndef PI5_FUNC
#define PI5_FUNC    AS_MSPI_IO2
#endif
#ifndef PI6_FUNC
#define PI6_FUNC    AS_MSPI_IO4
#endif
#ifndef PI7_FUNC
#define PI7_FUNC    AS_MSPI_IO5
#endif
/************************************************************PJ*******************************************************/
#ifndef PJ0_INPUT_ENABLE
#define PJ0_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PJ1_INPUT_ENABLE
#define PJ1_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PJ2_INPUT_ENABLE
#define PJ2_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PJ3_INPUT_ENABLE
#define PJ3_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PJ4_INPUT_ENABLE
#define PJ4_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PJ5_INPUT_ENABLE
#define PJ5_INPUT_ENABLE    1   //MSPI
#endif
#ifndef PJ0_OUTPUT_ENABLE
#define PJ0_OUTPUT_ENABLE   0
#endif
#ifndef PJ1_OUTPUT_ENABLE
#define PJ1_OUTPUT_ENABLE   0
#endif
#ifndef PJ2_OUTPUT_ENABLE
#define PJ2_OUTPUT_ENABLE   0
#endif
#ifndef PJ3_OUTPUT_ENABLE
#define PJ3_OUTPUT_ENABLE   0
#endif
#ifndef PJ4_OUTPUT_ENABLE
#define PJ4_OUTPUT_ENABLE   0
#endif
#ifndef PJ5_OUTPUT_ENABLE
#define PJ5_OUTPUT_ENABLE   0
#endif
#ifndef PJ0_DATA_STRENGTH
#define PJ0_DATA_STRENGTH   1
#endif
#ifndef PJ1_DATA_STRENGTH
#define PJ1_DATA_STRENGTH   1
#endif
#ifndef PJ2_DATA_STRENGTH
#define PJ2_DATA_STRENGTH   1
#endif
#ifndef PJ3_DATA_STRENGTH
#define PJ3_DATA_STRENGTH   1
#endif
#ifndef PJ4_DATA_STRENGTH
#define PJ4_DATA_STRENGTH   1
#endif
#ifndef PJ5_DATA_STRENGTH
#define PJ5_DATA_STRENGTH   1
#endif
#ifndef PJ0_DATA_OUT
#define PJ0_DATA_OUT    0
#endif
#ifndef PJ1_DATA_OUT
#define PJ1_DATA_OUT    0
#endif
#ifndef PJ2_DATA_OUT
#define PJ2_DATA_OUT    0
#endif
#ifndef PJ3_DATA_OUT
#define PJ3_DATA_OUT    0
#endif
#ifndef PJ4_DATA_OUT
#define PJ4_DATA_OUT    0
#endif
#ifndef PJ5_DATA_OUT
#define PJ5_DATA_OUT    0
#endif
#ifndef PJ0_FUNC
#define PJ0_FUNC    AS_MSPI_IO6
#endif
#ifndef PJ1_FUNC
#define PJ1_FUNC    AS_MSPI_IO7
#endif
#ifndef PJ2_FUNC
#define PJ2_FUNC    AS_MSPI_CN1
#endif
#ifndef PJ3_FUNC
#define PJ3_FUNC    AS_MSPI_CN2
#endif
#ifndef PJ4_FUNC
#define PJ4_FUNC    AS_MSPI_CN3
#endif
#ifndef PJ5_FUNC
#define PJ5_FUNC    AS_MSPI_DM
#endif

/************************************************************PANA*******************************************************/
#ifndef PANA0_INPUT_ENABLE
#define PANA0_INPUT_ENABLE  0
#endif
#ifndef PANA1_INPUT_ENABLE
#define PANA1_INPUT_ENABLE  0
#endif
#ifndef PANA0_OUTPUT_ENABLE
#define PANA0_OUTPUT_ENABLE 0
#endif
#ifndef PANA1_OUTPUT_ENABLE
#define PANA1_OUTPUT_ENABLE 0
#endif
#ifndef PANA0_DATA_STRENGTH
#define PANA0_DATA_STRENGTH 1
#endif
#ifndef PANA1_DATA_STRENGTH
#define PANA1_DATA_STRENGTH 1
#endif
#ifndef PANA0_DATA_OUT
#define PANA0_DATA_OUT  0
#endif
#ifndef PANA1_DATA_OUT
#define PANA1_DATA_OUT  0
#endif
#ifndef PULL_WAKEUP_SRC_PANA0
#define PULL_WAKEUP_SRC_PANA0   0
#endif
#ifndef PULL_WAKEUP_SRC_PANA1
#define PULL_WAKEUP_SRC_PANA1   0
#endif
/**
 * @brief      This function servers to initiate pull up-down resistor of all gpio.
 * @param[in]  none
 * @return     none.
 * @attention  Processing methods of unused GPIO
 *             Set it to high resistance state and set it to open pull-up or pull-down resistance to
 *             let it be in the determined state.When GPIO uses internal pull-up or pull-down resistance,
 *             do not use pull-up or pull-down resistance on the board in the process of practical
 *             application because it may have the risk of electric leakage .
 */
static inline void gpio_analog_resistance_init(void){
    //A<3:0>
    analog_write_reg8 (0x80,  PULL_WAKEUP_SRC_PA0 |
                        (PULL_WAKEUP_SRC_PA1<<2) |
                        (PULL_WAKEUP_SRC_PA2<<4) |
                        (PULL_WAKEUP_SRC_PA3<<6));
    //A<7:4>
    analog_write_reg8 (0x81,  PULL_WAKEUP_SRC_PA4 |
                        (PULL_WAKEUP_SRC_PA5<<2) |
                        (PULL_WAKEUP_SRC_PA6<<4) |
                        (PULL_WAKEUP_SRC_PA7<<6));
    //B<3:0>
    analog_write_reg8 (0x82,  PULL_WAKEUP_SRC_PB0 |
                        (PULL_WAKEUP_SRC_PB1<<2) |
                        (PULL_WAKEUP_SRC_PB2<<4) |
                        (PULL_WAKEUP_SRC_PB3<<6));
    //B<7:4>
    analog_write_reg8 (0x83,  PULL_WAKEUP_SRC_PB4 |
                        (PULL_WAKEUP_SRC_PB5<<2) |
                        (PULL_WAKEUP_SRC_PB6<<4) |
                        (PULL_WAKEUP_SRC_PB7<<6));
    //C<3:0>
    analog_write_reg8 (0x84,  PULL_WAKEUP_SRC_PC0 |
                        (PULL_WAKEUP_SRC_PC1<<2) |
                        (PULL_WAKEUP_SRC_PC2<<4) |
                        (PULL_WAKEUP_SRC_PC3<<6));
    //C<7:4>
    analog_write_reg8 (0x85,  PULL_WAKEUP_SRC_PC4 |
                        (PULL_WAKEUP_SRC_PC5<<2) |
                        (PULL_WAKEUP_SRC_PC6<<4) |
                        (PULL_WAKEUP_SRC_PC7<<6));
    //D<3:0>
    analog_write_reg8 (0x86,  PULL_WAKEUP_SRC_PD0 |
                        (PULL_WAKEUP_SRC_PD1<<2) |
                        (PULL_WAKEUP_SRC_PD2<<4) |
                        (PULL_WAKEUP_SRC_PD3<<6));
    //D<7:4>
    analog_write_reg8 (0x87,  PULL_WAKEUP_SRC_PD4 |
                        (PULL_WAKEUP_SRC_PD5<<2) |
                        (PULL_WAKEUP_SRC_PD6<<4) |
                        (PULL_WAKEUP_SRC_PD7<<6));
    //E<3:0>
    analog_write_reg8 (0x88,  PULL_WAKEUP_SRC_PE0 |
                        (PULL_WAKEUP_SRC_PE1<<2) |
                        (PULL_WAKEUP_SRC_PE2<<4) |
                        (PULL_WAKEUP_SRC_PE3<<6));
    //E<7:4>
    analog_write_reg8 (0x89,  PULL_WAKEUP_SRC_PE4 |
                        (PULL_WAKEUP_SRC_PE5<<2) |
                        (PULL_WAKEUP_SRC_PE6<<4) |
                        (PULL_WAKEUP_SRC_PE7<<6));
    //F<3:0>
    analog_write_reg8 (0x8a,  PULL_WAKEUP_SRC_PF0 |
                        (PULL_WAKEUP_SRC_PF1<<2) |
                        (PULL_WAKEUP_SRC_PF2<<4) |
                        (PULL_WAKEUP_SRC_PF3<<6));
    //F<7:4>
    analog_write_reg8 (0x8b,  PULL_WAKEUP_SRC_PF4 |
                        (PULL_WAKEUP_SRC_PF5<<2) |
                        (PULL_WAKEUP_SRC_PF6<<4) |
                        (PULL_WAKEUP_SRC_PF7<<6));
    //G<3:0>
    analog_write_reg8 (0x8c,  PULL_WAKEUP_SRC_PG0 |
                            (PULL_WAKEUP_SRC_PG1<<2) |
                            (PULL_WAKEUP_SRC_PG2<<4) |
                            (PULL_WAKEUP_SRC_PG3<<6));
    //G<7:4>
    analog_write_reg8 (0x8d,  PULL_WAKEUP_SRC_PG4 |
                            (PULL_WAKEUP_SRC_PG5<<2) |
                            (PULL_WAKEUP_SRC_PG6<<4) |
                            (PULL_WAKEUP_SRC_PG7<<6));
    //H<3:0>
    analog_write_reg8 (0x8e,  PULL_WAKEUP_SRC_PH0 |
                            (PULL_WAKEUP_SRC_PH1<<2) |
                            (PULL_WAKEUP_SRC_PH2<<4) |
                            (PULL_WAKEUP_SRC_PH3<<6));
    //H<7:4>
    analog_write_reg8 (0x8f,  PULL_WAKEUP_SRC_PH4 |
                            (PULL_WAKEUP_SRC_PH5<<2) |
                            (PULL_WAKEUP_SRC_PH6<<4));

    //ANA<1:0>
    analog_write_reg8 (0xaa,  (PULL_WAKEUP_SRC_PANA0<<4) |
                            (PULL_WAKEUP_SRC_PANA1<<6));
}

_attribute_ram_code_sec_
static inline void gpio_init(int anaRes_init_en){
    //PA group
    reg_gpio_pa_out_set  =  (PA0_DATA_OUT<<0) | (PA1_DATA_OUT<<1) | (PA2_DATA_OUT<<2) | (PA3_DATA_OUT<<3) |
                            (PA4_DATA_OUT<<4) | (PA5_DATA_OUT<<5) | (PA6_DATA_OUT<<6) | (PA7_DATA_OUT<<7) ;
    reg_gpio_pa_setting1 =
        (PA0_INPUT_ENABLE<<8)  | (PA1_INPUT_ENABLE<<9)  | (PA2_INPUT_ENABLE<<10) | (PA3_INPUT_ENABLE<<11) |
        (PA4_INPUT_ENABLE<<12) | (PA5_INPUT_ENABLE<<13) | (PA6_INPUT_ENABLE<<14) | (PA7_INPUT_ENABLE<<15) |
        ((PA0_OUTPUT_ENABLE?0:1)<<16) | ((PA1_OUTPUT_ENABLE?0:1)<<17) | ((PA2_OUTPUT_ENABLE?0:1)<<18) | ((PA3_OUTPUT_ENABLE?0:1)<<19) |
        ((PA4_OUTPUT_ENABLE?0:1)<<20) | ((PA5_OUTPUT_ENABLE?0:1)<<21) | ((PA6_OUTPUT_ENABLE?0:1)<<22) | ((PA7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pa_setting2 =
        (PA0_DATA_STRENGTH<<8)  | (PA1_DATA_STRENGTH<<9)  | (PA2_DATA_STRENGTH<<10) | (PA3_DATA_STRENGTH<<11) |
        (PA4_DATA_STRENGTH<<12) | (PA5_DATA_STRENGTH<<13) | (PA6_DATA_STRENGTH<<14) | (PA7_DATA_STRENGTH<<15) |
        (PA0_FUNC==AS_GPIO ? BIT(16):0) | (PA1_FUNC==AS_GPIO ? BIT(17):0) | (PA2_FUNC==AS_GPIO ? BIT(18):0)| (PA3_FUNC==AS_GPIO ? BIT(19):0) |
        (PA4_FUNC==AS_GPIO ? BIT(20):0) | (PA5_FUNC==AS_GPIO ? BIT(21):0) | (PA6_FUNC==AS_GPIO ? BIT(22):0)| (PA7_FUNC==AS_GPIO ? BIT(23):0);

    //PB group
    reg_gpio_pb_out_set  =  (PB0_DATA_OUT<<0) | (PB1_DATA_OUT<<1) | (PB2_DATA_OUT<<2) | (PB3_DATA_OUT<<3) |
                            (PB4_DATA_OUT<<4) | (PB5_DATA_OUT<<5) | (PB6_DATA_OUT<<6) | (PB7_DATA_OUT<<7) ;
    reg_gpio_pb_setting1 =
        (PB0_INPUT_ENABLE<<8)  | (PB1_INPUT_ENABLE<<9)  | (PB2_INPUT_ENABLE<<10) | (PB3_INPUT_ENABLE<<11) |
        (PB4_INPUT_ENABLE<<12) | (PB5_INPUT_ENABLE<<13) | (PB6_INPUT_ENABLE<<14) | (PB7_INPUT_ENABLE<<15) |
        ((PB0_OUTPUT_ENABLE?0:1)<<16) | ((PB1_OUTPUT_ENABLE?0:1)<<17) | ((PB2_OUTPUT_ENABLE?0:1)<<18) | ((PB3_OUTPUT_ENABLE?0:1)<<19) |
        ((PB4_OUTPUT_ENABLE?0:1)<<20) | ((PB5_OUTPUT_ENABLE?0:1)<<21) | ((PB6_OUTPUT_ENABLE?0:1)<<22) | ((PB7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pb_setting2 =
        (PB0_DATA_STRENGTH<<8)  | (PB1_DATA_STRENGTH<<9)  | (PB2_DATA_STRENGTH<<10) | (PB3_DATA_STRENGTH<<11) |
        (PB4_DATA_STRENGTH<<12) | (PB5_DATA_STRENGTH<<13) | (PB6_DATA_STRENGTH<<14) | (PB7_DATA_STRENGTH<<15) |
        (PB0_FUNC==AS_GPIO ? BIT(16):0) | (PB1_FUNC==AS_GPIO ? BIT(17):0)| (PB2_FUNC==AS_GPIO ? BIT(18):0)| (PB3_FUNC==AS_GPIO ? BIT(19):0) |
        (PB4_FUNC==AS_GPIO ? BIT(20):0) | (PB5_FUNC==AS_GPIO ? BIT(21):0)| (PB6_FUNC==AS_GPIO ? BIT(22):0)| (PB7_FUNC==AS_GPIO ? BIT(23):0);

    //PC group
    reg_gpio_pc_out_set  =  (PC0_DATA_OUT<<0) | (PC1_DATA_OUT<<1) | (PC2_DATA_OUT<<2) | (PC3_DATA_OUT<<3) |
                            (PC4_DATA_OUT<<4) | (PC5_DATA_OUT<<5) | (PC6_DATA_OUT<<6) | (PC7_DATA_OUT<<7) ;
    reg_gpio_pc_setting1 =
        (PC0_INPUT_ENABLE<<8)  | (PC1_INPUT_ENABLE<<9)  | (PC2_INPUT_ENABLE<<10) | (PC3_INPUT_ENABLE<<11) |
        (PC4_INPUT_ENABLE<<12) | (PC5_INPUT_ENABLE<<13) | (PC6_INPUT_ENABLE<<14) | (PC7_INPUT_ENABLE<<15) |
        ((PC0_OUTPUT_ENABLE?0:1)<<16) | ((PC1_OUTPUT_ENABLE?0:1)<<17) | ((PC2_OUTPUT_ENABLE?0:1)<<18) | ((PC3_OUTPUT_ENABLE?0:1)<<19) |
        ((PC4_OUTPUT_ENABLE?0:1)<<20) | ((PC5_OUTPUT_ENABLE?0:1)<<21) | ((PC6_OUTPUT_ENABLE?0:1)<<22) | ((PC7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pc_setting2 =
        (PC0_DATA_STRENGTH<<8)  | (PC1_DATA_STRENGTH<<9)  | (PC2_DATA_STRENGTH<<10) | (PC3_DATA_STRENGTH<<11) |
        (PC4_DATA_STRENGTH<<12) | (PC5_DATA_STRENGTH<<13) | (PC6_DATA_STRENGTH<<14) | (PC7_DATA_STRENGTH<<15) |
        (PC0_FUNC==AS_GPIO ? BIT(16):0) | (PC1_FUNC==AS_GPIO ? BIT(17):0)| (PC2_FUNC==AS_GPIO ? BIT(18):0)| (PC3_FUNC==AS_GPIO ? BIT(19):0) |
        (PC4_FUNC==AS_GPIO ? BIT(20):0) | (PC5_FUNC==AS_GPIO ? BIT(21):0)| (PC6_FUNC==AS_GPIO ? BIT(22):0)| (PC7_FUNC==AS_GPIO ? BIT(23):0);

    //PD group
    reg_gpio_pd_out_set  =  (PD0_DATA_OUT<<0) | (PD1_DATA_OUT<<1) | (PD2_DATA_OUT<<2) | (PD3_DATA_OUT<<3) |
                            (PD4_DATA_OUT<<4) | (PD5_DATA_OUT<<5) | (PD6_DATA_OUT<<6) | (PD7_DATA_OUT<<7) ;
    reg_gpio_pd_setting1 =
        (PD0_INPUT_ENABLE<<8)  | (PD1_INPUT_ENABLE<<9)  | (PD2_INPUT_ENABLE<<10) | (PD3_INPUT_ENABLE<<11) |
        (PD4_INPUT_ENABLE<<12) | (PD5_INPUT_ENABLE<<13) | (PD6_INPUT_ENABLE<<14) | (PD7_INPUT_ENABLE<<15) |
        ((PD0_OUTPUT_ENABLE?0:1)<<16) | ((PD1_OUTPUT_ENABLE?0:1)<<17) | ((PD2_OUTPUT_ENABLE?0:1)<<18) | ((PD3_OUTPUT_ENABLE?0:1)<<19) |
        ((PD4_OUTPUT_ENABLE?0:1)<<20) | ((PD5_OUTPUT_ENABLE?0:1)<<21) | ((PD6_OUTPUT_ENABLE?0:1)<<22) | ((PD7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pd_setting2 =
        (PD0_DATA_STRENGTH<<8)  | (PD1_DATA_STRENGTH<<9)  | (PD2_DATA_STRENGTH<<10) | (PD3_DATA_STRENGTH<<11) |
        (PD4_DATA_STRENGTH<<12) | (PD5_DATA_STRENGTH<<13) | (PD6_DATA_STRENGTH<<14) | (PD7_DATA_STRENGTH<<15) |
        (PD0_FUNC==AS_GPIO ? BIT(16):0) | (PD1_FUNC==AS_GPIO ? BIT(17):0)| (PD2_FUNC==AS_GPIO ? BIT(18):0)| (PD3_FUNC==AS_GPIO ? BIT(19):0) |
        (PD4_FUNC==AS_GPIO ? BIT(20):0) | (PD5_FUNC==AS_GPIO ? BIT(21):0)| (PD6_FUNC==AS_GPIO ? BIT(22):0)| (PD7_FUNC==AS_GPIO ? BIT(23):0);

    //PE group
    reg_gpio_pe_out_set  =  (PE0_DATA_OUT<<0) | (PE1_DATA_OUT<<1) | (PE2_DATA_OUT<<2) | (PE3_DATA_OUT<<3) |
                            (PE4_DATA_OUT<<4) | (PE5_DATA_OUT<<5) | (PE6_DATA_OUT<<6) | (PE7_DATA_OUT<<7) ;
    reg_gpio_pe_setting1 =
        (PE0_INPUT_ENABLE<<8)   | (PE1_INPUT_ENABLE<<9)  | (PE2_INPUT_ENABLE<<10) | (PE3_INPUT_ENABLE<<11) |
        (PE4_INPUT_ENABLE<<12)  | (PE5_INPUT_ENABLE<<13) | (PE6_INPUT_ENABLE<<14) | (PE7_INPUT_ENABLE<<15) |
        ((PE0_OUTPUT_ENABLE?0:1)<<16) | ((PE1_OUTPUT_ENABLE?0:1)<<17) | ((PE2_OUTPUT_ENABLE?0:1)<<18) | ((PE3_OUTPUT_ENABLE?0:1)<<19) |
        ((PE4_OUTPUT_ENABLE?0:1)<<20) | ((PE5_OUTPUT_ENABLE?0:1)<<21) | ((PE6_OUTPUT_ENABLE?0:1)<<22) | ((PE7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pe_setting2 =
        (PE0_DATA_STRENGTH<<8)  | (PE1_DATA_STRENGTH<<9)  | (PE2_DATA_STRENGTH<<10) | (PE3_DATA_STRENGTH<<11) |
        (PE4_DATA_STRENGTH<<12) | (PE5_DATA_STRENGTH<<13) | (PE6_DATA_STRENGTH<<14) | (PE7_DATA_STRENGTH<<15) |
        (PE0_FUNC==AS_GPIO ? BIT(16):0) | (PE1_FUNC==AS_GPIO ? BIT(17):0) | (PE2_FUNC==AS_GPIO ? BIT(18):0) | (PE3_FUNC==AS_GPIO ? BIT(19):0) |
        (PE4_FUNC==AS_GPIO ? BIT(20):0) | (PE5_FUNC==AS_GPIO ? BIT(21):0) | (PE6_FUNC==AS_GPIO ? BIT(22):0) | (PE7_FUNC==AS_GPIO ? BIT(23):0);

    //PF group
    reg_gpio_pf_out_set  =  (PF0_DATA_OUT<<0) | (PF1_DATA_OUT<<1) | (PF2_DATA_OUT<<2) | (PF3_DATA_OUT<<3) |
                            (PF4_DATA_OUT<<4) | (PF5_DATA_OUT<<5) | (PF6_DATA_OUT<<6) | (PF7_DATA_OUT<<7) ;
    reg_gpio_pf_setting1 =
        (PF0_INPUT_ENABLE<<8)  | (PF1_INPUT_ENABLE<<9)  | (PF2_INPUT_ENABLE<<10) | (PF3_INPUT_ENABLE<<11) |
        (PF4_INPUT_ENABLE<<12) | (PF5_INPUT_ENABLE<<13) | (PF6_INPUT_ENABLE<<14) | (PF7_INPUT_ENABLE<<15) |
        ((PF0_OUTPUT_ENABLE?0:1)<<16) | ((PF1_OUTPUT_ENABLE?0:1)<<17) | ((PF2_OUTPUT_ENABLE?0:1)<<18) | ((PF3_OUTPUT_ENABLE?0:1)<<19) |
        ((PF4_OUTPUT_ENABLE?0:1)<<20) | ((PF5_OUTPUT_ENABLE?0:1)<<21) | ((PF6_OUTPUT_ENABLE?0:1)<<22) | ((PF7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pf_setting2 =
        (PF0_DATA_STRENGTH<<8)  | (PF1_DATA_STRENGTH<<9)  | (PF2_DATA_STRENGTH<<10) | (PF3_DATA_STRENGTH<<11) |
        (PF4_DATA_STRENGTH<<12) | (PF5_DATA_STRENGTH<<13) | (PF6_DATA_STRENGTH<<14) | (PF7_DATA_STRENGTH<<15) |
        (PF0_FUNC==AS_GPIO ? BIT(16):0) | (PF1_FUNC==AS_GPIO ? BIT(17):0)| (PF2_FUNC==AS_GPIO ? BIT(18):0)| (PF3_FUNC==AS_GPIO ? BIT(19):0) |
        (PF4_FUNC==AS_GPIO ? BIT(20):0) | (PF5_FUNC==AS_GPIO ? BIT(21):0)| (PF6_FUNC==AS_GPIO ? BIT(22):0)| (PF7_FUNC==AS_GPIO ? BIT(23):0);

    //PG group
    reg_gpio_pg_out_set  =  (PG0_DATA_OUT<<0) | (PG1_DATA_OUT<<1) | (PG2_DATA_OUT<<2) | (PG3_DATA_OUT<<3) |
                            (PG4_DATA_OUT<<4) | (PG5_DATA_OUT<<5) | (PG6_DATA_OUT<<6) | (PG7_DATA_OUT<<7) ;
    reg_gpio_pg_setting1 =
        (PG0_INPUT_ENABLE<<8)  | (PG1_INPUT_ENABLE<<9)  | (PG2_INPUT_ENABLE<<10) | (PG3_INPUT_ENABLE<<11) |
        (PG4_INPUT_ENABLE<<12) | (PG5_INPUT_ENABLE<<13) | (PG6_INPUT_ENABLE<<14) | (PG7_INPUT_ENABLE<<15) |
        ((PG0_OUTPUT_ENABLE?0:1)<<16) | ((PG1_OUTPUT_ENABLE?0:1)<<17) | ((PG2_OUTPUT_ENABLE?0:1)<<18) | ((PG3_OUTPUT_ENABLE?0:1)<<19) |
        ((PG4_OUTPUT_ENABLE?0:1)<<20) | ((PG5_OUTPUT_ENABLE?0:1)<<21) | ((PG6_OUTPUT_ENABLE?0:1)<<22) | ((PG7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pg_setting2 =
        (PG0_DATA_STRENGTH<<8)  | (PG1_DATA_STRENGTH<<9)  | (PG2_DATA_STRENGTH<<10) | (PG3_DATA_STRENGTH<<11) |
        (PG4_DATA_STRENGTH<<12) | (PG5_DATA_STRENGTH<<13) | (PG6_DATA_STRENGTH<<14) | (PG7_DATA_STRENGTH<<15) |
        (PG0_FUNC==AS_GPIO ? BIT(16):0) | (PG1_FUNC==AS_GPIO ? BIT(17):0) | (PG2_FUNC==AS_GPIO ? BIT(18):0) | (PG3_FUNC==AS_GPIO ? BIT(19):0) |
        (PG4_FUNC==AS_GPIO ? BIT(20):0) | (PG5_FUNC==AS_GPIO ? BIT(21):0) | (PG6_FUNC==AS_GPIO ? BIT(22):0) | (PG7_FUNC==AS_GPIO ? BIT(23):0);

    //PH group
    reg_gpio_ph_out_set  =  (PH0_DATA_OUT<<0) | (PH1_DATA_OUT<<1) | (PH2_DATA_OUT<<2) | (PH3_DATA_OUT<<3) |
                            (PH4_DATA_OUT<<4) | (PH5_DATA_OUT<<5) | (PH6_DATA_OUT<<6) ;
    reg_gpio_ph_setting1 =
        (PH0_INPUT_ENABLE<<8)  | (PH1_INPUT_ENABLE<<9)  | (PH2_INPUT_ENABLE<<10) | (PH3_INPUT_ENABLE<<11) |
        (PH4_INPUT_ENABLE<<12) | (PH5_INPUT_ENABLE<<13) | (PH6_INPUT_ENABLE<<14)|
        ((PH0_OUTPUT_ENABLE?0:1)<<16) | ((PH1_OUTPUT_ENABLE?0:1)<<17) | ((PH2_OUTPUT_ENABLE?0:1)<<18) | ((PH3_OUTPUT_ENABLE?0:1)<<19) |
        ((PH4_OUTPUT_ENABLE?0:1)<<20) | ((PH5_OUTPUT_ENABLE?0:1)<<21) | ((PH6_OUTPUT_ENABLE?0:1)<<22);
    reg_gpio_ph_setting2 =
        (PH0_DATA_STRENGTH<<8)  | (PH1_DATA_STRENGTH<<9)  | (PH2_DATA_STRENGTH<<10) | (PH3_DATA_STRENGTH<<11) |
        (PH4_DATA_STRENGTH<<12) | (PH5_DATA_STRENGTH<<13) | (PH6_DATA_STRENGTH<<14) |
        (PH0_FUNC==AS_GPIO ? BIT(16):0) | (PH1_FUNC==AS_GPIO ? BIT(17):0) | (PH2_FUNC==AS_GPIO ? BIT(18):0) | (PH3_FUNC==AS_GPIO ? BIT(19):0) |
        (PH4_FUNC==AS_GPIO ? BIT(20):0) | (PH5_FUNC==AS_GPIO ? BIT(21):0) | (PH6_FUNC==AS_GPIO ? BIT(22):0);

    //PI group
    reg_gpio_pi_out_set  =  (PI0_DATA_OUT<<0) | (PI1_DATA_OUT<<1) | (PI2_DATA_OUT<<2) | (PI3_DATA_OUT<<3) |
                            (PI4_DATA_OUT<<4) | (PI5_DATA_OUT<<5) | (PI6_DATA_OUT<<6) | (PI7_DATA_OUT<<7) ;
    reg_gpio_pi_setting1 =
        (PI0_INPUT_ENABLE<<8)  | (PI1_INPUT_ENABLE<<9)  | (PI2_INPUT_ENABLE<<10) | (PI3_INPUT_ENABLE<<11) |
        (PI4_INPUT_ENABLE<<12) | (PI5_INPUT_ENABLE<<13) | (PI6_INPUT_ENABLE<<14) | (PI7_INPUT_ENABLE<<15) |
        ((PI0_OUTPUT_ENABLE?0:1)<<16) | ((PI1_OUTPUT_ENABLE?0:1)<<17) | ((PI2_OUTPUT_ENABLE?0:1)<<18) | ((PI3_OUTPUT_ENABLE?0:1)<<19) |
        ((PI4_OUTPUT_ENABLE?0:1)<<20) | ((PI5_OUTPUT_ENABLE?0:1)<<21) | ((PI6_OUTPUT_ENABLE?0:1)<<22) | ((PI7_OUTPUT_ENABLE?0:1)<<23);
    reg_gpio_pi_setting2 =
        (PI0_DATA_STRENGTH<<8)  | (PI1_DATA_STRENGTH<<9)  | (PI2_DATA_STRENGTH<<10) | (PI3_DATA_STRENGTH<<11) |
        (PI4_DATA_STRENGTH<<12) | (PI5_DATA_STRENGTH<<13) | (PI6_DATA_STRENGTH<<14) | (PI7_DATA_STRENGTH<<15) |
        (PI0_FUNC==AS_GPIO ? BIT(16):0) | (PI1_FUNC==AS_GPIO ? BIT(17):0) | (PI2_FUNC==AS_GPIO ? BIT(18):0) | (PI3_FUNC==AS_GPIO ? BIT(19):0) |
        (PI4_FUNC==AS_GPIO ? BIT(20):0) | (PI5_FUNC==AS_GPIO ? BIT(21):0) | (PI6_FUNC==AS_GPIO ? BIT(22):0) | (PI7_FUNC==AS_GPIO ? BIT(23):0);

    //PJ group
    reg_gpio_pj_out_set  =  (PJ0_DATA_OUT<<0) | (PJ1_DATA_OUT<<1) | (PJ2_DATA_OUT<<2) | (PJ3_DATA_OUT<<3) | (PJ4_DATA_OUT<<4) | (PJ5_DATA_OUT<<5) ;
    reg_gpio_pj_setting1 =
        (PJ0_INPUT_ENABLE<<8)  | (PJ1_INPUT_ENABLE<<9)  | (PJ2_INPUT_ENABLE<<10) | (PJ3_INPUT_ENABLE<<11) |(PJ4_INPUT_ENABLE<<12) | (PJ5_INPUT_ENABLE<<13) |
        ((PJ0_OUTPUT_ENABLE?0:1)<<16) | ((PJ1_OUTPUT_ENABLE?0:1)<<17) | ((PJ2_OUTPUT_ENABLE?0:1)<<18)   | ((PJ3_OUTPUT_ENABLE?0:1)<<19) | ((PJ4_OUTPUT_ENABLE?0:1)<<20) | ((PJ5_OUTPUT_ENABLE?0:1)<<21);
    reg_gpio_pj_setting2 =
        (PJ0_DATA_STRENGTH<<8)  | (PJ1_DATA_STRENGTH<<9)  | (PJ2_DATA_STRENGTH<<10) | (PJ3_DATA_STRENGTH<<11) |
        (PJ4_DATA_STRENGTH<<12)|(PJ5_DATA_STRENGTH<<13)|
        (PJ0_FUNC==AS_GPIO ? BIT(16):0) | (PJ1_FUNC==AS_GPIO ? BIT(17):0)| (PJ2_FUNC==AS_GPIO ? BIT(18):0)| (PJ3_FUNC==AS_GPIO ? BIT(19):0) |
        (PJ4_FUNC==AS_GPIO ? BIT(20):0) | (PJ5_FUNC==AS_GPIO ? BIT(21):0) ;

    //PANA group
    analog_write_reg8(0x13d,((analog_read_reg8(0x13d)&0x03)|PANA0_INPUT_ENABLE)==0?0:(BIT(0)|PANA1_INPUT_ENABLE)==0?0:(BIT(1)
      |PANA0_OUTPUT_ENABLE)?0:(BIT(2)|PANA1_OUTPUT_ENABLE)?0:(BIT(3)|PANA0_DATA_STRENGTH)==0?0:(BIT(4)|PANA1_DATA_STRENGTH)==0?0:BIT(5));
    analog_write_reg8(0x13f,((analog_read_reg8(0x13f)&0xfc)|PANA0_DATA_OUT)==0?0:(BIT(0)|PANA1_DATA_OUT)==0?0:BIT(1));

    if(anaRes_init_en){
        gpio_analog_resistance_init();
    }
}

#endif /* DRIVERS_GPIO_SETTING_H_ */

