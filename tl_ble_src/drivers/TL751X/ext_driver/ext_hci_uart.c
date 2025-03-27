/********************************************************************************************************
 * @file    ext_hci_uart.c
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
#include "ext_hci_uart.h"
#include "clock.h"

void HCI_UartInit(uart_num_e uartId, u32 baudrate, u8 *rxBuf, u32 len)
{

}



#if 0
static u8 rxBuf[128];

void HCI_InitUart(void)
{
    HCI_Tr_InitUart(1, rxBuf, sizeof(rxBuf));
    HCI_Tr_TimeInit(100);
}

void HCI_Tr_InitUart(u8 *rxBuf, u32 byteNum)
{
    u16 div = 0;
    u8 bwpc = 0;

    uart_reset(uartId);

    //uart_set_pin(HCI_TR_TX_PIN, HCI_TR_RX_PIN);//UART0_TX_PB2,UART0_RX_PB3
    //uart_set_pin(UART0_TX_PA3,UART0_RX_PA4);

    uart_cal_div_and_bwpc(HCI_TR_BAUDRATE, sys_clk.pclk*1000*1000, &div, &bwpc);
    uart_init(HCI_TR_UART_ID, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    if(isDmaMode)
    {
        uart_set_rx_timeout(HCI_TR_UART_ID, bwpc, 12, UART_BW_MUL3);//[!!important] //UART_BW_MUL2

        uart_set_rx_dma_config(HCI_TR_UART_ID, UART_DMA_CHN_RX);
        uart_set_tx_dma_config(HCI_TR_UART_ID, UART_DMA_CHN_TX);

        uart_clr_irq_status(HCI_TR_UART_ID,UART_TXDONE_IRQ_STATUS);

        dma_clr_irq_mask(UART_DMA_CHN_TX, TC_MASK|ABT_MASK|ERR_MASK);//disable UART DMA TX IRQ
        dma_clr_irq_mask(UART_DMA_CHN_RX, TC_MASK|ABT_MASK|ERR_MASK);//disable UART DMA RX IRQ
    }

    uart_set_irq_mask(HCI_TR_UART_ID, UART_RXDONE_MASK);//enable UART RX IRQ
    uart_set_irq_mask(HCI_TR_UART_ID, UART_TXDONE_MASK);//enable UART TX IRQ
//  uart_clr_irq_mask(HCI_TR_UART_ID, UART_TXDONE_MASK);
    plic_interrupt_enable(HCI_TR_UART_IRQn);        //enable UART global IRQ

    uart_receive_dma(HCI_TR_UART_ID, rxBuf, byteNum);   //set UART DMA RX buffer.
}

_attribute_ram_code_
void HCI_UART_IRQHandler(void)
{
#if 1
    HCI_Tr_UartIRQHandler();

#else//for test
    if(uart_get_irq_status(HCI_TR_UART_ID, UART_RXDONE)) //A0-SOC can't use RX-DONE status,so this interrupt can only used in A1-SOC.
    {
        if((uart_get_irq_status(HCI_TR_UART_ID, UART_RX_ERR)))
        {
            uart_clr_irq_status(HCI_TR_UART_ID, UART_CLR_RX);//it will clear rx_fifo and rx_err_status,rx_done_irq.
        }

        /* Get the length of Rx data */
        u32 rxLen = 0;
        if(((reg_uart_status1(HCI_TR_UART_ID) & FLD_UART_RBCNT) % 4)==0){
            rxLen = 4 * (0xffffff - reg_dma_size(UART_DMA_CHN_RX));
        }
        else{
            rxLen = 4 * (0xffffff - reg_dma_size(UART_DMA_CHN_RX)-1) + (reg_uart_status1(HCI_TR_UART_ID) & FLD_UART_RBCNT) % 4;
        }

        if(rxLen && !uart_tx_is_busy(HCI_TR_UART_ID)){
            uart_send_dma(HCI_TR_UART_ID, rxBuf, rxLen);
        }

        /* Clear RxDone state */
        uart_clr_irq_status(HCI_TR_UART_ID, UART_CLR_RX);
        uart_receive_dma(HCI_TR_UART_ID, rxBuf, sizeof(rxBuf));//[!!important - must]
    }

    if(uart_get_irq_status(HCI_TR_UART_ID, UART_TXDONE))
    {
        uart_clr_irq_status(HCI_TR_UART_ID,UART_TXDONE_IRQ_STATUS);
    }
#endif
}


_attribute_ram_code_
void HCI_TIMER_IRQHandler(void)
{
    HCI_Tr_TimerIRQHandler();
}


#endif

