/********************************************************************************************************
 * @file    share_memory.c
 *
 * @brief   This is the source file for share_memory
 *
 * @author  Zigbee Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
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
#include "share_memory.h"
#include "tl_common.h"

void share_memory_fifo_init(share_mem_fifo_t *smFifo, u8 *p, u32 num, u32 size)
{
    smFifo->fifo.p    = p;
    smFifo->fifo.num  = num;
    smFifo->fifo.size = size;
    smFifo->fifo.rptr = 0;
    smFifo->fifo.wptr = 0;
}

void share_memory_register_fifo_receive_cb(share_mem_fifo_t *smFifo, share_mem_type_e type, share_mem_rx_cb_t cb)
{
    if (smFifo && (type < SHARE_MEMORY_MESSAGE_TYPE_MAX)) {
        smFifo->rxCb[type] = cb;
    }
}

void share_memory_set_fifo_status(share_mem_fifo_t *smFifo, share_mem_status_e status)
{
    smFifo->status = status;
}

share_mem_ret_e share_memory_data_push(share_mem_fifo_t *smFifo, share_mem_type_e type, u8 *data, u32 dataLen)
{
    if ((smFifo->fifo.rptr & (smFifo->fifo.num - 1)) == (((smFifo->fifo.wptr + 1) & (smFifo->fifo.num - 1)))) {
        return SHARE_MEMORY_FULL;
    }

    if ((smFifo == NULL) || (smFifo->status != SHARE_MEMORY_STATUS_READY)) {
        return SHARE_MEMORY_NOT_READY;
    }

    u8 *p = smFifo->fifo.p + (smFifo->fifo.wptr & (smFifo->fifo.num - 1)) * smFifo->fifo.size;
    u32 calLen = (dataLen > (smFifo->fifo.size - 8) ? (smFifo->fifo.size - 8) : dataLen);
    U32_TO_STREAM(p, type);
    U32_TO_STREAM(p, calLen);
    memcpy(p, data, calLen);
    smFifo->fifo.wptr++;

    return SHARE_MEMORY_SUCCESS;
}

int share_memory_data_pop(share_mem_fifo_t *smFifo)
{
    if (smFifo && (smFifo->status == SHARE_MEMORY_STATUS_READY) && (smFifo->fifo.wptr != smFifo->fifo.rptr)) {
        u8 *p = smFifo->fifo.p + (smFifo->fifo.rptr & (smFifo->fifo.num - 1)) * smFifo->fifo.size;
        u32 type = 0;
        u32 calLen = 0;
        BYTE_TO_UINT32(type, p);
        p += 4;
        BYTE_TO_UINT32(calLen, p);
        p += 4;

        if ((smFifo->rxCb[type] != NULL) && (type < SHARE_MEMORY_MESSAGE_TYPE_MAX)) {
            smFifo->rxCb[type](p, calLen);
        }
        smFifo->fifo.rptr++;

        return 1;
    }

    return 0;
}

int share_memory_data_pop_all(share_mem_fifo_t *smFifo)
{
    if ((smFifo == NULL) || (smFifo->status != SHARE_MEMORY_STATUS_READY)) {
        return 0;
    }

    while (smFifo->fifo.wptr != smFifo->fifo.rptr) {
        u8 *p = smFifo->fifo.p + (smFifo->fifo.rptr & (smFifo->fifo.num - 1)) * smFifo->fifo.size;
        u32 type = 0;
        u32 len = 0;
        BYTE_TO_UINT32(type, p);
        p += 4;
        BYTE_TO_UINT32(len, p);
        p += 4;

        if ((smFifo->rxCb[type] != NULL) && (type < SHARE_MEMORY_MESSAGE_TYPE_MAX)) {
            smFifo->rxCb[type](p, len);
        }
        smFifo->fifo.rptr++;
    }

    return 1;
}