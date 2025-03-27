/********************************************************************************************************
 * @file    alg_audio_cfg.h
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef ALGORITHM_AUDIO_ALG_ALG_AUDIO_CFG_H_
#define ALGORITHM_AUDIO_ALG_ALG_AUDIO_CFG_H_

#pragma once
#include "vendor/common/user_config.h"

#ifndef ALG_AUDIO_EN
#define ALG_AUDIO_EN    0
#endif

#if ALG_AUDIO_EN
    #define ALG_HYBRID_ALG_EN               1       //can configure, 0 or 1
#else
    #define ALG_HYBRID_ALG_EN               0       //can configure, 0 or 1
#endif


#define FLG_ALG_AUDIO_W_NS_MASK             (0x01<<0)
#define FLG_ALG_AUDIO_S_NS_MASK             (0x01<<0)
#define FLG_ALG_AUDIO_AEC_MASK              (0x01<<1)
#define FLG_ALG_AUDIO_AECM_MASK             (0x01<<1)
#define FLG_ALG_AUDIO_GSC_MASK              (0x01<<2)
#define FLG_ALG_AUDIO_AGC_MASK              (0x01<<3)
#define FLG_ALG_AUDIO_DRC_MASK              (0x01<<4)


#define tmemset     tlk_mem_set
#define tmemcpy     tlk_mem_cpy
#define tmemcmp     tlk_mem_cmp

#if ALG_HYBRID_ALG_EN
    #define ALG_AEC_DELAY           258
#else
    #define ALG_AEC_DELAY           258
#endif

#define ALG_AUDIO_TEST_EN       0   ///for test

#endif /* ALGORITHM_AUDIO_ALG_ALG_AUDIO_CFG_H_ */
