/********************************************************************************************************
 * @file    tlk_adpcm.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BT Audio Group
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
#ifndef TLK_ADPCM_H
#define TLK_ADPCM_H
#include "vendor/common/user_config.h"

#ifndef TLK_TONE_ENABLE
#define TLK_TONE_ENABLE     0
#endif

#if (TLK_TONE_ENABLE)
#define ALG_ADPCM_EN    1
#if (TLK_TONE_MONO_MODE)
#define CODEC_DAC_MONO_MODE 1
#endif
#endif

#ifndef ALG_ADPCM_EN
#define ALG_ADPCM_EN    0
#endif
/**
 * @brief Initialization of the adpcm module
 *
 * @param[in] System pointer of the adpcm module
 * @param[in] decorded size
 * @param[in] Preprocessing encoding is enabled
 * @param[in] Preprocessing code index
 *
 * @returns none
 */
void adpcm_init(unsigned char *ps, int len, int pre, int idx);

/**
 * @brief Set the ADPCM conversion gain
 *
 * @param[in] conversion gain
 *
 * @returns none
 */
void adpcm_set_gain(int gain);

/**
 * @brief ADPCM to pcm
 *
 * @param[in] pointer to the adpcm source buffer
 * @param[in] decorded size
 * @param[in] Conversion sampling rate
 *
 * @returns Encoding result size
 */
_attribute_ram_code_ int adpcm_get_sample(signed short *pd, int n, int sample_rate);

#endif
