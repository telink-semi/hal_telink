/********************************************************************************************************
 * @file    tlk_heap.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "common/types.h"
#include "common/compiler.h"
#include "tlk_mem.h"
#include "common/config/user_config.h"


/**************************************************************************************************
  Macros
**************************************************************************************************/

#ifndef HEAP_MEM_SIZE_CFG
    #define HEAP_MEM_SIZE_CFG 0
#endif

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

/*! Global pool buffers. */
//_attribute_iram_bss_
static u8 tlkMemBuf[HEAP_MEM_SIZE_CFG] __attribute__((aligned(4)));

_attribute_data_retention_ static u8 *tlk_heap_start = tlkMemBuf;
_attribute_data_retention_ static u32 tlk_heap_size  = HEAP_MEM_SIZE_CFG;

/**
 * @brief       This function serves alloc heap memory.
 * @param[in]   size   - heap memory size need to allocated.
 * @return      0      - memory alloc success.
 *              others - memory alloc failed,search for TLK_MEM_STATE_T.
 */
u8 tlk_heap_alloc(u32 size)
{
    /* Round up to nearest multiple of 4 for word alignment */
    size = (size + 3) & ~3;
    if (size > tlk_heap_size) {
        return TLK_MEM_OUT_OF_MEM;
    }
    tlk_heap_start += size;
    tlk_heap_size -= size;
    return TLK_MEM_SUCCESS;
}

/**
 * @brief       This function serves get next available heap memory.
 * @param[in]   none.
 * @return      Address of the start of heap memory.
 */
void *tlk_get_heap_free_startAddr(void)
{
    return (void *)tlk_heap_start;
}

/**
 * @brief       This function serves to get the available heap size.
 * @param[in]   none.
 * @return      Size of available heap.
 */
u32 tlk_get_available_heap_size(void)
{
    return tlk_heap_size;
}

/**
 * @brief       This function serves to get the used heap size.
 * @param[in]   none.
 * @return      Used heap size.
 */
u32 tlk_get_used_heap_size(void)
{
    return HEAP_MEM_SIZE_CFG - tlk_heap_size;
}
