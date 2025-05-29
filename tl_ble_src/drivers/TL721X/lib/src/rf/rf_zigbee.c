/********************************************************************************************************
 * @file    rf_zigbee.c
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

/**
 * @brief     This function serves to  set zigbee_dcoc  mode of RF.
 * @return    none.
 */
void rf_set_zigbee_250K_mode(void)
{
    write_reg8(0x17063d, 0x41); //ble:bw_code.
    write_reg8(0x170620, 0x00); //sc_code.
    write_reg8(0x170621, 0x2a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x170622, 0x43); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x170623, 0x06); //HPMC_EXP_DIFF_COUNT_H.
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


    write_reg8(0x170799, 0x01); //ZB_FREQ_FIXED_OW
    write_reg8(0x17062d, 0x01); //CHNL_NUM switch by sw en
#if (!RF_ZIGBEE_OLD_DATA_PATH)
    write_reg8(0x17044e, 0x14); //ble sync threshold:To modem.
    write_reg8(0x17044c, 0x3c); //Rx: sfd match symb num

    write_reg8(0x17043b, 0x7c); //Rx: sfd match symb0 num
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x8d); //modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x17042c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170426, 0x00); //modem:sync rst sel,for zigbee access code sync
    //dcest
    write_reg8(0x170450, 0xff); //dciq edr  auto
    write_reg8(0x170451, 0x0f); //edr dcoc auto


    //pdet sync thd default:0x190 [9'd400]
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    //pdet hardec thd default:0x18 [24]
    write_reg8(0x1704e2, 0x1a); //pdet_hardec_thd

#else
    write_reg8(0x17044e, 0x18); //ble sync threshold:To modem.
    write_reg8(0x17044c, 0x4c); //Rx: sfd match symb num
    write_reg8(0x17043b, 0x1c); //Rx: sfd match symb0 num
    //AURA enable TX zigbee and disable MSK
    //1:zigbee 250k , 2: hb1m , 4:hb2m , 8:hb500k
    write_reg8(0x170421, 0x0d); //modem:ZIGBEE_MODE:01. enable RX mode
    write_reg8(0x17042c, 0x39); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170426, 0x02); //modem:sync rst sel,for zigbee access code sync
    write_reg8(0x170450, 0x3f); //dciq edr  auto
    write_reg8(0x170451, 0x0e); //edr dcoc auto
    write_reg8(0x1704e0, 0x90); //sync_thd
    write_reg8(0x1704e1, 0x19); //sync_thd    0x1f4 == 500  0x1c2 == 450  0x1a4 == 420
    write_reg8(0x1704e2, 0x18); //pdet_hardec_thd
#endif

    write_reg8(0x170436, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x170437, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x170438, 0xb6); //LR_TIM_EDGE_DEV.
    write_reg8(0x170439, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x170473, 0x01); //TOT_DEV_RST.

    write_reg8(0x17049a, 0x00); //tx_tp_align.
    write_reg8(0x1704c2, 0x3d); //grx_0.
    write_reg8(0x1704c3, 0x4b); //grx_1.
    write_reg8(0x1704c4, 0x5a); //grx_2.
    write_reg8(0x1704c5, 0x67); //grx_3.
    write_reg8(0x1704c6, 0x73); //grx_4.
    write_reg8(0x1704c7, 0x7e); //grx_5.
    write_reg8(0x1704c8, 0x39); //default:0x00->0x39 Gain offset to compensate system error

    write_reg8(0x170000, 0x03); //tx_mode.
    write_reg8(0x170001, 0x00); //PN.
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
    write_reg8(0x170132, 0x01);        //zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet
    write_reg8(0x1704bb, 0x00);        //disable 2 stage filter
    write_reg8(0x17043e, 0x81);        //BIT<7>:0 new ,1 old;pm2fm suppress more than pi/4

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0, 0x1c); //defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    rf_set_crc_config(&rf_crc_config[2]);
    g_rfmode = RF_MODE_ZIGBEE_250K;
}
