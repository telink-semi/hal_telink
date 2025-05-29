/********************************************************************************************************
 * @file    keyscan_ana.h
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
#ifndef KEYSCAN_ANA_H
#define KEYSCAN_ANA_H

#include "adc.h"
#include "dma.h"
#include "gpio.h"

#define KS_EVK_KEYBOARD             0 //2*16
#define KS_FULL_KEYBOARD            1 //8*16

#define KS_BOARD_TYPE               KS_EVK_KEYBOARD

/**
 * @brief keyscan mode
 * 
 */
typedef enum {
    KEYSCAN_1XADC_MODE               = 0,
    KEYSCAN_2XADC_MODE               = 1,
    KEYSCAN_1XADC_8K_ONCE_MODE       = 2,
    KEYSCAN_2XADC_8K_ONCE_MODE       = 3,
    KEYSCAN_2XADC_8K_TWICE_M1_MODE   = 4,
    KEYSCAN_2XADC_8K_TWICE_M2_MODE   = 5,//note, this mode will trigger twice interrupt
    KEYSCAN_MODE_MAX,
}ks_mode_e;

typedef struct
{
    unsigned char adc_simple;
    unsigned char setl_time;
    unsigned char setl_capture;
    unsigned char setl_capture1;
    unsigned char setl_time_pwr;
    unsigned char setl_time_scan;
    unsigned char setl_time_scan2;

    unsigned char pwr_on0;
    unsigned char pwr_on1;
    unsigned char pwr_on2;
    unsigned char pwr_on3;
    unsigned char pwr_on4;
    unsigned char pwr_on5;
    unsigned char pwr_on6;
    unsigned char pwr_on7;

    unsigned char pwr_dn0;
    unsigned char pwr_dn1;
    unsigned char pwr_dn2;
    unsigned char pwr_dn3;
    unsigned char pwr_dn4;
    unsigned char pwr_dn5;
    unsigned char pwr_dn6;
    unsigned char pwr_dn7;
} ks_config_t;

/**
 * @brief keyscan clock mode source 
 */
typedef enum
{
    KS_SRC_32K       = 0x01,
    KS_8K_SRC_RC24M  = 0x23,
    KS_8K_SRC_XTL24M = 0x43,
} ks_clock_mode_e;

/**
 * @brief the data length of a key with sar_adc selection
 * @note  this enumeration is for debugging EVK boards (2*16), for full keyboard set to 1
 */
typedef enum
{
#if (KS_BOARD_TYPE == KS_EVK_KEYBOARD)
    KS_ADC0_SCAN_ONCE_DATA_LEN    = 8,
    KS_ADC0_1_SCAN_ONCE_DATA_LEN  = 4,
    KS_ADC0_1_SCAN_TWICE_DATA_LEN = 8,
#elif (KS_BOARD_TYPE == KS_FULL_KEYBOARD)
    KS_ADC0_SCAN_ONCE_DATA_LEN    = 1,
    KS_ADC0_1_SCAN_ONCE_DATA_LEN  = 1,
    KS_ADC0_1_SCAN_TWICE_DATA_LEN = 1,
#endif
} ks_saradc_data_len_e;

/**
 * @brief the data size of a scan round with sar_adc selection (half word)
 */
typedef enum
{
    KS_ADC0_SCAN_ONCE_DATA_SIZE    = 128,
    KS_ADC0_1_SCAN_ONCE_DATA_SIZE  = 64,
    KS_ADC0_1_SCAN_TWICE_DATA_SIZE = 128,
} ks_saradc_data_size_e;

/**
 * @brief the channel and power pin map
 * 
 */
typedef enum
{
    KS_CHANNEL_POWER_PIN_0 = 0,
    KS_CHANNEL_POWER_PIN_1,
    KS_CHANNEL_POWER_PIN_2,
    KS_CHANNEL_POWER_PIN_3,
    KS_CHANNEL_POWER_PIN_TOTAL,
}ks_channel_power_pin_group_e;

/**
 * @brief    This function servers to clear the keyscan irq status.
 * @return   none.
 */
static inline void keyscan_ana_clr_irq_status(unsigned char irq)
{
    reg_ks_a_status |= irq;
}

/**
 * @brief    This function servers to get the keyscan irq status.
 * @return   the keyscan irq status.
 */
static inline unsigned char keyscan_ana_get_irq_status(void)
{
    return reg_ks_a_status & 0x03;
}

/**
 * @brief      This function serves to init keyscan analog module.
 * @param[in]  ks_mode          - the keyscan mode
 * @param[in]  ks_clock_mode    - the keyscan clock mode
 * @param[in]  ks_pin_group     - the keyscan pin group
 * @return     none
 */
void keyscan_ana_init(ks_mode_e ks_mode, ks_clock_mode_e ks_clock_mode, ks_channel_power_pin_group_e ks_pin_group);

/**
 * @brief      This function serves to get sar adc data with DMA.
 * @param[in]  chn              - the DMA channel
 * @param[in]  buffer_addr      - the DMA data_buffer address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc_data_dma(dma_chn_e chn, unsigned short* buffer_addr, unsigned int size_byte);

/**
 * @brief      This function serves to get sar adc data with DMA LLP.
 * @param[in]  chn              - the DMA channel
 * @param[in]  buffer_addr      - the DMA data_buffer address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc_data_dma_llp(dma_chn_e chn, unsigned short* buffer_addr, unsigned int size_byte);

/**
 * @brief      This function serves to get sar adc0 adc1 data with DMA.
 * @param[in]  ks_mode          - the Keyscan mode
 * @param[in]  chn0             - the DMA channel
 * @param[in]  chn1             - the DMA channel
 * @param[in]  data_buffer0     - the DMA data_buffer0 address
 * @param[in]  data_buffer1     - the DMA data_buffer1 address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc0_adc1_data_dma(ks_mode_e ks_mode, dma_chn_e chn0, dma_chn_e chn1, unsigned short *data_buffer0, unsigned short *data_buffer1, unsigned int size_byte);

/**
 * @brief      This function serves to get sar adc0 adc1 data with DMA LLP.
 * @param[in]  ks_mode          - the Keyscan mode
 * @param[in]  chn0             - the DMA channel
 * @param[in]  chn1             - the DMA channel
 * @param[in]  data_buffer0     - the DMA data_buffer0 address
 * @param[in]  data_buffer1     - the DMA data_buffer1 address
 * @param[in]  size_byte        - the DMA buffer size 
 * @return     none
 */
void keyscan_get_sar_adc0_adc1_data_dma_llp(ks_mode_e ks_mode, dma_chn_e chn0, dma_chn_e chn1, unsigned short *data_buffer0, unsigned short *data_buffer1, unsigned int size_byte);

/**
 * @brief      This function serves to get the keynumber from meta data.
 * @param[in]  ks_mode          - the keyscan mode
 * @param[in]  data             - the meta data
 * @param[in]  index            - the meta data index
 * @return     the keynumber
 */
int keyscan_get_keynumber_from_meta_data(ks_mode_e ks_mode, short *data, unsigned char index);
#endif
