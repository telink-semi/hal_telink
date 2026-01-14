/********************************************************************************************************
 * @file    mailbox_service.c
 *
 * @brief   This is the source file for mailbox_service
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
#include "mailbox_service.h"
#include "mailbox.h"
#include "tl_common.h"

static mailbox_service_t mailbox_service[MAILBOX_MSG_CB_MAX_NUM];
static u8 mailbox_service_num = 0;

mailbox_service_status_e mailbox_service_cb_register(u8 cmd, mailbox_msg_cb_t cb)
{
    if (mailbox_service_num >= MAILBOX_MSG_CB_MAX_NUM) {
        return MAILBOX_SERVICE_FULL;
    }

    if (cb == NULL) {
        return MAILBOX_SERVICE_INVALID;
    }

    for (u8 i = 0; i < mailbox_service_num; i++) {
        if (mailbox_service[i].cmd == cmd) {
            return MAILBOX_SERVICE_EXIST;
        }
    }

    mailbox_service[mailbox_service_num].cmd = cmd;
    mailbox_service[mailbox_service_num].cb = cb;
    mailbox_service_num++;

    return MAILBOX_SERVICE_SUCCESS;
}

_attribute_ram_code_ static void mailbox_service_cb_process(u8 *msg)
{
    u8 cmd = msg[0];
    u8 *data = &msg[1];

    for (u8 i = 0; i < mailbox_service_num; i++) {
        if (mailbox_service[i].cmd == cmd) {
            mailbox_service[i].cb(data);
            return;
        }
    }
}

void mailbox_service_data_send(u8 cmd, u8 *data)
{
    u8 msg[8] = {0};

    msg[0] = cmd;
    memcpy(&msg[1], data, 7);

    mailbox_send(msg);
}

void mailbox_service_flush(void)
{
    mailbox_loop();
}

void mailbox_service_init(void)
{
    mailbox_init(mailbox_service_cb_process);
}