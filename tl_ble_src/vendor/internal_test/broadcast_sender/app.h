/********************************************************************************************************
 * @file    app.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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

#include "app_config.h"

#if (INTER_TEST_MODE == TEST_BIS_AUDIO_SENDER)


extern unsigned short app_bisBcstHandle[];

/* Sampling_Frequency */
typedef enum
{
    SF_8000HZ  = 0x01,
    SF_11025HZ = 0x02,
    SF_16000HZ = 0x03,
    SF_22050HZ = 0x04,

    SF_24000HZ = 0x05,
    SF_32000HZ = 0x06,
    SF_44100HZ = 0x07,
    SF_48000HZ = 0x08,

    SF_88200HZ  = 0x09,
    SF_96000HZ  = 0x0A,
    SF_176400HZ = 0x0B,
    SF_192000HZ = 0x0C,
    SF_384000HZ = 0x0D,
} sampling_frequency_t;

/* Frame_Duration */
typedef enum
{
    FD_7_5ms = 0x00, // Use 7.5 ms codec frames
    FD_10ms  = 0x01, // Use 10 ms codec frames

} Frame_Duration_t;

/**
 * @brief       user initialization when MCU power on or wake_up from deepSleep mode
 * @param[in]   none
 * @return      none
 */
void user_init_normal(void);


/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void);


/**
 * @brief     BLE main loop
 * @param[in]  none.
 * @return     none.
 */
void main_loop(void);


/**
 * @brief     BIS ISO send timer0 loop
 * @param[in]  none.
 * @return     none.
 */
void app_timer_test_irq_proc(void);


#endif /* INTER_TEST_MODE */
