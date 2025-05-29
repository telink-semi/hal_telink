/********************************************************************************************************
 * @file    tlk_heap.h
 *
 * @brief   This is the header file for BLE SDK
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
#pragma once


#include "common/types.h"
#include "common/compiler.h"


/**
 * @brief       This function serves to get the available heap size.
 * @param[in]   none.
 * @return      Size of available heap.
 */
u32 tlk_get_available_heap_size(void);

/**
 * @brief       This function serves to get the used heap size.
 * @param[in]   none.
 * @return      Used heap size.
 */
u32 tlk_get_used_heap_size(void);

/**
 * @brief       This function serves alloc heap memory.
 * @param[in]   size - heap memory need to allocated.
 * @return      none.
 */
u8 tlk_heap_alloc(u32 size);

/**
 * @brief       This function serves get next available heap memory.
 * @param[in]   none.
 * @return      Address of the start of heap memory.
 */
void *tlk_get_heap_free_startAddr(void);
