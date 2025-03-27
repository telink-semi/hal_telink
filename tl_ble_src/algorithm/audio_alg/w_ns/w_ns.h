/********************************************************************************************************
 * @file    w_ns.h
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
#ifndef ALGORITHM_AUDIO_ALG_W_NS_W_NS_H_
#define ALGORITHM_AUDIO_ALG_W_NS_W_NS_H_

#pragma once

#include "../alg_audio_cfg.h"
#include "types.h"
#include "tlka_w_ns_api.h"

#ifndef ALG_W_NS
#define ALG_W_NS    0
#endif

#if ALG_W_NS
    extern void* w_ns_st;
    extern void* w_ns_scratch_st;
    extern W_NS_CFG_PARAM W_nsParas;

#endif
#endif /* ALGORITHM_AUDIO_ALG_W_NS_W_NS_H_ */
