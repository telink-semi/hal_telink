/********************************************************************************************************
 * @file    ext_hadm_rf.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "tl_common.h"
#include "drivers.h"
#include "ext_lib.h"
#include "ext_rf.h"
#include "stack/ble/controller/ble_controller.h"
#include "lib/include/rf/rf_common.h"

#if 1 //20250516 lijing enable
////////////////////////////////rf_cs.h start/////////////////////////////////
static unsigned int g_iq_data_len,g_iq_group_num,g_sample_interval;

typedef struct __attribute__((packed))
{
    unsigned int crc;
    unsigned int r_tstamp;
    unsigned short pkt_fdc:11;
    unsigned short sync_flag:1;
    unsigned short rx_pkt_chn_efuse:3;
    unsigned short tlk_mic_err:1;
    unsigned char pkt_rssi;
    unsigned char crc_error:1;
    unsigned char sfd_error:1;
    unsigned char link_layer_error:1;
    unsigned char power_error:1;
    unsigned char long_range_125K_indicator:1;
    unsigned char :2;
    unsigned char xxx_NoACK_indicator:1;
    unsigned int iq_start_tstamp;
    unsigned short match_reg_sync:9;
    unsigned short :3;
    unsigned short filt_agc_gain:3;
    unsigned short :1;
    unsigned char :8;
    unsigned char :8;
    unsigned int hpm_time_pos;
    unsigned int hpm_time_neg;
    unsigned int tr_turnaround_time_pos;
    unsigned int tr_turnaround_time_neg;
    unsigned int tx_pa_pup_time_pos;
    unsigned int tx_pa_pup_time_neg;
    unsigned int en_freq_comp_pos;
    unsigned int tx_frac_time_pos;
    unsigned int tx_frac_time_neg;
}ble_pkt_extend_info_t, *ble_pkt_extend_info_t_ptr;


/**********************************************************************************************************************
 *                                         RF : AOA/D related functions                                    *
 *********************************************************************************************************************/

/**
 * @brief        This function is mainly used for the disable hpmc trim function.
 * @return        none.
 */
_attribute_ram_code_  // ble used
void rf_cs_dis_hpmc_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(5)));    //HPMC_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(5));    //HPMC_RUN_OW
}

/**
 * @brief        This function is mainly used for the disable ldo trim function.
 * @return        none.
 */
_attribute_ram_code_  //ble use
void rf_cs_dis_ldo_trim(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(2)));//LDOT_DEBUG_RUN
    write_reg8(0x170681,read_reg8(0x170681)|BIT(2));//LDOT_DEBUG_RUN_OW
}

/**
 * @brief        This function is mainly used for the disable dcoc trim function.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_cs_dis_dcoc_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(4))); //RXDCOC_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(4));    //RXDCOC_RUN_OW
}

/**
 * @brief        This function is mainly used for the disable rccal trim function.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_cs_dis_rccal_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(2))); //RCCAL_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(2));    //RCCAL_RUN_OW
}

/**
 * @brief        This function is mainly used for the disable fcal trim function.
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_cs_dis_fcal_trim(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3))); //FCAL_DEBUG_RUN
    write_reg8(0x170681,read_reg8(0x170681)|BIT(3));  //FCAL_DEBUG_RUN_OW
}

/**
 * @brief        This function is used to set the position of the first antenna switch after the AOA receiver reference.The default is in the
 *                 middle of the first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code.Each additional code
 *                 will move the switch point back by 0.125us
 * @param[in]    swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
 *                             back 0.125us.
 * @return        none.
 */
void rf_aoa_rx_ant_switch_point_adjust(unsigned short swt_offset)
{
    unsigned char temp = (((swt_offset >> 8) & 0x01) << 2);
    reg_rf_ant_msb = ((((reg_rf_ant_msb) & (~FLD_RF_RX_ANT_OFFSET_MSB))) | temp);
    reg_rf_rx_antoffset = swt_offset & 0xff;
}


/**
 * @brief        This function is used to set the position of the first antenna switch after the AOD transmitter reference.The default is in the middle of the
 *                 first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code. Each additional code will move
 *                 the switch point back by 0.125us
 * @param[in]    swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
 *                             back 0.125us.
 * @return        none.
 */
void rf_aod_tx_ant_switch_point_adjust(unsigned short swt_offset)
{
    unsigned char temp = (((swt_offset >> 8) & 0x01) << 1);
    reg_rf_ant_msb = ((((reg_rf_ant_msb) & (~FLD_RF_TX_ANT_OFFSET_MSB))) | temp);
    reg_rf_tx_antoffset = swt_offset & 0xff;
}

/**
 * @brief        This function is mainly used to set the IQ data sample interval time. In normal mode, the sampling interval of AOA is 4us, and AOD will judge whether
 *                 the sampling interval is 4us or 2us according to CTE info.The 4us/2us sampling interval corresponds to the 2us/1us slot mode stipulated in the protocol.
 *                 Since the current hardware only supports the antenna switching interval of 4us/2us, setting the sampling interval to 1us or less will cause multiple
 *                 sampling at the interval of one antenna switching. Therefore, the sampling data needs to be processed by the upper layer according to the needs, and
 *                 currently it is mostly used Used in the debug process.
 *                 After configuring RF, you can call this function to configure slot time.
 * @param[in]    time_us    - AOA or AOD slot time mode.
 * @return        none.
 * @note        Attention:(1)When the time is 0.25us, it cannot be used with the 20bit iq data type, which will cause the sampling data to overflow.
 *                           (2)Since only the antenna switching interval of 4us/2us is supported, the sampling interval of 1us and shorter time intervals
 *                               will be sampled multiple times in one antenna switching interval. Suggestions can be used according to specific needs.
 */
void rf_aoa_aod_sample_interval_time(rf_aoa_aod_sample_interval_time_e time_us)
{
    if(time_us <= SAMPLE_2US_INTERVAL)
    {
        reg_rf_man_ant_slot = ((reg_rf_man_ant_slot & 0xcf)|time_us);
        BM_CLR(reg_rf_mode_ctrl0,FLD_RF_INTV_MODE);
    }
    else
    {
        reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & (~FLD_RF_INTV_MODE))|(time_us-3));
        BM_CLR(reg_rf_man_ant_slot,(FLD_RF_SLOT_1US_MAN_EN|FLD_RF_SLOT_1US_MAN));
    }
}

/**
 * @brief        This function is mainly used to set the type of AOA/AODiq data. The default data type is 8bit. This configuration can be done before starting to receive
 *                 the package.
 * @param[in]    mode    - The length of each I or Q data.
 * @return        none.
 */
void rf_aoa_aod_iq_data_mode(rf_iq_data_mode_e mode)
{
    reg_rf_sof_offset = ((reg_rf_sof_offset & (~FLD_RF_SUPP_MODE))|((mode&0x07) << 4));
    g_iq_data_len = mode;
}

/****************************************************************************************************************************************
 *                                         RF : Channel Sounding related functions                                                                     *
 ****************************************************************************************************************************************/

/**
 * @brief        This function is mainly used to initialize some parameter settings of the CS IQ sample.
 * @param[in]    samp_num    - Number of groups to sample IQ data.Value range 0x00~0xffff.
 * @param[in]    interval    - The interval time between each IQ sampling is (interval + 1)*0.125us.
 *                               Value range 0x01~0x0f.
 * @param[in]    start_point    - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                               at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                               starts at (start_point + 1) * 0.125us after sync.Value range 0x00~0xff.
 * @param[in]    suppmode    - The length of each I or Q data.
 * @param[in]    sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return        none.
 */
void rf_cs_iq_sample_init(unsigned short samp_num,unsigned char interval,unsigned char start_point,rf_iq_data_mode_e suppmode,rf_cs_iq_sample_mode_e sample_mode)
{
    rf_cs_iq_sample_number(samp_num);
    rf_cs_sample_interval_time(interval);
    rf_cs_iq_start_point(start_point);
    rf_aoa_aod_iq_data_mode(suppmode);
    rf_cs_iq_sample_mode(sample_mode);
    rf_cs_iq_sample_enable();

}

/**
 * @brief        This function is mainly used to set the IQ sample interval.
 * @param[in]    interval     - Set the interval for IQ sample, (interval + 1)*0.125us.Value range 0x01~0x0f.
 * @return        none.
 * @note         Sampling frequency = 1/sampling interval, so the maximum sampling frequency is 4MHz.
 */
void rf_cs_sample_interval_time(unsigned char interval)
{
    reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & (~FLD_RF_IQ_SAMP_INTERVAL)) | (interval<<4));
}

/**
 * @brief        This function is mainly used to initialize the parameters related to cs antennas.
 * @param[in]    clk_mode    - Set whether the antenna-related clock is always on or only when switching antennas.
 * @param[in]    ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.
 * @param[in]    ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @param[in]    ant_txoffset- Adjust the switching start point of the tx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @return        none.
 */
void rf_cs_ant_init(rf_cs_ant_clk_mode_e clk_mode,unsigned char ant_interval,unsigned char ant_rxoffset,unsigned char ant_txoffset)
{
    rf_cs_ant_clk_mode(clk_mode);
    rf_cs_set_ant_interval(ant_interval);
    rf_cs_set_rx_ant_offset(ant_rxoffset);
    rf_cs_set_tx_ant_offset(ant_txoffset);
}

/**
 * @brief        * This function initializes the GPIO pins used for antenna switching according to the specified configuration.
 *                 It disables the input and output functions of the GPIO pins, sets the pull-down resistance, and configures
 *                 the multiplexing function of the pins.
 *
 * @param[in]   switch_ctrl    - Pointer to the antenna switch control structure array.
 * @param[in]   num            - Number of antenna switch pins
 * @return      none.
 */
void rf_cs_ant_switch_pin_init(rf_cs_ant_switch_ctrl *switch_ctrl, u8 num){

    for(int i = 0; i< num; i++){
        rf_cs_ant_switch_ctrl *pCtrl = &switch_ctrl[i];
        // set ANT config GPIO
        gpio_function_dis(pCtrl->pin);
        gpio_input_dis(pCtrl->pin);
        gpio_output_dis(pCtrl->pin);
        gpio_set_up_down_res(pCtrl->pin, GPIO_PIN_PULLDOWN_100K);

        gpio_set_mux_function(pCtrl->pin,  pCtrl->pin_fun);
    }
}

/**
 * @brief        This function is mainly used to set the antenna switching interval.
 * @param[in]    ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.Value range 0x00~0x1ff.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_ant_interval(unsigned short ant_interval)
{
    write_reg8(0x170035,ant_interval);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(0)))|(ant_interval>>8));
}

/**
 * @brief        This function is mainly used to set the starting position of the antenna switching at the rx-side.
 * @param[in]    ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 *                 Value range 0x00~0x1ff.
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_cs_set_rx_ant_offset(unsigned short ant_rxoffset)
{
    write_reg8(0x17003a,ant_rxoffset);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(2)))|((ant_rxoffset>>8)<<2));
    write_reg8(0x170007,read_reg8(0x170007)|BIT(2));//rx_ant_switch_en
}

/**
 * @brief        This function is mainly used to set the starting position of the antenna switching at the tx-side.
 * @param[in]    ant_txoffset- Adjust the switching start point of the rx-side antenna,(ant_txoffset + 1)*0.125us.
 *                 Value range 0x00~0x1ff.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_tx_ant_offset(unsigned short ant_txoffset)
{
    write_reg8(0x170039,ant_txoffset);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(1)))|((ant_txoffset>>8)<<1));
//    write_reg8(0x170007,(read_reg8(0x170007)&0xfc)|0x02);//tx_ant_switch_en  check by lijing, needn't this setting
}

/**
 * @brief        This function is mainly used to set the clock working mode of the antenna.
 * @param[in]    clk_mode    - Open all the time or only when switching antennas.
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_cs_ant_clk_mode(rf_cs_ant_clk_mode_e clk_mode)
{
    reg_rf_rxclk_auto = ((reg_rf_rxclk_auto&0xfe) | clk_mode);
}

/**
 * @brief        This function is mainly used to set the way IQ sampling starts.
 * @param[in]    sample_mode    - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return        none.
 */
void rf_cs_iq_sample_mode(rf_cs_iq_sample_mode_e sample_mode)
{
    if(sample_mode == RF_CS_IQ_SAMPLE_SYNC_MODE)
    {
        reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;
    }
    else
    {
        reg_rf_rxlatf &= (~FLD_RF_R_IQ_SAMP_MODE);
    }
}

/**
 * @brief        This function is mainly used to set the starting position of IQ sampling.
 * @param[in]    start_point  - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                               at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                               starts at (start_point + 1) * 0.125us after sync.value range is 0x00~0xff.
 *                               The rx_en mode and sync mode can be configured with the function rf_cs_iq_sample_mode.
 * @return        none.
 */
void rf_cs_iq_start_point(unsigned char start_point)
{
    reg_rf_iq_samp_start = start_point;
}

/**
 * @brief        This function is mainly used to set the number of IQ samples in groups.
 * @param[in]    samp_num    - Number of groups to sample IQ data.Value range 0x00~0xffff.
 * @return        none.
 */
void rf_cs_iq_sample_number(unsigned short samp_num)
{
    reg_rf_iq_samp_num = samp_num;
    g_iq_group_num = samp_num;
}

/**
 * @brief        This function is mainly used to enable the IQ sampling function.
 * @return        none.
 */
void rf_cs_iq_sample_enable()
{
    reg_rf_mode_ctrl0 |= FLD_RF_IQ_SAMP_EN;
}

/**
 * @brief        This function is mainly used to disable the IQ sampling function.
 * @return        none.
 */
void rf_cs_iq_sample_disable()
{
    reg_rf_mode_ctrl0 &= (~FLD_RF_IQ_SAMP_EN);
}

/**
 * @brief        This function is mainly used to obtain the sync flag bit from the packet, which is
 *                 used to identify whether the packet is data received after passing sync.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the Sync flag information in the packet.
 */
unsigned char rf_cs_get_pkt_sync_flag(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return ((p[x*sample_num+9]&BIT(3))>>3);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->sync_flag;
}

/**
 * @brief        This function is mainly used to obtain the packet quality indicator from the packet,which is
 *                 used to identify the quality of the received packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the packet quality information in the packet.0:very good;1:good;2:bad;
 */
unsigned char rf_cs_get_packet_quality_indicator(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0; unsigned char quality_indicator = 0;
    x = ((data_len >> 8) & 0xff);
//    quality_indicator = 32 - (p[x*sample_num+16]);
    quality_indicator = 32 - ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->match_reg_sync;
    return  (quality_indicator > 2 ? 2 : quality_indicator);
}

/**
 * @brief        This function is mainly used to get the timestamp of the sync-to-packet moment from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the Sync timestamp information in the packet.
 */
unsigned int rf_cs_get_pkt_rx_sync_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+7]<<24 | p[x*sample_num+6]<<16 | p[x*sample_num+5]<<8 | p[x*sample_num+4]);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->r_tstamp;
}

/**
 * @brief        This function is mainly used to get the timestamp of the moment tx_en is pulled up from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment tx_en is pulled up in the packet.
 */
unsigned int rf_cs_get_pkt_tx_pos_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+31]<<24 | p[x*sample_num+30]<<16 | p[x*sample_num+29]<<8 | p[x*sample_num+28]);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->tr_turnaround_time_pos;
}

/**
 * @brief        This function is mainly used to get the timestamp of the moment tx_en is pulled down from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information  of the moment tx_en is pulled down in the packet.
 */
unsigned int rf_cs_get_pkt_tx_neg_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+35]<<24 | p[x*sample_num+34]<<16 | p[x*sample_num+33]<<8 | p[x*sample_num+32]);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->tr_turnaround_time_neg;
}

/**
 * @brief        This function is mainly used to get the timestamp of the moment when the IQ data collection starts from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment when the IQ data collection starts in the packet.
 */
unsigned int rf_cs_get_pkt_iq_start_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+15]<<24 | p[x*sample_num+14]<<16 | p[x*sample_num+13]<<8 | p[x*sample_num+12]);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->iq_start_tstamp;
}

/**
 * @brief        This function is mainly used to get the timestamp of the moment when the IQ data collection starts from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment when the IQ data collection starts in the packet.
 */
unsigned int rf_cs_get_pkt_tx_frac_pos_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+15]<<24 | p[x*sample_num+14]<<16 | p[x*sample_num+13]<<8 | p[x*sample_num+12]);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->tx_frac_time_pos;
}

/**
 * @brief        This function is mainly used to get the timestamp of the moment when the IQ data collection starts from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment when the IQ data collection starts in the packet.
 */
unsigned int rf_cs_get_pkt_tx_frac_neg_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+15]<<24 | p[x*sample_num+14]<<16 | p[x*sample_num+13]<<8 | p[x*sample_num+12]);
    return ((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->tx_frac_time_neg;
}

/**
 * @brief        This function is mainly used to obtain the rssi information from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the rssi information in the packet.
 */
signed char rf_cs_get_pkt_rssi_value(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
{
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
//    return (p[x*sample_num+10]-110);
    return (((ble_pkt_extend_info_t_ptr)(p + x*sample_num))->pkt_rssi - 110);
}

/**
 * @brief       This function serves to set RF's channel.The step of this function is in KHz.
 *                The frequency set by this function is (chn+2400) MHz+chn_k KHz.
 * @param[in]   chn_m - RF channel. The unit of this parameter is MHz, and its set frequency
 *                          point is (2400+chn)MHz.
 * @param[in]   chn_k - The unit of this parameter is KHz, which means to shift chn_k KHz to
 *                         the right on the basis of chn.Its value ranges from 0 to 999.
 * @param[in]    trx_mode - Defines the frequency point setting of tx mode or rx mode.
 * @return      none.
 */
_attribute_ram_code_sec_ void rf_set_channel_k_step(signed char chn_m,unsigned int chn_k,rf_trx_chn_e trx_mode)//general
{
    unsigned int rf_chn_k =0;
    unsigned int ctrim_k;
    unsigned int temp_k;
    long chnl_freq_k;

    rf_set_chn(chn_m-trx_mode);

    rf_chn_k = (((chn_m+2400-trx_mode)*1000)+chn_k)*100;
    ctrim_k = rf_chn_k/48;
    temp_k = ((rf_chn_k/100000+24)/48);
    temp_k *= 100000;
    if(ctrim_k >= temp_k)
    {
        chnl_freq_k = ctrim_k - temp_k;
        chnl_freq_k = chnl_freq_k*2621/1000;
    }
    else
    {
        chnl_freq_k = temp_k - ctrim_k;
        chnl_freq_k = chnl_freq_k*2621/1000;
        chnl_freq_k = 0x40000 - chnl_freq_k;
    }
    write_reg8(0x170649,(chnl_freq_k & 0x3fc00)>>10);  //DSM_FRAC higher 8 bits
    write_reg8(0x170648, (chnl_freq_k & 0x3f8)>>2);   //DSM_FRAC next 7 bits
    write_reg8(0x170641, ((read_reg8(0x170641)&0xfc) | (chnl_freq_k & 0x06)>>1 ));  //DSM_FRAC next 2 bits
    write_reg8(0x170640, (read_reg8(0x170640)&0x7f) | ((chnl_freq_k & 0x01)<<7));  //DSM_FRAC last bit

    write_reg8(0x170648, (read_reg8(0x170648) | 0x01));  //enable DSM_FRAC_OW manual mode


}


/**
 * @brief        This function is mainly used to set the sequence related to Fast Settle in cs.
 * @return        none.
 * @note        This function needs to be called after rf_agc_disable, otherwise there will be problems with rssi
 *                 value exceptions.
 */
_attribute_ram_code_//ble use
void rf_agc_disable()
{
    char gain_lat, lna_hgain, lna_lgain, lna_attn, cbpf_gain;
    reg_rf_radio_txrx_dbg1_0 |= FLD_RF_AGC_DISABLE;
#if 0
    gain_lat = (read_reg8(0x170059)>>4)&0x07;
#else
    gain_lat = (read_reg8(0x17045c))&0x07; // ble use
#endif
    write_reg8(0x170640,(read_reg8(0x170640)&0xe3)|((gain_lat&0x07)<<2));

    if(gain_lat == 0)
    {
        lna_hgain = 0;
        lna_lgain = 1;
        lna_attn  = 3;
        cbpf_gain = 0;
    }
    else if(gain_lat == 1)
    {
        lna_hgain = 0;
        lna_lgain = 3;
        lna_attn  = 2;
        cbpf_gain = 1;
    }
    else if(gain_lat == 2)
    {
        lna_hgain = 0;
        lna_lgain = 3;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 3)
    {
        lna_hgain = 3;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 4)
    {
        lna_hgain = 0xf;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 5)
    {
        lna_hgain = 0x3f;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 6)
    {
        lna_hgain = 0;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else
    {
        lna_hgain = 0;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 0;
    }

    write_reg8(0x17077a,(read_reg8(0x17077a)&0x81)|(lna_hgain<<1));
    write_reg8(0x170778,read_reg8(0x170778)|0x02);

    write_reg8(0x17077b,(read_reg8(0x17077b)&0xfe)|(lna_lgain>>1));
    write_reg8(0x17077a,(read_reg8(0x17077a)&0x7f)|(lna_lgain<<7));
    write_reg8(0x170778,read_reg8(0x170778)|0x04);

    write_reg8(0x17077b,(read_reg8(0x17077b)&0xf9)|((lna_attn&0x03)<<1));
    write_reg8(0x170778,read_reg8(0x170778)|0x08);

    write_reg8(0x170782,(read_reg8(0x170782)&0xfd)|(cbpf_gain&0x01)<<1);
    write_reg8(0x170780,read_reg8(0x170780)|0x02);
}

/**
 * @brief        This function is mainly used for agc auto run.
 * @return        none.
 * @note        Call this function to enable agc auto tuning if you want to receive different energy packets correctly
 *                 after calling rf_agc_disable to disable agc auto tuning.
 */
_attribute_ram_code_ //ble use
void rf_agc_enable(void)
{
    reg_rf_radio_txrx_dbg1_0 &= (~FLD_RF_AGC_DISABLE);
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(1)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(2)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(3)));
    write_reg8(0x170780,read_reg8(0x170780)&(~BIT(1)));
}

/**
 * @brief        This function is mainly used to set the sequence related to Fast Settle in cs.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_phase_continuous(void)
{
    //seq_ldo_pll_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(3));    //LDO_PLL_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(3));    //LDO_PLL_PUP_OW

    //seq_ldo_vco_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(4));    //LDO_VCO_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(4));    //LDO_VCO_PUP_OW

    //seq_ldo_pll_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(3)));    //LDO_PLL_FC
    write_reg8(0x170761,read_reg8(0x170761)|BIT(3));    //LDO_PLL_FC_O

    //rf_seq_ldo_vco_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(4)));    //LDO_VCO_FC
    write_reg8(0x170761,read_reg8(0x170761)|BIT(4));    //LDO_VCO_FC_OW

    //seq_pd_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(0));    //PD_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(0));    //PD_PUP_OW

    //seq_pd_en_fcal_bias_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));    //PD_EN_FCAL_BIAS
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));    //PD_EN_FCAL_BIAS_OW

    //seq_xo_en_clk_ref_ow
    write_reg8(0x170770,read_reg8(0x170770)|BIT(3));    //XO_EN_CLK_REF
    write_reg8(0x170770,read_reg8(0x170770)|BIT(1));    //XO_EN_CLK_REF_OW

    //seq_vco_pup_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(0));    //VCO_PUP
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(0));    //VCO_PUP_OW

    //seq_lo_pup_vlo_fbk_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(6));    //LO_PUP_VLO_FBK
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(6));    //LO_PUP_VLO_FBK_OW

    //seq_fcal_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));    //FCAL_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));    //FCAL_PUP_OW

    //_seq_fcal_set_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));    //FCAL_SET
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));    //FCAL_SET_OW

    //seq_fcal_run_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));    //FCAL_RUN
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));    //FCAL_RUN_OW

    //seq_divn_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(6));    //DIVN_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(6));    //DIVN_PUP_OW

    //seq_divn_openloop_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(7)));    //DIVN_OPENLOOP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(7));    //DIVN_OPENLOOP_OW

    //ldo_rxtxhf_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(1));    //LDO_RXTXHF_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(1));    //LDO_RXTXHF_PUP_OW

    //ldo_lv_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(0));    //LDO_LV_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(0));    //LDO_LV_PUP_OW

    //bg_pup_ow
    write_reg8(0x170766,read_reg8(0x170766)|BIT(0));    //BG_PUP
    write_reg8(0x170764,read_reg8(0x170764)|BIT(0));    //BG_PUP_OW

    //rf_mixer_pup_ow
    write_reg8(0x17077b,read_reg8(0x17077b)|BIT(3));    //RX_MIX_PUP
    write_reg8(0x170778,read_reg8(0x170778)|BIT(4));    //RX_MIX_PUP_OW

    //dsm_run
    write_reg8(0x170682,read_reg8(0x170682)|BIT(0));    //DSM_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(0));    //DSM_RUN_OW

#if 0
    //rf_rx_dig_mixer_en_ow
    write_reg8(0x170688,read_reg8(0x170688)|BIT(1));    //RX_DIG_EN
    write_reg8(0x170686,read_reg8(0x170686)|BIT(1));    //RX_DIG_EN_OW
#else

    write_reg8(0x170450,read_reg8(0x170450)&(~BIT(5)));    //GFSK_AUTO
    write_reg8(0x170453,read_reg8(0x170453)|(BIT(1)));    //FREQ_COMP_EN
    write_reg8(0x170452,read_reg8(0x170452)|(BIT(5)));    //GFSK_EN

//    write_reg8(0x170451,read_reg8(0x170451)|(BIT(1)));    //FREQ_COMP_AUTO

#endif

    //rf_hpm_cal_disable
    write_reg8(0x170688,read_reg8(0x170688)&(~BIT(3)));    //TX_HPM_CAL_EN
    write_reg8(0x170686,read_reg8(0x170686)|BIT(3));    //TX_HPM_CAL_EN_OW

    //rf_seq_lo_pup_vlo_txfsk_ow
//    write_reg8(0x170792,read_reg8(0x170792)|BIT(6));    //LO_PUP_VLO_TXFSK
//    write_reg8(0x170790,read_reg8(0x170790)|BIT(6));    //LO_PUP_VLO_TXFSK_OW
//    write_reg8(0x170792,read_reg8(0x170792)|BIT(7));    //LO_PUP_VLO_TXFSKDRV
//    write_reg8(0x170790,read_reg8(0x170790)|BIT(7));    //LO_PUP_VLO_TXFSKDRV_OW
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(4));    //LO_PUP_VLO_TX_OW
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(5));    //LO_PUP_VLO_TXDRV_OW
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(4));    //LO_PUP_VLO_TX
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(5));    //LO_PUP_VLO_TXDRV

    //seq_lo_pup_vlo_rx_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(2));    //LO_PUP_VLO_RX
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(2));    //LO_PUP_VLO_RX_OW
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(3));    //LO_PUP_VLO_RXDRV
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(3));    //LO_PUP_VLO_RXDRV_OW


}
#endif


/**
 * @brief        This function is mainly used to turn off the energy of the tone.
 * @return        none.
 * @note        After setting the tone energy with rf_set_power_level_singletone, you need to call
 *                 rf_cs_set_power_off_singletone to turn off the tone energy if you enter the send packet.
 */
_attribute_ram_code_ //ble use
void rf_cs_set_power_off_singletone(void)
{
    write_reg8(0x17077c,(read_reg8(0x17077c)&0x81));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(6)));
}

#if 1 //20250516 lijing enable
/**
 * @brief        This function is mainly used to set the preparation and enable of manual fcal(frequency calibration).
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_manual_fcal_start(void)
{
//    rf_seq_pd_en_pd_drv_ow(0);
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(1));    //PD_EN_PD_DRV
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1)));    //PD_EN_PD_DRV_OW

    write_reg8(0x170738,read_reg8(0x170738)|BIT(2));    //BYPASS_CAL_CLK_GAT

//    rf_seq_pd_en_fcal_bias_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(2));    //PD_EN_FCAL_BIAS
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));    //PD_EN_FCAL_BIAS_OW

//    rf_seq_fcal_pup_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(3));    //FCAL_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));    //FCAL_PUP_OW

//    rf_seq_fcal_set_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));    //FCAL_SET
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4)));    //FCAL_SET_OW

//    rf_seq_fcal_run_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));    //FCAL_RUN
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5)));    //FCAL_RUN_OW

    write_reg8(0x170683,read_reg8(0x170683)|BIT(3));    //FCAL_DEBUG_RUN
}

/**
 * @brief        This function is mainly used to set the relevant value after manual fcal(frequency calibration).
 * @return        none.
 * @note        The function needs to be called after the rf_manual_fcal_start call 22us.
 */
_attribute_ram_code_ //ble use
void rf_manual_fcal_done(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//FCAL_DEBUG_RUN
    write_reg8(0x170738,read_reg8(0x170738)&(~BIT(2)));//BYPASS_CAL_CLK_GAT

//    rf_seq_pd_en_fcal_bias_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));    //PD_EN_FCAL_BIAS
//    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2)));

//    rf_seq_fcal_pup_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));    //FCAL_PUP
//    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3)));

//    rf_seq_fcal_set_ow();
//    write_reg8(0x17078a,read_reg8(0x17078a)|(BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));    //FCAL_SET_OW

//    rf_seq_fcal_run_ow();
//    write_reg8(0x17078a,read_reg8(0x17078a)|(BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));    //FCAL_RUN_OW

//    rf_seq_pd_en_pd_drv_ow(1);
    write_reg8(0x170788,read_reg8(0x170788)|BIT(1));    //PD_EN_PD_DRV_OW
}

/**
 *  @brief        This function is mainly used to get LDO Calibration-related values.
 *  @param[in]    ldo_trim   - ldo trim calibration value address pointer
 *  @return         none
*/
_attribute_ram_code_ // ble use
void rf_cs_get_ldo_trim_val(rf_ldo_trim_t *ldo_trim)
{
    ldo_trim->LDO_CAL_TRIM = read_reg8(0x1706ea) & 0x3f;
    ldo_trim->LDO_RXTXHF_TRIM = read_reg8(0x1706ec) & 0x3f;
    ldo_trim->LDO_RXTXLF_TRIM = ((read_reg8(0x1706ed) & 0x0f) << 2) + ((read_reg8(0x1706ec) & 0xc0) >> 6);
    ldo_trim->LDO_PLL_TRIM = read_reg8(0x1706ee) & 0x3f;
    ldo_trim->LDO_VCO_TRIM = ((read_reg8(0x1706ef) & 0x0f) << 2) + ((read_reg8(0x1706ee) & 0xc0) >> 6);
}

/**
 *  @brief        This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_ // ble use
void rf_cs_set_ldo_trim_val(rf_ldo_trim_t ldo_trim)
{
    write_reg8(0x1706e2 ,(ldo_trim.LDO_CAL_TRIM << 1) | 0x01);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,(ldo_trim.LDO_RXTXHF_TRIM << 2) | 0x03);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS
    write_reg8(0x1706e5 , ldo_trim.LDO_RXTXLF_TRIM);
    write_reg8(0x1706e6 ,(ldo_trim.LDO_PLL_TRIM << 2) | 0x03);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS
    write_reg8(0x1706e7 , ldo_trim.LDO_VCO_TRIM);
}

/**
 *  @brief        This function is mainly used to get hpmc Calibration-related values.
 *  @param[in]    none
 *  @return         Returns the hpmc_gain value
*/
_attribute_ram_code_sec_noinline_ unsigned short rf_cs_get_hpmc_cal_val()
{
    unsigned short cali;
    unsigned short hpmc_gain;
    cali = read_reg16(0x1706fe);
    hpmc_gain = (cali<<1)& 0x0ffe;
    return hpmc_gain;
}

/**
 *  @brief        This function is mainly used to set hpmc Calibration-related values.
 *  @param[in]  hpmc_gain  - hpmc Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_sec_noinline_ void rf_cs_set_hpmc_cal_val(unsigned short hpmc_gain)
{
    //The calibration value of hpmc is different at different frequency points,
    //So you need to reset it every time you switch channels.

    unsigned short tmp = read_reg16(0x1706f6);
    tmp = (tmp & 0xf001) | hpmc_gain | 0x0001;    //bit<1:11> 1111 0000 0000 0001    //HPMC_BYPASS
    write_reg16(0x1706f6,tmp);
}

/**
 *  @brief        This function is mainly used to get LDO Calibration-related values.
 *  @param[in]    dcoc_cal   - dcoc calibration value address pointer
 *  @return         none
*/
_attribute_ram_code_ // ble use
void rf_cs_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal)
{
    dcoc_cal->DCOC_IDAC = read_reg8(0x1706d8) & 0x3f;//DCOC_IDAC 0xd8[5:0]
    dcoc_cal->DCOC_QDAC = read_reg8(0x1706da) & 0x3f;//DCOC_QDAC 0xda[5:0]
    dcoc_cal->DCOC_IADC_OFFSET = read_reg8(0x1706dc) & 0x7f;//DCOC_IADC_OFFSET 0xdc[6:0]
    dcoc_cal->DCOC_QADC_OFFSET = (read_reg8(0x1706dc) & 0x80) >> 7 |(read_reg8(0x1706dd) & 0x3f) << 1;//DCOC_QADC_OFFSET 0xdc[7] 0xdd[5:0]
}


/**
 *  @brief        This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_//ble use
void rf_cs_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal)
{
    write_reg8(0x1706d0,(dcoc_cal.DCOC_IDAC << 1) | 0x01);            //DCOC_BYPASS_DAC
    write_reg8(0x1706d0,read_reg8(0x1706d0)|((dcoc_cal.DCOC_QDAC&0x01) << 7));
    write_reg8(0x1706d1,((dcoc_cal.DCOC_QDAC)&0x3e) >> 1);
    write_reg8(0x1706ce,(dcoc_cal.DCOC_IADC_OFFSET << 1) | 0x01);    //DCOC_BYPASS_ADC
    write_reg8(0x1706cf,dcoc_cal.DCOC_QADC_OFFSET);
}

/**
 *  @brief        This function is mainly used to get rccal Calibration-related values.
 *  @param[in]    rccal_cal   - rccal calibration value address pointer
 *  @return         none
*/
_attribute_ram_code_//ble use
void rf_cs_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal)
{
    rccal_cal->RCCAL_CODE = read_reg8(0x1706ca)&0x3f;
    rccal_cal->CBPF_CCODE_L = read_reg8(0x1706ca)&0xc0 >> 6;
    rccal_cal->CBPF_CCODE_H = read_reg8(0x1706cb)&0x1f;
}

/**
 *  @brief        This function is mainly used to set rccal Calibration-related values.
 *  @param[in]    rccal_cal    - rccal Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_ //ble use
void rf_cs_set_rccal_cal_val(rf_rccal_cal_t rccal_cal)
{
#if 1 //check by zhiwei & lijing, for open CBPF
    write_reg8(0x1706c6,(rccal_cal.RCCAL_CODE << 1) & 0xfe);//open CBPF
    write_reg8(0x1706c6,(rccal_cal.CBPF_CCODE_L & 0x01 ) << 7 | (read_reg8(0x1706c6)&(~BIT(6))));//open CBPF
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_L & 0x06) >> 1 | read_reg8(0x1706c7));
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_H << 2 | (read_reg8(0x1706c7)|BIT(6))));//RCCAL_DBG1_1 --> COMP_POL
#else
    write_reg8(0x1706c6,(rccal_cal.RCCAL_CODE));
    write_reg8(0x1706c6,(rccal_cal.CBPF_CCODE_L & 0x01 ) << 7 | (read_reg8(0x1706c6)|BIT(6)));//CBPF_CCODE_BYPASS
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_L & 0x02) >> 1 | read_reg8(0x1706c7));
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_H << 1 | (read_reg8(0x1706c7)|BIT(6)) | BIT(7)));//RCCAL BYPASS

#endif
}

/**
 * @brief        This function is mainly used to get the calibration value of the rx state that needs to be
 *                 recorded in the cs function.
 * @param[out]    rx_cali    -    Pointer to a structure that stores the value associated with the rx calibration.
 * @return        none.
 * @note        This function is usually called after a package has been received.
 */
_attribute_ram_code_ //ble use
void rf_cs_get_rx_cali_vlue(rf_cs_rx_cali_t *rx_cali)
{
    rf_cs_get_ldo_trim_val(&rx_cali->ldo_trim);
#if !SW_DCOC_EN
    rf_cs_get_dcoc_cal_val(&rx_cali->dcoc_cal);
#endif
    rf_cs_get_rccal_cal_val(&rx_cali->rccal_cal);
}

/**
 * @brief        This function is mainly used to get the calibration value of the tx state that needs to be
 *                 recorded in the cs function.
 * @param[out]    rx_cali    -    Pointer to a structure that stores the value associated with the tx calibration.
 * @return        none.
 * @note        This function is usually called after a package has been sent.
 */
_attribute_ram_code_ //ble use
void rf_cs_get_tx_cali_vlue(rf_cs_tx_cali_t *tx_cali)
{
    rf_cs_get_ldo_trim_val(&tx_cali->ldo_trim);
    tx_cali->tx_hpmc = rf_cs_get_hpmc_cal_val();
}

/**
 * @brief        This function is mainly used to enable LNA.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_lna_pup(void)
{
    write_reg8(0x17077a,read_reg8(0x17077a)|BIT(0));//RX_LNA_PUP
    write_reg8(0x170778,read_reg8(0x170778)|BIT(0));//RX_LNA_PUP_OW
}

/**
 * @brief        This function is mainly used to write the calibration value obtained through the rf_cs_get_rx_cali_vlue
 *                 function to the corresponding register.
 * @param[in]    rx_cali        -    rx calibration value obtained by the rf_cs_get_rx_cali_vlue function.
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_cs_set_rx_cali_vlue(rf_cs_rx_cali_t rx_cali)
{
    rf_cs_dis_fcal_trim();
    rf_cs_set_ldo_trim_val(rx_cali.ldo_trim);
    rf_cs_dis_ldo_trim();
#if !SW_DCOC_EN
    rf_cs_set_dcoc_cal_val(rx_cali.dcoc_cal);
#endif
    rf_cs_dis_rccal_trim();
    rf_cs_set_rccal_cal_val(rx_cali.rccal_cal);
    rf_cs_dis_dcoc_trim();
    rf_lna_pup();
}

/**
 * @brief        This function is used to write the tx calibration value obtained by rf_cs_get_tx_cali_vlue to the
 *                 corresponding register.
 * @param[in]    tx_cali        -    tx calibration value obtained by the rf_cs_get_tx_cali_vlue function.
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_cs_set_tx_cali_vlue(rf_cs_tx_cali_t tx_cali)
{
    rf_cs_dis_fcal_trim();
    rf_cs_set_ldo_trim_val(tx_cali.ldo_trim);
    rf_cs_set_hpmc_cal_val(tx_cali.tx_hpmc);
    rf_cs_dis_hpmc_trim();
}


/**
 * @brief        This function is mainly used to enable the rx-related trim functions that are bypassed during channel sounding.
 * @param[in]    mix_en : Used to control whether the digital IF is continuous or not.1:Maintaining continuity;0:No longer continuous.
 * @return        none.
 */
_attribute_ram_code_ // ble use
void rf_cs_restore_cali_auto_run(unsigned char phase_en)
{
    write_reg8(0x170681,read_reg8(0x170681)&0xf3);        //FCAL_DEBUG_RUN_OW //LDOT_DEBUG_RUN_OW

#if !SW_DCOC_EN
    write_reg8(0x170680,read_reg8(0x170680)&0xea);      //RCCAL_RUN_OW//RXDCOC_RUN_OW//DSM_RUN_OW
#else
    write_reg8(0x170680,read_reg8(0x170680)&0xfa);      //RCCAL_RUN_OW//RXDCOC_RUN_OW//DSM_RUN_OW
#endif

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);        //LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);        //LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);        //LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS


    write_reg8(0x1706ce,read_reg8(0x1706ce)&0xfe);        //DCOC_BYPASS_ADC

    write_reg8(0x1706d0,read_reg8(0x1706d0)&0xfe);        //DCOC_BYPASS_DAC

    write_reg8(0x1706c6,(read_reg8(0x1706c6)&0xbf));    //CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(7))));    //RCCAL_DBG1_0-->BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0)));    //RX_LNA_PUP_OW

//    write_reg8(0x170760,read_reg8(0x170760)&0xf5);        //LDO_RXTXHF_PUP_OW//LDO_RXTXHF_PUP_OW
    write_reg8(0x170760,read_reg8(0x170760)&0xf9);        //LDO_RXTXHF_PUP_OW//LDO_RXTXHF_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&0xe7);        //LDO_PLL_FC_OW//LDO_VCO_FC_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1)));    //XO_EN_CLK_REF_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6)));    //LO_PUP_VLO_FBK_OW


    if(phase_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW

        write_reg8(0x170450,read_reg8(0x170450)&(~BIT(5)));    //GFSK_AUTO
        write_reg8(0x170453,read_reg8(0x170453)|(BIT(1)));    //FREQ_COMP_EN
        write_reg8(0x170452,read_reg8(0x170452)|(BIT(5)));    //GFSK_EN

        //VCO
        write_reg8(0x1706f6,read_reg8(0x1706f6)|BIT(0));    //HPMC_BYPASS

        write_reg8(0x170680,read_reg8(0x170680)|(BIT(5)));    //HPMC_RUN_OW
        write_reg8(0x170760,read_reg8(0x170760)|(BIT(4)));    //LDO_VCO_PUP_OW
        write_reg8(0x17078c,read_reg8(0x17078c)|(BIT(0)));    //VCO_PUP_OW

        write_reg8(0x170788,0xc0);                            //PD_DIVN_FCAL_OW_CTRL 0x00->0xc0
                                                            //<0>:PD_PUP_OW
                                                            //<1>:PD_EN_PD_DRV_OW
                                                            //<2>:PD_EN_FCAL_BIAS_OW
                                                            //<3>:FCAL_PUP_OW
                                                            //<4>:FCAL_SET_OW
                                                            //<5>:FCAL_RUN_OW
                                                            //<6>:DIVN_PUP_OW       default 0 -> 1 open divn_pup overwrite
                                                            //<7>:DIVN_OPENLOOP_OW default 0 -> 1 open divn_openloop overwrite

        write_reg8(0x170760,read_reg8(0x170760)|(BIT(0)));    //LDO_LV_PUP_OW
        write_reg8(0x170764,read_reg8(0x170764)|(BIT(0)));    //BG_PUP_OW

//        write_reg8(0x170790,read_reg8(0x170790)|(BIT(6)));    //LO_PUP_VLO_TXFSK_OW

        write_reg8(0x17078c,read_reg8(0x17078c)|(BIT(2)));    //LO_PUP_VLO_RX_OW

    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW

        write_reg8(0x170450,read_reg8(0x170450)|(BIT(5)));    //GFSK_AUTO
        write_reg8(0x170453,read_reg8(0x170453)&(~BIT(1)));    //FREQ_COMP_EN
        write_reg8(0x170452,read_reg8(0x170452)&(~BIT(5)));    //GFSK_EN

        //VCO
        write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);        //HPMC_BYPASS

        write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5)));    //HPMC_RUN_OW
        write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4)));    //LDO_VCO_PUP_OW
        write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0)));    //VCO_PUP_OW

        write_reg8(0x170788,0x00);//PD_PUP_OW//PD_EN_PD_DRV_OW//PD_EN_FCAL_BIAS_OW//FCAL_PUP_OW//FCAL_SET_OW//FCAL_RUN_OW//DIVN_PUP_OW//DIVN_OPENLOOP_OW

        write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0)));    //LDO_LV_PUP_OW
        write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0)));    //BG_PUP_OW

//        write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6)));    //LO_PUP_VLO_TXFSK_OW

        write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2)));    //LO_PUP_VLO_RX_OW
    }

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3)));    //TX_HPM_CAL_EN_OW

//    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7)));    //LO_PUP_VLO_TXFSKDRV_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3)));    //LO_PUP_VLO_RXDRV_OW
}

/**
 * @brief        This function is used to enable or disable the corresponding sequence of shuttle in channel sounding mode; usually call
 *                 this function before entering mode1/mode2 and pass the parameter RF_CS_SETTLE_SEQ_ON to enable the corresponding sequence;
 *                 and call the parameter RF_CS_SETTLE_SEQ_OFF to disable the sequence after ending channel sounding.
 * @param[in]    on_off : Used to control whether to enable settle sequence in channel sounding mode.RF_CS_SETTLE_SEQ_OFF:off,RF_CS_SETTLE_SEQ_ON:on
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_cs_settle_sequence_mode(rf_cs_settle_seq_mode_e on_off)
{
    if (on_off == RF_CS_SETTLE_SEQ_ON)
    {
        //close hpmc and ldo trim,close hpmc(53us), ldotrim(4.5us),save 58us
        //Default settle time:108.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x17068a,0x00);    //sub-sequence1 start time:0
        write_reg8(0x17068b,0x08);    //sub-sequence2 start time:8us
        write_reg8(0x17068c,0x30);    //sub-sequence3 start time:48us
        write_reg8(0x17068d,0x31);    //sub-sequence4 start time:48.5us
        write_reg8(0x17068e,0x33);    //sub-sequence5 start time:51us
        write_reg8(0x17068f,0x30);    //sub-sequence6 start time:48us

        //RX: rx_ldo_trim (4.5us), rx_dcoc(40us)
        //RX Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x170690,0x00);    //sub-sequence1 start time:0us
        write_reg8(0x170691,0x09);    //sub-sequence2 start time:9us
        write_reg8(0x170692,0x09);    //sub-sequence3 start time:9us
        write_reg8(0x170693,0x1b);    //sub-sequence4 start time:27us
        write_reg8(0x170694,0x2d);    //sub-sequence5 start time:45us
        write_reg8(0x170695,0x2d);    //sub-sequence6 start time:45us
    }
    else if(on_off == RF_CS_SETTLE_SEQ_OFF)
    {
        //Default settle time:108.5us
        write_reg8(0x17068a,0x00);    //sub-sequence1 start time:0
        write_reg8(0x17068b,0x0d);    //sub-sequence2 start time:13us
        write_reg8(0x17068c,0x6a);    //sub-sequence3 start time:106us
        write_reg8(0x17068d,0x6b);    //sub-sequence4 start time:107us
        write_reg8(0x17068e,0x6e);    //sub-sequence5 start time:110us
        write_reg8(0x17068f,0x6a);    //sub-sequence6 start time:106us

        //RX Default settle time:85us
        write_reg8(0x170690,0x00);    //sub-sequence1 start time:0us
        write_reg8(0x170691,0x0d);    //sub-sequence2 start time:13us
        write_reg8(0x170692,0x0d);    //sub-sequence3 start time:13us
        write_reg8(0x170693,0x27);    //sub-sequence4 start time:43us
        write_reg8(0x170694,0x52);    //sub-sequence5 start time:82us
        write_reg8(0x170695,0x52);    //sub-sequence6 start time:82us
    }
}

void rf_cs_send_tone_pkt(void* addr,  unsigned int tick, unsigned int time_tone_to_pkt)
{
    reg_rf_tx_frac_ctrl0 |= (FLD_RF_TX_MI_SWITCH_TONE_EN|FLD_RF_TX_HAFM_RAMP_DOWN_EN|FLD_RF_TX_RAMP_DOWN_TONE_EN);
//    write_reg8(0x17013e,((read_reg8(0x17013e))&(~BIT(5))|0xc0));
    write_reg8(0x17013e,(read_reg8(0x17013e)|0xe0));
    write_reg8(0x17013f,time_tone_to_pkt);
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x85;//stx

}

void rf_cs_send_pkt_tone(void* addr,  unsigned int tick, unsigned int time_pkt_to_tone)
{
    reg_rf_tx_frac_ctrl0 |= (FLD_RF_TX_MI_SWITCH_TONE_EN|FLD_RF_TX_HAFM_RAMP_DOWN_EN|FLD_RF_TX_RAMP_DOWN_TONE_EN);
    write_reg8(0x17013e,(read_reg8(0x17013e)&(~BIT(5)))|0xc0);
    write_reg8(0x17013f,time_pkt_to_tone);
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x85;//stx

}

_attribute_ram_code_ //ble use
void rf_cs_txant_switch_mode(rf_cs_tx_ant_mode_e mode)
{
    reg_rf_rxchn = (reg_rf_rxchn&0x7c)|mode;
}

void rf_cs_rxant_switch_on(void)
{
    reg_rf_rxchn = (reg_rf_rxchn|BIT(2));
}

void rf_cs_rxant_switch_off(void)
{
    reg_rf_rxchn = (reg_rf_rxchn&(~BIT(2)));
}
_attribute_ram_code_ //ble use
void rf_cs_ant_switch_auto(void)
{
    reg_rf_man_ant_slot &= (~FLD_RF_ANT_SEL_MAN_EN);
}
_attribute_ram_code_ //ble use
void rf_cs_ant_switch_manual(void)
{
    reg_rf_man_ant_slot |= FLD_RF_ANT_SEL_MAN_EN;
}

/**
 * @brief        This function is mainly used to set the antenna switching mode. Vulture support three different
 *                 table lookup sequences.The setting here is just the order of the table lookup, and the content
 *                 in the table is the number of the antenna to be switched to.The switching sequence of the antenna
 *                 needs to be determined by the combination of the table look-up sequence and the antenna number in
 *                 the table,so this function is usually used together with the rf_aoa_aod_ant_lut function.
 * @param[in]    pattern     - Enumeration of several different look-up table order modes.Refer to the corresponding
 *                               enumeration annotation for the meaning of the mode.
 * @return        none.
 */
_attribute_ram_code_ //ble use
void rf_aoa_aod_ant_pattern(rf_ant_pattern_e pattern)
{
    reg_rf_man_ant_slot = ((reg_rf_man_ant_slot&(~FLD_RF_ANT_PAT))|pattern);
}

/**
 * @brief        This function is mainly used to set the number of antennas enabled by the multi-antenna board in the
 *                 AOA/AOD function;the vulture series chips currently support up to 8 antennas for switching.By default,
 *                 it is set to 8 antennas. After configuring the RF-related settings, you can set the number of enabled
 *                 antennas, and this setting needs to be completed before sending and receiving packets.
 * @param[in]    ant_num     - The number of antennas, the value ranges from 1 to 8.
 * @return        none.
 */
_attribute_ram_code_//ble use
void rf_aoa_aod_set_ant_num(unsigned char ant_num)
{
    ant_num = ((ant_num - 1) & 0x01f);
    reg_rf_ant_num = ((reg_rf_ant_num&(~FLD_RF_ANT_NUM)) | ant_num);
}


/**
 * @brief        This function is mainly used to initialize the parameters related to AOA/AOD antennas, including the
 *                 number of antennas, the pins for controlling the antennas,the look-up mode of antenna switching, and
 *                 the content of the antenna switching sequence table.
 * @param[in]    ant_num            - The number of antennas, the value ranges from 1 to 8.
 * @param[in]    ant_pin_config:    - Control antenna pin selection and configuration.The parameter setting needs to be
 *                                   set according to the number and position of the control antenna.For example,if you
 *                                   need to control four antennas, it is best to use Antsel0 and Antsel2.
 * @param[in]    pattern            - Enumeration of several different look-up table order modes.
 * @param[in]    dat             - The antenna value written into the antenna switching sequence table ranges from 0 to 7.
 * @return        none.
 */
void rf_aoa_aod_ant_init(unsigned char num,rf_ant_pin_sel_t * ant_pin_config,rf_ant_pattern_e pattern,unsigned int dat)
{
    rf_aoa_aod_set_ant_num(num);
    if(((ant_pin_config->antsel0_pin)!=(ant_pin_config->antsel1_pin))&&((ant_pin_config->antsel0_pin)!=(ant_pin_config->antsel2_pin))&&((ant_pin_config->antsel1_pin)!=(ant_pin_config->antsel2_pin)))
    {
        gpio_set_mux_function(ant_pin_config->antsel0_pin,ATSEL_0);
        gpio_set_mux_function(ant_pin_config->antsel1_pin,ATSEL_1);
        gpio_set_mux_function(ant_pin_config->antsel2_pin,ATSEL_2);

    }
     rf_aoa_aod_ant_pattern(pattern);

     ble_rf_cs_ant_lut(dat);
}

/**
 * @brief        This function is used to calculate the number of IQ groups in the received AOA/AOD packet.
 * @param[in]    p                - Received packet address pointer.
 * @return        Returns the number of groups of iq in the package.
 */
unsigned int rf_aoa_aod_iq_group_number(unsigned char *p)
{
    unsigned char y=0;
    y = ((g_sample_interval >> 8) & 0xff);

    if(SAMPLE_NORMAL_INTERVAL == g_sample_interval)
    {
        if((p[6]&0xc0) == 0x40)
        {
            y = 8;
        }
    }
    //CTE time = CTEinfo_CTETime*8us
    //IQ sample_time = (CTEinfo_CTETime*8us - (Guard period + Reference period)) / ((interval time/4) + Reference period)
    return ((((p[6]&0x1f)<<3) - 12)/(y>>2) + 8);
}

/**
 * @brief        This function is mainly used to obtain the CRC value in the AOA/AOD packet.
 * @param[in]    p                - Received packet address pointer.
 * @return        The return value is the rssi value in headerinformation.
 */
signed char rf_aoa_aod_get_pkt_rssi(unsigned char *p)
{

    unsigned char x=0;
    x = ((g_iq_data_len >> 8) & 0xff);

    return (p[rf_aoa_aod_iq_group_number(p)*x + p[5] + 16]-110);
}
#endif

/**
 * @brief          This function is mainly used to set the energy when sending a single carrier.
 * @param[in]    level        - The slice corresponding to the energy value.
 * @return         none.
 */
_attribute_ram_code_ void rf_cs_set_power_level_singletone(rf_power_level_e level)
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
////////////////////////////////rf_cs.h end/////////////////////////////////

#if (HADM_PHASE_CONTINUITY)
#if FAST_SETTLE
extern _attribute_data_retention_sec_ rf_fast_settle_t *g_fast_settle_cal_val_ptr;
#endif
_attribute_data_retention_ rf_cs_tx_cali_t tx_cs_cali;
_attribute_data_retention_ rf_cs_rx_cali_t rx_cs_cali;
_attribute_data_retention_ unsigned char cs_phase_continuity_flag = 0;

unsigned char  ble_customer_tx_packet[280] __attribute__ ((aligned (4))) =
        {3,0,0,0,0xaa,0xaa,0x8e,0x89,0xbe,0xd6,0xaa,0x55,0x55,0xaa,0x00,0x00};

typedef struct __attribute__((packed))
{
    unsigned int dma_len;
    unsigned char header;
    unsigned char payload_len;
}ble_packet_header_t;


/**
 * @brief       This function is mainly used to get tx and rx calibration value by running a self-made stx and srx.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_settle_cali_init(void)
{
//    rf_mode_init();
//    rf_set_ble_1M_NO_PN_mode();

    reg_rf_ll_cmd = 0x80;//STOP_RF_STATE_MACHINE;
    rf_set_tx_rx_off ();
    rf_clr_irq_status(0xffff);//CLEAR_ALL_RFIRQ_STATUS;

//    rf_tx_settle_us(110);//adjust TX settle time
//    rf_set_rx_settle_time(85);
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);
    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_set_tx_dma(0,128);
    ble_packet_header_t *ptr = (ble_packet_header_t*)ble_customer_tx_packet;
    ptr->dma_len = rf_tx_packet_dma_len(5);
//  for(unsigned char i=0;i<40;i++)
    {
        rf_set_chn(40);
        rf_start_fsm(FSM_STX,(void*)&ble_customer_tx_packet,stimer_get_tick());
        while(!(rf_get_irq_status(FLD_RF_IRQ_TR_TURNAROUND)));
        rf_clr_irq_status(FLD_RF_IRQ_TX|FLD_RF_IRQ_TR_TURNAROUND);

        reg_rf_ll_cmd = 0x80;
        rf_clr_irq_status(0xffff);
//      fast_settle.cal_tbl[i] = rf_get_hpmc_cal_val();
    }

    //48M : 4.46us
    rf_cs_get_tx_cali_vlue(&tx_cs_cali);


    rf_start_fsm(FSM_SRX,NULL,stimer_get_tick());
    delay_us(85);//Wait for the rx packetization action to complete
//  rf_get_dcoc_cal_val(&fast_settle.dcoc_cal);

    reg_rf_ll_cmd = 0x80;
//
//    rf_set_tx_rx_off ();
//    rf_clr_irq_status(0xffff);
//    reg_rf_ll_cmd = 0x80;
//  rf_get_ldo_trim_val(&fast_settle.ldo_trim);

    //48M : 7.17us
    rf_cs_get_rx_cali_vlue(&rx_cs_cali);

//  reg_rf_radio_txrx_dbg1_0 |= FLD_RF_AGC_DISABLE;

}


/**
 * @brief       This function is mainly used to set the sequence related to Fast Settle in cs.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_phase_continuity_en(void)//CCLK_96M, consume 49us
{
    #if (FAST_SETTLE)
        //rf_rx_fast_settle_dis
        #if (!SW_DCOC_EN)
            reg_rf_dcoc_bypass_dac_0 &= ~FLD_RF_DCOC_BYPASS_DAC;                                //dcoc bypass dac disable
            reg_rf_dcoc_bypass_adc_0 &= ~FLD_RF_DCOC_BYPASS_ADC;                                //dcoc offset bypass adc disable
        #endif
        reg_rf_ldot_dbg1 &= ~FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass disable
        reg_rf_ldot_dbg2_0 &= ~(FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass disable
        reg_rf_ldot_dbg3_0 &= ~(FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass disable.
        reg_rf_tx_frac_ctrl0 &= ~FLD_RF_FCAL_STL_DCAP_RX_EN;                                    //RX FCAL settle disable
        reg_rf_rccal_dbg1_0 &= ~FLD_RF_CBPF_CCODE_BYPASS;                                       //CBPF_CCODE_BYPASS disable
        reg_rf_burst_cfg_txrx_1 &= ~FLD_RF_RX_TIM_SRQ_SEL_TESQ;                                 //rx fast settle disable

        //rf_tx_fast_settle_dis
        reg_rf_hpmc_debug_0 &= ~FLD_RF_HPMC_BYPASS;                                             //hpmc bypass disable
        reg_rf_ldot_dbg1 &= ~FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass disable
        reg_rf_ldot_dbg2_0 &= ~(FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass disable
        reg_rf_ldot_dbg3_0 &= ~(FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass disable.
        reg_rf_frac_ctrl0 &= ~FLD_RF_FCAL_STL_DCAP_TX_EN;                                       //TX FCAL settle disable
        reg_rf_burst_cfg_txrx_1 &= ~FLD_RF_TX_TIM_SRQ_SEL_TESQ;                                 //tx fast settle disable
    #endif

    rf_cs_set_rx_cali_vlue(rx_cs_cali);
    rf_cs_set_tx_cali_vlue(tx_cs_cali);

    rf_cs_set_phase_continuous();

    rf_cs_settle_sequence_mode(RF_CS_SETTLE_SEQ_ON);

    cs_phase_continuity_flag = 1;
}

_attribute_ram_code_ void ble_rf_cs_phase_continuity_dis(unsigned char phase_en)
{
    rf_cs_settle_sequence_mode(RF_CS_SETTLE_SEQ_OFF);
    rf_cs_restore_cali_auto_run(phase_en);

#if (FAST_SETTLE)
    #if !SW_DCOC_EN
        rf_cs_set_dcoc_cal_val(g_fast_settle_cal_val_ptr->dcoc_cal);
    #endif
    rf_cs_set_ldo_trim_val(g_fast_settle_cal_val_ptr->ldo_trim);
    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);
    rf_rx_fast_settle_en();
    rf_tx_fast_settle_en();
#endif

    cs_phase_continuity_flag = 0;
}

_attribute_ram_code_ void ble_rf_channel_sounding_init(void)
{
    reg_rf_mode_ctrl0 |= FLD_RF_INFO_EXTENSION;
    reg_dma_ctr3(1) = ((reg_dma_ctr3(1) & 0xf8) | RF_QWORLD_WIDTH);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | RF_QWORLD_WIDTH);
    write_reg8(0x170033,read_reg8(0x170033)|BIT(6));  //iq_sample_align
    write_reg8(0x170641,read_reg8(0x170641)|BIT(7));  //txc_align_tx_en
    write_reg8(0x170798,read_reg8(0x170798)|BIT(0));  //TX_FRAC_TIME_MUX

//    if((g_chip_version == CHIP_VERSION_A0)||(g_chip_version == CHIP_VERSION_A1)){
//        write_reg8(0x170752,read_reg8(0x170752)|0x03);      //FCAL_BIAS
//    }

    write_reg8(0x170030, 0x3e); //enable tx timestamp


    //ble_rf_tx_channel_sounding_mode_en();
    BM_CLR(reg_rf_tx_mode1,FLD_RF_CRC_EN);
    BM_CLR(reg_rf_tx_mode2,FLD_RF_V_PN_EN);
    BM_SET(reg_rf_tx_mode2,FLD_RF_R_CUSTOM_MADE);
    BM_SET_MASK_VAL(reg_rf_preamble_trail, FLD_RF_TRAILER_LEN, MV(FLD_RF_TRAILER_LEN, 0));

    //ble_rf_rx_channel_sounding_mode_en(1, IQ_20_BIT_MODE); //sample rate 4MHz, IQ 20 bit
    //sample_interval_time: (1 + 1)*0.125us ---> 0.25us ---> 4MHz
    unsigned char interval = 1;
    rf_iq_data_mode_e suppmode = IQ_20_BIT_MODE;
    reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & (~FLD_RF_IQ_SAMP_INTERVAL)) | (interval << 4));//The max sample rate is 4Mhz.
    reg_rf_sof_offset = ((reg_rf_sof_offset & (~FLD_RF_SUPP_MODE)) | ((suppmode&0x07) << 4));
    reg_rf_mode_ctrl0 |= FLD_RF_IQ_SAMP_EN;

    reg_rf_lnm_pa_ow_ctrl_val &=~FLD_RF_PA_RAMP_TSEQ_OR_TX_ON;
    #if(CS_EBQ_TEST)
        write_reg8(0x170624, 0x4b); //PA_RAMP_MODE 1000ns fix EBQ mode2 rf timing issue -- yuexin sync with haili and xuqiang
    #else
        write_reg8(0x170624, 0x49); //mode1 fine rtt need this setting, otherwise range jitter.
    #endif
}

_attribute_ram_code_ void ble_rf_channel_sounding_deinit(void)
{
    //ble_rf_tx_channel_sounding_mode_dis();
    BM_SET(reg_rf_tx_mode1,FLD_RF_CRC_EN);
    BM_SET(reg_rf_tx_mode2,FLD_RF_V_PN_EN);
    BM_CLR(reg_rf_tx_mode2,FLD_RF_R_CUSTOM_MADE);
    BM_SET_MASK_VAL(reg_rf_preamble_trail, FLD_RF_TRAILER_LEN, MV(FLD_RF_TRAILER_LEN, 2));

    reg_rf_mode_ctrl0 &= (~FLD_RF_IQ_SAMP_EN);//ble_rf_rx_channel_sounding_mode_dis();

    reg_rf_mode_ctrl0 &= (~FLD_RF_INFO_EXTENSION);
    reg_dma_ctr3(1) = ((reg_dma_ctr3(1) & 0xf8) | RF_WORLD_WIDTH);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | RF_WORLD_WIDTH);

    reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;

    write_reg8(0x170030, 0x36); //disable tx timestamp

    rf_agc_enable();
    reg_rf_lnm_pa_ow_ctrl_val |=FLD_RF_PA_RAMP_TSEQ_OR_TX_ON;
    write_reg8(0x170624, 0x4b);
}

/**
 * @brief       This function is mainly used to initialize some parameter settings of channel sounding IQ sample.
 * @param[in]   sample_num  - Number of groups to sample IQ data.Value range 0x01~0xffff.
 * @param[in]   start_point - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                            at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                            starts at (start_point + 1) * 0.125us after sync.
 *                            Value range 0x00~0xff.
 * @param[in]   sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_channel_sounding_iq_sample_config(unsigned short sample_num, unsigned char start_point, rf_cs_iq_sample_mode_e sample_mode)
{
    reg_rf_iq_samp_num = sample_num;
    reg_rf_iq_samp_start = start_point;
    if(sample_mode == RF_CS_IQ_SAMPLE_SYNC_MODE)
    {
        reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;
    }
    else
    {
        reg_rf_rxlatf &= (~FLD_RF_R_IQ_SAMP_MODE);
    }
}

_attribute_ram_code_ void ble_rf_set_manual_tx_mode(void)
{
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
    reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}


_attribute_ram_code_ void ble_rf_set_tx_modulation_index(rf_mi_value_e mi_value)//only support RF_MI_P0p00 and RF_MI_P0p50
{

    unsigned char modulation_index_low;
    unsigned char kvm_trim = 0;

    if(mi_value == RF_MI_P0p00){
        modulation_index_low = 0;
    }
    else if(mi_value == RF_MI_P0p50){
        modulation_index_low = 64;
    }
    else{
        return;
    }

    if(reg_rf_mode_cfg_tx1_0 & 0x01)
    {
        kvm_trim = 1;
    }

    reg_rf_radio_mode_cfg_rx2_0 = modulation_index_low;
    //reg_rf_radio_mode_cfg_rx2_1 default is 0, when mi_value = RF_MI_P0p00 or RF_MI_P0p00 , reg_rf_radio_mode_cfg_rx2_1 need set to 0.
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));

}

/**
 * @brief       This function serves to set rf channel for CS.The actual channel set by this function is 2402 + chn.
 * @param[in]   chn   - That you want to set the channel as 2402 + chn.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_set_cs_channel(signed char chn)
{
    rf_set_chn(chn + 2);
}



/**
 * @brief       This function is mainly used for agc auto run.
 * @return      none.
 * @note        Call this function to enable agc auto tuning if you want to receive different energy packets correctly
 *              after calling ble_rf_agc_disable to disable agc auto tuning.
 */




/**
 * @brief       This function is used to set the antenna switching sequence table. The content in the table is the
 *              antenna sequence number that needs to be switched to when the position is found by the look-up table.
 *              Since determining the antenna switching sequence needs to determine the order of the table lookup and
 *              the setting of the table content, this function is usually used in conjunction with the function
 *              rf_aoa_aod_ant_pattern.
 * @param[in]   dat      - Antenna serial number written into the antenna switching sequence table.The value in the table
 *                       corresponds to the antenna number that needs to be switched to when it is found in the table.The
 *                       value range is 0 to 7.
 * @return      none.
 */
_attribute_ram_code_
void ble_rf_cs_ant_lut(unsigned int dat)
{
    // tercel 6bits, jaguar 4bits
    unsigned int dat1 = 0;
    for(int i = 0; i < 5; i++){
        dat1 |= (((dat >> 4*i)&0x0F) << (6*i));
    }
    write_reg32(0x1700e0,dat1);
}
#endif //HADM_PHASE_CONTINUITY

/**
 * @brief     This function performs to set RF Access Code Threshold.
 * @param[in] threshold   cans be 0-32bits
 * @return    none.
 */
_attribute_ram_code_ void ble_rf_set_accessCodeThreshold(u8 threshold)
{
    write_reg8(0x17044e, threshold);
}
