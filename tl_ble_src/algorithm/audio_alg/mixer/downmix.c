/********************************************************************************************************
 * @file    downmix.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "common/compiler.h"
#include "tl_common.h"
#include "downmix.h"

#if ALG_MIXER_EN
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
void downmix_2ch(short *pLeft,  /* point to start of left channel */
                 short *pRight, /* point to start of right channel */
                 short *pOut,   /* point to output buffer */
                 int stride,    /* stride between two samples */
                 int length)    /* number of samples to process */
{
    int i;
    int sum;
    for (i = 0; i < length; i++) {
        sum = pRight[i * stride] * S_2CH_RIGHT + pLeft[i * stride] * S_2CH_LEFT;
        sum = (sum + (1 << 14)) >> 15;
        if (sum > 32767) {
            pOut[i] = 32767;
        } else if (sum < -32768) {
            pOut[i] = -32768;
        } else {
            pOut[i] = sum;
        }
    }

    return;
}
#endif
