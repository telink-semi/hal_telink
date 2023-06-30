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
 * @file	uart.c
 *
 * @brief	This is the source file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "uart.h"

/**********************************************************************************************************************
 *                                			  local constants                                                       *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                           	local macro                                                        *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                             local data type                                                     *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              global variable                                                       *
 *********************************************************************************************************************/
dma_config_t uart_tx_dma_config[2]={
	{	.dst_req_sel 		= DMA_REQ_UART0_TX,//tx req
		.src_req_sel 		= 0,
		.dst_addr_ctrl		= DMA_ADDR_FIX,
		.src_addr_ctrl	 	= DMA_ADDR_INCREMENT,//increment
		.dstmode		 	= DMA_HANDSHAKE_MODE,//handshake
		.srcmode			= DMA_NORMAL_MODE,
		.dstwidth 			= DMA_CTR_WORD_WIDTH,//must be word
		.srcwidth 			= DMA_CTR_WORD_WIDTH,//must be word
		.src_burst_size 	= 0,//must be 0
		.read_num_en		= 0,
		.priority 			= 0,
		.write_num_en		= 0,
		.auto_en 			= 0,//must be 0
	},
	{	.dst_req_sel 		= DMA_REQ_UART1_TX,//tx req
		.src_req_sel 		= 0,
		.dst_addr_ctrl		= DMA_ADDR_FIX,
		.src_addr_ctrl	 	= DMA_ADDR_INCREMENT,//increment
		.dstmode		 	= DMA_HANDSHAKE_MODE,//handshake
		.srcmode			= DMA_NORMAL_MODE,
		.dstwidth 			= DMA_CTR_WORD_WIDTH,//must be word
		.srcwidth 			= DMA_CTR_WORD_WIDTH,//must be word
		.src_burst_size 	= 0,//must be 0
		.read_num_en		= 0,
		.priority 			= 0,
		.write_num_en		= 0,
		.auto_en 			= 0,//must be 0
	}
};
dma_config_t uart_rx_dma_config[2]={
	{ 	.dst_req_sel 		= 0,//tx req
		.src_req_sel 		= DMA_REQ_UART0_RX,
		.dst_addr_ctrl 		= DMA_ADDR_INCREMENT,
		.src_addr_ctrl 		= DMA_ADDR_FIX,
		.dstmode 			= DMA_NORMAL_MODE,
		.srcmode 			= DMA_HANDSHAKE_MODE,
		.dstwidth 			= DMA_CTR_WORD_WIDTH,//must be word
		.srcwidth 			= DMA_CTR_WORD_WIDTH,////must be word
		.src_burst_size 	= 0,
		.read_num_en 		= 0,
		.priority 			= 0,
		.write_num_en 		= 1,
		.auto_en 			= 0,//must be 0
	},
	{ 	.dst_req_sel 		= 0,//tx req
		.src_req_sel 		= DMA_REQ_UART1_RX,
		.dst_addr_ctrl 		= DMA_ADDR_INCREMENT,
		.src_addr_ctrl 		= DMA_ADDR_FIX,
		.dstmode 			= DMA_NORMAL_MODE,
		.srcmode 			= DMA_HANDSHAKE_MODE,
		.dstwidth 			= DMA_CTR_WORD_WIDTH,//must be word
		.srcwidth 			= DMA_CTR_WORD_WIDTH,////must be word
		.src_burst_size 	= 0,
		.read_num_en 		= 0,
		.priority 			= 0,
		.write_num_en 		= 1,
		.auto_en 			= 0,//must be 0
	}
};
/**********************************************************************************************************************
 *                                              local variable                                                     *
 *********************************************************************************************************************/
 static unsigned char uart_dma_tx_chn[2];
 static unsigned char uart_dma_rx_chn[2];
 static unsigned int uart_dma_rev_size[2];
 dma_chain_config_t g_uart_rx_dma_list_cfg;
/**********************************************************************************************************************
 *                                          local function prototype                                               *
 *********************************************************************************************************************/
 /**
  * @brief     This function is used to look for the prime.if the prime is found,it will return 1, or return 0.
  * @param[in] n - the value to judge.
  * @return    0 or 1
  */
 static unsigned char uart_is_prime(unsigned int n);

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/

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
void uart_init(uart_num_e uart_num,unsigned short div, unsigned char bwpc, uart_parity_e parity, uart_stop_bit_e stop_bit)
{
	reg_uart_ctrl0(uart_num) = ((reg_uart_ctrl0(uart_num) & (~FLD_UART_BPWC_O))| bwpc);//set bwpc
    reg_uart_clk_div(uart_num) = (div | FLD_UART_CLK_DIV_EN); //set div_clock

    //parity config
    if (parity) {
    	reg_uart_ctrl1(uart_num)  |= FLD_UART_PARITY_ENABLE; //enable parity function
        if (UART_PARITY_EVEN == parity) {
        	reg_uart_ctrl1(uart_num)  &= (~FLD_UART_PARITY_POLARITY); //enable even parity
        }
        else if (UART_PARITY_ODD == parity) {
        	reg_uart_ctrl1(uart_num)  |= FLD_UART_PARITY_POLARITY; //enable odd parity
        }
    }
    else {
    	reg_uart_ctrl1(uart_num)  &= (~FLD_UART_PARITY_ENABLE); //disable parity function
    }

    //stop bit config
    reg_uart_ctrl1(uart_num) = ((reg_uart_ctrl1(uart_num) & (~FLD_UART_STOP_SEL)) | stop_bit);
    uart_rts_stop_rxtimeout_dis(uart_num);
    uart_rxdone_rts_dis(uart_num);
}

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
void uart_cal_div_and_bwpc(unsigned int baudrate, unsigned int pclk, unsigned short* div, unsigned char *bwpc)
{
	unsigned char i = 0, j= 0;
	unsigned int primeInt = 0;
	unsigned char primeDec = 0;
	unsigned int D_intdec[13],D_int[13];
	unsigned char D_dec[13];

	primeInt = pclk/baudrate;
	primeDec = 10*pclk/baudrate - 10*primeInt;

	if(uart_is_prime(primeInt)){ // primeInt is prime
		primeInt += 1;  //+1 must be not prime. and primeInt must be larger than 2.
	}
	else{
		if(primeDec > 5){ // >5
			primeInt += 1;
			if(uart_is_prime(primeInt)){
				primeInt -= 1;
			}
		}
	}

	for(i=3;i<=15;i++){
		D_intdec[i-3] = (10*primeInt)/(i+1);//get the LSB
		D_dec[i-3] = D_intdec[i-3] - 10*(D_intdec[i-3]/10);//get the decimal section
		D_int[i-3] = D_intdec[i-3]/10;//get the integer section
	}

	//find the max and min one decimation point
	unsigned char position_min = 0,position_max = 0;
	unsigned int min = 0xffffffff,max = 0x00;
	for(j=0;j<13;j++){
		if((D_dec[j] <= min)&&(D_int[j] != 0x01)){
			min = D_dec[j];
			position_min = j;
		}
		if(D_dec[j]>=max){
			max = D_dec[j];
			position_max = j;
		}
	}

	if((D_dec[position_min]<5) && (D_dec[position_max]>=5)){
		if(D_dec[position_min]<(10-D_dec[position_max])){
			*bwpc = position_min + 3;
			*div = D_int[position_min]-1;
		}
		else{
			*bwpc = position_max + 3;
			*div = D_int[position_max];
		}
	}
	else if((D_dec[position_min]<5) && (D_dec[position_max]<5)){
		*bwpc = position_min + 3;
		*div = D_int[position_min] - 1;
	}
	else{
		*bwpc = position_max + 3;
		*div = D_int[position_max];
	}
}

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
void uart_set_rx_timeout(uart_num_e uart_num,unsigned char bwpc, unsigned char bit_cnt, uart_timeout_mul_e mul)
{
    reg_uart_rx_timeout0(uart_num) = (bwpc+1) * bit_cnt; //one byte includes 12 bits at most
    reg_uart_rx_timeout1(uart_num) = (((reg_uart_rx_timeout1(uart_num))&(~FLD_UART_TIMEOUT_MUL))|mul); //if over 2*(tmp_bwpc+1),one transaction end.
}

 unsigned char uart_tx_byte_index[2] = {0};
 /**
   * @brief     Send uart data by byte in no_dma mode.
   * @param[in] uart_num - UART0 or UART1.
   * @param[in] tx_data  - the data to be send.
   * @return    none
   */
void uart_send_byte(uart_num_e uart_num, unsigned char tx_data)
{
	while(uart_get_txfifo_num(uart_num)>7);

	reg_uart_data_buf(uart_num, uart_tx_byte_index[uart_num]) = tx_data;
	uart_tx_byte_index[uart_num] ++;
	(uart_tx_byte_index[uart_num]) &= 0x03;
}

unsigned char uart_rx_byte_index[2]={0};
/**
 * @brief     Receive uart data by byte in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @return    none
 */
unsigned char uart_read_byte(uart_num_e uart_num)
{
	unsigned char rx_data = reg_uart_data_buf(uart_num, uart_rx_byte_index[uart_num]) ;
	uart_rx_byte_index[uart_num]++;
	uart_rx_byte_index[uart_num] &= 0x03 ;
	return rx_data;
}

/**
 * @brief     Judge if the transmission of uart is done.
 * @param[in] uart_num - UART0 or UART1.
 * @return    0:tx is done     1:tx isn't done
 */
unsigned char uart_tx_is_busy(uart_num_e uart_num)
{
     return  ((reg_uart_irq(uart_num) & FLD_UART_TXDONE_IRQ) ? 0:1 ) ;
}

/**
 * @brief     Send uart data by halfword in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] data  - the data to be send.
 * @return    none
 */
void uart_send_hword(uart_num_e uart_num, unsigned short data)
{
	static unsigned char uart_tx_hword_index[2]={0};

	while(uart_get_txfifo_num(uart_num)>6);

	reg_uart_data_hword_buf(uart_num, uart_tx_hword_index[uart_num]) = data;
	uart_tx_hword_index[uart_num]++ ;
	uart_tx_hword_index[uart_num] &= 0x01 ;
}

/**
 * @brief     Send uart data by word in no_dma mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] data - the data to be send.
 * @return    none
 */
void uart_send_word(uart_num_e uart_num, unsigned int data)
{
	while (uart_get_txfifo_num(uart_num)>4);
	reg_uart_data_word_buf(uart_num) = data;

}

/**
 * @brief     Sets the RTS pin's level manually,this function is used in combination with uart_rts_manual_mode.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] polarity - set the output of RTS pin.
 * @return    none
 */
void uart_set_rts_level(uart_num_e uart_num, unsigned char polarity)
{
    if (polarity) {
    	reg_uart_ctrl2(uart_num) |= FLD_UART_RTS_MANUAL_V;
    }
    else {
    	reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_MANUAL_V);
    }
}

/**
 *	@brief		Set pin for UART cts function,the pin connection mode:CTS<->RTS.
 *	@param[in]  uart_num - UART0 or UART1.
 *	@param[in]  cts_pin -To set cts pin.
 *	@return		none
 */
void uart_set_cts_pin(uart_num_e uart_num,gpio_func_pin_e cts_pin)
{
	if(0==uart_num)
	{
	    gpio_set_mux_function(cts_pin,UART0_CTS_I);
	}
	else
	{
		gpio_set_mux_function(cts_pin,UART1_CTS_I);
	}
	gpio_function_dis((gpio_pin_e)cts_pin);
}

/**
 *	@brief		Set pin for UART rts function,the pin connection mode:RTS<->CTS.
 *	@param[in]  uart_num - UART0 or UART1.
 *	@param[in]  rts_pin - To set rts pin.
 *	@return		none
 */
void uart_set_rts_pin(uart_num_e uart_num,gpio_func_pin_e rts_pin)
{
	if(0==uart_num)
	{
		gpio_set_mux_function(rts_pin,UART0_RTS);
	}
	else
	{
		gpio_set_mux_function(rts_pin,UART1_RTS);
	}
	gpio_function_dis((gpio_pin_e)rts_pin);
}

/**
* @brief      Select pin for UART module,the pin connection mode:TX<->RX RX<->TX.
* @param[in]  uart_num - UART0 or UART1.
* @param[in]  tx_pin   - the pin to send data.
* @param[in]  rx_pin   - the pin to receive data.
* @return     none
*/
void uart_set_pin(uart_num_e uart_num,gpio_func_pin_e tx_pin,gpio_func_pin_e rx_pin)
{
	//When the pad is configured with mux input and a pull-up resistor is required, gpio_input_en needs to be placed before gpio_function_dis,
	//otherwise first set gpio_input_disable and then call the mux function interface,the mux pad will misread the short low-level timing.confirmed by minghai.20210709.
	if(tx_pin != GPIO_NONE_PIN){
		gpio_input_en((gpio_pin_e)tx_pin);
		gpio_set_up_down_res((gpio_pin_e)tx_pin, GPIO_PIN_PULLUP_10K);
		if (0 == uart_num) {
			gpio_set_mux_function(tx_pin, UART0_TX);
		} else {
			gpio_set_mux_function(tx_pin, UART1_TX);
		}
		gpio_function_dis((gpio_pin_e)tx_pin);
	}
	if(rx_pin != GPIO_NONE_PIN){
		gpio_input_en((gpio_pin_e)rx_pin);
		gpio_set_up_down_res((gpio_pin_e)rx_pin, GPIO_PIN_PULLUP_10K);
		if (0 == uart_num) {
			gpio_set_mux_function(rx_pin, UART0_RTX_IO);
		} else {
			gpio_set_mux_function(rx_pin, UART1_RTX);
		}
		gpio_function_dis((gpio_pin_e)rx_pin);
	}
}


/**
* @brief      Set rtx pin for UART module,this pin can be used as either tx or rx,it is the rx function by default,
*             When there is data in tx_fifo and the interface uart_rtx_pin_tx_trig is called, it is converted to tx function until tx_fifo is empty and converted to rx.
* @param[in]  uart_num - UART0 or UART1.
* @param[in]  rtx_pin  - the rtx pin need to set.
* @return     none
*/
void uart_set_rtx_pin(uart_num_e uart_num,gpio_func_pin_e rtx_pin)
{
	//When the pad is configured with mux input and a pull-up resistor is required, gpio_input_en needs to be placed before gpio_function_dis,
	//otherwise first set gpio_input_disable and then call the mux function interface,the mux pad will misread the short low-level timing.confirmed by minghai.20210709.
	gpio_input_en((gpio_pin_e)rtx_pin);
	gpio_set_up_down_res((gpio_pin_e)rtx_pin, GPIO_PIN_PULLUP_10K);
	if(0==uart_num){
    	gpio_set_mux_function(rtx_pin,UART0_RTX_IO);
	}
	else if(1==uart_num)
	{
		gpio_set_mux_function(rtx_pin,UART1_RTX);
	}
	gpio_function_dis((gpio_pin_e)rtx_pin);
}
/**
* @brief     Send an amount of data in NODMA mode
* @param[in] uart_num - UART0 or UART1.
* @param[in] addr     - pointer to the buffer.
* @param[in] len      - NDMA transmission length.
* @return    1
*/
unsigned char uart_send(uart_num_e uart_num, unsigned char * addr, unsigned char len )
{
	for(unsigned char i=0;i<len;i++)
	{
		uart_send_byte(uart_num,addr[i]);
	}
	return 1;
}


/**
 * @brief     	Send an amount of data in DMA mode
 * @param[in]  	uart_num - uart channel
 * @param[in] 	addr     - Pointer to data buffer. It must be 4-bytes aligned address
 * @param[in] 	len      - Amount of data to be sent in bytes, range from 1 to 0xFFFFFC
 * @return      1  DMA start send.
 *              0  the length is error.
 */
unsigned char uart_send_dma(uart_num_e uart_num, unsigned char * addr, unsigned int len )
{
	if(len!=0)
	{
		//In order to prevent the time between the last piece of data and the next piece of data is less than the set timeout time,
	    //causing the receiver to treat the next piece of data as the last piece of data.
		uart_clr_irq_status(uart_num,UART_TXDONE_IRQ_STATUS);
	    dma_set_address(uart_dma_tx_chn[uart_num],(unsigned int)(addr),reg_uart_data_buf_adr(uart_num));
	    dma_set_size(uart_dma_tx_chn[uart_num],len,DMA_WORD_WIDTH);
	    dma_chn_en(uart_dma_tx_chn[uart_num]);
	    return 1;
	}
	else
	{
		return 0;
	}
}


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
void uart_receive_dma(uart_num_e uart_num, unsigned char * addr,unsigned int rev_size)
{

	uart_dma_rev_size[uart_num] = rev_size;
	uart_rxdone_sel(uart_num,UART_DMA_MODE);
/*
 *1.If the DMA length is set to 0xFFFFFC (maximum), the hardware has a write back length, no need uart_get_dma_rev_data_len to calculate,
 *  uart_auto_clr_rx_fifo_pointer is enabled, the hardware can clear the fifo without manual clearing,the benefits of this feature:
 *  when the interval between two packets is small and rx_done is also generated, use this feature to hardware clear rx_done,
 *  avoiding manual clearing of data that will be in the next packet.
 *2.If the DMA length is not the maximum, have no write back length,and uart_get_dma_rev_data_len is required to manually calculate the length,
 *  the uart_auto_clr_rx_fifo_pointer function cannot be enabled,Otherwise, the length is incorrectly calculated,and write_num need to disable,Otherwise
 *  Otherwise, the dma work will be abnormal.
 */
	if(rev_size == 0xFFFFFC){
		uart_auto_clr_rx_fifo_ptr(uart_num,1);
		dma_set_wnum_en(uart_dma_rx_chn[uart_num]);
	}
	else if(rev_size < 0xFFFFFC){
		dma_set_wnum_dis(uart_dma_rx_chn[uart_num]);
		dma_chn_dis(uart_dma_rx_chn[uart_num]);
		uart_auto_clr_rx_fifo_ptr(uart_num,0);
	}
	dma_set_address(uart_dma_rx_chn[uart_num],reg_uart_data_buf_adr(uart_num),(unsigned int)(addr));
	dma_set_size(uart_dma_rx_chn[uart_num], rev_size, DMA_WORD_WIDTH);
	dma_chn_en(uart_dma_rx_chn[uart_num]);
}

/**
 * @brief     Get data length that dma received In the case of rev_size < 0xFFFFFC,
 *            and when the receive length is larger than the set length,the length calculated by this function is the length set by DMA, and excess data is discarded.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] chn      - dma channel.
 * @return    data length.
 */
unsigned int uart_get_dma_rev_data_len(uart_num_e uart_num,dma_chn_e chn)
{
 /*
  * the real dma received length is coming from the following:
  * 1.uart_dma_rev_size[uart_num] - dma_remaining_word*4;
  * 2.due to dma only move 4 bytes from fifo,so check the uart receive cnt to see how many valid data in fifo,
  *   then minus the last word(4bytes) + the valid remaining bytes.
  */

	unsigned int data_len=0;
	unsigned int buff_data_len = (reg_uart_status(uart_num)&FLD_UART_RBCNT)%4;

	data_len = uart_dma_rev_size[uart_num] - reg_dma_size(chn)*4;
	if(buff_data_len){
		data_len =data_len - 4 + buff_data_len;
	}
	return data_len;
}


/**
  * @brief     Configures the uart tx_dma channel control register.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] chn      - dma channel.
  * @return    none
  */
 void uart_set_tx_dma_config(uart_num_e uart_num, dma_chn_e chn)
 {
	uart_dma_tx_chn[uart_num]=chn;
 	dma_config(chn, &uart_tx_dma_config[uart_num]);
 }

 /**
   * @brief     Configures uart rx_dma channel control register.
   * @param[in] uart_num - UART0 or UART1.
   * @param[in] chn      - dma channel.
   * @return    none
   */
 void uart_set_rx_dma_config(uart_num_e uart_num, dma_chn_e chn)
 {
	//no_dma mode: rxdone(UART_RXDONE_IRQ_STATUS) function switch; 1:enable,0:disable;dma mode must disable.
	reg_uart_ctrl0(uart_num)&=~FLD_UART_NDMA_RXDONE_EN;
	uart_dma_rx_chn[uart_num]=chn;
 	dma_config(chn, &uart_rx_dma_config[uart_num]);
 }

 /**
   * @brief     Configure UART hardware flow CTS.
   * @param[in] uart_num   - UART0 or UART1.
   * @param[in] cts_pin    - cts pin select.
   * @param[in] cts_parity -  1:Active high,when the cts receives an active level, it stops sending data.
   *                          0:Active low
   * @return    none
   */
 void uart_cts_config(uart_num_e uart_num,gpio_func_pin_e cts_pin,unsigned char cts_parity)
 {
	 //When the pad is configured with mux input and a pull-up resistor is required, gpio_input_en needs to be placed before gpio_function_dis,
	 //otherwise first set gpio_input_disable and then call the mux function interface,the mux pad will misread the short low-level timing.confirmed by minghai.20210709.
	 gpio_input_en((gpio_pin_e)cts_pin);//enable input
	 uart_set_cts_pin(uart_num,cts_pin);

	if (cts_parity)
	{
		reg_uart_ctrl1(uart_num) |= FLD_UART_TX_CTS_POLARITY;
	}
	else
	{
		reg_uart_ctrl1(uart_num)  &= (~FLD_UART_TX_CTS_POLARITY);
	}
 }

 /**
  * @brief     Configure UART hardware flow RTS.
  * @param[in] uart_num     - UART0 or UART1.
  * @param[in] rts_pin      - RTS pin select.
  * @param[in] rts_parity   - 0: Active high  1: Active low
  *                           in auto mode,when the condition is reached, the rts active level is activated, and in manual mode, the rts level can be manually controlled.
  * @param[in] auto_mode_en - set the mode of RTS(auto or manual).
  * @return    none
  */
 void uart_rts_config(uart_num_e uart_num,gpio_func_pin_e rts_pin,unsigned char rts_parity,unsigned char auto_mode_en)
 {

	uart_set_rts_pin(uart_num,rts_pin);

	if (auto_mode_en)
	{
		reg_uart_ctrl2(uart_num) |= FLD_UART_RTS_MANUAL_M;
	}
	else {
		reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_MANUAL_M);
	}

	if (rts_parity)
	{
		reg_uart_ctrl2(uart_num) |= FLD_UART_RTS_POLARITY;
	}
	else
	{
		reg_uart_ctrl2(uart_num) &= (~FLD_UART_RTS_POLARITY);
	}
 }

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
  void uart_set_dma_chain_llp(uart_num_e uart_num, dma_chn_e chn,unsigned char * dst_addr,unsigned int data_len,dma_chain_config_t *head_of_list)
  {
 	uart_dma_rev_size[uart_num] = data_len;
 	uart_auto_clr_rx_fifo_ptr(uart_num,1);
 	uart_rxdone_sel(uart_num,UART_DMA_MODE);
  	dma_config(chn, &uart_rx_dma_config[uart_num]);
  	dma_set_address(chn,reg_uart_data_buf_adr(uart_num),(unsigned int)(dst_addr));
  	dma_set_size(chn, data_len, DMA_WORD_WIDTH);
  	reg_dma_llp(chn)=(unsigned int)convert_ram_addr_cpu2bus(head_of_list);

  }

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
  void uart_rx_dma_add_list_element(uart_num_e uart_num,dma_chn_e chn,dma_chain_config_t *config_addr,dma_chain_config_t *llpointer ,unsigned char * dst_addr,unsigned int data_len)
  {
	uart_rxdone_sel(uart_num,UART_DMA_MODE);
	uart_auto_clr_rx_fifo_ptr(uart_num,1);
  	config_addr->dma_chain_ctl=reg_dma_ctrl(chn)|BIT(0);
  	config_addr->dma_chain_src_addr=reg_uart_data_buf_adr(uart_num);
  	config_addr->dma_chain_dst_addr= (unsigned int)convert_ram_addr_cpu2bus(dst_addr);
  	config_addr->dma_chain_data_len=dma_cal_size(data_len,4);
  	config_addr->dma_chain_llp_ptr=(unsigned int)convert_ram_addr_cpu2bus(llpointer);
  }

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
void uart_rx_dma_chain_init (uart_num_e uart_num, dma_chn_e chn,unsigned char * in_buff,unsigned int buff_size )
{
    uart_dma_rev_size[uart_num] = buff_size;
    uart_rxdone_sel(uart_num,UART_DMA_MODE);
    uart_auto_clr_rx_fifo_ptr(uart_num,1);
    uart_set_dma_chain_llp(uart_num,chn,(unsigned char*)in_buff, buff_size, &g_uart_rx_dma_list_cfg);
    uart_rx_dma_add_list_element(uart_num,chn,&g_uart_rx_dma_list_cfg, &g_uart_rx_dma_list_cfg,(unsigned char*)in_buff, buff_size);
    dma_chn_en(chn);
}
 /**********************************************************************************************************************
  *                    						local function implementation                                             *
  *********************************************************************************************************************/
 /**
   * @brief     This function is used to look for the prime.if the prime is found,it will return 1, or return 0.
   * @param[in] n - the value to judge.
   * @return    0 or 1
   */
 static unsigned char uart_is_prime(unsigned int n)
 {
 	unsigned int i = 5;
 	if(n <= 3){
 		return 1; //although n is prime, the bwpc must be larger than 2.
 	}
 	else if((n %2 == 0) || (n % 3 == 0)){
 		return 0;
 	}
 	else{
 		for(i=5;i*i<n;i+=6){
 			if((n % i == 0)||(n %(i+2))==0){
 				return 0;
 			}
 		}
 		return 1;
 	}
 }

