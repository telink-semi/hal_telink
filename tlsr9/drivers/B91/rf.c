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
