/********************************************************************************************************
 * @file    app_uart.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "../bis_sink_config.h"
#if (PRODUCT_BIS_SINK_SELECT == PRODUCT_GOOGLE_BROADCAST_SINK)

#include "tl_common.h"
#include "drivers.h"
#include "app_uart.h"

typedef struct{
    int size;
    u8 txBuf[UART_TX_BUFF_SIZE];
} uart_tx_t;

#define UART_TX_RING_BUFF_COUNT             16

uart_tx_t uartTxDmaBuf[UART_TX_RING_BUFF_COUNT];

int txWptr = 0;
int txRptr = 0;
int uartTxState = 0;

_attribute_ram_code_ void uart1_irq_handler(void)
{
    if(uart_get_irq_status(UART_PORT, UART_TXDONE))
    {
        uart_clr_tx_done(UART_PORT);
        txRptr = (txRptr+1)&(UART_TX_RING_BUFF_COUNT-1);
        if(txRptr == txWptr)
        {
            uartTxState = 0;
        }
        else
        {
            uart_send_dma(UART_PORT, uartTxDmaBuf[txRptr].txBuf, uartTxDmaBuf[txRptr].size);
        }
    }
}

void app_uart_init(void)
{
    uart_reset(UART_PORT);
    uart_set_pin(UART_TX_PIN, UART_RX_PIN);
    unsigned short div;
    unsigned char bwpc;
    uart_cal_div_and_bwpc(UART_BAUDRATE, sys_clk.pclk*1000*1000, &div, &bwpc);
    uart_init(UART_PORT, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    uart_set_tx_dma_config(UART_PORT, UART_TX_DMA);
    uart_set_rx_dma_config(UART_PORT, UART_RX_DMA);

    uart_set_rts_en(UART_PORT);
    uart_rts_config(UART_PORT, UART_RTS_PIN, 1, UART_RTS_MODE_AUTO);
    uart_rts_trig_level_auto_mode(UART_PORT, 5);

    uart_clr_irq_mask(UART_PORT, UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);
    uart_clr_tx_done(UART_PORT);
    dma_clr_irq_mask(UART_TX_DMA, TC_MASK|ABT_MASK|ERR_MASK);

    uart_set_rx_timeout(UART_PORT, bwpc, 12, UART_BW_MUL2);
//  uart_set_irq_mask(UART_PORT, UART_RXDONE_MASK);
    uart_set_irq_mask(UART_PORT, UART_TXDONE_MASK);

    plic_interrupt_enable(UART_PORT == UART0 ? IRQ_UART0:IRQ_UART1);
    plic_set_priority(UART_PORT == UART0 ? IRQ_UART0:IRQ_UART1, 2);

}

void app_uart_send_value(u8* inBuf, int size)
{
    uartTxDmaBuf[txWptr].size = min(UART_TX_BUFF_SIZE, size);
    memcpy(uartTxDmaBuf[txWptr].txBuf, inBuf, uartTxDmaBuf[txWptr].size);
    if(uartTxState == 0)
    {
        uart_send_dma(UART_PORT, uartTxDmaBuf[txWptr].txBuf, uartTxDmaBuf[txWptr].size);
        uartTxState = 1;
    }
    txWptr = (txWptr+1)&(UART_TX_RING_BUFF_COUNT-1);
}

#endif
