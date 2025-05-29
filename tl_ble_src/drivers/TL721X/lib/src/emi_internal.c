/********************************************************************************************************
 * @file    emi_internal.c
 *
 * @brief   This is the source file for TL721X
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
#include "lib/include/emi_internal.h"

const rf_mode_e rf_mode_list[10] = {
    RF_MODE_BLE_2M_NO_PN, /**< ble 2m mode */
    RF_MODE_BLE_1M_NO_PN, /**< ble 1M close pn mode */
    RF_MODE_ZIGBEE_250K,  /**< zigbee 250K mode */
    RF_MODE_LR_S2_500K,   /**< ble 500K mode */
    RF_MODE_LR_S8_125K,   /**< ble 125K mode */
    RF_MODE_PRIVATE_1M,   /**< private 1M mode */
    RF_MODE_PRIVATE_2M,   /**< private 2M mode */
};


emi_cfg_param_t emi_cfg_param =
    {
        .rf_chn             = 2,
        .pkt_type           = 0,
        .emi_tx_payload_len = 37,
        .emi_pkt_duty_cycle = 50,
        .emi_access_code    = EMI_ACCESS_CODE_V1,
        .rf_mode            = RF_MODE_BLE_2M_NO_PN,
        .power_level        = 0,
};

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

static unsigned char rxpara_flag = 1;

/**
 * @brief      This function serves to set rx mode and channel
 * @param[in]  none
 * @return     none
 * @note The V1 version can modify RF related configurations through the emi_tx_burst_cfg_t structure
 */
void rf_emi_rx_setup_v1(void)
{
    rf_mode_init();
    switch (emi_cfg_param.rf_mode) {
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
    rf_set_rx_maxlen(EMI_RX_MAX_PKT_PAYLOAD - 2);           //Rx mode in EMI is manual mode, and only one DMA fifo is used in manual mode.
                                                            //If multiple DMA fifo are used, it should be noted that rx packet length cannot be greater than the depth of DMA fifo
    rf_set_rx_dma(emi_rx_packet, 0, 253);
    rf_pn_disable();
    rf_set_chn(emi_cfg_param.rf_chn);                       //set freq
    if (emi_cfg_param.rf_mode != RF_MODE_ZIGBEE_250K) {
        rf_access_code_comm(emi_cfg_param.emi_access_code); //access code
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

    if (emi_cfg_param.rf_chn == 24 || emi_cfg_param.rf_chn == 48 || emi_cfg_param.rf_chn == 72) {
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
 * @brief   This function is used to get the packet transmitting time for different payload lengths and modes
 * @param   payload_len: payload length
 * @param   mode: rf mode
 * @return  the packet transmit time
 */
unsigned int emi_get_pkt_time(unsigned char payload_len, rf_mode_e rf_mode)
{
    unsigned int  bit_time;
    unsigned int  total_len, byte_time = 8;
    unsigned char preamble_len;
    unsigned int  total_time = 0;
    switch (rf_mode) {
    case RF_MODE_BLE_2M_NO_PN:                                                          //ble2m
        preamble_len = reg_rf_preamble_trail & 0x1f;
        total_len    = preamble_len + 4 + 2 + payload_len + 3;                          // preamble + access_code + header + payload + crc
        byte_time    = 4;                                                               //4us/byte,0.5us/bit
        total_time   = byte_time * total_len;
        break;
    case RF_MODE_BLE_1M_NO_PN:                                                          //ble1m
        preamble_len = reg_rf_preamble_trail & 0x1f;
        total_len    = preamble_len + 4 + 2 + payload_len + 3;                          // preamble + access_code + header + payload + crc
        byte_time    = 8;                                                               //8us/byte,1us/bit
        total_time   = byte_time * total_len;
        break;
    case RF_MODE_ZIGBEE_250K:                                                           //zigbee_250k
        bit_time   = 4;                                                                 //4us/bit
        total_time = (32 + 8 + 8 + 8 * payload_len) * bit_time;                         // preamble + SFD + PHR + payload + crc
        break;

    case RF_MODE_LR_S2_500K:                                                            //s2
        bit_time   = 2;                                                                 //2us/bit
        total_time = (80 + 256 + 16 + 24) + (16 + payload_len * 8 + 24 + 3) * bit_time; // preamble + access_code + coding indicator + TERM1 + header + payload + crc + TERM2
        break;

    case RF_MODE_LR_S8_125K:                                                            //s8
        bit_time   = 8;                                                                 //8us/bit
        total_time = (80 + 256 + 16 + 24) + (16 + payload_len * 8 + 24 + 3) * bit_time; // preamble + access_code + coding indicator + TERM1 + header + payload + crc + TERM2
        break;

    case RF_MODE_PRIVATE_1M:                                                            //private_1m_tpll
        preamble_len = reg_rf_preamble_trail & 0x1f;
        total_len    = preamble_len + 4 + 1 + payload_len + 2;                          // preamble + access_code + header + payload + crc
        byte_time    = 8;                                                               //8us/byte,1us/bit
        total_time   = byte_time * total_len;
        break;

    case RF_MODE_PRIVATE_2M:                                   //private_2m_tpll
        preamble_len = reg_rf_preamble_trail & 0x1f;
        total_len    = preamble_len + 4 + 1 + payload_len + 2; // preamble + access_code + header + payload + crc
        byte_time    = 4;
        total_time   = byte_time * total_len;
        break;

    default:
        break;
    }
    return (total_time);
}

/**
 * @brief      This function serves to send packets in the burst mode
 * @param[in]  none
 * @return     none
 * @note The V1 version can modify RF related configurations through the emi_cfg_param_t structure
 */
void rf_emi_tx_burst_loop_v1(void)
{
    unsigned int pkt_time  = 0;
    unsigned int pkt_delay = 0;
    void        *packet_ptr;
    reg_rf_ll_cmd = 0x80; // stop SM
    pkt_time      = emi_get_pkt_time(emi_cfg_param.emi_tx_payload_len, emi_cfg_param.rf_mode);
    pkt_delay     = ((pkt_time * 100) / emi_cfg_param.emi_pkt_duty_cycle) - pkt_time;
    switch (emi_cfg_param.rf_mode) {
    case RF_MODE_BLE_1M_NO_PN:
    case RF_MODE_BLE_2M_NO_PN:
    case RF_MODE_LR_S2_500K:
    case RF_MODE_LR_S8_125K:
        packet_ptr = emi_ble_tx_packet;
        break;
    case RF_MODE_ZIGBEE_250K:
        packet_ptr = emi_zigbee_tx_packet;
        break;
    case RF_MODE_PRIVATE_1M:
    case RF_MODE_PRIVATE_2M:
        packet_ptr = Private_TPLL_tx_packet;
        break;
    default:
        return;
    }
    delay_us(pkt_delay);
    rf_tx_pkt(packet_ptr);

    while (!(rf_get_irq_status(FLD_RF_IRQ_TX)));
    rf_clr_irq_status(FLD_RF_IRQ_TX);
}

/**
 * @brief    This function serves to update the number of receiving packet and the RSSI
 * @return   none
 */
void rf_emi_rx_loop_v1(void)
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
unsigned int rf_emi_get_rxpkt_cnt_v1(void)
{
    return s_emi_rx_cnt;
}

/**
 * @brief    This function serves to get the RSSI of packets received
 * @return   the RSSI of packets received
 */
char rf_emi_get_rssi_avg_v1(void)
{
    return s_emi_rssi;
}

/**
 * @brief      This function serves to set the burst mode
 * @param[in]  none
 * @return     none
 * @note The V1 version can modify RF related configurations through the emi_cfg_param_t structure
 */
void rf_emi_tx_burst_setup_v1(void)
{
    unsigned char i             = 0;
    unsigned char tx_data       = 0;
    unsigned int  rf_data_len   = emi_cfg_param.emi_tx_payload_len + 1;
    unsigned int  rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
    rf_mode_init();
    rf_set_tx_dma(2, 128);
    rf_set_chn(emi_cfg_param.rf_chn);
    switch (emi_cfg_param.rf_mode) {
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
    if (emi_cfg_param.rf_mode != RF_MODE_ZIGBEE_250K) {
        rf_access_code_comm(emi_cfg_param.emi_access_code); //access code
    }

    rf_pn_disable();
    rf_set_power_level(emi_cfg_param.power_level);

    switch (emi_cfg_param.pkt_type) {
    case 1:
        tx_data = 0x0f;
        break;
    case 2:
        tx_data = 0x55;
        break;
    case 3:
        tx_data = 0xaa;
        break;
    case 4:
        tx_data = 0xf0;
        break;
    case 5:
        tx_data = 0x00;
        break;
    case 6:
        tx_data = 0xff;
        break;
    default:
        break;
    }
    switch (emi_cfg_param.rf_mode) {
    case RF_MODE_LR_S2_500K:
    case RF_MODE_LR_S8_125K:
    case RF_MODE_BLE_1M_NO_PN:
    case RF_MODE_BLE_2M_NO_PN:
        rf_data_len          = emi_cfg_param.emi_tx_payload_len + 2;
        rf_tx_dma_len        = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[5] = emi_cfg_param.emi_tx_payload_len;
        emi_ble_tx_packet[4] = emi_cfg_param.pkt_type; //type
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len & 0xff;
        if (emi_cfg_param.pkt_type == 0) {
            rf_phy_test_prbs9(&emi_ble_tx_packet[6], emi_cfg_param.emi_tx_payload_len);
        } else {
            for (i = 0; i < emi_cfg_param.emi_tx_payload_len; i++) {
                emi_ble_tx_packet[6 + i] = tx_data;
            }
        }
        break;
    case RF_MODE_ZIGBEE_250K:

        rf_data_len             = emi_cfg_param.emi_tx_payload_len + 1;
        rf_tx_dma_len           = rf_tx_packet_dma_len(rf_data_len);
        emi_zigbee_tx_packet[5] = emi_cfg_param.pkt_type; // type
        emi_zigbee_tx_packet[4] = emi_cfg_param.emi_tx_payload_len + 2;
        emi_zigbee_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        emi_zigbee_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        emi_zigbee_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        emi_zigbee_tx_packet[0] = rf_tx_dma_len & 0xff;


        if (emi_cfg_param.pkt_type == 0) {
            rf_phy_test_prbs9(&emi_zigbee_tx_packet[5], emi_cfg_param.emi_tx_payload_len);
        } else {
            for (i = 0; i < emi_cfg_param.emi_tx_payload_len; i++) {
                emi_zigbee_tx_packet[5 + i] = tx_data;
            }
        }
        break;
    case RF_MODE_PRIVATE_1M:
    case RF_MODE_PRIVATE_2M:
        rf_data_len               = emi_cfg_param.emi_tx_payload_len + 1;
        Private_TPLL_tx_packet[5] = emi_cfg_param.pkt_type; // type
        Private_TPLL_tx_packet[4] = emi_cfg_param.emi_tx_payload_len;
        rf_tx_dma_len             = rf_tx_packet_dma_len(rf_data_len);
        Private_TPLL_tx_packet[3] = (rf_tx_dma_len >> 24) & 0xff;
        Private_TPLL_tx_packet[2] = (rf_tx_dma_len >> 16) & 0xff;
        Private_TPLL_tx_packet[1] = (rf_tx_dma_len >> 8) & 0xff;
        Private_TPLL_tx_packet[0] = rf_tx_dma_len & 0xff;

        if (emi_cfg_param.pkt_type == 0) {
            rf_phy_test_prbs9(&Private_TPLL_tx_packet[5], emi_cfg_param.emi_tx_payload_len);
        } else {
            for (i = 0; i < emi_cfg_param.emi_tx_payload_len; i++) {
                Private_TPLL_tx_packet[5 + i] = tx_data;
            }
        }
        break;

    default:
        break;
    }
    rf_set_txmode();
    delay_us(150);
}
