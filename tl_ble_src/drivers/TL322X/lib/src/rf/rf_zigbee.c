/********************************************************************************************************
 * @file    rf_zigbee.c
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
#include "lib/include/pm/pm.h"
#include "compiler.h"


/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/
#define RF_ZIGBEE_OLD_DATA_PATH 0

void rf_set_zigbee_250K_mode(void)
{
    write_reg8(0x170000, 0x03); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // 2 stage filter
    write_reg8(0x1704c2, 0x3f);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x56);        // grx_2.
    write_reg8(0x1704c5, 0x61);        // grx_3.
    write_reg8(0x1704c6, 0x6c);        // grx_4.
    write_reg8(0x1704c7, 0x7b);        // grx_5.
    write_reg8(0x1704c8, 0x39);        // default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); // ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); // dciq edr  auto
    write_reg8(0x170451, 0x1f); // edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); // sync_thd
    write_reg8(0x1704e1, 0xd9); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); // pdet_hardec_thd
#else
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); // ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); // dciq edr  auto
    write_reg8(0x170451, 0x1e); // edr dcoc auto
    write_reg8(0x1704e0, 0x90); // sync_thd
    write_reg8(0x1704e1, 0x19); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); // pdet_hardec_thd
#endif

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.


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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_ZIGBEE_250K;
}

/**
 * @brief     This function serves to set hybee_1M  mode of RF.
 * @return     none.
 */
void rf_set_hybee_1M_mode(void)
{
    write_reg8(0x170000, 0x07); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x3f);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x56);        // grx_2.
    write_reg8(0x1704c5, 0x61);        // grx_3.
    write_reg8(0x1704c6, 0x6c);        // grx_4.
    write_reg8(0x1704c7, 0x7b);        // grx_5.
    write_reg8(0x1704c8, 0x39);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); // ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); // dciq edr  auto
    write_reg8(0x170451, 0x1f); // edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); // sync_thd
    write_reg8(0x1704e1, 0xd9); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); // pdet_hardec_thd
#else
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); // ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); // dciq edr  auto
    write_reg8(0x170451, 0x1e); // edr dcoc auto
    write_reg8(0x1704e0, 0x90); // sync_thd
    write_reg8(0x1704e1, 0x19); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); // pdet_hardec_thd
#endif

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M;
}

/**
 * @brief     This function serves to set hybee_2M  mode of RF.
 * @return     none.
 */
void rf_set_hybee_2M_mode(void)
{
    write_reg8(0x170000, 0x0b); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x3f);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x56);        // grx_2.
    write_reg8(0x1704c5, 0x61);        // grx_3.
    write_reg8(0x1704c6, 0x6c);        // grx_4.
    write_reg8(0x1704c7, 0x7b);        // grx_5.
    write_reg8(0x1704c8, 0x39);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); // ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); // dciq edr  auto
    write_reg8(0x170451, 0x1f); // edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); // sync_thd
    write_reg8(0x1704e1, 0xd9); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); // pdet_hardec_thd
#else
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); // ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); // dciq edr  auto
    write_reg8(0x170451, 0x1e); // edr dcoc auto
    write_reg8(0x1704e0, 0x90); // sync_thd
    write_reg8(0x1704e1, 0x19); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); // pdet_hardec_thd
#endif

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M;
}

/**
 * @brief     This function serves to set hybee_500K  mode of RF.
 * @return     none.
 */
void rf_set_hybee_500K_mode(void)
{
    write_reg8(0x170000, 0x13); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x3f);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x56);        // grx_2.
    write_reg8(0x1704c5, 0x61);        // grx_3.
    write_reg8(0x1704c6, 0x6c);        // grx_4.
    write_reg8(0x1704c7, 0x7b);        // grx_5.
    write_reg8(0x1704c8, 0x39);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); // ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); // dciq edr  auto
    write_reg8(0x170451, 0x1f); // edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); // sync_thd
    write_reg8(0x1704e1, 0xd9); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); // pdet_hardec_thd
#else
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); // modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); // Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); // Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); // ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); // dciq edr  auto
    write_reg8(0x170451, 0x1e); // edr dcoc auto
    write_reg8(0x1704e0, 0x90); // sync_thd
    write_reg8(0x1704e1, 0x19); // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); // pdet_hardec_thd
#endif

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32
    // 2-byte SFD setup
    write_reg8(0x170134, 0x40); // r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K;
}

/**
 * @brief     This function serves to  set hybee_1M_old  mode of RF.
 * @return   none.
 * @note      TODO:This function interface is not available at this time,and will be updated in subsequent releases.(unverified)
 */
void rf_set_hybee_1M_old_mode(void)
{
    write_reg8(0x170000, 0x07); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x04);        // bit<0:1>private mode control. bit<2:3> tx mode.old tx h1m xmode
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x00008200); // access code for zigbee
    write_reg32(0x170010, 0x009500d3); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x03);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0x8d);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c);        //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14);        //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x0d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif

    //The following registers are configured in SFD, which maintains the register defaults
    //2-byte SFD setup
    write_reg8(0x170134, 0x40); //r_zb_sfd_length
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M_OLD;
}

/**
 * @brief     This function serves to  set hybee_2M_old  mode of RF.
 * @return   none.
 * @note      TODO:This function interface is not available at this time,and will be updated in subsequent releases.(unverified)
 */
void rf_set_hybee_2M_old_mode(void)
{
    write_reg8(0x170000, 0x0b); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x44);        // bit<0:1>private mode control. bit<2:3> tx mode.old tx hybee xmode
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x03);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0x8d);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c);        //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14);        //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x0d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif

    //The following registers are configured in SFD, which maintains the register defaults
    //2-byte SFD setup
    write_reg8(0x170134, 0x40); //r_zb_sfd_length
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M_OLD;
}

/**
 * @brief     This function serves to  set hybee_500K_new  mode of RF.
 * @return   none.
 * @note      TODO:This function interface is not available at this time,and will be updated in subsequent releases.(unverified)
 */
void rf_set_hybee_500K_new_mode(void)
{
    write_reg8(0x170000, 0x13); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.old tx hybee xmode
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x05);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0xad);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c);        //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14);        //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x2d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif

    //The following registers are configured in SFD, which maintains the register defaults
    //2-byte SFD setup
    write_reg8(0x170134, 0x40); //r_zb_sfd_length
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K_NEW;
}

/**
 * @brief     This function serves to  set hybee_1M_new  mode of RF.
 * @return    none.
 */
void rf_set_hybee_1M_new_mode(void)
{
    write_reg8(0x170000, 0x07); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.old tx hybee xmode
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x05);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0xad);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c);        //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14);        //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x2d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
    //The following registers are configured in SFD, which maintains the register defaults
    //2-byte SFD setup
    write_reg8(0x170134, 0x40); //r_zb_sfd_length
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M_NEW;
}

/**
 * @brief     This function serves to  set hybee_2M_new  mode of RF.
 * @return    none.
 */
void rf_set_hybee_2M_new_mode(void)
{
    write_reg8(0x170000, 0x0b); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000000a7); // access code for zigbee
    write_reg32(0x17000c, 0x0000d100); // access code for zigbee
    write_reg32(0x170010, 0x00950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x00);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x05);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0xad);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c);        //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14);        //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x2d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
    //The following registers are configured in SFD, which maintains the register defaults
    //2-byte SFD setup
    write_reg8(0x170134, 0x40); //r_zb_sfd_length
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M_NEW;
}

/**
 * @brief     This function serves to  set hybee_500K_2byte_sfd mode of RF.
 * @return    none.
 */
void rf_set_hybee_500K_2byte_sfd_mode(void)
{
    write_reg8(0x170000, 0x13); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000035a7); // access code for zigbee
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

    //2-byte SFD setup
    write_reg8(0x170134, 0x80); //r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0x9d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x1d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K_2BYTE_SFD;
}

/**
 * @brief     This function serves to  set hybee_1M_2byte_sfd mode of RF.
 * @return    none.
 */
void rf_set_hybee_1M_2byte_sfd_mode(void)
{
    write_reg8(0x170000, 0x07); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000035a7); // access code for zigbee
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

    //2-byte SFD setup
    write_reg8(0x170134, 0x80); //r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0x9d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x1d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif

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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M_2BYTE_SFD;
}

/**
 * @brief     This function serves to  set hybee_2M_2byte_sfd mode of RF.
 * @return    none.
 */
void rf_set_hybee_2M_2byte_sfd_mode(void)
{
    write_reg8(0x170000, 0x0b); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000035a7); // access code for zigbee
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x170021, 0x27);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x01);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

    //2-byte SFD setup
    write_reg8(0x170134, 0x80); //r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0x9d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x1d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M_2BYTE_SFD;
}

/**
 * @brief     This function serves to  set hybee_2M_2byte_sfd_new of RF.
 * @return    none.
 */
void rf_set_hybee_2M_2byte_sfd_new_mode(void)
{
    write_reg8(0x170000, 0x0b); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000035a7); // access code for zigbee
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x05);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

    //2-byte SFD setup
    write_reg8(0x170134, 0x80); //r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0xbd); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x3d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M_2BYTE_SFD_NEW;
}

/**
 * @brief     This function serves to  set hybee_1M_2byte_sfd_new of RF.
 * @return    none.
 */
void rf_set_hybee_1M_2byte_sfd_new_mode(void)
{
    write_reg8(0x170000, 0x07); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000035a7); // access code for zigbee
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x05);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

    //2-byte SFD setup
    write_reg8(0x170134, 0x80); //r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0xbd); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x3d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M_2BYTE_SFD_NEW;
}

/**
 * @brief     This function serves to  set hybee_500K_2byte_sfd_new of RF.
 * @return    none.
 */
void rf_set_hybee_500K_2byte_sfd_new_mode(void)
{
    write_reg8(0x170000, 0x13); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x000035a7); // access code for zigbee
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x170021, 0x23);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x05);        // zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17042a, 0x10);        // modem:disable MSK.
    write_reg8(0x170436, 0xb7);        // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d, 0x01);        // modem:zb_sfd_frm_ll.
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044d, 0x0f);        // r_rxchn_en_i:To modem.
    write_reg8(0x170473, 0x01);        // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00);        // tx_tp_align.
    write_reg8(0x1704bb, 0x20);        // disable 2 stage filter
    write_reg8(0x1704c2, 0x36);        // grx_0.
    write_reg8(0x1704c3, 0x48);        // grx_1.
    write_reg8(0x1704c4, 0x54);        // grx_2.
    write_reg8(0x1704c5, 0x62);        // grx_3.
    write_reg8(0x1704c6, 0x6e);        // grx_4.
    write_reg8(0x1704c7, 0x79);        // grx_5.
    write_reg8(0x1704c8, 0x00);        // default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20);        // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x17063d, 0x21);        // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00);        // 250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW

    //2-byte SFD setup
    write_reg8(0x170134, 0x80); //r_zb_sfd_length
    write_reg8(0x17043f, 0x00); // LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421, 0xbd); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x1f); //edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x40); //sync_thd
    write_reg8(0x1704e1, 0xd9); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd
#else
    write_reg8(0x170421, 0x3d); //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x17043b, 0x5c); //Rx: sfd match symb0 num
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x170450, 0x3f); //dciq edr auto
    write_reg8(0x170451, 0x1e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K_2BYTE_SFD_NEW;
}

/**
 * @brief     This function serves to  set zigbee_hr_2m of RF.
 * @return    none.
 */
void rf_set_zigbee_hr_2m_mode(void)
{
    write_reg8(0x170000, 0x0f); // tx_mode.
    write_reg8(0x170001, 0x00); // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002, 0x42);        // preamble len.
    write_reg8(0x170003, 0x54);        // bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        // lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x170008, 0x5555370b); // access code for zigbee
    write_reg8(0x170021, 0xa1);        // rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        // rxchn_man_en.
    write_reg8(0x170132, 0x03);        // zb_phr_extend_en
    write_reg8(0x170132, 0x03);
    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170421, 0x8c);        // modem:ZIGBEE_MODE:01. enable RX mode
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170422, 0x01); // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x00); // modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x170426, 0x00); // modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a, 0x10); // modem:disable MSK.
    write_reg8(0x17042c, 0x38); // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436, 0xb7); // LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); // LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); // LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71); // LR_TIM_REC_CFG_1.
    //sfd from ll
    write_reg8(0x17043d, 0x00); // modem:zb_sfd_frm_ll.
    write_reg8(0x17044c, 0x0c); // RX:acc_len modem.
    write_reg8(0x17044d, 0x01); // r_rxchn_en_i:To modem.
    write_reg8(0x17044e, 0x1e); // ble sync threshold:To modem.
    write_reg8(0x170473, 0x01); // TOT_DEV_RST.
    write_reg8(0x17049a, 0x00); // tx_tp_align.
    write_reg8(0x1704c2, 0x36); // grx_0.
    write_reg8(0x1704c3, 0x48); // grx_1.
    write_reg8(0x1704c4, 0x54); // grx_2.
    write_reg8(0x1704c5, 0x62); // grx_3.
    write_reg8(0x1704c6, 0x6e); // grx_4.
    write_reg8(0x1704c7, 0x79); // grx_5.
    write_reg8(0x1704c8, 0x00); //default:0x00->0x39 Gain offset to compensate system error
    write_reg8(0x170620, 0x20); // sc_code.10 = IF of 1500MHz (2MBPS mode)
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43); // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26); // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063d, 0x21); // ble:bw_code.001 -> IF = 1.5MHz, BW = 1308kHz) (LIF, 2MBPS)
    write_reg8(0x17063f, 0x00); // 250k modulation index:telink add rx for 250k/500k.

    //The following registers are configured in other zigbee config.
    write_reg32(0x17000c, 0x0035d100); // access code for zigbee
    write_reg32(0x170010, 0x35950000); // access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x7a);        // access code for hybee 500K.
    write_reg8(0x170015, 0x35);        // access code for hybee 500K.
    write_reg8(0x17043b, 0x1c);        // Rx: sfd match symb0 num
    write_reg8(0x17043e, 0x81);        // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x170450, 0x3f);        // dciq edr  auto
    write_reg8(0x170451, 0x1e);        // edr dcoc auto
    write_reg8(0x1704bb, 0x00);        // disable 2 stage filter
    write_reg8(0x1704e0, 0x90);        // sync_thd
    write_reg8(0x1704e1, 0x19);        // sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18);        // pdet_hardec_thd
    write_reg8(0x17062d, 0x00);        // CHNL_NUM switch by sw en
    write_reg8(0x170799, 0x00);        // ZB_FREQ_FIXED_OW
    //The following registers are configured in SFD, which maintains the register defaults
    //2-byte SFD setup
    write_reg8(0x170134, 0x40); //r_zb_sfd_length
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
    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HR_2M;
}

/**
 * @brief      This function serves to enable phr_extend.
 * @return     none.
 */
void rf_hybee_phr_extend_en(void)
{
    write_reg8(0x170132, (read_reg8(0x170132) & 0xfd) | BIT(1));
    write_reg8(0x170421, (read_reg8(0x170421) & 0xbf) | BIT(6));
}

/**
 * @brief      This function serves to disable zigbee/hybee PHR.
 * @return     none.
 */
void rf_hybee_phr_dis(void)
{
    write_reg8(0x170132, read_reg8(0x170132) & (~BIT(0)));
}

/**
 * @brief      This function serves to set_no_phr_payload_len.
 * @param[in]  tx_pkt_length- zigbee/hybee PSDU length(PSDU length range:0~0x7ff)
 * @return     none.
 */
void rf_hybee_set_no_phr_payload_len(unsigned int tx_pkt_length)
{
    write_reg8(0x170133, (tx_pkt_length & 0xff));
    write_reg8(0x170132, (read_reg8(0x170132) & 0x1f) | ((tx_pkt_length & 0x0700) >> 8) << 5);
}

/**
 * @brief      This function serves to set hybee gap length between SFD and PHR in octet.
 * @param[in]  gap_length - zigbee/hybee gap length(Gap length range:0~63)
 * @return     none.
 */
void rf_hybee_set_gap_len(unsigned char gap_length)
{
    write_reg8(0x170134, (read_reg8(0x170134) & 0xc0) | gap_length);
}
