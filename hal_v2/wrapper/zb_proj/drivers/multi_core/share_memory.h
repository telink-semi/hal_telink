/********************************************************************************************************
 * @file    share_memory.h
 *
 * @brief   This is the header file for share_memory
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
#pragma once

#include "types.h"
#include "mailbox.h"

typedef enum {
    SHARE_MEMORY_STATUS_NOT_READY,
    SHARE_MEMORY_STATUS_READY,
} share_mem_status_e;

typedef enum {
    SHARE_MEMORY_SUCCESS,
    SHARE_MEMORY_NOT_READY,
    SHARE_MEMORY_FULL,
} share_mem_ret_e;

typedef enum {
    SHARE_MEMORY_MESSAGE_TYPE_BLE,
    SHARE_MEMORY_MESSAGE_TYPE_BT,
    SHARE_MEMORY_MESSAGE_TYPE_ZB,
    SHARE_MEMORY_MESSAGE_TYPE_OT,
    SHARE_MEMORY_MESSAGE_TYPE_TPSLL,
    SHARE_MEMORY_MESSAGE_TYPE_LOG_USB,
    SHARE_MEMORY_MESSAGE_TYPE_LOG_UART,
    SHARE_MEMORY_MESSAGE_TYPE_FLASH,
    SHARE_MEMORY_MESSAGE_TYPE_OT_FLASH,
    SHARE_MEMORY_MESSAGE_TYPE_MAX,
} share_mem_type_e;

#define SHARE_MEMORY_CB_NUM     SHARE_MEMORY_MESSAGE_TYPE_MAX

typedef struct _attribute_packed_ {
    mailbox_fifo_t fifo;
    u16 status;
    u16 reserved;
    void (*rxCb[SHARE_MEMORY_CB_NUM])(u8 *, u32);
} share_mem_fifo_t;

typedef void (*share_mem_rx_cb_t)(u8 *, u32);

void share_memory_fifo_init(share_mem_fifo_t *smFifo, u8 *p, u32 num, u32 size);
void share_memory_register_fifo_receive_cb(share_mem_fifo_t *smFifo, share_mem_type_e type, share_mem_rx_cb_t cb);
void share_memory_set_fifo_status(share_mem_fifo_t *smFifo, share_mem_status_e status);
share_mem_ret_e share_memory_data_push(share_mem_fifo_t *smFifo, share_mem_type_e type, u8 *data, u32 dataLen);
int share_memory_data_pop(share_mem_fifo_t *smFifo);
int share_memory_data_pop_all(share_mem_fifo_t *smFifo);