/********************************************************************************************************
 * @file    audio.c
 *
 * @brief   This is the source file for TL751X
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
#include "audio.h"

/**
 * @brief Audio rx fifo channel.
 *
 */
static unsigned char audio_rx_fifo_chn;

/**
 * @brief Audio rx dma channel.
 *
 */
static unsigned char audio_rx_dma_chn;

/**
 * @brief Audio tx fifo channel.
 *
 */
static unsigned char audio_tx_fifo_chn;

/**
 * @brief Audio tx dma channel.
 *
 */
static unsigned char audio_tx_dma_chn;

/**
 * @brief Audio tx dma list config table.
 *
 */
static dma_chain_config_t g_audio_tx_dma_list_cfg[4];

/**
 * @brief Audio rx dma list config table.
 *
 */
static dma_chain_config_t g_audio_rx_dma_list_cfg[4];

/**
 * @brief Audio i2s data/lr_clk invert config table.
 *
 */
static i2s_invert_config_t audio_i2s_invert_config[3] = {
    {
     .i2s_lr_clk_invert_select = I2S_LR_CLK_INVERT_DIS,
     .i2s_data_invert_select   = I2S_DATA_INVERT_DIS,
     },
    {
     .i2s_lr_clk_invert_select = I2S_LR_CLK_INVERT_DIS,
     .i2s_data_invert_select   = I2S_DATA_INVERT_DIS,
     },
    {
     .i2s_lr_clk_invert_select = I2S_LR_CLK_INVERT_DIS,
     .i2s_data_invert_select   = I2S_DATA_INVERT_DIS,
     },
};

/**
 * @brief Audio rx dma config table.
 *
 */
dma_config_t audio_dma_rx_config[4] = {
    {
     .dst_req_sel    = 0,
     .src_req_sel    = DMA_REQ_AUDIO0_RX,
     .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
     .src_addr_ctrl  = DMA_ADDR_FIX,
     .dstmode        = DMA_NORMAL_MODE,
     .srcmode        = DMA_HANDSHAKE_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
    {
     .dst_req_sel    = 0,
     .src_req_sel    = DMA_REQ_AUDIO1_RX,
     .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
     .src_addr_ctrl  = DMA_ADDR_FIX,
     .dstmode        = DMA_NORMAL_MODE,
     .srcmode        = DMA_HANDSHAKE_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
    {
     .dst_req_sel    = 0,
     .src_req_sel    = DMA_REQ_AUDIO2_RX,
     .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
     .src_addr_ctrl  = DMA_ADDR_FIX,
     .dstmode        = DMA_NORMAL_MODE,
     .srcmode        = DMA_HANDSHAKE_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
    {
     .dst_req_sel    = 0,
     .src_req_sel    = DMA_REQ_AUDIO3_RX,
     .dst_addr_ctrl  = DMA_ADDR_INCREMENT,
     .src_addr_ctrl  = DMA_ADDR_FIX,
     .dstmode        = DMA_NORMAL_MODE,
     .srcmode        = DMA_HANDSHAKE_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
};

/**
 * @brief Audio tx dma config table.
 *
 */
dma_config_t audio_dma_tx_config[4] = {
    {
     .dst_req_sel    = DMA_REQ_AUDIO0_TX,
     .src_req_sel    = 0,
     .dst_addr_ctrl  = DMA_ADDR_FIX,
     .src_addr_ctrl  = DMA_ADDR_INCREMENT,
     .dstmode        = DMA_HANDSHAKE_MODE,
     .srcmode        = DMA_NORMAL_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
    {
     .dst_req_sel    = DMA_REQ_AUDIO1_TX,
     .src_req_sel    = 0,
     .dst_addr_ctrl  = DMA_ADDR_FIX,
     .src_addr_ctrl  = DMA_ADDR_INCREMENT,
     .dstmode        = DMA_HANDSHAKE_MODE,
     .srcmode        = DMA_NORMAL_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
    {
     .dst_req_sel    = DMA_REQ_AUDIO2_TX,
     .src_req_sel    = 0,
     .dst_addr_ctrl  = DMA_ADDR_FIX,
     .src_addr_ctrl  = DMA_ADDR_INCREMENT,
     .dstmode        = DMA_HANDSHAKE_MODE,
     .srcmode        = DMA_NORMAL_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
    {
     .dst_req_sel    = DMA_REQ_AUDIO3_TX,
     .src_req_sel    = 0,
     .dst_addr_ctrl  = DMA_ADDR_FIX,
     .src_addr_ctrl  = DMA_ADDR_INCREMENT,
     .dstmode        = DMA_HANDSHAKE_MODE,
     .srcmode        = DMA_NORMAL_MODE,
     .dstwidth       = DMA_CTR_WORD_WIDTH,
     .srcwidth       = DMA_CTR_WORD_WIDTH,
     .src_burst_size = DMA_BURST_1_WORD,
     .read_num_en    = 0,
     .priority       = 0,
     .write_num_en   = 0,
     .auto_en        = 0,
     },
};

/**********************************************************************************************************************
 *                                                Audio anc interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio anc interface
 * @{
 */

/**
 * @brief      This function serves to set dac control resample fs.
 *
 * @param[in]  anc_chn - anc channel.
 * @param[in]  in_fs   - resample input fs.
 * @param[in]  out_fs  - resample output fs.
 * @note
 *             - when resample fs rely on dac, this function must be called
 */
void audio_dac_ctl_anc_resample_fs(audio_anc_chn_e anc_chn, audio_anc_resample_in_fs_e in_fs, audio_anc_resample_out_fs_e out_fs)
{
    char reg_data = 0;
    /* find dac cnt mode register data: 0: 48k->768k; 1: 96k->768k or 48k->384k; 2: 96k->384k. */
    if ((ANC_RESAMPLE_IN_FS_48K == in_fs) && (ANC_RESAMPLE_OUT_FS_768K == out_fs))
    {
        reg_data = 0;
    }
    else if ((ANC_RESAMPLE_IN_FS_96K == in_fs) && (ANC_RESAMPLE_OUT_FS_384K == out_fs))
    {
        reg_data = 2;
    }
    else
    {
        reg_data = 1;
    }

    reg_audio_anc_config2(anc_chn) = (reg_audio_anc_config2(anc_chn) & (~FLD_ANC_DAC_CNT_MODE)) |
                                     MASK_VAL(FLD_ANC_DAC_CNT_MODE, reg_data); /* set dac cnt mode to control audio pcm rate. */
}

/**
 * @brief      This function serves to set the resample frequency of anc.
 * 
 * @param[in]  anc_chn     - anc channel.
 * @param[in]  fs_decision - who decides the resample frequency of anc.
 * @param[in]  in_fs       - resample input fs.
 * @param[in]  out_fs      - resample output fs.
 */
void audio_anc_set_resample_in_out_fs(audio_anc_chn_e anc_chn, audio_anc_resample_fs_decision_e fs_decision, audio_anc_resample_in_fs_e in_fs,
                                      audio_anc_resample_out_fs_e out_fs)
{
    if (fs_decision == ANC_RESAMPLE_DAC_DECISION_FS)
    {
        BM_SET(reg_audio_anc_config(anc_chn), FLD_ANC_SRC_EN);
        BM_SET(reg_audio_anc_config(anc_chn), FLD_ANC_SRC_RATE_SEL);
        audio_dac_ctl_anc_resample_fs(anc_chn, in_fs, out_fs);
    }
    else
    {
        BM_CLR(reg_audio_anc_config(anc_chn), FLD_ANC_SRC_RATE_SEL);
    }

    audio_anc_set_resample_in_fs(anc_chn, in_fs);
    audio_anc_set_resample_out_fs(anc_chn, out_fs);
}

/**
 * @brief      This function servers to set anc wz fir filter coefficients.
 *
 * @param[in]  anc_chn  - anc channel.
 * @param[in]  data     - wz fir filter data address.
 * @param[in]  data_len - wz fir data len max length is 384(wz fir max is 384taps).
 * @note
 *             - bypass coefficients: wz[0] = 0x4000, the rest of the parameters are all set to 0
 * @return     none
 */
void audio_anc_set_wz_fir_coef(audio_anc_chn_e anc_chn, signed short *data, unsigned short data_len)
{
    for (unsigned short i = 0; i < data_len; i++)
    {
        reg_audio_anc_wz_h(anc_chn, i) = data[i];
    }
}

/**
 * @brief      This function servers to set anc wz biquad iir filter coefficients.
 *
 * @param[in]  anc_chn - anc channel.
 * @param[in]  data    - wz biquad iir filter data address data[][b0, b1, b2, a1, a2].
 * @note
 *             - bypass coefficients: every b[0] = 0x10000000, the rest of the parameters are all set to 0
 * @return     none
 */
void audio_anc_set_wz_iir_coef(audio_anc_chn_e anc_chn, signed int data[5][5])
{
    for (unsigned char i = 0; i < 5; i++)
    {
        reg_audio_anc_wz_iir_b0(anc_chn, i) = data[i][0];
        reg_audio_anc_wz_iir_b1(anc_chn, i) = data[i][1];
        reg_audio_anc_wz_iir_b2(anc_chn, i) = data[i][2];
        reg_audio_anc_wz_iir_a1(anc_chn, i) = data[i][3];
        reg_audio_anc_wz_iir_a2(anc_chn, i) = data[i][4];
    }
}

/**
 * @brief      This function servers to set anc cz fir filter coefficients.
 *
 * @param[in]  anc_chn  - anc channel.
 * @param[in]  data     - cz fir filter data address.
 * @param[in]  data_len - cz fir data len max length is 128(cz fir max step is 128taps).
 * @return     none
 */
void audio_anc_cz_fir_set(audio_anc_chn_e anc_chn, signed short *data, unsigned short data_len)
{
    for (unsigned short i = 0; i < data_len; i++)
    {
        reg_audio_anc_cz_h(anc_chn, i) = data[i];
    }
}

/**
 * @brief      This function servers to set anc cz biquad iir filter coefficients.
 *
 * @param[in]  anc_chn - anc channel.
 * @param[in]  data    - cz biquad iir filter data address data[][b0, b1, b2, a1, a2].
 * @return     none
 */
void audio_anc_cz_iir_set(audio_anc_chn_e anc_chn, signed int data[5][5])
{
    for (unsigned char i = 0; i < 5; i++)
    {
        reg_audio_anc_cz_iir_b0(anc_chn, i) = data[i][0];
        reg_audio_anc_cz_iir_b1(anc_chn, i) = data[i][1];
        reg_audio_anc_cz_iir_b2(anc_chn, i) = data[i][2];
        reg_audio_anc_cz_iir_a1(anc_chn, i) = data[i][3];
        reg_audio_anc_cz_iir_a2(anc_chn, i) = data[i][4];
    }
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio asrc interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio asrc interface
 * @{
 */
/**
 * @brief      This function serves to update asrc droop coefficients.
 * 
 * @param[in]  asrc_chn  - asrc channel select.
 * @param[in]  d_coef    - asrc droop coefficients data address.
 * @param[in]  data_len  - asrc droop coefficients data length.
 * @return     none
 * @note
 *             - droop coef max length is 9.
 */
void audio_asrc_set_droop_coef(audio_hac_chn_e asrc_chn, signed short *d_coef, unsigned char data_len)
{
    for (unsigned char i = 0; i < data_len; i++)
    {
        reg_audio_asrc_drop_coef(asrc_chn, i) = d_coef[i] & FLD_ASRC_DROP_COEF;
    }
}

/**
 * @brief      This function serves to update asrc half_band1 coefficients.
 * 
 * @param[in]  asrc_chn - asrc channel.
 * @param[in]  hb1_coef - asrc half_band1 coefficients data address.
 * @return     none
 * @note
 *             - asrc half_band1 coefficient length is 32 word, bit[0-25] valid.
 */
void audio_asrc_set_hb1_coef(audio_hac_chn_e asrc_chn, signed int *hb1_coef)
{
    for (unsigned char i = 0; i < 32; i++)
    {
        reg_audio_asrc_hb1_coef(asrc_chn, i) = hb1_coef[i] & FLD_ASRC_HB1_COEF;
    }
}

/**
 * @brief      This function serves to update asrc half_band2 coefficients.
 * 
 * @param[in]  asrc_chn - asrc channel.
 * @param[in]  hb2_coef - asrc half_band2 coefficients data address. 
 * @return     none
 * @note
 *             - asrc half_band2 coefficient length is 7 word, bit[0-25] valid.
 */
void audio_asrc_set_hb2_coef(audio_hac_chn_e asrc_chn, signed int *hb2_coef)
{
    for (unsigned char i = 0; i < 7; i++)
    {
        reg_audio_asrc_hb2_coef(asrc_chn, i) = hb2_coef[i] & FLD_ASRC_HB1_COEF;
    }
}

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
void audio_init(sys_audio_pll_clk_e audio_pll)
{
    unsigned char aclk_div = 0;

    pm_set_dig_module_power_switch(FLD_PD_AUDIO_EN, PM_POWER_UP);

    clock_audio_pll_config(audio_pll);
    pm_audio_pll_power_on();
    pm_wait_audio_pll_done(); /* 61.84us */

    BM_SET(reg_rst2, FLD_RST2_AUD);
    BM_SET(reg_clk_en2, FLD_CLK2_AUD_EN);

    switch (audio_pll)
    {
    case AUDIO_PLL_CLK_169P344M: /* audio pll = 169.344MHz */
        aclk_div = 5;
        break;
    case AUDIO_PLL_CLK_147P456M: /* audio pll = 147.456MHz */
        aclk_div = 4;
        break;
    case AUDIO_PLL_CLK_36P864M: /* audio pll = 36.864MHz */
        aclk_div = 1;
        break;
    case AUDIO_PLL_CLK_33P8688M: /* audio pll = 33.8688MHz */
        aclk_div = 1;
        break;
    default:
        aclk_div = 1;
        break;
    }

    reg_audio_clk_aclk_sel = (reg_audio_clk_aclk_sel & (~FLD_CLK_ACLK_SET)) | MASK_VAL(FLD_CLK_ACLK_SET, aclk_div);
    BM_SET(reg_audio_clk_en, FLD_CLK_ACLK_EN);
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio codec0 interface                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio codec0 interface
 * @{
 */

/**
 * @brief      This function serves to power on codec0.
 * @param[in]  power_mode - codec0 power mode selection.
 * @param[in]  volt       - codec0 analog voltage selection.
 * @return     none 
 */
void audio_codec0_power_on(audio_codec0_power_e power_mode, audio_codec0_volt_supply_e volt)
{
    BM_SET(reg_audio_clk_en, FLD_CLK_CODEC0_EN);
    delay_us(1); /* wait codec0 clock stable. */
    reg_audio_codec0_cr_vic = (reg_audio_codec0_cr_vic & (~(FLD_CODEC0_POWER_CTRL | FLD_CODEC0_AVD_1V8))) |
                              MASK_VAL(FLD_CODEC0_POWER_CTRL, power_mode, FLD_CODEC0_AVD_1V8, volt);
    unsigned int ref_tick = stimer_get_tick();
    while (!(reg_audio_codec0_sr & FLD_CODEC0_POP_ACK)) /* wait codec can be configured */
    {
        if (clock_time_exceed(ref_tick, 130000)) /* 130ms codec power-up timing requirements. */
        {
            return;
        }
    }
}

/**
 * @brief      This function serves to power down codec0 adc.
 * @param[in]  adc - adc channel.
 * @return     none
 * @note
 *             - ADC_B only ADC_B1 support analog data.
 *             - This interface only powers down the analog ADC.
 */
void audio_codec0_adc_power_down(audio_codec0_input_select_e adc)
{
    unsigned char channel = adc & BIT_RNG(0, 3);

    audio_codec0_set_input_mute(channel, 1); /* adc mute. */
    audio_codec0_set_micbias(channel, 0);

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1:                                      /* ADC_A1. */
        BM_SET(reg_audio_codec0_cr_adca12, FLD_CODEC0_SB_ADCA1); /* adc A1 channel inactive */
        break;
    case AUDIO_DMIC_ADC_A2:                                      /* ADC_A2. */
        BM_SET(reg_audio_codec0_cr_adca12, FLD_CODEC0_SB_ADCA2); /* adc A2 channel inactive */
        break;
    case AUDIO_DMIC_ADC_B1:                                      /* ADC_B1. */
        BM_SET(reg_audio_codec0_cr_adcb12, FLD_CODEC0_SB_ADCB1); /* adc B1 channel inactive */
        break;
    case AUDIO_DMIC_ADC_A1_A2: /* ADC_A1_A2. */
        reg_audio_codec0_cr_adca12 = (reg_audio_codec0_cr_adca12 & (~(FLD_CODEC0_SB_ADCA1 | FLD_CODEC0_SB_ADCA2))) |
                                     MASK_VAL(FLD_CODEC0_SB_ADCA1, 1, FLD_CODEC0_SB_ADCA2, 1); /* adc A1/A2 channel inactive */
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to power on codec0 output.
 * @param[in]  output - output channel.
 * @return     none
 */
void audio_codec0_output_power_down(audio_codec0_output_select_e output)
{
    audio_codec0_set_output_mute(output, 1); /* dac mute. */

    switch (output)
    {
    case AUDIO_DAC_A1:                                           /* DAC_A1. */
        BM_SET(reg_audio_codec0_cr_daca12, FLD_CODEC0_SB_DACA1); /* DAC A1 in power-down. */
        BM_SET(reg_audio_codec0_cr_hpa, FLD_CODEC0_SB_HPA1);     /* Headphone A1 output stage is in power down. */
        break;
    case AUDIO_DAC_A2:                                           /* DAC_A1. */
        BM_SET(reg_audio_codec0_cr_daca12, FLD_CODEC0_SB_DACA2); /* DAC A2 in power-down. */
        BM_SET(reg_audio_codec0_cr_hpa, FLD_CODEC0_SB_HPA2);     /* Headphone A2 output stage is in power down. */
        break;
    case AUDIO_DAC_A1_A2: /* DAC_A1_A2. */
        reg_audio_codec0_cr_daca12 = (reg_audio_codec0_cr_daca12 & (~(FLD_CODEC0_SB_DACA1 | FLD_CODEC0_SB_DACA2))) |
                                     MASK_VAL(FLD_CODEC0_SB_DACA1, 1, FLD_CODEC0_SB_DACA2, 1); /* DAC A1/A2 in power-down. */
        reg_audio_codec0_cr_hpa = (reg_audio_codec0_cr_hpa & (~(FLD_CODEC0_SB_HPA1 | FLD_CODEC0_SB_HPA2))) |
                                  MASK_VAL(FLD_CODEC0_SB_HPA1, 1, FLD_CODEC0_SB_HPA2, 1); /* Headphone A1/A2 output stage is in power down. */
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to power down codec0.
 * @return     none 
 * @note
 *             - After power down, cannot access codec0 registers.
 */
void audio_codec0_power_down(void)
{
    /* power down output. */
    audio_codec0_output_power_down(AUDIO_DAC_A1_A2);

    /* power down adc. */
    audio_codec0_adc_power_down(AUDIO_DMIC_ADC_A1_A2);
    audio_codec0_adc_power_down(AUDIO_DMIC_ADC_B1);

    /* power down dmic. */
    audio_codec0_dmic_clk_en(AUDIO_DMIC_ADC_A1_A2, 0);
    audio_codec0_dmic_clk_en(AUDIO_DMIC_ADC_B1_B2, 0);

    /* power down codec0. */
    reg_audio_codec0_cr_vic = reg_audio_codec0_cr_vic | 0x0f;
    BM_CLR(reg_audio_clk_en, FLD_CLK_CODEC0_EN); /* After clock disable, cannot access codec registers. */
}

/**
 * @brief      This function configures codec0 stream0 dmic pin.
 * @param[in]  dmic0_data - the data of  dmic pin
 * @param[in]  dmic0_clk1 - the clk1 of dmic pin
 * @param[in]  dmic0_clk2 - the clk2 of dmic pin,if need not set clk2, please set AUDIO_DMIC_NONE_PIN.
 * @return     none
 */
void audio_codec0_set_stream0_dmic_pin(audio_dmic_pin_e dmic0_data, audio_dmic_pin_e dmic0_clk1, audio_dmic_pin_e dmic0_clk2)
{
    /* dmic0 data. */
    gpio_input_en((gpio_pin_e)dmic0_data);
    gpio_set_mux_function((gpio_func_pin_e)dmic0_data, DMIC0_DAT_I);
    gpio_function_dis((gpio_pin_e)dmic0_data);
    /* dmic0 clock0. */
    gpio_set_mux_function((gpio_func_pin_e)dmic0_clk1, DMIC0_CLK0);
    gpio_function_dis((gpio_pin_e)dmic0_clk1);
    /* dmic1 clock0. */
    if (dmic0_clk2 != AUDIO_DMIC_NONE_PIN)
    {
        gpio_set_mux_function((gpio_func_pin_e)dmic0_clk2, DMIC0_CLK1);
        gpio_function_dis((gpio_pin_e)dmic0_clk2);
    }
}

/**
 * @brief      This function configures codec0 stream1 dmic pin.
 * @param[in]  dmic1_data - the data of dmic pin.
 * @param[in]  dmic1_clk1 - the clk1 of dmic pin.
 * @param[in]  dmic1_clk2 - the clk2 of dmic pin, if need not set clk2,please set AUDIO_DMIC_NONE_PIN.
 * @return     none
 */
void audio_codec0_set_stream1_dmic_pin(audio_dmic_pin_e dmic1_data, audio_dmic_pin_e dmic1_clk1, audio_dmic_pin_e dmic1_clk2)
{
    /* dmic1 data. */
    gpio_input_en((gpio_pin_e)dmic1_data);
    gpio_set_mux_function((gpio_func_pin_e)dmic1_data, DMIC1_DAT_I);
    gpio_function_dis((gpio_pin_e)dmic1_data);
    /* dmic1 clock0. */
    gpio_set_mux_function((gpio_func_pin_e)dmic1_clk1, DMIC1_CLK0);
    gpio_function_dis((gpio_pin_e)dmic1_clk1);
    /* dmic1 clock0. */
    if (dmic1_clk2 != AUDIO_DMIC_NONE_PIN)
    {
        gpio_set_mux_function((gpio_func_pin_e)dmic1_clk2, DMIC1_CLK1);
        gpio_function_dis((gpio_pin_e)dmic1_clk2);
    }
}

/**
 * @brief      This function serves to enable/disable codec0 dmic clock.
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: active dmic clock, 0: power-down dmic clock.
 */
void audio_codec0_dmic_clk_en(audio_codec0_input_select_e input, unsigned char enable)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1: /* ADC_A1. */
        reg_audio_codec0_cr_dmic_adca_sb = (reg_audio_codec0_cr_dmic_adca_sb & (~FLD_CODEC0_SB_DMIC_ADCA1)) |
                                           MASK_VAL(FLD_CODEC0_SB_DMIC_ADCA1, !enable);
        break;
    case AUDIO_DMIC_ADC_A2: /* ADC_A2. */
        reg_audio_codec0_cr_dmic_adca_sb = (reg_audio_codec0_cr_dmic_adca_sb & (~FLD_CODEC0_SB_DMIC_ADCA2)) |
                                           MASK_VAL(FLD_CODEC0_SB_DMIC_ADCA2, !enable);
        break;
    case AUDIO_DMIC_ADC_B1: /* ADC_B1. */
        reg_audio_codec0_cr_dmic_adcb_sb = (reg_audio_codec0_cr_dmic_adcb_sb & (~FLD_CODEC0_SB_DMIC_ADCB1)) |
                                           MASK_VAL(FLD_CODEC0_SB_DMIC_ADCB1, !enable);
        break;
    case AUDIO_DMIC_ADC_B2: /* ADC_B2. */
        reg_audio_codec0_cr_dmic_adcb_sb = (reg_audio_codec0_cr_dmic_adcb_sb & (~FLD_CODEC0_SB_DMIC_ADCB2)) |
                                           MASK_VAL(FLD_CODEC0_SB_DMIC_ADCB2, !enable);
        break;
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2. */
        reg_audio_codec0_cr_dmic_adca_sb = (reg_audio_codec0_cr_dmic_adca_sb & (~(FLD_CODEC0_SB_DMIC_ADCA1 | FLD_CODEC0_SB_DMIC_ADCA2))) |
                                           MASK_VAL(FLD_CODEC0_SB_DMIC_ADCA1, !enable, FLD_CODEC0_SB_DMIC_ADCA2, !enable);
        break;
    case AUDIO_DMIC_ADC_B1_B2: /*ADC_B1_B2. */
        reg_audio_codec0_cr_dmic_adcb_sb = (reg_audio_codec0_cr_dmic_adcb_sb & (~(FLD_CODEC0_SB_DMIC_ADCB1 | FLD_CODEC0_SB_DMIC_ADCB2))) |
                                           MASK_VAL(FLD_CODEC0_SB_DMIC_ADCB1, !enable, FLD_CODEC0_SB_DMIC_ADCB2, !enable);
        break;
    default:
        break;
    }
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
void audio_codec0_set_micbias(audio_codec0_input_select_e input, unsigned char enable)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1: /* ADC_A1. */
    case AUDIO_DMIC_ADC_A2: /* ADC_A2. */
    case AUDIO_DMIC_ADC_B1: /* ADC_B1. */
        reg_audio_codec0_cr_adc_mic(channel) = (reg_audio_codec0_cr_adc_mic(channel) & (~FLD_CODEC0_SB_MICBIAS)) |
                                               MASK_VAL(FLD_CODEC0_SB_MICBIAS, !enable);
        break;
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2. */
        reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A1) = (reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A1) & (~FLD_CODEC0_SB_MICBIAS)) |
                                                         MASK_VAL(FLD_CODEC0_SB_MICBIAS, !enable);
        reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A2) = (reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A2) & (~FLD_CODEC0_SB_MICBIAS)) |
                                                         MASK_VAL(FLD_CODEC0_SB_MICBIAS, !enable);
        break;
    case AUDIO_DMIC_ADC_B1_B2: /* ADC_B only ADC_B1 support bias output. */
        reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_B1) = (reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_B1) & (~FLD_CODEC0_SB_MICBIAS)) |
                                                         MASK_VAL(FLD_CODEC0_SB_MICBIAS, !enable);
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 adc mode.
 * @param[in]  adc      - adc channel.
 * @param[in]  adc_mode - 1: differential input, 0: single-ended input.
 * @return     none
 * @note
 *             - adc mode only for line_in or amic.
 */
void audio_codec0_set_adc_mode(audio_codec0_input_select_e adc, audio_codec0_adc_mode_e adc_mode)
{
    unsigned char channel = adc & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1: /* ADC_A1. */
    case AUDIO_DMIC_ADC_A2: /* ADC_A2. */
    case AUDIO_DMIC_ADC_B1: /* ADC_B1. */
        reg_audio_codec0_cr_adc_mic(channel) = (reg_audio_codec0_cr_adc_mic(channel) & (~FLD_CODEC0_MICDIFF)) |
                                               MASK_VAL(FLD_CODEC0_MICDIFF, adc_mode);
        break;
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2. */
        reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A1) = (reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A1) & (~FLD_CODEC0_MICDIFF)) |
                                                         MASK_VAL(FLD_CODEC0_MICDIFF, adc_mode);
        reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A2) = (reg_audio_codec0_cr_adc_mic(AUDIO_DMIC_ADC_A2) & (~FLD_CODEC0_MICDIFF)) |
                                                         MASK_VAL(FLD_CODEC0_MICDIFF, adc_mode);
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 input mute.
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: soft mute active, 0: soft mute inactive.
 * @return     none
 */
void audio_codec0_set_input_mute(audio_codec0_input_select_e input, unsigned char enable)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1: /* ADC_A1. */
    case AUDIO_DMIC_ADC_A2: /* ADC_A2. */
    case AUDIO_DMIC_ADC_B1: /* ADC_B1. */
    case AUDIO_DMIC_ADC_B2: /* ADC_B2. */
        reg_audio_codec0_cr_adc_dgain(channel) = (reg_audio_codec0_cr_adc_dgain(channel) & (~FLD_CODEC0_ADC_SOFT_MUTE)) |
                                                 MASK_VAL(FLD_CODEC0_ADC_SOFT_MUTE, enable); /* ADC soft mute. */
        break;
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2. */
        reg_audio_codec0_cr_adca1_dgain = (reg_audio_codec0_cr_adca1_dgain & (~FLD_CODEC0_ADCA1_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_ADCA1_SOFT_MUTE, enable); /* ADC_A1 soft mute. */
        reg_audio_codec0_cr_adca2_dgain = (reg_audio_codec0_cr_adca2_dgain & (~FLD_CODEC0_ADCA2_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_ADCA2_SOFT_MUTE, enable); /* ADC_A2 soft mute. */
        break;
    case AUDIO_DMIC_ADC_B1_B2: /*ADC_B1_B2. */
        reg_audio_codec0_cr_adcb1_dgain = (reg_audio_codec0_cr_adcb1_dgain & (~FLD_CODEC0_ADCB1_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_ADCB1_SOFT_MUTE, enable); /* ADC_B1 soft mute. */
        reg_audio_codec0_cr_adcb2_dgain = (reg_audio_codec0_cr_adcb2_dgain & (~FLD_CODEC0_ADCB2_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_ADCB2_SOFT_MUTE, enable); /* ADC_B2 soft mute. */
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 output mute.
 * @param[in]  output - output channel.
 * @param[in]  enable - 1: soft mute active, 0: soft mute inactive.
 * @return     none 
 */
void audio_codec0_set_output_mute(audio_codec0_output_select_e output, unsigned char enable)
{
    switch (output)
    {
    case AUDIO_DAC_A1: /* DAC_A1. */
        reg_audio_codec0_cr_daca1_dgain = (reg_audio_codec0_cr_daca1_dgain & (~FLD_CODEC0_DACA1_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_DACA1_SOFT_MUTE, enable); /* DAC_A1 soft mute. */
        break;
    case AUDIO_DAC_A2: /* DAC_A2. */
        reg_audio_codec0_cr_daca2_dgain = (reg_audio_codec0_cr_daca2_dgain & (~FLD_CODEC0_DACA2_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_DACA2_SOFT_MUTE, enable); /* DAC_A2 soft mute. */
        break;
    case AUDIO_DAC_A1_A2: /* DAC_A1_A2. */
        reg_audio_codec0_cr_daca1_dgain = (reg_audio_codec0_cr_daca1_dgain & (~FLD_CODEC0_DACA1_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_DACA1_SOFT_MUTE, enable); /* DAC_A1 soft mute. */
        reg_audio_codec0_cr_daca2_dgain = (reg_audio_codec0_cr_daca2_dgain & (~FLD_CODEC0_DACA2_SOFT_MUTE)) |
                                          MASK_VAL(FLD_CODEC0_DACA2_SOFT_MUTE, enable); /* DAC_A2 soft mute. */
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 input sample rate.
 * @param[in]  input - input channel.
 * @param[in]  fs    - input sample rate.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 sample rates are set in pairs.
 */
void audio_codec0_set_input_fs(audio_codec0_input_select_e input, audio_sample_rate_e fs)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    if ((channel == AUDIO_DMIC_ADC_A1) || (channel == AUDIO_DMIC_ADC_A2) || (channel == AUDIO_DMIC_ADC_A1_A2)) /* ADC_A. */
    {
        reg_audio_codec0_cr_adca_freq_sel = (reg_audio_codec0_cr_adca_freq_sel & (~FLD_CODEC0_ADCA_FREQ)) | (fs & 0xff);
        reg_audio_codec0_dec0_pcm_num     = fs >> 8; /* fs = 12.288MHz/11.2896MHz / (reg_audio_codec_dec_pcm_num + 1) */
    }
    else if ((channel == AUDIO_DMIC_ADC_B1) || (channel == AUDIO_DMIC_ADC_B2) || channel == AUDIO_DMIC_ADC_B1_B2) /* ADC_B. */
    {
        reg_audio_codec0_cr_adcb_freq_sel = (reg_audio_codec0_cr_adcb_freq_sel & (~FLD_CODEC0_ADCB_FREQ)) | (fs & 0xff);
        reg_audio_codec0_dec1_pcm_num     = fs >> 8; /* fs = 12.288MHz/11.2896MHz / (reg_audio_codec_dec_pcm_num + 1) */
    }
}

/**
 * @brief      This function serves to set codec0 output sample rate.
 * @param[in]  fs - output sample rate.
 * @return     none 
 * @note
 *             - DAC_A1/2 or DAC_B1/2 sample rates are set in pairs.
 */
void audio_codec0_set_output_fs(audio_sample_rate_e fs)
{
    reg_audio_codec0_cr_daca_freq_sel = (reg_audio_codec0_cr_daca_freq_sel & (~FLD_CODEC0_DACA_FREQ)) | (fs & 0xff);
    reg_audio_codec0_int_pcm_num      = fs >> 8; /* fs = 12.288MHz/11.2896MHz / (reg_audio_codec_dec_pcm_num + 1) */
}

/**
 * @brief      This function serves to set codec0 input data world length.
 * @param[in]  input - input channel.
 * @param[in]  wl    - world length.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 world length are set in pairs.
 */
void audio_codec0_set_input_wl(audio_codec0_input_select_e input, audio_codec0_data_select_e wl)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    if ((channel == AUDIO_DMIC_ADC_A1) || (channel == AUDIO_DMIC_ADC_A2) || (channel == AUDIO_DMIC_ADC_A1_A2)) /* ADC_A. */
    {
        reg_audio_codec0_cr_adca_ai = (reg_audio_codec0_cr_adca_ai & (~(FLD_CODEC0_ADCA_ADWL | FLD_CODEC0_ADCA_SLAVE))) |
                                      MASK_VAL(FLD_CODEC0_ADCA_SLAVE, 1, FLD_CODEC0_ADCA_ADWL, wl);
        reg_audio_codec_data_fmt_l = (reg_audio_codec_data_fmt_l & (~(FLD_CODEC_CTRL_ADC0_SL_SEL | FLD_CODEC_CTRL_ADC0_SR_SEL))) |
                                     MASK_VAL(FLD_CODEC_CTRL_ADC0_SL_SEL, !wl, FLD_CODEC_CTRL_ADC0_SR_SEL, !wl); /* 16bit: 1, 20/24bit: 0 */
    }
    else if ((channel == AUDIO_DMIC_ADC_B1) || (channel == AUDIO_DMIC_ADC_B2) || channel == AUDIO_DMIC_ADC_B1_B2) /* ADC_B. */
    {
        reg_audio_codec0_cr_adcb_ai = (reg_audio_codec0_cr_adcb_ai & (~(FLD_CODEC0_ADCB_ADWL | FLD_CODEC0_ADCB_SLAVE))) |
                                      MASK_VAL(FLD_CODEC0_ADCB_SLAVE, 1, FLD_CODEC0_ADCB_ADWL, wl);
        reg_audio_codec_data_fmt_l = (reg_audio_codec_data_fmt_l & (~(FLD_CODEC_CTRL_ADC1_SL_SEL | FLD_CODEC_CTRL_ADC1_SR_SEL))) |
                                     MASK_VAL(FLD_CODEC_CTRL_ADC1_SL_SEL, !wl, FLD_CODEC_CTRL_ADC1_SR_SEL, !wl); /* 16bit: 1, 20/24bit: 0 */
    }
}

/**
 * @brief      This function serves to set codec0 output data world length.
 * @param[in]  output - output channel.
 * @param[in]  wl     - world length.
 * @return     none 
 */
void audio_codec0_set_output_wl(audio_codec0_output_select_e output, audio_codec0_data_select_e wl)
{
    switch (output)
    {
    case AUDIO_DAC_A1: /* DAC_A1. */
        reg_audio_codec_data_fmt_l = (reg_audio_codec_data_fmt_l & (~FLD_CODEC_CTRL_DAC_SL_SEL)) |
                                     MASK_VAL(FLD_CODEC_CTRL_DAC_SL_SEL, !wl); /* 16bit: 1, 20/24bit: 0 */
        break;
    case AUDIO_DAC_A2: /* DAC_A2. */
        reg_audio_codec_data_fmt_l = (reg_audio_codec_data_fmt_l & (~FLD_CODEC_CTRL_DAC_SR_SEL)) |
                                     MASK_VAL(FLD_CODEC_CTRL_DAC_SR_SEL, !wl); /* 16bit: 1, 20/24bit: 0 */
        break;
    case AUDIO_DAC_A1_A2: /* DAC_A1_A2. */
        reg_audio_codec_data_fmt_l = (reg_audio_codec_data_fmt_l & (~(FLD_CODEC_CTRL_DAC_SL_SEL | FLD_CODEC_CTRL_DAC_SR_SEL))) |
                                     MASK_VAL(FLD_CODEC_CTRL_DAC_SL_SEL, !wl, FLD_CODEC_CTRL_DAC_SR_SEL, !wl); /* 16bit: 1, 20/24bit: 0 */
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 input analog gain.
 * @param[in]  input - input channel.
 * @param[in]  gain  - input analog gain.
 * @return     none 
 * @note
 *             - input analog gain only for line_in or amic.
 */
void audio_codec0_set_input_again(audio_codec0_input_select_e input, audio_codec0_input_again_e gain)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1: /* ADC_A1. */
        reg_audio_codec0_gcr_mica12 = (reg_audio_codec0_gcr_mica12 & (~FLD_CODEC0_GIMA1)) | MASK_VAL(FLD_CODEC0_GIMA1, gain);
        break;
    case AUDIO_DMIC_ADC_A2: /* ADC_A2. */
        reg_audio_codec0_gcr_mica12 = (reg_audio_codec0_gcr_mica12 & (~FLD_CODEC0_GIMA2)) | MASK_VAL(FLD_CODEC0_GIMA2, gain);
        break;
    case AUDIO_DMIC_ADC_B1: /* ADC_B1. */
        reg_audio_codec0_gcr_micb12 = (reg_audio_codec0_gcr_micb12 & (~FLD_CODEC0_GIMB1)) | MASK_VAL(FLD_CODEC0_GIMB1, gain);
        break;
    case AUDIO_DMIC_ADC_A1_A2: /* ADC_A1_A2 .*/
        reg_audio_codec0_gcr_mica12 = (gain << 4) | gain;
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 input digital gain.
 * @param[in]  input - input channel.
 * @param[in]  gain  - input digital gain.
 * @return     none 
 */
void audio_codec0_set_input_dgain(audio_codec0_input_select_e input, audio_codec0_input_dgain_e gain)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1: /* ADC_A1 */
    case AUDIO_DMIC_ADC_A2: /* ADC_A2 */
    case AUDIO_DMIC_ADC_B1: /* ADC_B1 */
    case AUDIO_DMIC_ADC_B2: /* ADC_B2 */
        reg_audio_codec0_cr_adc_dgain(channel) = (reg_audio_codec0_cr_adc_dgain(channel) & (~FLD_CODEC0_GOD_ADC)) | gain;
        break;
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2 */
        reg_audio_codec0_cr_adca1_dgain = (reg_audio_codec0_cr_adca1_dgain & (~FLD_CODEC0_GOD_ADCA1)) | gain;
        reg_audio_codec0_cr_adca2_dgain = (reg_audio_codec0_cr_adca2_dgain & (~FLD_CODEC0_GOD_ADCA2)) | gain;
        break;
    case AUDIO_DMIC_ADC_B1_B2: /*ADC_B1_B2 */
        reg_audio_codec0_cr_adcb1_dgain = (reg_audio_codec0_cr_adcb1_dgain & (~FLD_CODEC0_GID_ADCB1)) | gain;
        reg_audio_codec0_cr_adcb2_dgain = (reg_audio_codec0_cr_adcb2_dgain & (~FLD_CODEC0_GID_ADCB2)) | gain;
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 output analog gain.
 * @param[in]  output - output channel.
 * @param[in]  gain   - output analog gain.
 * @return     none 
 */
void audio_codec0_set_output_again(audio_codec0_output_select_e output, audio_codec0_output_again_e gain)
{
    switch (output)
    {
    case AUDIO_DAC_A1: /* DAC_A1. */
        reg_audio_codec0_gcr_hp(AUDIO_DAC_A1) = (reg_audio_codec0_gcr_hp(AUDIO_DAC_A1) & (~FLD_CODEC0_GOA_DAC)) | gain;
        break;
    case AUDIO_DAC_A2: /* DAC_A2. */
        reg_audio_codec0_gcr_hp(AUDIO_DAC_A2) = (reg_audio_codec0_gcr_hp(AUDIO_DAC_A2) & (~FLD_CODEC0_GOA_DAC)) | gain;
        break;
    case AUDIO_DAC_A1_A2: /* DAC_A1_A2. */
        reg_audio_codec0_gcr_hp(AUDIO_DAC_A1) = (reg_audio_codec0_gcr_hp(AUDIO_DAC_A1) & (~FLD_CODEC0_GOA_DAC)) | gain;
        reg_audio_codec0_gcr_hp(AUDIO_DAC_A2) = (reg_audio_codec0_gcr_hp(AUDIO_DAC_A2) & (~FLD_CODEC0_GOA_DAC)) | gain;
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to set codec0 output digital gain.
 * @param[in]  output - output channel.
 * @param[in]  gain   - output digital gain.
 * @return     none 
 */
void audio_codec0_set_output_dgain(audio_codec0_output_select_e output, audio_codec0_output_dgain_e gain)
{
    switch (output)
    {
    case AUDIO_DAC_A1: /* DAC_A1. */
        reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A1) = (reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A1) & (~FLD_CODEC0_GOD_DAC)) | gain;
        break;
    case AUDIO_DAC_A2: /* DAC_A2. */
        reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A2) = (reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A2) & (~FLD_CODEC0_GOD_DAC)) | gain;
        break;
    case AUDIO_DAC_A1_A2: /* DAC_A1_A2. */
        reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A1) = (reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A1) & (~FLD_CODEC0_GOD_DAC)) | gain;
        reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A2) = (reg_audio_codec0_cr_dac_dgain(AUDIO_DAC_A2) & (~FLD_CODEC0_GOD_DAC)) | gain;
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to enable/disable codec0 input SNR optimisation.
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: adc SNR optimisation active, 0:adc SNR optimisation inactive.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 SNR optimisation are set in pairs.
 */
void audio_codec0_set_input_snr_opt(audio_codec0_input_select_e input, unsigned char enable)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1:    /*ADC_A1 */
    case AUDIO_DMIC_ADC_A2:    /*ADC_A2 */
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2 */
        reg_audio_codec0_adca_12_alc_0 = (reg_audio_codec0_adca_12_alc_0 & (~FLD_CODEC0_ADCA_12_SNR_OPT_EN)) |
                                         MASK_VAL(FLD_CODEC0_ADCA_12_SNR_OPT_EN, enable);
        break;
    case AUDIO_DMIC_ADC_B1:    /*ADC_B1 */
    case AUDIO_DMIC_ADC_B2:    /*ADC_B2 */
    case AUDIO_DMIC_ADC_B1_B2: /*ADC_B1_B2 */
        reg_audio_codec0_adcb_12_alc_0 = (reg_audio_codec0_adcb_12_alc_0 & (~FLD_CODEC0_ADCB_12_SNR_OPT_EN)) |
                                         MASK_VAL(FLD_CODEC0_ADCB_12_SNR_OPT_EN, enable);
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to enable/disable codec0 input.
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: enable, 0: disable.
 * @return     none 
 * @note
 *             - Must be disable when switching the input sample rate.
 *             - ADC_A1/2 or ADC_B1/2 are set in pairs.
 */
void audio_codec0_input_en(audio_codec0_input_select_e input, unsigned char enable)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1:    /*ADC_A1 */
    case AUDIO_DMIC_ADC_A2:    /*ADC_A2 */
    case AUDIO_DMIC_ADC_A1_A2: /*ADC_A1_A2 */
        reg_audio_codec_ctrl = (reg_audio_codec_ctrl & (~FLD_CODEC_CTRL_CODEC0_ADC_A_MST_EN)) | MASK_VAL(FLD_CODEC_CTRL_CODEC0_ADC_A_MST_EN, enable);
        break;
    case AUDIO_DMIC_ADC_B1:    /*ADC_B1 */
    case AUDIO_DMIC_ADC_B2:    /*ADC_B2 */
    case AUDIO_DMIC_ADC_B1_B2: /*ADC_B1_B2 */
        reg_audio_codec_ctrl = (reg_audio_codec_ctrl & (~FLD_CODEC_CTRL_CODEC0_ADC_B_MST_EN)) | MASK_VAL(FLD_CODEC_CTRL_CODEC0_ADC_B_MST_EN, enable);
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to enable/disable codec0 input HPF(High Pass Filter).
 * @param[in]  input  - input channel.
 * @param[in]  enable - 1: adc High Pass Filter active, 0:adc High Pass Filter inactive.
 * @return     none 
 * @note
 *             - ADC_A1/2 or ADC_B1/2 High Pass Filter are set in pairs.
 */
void audio_codec0_input_hpf_en(audio_codec0_input_select_e input, unsigned char enable)
{
    unsigned char channel = input & BIT_RNG(0, 3); /* bit[0-3] adc channel. */

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1:    /* ADC_A1. */
    case AUDIO_DMIC_ADC_A2:    /* ADC_A2. */
    case AUDIO_DMIC_ADC_A1_A2: /* ADC_A1_A2. */
        reg_audio_codec0_cr_adca_hpf = (reg_audio_codec0_cr_adca_hpf & (~FLD_CODEC0_ADCA12_HPF_EN)) | enable;
        break;
    case AUDIO_DMIC_ADC_B1:    /* ADC_B1. */
    case AUDIO_DMIC_ADC_B2:    /* ADC_B2. */
    case AUDIO_DMIC_ADC_B1_B2: /* ADC_B1_B2. */
        reg_audio_codec0_cr_adcb_hpf = (reg_audio_codec0_cr_adcb_hpf & (~FLD_CODEC0_ADCB12_HPF_EN)) | enable;
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to init codec0 input.
 * @param[in]  input_config - codec0 input config.
 * @return     none 
 */
void audio_codec0_input_init(audio_codec0_input_config_t *input_config)
{
    unsigned char channel    = input_config->input_src & BIT_RNG(0, 3);
    unsigned char input_type = 0; /* 0: analog data, 1: digital data. */

    audio_codec0_set_input_mute(input_config->input_src, 1); /* adc mute. */
    audio_codec0_set_input_wl(input_config->input_src, input_config->data_format);
    audio_codec0_set_input_fs(input_config->input_src, input_config->sample_rate);

    if (input_config->input_src & BIT(5)) /* amic enable bias and set differential input. */
    {
        input_type = 0;                                        /* analog data. */
        audio_codec0_set_micbias(input_config->input_src, 1);  /* enable bias output(2.5v). */
        audio_codec0_set_adc_mode(input_config->input_src, 1); /* adc differential input. */
    }
    else if (input_config->input_src & BIT(4)) /* line_in set differential input. */
    {
        input_type = 0;                                        /* analog data. */
        audio_codec0_set_adc_mode(input_config->input_src, 1); /* adc differential input. */
    }
    else
    {
        input_type = 1;                                       /* digital data. */
        audio_codec0_dmic_clk_en(input_config->input_src, 1); /* enable dmic clock. */
    }

    switch (channel)
    {
    case AUDIO_DMIC_ADC_A1:                                               /* ADC_A1. */
        BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_CODEC0_ADC_A_MST_EN); /* audio adc A master en, codec adc A is slave. */
        if (0 == input_type)                                              /* analog data. */
        {
            BM_CLR(reg_audio_codec0_cr_adca12, FLD_CODEC0_SB_ADCA1); /* adc A1 channel active. */
        }
        BM_CLR(reg_audio_codec0_cr_adca_ai_sb, FLD_CODEC0_SB_AICR_ADCA12); /* ADCA12 audio interface active. */
        reg_audio_codec0_cr_mic_adca_12_sel = (reg_audio_codec0_cr_mic_adca_12_sel & ~(FLD_CODEC0_ADCA1_MIC_SEL)) |
                                              MASK_VAL(FLD_CODEC0_ADCA1_MIC_SEL, input_type); /* select ADC_A1 input data type. */
        break;
    case AUDIO_DMIC_ADC_A2:                                               /* ADC_A2. */
        BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_CODEC0_ADC_A_MST_EN); /* audio adc A master en, codec adc A is slave. */
        if (0 == input_type)                                              /* analog data. */
        {
            BM_CLR(reg_audio_codec0_cr_adca12, FLD_CODEC0_SB_ADCA2); /* adc A2 channel active. */
        }
        BM_CLR(reg_audio_codec0_cr_adca_ai_sb, FLD_CODEC0_SB_AICR_ADCA12); /* ADCA12 audio interface active. */
        reg_audio_codec0_cr_mic_adca_12_sel = (reg_audio_codec0_cr_mic_adca_12_sel & ~(FLD_CODEC0_ADCA2_MIC_SEL)) |
                                              MASK_VAL(FLD_CODEC0_ADCA2_MIC_SEL, input_type); /* select ADC_A2 input data type. */
        break;
    case AUDIO_DMIC_ADC_B1:                                                /* ADC_B1. */
        BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_CODEC0_ADC_B_MST_EN);  /* audio adc B master en, codec adc B is slave. */
        BM_CLR(reg_audio_codec0_cr_adcb12, FLD_CODEC0_SB_ADCB1);           /* adc B2 channel active. */
        BM_CLR(reg_audio_codec0_cr_adcb_ai_sb, FLD_CODEC0_SB_AICR_ADCB12); /* ADCB12 audio interface active. */
        reg_audio_codec0_cr_mic_adcb_12_sel = (reg_audio_codec0_cr_mic_adcb_12_sel & ~(FLD_CODEC0_ADCB1_MIC_SEL)) |
                                              MASK_VAL(FLD_CODEC0_ADCB1_MIC_SEL, input_type); /* select ADC_B1 input data type. */
        if (1 == input_type) /* digital data, set dmic1 clk(dmic0 clk default is 3072KHZ). */
        {
            audio_codec0_set_dmic1_clk(AUDIO_CODEC0_DMIC_CLK_3072KHZ);
        }
        break;
    case AUDIO_DMIC_ADC_B2:                                                    /* ADC_B2. */
        BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_CODEC0_ADC_B_MST_EN);      /* audio adc B master en, codec adc B is slave. */
        BM_CLR(reg_audio_codec0_cr_adcb_ai_sb, FLD_CODEC0_SB_AICR_ADCB12);     /* ADCB12 audio interface active. */
        BM_SET(reg_audio_codec0_cr_mic_adcb_12_sel, FLD_CODEC0_ADCB2_MIC_SEL); /* set ADC_B2 digital input. */
        if (1 == input_type)                                                   /* digital data, set dmic1 clk(dmic0 clk default is 3072KHZ). */
        {
            audio_codec0_set_dmic1_clk(AUDIO_CODEC0_DMIC_CLK_3072KHZ);
        }
        break;
    case AUDIO_DMIC_ADC_A1_A2:                                            /* ADC_A1_A2. */
        BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_CODEC0_ADC_A_MST_EN); /* audio adc A master en, codec adc A is slave. */
        if (0 == input_type)                                              /* analog data. */
        {
            reg_audio_codec0_cr_adca12 = (reg_audio_codec0_cr_adca12 & (~(FLD_CODEC0_SB_ADCA1 | FLD_CODEC0_SB_ADCA2))) |
                                         MASK_VAL(FLD_CODEC0_SB_ADCA1, 0, FLD_CODEC0_SB_ADCA2, 0); /* adc A1/A2 channel active. */
        }
        BM_CLR(reg_audio_codec0_cr_adca_ai_sb, FLD_CODEC0_SB_AICR_ADCA12); /* ADCA12 audio interface active. */
        reg_audio_codec0_cr_mic_adca_12_sel =
                (reg_audio_codec0_cr_mic_adca_12_sel & ~((FLD_CODEC0_ADCA1_MIC_SEL | FLD_CODEC0_ADCA2_MIC_SEL))) |
                MASK_VAL(FLD_CODEC0_ADCA1_MIC_SEL, input_type, FLD_CODEC0_ADCA2_MIC_SEL, input_type); /* select ADC_A1/ADC_A2 input data type. */
        break;
    case AUDIO_DMIC_ADC_B1_B2:                                             /* ADC_B1_B2. */
        BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_CODEC0_ADC_B_MST_EN);  /* audio adc B master en, codec adc B is slave. */
        BM_CLR(reg_audio_codec0_cr_adcb12, FLD_CODEC0_SB_ADCB1);           /* adc B2 channel active. */
        BM_CLR(reg_audio_codec0_cr_adcb_ai_sb, FLD_CODEC0_SB_AICR_ADCB12); /* ADCB12 audio interface active. */
        reg_audio_codec0_cr_mic_adcb_12_sel = (reg_audio_codec0_cr_mic_adcb_12_sel & ~(FLD_CODEC0_ADCB1_MIC_SEL | FLD_CODEC0_ADCB2_MIC_SEL)) |
                                              MASK_VAL(FLD_CODEC0_ADCB1_MIC_SEL, 1, FLD_CODEC0_ADCB2_MIC_SEL, 1); /* select ADC_B1 input data type. */
        if (1 == input_type) /* digital data, set dmic1 clk(dmic0 clk default is 3072KHZ). */
        {
            audio_codec0_set_dmic1_clk(AUDIO_CODEC0_DMIC_CLK_3072KHZ);
        }
        break;
    default:
        break;
    }

    delay_ms(1);                                             /* codec power-up timing requirements. */
    audio_codec0_set_input_mute(input_config->input_src, 0); /* adc un-mute. */
    delay_ms(25);                                            /* codec power-up timing requirements. */
}

/**
 * @brief      This function serves to init codec0 output config.
 * @param[in]  output_config - codec0 output config.
 * @return     none 
 */
void audio_codec0_output_init(audio_codec0_output_config_t *output_config)
{
    audio_codec0_set_output_mute(output_config->output_dst, 1); /* dac mute. */
    audio_codec0_set_output_wl(output_config->output_dst, output_config->data_format);
    audio_codec0_set_output_fs(output_config->sample_rate);
    BM_SET(reg_audio_codec_ctrl, FLD_CODEC_CTRL_DAC_MST_EN); /* audio dac master en, codec dac is slave. */

    reg_audio_codec0_cr_daca_ai = (reg_audio_codec0_cr_daca_ai & (~(FLD_CODEC0_SB_AICR_DACA | FLD_CODEC0_DACA_SLAVE))) |
                                  MASK_VAL(FLD_CODEC0_SB_AICR_DACA, 0, FLD_CODEC0_DACA_SLAVE, 1); /* DACA audio interface active and select slave. */

    switch (output_config->output_dst)
    {
    case AUDIO_DAC_A1:                                           /* DAC_A1. */
        BM_CLR(reg_audio_codec0_cr_daca12, FLD_CODEC0_SB_DACA1); /* DAC A1 active. */
        BM_CLR(reg_audio_codec0_cr_hpa, FLD_CODEC0_SB_HPA1);     /* Headphone A1 output stage is active. */
        break;
    case AUDIO_DAC_A2:                                           /* DAC_A2. */
        BM_CLR(reg_audio_codec0_cr_daca12, FLD_CODEC0_SB_DACA2); /* DAC A2 active. */
        BM_CLR(reg_audio_codec0_cr_hpa, FLD_CODEC0_SB_HPA2);     /* Headphone A2 output stage is active. */
        break;
    case AUDIO_DAC_A1_A2: /* DAC_A1_A2. */
        reg_audio_codec0_cr_daca12 = (reg_audio_codec0_cr_daca12 & (~(FLD_CODEC0_SB_DACA1 | FLD_CODEC0_SB_DACA2))) |
                                     MASK_VAL(FLD_CODEC0_SB_DACA1, 0, FLD_CODEC0_SB_DACA2, 0); /* DAC A1/A2 active. */
        reg_audio_codec0_cr_hpa = (reg_audio_codec0_cr_hpa & (~(FLD_CODEC0_SB_HPA1 | FLD_CODEC0_SB_HPA2))) |
                                  MASK_VAL(FLD_CODEC0_SB_HPA1, 0, FLD_CODEC0_SB_HPA2, 0); /* Headphone A1/A2 output stage is active. */
        break;
    default:
        break;
    }

    delay_ms(1);                                                /* codec power-up timing requirements */
    audio_codec0_set_output_mute(output_config->output_dst, 0); /* dac un-mute. */
    delay_ms(25);                                               /* codec power-up timing requirements */
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio dma/fifo interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio dma/fifo interface
 * @{
 */

/**
 * @brief      This function serves to config rx_dma channel.
 * @param[in]  chn          - dma channel.
 * @param[in]  dst_addr     - Pointer to data buffer, it must be 4-bytes aligned address.
 *                            and the actual buffer size defined by the user needs to be not smaller than the data_len, otherwise there may be an out-of-bounds problem.
 * @param[in]  data_len     - Length of DMA in bytes, it must be set to a multiple of 4. The maximum value that can be set is 0x10000.
 * @param[in]  head_of_list - the head address of dma llp.
 * @return     none
 */
void audio_rx_dma_config(dma_chn_e chn, unsigned short *dst_addr, unsigned int data_len, dma_chain_config_t *head_of_list)
{
    audio_rx_dma_chn = chn;
    audio_set_rx_buff_len(audio_rx_fifo_chn, data_len);
    dma_config(chn, &audio_dma_rx_config[audio_rx_fifo_chn]);
    dma_set_address(chn, REG_AUDIO_FIFO_ADDR(audio_rx_fifo_chn), (unsigned int)(dst_addr));
    dma_set_size(chn, data_len, DMA_WORD_WIDTH);
    reg_dma_llp(chn) = (unsigned int)(head_of_list);
}

/**
 * @brief      This function serves to set rx dma chain transfer.
 * @param[in]  config_addr - the head of list of llp_pointer.
 * @param[in]  llpointer   - the next element of llp_pointer.
 * @param[in]  dst_addr    - Pointer to data buffer, it must be 4-bytes aligned address and the actual buffer size defined by the user needs to \n
 *                           be not smaller than the data_len, otherwise there may be an out-of-bounds problem.
 * @param[in]  data_len    - Length of DMA in bytes, it must be set to a multiple of 4. The maximum value that can be set is 0x10000.
 * @return     none
 */
void audio_rx_dma_add_list_element(dma_chain_config_t *config_addr, dma_chain_config_t *llpointer, unsigned short *dst_addr, unsigned int data_len)
{
    config_addr->dma_chain_ctl      = reg_dma_ctrl(audio_rx_dma_chn) | FLD_DMA_CHANNEL_ENABLE;
    config_addr->dma_chain_src_addr = REG_AUDIO_FIFO_ADDR(audio_rx_fifo_chn);
    config_addr->dma_chain_dst_addr = (unsigned int)(dst_addr);
    config_addr->dma_chain_data_len = dma_cal_size(data_len, 4);
    config_addr->dma_chain_llp_ptr  = (unsigned int)(llpointer);
}

/**
 * @brief      This function serves to set audio rx dma chain transfer.
 * @param[in]  rx_fifo_chn - rx fifo select.
 * @param[in]  chn         - dma channel.
 * @param[in]  in_buff     - Pointer to data buffer, it must be 4-bytes aligned address and the actual buffer size defined by the user needs to \n
 *                           be not smaller than the data_len, otherwise there may be an out-of-bounds problem.
 * @param[in]  buff_size   - Length of DMA in bytes, it must be set to a multiple of 4. The maximum value that can be set is 0x10000.
 * @return     none
 */
void audio_rx_dma_chain_init(audio_fifo_chn_e rx_fifo_chn, dma_chn_e chn, unsigned short *in_buff, unsigned int buff_size)
{
    audio_rx_fifo_chn = rx_fifo_chn;
    BM_SET(reg_audio_dma_ptr_en, BIT(4 + rx_fifo_chn)); /* if want to get rx write pointer(audio_get_rx_wptr()), rx0_wptr_en must enable. */
    audio_rx_dma_config(chn, (unsigned short *)in_buff, buff_size, &g_audio_rx_dma_list_cfg[rx_fifo_chn]);
    audio_rx_dma_add_list_element(&g_audio_rx_dma_list_cfg[rx_fifo_chn], &g_audio_rx_dma_list_cfg[rx_fifo_chn], (unsigned short *)in_buff, buff_size);
}

/**
 * @brief      This function serves to config  tx_dma channel.
 * @param[in]  chn          - dma channel.
 * @param[in]  src_addr     - Pointer to data buffer, it must be 4-bytes aligned address.
 * @param[in]  data_len     - Length of DMA in bytes, range from 1 to 0x10000.
 * @param[in]  head_of_list - the head address of dma llp.
 * @return     none
 */
void audio_tx_dma_config(dma_chn_e chn, unsigned short *src_addr, unsigned int data_len, dma_chain_config_t *head_of_list)
{
    audio_tx_dma_chn = chn;
    audio_set_tx_buff_len(audio_tx_fifo_chn, data_len);
    dma_config(chn, &audio_dma_tx_config[audio_tx_fifo_chn]);
    dma_set_address(chn, (unsigned int)(src_addr), REG_AUDIO_FIFO_ADDR(audio_tx_fifo_chn));
    dma_set_size(chn, data_len, DMA_WORD_WIDTH);
    reg_dma_llp(chn) = (unsigned int)head_of_list;
}

/**
 * @brief      This function serves to set tx dma chain transfer.
 * @param[in]  config_addr - the head of list of llp_pointer.
 * @param[in]  llpointer   - the next element of llp_pointer.
 * @param[in]  src_addr    - Pointer to data buffer, it must be 4-bytes aligned address.
 * @param[in]  data_len    - Length of DMA in bytes, range from 1 to 0x10000.
 * @return     none
 */
void audio_tx_dma_add_list_element(dma_chain_config_t *config_addr, dma_chain_config_t *llpointer, unsigned short *src_addr, unsigned int data_len)
{
    config_addr->dma_chain_ctl      = reg_dma_ctrl(audio_tx_dma_chn) | FLD_DMA_CHANNEL_ENABLE;
    config_addr->dma_chain_src_addr = (unsigned int)src_addr;
    config_addr->dma_chain_dst_addr = REG_AUDIO_FIFO_ADDR(audio_tx_fifo_chn);
    config_addr->dma_chain_data_len = dma_cal_size(data_len, 4);
    config_addr->dma_chain_llp_ptr  = (unsigned int)llpointer;
}

/**
 * @brief      This function serves to initialize audio tx dma chain transfer.
 * @param[in]  tx_fifo_chn - tx fifo select.
 * @param[in]  chn         - dma channel.
 * @param[in]  out_buff    - Pointer to data buffer, it must be 4-bytes aligned address.
 * @param[in]  buff_size   - Length of DMA in bytes, range from 1 to 0x10000.
 * @return     none
 */
void audio_tx_dma_chain_init(audio_fifo_chn_e tx_fifo_chn, dma_chn_e chn, unsigned short *out_buff, unsigned int buff_size)
{
    audio_tx_fifo_chn = tx_fifo_chn;
    audio_tx_dma_config(chn, (unsigned short *)out_buff, buff_size, &g_audio_tx_dma_list_cfg[tx_fifo_chn]);
    audio_tx_dma_add_list_element(&g_audio_tx_dma_list_cfg[tx_fifo_chn], &g_audio_tx_dma_list_cfg[tx_fifo_chn], (unsigned short *)out_buff,
                                  buff_size);
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio hac interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio hac interface
 * @{
 */

/**
 * @brief      This function servers to update hac biquad filter coefficients.
 *
 * @param[in]  hac_chn - hac channel.
 * @param[in]  biquad  - biquad step audio_hac_biquad_e.
 * @param[in]  data    - biquad filter data address, [b0, b1, b2, a1, a2].
 * @return     none
 */
void audio_hac_biquad_coef_update(audio_hac_chn_e hac_chn, audio_hac_biquad_e biquad, signed int *data)
{
    reg_audio_hac_bq_b0(hac_chn, biquad) = data[0];
    reg_audio_hac_bq_b1(hac_chn, biquad) = data[1];
    reg_audio_hac_bq_b2(hac_chn, biquad) = data[2];
    reg_audio_hac_bq_a1(hac_chn, biquad) = data[3];
    reg_audio_hac_bq_a2(hac_chn, biquad) = data[4];
}

/**
 * @brief This function servers to select hac's asrc input and output fs.
 * 
 * @param[in]  hac_chn   - asrc channel.
 * @param[in]  fs_in_out - input and output fs audio_hac_fs_in_out_e.
 * @param[in]  ppm       - ppm value.
 * @return     none
 */
void audio_asrc_fs_select(audio_hac_chn_e hac_chn, audio_hac_fs_in_out_e fs_in_out, int ppm)
{
    int frac_advance = 0;
    int den_rate     = 0;
    char int_advance = 0;

    if (fs_in_out >= BIT(8)) /* in_fs == out_fs, only ppm work */
    {
        den_rate = 1000000;
        if (ppm <= 0)
        {
            frac_advance = 1000000 + ppm * 16;
            int_advance  = 15;
        }
        else
        {
            frac_advance = ppm * 16;
            int_advance  = 16;
        }
    }
    else
    {
        switch (fs_in_out)
        {
        case AUDIO_ASRC_FS_IN_32K_OUT_16K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 32;
                int_advance  = 31;
            }
            else
            {
                frac_advance = ppm * 32;
                int_advance  = 32;
            }
            break;
        case AUDIO_ASRC_FS_IN_44P1K_OUT_16K:
            den_rate     = 10000000;
            frac_advance = 1000000 + ppm * 441;
            int_advance  = 44;
            break;
        case AUDIO_ASRC_FS_IN_48K_OUT_16K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 48;
                int_advance  = 47;
            }
            else
            {
                frac_advance = ppm * 48;
                int_advance  = 48;
            }
            break;
        case AUDIO_ASRC_FS_IN_96K_OUT_16K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 96;
                int_advance  = 95;
            }
            else
            {
                frac_advance = ppm * 96;
                int_advance  = 96;
            }
            break;
        case AUDIO_ASRC_FS_IN_16K_OUT_32K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 8;
                int_advance  = 7;
            }
            else
            {
                frac_advance = ppm * 8;
                int_advance  = 8;
            }
            break;
        case AUDIO_ASRC_FS_IN_44P1K_OUT_32K:
            den_rate     = 20000000;
            frac_advance = 1000000 + ppm * 441;
            int_advance  = 22;
            break;
        case AUDIO_ASRC_FS_IN_48K_OUT_32K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 24;
                int_advance  = 23;
            }
            else
            {
                frac_advance = ppm * 24;
                int_advance  = 24;
            }
            break;
        case AUDIO_ASRC_FS_IN_96K_OUT_32K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 48;
                int_advance  = 47;
            }
            else
            {
                frac_advance = ppm * 48;
                int_advance  = 48;
            }
            break;
        case AUDIO_ASRC_FS_IN_16K_OUT_44P1K:
            den_rate     = 1378125; /* 459375 * 3 */
            frac_advance = 1109375 + ppm * 8;
            int_advance  = 5;
            break;
        case AUDIO_ASRC_FS_IN_32K_OUT_44P1K:
            den_rate     = 1378125; /* 459375 * 3 */
            frac_advance = 840625 + ppm * 16;
            int_advance  = 11;
            break;
        case AUDIO_ASRC_FS_IN_48K_OUT_44P1K:
            den_rate     = 459375;
            frac_advance = 190625 + ppm * 8;
            int_advance  = 17;
            break;
        case AUDIO_ASRC_FS_IN_96K_OUT_44P1K:
            den_rate     = 459375;
            frac_advance = 381250 + ppm * 16;
            int_advance  = 34;
            break;
        case AUDIO_ASRC_FS_IN_16K_OUT_48K:
            den_rate     = 3000000;
            frac_advance = 1000000 + ppm * 16;
            int_advance  = 5;
            break;
        case AUDIO_ASRC_FS_IN_32K_OUT_48K:
            den_rate     = 3000000;
            frac_advance = 2000000 + ppm * 32;
            int_advance  = 10;
            break;
        case AUDIO_ASRC_FS_IN_44P1K_OUT_48K:
            den_rate     = 10000000;
            frac_advance = 7000000 + ppm * 147;
            int_advance  = 14;
            break;
        case AUDIO_ASRC_FS_IN_96K_OUT_48K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 32;
                int_advance  = 31;
            }
            else
            {
                frac_advance = ppm * 32;
                int_advance  = 32;
            }
            break;
        case AUDIO_ASRC_FS_IN_16K_OUT_96K:
            den_rate     = 3000000;
            frac_advance = 2000000 + ppm * 8;
            int_advance  = 2;
            break;
        case AUDIO_ASRC_FS_IN_32K_OUT_96K:
            den_rate     = 3000000;
            frac_advance = 1000000 + ppm * 16;
            int_advance  = 5;
            break;
        case AUDIO_ASRC_FS_IN_44P1K_OUT_96K:
            den_rate     = 20000000;
            frac_advance = 7000000 + ppm * 147;
            int_advance  = 7;
            break;
        case AUDIO_ASRC_FS_IN_48K_OUT_96K:
            den_rate = 1000000;
            if (ppm <= 0)
            {
                frac_advance = 1000000 + ppm * 8;
                int_advance  = 7;
            }
            else
            {
                frac_advance = ppm * 8;
                int_advance  = 8;
            }
            break;
        default:
            break;
        }
    }

    audio_asrc_interval_set(hac_chn, fs_in_out);
    audio_asrc_frac_adc_set(hac_chn, frac_advance);
    audio_asrc_den_rate_set(hac_chn, den_rate);
    audio_asrc_int_adv_set(hac_chn, int_advance);
    audio_asrc_lag_int_config_done(hac_chn);
}

/**
 * @brief      This function serves to get asrc out data cnt.
 *
 * @param[in]  hac_chn - hac channel.
 * @return     hac out data cnt, 0 means timeout, the unit is word.
 * @note
 *             - You need to stop the ASRC data input before calling this interface.
 */
unsigned int audio_asrc_get_out_data_cnt(audio_hac_chn_e hac_chn)
{
    unsigned int fifo_cnt = 0;

    delay_us(313); /* Wait for ASRC calculation to complete, maximum wait time is 16K samples, 5 samples(5 / 16000 * 1000000 = 313us). */

    BM_SET(reg_audio_hac_tx_fifo_cnt_ind(hac_chn), FLD_HAC_TX_FIFO_INDICATE); /* Write 1 means software need to read txfifo_cnt */
    unsigned int ref_tick = stimer_get_tick();
    while (BM_IS_CLR(reg_audio_hac_tx_fifo_cnt_ind(hac_chn), FLD_HAC_TX_FIFO_INDICATE)) /* bit set means read done. */
    {
        if (clock_time_exceed(ref_tick, 10000)) /* timeout. */
            return 0;
    }
    fifo_cnt = reg_audio_hac_tx_fifo_cnt_ind(hac_chn) & FLD_HAC_TX_FIFO_CNT;
    BM_CLR(reg_audio_hac_tx_fifo_cnt_ind(hac_chn), FLD_HAC_TX_FIFO_INDICATE); /* clear tx data done */

    return fifo_cnt;
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio I2S interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio I2S interface
 * @{
 */

/**
 * @brief      This function serves to config i2s io mode.
 * 
 * @param[in]  i2s_sel - i2s select.
 * @param[in]  io_mode - io mode.
 * @return     none
 */
static void audio_i2s_io_mode_select(i2s_select_e i2s_sel, i2s_io_mode_e io_mode)
{
    switch (io_mode)
    {
    case I2S_5_LINE_MODE:
        reg_audio_i2s_route(i2s_sel) = (reg_audio_i2s_route(i2s_sel) & (~FLD_I2S_MODE)) | I2S_5_LINE_MODE;
        break;
    case I2S_4_LINE_DAC_MODE:
        reg_audio_i2s_route(i2s_sel) = (reg_audio_i2s_route(i2s_sel) & (~FLD_I2S_MODE)) | I2S_4_LINE_DAC_MODE;
        break;
    case I2S_4_LINE_ADC_MODE:
        reg_audio_i2s_route(i2s_sel) = (reg_audio_i2s_route(i2s_sel) & (~FLD_I2S_MODE)) | I2S_4_LINE_ADC_MODE;
        break;
    case I2S_2_LANE_TX_MODE:
        BM_SET(reg_audio_i2s_cfg3(i2s_sel), FLD_I2S_TX_2LINE_EN);
        break;
    case I2S_2_LANE_RX_MODE:
        BM_SET(reg_audio_i2s_cfg3(i2s_sel), FLD_I2S_RX_2LINE_EN);
        break;
    default:
        break;
    }
}

/**
 * @brief      This function serves to config i2s0 interface, word length, and m/s.
 * @param[in]  i2s_sel      - i2s channel select
 * @param[in]  i2s_format   - interface protocol
 * @param[in]  wl           - audio data word length
 * @param[in]  m_s          - select i2s as master or slave
 * @param[in]  i2s_config_t - the ptr of i2s_config_t that configure i2s lr_clk phase and lr_clk swap.
 *  i2s_config_t->i2s_lr_clk_invert_select-lr_clk phase control(in RJ,LJ or i2s modes),in i2s mode(opposite phasing in  RJ,LJ mode), 0=right channel data when lr_clk high ,1=right channel data when lr_clk low.
 *                                                                                     in DSP mode(in DSP mode only), DSP mode A/B select,0=DSP mode A ,1=DSP mode B.
 *            i2s_config_t->i2s_data_invert_select - 0=left channel data left,1=right channel data left.
 * but data output channel will be inverted,you can also set i2s_config_t->i2s_data_invert_select=1 to recovery it.
 * @return    none
 */
static void audio_i2s_config(i2s_select_e i2s_sel, i2s_mode_select_e i2s_format, i2s_wl_mode_e wl, i2s_m_s_mode_e m_s,
                             i2s_invert_config_t *i2s_config_t)
{
    reg_audio_i2s_cfg1(i2s_sel) = (reg_audio_i2s_cfg1(i2s_sel) & (~(FLD_I2S_ADC_DCI_MS | FLD_I2S_DAC_DCI_MS))) |
                                  MASK_VAL(FLD_I2S_ADC_DCI_MS, m_s, FLD_I2S_DAC_DCI_MS, m_s);

    reg_audio_i2s_cfg2(i2s_sel) = (reg_audio_i2s_cfg2(i2s_sel) & (~(FLD_I2S_Wl | FLD_I2S_FORMAT))) |
                                  MASK_VAL(FLD_I2S_Wl, wl, FLD_I2S_FORMAT, i2s_format);

    reg_audio_i2s_cfg3(i2s_sel) =
            (reg_audio_i2s_cfg3(i2s_sel) & (~(FLD_I2S_LR_SWAP | FLD_I2S_LRP))) |
            MASK_VAL(FLD_I2S_LR_SWAP, i2s_config_t->i2s_data_invert_select, FLD_I2S_LRP, i2s_config_t->i2s_lr_clk_invert_select);
}

/**
 * @brief      This function serves to config tdm mode, word length, slot width, and master/slave.
 * 
 * @param[in]  tdm_mode       - tdm mode.
 * @param[in]  tdm_slot_width - tdm slow width.
 * @param[in]  rx_ch_num      - tdm rx channel num.
 * @param[in]  tx_ch_num      - tdm tx channel num.
 * @return     none
 */
static void audio_i2s_tdm_config(i2s_tdm_mode_select_e tdm_mode, i2s_tdm_slot_width_e tdm_slot_width, unsigned char rx_ch_num,
                                 unsigned char tx_ch_num)
{
    reg_audio_i2s0_tdm_cfg = MASK_VAL(FLD_I2S_TDM_RX_CH_NUM, ((rx_ch_num - 1) >> 1), FLD_I2S_TDM_TX_CH_NUM, ((tx_ch_num - 1) >> 1), FLD_I2S_TDM_MODE,
                                      tdm_mode, FLD_I2S_TDM_SLOT, tdm_slot_width);
}

/**
 * @brief      This function serves to set sampling rate when i2s as master.
 * @param[in]  i2s_select - i2s channel select
 * @param[in]  i2s_clk_config                         i2s_clk_config[2]                   i2s_clk_config[3]-->lrclk_adc(sampling rate)
                                                             ||                                 ||
 *   audio_pll(36.864M default)------>div---->i2s_clk--->2 * div(div = 0, bypass)--->blck----->div
 *                                    ||                                                        ||
 *                     i2s_clk_config[0]/i2s_clk_config[1]                               i2s_clk_config[4]-->lrclk_dac (sampling rate)
 *
 *  For example: sampling rate = 16K, i2s_clk_config[5] = { 1, 3, 6, 64, 64 }, sampling rate = 36.864MHz * (1 / 3) / (2 * 6) / (64)  = 16KHz.
 * @return    none
 * @attention The default is from audio_pll 36.864M(default). If the audio_pll is changed, the clk will be changed accordingly.
 */
static void audio_i2s_set_clock(i2s_select_e i2s_select, unsigned short *i2s_clk_config)
{
    audio_i2s_set_clk(i2s_select, i2s_clk_config[0], i2s_clk_config[1]);
    audio_i2s_set_bclk(i2s_select, i2s_clk_config[2]);
    audio_i2s_set_lrclk(i2s_select, i2s_clk_config[3], i2s_clk_config[4]);
}

/**
 * @brief      This function serves to initialize configuration i2s.
 * @param[in]  i2s_config - the relevant configuration struct pointer @see audio_i2s_config_t.
 * @return     none
 * @note
 *             - audio_pll default 36.864MHz. If the audio_pll is changed, the clk will be changed accordingly.
 */
void audio_i2s_config_init(audio_i2s_config_t *i2s_config)
{
    audio_i2s_set_pin(i2s_config->i2s_select, i2s_config->pin_config);
    if (i2s_config->master_slave_mode == I2S_AS_MASTER_EN)
    {
        audio_i2s_set_clock(i2s_config->i2s_select, i2s_config->sample_rate);
    }
    audio_i2s_io_mode_select(i2s_config->i2s_select, i2s_config->io_mode);
    audio_i2s_config(i2s_config->i2s_select, i2s_config->i2s_mode, i2s_config->data_width, i2s_config->master_slave_mode,
                     &audio_i2s_invert_config[i2s_config->i2s_select]);

    if (i2s_config->i2s_mode == I2S_TDM_MODE && i2s_config->i2s_select == I2S0)
    {
        audio_i2s_tdm_config(i2s_config->tdm_mode, i2s_config->tdm_slot_width, (i2s_config->sample_rate[3] / (16 + i2s_config->tdm_slot_width * 8)),
                             i2s_config->sample_rate[4] / (16 + i2s_config->tdm_slot_width * 8));
    }

    reg_audio_clk_en |= BIT(1 + i2s_config->i2s_select);
    audio_i2s_clk_en(i2s_config->i2s_select);
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio matrix interface                                              *
 *********************************************************************************************************************/
/*!
 * @name Audio matrix interface
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
void audio_matrix_set_rx_fifo_route(audio_fifo_chn_e fifo_num, audio_matrix_rx_fifo_route_e route_from, audio_matrix_rx_fifo_format_e data_format)
{
    reg_audio_matrix_fifo_wr_sel(fifo_num) = (reg_audio_matrix_fifo_wr_sel(fifo_num) & (~FLD_MATRIX_FIFO_WR_SEL)) | route_from;

    switch (route_from)
    {
    case FIFO_RX_ROUTE_I2S0_RX:
        reg_audio_matrix_i2s0_rx_sel = (reg_audio_matrix_i2s0_rx_sel & (~FLD_MATRIX_I2S0_RX_SEL)) | data_format;
        break;
    case FIFO_RX_ROUTE_I2S1_RX:
        reg_audio_matrix_i2s1_rx_sel = (reg_audio_matrix_i2s1_rx_sel & (~FLD_MATRIX_I2S1_RX_SEL)) | data_format;
        break;
    case FIFO_RX_ROUTE_I2S2_RX:
        reg_audio_matrix_i2s2_rx_sel = (reg_audio_matrix_i2s2_rx_sel & (~FLD_MATRIX_I2S2_RX_SEL)) | data_format;
        break;
    case FIFO_RX_ROUTE_ANC0:
        if (fifo_num <= FIFO1)
        {
            reg_audio_matrix_anc0_rx_sel = (reg_audio_matrix_anc0_rx_sel & (~FLD_MATRIX_ANC0_RX_SEL)) | MASK_VAL(FLD_MATRIX_ANC0_RX_SEL, data_format);
        }
        else
        {
            reg_audio_matrix_anc0_rx_sel = (reg_audio_matrix_anc0_rx_sel & (~FLD_MATRIX_ANC0_RX_V2_SEL)) |
                                           MASK_VAL(FLD_MATRIX_ANC0_RX_V2_SEL, data_format);
        }
        break;
    case FIFO_RX_ROUTE_ANC1:
        if (fifo_num <= FIFO1)
        {
            reg_audio_matrix_anc1_rx_sel = (reg_audio_matrix_anc1_rx_sel & (~FLD_MATRIX_ANC1_RX_SEL)) | MASK_VAL(FLD_MATRIX_ANC1_RX_SEL, data_format);
        }
        else
        {
            reg_audio_matrix_anc1_rx_sel = (reg_audio_matrix_anc1_rx_sel & (~FLD_MATRIX_ANC1_RX_V2_SEL)) |
                                           MASK_VAL(FLD_MATRIX_ANC1_RX_V2_SEL, data_format);
        }
        break;
    case FIFO_RX_ROUTE_CODEC0_ADCA:
        reg_audio_matrix_adc01_sel = (reg_audio_matrix_adc01_sel & (~FLD_MATRIX_ADC0_SEL)) | MASK_VAL(FLD_MATRIX_ADC0_SEL, data_format);
        break;
    case FIFO_RX_ROUTE_CODEC0_ADCB:
        reg_audio_matrix_adc01_sel = (reg_audio_matrix_adc01_sel & (~FLD_MATRIX_ADC1_SEL)) | MASK_VAL(FLD_MATRIX_ADC1_SEL, data_format);
        break;
    case FIFO_RX_ROUTE_CODEC1_ADCA:
        reg_audio_matrix_adc2_sel = (reg_audio_matrix_adc2_sel & (~FLD_MATRIX_ADC2_SEL)) | MASK_VAL(FLD_MATRIX_ADC2_SEL, data_format);
        break;
    default:
        break;
    }
}

/**
 * @brief   This function serves to select i2s tx route source and data format.
 *
 * @param[in]  i2s_tx_chn  - i2s tx channel.
 * @param[in]  route_from  - i2s tx route from.
 * @param[in]  data_format - i2s tx data format(route from fifo valid).
 * @return     none
 */
void audio_matrix_set_i2s_tx_route(audio_i2s_tx_chn_e i2s_tx_chn, audio_matrix_i2s_tx_route_e route_from, audio_matrix_i2s_tx_format_e data_format)
{
    switch (i2s_tx_chn)
    {
    case I2S0_CHN0:
        reg_audio_matrix_i2s0_ch01_tx_sel = (reg_audio_matrix_i2s0_ch01_tx_sel & (~FLD_MATRIX_I2S0_CH0_TX_SEL)) | route_from;
        break;
    case I2S0_CHN1:
        reg_audio_matrix_i2s0_ch01_tx_sel = (reg_audio_matrix_i2s0_ch01_tx_sel & (~FLD_MATRIX_I2S0_CH1_TX_SEL)) | (route_from << 4);
        break;
    case I2S0_CHN2:
        reg_audio_matrix_i2s0_ch23_tx_sel = (reg_audio_matrix_i2s0_ch23_tx_sel & (~FLD_MATRIX_I2S0_CH2_TX_SEL)) | route_from;
        break;
    case I2S0_CHN3:
        reg_audio_matrix_i2s0_ch23_tx_sel = (reg_audio_matrix_i2s0_ch23_tx_sel & (~FLD_MATRIX_I2S0_CH3_TX_SEL)) | (route_from << 4);
        break;
    case I2S0_CHN4:
        reg_audio_matrix_i2s0_ch45_tx_sel = (reg_audio_matrix_i2s0_ch45_tx_sel & (~FLD_MATRIX_I2S0_CH4_TX_SEL)) | route_from;
        break;
    case I2S0_CHN5:
        reg_audio_matrix_i2s0_ch45_tx_sel = (reg_audio_matrix_i2s0_ch45_tx_sel & (~FLD_MATRIX_I2S0_CH5_TX_SEL)) | (route_from << 4);
        break;
    case I2S0_CHN6:
        reg_audio_matrix_i2s0_ch67_tx_sel = (reg_audio_matrix_i2s0_ch67_tx_sel & (~FLD_MATRIX_I2S0_CH6_TX_SEL)) | route_from;
        break;
    case I2S0_CHN7:
        reg_audio_matrix_i2s0_ch67_tx_sel = (reg_audio_matrix_i2s0_ch67_tx_sel & (~FLD_MATRIX_I2S0_CH7_TX_SEL)) | (route_from << 4);
        break;
    case I2S1_CHN0:
        reg_audio_matrix_i2s1_ch01_tx_sel = (reg_audio_matrix_i2s1_ch01_tx_sel & (~FLD_MATRIX_I2S1_CH0_TX_SEL)) | route_from;
        break;
    case I2S1_CHN1:
        reg_audio_matrix_i2s1_ch01_tx_sel = (reg_audio_matrix_i2s1_ch01_tx_sel & (~FLD_MATRIX_I2S1_CH1_TX_SEL)) | (route_from << 4);
        break;
    case I2S2_CHN0:
        reg_audio_matrix_i2s2_ch01_tx_sel = (reg_audio_matrix_i2s2_ch01_tx_sel & (~FLD_MATRIX_I2S2_CH0_TX_SEL)) | route_from;
        break;
    case I2S2_CHN1:
        reg_audio_matrix_i2s2_ch01_tx_sel = (reg_audio_matrix_i2s2_ch01_tx_sel & (~FLD_MATRIX_I2S2_CH1_TX_SEL)) | (route_from << 4);
        break;
    default:
        break;
    }

    /* set i2s data format. */
    if (route_from == I2S_TX_ROUTE_FIFO)
    {
        /* find i2s num. */
        unsigned char i2s_select = 0;
        if (i2s_tx_chn <= I2S0_CHN7)
        {
            i2s_select = I2S0;
        }
        else if ((i2s_tx_chn > I2S0_CHN7) && (i2s_tx_chn <= I2S1_CHN1))
        {
            i2s_select = I2S1;
        }
        else
        {
            i2s_select = I2S2;
        }

        switch (i2s_select)
        {
        case I2S0:
            reg_audio_matrix_i2s0_tx_dma_sel = (reg_audio_matrix_i2s0_tx_dma_sel & (~FLD_MATRIX_I2S0_TX_DMA_SEL)) | data_format;
            break;
        case I2S1:
            reg_audio_matrix_i2s1_tx_dma_sel = (reg_audio_matrix_i2s1_tx_dma_sel & (~FLD_MATRIX_I2S1_TX_DMA_SEL)) | data_format;
            break;
        case I2S2:
            reg_audio_matrix_i2s2_tx_dma_sel = (reg_audio_matrix_i2s2_tx_dma_sel & (~FLD_MATRIX_I2S2_TX_DMA_SEL)) | data_format;
            break;
        default:
            break;
        }
    }
}

/**
 * @brief      This function serves to select anc_src route source and format.
 *
 * @param[in]  anc_chn     - anc channel.
 * @param[in]  route_from  - anc_src route from.
 * @param[in]  data_format - anc src data format(route from fifo/i2s/adc valid), others select ANC_SRC_DATA_FORMAT_INVALID.
 * @return     none
 */
void audio_matrix_set_anc_src_route(audio_anc_chn_e anc_chn, audio_matrix_anc_src_route_e route_from, audio_matrix_anc_src_format_e data_format)
{
    reg_audio_matrix_anc_src_sel(anc_chn) = (reg_audio_matrix_anc_src_sel(anc_chn) & (~FLD_MATRIX_ANC_SRC_SEL)) | (route_from << 4);
    if (anc_chn == ANC0)
    {
        switch (route_from)
        {
        case ANC_SRC_ROUTE_FIFO:
            reg_audio_matrix_anc0_src_sel = (reg_audio_matrix_anc0_src_sel & (~FLD_MATRIX_ANC0_SRC_DMA_SEL)) |
                                            MASK_VAL(FLD_MATRIX_ANC0_SRC_DMA_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_I2S0_RX:
            reg_audio_matrix_i2s_rx_anc_sel_0(0) = (reg_audio_matrix_i2s_rx_anc_sel_0(0) & (~FLD_MATRIX_I2S_RX_ANC0_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_I2S1_RX:
            reg_audio_matrix_i2s_rx_anc_sel_0(1) = (reg_audio_matrix_i2s_rx_anc_sel_0(1) & (~FLD_MATRIX_I2S_RX_ANC0_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_I2S2_RX:
            reg_audio_matrix_i2s_rx_anc_sel_0(2) = (reg_audio_matrix_i2s_rx_anc_sel_0(2) & (~FLD_MATRIX_I2S_RX_ANC0_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_CODEC0_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_0(0) = (reg_audio_matrix_adc_rx_anc_sel_0(0) & (~FLD_MATRIX_ADC_RX_ANC0_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_CODEC0_ADCB:
            reg_audio_matrix_adc_rx_anc_sel_0(1) = (reg_audio_matrix_adc_rx_anc_sel_0(1) & (~FLD_MATRIX_ADC_RX_ANC0_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_CODEC1_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_0(2) = (reg_audio_matrix_adc_rx_anc_sel_0(2) & (~FLD_MATRIX_ADC_RX_ANC0_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_SRC_SEL, data_format);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (route_from)
        {
        case ANC_SRC_ROUTE_FIFO:
            reg_audio_matrix_anc1_src_sel = (reg_audio_matrix_anc1_src_sel & (~FLD_MATRIX_ANC1_SRC_DMA_SEL)) |
                                            MASK_VAL(FLD_MATRIX_ANC1_SRC_DMA_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_I2S0_RX:
            reg_audio_matrix_i2s_rx_anc_sel_1(0) = (reg_audio_matrix_i2s_rx_anc_sel_1(0) & (~FLD_MATRIX_I2S_RX_ANC1_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_I2S1_RX:
            reg_audio_matrix_i2s_rx_anc_sel_1(1) = (reg_audio_matrix_i2s_rx_anc_sel_1(1) & (~FLD_MATRIX_I2S_RX_ANC1_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_I2S2_RX:
            reg_audio_matrix_i2s_rx_anc_sel_1(2) = (reg_audio_matrix_i2s_rx_anc_sel_1(2) & (~FLD_MATRIX_I2S_RX_ANC1_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_CODEC0_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_1(0) = (reg_audio_matrix_adc_rx_anc_sel_1(0) & (~FLD_MATRIX_ADC_RX_ANC1_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_CODEC0_ADCB:
            reg_audio_matrix_adc_rx_anc_sel_1(1) = (reg_audio_matrix_adc_rx_anc_sel_1(1) & (~FLD_MATRIX_ADC_RX_ANC1_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_SRC_SEL, data_format);
            break;
        case ANC_SRC_ROUTE_CODEC1_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_1(2) = (reg_audio_matrix_adc_rx_anc_sel_1(2) & (~FLD_MATRIX_ADC_RX_ANC1_SRC_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_SRC_SEL, data_format);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief      This function serves to select anc_src route source and format.
 *
 * @param[in]  anc_chn     - anc channel.
 * @param[in]  route_from  - anc_ref route from.
 * @param[in]  data_format - anc ref data format(route from i2s/adc valid), others select ANC_REF_DATA_FORMAT_INVALID.
 * @return     none
 */
void audio_matrix_set_anc_ref_route(audio_anc_chn_e anc_chn, audio_matrix_anc_ref_route_e route_from, audio_matrix_anc_ref_format_e data_format)
{
    reg_audio_matrix_anc_ref_sel(anc_chn) = (reg_audio_matrix_anc_ref_sel(anc_chn) & (~FLD_MATRIX_ANC_REF_SEL)) | (route_from << 4);
    if (anc_chn == ANC0)
    {
        switch (route_from)
        {
        case ANC_REF_ROUTE_I2S0_RX:
            reg_audio_matrix_i2s_rx_anc_sel_0(0) = (reg_audio_matrix_i2s_rx_anc_sel_0(0) & (~FLD_MATRIX_I2S_RX_ANC0_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_I2S1_RX:
            reg_audio_matrix_i2s_rx_anc_sel_0(1) = (reg_audio_matrix_i2s_rx_anc_sel_0(1) & (~FLD_MATRIX_I2S_RX_ANC0_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_I2S2_RX:
            reg_audio_matrix_i2s_rx_anc_sel_0(2) = (reg_audio_matrix_i2s_rx_anc_sel_0(2) & (~FLD_MATRIX_I2S_RX_ANC0_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_CODEC0_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_0(0) = (reg_audio_matrix_adc_rx_anc_sel_0(0) & (~FLD_MATRIX_ADC_RX_ANC0_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_CODEC0_ADCB:
            reg_audio_matrix_adc_rx_anc_sel_0(1) = (reg_audio_matrix_adc_rx_anc_sel_0(1) & (~FLD_MATRIX_ADC_RX_ANC0_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_CODEC1_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_0(2) = (reg_audio_matrix_adc_rx_anc_sel_0(2) & (~FLD_MATRIX_ADC_RX_ANC0_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_REF_SEL, data_format);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (route_from)
        {
        case ANC_REF_ROUTE_I2S0_RX:
            reg_audio_matrix_i2s_rx_anc_sel_2(0) = (reg_audio_matrix_i2s_rx_anc_sel_2(0) & (~FLD_MATRIX_I2S_RX_ANC1_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_I2S1_RX:
            reg_audio_matrix_i2s_rx_anc_sel_2(1) = (reg_audio_matrix_i2s_rx_anc_sel_2(1) & (~FLD_MATRIX_I2S_RX_ANC1_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_I2S2_RX:
            reg_audio_matrix_i2s_rx_anc_sel_2(2) = (reg_audio_matrix_i2s_rx_anc_sel_2(2) & (~FLD_MATRIX_I2S_RX_ANC1_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_CODEC0_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_2(0) = (reg_audio_matrix_adc_rx_anc_sel_2(0) & (~FLD_MATRIX_ADC_RX_ANC1_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_CODEC0_ADCB:
            reg_audio_matrix_adc_rx_anc_sel_2(1) = (reg_audio_matrix_adc_rx_anc_sel_2(1) & (~FLD_MATRIX_ADC_RX_ANC1_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_REF_SEL, data_format);
            break;
        case ANC_REF_ROUTE_CODEC1_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_2(2) = (reg_audio_matrix_adc_rx_anc_sel_2(2) & (~FLD_MATRIX_ADC_RX_ANC1_REF_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_REF_SEL, data_format);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief      This function serves to select anc_err route source and data format.
 *
 * @param[in]  anc_chn     - anc channel.
 * @param[in]  route_from  - anc_err route from.
 * @param[in]  data_format - anc err data format(route from i2s/adc valid), others select ANC_ERR_DATA_FORMAT_INVALID.
 * @return     none
 */
void audio_matrix_set_anc_err_route(audio_anc_chn_e anc_chn, audio_matrix_anc_err_route_e route_from, audio_matrix_anc_err_format_e data_format)
{
    reg_audio_matrix_anc_err_sel(anc_chn) = (reg_audio_matrix_anc_err_sel(anc_chn) & (~FLD_MATRIX_ANC_ERR_SEL)) | (route_from << 4);
    if (anc_chn == ANC0)
    {
        switch (route_from)
        {
        case ANC_ERR_ROUTE_I2S0_RX:
            reg_audio_matrix_i2s_rx_anc_sel_1(0) = (reg_audio_matrix_i2s_rx_anc_sel_1(0) & (~FLD_MATRIX_I2S_RX_ANC0_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_I2S1_RX:
            reg_audio_matrix_i2s_rx_anc_sel_1(1) = (reg_audio_matrix_i2s_rx_anc_sel_1(1) & (~FLD_MATRIX_I2S_RX_ANC0_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_I2S2_RX:
            reg_audio_matrix_i2s_rx_anc_sel_1(2) = (reg_audio_matrix_i2s_rx_anc_sel_1(2) & (~FLD_MATRIX_I2S_RX_ANC0_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC0_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_CODEC0_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_1(0) = (reg_audio_matrix_adc_rx_anc_sel_1(0) & (~FLD_MATRIX_ADC_RX_ANC0_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_CODEC0_ADCB:
            reg_audio_matrix_adc_rx_anc_sel_1(1) = (reg_audio_matrix_adc_rx_anc_sel_1(1) & (~FLD_MATRIX_ADC_RX_ANC0_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_CODEC1_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_1(2) = (reg_audio_matrix_adc_rx_anc_sel_1(2) & (~FLD_MATRIX_ADC_RX_ANC0_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC0_ERR_SEL, data_format);
            break;
        default:
            break;
        }
    }
    else
    {
        switch (route_from)
        {
        case ANC_ERR_ROUTE_I2S0_RX:
            reg_audio_matrix_i2s_rx_anc_sel_2(0) = (reg_audio_matrix_i2s_rx_anc_sel_2(0) & (~FLD_MATRIX_I2S_RX_ANC1_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_I2S1_RX:
            reg_audio_matrix_i2s_rx_anc_sel_2(1) = (reg_audio_matrix_i2s_rx_anc_sel_2(1) & (~FLD_MATRIX_I2S_RX_ANC1_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_I2S2_RX:
            reg_audio_matrix_i2s_rx_anc_sel_2(2) = (reg_audio_matrix_i2s_rx_anc_sel_2(2) & (~FLD_MATRIX_I2S_RX_ANC1_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_I2S_RX_ANC1_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_CODEC0_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_2(0) = (reg_audio_matrix_adc_rx_anc_sel_2(0) & (~FLD_MATRIX_ADC_RX_ANC1_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_CODEC0_ADCB:
            reg_audio_matrix_adc_rx_anc_sel_2(1) = (reg_audio_matrix_adc_rx_anc_sel_2(1) & (~FLD_MATRIX_ADC_RX_ANC1_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_ERR_SEL, data_format);
            break;
        case ANC_ERR_ROUTE_CODEC1_ADCA:
            reg_audio_matrix_adc_rx_anc_sel_2(2) = (reg_audio_matrix_adc_rx_anc_sel_2(2) & (~FLD_MATRIX_ADC_RX_ANC1_ERR_SEL)) |
                                                   MASK_VAL(FLD_MATRIX_ADC_RX_ANC1_ERR_SEL, data_format);
            break;
        default:
            break;
        }
    }
}

/**
 * @brief      This function serves to select dac route source and data format.
 *
 * @param[in]  dac_chn     - dac channel.
 * @param[in]  route_from  - dac route from.
 * @param[in]  data_format - dac data format(route from fifo valid).
 * @return     none
 */
void audio_matrix_set_dac_route(audio_codec0_output_select_e dac_chn, audio_matrix_dac_route_e route_from, audio_matrix_dac_format_e data_format)
{
    switch (dac_chn)
    {
    case AUDIO_DAC_A1:
        reg_audio_matrix_dac_sel = (reg_audio_matrix_dac_sel & (~FLD_MATRIX_DAC_L_SEL)) | route_from;
        break;
    case AUDIO_DAC_A2:
        if (route_from == DAC_ROUTE_FIFO)
        {
            reg_audio_matrix_dac_sel = (route_from << 4) | route_from;
        }
        else
        {
            reg_audio_matrix_dac_sel = (reg_audio_matrix_dac_sel & (~FLD_MATRIX_DAC_R_SEL)) | (route_from << 4);
        }
        break;
    case AUDIO_DAC_A1_A2:
        reg_audio_matrix_dac_sel = (route_from << 4) | route_from;
        break;
    default:
        break;
    }

    if (route_from == DAC_ROUTE_FIFO)
    {
        reg_audio_matrix_dac_dma_sel = (reg_audio_matrix_dac_dma_sel & (~FLD_MATRIX_DAC_DMA_SEL)) | MASK_VAL(FLD_MATRIX_DAC_DMA_SEL, data_format);
    }
}

/**
 * @brief      This function serves to select hac route source and data format.
 *
 * @param[in]  hac_chn     - hac channel.
 * @param[in]  route_from  - hac route from.
 * @param[in]  data_format - hac data format(route from i2s/adc valid), others select HAC_DATA_FORMAT_INVALID.
 * @return     none
 * @note
 *             - The filters inside the HAC are processed according to 24bit data.
 */
void audio_matrix_set_hac_route(audio_hac_chn_e hac_chn, audio_matrix_hac_route_e route_from, audio_matrix_hac_format_e data_format)
{
    reg_audio_matrix_hac_tx_sel(hac_chn) = (reg_audio_matrix_hac_tx_sel(hac_chn) & (~FLD_MATRIX_HAC_TX_SEL)) | route_from;

    /* hac data format set(route from i2s/adc valid). */
    unsigned char chn_mask = (hac_chn % 2) ? FLD_MATRIX_I2S_RX_HAC_ODD_SEL : FLD_MATRIX_I2S_RX_HAC_EVEN_SEL;

    switch (route_from)
    {
    case HAC_DATA_ROUTE_I2S0_RX:
        reg_audio_matrix_i2s_rx_hac_sel(I2S0, hac_chn) = (reg_audio_matrix_i2s_rx_hac_sel(I2S0, hac_chn) & (~chn_mask)) |
                                                         MASK_VAL(chn_mask, data_format);
        break;
    case HAC_DATA_ROUTE_I2S1_RX:
        reg_audio_matrix_i2s_rx_hac_sel(I2S1, hac_chn) = (reg_audio_matrix_i2s_rx_hac_sel(I2S1, hac_chn) & (~chn_mask)) |
                                                         MASK_VAL(chn_mask, data_format);
        break;
    case HAC_DATA_ROUTE_I2S2_RX:
        reg_audio_matrix_i2s_rx_hac_sel(I2S2, hac_chn) = (reg_audio_matrix_i2s_rx_hac_sel(I2S2, hac_chn) & (~chn_mask)) |
                                                         MASK_VAL(chn_mask, data_format);
        break;
    case HAC_DATA_ROUTE_CODEC0_ADCA:
        reg_audio_matrix_adc_rx_hac_sel(0, hac_chn) = (reg_audio_matrix_adc_rx_hac_sel(0, hac_chn) & (~chn_mask)) | MASK_VAL(chn_mask, data_format);
        break;
    case HAC_DATA_ROUTE_CODEC0_ADCB:
        reg_audio_matrix_adc_rx_hac_sel(1, hac_chn) = (reg_audio_matrix_adc_rx_hac_sel(1, hac_chn) & (~chn_mask)) | MASK_VAL(chn_mask, data_format);
        break;
    case HAC_DATA_ROUTE_CODEC1_ADCA:
        reg_audio_matrix_adc_rx_hac_sel(2, hac_chn) = (reg_audio_matrix_adc_rx_hac_sel(2, hac_chn) & (~chn_mask)) | MASK_VAL(chn_mask, data_format);
        break;
    default:
        break;
    }
}

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio pin interface                                                 *
 *********************************************************************************************************************/
/*!
 * @name Audio pin interface
 * @{
 */

/**
 * @brief      This function serves to configure i2s pin.
 * @param[in]  i2s_select - channel select.
 * @param[in]  config     - i2s config pin struct.
 * @return     none
 */
void audio_i2s_set_pin(i2s_select_e i2s_select, i2s_pin_config_t *config)
{
    gpio_input_en((gpio_pin_e)config->bclk_pin);
    gpio_set_mux_function((gpio_func_pin_e)config->bclk_pin, I2S0_BCK_IO + i2s_select);
    gpio_function_dis((gpio_pin_e)config->bclk_pin);

    if (config->adc_lr_clk_pin != I2S_ADC_LR_CLK_NONE_PIN)
    {
        gpio_input_en((gpio_pin_e)config->adc_lr_clk_pin);
        gpio_set_mux_function((gpio_func_pin_e)config->adc_lr_clk_pin, I2S0_LR0_IO + i2s_select);
        gpio_function_dis((gpio_pin_e)config->adc_lr_clk_pin);
    }

    if (config->adc_dat_pin != I2S_ADC_DAT_NONE_PIN)
    {
        gpio_input_en((gpio_pin_e)config->adc_dat_pin);
        gpio_set_mux_function((gpio_func_pin_e)config->adc_dat_pin, I2S0_DAT0_IO + i2s_select);
        gpio_function_dis((gpio_pin_e)config->adc_dat_pin);
    }

    if (config->dac_lr_clk_pin != I2S_DAC_LR_NONE_PIN)
    {
        gpio_input_en((gpio_pin_e)config->dac_lr_clk_pin);
        gpio_set_mux_function((gpio_func_pin_e)config->dac_lr_clk_pin, I2S0_LR1_IO + i2s_select);
        gpio_function_dis((gpio_pin_e)config->dac_lr_clk_pin);
    }

    if (config->dac_dat_pin != I2S_DAC_DAT_NONE_PIN)
    {
        gpio_input_en((gpio_pin_e)config->dac_dat_pin);
        gpio_set_mux_function((gpio_func_pin_e)config->dac_dat_pin, I2S0_DAT1_IO + i2s_select);
        gpio_function_dis((gpio_pin_e)config->dac_dat_pin);
    }
}
/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio side_tone interface                                           *
 *********************************************************************************************************************/
/*!
 * @name Audio side_tone interface
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio spdif interface                                               *
 *********************************************************************************************************************/
/*!
 * @name Audio spdif interface
 * @{
 */

/**
 * @}
 */

/**********************************************************************************************************************
 *                                                Audio config/route interface                                        *
 *********************************************************************************************************************/
/*!
 * @name Audio config/route interface
 * @{
 */

/**
 * @}
 */
