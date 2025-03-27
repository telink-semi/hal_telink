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

#if (INTER_TEST_MODE == TEST_LE_AUDIO_SWTICH_CLIENT)

#define APP_AUDIO_INPUT_BUFFER_SIZE                 2048
#define APP_AUDIO_INPUT_FRAME_SAMPLE_MAX             480
#define APP_AUDIO_INPUT_FRAME_ENCODE_BYTES_MAX       155

#define APP_AUDIO_OUTPUT_BUFFER_SIZE                2048
#define APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX            160
#define APP_AUDIO_OUTPUT_FRAME_ENCODE_BYTES_MAX       40


/**
 *  @brief  list node
 */
#if(APP_AUDIO_SCENE == APP_SCENE_TWS)
struct __attribute__((packed))  list_node_t
{
    u32     renderPoint;
    u16     buffer[2*APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX];
    struct __attribute__((packed))  list_node_t *next;
};
#elif(APP_AUDIO_SCENE == APP_SCENE_HEADSET)
struct __attribute__((packed))  list_node_t
{
    u32     renderPoint;
    u16     buffer[APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX];
    struct __attribute__((packed))  list_node_t *next;
};
#endif

/**
 *  @brief  app render point buffers
 */
typedef struct __attribute__((packed)) 
{
    u32     renderPoint;
    u16     buffer[APP_AUDIO_OUTPUT_FRAME_SAMPLE_MAX];
}audio_pkt_t;

/**
 *  @brief  app codec concerned parameters
 */
typedef struct __attribute__((packed)) {
    u8   cC;       //channel counts
    u16  fSample;  //sample each frame
    u16  fOctets;  //octets each frame
    u16  frameUs;  //time each frame,us conut
    u8   rsvd;
}app_codec_desc_t;

/**
 * @brief      Codec init function.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_init(void);

/**
 * @brief      Codec process in loop function,include audio data send,audio data receive and other codec process.
 * @param[in]  none.
 * @return     none.
 */
void app_codec_handler(void);

/**
 * @brief      Timer irq process,used to playback audio data at a specific tick.
 * @param[in]  none.
 * @return     none.
 */
void app_timer_irq_proc(void);

/**
 * @brief      Free all node in list.
 * @param[in]  none.
 * @return     none.
 */
void app_list_free(void);


#endif

#endif /* VENDOR_AUDIO_UNICAST_CLIENT_APP_CODEC_H_ */
