/********************************************************************************************************
 * @file    adc.c
 *
 * @brief   This is the source file for tl322x
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
#include "adc.h"
#include "audio.h"
#include "compiler.h"
#include "lib/include/stimer.h"
/**********************************************************************************************************************
 *                                              local macro                                                        *
 *********************************************************************************************************************/
#define  ADC_BIT_POSITION(byte)    (byte ? __builtin_ctz(byte) : -1)
#define  ADC_CHN_CNT          3
/**
 * Note: When the reference voltage is configured to 1.2V, the calculated ADC voltage value is closest to the actual voltage value using 1175 as the coefficient default.
 * 1175 is the value obtained by ATE through big data statistics, which is more in line with most chips than 1200.
 */
_attribute_data_retention_sec_ unsigned short g_adc_vref[ADC_CHN_CNT] = {1175,1175,1175}; //default ADC ref voltage (unit:mV)
_attribute_data_retention_sec_ signed char g_adc_vref_offset[ADC_CHN_CNT];//ADC calibration value voltage offset (unit:mV).

volatile unsigned char g_adc_pre_scale[ADC_CHN_CNT];
volatile unsigned char g_adc_vbat_divider[ADC_CHN_CNT];
unsigned char g_adc_rx_fifo_index[2];
unsigned char g_channel_cnt = 0;


dma_chn_e adc_dma_chn;
dma_config_t adc_rx_dma_config=
{
    .dst_req_sel= 0,
    .src_req_sel=DMA_REQ_SAR_ADC_RX,
    .dst_addr_ctrl=DMA_ADDR_INCREMENT,
    .src_addr_ctrl=DMA_ADDR_FIX,
    .dstmode=DMA_NORMAL_MODE,
    .srcmode=DMA_HANDSHAKE_MODE,
    .dstwidth=DMA_CTR_WORD_WIDTH,//must word
    .srcwidth=DMA_CTR_WORD_WIDTH,//must word
    .src_burst_size=0,//must 0
    .read_num_en=0,
    .priority=0,
    .write_num_en=0,
    .auto_en=0,//must 0
};
dma_config_t adc1_rx_dma_config=
{
    .dst_req_sel= 0,
    .src_req_sel=DMA_REQ_SAR_ADC1_RX,
    .dst_addr_ctrl=DMA_ADDR_INCREMENT,
    .src_addr_ctrl=DMA_ADDR_FIX,
    .dstmode=DMA_NORMAL_MODE,
    .srcmode=DMA_HANDSHAKE_MODE,
    .dstwidth=DMA_CTR_WORD_WIDTH,//must word
    .srcwidth=DMA_CTR_WORD_WIDTH,//must word
    .src_burst_size=0,//must 0
    .read_num_en=0,
    .priority=0,
    .write_num_en=0,
    .auto_en=0,//must 0
};
/**
 * @brief      This setting serves to set the configuration of stimer PEM event.
 */
pem_event_config_t adc_pem_event_config={
            .module         = PEM_EVENT_SAR_ADC,
            .sig_sel        = 0,
            .clk_sel        = PCLK,
            .lvl            = PULSE,
            .edge_detect    = RISING_EDGE,
            .inv            = 0,
};

/**
 * @brief      This setting serves to set the configuration of stimer PEM task.
 */
pem_task_config_t adc_pem_task_config={
            .module         = PEM_TASK_SAR_ADC,
            .sig_sel        = 0,
            .clk_sel        = PCLK,
            .lvl            = PULSE,
};

/**********************************************************************************************************************
 *                                         DMA and NDMA common interface                                              *
 **********************************************************************************************************************/
/**
 * @brief      This function is used to reset sar_adc module.
 * @return     none
 */
static inline void adc_reset(adc_num_e sar_adc_num)
{
    if(sar_adc_num == 0)
    {
        reg_rst3 &= (~FLD_RST3_SARADC );
        reg_rst3 |=FLD_RST3_SARADC;
    }
    else
    {
        reg_rst5 &= (~FLD_RST5_SAR1ADC );
        reg_rst5 |=FLD_RST5_SAR1ADC;
    }

    adc_clr_rx_index(sar_adc_num);

}

/**
 * @brief      This function enable adc source clock: xtal_24M
 * @return     none
 */
static inline void adc_clk_en(adc_num_e sar_adc_num)
{
    if(sar_adc_num == 0)
    {

        reg_clk_en3 |= FLD_CLK3_SARADC_EN;
    }
    else
    {

        reg_clk_en5 |= FLD_CLK5_SARADC1_EN;
    }

}

/**
 * @brief      This function sets ADC input channel as M channel,L channel or R channel.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @return     none
 */
static inline void adc_set_chn_en(adc_num_e sar_adc_num,adc_sample_chn_e chn)
{
    reg_adc_config2(sar_adc_num) |= BIT(chn);
}
/**
 * @brief This function serves to set resolution.
 * @param[in]  res - enum variable of ADC resolution.
 * @return none
 */
static inline void adc_set_resolution(adc_num_e sar_adc_num,adc_res_e res)
{
    analog_write_reg8(areg_adc_res_m(sar_adc_num), (analog_read_reg8(areg_adc_res_m(sar_adc_num) )&(~FLD_ADC_RES_M)) | res);
}
/**
 * @brief      This function serves to set ADC sample time(the number of adc clocks for sample cycles) for M channel,L channel or R channel.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  sample_cycle - enum variable of adc sample cycles.
 * @return     none
 */
static void adc_set_tsample_cycle(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_sample_cycle_e sample_cycle)
{
    switch(chn)
    {
        case ADC_M_CHANNEL :
            reg_adc_tsamp(sar_adc_num) = ((reg_adc_tsamp(sar_adc_num) & (~FLD_M_TSAMP)) | sample_cycle);
            break;
        case ADC_L_CHANNEL :
            reg_adc_tsamp(sar_adc_num) = ((reg_adc_tsamp(sar_adc_num) & (~FLD_L_TSAMP)) | (sample_cycle<<4));
            break;
        case ADC_R_CHANNEL :
            reg_adc_r_tsamp(sar_adc_num) = ((reg_adc_r_tsamp(sar_adc_num) & (~FLD_R_TSAMP)) | sample_cycle);
            break;
        default:
            break;
    }
}
/**
 * @brief This function serves to set state and capture_state length.
 * @param[in]   chn - enum variable of ADC sample channel.
 * @param[in]   r_max_mc - Value of length of "capture" state for channel.
 * @param[in]   r_max_s - Value of length of "set" state for channel.
 * @return none
 */
static inline void adc_set_state_length(adc_num_e sar_adc_num,adc_sample_chn_e chn,unsigned short r_max_mc,unsigned char r_max_s)
{
    reg_adc_capture_state(sar_adc_num,chn) = r_max_mc;
    reg_adc_channel_set_state(sar_adc_num,chn) = ((reg_adc_channel_set_state(sar_adc_num,chn)&(~FLD_R_MAX_S)) | r_max_s);
}
/**
 * @brief   This function is used to enable the transmission of data from the adc's M channel, L channel, and R channel to the sar adc rxfifo.
 * @return  none
 * @note    -# In DMA mode, must enable this function.
 *          -# In NDMA mode, if this function is not enabled, the adc code can only be read from the registers, but it will lead to the problem of repeatedly getting the same adc code when calling adc_get_code() several times,
 *             if this function is enabled, the adc code can be read from the rx fifo, and this problem can be avoided, so it is also must enable it.
 */
static inline void adc_all_chn_data_to_fifo_en(adc_num_e sar_adc_num)
{
    analog_write_reg8(areg_adc_data_transfer_control(sar_adc_num) ,analog_read_reg8(areg_adc_data_transfer_control(sar_adc_num) ) & (~FLD_AUTO_NOT_EN));
}

/**
 * @brief   This function is used to disable the transmission of data from the adc's M channel, L channel, and R channel to the sar adc rxfifo.
 * @return  none
 */
static inline void adc_all_chn_data_to_fifo_dis(adc_num_e sar_adc_num)
{
    analog_write_reg8(areg_adc_data_transfer_control(sar_adc_num),FLD_AUTO_NOT_EN);
}
/**
 * @brief       This function is used to set the scan channel cnt.
 * @param[in]   chn_cnt - range:1-3
 * @return      none
 */
static inline void adc_set_scan_chn_cnt(adc_num_e sar_adc_num,unsigned char chn_cnt)
{
    reg_adc_config0(sar_adc_num) = ((reg_adc_config0(sar_adc_num) & (~FLD_SCANT_MAX)) | ((chn_cnt * 2)<< 4));//scan_cnt = chn_cnt*2

}

/**
 * @brief      This function disable adc digital clock.
 * @return     none
 * @note       -# After disabling the digital clock, rxfifo will not be updated.init
 *             -# When you don't need to keep the adc sampling, you can disable the digital clock for power saving purposes.
 */
static inline void adc_dig_clk_dis(adc_num_e sar_adc_num)
{
    reg_adc_config2(sar_adc_num) &= (~FLD_CLK_EN);
}
/**
 * @brief      This function enable adc digital clock.
 * @return     none
 */
static inline void adc_dig_clk_en(adc_num_e sar_adc_num)
{
    reg_adc_config2(sar_adc_num) |= FLD_CLK_EN;

}

/**
 * @brief      This function serves to stop the state machine at the beginning of the M channel and not start sampling.
 * @return     none
 */
static inline void adc_set_scan_chn_dis(adc_num_e sar_adc_num)
{
    /**
     * FLD_SCANT_MAX cannot be configured to 0, because 0 is the RNG channel, if RNG is not enabled (default is not enabled), the state machine will not stop at the RNG channel after setting 0, it will not stop.
     */
    reg_adc_config0(sar_adc_num) = ((reg_adc_config0(sar_adc_num) & (~FLD_SCANT_MAX)) | (1 << 4));
}
/**
 * @brief    This function is used to power on sar_adc.
 * @return   none.
 * @note     -# User need to wait >30us after adc_power_on() for ADC to be stable.
 *           -# If you calling adc_power_off(), because all analog circuits of ADC are turned off after adc_power_off(),
 *            it is necessary to wait >30us after re-adc_power_on() for ADC to be stable.
 */
void adc_power_on(adc_num_e sar_adc_num)
{
    adc_set_scan_chn_dis(sar_adc_num);
    adc_reset(sar_adc_num);
    adc_set_scan_chn_cnt(sar_adc_num, 1);
    analog_write_reg8(areg_adc_pga_ctrl(sar_adc_num), (analog_read_reg8(areg_adc_pga_ctrl(sar_adc_num))&(~FLD_SAR_ADC_POWER_DOWN)));
    adc_dig_clk_en(sar_adc_num);
}
/**
 * @brief      This function is used to power off sar_adc.
 * @return     none
 */
void adc_power_off(adc_num_e sar_adc_num)
{
    adc_dig_clk_dis(sar_adc_num);
    analog_write_reg8(areg_adc_pga_ctrl(sar_adc_num), (analog_read_reg8(areg_adc_pga_ctrl(sar_adc_num))|FLD_SAR_ADC_POWER_DOWN));

}
/**
 * @brief This function is used to set IO port for ADC supply or ADC IO port voltage sampling.
 * @param[in]  mode - ADC gpio pin sample mode
 * @param[in]  pin - adc_input_pin_def_e ADC input gpio pin
 * @return none
 */
void adc_pin_config(adc_input_pin_mode_e mode ,adc_input_pin_def_e pin)
{
    unsigned short adc_input_pin = pin & 0xfff;
    switch(mode)
    {
        case ADC_GPIO_MODE :
            gpio_function_en(adc_input_pin);
            gpio_input_dis(adc_input_pin);
            gpio_output_dis(adc_input_pin);
            gpio_set_low_level(adc_input_pin);
            break;
        case ADC_VBAT_MODE :
            gpio_function_en(adc_input_pin);
            gpio_input_dis(adc_input_pin);
            gpio_output_en(adc_input_pin);
            gpio_set_high_level(adc_input_pin);
            break;
    }
}
/**
 * @brief This function is used to set two IO port configuration and set it as input channel of ADC difference IO port voltage sampling.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  p_pin - enum variable of ADC analog positive input IO.
 * @param[in]  n_pin - enum variable of ADC analog negative input IO.
 * @return none
 */
void adc_set_diff_pin(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_input_pin_def_e p_pin, adc_input_pin_def_e n_pin)
{
    adc_pin_config(ADC_GPIO_MODE, p_pin);
    adc_pin_config(ADC_GPIO_MODE, n_pin);
    adc_set_diff_input(sar_adc_num, chn,p_pin >> 12, n_pin >> 12);
}

/**
 * @brief This function serves to set the reference voltage of the channel.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  v_ref - enum variable of ADC reference voltage.
 * @return none
 * @note       adc_set_ref_voltage does not take effect immediately after configuration, it needs to be delayed 30us after calling adc_dig_clk_en().
 */
static void adc_set_ref_voltage(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_ref_vol_e v_ref)
{
    reg_adc_channel_set_state(sar_adc_num,chn) = (reg_adc_channel_set_state(sar_adc_num,chn)&(~FLD_SEL_VREF)) | (v_ref<<6);
    if(v_ref == ADC_VREF_1P2V)
    {
        //Vref buffer bias current trimming:        150%
        //Comparator preamp bias current trimming:  100%
        analog_write_reg8(areg_ain_scale(sar_adc_num)  , (analog_read_reg8(areg_ain_scale(sar_adc_num)  )&(0xC0)) | 0x1d );
        g_adc_vref[chn] = 1175;// v_ref = ADC_VREF_0P9V,
    }
    else if(v_ref == ADC_VREF_0P9V)
    {
        //Vref buffer bias current trimming:        100%
        //Comparator preamp bias current trimming:  100%
        analog_write_reg8(areg_ain_scale(sar_adc_num)  , (analog_read_reg8(areg_ain_scale(sar_adc_num))&(0xC0)) | 0x15 );
        g_adc_vref[chn] = 900;// v_ref = ADC_VREF_0P9V,
    }
}
/**
 * @brief This function serves to set the sample frequency.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  sample_freq - enum variable of ADC sample frequency.
 * @return none
 */
static void adc_set_sample_rate(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_sample_freq_e sample_freq)
{
    unsigned char adc_set = sample_freq & 0xf;
    unsigned char adc_sample_cycle = (sample_freq>>4) & 0xf;
    unsigned short adc_capture = (sample_freq>>8) & 0xffff;
    adc_set_state_length(sar_adc_num,chn,adc_capture, adc_set);
    adc_set_tsample_cycle(sar_adc_num,chn,adc_sample_cycle);
}
/**
 * @brief This function serves to set pre_scaling factor.
 * @param[in]  chn - enum variable of ADC sample channel.
 * @param[in]  pre_scale - enum variable of ADC pre_scaling factor.
 * @return none
 */
static inline void adc_set_scale_factor(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_pre_scale_e pre_scale)
{
    reg_adc_channel_set_state(sar_adc_num,chn) = ((reg_adc_channel_set_state(sar_adc_num,chn)&(~FLD_SEL_AI_SCALE)) | (pre_scale<<4));
    g_adc_pre_scale[chn] = 1<<pre_scale;
}
/**
 * @brief      This function serves to select Vbat voltage division factor.
 * @param[in]  chn - enum variable of ADC sample channel
 * @param[in]  vbat_div - enum variable of Vbat division factor.
 * @return     none
 * @note       adc_set_vbat_divider() does not take effect immediately after configuration, it needs to be delayed 30us after calling adc_dig_clk_en().
 */
void adc_set_vbat_divider(adc_num_e sar_adc_num,adc_sample_chn_e chn,adc_vbat_div_e vbat_div)
{
    unsigned char offset = (chn==0) ? 0 : (1<<chn);

    reg_adc_vabt_div(sar_adc_num) = (reg_adc_vabt_div(sar_adc_num) & (~BIT_RNG(offset,offset+1)))| ((vbat_div) << offset);
    g_adc_vbat_divider[chn] = vbat_div ? (5 - vbat_div) : 1;
}
/**
 * @brief       This function is used to enable the status of the valid adc code for the m channel.
 * @return      none
 * @attention   The adc_ana_read_en() API must be called before the adc_get_m_chn_valid_status()API.
 * @note        This function is used in NDMA mode where adc_get_m_chn_valid_status() needs to be called.
 */
static inline void adc_ana_read_en(adc_num_e sar_adc_num)
{
    analog_write_reg8(areg_adc_data_sample_control(sar_adc_num),  analog_read_reg8(areg_adc_data_sample_control(sar_adc_num)) | FLD_ANA_RD_EN);
}

/**
 * @brief This function is used to initialize the ADC.
 * @param[in]  channel_cnt - transfer_mode and the number of channels used.
 * @return none
 * @attention Many features are configured in adc_init function. But some features
 *      such as adc digital clk, adc analog clk, resolution, we think better to set as default value,
 *      and user don't need to change them in most use cases.
 */
void adc_init(adc_num_e sar_adc_num,adc_chn_cnt_e channel_cnt)
{
    adc_power_off(sar_adc_num);//power off sar adc
    adc_reset(sar_adc_num);
    adc_clk_en(sar_adc_num);
    adc_set_resolution(sar_adc_num,ADC_RES12);//default adc_resolution set as 12bit ,BIT(11) is sign bit
    if(NDMA_M_CHN == channel_cnt)
    {
        adc_all_chn_data_to_fifo_dis(sar_adc_num);
        reg_adc_config2(sar_adc_num) &= ~FLD_RX_DMA_ENABLE;//In NDMA mode,RX DMA needs to be disabled.
        reg_adc_config2(sar_adc_num) |= FLD_SAR_RX_INTERRUPT_ENABLE;//SAR_RX_INTERRUPT must be enabled to call adc_get_irq_status() to get adc irq status.
    }

    /**
     * The set and capture of RNG channel are configured to 0 by default, and the actual state machine scanning time of RNG channel is the maximum time(about 25us),
     * and by configuring both of them to 1 (the minimum scanning time), the state machine scanning time of RNG channel is only (1+1)/24M=83ns,
     * which enables the state machine to enter into the set state of the M channel faster, and prevents the first code of M channel abnormality.
     */
    reg_adc_rng_set_state(sar_adc_num)     = 0x00;
    reg_adc_rng_capture_state(sar_adc_num) = 0x01;

    g_channel_cnt = channel_cnt;
}
/**
 * @brief      This function serves to turn on hardware averaging mode and set multiple acquisition data averaging
 * @param[in]  cnt - Set trigger to collect cnt data at a time
 * @return     none
 */
void adc_ovrsmpl_set(adc_num_e sar_adc_num,char cnt)
{
    adc_ovrsmpl_en(sar_adc_num);
    reg_adc_ovrsmpl(sar_adc_num) = ((reg_adc_ovrsmpl(sar_adc_num) & (~FLD_MAX_OVRSMPL_CNT)) | cnt);
}
/**
 * @brief      This function enables the trigger mode and sets the number of data collected by the trigger at a time.
 * @param[in]  cnt - Set cnt data to be averaged once.
 * @return     none
 */
void adc_trigger_set(adc_num_e sar_adc_num,char cnt)
{
    adc_trigger_en(sar_adc_num);
    reg_sample_times(sar_adc_num) =  cnt;
}

/**
 * @brief This function is used to configure the channel of the ADC.
 * @param[in]  chn -the channel to be configured.
 * @param[in]  adc_cfg -structure for configuring ADC channel.
 * @return none
 */
void adc_chn_config(adc_num_e sar_adc_num,adc_sample_chn_e chn , adc_chn_cfg_t adc_cfg)
{
    adc_set_diff_input(sar_adc_num,chn, adc_cfg.input_p, adc_cfg.input_n);

    adc_set_vbat_divider(sar_adc_num,chn, adc_cfg.divider);
    adc_set_ref_voltage(sar_adc_num,chn, adc_cfg.v_ref);
    adc_set_scale_factor(sar_adc_num, chn, adc_cfg.pre_scale);
    adc_set_sample_rate(sar_adc_num, chn, adc_cfg.sample_freq);
    adc_set_chn_en(sar_adc_num, chn);
    if(adc_cfg.sample_freq !=  ADC_SAMPLE_FREQ_2M && adc_cfg.sample_freq !=  ADC_SAMPLE_FREQ_1M )
    {
        adc_select_clk(sar_adc_num,FLD_CLOCK_XTL,0x01);
        adc_set_clk(sar_adc_num,5);//set adc digital clk to 24MHz and adc analog clk to 4MHz
    }
    else
    {
        adc_select_clk(sar_adc_num,FLD_CLOCK_PLL,0x04);
        adc_set_clk(sar_adc_num,0);//set adc digital clk to 24MHz and adc analog clk to 4MHz
    }
}
/**
 * @brief This function is used to initialize the ADC for gpio sampling.
 * @param[in]  chn -the channel to be configured.
 * @param[in]  cfg -structure for configuring ADC channel.
 * @return none
 */
void adc_gpio_sample_init(adc_num_e sar_adc_num,adc_sample_chn_e chn , adc_gpio_cfg_t cfg)
{
    adc_pin_config(ADC_GPIO_MODE, cfg.pin);
    adc_chn_cfg_t chn_cfg =
    {
        .divider = ADC_VBAT_DIV_OFF,
        .v_ref = cfg.v_ref,
        .pre_scale = cfg.pre_scale,
        .sample_freq = cfg.sample_freq,
        .input_p = cfg.pin >> 12,
        .input_n = GND,
    };
    adc_chn_config(sar_adc_num, chn, chn_cfg);
    analog_write_reg8(areg_adc_verf_fast_startup_sampled_inuput(sar_adc_num),FLD_VERF_FAST_STARTUP|FLD_SAMPLED_INPUT_MODEBAR);
}

/**
 * @brief This function is used to initialize the ADC for vbat sampling.
 * @param[in]  chn -structure for configuring ADC channel.
 * @return none
 */
void adc_vbat_sample_init(adc_num_e sar_adc_num,adc_sample_chn_e chn)
{
    adc_chn_cfg_t chn_cfg =
    {
        .divider = ADC_VBAT_DIV_1F4,
        .v_ref = ADC_VREF_1P2V,
        .pre_scale = ADC_PRESCALE_1,
        .sample_freq = ADC_SAMPLE_FREQ_96K,
        .input_p = ADC_VBAT,
        .input_n = GND,

    };
    adc_chn_config(sar_adc_num, chn, chn_cfg);
    analog_write_reg8(areg_adc_verf_fast_startup_sampled_inuput(sar_adc_num),FLD_VERF_FAST_STARTUP|FLD_SAMPLED_INPUT_MODEBAR);
}

/**
 * @brief This function is used to initialize the ADC for keyscan sampling.
 * @param[in]  chn -structure for configuring ADC channel.
 * @return none
 */
void adc_keyscan_sample_init(adc_num_e sar_adc_num)
{
    adc_chn_cfg_t chn_cfg =
    {
        .divider = ADC_VBAT_DIV_OFF,
        .v_ref = ADC_VREF_1P2V,
        .pre_scale = ADC_PRESCALE_1F4,
        .sample_freq = ADC_SAMPLE_FREQ_2M,
        .input_p = ADC_GPIO_PCPB0P,
        .input_n = GND,
    };

    adc_chn_config(sar_adc_num, ADC_M_CHANNEL, chn_cfg);
    adc_keyscan_auto_en(sar_adc_num);
    adc_trigger_set(sar_adc_num, 1);

    if (sar_adc_num == ADC0) {
        reg_pad_auto_p(ADC0) = 0x87654321;
        reg_pad_auto_n(ADC0) = 0xbbbbbbbb;
    } else if (sar_adc_num == ADC1) {
        reg_pad_auto_p(ADC0) = 0x43214321;
        reg_pad_auto_n(ADC0) = 0xbbbbbbbb;//GND

        reg_pad_auto_p(ADC1) = 0x43214321;
        reg_pad_auto_n(ADC1) = 0x88888888;//PB7->GND
    }

    analog_write_reg8(areg_adc_verf_fast_startup_sampled_inuput(sar_adc_num), FLD_VERF_FAST_STARTUP | FLD_SAMPLED_INPUT_MODEBAR); //Quickly start the ADC
}

void adc_keyscan_sample_init_manual(adc_num_e sar_adc_num)
{
    reg_rst3 |= FLD_RST3_SARADC;
    reg_clk_en3 |= FLD_CLK3_SARADC_EN;

    reg_rst5 |= FLD_RST5_SAR1ADC;
    reg_clk_en5 |= FLD_CLK5_SARADC1_EN;

    for (int i = 0; i < (int)sar_adc_num; i++) {
        reg_adcdig_clk_set(i) = 0x24;

        analog_write_reg8(0x100 * i + 0x26c, 0x42);
        analog_write_reg8(0x100 * i + 0x272, 0x00);
        analog_write_reg8(0x100 * i + 0x273, 0x00);
        analog_write_reg8(0x100 * i + 0x274, 0x00);
        analog_write_reg8(0x100 * i + 0x279, 0x00);
        analog_write_reg8(0x100 * i + 0x27a, 0x1d);

        analog_write_reg8(0x100 * i + 0x27d, 0x03);

        reg_adc_r_mux(i, 0) = 0xb1;
        reg_adc_tsamp(i) = 0x02;
        reg_adc_vabt_div(i) = 0x00;

        reg_adc_channel_set_state(i, 0) = 0xa1;
        reg_adc_capture_state(i, 0) = 0x16;

        reg_adc_ovrsmpl(i) = 0x20;
        reg_adc_config0(i) = 0x20;
        reg_adc_config1(i) = 0x01;
        reg_adc_config2(i) = 0x11;
        reg_adc_rxfifo_trig_num(i) = 0x01;

        reg_pad_auto_p(i) = 0x11111111;//0x43214321;

        reg_adc_config2(i) = 0x31;//[5] sar_adc_clk_en
        reg_adc_config2(i) = 0xb1;//[7] trig_mode enable

        reg_sample_times(i) = 0x01;//sample_times = 1

        analog_write_reg8(0x100 * i + 0x27c, 0x00);//adc_power_down
    }

    reg_pad_auto_n(0) = 0xbbbbbbbb;//GND

    reg_pad_auto_n(1) = 0x88888888;//PB7 -> GND
}
#if INTERNAL_TEST_FUNC_EN
/**
 * @brief       This function is used to get the status of the valid adc code for the m channel.
 * @return      0:Before sampling one adc_code at each time,m_chn_valid_status is automatically set to 0.
 *              !0:After sampling one adc_code at each time,m_chn_valid_status is automatically set to 1.
 * @attention   Must call adc_ana_read_en()API first to get the status of the valid adc code for the m channel.
 */
static inline unsigned char adc_get_m_chn_valid_status(adc_num_e sar_adc_num)
{
    return (analog_read_reg8(areg_m_chn_data_valid_status(sar_adc_num)) & FLD_M_CHN_DATA_VALID_STATUS);
}
/**
 * @brief       This function is used to enable the data weighted average algorithm function to improve ADC performance.
 * @return      none
 */
static inline void adc_data_weighted_average_en(adc_num_e sar_adc_num)
{
    analog_write_reg8(areg_adc_data_sample_control(sar_adc_num), (analog_read_reg8(areg_adc_data_sample_control(sar_adc_num)) | FLD_DWA_EN_O));
}

/**
 * @brief       This function serves to enable software mode to collect data one time at a time.
 * @return      none
 * @note        This function requires adc_software_start to trigger and collect an adc data.
 */
void adc_software_enable(adc_num_e sar_adc_num)
{
    reg_m_config(sar_adc_num)  = 0x01;
}
/**
 * @brief       This function serves to disable software mode.
 * @return      none
 * @note        This function requires adc_software_start to trigger and collect an adc data.
 */
void adc_software_disable(adc_num_e sar_adc_num)
{
    reg_m_config(sar_adc_num)  = 0x00;
}

/**
 * @brief      This function open temperature sensor power.
 * @return     none
 */
static inline void adc_temp_sensor_power_on(void)
{
    analog_write_reg8(areg_temp_sensor_ctrl, (analog_read_reg8(areg_temp_sensor_ctrl)&(~FLD_TEMP_SENSOR_POWER_DOWN)));
}
/**
 * @brief      This function close temperature sensor power.
 * @return     none
 */
static inline void adc_temp_sensor_power_off(void)
{
    analog_write_reg8(areg_temp_sensor_ctrl, (analog_read_reg8(areg_temp_sensor_ctrl)|FLD_TEMP_SENSOR_POWER_DOWN));
}
/**
 * @brief This function is used to initialize the ADC for Temperature Sensor sampling.
 * @param[in]  chn -structure for configuring ADC channel.
 * @return none
 */
void adc_temp_init(adc_num_e sar_adc_num,adc_sample_chn_e chn)
{
    adc_chn_cfg_t chn_cfg =
    {
        .divider = ADC_VBAT_DIV_OFF,
        .v_ref = ADC_VREF_1P2V,
        .pre_scale = ADC_PRESCALE_1,
        .sample_freq = ADC_SAMPLE_FREQ_96K,
        .input_p = ADC_TEMPSENSORP_EE,
        .input_n = ADC_TEMPSENSORN_EE,

    };
    adc_chn_config(sar_adc_num,chn, chn_cfg);
    adc_temp_sensor_power_on();
    analog_write_reg8(areg_adc_verf_fast_startup_sampled_inuput(sar_adc_num),FLD_VERF_FAST_STARTUP|FLD_SAMPLED_INPUT_MODEBAR);
}
/**
 * @brief This function serves to calculate temperature from temperature sensor adc sample code.
 * @param[in]   adc_code            - the temperature sensor adc sample code.
 * @return      adc_temp_value      - the temperature value.
 * @attention   Temperature and adc_code are linearly related. We test four chips between -40~130 (Celsius) and got an average relationship:
 *          Temp =  564 - ((adc_code * 819)>>11),when Vref = 1.2V, pre_scale = 1.
 */
unsigned short adc_calculate_temperature(unsigned short adc_code)
{
    //////////////// adc sample data convert to temperature(Celsius) ////////////////
    //adc_temp_value = 564 - ((adc_code * 819)>>11)
    return 564 - ((adc_code * 819)>>11);
}
#endif
/**
 * @brief This function serves to calculate voltage from adc sample code.
 * @param[in]   chn - enum variable of ADC sample channel.
 * @param[in]   adc_code    - the adc sample code.
 * @return      adc_vol_mv  - the average value of adc voltage value.
 */
unsigned short adc_calculate_voltage(adc_sample_chn_e chn,unsigned short adc_code)
{
    //When the code value is 0, the returned voltage value should be 0.
    if(adc_code == 0)
    {
        return 0;
    }
    else
    {
    //////////////// adc sample data convert to voltage(mv) ////////////////
    //                          (Vref, adc_pre_scale)   (BIT<10~0> valid data)
    //           =  (adc_code * Vref * adc_pre_scale / 0x800) + offset
    //           =  (adc_code * Vref * adc_pre_scale >>11) + offset
        return (((adc_code * g_adc_vbat_divider[chn] * g_adc_pre_scale[chn] * g_adc_vref[chn])>>11) + g_adc_vref_offset[chn]);
    }
}


/**********************************************************************************************************************
 *                                                DMA only interface                                                  *
 **********************************************************************************************************************/
/**
 * @brief      This function serves to configure adc_dma_chn channel.
 * @param[in]  chn - the DMA channel
 * @return     none
 */
void adc_set_dma_config(adc_num_e sar_adc_num,dma_chn_e chn)
{
    adc_dma_chn = chn;
    reg_adc_config2(sar_adc_num) = FLD_RX_DMA_ENABLE;
    if(sar_adc_num == 0)
    {
        dma_config(chn, &adc_rx_dma_config);
    }
    else
    {
        dma_config(chn, &adc1_rx_dma_config);
    }

    reg_dma_llp(adc_dma_chn) = 0;
    /*
     * Configuration differs from TL751x for the following reasons:
     * #1 The TL751x's RX FIFO is stored in WORD units, and a setting of 0 indicates that a DMA request is sent to the DMA when the data in the RX FIFO is greater than 0 WORD units of data (i.e., 1 WORD).
     * #2 Because the TL321X's RX FIFO is stored in HALF WORD units and the DMA is configured to transfer data by WORD,
     *    it is necessary to set trigger number to 1 to indicate that if the data in the RX FIFO is greater than 1 HALF WORD unit of data (i.e., 1 WORD),
     *    it will send a DMA request to the DMA to prevent the RX FIFO from not yet being full of 1 WORD,
     *    it will be transferred away by the DMA by WORD, resulting in data misalignment when using multiple channels.
     */
    adc_set_rx_fifo_trig_cnt(sar_adc_num,1);//Default value is 1, users should not change.
    adc_all_chn_data_to_fifo_en(sar_adc_num);
}

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
void adc_start_sample_dma(adc_num_e sar_adc_num,unsigned short *adc_data_buf,unsigned int data_byte_len)
{
    dma_set_address(adc_dma_chn,SAR_ADC_FIFO(sar_adc_num),(unsigned int)adc_data_buf);
    dma_set_size(adc_dma_chn,data_byte_len,DMA_WORD_WIDTH);
    /*
     * dma_chn_en() must be in front of adc_set_scan_chn_cnt() to prevent mis-ordering of multi-channel sampling data when using multiple channels.
     */
    dma_chn_en(adc_dma_chn);
    adc_set_scan_chn_cnt(sar_adc_num,g_channel_cnt & 0x0f);
    adc_dig_clk_en(sar_adc_num);
}
/**
 * @brief     This function serves to get adc DMA irq status.
 * @return    0: the sample is in progress.
 *            !0: the sample is finished.
 * @note      The code is placed in the ram code section, in order to shorten the time.
 */
_attribute_ram_code_sec_noinline_ unsigned char adc_get_irq_status_dma(void)
{
    return (dma_get_tc_irq_status(1<<adc_dma_chn));
}
/**
 * @brief     This function serves to clear adc DMA irq status.
 * @return    none
 * @note      The code is placed in the ram code section, in order to shorten the time.
 */
_attribute_ram_code_sec_noinline_ void adc_clr_irq_status_dma(void)
{
    dma_clr_tc_irq_status(1<<adc_dma_chn);
}
/**********************************************************************************************************************
 *                                                NDMA only interface                                                 *
 **********************************************************************************************************************/
/**
 * @brief This function serves to directly get an adc sample code from fifo.
 * @return  adc_code    - the adc sample code.
 */
unsigned short adc_get_code(adc_num_e sar_adc_num)
{
    unsigned short adc_code=0;
    /**
     * Change the way to get adc code from read areg_adc_misc to read rx fifo, to solve the problem that the valid status is 1 for a long time
     * due to the slow change of valid status in adc_get_m_chn_valid_status(), which leads to fetching the same code repeatedly.(Confirmed by qiangkai.xu, added by xiaobin.huang at 20240903)
     */
    if(!g_adc_rx_fifo_index[sar_adc_num])
    {
        adc_code = reg_adc_rxfifo_dat(sar_adc_num,g_adc_rx_fifo_index[sar_adc_num]);
        g_adc_rx_fifo_index[sar_adc_num] = 1;
    }else{
        adc_code = reg_adc_rxfifo_dat(sar_adc_num,g_adc_rx_fifo_index[sar_adc_num]);
        g_adc_rx_fifo_index[sar_adc_num] = 0;
    }
    return adc_code;
}

/**
 * @brief   This function is used to enable software mode to collect data one time at a time
 * @return  none
 */
void adc_start_sample_nodma(adc_num_e sar_adc_num)
{
    adc_all_chn_data_to_fifo_en(sar_adc_num);
    adc_set_scan_chn_cnt(sar_adc_num,1);
}

/**
 * @brief   This function is used to disable the transmission of data from the adc's M channel, L channel, and R channel to the sar adc rxfifo and clear rx fifo cnt and g_adc_rx_fifo_index.
 * @return  none
 */
void adc_stop_sample_nodma(adc_num_e sar_adc_num)
{
    adc_all_chn_data_to_fifo_dis(sar_adc_num);
    adc_clr_rx_fifo_cnt(sar_adc_num);
}
/**
 * @brief      This function sets adc digital clock and analog clock.
 * @param      div - Analog adc frequency division coefficient.
 * @return     none
 * @attention  ADC digital clock from crystal(24MHz), analog clock from digital clock.
 *             The ADC digital clock and analog clock are fixed at 24 MHz and 4 MHz, respectively, and should not be modified.
 */
void adc_set_clk(adc_num_e sar_adc_num,char div)
{
    reg_adc_config1(sar_adc_num) = ((reg_adc_config1(sar_adc_num)  & FLD_SAR_ADC_CLK_DIV) | 1);//div=1, adc digital clk = 24MHz/div.(crystal = 24MHz)
    analog_write_reg8(areg_adc_sample_clk_div(sar_adc_num), div & 0x0f);//div=5, adc analog clk = 24MHz/(1+div) = 4M.
}

/**
 * @brief      This function servers to configure PEM channel and some configures.
 * @param[in]  chn - to select the PEM channel.
 * @return     none.
 */
void adc_pem_task(adc_num_e sar_adc_num,pem_chn_e chn)
{
    reg_pem_enable(sar_adc_num) |= FLD_PEM_TASK_ENABLE;
    adc_pem_task_config.module = PEM_TASK_SAR_ADC;
    adc_pem_task_config.sig_sel = 0;         //only single adc_trig task
    pem_task_config(chn,adc_pem_task_config);
}

/**
 * @brief      This function servers to configure PEM channel and some configures.
 * @param[in]  chn - to select the PEM channel.
 * @param[in]  event_signal - to select the event signal.
 * @return     none.
 */
void adc_pem_event(adc_num_e sar_adc_num,pem_chn_e chn,unsigned char adc_sel)
{
    reg_pem_enable(sar_adc_num) |= FLD_PEM_EVENT1_ENABLE;
    adc_pem_event_config.module = PEM_EVENT_SAR_ADC;
    adc_pem_event_config.sig_sel = adc_sel;
    pem_event_config(chn, adc_pem_event_config);
}
