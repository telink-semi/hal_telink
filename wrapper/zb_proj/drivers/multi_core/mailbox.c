/********************************************************************************************************
 * @file    mailbox.c
 *
 * @brief   This is the source file for mailbox
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
#include "mailbox.h"
#include "tl_common.h"

#if defined(MCU_CORE_TL322X_D25F)
#define MAILBOX_BUSY()   mailbox_get_irq_status_d25f()
#define MAILBOX_SEND(v)  mailbox_d25f_set_n22_msg(v)
#define MAILBOX_GET(v)   mailbox_d25f_get_n22_msg(v)
#elif defined(MCU_CORE_TL322X_N22)
#define MAILBOX_BUSY()   mailbox_get_irq_status_n22()
#define MAILBOX_SEND(v)  mailbox_n22_set_d25f_msg(v)
#define MAILBOX_GET(v)   mailbox_n22_get_d25f_msg(v)
#endif

static mailbox_rx_cb_t mailboxRxCb;
static mailbox_fifo_t mailboxFifo;
static u8 mailboxTxBuffer[8 * MAILBOX_BUFFER_NUM];

void mailbox_init(mailbox_rx_cb_t cb)
{
    mailboxFifo.p = mailboxTxBuffer;
    mailboxFifo.rptr = 0;
    mailboxFifo.wptr = 0;
    mailboxFifo.num = MAILBOX_BUFFER_NUM;
    mailboxFifo.size = 8;

    if (cb) {
        mailboxRxCb = cb;
    }

#if defined(MCU_CORE_TL322X_D25F)
    mailbox_clr_irq_status_d25f();
    mailbox_set_irq_mask_d25f();
    plic_interrupt_enable(IRQ_MAILBOX_N22_TO_D25);
#elif defined(MCU_CORE_TL322X_N22)
    mailbox_clr_irq_status_n22();
    mailbox_set_irq_mask_n22();
    clic_interrupt_enable(IRQ_IRQ_MAILBOX_D25_TO_N22);
#endif
}

void mailbox_send(u8 *data)
{
    if (MAILBOX_BUSY()) {
        u8 *p = mailboxFifo.p + (mailboxFifo.wptr & (mailboxFifo.num - 1)) * mailboxFifo.size;
        memcpy(p, data, 8);
        mailboxFifo.wptr++;
    } else {
        MAILBOX_SEND((u32 *)data);
    }
}

void mailbox_loop(void)
{
    if (mailboxFifo.wptr != mailboxFifo.rptr) {
        u8 *pData = mailboxFifo.p + (mailboxFifo.rptr & (mailboxFifo.num - 1)) * mailboxFifo.size;

        if (MAILBOX_BUSY()) {
            return;
        } else {
            MAILBOX_SEND((u32 *)pData);
            mailboxFifo.rptr++;
        }
    }
}

volatile u8 T_DBG_mailbox_irq_cnt[4] = {0};
_attribute_ram_code_sec_noinline_ void mailbox_irq_handler(void)
{
    u8 msg[8] = {0};

    T_DBG_mailbox_irq_cnt[0]++;

    if (MAILBOX_BUSY()) {
        T_DBG_mailbox_irq_cnt[1]++;

        MAILBOX_GET((u32 *)msg);

        if (mailboxRxCb) {
            T_DBG_mailbox_irq_cnt[2]++;
            mailboxRxCb(msg);
        }
    }
}

#if defined(MCU_CORE_TL322X_D25F)
PLIC_ISR_REGISTER(mailbox_irq_handler, IRQ_MAILBOX_N22_TO_D25)
#elif defined(MCU_CORE_TL322X_N22)
CLIC_ISR_REGISTER(mailbox_irq_handler, IRQ_IRQ_MAILBOX_D25_TO_N22)
#endif