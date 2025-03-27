/********************************************************************************************************
 * @file    w_ns.c
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
#include "w_ns.h"
#include "common/compiler.h"
#include "tl_common.h"

#if ALG_W_NS
void* w_ns_st             = NULL;
void* w_ns_scratch_st     = NULL;

W_NS_CFG_PARAM W_nsParas =
{
    .frame_size =120,
    .sampleRate = 16000,
    .target_level = k12dB,
    .lowShelf_En = 1,
    .preGain = 1.0,                 //4.0,//1.584893192461,
    .postGain = 1.0,
};





#endif

