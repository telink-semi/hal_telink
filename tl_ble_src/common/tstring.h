/********************************************************************************************************
 * @file    tstring.h
 *
 * @brief   This is the header file for TLSR/TL
 *
 * @author  Bluetooth Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#if defined(HOST_V2_ENABLE)
#pragma GCC optimize "no-tree-loop-distribute-patterns"
#pragma once
#include "compiler.h"
#include "types.h"


#ifndef NULL
    #define NULL 0
#endif

void *memset(void *d, int c, unsigned int n);

int   tstrlen(const char *pStr);
int   tmemcmp(const void *m1, const void *m2, u32 len);
int   tmemcmp4(void *m1, void *m2, register unsigned int len);
void *tmemset(void *dest, int val, unsigned int len);
void  tmemcpy(void *out, const void *in, unsigned int length);
void  tmemcpy4(void *d, const void *s, unsigned int length);

/**
 * @brief  set value, for performance, assume length % 4 == 0,  and no memory overlapped.
 *
 * @param[in]  dest: data address
 * @param[in]  val: value
 * @param[in]  length: data length
 * @param[out] none
 *
 * @returns 0-fail other-success
 */
_attribute_retention_code_ void tmemset4(void *dest, int val, unsigned int length);

extern volatile uint32_t tdest_addr;
extern volatile uint32_t tdest_addr_end;

#define smemcmp  tmemcmp
#define smemcmp4 tmemcmp4
#define smemset  tmemset
#define smemcpy  tmemcpy
#define smemcpy4 tmemcpy4
#define smemset4 tmemset4
#endif
