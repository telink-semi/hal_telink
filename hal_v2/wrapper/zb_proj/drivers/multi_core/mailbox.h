/********************************************************************************************************
 * @file    mailbox.h
 *
 * @brief   This is the header file for mailbox
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

#ifndef MAILBOX_BUFFER_NUM
#define MAILBOX_BUFFER_NUM  32
#endif

typedef struct {
    u8 *p;
    u32 size;
    u32 num;
    u32 wptr;
    u32 rptr;
} mailbox_fifo_t;

typedef void (*mailbox_rx_cb_t)(u8 *msg);

void mailbox_init(mailbox_rx_cb_t cb);
void mailbox_send(u8 *data);
void mailbox_loop(void);
