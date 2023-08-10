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
 *
 *	API Reference
 *	===============
 *	Header File: uart.h
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
 * @brief     This function serves to get the rxfifo cnt.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline unsigned char uart_get_rxfifo_num(uart_num_e uart_num)
{
	return reg_uart_buf_cnt(uart_num)&FLD_UART_RX_BUF_CNT ;
}

/**
 * @brief     This function serves to get the txfifo cnt.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline unsigned char uart_get_txfifo_num(uart_num_e uart_num)
{
	return (reg_uart_buf_cnt(uart_num)&FLD_UART_TX_BUF_CNT )>>4;
}

/**
 * @brief     This function resets the UART module.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_reset(uart_num_e uart_num)
{

	reg_rst0 &= (~((uart_num)?FLD_RST0_UART1:FLD_RST0_UART0));
	reg_rst0 |= ((uart_num)?FLD_RST0_UART1:FLD_RST0_UART0);
}

/**
 * @brief     This function enable the clock of UART module.
 * @param[in] uart_num - UART0/UART1.
 * @return    none
 */
static inline void uart_clk_en(uart_num_e uart_num)
{
	reg_clk_en0 |= ((uart_num)?FLD_CLK0_UART1_EN:FLD_CLK0_UART0_EN);
}
/**
 * @brief     This function is used to choose rxdone function.
 * @param[in] uart_num.
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
 * @brief     This function is used to choose auto clr rx fifo pointer,if is enabled, when rx_done,the hardware will automatically clear the rx_fifo pointer,
 *            note that this feature cannot be turned on when no_dma and dma is configured to a certain length rather than a maximum length.
 * @param[in] uart_num.
 * @param[in] en - 0:disable 1:enable
 * @return    none.
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
 * @brief     This function serves to config the number level setting the irq bit of status register.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] rx_level - receive level value. ie 0x140089[0,3].
 * @return    none
 */
static inline void uart_rx_irq_trig_level(uart_num_e uart_num,unsigned char rx_level)
{
	uart_rxdone_sel(uart_num,UART_NO_DMA_MODE);
	uart_auto_clr_rx_fifo_ptr(uart_num,0);
	reg_uart_ctrl3(uart_num) = (reg_uart_ctrl3(uart_num) & (~FLD_UART_RX_IRQ_TRIQ_LEV)) | (rx_level & 0x0f);
}

/**
 * @brief     This function serves to config the number level setting the irq bit of status register.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] tx_level - transmit level value.ie 0x140089[4,7].
 * @return    none
 */
static inline void uart_tx_irq_trig_level(uart_num_e uart_num,unsigned char tx_level)
{
	reg_uart_ctrl3(uart_num) = (reg_uart_ctrl3(uart_num) & (~FLD_UART_TX_IRQ_TRIQ_LEV)) | (tx_level << 4);
}


/**
 * @brief     This function serves to config the irq of uart tx and rx.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] mask     - uart irq mask.
 * @return    none
 */
static inline void uart_set_irq_mask(uart_num_e uart_num,uart_irq_mask_e mask)
{
	reg_uart_mask(uart_num)|=mask;
}

/**
 * @brief     This function serves to clear the irq of uart tx and rx.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] mask     - uart irq mask.
 * @return    none
 */
static inline void uart_clr_irq_mask(uart_num_e uart_num,uart_irq_mask_e mask)
{
	reg_uart_mask(uart_num)&=~mask;
}

/**
 * @brief     This function is used to set the 'uart_rx_byte_index' to 0.
 *			  after wakeup from power-saving mode or reset uart, you must call this function before receiving the data.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_clr_rx_index(uart_num_e uart_num)
{
	uart_rx_byte_index[uart_num]=0;
}


/**
 * @brief     This function is used to set the 'uart_tx_byte_index' to 0.
 *			  after wakeup from power-saving mode or reset uart, you must call this function before sending the data.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_clr_tx_index(uart_num_e uart_num)
{
	uart_tx_byte_index[uart_num]=0;
}

/**
 * @brief     This function serves to get the irq status of uart tx and rx.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] status   - uart irq status.
 * @return    irq status
 */
static inline unsigned int  uart_get_irq_status(uart_num_e uart_num,uart_irq_status_e status)
{
	return reg_uart_irq(uart_num)&status;
}

/**
 * @brief     This function serves to clear the irq status of uart tx and rx.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] status - uart irq status.
 * @return    none
 */

static inline void uart_clr_irq_status(uart_num_e uart_num,uart_irq_status_e status)
{
/*
 * the purpose of judging the status of rx_done:
 * when rx_done is cleared, the rx_buff is also cleared, because rx_fifo is cleared, and the software pointer is also cleared:
 * dma: rx_fifo is cleared because when the send length is larger than the receive length, when rx_done is generated, there is data in rx_fifo, and rx_done interrupt will always be generated, affecting the function.
 * no_dma: for unified processing with DMA, because rx_fifo is cleared, the software pointer also needs to be cleared, otherwise an exception occurs.
 * the purpose of judging the status of the rx_buff interrupt:
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
 * @brief     This function serves to set uart rts enable.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_rts_en(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) |= FLD_UART_RTS_EN; //enable RTS function
}

/**
 * @brief     This function serves to set uart rts disable.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_rts_dis(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_EN); //disable RTS function
}

/**
 * @brief     This function serves to set uart cts enable.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_cts_en(uart_num_e uart_num)
{
	reg_uart_ctrl1(uart_num) |= FLD_UART_TX_CTS_ENABLE; //enable CTS function
}

/**
 * @brief     This function serves to set uart cts disable.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_set_cts_dis(uart_num_e uart_num)
{
	reg_uart_ctrl1(uart_num) &= (~FLD_UART_TX_CTS_ENABLE); //disable CTS function
}

/**
 * @brief     This function serves to set uart rts trig lexel in auto mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] level    - threshold of trig RTS pin's level toggle(only for auto mode).
 * @return    none
 */
static inline void uart_rts_trig_level_auto_mode(uart_num_e uart_num,unsigned char level)
{
    reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_TRIQ_LEV);
    reg_uart_ctrl2(uart_num) |= (level & FLD_UART_RTS_TRIQ_LEV);
}

/**
 * @brief     This function serves to set uart rts auto mode,flow control generation condition 1. rx_fifo number is greater than or equal to the set threshold 2. rx_done signal generation (if uart_rxdone_rts_en enable)).
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_rts_auto_mode(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_MANUAL_M);
}

/**
 * @brief     This function serves to set uart rts manual mode,in this mode, the rts level changes are controlled by calling the uart_set_rts_level interface.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
static inline void uart_rts_manual_mode(uart_num_e uart_num)
{
	reg_uart_ctrl2(uart_num) |= (FLD_UART_RTS_MANUAL_M);
}

/**
 * @brief      	This function is used to get the count of uart-rx-buff.
 * @param[in]  	chn - UART0 or UART1.
 * @return     	count.
 */
static inline unsigned char uart_get_rx_buff_cnt(uart_num_e uart_num)
{
	unsigned char rx_cnt = 0;
	rx_cnt = (reg_uart_irq(uart_num)&FLD_UART_RX_MEM_CNT);
	return rx_cnt;
}

/**
 * @brief      	This function is used to enable the rtx function of .
 * @param[in]  	chn - UART0 or UART1.
 * @return     	none.
 */
static inline void uart_rtx_en(uart_num_e chn)
{
	reg_uart_ctrl0(chn)|=FLD_UART_S7816_EN;
}

/**
 * @brief     This function is used to eable rxtimeout rts.
 *            the conditions for rts generation are determined by uart_rts_auto_mode. there are two workflows for rts generation:
 *            1. the threshold -> rts->timeout->rx_done 2.rx_done->rts
 *            uart_rts_stop_rxtimeout_en is called, the timeout will be stopped when the rts signal is generated,when the timeout is stopped, the rx_done signal will not be generated.
 *            Note: this function is turned off by uart_init, this function is used in combination with rts function enable, the application determines whether to configure this function.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_rts_stop_rxtimeout_en(uart_num_e uart_num){
	reg_uart_ctrl0(uart_num) |= FLD_UART_RXTIMEOUT_RTS_EN;
}

/**
 * @brief     This function is used to disable rxtimeout rts.
 *            the conditions for rts generation are determined by uart_rts_auto_mode. there are two workflows for rts generation:
 *            1. the threshold -> rts->timeout->rx_done 2.rx_done->rts
 *            uart_rts_stop_rxtimeout_dis is called, the timeout will not be stop when the rts signal is generated,the rx_done signal will not be affected.
 *            Note: this function is turned off by uart_init, this function is used in combination with rts function enable, the application determines whether to configure this function.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_rts_stop_rxtimeout_dis(uart_num_e uart_num){
	reg_uart_ctrl0(uart_num) &= ~FLD_UART_RXTIMEOUT_RTS_EN;
}

/**
 * @brief     This function is used to enable rxdone rts.
 *            the function is turned off by default,If the rts function is used and this interface is called, generating the rx_done signal will trigger the rts pull up,
 *            and if the interface uart_clr_irq_status is called to clear the rx_done signal, the rts pull down.
 *            Note: in DMA, when the length is set to 0xfffffc, uart_auto_clr_rx_fifo_ptr is enabled by default, and it is up to the hardware to clear rx_done. the software is out of control,
 *            so if want to use rx_done_rts when using dma mode and the length is set to 0xfffffc, must call uart_receive_dma,immediately call uart_auto_clr_rx_fifo_ptr to set manual rx_done.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_rxdone_rts_en(uart_num_e uart_num){
	 reg_uart_ctrl4(uart_num) |= FLD_UART_RXDONE_RTS_EN;
}

/**
 * @brief     This function is used to disable rxdone rts.
 * @param[in] uart_num
 * @return    none.
 */
static inline void uart_rxdone_rts_dis(uart_num_e uart_num){
	reg_uart_ctrl4(uart_num) &= ~FLD_UART_RXDONE_RTS_EN;
}

/**
 * @brief      This function initializes the UART module.
 * @param[in]  uart_num    - UART0 or UART1.
 * @param[in]  div         - uart clock divider.
 * @param[in]  bwpc        - bitwidth, should be set to larger than 2.
 * @param[in]  parity      - selected parity type for UART interface.
 * @param[in]  stop_bit    - selected length of stop bit for UART interface.
 * @return     none
 * @note 	   sys_clk      baudrate   g_uart_div         g_bwpc
 *
 *  	       16Mhz        9600          118   			 13
 *                          19200         118     			  6
 *          	            115200          9       		 13
 *
 * 	           24Mhz        9600          249       		  9
 *           	 	    	19200		  124                 9
 *          	 	    	115200         12    			 15
 *
 *   	       32Mhz        9600          235       		 13
 *          	 	        19200		  235                 6
 *           	 	 	    115200         17    			 13
 *
 *   	       48Mhz        9600          499       		  9
 *          	 	 	    19200		  249                 9
 *           	 	 	    115200         25    			 15
*/
extern void uart_init(uart_num_e uart_num,unsigned short div, unsigned char bwpc, uart_parity_e parity, uart_stop_bit_e stop_bit);

/***********************************************************
 * @brief  		This function serves to calculate the best bwpc(bit width) .i.e reg0x96.
 * @param[in]	baudrate - baut rate of UART.
 * @param[in]	sysclk   - system clock.
 * @param[out]	div      - uart clock divider.
 * @param[out]	bwpc     - bitwidth, should be set to larger than 2.
 * @return 		none
 * @note        BaudRate*(div+1)*(bwpc+1) = system clock.
 *  		    simplify the expression: div*bwpc =  constant(z).
 * 		        bwpc range from 3 to 15.so loop and get the minimum one decimal point.
 */
void uart_cal_div_and_bwpc(unsigned int baudrate, unsigned int sysclk, unsigned short* div, unsigned char *bwpc);

/**
 * @brief  		This function serves to set r_rxtimeout. this setting is transfer one bytes need cycles base on uart_clk.
 * 				For example, if transfer one bytes (1start bit+8bits data+1 priority bit+2stop bits) total 12 bits,
 * 				this register setting should be (bpwc+1)*12.
 * @param[in]	uart_num - UART0 or UART1.
 * @param[in]	bwpc     - bitwidth, should be set to larger than 2.
 * @param[in]	bit_cnt  - bit number.
 * @param[in]	mul	     - mul.
 * @return 		none
 */
void uart_set_rx_timeout(uart_num_e uart_num,unsigned char bwpc, unsigned char bit_cnt, uart_timeout_mul_e mul);


/**
 * @brief     This function serves to send data by byte with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] tx_data  - the data to be send.
 * @return    none
 */
void uart_send_byte(uart_num_e uart_num,unsigned char tx_data);

/**
 * @brief     This function serves to receive uart data by byte with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
unsigned char uart_read_byte(uart_num_e uart_num);
/**
 * @brief     This function serves to judge if the transmission of uart is done.
 * @param[in] uart_num - UART0 or UART1.
 * @return    return the tx status
 * -          0:tx is done     1:tx isn't done
 */
unsigned char uart_tx_is_busy(uart_num_e uart_num);
/**
 * @brief     This function serves to send uart0 data by halfword with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] data  - the data to be send.
 * @return    none
 */
void uart_send_hword(uart_num_e uart_num, unsigned short data);

/**
 * @brief     This function serves to send uart0 data by word with not DMA method.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] data - the data to be send.
 * @return    none
 */
void uart_send_word(uart_num_e uart_num, unsigned int data);


/**
 * @brief     This function sets the RTS pin's level manually.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] polarity - set the output of RTS pin(only for manual mode).
 * @return    none
 */
void uart_set_rts_level(uart_num_e uart_num, unsigned char polarity);

/**
 *	@brief		This function serves to set pin for UART cts function.
 *	@param[in]  uart_num - UART0 or UART1.
 *	@param[in]  cts_pin -To set cts pin.
 *	@return		none
 */
void uart_set_cts_pin(uart_num_e uart_num,gpio_func_pin_e cts_pin);

/**
 *	@brief		This function serves to set pin for UART rts function.
 *	@param[in]  uart_num - UART0 or UART1.
 *	@param[in]  rts_pin - To set rts pin.
 *	@return		none
 */
void uart_set_rts_pin(uart_num_e uart_num,gpio_func_pin_e rts_pin);

/**
* @brief      This function serves to select pin for UART module.
* @param[in]  tx_pin   - the pin to send data.
* @param[in]  rx_pin   - the pin to receive data.
* @return     none
*/
void uart_set_pin(uart_num_e uart_num,gpio_func_pin_e tx_pin,gpio_func_pin_e rx_pin);



/**
* @brief      This function serves to set rtx pin for UART module.
*             this pin can be used as either tx or rx. this pin is only used as tx when there is a sending action, but it is used as an rx at all times.
* @param[in]  rtx_pin  - the rtx pin need to set.
* @return     none
*/
void uart_set_rtx_pin(uart_num_e uart_num,gpio_func_pin_e rtx_pin);

/**
 * @brief     	This function serves to send data by DMA, this function tell the DMA to get data from the RAM and start.
 * @param[in]  	uart_num - UART0 or UART1.
 * @param[in] 	addr     - pointer to the buffer containing data need to send.
 * @param[in] 	len      - DMA transmission length.The maximum transmission length of DMA is 0xFFFFFC bytes, so dont'n over this length.
 * @return      1  dma start send.
 *              0  the length is error.
 * @note        addr: must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
unsigned char uart_send_dma(uart_num_e uart_num, unsigned char * addr, unsigned int len );

/**
* @brief     This function serves to send data with not DMA method.
* @param[in] uart_num - UART0 or UART1.
* @param[in] addr     - pointer to the buffer containing data need to send.
* @param[in] len      - NDMA transmission length.
* @return    1
*/
unsigned char uart_send(uart_num_e uart_num, unsigned char * addr, unsigned char len );


/**
 * @brief     	This function serves to receive data function by DMA, this function tell the DMA to get data from the uart data fifo.
 *              1. if the receiving length information of DMA is set to 0xFFFFFC byte(max_value), and write_num is turned on,
 *                 then The length of the data received by dma will be written back to the first four bytes of addr,so need addr = the receive buff addr +4.
 *              2. if the receiving length information of DMA is set to less than 0xFFFFFC byte, and write_num is turned on,
 *                 then the length of data received by DMA will not be written to the first four bytes of addr,  requires the uart_get_dma_rev_data_len calculation.
 * @param[in]  	uart_num - UART0 or UART1.
 * @param[in] 	addr     - pointer to the buffer receive data.
 * @param[in]   rev_size - the receive length of DMA,The maximum transmission length of DMA is 0xFFFFFC bytes, so dont'n over this length.
 * @return    	none
 * @note        addr: must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void uart_receive_dma(uart_num_e uart_num, unsigned char * addr,unsigned int rev_size);

/**
 * @brief     This function serves to get the length of the data that dma received.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] chn      - dma channel.
 * @return    data length.
 */
unsigned int uart_get_dma_rev_data_len(uart_num_e uart_num,dma_chn_e chn);
/**
  * @brief     This function serves to set uart tx_dam channel and config dma tx default.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] chn      - dma channel.
  * @return    none
  */
extern void uart_set_tx_dma_config(uart_num_e uart_num, dma_chn_e chn);

/**
  * @brief     This function serves to set uart rx_dam channel and config dma rx default.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] chn      - dma channel.
  * @return    none
  */
extern void uart_set_rx_dma_config(uart_num_e uart_num, dma_chn_e chn);

/**
  * @brief     This function serves to configure UART hardware flow. Configure CTS.
  * @param[in] uart_num   - UART0 or UART1.
  * @param[in] cts_pin    - RTS pin select.
  * @param[in] cts_parity - when CTS's input equals to select, tx will be stopped.
  * @return    none
  */
extern void uart_cts_config(uart_num_e uart_num,gpio_func_pin_e cts_pin,unsigned char cts_parity);

/**
 * @brief     This function serves to configure UART hardware flow. Configure RTS.
 * @param[in] uart_num     - UART0 or UART1.
 * @param[in] rts_pin      - RTS pin select.
 * @param[in] rts_parity   - whether invert the output of RTS pin(only for auto mode).
 * @param[in] auto_mode_en - set the mode of RTS(auto or manual).
 * @return    none
 */
extern void uart_rts_config(uart_num_e uart_num,gpio_func_pin_e rts_pin,unsigned char rts_parity,unsigned char auto_mode_en);

/**
  * @brief     This function servers to configure DMA head node,the chain function only applies to data_len = 0xFFFFFC.
  * @param[in] uart_num - UART0/UART1.
  * @param[in] chn          - to select the DMA channel.
  * @param[in] dst_addr     - the dma address of destination.
  * @param[in] data_len     - to configure DMA length.
  * @param[in] head_of_list - the head address of dma llp.
  * @return    none
  * @note      dst_addr: must be aligned by word (4 bytes), otherwise the program will enter an exception.
  */
 void uart_set_dma_chain_llp(uart_num_e uart_num, dma_chn_e chn,unsigned char * dst_addr,unsigned int data_len,dma_chain_config_t *head_of_list);
 /**
  * @brief     This function servers to configure DMA cycle chain node,the chain function only applies to data_len = 0xFFFFFC.
  * @param[in] uart_num    - UART0/UART1.
  * @param[in] chn         - to select the DMA channel.
  * @param[in] config_addr - to servers to configure the address of the current node.
  * @param[in] llpointer   - to configure the address of the next node configure.
  * @param[in] dst_addr    - the dma address of destination.
  * @param[in] data_len    - to configure DMA length.
  * @return    none
  * @note      dst_addr: must be aligned by word (4 bytes), otherwise the program will enter an exception.
  */
 void uart_rx_dma_add_list_element(uart_num_e uart_num,dma_chn_e chn,dma_chain_config_t *config_addr,dma_chain_config_t *llpointer ,unsigned char * dst_addr,unsigned int data_len);
 /**
  * @brief  This function serves to set dma single chain transfer,the chain function only applies to data_len = 0xFFFFFC.
  * @param[in] uart_num  - UART0/UART1.
  * @param[in] chn       -  dma channel
  * @param[in] in_buff   - the pointer of rx_buff.
  * @param[in] buff_size - the size of rx_buff.
  * @return    none
  * @note      in_buff: must be aligned by word (4 bytes), otherwise the program will enter an exception.
  */
  void uart_rx_dma_chain_init (uart_num_e uart_num, dma_chn_e chn,unsigned char * in_buff,unsigned int buff_size );

#endif	/* UART_H_ */
