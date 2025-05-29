/********************************************************************************************************
 * @file    rf_ble.c
 *
 * @brief   This is the source file for tl322x
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
#include "lib/include/rf/rf_ble.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"

/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

#define BLE_S2_S8_OLD_PATH 1 //For internal testing only, the old path will not be released to the public

/**
 * @brief       This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return      none.
 */
void rf_set_ble_chn(signed char chn_num)
{
    signed char ble_chn_num = 0;
    write_reg8(0x170020, chn_num);

    if (chn_num < 11) {
        ble_chn_num = chn_num + 2;
    }

    else if (chn_num < 37) {
        ble_chn_num = chn_num + 3;
    }

    else if (chn_num == 37) {
        ble_chn_num = 1;
    }

    else if (chn_num == 38) {
        ble_chn_num = 13;
    }

    else if (chn_num == 39) {
        ble_chn_num = 40;
    }

    else if (chn_num < 51) {
        ble_chn_num = chn_num;
    }

    else if (chn_num <= 61) {
        ble_chn_num = -61 + chn_num;
    }

    ble_chn_num = ble_chn_num << 1;
    rf_set_chn(ble_chn_num);
}

/**
 * @brief     This function serves to  set ble_1M  mode of RF.
 * @return    none.
 */
_attribute_ram_code_  //BLE SDK use
void rf_set_ble_1M_mode(void)
{
    //aura_1m
    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30); //sc_code.11 = IF of 1000MHz (1MBPS mode)
    /*
    *       bit                 default     value                note
    *                                                            note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x20); //RADIO BLE_MODE_TX,1MBPS:bit<0>;VCO_TRIM_KV:bit<1-3>;HPMC_EXP_DIFF_COUNT_L:bit<4-7>.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,1MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    //rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc. rx continue mode on:bit<3>


    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table_1m
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    //ble1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x08); //PN.
    write_reg8(0x170002, 0x42); //preamble length
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
                                //bit<5>:write packet length filed into sram

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.0x4c->0x0c
    write_reg8(0x1704bb, 0x00); //disable 2 stage filter
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[0]);

    g_rfmode = RF_MODE_BLE_1M;
}

/**
 * @brief     This function serves to  set ble_1M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_1M_NO_PN_mode(void)
{
    //aura_1m
    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30); //sc_code.11 = IF of 1000MHz (1MBPS mode)
    /*
    *       bit                 default     value                note
    *                                                            note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x20); //RADIO BLE_MODE_TX,1MBPS:bit<0>;VCO_TRIM_KV:bit<1-3>;HPMC_EXP_DIFF_COUNT_L:bit<4-7>.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,1MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    //rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc. rx continue mode on:bit<3>

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table_1m
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    //ble1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x42); //preamble length
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
                                //bit<5>:write packet length filed into sram

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.0x4c->0x0c
    write_reg8(0x1704bb, 0x00); //disable 2 stage filter
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);
    rf_set_crc_config(&rf_crc_config[0]);

    g_rfmode = RF_MODE_BLE_1M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_2M  mode of RF.
 * @return    none.
 */
void rf_set_ble_2M_mode(void)
{
    //aura_2m
    write_reg8(0x17063d, 0x21); //ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x170620, 0x20); //sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.0x20->0x1e


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb, 0x00); //2 stage filter

    //rx_cont_mode
    write_reg8(0x170420, 0xc8);

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table
    write_reg8(0x1704c2, 0x40); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x59); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x70); //grx_4.
    write_reg8(0x1704c7, 0x7b); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x08); //PN.
    write_reg8(0x170002, 0x43); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.


    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (1:2mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (1:2mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[0]);

    g_rfmode = RF_MODE_BLE_2M;
}

/**
 * @brief     This function serves to  set ble_2M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_2M_NO_PN_mode(void)
{
    //aura_2m
    write_reg8(0x17063d, 0x21); //ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x170620, 0x20); //sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.0x20->0x1e


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb, 0x00); //2 stage filter,for FPGA

    //rx_cont_mode
    write_reg8(0x170420, 0xc8);

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table
    write_reg8(0x1704c2, 0x40); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x59); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x70); //grx_4.
    write_reg8(0x1704c7, 0x7b); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x43); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (1:2mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (1:2mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);
    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_BLE_2M_NO_PN;
}

/**
 * @brief     This function serves to set ble_500K  mode of RF.
 * @return    none.
 */
void rf_set_ble_500K_mode(void)
{
    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30); //sc_code.11 = IF of 1000MHz (1MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170473, 0xa1); //TOT_DEV_RST.
    write_reg8(0x170437, 0x0c); //LR_NUM_GEAR_H.
    write_reg8(0x170439, 0x7d); //LR_TIM_REC_CFG_1.

    write_reg8(0x170420, 0xc9); // script cc.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.


    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x08); //PN.
    write_reg8(0x170002, 0x4a); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x24); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x1704bb, 0x00); //disable 2 stage filter

#if (BLE_S2_S8_OLD_PATH)
    //old path config
    write_reg8(0x17044e, 0xf0); //ble sync threshold:To modem.
    write_reg8(0x170436, 0xee); //LR_NUM_GEAR_L.
    write_reg8(0x170438, 0xb8); //LR_TIM_EDGE_DEV.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
#else
    //new path config,hp need
    write_reg8(0x17044e, 0x3c); //ble sync threshold:To modem.
    write_reg8(0x170436, 0xf6); //LR_NUM_GEAR_L.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.//new mdm, new viterbi,hp need
    write_reg8(0x17043e, 0x01); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4,hp need
#endif

    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32,for FPGA

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //for RF 48M
    reg_rf_hshp_ctrl_0 = reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LR_S2_500K;
}

/**
 * @brief     This function serves to  set ble_125K  mode of RF.
 * @return    none.
 */
void rf_set_ble_125K_mode(void)
{
    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30); //sc_code.11 = IF of 1000MHz (1MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170473, 0xa1); //TOT_DEV_RST.
    write_reg8(0x170436, 0xf6); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0c); //LR_NUM_GEAR_H.
    write_reg8(0x170439, 0x7d); //LR_TIM_REC_CFG_1.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.


    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.

    write_reg8(0x170420, 0xc9); // script cc.

    write_reg8(0x17049a, 0x00); //tx_tp_align.
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x08); //PN.
    write_reg8(0x170002, 0x4a); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x34); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.


    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x1704bb, 0x00); //disable 2 stage filter

#if (BLE_S2_S8_OLD_PATH)
    //old path config
    write_reg8(0x17044e, 0xf0); //ble sync threshold:To modem.
    write_reg8(0x170438, 0xb8); //LR_TIM_EDGE_DEV.
    write_reg8(0x170421, 0x80); //modem:bit<2> LR_MODEM_SEL,bit<3> LR_VITERBI_SEL
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
#else
    //new path config,hp need
    write_reg8(0x17044e, 0x3c); //ble sync threshold:To modem.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.hp need
    write_reg8(0x17043e, 0x01); //BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4.hp need
#endif

    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32,for FPGA

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //for RF 48M
    reg_rf_hshp_ctrl_0 = reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LR_S8_125K;
}

/**
 * @brief     This function serves to  set ble_4M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_4M_NO_PN_mode(void)
{
    //aura_2m
    write_reg8(0x17063d, 0x41); //ble:bw_code.010 -> IF = 3MHz, BW = 2.3MHz) (LIF, 4MBPS)
    write_reg8(0x170620, 0x10); //sc_code.01 = IF of 3000MHz (4MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x02(IF:1MHz->3MHz,BW:1MHz->2.3MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x02 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170621, 0x4a); //if_freq,IF = 3Mhz,BW = 2.3Mhz.
    write_reg8(0x170622, 0x88); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x2c); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.0x20->0x1e

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb, 0x00); //2 stage filter,for FPGA

    //rx_cont_mode
    write_reg8(0x170420, 0xc8);

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table
    write_reg8(0x1704c2, 0x3b); //grx_0.
    write_reg8(0x1704c3, 0x4c); //grx_1.
    write_reg8(0x1704c4, 0x58); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x44); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (2:4mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (2:4mbps)
    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN; //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x12);
    //    write_reg8(0x170216,0x29);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <4>:CBPF_HIGH_GBW           default:0,->1    Enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * This setting is used to enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * Applies exclusively to 4MHz/6MHz modes; other modes remain disabled by default. 
    * modified by chenxi.wang,confirmed by yuya.hao 20250421.
    */
    reg_rf_reg_sparelv1 |=FLD_RF_CBPF_HIGH_GBW;

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_BLE_4M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_6M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_6M_NO_PN_mode(void)
{
    //aura_2m
    write_reg8(0x17063d, 0x81); //ble:bw_code.100 -> IF = 4.5MHz, BW = 3.5MHz (ZIF, 6MBPS)
    write_reg8(0x170620, 0x00); //sc_code.00 = IF of 4500MHz (6MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x03(IF:1MHz->4.5MHz,BW:1MHz->3.5MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x03 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0xce); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x32); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.0x20->0x1e

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb, 0x00); //2 stage filter,for FPGA

    //rx_cont_mode
    write_reg8(0x170420, 0xc8);

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table
    write_reg8(0x1704c2, 0x3b); //grx_0.
    write_reg8(0x1704c3, 0x4c); //grx_1.
    write_reg8(0x1704c4, 0x58); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x46); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (3:6mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (3:6mbps)
    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN; //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x11);
    //    write_reg8(0x170216,0x29);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <4>:CBPF_HIGH_GBW           default:0,->1    Enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * This setting is used to enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * Applies exclusively to 4MHz/6MHz modes; other modes remain disabled by default. 
    * modified by chenxi.wang,confirmed by yuya.hao 20250421.
    */
    reg_rf_reg_sparelv1 |=FLD_RF_CBPF_HIGH_GBW;


    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_BLE_6M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_4M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_4M_mode(void)
{
    //aura_2m
    write_reg8(0x17063d, 0x41); //ble:bw_code.010 -> IF = 3MHz, BW = 2.3MHz) (LIF, 4MBPS)
    write_reg8(0x170620, 0x10); //sc_code.01 = IF of 3000MHz (4MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x02(IF:1MHz->3MHz,BW:1MHz->2.3MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x02 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x88); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x2c); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.0x20->0x1e

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb, 0x00); //2 stage filter,for FPGA

    //rx_cont_mode
    write_reg8(0x170420, 0xc8);

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table
    write_reg8(0x1704c2, 0x3b); //grx_0.
    write_reg8(0x1704c3, 0x4c); //grx_1.
    write_reg8(0x1704c4, 0x58); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x08); //PN.
    write_reg8(0x170002, 0x44); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (2:4mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (2:4mbps)
    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN; //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x12);
    // write_reg8(0x170216,0x29);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <4>:CBPF_HIGH_GBW           default:0,->1    Enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * This setting is used to enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * Applies exclusively to 4MHz/6MHz modes; other modes remain disabled by default. 
    * modified by chenxi.wang,confirmed by yuya.hao 20250421.
    */
    reg_rf_reg_sparelv1 |=FLD_RF_CBPF_HIGH_GBW;

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_BLE_4M;
}

/**
 * @brief     This function serves to  set ble_6M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_6M_mode(void)
{
    //aura_2m
    write_reg8(0x17063d, 0x81); //ble:bw_code.100 -> IF = 4.5MHz, BW = 3.5MHz (ZIF, 6MBPS)
    write_reg8(0x170620, 0x00); //sc_code. 00 = IF of 4500MHz (6MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x03(IF:1MHz->4.5MHz,BW:1MHz->3.5MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x03 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0xce); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x32); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.0x20->0x1e


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb, 0x00); //2 stage filter,for FPGA

    //rx_cont_mode
    write_reg8(0x170420, 0xc8);

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
    write_reg8(0x170423, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    write_reg8(0x17043d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); //tx_tp_align.

    //agc_table
    write_reg8(0x1704c2, 0x3b); //grx_0.
    write_reg8(0x1704c3, 0x4c); //grx_1.
    write_reg8(0x1704c4, 0x58); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x08); //PN.
    write_reg8(0x170002, 0x46); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe1); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5>

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.

    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
    write_reg8(0x17043e, 0x81); //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014, 0x7a); //access code for hybee 500K.
    write_reg8(0x170015, 0x35); //access code for hybee 500K.
    write_reg8(0x17043b, 0x1c); //ZB_NUM_GEAR_H
    write_reg8(0x170132, 0x01); //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (3:6mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (3:6mbps)
    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW; //When running 48MHz, you can turn off 1m/2m at 24MHz by overwrite; preventing interference with 4/6MHz at 48Mhz.
    write_reg8(0x17051a, 0x20);               //sync threshold:TO MODEM (HP),Function is the same as 0x17044e.The hp mode uses this threshold.
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN;  //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x11);               //HIGH_RATE_CTRL
    // write_reg8(0x170216,0x29);//tx en delay

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <4>:CBPF_HIGH_GBW           default:0,->1    Enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * This setting is used to enable the analog filter HIGH-GBW (Gain-Bandwidth Product) mode.
    * Applies exclusively to 4MHz/6MHz modes; other modes remain disabled by default. 
    * modified by chenxi.wang,confirmed by yuya.hao 20250421.
    */
    reg_rf_reg_sparelv1 |=FLD_RF_CBPF_HIGH_GBW;
    
    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_BLE_6M;
}

/**
  * @brief      This function serves to start Rx of auto mode. In this mode,
  *             RF module stays in Rx status until a packet is received or it fails to receive packet when timeout expires.
  *             Timeout duration is set by the parameter "tick".
  *             The address to store received data is set by the function "addr".
  * @param[in]  addr   - The address to store received data.
  * @param[in]  tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
  * @return     none
  * @note       addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
  */
void rf_start_brx(void *addr, unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(addr));
    reg_rf_ll_rx_fst_timeout = 0x0fffffff;
    reg_rf_ll_cmd_schedule   = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x82;                        // ble rx.
}

/**
  * @brief      This function serves to start tx of auto mode. In this mode,
  *             RF module stays in tx status until a packet is sent or it fails to sent packet when timeout expires.
  *             Timeout duration is set by the parameter "tick".
  *             The address to store send data is set by the function "addr".
  * @param[in]  addr   - The address to store send data.
  * @param[in]  tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
  * @return     none.
  * @note       addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
  */
void rf_start_btx(void *addr, unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x81;                        // ble tx.
}
