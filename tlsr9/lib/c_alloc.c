/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/heap_listener.h>
#include <zephyr/init.h>
#include <zephyr/fatal.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(c_alloc);

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)

#ifndef CONFIG_COMMON_LIBC_MALLOC 

void* malloc(size_t size) {
    void* result = NULL;
    result = k_malloc(size);

    if (result == NULL)
    {
        LOG_ERR("Failed to allocate memory, size %d bytes, abort", size);
        k_fatal_halt(0xFFF2);
    }
    else
    {
        LOG_DBG("Memory allocated at %p, size %d", result, size);
    }
    return result;
}

void* calloc(size_t num, size_t size) {

    void* result = NULL;
    result = k_calloc(num, size);

    if (result == NULL)
    {
        LOG_ERR("Failed to allocate memory, size %d bytes, abort", num * size);
        k_fatal_halt(0xFFF1);
    }
    else
    {
        LOG_DBG("Memory allocated at %p, size %d", result, size * num);
    }

    return result;
}

void free(void* ptr) {
    if (ptr == NULL)
    {
        LOG_ERR("Failed to free the NULL address, abort");
        k_fatal_halt(0xFFF0);
    }
    else
    {
        k_free(ptr);
        LOG_DBG("Memory released at %p \n", ptr);
    }
}

#endif
#endif
