/********************************************************************************************************
 * @file    downmix.h
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
#ifndef _DOWNMIX_H
#define _DOWNMIX_H

#include "vendor/common/user_config.h"

#ifndef ALG_MIXER_EN
#define ALG_MIXER_EN    0
#endif
// #define  S_2CH_LEFT   (23170)  //round(sqrt(1/2)*pow2(15))
// #define  S_2CH_RIGHT  (23170)  //round(sqrt(1/2)*pow2(15))

#define S_2CH_LEFT  (16384) // round(sqrt(1/2)*pow2(15))
#define S_2CH_RIGHT (16384) // round(sqrt(1/2)*pow2(15))

/**
 * @brief Two-channel data mixing
 *
 * @param[in] point to start of left channel
 * @param[in] point to start of right channel
 * @param[out] point to output buffer
 * @param[in] stride between two sample
 * @param[in] number of samples to process
 *
 * @returns none
 */
void downmix_2ch(short *pLeft, short *pRight, short *pOut, int stride, int length);

#endif
