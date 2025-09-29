/********************************************************************************************************
 * @file    mailbox_msg.c
 *
 * @brief   This is the source file for mailbox_msg
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
#include "mailbox_msg.h"
#include "mailbox_service.h"
#include "utility.h"

static share_mem_fifo_t *sm_tx_fifo = NULL;
static share_mem_fifo_t *sm_rx_fifo = NULL;
static share_mem_fifo_t *sm_nv_fifo = NULL;

bool mailbox_msg_tx_fifo_is_prepared(void)
{
    return (sm_tx_fifo != NULL) ? TRUE: FALSE;
}

bool mailbox_msg_rx_fifo_is_prepared(void)
{
    return (sm_rx_fifo != NULL) ? TRUE: FALSE;
}

bool mailbox_msg_nv_fifo_is_prepared(void)
{
    return (sm_nv_fifo != NULL) ? TRUE: FALSE;
}


void mailbox_msg_fifo_set(share_mem_fifo_t *fifo,mailbox_service_cmd_e type)
{
    if(fifo== NULL) return;
    
    if(type == MAILBOX_MSG_TX_FIFO) {
        sm_tx_fifo = fifo;
    } else if(type == MAILBOX_MSG_RX_FIFO) {
        sm_rx_fifo = fifo;
    } else if(type == MAILBOX_MSG_NV_FIFO) {
        sm_nv_fifo = fifo;
    }

}


void mailbox_msg_rx_cb_register(share_mem_type_e type, share_mem_rx_cb_t cb)
{
    switch (type) {
        case SHARE_MEMORY_MESSAGE_TYPE_BLE:
        case SHARE_MEMORY_MESSAGE_TYPE_ZB:
        case SHARE_MEMORY_MESSAGE_TYPE_OT:
            if (sm_rx_fifo) {
                share_memory_register_fifo_receive_cb(sm_rx_fifo, type, cb);
            }
            break;
        case SHARE_MEMORY_MESSAGE_TYPE_FLASH:
            if (sm_nv_fifo) {
                share_memory_register_fifo_receive_cb(sm_nv_fifo, type, cb);
            }
            break;
        default:
            break;
    }
}

void mailbox_msg_tx(share_mem_type_e type, u8 *data, u32 len)
{
    u8 ret = share_memory_data_push(sm_tx_fifo, type, data, len);

    printf("MSG_TX: [sta = %x] [", ret);
    for (u32 i = 0; i < len; i++) {
        printf(" %02x", data[i]);
    }
    printf("]\n");
}

static void mailbox_msg_tx_fifo_handler(u8 *data)
{
    sm_tx_fifo = (share_mem_fifo_t *)BUILD_U32(data[3], data[4], data[5], data[6]);
    share_memory_set_fifo_status(sm_tx_fifo, SHARE_MEMORY_STATUS_READY);
    printf("sm_tx_fifo: %x\n", (u32)sm_tx_fifo);
}

static void mailbox_msg_rx_fifo_handler(u8 *data)
{
    sm_rx_fifo = (share_mem_fifo_t *)BUILD_U32(data[3], data[4], data[5], data[6]);
    share_memory_set_fifo_status(sm_rx_fifo, SHARE_MEMORY_STATUS_READY);
    printf("sm_rx_fifo: %x\n", (u32)sm_rx_fifo);
}

static void mailbox_msg_nv_fifo_handler(u8 *data)
{
    sm_nv_fifo = (share_mem_fifo_t *)BUILD_U32(data[3], data[4], data[5], data[6]);
    share_memory_set_fifo_status(sm_nv_fifo, SHARE_MEMORY_STATUS_READY);
    printf("sm_nv_fifo: %x\n", (u32)sm_nv_fifo);
}
#if 0
void mailbox_msg_init(void)
{
    mailbox_service_cb_register(MAILBOX_MSG_TX_FIFO, mailbox_msg_tx_fifo_handler);
    mailbox_service_cb_register(MAILBOX_MSG_RX_FIFO, mailbox_msg_rx_fifo_handler);
    mailbox_service_cb_register(MAILBOX_MSG_NV_FIFO, mailbox_msg_nv_fifo_handler);

    mailbox_service_init();
}
#endif


void mailbox_msg_loop(void)
{
    mailbox_service_flush();

    if (sm_tx_fifo) {
        share_memory_data_pop(sm_tx_fifo);
    }
}

void mailbox_msg_nv_task(void)
{
    if (sm_nv_fifo) {
        share_memory_data_pop(sm_nv_fifo);
    }
}
