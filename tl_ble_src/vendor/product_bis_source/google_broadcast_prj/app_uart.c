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
#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_GOOGLE_BROADCAST_SOURCE)

#include "tl_common.h"
#include "drivers.h"
#include "app_uart.h"
#include "stack/ble/ble.h"

#define UART1_RTS_ENABLE                gpio_write(UART_RTS_PIN, 1)
#define UART1_RTS_DISABLE               gpio_write(UART_RTS_PIN, 0)


u8 uartRxDmaBuf[UART_BUFF_SIZE];

#define UART_RING_BUFF_SIZE                 4096

u8 uartRingBuf[UART_RING_BUFF_SIZE];
int ringWptr = 0;
int ringRptr = 0;

_attribute_ram_code_ void uart1_irq_handler(void)
{
    if(uart_get_irq_status(UART_PORT, UART_RXDONE))
    {
        uart_clr_irq_status(UART_PORT, UART_CLR_RX);
        dma_chn_dis(UART_RX_DMA);
        /*In order to be able to receive data of unknown length(A0 doesn't support),the DMA SIZE is set to the longest value 0xffffffff.After entering suspend and wake up, and then continue to receive,
        DMA will no longer move data from uart fifo, because DMA thinks that the last transmission was not completed and must disable dma_chn first.modified by minghai,confirmed qiangkai 2020.11.26.*/
        dma_set_address(UART_RX_DMA,reg_uart_data_buf_adr(UART_PORT),(unsigned int)convert_ram_addr_cpu2bus(uartRxDmaBuf));
        dma_set_size(UART_RX_DMA, UART_BUFF_SIZE, DMA_WORD_WIDTH);
        dma_chn_en(UART_RX_DMA);
    }

}

void app_uart_init(void)
{
    gpio_set_gpio_en(UART_RTS_PIN);
    gpio_set_output_en(UART_RTS_PIN, 1);
    UART1_RTS_ENABLE;

    uart_reset(UART_PORT);
    uart_set_pin(UART_TX_PIN, UART_RX_PIN);
    unsigned short div;
    unsigned char bwpc;
    uart_cal_div_and_bwpc(UART_BAUDRATE, sys_clk.pclk*1000*1000, &div, &bwpc);
    uart_init(UART_PORT, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    uart_set_rx_timeout(UART_PORT, bwpc, 12, UART_BW_MUL2);//[!!important] //UART_BW_MUL2

    uart_set_tx_dma_config(UART_PORT, UART_TX_DMA);
    uart_set_rx_dma_config(UART_PORT, UART_RX_DMA);

    uart_clr_irq_mask(UART_PORT, UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);
    uart_clr_tx_done(UART_PORT);
    dma_clr_irq_mask(UART_RX_DMA, TC_MASK|ABT_MASK|ERR_MASK);
    dma_clr_irq_mask(UART_TX_DMA, TC_MASK|ABT_MASK|ERR_MASK);

    uart_set_irq_mask(UART_PORT, UART_RXDONE_MASK);
    uart_set_irq_mask(UART_PORT, UART_TXDONE_MASK);

    dma_chn_dis(UART_RX_DMA);
    /*In order to be able to receive data of unknown length(A0 doesn't support),the DMA SIZE is set to the longest value 0xffffffff.After entering suspend and wake up, and then continue to receive,
    DMA will no longer move data from uart fifo, because DMA thinks that the last transmission was not completed and must disable dma_chn first.modified by minghai,confirmed qiangkai 2020.11.26.*/
    dma_set_address(UART_RX_DMA,reg_uart_data_buf_adr(UART_PORT),(unsigned int)convert_ram_addr_cpu2bus(uartRxDmaBuf));
    dma_set_size(UART_RX_DMA, UART_BUFF_SIZE, DMA_WORD_WIDTH);
    dma_chn_en(UART_RX_DMA);

    plic_interrupt_enable(UART_PORT == UART0 ? IRQ_UART0:IRQ_UART1);
    plic_set_priority(UART_PORT == UART0 ? IRQ_UART0:IRQ_UART1, 1);
    reg_uart_ctrl2(UART_PORT) = (reg_uart_ctrl2(UART_PORT)&0xf0)|0x80;
}


void app_uart_loop(void)
{
    if(((ringWptr - ringRptr)&(UART_RING_BUFF_SIZE-1)) > (UART_RING_BUFF_SIZE-4*UART_BUFF_SIZE))
    {
        UART1_RTS_ENABLE;
        return ;
    }

    if(((ringWptr - ringRptr)&(UART_RING_BUFF_SIZE-1)) < (UART_RING_BUFF_SIZE>>2))
    {
        UART1_RTS_DISABLE;
    }

    static u32 uartRxDmaTimer = 0;

    if(!clock_time_exceed(uartRxDmaTimer, UART_CHECK_RX_DMA_TIMEOUT))
    {
        return ;
    }
    uartRxDmaTimer = clock_time();

    if(reg_dma_size(UART_RX_DMA) != (UART_BUFF_SIZE/4))
    {
        UART1_RTS_ENABLE;
        sleep_us(40);

        int rxLen = UART_BUFF_SIZE - 4*reg_dma_size(UART_RX_DMA);

        for(int i=0; i<rxLen; i++)
        {
            uartRingBuf[ringWptr++] = uartRxDmaBuf[i];
            ringWptr &= (UART_RING_BUFF_SIZE-1);
        }
        dma_chn_dis(UART_RX_DMA);
        /*In order to be able to receive data of unknown length(A0 doesn't support),the DMA SIZE is set to the longest value 0xffffffff.After entering suspend and wake up, and then continue to receive,
        DMA will no longer move data from uart fifo, because DMA thinks that the last transmission was not completed and must disable dma_chn first.modified by minghai,confirmed qiangkai 2020.11.26.*/
        dma_set_address(UART_RX_DMA,reg_uart_data_buf_adr(UART_PORT),(unsigned int)convert_ram_addr_cpu2bus(uartRxDmaBuf));
        dma_set_size(UART_RX_DMA, UART_BUFF_SIZE, DMA_WORD_WIDTH);
        dma_chn_en(UART_RX_DMA);
    }
}

int sendCount = 0;;

_attribute_ram_code_
int app_uart_getRecvDataLen(void)
{
    return (ringWptr-ringRptr)&(UART_RING_BUFF_SIZE-1);
}

_attribute_ram_code_
int app_uart_getRecvData(u8* outBuf, int maxSize)
{
    int size = (ringWptr-ringRptr)&(UART_RING_BUFF_SIZE-1);
    int resSize = min(size, maxSize);
    sendCount += resSize;

    if(resSize)
        tlkapi_printf(1, "get data size is %d %d %d", resSize, sendCount, size);

    for(int i=0; i<resSize; i++)
    {
        outBuf[i] = uartRingBuf[ringRptr++];
        ringRptr &= (UART_RING_BUFF_SIZE-1);
    }

//  if(resSize)
//      tlkapi_send_string_data(1, "value is ",outBuf, resSize);

    return resSize;
}

#endif
