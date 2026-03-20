/********************************************************************************************************
 * @file    analog.c
 *
 * @brief   This is the source file for tl323x
 *
 * @author  Driver Group
 * @date    2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/plic.h"
#include "lib/include/analog.h"
#include "compiler.h"
#include "lib/include/stimer.h"

/**
 * @brief      This function serves to analog register read by byte.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned char analog_read_reg8(unsigned int addr)
{
    unsigned int r = core_interrupt_disable();
    reg_ana_addr   = addr;
    reg_ana_len    = 0x1;
    reg_ana_ctrl   = FLD_ANA_CYC | ((addr & 0x00000300) >> 8);
    analog_wait();
    unsigned char data = reg_ana_data(0);
    core_restore_interrupt(r);
    return data;
}

/**
 * @brief      This function serves to analog register write by byte.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg8(unsigned int addr, unsigned char data)
{
    unsigned int r  = core_interrupt_disable();
    reg_ana_len     = 1;
    reg_ana_addr    = addr;
    reg_ana_data(0) = data;
    analog_wait_txbuf_no_empty();
    reg_ana_ctrl = (FLD_ANA_CYC | FLD_ANA_RW | ((addr & 0x00000300) >> 8));
    analog_wait();
    reg_ana_ctrl = 0x00;
    core_restore_interrupt(r);
}

/**
 * @brief      This function serves to analog register write by halfword.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg16(unsigned int addr, unsigned short data)
{
    unsigned int r      = core_interrupt_disable();
    reg_ana_len         = 2;
    reg_ana_addr        = addr;
    reg_ana_addr_data16 = data;
    analog_wait_txbuf_no_empty();
    reg_ana_ctrl = (FLD_ANA_CYC | FLD_ANA_RW | ((addr & 0x00000300) >> 8));
    analog_wait();
    reg_ana_ctrl = 0x00;
    core_restore_interrupt(r);
}

/**
 * @brief      This function serves to analog register read by halfword.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned short analog_read_reg16(unsigned int addr)
{
    unsigned int r = core_interrupt_disable();
    reg_ana_len    = 2;
    reg_ana_addr   = addr;
    reg_ana_ctrl   = FLD_ANA_CYC | ((addr & 0x00000300) >> 8);
    analog_wait();
    unsigned short data = reg_ana_addr_data16;
    core_restore_interrupt(r);
    return data;
}

/**
 * @brief      This function serves to analog register read by word.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned int analog_read_reg32(unsigned int addr)
{
    unsigned int r = core_interrupt_disable();
    reg_ana_len    = 4;
    reg_ana_addr   = addr;
    reg_ana_ctrl   = FLD_ANA_CYC | ((addr & 0x00000300) >> 8);
    analog_wait();
    unsigned int data = reg_ana_addr_data32;
    core_restore_interrupt(r);
    return data;
}

/**
 * @brief      This function serves to analog register write by word.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg32(unsigned int addr, unsigned int data)
{
    unsigned int r      = core_interrupt_disable();
    reg_ana_len         = 4;
    reg_ana_addr        = addr;
    reg_ana_addr_data32 = data;
    analog_wait_txbuf_no_empty();
    reg_ana_ctrl = (FLD_ANA_CYC | FLD_ANA_RW | ((addr & 0x00000300) >> 8));
    analog_wait();
    reg_ana_ctrl = 0x00;
    core_restore_interrupt(r);
}

/* Operate analog reg8 by defalut */
_attribute_data_retention_ analog_read_f analog_read = analog_read_reg8;
_attribute_data_retention_ analog_write_f analog_write = analog_write_reg8;
