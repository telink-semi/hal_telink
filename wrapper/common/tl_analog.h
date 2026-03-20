/********************************************************************************************************
 * @file    analog.h
 *
 * @brief   This is the header file for tl323x
 *
 * @author  Driver Group
 * @date    2025
 *
 * @par     Copyright (c) 2025, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
/*******************************      analog control registers: 0xb8      ******************************/
/** @page ANALOG
 *
 *  Introduction
 *  ===============
 *  analog support dma and normal mode, in each mode, support byte/halfword/word/buffer write and read.
 *  When reading and writing analog registers in DMA mode, exit interface after configuration.
 *  But the actual operation of the analog register is not finished, and the DMA is still moving the data.
 *  An interrupt may be opened at this time, and if there is an operation on the analog register,
 *  it will interrupt the previous DMA reading and writing the analog register, creating an unknown risk.
 *  Therefore, it is not recommended to use DMA to read and write analog registers.
 *
 *  API Reference
 *  ===============
 *  Header File: analog.h
 */
#pragma once


#include "reg_include/register.h"
#include "compiler.h"
#include "lib/include/core.h"
#include "error_handler/error_handler.h"

/**
 * @brief      This function serves to analog register read by byte.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned char analog_read_reg8(unsigned int addr);

/**
 * @brief      This function serves to analog register write by byte.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg8(unsigned int addr, unsigned char data);

/**
 * @brief      This function serves to analog register read by halfword.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned short analog_read_reg16(unsigned int addr);

/**
 * @brief      This function serves to analog register write by halfword.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg16(unsigned int addr, unsigned short data);

/**
 * @brief      This function serves to analog register read by word.
 * @param[in]  addr - address need to be read.
 * @return     the result of read.
 */
_attribute_ram_code_sec_noinline_ unsigned int analog_read_reg32(unsigned int addr);

/**
 * @brief      This function serves to analog register write by word.
 * @param[in]  addr - address need to be write.
 * @param[in]  data - the value need to be write.
 * @return     none.
 */
_attribute_ram_code_sec_noinline_ void analog_write_reg32(unsigned int addr, unsigned int data);

/* Operate analog reg8 by defalut */
typedef unsigned char (*analog_read_f)(unsigned char addr);
typedef void (*analog_write_f)(unsigned char addr, unsigned char data);

extern _attribute_data_retention_ analog_read_f analog_read;
extern _attribute_data_retention_ analog_write_f analog_write;
