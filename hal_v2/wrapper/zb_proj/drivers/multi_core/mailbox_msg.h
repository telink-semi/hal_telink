/********************************************************************************************************
 * @file    mailbox_msg.h
 *
 * @brief   This is the header file for mailbox_msg
 *
 * @author  Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *			All rights reserved.
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
#ifndef _MAILBOX_MSG_H_
#define _MAILBOX_MSG_H_

#include "share_memory.h"

void mailbox_msg_init(void);
void mailbox_msg_loop(void);
void mailbox_msg_nv_task(void);
bool mailbox_msg_tx_fifo_is_prepared(void);
bool mailbox_msg_rx_fifo_is_prepared(void);
bool mailbox_msg_nv_fifo_is_prepared(void);
void mailbox_msg_tx(share_mem_type_e type, u8 *data, u32 len);
void mailbox_msg_rx_cb_register(share_mem_type_e type, share_mem_rx_cb_t cb);

#endif /* _MAILBOX_MSG_H_ */