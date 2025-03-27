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
#ifndef VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_
#define VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_

#if (INTER_TEST_MODE == TEST_USB_PPM_ASRC)

#define APP_AUDIO_INPUT_BUFFER_SIZE                 2048
#define APP_AUDIO_INPUT_FRAME_SAMPLE_MAX             480
#define APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX       155

#define APP_AUDIO_OUTPUT_BUFFER_SIZE                2048
#define APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX            160
#define APP_AUDIO_OUTPUT_FRAME_ENCODE_BYTES_MAX       40

#define DMIC_IN_PINGPONG_SIZE            960
#define USB_OUT_PINGPONG_SIZE            1920

#define BLC_CODEC_MIC_DMA                DMA2
#define BLC_CODEC_SPK_DMA                DMA3

#define MIC_BUFF_IDX                     0
#define SPK_BUFF_IDX                     1

extern volatile signed short app_dmic_in_pingpong[];
extern volatile u32 iso_in_irq_stick;
extern volatile u8 iso_in_permiteFlag;


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

#endif

#endif /* VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_ */
