/********************************************************************************************************
 * @file    otp.c
 *
 * @brief   This is the source file for B95
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#include "otp.h"
#include "stimer.h"

/**
 * @brief      This function serves to otp set active mode,if otp is in deep mode,need to operate on the otp,set active mode.
 * @param[in]  none
 * @return     none
 */
void otp_set_active_mode(void)
{
    reg_otp_ctrl1 |= FLD_OTP_PLDO; /* pldo = 1 */
    delay_us(Tpls);
    reg_otp_ctrl1 |= FLD_OTP_PDSTD; /* pdstd = 1 */
    delay_us(Tsas);
}

/**
 * @brief      This function serves to otp set deep standby mode,if code run in flash,otp is idle,can enter deep to save current.
 * @param[in]  none
 * @return     none
 */
void otp_set_deep_standby_mode(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE); /* pce=0 */
    /* Tash >= 0(ns) */
    reg_otp_ctrl1 &= ~(FLD_OTP_PLDO | FLD_OTP_PDSTD); /* pdstb=0  pldo=0  Tplh >= 0(ns) */
}

/**
 * @brief      This function serves to wait until the operation of OTP is done.
 * @param[in]  none
 * @return     none
 */
static inline void otp_wait_done(void)
{
    while (reg_otp_status & FLD_OTP_BUSY);
}

/**
 * @brief      This function serves to set otp auto power up,
 *             before config auto power up, must config FLD_OTP_TIM_CFG to tell internal counter which pclk used.
 * @param[in]  none
 * @return     none
 */
void otp_auto_power_en(void)
{
    reg_otp_status |= FLD_OTP_AUTO_EN_BUSY;
    while (reg_otp_status & FLD_OTP_AUTO_EN_BUSY);
}

/**
 * @brief     This function is a common sequence used by these interfaces:otp_write32/otp_read_cycle_auto/otp_read_cycle_manual.
 * @param[in] ptm_mode - ptm type.
 * @return    none
 */
static void otp_start(otp_ptm_type_e ptm_mode)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE);
    reg_otp_ctrl1 = ((reg_otp_ctrl1 & ~(FLD_OTP_PTM)) | ptm_mode); /* ptm mode */
    /* Tms >= 1(ns) */
    reg_otp_ctrl0 |= (FLD_OTP_PCE); /* pce = 1*/
}

/**
 * @brief      This function serves to auto read data from OTP memory.
 * @param[in]  ptm_mode - read mode.
 * @param[in]  addr - the address of the data,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ static void otp_read_cycle_auto(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    delay_us(Tcs);

    reg_otp_pa = addr;
    buff[0] = reg_otp_rd_dat; /* trigger read */
    otp_wait_done();

    /* pa auto inc */
    for (unsigned int i = 0; i < word_len; i++)
    {
        buff[i] = reg_otp_rd_dat;
        otp_wait_done();
    }

    reg_otp_ctrl0 &= ~(FLD_OTP_PCE); /* pce = 0*/
}

/**
 * @brief      This function serves to manual read data from OTP memory.
 * @param[in]  ptm_mode - read mode.
 * @param[in]  addr - the address of the data,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ static void otp_read_cycle_manual(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    delay_us(Tcs);

    for (unsigned int i = 0; i < word_len; i++)
    {
        reg_otp_pa = addr + i * 4;
        /* Tas >= 1(ns) */
        reg_otp_ctrl3 |= FLD_OTP_MAN_PCLK;
        /* Tkh >= 20(ns) */
        for (int j = 0; j < 4; j++)
        {
            __asm__("nop");
        }
        reg_otp_ctrl3 &= ~(FLD_OTP_MAN_PCLK);
        /* Tkl >= 20(ns) */
        for (int j = 0; j < 4; j++)
        {
            __asm__("nop");
        }
        buff[i] = reg_otp_rd_dat;
        otp_wait_done();
    }

    reg_otp_ctrl0 &= ~(FLD_OTP_PCE);
}

/**
 * @brief      This function serves to write data to OTP memory,4 bytes one time.
 * @param[in]  ptm_mode - write mode.
 * @param[in]  addr  - the address of the data,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  data  - the data need to be write,4 bytes.
 * @return     none
 */
_attribute_ram_code_sec_noinline_ static void otp_write32(otp_ptm_type_e ptm_mode, unsigned int addr, unsigned int data)
{
    /* when write to otp, ptm needs to be configured. */
    otp_start(ptm_mode);
    /* Tcsp */
    delay_us(Tcsp);

    /* prog pas addr data */
    reg_otp_ctrl0 |= FLD_OTP_PPROG;
    reg_otp_ctrl0 |= FLD_OTP_PAS;
    reg_otp_pa = addr;
    reg_otp_wr_dat = data;
    reg_otp_paio = 0;
    /* Tpps */
    delay_us(Tpps);

    /* redundancy programming  38*2 */
    for (unsigned char i = 1; i <= 76; i++)
    {
        reg_otp_ctrl0 |= FLD_OTP_PWE;
        delay_us(Tpw);
        reg_otp_ctrl0 &= ~(FLD_OTP_PWE);
        if (i < 38)
        {
            reg_otp_paio = i;
        }
        else if (i == 38)
        {
            reg_otp_ctrl0 &= (~FLD_OTP_PAS);
            reg_otp_pa = addr;
            reg_otp_wr_dat = data;
            reg_otp_paio = 0;
        }
        else if ((i > 38) && (i < 76))
        {
            reg_otp_paio = i - 38;
        }
        else if (i == 76)
        {
            break;
        }
        // because the for loop and the if judge the time,choose to use Tpwi/2.
        delay_us(Tpwi / 2);
    }
    delay_us(Tpph);
    reg_otp_ctrl0 &= ~(FLD_OTP_PPROG); /* pporg = 0 */
    delay_us(Tppr);
    reg_otp_ctrl0 &= ~(FLD_OTP_PCE); /* pce = 0 */
    /* Tmh >= 1(ns) */
    reg_otp_ctrl1 = ~(FLD_OTP_PTM);
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp normal read.
 *             otp has three kinds of read mode,in general,just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 */
void otp_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    if (reg_otp_ctrl3 & FLD_OTP_MAN_MODE)
    {
        otp_read_cycle_manual(OTP_PTM_READ, addr, word_len, buff);
    }
    else
    {
        otp_read_cycle_auto(OTP_PTM_READ, addr, word_len, buff);
    }
}

/**
 * @brief      This function serves to write data to OTP memory.
 *             the minimum unit of otp read-write operation is 4 bytes, that is a word. meanwhile, the otp cannot be burned repeatedly,
 *             this function is limited to writing only once,this function will determine if the otp is 0xffffffff, and if it is 0xffffffff,
 *             it will write the otp.
 * @param[in]  addr - the address of the data,it has to be a multiple of 4,the OTP memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  word_len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     0 :it means that the otp operation area is 0xffffffff or the write data,
 *                return 0 not mean that the burning was successful,need to use three kinds of read mode to check whether the writing was successful.
 *             1 :it means that there is an operation value in the operation area,it is not 0xffffffff or the write data,no burning action is performed.
 */
unsigned char otp_write(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    for (unsigned int i = 0; i < word_len; i++)
    {
        unsigned int temp = 0;
        otp_read(addr + i * 4, 1, (unsigned int *)&temp);
        if (temp == 0xffffffff)
        {
            otp_write32(OTP_PTM_PROG, addr + i * 4, buff[i]);
        }
        else if (temp != buff[i])
        {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp pgm margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 */
void otp_pgm_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    otp_read_cycle_auto(OTP_PTM_INIT_MARGIN_READ, addr, word_len, buff);
}

/**
 * @brief      This function serves to read data from OTP memory,belong to otp initial margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 *
 */
void otp_initial_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff)
{
    otp_read_cycle_auto(OTP_PTM_PGM_MARGIN_READ, addr, word_len, buff);
}
