/********************************************************************************************************
 * @file    app_att.h
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
#ifndef APP_ATT_H_
#define APP_ATT_H_

#if (INTER_TEST_MODE == TEST_CIS_AUDIO_SERVER)

    #define APP_AUDIO_PREFERRED_CONTEXTS (BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA)
    #define APP_AUDIO_STREAMING_CONTEXTS (BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA)
    #define APP_AUDIO_SUPPORTED_CONTEXTS (BLC_AUDIO_CONTEXT_TYPE_CONVERSATIONAL | BLC_AUDIO_CONTEXT_TYPE_MEDIA)

void app_audio_acceptor_init(void);

#endif /* INTER_TEST_MODE */

#endif /* APP_ATT_H_ */
