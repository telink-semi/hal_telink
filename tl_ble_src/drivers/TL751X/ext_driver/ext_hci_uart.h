/********************************************************************************************************
 * @file    ext_hci_uart.h
 *
 * @brief   This is the header file for BLE SDK
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
#ifndef DRIVERS_TL751X_EXT_DRIVER_EXT_HCI_UART_H_
#define DRIVERS_TL751X_EXT_DRIVER_EXT_HCI_UART_H_


#include "common/types.h"
#include "uart.h"

#define UartId_t  uart_num_e

#define UART_DMA_CHN_RX  DMA2
#define UART_DMA_CHN_TX  DMA3

#define UART_DMA_RX_IRQ_CHN  FLD_DMA_CHANNEL2_IRQ
#define UART_DMA_TX_IRQ_CHN  FLD_DMA_CHANNEL3_IRQ

/*** RTS ***/
#define RTS_INVERT    1 /*!< 0: RTS PIN will change from low to high. !! use for RTS auto mode(default auto mode)*/
#define RTS_THRESH    5 /*!< RTS trigger threshold. range: 1-16. */
/*** CTS ***/
#define CTS_POLARITY  1 /*!< 0: low level stop TX; 1: high level stop TX.*/

void HCI_UartInit(uart_num_e uartId,u32 baudrate, u8 *txBuf, u32 len);

static inline void HCI_UartSetPin(uart_num_e uartId,u16 txPin, u16 rxPin)
{
    uart_set_pin(uartId, txPin, rxPin);
}

static inline void HCI_UartSetFlowCtrl(uart_num_e uartId, u16 ctsPin, u16 rtsPin)
{
    uart_rts_config(uartId, rtsPin, RTS_INVERT, true);
    uart_rts_trig_level_auto_mode(uartId, RTS_THRESH);

    uart_cts_config(uartId, ctsPin, CTS_POLARITY);
}

static inline void HCI_UartSetRxDmaBuffer(UartId_t uartId, u8 *buf, u32 len)
{
    uart_receive_dma(uartId, buf, len);
}

static inline unsigned int HCI_UartGetRxDataLen(UartId_t uartId)
{
    unsigned int rxLen = 0;

//  unsigned int buff_data_len =  (reg_uart_status(uartId)&FLD_UART_RBCNT)%4;
//  if(buff_data_len==0){
//      rxLen = 4 * ((pRxFifo->size - 4)/4 - reg_dma_size(UART_DMA_CHN_RX));
//  }else{
//      rxLen = 4 * ((pRxFifo->size - 4)/4 - reg_dma_size(UART_DMA_CHN_RX)-1) + buff_data_len;
//  }
//  rxLen = uart_get_dma_rev_data_len(uartId, UART_DMA_CHN_RX);

    return rxLen;
}

//Rx IRQ
static inline bool HCI_UartGetRxIrqFlag(UartId_t uartId)
{
    return uart_get_irq_status(uartId, UART_RXDONE_IRQ_STATUS);
}

static inline void HCI_UartClearRxIrqFlag(UartId_t uartId)
{
    uart_clr_irq_status(uartId, UART_RXDONE_IRQ_STATUS);
}

//Rx error IRQ
static inline bool HCI_UartGetRxErrorIrqFlag(UartId_t uartId)
{
    return uart_get_irq_status(uartId, UART_RX_ERR);
}

static inline void HCI_UartClearRxErrorIrqFlag(UartId_t uartId)
{
    uart_clr_irq_status(uartId, UART_RXBUF_IRQ_STATUS);
}

//Tx IRQ
static inline bool HCI_UartGetTxIrqFlag(UartId_t uartId)
{
    return uart_get_irq_status(uartId,UART_TXDONE_IRQ_STATUS);
}

static inline void HCI_UartClearTxIrqFlag(UartId_t uartId)
{
    uart_clr_irq_status(uartId, UART_TXDONE_IRQ_STATUS);
}

#endif /* DRIVERS_TL751X_EXT_DRIVER_EXT_HCI_UART_H_ */
