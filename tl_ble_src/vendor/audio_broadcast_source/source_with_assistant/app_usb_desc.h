/********************************************************************************************************
 * @file    app_usb_desc.c
 *
 * @brief   This is the source file for BLE SDK
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

#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_WITH_ASSISTANT)

typedef enum{
    USB_SAMPLING_FREQ_8KHZ,
    USB_SAMPLING_FREQ_16KHZ,
    USB_SAMPLING_FREQ_24KHZ,
    USB_SAMPLING_FREQ_32KHZ,
    USB_SAMPLING_FREQ_48KHZ,
} usb_audioSamplingFreq_enum;

typedef enum{
    AUDIO_TYPE_MONO,
    AUDIO_TYPE_STEREO,
} usb_audioType_enum;

typedef struct __attribute__((packed)) {
    u16 vendorId;
    u16 productId;
    u32 speakSampleRate;
    u8 speakNum;
} vendor_usbDesc_t;

void app_usb_changeDesc(usb_audioSamplingFreq_enum freq, usb_audioType_enum type);

#endif  //SOURCE_VERSION == SOURCE_WITH_ASSISTANT
