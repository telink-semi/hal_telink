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
 * @file	mspi.c
 *
 * @brief	This is the source file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "mspi.h"


/**
 * @brief     	This function servers to write mspi fifo.
 * @param[in] 	data 	- the pointer to the data for write.
 * @param[in] 	len 	- write length.
 * @return    	none
 */
_attribute_ram_code_sec_noinline_  void mspi_write(unsigned char *data, unsigned int len)
{
	unsigned char word_len = len >> 2;
	unsigned char single_len = len & 3;
	for (unsigned int i = 0; i < word_len; i++)
	{
		//while (20 - (reg_mspi_txfifo_status & FLD_MSPI_TXFIFO_ENTRIES) < 4);    //do not need software to block,use hardware block mcu.
		reg_mspi_wr_rd_data_word = ((unsigned int *)data)[i];
	}

	for (unsigned int i = 0; i < single_len; i++)
	{
		//while (reg_mspi_txfifo_status & FLD_MSPI_TXFIFO_FULL);    //do not need software to block,use hardware block mcu.
		reg_mspi_wr_rd_data(i % 4) = data[(word_len*4) + i];
	}
	while (mspi_is_busy());
}

/**
 * @brief     	This function servers to read mspi fifo.
 * @param[in] 	data 	- the pointer to the data for read.
 * @param[in] 	len 	- write length.
 * @return    	none
 */
_attribute_ram_code_sec_noinline_  void mspi_read(unsigned char *data, unsigned int len)
{
	unsigned int word_len = len >> 2;
	unsigned int single_len = len & 3;

	for (unsigned int i = 0; i < word_len; i++)
	{
//	    do{
//	    }while ((reg_mspi_rxfifo_status & FLD_MSPI_RXFIFO_ENTRIES) < 4);  //do not need software to block,use hardware block mcu.
		((unsigned int *)data)[i] = reg_mspi_wr_rd_data_word;
	}

	for (unsigned char i = 0; i < single_len; i++)
	{
//		while (reg_mspi_rxfifo_status & FLD_MSPI_RXFIFO_EMPTY);   //do not need software to block,use hardware block mcu.
		data[(word_len*4) + i] = reg_mspi_wr_rd_data(i % 4);
	}
	while (mspi_is_busy());
}
