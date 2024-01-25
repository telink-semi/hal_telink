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
 * @file	i2c.h
 *
 * @brief	This is the header file for B93
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
/**	@page I2C
 *
 *	Introduction
 *	===============
 *	-# B93 supports two i2c: i2c/i2c1_m.
 *	-# i2c:support master mode or slave mode,and support nodma/dma.
 *	-# i2c1_m: only support master mode,only support nodma.
 *  -# i2c1_m master mode and i2c master mode are different in design, see the demo and interface implementation.
 *
 *	API Reference
 *	===============
 *	Header File: i2c.h
 */

#ifndef I2C_H
#define I2C_H
#include <stdbool.h>
#include "analog.h"
#include "dma.h"
#include "gpio.h"
#include "reg_include/i2c_reg.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
extern unsigned char i2c_slave_rx_index;
typedef enum{
    I2C_SLAVE_WR_MASK      =  BIT(0),
	I2C_MASTER_NAK_MASK     =  BIT(1),
	I2C_RX_BUF_MASK         =  BIT(2),
	I2C_TX_BUF_MASK         =  BIT(3),
	I2C_RX_DONE_MASK 		=  BIT(4),
	I2C_TX_DONE_MASK        =  BIT(5),
	I2C_RX_END_MASK         =  BIT(6),
	I2C_TX_END_MASK         =  BIT(7),
	I2C_STRETCH_IRQ         =  BIT(8),
}i2c_irq_mask_e;

/**
 *  @brief  Define UART IRQ BIT STATUS
 *  -# I2C_SLAVE_WR_STATUS:the slave parsing master cmd interrupt,when received the master read or write cmd, will generate interrupt.
 *  -# I2C_MASTER_NAK_STATUS:master detect to the nack, will generate interrupt
 *  -# I2C_RX_BUF_STATUS: rxfifo cnt> = i2c_rx_irq_trig_cnt generates an interrupt.
 *  -# I2C_TX_BUF_STATUS: rxfifo_cnt <= tx_irq_trig_lev, generate interrupt.
 *  -# I2C_RX_DONE_STATUS:when the stop signal is detected, an interrupt occurs.
 *  -# I2C_TX_DONE_STATUS:when the stop signal is detected, an interrupt occurs.
 *  -# I2C_RX_END_STATUS:An interrupt is generated when one frame of data is received(the stop signal has been sent).
 *  -# I2C_TX_END_STATUS:An interrupt is generated when one frame of data is sent(the stop signal has been sent).
 *  -# I2C_STRETCH_STATUS: the slave stretch interrupt,when tx_fifo is empty or rx_fifo is full,will generate interrupt.
 */
typedef enum{
	I2C_SLAVE_WR_STATUS     =  BIT(0),
	I2C_MASTER_NAK_STATUS    =  BIT(1),
	I2C_RX_BUF_STATUS        =  BIT(2),
	I2C_TX_BUF_STATUS        =  BIT(3),
	I2C_RX_DONE_STATUS 		 =  BIT(4),
	I2C_TX_DONE_STATUS       =  BIT(5),
	I2C_RX_END_STATUS        =  BIT(6),
	I2C_TX_END_STATUS        =  BIT(7),
	I2C_STRETCH_STATUS       =  BIT(8),
}i2c_irq_status_e;

typedef enum{
	I2C_SLAVE_WR_CLR      =  BIT(0),
	I2C_MASTER_NAK_CLR    =  BIT(1),
	I2C_RX_BUF_CLR        =  BIT(2),
	I2C_TX_BUF_CLR        =  BIT(3),
	I2C_RX_DONE_CLR 	  =  BIT(4),
	I2C_TX_DONE_CLR       =  BIT(5),
	I2C_RX_END_CLR        =  BIT(6),
	I2C_TX_END_CLR        =  BIT(7),
	I2C_STRETCH_CLR       =  BIT(8),

}i2c_irq_clr_e;


/**
 * BIT[2] can clear everything associated i2c rx_fifo.
 * BIT[3] can clear everything associated i2c tx_fifo.
 */

typedef enum{
	I2C_RX_BUFF_CLR  		= BIT(2),
	I2C_TX_BUFF_CLR         = BIT(3),
}i2c_buff_clr_e;


/**
 * this register is use to get slave relevant status
 * BIT[1] slave stretch indicate.
 * BIT[2] judge slave tx_fifo is empty.
 * BIT[3] judge slave rx_fifo is full.
 */
typedef enum{
	I2C_SLAVER_STRETCH_INDICATE         = BIT(1),
	I2C_SLAVER_TX_EMPTY                 = BIT(2),
	I2C_SLAVE_RX_FULL                   = BIT(3),
}i2c_slave_status1_e;


/**
 *  judge the read and write status of slave.
 */
typedef enum{
    I2C_SLAVE_WRITE   = 1,
	I2C_SLAVE_READ    = 0,
}i2c_slave_wr_e;

/**
 * judge the read and write status of master.
 */
typedef enum{
	I2C_MASTER_WRITE =0,
	I2C_MASTER_READ =1,
}i2c_master_wr_e;


/**
 * @brief      The function of this API is to determine whether the bus is busy.
 * @return     1:Indicates that the bus is busy. 0:Indicates that the bus is free
 */
static inline _Bool i2c_master_busy(void)
{
    return reg_i2c_mst & FLD_I2C_MST_BUSY;
}

/**
 * @brief      This function serves to get the txfifo cnt,FLD_I2C_TX_BUFCNT is decremented when there are outgoing sent data in the txfifo,
 *             when there is write data in the tx_fifo, this register is increased.
 * @return     The actual number of bytes in txfifo.
 */
static inline unsigned char i2c_get_tx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_TX_BUFCNT)>>4;
}


/**
 * @brief      This function serves to get the rxfifo cnt,FLD_I2C_RX_BUFCNT is increased when there are incoming received data rxfifo,
 *             when there is read data in the rxfifo, FLD_I2C_RX_BUFCNT is decremented.
 * @return     The actual number of bytes in rxfifo.
 */
static inline unsigned char i2c_get_rx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_RX_BUFCNT);
}


/**
 * @brief      The function of this API is to set the number of bytes to triggered the receive interrupt.
 *             Its default value is 4. We recommend setting it to 1 or 4.
 * @param[in]  cnt - the interrupt trigger level,the range is less than 8.
 * @return     none
 */
static inline void i2c_rx_irq_trig_cnt(unsigned char cnt)
{
	/*
	   in the i2c_rx_irq_trig_cnt interface,originally first set i2c_rc_irq_trig_cnt to 0 and then assign,
	   if the rx_buff mask is opened first, when set i2c_rc_irq_trig_cnt to 0,rx_fifo is empty, an interrupt will be triggered by mistake.
	*/
	reg_i2c_trig=(((reg_i2c_trig)&(~FLD_I2C_RX_IRQ_TRIG_LEV))|(cnt& 0x0f));
}

/**
 * @brief      This function serves to enable i2c interrupt function.
 * @param[in]  mask - to select interrupt type.
 * @return     none
 */
static inline void i2c_set_irq_mask(i2c_irq_mask_e mask)
{
	if(mask & I2C_STRETCH_IRQ)
	{
		reg_i2c_ctrl2  |=  FLD_I2C_MASK_STRETCH;
	}

	 reg_i2c_mask0  |=  mask;
}

/**
 * @brief      This function serves to disable i2c interrupt function.
 * @param[in]  mask - to select Interrupt type.
 * @return     none
 */
static inline void i2c_clr_irq_mask(i2c_irq_mask_e mask)
{
	if(mask & I2C_STRETCH_IRQ)
	{
		reg_i2c_ctrl2  &= (~ FLD_I2C_MASK_STRETCH);
	}

	    reg_i2c_mask0  &=  (~mask);
}

/**
 * @brief      This function serves to get i2c interrupt status.
 * @param[in]  status - to select Interrupt status type.
 * @return     1:the interrupt status type is 1, 0: the interrupt status type is 0.
 *
 */
static inline unsigned char i2c_get_irq_status(i2c_irq_status_e status)
{
	if(status  & I2C_STRETCH_IRQ)
	{
		return i2c_slave_status1&FLD_I2C_SS_SCL_IRQ;
	}

	    return reg_i2c_status0&status;

}
/**
 * @brief     This function is used to set the 'i2c_slave_rx_index' to 0,
 *            'i2c_slave_rx_index' is used to synchronize the rxfifo hardware pointer in no_dma mode.
 * @return    none.
 * @note      note the following:
 *            -# After calling the i2c reset interface, i2c_slave_clr_rx_index must be called to clear the read pointer,
 *               after the i2c reset interface is invoked, the hardware read pointers are cleared to zero.
 *               Therefore, the software write pointers are cleared to ensure logical correctness.
 *            -# After suspend wakes up, you must call i2c_slave_clr_rx_index to clear read pointers,
 *               because after suspend wakes up, the chip is equivalent to performing a i2c_reset,
 *               so the software read pointer also needs to be cleared to zero.
 */
static inline void i2c_slave_clr_rx_index()
{
	i2c_slave_rx_index=0;
}
/**
 * @brief      This function serves to clear i2c fifo.
 * @return     none
 */
static inline void  i2c_clr_fifo(i2c_buff_clr_e clr)
{
	reg_i2c_status0 |=clr;
	if(I2C_RX_BUFF_CLR == clr)
	{
		i2c_slave_clr_rx_index();
	}
}
/**
 * @brief      This function serves to clear i2c irq status.
 * @param[in]  status - to select interrupt status type.
 * @return     none
 */
static inline void  i2c_clr_irq_status(i2c_irq_clr_e status)
{
	if(status  & I2C_STRETCH_IRQ)
	{
		i2c_slave_status1 |= FLD_I2C_SS_SCL_IRQ;
	}

	 reg_i2c_status0=status;
}

/**
 * @brief      This function serves to enable slave mode.
 * @param[in]  id - the id of slave device.it contains write or read bit,the laster bit is write or read bit.
 *                  ID|0x01 indicate read. ID&0xfe indicate write.
 * @return     none
 */
void i2c_slave_init(unsigned char id);



/**
 * @brief      The function of this API is to ensure that the data can be successfully sent out.
 *             -# in the id phase,detect nack,stop sending.
 *             -# in the data phase,detect nack,stop sending.
 * @param[in]  id - to set the slave ID.
 * @param[in]  data - The data to be sent.
 * @param[in]  len - This length is the total length, including both the length of the slave RAM address and the length of the data to be sent.
 * @return     0:received nack in id or data phase,and then stop, 1:write successfully
 */
unsigned char i2c_master_write(unsigned char id, unsigned char *data, unsigned int len);


/**
 * @brief      This function serves to read a packet of data from the specified address of slave device.
 *             in id phase, detect nack,stop receiving.
 * @param[in]  id - to set the slave ID.
 * @param[in]  data - Store the read data
 * @param[in]  len - The total length of the data read back.
 * @return     0 : the master receive NACK after sending out the id and then send stop.  1: the master receive the data successfully.
 */
unsigned char  i2c_master_read(unsigned char id, unsigned char *data, unsigned int len);

/**
 * @brief      This function serves to write data and read data,
 *             can choose whether to send stop or not,If i2c stop is not configured,the master between writing and reading, it will be a restart signal,
 *             and after reading and writing, a stop signal is sent,but the stop configure,stays in its original state,
 *             when a nack exception signal is received, during exception handling, a stop signal will be sent.
 * @param[in]  id - to set the slave ID.
 * @param[in]  wr_data - The data to be sent.
 * @param[in]  wr_len -  This length is the total length, including both the length of the slave RAM address and the length of the data to be sent.
 * @param[in]  rd_data - Store the read data
 * @param[in]  rd_len -  The total length of the data read back.
 * @return     0: the master detect nack in the id or data when the master write,stop sending,and return.
 *                or the master detect nack in the id when the master read,stop receiving,and return.
 *             1: the master receive the data successfully.
 */
unsigned char i2c_master_write_read(unsigned char id, unsigned char *wr_data, unsigned int wr_len, unsigned char *rd_data, unsigned int rd_len);



/**
 * @brief      The function of this API is just to write data to the i2c tx_fifo by DMA.
 *             can choose whether to send stop,if i2c stop is not configured, the next time a start signal is sent, it will be a restart signal,
 *             but if a nack exception is received in id or data phase, during exception handling, a stop signal will be sent.
 * @param[in]  id   - to set the slave ID.
 * @param[in]  data - The data to be sent.
 * @param[in]  len  - This length is the total length, including both the length of the slave RAM address and the length of the data to be sent,
 *                    the maximum transmission length of DMA is 0xFFFFFC bytes, so dont'n over this length.
 * @return     none.
 * @note       data: must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void i2c_master_write_dma(unsigned char id, unsigned char *data, unsigned int len);


/**
 * @brief      This function serves to read a packet of data from the specified address of slave device by dma.
 *             can choose whether to send stop,if i2c stop is not configured, the next time a start signal is sent, it will be a restart signal,
 *             but if a nack exception is received in id phase, during exception handling, a stop signal will be sent.
 * @param[in]  id      - to set the slave ID.
 * @param[in]  rx_data - Store the read data.
 * @param[in]  len     - The total length of the data read back,the maximum transmission length of DMA is 0xFFFFFC bytes, so dont'n over this length.
 * @return     none
 * @note       rx_data: must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void i2c_master_read_dma(unsigned char id, unsigned char *rx_data, unsigned int len);


/**
 * @brief      Send an amount of data in DMA mode
 * @param[in]  data -  Pointer to data buffer, it must be 4-bytes aligned address
 * @param[in]  len  -  Amount of data to be sent in bytes, range from 1 to 0xFFFFFC
 * @return     none.
 */
void i2c_slave_set_tx_dma( unsigned char *data, unsigned int len);



/**
 * @brief      Receive an amount of data in DMA mode
 * @param[in]  data - Pointer to data buffer, it must be 4-bytes aligned address
 * @param[in]  len  - Length of DMA in bytes��It must be set to 0xFFFFFC.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *			   -# you need to leave 4bytes of space for the length information.
 *			   -# dma is transmitted in accordance with 4bytes, so the length of the buffer needs to be a multiple of 4. Otherwise, there may be an out-of-bounds problem
 *			   For example, the actual received data length is 5bytes, the minimum value of the actual buffer size that the user needs to set is 12bytes, and the calculation of 12bytes is explained as follows:
 *			   4bytes (length information) + 5bytes (data) + 3bytes (the number of additional bytes to prevent out-of-bounds)
 * @return     none.
 */
void i2c_slave_set_rx_dma(unsigned char *data, unsigned int len);


/**
 * @brief     This function serves to receive data .
 * @param[in]  data - the data need read.
 * @param[in]  len - The total length of the data
 * @return    none
 */
void i2c_slave_read(unsigned char* data , unsigned int len );


/**
 * @brief     This function serves to receive i2c data by byte with not DMA method.
 * @param[in]  data - the data need send.
 * @param[in]  len - The total length of the data.
 * @return    none
 */
void i2c_slave_write(unsigned char* data , unsigned int len);


/**
 * @brief      This function serves to set the i2c clock frequency.The i2c clock is consistent with the pclk.
 * @param[in]  clock - the division factor of I2C clock,the i2c frequency can meet 1M, and the maximum limit is not confirmed,
 *             I2C frequency = pclk/ (4*DivClock).
 * @return     none
 */
void i2c_set_master_clk(unsigned char clock);

/**
 * @brief     This function serves to set i2c tx_dma channel and config dma tx default.
 * @param[in] chn: dma channel.
 * @return    none
 */
void i2c_set_tx_dma_config(dma_chn_e chn);

/**
 * @brief     This function serves to set i2c rx_dma channel and config dma rx default.
 * @param[in] chn: dma channel.
 * @return    none
 */
void i2c_set_rx_dma_config(dma_chn_e chn);


/**
 *@brief     This function serves to enable i2c slave stretch function,conjunction with stretch function of master,
 *           this stretch function is usually used in combination with I2C_SLAVER_WR_IRQ interrupt,
 *           when TX_FIFO of slave terminal is empty or RX_FIFO of slave terminal is full, the interrupt state is up and the clock line is pulled down.
 *@return    none.
 */
static inline void i2c_slave_stretch_en(void){

	reg_i2c_ctrl3 |= FLD_I2C_SLAVE_STRETCH_EN;
}

/**
 *@brief     This function serves to disable i2c slave stretch function.
 *@return    none.
 */
static inline void i2c_slave_stretch_dis(void){

	reg_i2c_ctrl3 &= ~FLD_I2C_SLAVE_STRETCH_EN;
}

/**
 *@brief     This function serves to i2c slave manual stretch function,conjunction with stretch function of master,
 *           When this interface is called, clk will be pulled, it should be noted that this interface can only be called when the master is in the idle state.
 *@return    none.
 */
static inline void i2c_slave_manual_stretch_en(){
	reg_i2c_status|=FLD_I2C_R_MANUAL_STRETCH;
}

/**
 *@brief     This function serves to clear i2c slave manual stretch function,When the interface is called, clk recovers.
 *@return    none.
 */
static inline void i2c_slave_manual_stretch_clr(){
	reg_i2c_status |= FLD_I2C_MANUAL_STRETCH_CLR;
}
/**
 *@brief     This function serves to enable i2c master stretch function,
 *           If stretch is enabled on the slave, it needs to be enabled,by default, it is enabled on the interface i2c_master_init.
 *@return    none.
 */
static inline void i2c_master_stretch_en(void){

	reg_i2c_ctrl2 |=FLD_I2C_MASTER_STRETCH_EN;
}

/**
 *@brief     This function serves to disable i2c master stretch function.
 *@return    none.
 */
static inline void i2c_master_stretch_dis(void){

	reg_i2c_ctrl2 &=~(FLD_I2C_MASTER_STRETCH_EN);
}


/**
 * @brief      this function serves to:
 *             -# when the nack detect function is used on the master, need to enable;
 *             -# during the sending and receiving of data, if the master detects nack,turn on the function that sends the stop signal,
 *                otherwise, the stop signal cannot be sent, resulting in the busy state of the master,this interface is enabled by default on the i2c_master_init interface.
 * @return     none.
 */
static inline void i2c_master_detect_nack_en(void)
{
	reg_i2c_ctrl2 |= FLD_I2C_MASTER_NAK_STOP_EN;
}

/**
 * @brief      This function serves to disable i2c master nack stop function.
 * @return     none.
 */
static inline void i2c_master_detect_nack_dis(void)
{
	reg_i2c_ctrl2 &= ~(FLD_I2C_MASTER_NAK_STOP_EN);
}
/**
 * @brief     This function serves to configure the master i2c send and receive byte length,the hardware needs to know what the length is.
 * @param[in] len - the maximum transmission length of i2c is 0xffffff bytes, so dont'n over this length.
 * @return    none
 */
void i2c_master_set_len(unsigned int len);

/**
 *@brief     in slave ,judge whether the master is sending a read cmd or a write cmd.
 *@return    1:if return 1,it means that the slave is to get master read cmd.
 *           0:if return 0,it means that the slave is to get master write cmd.
 */
i2c_slave_wr_e i2c_slave_get_cmd();

/**
 * @brief     in master,judge whether master is to read or write
 * @return    1:if return 1,it means that the state of the master is read status.
 *            0:if return 0,it means that the state of the master is not read status.
 */
i2c_master_wr_e i2c_get_master_wr_status();

/**
 * @brief      The function of this interface is equivalent to that after the user finishes calling the write or read interface, the stop signal is not sent,
 * 			   and then the write or read command is executed again. The driver defaults that every write or read API will send a stop command at the end
 * @param[in]  en - Input parameters.Decide whether to disable the stop function after each write or read interface
 * @return     none
 */
void i2c_master_send_stop(unsigned char en);

/**
 * @brief      This function selects a pin port for I2C interface.
 * @param[in]  sda_pin - the pin port selected as I2C sda pin port.
 * @param[in]  scl_pin - the pin port selected as I2C scl pin port.
 * @return     none
 */
void i2c_set_pin(gpio_func_pin_e sda_pin,gpio_func_pin_e scl_pin);


/**
 * @brief      This function serves to enable i2c master function.
 * @return     none.
 */
void i2c_master_init(void);


/********************************************************************************************
 *****|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|*****
 *****|								i2c1_m.h 						                   |*****
 *****|~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~|*****
 ********************************************************************************************/

/**
 * @brief      The function of this API is to determine whether the bus is busy.
 * @return     1:Indicates that the bus is busy. 0:Indicates that the bus is free
 */
static inline _Bool i2c1_m_master_busy(void)
{
    return reg_i2c1_m_mst & FLD_I2C1_M_MST_BUSY;
}



/**
 * @brief      The function of this interface is equivalent to that after the user finishes calling the write or read interface, the stop signal is not sent,
 * 			   and then the write or read command is executed again. The driver defaults that every write or read API will send a stop command at the end
 * @param[in]  en - Input parameters.Decide whether to disable the stop function after each write or read interface
 * @return     none
 */
void i2c1_m_master_send_stop(unsigned char en);


/**
 * @brief      This function selects a pin port for I2C interface.
 * @param[in]  sda_pin - the pin port selected as I2C sda pin port.
 * @param[in]  scl_pin - the pin port selected as I2C scl pin port.
 * @return     none
 */
void i2c1_m_set_pin(gpio_func_pin_e sda_pin,gpio_func_pin_e scl_pin);

/**
 * @brief      This function serves to enable i2c1_m master function.
 * @return     none.
 */
void i2c1_m_master_init(void);


/**
 * @brief      This function serves to set the i2c1_m clock frequency.The i2c1_m clock is consistent with the pclk.
 * @param[in]  clock - the division factor of i2c1_m clock,
 *             i2c1_m frequency = pclk / (4*clock).
 * @return     none
 */
void i2c1_m_set_master_clk(unsigned char clock);


/**
 *  @brief      This function serves to write a packet of data to the specified address of slave device
 *  @param[in]  id      - to set the slave ID.
 *  @param[in]  data_buf - the first SRAM buffer address to write data to slave in.
 *  @param[in]  data_len - the length of data master write to slave.
 *  @return     0:During sending id or data, the master receives the nack returned by the slave, and stops sending.
 *              1:Master sent data successfully.
 */
unsigned char i2c1_m_master_write(unsigned char id, unsigned char * data_buf, unsigned int data_len);



/**
 * @brief      This function serves to read a packet of data.
 * @param[in]  id      - to set the slave ID.
 * @param[in]  data_buf - the first address of SRAM buffer master store data in.
 * @param[in]  data_len - the length of data master read from slave.
 * @return     0:During sending id, the master receives the nack returned by the slave, and stops sending.
 *             1:Master receive data successfully.
 */
unsigned char i2c1_m_master_read(unsigned char id, unsigned char * data_buf, unsigned int data_len);



/**
 * @brief      This function serves to read a packet of data from the specified address of slave device.
 *             the master between writing and reading, it will be a restart signal,and after reading and writing, a stop signal is sent,
 *             but the stop configure,stays in its original state,when a nack exception signal is received, during exception handling, a stop signal will be sent.
 * @param[in]  id      - to set the slave ID.
 * @param[in]  wr_data - specifies the SRAM address of the slave to send data.
 * @param[in]  wr_len - the length of register. enum 0 or 1 or 2 or 3 based on the demand of i2c slave.
 * @param[in]  rd_data - the first address of SRAM buffer master store data in.
 * @param[in]  rd_len - the length of data master read from slave.
 * @return     0:During sending id+address, the master receives the nack returned by the slave, and stops sending.
 *             1:Master receive data successfully.
 */
unsigned char i2c1_m_master_write_read(unsigned char id, unsigned char *wr_data, unsigned int wr_len, unsigned char *rd_data, unsigned int rd_len);



#endif










