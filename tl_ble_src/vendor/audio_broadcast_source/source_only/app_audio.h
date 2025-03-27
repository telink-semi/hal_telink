/********************************************************************************************************
 * @file    app_audio.h
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
#include "../source_config.h"

#if (SOURCE_VERSION == SOURCE_ONLY_VERSION)

#pragma once

#include "tl_common.h"
#include "app_config.h"
#include "app_buffer.h"
#include "stack/ble/ble.h"

#define APP_AUDIO_INPUT_AMIC                        1
#define APP_AUDIO_INPUT_LINEIN                      2
#define APP_AUDIO_INPUT_IISIN                       4
#define APP_AUDIO_INPUT_CODEC_ENDING                4

#define APP_AUDIO_INPUT_USB_MIC                     5
#define APP_AUDIO_INPUT_NONE                        6

#define APP_AUDIO_INPUT_MODE                        APP_AUDIO_INPUT_USB_MIC

#if APP_AUDIO_INPUT_MODE == APP_AUDIO_INPUT_USB_MIC
#if TLKAPI_DEBUG_ENABLE && (TLKAPI_DEBUG_CHANNEL == TLKAPI_DEBUG_CHANNEL_UDB)
#error "use usb mic mode, must close tlk debug mode or not use usb debug mode."
#endif
#endif

/* Audio configuration */
#define APP_AUDIO_FRAME_SAMPLE                      (480)// 48K => 10ms * 48sample
#define APP_AUDIO_FRAME_BYTES                       (APP_AUDIO_FRAME_SAMPLE << 1) // 1 sample 16bits


typedef struct{
    u8 dataInMode;
    blc_bcstAudioAnnouncements_param_t BASE;
} app_bisSource_param_t;

extern int codecFrameDataLen;
extern app_bisSource_param_t bisSource;

/**
 * @brief       audio initial function.
 * @param[in]   none.
 * @return      true: initial successful, fail: initial failed.
 */
bool  app_audio_init(void);

/**
 * @brief      audio loop handler process.
 * @param[in]  none.
 * @return     none.
 */
void app_audio_handler(void);

/**
 * @brief       audio initial usb audio speak role.
 * @param[in]   none
 * @return      none
 */
void app_audio_initUsbMic(void);

/**
 * @brief       usb audio clean rx Buffer.
 * @param[in]   none
 * @return      none
 */
void usb_audio_cleanUsbRxBuffer(void);

/**
 * @brief       usb audio get pcm data.
 * @param[in]   none
 * @return      none
 */
void app_audio_getUsbMicData(u16* pcm);

/**
 * @brief       usb audio handler.
 * @param[in]   none
 * @return      none
 */
void app_audio_usbMicHandler(void);

/**
 * @brief       audio initial codec function
 * @param[in]   none
 * @return      none
 */
void app_audio_initCodec(void);

/**
 * @brief       codec audio clean rx Buffer.
 * @param[in]   none
 * @return      none
 */
void app_audio_cleanCodecRxBuffer(void);

/**
 * @brief       codec audio get pcm data.
 * @param[in]   none
 * @return      none
 */
void app_audio_getCodecData(u16* pcm);


#endif      //SOURCE_VERSION == SOURCE_ONLY_VERSION
