
/********************************************************************************************************
 * @file    tl_sd_adc.c
 *
 * @brief   This is the source file for tl323x
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
 *******************************************************************************************************/

#include <stdio.h>
#include "sd_adc.h"
#include "tl_sd_adc.h"
#include "lib/include/stimer.h"
#include "lpc.h"

#define SD_ADC_SAMPLE_CNT 16 // Number of samples used to calculate the average.
static signed int sd_adc_sample_buffer[SD_ADC_SAMPLE_CNT] __attribute__((aligned(4))) = {0};

int adc_sort_and_get_average_code(void)
{
    signed int code_average = 0;

	// Enable ADC and start sampling
	sd_adc_power_on(SD_ADC_SAMPLE_MODE);

    /* Wait for ADC to stabilize */
    delay_us(200);

    /* Start sampling */
    sd_adc_sample_start();

    // Discard the first 4 invalid samples
    for (int i = 0; i < 4; i++) {
        sd_adc_get_raw_code();
    }

    // Collect multiple samples
    int cnt = 0;
    while (cnt < SD_ADC_SAMPLE_CNT) {
        int sample_cnt = sd_adc_get_rxfifo_cnt();
        if (sample_cnt > 0) {
            sd_adc_sample_buffer[cnt] = sd_adc_get_raw_code();
            cnt++;
        }
    }

    // Sort samples using insertion sort
    for(int i = 1; i < SD_ADC_SAMPLE_CNT; i++) {
        if(sd_adc_sample_buffer[i] < sd_adc_sample_buffer[i-1]) {
            signed int temp = sd_adc_sample_buffer[i];
            sd_adc_sample_buffer[i] = sd_adc_sample_buffer[i-1];
            int j;
            for(j = i-1; j >= 0 && sd_adc_sample_buffer[j] > temp; j--) {
                sd_adc_sample_buffer[j+1] = sd_adc_sample_buffer[j];
            }
            sd_adc_sample_buffer[j+1] = temp;
        }
    }

    // Calculate average (remove the highest and lowest 1/4 data)
    for (int i = SD_ADC_SAMPLE_CNT >> 2; i < (SD_ADC_SAMPLE_CNT - (SD_ADC_SAMPLE_CNT >> 2)); i++) {
        code_average += sd_adc_sample_buffer[i] / (SD_ADC_SAMPLE_CNT >> 1);
    }

    signed int sd_adc_vol_10x = sd_adc_calculate_voltage(code_average, SD_ADC_VOLTAGE_10X_MV);
    return sd_adc_vol_10x / 10;
}

void AdcDriverInit(void)
{
    /* Initialize the SD ADC module to single-channel mode */
    sd_adc_init(SD_ADC_SINGLE_DC_MODE);
    sd_adc_vbat_sample_init(SD_ADC_SAPMPLE_CLK_1M, 
        SD_ADC_VBAT_DIV_1F4, SD_ADC_DOWNSAMPLE_RATE_128);
}

int AdcDriverRead(void)
{
    int adc_mv_val = adc_sort_and_get_average_code();
    sd_adc_power_off(SD_ADC_SAMPLE_MODE);
	/*
	 * Close LPC before sleep, otherwise
	 * it will increase the standby current.
	 */
	lpc_vbat_detect_disable();
	lpc_power_down();
    return adc_mv_val;
}

adc_init_f _TLAdcDriverInit = AdcDriverInit;
adc_read_f _TLAdcDriverRead = AdcDriverRead;
