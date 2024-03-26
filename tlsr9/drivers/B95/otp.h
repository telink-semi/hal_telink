/********************************************************************************************************
 * @file    otp.h
 *
 * @brief   This is the header file for B95
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
#ifndef OTP_H_
#define OTP_H_
#include "reg_include/register.h"
#include "compiler.h"

/* delay times in spec */
#define Tcs                          (15)       /* pwe=1 ---> enable,  Tcs > 10(us) */
#define Tpw	                         (22)       /* pwe high time,  20 < Tpw < 25(us) */
#define Tpwi                         (4)        /* pwe low time,   1 < Tpwi < 5(us) */
#define Tppr                         (10)       /* pporg=0 ---> pce=0,  5 < Tppr < 100(us) */
#define Tpps                         (5)        /* pprog=1 ---> pwe=1,  5 < Tpps < 20(us) */
#define Tpph                         (10)       /* pwe=0 ---> pprog=0,  5 < Tpph < 20(us) */
#define Tsas                         (5)        /* deep standby to active mode setup time, Tsas >2(us) */
#define Tms                          (1)        /* ptm  -->  pce = 1,   Tms>1(ns) */
#define Tmh                          (1)        /* ptm  -->  pce = 0,   Tmh>1(ns) */
#define Tcsp                         (20)       /* pce = 1 ---> pprog=1,  10 < Tcsp < 100(us) */
#define Tpls                         (15)       /* ldo setup time,Tpls > 10(us) */

typedef enum {
    OTP_PTM_READ                     = 0x00,
    OTP_PTM_PROG                     = 0x02,
    OTP_PTM_INIT_MARGIN_READ         = 0x01,
    OTP_PTM_PGM_MARGIN_READ          = 0x04,
}otp_ptm_type_e;

typedef enum {
    OTP_TIM_CONFIG_24M               = 0x00,
    OTP_TIM_CONFIG_48M               = 0x01,
    OTP_TIM_CONFIG_96M               = 0x02,
    OTP_TIM_CONFIG_192M              = 0x03,
    OTP_TIM_CONFIG_384M              = 0x04,
}otp_tim_config_e;

/**
 * @brief      This function serves to otp set active mode,if otp is in deep mode,need to operate on the otp,set active mode.
 * @param[in]  none
 * @return     none
 */
void otp_set_active_mode(void);

/**
 * @brief      This function serves to otp set deep standby mode,if code run in flash,otp is idle,can enter deep to save current.
 * @param[in]  none
 * @return     none
 */
void otp_set_deep_standby_mode(void);

/**
 * @brief      This function serves to set otp auto power up,
 *             before config auto power up, must config FLD_OTP_TIM_CFG to tell internal counter which pclk used.
 * @param[in]  none
 * @return     none
 */
void otp_auto_power_en(void);

/**
 * @brief      This function serves to read data from OTP memory,belong to otp normal read.
 *             otp has three kinds of read mode,in general,just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 */
void otp_read(unsigned int addr, unsigned int word_len, unsigned int *buff);

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
unsigned char otp_write(unsigned int addr, unsigned int word_len, unsigned int *buff);

/**
 * @brief      This function serves to read data from OTP memory,belong to otp pgm margin read.
 *             otp has three kinds of read mode,in general, just use OTP_READ normal read operation, when the execution of burning operation,
 *             need to use margin read(otp_pgm_margin_read,otp_initial_margin_read),check whether the write is successful.
 * @param[in]  addr - the otp address of the data,it has to be a multiple of 4,the otp memory that can access is from 0x0000-0x3FC,can't access other address.
 * @param[in]  len  - the length of the data,the unit is word(4 bytes).
 * @param[in]  buff - data buff.
 * @return     none
 */
void otp_pgm_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff);

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
void otp_initial_margin_read(unsigned int addr, unsigned int word_len, unsigned int *buff);

/**
 * @brief      This function serves to config clock frequency division when auto power up enable.
 * @param[in]  tim - time config
 * @return     none
 */
static inline void otp_set_clk(otp_tim_config_e tim)
{
    reg_otp_ctrl5 = (reg_otp_ctrl5 & ~(FLD_OTP_TIM_CFG)) | tim;
}

/**
 * @brief      This function serves to config pclk cnt to latch data when OTP PCLK rising, cnt * Tpclk > Tcd(50ns).
 * @param[in]  cnt - pclk cnt config
 * @return     none
 */
static inline void otp_set_capture_edge(unsigned char cnt)
{
    reg_otp_ctrl4 = ((reg_otp_ctrl4 & ~(FLD_OTP_CAP_EDGE)) | cnt);
}

/**
 * @brief      This function serves to select read mode as manual.
 * @param[in]  none
 * @return     none
 */
static inline void otp_manual_mode(void)
{
    reg_otp_ctrl3 |= FLD_OTP_MAN_MODE;
}

/**
 * @brief      This function serves to select read mode as auto.
 * @param[in]  none
 * @return     none
 */
static inline void otp_auto_mode(void)
{
    reg_otp_ctrl3 &= ~(FLD_OTP_MAN_MODE);
}

/**
 * @brief      This function serves to enable key lock, 0 can write 1, 1 can't write 0.
 * @param[in]  none
 * @return     none
 */
static inline void otp_key_lock(void)
{
    reg_otp_status |= FLD_OTP_KEYLOCK;
}

/**
 * @brief      This function serves to enable ecc.
 * @param[in]  none
 * @return     none
 */
static inline void otp_ecc_en(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_ECC_RDB);
}

/**
 * @brief      This function serves to disable ecc.
 * @param[in]  none
 * @return     none
 */
static inline void otp_ecc_dis(void)
{
    reg_otp_ctrl0 |= FLD_OTP_ECC_RDB;
}

/**
 * @brief      This function serves to enable row test.
 * @param[in]  none
 * @return     none
 */
static inline void otp_test_row_en(void)
{
    reg_otp_ctrl0 |= FLD_OTP_PTR;
    reg_otp_ctrl0 &= ~(FLD_OTP_PTC);
}

/**
 * @brief      This function serves to enable column test.
 * @param[in]  none
 * @return     none
 */
static inline void otp_test_column_en(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PTR);
    reg_otp_ctrl0 |= FLD_OTP_PTC;
}

/**
 * @brief      This function serves to disable test mode.
 * @param[in]  none
 * @return     none
 */
static inline void otp_test_dis(void)
{
    reg_otp_ctrl0 &= ~(FLD_OTP_PTR | FLD_OTP_PTC);
}

#endif /* OTP_H_ */
