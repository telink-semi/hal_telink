/********************************************************************************************************
 * @file    flash_base.c
 *
 * @brief   This is the source file for TL721X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
/********************************************************************************************************
 *                              Functions for internal use in flash,
 *      There is no need to add an evasion solution to solve the problem of access flash conflicts.
 *******************************************************************************************************/
#include "lib/include/mspi.h"
#include "lib/include/plic.h"
#include "flash.h"
#include "reg_include/mspi_reg.h"
#include "lib/include/flash_base.h"


volatile unsigned int g_r = 0;

#define reg_mspi_cipher_ctrl REG_ADDR8(MSPI_BASE_ADDR + 0x85)

enum
{
    FLD_MSPI_CIPHER_RD_EN = BIT(0),
    FLD_MSPI_CIPHER_WR_EN = BIT(1),
};

extern _attribute_data_retention_sec_ preempt_config_t s_flash_preempt_config;

/*******************************************************************************************************************
 *                              Functions for internal use in flash,
 *      There is no need to add an evasion solution to solve the problem of access flash conflicts.
 ******************************************************************************************************************/

/**
 * @brief       This function serves to enable cipher read.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_read_en(void)
{
    reg_mspi_cipher_ctrl |= FLD_MSPI_CIPHER_RD_EN;
}

/**
 * @brief       This function serves to disable cipher read.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_read_dis(void)
{
    reg_mspi_cipher_ctrl &= ~FLD_MSPI_CIPHER_RD_EN;
}

/**
 * @brief       This function serves to enable cipher write.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_write_en(void)
{
    reg_mspi_cipher_ctrl |= FLD_MSPI_CIPHER_WR_EN;
}

/**
 * @brief       This function serves to disable cipher write.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void mspi_cipher_write_dis(void)
{
    reg_mspi_cipher_ctrl &= ~FLD_MSPI_CIPHER_WR_EN;
}

/**
 * @brief       This function serves to the interrupt protection of flash interface.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
_attribute_ram_code_sec_ _always_inline void flash_reg_access_protect(void)
{
    g_r = plic_enter_critical_sec(s_flash_preempt_config.preempt_en, s_flash_preempt_config.threshold);
    mspi_stop_xip();
}

/**
 * @brief       This function serves to the interrupt restore of flash interface.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
_attribute_ram_code_sec_ _always_inline void flash_reg_access_restore(void)
{
    CLOCK_DLY_5_CYC;
    mspi_set_xip_en();
    plic_exit_critical_sec(s_flash_preempt_config.preempt_en, g_r);
}

/**
 * @brief       This function serves to set flash write command.This function interface is only used internally by flash,
 *              and is currently included in the H file for compatibility with other SDKs. When using this interface,
 *              please ensure that you understand the precautions of flash before using it.
 * @param[in]   addr - slave base address. The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[in]   cmd - set command.
 * @return      none.
 */
_attribute_ram_code_sec_optimize_o2_noinline_ void flash_send_cmd(unsigned long addr, unsigned int cmd)
{
    mspi_set_address(addr);
    mspi_set_ctrl((unsigned short)cmd); //set mspi format, dummy cnt and read mode
    mspi_set_reg_ctrl0((cmd >> 16) & 0xff);
    mspi_set_cmd((cmd >> 24) & 0xff);   //set write/read command,write cmd is the trigger signal of the mspi time sequence
    mspi_wait();
}

/**
 * @brief       This function to determine whether the flash is busy.
 * @param[in]   addr - slave base address. The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[in]   cmd  - set command.
 * @return      1:Indicates that the flash is busy. 0:Indicates that the flash is free
 */
_attribute_ram_code_sec_ static _always_inline unsigned char flash_is_busy(unsigned long addr, unsigned int cmd)
{
    unsigned char status = 0;

    mspi_rx_cnt(1);
    mspi_set_address(addr);                   //set address
    unsigned char cipher_sta = reg_mspi_cipher_ctrl;
    reg_mspi_cipher_ctrl     = 0;             //disable cipher mode
    mspi_set_ctrl((unsigned short)cmd);       //set mspi format, dummy cnt and read mode
    mspi_set_reg_ctrl0((cmd >> 16) & 0xff);
    mspi_set_cmd((cmd >> 24) & 0xff);         //set write/read command,write cmd is the trigger signal of the mspi time sequence
    mspi_read((unsigned char *)(&status), 1); //read data.
    reg_mspi_cipher_ctrl = cipher_sta;
    mspi_wait();

    return (status & BIT(0));
}

/**
 * @brief     This function serves to wait flash done.(make this a asynchronous version).
 * @param[in] addr - slave base address. The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[in] cmd  - set command.
 * @return    none.
 */
_attribute_ram_code_sec_noinline_ static void flash_wait_done(unsigned long addr, unsigned int cmd)
{
    int i;
    for (i = 0; i < 10000000; ++i) {
        if (!flash_is_busy(addr, cmd)) {
            break;
        }
    }
}

/********************************************************************************************************
 *      It is necessary to add an evasion plan to solve the problem of access flash conflict.
 *******************************************************************************************************/
/**
 * @brief       This function reads the content from a page to the buf.
 * @param[in]   cmd     - the data fmt and cmd.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[out]  data    - the start address of the buffer.
 * @param[in]   data_len- the length(in byte) of content needs to read out from the page.
 * @return      none.
 * @note       _always_inline:the function is inlined to ensure security.
 */
static _always_inline void flash_mspi_read(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len)
{
    mspi_rx_cnt(data_len);              //set read length
    mspi_set_address(addr);             //set read address
    mspi_set_ctrl((unsigned short)cmd); //set mspi format, dummy cnt and read mode
    mspi_set_reg_ctrl0((cmd >> 16) & 0xff);
    mspi_set_cmd((cmd >> 24) & 0xff);   //set write/read command,write cmd is the trigger signal of the mspi time sequence
    mspi_read(data, data_len);          //read data/status
}

/**
 * @brief       This function serves to write data to flash(include erase,write status).
 * @param[in]   cmd     - the flash cmd and mspi control.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[in]   data    - the buffer which stored the data you want to write to flash.
 * @param[in]   data_len- the length(in byte, must be above 0) you want to write.
 * @param[in]   w_en_cmd- the flash write enable cmd.
 * @param[in]   busy_cmd- the flash read status cmd.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void flash_mspi_write(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len, unsigned int w_en_cmd, unsigned int busy_cmd)
{
    //  This interface is compatible with writing to PSRAM.
    //  PSRAM does not need to send write enable commands and wait for write operations to complete.You can set w_en_cmd and busy_cmd to 0.
    if (w_en_cmd != 0x00) {
        flash_send_cmd(addr, w_en_cmd);          //write enable
    }

    mspi_tx_cnt(data_len);                       //set write length
    mspi_set_address(addr);                      //set write address
    mspi_set_ctrl((unsigned short)cmd);          //set mspi format, dummy cnt and read mode
    mspi_set_reg_ctrl0((cmd >> 16) & 0xff);
    mspi_set_cmd((cmd >> 24) & 0xff);            //set write/read command,write cmd is the trigger signal of the mspi time sequence
    mspi_write((unsigned char *)data, data_len); //write data/status

    if (busy_cmd != 0x00) {
        flash_wait_done(addr, busy_cmd);         //wait flash done
    }
}

/**
 * @brief       This function reads or write the content from a page to the buf.
 * @param[in]   cmd         - the data fmt and cmd.
 * @param[in]   addr        - slave base address + the access address of flash.The base address of device0 is 0.
 *                          If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[out]  data        - the start address of the buffer.
 * @param[in]   data_len    - the length(in byte, must be above 0) of content needs to read out from the page.
 * @param[in]   is_is_encrypt   - whether to encrypt or decrypt.
 * @param[in]   mspi_wr     - Reading and writing to choose.
 * @param[in]   w_en_cmd    - the flash write enable cmd.
 * @param[in]   busy_cmd    - the flash read status cmd.
 * @return      none.
 * @note       _always_inline : make it harder to crack encrypted data, so this interface is not allowed to be modified.
 */
_attribute_ram_code_sec_ _always_inline void flash_mspi_wr_ram(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len, unsigned char is_encrypt, mspi_func_e mspi_wr, unsigned int w_en_cmd, unsigned int busy_cmd)
{
    unsigned char cipher_sta = 0;

    cipher_sta = reg_mspi_cipher_ctrl;

    if (is_encrypt == 1) {
        if (mspi_wr == MSPI_READ) {
            mspi_cipher_read_en();
            flash_mspi_read(cmd, addr, data, data_len);
        } else if (mspi_wr == MSPI_WRITE) {
            mspi_cipher_write_en();
            flash_mspi_write(cmd, addr, data, data_len, w_en_cmd, busy_cmd);
        }
    } else if (is_encrypt == 0) {
        if (mspi_wr == MSPI_READ) {
            mspi_cipher_read_dis();
            flash_mspi_read(cmd, addr, data, data_len);
        } else if (mspi_wr == MSPI_WRITE) {
            mspi_cipher_write_dis();
            flash_mspi_write(cmd, addr, data, data_len, w_en_cmd, busy_cmd);
        }
    }

    reg_mspi_cipher_ctrl = cipher_sta;
}

/**
 * @brief       This function reads the content from a page to the buf.
 * @param[in]   cmd     - the data fmt and cmd.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[out]  data    - the start address of the buffer.
 * @param[in]   data_len- the length(in byte) of content needs to read out from the page.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void flash_mspi_read_ram(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len)
{
    flash_reg_access_protect();
    flash_mspi_wr_ram(cmd, addr, data, data_len, 0, MSPI_READ, 0, 0);
    flash_reg_access_restore();
}

/**
 * @brief       This function reads the content from a page to the buf in in decrypt mode.
 * @param[in]   cmd     - the data fmt and cmd.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[out]  data    - the start address of the buffer.
 * @param[in]   data_len- the length(in byte) of content needs to read out from the page.
 * @return      none.
 * @note        _always_inline:the function is inlined to ensure security.
 */
static _always_inline void flash_mspi_read_decrypt(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len)
{
    flash_reg_access_protect();
    flash_mspi_wr_ram(cmd, addr, data, data_len, 1, MSPI_READ, 0, 0);
    flash_reg_access_restore();
}

/**
 * @brief       This function serves to write write data to flash(include erase,write status).
 * @param[in]   cmd     - the flash cmd and mspi control.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[in]   data    - the buffer which stored the data you want to write to flash.
 * @param[in]   data_len- the byte length you want to write.
 * @param[in]   w_en_cmd- the flash write enable cmd.
 * @param[in]   busy_cmd- the flash read status cmd.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void flash_mspi_write_ram(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len, unsigned int w_en_cmd, unsigned int busy_cmd)
{
    flash_reg_access_protect();
    flash_mspi_wr_ram(cmd, addr, data, data_len, 0, MSPI_WRITE, w_en_cmd, busy_cmd);
    flash_reg_access_restore();
}

/**
 * @brief       This function serves to write write data to flash(include erase,write status) in encrypt mode.
 * @param[in]   cmd     - the flash cmd and mspi control.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[in]   data    - the buffer which stored the data you want to write to flash.
 * @param[in]   data_len- the byte length you want to write.
 * @param[in]   w_en_cmd- the flash write enable cmd.
 * @param[in]   busy_cmd- the flash read status cmd.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void flash_mspi_write_encrypt_ram(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len, unsigned int w_en_cmd, unsigned int busy_cmd)
{
    flash_reg_access_protect();
    flash_mspi_wr_ram(cmd, addr, data, data_len, 1, MSPI_WRITE, w_en_cmd, busy_cmd);
    flash_reg_access_restore();
}

/**
 * @brief       This function serves to decrypt the read data from the flash at the specified address and compare it with the plain text in dual read mode.
 * @param[in]   cmd     - the data fmt and cmd.
 * @param[in]   addr    - slave base address + the access address of flash.The base address of device0 is 0.
 *                      If there are multiple devices, the base address of other devices is determined by the mspi_slave_device_addr_space_config() function.
 * @param[out]  data    - the start address of the plain buffer.
 * @param[in]   data_len- the length(in byte) of content needs to read out from the page.
 * @return      0: check pass; 1: check fail.
 * @note        the purpose the interface is all in ramcode : make it harder to crack encrypted data, so this interface is not allowed to be modified.
 */
_attribute_ram_code_sec_noinline_ unsigned char flash_mspi_read_decrypt_check_ram(unsigned int cmd, unsigned long addr, unsigned char *data, unsigned long data_len)
{
    unsigned short decrypt_buf_len = 256;
    unsigned char  decrypt_buf[256];
    unsigned int   nw = 0;
    unsigned int   ns = 0;

    ns = data_len % decrypt_buf_len;
    do {
        nw = data_len >= decrypt_buf_len ? decrypt_buf_len : ns;
        flash_mspi_read_decrypt(cmd, addr, decrypt_buf, nw);
        for (unsigned long len = 0; len < nw; len++) {
            if (decrypt_buf[len] != data[len]) {
                return 1;
            }
        }
        addr += nw;
        data += nw;
        data_len -= nw;

    } while (data_len > 0);
    return 0;
}

/**
 * @brief       This function is used to update the read configuration parameters of xip(eXecute In Place),
 *              this configuration will affect the speed of MCU fetching,
 *              this parameter needs to be consistent with the corresponding parameters in the flash datasheet.
 * @param[in]   device_num  - the number of slave device.
 * @param[in]   config  - xip configuration,reference structure flash_rd_xip_config_t
 * @return none
 */
_attribute_ram_code_sec_noinline_ void flash_set_rd_xip_config_sram(mspi_slave_device_num_e device_num, unsigned int config)
{
    flash_reg_access_protect();
    reg_mspi_xip_rd_config(device_num) = (*(unsigned int *)(&config));
    reg_mspi_xip_rd_cmd1(device_num)   = (*(unsigned int *)(&config)) >> 24;
    flash_reg_access_restore();
}

/**
 * @brief       This function is used to update the write configuration parameters of xip(eXecute In Place),
 *              this parameter needs to be consistent with the corresponding parameters in the flash datasheet.
 * @param[in]   device_num  - the number of slave device.
 * @param[in]   config  - xip configuration,reference structure flash_wr_xip_config_t
 * @return none
 */
_attribute_ram_code_sec_noinline_ void flash_set_wr_xip_config_sram(mspi_slave_device_num_e device_num, flash_wr_xip_config_t config)
{
    flash_reg_access_protect();
    reg_mspi_xip_wr_config(device_num) = (*(unsigned int *)(&config));
    reg_mspi_xip_wr_cmd1(device_num)   = (*(unsigned int *)(&config)) >> 24;
    flash_reg_access_restore();
}
