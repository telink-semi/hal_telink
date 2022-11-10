/******************************************************************************
 * Copyright (c) 2022 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *****************************************************************************/

/********************************************************************************************************
 * @file	rf.c
 *
 * @brief	This is the source file for B91
 *
 * @author	Driver Group
 *
 *******************************************************************************************************/
#include "rf.h"
#include "compiler.h"
#include "dma.h"


/**********************************************************************************************************************
 *                                         RF global constants                                                        *
 *********************************************************************************************************************/
/**
 * @brief The table of rf power level.
 */
const rf_power_level_e rf_power_Level_list[30] =
{
	 /*VBAT*/
	 RF_POWER_P9p11dBm,
	 RF_POWER_P8p57dBm,
	 RF_POWER_P8p05dBm,
	 RF_POWER_P7p45dBm,
	 RF_POWER_P6p98dBm,
	 RF_POWER_P5p68dBm,
	 /*VANT*/
	 RF_POWER_P4p35dBm,
	 RF_POWER_P3p83dBm,
	 RF_POWER_P3p25dBm,
	 RF_POWER_P2p79dBm,
	 RF_POWER_P2p32dBm,
	 RF_POWER_P1p72dBm,
	 RF_POWER_P0p80dBm,
	 RF_POWER_P0p01dBm,
	 RF_POWER_N0p53dBm,
	 RF_POWER_N1p37dBm,
	 RF_POWER_N2p01dBm,
	 RF_POWER_N3p37dBm,
	 RF_POWER_N4p77dBm,
	 RF_POWER_N6p54dBm,
	 RF_POWER_N8p78dBm,
	 RF_POWER_N12p06dBm,
	 RF_POWER_N17p83dBm,
	 RF_POWER_N23p54dBm,
};

rf_mode_e   g_rfmode;

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/

/**
 * @brief     This function serves to initiate information of RF.
 * @return	   none.
 */
void rf_mode_init(void)
{
	write_reg8(0x140ed2,0x9b);//DCOC_SFIIP DCOC_SFQQP
	write_reg8(0x140ed3,0x19);//DCOC_SFQQ
#if RF_RX_SHORT_MODE_EN
	write_reg8(0x140c7b,0x0e);//BLANK_WINDOW
#else
	write_reg8(0x140c7b,0xfe);//BLANK_WINDOW
#endif

	write_reg8(0x140e4a,0x0e);//POW_000_001
	write_reg8(0x140e4b,0x09);//POW_001_010_H
	write_reg8(0x140e4e,0x09);//POW_100_101 //POW_101_100_L
	write_reg8(0x140e4f,0x0f);//POW_101_100_H
	write_reg8(0x140e54,0x0e);//POW_001_010_L
	write_reg8(0x140e55,0x09);//POW_001_010_H
	write_reg8(0x140e56,0x0c);//POW_011_100_L
	write_reg8(0x140e57,0x08);//POW_011_100_H
	write_reg8(0x140e58,0x09);//POW_101_100_L
	write_reg8(0x140e59,0x0f);//POW_101_100_H

	write_reg8(0x140c76,0x50);//FREQ_CORR_CFG2_0
	write_reg8(0x140c77,0x73);//FREQ_CORR_CFG2_1
#if RF_RX_SHORT_MODE_EN
	write_reg8(0x14083a,0x86);//rx_ant_offset  rx_dly(0x140c7b,0x140c79,0x14083a,0x14083b)
	write_reg8(0x14083b,0x65);//samp_offset
#endif
	analog_write_reg8(0x8b,0x04);//FREQ_CORR_CFG2_1
}

/**
 * @brief     This function serves to  set zigbee_250K  mode of RF.
 * @return	  none.
 */
void rf_set_zigbee_250K_mode(void)
{
	write_reg8(0x140e3d,0x41);//ble:bw_code.
	write_reg8(0x140e20,0x06);//sc_code.
	write_reg8(0x140e21,0x2a);//if_freq,IF = 1Mhz,BW = 1Mhz.
	write_reg8(0x140e22,0x43);//HPMC_EXP_DIFF_COUNT_L.
	write_reg8(0x140e23,0x26);//HPMC_EXP_DIFF_COUNT_H.
	write_reg8(0x140e3f,0x00);//250k modulation index:telink add rx for 250k/500k.
	write_reg8(0x140c3f,0x00);//LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
	write_reg8(0x140c20,0x84);// script cc.

	write_reg8(0x140c22,0x01);//modem:BLE_MODE_TX,2MBPS.
	write_reg8(0x140c4e,0x18);//ble sync thre:To modem.
	write_reg8(0x140c4d,0x0f);//r_rxchn_en_i:To modem.
	write_reg8(0x140c21,0x01);//modem:ZIGBEE_MODE:01.
	write_reg8(0x140c23,0x80);//modem:ZIGBEE_MODE_TX.
	write_reg8(0x140c26,0x02);//modem:sync rst sel,for zigbee access code sync.
	write_reg8(0x140c2a,0x10);//modem:disable MSK.
	write_reg8(0x140c3d,0x01);//modem:zb_sfd_frm_ll.
	write_reg8(0x140c2c,0x39);//modem:zb_dis_rst_pdet_isfd.
	write_reg8(0x140c36,0xb7);//LR_NUM_GEAR_L.
	write_reg8(0x140c37,0x0e);//LR_NUM_GEAR_H.
	write_reg8(0x140c38,0xc4);//LR_TIM_EDGE_DEV.
	write_reg8(0x140c39,0x71);//LR_TIM_REC_CFG_1.
	write_reg8(0x140c73,0x01);//TOT_DEV_RST.

#if RF_RX_SHORT_MODE_EN
	write_reg8(0x140c79,0x30);//RX_DIS_PDET_BLANK.
#else
	write_reg8(0x140c79,0x00);//RX_DIS_PDET_BLANK.
#endif
	write_reg8(0x140c9a,0x00);//tx_tp_align.
	write_reg8(0x140cc2,0x36);//grx_0.
	write_reg8(0x140cc3,0x48);//grx_1.
	write_reg8(0x140cc4,0x54);//grx_2.
	write_reg8(0x140cc5,0x62);//grx_3.
	write_reg8(0x140cc6,0x6e);//grx_4.
	write_reg8(0x140cc7,0x79);//grx_5.

	write_reg8(0x140800,0x13);//tx_mode.
	write_reg8(0x140801,0x00);//PN.
	write_reg8(0x140802,0x42);//preamble len.
	write_reg8(0x140803,0x44);//bit<0:1>private mode control. bit<2:3> tx mode.
	write_reg8(0x140804,0xe0);//bit<4>mode:1->1m;bit<0:3>:ble head.
	write_reg8(0x140805,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

	write_reg32(0x140808,0x000000a7);//access code for zigbee 250K.
	write_reg32(0x140810,0x000000d1);//access code for hybee 1m.
	write_reg8(0x140818,0x95);//access code for hybee 2m.
	write_reg8(0x140819,0x0c);//access code for hybee 500K.

	write_reg8(0x140821,0x23);//rx packet len 0 enable.
	write_reg8(0x140822,0x00);//rxchn_man_en.
	write_reg8(0x140c4c,0x4c);//RX:acc_len modem.

	g_rfmode = RF_MODE_ZIGBEE_250K;
}

/**
 * @brief     This function serves to  set ble_1M  mode of RF.
 * @return	  none.
 */
void rf_set_ble_1M_mode(void)
{
	write_reg8(0x140e3d,0x61);//ble:bw_code.
	write_reg8(0x140e20,0x16);//sc_code.
	write_reg8(0x140e21,0x0a);//if_freq,IF = 1Mhz,BW = 1Mhz.
	write_reg8(0x140e22,0x20);//HPMC_EXP_DIFF_COUNT_L.
	write_reg8(0x140e23,0x23);//HPMC_EXP_DIFF_COUNT_H.
	write_reg8(0x140e3f,0x00);//250k modulation index:telink add rx for 250k/500k.
	write_reg8(0x140c3f,0x00);//LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
	write_reg8(0x140c20,0x84);// script cc.

	write_reg8(0x140c22,0x00);//modem:BLE_MODE_TX,2MBPS.
	write_reg8(0x140c4e,0x1e);//ble sync thre:To modem.
	write_reg8(0x140c4d,0x01);//r_rxchn_en_i:To modem.
	write_reg8(0x140c21,0x00);//modem:ZIGBEE_MODE:01.
	write_reg8(0x140c23,0x00);//modem:ZIGBEE_MODE_TX.
	write_reg8(0x140c26,0x00);//modem:sync rst sel,for zigbee access code sync.
	write_reg8(0x140c2a,0x10);//modem:disable MSK.
	write_reg8(0x140c3d,0x00);//modem:zb_sfd_frm_ll.
	write_reg8(0x140c2c,0x38);//modem:zb_dis_rst_pdet_isfd.
	write_reg8(0x140c36,0xb7);//LR_NUM_GEAR_L.
	write_reg8(0x140c37,0x0e);//LR_NUM_GEAR_H.
	write_reg8(0x140c38,0xc4);//LR_TIM_EDGE_DEV.
	write_reg8(0x140c39,0x71);//LR_TIM_REC_CFG_1.
	write_reg8(0x140c73,0x01);//TOT_DEV_RST.

#if RF_RX_SHORT_MODE_EN
	write_reg8(0x140c79,0x38);//RX_DIS_PDET_BLANK.
#else
	write_reg8(0x140c79,0x08);//RX_DIS_PDET_BLANK.
#endif
	write_reg8(0x140c9a,0x00);//tx_tp_align.
	write_reg8(0x140cc2,0x39);//grx_0.
	write_reg8(0x140cc3,0x4b);//grx_1.
	write_reg8(0x140cc4,0x56);//grx_2.
	write_reg8(0x140cc5,0x62);//grx_3.
	write_reg8(0x140cc6,0x6e);//grx_4.
	write_reg8(0x140cc7,0x79);//grx_5.

	write_reg8(0x140800,0x1f);//tx_mode.
	write_reg8(0x140801,0x08);//PN.
	write_reg8(0x140802,0x46);//preamble len 0x46 for ble confirmed by biao.li.20200828.
	write_reg8(0x140803,0x44);//bit<0:1>private mode control. bit<2:3> tx mode.
	write_reg8(0x140804,0xf5);//bit<4>mode:1->1m;bit<0:3>:ble head.
	write_reg8(0x140805,0x04);//lr mode bit<4:5> 0:off,3:125k,2:500k.

	write_reg8(0x140821,0xa1);//rx packet len 0 enable.
	write_reg8(0x140822,0x00);//rxchn_man_en.
	write_reg8(0x140c4c,0x4c);//RX:acc_len modem.

	write_reg32(0x140808,0x00000000);
	write_reg8(0x140830,0x36);
	write_reg8(0x140a06,0x00);
	write_reg8(0x140a0c,0x50);
	write_reg8(0x140a0e,0x00);
	write_reg8(0x140a10,0x00);

	g_rfmode = RF_MODE_BLE_1M;
}

/**
 * @brief 	   This setting serve to set the configuration of Tx DMA.
 */
dma_config_t rf_tx_dma_config={
	.dst_req_sel= DMA_REQ_ZB_TX,//tx req.
	.src_req_sel=0,
	.dst_addr_ctrl=DMA_ADDR_FIX,
	.src_addr_ctrl=DMA_ADDR_INCREMENT,//increment.
	.dstmode=DMA_HANDSHAKE_MODE,//handshake.
	.srcmode=DMA_NORMAL_MODE,
	.dstwidth=DMA_CTR_WORD_WIDTH,//must word.
	.srcwidth=DMA_CTR_WORD_WIDTH,//must word.
	.src_burst_size=0,//must 0.
	.vacant_bit=0,
	.read_num_en=1,
	.priority=0,
	.write_num_en=0,
	.auto_en=1,//must 1.
};

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] none
 * @return	  none.
 */
void rf_set_tx_dma_config(void)
{
	reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN|FLD_RF_CH_0_RNUM_EN_BK);//u_pd_mcu.u_dmac.atcdmac100_ahbslv.tx_multi_en,rx_multi_en,ch_0_rnum_en_bk.
	dma_config(DMA0,&rf_tx_dma_config);
	dma_set_dst_address(DMA0,reg_rf_txdma_adr);
}

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] fifo_depth 		- tx chn deep.
 * @param[in] fifo_byte_size 	- tx_idx_addr = {tx_chn_adr*bb_tx_size,4'b0}.
 * @return	  none.
 */
void rf_set_tx_dma(unsigned char fifo_dep,unsigned short fifo_byte_size)
{
	rf_set_tx_dma_config();
	rf_set_tx_dma_fifo_num(fifo_dep);
	rf_set_tx_dma_fifo_size(fifo_byte_size);

}

/**
 * @brief 	   This setting serve to set the configuration of Rx DMA.
 * @note	   In this struct write_num_en must be 0;This seeting will cause the conflict of DMA.
 */
dma_config_t rf_rx_dma_config={
		.dst_req_sel= 0,//tx req.
		.src_req_sel=DMA_REQ_ZB_RX,
		.dst_addr_ctrl=0,
		.src_addr_ctrl=DMA_ADDR_FIX,//increment.
		.dstmode=DMA_NORMAL_MODE,
		.srcmode=DMA_HANDSHAKE_MODE,//handshake.
		.dstwidth=DMA_CTR_WORD_WIDTH,//must word.
		.srcwidth=DMA_CTR_WORD_WIDTH,//must word.
		.src_burst_size=0,//must 0.
		.vacant_bit=0,
		.read_num_en=0,
		.priority=0,
		.write_num_en=0,//must 0.
		.auto_en=1,//must 1.
};

/**
 * @brief		This function serve to rx dma config
 * @param[in]	none
 * @return		none
 */
void rf_set_rx_dma_config(void)
{
	reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN|FLD_RF_CH_0_RNUM_EN_BK);//ch0_rnum_en_bk,tx_multi_en,rx_multi_en.
	dma_config(DMA1,&rf_rx_dma_config);
	dma_set_src_address(DMA1,reg_rf_rxdma_adr);
	reg_dma_size(1)=0xffffffff;
}

/**
 * @brief     This function serves to rx dma setting.
 * @param[in] buff 		 	  - The buffer that store received packet.
 * @param[in] wptr_mask  	  - DMA fifo mask value (0~fif0_num-1).
 * @param[in] fifo_byte_size  - The length of one dma fifo.
 * @return	  none.
 */
void rf_set_rx_dma(unsigned char *buff,unsigned char wptr_mask,unsigned short fifo_byte_size)
{
	rf_set_rx_dma_config();
	rf_set_rx_buffer(buff);
	rf_set_rx_dma_fifo_num(wptr_mask);
	rf_set_rx_dma_fifo_size(fifo_byte_size);
}

volatile unsigned char  g_single_tong_freqoffset = 0;//for eliminate single carrier frequency offset.

/**
 * @brief   	This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return  	none.
 */


_attribute_ram_code_
void rf_set_ble_chn (signed char chn_num)
{
    write_reg8 (0x14080d, chn_num);
	if (chn_num < 11)
		chn_num += 2;
	else if (chn_num < 37)
		chn_num += 3;
	else if (chn_num == 37)
		chn_num = 1;
	else if (chn_num == 38)
		chn_num = 13;
	else if	(chn_num == 39)
		chn_num = 40;
	else if	(chn_num < 51)
		chn_num = chn_num;
	else if(chn_num <= 61)
		chn_num = -61 + chn_num;

	chn_num = chn_num << 1;
	rf_set_chn(chn_num);

}

/**
 * @brief   	This function serves to set rf channel for all mode.The actual channel set by this function is 2400+chn.
 * @param[in]   chn   - That you want to set the channel as 2400+chn.
 * @return  	none.
 */
_attribute_ram_code_
void rf_set_chn(signed char chn)
{
	unsigned int freq_low;
	unsigned int freq_high;
	unsigned int chnl_freq;
	unsigned char ctrim;
	unsigned int freq;

	freq = 2400+chn;
	if(freq >= 2550){
		ctrim = 0;
	}
	else if(freq >= 2520){
		ctrim = 1;
	}
	else if(freq >= 2495){
		ctrim = 2;
	}
	else if(freq >= 2465){
		ctrim = 3;
	}
	else if(freq >= 2435){
		ctrim = 4;
	}
	else if(freq >= 2405){
		ctrim = 5;
	}
	else if(freq >= 2380){
		ctrim = 6;
	}
	else{
		ctrim = 7;
	}

	chnl_freq = freq*2 + g_single_tong_freqoffset;
	freq_low  = (chnl_freq & 0x7f);
	freq_high = ((chnl_freq>>7)&0x3f);

	write_reg8(0x140e44,  (read_reg8(0x140e44) | 0x01 ));
	write_reg8(0x140e44,  (read_reg8(0x140e44) & 0x01) | freq_low << 1);
	write_reg8(0x140e45,  (read_reg8(0x140e45) & 0xc0) | freq_high);
	write_reg8(0x140e29,  (read_reg8(0x140e29) & 0x1f) | (ctrim<<5) );  //FE_CTRIM
}

/**
 * @brief	  	This function serves to get rssi.
 * @return	 	rssi value.
 */
signed char rf_get_rssi(void)
{
	return (((signed char)(read_reg8(REG_TL_MODEM_BASE_ADDR+0x5d))) - 110);//this function can not tested on fpga
}

/**
 * @brief   	This function serves to set RF Rx manual on.
 * @return  	none.
 */
void rf_set_rxmode(void)
{
	reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
	reg_rf_modem_mode_cfg_rx1_0 |= FLD_RF_CONT_MODE;//set continue mode.
	reg_rf_ll_ctrl0 |= FLD_RF_R_RX_EN_MAN;//rx enable.
	reg_rf_rxmode |= FLD_RF_RX_ENABLE;//bb rx enable.


}

/**
 * @brief  	 	This function serves to set RF Tx mode.
 * @return  	none.
 */
void rf_set_txmode(void)
{
	reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
	reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
	reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}

/**
 * @brief	  	This function serves to set RF Tx packet address to DMA src_addr.
 * @param[in]	addr   - The packet address which to send.
 * @return	 	none.
 */
void rf_tx_pkt(void* addr)
{
	dma_set_src_address(DMA0,convert_ram_addr_cpu2bus(addr));
	reg_dma_ctr0(0) |= 0x01;
}

/**
 * @brief   	This function serves to set RF power level.
 * @param[in]   level 	 - The power level to set.
 * @return 		none.
 */
void rf_set_power_level(rf_power_level_e level)
{
	unsigned char value;
	if(level&BIT(7))
	{
		reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;
	}
	else
	{
		reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
	}

	value = (unsigned char)(level & 0x3F);
	reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value&0x01)<<7));
	reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value>>1)&0x1f));
}

/**
 * @brief	  	This function serves to start tx of auto mode. In this mode,
 *				RF module stays in tx status until a packet is sent or it fails to sent packet when timeout expires.
 *				Timeout duration is set by the parameter "tick".
 *				The address to store send data is set by the function "addr".
 * @param[in]	addr   - The address to store send data.
 * @param[in]	tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
 * @return	 	none.
 */
void rf_start_btx (void* addr, unsigned int tick)
{
	write_reg32(0x80140a18, tick);
	reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHDULE_EN;	// Enable cmd_schedule mode.
	dma_set_src_address(DMA0,convert_ram_addr_cpu2bus(addr));
	write_reg8 (0x80140a00, 0x81);						// ble tx.
}

/**
 * @brief	  	This function serves to start Rx of auto mode. In this mode,
 *				RF module stays in Rx status until a packet is received or it fails to receive packet when timeout expires.
 *				Timeout duration is set by the parameter "tick".
 *				The address to store received data is set by the function "addr".
 * @param[in]	addr   - The address to store received data.
 * @param[in]	tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
 * @return	 	none
 */
void rf_start_brx  (void* addr, unsigned int tick)
{
	write_reg32 (0x80140a28, 0x0fffffff);
	write_reg32(0x80140a18, tick);
	reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHDULE_EN;	// Enable cmd_schedule mode.
	dma_set_src_address(DMA0,convert_ram_addr_cpu2bus(addr));
	write_reg8 (0x80140a00, 0x82);// ble rx.
}

/**
 * @brief     	This function serves to RF trigger stx2rx.
 * @param[in] 	addr  - DMA tx buffer.
 * @param[in] 	tick  - Trigger tx send packet after tick delay.
 * @return	    none.
 */
void rf_start_stx2rx  (void* addr, unsigned int tick)
{
	dma_set_src_address(DMA0,convert_ram_addr_cpu2bus(addr));
	write_reg32(0x80140a18, tick);
	reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHDULE_EN;	// Enable cmd_schedule mode.
	write_reg8  (0x80140a00, 0x87);	// single tx2rx.
}

/**
 * @brief     	This function serves to RF trigger stx.
 * @param[in] 	addr  - DMA tx buffer.
 * @param[in] 	tick  - Trigger tx after tick delay.
 * @return	   	none.
 */
void rf_start_stx  (void* addr,  unsigned int tick)
{
	dma_set_src_address(DMA0,convert_ram_addr_cpu2bus(addr));
	reg_rf_ll_cmd_schedule = tick;
	reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHDULE_EN;	// Enable cmd_schedule mode.
	reg_rf_ll_cmd = 0x85;
}

/**
 * @brief      This function serves to reset baseband
 * @return     none
 */
void rf_baseband_reset(void)
{
	reg_rst3 &= (~FLD_RST3_ZB);      		  // reset baseband
	reg_rst3 |= (FLD_RST3_ZB);				  // clr baseband
}

/**
 * @brief   	This function serves to set RF power through select the level index.
 * @param[in]   idx 	 - The index of power level which you want to set.
 * @return  	none.
 */
void rf_set_power_level_index(rf_power_level_index_e idx)
{
	unsigned char value;
	unsigned char level = 0;

	if(idx < sizeof(rf_power_Level_list)/sizeof(rf_power_Level_list[0]))
	{
		level = rf_power_Level_list[idx];
	}

	if(level&BIT(7))
	{
		reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;
	}
	else
	{
		reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
	}

	value = (unsigned char)(level & 0x3F);

	reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value&0x01)<<7));
	reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value>>1)&0x1f));

}

void rf_reset_dma(void)
{
	write_reg8(0x80100400, 0x12); // DMAC_ID0
	write_reg8(0x80100401, 0x10); // DMAC_ID1
	write_reg8(0x80100402, 0x02); // DMAC_ID2
	write_reg8(0x80100403, 0x01); // DMAC_ID3
	write_reg8(0x80100410, 0x88); // DMAC_CFG0
	write_reg8(0x80100411, 0x60); // DMAC_CFG1
	write_reg8(0x80100413, 0x80); // DMAC_CFG3
	write_reg8(0x80100430, 0x00); // DMAC_ISR0
	write_reg8(0x80100431, 0x00); // DMAC_ISR1
	write_reg8(0x80100432, 0x00); // DMAC_ISR2
	write_reg8(0x80100434, 0x00); // DMAC_CER0
	write_reg8(0x80100435, 0x00); // DMAC_CER1
	write_reg8(0x80100436, 0x00); // DMAC_CER2
	write_reg8(0x80100437, 0x00); // DMAC_CER3
	write_reg8(0x80100440, 0x00); // DMAC_CAR
	write_reg8(0x80100444, 0x00); // DMAC_CH0_CR0
	write_reg8(0x80100445, 0x00); // DMAC_CH0_CR1
	write_reg8(0x80100446, 0x00); // DMAC_CH0_CR2
	write_reg8(0x80100447, 0x00); // DMAC_CH0_CR3
	write_reg8(0x80100448, 0x0); // DMAC_CH0_SRCAddr0
	write_reg8(0x80100449, 0x0); // DMAC_CH0_SRCAddr1
	write_reg8(0x8010044a, 0x0); // DMAC_CH0_SRCAddr2
	write_reg8(0x8010044b, 0x0); // DMAC_CH0_SRCAddr3
	write_reg8(0x8010044c, 0x0); // DMAC_CH0_DSTAddr0
	write_reg8(0x8010044d, 0x0); // DMAC_CH0_DSTAddr1
	write_reg8(0x8010044e, 0x0); // DMAC_CH0_DSTAddr2
	write_reg8(0x8010044f, 0x0); // DMAC_CH0_DSTAddr3
	write_reg8(0x80100450, 0x0); // DMAC_CH0_TSR0
	write_reg8(0x80100451, 0x0); // DMAC_CH0_TSR1
	write_reg8(0x80100452, 0x0); // DMAC_CH0_TSR2
	write_reg8(0x80100454, 0x0); // DMAC_CH0_LLP0
	write_reg8(0x80100455, 0x0); // DMAC_CH0_LLP1
	write_reg8(0x80100456, 0x0); // DMAC_CH0_LLP2
	write_reg8(0x80100457, 0x0); // DMAC_CH0_LLP3
	write_reg8(0x80100458, 0x00); // DMAC_CH1_CR0
	write_reg8(0x80100459, 0x00); // DMAC_CH1_CR1
	write_reg8(0x8010045a, 0x00); // DMAC_CH1_CR2
	write_reg8(0x8010045b, 0x00); // DMAC_CH1_CR3
	write_reg8(0x8010045c, 0x0); // DMAC_CH1_SRCAddr0
	write_reg8(0x8010045d, 0x0); // DMAC_CH1_SRCAddr1
	write_reg8(0x8010045e, 0x0); // DMAC_CH1_SRCAddr2
	write_reg8(0x8010045f, 0x0); // DMAC_CH1_SRCAddr3
	write_reg8(0x80100460, 0x0); // DMAC_CH1_DSTAddr0
	write_reg8(0x80100461, 0x0); // DMAC_CH1_DSTAddr1
	write_reg8(0x80100462, 0x0); // DMAC_CH1_DSTAddr2
	write_reg8(0x80100463, 0x0); // DMAC_CH1_DSTAddr3
	write_reg8(0x80100464, 0x0); // DMAC_CH1_TSR0
	write_reg8(0x80100465, 0x0); // DMAC_CH1_TSR1
	write_reg8(0x80100466, 0x0); // DMAC_CH1_TSR2
	write_reg8(0x80100468, 0x0); // DMAC_CH1_LLP0
	write_reg8(0x80100469, 0x0); // DMAC_CH1_LLP1
	write_reg8(0x8010046a, 0x0); // DMAC_CH1_LLP2
	write_reg8(0x8010046b, 0x0); // DMAC_CH1_LLP3
	write_reg8(0x8010046c, 0x00); // DMAC_CH2_CR0
	write_reg8(0x8010046d, 0x00); // DMAC_CH2_CR1
	write_reg8(0x8010046e, 0x00); // DMAC_CH2_CR2
	write_reg8(0x8010046f, 0x00); // DMAC_CH2_CR3
	write_reg8(0x80100470, 0x0); // DMAC_CH2_SRCAddr0
	write_reg8(0x80100471, 0x0); // DMAC_CH2_SRCAddr1
	write_reg8(0x80100472, 0x0); // DMAC_CH2_SRCAddr2
	write_reg8(0x80100473, 0x0); // DMAC_CH2_SRCAddr3
	write_reg8(0x80100474, 0x0); // DMAC_CH2_DSTAddr0
	write_reg8(0x80100475, 0x0); // DMAC_CH2_DSTAddr1
	write_reg8(0x80100476, 0x0); // DMAC_CH2_DSTAddr2
	write_reg8(0x80100477, 0x0); // DMAC_CH2_DSTAddr3
	write_reg8(0x80100478, 0x0); // DMAC_CH2_TSR0
	write_reg8(0x80100479, 0x0); // DMAC_CH2_TSR1
	write_reg8(0x8010047a, 0x0); // DMAC_CH2_TSR2
	write_reg8(0x8010047c, 0x0); // DMAC_CH2_LLP0
	write_reg8(0x8010047d, 0x0); // DMAC_CH2_LLP1
	write_reg8(0x8010047e, 0x0); // DMAC_CH2_LLP2
	write_reg8(0x8010047f, 0x0); // DMAC_CH2_LLP3
	write_reg8(0x80100480, 0x00); // DMAC_CH3_CR0
	write_reg8(0x80100481, 0x00); // DMAC_CH3_CR1
	write_reg8(0x80100482, 0x00); // DMAC_CH3_CR2
	write_reg8(0x80100483, 0x00); // DMAC_CH3_CR3
	write_reg8(0x80100484, 0x0); // DMAC_CH3_SRCAddr0
	write_reg8(0x80100485, 0x0); // DMAC_CH3_SRCAddr1
	write_reg8(0x80100486, 0x0); // DMAC_CH3_SRCAddr2
	write_reg8(0x80100487, 0x0); // DMAC_CH3_SRCAddr3
	write_reg8(0x80100488, 0x0); // DMAC_CH3_DSTAddr0
	write_reg8(0x80100489, 0x0); // DMAC_CH3_DSTAddr1
	write_reg8(0x8010048a, 0x0); // DMAC_CH3_DSTAddr2
	write_reg8(0x8010048b, 0x0); // DMAC_CH3_DSTAddr3
	write_reg8(0x8010048c, 0x0); // DMAC_CH3_TSR0
	write_reg8(0x8010048d, 0x0); // DMAC_CH3_TSR1
	write_reg8(0x8010048e, 0x0); // DMAC_CH3_TSR2
	write_reg8(0x80100490, 0x0); // DMAC_CH3_LLP0
	write_reg8(0x80100491, 0x0); // DMAC_CH3_LLP1
	write_reg8(0x80100492, 0x0); // DMAC_CH3_LLP2
	write_reg8(0x80100493, 0x0); // DMAC_CH3_LLP3
	write_reg8(0x80100494, 0x00); // DMAC_CH4_CR0
	write_reg8(0x80100495, 0x00); // DMAC_CH4_CR1
	write_reg8(0x80100496, 0x00); // DMAC_CH4_CR2
	write_reg8(0x80100497, 0x00); // DMAC_CH4_CR3
	write_reg8(0x80100498, 0x0); // DMAC_CH4_SRCAddr0
	write_reg8(0x80100499, 0x0); // DMAC_CH4_SRCAddr1
	write_reg8(0x8010049a, 0x0); // DMAC_CH4_SRCAddr2
	write_reg8(0x8010049b, 0x0); // DMAC_CH4_SRCAddr3
	write_reg8(0x8010049c, 0x0); // DMAC_CH4_DSTAddr0
	write_reg8(0x8010049d, 0x0); // DMAC_CH4_DSTAddr1
	write_reg8(0x8010049e, 0x0); // DMAC_CH4_DSTAddr2
	write_reg8(0x8010049f, 0x0); // DMAC_CH4_DSTAddr3
	write_reg8(0x801004a0, 0x0); // DMAC_CH4_TSR0
	write_reg8(0x801004a1, 0x0); // DMAC_CH4_TSR1
	write_reg8(0x801004a2, 0x0); // DMAC_CH4_TSR2
	write_reg8(0x801004a4, 0x0); // DMAC_CH4_LLP0
	write_reg8(0x801004a5, 0x0); // DMAC_CH4_LLP1
	write_reg8(0x801004a6, 0x0); // DMAC_CH4_LLP2
	write_reg8(0x801004a7, 0x0); // DMAC_CH4_LLP3
	write_reg8(0x801004a8, 0x00); // DMAC_CH5_CR0
	write_reg8(0x801004a9, 0x00); // DMAC_CH5_CR1
	write_reg8(0x801004aa, 0x00); // DMAC_CH5_CR2
	write_reg8(0x801004ab, 0x00); // DMAC_CH5_CR3
	write_reg8(0x801004ac, 0x0); // DMAC_CH5_SRCAddr0
	write_reg8(0x801004ad, 0x0); // DMAC_CH5_SRCAddr1
	write_reg8(0x801004ae, 0x0); // DMAC_CH5_SRCAddr2
	write_reg8(0x801004af, 0x0); // DMAC_CH5_SRCAddr3
	write_reg8(0x801004b0, 0x0); // DMAC_CH5_DSTAddr0
	write_reg8(0x801004b1, 0x0); // DMAC_CH5_DSTAddr1
	write_reg8(0x801004b2, 0x0); // DMAC_CH5_DSTAddr2
	write_reg8(0x801004b3, 0x0); // DMAC_CH5_DSTAddr3
	write_reg8(0x801004b4, 0x0); // DMAC_CH5_TSR0
	write_reg8(0x801004b5, 0x0); // DMAC_CH5_TSR1
	write_reg8(0x801004b6, 0x0); // DMAC_CH5_TSR2
	write_reg8(0x801004b8, 0x0); // DMAC_CH5_LLP0
	write_reg8(0x801004b9, 0x0); // DMAC_CH5_LLP1
	write_reg8(0x801004ba, 0x0); // DMAC_CH5_LLP2
	write_reg8(0x801004bb, 0x0); // DMAC_CH5_LLP3
	write_reg8(0x801004bc, 0x00); // DMAC_CH6_CR0
	write_reg8(0x801004bd, 0x00); // DMAC_CH6_CR1
	write_reg8(0x801004be, 0x00); // DMAC_CH6_CR2
	write_reg8(0x801004bf, 0x00); // DMAC_CH6_CR3
	write_reg8(0x801004c0, 0x0); // DMAC_CH6_SRCAddr0
	write_reg8(0x801004c1, 0x0); // DMAC_CH6_SRCAddr1
	write_reg8(0x801004c2, 0x0); // DMAC_CH6_SRCAddr2
	write_reg8(0x801004c3, 0x0); // DMAC_CH6_SRCAddr3
	write_reg8(0x801004c4, 0x0); // DMAC_CH6_DSTAddr0
	write_reg8(0x801004c5, 0x0); // DMAC_CH6_DSTAddr1
	write_reg8(0x801004c6, 0x0); // DMAC_CH6_DSTAddr2
	write_reg8(0x801004c7, 0x0); // DMAC_CH6_DSTAddr3
	write_reg8(0x801004c8, 0x0); // DMAC_CH6_TSR0
	write_reg8(0x801004c9, 0x0); // DMAC_CH6_TSR1
	write_reg8(0x801004ca, 0x0); // DMAC_CH6_TSR2
	write_reg8(0x801004cc, 0x0); // DMAC_CH6_LLP0
	write_reg8(0x801004cd, 0x0); // DMAC_CH6_LLP1
	write_reg8(0x801004ce, 0x0); // DMAC_CH6_LLP2
	write_reg8(0x801004cf, 0x0); // DMAC_CH6_LLP3
	write_reg8(0x801004d0, 0x00); // DMAC_CH7_CR0
	write_reg8(0x801004d1, 0x00); // DMAC_CH7_CR1
	write_reg8(0x801004d2, 0x00); // DMAC_CH7_CR2
	write_reg8(0x801004d3, 0x00); // DMAC_CH7_CR3
	write_reg8(0x801004d4, 0x0); // DMAC_CH7_SRCAddr0
	write_reg8(0x801004d5, 0x0); // DMAC_CH7_SRCAddr1
	write_reg8(0x801004d6, 0x0); // DMAC_CH7_SRCAddr2
	write_reg8(0x801004d7, 0x0); // DMAC_CH7_SRCAddr3
	write_reg8(0x801004d8, 0x0); // DMAC_CH7_DSTAddr0
	write_reg8(0x801004d9, 0x0); // DMAC_CH7_DSTAddr1
	write_reg8(0x801004da, 0x0); // DMAC_CH7_DSTAddr2
	write_reg8(0x801004db, 0x0); // DMAC_CH7_DSTAddr3
	write_reg8(0x801004dc, 0x0); // DMAC_CH7_TSR0
	write_reg8(0x801004dd, 0x0); // DMAC_CH7_TSR1
	write_reg8(0x801004de, 0x0); // DMAC_CH7_TSR2
	write_reg8(0x801004e0, 0x0); // DMAC_CH7_LLP0
	write_reg8(0x801004e1, 0x0); // DMAC_CH7_LLP1
	write_reg8(0x801004e2, 0x0); // DMAC_CH7_LLP2
	write_reg8(0x801004e3, 0x0); // DMAC_CH7_LLP3
	write_reg8(0x801004f2, 0x0); // BB_TX_SIZE
	write_reg8(0x801004f3, 0x0); // BB_TX_CHN_DEP
	write_reg8(0x801004f4, 0x0); // BB_RX_WPTR
	write_reg8(0x801004f5, 0x0); // BB_RX_RPTR
	write_reg8(0x801004f6, 0x0); // BB_RX_SIZE
	write_reg8(0x80100500, 0x0); // BB_TX_WPTR0
	write_reg8(0x80100501, 0x0); // BB_TX_RPTR0
	write_reg8(0x80100502, 0x0); // BB_TX_WPTR1
	write_reg8(0x80100503, 0x0); // BB_TX_RPTR1
	write_reg8(0x80100504, 0x0); // BB_TX_WPTR2
	write_reg8(0x80100505, 0x0); // BB_TX_RPTR2
	write_reg8(0x80100506, 0x0); // BB_TX_WPTR3
	write_reg8(0x80100507, 0x0); // BB_TX_RPTR3
	write_reg8(0x80100508, 0x0); // BB_TX_WPTR4
	write_reg8(0x80100509, 0x0); // BB_TX_RPTR4
	write_reg8(0x8010050a, 0x0); // BB_TX_WPTR5
	write_reg8(0x8010050b, 0x0); // BB_TX_RPTR5
	write_reg8(0x8010050c, 0x04); // BB_AUTO_CTRL
	write_reg8(0x8010050d, 0x00); // RX_WPTR_MASK
}

void rf_radio_reset(void)
{
	/* Baseband */
	write_reg8(0x140800, 0x1f); // TXMODE1
	write_reg8(0x140801, 0x00); // TXMODE2
	write_reg8(0x140802, 0x42); // PREAMBLE_TRAIL
	write_reg8(0x140803, 0x44); // BBDBG
	write_reg8(0x140804, 0xf5); // FORMAT
	write_reg8(0x140805, 0x04); // ACCLEN
	write_reg8(0x140806, 0x0a); // SBLEN
	write_reg8(0x140807, 0x00); // RXCHN
	write_reg8(0x140808, 0xc9); // ACCESS_0
	write_reg8(0x140809, 0x8a); // ACCESS_1
	write_reg8(0x14080a, 0x11); // ACCESS_2
	write_reg8(0x14080b, 0xf8); // ACCESS_3
	write_reg8(0x14080c, 0x95); // ACCESS_4
	write_reg8(0x14080d, 0x00); // PN
	write_reg8(0x140810, 0x77); // ACCESS_5
	write_reg8(0x140811, 0x35); // ACCESS_6
	write_reg8(0x140812, 0xf0); // ACCESS_7
	write_reg8(0x140813, 0xd3); // ACCESS_8
	write_reg8(0x140814, 0xe7); // ACCESS_9
	write_reg8(0x140818, 0x03); // ACCESS_10
	write_reg8(0x140819, 0x0c); // ACCESS_11
	write_reg8(0x14081a, 0x30); // ACCESS_12
	write_reg8(0x14081b, 0xc0); // ACCESS_13
	/* write_reg8(0x14081c, 0x00); // TXFIFO (Read only) */
	write_reg8(0x140821, 0xa1); // RXGTH1
	write_reg8(0x140822, 0x00); // RXSFD0_NUM
	write_reg8(0x140824, 0x55); // CRC_INIT0
	write_reg8(0x140825, 0x55); // CRC_INIT1
	write_reg8(0x140826, 0x55); // CRC_INIT2
	write_reg8(0x140827, 0x00); // CTRL_0
	write_reg8(0x140828, 0x00); // RXMODE
	write_reg8(0x14082a, 0x03); // RXCLK_ON
	write_reg8(0x14082b, 0x03); // RXCLK_AUTO
	write_reg8(0x14082c, 0x01); // RXDMA_HS
	write_reg8(0x140830, 0x3c); // RXTCRPKT
	write_reg8(0x140831, 0xff); // RXTMAXLEN
	write_reg8(0x140833, 0x00); // RXLATF
	write_reg8(0x140834, 0x01); // BANK_CNT
	write_reg8(0x140838, 0x70); // RXSUPP
	write_reg8(0x140839, 0x74); // TX_ANTOFFSET
	write_reg8(0x14083a, 0x4e); // RX_ANTOFFSET
	write_reg8(0x14083b, 0x2d); // SAMP_OFFSET
	write_reg8(0x14083c, 0x00); // MAN_ANT_SLOT
	write_reg8(0x14083d, 0x0a); // SOF_OFFSET
	write_reg8(0x14083e, 0x70); // MODE_CTRL0
	write_reg8(0x14083f, 0x14); // IQ_SAMP_START
	/* write_reg8(0x140840, 0x00); // DEC_ERR (Read only) */
	/* write_reg8(0x140850, 0x00); // TSTAMP0 (Read only) */
	/* write_reg8(0x140851, 0x00); // TSTAMP1 (Read only) */
	/* write_reg8(0x140852, 0x00); // TSTAMP2 (Read only) */
	/* write_reg8(0x140853, 0x00); // TSTAMP3 (Read only) */
	write_reg8(0x140868, 0x10); // ANT_LUT_0 
	write_reg8(0x140869, 0x32); // ANT_LUT_1 
	write_reg8(0x14086a, 0x54); // ANT_LUT_2 
	write_reg8(0x14086b, 0x76); // ANT_LUT_3 
	write_reg8(0x14086c, 0x10); // ANT_LUT_4 
	write_reg8(0x14086d, 0x32); // ANT_LUT_5 
	write_reg8(0x14086e, 0x54); // ANT_LUT_6
	write_reg8(0x14086f, 0x76); // ANT_LUT_7 
	write_reg8(0x140880, 0x00); // RXDMA_FIFO0
	write_reg8(0x140881, 0x00); // RXDMA_FIFO1
	write_reg8(0x140882, 0x00); // RXDMA_FIFO2
	write_reg8(0x140883, 0x00); // RXDMA_FIFO3
	write_reg8(0x140884, 0x00); // TXDMA_FIFO0
	write_reg8(0x140885, 0x00); // TXDMA_FIFO1
	write_reg8(0x140886, 0x00); // TXDMA_FIFO2
	write_reg8(0x140887, 0x00); // TXDMA_FIFO3

	/* BB_LL */
	write_reg8(0x80140a00, 0x00); // LL_CMD
	write_reg8(0x80140a01, 0x00); // LL_RESET_PID
	write_reg8(0x80140a02, 0x45); // LL_CTRL0
	write_reg8(0x80140a03, 0x1e); // LL_CTRL1
	write_reg8(0x80140a04, 0x95); // LL_TXSTL_L
	write_reg8(0x80140a05, 0x00); // LL_TXSTL_H
	write_reg8(0x80140a06, 0x09); // LL_RXWAIT_L
	write_reg8(0x80140a07, 0x00); // LL_RXWAIT_H
	write_reg8(0x80140a0a, 0xf9); // LL_RX_L
	write_reg8(0x80140a0b, 0x00); // LL_RX_H
	write_reg8(0x80140a0c, 0x95); // LL_RXSTL_L
	write_reg8(0x80140a0d, 0x00); // LL_RXSTL_H
	write_reg8(0x80140a0e, 0x09); // LL_TXWAIT_L
	write_reg8(0x80140a0f, 0x00); // LL_TXWAIT_H
	write_reg8(0x80140a10, 0x63); // LL_ARD_L
	write_reg8(0x80140a11, 0x00); // LL_ARD_H
	write_reg8(0x80140a12, 0x31); // T_COEX_T1
	write_reg8(0x80140a13, 0x13); // T_COEX_T2
	write_reg8(0x80140a14, 0x03); // LL_MAX_RETRY_CNT
	write_reg8(0x80140a15, 0xf0); // LL_CTRL2
	write_reg8(0x80140a16, 0x19); // LL_CTRL3
	write_reg8(0x80140a17, 0x20); // LL_PLL_RESET
	write_reg8(0x80140a18, 0x00); // LL_CMD_SCHEDULE0
	write_reg8(0x80140a19, 0x00); // LL_CMD_SCHEDULE1
	write_reg8(0x80140a1a, 0x00); // LL_CMD_SCHEDULE2
	write_reg8(0x80140a1b, 0x00); // LL_CMD_SCHEDULE3
	write_reg8(0x80140a1c, 0xff); // LL_IRQ_MASK_L
	write_reg8(0x80140a1d, 0xff); // LL_IRQ_MASK_H
	write_reg8(0x80140a1e, 0x00); // LL_TX_ID
	/* write_reg8(0x80140a1f, 0x3f); // LL_TX_COMMITTED (Read only) */
	write_reg8(0x80140a20, 0x00); // LL_IRQ_LIST_L
	write_reg8(0x80140a21, 0x00); // LL_IRQ_LIST_H
	/* write_reg8(0x80140a22, 0x00); // LL_PID_L  (Read only) */
	/* write_reg8(0x80140a23, 0x00); // LL_PID_H (Read only) */
	/* write_reg8(0x80140a24, 0x00); // LL_2D_SCLK  (Read only) */
	/* write_reg8(0x80140a25, 0x00); // LL_RETRY_CNT  (Read only) */
	/* write_reg8(0x80140a26, 0x00); // LL_CNT0  (Read only) */
	/* write_reg8(0x80140a27, 0x00); // LL_CNT1  (Read only) */
	write_reg8(0x80140a28, 0xf9); // LL_RX_FST_L
	write_reg8(0x80140a29, 0x00); // LL_RX_FST_M
	write_reg8(0x80140a2a, 0x00); // LL_RX_FST_H
	write_reg8(0x80140a2c, 0x0f); // LL_FSM_TIMROUT_L
	write_reg8(0x80140a2d, 0x27); // LL_FSM_TIMROUT_M
	write_reg8(0x80140a2e, 0x00); // LL_FSM_TIMROUT_H
	write_reg8(0x80140a30, 0x00); // COEX_EANBLE

	/* BB PDZB */
	write_reg8(0x80140b00, 0x00); // SEL_NTL_CV
	write_reg8(0x80140b02, 0x00); // GCLK_ENABLE
	write_reg8(0x80140b03, 0x00); // RSSI_READY_TIE1
	write_reg8(0x80140b04, 0x00); // EMBASE_ADDR0
	write_reg8(0x80140b05, 0x00); // EMBASE_ADDR1
	write_reg8(0x80140b06, 0x20); // EMBASE_ADDR2
	write_reg8(0x80140b07, 0xc0); // EMBASE_ADDR3
	write_reg8(0x80140b08, 0x00); // SEL_DEBUG_EN

	/* TL_MODEM */
	write_reg8(0x80140c20, 0xc4); // MODEM_MODE_CFG_RX1_0
	write_reg8(0x80140c21, 0x00); //  MODEM_MODE_CFG_RX1_1
	write_reg8(0x80140c22, 0x00); //  MODEM_MODE_CTRL_TX1_0
	write_reg8(0x80140c23, 0x00); //  MODEM_MODE_CTRL_TX1_1
	write_reg8(0x80140c26, 0x00); //  MODEM_MODE_CFG_TXRX_0
	write_reg8(0x80140c27, 0x22); //  MODEM_MODE_CFG_TXRX_1
	write_reg8(0x80140c2a, 0x10); //  MODEM_DEBUG_TXRX1_0
	write_reg8(0x80140c2b, 0x44); //  MODEM_DEBUG_TXRX1_1
	write_reg8(0x80140c2c, 0x38); //  ZB_PDET_CFG_0
	write_reg8(0x80140c2d, 0x04); //  ZB_PDET_CFG_1
	/* write_reg8(0x80140c2e, 0x00); //  MODEM_RSSI_0 (Read only) */
	/* write_reg8(0x80140c2f, 0x08); //  MODEM_RSSI_1 (Read only) */
	write_reg8(0x80140c30, 0x02); //  RSSI_START_0
	write_reg8(0x80140c31, 0x00); //  RSSI_START_1
	write_reg8(0x80140c32, 0xff); //  RSSI_STOP_0
	write_reg8(0x80140c33, 0x0f); //  RSSI_STOP_1
	write_reg8(0x80140c34, 0x03); //  RSSI_FILT
	write_reg8(0x80140c36, 0xb7); //  LR_FREQ_CORR_CFG_0
	write_reg8(0x80140c37, 0x0e); //  LR_FREQ_CORR_CFG_1
	write_reg8(0x80140c38, 0xc4); //  LR_TIM_REC_CFG_0
	write_reg8(0x80140c39, 0x71); //  LR_TIM_REC_CFG_1
	write_reg8(0x80140c3a, 0x95); //  ZB_FREQ_CORR_CFG_0
	write_reg8(0x80140c3b, 0x1c); //  ZB_FREQ_CORR_CFG_1
	write_reg8(0x80140c3c, 0xc8); //  ZB_THR_CFG_0
	write_reg8(0x80140c3d, 0x00); //  ZB_THR_CFG_1
	write_reg8(0x80140c3e, 0x81); //  MODE_CFG_RX2_0
	write_reg8(0x80140c3f, 0x00); //  MODE_CFG_RX2_1
	write_reg8(0x80140c40, 0x00); //  TXRX_DBG1_0
	write_reg8(0x80140c41, 0x00); //  TXRX_DBG1_1
	write_reg8(0x80140c46, 0x06); //  MODEM_TXRX_DBG4_0
	write_reg8(0x80140c48, 0x40); //  MODEM_RXC_MI_FLEX_BLE_0
	write_reg8(0x80140c49, 0x00); //  MODEM_RXC_MI_FLEX_BLE_1
	write_reg8(0x80140c4a, 0x29); //  MODEM_RXC_MI_FLEX_BT_0
	write_reg8(0x80140c4b, 0x00); //  MODEM_RXC_MI_FLEX_BT_1
	write_reg8(0x80140c4c, 0x4c); //  MODEM_RX_CTRL_0
	write_reg8(0x80140c4d, 0x01); //  MODEM_RX_CTRL_1
	write_reg8(0x80140c4e, 0x1e); //  MODEM_SYNC_THRE_BLE
	write_reg8(0x80140c4f, 0x1c); //  MODEM_SYNC_THRE_BT
	write_reg8(0x80140c50, 0xff); //  MODEM_RX_AUTO0
	write_reg8(0x80140c51, 0xff); //  MODEM_RX_AUTO1
	write_reg8(0x80140c52, 0x00); //  MODEM_RX_ENABLE0
	write_reg8(0x80140c53, 0x00); //  MODEM_RX_ENABLE1
	/* write_reg8(0x80140c54, 0x00); //  MODEM_MATCH0  (Read only) */
	/* write_reg8(0x80140c55, 0x00); //  MODEM_MATCH1  (Read only) */
	/* write_reg8(0x80140c56, 0x00); //  MODEM_FDC_DBG0  (Read only) */
	/* write_reg8(0x80140c57, 0x00); //  MODEM_FDC_DBG1  (Read only) */
	/* write_reg8(0x80140c58, 0x00); //  MODEM_FDC_DBG_LAT0  (Read only) */
	/* write_reg8(0x80140c59, 0x00); //  MODEM_FDC_DBG_LAT1 (Read only) */
	/* write_reg8(0x80140c5a, 0x00); //  MODEM_PKT_POE_LAT0 (Read only) */  
	/* write_reg8(0x80140c5b, 0x00); //  MODEM_PKT_POE_LAT1 (Read only) */
	/* write_reg8(0x80140c5c, 0x00); //  MODEM_GAIN_LAT  (Read only) */
	/* write_reg8(0x80140c5d, 0x00); //  AGC_RSSI_LAT  (Read only) */
	/* write_reg8(0x80140c5e, 0x00); //  RSSI_RO  (Read only) */
	/* write_reg8(0x80140c5f, 0x00); //  RSSI_MAX  (Read only) */
	write_reg8(0x80140c72, 0x9b); //  TIM_REC_CFG_0
	write_reg8(0x80140c73, 0x01); //  TIM_REC_CFG_1
	write_reg8(0x80140c74, 0xca); //  FREQ_CORR_CFG1_0
	write_reg8(0x80140c75, 0x02); //  FREQ_CORR_CFG1_1
	write_reg8(0x80140c76, 0x57); //  FREQ_CORR_CFG2_0
	write_reg8(0x80140c77, 0x73); //  FREQ_CORR_CFG2_1
	write_reg8(0x80140c78, 0x13); //  TIM_ALIGN_0
	write_reg8(0x80140c79, 0x00); //  TIM_ALIGN_1
	write_reg8(0x80140c7a, 0x20); //  BYPASS_FILT_0
	write_reg8(0x80140c7b, 0xf6); //  BYPASS_FILT_1
	write_reg8(0x80140c7e, 0x44); //  MODEM_FREQ_CORR0
	write_reg8(0x80140c7f, 0x02); //  MODEM_FREQ_CORR1
	write_reg8(0x80140c80, 0x8a); //  MODEM_TX_CTRL_0
	write_reg8(0x80140c81, 0x00); //  MODEM_IQ_CTRL_1
	write_reg8(0x80140c82, 0x99); //  MODEM_TX_STB_DLY_0
	write_reg8(0x80140c84, 0x00); //  GUARD_ENABLE_0
	write_reg8(0x80140c86, 0x00); //  MODEM_BT_MASK
	write_reg8(0x80140c88, 0xd5); //  MODEM_GFSK_MPSK_SWING_0
	write_reg8(0x80140c89, 0xde); //  MODEM_GFSK_MPSK_SWING_1
	write_reg8(0x80140c8a, 0x00); //  MODEM_MODE_CFG_TX_0
	write_reg8(0x80140c8c, 0xff); //  MODEM_TX_IGAIN_0
	write_reg8(0x80140c8d, 0x03); //  MODEM_TX_IGAIN_1
	write_reg8(0x80140c8e, 0xff); //  MODEM_TX_QGAIN_0
	write_reg8(0x80140c8f, 0x03); //  MODEM_TX_QGAIN_1
	write_reg8(0x80140c90, 0x00); //  MODEM_DC_COM
	write_reg8(0x80140c91, 0x1e); //  MODEM_TX_SCALE
	write_reg8(0x80140c92, 0xd5); //  MODEM_PA_UP_DIQ_0
	write_reg8(0x80140c93, 0x80); //  MODEM_PA_UP_DIQ_1
	write_reg8(0x80140c94, 0xd5); //  MODEM_PA_DOWN_DIQ_0
	write_reg8(0x80140c95, 0x80); //  MODEM_PA_DOWN_DIQ_1
	write_reg8(0x80140c96, 0xb5); //  TX_STB_DLY_0
	write_reg8(0x80140c97, 0x02); //  TX_STB_DLY_1
	write_reg8(0x80140c98, 0x07); //  CLOCK_GATE
	write_reg8(0x80140c9a, 0x08); //  TX_TL_CTRL
	write_reg8(0x80140c9c, 0x0a); //  TX_MIDX_BT_0
	write_reg8(0x80140c9d, 0x07); //  TX_MIDX_BT_1
	write_reg8(0x80140c9e, 0xab); //  TX_MIDX_BLE_0
	write_reg8(0x80140c9f, 0x0a); //  TX_MIDX_BLE_1
	write_reg8(0x80140ca0, 0x1c); //  MODEM_PDET_THRE
	write_reg8(0x80140ca2, 0x3c); //  MODEM_DCOC_DIST
	write_reg8(0x80140ca3, 0x03); //  MODEM_DCOC_HD
	write_reg8(0x80140ca4, 0x6b); //  DC_ENABLE
	write_reg8(0x80140ca5, 0x13); //  EDR_SFT
	write_reg8(0x80140ca6, 0x50); //  MODEM_EDR_TIME_ERR_BETA_0
	write_reg8(0x80140ca7, 0x20); //  MODEM_EDR_TIME_ERR_BETA_1
	write_reg8(0x80140ca8, 0x80); //  MODEM_EDR_PHASE_ERR_BETA_0
	write_reg8(0x80140ca9, 0x00); //  MODEM_EDR_PHASE_ERR_BETA_1
	write_reg8(0x80140caa, 0x16); //  DIG_AGX_TB0
	write_reg8(0x80140cab, 0x2d); //  DIG_AGX_TB1
	write_reg8(0x80140cac, 0x5a); //  DIG_AGX_TB2
	write_reg8(0x80140cae, 0xf4); //  DIG_AGX_TB3_0
	write_reg8(0x80140caf, 0x01); //  DIG_AGX_TB3_1
	write_reg8(0x80140cb0, 0xe8); //  DIG_AGX_TB4_0
	write_reg8(0x80140cb1, 0x03); //  DIG_AGX_TB4_1
	write_reg8(0x80140cb2, 0xe8); //  SRC_LEN
	write_reg8(0x80140cb3, 0x13); //  IDX_RAND
	write_reg8(0x80140cb4, 0x00); //  MODEM_FDC_EDR2
	write_reg8(0x80140cb5, 0x00); //  MODEM_FDC_EDR3
	write_reg8(0x80140cb6, 0x39); //  RX_RATE0
	write_reg8(0x80140cb7, 0x39); //  RX_RATE1
	write_reg8(0x80140cb8, 0x33); //  RATE_DLY
	write_reg8(0x80140cb9, 0x55); //  SYNC_WIN
	write_reg8(0x80140cba, 0x0e); //  SYNC_THD_BT_ID_FAST_STL
	write_reg8(0x80140cc2, 0x36); //  GRX_0
	write_reg8(0x80140cc3, 0x48); //  GRX_1
	write_reg8(0x80140cc4, 0x54); //  GRX_2
	write_reg8(0x80140cc5, 0x62); //  GRX_3
	write_reg8(0x80140cc6, 0x6e); //  GRX_4
	write_reg8(0x80140cc7, 0x79); //  GRX_5
	write_reg8(0x80140cc8, 0x00); //  GRX_FIX
	write_reg8(0x80140d20, 0x00); //  ADC_DAC_DFT1_0
	write_reg8(0x80140d21, 0x00); //  ADC_DAC_DFT1_1
	write_reg8(0x80140d38, 0x00); //  MODEM_BYPASS_CLK_GATING

	/* TL RADIO */

	/* write_reg8(0x80140e02, 0x00); // MODE_CFG_RX1_0 (Volatile) */
	write_reg8(0x80140e20, 0x16); // MODE_CFG_RX1_0
	write_reg8(0x80140e21, 0x0a); // MODE_CFG_RX1_1
	write_reg8(0x80140e22, 0x20); // MODE_CFG_TX1_0
	write_reg8(0x80140e23, 0x23); // MODE_CFG_TX1_1
	write_reg8(0x80140e24, 0x49); // MODE_CFG_TX2_0
	write_reg8(0x80140e25, 0x40); // MODE_CFG_TX2_1
	write_reg8(0x80140e26, 0x4c); // MODE_CFG_TXRX_0
	write_reg8(0x80140e27, 0x14); // MODE_CFG_TXRX_1
	write_reg8(0x80140e28, 0x26); // BURST_CFG_TXRX_0
	write_reg8(0x80140e29, 0x00); // BURST_CFG_TXRX_1
	write_reg8(0x80140e2a, 0x80); // DEBUG_TXRX1_0
	write_reg8(0x80140e2b, 0x02); // DEBUG_TXRX1_1
	/* write_reg8(0x80140e2e, 0x07); // RSSI_0 (Read only) */
	write_reg8(0x80140e37, 0x80); // LR_FREQ_CORR_CFG_1
	write_reg8(0x80140e3c, 0x7f); // MODE_CFG_TX3_0
	write_reg8(0x80140e3d, 0x61); // MODE_CFG_TX3_1
	write_reg8(0x80140e3e, 0x40); // MODE_CFG_RX2_0
	write_reg8(0x80140e3f, 0x00); // MODE_CFG_RX2_1
	write_reg8(0x80140e40, 0x14); // TXRX_DBG1_0
	write_reg8(0x80140e41, 0x00); // TXRX_DBG1_1
	write_reg8(0x80140e42, 0x40); // TXRX_DBG2_0
	write_reg8(0x80140e43, 0x06); // TXRX_DBG2_1
	write_reg8(0x80140e44, 0x21); // TXRX_DBG3_0
	write_reg8(0x80140e45, 0x26); // TXRX_DBG3_1
	write_reg8(0x80140e46, 0x03); // TXRX_DBG4_0
	write_reg8(0x80140e47, 0xa9); // TXRX_DBG4_1
	write_reg8(0x80140e48, 0x00); // DSM_FRAC1_0
	write_reg8(0x80140e49, 0x00); // DSM_FRAC1_1
	write_reg8(0x80140e4a, 0x89); // AGC_THRSHLD1_1M_0
	write_reg8(0x80140e4b, 0x06); // AGC_THRSHLD1_1M_1
	write_reg8(0x80140e4c, 0x8c); // AGC_THRSHLD2_1M_0
	write_reg8(0x80140e4d, 0x07); // AGC_THRSHLD2_1M_1
	write_reg8(0x80140e4e, 0x09); // AGC_THRSHLD3_1M_0
	write_reg8(0x80140e4f, 0x0f); // AGC_THRSHLD3_1M_1
	write_reg8(0x80140e50, 0x3c); // AGC_THRSHLD4_1M_0
	write_reg8(0x80140e51, 0x1e); // AGC_THRSHLD4_1M_1
	write_reg8(0x80140e52, 0x3c); // AGC_THRSHLD5_1M_0
	write_reg8(0x80140e53, 0x1e); // AGC_THRSHLD5_1M_1
	write_reg8(0x80140e54, 0x89); // AGC_THRSHLD1_2M_0
	write_reg8(0x80140e55, 0x06); // AGC_THRSHLD1_2M_1
	write_reg8(0x80140e56, 0x8c); // AGC_THRSHLD2_2M_0
	write_reg8(0x80140e57, 0x07); // AGC_THRSHLD2_2M_1
	write_reg8(0x80140e58, 0x09); // AGC_THRSHLD3_2M_0
	write_reg8(0x80140e59, 0x0f); // AGC_THRSHLD3_2M_1
	write_reg8(0x80140e5a, 0x3c); // AGC_THRSHLD4_2M_0
	write_reg8(0x80140e5b, 0x1e); // AGC_THRSHLD4_2M_1
	write_reg8(0x80140e5c, 0x3c); // AGC_THRSHLD5_2M_0
	write_reg8(0x80140e5d, 0x1e); // AGC_THRSHLD5_2M_1
	write_reg8(0x80140e5e, 0x67); // RX_GRP_DLY_1M_0
	write_reg8(0x80140e5f, 0x9d); // RX_GRP_DLY_1M_1
	write_reg8(0x80140e60, 0x3f); // RX_GAIN_VHG_0
	write_reg8(0x80140e61, 0x04); // RX_GAIN_VHG_1
	write_reg8(0x80140e62, 0x0f); // RX_GAIN_HG_0
	write_reg8(0x80140e63, 0x04); // RX_GAIN_HG_1
	write_reg8(0x80140e64, 0x03); // RX_GAIN_IHG_0
	write_reg8(0x80140e65, 0x04); // RX_GAIN_IHG_1
	write_reg8(0x80140e66, 0xc0); // RX_GAIN_MG_0
	write_reg8(0x80140e67, 0x04); // RX_GAIN_MG_1
	write_reg8(0x80140e68, 0xc0); // RX_GAIN_LG_0
	write_reg8(0x80140e69, 0x06); // RX_GAIN_LG_1
	write_reg8(0x80140e6a, 0x40); // RX_GAIN_VLG_0
	write_reg8(0x80140e6b, 0x03); // RX_GAIN_VLG_1
	write_reg8(0x80140e6c, 0x00); // ADC_RBK_I_0
	write_reg8(0x80140e6d, 0x00); // ADC_RBK_I_1
	write_reg8(0x80140e6e, 0x00); // ADC_RBK_Q_0
	write_reg8(0x80140e6f, 0x00); // ADC_RBK_Q_1
	write_reg8(0x80140e70, 0x34); // RX_GRP_DLY_2M_0
	write_reg8(0x80140e71, 0x52); // RX_GRP_DLY_2M_1
	write_reg8(0x80140e72, 0x53); // CTRIM_1_0
	write_reg8(0x80140e73, 0x44); // CTRIM_1_1
	write_reg8(0x80140e74, 0x35); // CTRIM_2_0
	write_reg8(0x80140e75, 0x20); // CTRIM_2_1
	write_reg8(0x80140e76, 0x08); // CTRIM_3_0
	write_reg8(0x80140e77, 0x00); // CTRIM_3_1
	write_reg8(0x80140e78, 0x00); // CTRIM_4_0
	write_reg8(0x80140e79, 0x80); // CT_DINT_CARE
	write_reg8(0x80140e7a, 0x20); // BYPASS_FILT_0
	/* write_reg8(0x80140e7c, 0x00); // AGC_RSSI_RDBK_0 (Read only) */
	/* write_reg8(0x80140e7d, 0x00); // AGC_RSSI_RDBK_1 (Read only) */
	write_reg8(0x80140e7e, 0x02); // TIM_RESTART_DLY_0
	write_reg8(0x80140e7f, 0x01); // TIM_RESTART_DLY_1
	write_reg8(0x80140e80, 0x40); // CAL_OW_CTRL_0
	write_reg8(0x80140e81, 0x00); // CAL_OW_CTRL_1
	write_reg8(0x80140e82, 0x00); // CAL_OW_VAL_0
	write_reg8(0x80140e83, 0x00); // CAL_OW_VAL_1
	write_reg8(0x80140e84, 0x62); // TXRX_CB_CAL_CTRL
	write_reg8(0x80140e85, 0xf2); // TXRX_CB1_CAL_CTRL
	write_reg8(0x80140e86, 0x00); // TXRX_EN_DBG_OW_CTRL
	write_reg8(0x80140e88, 0x00); // TXRX_EN_DBG_VAL
	write_reg8(0x80140e8a, 0x00); // IDLE_TXFSK_SS1_SS2_STRT_0
	write_reg8(0x80140e8b, 0x0d); // IDLE_TXFSK_SS1_SS2_STRT_1
	write_reg8(0x80140e8c, 0x6a); // IDLE_TXFSK_SS3_SS4_STRT_0
	write_reg8(0x80140e8d, 0x6b); // IDLE_TXFSK_SS3_SS4_STRT_1
	write_reg8(0x80140e8e, 0x6e); // IDLE_TXFSK_SS6_SS7_STRT_0
	write_reg8(0x80140e8f, 0x6a); // IDLE_TXFSK_SS6_SS7_STRT_1
	write_reg8(0x80140e90, 0x00); // IDLE_RX_SS1_SS2_STRT_0
	write_reg8(0x80140e91, 0x0d); // IDLE_RX_SS1_SS2_STRT_1
	write_reg8(0x80140e92, 0x0d); // IDLE_RX_SS3_SS4_STRT_0
	write_reg8(0x80140e93, 0x27); // IDLE_RX_SS3_SS4_STRT_1
	write_reg8(0x80140e94, 0x52); // IDLE_RX_SS5_SS6_STRT_0
	write_reg8(0x80140e95, 0x52); // IDLE_RX_SS5_SS6_STRT_1
	write_reg8(0x80140e96, 0x00); // IDLE_TXFSK_SS1_SS2_STRT_CB_0
	write_reg8(0x80140e97, 0x09); // IDLE_TXFSK_SS1_SS2_STRT_CB_1
	write_reg8(0x80140e98, 0x32); // IDLE_TXFSK_SS3_SS4_STRT_CB0
	write_reg8(0x80140e99, 0x33); // IDLE_TXFSK_SS3_SS4_STRT_CB1
	write_reg8(0x80140e9a, 0x36); // IDLE_TXFSK_SS6_SS7_STRT_CB_0
	write_reg8(0x80140e9b, 0x32); // IDLE_TXFSK_SS6_SS7_STRT_CB_1
	write_reg8(0x80140e9c, 0x00); // IDLE_RX_SS1_SS2_STRT_CB_0
	write_reg8(0x80140e9d, 0x09); // IDLE_RX_SS1_SS2_STRT_CB_1
	write_reg8(0x80140e9e, 0x09); // IDLE_RX_SS3_SS4_STRT_CB_0
	write_reg8(0x80140e9f, 0x23); // IDLE_RX_SS3_SS4_STRT_CB_1
	write_reg8(0x80140ea0, 0x32); // IDLE_RX_SS5_SS6_STRT_CB_0
	write_reg8(0x80140ea1, 0x32); // IDLE_RX_SS5_SS6_STRT_CB_1
	write_reg8(0x80140ea2, 0x00); // IDLE_TXIQ_SS1_SS2_STRT_0
	write_reg8(0x80140ea3, 0x0d); // IDLE_TXIQ_SS1_SS2_STRT_1
	write_reg8(0x80140ea4, 0x35); // IDLE_TXIQ_SS3_SS4_STRT_0
	write_reg8(0x80140ea5, 0x47); // IDLE_TXIQ_SS3_SS4_STRT_1
	write_reg8(0x80140ea6, 0x4a); // IDLE_TXIQ_SS6_SS7_STRT_0
	write_reg8(0x80140ea7, 0x46); // IDLE_TXIQ_SS6_SS7_STRT_1
	write_reg8(0x80140ea8, 0x00); // IDLE_TXIQ_SS1_STRT_CB
	write_reg8(0x80140ea9, 0x09); // IDLE_TXIQ_SS2_STRT_CB
	write_reg8(0x80140eaa, 0x31); // IDLE_TXIQ_SS3_STRT_CB
	write_reg8(0x80140eab, 0x35); // IDLE_TXIQ_SS4_STRT_CB
	write_reg8(0x80140eac, 0x38); // IDLE_TXIQ_SS6_STRT_CB
	write_reg8(0x80140ead, 0x34); // IDLE_TXIQ_SS7_STRT_CB
	write_reg8(0x80140eae, 0x00); // LOCD_TRIG
	write_reg8(0x80140eb0, 0x00); // IDLE_TXIQ_SS1_STRT_CB1
	write_reg8(0x80140eb1, 0x09); // IDLE_TXIQ_SS2_STRT_CB1
	write_reg8(0x80140eb2, 0x31); // IDLE_TXIQ_SS3_STRT_CB1
	write_reg8(0x80140eb3, 0x35); // IDLE_TXIQ_SS4_STRT_CB1
	write_reg8(0x80140eb4, 0x38); // IDLE_TXIQ_SS6_STRT_CB1
	write_reg8(0x80140eb5, 0x34); // IDLE_TXIQ_SS7_STRT_CB1
	write_reg8(0x80140eb6, 0x00); // RX_SS1_STRT_CB1
	write_reg8(0x80140eb7, 0x09); // RX_SS2_STRT_CB1
	write_reg8(0x80140eb8, 0x09); // RX_SS3_STRT_CB1
	write_reg8(0x80140eb9, 0x23); // RX_SS4_STRT_CB1
	write_reg8(0x80140eba, 0x32); // RX_SS5_STRT_CB
	write_reg8(0x80140ebb, 0x32); // RX_SS6_STRT_CB1
	write_reg8(0x80140ec0, 0x00); // FCAL_RDBK_0
	write_reg8(0x80140ec1, 0x00); // FCAL_RDBK_1
	write_reg8(0x80140ec2, 0xc0); // FCAL_DBG2_0
	write_reg8(0x80140ec3, 0x28); // FCAL_DBG2_1
	write_reg8(0x80140ec4, 0xff); // TXRX_CAL_CTRL_NORM	
	write_reg8(0x80140ec6, 0x1a); // RCCAL_DBG1_0
	write_reg8(0x80140ec7, 0x2b); // RCCAL_DBG1_1
	write_reg8(0x80140ec8, 0x00); // RCCAL_DBG2
	write_reg8(0x80140eca, 0xff); // RCCAL_RDBK_0
	write_reg8(0x80140ecb, 0x0f); // RCCAL_RDBK_1
	write_reg8(0x80140ece, 0x00); // DCOC_BYPASS_ADC_0
	write_reg8(0x80140ecf, 0x00); // DCOC_BYPASS_ADC_1
	write_reg8(0x80140ed0, 0x40); // DCOC_BYPASS_DAC_0
	write_reg8(0x80140ed1, 0x10); // DCOC_BYPASS_DAC_1
	write_reg8(0x80140ed2, 0xbb); // DCOC_DBG0
	write_reg8(0x80140ed3, 0x15); // DCOC_DBG1
	write_reg8(0x80140ed4, 0x00); // DCOC_LNA_OFFSET_0
	write_reg8(0x80140ed5, 0x00); // DCOC_LNA_OFFSET_1
	write_reg8(0x80140ed6, 0x00); // DCOC_CBPF_OFFSET_0
	write_reg8(0x80140ed7, 0x00); // DCOC_CBPF_OFFSET_1
	write_reg8(0x80140ed8, 0x20); // DCOC_RDBK1_0
	write_reg8(0x80140ed9, 0x00); // DCOC_RDBK1_1
	write_reg8(0x80140eda, 0x20); // DCOC_RDBK2
	write_reg8(0x80140edc, 0x00); // DCOC_RDBK3_0
	write_reg8(0x80140edd, 0x00); // DCOC_RDBK3_1
	write_reg8(0x80140ee2, 0x40); // LDOT_DBG1
	write_reg8(0x80140ee4, 0x80); // LDOT_DBG2_0
	write_reg8(0x80140ee5, 0x20); // LDOT_DBG2_1
	write_reg8(0x80140ee6, 0x80); // LDOT_DBG3_0
	write_reg8(0x80140ee7, 0x20); // LDOT_DBG3_1
	write_reg8(0x80140ee8, 0x01); // LDOT_DBG4_0
	write_reg8(0x80140ee9, 0x00); // LDOT_DBG4_1
	write_reg8(0x80140eea, 0x00); // LDOT_RDBK1
	write_reg8(0x80140eec, 0xc0); // LDOT_RDBK2_0
	write_reg8(0x80140eed, 0x05); // LDOT_RDBK2_1
	write_reg8(0x80140eee, 0x00); // LDOT_RDBK3_0
	write_reg8(0x80140eef, 0x00); // LDOT_RDBK3_1
	write_reg8(0x80140ef4, 0x00); // HPMC_CFG
	write_reg8(0x80140ef6, 0x00); // HPMC_DEBUG_0
	write_reg8(0x80140ef7, 0x08); // HPMC_DEBUG_1
	write_reg8(0x80140ef8, 0x00); // HPMC_RDBK1_0
	write_reg8(0x80140ef9, 0x00); // HPMC_RDBK1_1
	write_reg8(0x80140efa, 0x00); // HPMC_RDBK2_0
	write_reg8(0x80140efb, 0x00); // HPMC_RDBK2_1
	write_reg8(0x80140efc, 0x00); // HPMC_RDBK3_0
	write_reg8(0x80140efd, 0x00); // HPMC_RDBK3_1
	write_reg8(0x80140efe, 0x00); // HPMC_RDBK4_0
	write_reg8(0x80140eff, 0x00); // HPMC_RDBK4_1
	write_reg8(0x80140f04, 0xe2); // LOCD_CFG_0
	write_reg8(0x80140f05, 0x00); // LOCD_CFG_1
	write_reg8(0x80140f06, 0x00); // LOCD_RDBK
	write_reg8(0x80140f08, 0x84); // LOCD_TRGT_CNT11_0
	write_reg8(0x80140f09, 0xe9); // LOCD_TRGT_CNT11_1
	write_reg8(0x80140f0a, 0x00); // LOCD_TRGT_CNT12
	/* write_reg8(0x80140f10, 0x7f); // CAL_DONE_RDBK (Read only) */
	write_reg8(0x80140f12, 0x40); // TX_DCOC_CFG_0
	write_reg8(0x80140f13, 0x10); // TX_DCOC_CFG_1
	/* write_reg8(0x80140f14, 0x20); // TX_DCOC_RDBK_0 (Read only) */
	/* write_reg8(0x80140f15, 0x08); // TX_DCOC_RDBK_1 (Read only) */
	write_reg8(0x80140f20, 0x00); // RADIO_ADC_DAC_DFT1_0
	write_reg8(0x80140f21, 0x00); // RADIO_ADC_DAC_DFT1_1
	write_reg8(0x80140f22, 0x00); // RXTX_DBG_CTRL
	write_reg8(0x80140f26, 0x00); // DFT_MUX_0
	write_reg8(0x80140f27, 0x00); // DFT_MUX_1
	write_reg8(0x80140f2c, 0x01); // DIAG_0
	write_reg8(0x80140f2d, 0x00); // DIAG_1
	write_reg8(0x80140f38, 0x00); // BYPASS_CLK_GATING
	write_reg8(0x80140f3a, 0x00); // SFT_RST
	write_reg8(0x80140f40, 0x00); // LDO1_0
	write_reg8(0x80140f41, 0x00); // LDO1_1
	write_reg8(0x80140f42, 0xc0); // LDO_BG_CLK_AUX_DRV_0
	write_reg8(0x80140f43, 0x00); // LDO_BG_CLK_AUX_DRV_1
	write_reg8(0x80140f4c, 0x11); // LNM_PA_0
	write_reg8(0x80140f4d, 0x08); // LNM_PA_1
	write_reg8(0x80140f4e, 0x40); // CBPF_ADC_0
	write_reg8(0x80140f4f, 0x0b); // CBPF_ADC_1
	write_reg8(0x80140f52, 0x00); // PD_TXDAC_0
	write_reg8(0x80140f53, 0x21); // PD_TXDAC_1
	write_reg8(0x80140f54, 0x07); // VCO_LDOTRIM
	write_reg8(0x80140f56, 0x0e); // TXDAC_TXLPF_0
	write_reg8(0x80140f57, 0x10); // TXDAC_TXLPF_1
	write_reg8(0x80140f5c, 0x01); // REG_SPARELV1
	write_reg8(0x80140f60, 0x00); // LDO_OW_CTRL_0
	write_reg8(0x80140f61, 0x00); // LDO_OW_CTRL_1
	write_reg8(0x80140f62, 0x00); // LDO_OW_VAL_0
	write_reg8(0x80140f63, 0x00); // LDO_OW_VAL_1
	write_reg8(0x80140f64, 0x00); // BG_OW_CTRL
	write_reg8(0x80140f66, 0x00); // BG_OW_VAL
	write_reg8(0x80140f68, 0x00); // LDOTRIM_OW_CTRL
	write_reg8(0x80140f6a, 0x00); // LDOTRIM_OW_VAL
	write_reg8(0x80140f6c, 0x00); // RCCAL_OW_CTRL_VAL
	write_reg8(0x80140f70, 0x00); // XO_OW_CTRL_VAL
	write_reg8(0x80140f78, 0x00); // LNM_PA_OW_CTRL_VAL
	write_reg8(0x80140f7a, 0x7e); // LNM_OW_VAL_0
	write_reg8(0x80140f7b, 0x00); // LNM_OW_VAL_1
	write_reg8(0x80140f7c, 0x00); // PA_OW_VAL
	write_reg8(0x80140f80, 0x00); // CBPF_ADC_OW_CTRL
	write_reg8(0x80140f82, 0x00); // CBPF_ADC_OW_VAL
	write_reg8(0x80140f88, 0x00); // PD_DIVN_FCAL_OW_CTRL
	write_reg8(0x80140f8a, 0x00); // PD_DIVN_FCAL_OW_VAL
	write_reg8(0x80140f8c, 0x00); // VCO_LOPATH_DAC_OW_CTRL
	write_reg8(0x80140f8e, 0x00); // VCO_LOPATH_DAC_OW_VAL
	write_reg8(0x80140f90, 0x00); // VCO_LOPATH_TXDAC_TXLPF_OW_CTRL
	write_reg8(0x80140f92, 0x00); // VCO_LOPATH_TXDAC_TXLPF_OW_VAL
	write_reg8(0x80140f94, 0x00); // SEQ_SPARELV_OW_CTRL
	write_reg8(0x80140f96, 0x00); // SEQ_SPARELV_OW_VAL
}
