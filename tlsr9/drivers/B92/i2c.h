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
 * @brief	This is the header file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
/**	@page I2C
 *
 *	Introduction
 *	===============
 *	i2c support master mode or slave mode.
 *
 *	API Reference
 *	===============
 *	Header File: i2c.h
 */

#ifndef I2C_H
#define I2C_H
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


/*
 * BIT[2] can clear everything associated i2c rx_fifo.
 * BIT[3] can clear everything associated i2c tx_fifo.
 */

typedef enum{
	I2C_RX_BUFF_CLR  		= BIT(2),
	I2C_TX_BUFF_CLR         = BIT(3),
}i2c_buff_clr_e;


/*
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
 * @brief      The function of this API is to Get the number of bytes in tx_buffer.
 * @return     The actual number of bytes in tx_buffer.
 */
static inline unsigned char i2c_get_tx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_TX_BUFCNT)>>4;
}


/**
 * @brief      The function of this API is to Get the number of bytes in rx_buffer.
 * @return     The actual number of bytes in rx_buffer.
 */
static inline unsigned char i2c_get_rx_buf_cnt(void)
{
   return (reg_i2c_buf_cnt & FLD_I2C_RX_BUFCNT);
}


/**
 * @brief      The function of this API is to set the number of bytes to triggered the receive interrupt.
 *             Its default value is 4. We recommend setting it to 1 or 4.
 * @param[in]  cnt - the interrupt trigger level.
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
 * @brief      This function serves to enable i2c RX/TX/MASK_RX/MASK_TX  interrupt function.
 * @param[in]  mask - to select Interrupt type.
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
 * @brief      This function serves to disable i2c RX/TX/MASK_RX/MASK_TX  interrupt function.
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
 * @brief     This function is used to set the 'i2c_slave_rx_index' to 0.
 *			  after wakeup from power-saving mode or reset i2c or clear rx_buff, you must call this function before receiving the data.
 * @return    none.
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
 *                       ID|0x01 indicate read. ID&0xfe indicate write.
 * @return     none
 */
void i2c_slave_init(unsigned char id);



/*
 * @brief      The function of this API is to ensure that the data can be successfully sent out.
 *             1.in the id phase,detect nack,stop sending.
 *             2.in the data phase,detect nack,stop sending.
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 * @param[in]  data - The data to be sent.
 * @param[in]  len - This length is the total length, including both the length of the slave RAM address and the length of the data to be sent.
 * @return     0:received nack in id or data phase,and then stop, 1:write successfully
 */
unsigned char i2c_master_write(unsigned char id, unsigned char *data, unsigned int len);


/**
 * @brief      This function serves to read a packet of data from the specified address of slave device.
 *             in id phase, detect nack,stop receiving.
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
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
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
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
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 * @param[in]  data - The data to be sent.
 * @param[in]  len - This length is the total length, including both the length of the slave RAM address and the length of the data to be sent.
 * @return     none.
 */
void i2c_master_write_dma(unsigned char id, unsigned char *data, unsigned int len);


/**
 * @brief      This function serves to read a packet of data from the specified address of slave device by dma
 * @param[in]  id - to set the slave ID.for kite slave ID=0x5c,for eagle slave ID=0x5a.
 * @param[in]  data - Store the read data
 * @param[in]  len - The total length of the data read back.
 * @return     none
 */
void i2c_master_read_dma(unsigned char id, unsigned char *rx_data, unsigned int len);


/**
 * @brief      This function serves to write a packet of data to master device.
 * @param[in]  data - the pointer of tx_buff.
 * @param[in]  len - The total length of the data .
 * @return     none.
 */
void i2c_slave_set_tx_dma( unsigned char *data, unsigned int len);



/**
 * @brief      This function serves to receive a packet of data from master device.
 *             1.If the receiving length of DMA is set to max value:0xfffffc byte, and write_num is turned on,
 *               then The length of the data received by dma will be written to the first four bytes of addr,so need addr = the receive buff addr +4;
 *	           2.If the receiving length information of DMA is set to less than max value:0xfffffc byte, even if write_num is turned on,
 *	             the length of data received by DMA will not be written to the first four bytes of addr;
 * @param[in]  data - the pointer of rx_buff.
 * @param[in]  len  - the receive length of DMA,The maximum transmission length of DMA is 0xFFFFFC bytes, so dont'n over this length.
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
 * @brief     This function serves to receive uart data by byte with not DMA method.
 * @param[in]  data - the data need send.
 * @param[in]  len - The total length of the data.
 * @return    none
 */
void i2c_slave_write(unsigned char* data , unsigned int len);


/**
 * @brief      This function serves to set the i2c clock frequency.The i2c clock is consistent with the system clock.
 *             Currently, the default system clock is 48M, and the i2c clock is also 48M.
 * @param[in]  clock - the division factor of I2C clock,
 *             I2C frequency = System_clock / (4*DivClock).
 * @return     none
 */
void i2c_set_master_clk(unsigned char clock);

/**
 * @brief     This function serves to set i2c tx_dam channel and config dma tx default.
 * @param[in] chn: dma channel.
 * @return    none
 */
void i2c_set_tx_dma_config(dma_chn_e chn);

/**
 * @brief     This function serves to set i2c rx_dam channel and config dma rx default.
 * @param[in] chn: dma channel.
 * @return    none
 */
void i2c_set_rx_dma_config(dma_chn_e chn);


/*
 *@brief     enable i2c slave strech function,conjunction with Stretch function of master,
 *           this stretch function is usually used in combination with I2C_SLAVER_WR_IRQ interrupt,
 *           when TX_FIFO of slave terminal is empty or RX_FIFO of slave terminal is full, the interrupt state is up and the clock line is pulled down.
 *@return    none.
 */
void i2c_slave_strech_en(void);//i2c_slave_strech_en

/*
 *@brief     enable i2c master strech function,combination with whether the Stretch function of slave terminal is turned on.
 *@return    none.
 */
void i2c_master_strech_en(void);

/*
 * @brief      enable master nack function, can detect whether slave return nack.
 * @return     none.
 */
void i2c_master_detect_nack_en(void);//i2c_master_detect_nack_en
/*
 * @brief      disable i2c master nack function.
 * @return     none.
 */
void i2c_master_detect_nack_dis(void);
/**
 * @brief     This function serves to configure the master i2c send and receive byte length,the hardware needs to know what the length is..
 * @param[in] len:  len.
 * @return    none
 */
void i2c_master_set_len(unsigned int len);//i2c_set_length

/*
 *@brief     in slave ,judge whether the master is sending a read cmd or a write cmd.
 *@return    1:if return 1,it means that the slave is to get master read cmd.
 *           0:if return 0,it means that the slave is to get master write cmd.
 */
i2c_slave_wr_e i2c_slave_get_cmd();

/*
 *@brief     in master,judge whether master is to read or write
 *@return    1:if return 1,it means that the state of the master is read status.
 *           0:if return 0,it means that the state of the master is not read status.
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


/*
 *@brief   in no_dma master,check whether the master ID phase receives nack that slave returns,if nack is received, launch stop cycle.
 *@return  0:no detect nack, 1: detect nack in id phase.
 */
unsigned char i2c_master_id_nack_detect(void);

/*
 *@brief   in no_dma master,check whether the master data phase receives nack that slave returns,if nack is received,clear fifo.
 *@return  0:no detect nack, 1: detect nack in data phase.
 */
unsigned char i2c_master_data_nack_detect(void);
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
void i2c1_m_set_pin(gpio_pin_e sda_pin,gpio_pin_e scl_pin);

/**
 * @brief      This function serves to enable i2c1_m master function.
 * @param[in]  none.
 * @return     none.
 */
void i2c1_m_master_init(void);


/**
 * @brief      This function serves to set the i2c1_m clock frequency.The i2c1_m clock is consistent with the system clock.
 * @param[in]  clock - the division factor of i2c1_m clock,
 *             i2c1_m frequency = system_clock / (4*clock).
 * @return     none
 */
void i2c1_m_set_master_clk(unsigned char clock);


/**
 *  @brief      This function serves to write a packet of data to the specified address of slave device
 *  @param[in]  data_buf - the first SRAM buffer address to write data to slave in.
 *  @param[in]  data_len - the length of data master write to slave.
 *  @return     0:During sending id or data, the master receives the nack returned by the slave, and stops sending.
 *              1:Master sent data successfully.
 */
unsigned char i2c1_m_master_write(unsigned char id, unsigned char * data_buf, unsigned int data_len);



/**
 * @brief      This function serves to read a packet of data.
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
 * @param[in]  wr_data - specifies the SRAM address of the slave to send data.
 * @param[in]  wr_len - the length of register. enum 0 or 1 or 2 or 3 based on the demand of i2c slave.
 * @param[in]  rd_data - the first address of SRAM buffer master store data in.
 * @param[in]  rd_len - the length of data master read from slave.
 * @return     0:During sending id+address, the master receives the nack returned by the slave, and stops sending.
 *             1:Master receive data successfully.
 */
unsigned char i2c1_m_master_write_read(unsigned char id, unsigned char *wr_data, unsigned int wr_len, unsigned char *rd_data, unsigned int rd_len);



#endif










