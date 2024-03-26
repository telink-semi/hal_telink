/********************************************************************************************************
 * @file    otp_reg.h
 *
 * @brief   This is the header file for B95
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef OTP_REG_H_
#define OTP_REG_H_
#include "soc.h"

/******************************* otp registers: 0x140300 ******************************/

#define OTP_BASE_ADDR        0x140300

#define reg_otp_ctrl0        REG_ADDR8(OTP_BASE_ADDR + 0x0)
enum {
    FLD_OTP_PCE              = BIT(1),
    FLD_OTP_PPROG            = BIT(2),
    FLD_OTP_PWE              = BIT(3), 
    FLD_OTP_PAS              = BIT(4),
    FLD_OTP_PTR              = BIT(5),
    FLD_OTP_PTC              = BIT(6),
    FLD_OTP_ECC_RDB          = BIT(7),
};

#define reg_otp_ctrl1        REG_ADDR8(OTP_BASE_ADDR + 0x01)
enum {
    FLD_OTP_PTM              = BIT_RNG(0, 3),
    FLD_OTP_PLDO             = BIT(4),
    FLD_OTP_PDSTD            = BIT(5), 
};

#define reg_otp_ctrl2        REG_ADDR8(OTP_BASE_ADDR + 0x02)
enum {
    FLD_OTP_DATA_ECC         = BIT_RNG(0, 5),
};

#define reg_otp_ctrl3        REG_ADDR8(OTP_BASE_ADDR + 0x03)
enum {
    FLD_OTP_AUTO_INC         = BIT(4),
    FLD_OTP_MAN_MODE         = BIT(5),
    FLD_OTP_MAN_PCLK         = BIT(6),
};

#define reg_otp_pa           REG_ADDR16(OTP_BASE_ADDR + 0x04)

#define reg_otp_paio         REG_ADDR8(OTP_BASE_ADDR + 0x06)
enum {
    FLD_OTP_PAIO             = BIT_RNG(0, 5),
};


#define reg_otp_status       REG_ADDR8(OTP_BASE_ADDR + 0x07)
enum {
    FLD_OTP_BUSY             = BIT(0),
    FLD_OTP_AUTO_EN_BUSY     = BIT(1),
    FLD_OTP_KEYLOCK          = BIT(2),
};

#define reg_otp_wr_dat       REG_ADDR32(OTP_BASE_ADDR + 0x08)

#define reg_otp_rd_dat       REG_ADDR32(OTP_BASE_ADDR + 0x0c)

#define reg_otp_ctrl4        REG_ADDR8(OTP_BASE_ADDR + 0x10)
enum {
    FLD_OTP_CAP_EDGE         = BIT_RNG(0, 4),
};

#define reg_otp_ctrl5        REG_ADDR8(OTP_BASE_ADDR + 0x11)
enum {
    FLD_OTP_TIM_CFG          = BIT_RNG(0, 2),
};

#define reg_otp_auto_load_dat REG_ADDR32(OTP_BASE_ADDR + 0x14)

#endif /* OTP_REG_H_ */
