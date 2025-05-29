/********************************************************************************************************
 * @file    sar_adc_reg.h
 *
 * @brief   This is the header file for tl322x
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef SAR_ADC_REG_H
#define SAR_ADC_REG_H
#include "soc.h"

/*******************************      sar_adc0 registers: 0x101400      ******************************/
/*******************************      sar_adc1 registers: 0x104400      ******************************/

#define REG_SAR_ADC_BASE(i)         (0x101400 + 0x3000 * i)

#define reg_sar_adc_m_config(i)     REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x00)

#define reg_sar_adc_oversampling(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x01)

enum
{
    FLD_SAR_ADC_OVRSMPL_MAX_CNT = BIT_RNG(0, 3),
    FLD_SAR_ADC_OVRSMPL_EN      = BIT(4),
    FLD_SAR_ADC_PAD_AUTO_MUX_EN = BIT(5),
};

#define reg_sar_adc_tsamp1(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x03)

enum
{
    FLD_SAR_ADC_R_TSAMPM = BIT_RNG(0, 3),
    FLD_SAR_ADC_R_TSAMPL = BIT_RNG(4, 7),
};

#define reg_sar_adc_tsamp2(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x04)

enum
{
    FLD_SAR_ADC_R_TSAMPR = BIT_RNG(0, 3),
};

#define reg_sar_adc_vbat_div(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x05)

enum
{
    FLD_SAR_ADC_MISC_CHANNEL_VBAT_DIV = BIT_RNG(0, 1),
    FLD_SAR_ADC_L_CHANNEL_VBAT_DIV    = BIT_RNG(2, 3),
    FLD_SAR_ADC_R_CHANNEL_VBAT_DIV    = BIT_RNG(4, 5),
};

#define reg_sar_adc_m_channel_set_state(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x06)

enum
{
    FLD_SAR_ADC_R_MAX_S_M              = BIT_RNG(0, 3),
    FLD_SAR_ADC_M_CHANNEL_SEL_AI_SCALE = BIT_RNG(4, 5),
    FLD_SAR_ADC_M_CHANNEL_SEL_VREF     = BIT_RNG(6, 7),
};

#define reg_sar_adc_l_channel_set_state(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x07)

enum
{
    FLD_SAR_ADC_R_MAX_S_L              = BIT_RNG(0, 3),
    FLD_SAR_ADC_L_CHANNEL_SEL_AI_SCALE = BIT_RNG(4, 5),
    FLD_SAR_ADC_L_CHANNEL_SEL_VREF     = BIT_RNG(6, 7),
};

#define reg_sar_adc_r_channel_set_state(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x08)

enum
{
    FLD_SAR_ADC_R_MAX_S_R              = BIT_RNG(0, 3),
    FLD_SAR_ADC_R_CHANNEL_SEL_AI_SCALE = BIT_RNG(4, 5),
    FLD_SAR_ADC_R_CHANNEL_SEL_VREF     = BIT_RNG(6, 7),
};

#define reg_sar_adc_rng_set_state(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x09)

enum
{
    FLD_SAR_ADC_RNG_SET_CNT      = BIT_RNG(0, 3),
    FLD_SAR_ADC_RNG_SEL_AI_SCALE = BIT_RNG(4, 5),
    FLD_SAR_ADC_RNG_SEL_VREF     = BIT_RNG(6, 7),
};

#define reg_sar_adc_m_channel_capture_state(i)   REG_ADDR16(REG_SAR_ADC_BASE(i) + 0x0a)
#define reg_sar_adc_m_channel_capture_state_l(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x0a)

enum
{
    FLD_SAR_ADC_R_MAX_C_M_L = BIT_RNG(0, 7),
};

#define reg_sar_adc_m_channel_capture_state_h(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x0b)

enum
{
    FLD_SAR_ADC_R_MAX_C_M_H = BIT_RNG(0, 7),
};

#define reg_sar_adc_l_channel_capture_state(i)   REG_ADDR16(REG_SAR_ADC_BASE(i) + 0x0c)
#define reg_sar_adc_l_channel_capture_state_l(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x0c)

enum
{
    FLD_SAR_ADC_R_MAX_C_L_L = BIT_RNG(0, 7),
};

#define reg_sar_adc_l_channel_capture_state_h(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x0d)

enum
{
    FLD_SAR_ADC_R_MAX_C_L_H = BIT_RNG(0, 7),
};

#define reg_sar_adc_r_channel_capture_state(i)   REG_ADDR16(REG_SAR_ADC_BASE(i) + 0x0e)
#define reg_sar_adc_r_channel_capture_state_l(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x0e)

enum
{
    FLD_SAR_ADC_R_MAX_C_R_L = BIT_RNG(0, 7),
};

#define reg_sar_adc_r_channel_capture_state_h(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x0f)

enum
{
    FLD_SAR_ADC_R_MAX_C_R_H = BIT_RNG(0, 7),
};

#define reg_sar_adc_rng_cap_state(i)   REG_ADDR16(REG_SAR_ADC_BASE(i) + 0x0e)
#define reg_sar_adc_rng_cap_state_l(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x10)

enum
{
    FLD_SAR_ADC_RNG_CAP_CNT_L = BIT_RNG(0, 7),
};

#define reg_sar_adc_rng_cap_state_h(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x11)

enum
{
    FLD_SAR_ADC_RNG_CAP_CNT_H = BIT_RNG(0, 1),
};

#define reg_sar_adc_rng_config(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x13)

enum
{
    FLD_SAR_ADC_RNG_TSAMP    = BIT_RNG(0, 3),
    FLD_SAR_ADC_RNG_VBAT_DIV = BIT_RNG(4, 5),
};

#define reg_sar_adc_config0(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x28)

enum
{
    FLD_SAR_ADC_SCNT_MAX = BIT_RNG(4, 6),
};

#define reg_sar_adc_config1(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x29)

enum
{
    FLD_SAR_ADC_CLK_DIV  = BIT_RNG(0, 3),
    FLD_SAR_ADC_RNG_MODE = BIT(4),
    FLD_SAR_ADC_ADC_MODE = BIT(5),
};

#define reg_sar_adc_config2(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x2a)

enum
{
    FLD_SAR_ADC_M_CHANNEL_ENABLE        = BIT(0),
    FLD_SAR_ADC_L_CHANNEL_ENABLE        = BIT(1),
    FLD_SAR_ADC_R_CHANNEL_ENABLE        = BIT(2),
    FLD_SAR_ADC_RNG_ENABLE              = BIT(3),
    FLD_SAR_ADC_SAR_RX_DMA_ENABLE       = BIT(4),
    FLD_SAR_ADC_CLK_ENABLE              = BIT(5),
    FLD_SAR_ADC_SAR_RX_INTERRUPT_ENABLE = BIT(6),
    FLD_SAR_ADC_SAR_TRIG_MODE           = BIT(7),
};

#define reg_sar_adc_rxfifo_trig_num(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x2b)

enum
{
    FLD_SAR_ADC_RXFIFO_TRIG_NUM = BIT_RNG(0, 2),
    FLD_SAR_ADC_BUFCNT          = BIT_RNG(4, 7),
};

#define reg_sar_adc_rxfifo_data(i)  REG_ADDR32(REG_SAR_ADC_BASE(i) + 0x2c)
#define reg_sar_adc_rxfifo_data0(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x2c)
#define reg_sar_adc_rxfifo_data1(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x2d)
#define reg_sar_adc_rxfifo_data2(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x2e)
#define reg_sar_adc_rxfifo_data3(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x2f)

#define reg_sar_adc_soft_control(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x30)

enum
{
    FLD_SAR_ADC_RX_STATUS  = BIT(0),
    FLD_SAR_ADC_FIFO_CLR   = BIT(1),
    FLD_SAR_ADC_TRIG_START = BIT(2),
    FLD_SAR_ADC_SOFT_START = BIT(7),
};

#define reg_sar_adc_sample_times(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x31)

#define reg_sar_adc_pem_enable(i)   REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x32)

enum
{
    FLD_SAR_ADC_PEM_TASK_ENABLE   = BIT(0),
    FLD_SAR_ADC_PEM_EVENT0_ENABLE = BIT(1),
    FLD_SAR_ADC_PEM_EVENT1_ENABLE = BIT(2),
};

#define reg_sar_adc_fifo_status(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x33)

enum
{
    FLD_SAR_ADC_FIFO_OVERRUN  = BIT(0),
    FLD_SAR_ADC_FIFO_UNDERRUN = BIT(1),
};

#define reg_sar_adc_r_muxm_pos_neg(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x34)

enum
{
    FLD_SAR_ADC_R_MUXM_POSITIVE_INPUT = BIT_RNG(0, 3),
    FLD_SAR_ADC_R_MUXM_NEGATIVE_INPUT = BIT_RNG(4, 7),
};

#define reg_sar_adc_r_muxl_pos_neg(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x35)

enum
{
    FLD_SAR_ADC_R_MUXL_POSITIVE_INPUT = BIT_RNG(0, 3),
    FLD_SAR_ADC_R_MUXL_NEGATIVE_INPUT = BIT_RNG(4, 7),
};

#define reg_sar_adc_r_muxr_pos_neg(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x36)

enum
{
    FLD_SAR_ADC_R_MUXR_POSITIVE_INPUT = BIT_RNG(0, 3),
    FLD_SAR_ADC_R_MUXR_NEGATIVE_INPUT = BIT_RNG(4, 7),
};

#define reg_sar_adc_rng_sel_ain_pos_neg(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x37)

enum
{
    FLD_SAR_ADC_RNG_SEL_AIN_P = BIT_RNG(0, 3),
    FLD_SAR_ADC_RNG_SEL_AIN_N = BIT_RNG(4, 7),
};

#define reg_sar_adc_pad_auto_p(i)   REG_ADDR32(REG_SAR_ADC_BASE(i) + 0x38)
#define reg_sar_adc_pad_auto_p_0(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x38)
#define reg_sar_adc_pad_auto_p_1(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x39)
#define reg_sar_adc_pad_auto_p_2(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x3a)
#define reg_sar_adc_pad_auto_p_3(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x3b)

#define reg_sar_adc_pad_auto_n(i)   REG_ADDR32(REG_SAR_ADC_BASE(i) + 0x3c)
#define reg_sar_adc_pad_auto_n_0(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x3c)
#define reg_sar_adc_pad_auto_n_1(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x3d)
#define reg_sar_adc_pad_auto_n_2(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x3e)
#define reg_sar_adc_pad_auto_n_3(i) REG_ADDR8(REG_SAR_ADC_BASE(i) + 0x3f)
#endif
