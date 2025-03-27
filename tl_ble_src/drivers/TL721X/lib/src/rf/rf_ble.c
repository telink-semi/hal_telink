/********************************************************************************************************
 * @file    rf_ble.c
 *
 * @brief   This is the header file for TL721X
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
#define BLE_S2_S8_OLD_PATH      0   //For internal testing only, the old path will not be released to the public

/**
 * @brief       This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return      none.
 */
void rf_set_ble_chn (signed char chn_num)
{
    signed char ble_chn_num = 0;
    write_reg8 (0x170020, chn_num);

    if (chn_num < 11)
        ble_chn_num = chn_num + 2;

    else if (chn_num < 37)
        ble_chn_num = chn_num + 3;

    else if (chn_num == 37)
        ble_chn_num = 1;

    else if (chn_num == 38)
        ble_chn_num = 13;

    else if (chn_num == 39)
        ble_chn_num = 40;

    else if (chn_num < 51)
        ble_chn_num = chn_num;

    else if(chn_num <= 61)
        ble_chn_num = -61 + chn_num;

    ble_chn_num = ble_chn_num << 1;
    rf_set_chn(ble_chn_num);

}

/**
 * @brief     This function serves to  set ble_1M  mode of RF.
 * @return    none.
 */
void rf_set_ble_1M_mode(void)
{
    //aura_1m
    write_reg8(0x17063d,0x61);//ble:bw_code.
    write_reg8(0x170620,0x10);//sc_code.
    write_reg8(0x170621,0x0a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x20);//RADIO BLE_MODE_TX,1MBPS:bit<0>;VCO_TRIM_KV:bit<1-3>;HPMC_EXP_DIFF_COUNT_L:bit<4-7>.
    write_reg8(0x170623,0x23);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422,0x00);//modem:BLE_MODE_TX,1MBPS.
    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    //rx_cont_mode
    write_reg8(0x170420,0xc8);// script cc. rx continue mode on:bit<3>


    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x17049a,0x00);//tx_tp_align.

    //agc_table_1m
    write_reg8(0x1704c2,0x3b);//grx_0.
    write_reg8(0x1704c3,0x49);//grx_1.
    write_reg8(0x1704c4,0x58);//grx_2.
    write_reg8(0x1704c5,0x66);//grx_3.
    write_reg8(0x1704c6,0x71);//grx_4.
    write_reg8(0x1704c7,0x7b);//grx_5.
    write_reg8(0x1704c8,0x39);//default:0x00->0x39 Gain offset to compensate system error

    //ble1m_setup
    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x08);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x43);//preamble length
    write_reg8(0x170003,0x54);//bit<0:1>private mode control.
    write_reg8(0x170004,0xf1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5>

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.
                              //bit<5>:write packet length filed into sram

    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.0x4c->0x0c
    write_reg8(0x1704bb,0x00);//disable 2 stage filter
    write_reg8(0x17043e,0x81);//BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
    write_reg8(0x17062d,0x00);//CHNL_NUM switch by sw en
    write_reg8(0x170799,0x00);//ZB_FREQ_FIXED_OW

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0,0x1c);//defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

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
    write_reg8(0x17063d,0x61);//ble:bw_code.
    write_reg8(0x170620,0x10);//sc_code.
    write_reg8(0x170621,0x0a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x20);//RADIO BLE_MODE_TX,1MBPS:bit<0>;VCO_TRIM_KV:bit<1-3>;HPMC_EXP_DIFF_COUNT_L:bit<4-7>.
    write_reg8(0x170623,0x23);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422,0x00);//modem:BLE_MODE_TX,1MBPS.
    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    //rx_cont_mode
    write_reg8(0x170420,0xc8);// script cc. rx continue mode on:bit<3>

    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x17049a,0x00);//tx_tp_align.

    //agc_table_1m
    write_reg8(0x1704c2,0x3b);//grx_0.
    write_reg8(0x1704c3,0x49);//grx_1.
    write_reg8(0x1704c4,0x58);//grx_2.
    write_reg8(0x1704c5,0x66);//grx_3.
    write_reg8(0x1704c6,0x71);//grx_4.
    write_reg8(0x1704c7,0x7b);//grx_5.
    write_reg8(0x1704c8,0x39);//default:0x00->0x39 Gain offset to compensate system error

    //ble1m_setup
    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x43);//preamble length
    write_reg8(0x170003,0x54);//bit<0:1>private mode control.
    write_reg8(0x170004,0xf1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5>

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.
                              //bit<5>:write packet length filed into sram

    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.0x4c->0x0c
    write_reg8(0x1704bb,0x00);//disable 2 stage filter
    write_reg8(0x17043e,0x81);//BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
    write_reg8(0x17062d,0x00);//CHNL_NUM switch by sw en
    write_reg8(0x170799,0x00);//ZB_FREQ_FIXED_OW

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0,0x1c);//defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

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
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.0x20->0x1e


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb,0x20);//2 stage filter,

    //rx_cont_mode
    write_reg8(0x170420,0xc8);

    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x17049a,0x00);//tx_tp_align.

    //agc_table
    write_reg8(0x1704c2,0x3d);//grx_0.
    write_reg8(0x1704c3,0x4b);//grx_1.
    write_reg8(0x1704c4,0x5a);//grx_2.
    write_reg8(0x1704c5,0x67);//grx_3.
    write_reg8(0x1704c6,0x73);//grx_4.
    write_reg8(0x1704c7,0x7e);//grx_5.
    write_reg8(0x1704c8,0x39);//default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x08);//PN.

    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.

    write_reg8(0x170003,0x54);//bit<0:1>private mode control.
    write_reg8(0x170004,0xe1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5>

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.

    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.
    write_reg8(0x17043e,0x81);//BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
    write_reg8(0x17062d,0x00);//CHNL_NUM switch by sw en
    write_reg8(0x170799,0x00);//ZB_FREQ_FIXED_OW

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0,0x1c);//defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

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
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.0x20->0x1e


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x1704bb,0x20);//2 stage filter,

    //rx_cont_mode
    write_reg8(0x170420,0xc8);

    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x17049a,0x00);//tx_tp_align.

    //agc_table
    write_reg8(0x1704c2,0x3d);//grx_0.
    write_reg8(0x1704c3,0x4b);//grx_1.
    write_reg8(0x1704c4,0x5a);//grx_2.
    write_reg8(0x1704c5,0x67);//grx_3.
    write_reg8(0x1704c6,0x73);//grx_4.
    write_reg8(0x1704c7,0x7e);//grx_5.
    write_reg8(0x1704c8,0x39);//default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control.
    write_reg8(0x170004,0xe1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5>

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.

    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.
    write_reg8(0x17043e,0x81);//BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
    write_reg8(0x17062d,0x00);//CHNL_NUM switch by sw en
    write_reg8(0x170799,0x00);//ZB_FREQ_FIXED_OW

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0,0x1c);//defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_BLE_2M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_500K  mode of RF.
 * @return    none.
 */
void rf_set_ble_500K_mode(void)
{
    write_reg8(0x170000,0x0f);  // tx_mode.
    write_reg8(0x170001,0x08);  // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:0x0a,->0x0b(10byte->11byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x4b);  // preamble len.
    write_reg8(0x170003,0x54);  // bit<0:1>private mode control.
    write_reg8(0x170004,0xf1);  // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x24);  // lr mode bit<4:5>
    write_reg8(0x170021,0xa1);  // rx packet len 0 enable.
    write_reg8(0x170022,0x00);  // rxchn_man_en.
    write_reg8(0x170420,0xc9);  // script cc.
    write_reg8(0x170422,0x00);  // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423,0x00);  // modem:ZIGBEE_MODE_TX.
    // write_reg8(0x170425,0x01);  // trigger lr access fec
    write_reg8(0x170426,0x00);  // modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);  // modem:disable MSK.
    write_reg8(0x17042c,0x38);  // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170437,0x0c);  // LR_NUM_GEAR_H.
    write_reg8(0x170439,0x7d);  // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d,0x00);  // modem:zb_sfd_frm_ll.
    write_reg8(0x17044c,0x0c);  // RX:acc_len modem.
    write_reg8(0x17044d,0x01);  // r_rxchn_en_i:To modem.
    write_reg8(0x170473,0xa1);  // TOT_DEV_RST.
    write_reg8(0x17049a,0x00);  // tx_tp_align.
    write_reg8(0x1704c2,0x3b);  //grx_0.
    write_reg8(0x1704c3,0x49);  //grx_1.
    write_reg8(0x1704c4,0x58);  //grx_2.
    write_reg8(0x1704c5,0x66);  //grx_3.
    write_reg8(0x1704c6,0x71);  //grx_4.
    write_reg8(0x1704c7,0x7b);  //grx_5.
    write_reg8(0x1704c8,0x39);  // Gain offset to compensate system error
    write_reg8(0x170620,0x10);  // sc_code.
    write_reg8(0x170621,0x0a);  // if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x20);  // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x23);  // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063d,0x61);  // ble:bw_code.
    write_reg8(0x17063f,0x00);  // 250k modulation index:telink add rx for 250k/500k.

#if(BLE_S2_S8_OLD_PATH)
    //old path config
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
    write_reg8(0x170436,0xee);  // LR_NUM_GEAR_L.
    write_reg8(0x170438,0xb8);  // LR_TIM_EDGE_DEV.
    write_reg8(0x17043e,0x81);  // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044e,0xf0);  // ble sync threshold:To modem.
#else
    //new path config
    write_reg8(0x170421,0x8c);//modem:ZIGBEE_MODE:01.//new mdm, new viterbi
    write_reg8(0x170436,0xf6);  // LR_NUM_GEAR_L.
    write_reg8(0x170438,0xb6);  // LR_TIM_EDGE_DEV.
    write_reg8(0x17043e,0x01);  // BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4
    write_reg8(0x17044e,0x3c);  // ble sync threshold:To modem.
#endif

    //The following configurations fix the BLE 125k and BLE 500k PER floor non-zeroing issue.
    //Modified by chenxi.wang, confirmed by yuya.hao at 20240829.
    write_reg8(0x1704f0,0x1e);//defaults 0x1c->0x1e. lr_s8_pdet synv_success threshold 0~32

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
    write_reg8(0x17062d,0x00);//CHNL_NUM switch by sw en
    write_reg8(0x170799,0x00);//ZB_FREQ_FIXED_OW
    write_reg8(0x1704bb,0x00);//disable 2 stage filter

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LR_S2_500K;
}

/**
 * @brief     This function serves to  set ble_125K  mode of RF.
 * @return    none.
 */
void rf_set_ble_125K_mode(void)
{
    write_reg8(0x170000,0x0f);  // tx_mode.
    write_reg8(0x170001,0x08);  // PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:0x0a,->0x0b(10byte->11byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x4b);  // preamble len.
    write_reg8(0x170003,0x54);  // bit<0:1>private mode control.
    write_reg8(0x170004,0xf1);  // bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x34);  // lr mode bit<4:5>
    write_reg8(0x170021,0xa1);  // rx packet len 0 enable.
    write_reg8(0x170022,0x00);  // rxchn_man_en.
    write_reg8(0x170420,0xc9);  // script cc.
    write_reg8(0x170421,0x8c);  // modem:ZIGBEE_MODE:01.
    write_reg8(0x170422,0x00);  // modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423,0x00);  // modem:ZIGBEE_MODE_TX.
    // write_reg8(0x170425,0x01);  // trigger lr access fec
    write_reg8(0x170426,0x00);  // modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);  // modem:disable MSK.
    write_reg8(0x17042c,0x38);  // modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xf6);  // LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0c);  // LR_NUM_GEAR_H.
    write_reg8(0x170439,0x7d);  // LR_TIM_REC_CFG_1.
    write_reg8(0x17043d,0x00);  // modem:zb_sfd_frm_ll.
    write_reg8(0x17044c,0x0c);  // RX:acc_len modem.
    write_reg8(0x17044d,0x01);  // r_rxchn_en_i:To modem.
    write_reg8(0x170473,0xa1);  // TOT_DEV_RST.
    write_reg8(0x17049a,0x00);  // tx_tp_align.
    write_reg8(0x1704c2,0x3b);  //grx_0.
    write_reg8(0x1704c3,0x49);  //grx_1.
    write_reg8(0x1704c4,0x58);  //grx_2.
    write_reg8(0x1704c5,0x66);  //grx_3.
    write_reg8(0x1704c6,0x71);  //grx_4.
    write_reg8(0x1704c7,0x7b);  //grx_5.
    write_reg8(0x1704c8,0x39);  // Gain offset to compensate system error
    write_reg8(0x170620,0x10);  // sc_code.
    write_reg8(0x170621,0x0a);  // if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x20);  // HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x23);  // HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063d,0x61);  // ble:bw_code.
    write_reg8(0x17063f,0x00);  // 250k modulation index:telink add rx for 250k/500k.

#if(BLE_S2_S8_OLD_PATH)
    //old path config
    write_reg8(0x170438,0xb8);  // LR_TIM_EDGE_DEV.
    write_reg8(0x17043e,0x81);  // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044e,0xf0);  // ble sync threshold:To modem.
#else
    //new path config
    write_reg8(0x170438,0xb6);  // LR_TIM_EDGE_DEV.
    write_reg8(0x17043e,0x01);  // BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4
    write_reg8(0x17044e,0x3c);  // ble sync threshold:To modem.
#endif

    //The following configurations fix the BLE 125k and BLE 500k PER floor non-zeroing issue
    //Modified by chenxi.wang, confirmed by yuya.hao at 20240829.
    write_reg8(0x1704f0,0x1e);//defaults 0x1c->0x1e. lr_s8_pdet synv_success threshold 0~32

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
    write_reg8(0x17062d,0x00);//CHNL_NUM switch by sw en
    write_reg8(0x170799,0x00);//ZB_FREQ_FIXED_OW
    write_reg8(0x1704bb,0x00);//disable 2 stage filter

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LR_S8_125K;
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
 void rf_start_brx(void* addr, unsigned int tick)
 {
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_rx_fst_timeout = 0x0fffffff;
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x82;// ble rx.
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
 void rf_start_btx(void* addr, unsigned int tick)
 {
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    reg_rf_ll_cmd_schedule =  tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x81;                       // ble tx.
 }


