/********************************************************************************************************
 * @file    mailbox_service.h
 *
 * @brief   This is the header file for mailbox_service
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

#ifndef MAILBOX_MSG_CB_MAX_NUM
#define MAILBOX_MSG_CB_MAX_NUM      16
#endif

typedef enum {
    MAILBOX_MSG_TX_FIFO,
    MAILBOX_MSG_RX_FIFO,
    MAILBOX_MSG_NV_FIFO,
    MAILBOX_MSG_OT_NV_FIFO,
} mailbox_service_cmd_e;

typedef enum {
    MAILBOX_SERVICE_SUCCESS,
    MAILBOX_SERVICE_EXIST,
    MAILBOX_SERVICE_INVALID,
    MAILBOX_SERVICE_FULL
} mailbox_service_status_e;

typedef void(*mailbox_msg_cb_t)(u8 *data);

typedef struct {
    mailbox_msg_cb_t cb;
    u8 cmd;
} mailbox_service_t;

mailbox_service_status_e mailbox_service_cb_register(u8 cmd, mailbox_msg_cb_t cb);
void mailbox_service_data_send(u8 cmd, u8 *data);
void mailbox_service_flush(void);
void mailbox_service_init(void);
