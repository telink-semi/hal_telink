/********************************************************************************************************
 * @file    agc.h
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
#ifndef ALGORITHM_AUDIO_ALG_AGC_AGC_H_
#define ALGORITHM_AUDIO_ALG_AGC_AGC_H_

#pragma once


#include "../alg_audio_cfg.h"
#include "types.h"
#include "tlka_agc_api.h"

#ifndef ALG_AGC_EN
#define ALG_AGC_EN  0
#endif

#if ALG_AGC_EN

extern void *g_agc_st;

extern AGC_CFG_Param agc_param;

#endif
#endif /* ALGORITHM_AUDIO_ALG_AGC_AGC_H_ */
