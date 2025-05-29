/********************************************************************************************************
 * @file    drbg_driver.c
 *
 * @brief   This is the source file for Telink RISC-V MCU
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "tl_common.h"
#include "drivers.h"

_attribute_ram_code_sec_noinline_
void cs_iv_setup(unsigned int* cs_iv)
{
    reg_drbg_cs_iv(0) = *(cs_iv + 0);
    reg_drbg_cs_iv(1) = *(cs_iv + 1);
    reg_drbg_cs_iv(2) = *(cs_iv + 2);
    reg_drbg_cs_iv(3) = *(cs_iv + 3);
}

_attribute_ram_code_sec_noinline_
void cs_pv_setup(unsigned int* cs_pv)
{
    reg_drbg_cs_pv(0) = *(cs_pv + 0);
    reg_drbg_cs_pv(1) = *(cs_pv + 1);
    reg_drbg_cs_pv(2) = *(cs_pv + 2);
    reg_drbg_cs_pv(3) = *(cs_pv + 3);
}

_attribute_ram_code_sec_noinline_
void cs_in_setup(unsigned int* cs_in)
{
    reg_drbg_cs_in(0) = *(cs_in + 0);
    reg_drbg_cs_in(1) = *(cs_in + 1);
}

_attribute_ram_code_sec_noinline_
void cs_drbg_core_trigger(void)
{
    reg_drbg_ctrl = FLD_DRBG_START_P;
    while((reg_drbg_status & FLD_DRBG_STATUS) != FLD_DRBG_NORMAL_DONE);
}

_attribute_ram_code_sec_noinline_
void cs_drbg_3c_trigger(void)
{
    reg_drbg_ctrl = FLD_DRBG_START_P;
    while(!(reg_drbg_irq_raw & FLD_CSA_3C_DONE_IRQ_RAW));
}

_attribute_ram_code_sec_noinline_
void cs_drbg_irq_clear(void)
{
    reg_drbg_irq_clr = FLD_ALL_IRQ_CLEAR;
}

_attribute_ram_code_sec_noinline_
void cs_h9_instantiation_trigger(void)
{
    reg_drbg_ctrl = FLD_DRBG_INIT_P;
    while((reg_drbg_status & FLD_DRBG_STATUS) != FLD_DRBG_NORMAL_DONE);
}

_attribute_ram_code_sec_noinline_
void cs_cs_drbg_start(void)
{
    reg_drbg_ctrl = FLD_FUNC_DRBG_START_P;
    while((reg_drbg_status & FLD_DRBG_STATUS) != FLD_DRBG_NORMAL_DONE);
}

_attribute_ram_code_sec_noinline_
void cs_working_status_clear(void)
{
    reg_drbg_ctrl = FLD_DRBG_STATUS_CLR_P;
}

_attribute_ram_code_sec_noinline_
void cs_kdrbg_load(unsigned int* kdrbg)
{
    *(kdrbg + 0) = reg_k_drbg(0);
    *(kdrbg + 1) = reg_k_drbg(1);
    *(kdrbg + 2) = reg_k_drbg(2);
    *(kdrbg + 3) = reg_k_drbg(3);
}

_attribute_ram_code_sec_noinline_
void cs_vdrbg_load(unsigned int* vdrbg)
{
    *(vdrbg + 0) = reg_v_drbg(0);
    *(vdrbg + 1) = reg_v_drbg(1);
    *(vdrbg + 2) = reg_v_drbg(2);
    *(vdrbg + 3) = reg_v_drbg(3);
}

_attribute_ram_code_sec_noinline_
void cs_kdrbg_setup(unsigned int* kdrbg)
{
    reg_k_drbg(0) = *(kdrbg + 0);
    reg_k_drbg(1) = *(kdrbg + 1);
    reg_k_drbg(2) = *(kdrbg + 2);
    reg_k_drbg(3) = *(kdrbg + 3);
}

_attribute_ram_code_sec_noinline_
void cs_vdrbg_setup(unsigned int* vdrbg)
{
    reg_v_drbg(0) = *(vdrbg + 0);
    reg_v_drbg(1) = *(vdrbg + 1);
    reg_v_drbg(2) = *(vdrbg + 2);
    reg_v_drbg(3) = *(vdrbg + 3);
}

_attribute_ram_code_sec_noinline_
void cs_step_cnt_setup(unsigned char stepCnt)
{
    reg_drbg_cs_step_cnt = stepCnt;
}

_attribute_ram_code_sec_noinline_
void cs_transaction_id_setup(unsigned char transactionId)
{
    reg_drbg_cs_transaction_id = transactionId;
}

_attribute_ram_code_sec_noinline_
void cs_transaction_cnt_setup(unsigned char transactionCnt)
{
    reg_drbg_cs_transaction_cnt = transactionCnt;
}

_attribute_ram_code_sec_noinline_
void cs_randombits_load(unsigned int* randomBits)
{
    *(randomBits + 0) = reg_cs_drbg(0);
    *(randomBits + 1) = reg_cs_drbg(1);
    *(randomBits + 2) = reg_cs_drbg(2);
    *(randomBits + 3) = reg_cs_drbg(3);
}

_attribute_ram_code_sec_noinline_
void cs_accesscode_load(unsigned int* reflector_accessaddr, unsigned int* initiator_accessaddr)
{
    *((unsigned int*)reflector_accessaddr) = reg_cs_reflector_addr;
    *((unsigned int*)initiator_accessaddr) = reg_cs_initiator_addr;
}

_attribute_ram_code_sec_noinline_
void cs_channel_num(unsigned char chnNum)
{
    reg_drbg_channel_num = chnNum;
}

_attribute_ram_code_sec_noinline_
void cs_channel_array_pointer_setup(unsigned short chnBuffer)
{
    reg_drbg_channel_array_ptr = chnBuffer;
}

_attribute_ram_code_sec_noinline_
void cs_mapped_channel_array_pointer_setup(unsigned short mappedChnBuffer)
{
    reg_drbg_mapped_channel_array_ptr = mappedChnBuffer;
}

_attribute_ram_code_sec_noinline_
void cs_randombits_pointer_setup(unsigned char* randombitsBuffer)
{
    reg_drbg_randombits_ptr = (unsigned short)(unsigned int)randombitsBuffer;
}

_attribute_ram_code_sec_noinline_
void cs_restore_drbg_randombyte_index(unsigned char randomByteIndex)
{
    reg_drbg_byte_index = randomByteIndex;
}

_attribute_ram_code_sec_noinline_
unsigned char cs_load_drbg_randombyte_index(void)
{
    return reg_drbg_byte_index;
}

_attribute_ram_code_sec_noinline_
void cs_hr1_in(unsigned char hr1In)
{
    reg_drbg_hr1_in = hr1In;
}

_attribute_ram_code_sec_noinline_
unsigned char cs_hr1_out(void)
{
    return reg_drbg_hr1_out;
}

_attribute_ram_code_sec_noinline_
void chn_cas_3c_ctrl(unsigned char* chm, unsigned char CSShapeSelection, unsigned char CSChannelJump, unsigned char CSNumRepetitions)
{
    reg_csa_3c_ctrl1 = CSChannelJump<<4 | CSNumRepetitions<<2 | CSShapeSelection<<1 | FLD_CS_CSA_3C_SELECTION;//1 => #3c
    for(unsigned int i=0; i<10; i++)
    {
        reg_csa_3c_chm(i) = chm[i];
    }
}

_attribute_ram_code_sec_noinline_
void chn_cas_3c_disable(void)
{
    reg_csa_3c_ctrl1 = 0;//0 => #3b
}

_attribute_ram_code_sec_noinline_
unsigned char cs_get_NonMode0ShuffledChannelArrayNum(void)
{
    return reg_csa_3c_param9;
}

_attribute_ram_code_sec_noinline_
void cs_nShapeIteration_setup(unsigned char nShapeIteration)
{
    reg_csa_3c_ctrl2 = (reg_csa_3c_ctrl2 & (~FLD_CS_NSHAPE_ITERATION))|nShapeIteration;
}
