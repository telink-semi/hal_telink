/********************************************************************************************************
 * @file    fifo.h
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
#ifndef FIFO_H_
#define FIFO_H_
#if defined(HOST_V2_ENABLE)
#include "types.h"
#include "compiler.h"

typedef struct
{
    u32 size;
    u8  num;
    u8  mask;
    u8  wptr;
    u8  rptr;
    u8 *p;
} __attribute__((aligned(4))) tlk_fifo_t;

typedef struct
{
    u8 *p;
    u32 size;
    u32 num;
    u32 wptr;
    u32 rptr;
} tlk_fifo1_t;

typedef struct
{
    u16 fifo_size;
    u8  fifo_num;
    u8  conn_num;
    u8 *p;
} multi_conn_fifo_t;

#define FIFO_INIT(name, size, n) /*__attribute__ ((aligned (4)))*/      \
    u8         name##_b[size * n] __attribute__((aligned(4))) /*={0}*/; \
    tlk_fifo_t name = {size, n, 0, 0, name##_b}

#define FIFO_INIT_IRAM(name, size, n) /*__attribute__ ((aligned (4)))*/                            \
    _attribute_iram_data_ u8              name##_b[size * n] __attribute__((aligned(4))) /*={0}*/; \
    _attribute_data_retention_ tlk_fifo_t name = {size, n, 0, 0, name##_b}

#define MULTI_CONN_FIFO_INIT(name, fifo_size, fifo_num, conn_num)      \
    u8                name##_b[fifo_size * fifo_num * conn_num] = {0}; \
    multi_conn_fifo_t name                                      = {fifo_size, fifo_num, conn_num, name##_b}

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
void tlk_fifo_init(tlk_fifo_t *f, int s, u8 n, u8 *p);

/**
 * @brief  update fifo pointer.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns 0 or new fifo pointer
 */
u8 *tlk_fifo_wptr(tlk_fifo_t *f);

/**
 * @brief  fifo wptr add one.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
void tlk_fifo_next(tlk_fifo_t *f);

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
int tlk_fifo_push(tlk_fifo_t *f, u8 *p, u32 n);

/**
 * @brief  tlk_fifo_pop.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
__INLINE void tlk_fifo_pop(tlk_fifo_t *f)
{
    if (f->rptr != f->wptr) {
        f->rptr++;
    }
}

/**
 * @brief  tlk_fifo_reset.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
__INLINE void tlk_fifo_reset(tlk_fifo_t *f)
{
    f->rptr = f->wptr;
}

/**
 * @brief  get fifo pointer.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
__INLINE u8 *tlk_fifo_get(tlk_fifo_t *f)
{
    if (f->rptr != f->wptr) {
        u8 *p = f->p + (f->rptr & (f->num - 1)) * f->size;
        return p;
    }
    return 0;
}

/**
 * @brief  get fifo pointer.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
__INLINE bool tlk_fifo_is_full(tlk_fifo_t *f)
{
    if (f == NULL) {
        return 1;
    }
    u8 n = f->wptr - f->rptr;
    return n >= f->num;
}

/**
 * @brief  fifo is half full.
 *
 * @param[in]  f: fifo start address
 * @param[out] none
 *
 * @returns none
 */
__INLINE bool tlk_fifo_half_full(tlk_fifo_t *f)
{
    if (f == NULL) {
        return 1;
    }
    uint8_t n = f->wptr - f->rptr;
    return n >= (f->num / 2);
}
#endif
#endif /* FIFO_H_ */
