/********************************************************************************************************
 * @file    app_mem.h
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
#ifndef VENDOR_TAGSDK_EXAMPLE_TELINK_APP_MEM_H_
#define VENDOR_TAGSDK_EXAMPLE_TELINK_APP_MEM_H_

#include "tl_common.h"
#include "app_config.h"


/*
 * This structure should be a power of two.  This becomes the
 * alignment unit.
 */
struct app_free_arena_header;

struct app_arena_header
{
    unsigned short            type;
    unsigned short            size;
    struct app_free_arena_header *next, *prev;
};

#ifdef DEBUG_MALLOC
    #define ARENA_TYPE_USED 0x64e69c70
    #define ARENA_TYPE_FREE 0x012d610a
    #define ARENA_TYPE_HEAD 0x971676b5
    #define ARENA_TYPE_DEAD 0xeeeeeeee
#else
    #define ARENA_TYPE_USED 0
    #define ARENA_TYPE_FREE 1
    #define ARENA_TYPE_HEAD 2
#endif


#define ARENA_SIZE_MASK (~(sizeof(struct app_arena_header) - 1))

/*
 * This structure should be no more than twice the size of the
 * previous structure.
 */
struct app_free_arena_header
{
    struct app_arena_header       a;
    struct app_free_arena_header *next_free, *prev_free;
};

struct app_mem_arena_header
{
    struct app_free_arena_header a;
    unsigned char           *sbrkBase;
    unsigned char           *sbrkLimit;
    unsigned char           *brk;
};


void  app_initialNonRetentionBuffer(void *base, size_t size);
void *app_malloc_nonreten(size_t size);
void  app_free_nonreten(void *ptr);
void *app_realloc_nonreten(void *ptr, size_t size);

#endif /* VENDOR_TAGSDK_EXAMPLE_TELINK_APP_MEM_H_ */
