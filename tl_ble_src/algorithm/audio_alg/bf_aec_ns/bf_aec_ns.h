/********************************************************************************************************
 * @file    bf_aec_ns.h
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  Driver Group
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
#ifndef ALGORITHM_AUDIO_ALG_BF_AEC_NS_BF_AEC_NS_H_
#define ALGORITHM_AUDIO_ALG_BF_AEC_NS_BF_AEC_NS_H_

#pragma once

#include "../alg_audio_cfg.h"
#include "types.h"
#include "tlka_aec_ns_api.h"

#ifndef ALG_GSC_EN
#define ALG_GSC_EN              0
#endif

///to AEC
#ifndef ALG_AEC_EN
#define ALG_AEC_EN              0
#endif

///to NS
#ifndef ALG_S_NS
#define ALG_S_NS                0
#endif

#if ALG_GSC_EN
    extern gscState         *g_gsc_st_p;
    extern AEC_CFG_PARAS aecParas;

    int tlk_gsc_BeamFormer(short *x_in, short *ref_in, short *x_out, gscState *st, int was_speech);
#endif

#if (ALG_AEC_EN|ALG_S_NS)

    extern void *g_aec_st;
    extern void *g_s_ns_den;
    extern void *scratch_buff;

    extern AEC_CFG_PARAS aecParas;
    extern NS_CFG_PARAS nsParas;

    void tlk_aec_init(unsigned int sample_rate, unsigned char frame_size);

#endif
#endif /* ALGORITHM_AUDIO_ALG_BF_AEC_NS_BF_AEC_NS_H_ */
