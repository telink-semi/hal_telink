/********************************************************************************************************
 * @file    fifo.c
 *
 * @brief   This is the source file for TLSR/TL
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
#include "common/fifo.h"
#include "common/tstring.h"
#if 1

/**
 * @brief  tlk_fifo_init .
 *
 * @param[in]  f: fifo start address
 * @param[in]  s: fifo size
 * @param[in]  n: fifo number
 * @param[in]  p: fifo pointer
 * @param[out] none
 *
 * @returns none
 */
void tlk_fifo_init(tlk_fifo_t *f, int s, u8 n, u8 *p)
{
    f->size = s;
    f->num  = n;
    f->wptr = 0;
    f->rptr = 0;
    f->p    = p;
}

/**
 * @brief  update fifo pointer.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns 0 or new fifo pointer
 */
_attribute_retention_code_ u8 *tlk_fifo_wptr(tlk_fifo_t *f)
{
    if (((f->wptr - f->rptr) & 255) < f->num) {
        return f->p + (f->wptr & (f->num - 1)) * f->size;
    }
    return 0;
}

/**
 * @brief  fifo wptr add one .
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
_attribute_retention_code_ void tlk_fifo_next(tlk_fifo_t *f)
{
    f->wptr++;
}

/**
 * @brief  push data to fifo.
 *
 * @param[in]  f: fifo start address
 * @param[in]  p: data start address
 * @param[in]  n: data length
 * @param[out] none
 *
 * @returns 0-success -1-fail
 */
int tlk_fifo_push(tlk_fifo_t *f, u8 *p, u32 n)
{
    if (((f->wptr - f->rptr) & 255) >= f->num) {
        return -1;
    }

    if (n >= f->size) {
        return -1;
    }
    u8 *pd = f->p + (f->wptr++ & (f->num - 1)) * f->size;
    *pd++  = n & 0xff;
    *pd++  = (n >> 8) & 0xff;
    tmemcpy((void *)pd, (void *)p, (unsigned int)n);
    return 0;
}

#endif
#endif
