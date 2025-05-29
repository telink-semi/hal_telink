/********************************************************************************************************
 * @file    rf_zigbee.c
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

/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

/**
 * @brief     This function serves to  set zigbee_dcoc  mode of RF.
 * @return    none.
 */
void rf_set_zigbee_250K_mode(void)
{
    write_reg8(0x17063d, 0x41); //ble:bw_code.
    write_reg8(0x170620, 0x00); //sc_code.
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <4:2>:FE_RTRIM_RX          default:0x02->0x03  Front end matching resistor adjustment for RX. (Configured by the rf_rx_performance_mode interface)
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f, 0x00); //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420, 0xc8); // script cc.
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170422, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80); //modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x17044d, 0x0f); //r_rxchn_en_i:To modem.
    write_reg8(0x17042a, 0x10); //modem:disable MSK.
    //sfd from ll
    write_reg8(0x17043d, 0x01); //modem:zb_sfd_frm_ll.

    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); //modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x17042c, 0x3b); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x1704bb, 0x00); //modem:zb_dis_rst_pdet_isfd.
                                /******FPGA agc gain adjust: self test, not public-version******/
    //0X47 nice

    //  write_reg8(0x1705ff,0x7f);  //[b6:b5] : high 11 ,medium 10, low 0X
    //[b4:b0] : dB level --0x0 min, 0x1f max
    //Adjusting the external RF chip during the FPGA stage.
    /***************************************************************/

    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.

    write_reg8(0x17049a, 0x00); //tx_tp_align.
    //agc_table_2m
    if ((g_chip_version == CHIP_VERSION_A0) || (g_chip_version == CHIP_VERSION_A1))
    {
        write_reg8(0x1704c2, 0x40); //grx_0.
        write_reg8(0x1704c3, 0x4b); //grx_1.
        write_reg8(0x1704c4, 0x59); //grx_2.
        write_reg8(0x1704c5, 0x64); //grx_3.
        write_reg8(0x1704c6, 0x70); //grx_4.
        write_reg8(0x1704c7, 0x7b); //grx_5.
    }
    else
    {
        write_reg8(0x1704c2, 0x3e); //grx_0.
        write_reg8(0x1704c3, 0x49); //grx_1.
        write_reg8(0x1704c4, 0x56); //grx_2.
        write_reg8(0x1704c5, 0x63); //grx_3.
        write_reg8(0x1704c6, 0x6e); //grx_4.
        write_reg8(0x1704c7, 0x7a); //grx_5.
    }
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x03);        //tx_mode.
    write_reg8(0x170001, 0x00);        //PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3
    * At present, TX adopts the method of pa_ramp starting first and preamble sending later, so the preamble adopts this length setting
    * modified by chenxi.wang,confirmed by wenfeng.lou 20250114.
    */
    write_reg8(0x170002, 0x43);        //preamble len.
    write_reg8(0x170003, 0x54);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008, 0x000000a7); //access code for zigbee
    write_reg32(0x17000c, 0x0000d100); //access code for zigbee
    write_reg32(0x170010, 0x00950000); //access code for hybee 1m. for h2m
    write_reg8(0x170014, 0x2f);        //access code for hybee 500K.
    write_reg8(0x170015, 0x00);        //access code for hybee 500K.

    write_reg8(0x170021, 0x23);        //rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        //rxchn_man_en.
    write_reg8(0x170132, 0x01);
    write_reg8(0x170426, 0x00);
    write_reg8(0x17043e, 0x81);        //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_ZIGBEE_250K;
}

/**
 * @brief     This function serves to set hybee_1M  mode of RF.
 * @return     none.
 */
void rf_set_hybee_1M_mode(void)
{
    write_reg8(0x17063d, 0x41); //ble:bw_code.
    write_reg8(0x170620, 0x00); //sc_code.
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <4:2>:FE_RTRIM_RX          default:0x02->0x03  Front end matching resistor adjustment for RX. (Configured by the rf_rx_performance_mode interface)
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        //HPMC_EXP_DIFF_COUNT_H.

    write_reg8(0x17063f, 0x00);        //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170423, 0x80);        //modem:ZIGBEE_MODE_TX.enable TX mode
    write_reg8(0x170422, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044d, 0x0f);        //r_rxchn_en_i:To modem.

    write_reg8(0x170421, 0x8d);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x17044c, 0x4c);        //Rx: sfd match symb num
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17042c, 0x3b);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x1704bb, 0x00);        //2 stage filter

    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a, 0x10);        //modem:disable MSK.
    write_reg8(0x17043d, 0x01);        //modem:zb_sfd_frm_ll.

    write_reg8(0x170436, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01);        //TOT_DEV_RST.

    write_reg8(0x17049a, 0x00); //tx_tp_align.
    //agc_table_2m
    if ((g_chip_version == CHIP_VERSION_A0) || (g_chip_version == CHIP_VERSION_A1))
    {
        write_reg8(0x1704c2, 0x40); //grx_0.
        write_reg8(0x1704c3, 0x4b); //grx_1.
        write_reg8(0x1704c4, 0x59); //grx_2.
        write_reg8(0x1704c5, 0x64); //grx_3.
        write_reg8(0x1704c6, 0x70); //grx_4.
        write_reg8(0x1704c7, 0x7b); //grx_5.
    }
    else
    {
        write_reg8(0x1704c2, 0x3e); //grx_0.
        write_reg8(0x1704c3, 0x49); //grx_1.
        write_reg8(0x1704c4, 0x56); //grx_2.
        write_reg8(0x1704c5, 0x63); //grx_3.
        write_reg8(0x1704c6, 0x6e); //grx_4.
        write_reg8(0x1704c7, 0x7a); //grx_5.
    }
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error
//default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x07);        //tx_mode.
    write_reg8(0x170001, 0x00);        //PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3
    * At present, TX adopts the method of pa_ramp starting first and preamble sending later, so the preamble adopts this length setting
    * modified by chenxi.wang,confirmed by wenfeng.lou 20250114.
    */
    write_reg8(0x170002, 0x43);        //preamble len.
    write_reg8(0x170003, 0x54);        //bit<0:1>private mode control.
    write_reg8(0x170004, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008, 0x000000a7); //access code for zigbee
    write_reg32(0x17000c, 0x0000d100); //access code for hybee 1m.
    write_reg32(0x170010, 0x00950000); //access code for hybee 2m.
    write_reg8(0x170014, 0x2f);
    write_reg8(0x170015, 0x00);        //access code for hybee 500K.

    write_reg8(0x170021, 0x23);        //rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        //rxchn_man_en.

    write_reg8(0x170132, 0x01);
    write_reg8(0x17043e, 0x81);        //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M;
}

/**
 * @brief     This function serves to set hybee_2M  mode of RF.
 * @return     none.
 */
void rf_set_hybee_2M_mode(void)
{
    write_reg8(0x17063d, 0x41); //ble:bw_code.
    write_reg8(0x170620, 0x00); //sc_code.
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <4:2>:FE_RTRIM_RX          default:0x02->0x03  Front end matching resistor adjustment for RX. (Configured by the rf_rx_performance_mode interface)
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f, 0x00);        //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170423, 0x80);        //modem:ZIGBEE_MODE_TX.enable TX mode
    write_reg8(0x170422, 0x01);        //modem:BLE_MODE_TX,2MBPS.

    write_reg8(0x17044d, 0x0f);        //r_rxchn_en_i:To modem.
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a, 0x10);        //modem:disable MSK.
    write_reg8(0x17043d, 0x01);        //modem:zb_sfd_frm_ll.

    write_reg8(0x170436, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6);        //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01);        //TOT_DEV_RST.

    write_reg8(0x17049a, 0x00); //tx_tp_align.
    //agc_table_2m
    if ((g_chip_version == CHIP_VERSION_A0) || (g_chip_version == CHIP_VERSION_A1))
    {
        write_reg8(0x1704c2, 0x40); //grx_0.
        write_reg8(0x1704c3, 0x4b); //grx_1.
        write_reg8(0x1704c4, 0x59); //grx_2.
        write_reg8(0x1704c5, 0x64); //grx_3.
        write_reg8(0x1704c6, 0x70); //grx_4.
        write_reg8(0x1704c7, 0x7b); //grx_5.
    }
    else
    {
        write_reg8(0x1704c2, 0x3e); //grx_0.
        write_reg8(0x1704c3, 0x49); //grx_1.
        write_reg8(0x1704c4, 0x56); //grx_2.
        write_reg8(0x1704c5, 0x63); //grx_3.
        write_reg8(0x1704c6, 0x6e); //grx_4.
        write_reg8(0x1704c7, 0x7a); //grx_5.
    }
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170421, 0x01);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x17044c, 0x4c);        //Rx: sfd match symb num
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17042c, 0x3b);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x1704bb, 0x20);        //2 stage filter

    write_reg8(0x170000, 0x0b);        //tx_mode.
    write_reg8(0x170001, 0x00);        //PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3
    * At present, TX adopts the method of pa_ramp starting first and preamble sending later, so the preamble adopts this length setting
    * modified by chenxi.wang,confirmed by wenfeng.lou 20250114.
    */
    write_reg8(0x170002, 0x43);        //preamble len.
    write_reg8(0x170003, 0x54);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008, 0x000000a7); //access code for zigbee
    write_reg32(0x17000c, 0x0000d100); //access code for hybee 1m.
    write_reg32(0x170010, 0x00950000); //access code for hybee 2m.
    write_reg8(0x170014, 0x2f);
    write_reg8(0x170015, 0x00);        //access code for hybee 500k.

    write_reg8(0x170021, 0x23);        //rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        //rxchn_man_en.
    write_reg8(0x170132, 0x01);
    write_reg8(0x17043e, 0x81);        //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M;
}

/**
 * @brief     This function serves to set hybee_500K  mode of RF.
 * @return     none.
 */
void rf_set_hybee_500K_mode(void)
{
    write_reg8(0x17063d, 0x41); //ble:bw_code.
    write_reg8(0x170620, 0x00); //sc_code.0x06->0x00
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <4:2>:FE_RTRIM_RX          default:0x02->0x03  Front end matching resistor adjustment for RX. (Configured by the rf_rx_performance_mode interface)
    * <6:5>:IF_FREQ              default:0x00->0x01(IF:1MHz->1.5MHz,BW:1MHz->2MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | (0x01 << 5) | FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622, 0x43);        //HPMC_EXP_DIFF_COUNT_L.0x46->0x43
    write_reg8(0x170623, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f, 0x00);        //250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420, 0xc8);        // script cc.
    write_reg8(0x170422, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423, 0x80);        //modem:ZIGBEE_MODE_TX.enable TX mode
    write_reg8(0x170426, 0x00);        //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17044d, 0x0f);        //r_rxchn_en_i:To modem.
    write_reg8(0x17042a, 0x10);        //modem:disable MSK.
    write_reg8(0x17043d, 0x01);        //modem:zb_sfd_frm_ll.

    write_reg8(0x170421, 0x01);        //modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x17044c, 0x4c);        //Rx: sfd match symb num
    write_reg8(0x17043b, 0x1c);        //Rx: sfd match symb0 num
    write_reg8(0x17042c, 0x3b);        //modem:zb_dis_rst_pdet_isfd.0x39->0x00
    write_reg8(0x1704bb, 0x00);        //2 stage filter

    write_reg8(0x170436, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01);        //TOT_DEV_RST.

    write_reg8(0x17049a, 0x00); //tx_tp_align.
    //agc_table_2m
    if ((g_chip_version == CHIP_VERSION_A0) || (g_chip_version == CHIP_VERSION_A1))
    {
        write_reg8(0x1704c2, 0x40); //grx_0.
        write_reg8(0x1704c3, 0x4b); //grx_1.
        write_reg8(0x1704c4, 0x59); //grx_2.
        write_reg8(0x1704c5, 0x64); //grx_3.
        write_reg8(0x1704c6, 0x70); //grx_4.
        write_reg8(0x1704c7, 0x7b); //grx_5.
    }
    else
    {
        write_reg8(0x1704c2, 0x3e); //grx_0.
        write_reg8(0x1704c3, 0x49); //grx_1.
        write_reg8(0x1704c4, 0x56); //grx_2.
        write_reg8(0x1704c5, 0x63); //grx_3.
        write_reg8(0x1704c6, 0x6e); //grx_4.
        write_reg8(0x1704c7, 0x7a); //grx_5.
    }
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x13);        //tx_mode.
    write_reg8(0x170001, 0x00);        //PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3
    * At present, TX adopts the method of pa_ramp starting first and preamble sending later, so the preamble adopts this length setting
    * modified by chenxi.wang,confirmed by wenfeng.lou 20250114.
    */
    write_reg8(0x170002, 0x43);        //preamble len.
    write_reg8(0x170003, 0x54);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008, 0x000000a7); //access code for zigbee
    write_reg32(0x17000c, 0x0000d100); //access code for hybee 1m.
    write_reg32(0x170010, 0x00950000); //access code for hybee 2m.
    write_reg8(0x170014, 0x2f);        //access code for hybee 500K.
    write_reg8(0x170015, 0x00);        //access code for hybee 500K.

    write_reg8(0x170021, 0x23);        //rx packet len 0 enable.
    write_reg8(0x170022, 0x00);        //rxchn_man_en.
    write_reg8(0x170132, 0x01);        //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043e, 0x81);        //BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K;
}
