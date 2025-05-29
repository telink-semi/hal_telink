/********************************************************************************************************
 * @file    emi.c
 *
 * @brief   This is the source file for TL323X
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
#include "lib/include/emi.h"
#include "lib/include/stimer.h"
/**********************************************************************************************************************
 *                                           local macro                                                             *
 *********************************************************************************************************************/
#define EMI_STATE0       0x1234                        //Initial value of random number0
#define EMI_STATE1       0x5678                        //Initial value of random number1
#define EMI_TX_FIFO_ADDR REG_BASEBAND_BASE_ADDR + 0x1c //0x17001c

/**
 * @brief This definition is used to set the maximum payload for rx
 * @note  When using it, it should be noted that the packet length sent by tx should not exceed the maximum buffer of rx
 *
 *        packet_length = DMA_length + header_length + payload_length + crc_length + hd_info_length
 *
 *        Common package structure:
 *
 *              | DMA_length | header_length | payload_length               | crc_length | hd_info_length
 *       ble    |   4 byte   |    2 byte     |   N byte                     |   3 byte   | 8 byte
 *       private|   4 byte   |    1 byte     |   N byte                     |   2 byte   | 8 byte
 *       zigbee |   4 byte   |    1 byte     |   N-2(crc_length) byte       |   2 byte   | 8 byte
 *
 */
#define EMI_RX_MAX_PKT_PAYLOAD 255

/**********************************************************************************************************************
 *                                           global constants                                                        *
 *********************************************************************************************************************/
static unsigned char emi_rx_packet[280] __attribute__((aligned(4)));
static unsigned char emi_ble_tx_packet[280] __attribute__((aligned(4)))      = {3, 0, 0, 0, 0, 10};
static unsigned char emi_zigbee_tx_packet[280] __attribute__((aligned(4)))   = {19, 0, 0, 0, 20, 0, 0};
static unsigned char Private_TPLL_tx_packet[280] __attribute__((aligned(4))) = {3, 0, 0, 0, 0, 10};
static unsigned int  s_emi_rx_cnt __attribute__((aligned(4)))                = 0;
static unsigned int  s_emi_rssibuf                                           = 0;
static signed int    s_emi_rssi                                              = 0;
static unsigned int  s_state0, s_state1;

/**********************************************************************************************************************
 *                                          function implementation                                                   *
 *********************************************************************************************************************/

extern unsigned char g_single_tong_freqoffset;

/**
 * @brief      This function serves to set the TX singletone power and channel
 * @param[in]  power_level - the power level.
 * @param[in]  rf_chn      - the channel.
 * @return     none
 */
void rf_emi_tx_single_tone(rf_power_level_e power_level, signed char rf_chn)
{
    g_single_tong_freqoffset = 1;
    rf_mode_init();
    rf_set_zigbee_250K_mode();
    rf_set_chn(rf_chn);
    rf_set_power_level_singletone(power_level);
    rf_set_txmode();
    g_single_tong_freqoffset = 0;
}

/**
 * @brief      This function serves to stop emi/(close RF)
 * @return     none
 */
void rf_emi_stop(void)
{
    reg_rf_lnm_pa_ow_ctrl_val = 0;
    reg_rf_pa_ow_val          = 0; //TX_PA_PWRs
    rf_set_power_level_index(0);
    rf_set_tx_rx_off();
}

/**
 * @brief      This function serves to set the CD mode correlation register
 * @return     none
 */
void rf_emi_tx_continue_setup(void)
{
    reg_rf_tx_mode1    = 0x0e;
    reg_rf_access_code = 0x00; // access code
    reg_rf_tx_mode2    = 0x80; // kick tx controller to wait data

    s_state0 = EMI_STATE0;
    s_state1 = EMI_STATE1;
}

/**
 * @brief      This function serves to update the CD mode data
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  power_level - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @param[in]  pkt_type    - The type of data sent.
 *    -#0:random
 *    -#1:0x0f
 *    -#2:0x55
 *    -#3:0xaa
 *    -#4:0xf0
 *    -#5:0x00
 *    -#6:0xff
 * @return     none
 */
void rf_emi_tx_continue_update_data(rf_mode_e rf_mode, rf_power_level_e power_level, signed char rf_chn, unsigned char pkt_type)
{
    rf_mode_init();
    switch (rf_mode) {
    case RF_MODE_BLE_1M_NO_PN:
        rf_set_ble_1M_NO_PN_mode();
        break;
    case RF_MODE_BLE_2M_NO_PN:
        rf_set_ble_2M_NO_PN_mode();
        break;
    case RF_MODE_LR_S2_500K:
        rf_set_ble_500K_mode();
        break;
    case RF_MODE_LR_S8_125K:
        rf_set_ble_125K_mode();
        break;
    case RF_MODE_ZIGBEE_250K:
        rf_set_zigbee_250K_mode();
        break;
    case RF_MODE_PRIVATE_1M:
        rf_set_pri_1M_mode();
        break;
    case RF_MODE_PRIVATE_2M:
        rf_set_pri_2M_mode();
        break;

    default:
        break;
    }
    rf_pn_disable();
    rf_set_chn(rf_chn);
    reg_rf_ll_ctrl0 = 0x45; // reset tx/rx state machine
    rf_set_power_level(power_level);
    rf_set_txmode();
    delay_us(150);
    rf_emi_tx_continue_setup();
    reg_rf_access_code = pkt_type; // 0:pbrs9  1:0x0f  2:0x55  3:0xaa  4:0xf0  5:0x00  6:0xff
}

/**
 * @brief      This function serves to generate random number.
 * @param[in]  state - the old random number.
 * @return     the new random number
 */
unsigned int emi_pn_gen(unsigned int state)
{
    unsigned int feed = 0;
    feed              = (state & 0x4000) >> 1;
    state ^= feed;
    state <<= 1;
    state = (state & 0xfffe) + ((state & 0x8000) >> 15);
    return state;
}

/**
 * @brief      This function serves to continue to run the CD mode.The missing three in the program is because 
 *             the original writing is problematic and will not be used.
 * @return     none
 */
void rf_continue_mode_run(void)
{
    if (1 == reg_rf_access_code) {
        write_reg32(EMI_TX_FIFO_ADDR, 0x0f0f0f0f);
    } else if (2 == reg_rf_access_code) {
        write_reg32(EMI_TX_FIFO_ADDR, 0x55555555);
    } else if (3 == reg_rf_access_code) {
        write_reg32(EMI_TX_FIFO_ADDR, 0xaaaaaaaa);
    } else if (4 == reg_rf_access_code) {
        write_reg32(EMI_TX_FIFO_ADDR, 0xf0f0f0f0);
    } else if (5 == reg_rf_access_code) {
        write_reg32(EMI_TX_FIFO_ADDR, 0);
    } else if (6 == reg_rf_access_code) {
        write_reg32(EMI_TX_FIFO_ADDR, 0xffffffff);
    } else {
        write_reg32(EMI_TX_FIFO_ADDR, (s_state0 << 16) + s_state1);
        s_state0 = emi_pn_gen(s_state0);
        s_state1 = emi_pn_gen(s_state1);
    }

    while (reg_rf_txfifo & FLD_RF_TX_FIFO_FULL) {
    }
}

static unsigned char rxpara_flag = 1;

/**
 * @brief      This function serves to set rx mode and channel
 * @param[in]  mode   - mode of RF.
 * @param[in]  rf_chn - the rx channel.
 * @return     none
 */
void rf_emi_rx_setup(rf_mode_e mode, signed char rf_chn)
{
    rf_mode_init();
    switch (mode) {
    case RF_MODE_BLE_1M_NO_PN:
        rf_set_ble_1M_NO_PN_mode();
        break;
    case RF_MODE_BLE_2M_NO_PN:
        rf_set_ble_2M_NO_PN_mode();
        break;
    case RF_MODE_LR_S2_500K:
        rf_set_ble_500K_mode();
        break;
    case RF_MODE_LR_S8_125K:
        rf_set_ble_125K_mode();
        break;
    case RF_MODE_ZIGBEE_250K:
        rf_set_zigbee_250K_mode();
        break;
    case RF_MODE_PRIVATE_1M:
        rf_set_pri_1M_mode();
        break;
    case RF_MODE_PRIVATE_2M:
        rf_set_pri_2M_mode();
        break;
    default:
        break;
    }

    rf_set_rx_maxlen(EMI_RX_MAX_PKT_PAYLOAD - 2); //Rx mode in EMI is manual mode, and only one DMA fifo is used in manual mode.
                                                  //If multiple DMA fifo are used, it should be noted that rx packet length cannot be greater than the depth of DMA fifo
    rf_set_rx_dma(emi_rx_packet, 0, 253);
    rf_pn_disable();
    rf_set_chn(rf_chn);                           //set freq
    if (mode != RF_MODE_ZIGBEE_250K) {
        rf_access_code_comm(EMI_ACCESS_CODE);     //access code
    }
    rf_set_tx_rx_off();
    rf_set_rxmode();
    delay_us(150);
    if (rxpara_flag == 1) {
#if (0)
        rf_set_rxpara();
#endif
        rxpara_flag = 0;
    }

    if (rf_chn == 24 || rf_chn == 48 || rf_chn == 72) {
#if (0)
        rf_ldot_ldo_rxtxlf_bypass_en();
#endif
    } else {
#if (0)
        rf_ldot_ldo_rxtxlf_bypass_dis();
#endif
    }
    s_emi_rssi    = 0;
    s_emi_rssibuf = 0;
    s_emi_rx_cnt  = 0;
}

/**
 * @brief    This function serves to update the number of receiving packet and the RSSI
 * @return   none
 */
void rf_emi_rx_loop(void)
{
    if (rf_get_irq_status(FLD_RF_IRQ_RX)) // rx irq
    {
        if ((reg_rf_dec_err & 0xf0) == 0) // crc err
        {
            s_emi_rssibuf += reg_rf_agc_rssi_lat;
            if (s_emi_rx_cnt) {
                if (s_emi_rssibuf != 0) {
                    s_emi_rssibuf >>= 1;
                }
            }
            s_emi_rssi = s_emi_rssibuf - 110;
            s_emi_rx_cnt++;
        }
        rf_clr_irq_status(FLD_RF_IRQ_RX); // clr rx irq
        reg_rf_ll_cmd = 0x80;             // stop cmd
    }
}

/**
 * @brief    This function serves to get the number of packets received.
 * @return   the number of packets received.
 */
unsigned int rf_emi_get_rxpkt_cnt(void)
{
    return s_emi_rx_cnt;
}

/**
 * @brief    This function serves to get the RSSI of packets received
 * @return   the RSSI of packets received
 */
char rf_emi_get_rssi_avg(void)
{
    return s_emi_rssi;
}

/**
 * @brief      This function serves to generate random packets that need to be sent in burst mode
 * @param[in] *p - the address of random packets.
 * @param[in]  n - the number of random packets.
 * @return     none
 */
void rf_phy_test_prbs9(unsigned char *p, int n)
{
    unsigned short x = 0x1ff;
    int            i = 0;
    int            j = 0;
    for (i = 0; i < n; i++) {
        unsigned char d = 0;
        for (j = 0; j < 8; j++) {
            if (x & 1) {
                d |= BIT(j);
            }
            x = (x >> 1) | (((x << 4) ^ (x << 8)) & 0x100);
        }
        *p++ = d;
    }
}

/**
 * @brief      This function serves to generate random packets that need to be sent in burst mode
 * @param[in] *p - the address of random packets.
 * @param[in]  n - the number of random packets.
 * @return     none
 */
void rf_phy_test_prbs15(unsigned char *p, int n)
{
    unsigned short x = 0x7fff;
    int            i = 0;
    int            j = 0;
    for (i = 0; i < n; i++) {
        unsigned char d = 0;
        for (j = 0; j < 8; j++) {
            if (x & 1) {
                d |= BIT(j);
            }
            x = (x >> 1) | (((x << 4) ^ (x << 8)) & 0x8000);
        }
        *p++ = d;
    }
}

/**
 * @brief      This function serves to send packets in the burst mode
 * @param[in]  rf_mode  - mode of RF.
 * @param[in]  pkt_type - The type of data sent.
 *  * -#0:prbs9(random)
 *    -#1:0x0f
 *    -#2:0x55
 *    -#3:0xaa
 *    -#4:0xf0
 *    -#5:0x00
 *    -#6:0xff
 * @return     none
 */
void rf_emi_tx_burst_loop(rf_mode_e rf_mode, unsigned char pkt_type)
{
    unsigned char rf_data_len   = EMI_TX_PKT_PAYLOAD + 1;
    unsigned int  rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
    reg_rf_ll_cmd               = 0x80;                                         // stop SM
    if ((rf_mode == RF_MODE_BLE_1M_NO_PN) || (rf_mode == RF_MODE_BLE_2M_NO_PN)) //ble
    {
        rf_data_len          = EMI_TX_PKT_PAYLOAD + 2;
        rf_tx_dma_len        = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[4] = 0;
        emi_ble_tx_packet[5] = EMI_TX_PKT_PAYLOAD;
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len & 0xff;
        rf_start_stx((void *)emi_ble_tx_packet, stimer_get_tick() + 10);
        while (!(rf_get_irq_status(FLD_RF_IRQ_TX)))
            ;
        rf_clr_irq_status(FLD_RF_IRQ_TX);

        delay_ms(2);
        if (pkt_type == 0) {
            rf_phy_test_prbs9(&emi_ble_tx_packet[6], EMI_TX_PKT_PAYLOAD);
        }
    } else if (rf_mode == RF_MODE_LR_S8_125K) {
        rf_data_len          = EMI_TX_PKT_PAYLOAD + 2;
        rf_tx_dma_len        = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[4] = 0;
        emi_ble_tx_packet[5] = EMI_TX_PKT_PAYLOAD;
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len & 0xff;
        rf_start_stx((void *)emi_ble_tx_packet, stimer_get_tick() + 10);
        while (!(rf_get_irq_status(FLD_RF_IRQ_TX)))
            ;
        rf_clr_irq_status(FLD_RF_IRQ_TX);


        delay_ms(2);
        if (pkt_type == 0) {
            rf_phy_test_prbs9(&emi_ble_tx_packet[6], EMI_TX_PKT_PAYLOAD);
        }
    } else if (rf_mode == RF_MODE_LR_S2_500K) {
        rf_data_len          = EMI_TX_PKT_PAYLOAD + 2;
        rf_tx_dma_len        = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[4] = 0;
        emi_ble_tx_packet[5] = EMI_TX_PKT_PAYLOAD;
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len & 0xff;
        rf_start_stx((void *)emi_ble_tx_packet, stimer_get_tick() + 10);
        while (!(rf_get_irq_status(FLD_RF_IRQ_TX)))
            ;
        rf_clr_irq_status(FLD_RF_IRQ_TX);


        delay_ms(2);
        if (pkt_type == 0) {
            rf_phy_test_prbs9(&emi_ble_tx_packet[6], EMI_TX_PKT_PAYLOAD);
        }
    } else if (rf_mode == RF_MODE_ZIGBEE_250K) {
        rf_data_len             = EMI_TX_PKT_PAYLOAD + 1;
        rf_tx_dma_len           = rf_tx_packet_dma_len(rf_data_len);
        emi_zigbee_tx_packet[4] = EMI_TX_PKT_PAYLOAD + 2;
        emi_zigbee_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        emi_zigbee_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        emi_zigbee_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        emi_zigbee_tx_packet[0] = rf_tx_dma_len & 0xff;
        rf_start_stx((void *)emi_zigbee_tx_packet, stimer_get_tick() + 10);
        while (!(rf_get_irq_status(FLD_RF_IRQ_TX)))
            ;
        rf_clr_irq_status(FLD_RF_IRQ_TX);


        delay_us(625 * 2);
        if (pkt_type == 0) {
            rf_phy_test_prbs9(&emi_zigbee_tx_packet[5], EMI_TX_PKT_PAYLOAD);
        }
    } else if ((rf_mode == RF_MODE_PRIVATE_1M) || (rf_mode == RF_MODE_PRIVATE_2M)) {
        rf_data_len               = EMI_TX_PKT_PAYLOAD + 1;
        Private_TPLL_tx_packet[4] = EMI_TX_PKT_PAYLOAD;
        rf_tx_dma_len             = rf_tx_packet_dma_len(rf_data_len);
        Private_TPLL_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        Private_TPLL_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        Private_TPLL_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        Private_TPLL_tx_packet[0] = rf_tx_dma_len & 0xff;
        rf_start_stx((void *)Private_TPLL_tx_packet, stimer_get_tick() + 10);
        while (!(rf_get_irq_status(FLD_RF_IRQ_TX)))
            ;
        rf_clr_irq_status(FLD_RF_IRQ_TX);


        delay_ms(1);
        if (pkt_type == 0) {
            rf_phy_test_prbs9(&Private_TPLL_tx_packet[5], EMI_TX_PKT_PAYLOAD);
        }
    }
}

/**
 * @brief      This function serves to set the burst mode
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  power_level - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @param[in]  pkt_type    - The type of data sent.
 * -#0:random
 * -#1:0xf0
 * -#2:0x55
 * @return     none
 */
void rf_emi_tx_burst_setup(rf_mode_e rf_mode, rf_power_level_e power_level, signed char rf_chn, unsigned char pkt_type)
{
    unsigned char i       = 0;
    unsigned char tx_data = 0;
    rf_mode_init();
    //    write_reg8(0x100844,0x10); // print buffer size set
    rf_set_tx_dma(2, 128);
    rf_set_chn(rf_chn);
    switch (rf_mode) {
    case RF_MODE_BLE_1M_NO_PN:
        rf_set_ble_1M_NO_PN_mode();
        break;
    case RF_MODE_BLE_2M_NO_PN:
        rf_set_ble_2M_NO_PN_mode();
        break;
    case RF_MODE_LR_S2_500K:
        rf_set_ble_500K_mode();
        break;
    case RF_MODE_LR_S8_125K:
        rf_set_ble_125K_mode();
        break;
    case RF_MODE_ZIGBEE_250K:
        rf_set_zigbee_250K_mode();
        break;
    case RF_MODE_PRIVATE_1M:
        rf_set_pri_1M_mode();
        break;
    case RF_MODE_PRIVATE_2M:
        rf_set_pri_2M_mode();
        break;
    default:
        break;
    }
    if (rf_mode != RF_MODE_ZIGBEE_250K) {
        rf_access_code_comm(EMI_ACCESS_CODE); //access code
    }

    rf_pn_disable();
    rf_set_power_level(power_level);
    if (pkt_type == 1) {
        tx_data = 0x0f;
    } else if (pkt_type == 2) {
        tx_data = 0x55;
    } else if (pkt_type == 3) {
        tx_data = 0xaa;
    } else if (pkt_type == 4) {
        tx_data = 0xf0;
    }
    switch (rf_mode) {
    case RF_MODE_LR_S2_500K:
    case RF_MODE_LR_S8_125K:
    case RF_MODE_BLE_1M_NO_PN:
    case RF_MODE_BLE_2M_NO_PN:
        emi_ble_tx_packet[4] = pkt_type; //type
        for (i = 0; i < EMI_TX_PKT_PAYLOAD; i++) {
            emi_ble_tx_packet[6 + i] = tx_data;
        }
        break;
    case RF_MODE_ZIGBEE_250K:
        emi_zigbee_tx_packet[5] = pkt_type; // type
        for (i = 0; i < EMI_TX_PKT_PAYLOAD; i++) {
            emi_zigbee_tx_packet[5 + i] = tx_data;
        }
        break;
    case RF_MODE_PRIVATE_1M:
    case RF_MODE_PRIVATE_2M:
        Private_TPLL_tx_packet[5] = pkt_type; // type
        for (i = 0; i < EMI_TX_PKT_PAYLOAD; i++) {
            Private_TPLL_tx_packet[5 + i] = tx_data;
        }
        break;

    default:
        break;
    }
}
