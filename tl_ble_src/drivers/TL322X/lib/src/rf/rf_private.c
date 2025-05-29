/********************************************************************************************************
 * @file    rf_private.c
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
#include "lib/include/rf/rf_private.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"

/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

/**
 * @brief     This function serves to  set pri_250K  mode of RF.
 * @return    none.
 */
void rf_set_pri_250K_mode(void)
{
    /* note: TPLL do not support 250K_mode
 * if want TPLL support 250K_mode, should change register 0x170004 value from 0xf3 to 0xf2.
 */
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
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f, 0x12); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420, 0xc8); // script cc.

    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.
    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01. /***** note:this register'value is 0x00, and script's value is 0x8C, it doesn't seem to matter. *****/
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
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x41); //preamble len.
    write_reg8(0x170003, 0x55); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf3); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.


    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem.
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRIVATE_250K;
}

/**
 * @brief     This function serves to  set pri_500K  mode of RF.
 * @return    none.
 */
void rf_set_pri_500K_mode(void)
{
    /* note: TPLL do not support 500K_mode
 * if want TPLL support 500K_mode, should change register 0x170004 value from 0xf3 to 0xf2.
 */
    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30);        //sc_code.11 = IF of 1000MHz (1MBPS mode)
    /*
    *       bit                 default     value                note
    *                                                            note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x20);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23);        //HPMC_EXP_DIFF_COUNT_H.

    write_reg8(0x17063f, 0x0e);        //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x00);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e);        //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01);        //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c);        //modem:ZIGBEE_MODE:01. /***** note:this register'value is 0x00, and script's value is 0x8c, it doesn't seem to matter. *****/
    write_reg8(0x170423, 0x00);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10);        //modem:disable MSK.
    write_reg8(0x17043d, 0x00);        //modem:zb_sfd_frm_ll.
    write_reg8(0x17042c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01);        //TOT_DEV_RST.

    write_reg8(0x17049a, 0x00);        //tx_tp_align.
    write_reg8(0x1704c2, 0x3e);        //grx_0.
    write_reg8(0x1704c3, 0x4b);        //grx_1.
    write_reg8(0x1704c4, 0x56);        //grx_2.
    write_reg8(0x1704c5, 0x63);        //grx_3.
    write_reg8(0x1704c6, 0x6e);        //grx_4.
    write_reg8(0x1704c7, 0x7a);        //grx_5.
    write_reg8(0x1704c8, 0x39);        //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f);        //tx_mode.
    write_reg8(0x170001, 0x00);        //PN.
    write_reg8(0x170002, 0x41);        //preamble len.
    write_reg8(0x170003, 0x57);        //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf3);        //bit<4>mode:1->1m;bit<0:3>:ble head
    write_reg8(0x170005, 0x04);        //lr mode bit<4:5>

    write_reg32(0x170008, 0xf8118ac9); //access code for zigbee 250K.

    write_reg8(0x170021, 0xa1);        //rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        //rxchn_man_en.
    write_reg8(0x17044c, 0x0c);        //RX:acc_len modem.
    write_reg8(0x1704bb, 0x00);        //disable 2 stage filter
    write_reg8(0x17043e, 0x81);        //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRIVATE_500K;
}

/**
 * @brief     This function serves to  set pri_1M  mode of RF.
 * @return    none.
 */
void rf_set_pri_1M_mode(void)
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
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    //  rx_cont_mode

    write_reg8(0x170420, 0xc8); // script cc.

    //aura_1m
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x17044c, 0x0c);

    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x42); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control.
    write_reg8(0x170004, 0xf2); //bit<4>mode:1->1m;bit<0:2>:ble
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.


    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
    write_reg8(0x170022, 0x00); //rxchn_man_en.
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRIVATE_1M;

    /* add note:
     * if TX_device want to send packet without CRC (crc_len = 0), can use rf_tx_hw_crc_dis()
     * API forbid use CRC if CRC is enable, do not change rf_crc_config's crc_len equal 0.
     * if RX_device want to receive packet without CRC (crc_len = 0), should change rf_crc_config's
     * crc_len equal 0, and make sure CRC is enable.
     */
}

/**
 * @brief     This function serves to  set pri_2M  mode of RF.
 * @return    none.
 */
void rf_set_pri_2M_mode(void)
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
    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420, 0xc8); // script cc.
    write_reg8(0x1704bb, 0x00); //2 stage

    write_reg8(0x170422, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.
    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3>
    write_reg8(0x170004, 0xe2); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.
                                //  write_reg32(0x170008,0xf8118ac9);//access code for zigbee 250K.

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
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRIVATE_2M;
}

/**
 * @brief     This function serves to  set pri_4M  mode of RF.
 * @return    none.
 */
void rf_set_pri_4M_mode(void)
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
    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420, 0xc8); // script cc.
    write_reg8(0x1704bb, 0x00); //2 stage

    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.
    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x36); //grx_0.
    write_reg8(0x1704c3, 0x48); //grx_1.
    write_reg8(0x1704c4, 0x54); //grx_2.
    write_reg8(0x1704c5, 0x62); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x79); //grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x44); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3>
    write_reg8(0x170004, 0xe2); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.
                                //  write_reg32(0x170008,0xf8118ac9);//access code for zigbee 250K.

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
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (0:1mbps)

    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN; //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x12);
    //    write_reg8(0x170216,0x19);
    //private mdm bit-dly for BB crc calculate ACCESS
    write_reg8(0x17051d, 0xb2); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0xc7);

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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRIVATE_4M;
}

/**
 * @brief     This function serves to  set pri_6M  mode of RF.
 * @return    none.
 */
void rf_set_pri_6M_mode(void)
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
    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420, 0xc8); // script cc.
    write_reg8(0x1704bb, 0x00); //2 stage

    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.
    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x36); //grx_0.
    write_reg8(0x1704c3, 0x48); //grx_1.
    write_reg8(0x1704c4, 0x54); //grx_2.
    write_reg8(0x1704c5, 0x62); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x79); //grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x46); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3>
    write_reg8(0x170004, 0xe2); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.
                                //  write_reg32(0x170008,0xf8118ac9);//access code for zigbee 250K.

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
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN; //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x11);
    //    write_reg8(0x170216,0x19);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x72); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0xa7);

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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRIVATE_6M;
}

/**
 * @brief       This function serves to set pri sb mode enable.
 * @return      none.
 */
void rf_private_sb_en(void)
{
    reg_rf_format = ((reg_rf_format & (~FLD_RF_HEAD_MODE)) | 0x03);
}

/**
 * @brief       This function serves to set pri sb mode payload length.
 * @param[in]   pay_len  - In private sb mode packet payload length.
 * @return      none.
 */
void rf_set_private_sb_len(int pay_len)
{
    reg_rf_sblen = ((reg_rf_sblen & 0x00) | pay_len);
}

/**
 * @brief   This function serve to set access code.This function will first get the length of access code from register 0x170005
 *          and then set access code in addr.
 * @param[in]   pipe_id -The number of pipe.0<= pipe_id <=7.
 * @param[in]   acc -The value access code
 * @note        For compatibility with previous versions the access code should be bit transformed by bit_swap();
 */
void rf_set_pipe_access_code(unsigned int pipe_id, unsigned char *addr)
{
    unsigned char i       = 0;
    unsigned char acc_len = read_reg8(0x170005) & 0x07;

    switch (pipe_id) {
    case 0:
    case 1:
        for (i = 0; i < acc_len; i++) {
            write_reg8(reg_rf_access_code_base_pipe0 + i + (pipe_id * 5), addr[i]);
        }
        break;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        write_reg8(reg_rf_access_code_base_pipe0 + (pipe_id * 2 + 6), addr[0]);
        write_reg8(reg_rf_access_code_base_pipe0 + (pipe_id * 2 + 7), addr[1]);
        for (i = 2; i < acc_len; i++) {
            write_reg8(reg_rf_access_code_base_pipe0 + i + 5, addr[i]);
        }
        break;
    default:
        break;
    }
}

/**
  * @brief     This function serves to set ant  mode of RF.
  * @return    none.
  */
void rf_set_ant_mode(void)
{
    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30); //sc_code 11 = IF of 1000MHz (1MBPS mode)
    /*
    *       bit                 default     value                note
    *                                                            note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420, 0xc8); // script cc.

    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,1MBPS

    write_reg8(0x17044e, 0x0f); //sync threshold:TO MODEM  access_code threshold
    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x00); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x0f); //tx_mode
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x42); //preamble length
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xf3); //bit<4>mode:1->1m;bit<0:3>:private head.
    write_reg8(0x170005, 0x02); //lr mode bit<4:5> 0:off,3:125k,2:500k.bit<0:2> TX:acc_len

    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0a); //bit<0:2> RX:acc_len modem
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_ANT;
}

/**
 * @brief     This function serves to set pri_generic_250K  mode of RF.
 * @return    none.
 */
void rf_set_pri_generic_250K_mode(void)
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
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.


    write_reg8(0x17063f, 0x12); //250k modulation index:telink add rx for 250k/500k.

    //  rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc.

    //aura_1m
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    //  new_generic_1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x42); //preamble len.
    //  write_reg8(0x170003,0x55);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170003, 0x55); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xf4); //bit<4>mode:1->1m;bit<0:2>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.


    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem, crc_en<bit3>
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRI_GENERIC_250K;
}

/**
 * @brief     This function serves to set pri_generic_500K  mode of RF.
 * @return    none.
 */
void rf_set_pri_generic_500K_mode(void)
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
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.


    write_reg8(0x17063f, 0x0e); //250k modulation index:telink add rx for 250k/500k.

    //  rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc.

    //aura_1m
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    //  new_generic_1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x42); //preamble len.
    write_reg8(0x170003, 0x57); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xf4); //bit<4>mode:1->1m;bit<0:2>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.


    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem, crc_en<bit3>
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRI_GENERIC_500K;
}

/**
   * @brief     This function serves to set pri_generic_1M  mode of RF.
   * @return       none.
   */
void rf_set_pri_generic_1M_mode(void)
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
    write_reg8(0x170622, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x23); //HPMC_EXP_DIFF_COUNT_H.


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    //  rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc.

                                //aura_1m
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704c2, 0x3e); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x56); //grx_2.
    write_reg8(0x1704c5, 0x63); //grx_3.
    write_reg8(0x1704c6, 0x6e); //grx_4.
    write_reg8(0x1704c7, 0x7a); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

                                //    new_generic_1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x42); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xf4); //bit<4>mode:1->1m;bit<0:2>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.


    write_reg8(0x170021, 0xa1); //rx packet len 0 enable.
    write_reg8(0x170022, 0x00); //rxchn_man_en.
    write_reg8(0x17044c, 0x0c); //RX:acc_len modem, crc_en<bit3>
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRI_GENERIC_1M;
}

/**
   * @brief     This function serves to set pri_generic_2M  mode of RF.
   * @return       none.
   */
void rf_set_pri_generic_2M_mode(void)
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


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    //  rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc.

                                //aura_2m
    write_reg8(0x170422, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x1e); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704bb, 0x00); //2 stage filter.
    write_reg8(0x1704c2, 0x40); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x59); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x70); //grx_4.
    write_reg8(0x1704c7, 0x7b); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

                                //    new_generic_1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x43); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe4); //bit<4>mode:1->1m;bit<0:2>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k


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
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x01 << 2); //48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &= (~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a, 0x1d);
    reg_rf_tx_hlen_mode &= (~FLD_RF_TX_VLD_EN); //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d, 0x92); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRI_GENERIC_2M;
}

/**
   * @brief     This function serves to set pri_generic_4M  mode of RF.
   * @return       none.
   */
void rf_set_pri_generic_4M_mode(void)
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


    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    //  rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc.

                                //aura_2m
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704bb, 0x00); //2 stage filter.
    write_reg8(0x1704c2, 0x40); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x59); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x70); //grx_4.
    write_reg8(0x1704c7, 0x7b); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

                                //    new_generic_1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x44); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe4); //bit<4>mode:1->1m;bit<0:2>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k


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
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x02 << 2); //48M RF demod rate sel (0:1mbps)
    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN; //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x12);
    //    write_reg8(0x170216,0x19);

    //private mdm bit-dly for BB crc calculate ACCESS
    write_reg8(0x17051d, 0xb2); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0xc7);

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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRI_GENERIC_4M;
}

/**
   * @brief     This function serves to set pri_generic_6M  mode of RF.
   * @return       none.
   */
void rf_set_pri_generic_6M_mode(void)
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

    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    //  rx_cont_mode
    write_reg8(0x170420, 0xc8); // script cc.

                                //aura_2m
    write_reg8(0x170422, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e, 0x20); //ble sync threshold:To modem.

    write_reg8(0x17044d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x170421, 0x8c); //modem:ZIGBEE_MODE:01.
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
    write_reg8(0x1704bb, 0x00); //2 stage filter.
    write_reg8(0x1704c2, 0x40); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x59); //grx_2.
    write_reg8(0x1704c5, 0x64); //grx_3.
    write_reg8(0x1704c6, 0x70); //grx_4.
    write_reg8(0x1704c7, 0x7b); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

                                //    new_generic_1m_setup
    write_reg8(0x170000, 0x0f); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
    write_reg8(0x170002, 0x46); //preamble len.
    write_reg8(0x170003, 0x54); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe4); //bit<4>mode:1->1m;bit<0:2>:ble head.
    write_reg8(0x170005, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k


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
    reg_rf_hshp_ctrl_0 = (reg_rf_hshp_ctrl_0 & (~FLD_RF_RXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 = (reg_rf_hshp_ctrl_1 & (~FLD_RF_TXC_MODE_SEL)) | (0x03 << 2); //48M RF demod rate sel (0:1mbps)
    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 |= FLD_RF_RXC_MODE_OW;
    write_reg8(0x17051a, 0x20);
    reg_rf_tx_hlen_mode |= FLD_RF_TX_VLD_EN;  //r_tx_vld_en :tx vld output en
    write_reg8(0x170026, 0x11);
    //    write_reg8(0x170216,0x19);

    //private mdm bit-dly for BB crc calculate ACCESS
    write_reg8(0x17051d, 0x72); //bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e, 0xa7);
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

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_PRI_GENERIC_6M;
}

/**
  * @brief     This function is used to set a fixed offset for the extracted length field.
  * @param[in] length_adj  - The fixed offset for the extracted length field.
  *                          Length_adj range:-31 <=  length_adj <=31
  * @return    none.
  * @note      If length_adj is configured,the sum of length + length_adj represents the number of payload + crc octets.
  *            1. length_adj configuration positive number, CRC length (byte)
  *            2. Configuration 0, no CRC
  *            3. Configure negative numbers, no CRC, and the payload length is less than the corresponding value
  */
void rf_set_pri_generic_length_adj(signed char length_adj)
{
    if (length_adj < 0) {
        length_adj = ~length_adj + 1;
        write_reg8(0x17013d, ((read_reg8(0x17013d) & 0xc0) | 0x20) | length_adj);
    } else {
        write_reg8(0x17013d, (read_reg8(0x17013d) & 0xc0) | length_adj);
    }
}

/**
  * @brief      This function set the packet filter.
  * @param[in]  rf_pkt_flt - RF packet filtering parameters
  * @return     none.
  * @note       1. Filter from high bit to low bit
  *             2. Maximum matching 64bit
  *             3. Interrupt:FLD_RF_IRQ_PKT_MATCH/FLD_RF_IRQ_PKT_UNMATCH
  */
void rf_set_pkt_filter(rf_pkt_flt_t rf_pkt_flt)
{
    reg_rf_pkt_flt_start       = rf_pkt_flt.rf_pkt_flt_start;        //starting byte
    reg_rf_pkt_flt_end         = rf_pkt_flt.rf_pkt_flt_end;          //ending byte
    reg_rf_pkt_match_threshold = rf_pkt_flt.rf_pkt_match_threshold;  //Range of matches,In bits
    reg_rf_pkt_flt_match_l     = rf_pkt_flt.rf_pkt_match_low;        //rf_pkt_match_low
    reg_rf_pkt_flt_match_h     = rf_pkt_flt.rf_pkt_match_high;       //rf_pkt_match_high
    reg_rf_pkt_flt_mask_l      = rf_pkt_flt.rf_pkt_mask_low;         //rf_pkt_mask_low
    reg_rf_pkt_flt_mask_h      = rf_pkt_flt.rf_pkt_mask_high;        //rf_pkt_mask_high
    reg_rf_pkt_flt_cntl |= (FLD_RF_PKT_FLT_EN | FLD_RF_FLT_BYTE_EN); // pkt_flt setup
}

/**
  * @brief      This function disable the packet filter.
  * @return     none.
  */
void rf_dis_pkt_filter(void)
{
    reg_rf_pkt_flt_cntl &= ~(FLD_RF_PKT_FLT_EN);
}

/**
  * @brief     This function is used to set the size of the H0 field in the header of a generic packet.
  * @param[in] h0_size     - The size of H0 field in bits.(0 <=  h0_size <= 16)
  * @return          none.
  */
void rf_set_pri_generic_header_h0_size(unsigned char h0_size)
{
    write_reg8(0x170138, h0_size); //H0 field
}

/**
  * @brief     This function is used to set the size of the H1 field in the header of a generic packet.
  * @param[in] h1_size      - The size of H1 field in bits.(0 <=  h0_size <= 16)
  * @return    none.
  */
void rf_set_pri_generic_header_h1_size(unsigned char h1_size)
{
    write_reg8(0x170139, h1_size); //H1 field
}

/**
  * @brief     This function is used to set the size of the length field in the header of a generic packet.
  * @param[in] length_size            - The size of length field in bits. (0 <=  length_size <= 16)
  * @return    none.
  * @note      If length is present (non-zero size), its value determines the number of octetsremaining in the packet after the header is complete.
  *            That is, payload octets + crc octets.
  */
void rf_set_pri_generic_header_length_size(unsigned char length_size)
{
    write_reg8(0x17013a, (read_reg8(0x17013a) & 0xe0) | length_size); //LENGTH field
}

/**
  * @brief     This function is used to set the size of each field in the header of a generic packet.
  * @param[in] h0_size      - The size of H0 field in bits.(0 <=  h0_size <= 16)
  * @param[in] length_size  - The size of length field. (0 <=  length_size <= 16)
  * @param[in] h1_size      - The size of H1 field in bits.(0 <=  h1_size <= 16)
  * @return    none.
  * @note      Attention:The sum of the sizes (in bits) of H0, LENGTH and H1 must be an integer multiple of 8 bits.
  */
void rf_set_pri_generic_header_size(unsigned char h0_size, unsigned char length_size, unsigned char h1_size)
{
    rf_set_pri_generic_header_h0_size(h0_size);
    rf_set_pri_generic_header_length_size(length_size);
    rf_set_pri_generic_header_h1_size(h1_size);
}

/**
  * @brief     This function is used to set the PID position in the header field.
  * @param[in] pid_start_bit  - The bit in the header field starting with the PID.
  *            (0 is the first bit of header)
  * @return    none.
  */
void rf_set_pri_generic_pid_start_bit(unsigned char pid_start_bit)
{
    write_reg8(0x17013b, (read_reg8(0x17013b) & 0xc0) | pid_start_bit);
}

/**
  * @brief     This function serves to enable the 2-bit PID in the header field.
  * @param[in] none.
  * @return    none.
  */
void rf_set_pri_generic_pid_en(void)
{
    write_reg8(0x17013b, read_reg8(0x17013b) | 0x80);
}

/**
  * @brief     This function is used to set the no_ack position in the header field.
  * @param[in] noack_start_bit  - The bit in the header field starting with the no_ack.
  *            (0 is the first bit of header)
  * @return    none.
  */
void rf_set_pri_generic_noack_start_bit(unsigned char noack_start_bit)
{
    write_reg8(0x17013c, (read_reg8(0x17013c) & 0xc0) | noack_start_bit);
}

/**
  * @brief     This function serves to enable the 2-bit no_ack in the header field.
  * @param[in] none.
  * @return    none.
  */
void rf_set_pri_generic_noack_en(void)
{
    write_reg8(0x17013c, read_reg8(0x17013c) | 0x80);
}
