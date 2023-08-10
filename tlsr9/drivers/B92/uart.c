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
  * @brief     This function is used to look for the prime.if the prime is finded,it will return 1, or return 0.
  * @param[in] n - the calue to judge.
  * @return    0 or 1
  */
 static unsigned char uart_is_prime(unsigned int n);

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/

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

/***********************************************************
 * @brief  		This function serves to calculate the best bwpc(bit width) .i.e reg0x96.
 * @param[in]	baudrate - baut rate of UART.
 * @param[in]	pclk     - p-clock.
 * @param[out]	div      - uart clock divider. range[0-127]
 * @param[out]	bwpc     - bitwidth, should be set to larger than 2. range[3-15]
 * @return 		none
 * @note        BaudRate*(div+1)*(bwpc+1) = p-clock
 *  		    simplify the expression: div*bwpc =  constant(z)
 * 		        bwpc range from 3 to 15.so loop and get the minimum one decimal point
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
		D_intdec[i-3] = (10*primeInt)/(i+1);////get the LSB
		D_dec[i-3] = D_intdec[i-3] - 10*(D_intdec[i-3]/10);///get the decimal section
		D_int[i-3] = D_intdec[i-3]/10;///get the integer section
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
 * @brief  		This function serves to set r_rxtimeout. this setting is transfer one bytes need cycles base on uart_clk.
 * 				For example, if  transfer one bytes (1start bit+8bits data+1 priority bit+2stop bits) total 12 bits,
 * 				this register setting should be (bpwc+1)*12.
 * @param[in]	uart_num - UART0 or UART1.
 * @param[in]	bwpc     - bitwidth, should be set to larger than 2.
 * @param[in]	bit_cnt  - bit number.
 * @param[in]	mul	     - rx_timeout config,it decide the time of a packet.
 * @return 		none
 */
void uart_set_rx_timeout(uart_num_e uart_num,unsigned char bwpc, unsigned char bit_cnt, uart_timeout_mul_e mul)
{
    reg_uart_rx_timeout0(uart_num) = (bwpc+1) * bit_cnt; //one byte includes 12 bits at most
    reg_uart_rx_timeout1(uart_num) = (((reg_uart_rx_timeout1(uart_num))&(~FLD_UART_TIMEOUT_MUL))|mul); //if over 2*(tmp_bwpc+1),one transaction end.
}

 unsigned char uart_tx_byte_index[2] = {0};
/**
 * @brief     This function serves to send data by byte with not DMA method.
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
 * @brief     This function serves to receive uart data by byte with not DMA method.
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
 * @brief     This function serves to judge if the transmission of uart is done.
 * @param[in] uart_num - UART0 or UART1.
 * @return    return the tx status.
 * -          0:tx is done     1:tx isn't done
 */
unsigned char uart_tx_is_busy(uart_num_e uart_num)
{
     return  ((reg_uart_irq(uart_num) & FLD_UART_TXDONE_IRQ) ? 0:1 ) ;
}

/**
 * @brief     This function serves to send uart0 data by halfword with not DMA method.
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
 * @brief     This function serves to send data by word with not DMA method.
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
 * @brief     This function serves to set the RTS pin's level manually.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] polarity - set the output of RTS pin(only for manual mode).
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
 *	@brief		This function serves to set pin for UART cts function.
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
 *	@brief		This function serves to set pin for UART rts function.
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
* @brief      This function serves to select pin for UART module.
* @param[in]  tx_pin  - the pin to send data.
* @param[in]  rx_pin  - the pin to receive data.
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
* @brief      This function serves to set rtx pin for UART module.
*             this pin can be used as either tx or rx. this pin is only used as tx when there is a sending action, but it is used as an rx at all times.
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
* @brief     This function serves to send data with not DMA method.
* @param[in] uart_num - UART0 or UART1.
* @param[in] addr     - pointer to the buffer containing data need to send.
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
 * @brief     	This function serves to send data by DMA, this function tell the DMA to get data from the RAM and start.
 * @param[in]  	uart_num - UART0 or UART1.
 * @param[in] 	addr     - pointer to the buffer containing data need to send.
 * @param[in] 	len      - DMA transmission length.The maximum transmission length of DMA is 0xFFFFFC bytes, so dont'n over this length.
 * @return      1  dma start send.
 *              0  the length is error.
 * @note        addr: must be aligned by word (4 bytes), otherwise the program will enter an exception.
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
 * @brief     This function serves to get the length of the data that dma received.
 *            if the receiving length information of DMA is set to less than 0xFFFFFC byte,need use this function to calculate the length of data received by DMA,
 *            and when the receive length is larger than the set length,the length calculated by this function is the length set by DMA, and excess data is discarded.
 * @param[in] uart_num - UART0 or UART1.
 * @param[in] chn      - dma channel.
 * @return    data length.
 */
unsigned int uart_get_dma_rev_data_len(uart_num_e uart_num,dma_chn_e chn)
{
	unsigned int data_len=0;
	if((reg_dma_size(chn) == 0)&&(uart_get_rxfifo_num(uart_num)>0)){
		data_len = uart_dma_rev_size[uart_num];
	}
	else{
	    unsigned int buff_data_len = (reg_uart_status(uart_num)&FLD_UART_RBCNT)%4;
	    if(buff_data_len==0)
	    {
		     data_len=4*((uart_dma_rev_size[uart_num]/4)-reg_dma_size(chn));
	    }
	    else
	    {
		     data_len=4*((uart_dma_rev_size[uart_num]/4)-reg_dma_size(chn)-1)+buff_data_len;
	    }
	}
	return data_len;
}


 /**
  * @brief     This function serves to set uart tx_dam channel and config dma tx default.
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
  * @brief     This function serves to set uart rx_dam channel and config dma rx default.
  * @param[in] uart_num - UART0 or UART1.
  * @param[in] chn      - dma channel.
  * @return    none
  */
 void uart_set_rx_dma_config(uart_num_e uart_num, dma_chn_e chn)
 {
	//no_dma mode: rxdone(timeout) function switch; 1:enable,0:disable;dma mode must disable.
	reg_uart_ctrl0(uart_num)&=~FLD_UART_NDMA_RXDONE_EN;
	uart_dma_rx_chn[uart_num]=chn;
 	dma_config(chn, &uart_rx_dma_config[uart_num]);
 }

 /**
  * @brief     UART hardware flow control configuration. Configure CTS.
  * @param[in] uart_num   - UART0 or UART1.
  * @param[in] cts_pin    - RTS pin select.
  * @param[in] cts_parity - when CTS's input equals to select, tx will be stopped.
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
  * @brief     UART hardware flow control configuration. Configure RTS.
  * @param[in] uart_num     - UART0 or UART1.
  * @param[in] rts_pin      - RTS pin select.
  * @param[in] rts_parity   - whether invert the output of RTS pin(only for auto mode)
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
   * @brief     This function servers to configure DMA head node,the chain function only applies to data_len = 0xFFFFFC.
   * @param[in] uart_num - UART0/UART1.
   * @param[in] chn          - to select the DMA channel.
   * @param[in] dst_addr     - the dma address of destination.
   * @param[in] data_len     - to configure DMA length.
   * @param[in] head_of_list - the head address of dma llp.
   * @return    none
   * @note      dst_addr: must be aligned by word (4 bytes), otherwise the program will enter an exception.
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
   * @brief  This function serves to set dma single chain transfer,the chain function only applies to data_len = 0xFFFFFC.
   * @param[in] uart_num  - UART0/UART1.
   * @param[in] chn       -  dma channel
   * @param[in] in_buff   - the pointer of rx_buff.
   * @param[in] buff_size - the size of rx_buff.
   * @return    none
   * @note      in_buff: must be aligned by word (4 bytes), otherwise the program will enter an exception.
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
   * @brief     This function is used to look for the prime.if the prime is finded,it will return 1, or return 0.
   * @param[in] n - the calue to judge.
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
