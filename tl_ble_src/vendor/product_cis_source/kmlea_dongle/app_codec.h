/********************************************************************************************************
 * @file    app_codec.h
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
#if (PRODUCT_CIS_SOURCE_SELECT == PRODUCT_KMLEA_DONGLE)
#ifndef VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_
#define VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_

#define APP_AUDIO_INPUT_BUFFER_SIZE                 2048
#define APP_AUDIO_INPUT_FRAME_SAMPLE_MAX             480
#define APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX       155

#define APP_AUDIO_OUTPUT_BUFFER_SIZE                2048
#define APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX            160
#define APP_AUDIO_OUTPUT_FRAME_ENCODE_BYTES_MAX       40

struct list_node_t
{
    u32     renderPoint;
    u16     buffer[APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX];
    struct list_node_t *next;
};

typedef struct
{
    u32     renderPoint;
    u16     buffer[APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX];
}audio_pkt_t;

void app_codec_init();

void app_codec_handler();

void app_usb_irq_proc (void);


#endif //end of (PRODUCT_CIS_SOURCE_SELECT == ...)

#endif /* VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_ */
