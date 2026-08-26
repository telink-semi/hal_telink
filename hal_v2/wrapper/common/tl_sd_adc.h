/**************************************************************************************************
 * @file    tl_sd_adc.h
 *
 * @brief   This is the header file for tl323x
 *
 * @author  Driver Group
 * @date    2026
 *
 * @par     Copyright (c) 2026, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
 **************************************************************************************************/
#pragma once

#include "dma.h"
#include "compiler.h"
#include "gpio.h"
#include "reg_include/register.h"

#define SD_ADC_GPIO_MODE    1
#define SD_ADC_VBAT_MODE    2
#define SD_ADC_MODE SD_ADC_VBAT_MODE

#if(SD_ADC_MODE == SD_ADC_GPIO_MODE)
#define SD_ADC_GPIO_PIN SD_ADC_GPIO_PB5P
#define SD_ADC_DIV SD_ADC_GPIO_CHN_DIV_1F4
#elif(SD_ADC_MODE == SD_ADC_VBAT_MODE)
#define SD_ADC_DIV SD_ADC_VBAT_DIV_1F4
#endif
#define SD_ADC_CLK_FREQ SD_ADC_SAPMPLE_CLK_1M
#define SD_ADC_DOWNSAMPLE_RATE SD_ADC_DOWNSAMPLE_RATE_128

_attribute_ram_code_sec_
int adc_sort_and_get_average_code(void);

void AdcDriverInit(void);

int AdcDriverRead(void);

typedef void (*adc_init_f)(void);
typedef int (*adc_read_f)(void);

extern adc_init_f _TLAdcDriverInit;
extern adc_read_f _TLAdcDriverRead;
