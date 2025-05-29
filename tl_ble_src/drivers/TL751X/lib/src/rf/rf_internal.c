/********************************************************************************************************
 * @file    rf_internal.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/rf/rf_internal.h"


#if(0)

/**
 * @brief     This function serves to  set pri_1M  mode of RF.
 * @return    none.
 * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_pri_1M_mode(void)
{
    g_rfmode = RF_MODE_PRIVATE_1M;
}

/**
 * @brief     This function serves to  set pri_2M  mode of RF.
 * @return    none.
 * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_pri_2M_mode(void)
{
    g_rfmode = RF_MODE_PRIVATE_2M;
}

/**
 * @brief       This function serves to set pin for RFFE of RF.
 * @param[in]   tx_pin   - select pin as rffe to send.
 * @param[in]   rx_pin   - select pin as rffe to receive.
 * @return      none.
 * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_rffe_pin(rf_pa_tx_pin_e tx_pin, rf_lna_rx_pin_e rx_pin)
{
    unsigned char val = 0;
    unsigned char mask = 0xff;

    switch(tx_pin)
    {
        case RF_RFFE_TX_PB0:
            val = BIT(0);
            mask = (unsigned char)~(BIT(1)|BIT(0));
            break;

        case RF_RFFE_TX_PB6:
            val = 0;
            mask = (unsigned char)~(BIT(5)|BIT(4));
            break;

        case RF_RFFE_TX_PD7:
            val = BIT(7);
            mask = (unsigned char)~(BIT(7)|BIT(6));
            break;

        case RF_RFFE_TX_PE5:
            val = BIT(2);
            mask = (unsigned char)~(BIT(3)|BIT(2));
            break;

        default:
            val = 0;
            mask = 0xff;
            break;
    }

    reg_gpio_func_mux(tx_pin)=(reg_gpio_func_mux(tx_pin)& mask)|val;

    switch(rx_pin)
    {
        case RF_RFFE_RX_PB1:
            val = BIT(2);
            mask = (unsigned char)~(BIT(3)|BIT(2));
            break;

        case RF_RFFE_RX_PD6:
            val = BIT(5);
            mask = (unsigned char)~(BIT(5)|BIT(4));
            break;

        case RF_RFFE_RX_PE4:
            val = BIT(0);
            mask = (unsigned char)~(BIT(1)|BIT(0));
            break;

        default:
            val = 0;
            mask = 0xff;
            break;
    }
    reg_gpio_func_mux(rx_pin)=(reg_gpio_func_mux(rx_pin)& mask)|val;
    BM_CLR(reg_gpio_func(tx_pin), tx_pin&0xff);
    BM_CLR(reg_gpio_func(rx_pin), rx_pin&0xff);

}

/**
 * @brief       This function serves to set pri sb mode enable.
 * @return      none.
 * @note        TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_private_sb_en(void)
{
    reg_rf_format |= FLD_RF_HEAD_MODE;
}

/**
 * @brief       This function serves to set pri sb mode payload length.
 * @param[in]   pay_len  - In private sb mode packet payload length.
 * @return      none.
 * @note        TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_private_sb_len(int pay_len)
{
    reg_rf_sblen = ((reg_rf_sblen&0x00)|pay_len);
}

/**
 * @brief   This function serve to set the private ack enable,mainly used in prx/ptx.
 * @param[in]   rf_mode     -   Must be one of the private mode.
 * @return      none
 */
void rf_set_pri_tx_ack_en(rf_mode_e rf_mode)
{
    if(rf_mode == RF_MODE_PRIVATE_1M)
        write_reg8(0xd4170004, 0x9a);//1m 9a //enable  ack flag
    else if(rf_mode == RF_MODE_PRIVATE_2M)
        write_reg8(0xd4170004, 0x8a);//2m,8a
}

/**
 * @brief   This function serve to set access code.This function will first get the length of access code from register 0x140805
 *          and then set access code in addr.
 * @param[in]   pipe_id -The number of pipe.0<= pipe_id <=5.
 * @param[in]   acc -The value access code
 * @note        For compatibility with previous versions the access code should be bit transformed by bit_swap();
 *              TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
 void rf_set_pipe_access_code (unsigned int pipe_id, unsigned char *addr)
{

    unsigned char i=0;

     unsigned char acc_len = read_reg8(0xd4170005) & 0x07;
        switch (pipe_id) {
            case 0:
            case 1:
                for(i=0;i<acc_len;i++)
                {
                    write_reg8(reg_rf_access_code_base_pipe0+ i + (pipe_id*8),addr[i]);
                }
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                for(i=0;i<acc_len;i++)
                {
                    write_reg8(reg_rf_access_code_base_pipe0+ i + 8 ,addr[i]);
                }
                write_reg8((reg_rf_access_code_base_pipe2 + (pipe_id-2)), addr[0]);
                break;
            default:
                break;
        }

}

/**
 * @brief   This function serve to initial the ptx setting.
 * @return  none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_ptx_config(void)
{
    write_reg8(0xd4170202, read_reg8(0xd4170202)&0xfe);//md_en
    write_reg8(0xd4170203, read_reg8(0xd4170203)&0xf7);//crc_en
    write_reg8(0xd4170215, 0xd0);//chn tx_manual off
}

/**
 * @brief   This function serve to initial the prx setting.
 * @return  none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_prx_config(void)
{
    write_reg8(0xd4170203, 0x30);//rx timeout off
    write_reg8(0xd4170201, 0x3f);//reset pid
    write_reg8(0xd4170215, 0xc0);//chn tx_manual off
}


/**
 * @brief   This function serves to set RF ptx trigger.
 * @param[in]   addr    -   The address of tx_packet.
 * @param[in]   tick    -   Trigger ptx after (tick-current tick),If the difference is less than 0, trigger immediately.
 * @return  none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_start_ptx  (void* addr,  unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x83;
}

/**
 * @brief   This function serves to set RF prx trigger.
 * @param[in]   tick    -   Trigger prx after (tick-current tick),If the difference is less than 0, trigger immediately.
 * @return  none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_start_prx(unsigned int tick)
{
    write_reg32 (0xd4170228, 0x0fffffff);                   // first timeout.
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    write_reg8(0xd4170200, 0x84);
}

/**
 * @brief   This function serves to judge whether the FIFO is empty.
 * @param pipe_id specify the pipe.
 * @return TX FIFO empty bit.
 *          -#0 TX FIFO NOT empty.
 *          -#1 TX FIFO empty.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
unsigned char rf_is_rx_fifo_empty(unsigned char pipe_id)
{
    return (reg_rf_dma_tx_wptr(pipe_id)) == (reg_rf_dma_tx_rptr(pipe_id));
}

/**
 * @brief   This function to set retransmit and retransmit delay.
 * @param[in]   retry_times - Number of retransmit, 0: retransmit OFF
 * @param[in]   retry_delay - Retransmit delay time.
 * @return      none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_ptx_retry(unsigned char retry_times, unsigned short retry_delay)
{
    retry_times &= 0x0f;
    write_reg8(0xd4170214, retry_times);

    retry_delay &= 0x0fff;
    unsigned short tmp = read_reg16(0xd4170210);
    tmp &= 0xf000;
    tmp |= retry_delay;
    write_reg16(0xd4170210, tmp);
}

/**
 * @brief       This function is used to  set the modulation index of the receiver.
 *              This function is common to all modes,the order of use requirement:configure mode first,
 *              then set the the modulation index,default is 0.5 in drive,both sides need to be consistent
 *              otherwise performance will suffer,if don't specifically request,don't need to call this function.
 * @param[in]   mi_value- the value of modulation_index*100.
 * @return      none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_rx_modulation_index(rf_mi_value_e mi_value)
{
    unsigned char modulation_index_high;
    unsigned char modulation_index_low;
    unsigned char kvm_trim;
    mi_value = (unsigned int)(mi_value * 1.28);

    modulation_index_low = mi_value%256;

    modulation_index_high = (mi_value%512)>>8;
    (reg_rf_modem_rxc_mi_flex_ble_0) = (modulation_index_low);
    (reg_rf_modem_rxc_mi_flex_ble_0) |= (modulation_index_high);
    if((reg_rf_mode_cfg_tx1_0) & 0x01)
    {
        if ((mi_value >= 75)&&(mi_value <= 100))
            kvm_trim = 3;
        else if (mi_value > 100)
            kvm_trim = 7;
        else
            kvm_trim = 1;
    }
    else
    {

        if ((mi_value >= 75)&&(mi_value <= 100))
            kvm_trim = 1;
        else if ((mi_value > 100)&&(mi_value <= 150))
            kvm_trim = 3;
        else if (mi_value > 150)
            kvm_trim = 7;
        else
            kvm_trim = 0;
    }
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));
}


/**
 * @brief       This function is used to  set the modulation index of the sender.
 *              This function is common to all modes,the order of use requirement:configure mode first,
 *              then set the the modulation index,default is 0.5 in drive,both sides need to be consistent
 *              otherwise performance will suffer,if don't specifically request,don't need to call this function.
 * @param[in]   mi_value- the value of modulation_index*100.
 * @return      none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_tx_modulation_index(rf_mi_value_e mi_value)
{

    unsigned char modulation_index_high;
    unsigned char modulation_index_low;
    unsigned char kvm_trim;
    mi_value = (unsigned int)(mi_value * 1.28);
    modulation_index_low = mi_value%256;

    modulation_index_high = (mi_value%512)>>8;
    (reg_rf_radio_mode_cfg_rx2_0) = (modulation_index_low);
    (reg_rf_radio_mode_cfg_rx2_1) |= (modulation_index_high);

    if(reg_rf_mode_cfg_tx1_0 & 0x01)
    {
        if ((mi_value >= 75)&&(mi_value <= 100))
            kvm_trim = 3;
        else if (mi_value > 100)
            kvm_trim = 7;
        else
            kvm_trim = 1;
    }
    else
    {

        if ((mi_value >= 75)&&(mi_value <= 100))
            kvm_trim = 1;
        else if ((mi_value > 100)&&(mi_value <= 150))
            kvm_trim = 3;
        else if (mi_value > 150)
            kvm_trim = 7;
        else
            kvm_trim = 0;
    }
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));
}

/**
 * @brief       This function is used to set how many words as the transmission unit of baseband and dma.
 *              You don't need to call this function for normal use. By default, the unit is 1 world!
 *              After configuring the DMA, call this function to adjust the DMA rate.
 * @param[in]   rf_trans_unit_e size    - the unit of burst size .Identify how many bytes of data are
 *                                        handled by DMA each time
 * @return      none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_set_baseband_trans_unit(rf_trans_unit_e size)
{
    reg_bb_dma_ctr3(1) = ((reg_bb_dma_ctr3(1) & 0xf8) | size);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | size);
}


/**********************************************************************************************************************
 *                                         RF : AOA/D related functions                                   *
 *********************************************************************************************************************/


/**
 * @brief       This function is used to set the position of the first antenna switch after the AOA receiver reference.The default is in the
 *              middle of the first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code.Each additional code
 *              will move the switch point back by 0.125us
 * @param[in]   swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
 *                          back 0.125us.
 * @return      none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_aoa_rx_ant_switch_point_adjust(unsigned short swt_offset)
{
    unsigned char temp = (((swt_offset >> 8) & 0x01) << 2);
    reg_rf_ant_msb = ((((reg_rf_ant_msb) & (~FLD_RF_RX_ANT_OFFSET_MSB))) | temp);
    reg_rf_rx_antoffset = swt_offset & 0xff;
}


/**
 * @brief       This function is used to set the position of the first antenna switch after the AOD transmitter reference.The default is in the middle of the
 *              first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code. Each additional code will move
 *              the switch point back by 0.125us
 * @param[in]   swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
 *                          back 0.125us.
 * @return      none.
 * @note    TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
 */
void rf_aod_tx_ant_switch_point_adjust(unsigned short swt_offset)
{
    unsigned char temp = (((swt_offset >> 8) & 0x01) << 1);
    reg_rf_ant_msb = ((((reg_rf_ant_msb) & (~FLD_RF_TX_ANT_OFFSET_MSB))) | temp);
    reg_rf_tx_antoffset = swt_offset & 0xff;
}

/**
 * @brief       This function is mainly used to set the IQ data sample interval time. In normal mode, the sampling interval of AOA is 4us, and AOD will judge whether
 *              the sampling interval is 4us or 2us according to CTE info.The 4us/2us sampling interval corresponds to the 2us/1us slot mode stipulated in the protocol.
 *              Since the current hardware only supports the antenna switching interval of 4us/2us, setting the sampling interval to 1us or less will cause multiple
 *              sampling at the interval of one antenna switching. Therefore, the sampling data needs to be processed by the upper layer according to the needs, and
 *              currently it is mostly used Used in the debug process.
 *              After configuring RF, you can call this function to configure slot time.
 * @param[in]   time_us - AOA or AOD slot time mode.
 * @return      none.
 * @note        Attention:(1)When the time is 0.25us, it cannot be used with the 20bit iq data type, which will cause the sampling data to overflow.
 *                        (2)Since only the antenna switching interval of 4us/2us is supported, the sampling interval of 1us and shorter time intervals
 *                            will be sampled multiple times in one antenna switching interval. Suggestions can be used according to specific needs.
 *              TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
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

#endif
