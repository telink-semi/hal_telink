/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

/********************************************************************************************************
 * @file	uart.h
 *
 * @brief	This is the header file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
/**	@page UART
 *
 *	Introduction
 *	===============
 *	B92 supports two uart: uart0~ uart1.
 *	-# support nodma/dma
 *	-# support cts/rts
 *	-# support s7816
 *	-# support chain transport
 *
 *	API Reference
 *	===============
 *	Header File: uart.h
 *
 *	How to use this driver
 *	==============
- The uart two working modes nodma/dma,can be used as follows:
     -# UART Initialization and Configuration
         -# dma/nodma:
 	         - To prevent the uart module from storing history information, before use call uart_reset API;
 	         - Initializes the tx/rx pin by uart_set_pin API;
 	         - Configure the baud rate/stop bit/ parity by uart_cal_div_and_bwpc/uart_init API;
 	         - Configure rx_timeout by uart_set_rx_timeout;
 	     -# dma
 	         - dma initial configuration by uart_set_tx_dma_config/uart_set_rx_dma_config API;
 	 -# UART interrupts handling
 	     -# nodma:data is received via rx_irq interrupt and rx_done interrupt,and err receive detection.
 	         - rx_irq interrupt:when the number of rxfifo reaches the threshold uart_rx_irq_trig_level, the rx_irq is interrupted,
 	           configured by uart_rx_irq_trig_level/uart_set_irq_mask(UART_RX_IRQ_MASK) API;
 	         - rx_done interrupt:when rx is timeout, rx_done is generated to receive the data that rx_fifo does not meet the set threshold,
 	           configured by uart_set_irq_mask(UART_RXDONE_MASK) API;
 	         - detect the UART receives data incorrectly(such as a parity error or a stop bit error),configured by uart_set_irq_mask(UART_ERR_IRQ_MASK) API;
 	         - uart module interrupt enable and total interrupt enable by plic_interrupt_enable/core_interrupt_enable API;
 	     -# dma:use tx_done and rx_done to check whether the sending and receiving are complete,and err receive detection when rx_done.
 	         - tx_done:use the tx_done of the uart module,use uart_set_irq_mask(UART_TXDONE_MASK);
 	         - rx_done:when the received length configured for rx_dma is less than 0xfffffc, use rx_done of uart module rx_done,use uart_set_irq_mask(UART_RXDONE_MASK);
 	                   When the received length configured for rx_dma is 0xfffffc, use rx_done of the dma module,use dma_set_irq_mask(TC_MASK);
 	         - uart module interrupt or dma module interrupt enable and total interrupt by plic_interrupt_enable/core_interrupt_enable API;
     -# UART TX and RX
        -# nodma_tx
            - It can be sent in byte/half word/word polling, use uart_send_byte/uart_send_hword/uart_send_word API;
        -# nodma_rx
            - data is received via rx_irq interrupt and rx_done interrupt,the data is read in the interrupt by uart_read_byte API;
        -# dma_tx
            - send data by uart_send_dma API;
        -# dma_rx
            - receive data by uart_receive_dma API;
     -# Note:there are two types of dma receive:
         -# when the received length configured for rx_dma is less than 0xfffffc:
            - the receive length needs to be calculated manually by uart_get_dma_rev_data_len API;
            - Advantages of this method: prevents rx_buff from crossing boundaries;
            - Possible problems:
                 - the receive length cannot be greater than the DMA set length,otherwise excess data will be discarded.
                 - In an application, there may be a high priority interrupt source that interrupts the uart rx_done interrupt processing sequence,
                   in interruption period,if the next data is received to the fifo, when the re-entry of the UART RX_DONE interrupt,resulting in the error in computing the received length of the current data.
                 - The two packets of data are very close to each other, but the rx_done signal of the previous data has also been generated. Before the rx_done interrupt flag and rx_fifo software are cleared,
                   the next data has been transferred, which leads to the error of clearing.
         -# when the received length configured for rx_dma is 0xfffffc:
            - The receive length hardware automatically writes back to the first four bytes of rxbuf;
            - Possible problems:
                 - Because the dma receive length is set to 0xfffffc byte, it is possible that the dma receive length is larger than the rec_buff set length,
                   which is out of the rec_buff's address range.

- The UART flow control CTS/RTS
     -# CTS(when the cts pin receives an active level, it stops sending data)
            - Configure the cts pin and polarity,by uart_cts_config API;
            - enable cts,by uart_set_cts_en;
     -# RTS(In automatic mode,when the condition is reached, the rts active level is activated, and in manual mode, the rts level can be manually controlled)
            - Configure the rts pin and polarity,by uart_rts_config API;
            - enable rts,by uart_set_rts_en;
            - note: the rts has two modes:manual and auto
                - manual: can change the RTS pin's level manually,the related interfaces are as follows: uart_rts_manual_mode/uart_set_rts_level API;
                - auto:When the set conditions are met, rts is activated,there are two conditions as follows:
                     - When the number of rxfifo reaches the set threshold, rts is activated,when rx_fifo is less than the set threshold then rts will go down;
                       the related interfaces are as follows:uart_rts_auto_mode/uart_rts_trig_level_auto_mode API;
                     - Generates an rx_done signal,rts is activated,When rx_done is cleared, the rts signal automatically goes down,
                       the related interfaces are as follows:uart_rts_auto_mode/uart_rxdone_rts_en API;
                - The following two interfaces are configured by default in uart_init: uart_rts_stop_rxtimeout_dis()/uart_rxdone_rts_dis()

- Support S7816
     -# S7816(used to communicate with the s7816,for details, see driver s7816)
       - This is just to talk specifically about the rtx function:
          - Configure the rtx pin and enable,by uart_set_rtx_pin/uart_rtx_en API;
          - this pin can be used as either tx or rx,it is the rx function by default,When there is data in tx_fifo and the interface uart_rtx_pin_tx_trig is called,
                  it is converted to tx function until tx_fifo is empty and converted to rx.

- Chain Transfer
     -# Only rx supports chain transfer, and only supports the dma receive length is set to 0xfffffc.
       - single chain: uart_rx_dma_chain_init API;
       - Mul_chain:uart_set_dma_chain_llp/uart_rx_dma_add_list_element API;
 */
#ifndef     UART_H_
#define     UART_H_

#include "gpio.h"
#include "dma.h"
#include "reg_include/register.h"
#include "timer.h"

extern unsigned char uart_rx_byte_index[2];
extern unsigned char uart_tx_byte_index[2];

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/
/**
 *  @brief  Define parity type
 */
typedef enum {
	UART_PARITY_NONE = 0,
	UART_PARITY_EVEN,
	UART_PARITY_ODD,
} uart_parity_e;

/**
 *  @brief  Define UART chn
 */
typedef enum {
	UART0 = 0,
	UART1,
}uart_num_e;

/**
 *  @brief  Define mul bits
 */
typedef enum{
	UART_BW_MUL1  = 0,
	UART_BW_MUL2  = 1,
	UART_BW_MUL3  = 2,
	UART_BW_MUL4  = 3,
} uart_timeout_mul_e;

/**
 *  @brief  Define the length of stop bit
 */
typedef enum {
	UART_STOP_BIT_ONE          = 0,
	UART_STOP_BIT_ONE_DOT_FIVE = BIT(4),
	UART_STOP_BIT_TWO          = BIT(5),
} uart_stop_bit_e;

/**
 *  @brief  Define UART RTS mode
 */
typedef enum {
    UART_RTS_MODE_AUTO = 0,
    UART_RTS_MODE_MANUAL,
} uart_rts_mode_e;

/**
 *  @brief  Define UART IRQ MASK.The enumeration variable is just a index, and actually needs to be operated registers behind.
 */
typedef enum{
	UART_RX_IRQ_MASK  = BIT(2),//reg_uart_rx_timeout1(uart_num) BIT(2)
	UART_TX_IRQ_MASK  = BIT(3),//reg_uart_rx_timeout1(uart_num) BIT(3)
	UART_RXDONE_MASK  = BIT(4),//reg_uart_rx_timeout1(uart_num) BIT(4)
	UART_TXDONE_MASK  = BIT(5),//reg_uart_rx_timeout1(uart_num) BIT(5)
	UART_ERR_IRQ_MASK = BIT(6),//reg_uart_rx_timeout1(uart_num) BIT(6)
}uart_irq_mask_e;

/**
 *  @brief  Define UART IRQ BIT STATUS
 *  -# UART_RXBUF_IRQ_STATUS:When the number of rxfifo reaches the set threshold(uart_rx_irq_trig_level), an interrupt is generated, and the interrupt flag is automatically cleared;
 *  -# UART_TXBUF_IRQ_STATUS:When the number of txfifo is less than or equal to the set threshold(uart_tx_irq_trig_level), an interrupt is generated and the interrupt flag is automatically cleared;
 *  -# UART_RXDONE_IRQ_STATUS:no data is received in rx_timeout, rx_done is generated. If uart_auto_clr_rx_fifo_ptr is enabled, the interrupt flag is automatically cleared. If uart_auto_clr_rx_fifo_ptr is disabled, the interrupt flag must be manually cleared;
 *  -# UART_TXDONE_IRQ_STATUS:when there is no data in the tx_fifo, tx_done is generated, and the interrupt flag bit needs to be manually cleared;
 *  -# UART_RX_ERR:when the UART receives data incorrectly(such as a parity error or a stop bit error), the interrupt is generated,the interrupt flag bit needs to be manually cleared;
 */
typedef enum{
	UART_RXBUF_IRQ_STATUS 	     =  BIT(2), //reg_uart_irq(uart_num) BIT(2)
	UART_TXBUF_IRQ_STATUS 	     =  BIT(3), //reg_uart_irq(uart_num) BIT(3)
	UART_RXDONE_IRQ_STATUS       =  BIT(4), //reg_uart_irq(uart_num) BIT(4)
	UART_TXDONE_IRQ_STATUS       =  BIT(5), //reg_uart_irq(uart_num) BIT(5)
	UART_RX_ERR                  =  BIT(6), //reg_uart_irq(uart_num) BIT(6)
}uart_irq_status_e;


typedef enum{
	UART_NO_DMA_MODE  =0,
	UART_DMA_MODE     =1,
}uart_rxdone_sel_e;


#define uart_rtx_pin_tx_trig(uart_num)  uart_clr_irq_status(uart_num,UART_TXDONE_IRQ_STATUS)

/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/
/**
 * @brief     Get the rxfifo cnt,when data enters rxfifo, the rxfifo cnt increases,When reading data from rx_fifo, rxfifo cnt decays.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline unsigned char uart_get_rxfifo_num(uart_num_e uart_num)
{
	return reg_uart_buf_cnt(uart_num)&FLD_UART_RX_BUF_CNT ;
}

/**
 * @brief     Get the txfifo cnt,tx_fifo cnt decreases when data is sent from tx_fifo, and tx_fifo cnt increases when data is written to tx_fifo.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline unsigned char uart_get_txfifo_num(uart_num_e uart_num)
{
	return (reg_uart_buf_cnt(uart_num)&FLD_UART_TX_BUF_CNT )>>4;
}

/**
 * @brief     Resets uart module,before using uart, need to call uart_reset() to avoid affecting the use of uart.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_reset(uart_num_e uart_num)
{

	reg_rst0 &= (~((uart_num)?FLD_RST0_UART1:FLD_RST0_UART0));
	reg_rst0 |= ((uart_num)?FLD_RST0_UART1:FLD_RST0_UART0);
}

/**
 * @brief     Enable the clock of UART module.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline void uart_clk_en(uart_num_e uart_num)
{
	reg_clk_en0 |= ((uart_num)?FLD_CLK0_UART1_EN:FLD_CLK0_UART0_EN);
}
/**
 * @brief     Choose rxdone(UART_RXDONE_IRQ_STATUS) function,nodma needs to be opened, dma needs to be closed, internal interface, the upper layer does not need to be called.
 * @param[in] uart_num - UART0/UART1.
 * @param[in] sel - 0:no_dma  1:dma
 * @return    none.
 */
static inline void uart_rxdone_sel(uart_num_e uart_num,uart_rxdone_sel_e sel)
{
	if(sel==UART_NO_DMA_MODE){
	   reg_uart_ctrl0(uart_num)|= FLD_UART_NDMA_RXDONE_EN;
	}
	else if(sel==UART_DMA_MODE){
		 reg_uart_ctrl0(uart_num)&= ~FLD_UART_NDMA_RXDONE_EN;
	}
}

/**
 * @brief     Select whether to enable auto clr rx fifo pointer.
 * @param[in] uart_num - UART0/UART1.
 * @param[in] en - 1:enable, when UART_RXDONE_IRQ_STATUS trigger,the hardware will automatically clear the rx_fifo pointer,the software only needs to clear UART_RXDONE_IRQ_STATUS interrupt flag bit.
 *                 0:disable,the rxfifo pointer is cleared only when the software clears the UART_RXDONE_IRQ_STATUS interrupt flag bit.
 * @return    none.
 * @note      this function cannot be turned on when no_dma and dma is configured to a certain length rather than a maximum length.
 */
static inline void uart_auto_clr_rx_fifo_ptr(uart_num_e uart_num,unsigned char en){
	if(en==1){
		reg_uart_ctrl0(uart_num)|=FLD_UART_RX_CLR_EN;
	}
	else if(en==0){
		reg_uart_ctrl0(uart_num)&=~FLD_UART_RX_CLR_EN;
	}
}
/**
 * @brief     Configure the trigger level of the UART_RXBUF_IRQ_STATUS interrupt,when the number of rx_fifo is greater than or equal to the trigger level, UART_RXBUF_IRQ_STATUS interrupt rises.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] rx_level - the trigger level,the range is less than 8.
 * @return    none
 * @note      This interface is only used in no_dma mode.
 */
static inline void uart_rx_irq_trig_level(uart_num_e uart_num,unsigned char rx_level)
{
	uart_rxdone_sel(uart_num,UART_NO_DMA_MODE);
	uart_auto_clr_rx_fifo_ptr(uart_num,0);
	reg_uart_ctrl3(uart_num) = (reg_uart_ctrl3(uart_num) & (~FLD_UART_RX_IRQ_TRIQ_LEV)) | (rx_level & 0x0f);
}

/**
 * @brief     Configure the trigger level of the UART_TXBUF_IRQ_STATUS interrupt,when the number of tx_fifo data is less than or equal to the trigger level, UART_TXBUF_IRQ_STATUS interrupt rises.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] tx_level - the trigger level,the range is less than 8.
 * @return    none
 * @note      This interface is only used in no_dma mode.
 */
static inline void uart_tx_irq_trig_level(uart_num_e uart_num,unsigned char tx_level)
{
	reg_uart_ctrl3(uart_num) = (reg_uart_ctrl3(uart_num) & (~FLD_UART_TX_IRQ_TRIQ_LEV)) | (tx_level << 4);
}


/**
 * @brief     Enable the irq of uart.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] mask     - enum uart irq mask.
 * @return    none
 */
static inline void uart_set_irq_mask(uart_num_e uart_num,uart_irq_mask_e mask)
{
	reg_uart_mask(uart_num)|=mask;
}

/**
 * @brief     Disable the irq of uart.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] mask     - enum uart irq mask.
 * @return    none
 */
static inline void uart_clr_irq_mask(uart_num_e uart_num,uart_irq_mask_e mask)
{
	reg_uart_mask(uart_num)&=~mask;
}

/**
 * @brief     Set the 'uart_rx_byte_index' to 0,'uart_rx_byte_index' is used to synchronize the rxfifo hardware pointer in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none.
 * @note      note the following:
 *            -# After calling the uart_reset interface, uart_clr_tx_index and uart_clr_rx_index must be called to clear the read/write pointer,
 *               after the uart_reset interface is invoked, the hardware read and write Pointers are cleared to zero.
 *               Therefore, the software read and write Pointers are cleared to ensure logical correctness.
 *            -# After suspend wakes up, you must call uart_clr_tx_index and uart_clr_rx_index to clear read and write pointers,
 *               because after suspend wakes up, the chip is equivalent to performing a uart_reset,
 *               so the software read and write pointer also needs to be cleared to zero.
 */
static inline void uart_clr_rx_index(uart_num_e uart_num)
{
	uart_rx_byte_index[uart_num]=0;
}


/**
 * @brief     Set the 'uart_tx_byte_index' to 0,'uart_tx_byte_index' is used to synchronize the txfifo hardware pointer in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none.
 * @note      note the following:
 *            -# After calling the uart_reset interface, uart_clr_tx_index and uart_clr_rx_index must be called to clear the read/write pointer,
 *               after the uart_reset interface is invoked, the hardware read and write Pointers are cleared to zero.
 *               Therefore, the software read and write Pointers are cleared to ensure logical correctness.
 *            -# After suspend wakes up, you must call uart_clr_tx_index and uart_clr_rx_index to clear read and write pointers,
 *               because after suspend wakes up, the chip is equivalent to performing a uart_reset,
 *               so the software read and write pointer also needs to be cleared to zero.
 */
static inline void uart_clr_tx_index(uart_num_e uart_num)
{
	uart_tx_byte_index[uart_num]=0;
}

/**
 * @brief     Get the irq status of uart.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] status   - enum uart irq status.
 * @return    irq status
 */
static inline unsigned int  uart_get_irq_status(uart_num_e uart_num,uart_irq_status_e status)
{
	return reg_uart_irq(uart_num)&status;
}

/**
 * @brief     Clear the irq status of uart.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] status   - enum uart irq status.
 * @return    none
 */

static inline void uart_clr_irq_status(uart_num_e uart_num,uart_irq_status_e status)
{
/*
 * the purpose of judging the status of UART_RXDONE_IRQ_STATUS:
 * when UART_RXDONE_IRQ_STATUS is cleared, the UART_RXBUF_IRQ_STATUS is also cleared, because rx_fifo is cleared, and the software pointer is also cleared:
 * dma: rx_fifo is cleared because when the send length is larger than the receive length, when UART_RXDONE_IRQ_STATUS is generated, there is data in rx_fifo, and UART_RXDONE_IRQ_STATUS interrupt will always be generated, affecting the function.
 * no_dma: for unified processing with DMA, because rx_fifo is cleared, the software pointer also needs to be cleared, otherwise an exception occurs.
 * the purpose of judging the status of the UART_RXBUF_IRQ_STATUS interrupt:
*  Because the state of the err needs to be cleared by the clearing rx_buff when an err interrupt is generated, the software pointer needs to be cleared.
 */
	if(status == UART_RXDONE_IRQ_STATUS){
		reg_uart_irq(uart_num) =UART_RXBUF_IRQ_STATUS;
		uart_clr_rx_index(uart_num); //clear software pointer
	}
	if(status == UART_RXBUF_IRQ_STATUS){
		uart_clr_rx_index(uart_num); //clear software pointer
	}
	reg_uart_irq(uart_num) =status;
}

/**
 * @brief     Enable uart rts.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_rts_en(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) |= FLD_UART_RTS_EN; //enable RTS function
}

/**
 * @brief     Disable uart rts.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_rts_dis(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_EN); //disable RTS function
}

/**
 * @brief     Enable uart cts.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_cts_en(uart_num_e uart_num)
{
	reg_uart_ctrl1(uart_num) |= FLD_UART_TX_CTS_ENABLE; //enable CTS function
}

/**
 * @brief     Disable uart cts.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_cts_dis(uart_num_e uart_num)
{
	reg_uart_ctrl1(uart_num) &= (~FLD_UART_TX_CTS_ENABLE); //disable CTS function
}

/**
 * @brief     Set uart rts trig level,when the number of rx_fifo reaches the rts trig level, rts is raised.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] level    - the rts trigger level,the range is less than 8.
 * @return    none
 */
static inline void uart_rts_trig_level_auto_mode(uart_num_e uart_num,unsigned char level)
{
    reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_TRIQ_LEV);
    reg_uart_ctrl2(uart_num) |= (level & FLD_UART_RTS_TRIQ_LEV);
}

/**
 * @brief      Set uart rts auto mode,flow control generation condition:
 *            -# rx_fifo cnt is greater than or equal to the set threshold(uart_rts_trig_level_auto_mode), rts is valid,
 *               when the number of rx_fifo is less than the set threshold, the level automatically becomes invalid;
 *            -# rx_done signal generation (if uart_rxdone_rts_en enable)),rts is valid,
 *               when the UART_RXDONE_IRQ_STATUS signal is cleared,the level automatically becomes invalid;
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_rts_auto_mode(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_MANUAL_M);
}

/**
 * @brief     Set uart rts manual mode,in this mode, the rts level is controlled by uart_set_rts_level API.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_rts_manual_mode(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) |= (FLD_UART_RTS_MANUAL_M);
}

/**
 * @brief      	Get the remain count of rxfifo,the range is 0~3,in order to get the rx_fifo current pointer position when rxfifo received data less than 1 word.
 * @param[in]  	uart_num - UART0 or UART1.
 * @return     	the remain count of rxfifo.
 */
static inline unsigned char uart_get_rxfifo_rem_cnt(uart_num_e uart_num)
{
	unsigned char rx_cnt = 0;
	rx_cnt = (reg_uart_irq(uart_num)&FLD_UART_RX_MEM_CNT);
	return rx_cnt;
}

/**
 * @brief      	Enable the rtx function.
 * @param[in]  	chn - UART0 or UART1.
 * @return     	none.
 */
static inline void uart_rtx_en(uart_num_e chn)
{
	reg_uart_ctrl0(chn)|=FLD_UART_S7816_EN;
}

/**
 * @brief     Enable rxtimeout rts.
 *            the conditions for rts generation are determined by uart_rts_auto_mode. there are two workflows for rts generation:
 *            -# the threshold -> rts->timeout->UART_RXDONE_IRQ_STATUS interrupt
 *            -# rx_done->rts
 *            uart_rts_stop_rxtimeout_en is called, the timeout will be stopped when the rts signal is generated,when the timeout is stopped, the UART_RXDONE_IRQ_STATUS signal will not be generated.
 * @param[in] uart_num
 * @return    none.
 * @note      this function is turned off by uart_init, this function is used in combination with rts function enable, the application determines whether to configure this function.
 */
static inline void uart_rts_stop_rxtimeout_en(uart_num_e uart_num){
	reg_uart_ctrl0(uart_num) |= FLD_UART_RXTIMEOUT_RTS_EN;
}

/**
 * @brief     Disable rxtimeout rts.
 *            the conditions for rts generation are determined by uart_rts_auto_mode. there are two workflows for rts generation:
 *            -# the threshold -> rts->timeout->UART_RXDONE_IRQ_STATUS
 *            -# rx_done->rts
 *            uart_rts_stop_rxtimeout_dis is called, the timeout will not be stop when the rts signal is generated,the UART_RXDONE_IRQ_STATUS signal will not be affected.
 * @param[in] uart_num
 * @return    none.
 * @note      this function is turned off by uart_init, this function is used in combination with rts function enable, the application determines whether to configure this function.
 */
static inline void uart_rts_stop_rxtimeout_dis(uart_num_e uart_num){
	reg_uart_ctrl0(uart_num) &= ~FLD_UART_RXTIMEOUT_RTS_EN;
}

/**
 * @brief     Enable rxdone(UART_RXDONE_IRQ_STATUS) rts,the function is turned off by default.
 *            If the rts function is used and this interface is called, generating the UART_RXDONE_IRQ_STATUS signal will trigger the rts pull up,
 *            and if the interface uart_clr_irq_status is called to clear the UART_RXDONE_IRQ_STATUS signal, the rts pull down.
 * @param[in] uart_num
 * @return    none.
 * @note      in DMA, when the length is set to 0xfffffc, uart_auto_clr_rx_fifo_ptr is enabled by default, and it is up to the hardware to clear UART_RXDONE_IRQ_STATUS. the software is out of control,
 *            so if want to use rx_done_rts when using dma mode and the length is set to 0xfffffc, must call uart_receive_dma,immediately call uart_auto_clr_rx_fifo_ptr to set manual UART_RXDONE_IRQ_STATUS.
 */
static inline void uart_rxdone_rts_en(uart_num_e uart_num){
	 reg_uart_ctrl4(uart_num) |= FLD_UART_RXDONE_RTS_EN;
}

/**
 * @brief     Disable rxdone(UART_RXDONE_IRQ_STATUS) rts.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_rxdone_rts_dis(uart_num_e uart_num){
	reg_uart_ctrl4(uart_num) &= ~FLD_UART_RXDONE_RTS_EN;
}

/**
  * @brief      Initializes the UART module.
  * @param[in]  uart_num    - UART0 or UART1.
  * @param[in]  div         - uart clock divider.
  * @param[in]  bwpc        - bitwidth, should be set to larger than 2.
  * @param[in]  parity      - selected parity type for UART interface.
  * @param[in]  stop_bit    - selected length of stop bit for UART interface.
  * @return     none
  * @note
  * -# A few simple examples of sys_clk/baud rate/div/bwpc correspondence:
   @verbatim
               sys_clk      baudrate   g_uart_div         g_bwpc

               16Mhz        9600          118                13
                            19200         118                 6
                            115200          9                13

               24Mhz        9600          249                 9
                            19200         124                 9
                            115200         12                15

               32Mhz        9600          235                13
                            19200         235                 6
                            115200         17                13

               48Mhz        9600          499                 9
                            19200         249                 9
                            115200         25                15
   @endverbatim
    -# uart_init() set the baud rate by the div and bwpc of the uart_cal_div_and_bwpc, some applications have higher timing requirements,
       can first calculate the div and bwpc, and then just call uart_init.
 */
extern void uart_init(uart_num_e uart_num,unsigned short div, unsigned char bwpc, uart_parity_e parity, uart_stop_bit_e stop_bit);

/**
 * @brief  		Calculate the best bwpc(bit width).
 * @param[in]	baudrate - baud rate of UART(pclk is the main factor affecting the upper baud rate of uart,cclk and pclk affect interrupt processing times).
 *                         -# CCLK_16M_HCLK_16M_PCLK_16M:in nodma,the maximum speed is 750k; in dma,the maximum speed is 2M;
 *                         -# CCLK_24M_HCLK_24M_PCLK_24M:in nodma,the maximum speed is 2m(this is not a garbled code, interrupt processing can not come over);
 *                                                       in dma,3m can be met, the maximum limit of non-garbled codes has not been confirmed;
 *                         -# Higher rates can be achieved by increasing the cclk/hclk so that the interrupt response time is accelerated;
 * @param[in]	pclk   -   pclk.
 * @param[out]	div      - uart clock divider.
 * @param[out]	bwpc     - bitwidth, should be set to larger than 2,range[3-15].
 * @return 		none
 * @note        BaudRate*(div+1)*(bwpc+1) = pclk.
 * <p>
 *  		    simplify the expression: div*bwpc =  constant(z).
 * <p>
 * 		        bwpc range from 3 to 15.so loop and get the minimum one decimal point.
 */
void uart_cal_div_and_bwpc(unsigned int baudrate, unsigned int sysclk, unsigned short* div, unsigned char *bwpc);

/**
 * @brief  	  Set rx_timeout:
 *            -# it is used to generate UART_RXDONE_IRQ_STATUS signals
 *            -# the UART_RXDONE_IRQ_STATUS interrupt is required to process the remaining data below the threshold(the DMA Operation threshold is fixed at 4,
 *               the NDMA threshold can be configured through uart_rx_irq_trig_level)
 * <p>
 *            the rx_timeout interpretation:
 * <p>
 *            rx_timeout = reg_uart_rx_timeout0*reg_uart_rx_timeout1[0:1],when no data is received within the rx_timeout period, that is rx timeout, the UART_RXDONE_IRQ_STATUS interrupt is generated:
 *           -# reg_uart_rx_timeout0: the maximum value is 0xff,this setting is transfer one bytes need cycles base on uart_clk,for example, if transfer one bytes (1start bit+8bits data+1 priority bit+2stop bits) total 12 bits,this register setting should be ((bpwc+1)*12).
 *           -# reg_uart_rx_timeout1[0:1]: multiple of the reg_uart_rx_timeout0.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] bwpc     - bitwidth, should be set to larger than 2.
 * @param[in] bit_cnt  - bit number.
 * @param[in] mul	   - mul.
 * @return 	  none
 */
void uart_set_rx_timeout(uart_num_e uart_num,unsigned char bwpc, unsigned char bit_cnt, uart_timeout_mul_e mul);


/**
  * @brief     Send uart data by byte in no_dma mode.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] tx_data  - the data to be send.
  * @return    none
  */
void uart_send_byte(uart_num_e uart_num,unsigned char tx_data);

/**
 * @brief     Receive uart data by byte in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
unsigned char uart_read_byte(uart_num_e uart_num);
/**
 * @brief     Judge if the transmission of uart is done.
 * @param[in] uart_num - UART0 or UART1.
 * @return    0:tx is done     1:tx isn't done
 */
unsigned char uart_tx_is_busy(uart_num_e uart_num);
/**
 * @brief     Send uart data by halfword in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] data  - the data to be send.
 * @return    none
 */
void uart_send_hword(uart_num_e uart_num, unsigned short data);

/**
 * @brief     Send uart data by word in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] data - the data to be send.
 * @return    none
 */
void uart_send_word(uart_num_e uart_num, unsigned int data);


/**
 * @brief     Sets the RTS pin's level manually,this function is used in combination with uart_rts_manual_mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] polarity - set the output of RTS pin.
 * @return    none
 */
void uart_set_rts_level(uart_num_e uart_num, unsigned char polarity);

/**
 *	@brief		Set pin for UART cts function,the pin connection mode:CTS<->RTS.
 *	@param[in]  uart_num - UART0 or UART1.
 *	@param[in]  cts_pin -To set cts pin.
 *	@return		none
 */
void uart_set_cts_pin(uart_num_e uart_num,gpio_func_pin_e cts_pin);

/**
 *	@brief		Set pin for UART rts function,the pin connection mode:RTS<->CTS.
 *	@param[in]  uart_num - UART0 or UART1.
 *	@param[in]  rts_pin - To set rts pin.
 *	@return		none
 */
void uart_set_rts_pin(uart_num_e uart_num,gpio_func_pin_e rts_pin);

/**
* @brief      Select pin for UART module,the pin connection mode:TX<->RX RX<->TX.
* @param[in]  uart_num - UART0 or UART1.
* @param[in]  tx_pin   - the pin to send data.
* @param[in]  rx_pin   - the pin to receive data.
* @return     none
*/
void uart_set_pin(uart_num_e uart_num,gpio_func_pin_e tx_pin,gpio_func_pin_e rx_pin);



/**
* @brief      Set rtx pin for UART module,this pin can be used as either tx or rx,it is the rx function by default,
*             When there is data in tx_fifo and the interface uart_rtx_pin_tx_trig is called, it is converted to tx function until tx_fifo is empty and converted to rx.
* @param[in]  uart_num - UART0 or UART1.
* @param[in]  rtx_pin  - the rtx pin need to set.
* @return     none
*/
void uart_set_rtx_pin(uart_num_e uart_num,gpio_func_pin_e rtx_pin);


/**
 * @brief     	Send an amount of data in DMA mode
 * @param[in]  	uart_num - uart channel
 * @param[in] 	addr     - Pointer to data buffer. It must be 4-bytes aligned address
 * @param[in] 	len      - Amount of data to be sent in bytes, range from 1 to 0xFFFFFC
 * @return      1  DMA start send.
 *              0  the length is error.
 */
unsigned char uart_send_dma(uart_num_e uart_num, unsigned char * addr, unsigned int len );

/**
* @brief     Send an amount of data in NODMA mode
* @param[in] uart_num - UART0 or UART1.
* @param[in] addr     - pointer to the buffer.
* @param[in] len      - NDMA transmission length.
* @return    1
*/
unsigned char uart_send(uart_num_e uart_num, unsigned char * addr, unsigned char len );


/**
 * @brief     	Receive an amount of data in DMA mode
 * @param[in]  	uart_num - UART0 or UART1
 * @param[in] 	addr     - Pointer to data buffer, it must be 4-bytes aligned
 * @param[in]   rev_size - Length of DMA in bytes, it must be multiple of 4. The maximum value can be up to 0xFFFFFC.
 * @return    	none
 * @attention 	In the case of rev_size < 0xFFFFFC:
				-# there will be only data in the buffer of received data, not length information.
				-# the flag to judge the data reception completion is UART_RXDONE_IRQ_STATUS, that is, call this function uart_get_irq_status(UARTx,UART_RXDONE_IRQ_STATUS).
				-# If you want to calculate the length, you can call the function uart_get_dma_rev_data_len(x,x) to calculate the length of received data after detecting the status of UART_RXDONE_IRQ_STATUS.
				-# The actual buffer size defined by the user needs to be not smaller than the rev_size, otherwise there may be an out-of-bounds problem.

				In the case of rev_size=0xFFFFFC, then:
				-# The first four bytes in the buffer of the received data are the length of the received data.
				-# the flag to determine the completion of data reception is TC_MASK, that calls this function dma_get_tc_irq_status(UART_RX_DMA_STATUS).
				-# The actual buffer size to be defined by the user needs to be not less than (the length of the longest packet received + 4),otherwise there may be an out-of-bounds problem.
 */
void uart_receive_dma(uart_num_e uart_num, unsigned char * addr,unsigned int rev_size);

/**
 * @brief     Get data length that dma received In the case of rev_size < 0xFFFFFC,
 *            and when the receive length is larger than the set length,the length calculated by this function is the length set by DMA, and excess data is discarded.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] chn      - dma channel.
 * @return    data length.
 */
unsigned int uart_get_dma_rev_data_len(uart_num_e uart_num,dma_chn_e chn);
/**
  * @brief     Configures the uart tx_dma channel control register.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] chn      - dma channel.
  * @return    none
  */
void uart_set_tx_dma_config(uart_num_e uart_num, dma_chn_e chn);

/**
  * @brief     Configures uart rx_dma channel control register.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] chn      - dma channel.
  * @return    none
  */
void uart_set_rx_dma_config(uart_num_e uart_num, dma_chn_e chn);

/**
  * @brief     Configure UART hardware flow CTS.
  * @param[in] uart_num   - UART0 or UART1.
  * @param[in] cts_pin    - cts pin select.
  * @param[in] cts_parity -  1:Active high,when the cts receives an active level, it stops sending data.
  *                          0:Active low
  * @return    none
  */
void uart_cts_config(uart_num_e uart_num,gpio_func_pin_e cts_pin,unsigned char cts_parity);

/**
 * @brief     Configure UART hardware flow RTS.
 * @param[in] uart_num     - UART0 or UART1.
 * @param[in] rts_pin      - RTS pin select.
 * @param[in] rts_parity   - 0: Active high  1: Active low
 *                           in auto mode,when the condition is reached, the rts active level is activated, and in manual mode, the rts level can be manually controlled.
 * @param[in] auto_mode_en - set the mode of RTS(auto or manual).
 * @return    none
 */
void uart_rts_config(uart_num_e uart_num,gpio_func_pin_e rts_pin,unsigned char rts_parity,unsigned char auto_mode_en);

/**
 * @brief      Configure DMA head node.
 * @param[in]  uart_num    - UART0/UART1.
 * @param[in]  chn         - dma channel.
 * @param[in]  dst_addr    - Pointer to data buffer, which must be 4 bytes aligned.
 * @param[in]  data_len    - It must be set to 0xFFFFFC.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 * <p>
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *             -# you need to leave 4bytes of space for the length information.
 *             -# The actual buffer size to be defined by the user needs to be not less than (the length of the longest packet received + 4),otherwise there may be an out-of-bounds problem.
 * @param[in]  head_of_list - the head address of dma llp.
 * @return     none.
 */
 void uart_set_dma_chain_llp(uart_num_e uart_num, dma_chn_e chn,unsigned char * dst_addr,unsigned int data_len,dma_chain_config_t *head_of_list);

/**
  * @brief      Configure DMA cycle chain node.
  * @param[in]  uart_num    - UART0/UART1.
  * @param[in]  chn         -  dma channel.
  * @param[in]  config_addr - to servers to configure the address of the current node.
  * @param[in]  llpointer   - to configure the address of the next node configure.
  * @param[in]  dst_addr    - Pointer to data buffer, which must be 4 bytes aligned.
  * @param[in]  data_len    - It must be set to 0xFFFFFC.
  * @attention  The first four bytes in the buffer of the received data are the length of the received data.
  * <p>
  *             The actual buffer size that the user needs to set needs to be noted on two points:
  *            -# you need to leave 4bytes of space for the length information.
  *            -# The actual buffer size to be defined by the user needs to be not less than (the length of the longest packet received + 4),otherwise there may be an out-of-bounds problem.
  * @return     none.
  */
 void uart_rx_dma_add_list_element(uart_num_e uart_num,dma_chn_e chn,dma_chain_config_t *config_addr,dma_chain_config_t *llpointer ,unsigned char * dst_addr,unsigned int data_len);

/**
  * @brief      Set dma single chain transfer.
  * @param[in]  uart_num  - UART0/UART1.
  * @param[in]  chn       -  dma channel
  * @param[in]  in_buff   - Pointer to data buffer, which must be 4 bytes aligned.
  * @param[in]  buff_size - It must be set to 0xFFFFFC.
  * @attention  The first four bytes in the buffer of the received data are the length of the received data.
  * <p>
  *             The actual buffer size that the user needs to set needs to be noted on two points:
  *            -# you need to leave 4bytes of space for the length information.
  *            -# The actual buffer size to be defined by the user needs to be not less than (the length of the longest packet received + 4),otherwise there may be an out-of-bounds problem.
  * @return     none.
  */
void uart_rx_dma_chain_init (uart_num_e uart_num, dma_chn_e chn,unsigned char * in_buff,unsigned int buff_size );

#endif	/* UART_H_ */
