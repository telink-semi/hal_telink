/********************************************************************************************************
 * @file    adc.h
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
/** @page ADC
 *
 *  Introduction
 *  ===============
 *  TL321X supports hardware ADC function.
 *
 *  API Reference
 *  ===============
 *  Header File: adc.h
 */
#pragma once

#include "dma.h"
#include "audio.h"
#include "compiler.h"
#include "gpio.h"
#include "reg_include/register.h"
#include "pem.h"

#ifndef INTERNAL_TEST_FUNC_EN
#define INTERNAL_TEST_FUNC_EN            0//only for internal test
#endif

extern unsigned char g_adc_rx_fifo_index[2];

/**
 *  @brief  Define ADC chn
 */
typedef enum
{
    ADC0 = 0,
    ADC1,
} adc_num_e;

/**
 * @brief pem event module.
 */
typedef enum{
    ADC_PEM_EVENT_RX_THRESHOLD,
    ADC_PEM_EVENT_RX_DATA_FIFO_WR,
}adc_pem_event_module_sel_e;

typedef enum{
    ADC_VREF_0P9V = 0x01,//Only for internal testing,not recommended.
    ADC_VREF_1P2V = 0x02,
}adc_ref_vol_e;

typedef enum{
    ADC_VBAT_DIV_OFF = 0,
    ADC_VBAT_DIV_1F4 = 0x01,
}adc_vbat_div_e;

typedef enum {
    NOINPUTN = 0,
    ADC_GPIO_PCPB0N = 0x01,
    ADC_GPIO_PCPB1N = 0x02,
    ADC_GPIO_PCPB2N = 0x03,
    ADC_GPIO_PCPB3N = 0x04,
    ADC_GPIO_PCPB4N = 0x05,
    ADC_GPIO_PCPB5N = 0x06,
    ADC_GPIO_PCPB6N = 0x07,
    ADC_GPIO_PCPB7N = 0x08,
    ADC_TEMPSENSORN_EE = 0x09,
    GND = 0x0b,
}adc_input_nch_e;

typedef enum {
    NOINPUTP = 0,
    ADC_GPIO_PCPB0P = 0x01,
    ADC_GPIO_PCPB1P = 0x02,
    ADC_GPIO_PCPB2P = 0x03,
    ADC_GPIO_PCPB3P = 0x04,
    ADC_GPIO_PCPB4P = 0x05,
    ADC_GPIO_PCPB5P = 0x06,
    ADC_GPIO_PCPB6P = 0x07,
    ADC_GPIO_PCPB7P = 0x08,
    ADC_TEMPSENSORP_EE = 0x09,
    ADC_VBAT = 0x0b,
}adc_input_pch_e;

/**
 * @brief adc input pin type
 * |           |              |
 * | :-------- | :----------- |
 * |   <15:12> |    <11:0>    |
 * |adc channel|    gpio pin  |
 */
typedef enum{
    ADC_GPIO_PB0 = GPIO_PB0 | (0x1<<12),
    ADC_GPIO_PB1 = GPIO_PB1 | (0x2<<12),
    ADC_GPIO_PB2 = GPIO_PB2 | (0x3<<12),
    ADC_GPIO_PB3 = GPIO_PB3 | (0x4<<12),
    ADC_GPIO_PB4 = GPIO_PB4 | (0x5<<12),
    ADC_GPIO_PB5 = GPIO_PB5 | (0x6<<12),
    ADC_GPIO_PB6 = GPIO_PB6 | (0x7<<12),
    ADC_GPIO_PB7 = GPIO_PB7 | (0x8<<12),
    
    ADC_GPIO_PC0 = GPIO_PC0 | (0x1<<12),
    ADC_GPIO_PC1 = GPIO_PC1 | (0x2<<12),
    ADC_GPIO_PC2 = GPIO_PC2 | (0x3<<12),
    ADC_GPIO_PC3 = GPIO_PC3 | (0x4<<12),
    ADC_GPIO_PC4 = GPIO_PC4 | (0x5<<12),
    ADC_GPIO_PC5 = GPIO_PC5 | (0x6<<12),
    ADC_GPIO_PC6 = GPIO_PC6 | (0x7<<12),
    ADC_GPIO_PC7 = GPIO_PC7 | (0x8<<12),
}adc_input_pin_def_e;

typedef enum{
    ADC_GPIO_MODE,
    ADC_VBAT_MODE,
}adc_input_pin_mode_e;

/**
 * @brief adc sample channel type.
 * @attention
 *  #1 Only M channel can be used in NDMA mode, multiple channels can be used in DMA mode.
 *  #2 When using one channels in DMA mode, only M channel can be selected.
 *  #3 When using two channels in DMA mode, only M and L channels can be selected.
 */
typedef enum{
    ADC_M_CHANNEL,
    ADC_L_CHANNEL,
    ADC_R_CHANNEL,
}adc_sample_chn_e;

/**
 * @brief adc channel cnt type.
*|             |             |
*| :--------   | :-----------|
*|   <7:4>     |    <3:0>    |
*|transfer_mode| channel_cnt |
**/
typedef enum{
    NDMA_M_CHN    = 1 | (0 << 4),
    DMA_M_CHN     = 1 | (1 << 4),
    DMA_M_L_CHN   = 2 | (1 << 4),
    DMA_M_L_R_CHN = 3 | (1 << 4),
}adc_chn_cnt_e;

/**
 * @brief enumeration of adc resolutions.
 * @note  #1 The adc performs best when set to ADC_RES12 by default, and it is not recommended to change this configuration.
 *        #2 The sign and valid bits corresponding to adc code for setting different resolutions are as follows:
 *           If ADC_RES8 is set, the sign bit of adc code is bit11 and bit0-6 are valid bits.
 *           If ADC_RES10 is set, the sign bit of adc code is bit11 and bit0-8 are valid bits.
 *           If ADC_RES12 is set, the sign bit of adc code is bit11 and bit0-10 are valid bits (default)
 *        #3 Regardless of the set resolution, the final output code value is always 16bit.
 */
typedef enum{
    ADC_RES8 = 0,
    ADC_RES10 = 0x01,
    ADC_RES12 = 0x02,
}adc_res_e;

typedef enum{
    ADC_SAMPLE_CYC_3,
    ADC_SAMPLE_CYC_6,
    ADC_SAMPLE_CYC_9,
    ADC_SAMPLE_CYC_12,
    ADC_SAMPLE_CYC_15,
    ADC_SAMPLE_CYC_18,
    ADC_SAMPLE_CYC_21,
    ADC_SAMPLE_CYC_24,
    ADC_SAMPLE_CYC_27,
    ADC_SAMPLE_CYC_30,
    ADC_SAMPLE_CYC_33,
    ADC_SAMPLE_CYC_36,
    ADC_SAMPLE_CYC_39,
    ADC_SAMPLE_CYC_42,
    ADC_SAMPLE_CYC_45,
    ADC_SAMPLE_CYC_48,
}adc_sample_cycle_e;
/**
 * @brief adc sample frequency type.
* |         |           |             |             |
* | :-------|  :--------| :--------   | :-----------|
* | <25:24> |  <23:8>   |   <7:4>     |    <3:0>    |
* |  flag   |  capture  | sample_cycle|     set     |
 *
 */
typedef enum{
    /**
     * In NDMA mode, configure the sample cycle corresponding to each sampling frequency to the maximum value and delay >30us after adc power on to wait for adc to stabilize,
     * to solve the problem of the first code abnormality after adc power on.(jira:BUT-8) (Confirmed by haitao.gu, added by xiaobin.huang at 20240903)
     */
    ADC_SAMPLE_FREQ_23K = (0<<24) |(1023<<8) |(ADC_SAMPLE_CYC_48<<4) | 15,
    ADC_SAMPLE_FREQ_48K = (1<<24) |(490<<8)  |(ADC_SAMPLE_CYC_48<<4) | 10,
    ADC_SAMPLE_FREQ_96K = (2<<24) |(240<<8)  |(ADC_SAMPLE_CYC_27<<4) | 10,
    ADC_SAMPLE_FREQ_192K= (3<<24) |(115<<8)  |(ADC_SAMPLE_CYC_6<<4)  | 10,
    ADC_SAMPLE_FREQ_1M  = (3<<24) |(45<<8)   |(14<<4)  | 3,
    ADC_SAMPLE_FREQ_2M  = (3<<24) |(21<<8)   |(1<<4)   | 2,//confirmed by shiyi
}adc_sample_freq_e;

typedef enum{
    ADC_PRESCALE_1   = 0x00,
//  ADC_PRESCALE_1F2 = 0x01,//Only for internal testing
    ADC_PRESCALE_1F4 = 0x02,
//    ADC_PRESCALE_1F8 = 0x03,//Only for internal testing
}adc_pre_scale_e;

typedef struct{
    adc_vbat_div_e divider;
    adc_ref_vol_e v_ref;
    adc_pre_scale_e pre_scale;
    adc_sample_freq_e sample_freq;
    adc_input_pch_e input_p;
    adc_input_nch_e input_n;
}adc_chn_cfg_t;

typedef struct{
    adc_ref_vol_e v_ref;
    adc_pre_scale_e pre_scale;
    adc_sample_freq_e sample_freq;
    adc_input_pin_def_e pin;
}adc_gpio_cfg_t;

typedef enum{
    DMA,
    NDMA,
}adc_transfer_mode_e;

typedef enum{
    FLD_CLOCK_RC,
    FLD_CLOCK_XTL,
    FLD_CLOCK_PLL,
}adcdig_clk;
/**********************************************************************************************************************
 *                                         DMA and NDMA common interface                                              *
 **********************************************************************************************************************/
/**
 * @brief    This function is used to power on sar_adc.
 * @return   none.
 * @note     -# User need to wait >30us after adc_power_on() for ADC to be stable.
 *           -# If you calling adc_power_off(), because all analog circuits of ADC are turned off after adc_power_off(),
 *            it is necessary to wait >30us after re-adc_power_on() for ADC to be stable.
 */
void adc_power_on(adc_num_e sar_adc_num);
/**
 * @brief      This function is used to power off sar_adc.
 * @return     none
 */
void adc_power_off(adc_num_e sar_adc_num);
/**
 * @brief This function serves to set input channel in differential_mode.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  p_ain - enum variable of ADC analog positive input pin.
 * @param[in]  n_ain - enum variable of ADC analog negative input pin.
 * @return none
 */
static inline void adc_set_diff_input(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_input_pch_e p_ain, adc_input_nch_e n_ain)
{
    analog_write_reg8(areg_adc_res_m(sar_adc_num) , analog_read_reg8(areg_adc_res_m(sar_adc_num)) | FLD_ADC_EN_DIFF_CHN_M);
    reg_adc_r_mux(sar_adc_num,chn) = (p_ain | (n_ain<<4)) ;

}
/**
 * @brief This function is used to set IO port for ADC supply or ADC IO port voltage sampling.
 * @param[in]  mode - ADC gpio pin sample mode
 * @param[in]  pin - adc_input_pin_def_e ADC input gpio pin
 * @return none
 */
void adc_pin_config(adc_input_pin_mode_e mode ,adc_input_pin_def_e pin);
/**
 * @brief This function is used to set two IO port configuration and set it as input channel of ADC difference IO port voltage sampling.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  p_pin - enum variable of ADC analog positive input IO.
 * @param[in]  n_pin - enum variable of ADC analog negative input IO.
 * @return none
 */
void adc_set_diff_pin(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_input_pin_def_e p_pin, adc_input_pin_def_e n_pin);

/**
 * @brief      This function serves to select Vbat voltage division factor.
 * @param[in]  chn - enum variable of ADC sample channel
 * @param[in]  vbat_div - enum variable of Vbat division factor.
 * @return     none
 */
void adc_set_vbat_divider(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_vbat_div_e vbat_div);

/**
 * @brief This function is used to initialize the ADC.
 * @param[in]  channel_cnt - transfer_mode and the number of channels used.
 * @return none
 * @attention Many features are configured in adc_init function. But some features
 *      such as adc digital clk, adc analog clk, resolution, we think better to set as default value,
 *      and user don't need to change them in most use cases.
 */
void adc_init(adc_num_e sar_adc_num,adc_chn_cnt_e channel_cnt);
/**
 * @brief This function is used to initialize the ADC for gpio sampling.
 * @param[in]  chn -the channel to be configured.
 * @param[in]  cfg -structure for configuring ADC channel.
 * @return none
 */
void adc_gpio_sample_init(adc_num_e sar_adc_num,adc_sample_chn_e chn , adc_gpio_cfg_t cfg);
/**
 * @brief This function is used to initialize the ADC for vbat sampling.
 * @param[in]  chn -structure for configuring ADC channel.
 * @return none
 */
void adc_vbat_sample_init(adc_num_e sar_adc_num,adc_sample_chn_e chn);
/**
 * @brief This function is used to initialize the ADC for keyscan sampling.
 * @param[in]  chn -structure for configuring ADC channel.
 * @return none
 */
void adc_keyscan_sample_init(adc_num_e sar_adc_num);

void adc_keyscan_sample_init_manual(adc_num_e sar_adc_num);
#if INTERNAL_TEST_FUNC_EN
/**
 * @brief This function is used to initialize the ADC for Temperature Sensor sampling.
 * @param[in]  chn -structure for configuring ADC channel.
 * @return none
 */
void adc_temp_init(adc_num_e sar_adc_num,adc_sample_chn_e chn);
/**
 * @brief This function serves to calculate temperature from temperature sensor adc sample code.
 * @param[in]   adc_code            - the temperature sensor adc sample code.
 * @return      adc_temp_value      - the temperature value.
 * @attention   Temperature and adc_code are linearly related. We test four chips between -40~130 (Celsius) and got an average relationship:
 *          Temp =  564 - ((adc_code * 819)>>11),when Vref = 1.2V, pre_scale = 1.
 */
unsigned short adc_calculate_temperature(adc_num_e sar_adc_num,unsigned short adc_code);
#endif

/**
 * @brief This function serves to calculate voltage from adc sample code.
 * @param[in]   chn - enum variable of ADC sample channel.
 * @param[in]   adc_code    - the adc sample code.
 * @return      adc_vol_mv  - the average value of adc voltage value.
 */
unsigned short adc_calculate_voltage(adc_sample_chn_e chn,unsigned short adc_code);
/**
 * @brief       This function sets the threshold that triggers the DMA IRQ or SAR ADC RX IRQ.
 * @param[in]   trig_num -trigger threshold.
 * @return      none
 * @note        -# The DMA IRQ or SAR ADC RX IRQ is triggered when fifo_cnt> trig_cnt.
 *              -# unit:half word, range:0-7(the actual fifo depth is 8 words, but the register is only 3 bits, so it is limited to 0~7).
 *              -# The data of rx fifo is stored according to HALF word.
 *              -# In DMA mode, DMA is configured to transmit by word, so the trigger cnt must be set to an odd number, otherwise there will be incorrect data to the buffer.
 *              -# In DMA mode, it has been configured by default to 1 (greater than 1 half word, i.e., 1 word triggers a DMA request to the DMA once ).
 *              -# In NDMA mode, the threshold can be configured according to the usage situation.
 *              -# The threshold is recalculated after the dma has finished moving.
 */
static inline void adc_set_rx_fifo_trig_cnt(adc_num_e sar_adc_num,unsigned char trig_num)
{
    reg_adc_rxfifo_trig_num(sar_adc_num) = ((reg_adc_rxfifo_trig_num(sar_adc_num) & (~FLD_RXFIFO_TRIG_NUM)) | trig_num);
}
/**********************************************************************************************************************
 *                                                DMA only interface                                                  *
 **********************************************************************************************************************/
/**
 * @brief      This function serves to configure adc_dma_chn channel.
 * @param[in]  chn - the DMA channel
 * @return     none
 */
void adc_set_dma_config(adc_num_e sar_adc_num,dma_chn_e chn);

/**
 * @brief     The adc starts sampling in DMA mode.
 * @param[in] adc_data_buf  - Pointer to data buffer, it must be 4-bytes aligned address
 *                            and the actual buffer size defined by the user needs to be not smaller than the data_byte_len,
 *                            otherwise there may be an out-of-bounds problem.
 * @param[in] data_byte_len - Amount of data to be sampled in bytes, the maximum value can be up to 0xFFFFFC.
 * @note      The depth of SAR_ADC_FIFO is 8 words, and its characteristic is store first, fetch first.
 *            When all three channels are working at the same time, the order of storing adc channel data into SAR_ADC_FIFO is M: L: R,
 *            so the arrangement after dma handling is also M: L: R.
 * @return    none
 */
void adc_start_sample_dma(adc_num_e sar_adc_num,unsigned short *adc_data_buf,unsigned int data_byte_len);

/**
 * @brief     This function serves to get adc DMA irq status.
 * @return    0: the sample is in progress.
 *            !0: the sample is finished.
 * @note      The code is placed in the ram code section, in order to shorten the time.
 */
_attribute_ram_code_sec_noinline_ unsigned char adc_get_irq_status_dma(void);

/**
 * @brief     This function serves to clear adc DMA irq status.
 * @return    none
 * @note      The code is placed in the ram code section, in order to shorten the time.
 */
_attribute_ram_code_sec_noinline_ void adc_clr_irq_status_dma(void);


/**********************************************************************************************************************
 *                                                NDMA only interface                                                 *
 **********************************************************************************************************************/
/**
 * @brief This function serves to directly get an adc sample code from analog registers.
 * @return  adc_code    - the adc sample code.
 * @note   If you want to get the sampling results twice in succession,
 *         must ensure that the sampling interval is more than 2 times the sampling period.
 */
unsigned short adc_get_code(adc_num_e sar_adc_num);

/**
 * @brief     Get the irq status of ADC.
 * @retval    non-zero      -  the interrupt occurred.
 * @retval    zero  -  the interrupt did not occur.
 */
static inline unsigned char adc_get_irq_status(adc_num_e sar_adc_num)
{
    return reg_soft_control(sar_adc_num) & FLD_SAR_IRQ_RX_STATUS;
}

/**
 * @brief     Clear the irq status of ADC.
 * @return    none
 * @note      The irq status of ADC will not be cleared automatically after reading out the data from the rxfifo, you need to clear manually.
 */
static inline void adc_clr_irq_status(adc_num_e sar_adc_num)
{
    reg_soft_control(sar_adc_num) |= FLD_SAR_IRQ_RX_STATUS;
}

/**
 * @brief   This function is used to enable the transmission of data from the adc's M channel, L channel, and R channel to the sar adc rxfifo.
 * @return  none
 */
void adc_start_sample_nodma(adc_num_e sar_adc_num);

/**
 * @brief   This function is used to disable the transmission of data from the adc's M channel, L channel, and R channel to the sar adc rxfifo and clear rx fifo cnt and g_adc_rx_fifo_index.
 * @return  none
 */
void adc_stop_sample_nodma(adc_num_e sar_adc_num);

/**
 * @brief     Get the rxfifo cnt,when data enters rxfifo, the rxfifo cnt increases; when reading data from rx_fifo, rxfifo cnt decays.
 * @return    none
 * @note      After waking up from suspend mode,rxfifo will not be cleared.(confirmed by qiangkai.xu, added by xiaobin.huang at 20240903)
 */
static inline unsigned char adc_get_rxfifo_cnt(adc_num_e sar_adc_num)
{
    return ((reg_adc_rxfifo_trig_num(sar_adc_num) & FLD_BUF_CNT)>>4);
}

/**
 * @brief     Set the 'g_adc_rx_fifo_index' to 0,'g_adc_rx_fifo_index' is used to synchronize the rxfifo hardware pointer in NDMA mode.
 * @return    none.
 */
static inline void adc_clr_rx_index(adc_num_e sar_adc_num)
{
    g_adc_rx_fifo_index[sar_adc_num] = 0;
}



/**
 * @brief     Clear the ADC rx fifo.
 * @return    none
 */
static inline void adc_clr_rx_fifo_cnt(adc_num_e sar_adc_num)
{
    reg_soft_control(sar_adc_num) |= FLD_FIFO_CLR;
    adc_clr_rx_index(sar_adc_num);
}
/**
* @brief      This function enable adc source clock: xtal_24M
* @return     none
*/
static inline void adc_select_clk(adc_num_e sar_adc_num,adcdig_clk adc_clk,char div)
{
    reg_adcdig_clk_set(sar_adc_num)= (reg_adcdig_clk_set(sar_adc_num) & ~FLD_ADCDIG_DIV_IN_SEL) | ((adc_clk & 0x03) << 4);
    reg_adcdig_clk_set(sar_adc_num) = (reg_adcdig_clk_set(sar_adc_num) & ~FLD_ADCDIG_CLK_MOD) | (div & 0x0F);
}


/**
 * @brief      This function disable oversample mode.
 * @return     none
 * @note       Hardware averaging mode
 */
static inline void adc_ovrsmpl_dis(adc_num_e sar_adc_num)
{
    reg_adc_ovrsmpl(sar_adc_num) &= (~FLD_OVRSMPL_ENABLE);
}
/**
 * @brief      This function enable oversample mode.
 * @return     none
 * @note       Hardware averaging mode
 */
static inline void adc_ovrsmpl_en(adc_num_e sar_adc_num)
{
    reg_adc_ovrsmpl(sar_adc_num) |= FLD_OVRSMPL_ENABLE;
}
/**
 * @brief      This function disable trigger mode.
 * @return     none
 * @note       Manual trigger required
 */
static inline void adc_trigger_dis(adc_num_e sar_adc_num)
{
    reg_adc_config2(sar_adc_num) &= (~FLD_TRIG_MODE);
}
/**
 * @brief      This function enable trigger mode.
 * @return     none
 * @note       Manual trigger required
 */
static inline void adc_trigger_en(adc_num_e sar_adc_num)
{
    reg_adc_config2(sar_adc_num) |= FLD_TRIG_MODE;
}

/**
 * @brief      This function enable keyscan mode.
 * @return     none
 * @note       auto keyscan required
 */
static inline void adc_keyscan_auto_en(adc_num_e sar_adc_num)
{
    reg_adc_ovrsmpl(sar_adc_num) |= FLD_PAD_AUTO_MUX_EN;
}

/**
 * @brief      This function start trigger.
 * @return     none
 * @note       Start collecting adc data
 */
static inline void adc_trigger_start(adc_num_e sar_adc_num)
{
    reg_soft_control(sar_adc_num) = FLD_TRIG_START;
}
/**
 * @brief      This function start collect adc data.
 * @return     none
 * @note       Start collecting an adc data
 */
static inline void adc_software_start(adc_num_e sar_adc_num)
{
    reg_soft_control(sar_adc_num) = FLD_SOFT_START;
}
/**
 * @brief      This function servers to configure PEM channel and some configures.
 * @param[in]  chn - to select the PEM channel.
 * @param[in]  event_signal - to select the event signal.
 * @return     none.
 */
void adc_pem_event(adc_num_e sar_adc_num,pem_chn_e chn,unsigned char adc_sel);
/**
 * @brief      This function servers to configure PEM channel and some configures.
 * @param[in]  chn - to select the PEM channel.
 * @return     none.
 */
void adc_pem_task(adc_num_e sar_adc_num,pem_chn_e chn);
/**
 * @brief      This function sets adc digital clock and analog clock.
 * @param      div - Analog adc frequency division coefficient.
 * @return     none
 * @attention  ADC digital clock from crystal(24MHz), analog clock from digital clock.
 *             The ADC digital clock and analog clock are fixed at 24 MHz and 4 MHz, respectively, and should not be modified.
 */
void adc_set_clk(adc_num_e sar_adc_num,char div);
/**
 * @brief      This function enables the trigger mode and sets the number of data collected by the trigger at a time.
 * @param[in]  cnt - Set cnt data to be averaged once.
 * @return     none
 */
void adc_trigger_set(adc_num_e sar_adc_num,char cnt);
/**
 * @brief       This function serves to enable software mode to collect data one time at a time.
 * @return      none
 * @note        This function requires adc_software_start to trigger and collect an adc data.
 */
void adc_software_enable(adc_num_e sar_adc_num);
/**
 * @brief       This function serves to disable software mode.
 * @return      none
 * @note        This function requires adc_software_start to trigger and collect an adc data.
 */
void adc_software_disable(adc_num_e sar_adc_num);
/**
 * @brief      This function serves to turn on hardware averaging mode and set multiple acquisition data averaging
 * @param[in]  cnt - Set trigger to collect cnt data at a time
 * @return     none
 */
void adc_ovrsmpl_set(adc_num_e sar_adc_num,char cnt);
