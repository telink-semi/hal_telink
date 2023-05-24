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
 * @file	flash_base.c
 *
 * @brief	This is the source file for B92
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/

#include "mspi.h"
#include "plic.h"
#include "flash.h"
#include "mspi_reg.h"
#include "flash_base.h"

#define reg_mspi_cipher_ctrl			REG_ADDR8(MSPI_BASE_ADDR + 0x85)
enum{
	FLD_MSPI_CIPHER_RD_EN	       		= BIT(0),
	FLD_MSPI_CIPHER_WR_EN	       		= BIT(1),
};
extern _attribute_data_retention_sec_ preempt_config_t s_flash_preempt_config ;

/*******************************************************************************************************************
 *								Functions for internal use in flash,
 *		There is no need to add an evasion solution to solve the problem of access flash conflicts.
 ******************************************************************************************************************/

/**
 * @brief		This function serves to enable cipher read.
 * @return		none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_read_en(void){
	reg_mspi_cipher_ctrl |=FLD_MSPI_CIPHER_RD_EN;
}

/**
 * @brief		This function serves to disable cipher read.
 * @return		none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_read_dis(void){
	reg_mspi_cipher_ctrl &=~FLD_MSPI_CIPHER_RD_EN;
}

/**
 * @brief		This function serves to enable cipher write.
 * @return		none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_write_en(void){
	reg_mspi_cipher_ctrl |=FLD_MSPI_CIPHER_WR_EN;
}

/**
 * @brief		This function serves to disable cipher write.
 * @return		none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_write_dis(void){
	reg_mspi_cipher_ctrl &=~FLD_MSPI_CIPHER_WR_EN;
}


/**
 * @brief		This function serves to set flash write command.This function interface is only used internally by flash,
 * 				and is currently included in the H file for compatibility with other SDKs. When using this interface,
 * 				please ensure that you understand the precautions of flash before using it.
 * @param[in]	cmd	- set command.
 * @return		none.
 */
_attribute_ram_code_sec_noinline_ void flash_send_cmd(unsigned char cmd)
{
	reg_mspi_ctrl1 = FLD_MSPI_CMD_EN;

	mspi_set_transmode(MSPI_MODE_NONE_DATA);//nodata

	mspi_set_cmd(cmd);
	while (mspi_is_busy());
}


/**
 * @brief		This function to determine whether the flash is busy.
 * @return		1:Indicates that the flash is busy. 0:Indicates that the flash is free
 */
_attribute_ram_code_sec_ static inline unsigned char flash_is_busy(void)
{
	unsigned char status = 0;

	mspi_rx_cnt(1);
	mspi_set_address(0x0);
	reg_mspi_ctrl = (unsigned short)FLASH_READ_STATUS_CMD_LOWBYTE;    //set mspi format, dummy cnt and read mode

	unsigned char cipher_sta = reg_mspi_cipher_ctrl;
	reg_mspi_cipher_ctrl = 0;								//disable cipher mode
	mspi_set_cmd((FLASH_READ_STATUS_CMD_LOWBYTE>>16)&0xff);					//set command

	mspi_read((unsigned char *)(&status), 1);				//read data.
	reg_mspi_cipher_ctrl = cipher_sta;

	while (mspi_is_busy());

	return (status & BIT(0));
}

/**
 * @brief     This function serves to wait flash done.(make this a asynchorous version).
 * @return    none.
 */
_attribute_ram_code_sec_noinline_ static void flash_wait_done(void)
{
	int i;
	for(i = 0; i < 10000000; ++i){
		if(!flash_is_busy()){
			break;
		}
	}
}

/********************************************************************************************************
 *		It is necessary to add an evasion plan to solve the problem of access flash conflict.
 *******************************************************************************************************/
/**
 * @brief 		This function reads the content from a page to the buf.
 * @param[in]	cmd		  - the data fmt and cmd.
 * @param[in]   addr	  - the start address of the page.
 * @param[out]  data	  - the start address of the buffer.
 * @param[in]   data_len  - the length(in byte) of content needs to read out from the page.
 * @return 		none.
 * @note       _always_inline:the function is inlined to ensure security.
 */
static _always_inline void flash_mspi_read( flash_command_e cmd,unsigned long addr, unsigned char *data,unsigned long data_len){
	reg_mspi_ctrl = (unsigned short)cmd;				//set mspi format, dummy cnt and read mode

	mspi_rx_cnt(data_len);								//set read length

	mspi_set_address(addr);								//set read address

	mspi_set_cmd(((cmd>>16)&0xff));     				//set write/read command,write cmd is the trigger signal of the mspi time sequence

	mspi_read(data, data_len);						    //read data/status
}

/**
 * @brief 		This function serves to write data to flash(include erase,write status).
 * @param[in]   cmd     - the flash cmd and mspi control.
 * @param[in]   addr	- the start address you want to write, use 0 if do not need address.
 * @param[in]   data     - the buffer which stored the data you want to write to flash.
 * @param[in]   data_len     - the byte length you want to write.
 * @return 		none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void flash_mspi_write(flash_command_e cmd,unsigned long addr, unsigned char *data, unsigned long data_len)
{
	flash_send_cmd(FLASH_WRITE_ENABLE_CMD);           	//write enable

	reg_mspi_ctrl = (unsigned short)cmd;				//set mspi format, dummy cnt and read mode

	mspi_tx_cnt(data_len);									//set write length

	mspi_set_address(addr);								//set write address

	mspi_set_cmd(((cmd>>16)&0xff));						//set command,  write cmd is the trigger signal of the mspi time sequence

	mspi_write((unsigned char *)data, data_len);				//write data/status

	flash_wait_done();							//wait flash done
}

/**
 * @brief 		This function reads or write the content from a page to the buf.
 * @param[in]	cmd		       - the data fmt and cmd.
 * @param[in]   addr	       - the start address of the page.
 * @param[out]  data	       - the start address of the buffer.
 * @param[in]   data_len       - the length(in byte) of content needs to read out from the page.
 * @param[in]   is_is_encrypt  - whether to encrypt or decrypt.
 * @param[in]   mspi_wr        - Reading and writing to choose.
 * @return 		none.
 * @note       _always_inline : make it harder to crack encrypted data, so this interface is not allowed to be modified.
 */
static _always_inline void flash_mspi_wr_ram( flash_command_e cmd,unsigned long addr, unsigned char *data,unsigned long data_len,unsigned char is_encrypt,mspi_func_e mspi_wr){
	unsigned char cipher_sta=0;

	unsigned int r = plic_enter_critical_sec(s_flash_preempt_config.preempt_en,s_flash_preempt_config.threshold);

	mspi_stop_xip();

	cipher_sta = reg_mspi_cipher_ctrl;

	if(is_encrypt ==1){
		if(mspi_wr == MSPI_READ){
		    mspi_cipher_read_en();
		    flash_mspi_read(cmd,addr,data,data_len);
		}else if(mspi_wr == MSPI_WRITE){
			mspi_cipher_write_en();
			flash_mspi_write(cmd,addr,data,data_len);
		}
	}else if(is_encrypt ==0){
		if(mspi_wr == MSPI_READ){
			mspi_cipher_read_dis();
			flash_mspi_read(cmd,addr,data,data_len);
		}else if(mspi_wr == MSPI_WRITE){
			mspi_cipher_write_dis();
			flash_mspi_write(cmd,addr,data,data_len);
		}
	}

	reg_mspi_cipher_ctrl = cipher_sta;

	CLOCK_DLY_5_CYC;

	mspi_set_xip_en();

	plic_exit_critical_sec(s_flash_preempt_config.preempt_en,r);

}



/**
 * @brief 		This function reads the content from a page to the buf.
 * @param[in]	cmd		  - the data fmt and cmd.
 * @param[in]   addr	  - the start address of the page.
 * @param[out]  data	  - the start address of the buffer.
 * @param[in]   data_len  - the length(in byte) of content needs to read out from the page.
 * @return 		none.
 */
_attribute_ram_code_sec_noinline_ void flash_mspi_read_ram( flash_command_e cmd,unsigned long addr, unsigned char *data,unsigned long data_len)
{
	flash_mspi_wr_ram(cmd,addr,data,data_len,0,MSPI_READ);
}


/**
 * @brief 		This function reads the content from a page to the buf in in decrypt mode.
 * @param[in]	cmd		  - the data fmt and cmd.
 * @param[in]   addr	  - the start address of the page.
 * @param[out]  data	  - the start address of the buffer.
 * @param[in]   data_len  - the length(in byte) of content needs to read out from the page.
 * @return 		none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void flash_mspi_read_decrypt(flash_command_e cmd,unsigned long addr, unsigned char *data,unsigned long data_len )
{
	flash_mspi_wr_ram(cmd,addr,data,data_len,1,MSPI_READ);
}


/**
 * @brief 		This function serves to write write data to flash(include erase,write status).
 * @param[in]   cmd     - the flash cmd and mspi control.
 * @param[in]   addr	- the start address you want to write, use 0 if do not need address.
 * @param[in]   data     - the buffer which stored the data you want to write to flash.
 * @param[in]   data_len     - the byte length you want to write.
 * @return 		none.
 */
_attribute_ram_code_sec_noinline_ void flash_mspi_write_ram(flash_command_e cmd,unsigned long addr, unsigned char *data, unsigned long data_len)
{
	flash_mspi_wr_ram(cmd,addr,data,data_len,0,MSPI_WRITE);
}

/**
 * @brief 		This function serves to write write data to flash(include erase,write status) in encrypt mode.
 * @param[in]   cmd     - the flash cmd and mspi control.
 * @param[in]   addr	- the start address you want to write, use 0 if do not need address.
 * @param[in]   data     - the buffer which stored the data you want to write to flash.
 * @param[in]   data_len     - the byte length you want to write.
 * @return 		none.
 */
_attribute_ram_code_sec_noinline_ void flash_mspi_write_encrypt_ram(flash_command_e cmd,unsigned long addr, unsigned char *data, unsigned long data_len)
{
	flash_mspi_wr_ram(cmd,addr,data,data_len,1,MSPI_WRITE);
}
/**
 * @brief 		This function serves to decrypt the read data from the flash at the specified address and compare it with the plain text in dual read mode.
 * @param[in]	cmd		  - the data fmt and cmd.
 * @param[in]   addr	  - the start address of the page.
 * @param[out]  data	  - the start address of the plain buffer.
 * @param[in]   data_len  - the length(in byte) of content needs to read out from the page.
 * @return 		0: check pass; 1: check fail.
 * 				Attention: Before calling the FLASH function, please check the power supply voltage of the chip.
 *              Only if the detected voltage is greater than the safe voltage value, the FLASH function can be called.
 *              Taking into account the factors such as power supply fluctuations, the safe voltage value needs to be greater
 *              than the minimum chip operating voltage. For the specific value, please make a reasonable setting according
 *              to the specific application and hardware circuit.
 *
 *              Risk description: When the chip power supply voltage is relatively low, due to the unstable power supply,
 *              there may be a risk of error in the operation of the flash (especially for the write and erase operations.
 *              If an abnormality occurs, the firmware and user data may be rewritten, resulting in the final Product failure)
 * @note        the purpose the interface is all in ramcode : make it harder to crack encrypted data, so this interface is not allowed to be modified.
 */
_attribute_ram_code_sec_noinline_ unsigned char flash_mspi_read_decrypt_check_ram(flash_command_e cmd,unsigned long addr, unsigned char *data,unsigned long data_len ){
	unsigned short decrypt_buf_len = 256;
	unsigned char decrypt_buf[256];
	unsigned int nw=0;
	unsigned int ns=0;
	ns = data_len%decrypt_buf_len;
	do{
		nw = data_len >= decrypt_buf_len ? decrypt_buf_len :ns;
		flash_mspi_read_decrypt(cmd,addr,  decrypt_buf,nw);
		for(unsigned long len=0;len<nw;len++){
			if(decrypt_buf[len]!= data[len]){
				return 1;
			}
		}
	   addr+=nw;
	   data+=nw;
	   data_len-=nw;

	}while(data_len>0);
	return 0;
}


