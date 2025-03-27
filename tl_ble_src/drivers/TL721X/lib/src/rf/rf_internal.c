/********************************************************************************************************
 * @file    rf_internal.c
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
#include "lib/include/rf/rf_internal.h"
#include "lib/include/rf/rf_common.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"
#if(0)
#if RF_HADM_EN
static unsigned int g_iq_data_len,g_iq_group_num;
#endif

 /**
  * @brief     This function serves to set ant  mode of RF.
  * @return    none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_ant_mode(void)
 {

    write_reg8(0x17063d,0x61);//ble:bw_code
    write_reg8(0x170620,0x10);//sc_code
    write_reg8(0x170621,0x0a);//if_freq,IF = 1Mhz,BW = 1Mhz
    write_reg8(0x170622,0x20);//HPMC_EXP_DIFF_COUNT_L
    write_reg8(0x170623,0x23);//HPMC_EXP_DIFF_COUNT_H

    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420,0xc8);// script cc.


    write_reg8(0x170422,0x00);//modem:BLE_MODE_TX,1MBPS

    write_reg8(0x17044e,0x0f);//sync threshold:TO MODEM  access_code threshold
    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
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
     write_reg8(0x1704c2,0x36);//grx_0.
     write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

    write_reg8(0x170000,0x0f);//tx_mode
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:2,->3(2byte->3byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x43); //preamble length
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xf3);//bit<4>mode:1->1m;bit<0:3>:private head.
    write_reg8(0x170005,0x02);//lr mode bit<4:5> 0:off,3:125k,2:500k.bit<0:2> TX:acc_len

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0a);//bit<0:2> RX:acc_len modem

    rf_set_crc_config(&rf_crc_config[1]);
    g_rfmode = RF_MODE_ANT;

 }

 /**
  * @brief     This function serves to  set hybee_500K_dcoc  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_500K_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.0x06->0x00
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.0x46->0x43
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.enable TX mode
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.


 #if(!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x170421,0x81);//modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e,0x08);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x0c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x39);//modem:zb_dis_rst_pdet_isfd.
    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd
 #else
    write_reg8(0x170421,0x01);//modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.0x39->0x00
    write_reg8(0x1704bb,0x00);//2 stage filter

 #endif

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xc4);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

    write_reg8(0x170000,0x13);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K;
 }

 /**
  * @brief     This function serves to  set hybee_1M_dcoc  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_1M_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.

    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.enable TX mode
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x170421,0x81);//modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e,0x14);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x3c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x39);//modem:zb_dis_rst_pdet_isfd.
    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x1c);//pdet_hardec_thd

 #else

    write_reg8(0x170421,0x01);//modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x1704bb,0x00);//2 stage filter

 #endif

    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xc4);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

    write_reg8(0x170000,0x07);//tx_mode.
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
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.

    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M;
 }

 /**
  * @brief     This function serves to  set hybee_2M_dcoc  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_2M_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.enable TX mode
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.

    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x170421,0x8d);//modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e,0x14);//ble sync threshold:To modem.0x18->0x08
    write_reg8(0x17044c,0x3c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.0x39->0x00

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x1c);//pdet_hardec_thd

 #else
    write_reg8(0x170421,0x01);//modem:ZIGBEE_MODE:01.enable RX mode
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x1704bb,0x20);//2 stage filter

 #endif

    write_reg8(0x170000,0x0b);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500k.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_2M;
 }

 /**
  * @brief     This function serves to set hybee_1M_old  mode of RF.
  * @return    none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_1M_old_mode(void)
 {

    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.0x06->0x00
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x17043d,0x03);//modem:zb_sfd_frm_ll.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a,0x10);//modem:disable MSK.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x17043d,0x03);//modem:zb_sfd_frm_ll.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

     write_reg8(0x170421,0x8d);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x08);//ble sync threshold:To modem.
    write_reg8(0x17044d,0x06);//r_rxchn_en_i:To modem.0x0f
    write_reg8(0x17044c,0x0c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.0x39->0x00

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd

 #else

    write_reg8(0x170421,0x01);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.0x0f
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.

 #endif

    write_reg8(0x170000,0x07);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x04);//bit<0:1>private mode control. bit<2:3> tx mode.old tx h1m xmode
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x00008200);//access code for hybee 1m.0x0d:old h1m SFD
    write_reg32(0x170010,0x009500d3);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.

    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_1M_OLD;
 }

 /**
  * @brief     This function serves to  set hybee_2M_old  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_2M_old_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.0x06->0x00
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x17043d,0x03);//modem:zb_sfd_frm_ll.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x170421,0x8d);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x08);//ble sync threshold:To modem.
    write_reg8(0x17044d,0x06);//r_rxchn_en_i:To modem.0x0f
    write_reg8(0x17044c,0x0c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto


    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd

 #else

    write_reg8(0x170421,0x01);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.0x0f
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.

 #endif

    write_reg8(0x170000,0x0b);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x44);//bit<0:1>private mode control. bit<2:3> tx mode.old tx hybee xmode
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500K.
    write_reg8(0x170018,0xa3);//old h2m SFD

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_2M_OLD;
 }

 /**
  * @brief     This function serves to  set hybee_500K_new  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_500K_new_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
     //[6:4]Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
    //[4]   2byte SFD
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x170421,0xad);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x08);//ble sync threshold:To modem.0x18->0x08
    write_reg8(0x17044c,0x0c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0xc2);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd

 #else

    write_reg8(0x170421,0x21);//modem:ZIGBEE_MODE:01.
     write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.

 #endif
    write_reg8(0x170000,0x13);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170132,0x05);//r_zb_hybee_new
    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x170132,0x05);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_500K_NEW;
 }

 /**
  * @brief     This function serves to  set hybee_1M_new  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_1M_new_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
        //[6:4]Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
        //[4]   2byte SFD
        //[5]   250k rate payload_length
        //[6]   250k rate payload_length_extend
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x170421,0xad);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x08);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x0c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0xc2);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd

 #else

    write_reg8(0x170421,0x21);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.

 #endif

    write_reg8(0x170000,0x07);//tx_mode.
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
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170132,0x05);//r_zb_hybee_new
    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x170132,0x05);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_1M_NEW;
 }
 /**
  * @brief     This function serves to  set hybee_2M_new  mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_2M_new_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
        //[6:4]Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
        //[4]   2byte SFD
        //[5]   250k rate payload_length
        //[6]   250k rate payload_length_extend
    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x170421,0xad);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x08);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x0c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
     write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0xc2);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x18);//pdet_hardec_thd

 #else

    write_reg8(0x170421,0x21);//modem:ZIGBEE_MODE:01.
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.

 #endif
    write_reg8(0x170000,0x0b);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170132,0x05);//r_zb_hybee_new
    write_reg32(0x170008,0x000000a7);//access code for zigbee
    write_reg32(0x17000c,0x0000d100);//access code for hybee 1m.
    write_reg32(0x170010,0x00950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x00);//access code for hybee 500k.


    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x170132,0x05);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_2M_NEW;
 }

 /**
  * @brief     This function serves to  set hybee_500K_2byte_sfd mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_500K_2byte_sfd_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x17043f,0x00);//LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    //AURA TX 2msps config
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    //AURA enable TX zigbee and disable MSK

    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

 #if RF_RX_SHORT_MODE_EN
    write_reg8(0x170479,0x38);//RX_DIS_PDET_BLANK.
 #else
    write_reg8(0x170479,0x00);//RX_DIS_PDET_BLANK.
 #endif
    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x170420,0xc8);// script cc.
    //sfd 2byte
    write_reg8(0x17044e,0x14);//ble sync threshold:To modem. 2byte
    write_reg8(0x17044c,0x3c);//Rx: sfd match symb num 2byte<bit4~6:sfd num>
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x170421,0x9d);//zb_demod FSM open 2byte SFD
        //[6:4] Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
        //[4]   2byte SFD
        //[5]   250k rate payload_length
        //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x39);//modem:zb_dis_rst_pdet_isfd.0x39->0x00

    //dcest
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    write_reg8(0x170452,0xc0);//dciq edr enable
    write_reg8(0x170453,0x01);//edr dcoc enable
    write_reg8(0x1704a4,0x2a);//dc_en and edr dcoc on
    write_reg8(0x1704a7,0x00);//dcest en

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x58);//pdet_hardec_thd->0x1c

 // write_reg8(0x1704e6,0x1d);

 #else

    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.  rx channel enabled; multiple receivers: multiple channels can be configured

    write_reg8(0x170420,0xc8);// script cc.
    //sfd 2byte
    write_reg8(0x17044e,0x18);//ble sync thresholds:To modem. 2byte
    write_reg8(0x17044c,0x4c);//Rx: sfd match symb num 2byte
    write_reg8(0x170421,0x1d);//zb_demod FSM open 2byte SFD
        //[6:4]  Adjustment of the new package functionality switch, need to baseband tx and rx to do the relevant configuration to support the
        //[4]   2byte SFD
        //[5]   250k rate payload_length
        //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.0x39->0x00
    write_reg8(0x17043b,0x5c);//Rx: sfd match symb0 num
     write_reg8(0x1704bb,0x00);

 #endif

    write_reg8(0x170000,0x13);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    //2-byte SFD setup
    write_reg8(0x170134,0x80); //r_zb_sfd_length
 // write_reg8(0x1704e2,0x58);//SFD byte reverse

    write_reg32(0x170008,0x000035a7);//access code for zigbee
    write_reg32(0x17000c,0x0035d100);//access code for hybee 1m.
    write_reg32(0x170010,0x35950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x35);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_500K_2BYTE_SFD;
 }

 /**
  * @brief     This function serves to  set hybee_1M_2byte_sfd mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_1M_2byte_sfd_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x17043f,0x00);//LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

 #if RF_RX_SHORT_MODE_EN
    write_reg8(0x170479,0x38);//RX_DIS_PDET_BLANK.
 #else
    write_reg8(0x170479,0x00);//RX_DIS_PDET_BLANK.
 #endif
    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x17044e,0x14);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x3c);//rx: sfd match symb num
    write_reg8(0x17043b,0x3c);//Rx: sfd match symb0 num
    write_reg8(0x170421,0x9d);//modem:ZIGBEE_MODE:01.
    //[6:4]  Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
    //[4]   2byte SFD
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x39);//modem:zb_dis_rst_pdet_isfd.

    //dcest
    write_reg8(0x170450,0x3f);//dciq edr  auto
    write_reg8(0x170451,0x0e);//edr dcoc auto
    write_reg8(0x170452,0xc0);//dciq edr enable
    write_reg8(0x170453,0x01);//edr dcoc enable
    write_reg8(0x1704a4,0x2a);//dc_en and edr dcoc on
    write_reg8(0x1704a7,0x00);//dcest en

 //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x58);//pdet_hardec_thd

 #else

    write_reg8(0x17044e,0x18);//ble sync thresholds:To modem.
    write_reg8(0x17044c,0x4c);//rx: sfd match symb num
    write_reg8(0x17043b,0x5c);//Rx: 2byte_sfd support,  reverse sfd byte order
    write_reg8(0x170421,0x1d);//modem:ZIGBEE_MODE:01.
    //[6:4]  Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
    //[4]   2byte SFD
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x1704bb,0x20);

 #endif

    write_reg8(0x170000,0x07);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170134,0x80);//r_zb_hybee_new

    write_reg32(0x170008,0x000035a7);//access code for zigbee
    write_reg32(0x17000c,0x0035d100);//access code for hybee 1m.
    write_reg32(0x170010,0x35950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x35);//access code for hybee 500K.

    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.


    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_HYBEE_1M_2BYTE_SFD;
 }

 /**
  * @brief     This function serves to  set hybee_2M_2byte_sfd mode of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_2M_2byte_sfd_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)


    write_reg8(0x17044e,0x14);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x170421,0x9d);//modem:ZIGBEE_MODE:01.
    //[6:4]Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
    //[4]   2byte SFD
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x39);//modem:zb_dis_rst_pdet_isfd.

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto0x3f
    write_reg8(0x170451,0x0f);//edr dcoc auto0x0e


    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x58);//pdet_hardec_thd

 #else

    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x4c);//rx: sfd match symb num
    write_reg8(0x17043b,0x1c);//Rx: sfd match symb0 num
    write_reg8(0x170421,0x01);//modem:ZIGBEE_MODE:01.
    //[6:4]Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
    //[4]   2byte SFD
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.
     write_reg8(0x1704bb,0x00);
     write_reg8(0x1704e2,0x58);//pdet_hardec_thd

 #endif

    write_reg8(0x170000,0x0b);//tx_mode.
    write_reg8(0x170001,0x00);//PN.

    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170134,0x80);//r_zb_sfd_length



    write_reg32(0x170008,0x000035a7);//access code for zigbee
    write_reg32(0x17000c,0x0035d100);//access code for hybee 1m.
    write_reg32(0x170010,0x35950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x35);//access code for hybee 500k.


    write_reg8(0x170021,0x27);//rx packet len 0 enable.

    write_reg8(0x170022,0x00);//rxchn_man_en.

    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_2M_2BYTE_SFD;
 }
 /**
  * @brief     This function serves to  set hybee_2M_2byte_sfd_new of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_hybee_2M_2byte_sfd_new_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.0x41->0x43
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170420,0xc8);// script cc.
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044d,0x0f);//r_rxchn_en_i:To modem.

    write_reg8(0x170423,0x80);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x01);//modem:zb_sfd_frm_ll.

    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.

 #if(!RF_ZIGBEE_OLD_DATA_PATH)

    write_reg8(0x17044e,0x10);//ble sync threshold:To modem.
    write_reg8(0x17044c,0x2c);//rx: sfd match symb num
    write_reg8(0x17043b,0x7c);//Rx: sfd match symb0 num
    write_reg8(0x170421,0xbd);//modem:ZIGBEE_MODE:01.
    //[6:4]Adjusting the new ZIGBEE mode switch. Requires both baseband tx and rx configurations to support this.
    //[4]   2byte SFD
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.

    //dcest
    write_reg8(0x170450,0xff);//dciq edr  auto
    write_reg8(0x170451,0x0f);//edr dcoc auto

    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0,0x90);//sync_thd
    write_reg8(0x1704e1,0x19);//sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2,0x58);//pdet_hardec_thd

 #else
    write_reg8(0x17044e,0x18);//ble sync threshold:To modem.0x18->0x08
    write_reg8(0x17044c,0x4c);//rx: sfd match symb num
    write_reg8(0x17043b,0x5c);//Rx: sfd match symb0 num
    write_reg8(0x170421,0x3d);//modem:ZIGBEE_MODE:01.
    //[6:4]
    //[4]   2byte SFD,old data path
    //[5]   250k rate payload_length
    //[6]   250k rate payload_length_extend
    write_reg8(0x17042c,0x3b);//modem:zb_dis_rst_pdet_isfd.0x39->0x00
    write_reg8(0x1704bb,0x00);
 #endif

    write_reg8(0x170000,0x0b);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value               note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4->7(3byte->4byte->7byte)
    * Add 3Byte preamble length to fix Freq Drift Rate marginal fail for A2 chip in TX power above 10dBm.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
    */
    write_reg8(0x170002,0x47);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170134,0x80);//r_zb_sfd_length
    write_reg8(0x170132,0x05);//r_zb_hybee_new

    write_reg32(0x170008,0x000035a7);//access code for zigbee
    write_reg32(0x17000c,0x0035d100);//access code for hybee 1m.
    write_reg32(0x170010,0x35950000);//access code for hybee 2m.
    write_reg8(0x170014,0x2f);
    write_reg8(0x170015,0x35);//access code for hybee 500k.


    write_reg8(0x170021,0x23);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.

    write_reg8(0x170132,0x05);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HYBEE_2M_2BYTE_SFD_NEW;
 }

 /**
  * @brief     This function serves to  set zigbee_hr_2m of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_zigbee_hr_2m_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x170420,0xc8);// script cc.

    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.
    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.
    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x8c);//modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX. enable TX mode
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    //sfd from ll
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x36);//grx_0.
    write_reg8(0x1704c3,0x48);//grx_1.
    write_reg8(0x1704c4,0x54);//grx_2.
    write_reg8(0x1704c5,0x62);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x79);//grx_5.


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
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170132,0x03);//zb_phr_extend_en

    write_reg32(0x170008,0x5555370b);//access code for zigbee


    write_reg8(0x170021,0xa1);//rx packet len 0 enable.
    write_reg8(0x170022,0x00);//rxchn_man_en.

    write_reg8(0x170132,0x01);

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode  = RF_MODE_HR_2M;

 }

 /**
  * @brief     This function serves to  set lowrate_20K of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_lowrate_20K_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.

    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.


    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170420,0xc8);// script cc.


    // low-rate setup
    write_reg8(0x170500,0xc8);
    write_reg8(0x170502,0x4b);
    write_reg8(0x170504,0x95);
    write_reg8(0x170505,0x85);
    write_reg8(0x17050a,0x1f);

    //FPGA agc gain adjust: self test, not public-version
    write_reg8(0x170539,0x00);
    //[b6:b5] : high 11 ,medium 10, low 0X
    //[b4:b0] : dB level --0x0 min, 0x1f max
    //low 0x00~0x1f, 0x20~0x3f; 0x00gain == 0x20gain
    //medium 0x40~0x5f
    //high 0x60~0x7f

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x3b);//grx_0.
    write_reg8(0x1704c3,0x4c);//grx_1.
    write_reg8(0x1704c4,0x58);//grx_2.
    write_reg8(0x1704c5,0x64);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x7a);//grx_5.

    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4(3byte->4byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x44);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x0c);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170216,0x29);//r_t_tx_en_dly
    write_reg8(0x170130,0x64);
    write_reg8(0x170131,0x00);

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.


    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x4c);//RX:acc_len modem.

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LOW_RATE_20K;
 }

 /**
  * @brief     This function serves to  set lowrate_25K of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_lowrate_25K_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.

    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170420,0xc8);//script cc.


    // low-rate setup
    write_reg8(0x170500,0xa0);
    write_reg8(0x170502,0x3c);
    write_reg8(0x170504,0x95);
    write_reg8(0x170505,0x85);

    write_reg8(0x17050a,0x1f);

    //FPGA agc gain adjust: self test, not public-version
    write_reg8(0x170539,0x00);
    //[b6:b5] : high 11 ,medium 10, low 0X
    //[b4:b0] : dB level --0x0 min, 0x1f max
    //low 0x00~0x1f, 0x20~0x3f; 0x00gain == 0x20gain
    //medium 0x40~0x5f
    //high 0x60~0x7f

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x3b);//grx_0.
    write_reg8(0x1704c3,0x4c);//grx_1.
    write_reg8(0x1704c4,0x58);//grx_2.
    write_reg8(0x1704c5,0x64);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x7a);//grx_5.

    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4(3byte->4byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x44);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x0c);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170216,0x29);//r_t_tx_en_dly
    write_reg8(0x170130,0x50);
    write_reg8(0x170131,0x00);

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.


    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0a);//RX:acc_len modem.

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LOW_RATE_25K;
 }

 /**
  * @brief     This function serves to  set lowrate_100K of RF.
  * @return   none.
  * @note      TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_lowrate_100K_mode(void)
 {
    write_reg8(0x17063d,0x41);//ble:bw_code.
    write_reg8(0x170620,0x00);//sc_code.
    write_reg8(0x170621,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622,0x43);//HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623,0x26);//HPMC_EXP_DIFF_COUNT_H.


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    write_reg8(0x170422,0x01);//modem:BLE_MODE_TX,2MBPS.

    write_reg8(0x17044e,0x1e);//ble sync threshold:To modem.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.


    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01.
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170420,0xc8);// script cc.


    // low-rate setup
    write_reg8(0x170500,0x28);
    write_reg8(0x170502,0x00);
    write_reg8(0x170504,0x95);//low-rate rx sync sel: 1-self sync
    write_reg8(0x170505,0x65);
    write_reg8(0x170513,0x0f);//low-rate lpf dc 100k

    write_reg8(0x17050a,0x1e);

    //FPGA agc gain adjust: self test, not public-version
    write_reg8(0x170539,0x00);
    //[b6:b5] : high 11 ,medium 10, low 0X
    //[b4:b0] : dB level --0x0 min, 0x1f max
    //low 0x00~0x1f, 0x20~0x3f; 0x00gain == 0x20gain
    //medium 0x40~0x5f
    //high 0x60~0x7f

    write_reg8(0x17049a,0x00);//tx_tp_align.
    write_reg8(0x1704c2,0x3b);//grx_0.
    write_reg8(0x1704c3,0x4c);//grx_1.
    write_reg8(0x1704c4,0x58);//grx_2.
    write_reg8(0x1704c5,0x64);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x7a);//grx_5.

    write_reg8(0x170000,0x0f);//tx_mode.
    write_reg8(0x170001,0x00);//PN.
    /*
    *       bit                 default value                       note
    * ---------------------------------------------------------------------------
    * <4: 0>:preamble length     default:3,->4(3byte->4byte) Add 1Byte preamble length to fix Freq Drift Rate marginal fail.
    * modified by zhiwei.wang,confirmed by wenfeng.lou 20240606.jira:http://192.168.48.49:8080/browse/TER-64
    */
    write_reg8(0x170002,0x44);//preamble len.
    write_reg8(0x170003,0x54);//bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x170004,0xe1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x0c);//lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x170216,0x29);//r_t_tx_en_dly
    write_reg8(0x170130,0x14);
    write_reg8(0x170131,0x00);

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.


    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.

    rf_set_crc_config(&rf_crc_config[0]);
    g_rfmode = RF_MODE_LOW_RATE_100K;
 }



 /**
  * @brief      This function serves to set pin for RFFE of RF.
  * @param[in]   tx_pin   - select pin as rffe to send.
  * @param[in]   rx_pin   - select pin as rffe to receive.
  * @return     none.
  */
 void rf_set_rffe_pin(gpio_func_pin_e tx_pin, gpio_func_pin_e rx_pin)
 {
    if(tx_pin != rx_pin)
    {
        gpio_set_mux_function(rx_pin,RX_CYC2LNA);
        gpio_set_mux_function(tx_pin,TX_CYC2PA);
    }
 }

 /**
  * @brief  This function serve to set the private ack enable,mainly used in prx/ptx.
  * @param[in]  rf_mode     -   Must be one of the private mode.
  * @return     none
  */
 void rf_set_pri_tx_ack_en(rf_mode_e rf_mode)
 {
    if(rf_mode == RF_MODE_PRIVATE_1M)
        write_reg8(0x170004, 0x9a);//1m 9a //enable  ack flag
    else if(rf_mode == RF_MODE_PRIVATE_2M)
        write_reg8(0x170004, 0x8a);//2m,8a
    else if(rf_mode == RF_MODE_PRIVATE_500K || rf_mode == RF_MODE_PRIVATE_250K)
    {
        write_reg8(0x170004,read_reg8(0x170004)&0xbf);
    }
 }


 /**
  * @brief   This function serves to judge whether the FIFO is empty.
  * @param pipe_id specify the pipe.
  * @return TX FIFO empty bit.
  *             -#0 TX FIFO NOT empty.
  *             -#1 TX FIFO empty.
  */
 unsigned char rf_is_rx_fifo_empty(unsigned char pipe_id)
 {
    return (reg_rf_dma_tx_wptr(pipe_id)) == (reg_rf_dma_tx_rptr(pipe_id));
 }




 /**
  * @brief      This function is used to  set the modulation index of the receiver.
  *              This function is common to all modes,the order of use requirement:configure mode first,
  *              then set the the modulation index,default is 0.5 in drive,both sides need to be consistent
  *              otherwise performance will suffer,if don't specifically request,don't need to call this function.
  * @param[in]  mi_value- the value of modulation_index*100.
  * @return     none.
  */
 void rf_set_rx_modulation_index(rf_mi_value_e mi_value)
 {
    unsigned char modulation_index_high;
    unsigned char modulation_index_low;
    unsigned char kvm_trim;
    unsigned short mi_int = (unsigned short)(mi_value * 1.28)/10;

    modulation_index_low = mi_int%256;

    modulation_index_high = (mi_int%512)>>8;
    (reg_rf_modem_rxc_mi_flex_ble_0) = (modulation_index_low);
    (reg_rf_modem_rxc_mi_flex_ble_0) |= (modulation_index_high);
    if((reg_rf_mode_cfg_tx1_0) & 0x01)
    {
        if ((mi_value >= 750)&&(mi_value <= 1000))
            kvm_trim = 3;
        else if (mi_value > 1000)
            kvm_trim = 7;
        else
            kvm_trim = 1;
    }
    else
    {

        if ((mi_value >= 750)&&(mi_value <= 1000))
            kvm_trim = 1;
        else if ((mi_value > 1000)&&(mi_value <= 1500))
            kvm_trim = 3;
        else if (mi_value > 1500)
            kvm_trim = 7;
        else
            kvm_trim = 0;
    }
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));
 }


 /**
  * @brief      This function is used to  set the modulation index of the sender.
  *              This function is common to all modes,the order of use requirement:configure mode first,
  *              then set the the modulation index,default is 0.5 in drive,both sides need to be consistent
  *              otherwise performance will suffer,if don't specifically request,don't need to call this function.
  * @param[in]  mi_value- the value of modulation_index*100.
  * @return     none.
  */
 void rf_set_tx_modulation_index(rf_mi_value_e mi_value)
 {

    unsigned char modulation_index_high;
    unsigned char modulation_index_low;
    unsigned char kvm_trim;
    unsigned short mi_int = (unsigned short)(mi_value * 1.28)/10;
    modulation_index_low = mi_int%256;

    modulation_index_high = (mi_int%512)>>8;
    (reg_rf_radio_mode_cfg_rx2_0) = (modulation_index_low);
    (reg_rf_radio_mode_cfg_rx2_1) |= (modulation_index_high);

    if(reg_rf_mode_cfg_tx1_0 & 0x01)
    {
        if ((mi_value >= 750)&&(mi_value <= 1000))
            kvm_trim = 3;
        else if (mi_value > 1000)
            kvm_trim = 7;
        else
            kvm_trim = 1;
    }
    else
    {

        if ((mi_value >= 750)&&(mi_value <= 1000))
            kvm_trim = 1;
        else if ((mi_value > 1000)&&(mi_value <= 1500))
            kvm_trim = 3;
        else if (mi_value > 1500)
            kvm_trim = 7;
        else
            kvm_trim = 0;
    }
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));
 }


 /**
  * @brief      This function is mainly used to set the energy when sending a single carrier.
  * @param[in]  level       - The slice corresponding to the energy value.
  * @return     none.
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

 /**
  * @brief      This function serves to set the range of chn group corresponding to the process of obtaining fcal calibration values at different frequency points
  * @param[in]  *fcal_chn_range  - chn group range pointer(chn_num < fcal_chn_range)
  * @return     none
  * @note       If the frequency point is set using the rf_dual_chn() interface when obtaining FCAL calibration values for different chn,
  *             this interface needs to be used to set the range of chn.
 */
 void rf_set_fcal_chn_group_range_ctn(unsigned char *fcal_chn_range)
 {
    for(int i=0;i<8;i++)
    {
        reg_rf_fcal_chn_range_ctn(i)= fcal_chn_range[i];
    }
    write_reg8(0x170644,read_reg8(0x170644)&(~BIT(0)));// CHNL Frequency decided by channel number programmed in TXRX_CFG.CHNL_NUM

 }


 /**
  * @brief      This function is mainly used for the disable hpmc trim function.
  * @return     none.
  * @note       TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_dis_hpmc_trim(void)
 {
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(5)));
    write_reg8(0x170680,read_reg8(0x170680)|BIT(5));
 }

 /**
  * @brief      This function is mainly used for the disable ldo trim function.
  * @return     none.
  * @note       TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_dis_ldo_trim(void)
 {
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(2)));
    write_reg8(0x170681,read_reg8(0x170681)|BIT(2));
 }

 /**
  * @brief      This function is mainly used for the disable dcoc trim function.
  * @return     none.
  * @note       TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_dis_dcoc_trim(void)
 {
 write_reg8(0x170682,read_reg8(0x170682)&(~BIT(4)));
 write_reg8(0x170680,read_reg8(0x170680)|BIT(4));
 }

 /**
  * @brief      This function is mainly used for the disable rccal trim function.
  * @return     none.
  * @note       TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_dis_rccal_trim(void)
 {
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(2)));
    write_reg8(0x170680,read_reg8(0x170680)|BIT(2));
 }

 /**
  * @brief      This function is mainly used for the disable fcal trim function.
  * @return     none.
  * @note       TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_dis_fcal_trim(void)
 {
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));
    write_reg8(0x170681,read_reg8(0x170681)|BIT(3));
 }

 /**
  * @brief     This function sets the bit order of the length field.
  * @param[in] length_ord  - The bit order of LENGTH field.
  *            0: LSBit first  1: MSBit first
  * @return    none.
  * @note       TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_pri_generic_length_bit_ord(unsigned char length_bit_ord)
 {
         write_reg8(0x17013a,(read_reg8(0x17013a)&0x7f)|(length_bit_ord<<7));
 }

 #if(0)
 /**
  * @brief      This function serves to disable zigbee/hybee PHR.
  * @return     none.
  */
 void rf_hybee_phr_dis(void)
 {
         write_reg8(0x170132,read_reg8(0x170132)&(~BIT(0)));
 }

 /**
  * @brief      This function serves to set_no_phr_payload_len.
  * @param[in]  tx_pkt_length- zigbee/hybee PSDU length(PSDU length range:0~0x7ff)
  * @return     none.
  */
 void rf_hybee_set_no_phr_payload_len(unsigned int tx_pkt_length)
 {
         write_reg8(0x170133, (tx_pkt_length&0xff));
         write_reg8(0x170132, (read_reg8(0x170132)&0x1f)|((tx_pkt_length&0x0700)>>8)<<5);
 }
 /**
  * @brief      This function serves to enable phr_extend.
  * @return     none.
  */
 void rf_hybee_phr_extend_en(void)
 {
         write_reg8(0x170132,(read_reg8(0x170132)&0xfd) | BIT(1));
         write_reg8(0x170421,(read_reg8(0x170421)&0xbf) | BIT(6));
 }

 /**
  * @brief      This function serves to disable hybee phr extend.
  * @return     none.
  */
 void rf_hybee_phr_extend_dis(void)
 {
         write_reg8(0x170132,read_reg8(0x170132)&(~BIT(1)));
         write_reg8(0x170421,read_reg8(0x170421)&(~BIT(6)));
 }

 /**
  * @brief      This function serves to set hybee gap length between SFD and PHR in octet.
  * @param[in]  gap_length - zigbee/hybee gap length(Gap length range:0~63)
  * @return     none.
  */
 void rf_hybee_set_gap_len(unsigned char gap_length)
 {
         write_reg8(0x170134,(read_reg8(0x170134)&0xc0)|gap_length);
 }




 /**
  * @brief  This function serve to disable the private ack ,mainly used in prx/ptx.
  * @param[in]  none
  * @return     none
  */
 void rf_set_ptx_prx_ack_dis(void)
 {
    write_reg8(0x170004,read_reg8(0x170004)|BIT(6));//bit6 1
    write_reg8(0x170215,read_reg8(0x170215)&(~BIT(5)));//bit5 0
 }

 /**
  * @brief  This function serve to initial the ptx/prx setting.
  * @return none.
  */
 void rf_ptx_prx_config(void)
 {
     write_reg8(0x170202, read_reg8(0x170202) & 0xfe); // md_dis
     write_reg8(0x170203, read_reg8(0x170203) & (~BIT(3)));//This bit is required to be turned off in private mode, this bit is only for BLE mode to enable crc related interrupts in BLE mode.
 //  write_reg8(0x170203, read_reg8(0x170203) & 0xf0);//rx timeout disable
     reg_rf_ll_ctrl2 &= ~FLD_RF_R_NOACK_RETRY_CNT_EN;//noack_retry_count_dis
     //PID_EN
     reg_rf_ll_ctrl2 |= FLD_RF_R_REP_SN_PID_EN;
     write_reg8(0x170201, (read_reg8(0x170201)&0xc0)|0x3f);//reset pid1~5
 }

 /**
  * @brief  This function serve to initial the prx setting.
  * @return none.
  */
 void rf_prx_config(void)
 {
    write_reg8(0x170202, read_reg8(0x170202) & 0xfe); // md_dis
    write_reg8(0x170203, read_reg8(0x170203) & (~BIT(3)));//This bit is required to be turned off in private mode, this bit is only for BLE mode to enable crc related interrupts in BLE mode.
    write_reg8(0x170203, read_reg8(0x170203) & 0xf0);//rx timeout disable
    reg_rf_ll_ctrl2 &= ~FLD_RF_R_NOACK_RETRY_CNT_EN;//noack_retry_count_dis
    //PID_EN
    reg_rf_ll_ctrl2 |= FLD_RF_R_REP_SN_PID_EN;
    write_reg8(0x170201, (read_reg8(0x170201)&0xc0)|0x3f);//reset pid1~5
    write_reg8(0x170215, 0xc0);//chn tx_manual off
 }

 /**
   * @brief  This function serve to set the private ack enable,mainly used in prx/ptx.
   * @param[in]  none
   * @return     none
   */
  void rf_set_ptx_prx_ack_en(void)
  {
     write_reg8(0x170004,read_reg8(0x170004)&(~BIT(6)));
     write_reg8(0x170215,read_reg8(0x170215)&(~BIT(5)));
  }

  /**
   * @brief  This function serve to initial the ptx setting.
   * @return none.
   */
  void rf_ptx_config(void)
  {
     write_reg8(0x170202, read_reg8(0x170202) & 0xfe); // md_dis
     write_reg8(0x170203, read_reg8(0x170203) & (~BIT(3)));//This bit is required to be turned off in private mode, this bit is only for BLE mode to enable crc related interrupts in BLE mode.
 //  write_reg8(0x170203, read_reg8(0x170203) & 0xf0);//rx timeout disable
     reg_rf_ll_ctrl2 &= ~FLD_RF_R_NOACK_RETRY_CNT_EN;//noack_retry_count_dis
     //PID_EN
     reg_rf_ll_ctrl2 |= FLD_RF_R_REP_SN_PID_EN;
 //  write_reg8(0x170201, (read_reg8(0x170201)&0xc0)|0x3f);//reset pid1~5
 //  write_reg8(0x170215, 0xd0);//chn tx_manual off
  }

  /**
   * @brief      This function to set retransmit and retransmit delay.
   * @param[in]  retry_times - Number of retransmit, 0: retransmit OFF
   * @param[in]  retry_delay - Retransmit delay time.
   * @return     none.
   */
  void rf_set_ptx_retry(unsigned char retry_times, unsigned short retry_delay)
  {
     retry_times &= 0x0f;
      write_reg8(0x170214, retry_times);

     retry_delay &= 0x0fff;
      unsigned short tmp = read_reg16(0x170210);
     tmp &= 0xf000;
     tmp |= retry_delay;
     write_reg16(0x170210, tmp);
  }

  /**
   * @brief      This function serves to set RF ptx trigger.
   * @param[in]  addr    -   The address of tx_packet.
   * @param[in]  tick    -   Trigger ptx after (tick-current tick),If the difference is less than 0, trigger immediately.
   * @return     none.
   * @note       addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
   */
  void rf_start_ptx  (unsigned int tick)
  {
     reg_rf_ll_cmd_schedule = tick;
     reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
     reg_rf_ll_cmd = 0x83;
  }

  /**
   * @brief      This function serves to set RF prx trigger.
   * @param[in]  tick    -   Trigger prx after (tick-current tick),If the difference is less than 0, trigger immediately.
   * @return     none.
   */
  void rf_start_prx(unsigned int tick)
  {
     write_reg32 (0x170228, 0x0fffffff);                 // first timeout.
     reg_rf_ll_cmd_schedule = tick;
     reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
     reg_rf_ll_cmd = 0x84;
  }


 /**
  * @brief  This function serve to enable new/old crc value check for PRX new/old pkt
  * @return none.
  */
 void rf_prx_crc_check_en(void)
 {
    write_reg8(0x170201, read_reg8(0x170201)|BIT(6));
 }

 /**
  * @brief  This function serve to disable new/old crc value check for PRX new/old pkt
  * @return none.
  */
 void rf_prx_crc_check_dis(void)
 {
    write_reg8(0x170201, read_reg8(0x170201)&(~BIT(6)));
 }


 /**
  * @brief      This function serves to set the encryption master mode for ble.
  * @return     none.
  */
 void rf_set_ble_crypt_master_mode(void)
 {
    write_reg8(0x170001,read_reg8(0x170001)|BIT(2));
 }
 /**
  * @brief      This function serves to set the encryption slave mode for ble.
  * @return     none.
  */
 void rf_set_ble_crypt_slave_mode(void)
 {
    write_reg8(0x170001,read_reg8(0x170001)&(~BIT(2)));
 }

 /**
  * @brief      This function serves to enable ble crypt mode
  * @return     none.
  */
 void rf_ble_crypt_en(void)
 {
    reg_rf_tx_mode2 |= FLD_RF_TLK_CRYPT_ENABLE;
 }

 /**
  *  @brief       This function serves to set ble crypt
  *  @param[in]   *skey - aes key: 128 bit key for the encryption of the data
  *  @param[in]   *iv - Initialization Vector:64 bit Initialization Vector
  *  @return      none.
  */
 void rf_ble_crypt_setup(unsigned char* skey, unsigned char* iv)
 {
     for (int i = 0; i < 16; i++)
     {
         reg_rf_tlk_crypt_skey(i) = skey[i];
     }
     for (int i = 0; i < 8; i++)
     {
         reg_rf_tlk_crypt_iv(i) = iv[i];
     }
 }

 /**
 *  @brief      This function serves to set ble crypt tx cnt
 *  @param[in]  *txccmpktcnt - 40 bit tx ccm packet cnt
 *  @param[in]  *rxccmpktcnt - 40 bit rx ccm packet cnt
 */
 void rf_set_ble_crypt_tx_cnt(unsigned char* txccmpktcnt)
 {
    for (int i = 0; i < 5; i++)
    {
        write_reg8(0x170240 + i, txccmpktcnt[i]);
    }
 }

 /**
 *  @brief      This function serves to set ble crypt rx cnt
 *  @param[in]  *rxccmpktcnt - 40 bit rx ccm packet cnt
 */
 void rf_set_ble_crypt_rx_cnt(unsigned char* rxccmpktcnt)
 {
     for (int i = 0; i < 5; i++)
     {
         write_reg8(0x170248 + i, rxccmpktcnt[i]);
     }
 }

 /**
  *  @brief   This function serves to disable the tx ccm packet count auto-update feature
  *  @return  none.
  */
 void rf_ble_crypt_txpktcnt_updata_dis(void)
 {
     reg_rf_on_ccm_control |= FLD_RF_DIS_TXCNT_UPDATE;
 }

 /**
  *  @brief   This function serves to disable the rx ccm packet count auto-update feature
  *  @return  none.
  */
 void rf_ble_crypt_rxpktcnt_updata_dis(void)
 {
     reg_rf_on_ccm_control |= FLD_RF_DIS_RXCNT_UPDATE;
 }

 /**
  *  @brief   This function serves to reset the tlk_crypt module in software.
  *  @return  none.
  */
 void rf_ble_crypt_software_rst(void)
 {
     write_reg8(0x1700bd,read_reg8(0x1700bd)|BIT(0));
 }

 /**
  * @brief      This function serves to set rf channel for zigbee double channel.The actual channel set by this function is 2402+chn*2.
  * @param[in]   chn  -That you want to set the channel as 2402+chn*2.
  * @param[in]   chn1 -That you want to set the channel1 as 2402+chn*2.
  * @return     none.
  */
 void rf_zigbee_set_rx_dual_chn(signed char chn,signed char chn1)
 {
    unsigned char ctrim;
    unsigned char ctrim1;
    unsigned int freq = 0;
    unsigned int freq1 = 0;
    chn *= 2;
    chn1 *= 2;
    freq = chn + 2402;
    freq1 = chn1 + 2402;

    if(freq >= 2550){
        ctrim = 2;
    }
    else if(freq >= 2520){
        ctrim = 2;
    }
    else if(freq >= 2495){
        ctrim = 2;
    }
    else if(freq >= 2460){
        ctrim = 2;
    }
    else if(freq >= 2440){
        ctrim = 3;
    }
    else if(freq >= 2416){
        ctrim = 4;
    }
    else if(freq >= 2380){
        ctrim = 5;
    }
    else{
        ctrim = 7;
    }

    if(freq1 >= 2550){
        ctrim1 = 2;
    }
    else if(freq1 >= 2520){
        ctrim1 = 2;
    }
    else if(freq1 >= 2495){
        ctrim1 = 2;
    }
    else if(freq1 >= 2460){
        ctrim1 = 2;
    }
    else if(freq1 >= 2440){
        ctrim1 = 3;
    }
    else if(freq1 >= 2416){
        ctrim1 = 4;
    }
    else if(freq1 >= 2380){
        ctrim1 = 5;
    }
    else{
        ctrim1 = 7;
    }

    write_reg8(0x170644,read_reg8(0x170644)&0xfe);
    write_reg8(0x170629,read_reg8(0x170629)|0x01);//Front end matching capacitor adjustment for TX/RX. This code should be sent on reg_rx_lna_ctrim_lv<3:0>.
    write_reg8(0x170628,chn);//CHNL Frequency = (2402+CHNL_NUM*2) MHz,chnl num can be from 0 to 39
    write_reg8(0x170629,(read_reg8(0x170629)&0x1f)|(ctrim<<5));

    write_reg8(0x17062c,chn1);//CHNL Frequency = (2402+CHNL_NUM*2) MHz,chnl num can be from 0 to 39
    write_reg8(0x17062d,(read_reg8(0x17062d)&0xe3)|(ctrim1<<2));


    write_reg8(0x170799,(read_reg8(0x170799)|0x02));
    write_reg8(0x1707c8,(read_reg8(0x1707c8)|0x01));
 }
 #endif

 /**********************************************************************************************************************
  *                                         RF : packet_flt related functions                                 *
  *********************************************************************************************************************/
 /**
  * @brief      This function set the packet filter.
  * @param[in]  rf_pkt_flt - RF packet filtering parameters
  * @return     none.
  * @note        TODO:This function interface is not available at this time, and will be updated in subsequent releases.(unverified)
  */
 void rf_set_pkt_filter(rf_pkt_flt_t rf_pkt_flt)
 {
    reg_rf_pkt_flt_start = rf_pkt_flt.rf_pkt_flt_start;//starting byte
    reg_rf_pkt_flt_end = rf_pkt_flt.rf_pkt_flt_end;//ending byte
    reg_rf_pkt_match_threshold = rf_pkt_flt.rf_pkt_match_threshold;//Range of matches,In bits
    write_reg32(0x170064,rf_pkt_flt.rf_pkt_match_low);//rf_pkt_match_low
    write_reg32(0x170068,rf_pkt_flt.rf_pkt_match_high);//rf_pkt_match_high
    write_reg32(0x17006c,rf_pkt_flt.rf_pkt_mask_low);//rf_pkt_mask_low
    write_reg32(0x170070,rf_pkt_flt.rf_pkt_mask_high);//rf_pkt_mask_high
    reg_rf_pkt_flt_cntl |= (FLD_RF_PKT_FLT_EN|FLD_RF_FLT_BYTE_EN);// pkt_flt setup
 }


 #if RF_HADM_EN
 /**********************************************************************************************************************
  *                                         RF : AOA/D related functions                                  *
  *********************************************************************************************************************/
 /**
  * @brief      This function is used to set how many words as the transmission unit of baseband and dma.
  *                 You don't need to call this function for normal use. By default, the unit is 1 world!
  *                 After configuring the DMA, call this function to adjust the DMA rate.
  * @param[in]  rf_trans_unit_e size    - the unit of burst size .Identify how many bytes of data are
  *                                           handled by DMA each time
  * @return     none.
  */
 void rf_set_baseband_trans_unit(rf_trans_unit_e size)
 {
    reg_bb_dma_ctr3(1) = ((reg_bb_dma_ctr3(1) & 0xf8) | size);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | size);
 }

 /**
  * @brief      This function is used to set the position of the first antenna switch after the AOA receiver reference.The default is in the
  *                 middle of the first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code.Each additional code
  *                 will move the switch point back by 0.125us
  * @param[in]  swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
  *                             back 0.125us.
  * @return     none.
  */
 void rf_aoa_rx_ant_switch_point_adjust(unsigned short swt_offset)
 {
    unsigned char temp = (((swt_offset >> 8) & 0x01) << 2);
    reg_rf_ant_msb = ((((reg_rf_ant_msb) & (~FLD_RF_RX_ANT_OFFSET_MSB))) | temp);
    reg_rf_rx_antoffset = swt_offset & 0xff;
 }


 /**
  * @brief      This function is used to set the position of the first antenna switch after the AOD transmitter reference.The default is in the middle of the
  *                 first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code. Each additional code will move
  *                 the switch point back by 0.125us
  * @param[in]  swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
  *                             back 0.125us.
  * @return     none.
  */
 void rf_aod_tx_ant_switch_point_adjust(unsigned short swt_offset)
 {
    unsigned char temp = (((swt_offset >> 8) & 0x01) << 1);
    reg_rf_ant_msb = ((((reg_rf_ant_msb) & (~FLD_RF_TX_ANT_OFFSET_MSB))) | temp);
    reg_rf_tx_antoffset = swt_offset & 0xff;
 }

 /**
  * @brief      This function is mainly used to set the IQ data sample interval time. In normal mode, the sampling interval of AOA is 4us, and AOD will judge whether
  *                 the sampling interval is 4us or 2us according to CTE info.The 4us/2us sampling interval corresponds to the 2us/1us slot mode stipulated in the protocol.
  *                 Since the current hardware only supports the antenna switching interval of 4us/2us, setting the sampling interval to 1us or less will cause multiple
  *                 sampling at the interval of one antenna switching. Therefore, the sampling data needs to be processed by the upper layer according to the needs, and
  *                 currently it is mostly used Used in the debug process.
  *                 After configuring RF, you can call this function to configure slot time.
  * @param[in]  time_us - AOA or AOD slot time mode.
  * @return     none.
  * @note       Attention:(1)When the time is 0.25us, it cannot be used with the 20bit iq data type, which will cause the sampling data to overflow.
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
  * @brief      This function is mainly used to set the type of AOA/AODiq data. The default data type is 8bit. This configuration can be done before starting to receive
  *                 the package.
  * @param[in]  mode    - The length of each I or Q data.
  * @return     none.
  */
 void rf_aoa_aod_iq_data_mode(rf_iq_data_mode_e mode)
 {
    reg_rf_sof_offset = ((reg_rf_sof_offset & (~FLD_RF_SUPP_MODE))|((mode&0x07) << 4));
    g_iq_data_len = mode;
 }

 /****************************************************************************************************************************************
  *                                         RF : HADM related functions                                                                 *
  ****************************************************************************************************************************************/

 /**
  * @brief      This function is mainly used to initialize some parameter settings of the HADM IQ sample.
  * @param[in]  samp_num    - Number of groups to sample IQ data.
  * @param[in]  interval    - The interval time between each IQ sampling is (interval + 1)*0.125us.
  * @param[in]  start_point - Set the starting point of the sample.If it is rx_en mode, sampling starts
  *                               at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
  *                               starts at (start_point + 1) * 0.125us after sync.
  * @param[in]  suppmode    - The length of each I or Q data.
  * @param[in]  sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
  * @return     none.
  */
 void rf_hadm_iq_sample_init(unsigned short samp_num,unsigned char interval,unsigned char start_point,rf_iq_data_mode_e suppmode,rf_hadm_iq_sample_mode_e sample_mode)
 {
    rf_hadm_iq_sample_number(samp_num);
    rf_hadm_sample_interval_time(interval);
    rf_hadm_iq_start_point(start_point);
    rf_aoa_aod_iq_data_mode(suppmode);
    rf_hadm_iq_sample_mode(sample_mode);
    rf_iq_sample_enable();

 }

 /**
  * @brief      This function is mainly used to set the sample interval.
  * @param[in]  ant_interval- Set the interval for IQ sample, (interval + 1)*0.125us.
  * @return     none.
  * @note       The max sample rate is 4Mhz.
  */
 void rf_hadm_sample_interval_time(unsigned char interval)
 {
    reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & (~FLD_RF_IQ_SAMP_INTERVAL)) | (interval<<4));
 }

 /**
  * @brief      This function is mainly used to initialize the parameters related to HADM antennas.
  * @param[in]  clk_mode    - Set whether the antenna-related clock is always on or only when switching antennas.
  * @param[in]  ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.
  * @param[in]  ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
  * @param[in]  ant_txoffset- Adjust the switching start point of the tx-side antenna,(ant_rxoffset + 1)*0.125us.
  * @return     none.
  */
 void rf_hadm_ant_init(rf_hadm_ant_clk_mode_e clk_mode,unsigned char ant_interval,unsigned char ant_rxoffset,unsigned char ant_txoffset)
 {
    rf_hadm_ant_clk_mode(clk_mode);
    rf_set_hadm_ant_interval(ant_interval);
    rf_set_hadm_rx_ant_offset(ant_rxoffset);
    rf_set_hadm_tx_ant_offset(ant_txoffset);
 }

 /**
  * @brief      This function is mainly used to set the antenna switching interval.
  * @param[in]  ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.
  * @return     none.
  */
 void rf_set_hadm_ant_interval(unsigned char ant_interval)
 {
    write_reg8(0x170035,ant_interval);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(0)))|(ant_interval>>8));
 }

 /**
  * @brief      This function is mainly used to set the starting position of the antenna switching at the rx-side.
  * @param[in]  ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
  * @return     none.
  */
 void rf_set_hadm_rx_ant_offset(unsigned char ant_rxoffset)
 {
    write_reg8(0x17003a,ant_rxoffset);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(2)))|((ant_rxoffset>>8)<<2));
    write_reg8(0x170007,read_reg8(0x170007)|BIT(2));//rx_ant_switch
 }

 /**
  * @brief      This function is mainly used to set the starting position of the antenna switching at the tx-side.
  * @param[in]  ant_txoffset- Adjust the switching start point of the rx-side antenna,(ant_txoffset + 1)*0.125us.
  * @return     none.
  */
 void rf_set_hadm_tx_ant_offset(unsigned char ant_txoffset)
 {
    write_reg8(0x170039,ant_txoffset);
    write_reg8(0x170036,(read_reg8(0x170036)&(~BIT(1)))|((ant_txoffset>>8)<<1));
    write_reg8(0x170007,(read_reg8(0x170007)&0xfc)|0x02);//tx_ant_switch
 }

 /**
  * @brief      This function is mainly used to set the clock working mode of the antenna.
  * @para[in]   clk_mode    - Open all the time or only when switching antennas.
  * @return     none.
  */
 void rf_hadm_ant_clk_mode(rf_hadm_ant_clk_mode_e clk_mode)
 {
 //     write_reg8(0x17002b,read_reg8(0x17002b)&(~BIT(0)));
    reg_rf_rxclk_auto = ((reg_rf_rxclk_auto&0xfe) | clk_mode);
 }

 /**
  * @brief      This function is mainly used to set the way IQ sampling starts.
  * @para[in]   sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
  * @return     none.
  */
 void rf_hadm_iq_sample_mode(rf_hadm_iq_sample_mode_e sample_mode)
 {
    if(sample_mode == RF_HADM_IQ_SAMPLE_SYNC_MODE)
    {
        reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;
    }
    else
    {
        reg_rf_rxlatf &= (~FLD_RF_R_IQ_SAMP_MODE);
    }
 }

 /**
  * @brief      This function is mainly used to set the starting position of IQ sampling.
  * @para[in]   start_point  - Set the starting point of the sample.If it is rx_en mode, sampling starts
  *                               at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
  *                               starts at (start_point + 1) * 0.125us after sync.
  * @return     none.
  */
 void rf_hadm_iq_start_point(unsigned char start_point)
 {
    reg_rf_iq_samp_start = start_point;
 }

 /**
  * @brief      This function is mainly used to set the number of IQ samples in groups.
  * @para[in]   samp_num    - Number of groups to sample IQ data.
  * @return     none.
  */
 void rf_hadm_iq_sample_number(unsigned short samp_num)
 {
    reg_rf_iq_samp_num = samp_num;
    g_iq_group_num = samp_num;
 }

 /**
  * @brief      Mainly used to set thresholds when sync data packets.
  * @para[in]   thres_value  - The value of thresholds.
  * @return     none.
  */
 void rf_set_ble_sync_threshold(unsigned char thres_value)
 {
    reg_rf_modem_sync_thres_ble = thres_value;
 }

 /**
  * @brief      This function is mainly used to enable the IQ sampling function.
  * @return     none.
  */
 void rf_iq_sample_enable(void)
 {
    reg_rf_mode_ctrl0 |= FLD_RF_IQ_SAMP_EN;
 }

 /**
  * @brief      This function is mainly used to disable the IQ sampling function.
  * @return     none.
  */
 void rf_iq_sample_disable(void)
 {
    reg_rf_mode_ctrl0 &= (~FLD_RF_IQ_SAMP_EN);
 }

 /**
  * @brief      This function is mainly used to obtain the sync flag bit from the packet, which is
  *                 used to identify whether the packet is data received after passing synchronisation.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the Sync flag information in the packet.
  */
 unsigned char rf_hadm_sync_flag(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
    return ((p[x*sample_num+9]&BIT(3))>>3);
 }

 /**
  * @brief      This function is mainly used to obtain the packet quality indicator from the packet, which is
  *                 used to identify whether the packet is data received after passing synchronisation.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the packet quality information in the packet.
  */
 unsigned char rf_hadm_get_packet_quality_indicator(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0; unsigned char quality_indicator = 0;
    x = ((data_len >> 8) & 0xff);
    quality_indicator = 32 - (p[x*sample_num+16]);
    return  (quality_indicator > 2 ? 2 : quality_indicator);
 }

 /**
  * @brief      This function is mainly used to get the timestamp information from the packet that is
  *                 synchronised to the packet.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the Sync timestamp information in the packet.
  */
 unsigned int rf_hadm_get_pkt_rx_sync_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
    return (p[x*sample_num+7]<<24 | p[x*sample_num+6]<<16 | p[x*sample_num+5]<<8 | p[x*sample_num+4]);
 }

 /**
  * @brief      This function is mainly used to obtain the timestamp information of the tx_pos from the packet.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the timestamp information in the packet.
  */
 unsigned int rf_hadm_get_pkt_tx_pos_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
    return (p[x*sample_num+31]<<24 | p[x*sample_num+30]<<16 | p[x*sample_num+29]<<8 | p[x*sample_num+28]);
 }

 /**
  * @brief      This function is mainly used to obtain the timestamp information of the tx_neg from the packet.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the timestamp information in the packet.
  */
 unsigned int rf_hadm_get_pkt_tx_neg_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
    return (p[x*sample_num+35]<<24 | p[x*sample_num+34]<<16 | p[x*sample_num+33]<<8 | p[x*sample_num+32]);
 }

 /**
  * @brief      This function is mainly used to obtain the timestamp information of the iq_start from the packet.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the timestamp information in the packet.
  */
 unsigned int rf_hadm_get_pkt_iq_start_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
    return (p[x*sample_num+15]<<24 | p[x*sample_num+14]<<16 | p[x*sample_num+13]<<8 | p[x*sample_num+12]);
 }

 /**
  * @brief      This function is mainly used to obtain the rssi information from the packet.
  * @param[in]  p           - The packet address.
  * @param[in]  sample_num  - The number of sample points that the packet contains.
  * @param[in]  data_len    - The data length of the sample point in the packet.
  * @return     Returns the rssi information in the packet.
  */
 signed char rf_hadm_get_pkt_rssi_value(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len)
 {
    unsigned char x = 0;
    x = ((data_len >> 8) & 0xff);
    return (p[x*sample_num+10]-110);
 }

 /**
  * @brief      This function serves to set RF's channel.The step of this function is in KHz.
  *             The frequency set by this function is (chn+2400) MHz+chn_k KHz.
  * @param[in]   chn_m - RF channel. The unit of this parameter is MHz, and its set frequency
  *                         point is (2400+chn)MHz.
  * @param[in]   chn_k - The unit of this parameter is KHz, which means to shift chn_k KHz to
  *                         the right on the basis of chn.Its value ranges from 0 to 999.
  * @param[in]  trx_mode - Defines the frequency point setting of tx mode or rx mode.
  * @return     none.
  */
 void rf_set_channel_k_step(signed char chn_m,unsigned int chn_k,rf_trx_chn_e trx_mode)//general
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
  * @brief      This function is mainly used for frezee agc.
  * @return     none.
  * @note       It should be noted that this function should be called after receiving the package.
  */
 void rf_agc_disable(void)
 {
    char gain_lat, lna_hgain, lna_lgain, lna_attn, cbpf_gain;
    reg_rf_radio_txrx_dbg1_0 |= FLD_RF_AGC_DISABLE;
    gain_lat = (read_reg8(0x170059)>>4)&0x07;
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
  * @brief      This function is mainly used for agc auto run.
  * @return     none.
  * @note       It needs to be called before sending and receiving packets after the tone interaction is complete.
  */
 void rf_agc_enable(void)
 {
    reg_rf_radio_txrx_dbg1_0 &= (~FLD_RF_AGC_DISABLE);
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(1)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(2)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(3)));
    write_reg8(0x170780,read_reg8(0x170780)&(~BIT(1)));
 }

 /**
  * @brief      This function is mainly used to set the sequence related to Fast Settle in HADM.
  * @return     none.
  */
 void rf_fast_settle_sequence_set(void)
 {
    //seq_ldo_pll_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(3));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(3));

    //seq_ldo_vco_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(4));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(4));

    //seq_ldo_pll_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(3)));
    write_reg8(0x170761,read_reg8(0x170761)|BIT(3));

    //rf_seq_ldo_vco_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(4)));
    write_reg8(0x170761,read_reg8(0x170761)|BIT(4));

    //seq_pd_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(0));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(0));

    //seq_pd_en_fcal_bias_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));

    //seq_xo_en_clk_ref_ow
    write_reg8(0x170770,read_reg8(0x170770)|BIT(3));
    write_reg8(0x170770,read_reg8(0x170770)|BIT(1));

    //seq_vco_pup_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(0));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(0));

    //seq_lo_pup_vlo_fbk_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(6));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(6));

    //seq_fcal_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));

    //_seq_fcal_set_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));

    //seq_fcal_run_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));

    //seq_divn_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(6));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(6));

    //seq_divn_openloop_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(7)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(7));

    //ldo_rxtxhf_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(1));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(1));

    //ldo_lv_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(0));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(0));

    //bg_pup_ow
    write_reg8(0x170766,read_reg8(0x170766)|BIT(0));
    write_reg8(0x170764,read_reg8(0x170764)|BIT(0));

    //rf_mixer_pup_ow
    write_reg8(0x17077b,read_reg8(0x17077b)|BIT(3));
    write_reg8(0x170778,read_reg8(0x170778)|BIT(4));

    //dsm_run
    write_reg8(0x170682,read_reg8(0x170682)|BIT(0));
    write_reg8(0x170680,read_reg8(0x170680)|BIT(0));

    //rf_rx_dig_mixer_en_ow
    write_reg8(0x170688,read_reg8(0x170688)|BIT(1));
    write_reg8(0x170686,read_reg8(0x170686)|BIT(1));

    //rf_hpm_cal_disable
    write_reg8(0x170688,read_reg8(0x170688)&(~BIT(3)));
    write_reg8(0x170686,read_reg8(0x170686)|BIT(3));

    //rf_seq_lo_pup_vlo_txfsk_ow
    write_reg8(0x170792,read_reg8(0x170792)|BIT(6));
    write_reg8(0x170790,read_reg8(0x170790)|BIT(6));
    write_reg8(0x170792,read_reg8(0x170792)|BIT(7));
    write_reg8(0x170790,read_reg8(0x170790)|BIT(7));

    //seq_lo_pup_vlo_rx_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(2));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(2));
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(3));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(3));
 }

 /**
  * @brief      This function is mainly used to set the value of the dac_pup.
  * @param[in]  value   - The value of dac_pup.
  * @return     none.
  */
 void rf_seq_dac_pup_ow(unsigned char value)
 {
    write_reg8(0x17078e,(read_reg8(0x17078e)&0x7f) | (value&0x01)<<7);//tx:value = 1
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(7));
 }

 /**
  * @brief      This function is mainly used to set the value of the pa_pup.
  * @param[in]  value   - The value of pa_pup.
  * @return     none.
  */
 void rf_seq_tx_pa_pup_ow(unsigned char value)
 {
    write_reg8(0x17077c,(read_reg8(0x17077c)&0xfe)|(value&0x01));//tx value = 1
    write_reg8(0x170778,read_reg8(0x170778)|BIT(5));
 }

 /**
  * @brief      This function is mainly used to open the PA module.
  * @param[in]  pwr     - The slice value of power.
  * @return     none.
  */
 void rf_pa_pwr_on(unsigned char pwr)
 {
    write_reg8(0x17077c,(read_reg8(0x17077c)&0x81)|(pwr<<1));
    write_reg8(0x170778,read_reg8(0x170778)|BIT(6));
 }

 /**
  * @brief      This function is mainly used to close the PA module.
  * @return     none.
  */
 void rf_pa_pwr_off(void)
 {
    write_reg8(0x17077c,(read_reg8(0x17077c)&0x81)|(0<<1));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(6)));
 }

 /**
  * @brief      This function is mainly used to set the preparation and enable of manual fcal.
  * @return     none.
  */
 void rf_manual_fcal_setup(void)
 {
 // rf_seq_pd_en_pd_drv_ow(0);
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(1));
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1)));

    write_reg8(0x170738,read_reg8(0x170738)|BIT(2));

 // rf_seq_pd_en_fcal_bias_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(2));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));

 // rf_seq_fcal_pup_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(3));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));

 // rf_seq_fcal_set_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4)));

 // rf_seq_fcal_run_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5)));

    write_reg8(0x170683,read_reg8(0x170683)|BIT(3));
 }

 /**
  * @brief      This function is mainly used to set the relevant value after manual fcal.
  * @return     none.
  * @note       The function needs to be called after the rf_manual_fcal_setup call 22us.
  */
 void rf_manual_fcal_done(void)
 {
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));
    write_reg8(0x170738,read_reg8(0x170738)&(~BIT(2)));

 // rf_seq_pd_en_fcal_bias_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));
 // write_reg8(0x170788,read_reg8(0x170788)|BIT(2));

 // rf_seq_fcal_pup_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));
 // write_reg8(0x170788,read_reg8(0x170788)|BIT(3));

 // rf_seq_fcal_set_ow();
 // write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));

 // rf_seq_fcal_run_ow();
 // write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));

 // rf_seq_pd_en_pd_drv_ow(1);
    write_reg8(0x170788,read_reg8(0x170788)|BIT(1));
 }
 #endif

#endif

