/********************************************************************************************************
 * @file    audio.h
 *
 * @brief   This is the header file for TL751X
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
/** @page AUDIO
 *
 *  API Reference
 *  ===============
 *  Header File: audio.h
 */

#ifndef __AUDIO_H_
#define __AUDIO_H_

#include "compiler.h"
#include "reg_include/register.h"
#include "lib/include/pm/pm.h"
#include "gpio.h"
#include "stimer.h"
#include "dma.h"

/**********************************************************************************************************************
 *                                                Audio pin enum/struct                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio pin enum/struct.
 * @{
 */

/**
 * @brief DMIC data pin.
 * 
 */
typedef enum
{
    AUDIO_DMIC_CLK0_PA0 = GPIO_PA0,
    AUDIO_DMIC_CLK0_PA3 = GPIO_PA3,
    AUDIO_DMIC_CLK0_PA6 = GPIO_PA6,
    AUDIO_DMIC_CLK0_PB2 = GPIO_PB2,
    AUDIO_DMIC_CLK0_PB5 = GPIO_PB5,
    AUDIO_DMIC_CLK0_PD2 = GPIO_PD2,
    AUDIO_DMIC_CLK0_PD5 = GPIO_PD5,
    AUDIO_DMIC_CLK0_PF2 = GPIO_PF2,
    AUDIO_DMIC_CLK0_PF5 = GPIO_PF5,
    AUDIO_DMIC_CLK0_PG0 = GPIO_PG0,
    AUDIO_DMIC_CLK0_PG3 = GPIO_PG3,
    AUDIO_DMIC_CLK0_PG6 = GPIO_PG6,
    AUDIO_DMIC_CLK0_PH5 = GPIO_PH5,
    AUDIO_DMIC_CLK0_PJ2 = GPIO_PJ2,
    AUDIO_DMIC_CLK0_PJ5 = GPIO_PJ5,

    AUDIO_DMIC_CLK1_PA1 = GPIO_PA1,
    AUDIO_DMIC_CLK1_PA4 = GPIO_PA4,
    AUDIO_DMIC_CLK1_PB0 = GPIO_PB0,
    AUDIO_DMIC_CLK1_PB3 = GPIO_PB3,
    AUDIO_DMIC_CLK1_PB6 = GPIO_PB6,
    AUDIO_DMIC_CLK1_PD3 = GPIO_PD3,
    AUDIO_DMIC_CLK1_PD6 = GPIO_PD6,
    AUDIO_DMIC_CLK1_PF3 = GPIO_PF3,
    AUDIO_DMIC_CLK1_PF6 = GPIO_PF6,
    AUDIO_DMIC_CLK1_PG1 = GPIO_PG1,
    AUDIO_DMIC_CLK1_PG4 = GPIO_PG4,
    AUDIO_DMIC_CLK1_PG7 = GPIO_PG7,
    AUDIO_DMIC_CLK1_PH6 = GPIO_PH6,
    AUDIO_DMIC_CLK1_PJ3 = GPIO_PJ3,

    AUDIO_DMIC0_DATA_PA2 = GPIO_PA2,
    AUDIO_DMIC0_DATA_PA5 = GPIO_PA5,
    AUDIO_DMIC0_DATA_PB1 = GPIO_PB1,
    AUDIO_DMIC0_DATA_PB4 = GPIO_PB4,
    AUDIO_DMIC0_DATA_PB7 = GPIO_PB7,
    AUDIO_DMIC0_DATA_PD4 = GPIO_PD4,
    AUDIO_DMIC0_DATA_PD7 = GPIO_PD7,
    AUDIO_DMIC0_DATA_PF4 = GPIO_PF4,
    AUDIO_DMIC0_DATA_PF7 = GPIO_PF7,
    AUDIO_DMIC0_DATA_PG2 = GPIO_PG2,
    AUDIO_DMIC0_DATA_PG5 = GPIO_PG5,
    AUDIO_DMIC0_DATA_PH4 = GPIO_PH4,
    AUDIO_DMIC0_DATA_PJ4 = GPIO_PJ4,

    AUDIO_DMIC_NONE_PIN = GPIO_NONE_PIN,
} audio_dmic_pin_e;

/**
 * @brief I2S BCLK pin.
 * 
 */
typedef enum
{
    I2S_BCLK_PA0 = GPIO_PA0,
    I2S_BCLK_PA5 = GPIO_PA5,
    I2S_BCLK_PB3 = GPIO_PB3,
    I2S_BCLK_PD2 = GPIO_PD2,
    I2S_BCLK_PD7 = GPIO_PD7,
    I2S_BCLK_PF6 = GPIO_PF6,
    I2S_BCLK_PG3 = GPIO_PG3,
    I2S_BCLK_PH4 = GPIO_PH4,
    I2S_BCLK_PJ2 = GPIO_PJ2,

    I2S_BCLK_NONE_PIN = GPIO_NONE_PIN,
} i2s_bclk_pin_e;

/**
 * @brief I2S ADC LR_CLK pin.
 * 
 */
typedef enum
{
    I2S_ADC_LR_CLK_PA1 = GPIO_PA1,
    I2S_ADC_LR_CLK_PA6 = GPIO_PA6,
    I2S_ADC_LR_CLK_PB4 = GPIO_PB4,
    I2S_ADC_LR_CLK_PD3 = GPIO_PD3,
    I2S_ADC_LR_CLK_PF2 = GPIO_PF2,
    I2S_ADC_LR_CLK_PF7 = GPIO_PF7,
    I2S_ADC_LR_CLK_PG4 = GPIO_PG4,
    I2S_ADC_LR_CLK_PH5 = GPIO_PH5,
    I2S_ADC_LR_CLK_PJ3 = GPIO_PJ3,

    I2S_ADC_LR_CLK_NONE_PIN = GPIO_NONE_PIN,
} i2s_adc_lr_clk_pin_e;

/**
 * @brief I2S ADC data pin.
 * 
 */
typedef enum
{
    I2S_ADC_DAT_PA2 = GPIO_PA2,
    I2S_ADC_DAT_PB0 = GPIO_PB0,
    I2S_ADC_DAT_PB5 = GPIO_PB5,
    I2S_ADC_DAT_PD4 = GPIO_PD4,
    I2S_ADC_DAT_PF3 = GPIO_PF3,
    I2S_ADC_DAT_PG0 = GPIO_PG0,
    I2S_ADC_DAT_PG5 = GPIO_PG5,
    I2S_ADC_DAT_PH6 = GPIO_PH6,
    I2S_ADC_DAT_PJ4 = GPIO_PJ4,

    I2S_ADC_DAT_NONE_PIN = GPIO_NONE_PIN,
} i2s_adc_dat_pin_e;

/**
 * @brief I2S DAC LR_CLK pin.
 * 
 */
typedef enum
{
    I2S_DAC_LR_CLK_PA3 = GPIO_PA3,
    I2S_DAC_LR_CLK_PB1 = GPIO_PB1,
    I2S_DAC_LR_CLK_PB6 = GPIO_PB6,
    I2S_DAC_LR_CLK_PD5 = GPIO_PD5,
    I2S_DAC_LR_CLK_PF4 = GPIO_PF4,
    I2S_DAC_LR_CLK_PG1 = GPIO_PG1,
    I2S_DAC_LR_CLK_PG6 = GPIO_PG6,

    I2S_DAC_LR_NONE_PIN = GPIO_NONE_PIN,
} i2s_dac_lr_clk_pin_e;

/**
 * @brief I2S DAC data pin.
 * 
 */
typedef enum
{
    I2S_DAC_DAT_PA4 = GPIO_PA4,
    I2S_DAC_DAT_PB2 = GPIO_PB2,
    I2S_DAC_DAT_PB7 = GPIO_PB7,
    I2S_DAC_DAT_PD6 = GPIO_PD6,
    I2S_DAC_DAT_PF5 = GPIO_PF5,
    I2S_DAC_DAT_PG2 = GPIO_PG2,
    I2S_DAC_DAT_PG7 = GPIO_PG7,
    I2S_DAC_DAT_PJ5 = GPIO_PJ5,

    I2S_DAC_DAT_NONE_PIN = GPIO_NONE_PIN,
} i2s_dac_dat_pin_e;

/**
 * @brief I2S CLK pin.
 * 
 */
typedef enum
{
    I2S0_CLK_PA0 = GPIO_PA0,
    I2S0_CLK_PA3 = GPIO_PA3,
    I2S0_CLK_PA6 = GPIO_PA6,
    I2S0_CLK_PB2 = GPIO_PB2,
    I2S0_CLK_PB5 = GPIO_PB5,
    I2S0_CLK_PD2 = GPIO_PD2,
    I2S0_CLK_PD5 = GPIO_PD5,
    I2S0_CLK_PF2 = GPIO_PF2,
    I2S0_CLK_PF5 = GPIO_PF5,
    I2S0_CLK_PG0 = GPIO_PG0,
    I2S0_CLK_PG3 = GPIO_PG3,
    I2S0_CLK_PG6 = GPIO_PG6,
    I2S0_CLK_PH5 = GPIO_PH5,
    I2S0_CLK_PJ2 = GPIO_PJ2,
    I2S0_CLK_PJ5 = GPIO_PJ5,

    I2S1_CLK_PA1 = GPIO_PA1,
    I2S1_CLK_PA4 = GPIO_PA4,
    I2S1_CLK_PB0 = GPIO_PB0,
    I2S1_CLK_PB3 = GPIO_PB3,
    I2S1_CLK_PB6 = GPIO_PB6,
    I2S1_CLK_PD3 = GPIO_PD3,
    I2S1_CLK_PD6 = GPIO_PD6,
    I2S1_CLK_PF3 = GPIO_PF3,
    I2S1_CLK_PF6 = GPIO_PF6,
    I2S1_CLK_PG1 = GPIO_PG1,
    I2S1_CLK_PG4 = GPIO_PG4,
    I2S1_CLK_PG7 = GPIO_PG7,
    I2S1_CLK_PH6 = GPIO_PH6,
    I2S1_CLK_PJ3 = GPIO_PJ3,

    I2S2_CLK_PA2 = GPIO_PA2,
    I2S2_CLK_PA5 = GPIO_PA5,
    I2S2_CLK_PB1 = GPIO_PB1,
    I2S2_CLK_PB4 = GPIO_PB4,
    I2S2_CLK_PB7 = GPIO_PB7,
    I2S2_CLK_PD4 = GPIO_PD4,
    I2S2_CLK_PD7 = GPIO_PD7,
    I2S2_CLK_PF4 = GPIO_PF4,
    I2S2_CLK_PF7 = GPIO_PF7,
    I2S2_CLK_PG2 = GPIO_PG2,
    I2S2_CLK_PG5 = GPIO_PG5,
    I2S2_CLK_PH4 = GPIO_PH4,
    I2S2_CLK_PJ4 = GPIO_PJ4,

    I2S_CLK_NONE_PIN = GPIO_NONE_PIN,
} i2s_clk_pin_e;

/**
 * @brief SPDIF TX pin.
 * 
 */
typedef enum
{
    SPDIF_TX_PA0 = GPIO_PA0,
    SPDIF_TX_PA2 = GPIO_PA2,
    SPDIF_TX_PA4 = GPIO_PA4,
    SPDIF_TX_PA6 = GPIO_PA6,
    SPDIF_TX_PB1 = GPIO_PB1,
    SPDIF_TX_PB3 = GPIO_PB3,
    SPDIF_TX_PB5 = GPIO_PB5,
    SPDIF_TX_PB7 = GPIO_PB7,
    SPDIF_TX_PD3 = GPIO_PD3,
    SPDIF_TX_PD5 = GPIO_PD5,
    SPDIF_TX_PD7 = GPIO_PD7,
    SPDIF_TX_PF3 = GPIO_PF3,
    SPDIF_TX_PF5 = GPIO_PF5,
    SPDIF_TX_PF7 = GPIO_PF7,
    SPDIF_TX_PG1 = GPIO_PG1,
    SPDIF_TX_PG3 = GPIO_PG3,
    SPDIF_TX_PG5 = GPIO_PG5,
    SPDIF_TX_PG7 = GPIO_PG7,
    SPDIF_TX_PH5 = GPIO_PH5,
    SPDIF_TX_PJ2 = GPIO_PJ2,
    SPDIF_TX_PJ4 = GPIO_PJ4,

    SPDIF_TX_NONE_PIN = GPIO_NONE_PIN,
} spdif_tx_pin_e;

/**
 * @brief SPDIF RX pin.
 * 
 */
typedef enum
{
    SPDIF_RX_PA1 = GPIO_PA1,
    SPDIF_RX_PA3 = GPIO_PA3,
    SPDIF_RX_PA5 = GPIO_PA5,
    SPDIF_RX_PB0 = GPIO_PB0,
    SPDIF_RX_PB2 = GPIO_PB2,
    SPDIF_RX_PB4 = GPIO_PB4,
    SPDIF_RX_PB6 = GPIO_PB6,
    SPDIF_RX_PD2 = GPIO_PD2,
    SPDIF_RX_PD4 = GPIO_PD4,
    SPDIF_RX_PD6 = GPIO_PD6,
    SPDIF_RX_PF2 = GPIO_PF2,
    SPDIF_RX_PF4 = GPIO_PF4,
    SPDIF_RX_PF6 = GPIO_PF6,
    SPDIF_RX_PG0 = GPIO_PG0,
    SPDIF_RX_PG2 = GPIO_PG2,
    SPDIF_RX_PG4 = GPIO_PG4,
    SPDIF_RX_PG6 = GPIO_PG6,
    SPDIF_RX_PH4 = GPIO_PH4,
    SPDIF_RX_PH6 = GPIO_PH6,
    SPDIF_RX_PJ3 = GPIO_PJ3,
    SPDIF_RX_PJ5 = GPIO_PJ5,

    SPDIF_RX_NONE_PIN = GPIO_NONE_PIN,
} spdif_rx_pin_e;

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio anc enum/struct                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio anc enum/struct.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio asrc enum/struct                                              *
 *********************************************************************************************************************/
/*!
 * @name Audio asrc enum/struct.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio codec0 enum/struct                                             *
 *********************************************************************************************************************/
/*!
 * @name Audio codec0 enum/struct.
 * @{
 */

/**
 * @brief codec0 power on mode.
 * 
 */
typedef enum
{
    AUDIO_CODEC0_ADC_ONLY = 0x08,
    AUDIO_CODEC0_DAC_ONLY = 0x04,
    AUDIO_CODEC0_ADC_AND_DAC = 0x00,
} audio_codec0_power_e;

/**
 * @brief codec0 input select.
 *
 */
typedef enum
{
    AUDIO_DMIC_ADC_A1 = 0x00, /**< bit[0-3] channel. */
    AUDIO_DMIC_ADC_A2 = 0x01,
    AUDIO_DMIC_ADC_B1 = 0x02,
    AUDIO_DMIC_ADC_B2 = 0x03,
    AUDIO_DMIC_ADC_A1_A2 = 0x04,
    AUDIO_DMIC_ADC_B1_B2 = 0x05,

    AUDIO_LINEIN_ADC_A1 = 0x00 | BIT(4), /**< bit4 line_in. */
    AUDIO_LINEIN_ADC_A2 = 0x01 | BIT(4),
    AUDIO_LINEIN_ADC_B1 = 0x02 | BIT(4),
    AUDIO_LINEIN_ADC_A1_A2 = 0x04 | BIT(4),

    AUDIO_AMIC_ADC_A1 = 0x00 | BIT(5), /**< bit5: amic. */
    AUDIO_AMIC_ADC_A2 = 0x01 | BIT(5),
    AUDIO_AMIC_ADC_B1 = 0x02 | BIT(5),
    AUDIO_AMIC_ADC_A1_A2 = 0x04 | BIT(5),
} audio_codec0_input_select_e;

/**
 * @brief codec0 output channel.
 * 
 */
typedef enum
{
    AUDIO_DAC_A1,
    AUDIO_DAC_A2,
    AUDIO_DAC_A1_A2,
} audio_codec0_output_select_e;

/**
 * @brief codec0 voltage supply.
 * 
 */
typedef enum
{
    AUDIO_CODEC0_3P3V, /**< Normal Voltage operating 3.3V */
    AUDIO_CODEC0_1P8V, /**< Low Voltage operating 1.8V */
} audio_codec0_volt_supply_e;

/**
 * @brief codec0 adc mode selection.
 * 
 */
typedef enum
{
    AUDIO_CODEC0_ADC_SINGLE_ENDED,
    AUDIO_CODEC0_ADC_DIFFERENTIAL,
} audio_codec0_adc_mode_e;

/**
 * @brief codec0 input channel analog gain, [-2dB, 24dB], 2dB steps.
 */
typedef enum
{
    AUDIO_IN_A_GAIN_m2_DB, /**< -2dB */
    AUDIO_IN_A_GAIN_0_DB,  /**< 0dB */
    AUDIO_IN_A_GAIN_2_DB,
    AUDIO_IN_A_GAIN_4_DB,
    AUDIO_IN_A_GAIN_6_DB,
    AUDIO_IN_A_GAIN_8_DB,
    AUDIO_IN_A_GAIN_10_DB,
    AUDIO_IN_A_GAIN_12_DB,
    AUDIO_IN_A_GAIN_14_DB,
    AUDIO_IN_A_GAIN_16_DB,
    AUDIO_IN_A_GAIN_18_DB,
    AUDIO_IN_A_GAIN_20_DB,
    AUDIO_IN_A_GAIN_22_DB,
    AUDIO_IN_A_GAIN_24_DB, /**< 24dB */
} audio_codec0_input_again_e;

/**
 * @brief codec0 input channel digital gain, [-64dB, 63dB], 1dB steps.
 * @note
 *       - When SNR-Optimizer is activated, ADC digital gain range is limited to [-40dB, +61dB] range.
 */
typedef enum
{
    AUDIO_IN_D_GAIN_m64_DB = 0x40, /**< -64dB */
    AUDIO_IN_D_GAIN_m63_DB,        /**< -63dB */
    AUDIO_IN_D_GAIN_m62_DB,
    AUDIO_IN_D_GAIN_m61_DB,
    AUDIO_IN_D_GAIN_m60_DB,
    AUDIO_IN_D_GAIN_m59_DB,
    AUDIO_IN_D_GAIN_m58_DB,
    AUDIO_IN_D_GAIN_m57_DB,
    AUDIO_IN_D_GAIN_m56_DB,
    AUDIO_IN_D_GAIN_m55_DB,
    AUDIO_IN_D_GAIN_m54_DB,
    AUDIO_IN_D_GAIN_m53_DB,
    AUDIO_IN_D_GAIN_m52_DB,
    AUDIO_IN_D_GAIN_m51_DB,
    AUDIO_IN_D_GAIN_m50_DB,
    AUDIO_IN_D_GAIN_m49_DB,
    AUDIO_IN_D_GAIN_m48_DB,
    AUDIO_IN_D_GAIN_m47_DB,
    AUDIO_IN_D_GAIN_m46_DB,
    AUDIO_IN_D_GAIN_m45_DB,
    AUDIO_IN_D_GAIN_m44_DB,
    AUDIO_IN_D_GAIN_m43_DB,
    AUDIO_IN_D_GAIN_m42_DB,
    AUDIO_IN_D_GAIN_m41_DB,
    AUDIO_IN_D_GAIN_m40_DB,
    AUDIO_IN_D_GAIN_m39_DB,
    AUDIO_IN_D_GAIN_m38_DB,
    AUDIO_IN_D_GAIN_m37_DB,
    AUDIO_IN_D_GAIN_m36_DB,
    AUDIO_IN_D_GAIN_m35_DB,
    AUDIO_IN_D_GAIN_m34_DB,
    AUDIO_IN_D_GAIN_m33_DB,
    AUDIO_IN_D_GAIN_m32_DB,
    AUDIO_IN_D_GAIN_m31_DB,
    AUDIO_IN_D_GAIN_m30_DB,
    AUDIO_IN_D_GAIN_m29_DB,
    AUDIO_IN_D_GAIN_m28_DB,
    AUDIO_IN_D_GAIN_m27_DB,
    AUDIO_IN_D_GAIN_m26_DB,
    AUDIO_IN_D_GAIN_m25_DB,
    AUDIO_IN_D_GAIN_m24_DB,
    AUDIO_IN_D_GAIN_m23_DB,
    AUDIO_IN_D_GAIN_m22_DB,
    AUDIO_IN_D_GAIN_m21_DB,
    AUDIO_IN_D_GAIN_m20_DB,
    AUDIO_IN_D_GAIN_m19_DB,
    AUDIO_IN_D_GAIN_m18_DB,
    AUDIO_IN_D_GAIN_m17_DB,
    AUDIO_IN_D_GAIN_m16_DB,
    AUDIO_IN_D_GAIN_m15_DB,
    AUDIO_IN_D_GAIN_m14_DB,
    AUDIO_IN_D_GAIN_m13_DB,
    AUDIO_IN_D_GAIN_m12_DB,
    AUDIO_IN_D_GAIN_m11_DB,
    AUDIO_IN_D_GAIN_m10_DB,
    AUDIO_IN_D_GAIN_m9_DB,
    AUDIO_IN_D_GAIN_m8_DB,
    AUDIO_IN_D_GAIN_m7_DB,
    AUDIO_IN_D_GAIN_m6_DB,
    AUDIO_IN_D_GAIN_m5_DB,
    AUDIO_IN_D_GAIN_m4_DB,
    AUDIO_IN_D_GAIN_m3_DB,
    AUDIO_IN_D_GAIN_m2_DB,
    AUDIO_IN_D_GAIN_m1_DB, /**< -1dB */

    AUDIO_IN_D_GAIN_0_DB = 0x00, /**< 0dB */
    AUDIO_IN_D_GAIN_1_DB,        /**< 1dB */
    AUDIO_IN_D_GAIN_2_DB,
    AUDIO_IN_D_GAIN_3_DB,
    AUDIO_IN_D_GAIN_4_DB,
    AUDIO_IN_D_GAIN_5_DB,
    AUDIO_IN_D_GAIN_6_DB,
    AUDIO_IN_D_GAIN_7_DB,
    AUDIO_IN_D_GAIN_8_DB,
    AUDIO_IN_D_GAIN_9_DB,
    AUDIO_IN_D_GAIN_10_DB,
    AUDIO_IN_D_GAIN_11_DB,
    AUDIO_IN_D_GAIN_12_DB,
    AUDIO_IN_D_GAIN_13_DB,
    AUDIO_IN_D_GAIN_14_DB,
    AUDIO_IN_D_GAIN_15_DB,
    AUDIO_IN_D_GAIN_16_DB,
    AUDIO_IN_D_GAIN_17_DB,
    AUDIO_IN_D_GAIN_18_DB,
    AUDIO_IN_D_GAIN_19_DB,
    AUDIO_IN_D_GAIN_20_DB,
    AUDIO_IN_D_GAIN_21_DB,
    AUDIO_IN_D_GAIN_22_DB,
    AUDIO_IN_D_GAIN_23_DB,
    AUDIO_IN_D_GAIN_24_DB,
    AUDIO_IN_D_GAIN_25_DB,
    AUDIO_IN_D_GAIN_26_DB,
    AUDIO_IN_D_GAIN_27_DB,
    AUDIO_IN_D_GAIN_28_DB,
    AUDIO_IN_D_GAIN_29_DB,
    AUDIO_IN_D_GAIN_30_DB,
    AUDIO_IN_D_GAIN_31_DB,
    AUDIO_IN_D_GAIN_32_DB,
    AUDIO_IN_D_GAIN_33_DB,
    AUDIO_IN_D_GAIN_34_DB,
    AUDIO_IN_D_GAIN_35_DB,
    AUDIO_IN_D_GAIN_36_DB,
    AUDIO_IN_D_GAIN_37_DB,
    AUDIO_IN_D_GAIN_38_DB,
    AUDIO_IN_D_GAIN_39_DB,
    AUDIO_IN_D_GAIN_40_DB,
    AUDIO_IN_D_GAIN_41_DB,
    AUDIO_IN_D_GAIN_42_DB,
    AUDIO_IN_D_GAIN_43_DB,
    AUDIO_IN_D_GAIN_44_DB,
    AUDIO_IN_D_GAIN_45_DB,
    AUDIO_IN_D_GAIN_46_DB,
    AUDIO_IN_D_GAIN_47_DB,
    AUDIO_IN_D_GAIN_48_DB,
    AUDIO_IN_D_GAIN_49_DB,
    AUDIO_IN_D_GAIN_50_DB,
    AUDIO_IN_D_GAIN_51_DB,
    AUDIO_IN_D_GAIN_52_DB,
    AUDIO_IN_D_GAIN_53_DB,
    AUDIO_IN_D_GAIN_54_DB,
    AUDIO_IN_D_GAIN_55_DB,
    AUDIO_IN_D_GAIN_56_DB,
    AUDIO_IN_D_GAIN_57_DB,
    AUDIO_IN_D_GAIN_58_DB,
    AUDIO_IN_D_GAIN_59_DB,
    AUDIO_IN_D_GAIN_60_DB,
    AUDIO_IN_D_GAIN_61_DB,
    AUDIO_IN_D_GAIN_62_DB,
    AUDIO_IN_D_GAIN_63_DB, /**< 63dB */
} audio_codec0_input_dgain_e;

/**
 * @brief codec0 output channel analog gain, [-28dB, 6dB], 2dB steps.
 */
typedef enum
{
    AUDIO_OUT_A_GAIN_m28_DB, /**< -28dB */
    AUDIO_OUT_A_GAIN_m26_DB, /**< -26dB */
    AUDIO_OUT_A_GAIN_m24_DB,
    AUDIO_OUT_A_GAIN_m22_DB,
    AUDIO_OUT_A_GAIN_m20_DB,
    AUDIO_OUT_A_GAIN_m18_DB,
    AUDIO_OUT_A_GAIN_m16_DB,
    AUDIO_OUT_A_GAIN_m14_DB,
    AUDIO_OUT_A_GAIN_m12_DB,
    AUDIO_OUT_A_GAIN_m10_DB,
    AUDIO_OUT_A_GAIN_m8_DB,
    AUDIO_OUT_A_GAIN_m6_DB,
    AUDIO_OUT_A_GAIN_m4_DB,
    AUDIO_OUT_A_GAIN_m2_DB,
    AUDIO_OUT_A_GAIN_0_DB,
    AUDIO_OUT_A_GAIN_2_DB,
    AUDIO_OUT_A_GAIN_4_DB,
    AUDIO_OUT_A_GAIN_6_DB,
} audio_codec0_output_again_e;

/**
 * @brief codec0 output channel digital gain, [-64dB, 63dB], 1dB steps.
 * @note
 *       - When SNR-Optimizer is activated, DAC digital gain range is limited to [-64dB, +35dB] range.
 */
typedef enum
{
    AUDIO_OUT_D_GAIN_m64_DB = 0x40, /**< -64dB */
    AUDIO_OUT_D_GAIN_m63_DB,        /**< -63dB */
    AUDIO_OUT_D_GAIN_m62_DB,
    AUDIO_OUT_D_GAIN_m61_DB,
    AUDIO_OUT_D_GAIN_m60_DB,
    AUDIO_OUT_D_GAIN_m59_DB,
    AUDIO_OUT_D_GAIN_m58_DB,
    AUDIO_OUT_D_GAIN_m57_DB,
    AUDIO_OUT_D_GAIN_m56_DB,
    AUDIO_OUT_D_GAIN_m55_DB,
    AUDIO_OUT_D_GAIN_m54_DB,
    AUDIO_OUT_D_GAIN_m53_DB,
    AUDIO_OUT_D_GAIN_m52_DB,
    AUDIO_OUT_D_GAIN_m51_DB,
    AUDIO_OUT_D_GAIN_m50_DB,
    AUDIO_OUT_D_GAIN_m49_DB,
    AUDIO_OUT_D_GAIN_m48_DB,
    AUDIO_OUT_D_GAIN_m47_DB,
    AUDIO_OUT_D_GAIN_m46_DB,
    AUDIO_OUT_D_GAIN_m45_DB,
    AUDIO_OUT_D_GAIN_m44_DB,
    AUDIO_OUT_D_GAIN_m43_DB,
    AUDIO_OUT_D_GAIN_m42_DB,
    AUDIO_OUT_D_GAIN_m41_DB,
    AUDIO_OUT_D_GAIN_m40_DB,
    AUDIO_OUT_D_GAIN_m39_DB,
    AUDIO_OUT_D_GAIN_m38_DB,
    AUDIO_OUT_D_GAIN_m37_DB,
    AUDIO_OUT_D_GAIN_m36_DB,
    AUDIO_OUT_D_GAIN_m35_DB,
    AUDIO_OUT_D_GAIN_m34_DB,
    AUDIO_OUT_D_GAIN_m33_DB,
    AUDIO_OUT_D_GAIN_m32_DB,
    AUDIO_OUT_D_GAIN_m31_DB,
    AUDIO_OUT_D_GAIN_m30_DB,
    AUDIO_OUT_D_GAIN_m29_DB,
    AUDIO_OUT_D_GAIN_m28_DB,
    AUDIO_OUT_D_GAIN_m27_DB,
    AUDIO_OUT_D_GAIN_m26_DB,
    AUDIO_OUT_D_GAIN_m25_DB,
    AUDIO_OUT_D_GAIN_m24_DB,
    AUDIO_OUT_D_GAIN_m23_DB,
    AUDIO_OUT_D_GAIN_m22_DB,
    AUDIO_OUT_D_GAIN_m21_DB,
    AUDIO_OUT_D_GAIN_m20_DB,
    AUDIO_OUT_D_GAIN_m19_DB,
    AUDIO_OUT_D_GAIN_m18_DB,
    AUDIO_OUT_D_GAIN_m17_DB,
    AUDIO_OUT_D_GAIN_m16_DB,
    AUDIO_OUT_D_GAIN_m15_DB,
    AUDIO_OUT_D_GAIN_m14_DB,
    AUDIO_OUT_D_GAIN_m13_DB,
    AUDIO_OUT_D_GAIN_m12_DB,
    AUDIO_OUT_D_GAIN_m11_DB,
    AUDIO_OUT_D_GAIN_m10_DB,
    AUDIO_OUT_D_GAIN_m9_DB,
    AUDIO_OUT_D_GAIN_m8_DB,
    AUDIO_OUT_D_GAIN_m7_DB,
    AUDIO_OUT_D_GAIN_m6_DB,
    AUDIO_OUT_D_GAIN_m5_DB,
    AUDIO_OUT_D_GAIN_m4_DB,
    AUDIO_OUT_D_GAIN_m3_DB,
    AUDIO_OUT_D_GAIN_m2_DB,
    AUDIO_OUT_D_GAIN_m1_DB, /**< -1dB */

    AUDIO_OUT_D_GAIN_0_DB = 0x00, /**< 0dB */
    AUDIO_OUT_D_GAIN_1_DB,        /**< 1dB */
    AUDIO_OUT_D_GAIN_2_DB,
    AUDIO_OUT_D_GAIN_3_DB,
    AUDIO_OUT_D_GAIN_4_DB,
    AUDIO_OUT_D_GAIN_5_DB,
    AUDIO_OUT_D_GAIN_6_DB,
    AUDIO_OUT_D_GAIN_7_DB,
    AUDIO_OUT_D_GAIN_8_DB,
    AUDIO_OUT_D_GAIN_9_DB,
    AUDIO_OUT_D_GAIN_10_DB,
    AUDIO_OUT_D_GAIN_11_DB,
    AUDIO_OUT_D_GAIN_12_DB,
    AUDIO_OUT_D_GAIN_13_DB,
    AUDIO_OUT_D_GAIN_14_DB,
    AUDIO_OUT_D_GAIN_15_DB,
    AUDIO_OUT_D_GAIN_16_DB,
    AUDIO_OUT_D_GAIN_17_DB,
    AUDIO_OUT_D_GAIN_18_DB,
    AUDIO_OUT_D_GAIN_19_DB,
    AUDIO_OUT_D_GAIN_20_DB,
    AUDIO_OUT_D_GAIN_21_DB,
    AUDIO_OUT_D_GAIN_22_DB,
    AUDIO_OUT_D_GAIN_23_DB,
    AUDIO_OUT_D_GAIN_24_DB,
    AUDIO_OUT_D_GAIN_25_DB,
    AUDIO_OUT_D_GAIN_26_DB,
    AUDIO_OUT_D_GAIN_27_DB,
    AUDIO_OUT_D_GAIN_28_DB,
    AUDIO_OUT_D_GAIN_29_DB,
    AUDIO_OUT_D_GAIN_30_DB,
    AUDIO_OUT_D_GAIN_31_DB,
    AUDIO_OUT_D_GAIN_32_DB,
    AUDIO_OUT_D_GAIN_33_DB,
    AUDIO_OUT_D_GAIN_34_DB,
    AUDIO_OUT_D_GAIN_35_DB,
    AUDIO_OUT_D_GAIN_36_DB,
    AUDIO_OUT_D_GAIN_37_DB,
    AUDIO_OUT_D_GAIN_38_DB,
    AUDIO_OUT_D_GAIN_39_DB,
    AUDIO_OUT_D_GAIN_40_DB,
    AUDIO_OUT_D_GAIN_41_DB,
    AUDIO_OUT_D_GAIN_42_DB,
    AUDIO_OUT_D_GAIN_43_DB,
    AUDIO_OUT_D_GAIN_44_DB,
    AUDIO_OUT_D_GAIN_45_DB,
    AUDIO_OUT_D_GAIN_46_DB,
    AUDIO_OUT_D_GAIN_47_DB,
    AUDIO_OUT_D_GAIN_48_DB,
    AUDIO_OUT_D_GAIN_49_DB,
    AUDIO_OUT_D_GAIN_50_DB,
    AUDIO_OUT_D_GAIN_51_DB,
    AUDIO_OUT_D_GAIN_52_DB,
    AUDIO_OUT_D_GAIN_53_DB,
    AUDIO_OUT_D_GAIN_54_DB,
    AUDIO_OUT_D_GAIN_55_DB,
    AUDIO_OUT_D_GAIN_56_DB,
    AUDIO_OUT_D_GAIN_57_DB,
    AUDIO_OUT_D_GAIN_58_DB,
    AUDIO_OUT_D_GAIN_59_DB,
    AUDIO_OUT_D_GAIN_60_DB,
    AUDIO_OUT_D_GAIN_61_DB,
    AUDIO_OUT_D_GAIN_62_DB,
    AUDIO_OUT_D_GAIN_63_DB, /**< 63dB */
} audio_codec0_output_dgain_e;

/**
 * @brief Audio sample rate value.
  * |                                    |                            |
 * | :---------------------------------- | :------------------------- |
 * |            <31:8>                   |         <7:0>              |
 * |         audio_control_div           |       codec_fs             |
 * | fs = MCLK / (audio_control_div + 1) |   codec_freq_sel reg value |
 * 
 * @note MCLK = 11.2896MHz(for 44.1kHz) or 12.288MHz (for others).
 */
typedef enum
{
    AUDIO_16K = 0x03 | (767 << 8),
    AUDIO_44P1K = 0x08 | (255 << 8),
    AUDIO_48K = 0x08 | (255 << 8),
    AUDIO_96K = 0x0a | (127 << 8),
    AUDIO_192K = 0x0c | (63 << 8),
    AUDIO_384K = 0x0d | (31 << 8),
    AUDIO_768K = 0x0e | (15 << 8),
} audio_sample_rate_e;

/**
 * @brief Audio codec0 data format.
 * 
 */
typedef enum
{
    AUDIO_CODEC0_BIT_16_DATA,
    AUDIO_CODEC0_BIT_20_DATA = 0x02,
    AUDIO_CODEC0_BIT_24_DATA,
} audio_codec0_data_select_e;

/**
 * @brief codec0 ADC Wind Noise Filter selection
 * 
 */
typedef enum
{
    AUDIO_CODEC0_ADC_WNF_INACTIVE,
    AUDIO_CODEC0_ADC_WNF_MODE1,
    AUDIO_CODEC0_ADC_WNF_MODE2,
    AUDIO_CODEC0_ADC_WNF_MODE3,
} audio_codec0_adc_wnf_e;

/**
 * @brief codec0 ADC power mode.
 * 
 */
typedef enum
{
    AUDIO_CODEC0_ADC_NORMAL_MODE,
    AUDIO_CODEC0_ADC_LOW_POWER_MODE,
    AUDIO_CODEC0_ADC_ULTRA_LOW_POWER_MODE,
} audio_codec0_adc_power_mode;

/**
 * @brief codec0 input config.
 * 
 */
typedef struct
{
    audio_codec0_input_select_e input_src;
    audio_sample_rate_e sample_rate;
    audio_codec0_data_select_e data_format;
} audio_codec0_input_config_t;

/**
 * @brief codec0 output config.
 * 
 */
typedef struct
{
    audio_codec0_output_select_e output_dst;
    audio_sample_rate_e sample_rate;
    audio_codec0_data_select_e data_format;
} audio_codec0_output_config_t;

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio dma/fifo enum/struct                                          *
 *********************************************************************************************************************/
/*!
 * @name Audio dma/fifo enum/struct.
 * @{
 */

/**
 * @brief FIFO channel.
 * 
 */
typedef enum
{
    FIFO0 = 0x00,
    FIFO1,
    FIFO2,
    FIFO3,
} audio_fifo_chn_e;

/**
 * @brief FIFO TX/RX IRQ type.
 * 
 */
typedef enum
{
    AUDIO_TX_FIFO0 = BIT(0),
    AUDIO_TX_FIFO1 = BIT(1),
    AUDIO_TX_FIFO2 = BIT(2),
    AUDIO_TX_FIFO3 = BIT(3),
    AUDIO_RX_FIFO0 = BIT(4),
    AUDIO_RX_FIFO1 = BIT(5),
    AUDIO_RX_FIFO2 = BIT(6),
    AUDIO_RX_FIFO3 = BIT(7),
} audio_fifo_type_e;

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio hac enum/struct                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio hac enum/struct.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio i2s enum/struct                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio i2s enum struct.
 * @{
 */

/**
 * @brief I2S select.
 * 
 */
typedef enum
{
    I2S0,
    I2S1,
    I2S2,
} i2s_select_e;

/**
 * @brief I2S mode.
 * 
 */
typedef enum
{
    I2S_RJ_MODE,
    I2S_LJ_MODE,
    I2S_I2S_MODE,
    I2S_DSP_MODE,
    I2S_TDM_MODE, /**< only for i2s0(this version not support.). */
} i2s_mode_select_e;

/**
 * @brief TDM mode.
 * @note  Only for i2s0.
 */
typedef enum
{
    I2S_TDM_MODE_A,
    I2S_TDM_MODE_B,
    I2S_TDM_MODE_C,
} i2s_tdm_mode_select_e;

/**
 * @brief TDM channel.
 * @note  Only for i2s0.
 */
typedef enum
{
    I2S_TDM_4_CHN,
    I2S_TDM_6_CHN,
    I2S_TDM_8_CHN,
} i2s_tdm_chn_e;

/**
 * @brief TDM slot width.
 * @note  Only for i2s0.
 *
 */
typedef enum
{
    I2S_TDM_SLOT_WIDTH_16,
    I2S_TDM_SLOT_WIDTH_24,
    I2S_TDM_SLOT_WIDTH_32,
} i2s_tdm_slot_width_e;

/**
 * @brief I2S io mode.
 * 
 */
typedef enum
{
    I2S_5_LINE_MODE,     /**< BCLK, ADC_LR_CLK, DAC_LR_CLK, ADC_DATA, DAC_DATA. */
    I2S_4_LINE_DAC_MODE, /**< BCLK, DAC_LR_CLK, ADC_DATA, DAC_DATA. */
    I2S_4_LINE_ADC_MODE, /**< BCLK, ADC_LR_CLK, ADC_DATA, DAC_DATA. */
    I2S_2_LANE_TX_MODE,  /**< ADC_DATA and DAC_DATA as TX at the same time. */
    I2S_2_LANE_RX_MODE,  /**< ADC_DATA and DAC_DATA as RX at the same time. */
} i2s_io_mode_e;

/**
 * @brief I2S word length.
 * 
 */
typedef enum
{
    I2S_BIT_16_DATA,
    I2S_BIT_20_DATA,
    I2S_BIT_24_DATA,
} i2s_wl_mode_e;

/**
 * @brief I2S master/slave select.
 * 
 */
typedef enum
{
    I2S_AS_SLAVE_EN,
    I2S_AS_MASTER_EN,
} i2s_m_s_mode_e;

/**
 * @brief I2S align mode.
 * 
 */
typedef enum
{
    I2S0_I2S1_ALIGN = BIT(0) | BIT(1),
    I2S1_I2S2_ALIGN = BIT(1) | BIT(2),
    I2S0_I2S1_I2S2_ALIGN = BIT(0) | BIT(1) | BIT(2),
} i2s_align_mode_e;

/**
 * @brief I2S align clk.
 * 
 */
typedef enum
{
    I2S_ALIGN_SELF_CLK, /**< use self i2s clk as align clk.*/
    I2S_ALIGN_CLK,      /**< use i2s1 clk as align clk.*/
} i2s_align_clk_e;

/**
 * @brief I2S data invert select.
 * 
 */
typedef enum
{
    I2S_DATA_INVERT_DIS,
    I2S_DATA_INVERT_EN,
} i2s_data_invert_e;

/**
 * @brief I2S CLK invert select.
 * 
 */
typedef enum
{
    I2S_LR_CLK_INVERT_DIS, /**< dsp mode: dsp mode a */
    I2S_LR_CLK_INVERT_EN,  /**< dsp mode: dsp mode b */
} i2s_lr_clk_invert_e;

/**
 * @brief I2S TX channel.
 * 
 */
typedef enum
{
    I2S0_CHN0,
    I2S0_CHN1,
    I2S0_CHN2,
    I2S0_CHN3,
    I2S0_CHN4,
    I2S0_CHN5,
    I2S0_CHN6,
    I2S0_CHN7,

    I2S1_CHN0,
    I2S1_CHN1,

    I2S2_CHN0,
    I2S2_CHN1,
} audio_i2s_tx_chn_e;

/**
 * @brief I2S invert config.
 * 
 */
typedef struct
{
    unsigned char i2s_lr_clk_invert_select;
    unsigned char i2s_data_invert_select;
} i2s_invert_config_t;

/**
 * @brief I2S pin config.
 * 
 */
typedef struct
{
    i2s_bclk_pin_e bclk_pin;
    i2s_adc_lr_clk_pin_e adc_lr_clk_pin;
    i2s_adc_dat_pin_e adc_dat_pin;
    i2s_dac_lr_clk_pin_e dac_lr_clk_pin;
    i2s_dac_dat_pin_e dac_dat_pin;
} i2s_pin_config_t;

/**
 * @brief I2S align config.
 * 
 */
typedef struct
{
    unsigned int align_th; /**< align threshold*/
    i2s_align_mode_e align_mode;
    i2s_align_clk_e align_clk;
} i2s_align_config_t;

/**
 * @brief I2S config.
 * 
 */
typedef struct
{
    unsigned short *sample_rate;
    i2s_pin_config_t *pin_config;
    i2s_select_e i2s_select;
    i2s_wl_mode_e data_width;
    i2s_mode_select_e i2s_mode;
    i2s_tdm_mode_select_e tdm_mode;
    i2s_tdm_slot_width_e tdm_slot_width;
    i2s_m_s_mode_e master_slave_mode;
    i2s_io_mode_e io_mode;
} audio_i2s_config_t;

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio matrix enum/struct                                            *
 *********************************************************************************************************************/
/*!
 * @name Audio matrix enum/struct.
 * @{
 */

/**
 * @brief rx fifo route source select.
 * 
 */
typedef enum
{
    FIFO_RX_ROUTE_I2S0_RX = 0x01,
    FIFO_RX_ROUTE_I2S1_RX,
    FIFO_RX_ROUTE_I2S2_RX,

    FIFO_RX_ROUTE_CODEC0_ADCA = 0x06, /**< codec0 ADC_A1(amic/dmic), ADC_A2(amic/dmic). */
    FIFO_RX_ROUTE_CODEC0_ADCB,        /**< codec0 ADC_B1(amic/dmic), ADC_B2(dmic). */
    FIFO_RX_ROUTE_CODEC1_ADCA,        /**< codec1 ADC_A1(dmic), ADC_A2(dmic). */
} audio_matrix_rx_fifo_route_e;

/**
 * @brief fifo rx route data format select.
 * 
 */
typedef enum
{
    /* fifo rx route i2s data format(i2s0/1/2 common config). */
    FIFO_RX_I2S_RX_CHN01_20_OR_24 = 0x00, /**< fifo rx route from i2s data format. */
    FIFO_RX_I2S_RX_CHN01_16,
    FIFO_RX_I2S_RX_CHN0_20_OR_24,
    FIFO_RX_I2S_RX_CHN0_16,
    FIFO_RX_I2S_RX_CHN1_20_OR_24,
    FIFO_RX_I2S_RX_CHN1_16,

    FIFO_RX_I2S0_TDM_20_OR_24 = 0x06, /**< only for i2s0 tdm data format. */
    FIFO_RX_I2S0_TDM_16,

    /* fifo rx route adc data format. */
    FIFO_RX_CODEC0_ADCA_A1_A2_32BIT = 0x00, /**< fifo rx route from codec0 adca data format. */
    FIFO_RX_CODEC0_ADCA_A1_A2_16BIT,
    FIFO_RX_CODEC0_ADCA_A1_32BIT,
    FIFO_RX_CODEC0_ADCA_A1_16BIT,
    FIFO_RX_CODEC0_ADCA_A2_32BIT,
    FIFO_RX_CODEC0_ADCA_A2_16BIT,

    FIFO_RX_CODEC0_ADCB_B1_B2_32BIT = 0x00, /**< fifo rx route from codec0 adcb data format. */
    FIFO_RX_CODEC0_ADCB_B1_B2_16BIT,
    FIFO_RX_CODEC0_ADCB_B1_32BIT,
    FIFO_RX_CODEC0_ADCB_B1_16BIT,
    FIFO_RX_CODEC0_ADCB_B2_32BIT,
    FIFO_RX_CODEC0_ADCB_B2_16BIT,

    FIFO_RX_CODEC1_ADCA_A1_A2_32BIT = 0x00, /**< fifo rx route from codec1 adca data format. */
    FIFO_RX_CODEC1_ADCA_A1_A2_16BIT,
    FIFO_RX_CODEC1_ADCA_A1_32BIT,
    FIFO_RX_CODEC1_ADCA_A1_16BIT,
    FIFO_RX_CODEC1_ADCA_A2_32BIT,
    FIFO_RX_CODEC1_ADCA_A2_16BIT,
} audio_matrix_rx_fifo_format_e;

/**
 * @brief i2s tx route source select.
 * 
 */
typedef enum
{
    I2S_TX_ROUTE_FIFO = 0x01, /**< I2S channel 0/1 route */
} audio_matrix_i2s_tx_route_e;

/**
 * @brief i2s tx route data format select.
 * 
 */
typedef enum
{
    /**< i2s common config. */
    I2S_TX_FIFO0_20_OR_24_MONO = 0x00,
    I2S_TX_FIFO1_20_OR_24_MONO,
    I2S_TX_FIFO2_20_OR_24_MONO,
    I2S_TX_FIFO3_20_OR_24_MONO,

    I2S_TX_FIFO0_20_OR_24_STEREO = 0x06,
    I2S_TX_FIFO1_20_OR_24_STEREO,
    I2S_TX_FIFO2_20_OR_24_STEREO,
    I2S_TX_FIFO3_20_OR_24_STEREO,

    I2S_TX_FIFO0_16_MONO = 0x0a,
    I2S_TX_FIFO1_16_MONO,
    I2S_TX_FIFO2_16_MONO,
    I2S_TX_FIFO3_16_MONO,

    I2S_TX_FIFO0_16_STEREO = 0x10,
    I2S_TX_FIFO1_16_STEREO,
    I2S_TX_FIFO2_16_STEREO,
    I2S_TX_FIFO3_16_STEREO,
} audio_matrix_i2s_tx_format_e;

/**
 * @brief dac route source select.
 * 
 */
typedef enum
{
    DAC_ROUTE_FIFO = 0x01,
} audio_matrix_dac_route_e;

/**
 * @brief dac route data format select.
 * 
 */
typedef enum
{
    DAC_FIFO_MONO_24BIT_FIFO0 = 0x00, /**< dac route from fifo data format select. */
    DAC_FIFO_MONO_24BIT_FIFO1,
    DAC_FIFO_MONO_24BIT_FIFO2,
    DAC_FIFO_MONO_24BIT_FIFO3,

    DAC_FIFO_STEREO_24BIT_FIFO0 = 0x04,
    DAC_FIFO_STEREO_24BIT_FIFO1,
    DAC_FIFO_STEREO_24BIT_FIFO2,
    DAC_FIFO_STEREO_24BIT_FIFO3,

    DAC_FIFO_MONO_16BIT_FIFO0 = 0x0a,
    DAC_FIFO_MONO_16BIT_FIFO1,
    DAC_FIFO_MONO_16BIT_FIFO2,
    DAC_FIFO_MONO_16BIT_FIFO3,

    DAC_FIFO_STEREO_16BIT_FIFO0 = 0x0e,
    DAC_FIFO_STEREO_16BIT_FIFO1,
    DAC_FIFO_STEREO_16BIT_FIFO2,
    DAC_FIFO_STEREO_16BIT_FIFO3,
} audio_matrix_dac_format_e;

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio side_tone enum/struct                                         *
 *********************************************************************************************************************/
/*!
 * @name Audio side_tone enum/struct.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio spdif enum/struct                                             *
 *********************************************************************************************************************/
/*!
 * @name Audio spdif enum/struct
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio config/route enum/struct                                      *
 *********************************************************************************************************************/
/*!
 * @name Audio config/route enum/struct.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio anc interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio anc interface.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio asrc interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio asrc interface.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio power/clock interface                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio clock interface.
 * @{
 */

/**
 * @brief      This function serves to initialize audio.
 * @param[in]  audio_pll - audio pll clock select.
 * @return     none
 * @note       - When using the audio module, this interface must be configured first, otherwise the following interfaces will not take effect.
 *             - When the sampling rate is 44.1KHz, audio_pll needs to be set to AUDIO_PLL_CLK_33P8688M or AUDIO_PLL_CLK_169P344M.
 */
void audio_init(sys_audio_pll_clk_e audio_pll);

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio codec0 interface                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio codec0 interface.
 * @{
 */

/**
 * @brief      This function serves to power on codec0.
 * @param[in]  power_mode - codec0 power mode selection.
 * @param[in]  volt       - codec0 analog voltage selection.
 * @return     none 
 */
void audio_codec0_power_on(audio_codec0_power_e power_mode, audio_codec0_volt_supply_e volt);

/**
 * @brief      This function serves to power down codec0 adc.
 * @param[in]  adc - adc channel.
 * @return     none
 * @note
 *             - ADC_B only ADC_B1 support analog data.
 *             - This interface only powers down the analog ADC.
 */
void audio_codec0_adc_power_down(audio_codec0_input_select_e adc);

/**
 * @brief      This function serves to power on codec0 output.
 * @param[in]  output - output channel.
 * @return     none
 */
void audio_codec0_output_power_down(audio_codec0_output_select_e output);

/**
 * @brief      This function serves to power down codec0.
 * @return     none 
 * @note
 *             - After power down, cannot access codec0 registers.
 */
void audio_codec0_power_down(void);

/**
 * @brief      This function serves to set codec0 voltage supply.
 * @param[in]  voltage - codec0 voltage supply select.
 * 
 */
static inline void audio_codec0_vol_supply_select(audio_codec0_volt_supply_e voltage)
{
    reg_audio_codec0_cr_vic = (reg_audio_codec0_cr_vic & (~FLD_CODEC0_AVD_1V8)) | MASK_VAL(FLD_CODEC0_AVD_1V8, voltage);
}

/**
 * @brief      This function serves to enable/disable codec0 micbias output(2.5V).
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: enable micbias, 0: disable micbias.
 * @return     none 
 * @note
 *             - bias only for amic.
 *             - ADC_B only ADC_B1 support bias output.
 */
void audio_codec0_set_micbias(audio_codec0_input_select_e input, unsigned char enable);

/**
 * @brief      This function serves to set codec0 adc mode.
 * @param[in]  adc      - adc channel.
 * @param[in]  adc_mode - 1: differential input, 0: single-ended input.
 * @return     none
 * @note
 *             - adc mode only for line_in or amic.
 */
void audio_codec0_set_adc_mode(audio_codec0_input_select_e adc, audio_codec0_adc_mode_e adc_mode);

/**
 * @brief      This function serves to set codec0 input mute.
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: soft mute active, 0: soft mute inactive.
 * @return     none
 */
void audio_codec0_set_input_mute(audio_codec0_input_select_e input, unsigned char enable);

/**
 * @brief      This function serves to set codec0 output mute.
 * @param[in]  output - output channel.
 * @param[in]  enable - 1: soft mute active, 0: soft mute inactive.
 * @return     none 
 */
void audio_codec0_set_output_mute(audio_codec0_output_select_e output, unsigned char enable);

/**
 * @brief      This function serves to set codec0 input sample rate.
 * @param[in]  input - input channel.
 * @param[in]  fs    - input sample rate.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 sample rates are set in pairs.
 */
void audio_codec0_set_input_fs(audio_codec0_input_select_e input, audio_sample_rate_e fs);

/**
 * @brief      This function serves to set codec0 output sample rate.
 * @param[in]  fs - output sample rate.
 * @return     none 
 * @note
 *             - DAC_A1/2 or DAC_B1/2 sample rates are set in pairs.
 */
void audio_codec0_set_output_fs(audio_sample_rate_e fs);

/**
 * @brief      This function serves to set codec0 input data world length.
 * @param[in]  input - input channel.
 * @param[in]  wl    - world length.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 world length are set in pairs.
 */
void audio_codec0_set_input_wl(audio_codec0_input_select_e input, audio_codec0_data_select_e wl);

/**
 * @brief      This function serves to set codec0 output data world length.
 * @param[in]  output - output channel.
 * @param[in]  wl     - world length.
 * @return     none 
 */
void audio_codec0_set_output_wl(audio_codec0_output_select_e output, audio_codec0_data_select_e wl);

/**
 * @brief      This function serves to set codec0 input analog gain.
 * @param[in]  input - input channel.
 * @param[in]  gain  - input analog gain.
 * @return     none 
 * @note
 *             - input analog gain only for line_in or amic.
 */
void audio_codec0_set_input_again(audio_codec0_input_select_e input, audio_codec0_input_again_e gain);

/**
 * @brief      This function serves to set codec0 input digital gain.
 * @param[in]  input - input channel.
 * @param[in]  gain  - input digital gain.
 * @return     none 
 */
void audio_codec0_set_input_dgain(audio_codec0_input_select_e input, audio_codec0_input_dgain_e gain);

/**
 * @brief      This function serves to set codec0 output analog gain.
 * @param[in]  output - output channel.
 * @param[in]  gain   - output analog gain.
 * @return     none 
 */
void audio_codec0_set_output_again(audio_codec0_output_select_e output, audio_codec0_output_again_e gain);

/**
 * @brief      This function serves to set codec0 output digital gain.
 * @param[in]  output - output channel.
 * @param[in]  gain   - output digital gain.
 * @return     none 
 */
void audio_codec0_set_output_dgain(audio_codec0_output_select_e output, audio_codec0_output_dgain_e gain);

/**
 * @brief      This function serves to enable/disable codec0 input SNR optimisation.
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: adc SNR optimisation active, 0:adc SNR optimisation inactive.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 SNR optimisation are set in pairs.
 */
void audio_codec0_set_input_snr_opt(audio_codec0_input_select_e input, unsigned char enable);

/**
 * @brief      This function serves to enable/disable codec0 input HPF(High Pass Filter).
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: adc High Pass Filter active, 0:adc High Pass Filter inactive.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 High Pass Filter are set in pairs.
 */
void audio_codec0_input_hpf_en(audio_codec0_input_select_e input, unsigned char enable);

/**
 * @brief      This function serves to init codec0 input.
 * @param[in]  input_config - codec0 input config.
 * @return     none 
 */
void audio_codec0_input_init(audio_codec0_input_config_t *input_config);

/**
 * @brief      This function serves to init codec0 output config.
 * @param[in]  output_config - codec0 output config.
 * @return     none 
 */
void audio_codec0_output_init(audio_codec0_output_config_t *output_config);

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio dma/fifo interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio ama/fifo interface.
 * @{
 */

/**
 * @brief      This function serves to tx fifo dma trigger number.
 * @param[in]  tx_fifo_chn - the fifo channel.
 * @param[in]  number      - the number of dma trigger, the unit is word.
 * @return     none
 */
static inline void audio_set_fifo_tx_trig_num(audio_fifo_chn_e tx_fifo_chn, unsigned char number)
{
    reg_audio_dma_tx_fifo_trig_num(tx_fifo_chn) = (reg_audio_dma_tx_fifo_trig_num(tx_fifo_chn) & (~FLD_TX_FIFO_TRIG_NUM)) |
                                                  (number & FLD_TX_FIFO_TRIG_NUM);
}
/**
 * @brief      This function serves to rx fifo dma trigger number.
 * @param[in]  rx_fifo_chn - the fifo channel.
 * @param[in]  number      - the number of dma trigger, the unit is word.
 * @return     none
 */
static inline void audio_set_fifo_rx_trig_num(audio_fifo_chn_e rx_fifo_chn, unsigned char number)
{
    reg_audio_dma_rx_fifo_trig_num(rx_fifo_chn) = (reg_audio_dma_rx_fifo_trig_num(rx_fifo_chn) & (~FLD_RX_FIFO_TRIG_NUM)) |
                                                  (number & FLD_RX_FIFO_TRIG_NUM);
}

/**
 * @brief      This function serves to clear fifo data.
 * @param[in]  fifo_chn   - fifo channel
 * @param[in]  clear_flag
 *                        - 1: audio fifo cnt clear
 *                        - 0: audio fifo cnt clear release.
 * @return     none.
 */
static inline void audio_fifo_clear(audio_fifo_type_e fifo_chn, char clear_flag)
{
    reg_audio_dma_fifo_clr = reg_audio_dma_fifo_clr | MASK_VAL(fifo_chn, clear_flag);
}

/**
 * @brief      This function serves to enable read fifo ptr.
 * @param[in]  fifo_chn - fifo channel
 * @return     none
 */
static inline void audio_fifo_ptr_en(audio_fifo_type_e fifo_chn)
{
    BM_SET(reg_audio_dma_ptr_en, fifo_chn);
}

/**
 * @brief      This function serves to disable read fifo ptr.
 * @param[in]  fifo_chn - fifo channel
 * @return     none
 */
static inline void audio_fifo_ptr_dis(audio_fifo_type_e fifo_chn)
{
    BM_CLR(reg_audio_dma_ptr_en, fifo_chn);
}

/**
 * @brief      This function servers to get audio fifo irq status.
 * @param[in]  fifo_type - the audio fifo type.
 * @return     irq status of audio fifo.
 */
static inline unsigned char audio_get_fifo_irq_status(audio_fifo_type_e fifo_type)
{
    return reg_audio_dma_irq_st & fifo_type;
}

/**
 * @brief      This function servers to clear audio fifo irq status.
 * @param[in]  fifo_type - the audio fifo type.
 * @return     none
 */
static inline void audio_clr_fifo_irq_status(audio_fifo_type_e fifo_type)
{
    reg_audio_dma_irq_st = fifo_type;
}

/**
 * @brief      This function serves to enable audio fifo irq.
 * @param[in]  fifo_chn - audio fifo channel.
 * @return     none
 */
static inline void audio_fifo_irq_en(audio_fifo_type_e fifo_chn)
{
    BM_SET(reg_audio_dma_irq_en, fifo_chn);
}

/**
 * @brief      This function serves to disable audio fifo irq.
 * @param[in]  fifo_chn - audio fifo channel.
 * @return     none
 */
static inline void audio_fifo_irq_dis(audio_fifo_type_e fifo_chn)
{
    BM_CLR(reg_audio_dma_irq_en, fifo_chn);
}

/**
 * @brief      This function serves to get tx read pointer.
 * @param[in]  tx_fifo_chn - select fifo channel
 * @return     the result of tx read pointer.
 */
static inline unsigned short audio_get_tx_rptr(audio_fifo_chn_e tx_fifo_chn)
{
    return reg_audio_dma_tx_rptr(tx_fifo_chn);
}

/**
 * @brief      This function serves to get rx write pointer.
 * @param[in]  rx_fifo_chn - select fifo channel
 * @return     the result of rx write pointer.
 */
static inline unsigned short audio_get_rx_wptr(audio_fifo_chn_e rx_fifo_chn)
{
    return reg_audio_dma_rx_wptr(rx_fifo_chn);
}

/**
 * @brief      This function serves to set tx buff length.
 * @param[in]  tx_fifo_chn - the fifo channel.
 * @param[in]  len         - the length of tx buff, the unit is byte.
 * @return     none
 */
static inline void audio_set_tx_buff_len(audio_fifo_chn_e tx_fifo_chn, unsigned short len)
{
    reg_audio_dma_tx_max(tx_fifo_chn) = ((len) >> 2) - 1;
}

/**
 * @brief      This function serves to set rx buff length.
 * @param[in]  rx_fifo_chn - the fifo channel.
 * @param[in]  len         - the length of rx buff, the unit is byte.
 * @return     none
 */
static inline void audio_set_rx_buff_len(audio_fifo_chn_e rx_fifo_chn, unsigned short len)
{
    reg_audio_dma_rx_max(rx_fifo_chn) = ((len) >> 2) - 1;
}

/**
 * @brief      This function serves to set tx buff threshold.
 * @param[in]  tx_fifo_chn - the fifo channel.
 * @param[in]  threshold   - the threshold of tx buff, the unit is byte.
 * @return     none
 */
static inline void audio_set_tx_buff_thres(audio_fifo_chn_e tx_fifo_chn, unsigned short threshold)
{
    reg_audio_dma_tx_th(tx_fifo_chn) = ((threshold) >> 2) - 1;
}

/**
 * @brief      This function serves to set rx buff threshold.
 * @param[in]  rx_fifo_chn - the fifo channel.
 * @param[in]  threshold   - the threshold of rx buff, the unit is byte.
 * @return     none
 */
static inline void audio_set_rx_buff_thres(audio_fifo_chn_e rx_fifo_chn, unsigned short threshold)
{
    reg_audio_dma_rx_th(rx_fifo_chn) = ((threshold) >> 2) - 1;
}

/**
 * @brief      This function serves to enable rx_dma channel.
 * @param[in]  chn   - dma channel.
 * @return     none
 */
static inline void audio_rx_dma_en(dma_chn_e chn)
{
    dma_chn_en(chn);
}

/**
  * @brief      This function serves to disable rx_dma channel.
  * @param[in]  chn   - dma channel.
  * @return     none
  */
static inline void audio_rx_dma_dis(dma_chn_e chn)
{
    dma_chn_dis(chn);
}

/**
 * @brief      This function serves to enable tx_dma channel.
 * @param[in]  chn   - dma channel.
 * @return     none
 */
static inline void audio_tx_dma_en(dma_chn_e chn)
{
    dma_chn_en(chn);
}

/**
 * @brief      This function serves to disable dis_dma channel.
 * @param[in]  chn   - dma channel.
 * @return     none
 */
static inline void audio_tx_dma_dis(dma_chn_e chn)
{
    dma_chn_dis(chn);
}

/**
 * @brief      This function serves to get dma tx buff pointer.
 * @param[in]  chn - dma channel.
 * @return     the result of tx read pointer.
 */
static inline unsigned int audio_get_tx_dma_rptr(dma_chn_e chn)
{
    return reg_dma_src_addr(chn);
}

/**
 * @brief      This function serves to get dma rx buff pointer.
 * @param[in]  chn - dma channel.
 * @return     the result of rx write pointer.
 */
static inline unsigned int audio_get_rx_dma_wptr(dma_chn_e chn)
{
    return reg_dma_dst_addr(chn);
}

/**
 * @brief      This function serves to config rx_dma channel.
 * @param[in]  chn          - dma channel.
 * @param[in]  dst_addr     - Pointer to data buffer, it must be 4-bytes aligned address.
 *                            and the actual buffer size defined by the user needs to be not smaller than the data_len, otherwise there may be an out-of-bounds problem.
 * @param[in]  data_len     - Length of DMA in bytes, it must be set to a multiple of 4. The maximum value that can be set is 0x10000.
 * @param[in]  head_of_list - the head address of dma llp.
 * @return     none
 */
void audio_rx_dma_config(dma_chn_e chn, unsigned short *dst_addr, unsigned int data_len, dma_chain_config_t *head_of_list);

/**
 * @brief      This function serves to set rx dma chain transfer.
 * @param[in]  config_addr - the head of list of llp_pointer.
 * @param[in]  llpointer   - the next element of llp_pointer.
 * @param[in]  dst_addr    - Pointer to data buffer, it must be 4-bytes aligned address and the actual buffer size defined by the user needs to \n
 *                           be not smaller than the data_len, otherwise there may be an out-of-bounds problem.
 * @param[in]  data_len    - Length of DMA in bytes, it must be set to a multiple of 4. The maximum value that can be set is 0x10000.
 * @return     none
 */
void audio_rx_dma_add_list_element(dma_chain_config_t *config_addr, dma_chain_config_t *llpointer, unsigned short *dst_addr, unsigned int data_len);

/**
 * @brief      This function serves to set audio rx dma chain transfer.
 * @param[in]  rx_fifo_chn - rx fifo select.
 * @param[in]  chn         - dma channel.
 * @param[in]  in_buff     - Pointer to data buffer, it must be 4-bytes aligned address and the actual buffer size defined by the user needs to \n
 *                           be not smaller than the data_len, otherwise there may be an out-of-bounds problem.
 * @param[in]  buff_size   - Length of DMA in bytes, it must be set to a multiple of 4. The maximum value that can be set is 0x10000.
 * @return     none
 */
void audio_rx_dma_chain_init(audio_fifo_chn_e rx_fifo_chn, dma_chn_e chn, unsigned short *in_buff, unsigned int buff_size);

/**
 * @brief      This function serves to config  tx_dma channel.
 * @param[in]  chn          - dma channel.
 * @param[in]  src_addr     - Pointer to data buffer, it must be 4-bytes aligned address.
 * @param[in]  data_len     - Length of DMA in bytes, range from 1 to 0x10000.
 * @param[in]  head_of_list - the head address of dma llp.
 * @return     none
 */
void audio_tx_dma_config(dma_chn_e chn, unsigned short *src_addr, unsigned int data_len, dma_chain_config_t *head_of_list);

/**
 * @brief      This function serves to set tx dma chain transfer.
 * @param[in]  config_addr - the head of list of llp_pointer.
 * @param[in]  llpointer   - the next element of llp_pointer.
 * @param[in]  src_addr    - Pointer to data buffer, it must be 4-bytes aligned address.
 * @param[in]  data_len    - Length of DMA in bytes, range from 1 to 0x10000.
 * @return     none
 */
void audio_tx_dma_add_list_element(dma_chain_config_t *config_addr, dma_chain_config_t *llpointer, unsigned short *src_addr, unsigned int data_len);

/**
 * @brief      This function serves to initialize audio tx dma chain transfer.
 * @param[in]  tx_fifo_chn - tx fifo select.
 * @param[in]  chn         - dma channel.
 * @param[in]  out_buff    - Pointer to data buffer, it must be 4-bytes aligned address.
 * @param[in]  buff_size   - Length of DMA in bytes, range from 1 to 0x10000.
 * @return     none
 */
void audio_tx_dma_chain_init(audio_fifo_chn_e tx_fifo_chn, dma_chn_e chn, unsigned short *out_buff, unsigned int buff_size);

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio hac interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio hac interface.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio I2S interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio I2S interface.
 * @{
 */

/**
 * @brief      This function serves to set i2s clock.
 * @param[in]  i2s_select      - i2s channel select.
 * @param[in]  div_numerator   - the dividing factor of div_numerator bit[0-14] valid.
 * @param[in]  div_denominator - the dividing factor of div_denominator bit[0-15] valid.
 * @return     none
 */
static inline void audio_i2s_set_clk(i2s_select_e i2s_select, unsigned short div_numerator, unsigned short div_denominator)
{
    reg_audio_clk_i2s_step(i2s_select) = div_numerator & FLD_CLK_I2S_STEP;
    reg_audio_clk_i2s_mod(i2s_select) = div_denominator;
}

/**
 * @brief      This function serves to set the bclk divider.
 * @param[in]  i2s_select - i2s channel select.
 * @param[in]  div        - bclk = i2s_clk / (div * 2), if div = 0, i2s_clk = bclk.
 * @return     none
 */
static inline void audio_i2s_set_bclk(i2s_select_e i2s_select, unsigned char div)
{
    reg_audio_i2s_pcm_clk_num(i2s_select) = div;
}

/**
 * @brief      This function serves to set the i2s lrclk divider.
 * @param[in]  i2s_select - i2s channel select.
 * @param[in]  adc_div    - adc_lrclk = bclk / (adc_div).
 * @param[in]  dac_div    - dac_lrclk = bclk / (dac_div).
 * @return     none
 */
static inline void audio_i2s_set_lrclk(i2s_select_e i2s_select, unsigned short adc_div, unsigned short dac_div)
{
    reg_audio_i2s_int_pcm_num(i2s_select) = (adc_div - 1) & FLD_I2S_INT_PCM_NUM;
    reg_audio_i2s_dec_pcm_num(i2s_select) = (dac_div - 1) & FLD_I2S_DEC_PCM_NUM;
}

/**
 * @brief      This function serves to enable bclk and lr_clk.
 * @param[in]  i2s_select - i2s channel select
 * @return     none
 */
static inline void audio_i2s_clk_en(i2s_select_e i2s_select)
{
    BM_SET(reg_audio_i2s_cfg1(i2s_select), FLD_I2S_CLK_EN);
}

/**
 * @brief      This function serves to disable bclk and lr_clk
 * @param[in]  i2s_select - i2s channel select
 * @return     none
 */
static inline void audio_i2s_clk_dis(i2s_select_e i2s_select)
{
    BM_CLR(reg_audio_i2s_cfg1(i2s_select), FLD_I2S_CLK_EN);
}

/**
 * @brief      This function serves to set i2s schedule target value.
 * 
 * @param[in]  i2s_select   - i2s channel.
 * @param[in]  target_value - target value.
 * @return none
 */
static inline void audio_i2s_set_target_value(i2s_select_e i2s_select, unsigned int target_value)
{
    reg_audio_i2s_stimer_target(i2s_select) = target_value;
}

/**
 * @brief      This function serves to enable the i2s schedule.

 * @return    none
 */
static inline void audio_i2s_schedule_en(i2s_select_e i2s_select)
{
    BM_SET(reg_audio_i2s_route(i2s_select), FLD_I2S_SCHEDULE_EN);
}

/**
 * @brief      This function serves to disable the i2s schedule.
 * @return    none
 */
static inline void audio_i2s_schedule_dis(i2s_select_e i2s_select)
{
    BM_CLR(reg_audio_i2s_route(i2s_select), FLD_I2S_SCHEDULE_EN);
}

/**
 * @brief      This function serves to enable the i2s align function.
 * @return    none
 */
static inline void audio_i2s_align_en(void)
{
    BM_SET(reg_audio_i2s0_align_cfg, FLD_I2S_ALIGN_EN);
}

/**
 * @brief      This function serves to disable the i2s align function.
 * @return    none
 */
static inline void audio_i2s_align_dis(void)
{
    BM_CLR(reg_audio_i2s0_align_cfg, FLD_I2S_ALIGN_EN);
}

/**
 * @brief      This function serves to config i2s align mode.
 * 
 * @param[in]  align_config - i2s align config.
 * @return     none
 */
static inline void audio_i2s_align_config(i2s_align_config_t *align_config)
{
    reg_audio_i2s0_timer_th = align_config->align_th;
    reg_audio_i2s0_align_cfg = MASK_VAL(FLD_I2S_ALIGN_EN, 1, FLD_I2S_ALIGN_CTRL, align_config->align_mode, FLD_I2S_ALIGN_MASK, 0, FLD_I2S_CLK_SEL,
                                        align_config->align_clk);
}

/**
 * @brief      This function serves to initialize configuration i2s.
 * @param[in]  i2s_config - the relevant configuration struct pointer @see audio_i2s_config_t.
 * @return     none
 * @note
 *             - audio_pll default 36.864MHz. If the audio_pll is changed, the clk will be changed accordingly.
 */
void audio_i2s_config_init(audio_i2s_config_t *i2s_config);

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio matrix interface                                              *
 *********************************************************************************************************************/
/*!
 * @name Audio matrix interface.
 * @{
 */

/**
 * @brief      This function serves to select fifo rx route source and data format.
 *
 * @param[in]  fifo_num    - fifo channel.
 * @param[in]  route_from  - fifo rx route from.
 * @param[in]  data_format - fifo rx data format(route from i2s/anc/adc valid).
 * @return     none
 */
void audio_matrix_set_rx_fifo_route(audio_fifo_chn_e fifo_num, audio_matrix_rx_fifo_route_e route_from, audio_matrix_rx_fifo_format_e data_format);

/**
 * @brief   This function serves to select i2s tx route source and data format.
 *
 * @param[in]  i2s_tx_chn  - i2s tx channel.
 * @param[in]  route_from  - i2s tx route from.
 * @param[in]  data_format - i2s tx data format(route from fifo valid).
 * @return     none
 */
void audio_matrix_set_i2s_tx_route(audio_i2s_tx_chn_e i2s_tx_chn, audio_matrix_i2s_tx_route_e route_from, audio_matrix_i2s_tx_format_e data_format);

/**
 * @brief      This function serves to select dac route source and data format.
 *
 * @param[in]  dac_chn     - dac channel.
 * @param[in]  route_from  - dac route from.
 * @param[in]  data_format - dac data format(route from fifo valid).
 * @return     none
 */
void audio_matrix_set_dac_route(audio_codec0_output_select_e dac_chn, audio_matrix_dac_route_e route_from, audio_matrix_dac_format_e data_format);

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio pin interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio pin interface.
 * @{
 */

/**
 * @brief      This function serves to configure i2s pin.
 * @param[in]  i2s_select - channel select.
 * @param[in]  config     - i2s config pin struct.
 * @return     none
 */
void audio_i2s_set_pin(i2s_select_e i2s_select, i2s_pin_config_t *config);
/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio side_tone interface                                           *
 *********************************************************************************************************************/
/*!
 * @name Audio side_tone interface.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio spdif interface                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio spdif interface.
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio config/route interface                                        *
 *********************************************************************************************************************/
/*!
 * @name Audio config/route interface.
 * @{
 */

/**
 * @}
 */

#endif
