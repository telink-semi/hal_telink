/********************************************************************************************************
 * @file    rf_common.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#include "lib/include/rf/rf_common.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"

#include "ext_driver/driver_internal/ext_lib.h" //add by BLE


/*********************************************************************************************************************
 *                                         RF global constants                                                       *
 *********************************************************************************************************************/
/**
 *  @brief   Define power list of RF.
 *  @note    (1)The energy meter is averaged over 3 chips at room temperature and 3.3V supply voltage..
 *           (2)Transmit energy in VBAT mode decreases as the supply voltage drops.
 *           (3)There will be some differences in the energy values tested between different chips.
 */
 const     //BLE SDK use: in IRQ
rf_power_level_e rf_power_Level_list[60] =
{
     /*VBAT*/
     RF_POWER_P11p33dBm, /**<  11.3 dbm */
     RF_POWER_P11p16dBm, /**<  11.1 dbm */
     RF_POWER_P10p78dBm, /**<  10.8 dbm */
     RF_POWER_P10p56dBm, /**<  10.5 dbm */
     RF_POWER_P9p97dBm, /**<  10.0 dbm */
     RF_POWER_P9p50dBm, /**<  9.5 dbm */
     RF_POWER_P8p95dBm, /**<  9.0 dbm */
     RF_POWER_P8p48dBm, /**<  8.5 dbm */
     RF_POWER_P8p03dBm, /**<  8.0 dbm */
     RF_POWER_P7p61dBm, /**<  7.6 dbm */
     RF_POWER_P7p19dBm, /**<  7.2 dbm */
     RF_POWER_P6p97dBm, /**<  7.0 dbm */
     RF_POWER_P6p48dBm, /**<  6.5 dbm */
     RF_POWER_P5p97dBm, /**<  6.0 dbm */

     /*VANT*/
     RF_POWER_P5p79dBm,   /**<   5.8 dbm */
     RF_POWER_P5p54dBm,   /**<   5.5 dbm */
     RF_POWER_P5p25dBm,   /**<   5.2 dbm */
     RF_POWER_P5p09dBm,   /**<   5.1 dbm */
     RF_POWER_P5p00dBm,   /**<   5.0 dbm */
     RF_POWER_P4p73dBm,   /**<   4.7 dbm */
     RF_POWER_P4p52dBm,   /**<   4.5 dbm */
     RF_POWER_P4p05dBm,   /**<   4.0 dbm */
     RF_POWER_P3p49dBm,   /**<   3.5 dbm */
     RF_POWER_P2p99dBm,   /**<   3.0 dbm */
     RF_POWER_P2p45dBm,   /**<   2.5 dbm */
     RF_POWER_P2p03dBm,   /**<   2.0 dbm */
     RF_POWER_P1p53dBm,   /**<   1.5 dbm */
     RF_POWER_P1p33dBm,   /**<   1.3 dbm */
     RF_POWER_P1p09dBm,   /**<   1.0 dbm */
     RF_POWER_P0p55dBm,   /**<   0.5 dbm */
     RF_POWER_P0p25dBm,   /**<   0.2 dbm */
     RF_POWER_N0p07dBm,   /**<   0.0 dbm */
     RF_POWER_N0p41dBm,   /**<   -0.4 dbm */
     RF_POWER_N0p72dBm,   /**<   -0.7 dbm */
     RF_POWER_N1p10dBm,   /**<   -1.0 dbm */
     RF_POWER_N1p49dBm,   /**<   -1.5 dbm */
     RF_POWER_N2p21dBm,   /**<  -2.1 dbm */
     RF_POWER_N2p80dBm,   /**<  -2.8 dbm */
     RF_POWER_N3p21dBm,   /**<  -3.2 dbm */
     RF_POWER_N4p41dBm,   /**<  -4.4 dbm */
     RF_POWER_N5p07dBm,   /**<  -5.0 dbm */
     RF_POWER_N5p73dBm,   /**<  -5.7 dbm */
     RF_POWER_N6p53dBm,   /**<  -6.5 dbm */
     RF_POWER_N7p33dBm,   /**<  -7.3 dbm */     
     RF_POWER_N8p32dBm,   /**<  -8.3 dbm */
     RF_POWER_N9p45dBm,   /**<  -9.5 dbm */
     RF_POWER_N10p77dBm,   /**<  -10.7 dbm */
     RF_POWER_N14p13dBm,   /**<  -14.1 dbm */
     RF_POWER_N20p01dBm,   /**<  -20.0 dbm */
     RF_POWER_N25p53dBm,   /**<  -25.5 dbm */
     RF_POWER_N48p75dBm,   /**<  -48.8 dbm */
};


static rf_status_e s_rf_trxstate = RF_MODE_TX;
rf_mode_e   g_rfmode;
rf_crc_config_t rf_crc_config[3]=
{
    {0x555555, 0x0000065b, 0, 1,0,3},//ble
    {0xffffffff, 0x00001021, 0, 1,0,2},//private
    {0x00000000, 0x00001021, 0, 1,1,2},//zigbee,hybee
};

_attribute_data_retention_sec_ static rf_fast_settle_t g_fast_settle_cal_val;
_attribute_data_retention_sec_ static unsigned char g_rf_tx_fast_settle_chn_cal_flag = 0;
_attribute_data_retention_sec_ static rf_tx_fast_settle_time_e g_rf_tx_fast_settle_time = TX_FAST_SETTLE_NONE;
_attribute_data_retention_sec_ static rf_rx_fast_settle_time_e g_rf_rx_fast_settle_time = RX_FAST_SETTLE_NONE;
/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

/**
 * @brief     This function is used to select the rx performance mode.
 * @param[in] rx_performance - rx performance mode.
 * @return    none.
 * @note      There are two types of RX performance modes, RF_RX_LOW_POWER and RX_HIGH_PERFORMANCE
 *            The default is RF_RX_LOW_POWER, and in this mode, the rx performance can reach -96dBm
 *            RX_HIGH_PERFORMANCE mode can improve performance by 1dBm, but the rx power consumption will increase
 */
void rf_rx_performance_mode(rf_rx_performance_e rx_performance)
{
    if(rx_performance==RF_RX_LOW_POWER)
    {
        reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_FE_RTRIM_RX)) | (0x03 << 2);
    }
    else
    {
        reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_FE_RTRIM_RX)) | (0x06 << 2);
    }
}

/**
 * @brief     This function serves to initiate information of RF.
 * @return     none.
 */
void rf_mode_init(void)
{
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    reg_rst4    |=  FLD_RST4_ZB;
    reg_clk_en4 |=  FLD_CLK4_ZB_EN;

    reg_n22_rst = 0xffc;//reset dma_bb,zb,rst_modem,rstl_bb(bb:baseband)
    reg_n22_clk_en0 = 0xff;//enable dma_bb,zb_hclk
    reg_rf_tstimp_ctrl |= FLD_RF_R_STIMER_REVERT_EN; //Switching RF clock to stimer.

    //one_time_setup
    write_reg8(0x1706d2,0x9b);//DCOC_SFIIP:bit<4> DCOC_SFQQP:bit<5> DCOC_SFII_L:bit<6-7>
    write_reg8(0x1706d3,0x19);//DCOC_SFII_H:bit<0-1> DCOC_SFQQ:bit<2-5>
#if RF_RX_SHORT_MODE_EN
    write_reg8(0x17047b,0x0e);//BLANK_WINDOW
    write_reg8(0x170479,0x38);//BIT[3] RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix
                              //per floor issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#else
    write_reg8(0x17047b,0xfe);//BLANK_WINDOW
    write_reg8(0x170479,0x08);//RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix per floor
                              //issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#endif

    //To set AGC thresholds
    write_reg8(0x17064a,0x0e);//POW_000_001:bit<0-6> POW_001_010_L:bit<7>
    write_reg8(0x17064b,0x09);//POW_001_010_H:bit<0-5>
    write_reg8(0x17064e,0x09);//POW_100_101:bit<0-6> POW_101_100_L:bit<7>
    write_reg8(0x17064f,0x0f);//POW_101_100_H:bit<0-5>
    write_reg8(0x170654,0x0e);//POW_000_001:bit<0-6> POW_001_010_L:bit<7>
    write_reg8(0x170655,0x09);//POW_001_010_H:bit<0-5>
    write_reg8(0x170656,0x0c);//POW_010_011:bit<0-6> POW_011_100_L:bit<7>
    write_reg8(0x170657,0x08);//POW_011_100_H:bit<0-5>
    write_reg8(0x170658,0x09);//POW_100_101:bit<0-6> POW_101_100_L:bit<7>
    write_reg8(0x170659,0x0f);//POW_101_100_H:bit<0-5>

    //For optimum preamble detection
    write_reg8(0x170476,0x50);//RX_PE_DET_MIN_LO_THRESH
    write_reg8(0x170477,0x73);//RX_PE_DET_MIN_HI_THRESH

    rf_clr_irq_mask(FLD_RF_IRQ_ALL);//The default interrupt mask in RF is open.
    //Close the interrupt mask in the initialization code and reopen it when in use

    reg_rf_ll_ctrl3 &= ~(FLD_RF_R_TX_EN_DLY_EN);//Turn off the extension tx_en function

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1:0>:lna_itrim           default:0,->3(4.4u->6.2u)    Increase lna_itrim current to boost RX performance.
    * <5:4>:pa_vbias           default:1,->0(515mv->535mv)    Raise the pa_vbais voltage to boost transmit power level.
    * This setting is used for A0 to improve performance, pending A1 hardware to fix performance issues; this setting
    * will be canceled out to reduce power consumption. modified by zhiwei.wang,confirmed by wenfeng.lou 24020531.
    */
    // write_reg8(0x17074c,0x03);
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1:0>:cbpf_trim_i                default:0,->3(5.00u->8.75u)    Increasing the I-way trim current of cbpf to improve rx performance.
    * <3:2>:cbpf_trim_q                   default:0,->3(5.00u->8.75u)    Increasing the Q-way trim current of cbpf to improve rx performance.
    * <6:5>:cbpf_trim_short_dcbias    default:2,->1(460mv->490mv)    Increase cbpf_trim_short_dcbias voltage to boost RX performance.
    * <7:7>:cbpf_vcm_trim_l            default:1,->0(490mv->520mv) Increase cbpf_vcm_trim_l voltage to boost RX performance.
    * This setting is used for A0 to improve performance, pending A1 hardware to fix performance issues; this setting will be canceled
    * out to reduce power consumption. modified by zhiwei.wang,confirmed by wenfeng.lou 24020531.
    */
    write_reg8(0x17074e,0x4f);

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <2:0>:VCO_TRIM_KVT                default:0x7  Adjustment of Kv of vctrl path depending upon reference frequency. Default should change depending upon if the reference frequency is 24MHz or 32MHz
    * <3>  :VCO_EN_PKDET                default:0    Enable peak detector operation
    * <5:4>:LDOTRIM_TRIM_VREF           default:2,->0(0.946V->0.901V) Bump bits for the 900 mV LDOTRIM reference voltage.
    * This setting is used to change the LDO trimming reference voltage from 0.946 to 0.901 and then LDO output to 1.05v.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20240820.
    */
    write_reg8(0x170754,0x07);
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <0>:LDO_PLL_BOOST           default:0 To boost the LDO output voltage to close to 1.5V with trim code being maximum
    * <1>:LDO_VCO_BYPASS          default:0 Bypass the LDO output to Vline
    * <2>:LDO_VCO_BOOST           default:0 To boost the LDO output voltage to close to 1.5V with trim code being maximum
    * <3>:LDO_CAL_BYPASS          default:0 Bypass the LDO output to Vline
    * <4>:LDO_CAL_BOOST           default:0 To boost the LDO output voltage to close to 1.5V with trim code being maximum
    * <5>:LDO_ANT_BYPASS          default:0,->1 Bypass the LDO output to Vline
    * This setting is used for A1 to obtain higher output power under vant mode.modified by chenxi.wang,confirmed by wenfeng.lou 20240820.
    */
    write_reg8(0x170741,0x20);

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <0>:LNA_HP_DIG                 default:1 LNA high power high performance control: 0:normal mode;1:high power low noise mode
    * <3:1>:TX_ATTN_TRIM_DIG         default:4 Tx lo attenuation mode trim
    * <4>:TX_ATTN_SEL_DIG            default:0 Tx lo attenuation enable:0(default): tx lo normal mode; 1:tx lo attenuation mode
    * <7:5>:TX_BUF_TRIM_DIG          default:4,->0 Tx lo buffer vbias trim to adjust the duty cycle
    * This setting is used for A1 to optimise power consumption.modified by chenxi.wang,confirmed by wenfeng.lou 20240820.
    */
    write_reg8(0x170638,0x09);

    /*
    * This configuration is used for A1 to improve the performance of rf rx sensitivity.
    * Defaults to RX_LOW_POWER for A1.
    * If you need higher performance, you need to call rf_rx_performance_mode() after rf_mode_init;
    * Select the RX_HIGH_PERFORMANCE mode, in which the RX sensitivity is increased by 1dBm, but the receiving power consumption will increase
    * (modified by chenxi.wang,confirmed by wenfeng.lou 20240826.)
    */
    rf_rx_performance_mode(RF_RX_LOW_POWER);
}


/**
 * @brief      This setting serve to set the configuration of Tx DMA.
 */
rf_dma_config_t rf_tx_dma_config={
    .dst_req_sel= 8,//tx req.(must 8)
    .src_req_sel=0,
    .dst_addr_ctrl=DMA_ADDR_FIX,
    .src_addr_ctrl=DMA_ADDR_INCREMENT,//increment.
    .dstmode=DMA_HANDSHAKE_MODE,//handshake.
    .srcmode=DMA_NORMAL_MODE,
    .dstwidth=DMA_CTR_WORD_WIDTH,//must word.
    .srcwidth=DMA_CTR_WORD_WIDTH,//must word.
    .src_burst_size=0,//must 0.
    .vacant_bit=0,
    .read_num_en=1,
    .priority=0,
    .write_num_en=0,
    .auto_en=1,//must 1.
};

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] none
 * @return    none.
 */
void rf_set_tx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN|FLD_RF_CH_0_RNUM_EN_BK);//u_pd_mcu.u_dmac.atcdmac100_ahbslv.tx_multi_en,rx_multi_en,ch_0_rnum_en_bk.
    rf_dma_config(RF_TX_DMA,&rf_tx_dma_config);
    rf_dma_set_dst_address(RF_TX_DMA,reg_rf_txdma_adr);
}

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] fifo_depth        - tx chn deep,fifo_depth range: 0~5,Number of fifo=2^fifo_depth.
 * @param[in] fifo_byte_size    - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return    none.
 */
void rf_set_tx_dma(unsigned char fifo_dep,unsigned short fifo_byte_size)
{
    rf_set_tx_dma_config();
    rf_set_tx_dma_fifo_num(fifo_dep);
    rf_set_tx_dma_fifo_size(fifo_byte_size);

}


/**
 * @brief      This setting serve to set the configuration of Rx DMA.
 */
rf_dma_config_t rf_rx_dma_config={
        .dst_req_sel= 0,//tx req.
        .src_req_sel=9,//must 9
        .dst_addr_ctrl=0,
        .src_addr_ctrl=DMA_ADDR_FIX,//increment.
        .dstmode=DMA_NORMAL_MODE,
        .srcmode=DMA_HANDSHAKE_MODE,//handshake.
        .dstwidth=DMA_CTR_WORD_WIDTH,//must word.
        .srcwidth=DMA_CTR_WORD_WIDTH,//must word.
        .src_burst_size=0,//must 0.
        .vacant_bit=0,
        .read_num_en=0,
        .priority=0,
        .write_num_en=1,
        .auto_en=1,//must 1.
    };


/**
 * @brief       This function serve to rx dma config
 * @param[in]   none
 * @return      none
 */
void rf_set_rx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN|FLD_RF_CH_0_RNUM_EN_BK);//ch0_rnum_en_bk,tx_multi_en,rx_multi_en.
    rf_dma_config(RF_RX_DMA,&rf_rx_dma_config);
    rf_dma_set_src_address(RF_RX_DMA,reg_rf_rxdma_adr);
    rf_dma_set_size(RF_RX_DMA,0xFFFFFC,RF_DMA_WORD_WIDTH);

}

/**
 * @brief      This function serves to rx dma setting.
 * @param[in]  buff - This parameter is the first address of the received data buffer, which must be 4 bytes aligned, otherwise the program will enter an exception.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *             -# you need to leave 4bytes of space for the length information.
 *             -# dma is transmitted in accordance with 4bytes, so the length of the buffer needs to be a multiple of 4. Otherwise, there may be an out-of-bounds problem
 *             For example, the actual received data length is 5bytes, the minimum value of the actual buffer size that the user needs to set is 12bytes, and the calculation of 12bytes is explained as follows::
 *             4bytes (length information) + 5bytes (data) + 3bytes (the number of additional bytes to prevent out-of-bounds)
 * @param[in]  wptr_mask       - This parameter is used to set the mask value for the number of enabled FIFOs. The value of the mask must (0x00,0x01,0x03,0x07,0x0f,0x1f).
 *                               The number of FIFOs enabled is the value of wptr_mask plus 1.(0x01,0x02,0x04,0x08,0x10,0x20)
 * @param[in]  fifo_byte_size  - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return     none.
 */
void rf_set_rx_dma(unsigned char *buff,unsigned char wptr_mask,unsigned short fifo_byte_size)
{
    rf_set_rx_dma_config();
    rf_set_rx_buffer(buff);
    rf_set_rx_dma_fifo_num(wptr_mask);
    rf_set_rx_dma_fifo_size(fifo_byte_size);
}

/**
 * @brief       This function serves to RF trigger stx
 * @param[in]   addr    - DMA tx buffer.
 * @param[in]   tick    - Send after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_stx(void* addr,  unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x85;//stx
}



/**
 * @brief     This function serves to trigger srx on.
 * @param[in] tick  - Trigger rx receive packet after tick delay.
 * @return    none.
 */
void rf_start_srx(unsigned int tick)
{
    reg_rf_ll_rx_fst_timeout= 0x0fffffff;                   // first timeout.
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x86;
}

/**
 * @brief       This function serves to RF trigger stx2rx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger tx send packet after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_stx2rx(void* addr, unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x87;   // single tx2rx.

}

/**
 *  @brief      This function is mainly used to set hpmc Calibration-related values.
 *  @param[in]  hpmc_gain  - hpmc Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_sec_noinline_ static void rf_set_hpmc_cal_val(unsigned short hpmc_gain)
{
    //The calibration value of hpmc is different at different frequency points,
    //So you need to reset it every time you switch channels.

    unsigned short tmp = read_reg16(0x1706f6);
    tmp = (tmp & 0xf001) | hpmc_gain;   //bit<1:11> 1111 0000 0000 0000
    write_reg16(0x1706f6,tmp);
}

volatile unsigned char  g_single_tong_freqoffset = 0;//for eliminate single carrier frequency offset.
/**
 * @brief       This function serves to set rf channel for all mode.The actual channel set by this function is 2400+chn.
 * @param[in]   chn   - That you want to set the channel as 2400+chn.
 * @return      none.
 */
void rf_set_chn(signed char chn)
{
    if(g_rf_tx_fast_settle_chn_cal_flag == 1)
    {
        rf_set_hpmc_cal_val(g_fast_settle_cal_val.cal_tbl[chn]);
    }
    unsigned int freq_low;
    unsigned int freq_high;
    unsigned int chnl_freq;
    unsigned char ctrim;
    unsigned int freq;

    freq = 2400+chn;
    if(freq >= 2550){
        ctrim = 0;
    }
    else if(freq >= 2520){
        ctrim = 1;
    }
    else if(freq >= 2495){
        ctrim = 2;
    }
    else if(freq >= 2465){
        ctrim = 3;
    }
    else if(freq >= 2435){
        ctrim = 4;
    }
    else if(freq >= 2405){
        ctrim = 5;
    }
    else if(freq >= 2380){
        ctrim = 6;
    }
    else{
        ctrim = 7;
    }

    chnl_freq = freq*2 + g_single_tong_freqoffset;
    freq_low  = (chnl_freq & 0x7f);
    freq_high = ((chnl_freq>>7)&0x3f);

    write_reg8(0x170644,  (read_reg8(0x170644) | 0x01 ));
    write_reg8(0x170644,  (read_reg8(0x170644) & 0x01) | freq_low << 1);
    write_reg8(0x170645,  (read_reg8(0x170645) & 0xc0) | freq_high);
    write_reg8(0x170629,  (read_reg8(0x170629) & 0x1f) | (ctrim<<5) );  //FE_CTRIM
}



/**
 * @brief       This function serves to get rssi.
 * @return      rssi value.
 */
signed char rf_get_rssi(void)
{
    return (((signed char)(read_reg8(REG_TL_MODEM_BASE_ADDR+0x5d))) - 110);//this function can not tested on fpga
}


/**
 * @brief       This function serves to set RF Rx manual on.
 * @return      none.
 */
void rf_set_rxmode(void)
{
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    reg_rf_modem_mode_cfg_rx1_0 |= FLD_RF_CONT_MODE;//set continue mode.
    reg_rf_ll_ctrl0 |= FLD_RF_R_RX_EN_MAN;//rx enable.
    reg_rf_rxmode |= FLD_RF_RX_ENABLE;//bb rx enable.

}

/**
 * @brief       This function serves to get the right fifo packet.
 * @param[in]   fifo_num   - The number of fifo set in dma.
 * @param[in]   fifo_dep   - deepth of each fifo set in dma.
 * @param[in]   addr       - address of rx packet.
 * @return      the next rx_packet address.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
unsigned char* rf_get_rx_packet_addr(int fifo_num,int fifo_dep,void* addr)
{
    unsigned char rptr;
    rptr = reg_rf_dma_rx_rptr;
    unsigned char * raw_pkt =(unsigned char *)((unsigned char*)addr + (rptr & (fifo_num-1)) * (fifo_dep));
    reg_rf_dma_rx_rptr=0x40;
    return raw_pkt;
}

/**
 * @brief       This function serves to set RF Tx mode.
 * @return      none.
 */
void rf_set_txmode(void)
{
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
    reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}


/**
 * @brief       This function serves to set RF Tx packet address to DMA src_addr.
 * @param[in]   addr   - The packet address which to send.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_tx_pkt(void* addr)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_bb_dma_ctr0(0) |= 0x01;
}

/**
 * @brief       This function serves to set RF power level.
 * @param[in]   level    - The power level to set.
 * @return      none.
 */
void rf_set_power_level (rf_power_level_e level)
{
    unsigned char value;
    if(level&BIT(7))
    {
        reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;
    }
    else
    {
        reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
    }

    value = (unsigned char)(level & 0x3F);
    reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value&0x01)<<7));
    reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value>>1)&0x1f));
}

/**
 * @brief       This function serves to set RF power through select the level index.
 * @param[in]   idx      - The index of power level which you want to set.
 * @return      none.
 */
_attribute_ram_code_sec_        //BLE SDK use.
void rf_set_power_level_index(rf_power_level_index_e idx)
{
    unsigned char value;
    unsigned char level = 0;

    if(idx < sizeof(rf_power_Level_list_ble)/sizeof(rf_power_Level_list_ble[0]))
    {
        level = rf_power_Level_list[idx];
        if(level&BIT(7))
        {
            reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;
        }
        else
        {
            reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
        }

        value = (unsigned char)(level & 0x3F);

        reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value&0x01)<<7));
        reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value>>1)&0x1f));
    }
    /* add by BLE, important */
    blt_extRF.txPower_index = (unsigned char)idx;
}

/**
 * @brief     This function serves to get RF status.
 * @return    RF Rx/Tx status.
 */
rf_status_e rf_get_trx_state(void)
{
    return s_rf_trxstate;
}

/**
 * @brief       This function serves to judge RF Tx/Rx state.
 * @param[in]   rf_status   - Tx/Rx status.
 * @param[in]   rf_channel  - This param serve to set frequency channel(2400+rf_channel) .
 * @return      Whether the setting is successful(-1:failed;else success).
 */
int rf_set_trx_state(rf_status_e rf_status, signed char rf_channel)
{
      int err = 0;

      reg_rf_ll_ctrl0 = 0x45;           // reset tx/rx state machine.
      rf_set_chn(rf_channel);

    if (rf_status == RF_MODE_TX) {
        rf_set_txmode();
        s_rf_trxstate = RF_MODE_TX;
    }
    else if (rf_status == RF_MODE_RX) {
        rf_set_rxmode();
        s_rf_trxstate = RF_MODE_RX;
    }
    else if(rf_status == RF_MODE_OFF){
        rf_set_tx_rx_off();
        s_rf_trxstate = RF_MODE_OFF;
    }
    else if (rf_status == RF_MODE_AUTO) {
        reg_rf_ll_cmd = 0x80;       //stop cmd.
        reg_rf_ll_ctrl3 = 0x29;     // reg0x140a16 pll_en_man and tx_en_dly_en  enable.
        reg_rf_rxmode |= (~FLD_RF_RX_ENABLE);   //rx disable.
        reg_rf_ll_ctrl0 &=0xce;         //reg0x140a02 disable rx_en_man and tx_en_man.
        s_rf_trxstate = RF_MODE_AUTO;
    }
    else {
        err = -1;
    }
    return  err;

}

/**
 * @brief       This function serve to change the length of preamble.
 * @param[in]   len     -The value of preamble length.Set the register bit<0>~bit<4>.
 * @return      none
 */
void rf_set_preamble_len(unsigned char len)
{
    len = len&0x1f;
    reg_rf_preamble_trail =(reg_rf_preamble_trail&(~FLD_RF_PREAMBLE_LEN))|len;
}

/**
 * @brief       This function serve to set the length of access code.
 * @param[in]   byte_len    -   The value of access code length,the range is 3~5byte.
 * @return      none
 */
void rf_set_access_code_len(unsigned char byte_len)
{
    unsigned char temp;
    temp = byte_len & 0x07;
    reg_rf_acclen = (reg_rf_acclen&(~FLD_RF_ACC_LEN))|temp;
    reg_rf_modem_rx_ctrl_0 = (reg_rf_modem_rx_ctrl_0&(~FLD_RF_RX_ACC_LNE))|temp;
}

/**
 * @brief       This function serves to RF trigger srx2rx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger rx receive packet after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_srx2tx(void* addr, unsigned int tick)
{
    reg_rf_ll_rx_fst_timeout = 0x0fffffff;                  // first timeout
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_rest_pid = 0x3f;
    reg_rf_ll_cmd = 0x88;// single rx2tx

}

/**
 * @brief       This function is used to judge whether there is a CRC error in the received packet through hardware.
 *              For the same packet, the value of this bit is consistent with the CRC flag bit in the packet.
 * @param[in]   none.
 * @return      none.
 */
unsigned char rf_get_crc_err(void)
{
    return  (reg_rf_dec_err & 0x10);
}

/**
 * @brief       This function serves to disable pn of rf mode.
 * @return      none.
 */
void rf_pn_disable(void)
{
    reg_rf_tx_mode2 &= (~FLD_RF_ZB_PN_EN);
    reg_rf_tx_mode2 &= (~FLD_RF_V_PN_EN);
    reg_rf_format   &= (~FLD_RF_BLE_WT);
}

/**********************************************************************************************************************
 *  Fast settle related interfaces
 *  Attention:
 *  (1)This part of the function is only for the internal use of the driver, not open to customers to use,
 *  we will rewrite this part, and provide demo
 *  (2)When using TL321X fast settle, it should be noted that different settle times correspond to different calibration modules being turned off,
 *     so you need to manually configure the calibration values of these calibration modules before using fast settle.
 *  (3)Calibration modules that require manual setting of calibration values:
 *     RX_SETTLE_TIME_15US: ldo trim; rx_fcal; dcoc;
 *     RX_SETTLE_TIME_37US: ldo trim; dcoc;
 *     RX_SETTLE_TIME_77US: ldo trim;
 *
 *     TX_SETTLE_TIME_15US: ldo trim; tx_fcal; hpmc;
 *     TX_SETTLE_TIME_51US: ldo trim; hpmc;
 *     TX_SETTLE_TIME_104US: ldo trim;
 *
 *********************************************************************************************************************/

/**
 *  @brief      This function serve to adjust tx/rx settle timing sequence.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @return      0                   -  Correct configuration.
 *              -1                   -  Incorrect configuration.
*/

signed char rf_fast_settle_config(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us)
{
    if ((tx_settle_us == TX_SETTLE_TIME_15US && rx_settle_us != RX_SETTLE_TIME_15US) ||
        (rx_settle_us == RX_SETTLE_TIME_15US && tx_settle_us != TX_SETTLE_TIME_15US))
    {
        return -1;
    }

    g_rf_tx_fast_settle_time = tx_settle_us;
    g_rf_rx_fast_settle_time = rx_settle_us;
    //tx
    if (tx_settle_us == TX_SETTLE_TIME_15US)
    {
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_0 = 0x00; //sub-sequence1 start time:0
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_1 = 0x00; //sub-sequence2 start time:0us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb0 = 0x0c;  //sub-sequence3 start time:12us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb1 = 0x0d;  //sub-sequence4 start time:13us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_0 = 0x0f; //sub-sequence5 start time:15us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_1 = 0x0c; //sub-sequence6 start time:12us

        //disable Bandgap(8us),tx_ldo_trim(8.5us),PD_settle(5.5us),tx_fcal(12.5us),tx_hpmc(53us),save 25us
        //Default settle time:112.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl &= ~(FLD_RF_LDOT_TX_RUN_CB|FLD_RF_FCAL_TX_RUN_CB|FLD_RF_HPMC_RUN_CB);//0000
        reg_rf_txrx_en_dbg_ow_ctrl1 = (reg_rf_txrx_en_dbg_ow_ctrl1&0xfc); //1100
    }
    else if (tx_settle_us == TX_SETTLE_TIME_51US)
    {
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_0 = 0x00; //sub-sequence1 start time:0
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_1 = 0x08; //sub-sequence2 start time:8us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb0 = 0x30;  //sub-sequence3 start time:47.5us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb1 = 0x31;  //sub-sequence4 start time:48us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_0 = 0x33; //sub-sequence5 start time:51us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_1 = 0x30; //sub-sequence6 start time:48us

        //close hpmc and ldo trim,close hpmc(53us), ldotrim(8.5us),save 51us
        //Default settle time:112.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl&0xf0)|0x0a;//1010
    }
    else if(tx_settle_us == TX_SETTLE_TIME_104US)
    {
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_0 = 0x00; //sub-sequence1 start time:0
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_1 = 0x08; //sub-sequence2 start time:0us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb0 = 0x65;  //sub-sequence3 start time:101us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb1 = 0x66;  //sub-sequence4 start time:102us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_0 = 0x69; //sub-sequence5 start time:105us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_1 = 0x65; //sub-sequence6 start time:102us

        // only close ldo trim(8.5us)
        //Default settle time:112.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl&0xf8)|0x0e;//1110
    }

    //rx
    if (rx_settle_us == RX_SETTLE_TIME_15US)
    {
        reg_rf_idle_rx_ss1_ss2_strt_cb_0 = 0x00;//sub-sequence1 start time:0us
        reg_rf_idle_rx_ss1_ss2_strt_cb_1 = 0x00;    //sub-sequence2 start time:0us
        reg_rf_idle_rx_ss3_ss4_strt_cb_0 = 0x00;    //sub-sequence3 start time:0us
        reg_rf_idle_rx_ss3_ss4_strt_cb_1 = 0x08;    //sub-sequence4 start time:8us
        reg_rf_idle_rx_ss5_ss6_strt_cb_0 = 0x0f;    //sub-sequence5 start time:11us
        reg_rf_idle_rx_ss5_ss6_strt_cb_1 = 0x0f;    //sub-sequence6 start time:11us

        //disable Bandgap(8us), rx_ldo_trim(8.5us), PD_settle(5.5us), rx_fcal(12.5us),rx_rccal(9us), rx_dcoc(40us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl &= ~(FLD_RF_RXDCOC_RUN_CB|FLD_RF_RCCAL_RUN_CB|FLD_RF_FCAL_RX_RUN_CB|FLD_RF_LDOT_RX_RUN_CB);//0000
        reg_rf_txrx_en_dbg_ow_ctrl1 = (reg_rf_txrx_en_dbg_ow_ctrl1 & 0xf3);//0011
    }
    else if (rx_settle_us == RX_SETTLE_TIME_37US)
    {
        reg_rf_idle_rx_ss1_ss2_strt_cb_0 = 0x00;//sub-sequence1 start time:0us
        reg_rf_idle_rx_ss1_ss2_strt_cb_1 = 0x08;//sub-sequence2 start time:8us
        reg_rf_idle_rx_ss3_ss4_strt_cb_0 = 0x08;//sub-sequence3 start time:8us
        reg_rf_idle_rx_ss3_ss4_strt_cb_1 = 0x22;//sub-sequence4 start time:34us
        reg_rf_idle_rx_ss5_ss6_strt_cb_0 = 0x25;//sub-sequence5 start time:37us
        reg_rf_idle_rx_ss5_ss6_strt_cb_1 = 0x25;//sub-sequence6 start time:37us

        //disable ldo trim(8.5us),rx dcoc(40us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl & 0x0F) | 0x60;//0110
    }
    else if (rx_settle_us == RX_SETTLE_TIME_77US)
    {
        reg_rf_idle_rx_ss1_ss2_strt_cb_0 = 0x00;//sub-sequence1 start time:0us
        reg_rf_idle_rx_ss1_ss2_strt_cb_1 = 0x08;//sub-sequence2 start time:9us
        reg_rf_idle_rx_ss3_ss4_strt_cb_0 = 0x08;//sub-sequence3 start time:9us
        reg_rf_idle_rx_ss3_ss4_strt_cb_1 = 0x22;//sub-sequence4 start time:34us
        reg_rf_idle_rx_ss5_ss6_strt_cb_0 = 0x4d;//sub-sequence5 start time:77us
        reg_rf_idle_rx_ss5_ss6_strt_cb_1 = 0x4d;//sub-sequence6 start time:77us

        //disable ldo trim(8.5us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl & 0x0f)|0xe0;//1110
    }
    return 0;
}

/**
 *  @brief      This function serve to enable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_tx_fast_settle_en(void)
{
    if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_15US)
    {
        g_rf_tx_fast_settle_chn_cal_flag = 1;
        reg_rf_hpmc_debug_0 |= FLD_RF_HPMC_BYPASS; //hpmc bypass enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass enable.
        reg_rf_tx_frac_ctrl0 |= FLD_RF_FCAL_STL_DCAP_EN;//FCAL settle enable
    }
    else if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_51US)
    {
        g_rf_tx_fast_settle_chn_cal_flag = 1;
        reg_rf_hpmc_debug_0 |= FLD_RF_HPMC_BYPASS; //hpmc bypass enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass enable.
    }
    else if(g_rf_tx_fast_settle_time == TX_SETTLE_TIME_104US)
    {
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass enable.
    }
    reg_rf_burst_cfg_txrx_1 |= FLD_RF_TX_TIM_SRQ_SEL_TESQ;
}

/**
 *  @brief      This function serve to disable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_tx_fast_settle_dis(void)
{
    g_rf_tx_fast_settle_chn_cal_flag = 0;
    g_rf_tx_fast_settle_time = TX_FAST_SETTLE_NONE;
    reg_rf_hpmc_debug_0 &= ~FLD_RF_HPMC_BYPASS; //hpmc bypass disable
    reg_rf_ldot_dbg1 &= ~FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass disable
    reg_rf_ldot_dbg2_0 &= ~(FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass disable
    reg_rf_ldot_dbg3_0 &= ~(FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass disable.
    reg_rf_tx_frac_ctrl0 &= ~FLD_RF_FCAL_STL_DCAP_EN;//FCAL settle disable

    reg_rf_burst_cfg_txrx_1 &= ~FLD_RF_TX_TIM_SRQ_SEL_TESQ;//tx fast settle disable
}

/**
 *  @brief      This function serve to enable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_rx_fast_settle_en(void)
{
    if (g_rf_rx_fast_settle_time == RX_SETTLE_TIME_15US)
    {
        reg_rf_dcoc_bypass_dac_0 |= FLD_RF_DCOC_BYPASS_DAC;//dcoc bypass dac enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass enable.
        reg_rf_tx_frac_ctrl0 |= FLD_RF_FCAL_STL_DCAP_EN;//FCAL settle enable
    }
    else if (g_rf_rx_fast_settle_time == RX_SETTLE_TIME_37US)
    {
        reg_rf_dcoc_bypass_dac_0 |= FLD_RF_DCOC_BYPASS_DAC;//dcoc bypass dac enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass enable.
    }
    else if (g_rf_rx_fast_settle_time == RX_SETTLE_TIME_77US)
    {
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass enable.
    }
    reg_rf_burst_cfg_txrx_1 |= FLD_RF_RX_TIM_SRQ_SEL_TESQ;
}

/**
 *  @brief      This function serve to disable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_rx_fast_settle_dis(void)
{
    g_rf_rx_fast_settle_time = RX_FAST_SETTLE_NONE;
    reg_rf_dcoc_bypass_dac_0 &= ~FLD_RF_DCOC_BYPASS_DAC;//dcoc bypass dac disable
    reg_rf_ldot_dbg1 &= ~FLD_RF_LDOT_LDO_CAL_BYPASS;//ldo cal bypass disable
    reg_rf_ldot_dbg2_0 &= ~(FLD_RF_LDOT_LDO_RXTXHF_BYPASS|FLD_RF_LDOT_LDO_RXTXLF_BYPASS);//ldo RXTXHF/RXTXLF bypass disable
    reg_rf_ldot_dbg3_0 &= ~(FLD_RF_LDOT_LDO_PLL_BYPASS|FLD_RF_LDOT_LDO_VCO_BYPASS);//ldo PLL/VCO bypass disable.
    reg_rf_tx_frac_ctrl0 &= ~FLD_RF_FCAL_STL_DCAP_EN;//FCAL settle disable

    reg_rf_burst_cfg_txrx_1 &= ~FLD_RF_RX_TIM_SRQ_SEL_TESQ;//tx fast settle disable
}

/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim calibration value address pointer
 *  @return     none
*/
static rf_ldo_trim_t rf_get_ldo_trim_val(void)
{
    rf_ldo_trim_t ldo_cla;
    ldo_cla.LDO_CAL_TRIM = reg_rf_ldot_rdbk1 & FLD_RF_LDOT_LDO_CAL_TRIM;
    ldo_cla.LDO_RXTXHF_TRIM = reg_rf_ldot_rdbk2_0 & FLD_RF_LDOT_LDO_RXTXHF_TRIM;
    ldo_cla.LDO_RXTXLF_TRIM = ((reg_rf_ldot_rdbk2_1 & FLD_RF_LDOT_LDO_RXTXLF_TRIM_H) << 2) + ((reg_rf_ldot_rdbk2_0 & FLD_RF_LDOT_LDO_RXTXLF_TRIM_L) >> 6);
    ldo_cla.LDO_PLL_TRIM = reg_rf_ldot_rdbk3_0 & FLD_RF_LDOT_LDO_PLL_TRIM;
    ldo_cla.LDO_VCO_TRIM = ((reg_rf_ldot_rdbk3_1 & FLD_RF_LDOT_LDO_VCO_TRIM_H) << 2) + ((reg_rf_ldot_rdbk3_0 & FLD_RF_LDOT_LDO_VCO_TRIM_L) >> 6);
    return ldo_cla;
}

/**
 *  @brief      This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return     none
*/
static void rf_set_ldo_trim_val(rf_ldo_trim_t ldo_trim)
{
    reg_rf_ldot_dbg1 = (ldo_trim.LDO_CAL_TRIM << 1);
    reg_rf_ldot_dbg2_0 = (ldo_trim.LDO_RXTXHF_TRIM << 2);
    reg_rf_ldot_dbg2_1 = ldo_trim.LDO_RXTXLF_TRIM & FLD_RF_LDOT_LDO_RXTXLF_TRIM_OVERWRITE;
    reg_rf_ldot_dbg3_0 = (ldo_trim.LDO_PLL_TRIM << 2);
    reg_rf_ldot_dbg3_1 = ldo_trim.LDO_VCO_TRIM & FLD_RF_LDOT_LDO_VCO_TRIM_OVERWRITE;
}

/**
 *  @brief      This function is mainly used to get hpmc Calibration-related values.
 *  @param[in]  none
 *  @return     Returns the hpmc_gain value
*/
_attribute_ram_code_sec_noinline_ static unsigned short rf_get_hpmc_cal_val(void)
{
    unsigned short cali;
    unsigned short hpmc_gain;
    cali = read_reg16(0x1706fe);
    hpmc_gain = (cali<<1)& 0x0ffe;
    return hpmc_gain;
}

/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
static rf_dcoc_cal_t rf_get_dcoc_cal_val(void)
{
    rf_dcoc_cal_t dcoc_cal;
    dcoc_cal.DCOC_IDAC = reg_rf_dcoc_rdbk1_0 & FLD_RF_DCOC_IDAC_CODE;//DCOC_IDAC 0xd8[5:0]
    dcoc_cal.DCOC_QDAC = reg_rf_dcoc_rdbk2 & FLD_RF_DCOC_QDAC_CODE;//DCOC_QDAC 0xda[5:0]
    dcoc_cal.DCOC_IADC_OFFSET = reg_rf_dcoc_rdbk3_0 & FLD_RF_DCOC_IADC_OFFSET;//DCOC_IADC_OFFSET 0xdc[6:0]
    dcoc_cal.DCOC_QADC_OFFSET = (reg_rf_dcoc_rdbk3_0 & FLD_RF_DCOC_QADC_OFFSET_L) >> 7 |(reg_rf_dcoc_rdbk3_1 & FLD_RF_DCOC_QADC_OFFSET_H) << 1;//DCOC_QADC_OFFSET 0xdc[7] 0xdd[5:0]
    return dcoc_cal;
}


/**
 *  @brief      This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return     none
*/
static void rf_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal)
{
    reg_rf_dcoc_bypass_dac_0 = (dcoc_cal.DCOC_IDAC << 1);
    reg_rf_dcoc_bypass_dac_0 = reg_rf_dcoc_bypass_dac_0|((dcoc_cal.DCOC_QDAC&0x01) << 7);
    reg_rf_dcoc_bypass_dac_1 = ((dcoc_cal.DCOC_QDAC)&0x3e) >> 1;
    reg_rf_dcoc_bypass_adc_0 = (dcoc_cal.DCOC_IADC_OFFSET << 1) | 0x01;
    reg_rf_dcoc_bypass_adc_1 = dcoc_cal.DCOC_QADC_OFFSET;
}

/**
 *  @brief      This function is mainly used to get fcal Calibration-related values.
 *  @return     fcal dcap value
*/
_attribute_ram_code_sec_noinline_ static unsigned char rf_get_fcal_cal_val(void)
{
    return reg_rf_fcal_rdbk;
}

/**
 *  @brief      This function serves to set the range of chn group corresponding to the process of obtaining fcal calibration values at different frequency points
 *  @param[in]  *fcal_chn_range  - chn group range pointer(fcal_chn_range[0]<=chn_num <= fcal_chn_range[1])
 *  @return     none
 *  @note       If the frequency point is set using the rf_set_chn() interface when obtaining FCAL calibration values for different chn,
 *              this interface needs to be used to set the range of chn.
*/
static void rf_set_fcal_chn_group_range_ctf(unsigned short *fcal_chn_range)
{
    for(int i=0;i<8;i++)
    {
        fcal_chn_range[i] *= 2;
        reg_rf_fcal_chn_range_ctf_low(i)= fcal_chn_range[i]&0xff;
        reg_rf_fcal_chn_range_ctf_high(i)= (fcal_chn_range[i]>>8)&0x1f;
    }
    reg_rf_txrx_dbg3_0 |= FLD_RF_CHNL_FREQ_DIRECT;//CHNL Frequency decided by TXRX_DBG.CHNL_FREQ
}

/**
 *  @brief      This function is mainly used to get rccal Calibration-related values.
 *  @param[in]  rccal_cal   - rccal calibration value address pointer
 *  @return     none
*/
void rf_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal)
{
    rccal_cal->RCCAL_CODE = read_reg8(0x1706ca)&0x1f;
    rccal_cal->CBPF_CCODE_L = read_reg8(0x1706ca)&0xe0 >> 5;
    rccal_cal->CBPF_CCODE_H = read_reg8(0x1706cb)&0x0f;
}

/**
 *  @brief      This function is mainly used to set rccal Calibration-related values.
 *  @param[in]  rccal_cal    - rccal Calibration-related values.
 *  @return     none
*/
void rf_set_rccal_cal_val(rf_rccal_cal_t rccal_cal)
{
    write_reg8(0x1706c6,(rccal_cal.RCCAL_CODE));
    write_reg8(0x1706c6,(rccal_cal.CBPF_CCODE_L & 0x01 ) << 7 | (read_reg8(0x1706c6)|BIT(6)));//CBPF_CCODE_BYPASS
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_L & 0x02) >> 1 | read_reg8(0x1706c7));
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_H << 1 | (read_reg8(0x1706c7)|BIT(6)) | BIT(7)));//RCCAL BYPASS
}

/**
 *  @brief      This function is used to set the tx fast_settle calibration value.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to TX_SETTLE_TIME_15US and TX_SETTLE_TIME_51US, other parameters are invalid.
 *                              (When tx_settle_us is 15us or 51us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @return     none
 *  @note       TX_SETTLE_TIME_15US  - disable Bandgap,tx_ldo_trim,PD_settle,tx_fcal,tx_hpmc,reduce 87.5us of tx settle time.
                                       After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_51US  - disable tx_ldo_trim function and tx_hpmc,reduce 61.5us of tx settle time.After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_104US - disable tx_ldo_trim function,reduce 8.5us of tx settle time. Do a normal calibration at the beginning.
*/
void rf_tx_fast_settle_update_cal_val(rf_tx_fast_settle_time_e tx_settle_time,unsigned char chn)
{
    unsigned short rf_fcal_range[8]={2400,2410,2420,2430,2440,2450,2460,2470};
    if(tx_settle_time == TX_SETTLE_TIME_15US)
    {
        if(chn <= 80)
        {
            g_fast_settle_cal_val.cal_tbl[chn] = rf_get_hpmc_cal_val();
            if(chn%10 == 4)
            {
                rf_set_fcal_chn_group_range_ctf(rf_fcal_range);
                reg_rf_fcal_ctrl_tx(chn/10) = rf_get_fcal_cal_val();
            }
        }
    }
    else if(tx_settle_time == TX_SETTLE_TIME_51US)
    {
        if(chn <= 80)
        {
            g_fast_settle_cal_val.cal_tbl[chn] = rf_get_hpmc_cal_val();
        }
    }
    g_fast_settle_cal_val.ldo_trim=rf_get_ldo_trim_val();
    rf_set_ldo_trim_val(g_fast_settle_cal_val.ldo_trim);
}

/**
 *  @brief      This function is used to set the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to RX_SETTLE_TIME_15US, other parameters are invalid.
 *                              (When rx_settle_us is 15us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @return     none
 *  @note       RX_SETTLE_TIME_15US  - disable Bandgap, rx_ldo_trim, PD_settle, rx_fcal,rx_rccal, rx_dcoc calibration,reduce 74us of rx settle time.
                                       Receive for a period of time and then do a normal calibration
 *              RX_SETTLE_TIME_37US  - disable rx_ldo_trim and rx_dcoc calibration,reduce 48.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_77US  - disable rx_ldo_trim calibration,reduce 8.5us of rx settle time. Do a normal calibration at the beginning.
*/
void rf_rx_fast_settle_update_cal_val(rf_rx_fast_settle_time_e rx_settle_time,unsigned char chn)
{
    unsigned short rf_fcal_range[8]={2400,2410,2420,2430,2440,2450,2460,2470};
    if(rx_settle_time == RX_SETTLE_TIME_15US)
    {
        if(chn <= 80)
        {
            if(chn%10 == 4)
            {
                rf_set_fcal_chn_group_range_ctf(rf_fcal_range);
                reg_rf_fcal_ctrl_rx(chn/10) = rf_get_fcal_cal_val();
            }
        }
        g_fast_settle_cal_val.dcoc_cal=rf_get_dcoc_cal_val();
        rf_set_dcoc_cal_val(g_fast_settle_cal_val.dcoc_cal);
    }
    else if(rx_settle_time == RX_SETTLE_TIME_37US)
    {
        g_fast_settle_cal_val.dcoc_cal=rf_get_dcoc_cal_val();
        rf_set_dcoc_cal_val(g_fast_settle_cal_val.dcoc_cal);
    }
    g_fast_settle_cal_val.ldo_trim=rf_get_ldo_trim_val();
    rf_set_ldo_trim_val(g_fast_settle_cal_val.ldo_trim);
}

/**
 * @brief      This function serves to reset RF digital logic states.
 * @return     none
 * @note       This function requires setting reset zb, rstl_bb, and rst_mdm.
 *             It is used to clear RF related state machines, IRQ states, and digital internal logic states.
 */
_attribute_ram_code_sec_noinline_ void rf_clr_dig_logic_state(void)
{
    reg_n22_rst &= ~((FLD_RST0_ZB)|((FLD_RST1_RSTL_BB|FLD_RST1_RST_MDM)<<8));
    reg_n22_rst |= ((FLD_RST0_ZB)|((FLD_RST1_RSTL_BB|FLD_RST1_RST_MDM)<<8));
}

/**
 * @brief      This function is used to restore the rf related registers to their default values.
 * @return     none
 * @note       (1)After calling this interface, all configured interfaces of rf need to be called again.
 *             (2)After calling this interface, the tick of bb timer will be reset to zero.
 *             (3)After calling this interface, RF DMA configurations need to be reconfigured.
 */
_attribute_ram_code_sec_noinline_ void rf_reset_register_value(void)
{
    reg_n22_rst0 &= ~FLD_RST0_ZB_PON;
    reg_n22_rst0 |= FLD_RST0_ZB_PON;
}

/**
 * @brief          This function is mainly used to set the energy when sending a single carrier.
 * @param[in]    level        - The slice corresponding to the energy value.
 * @return         none.
 */
void rf_set_power_level_singletone(rf_power_level_e level)
{
    unsigned char value = 0;

    if(level&BIT(7))
    {
        reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;// VANT
    }
    else
    {
        reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
    }
    value = (unsigned char)level&0x3f;
    reg_rf_lnm_pa_ow_ctrl_val |= BIT(6);                            // TX_PA_PWR_OW  BIT6 set 1
    reg_rf_pa_ow_val = ((reg_rf_pa_ow_val&0x81)|(value<<1));        // TX_PA_PWR  BIT1 t0 BIT6 set value
}
