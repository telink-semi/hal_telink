/********************************************************************************************************
 * @file    emi.c
 *
 * @brief   This is the source file for B95
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "emi.h"
#include "stimer.h"
/**********************************************************************************************************************
 *                                           local macro                                                             *
 *********************************************************************************************************************/
#define EMI_STATE0		                     0x1234
#define EMI_STATE1		                     0x5678
#define EMI_TX_FIFO_ADDR                     REG_BASEBAND_BASE_ADDR	+ 0x1c//0x17001c
#define EMI_TX_PKT_PAYLOAD		             37
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
#define EMI_RX_MAX_PKT_PAYLOAD		             253

/**********************************************************************************************************************
 *                                           global constants                                                        *
 *********************************************************************************************************************/

static unsigned char  emi_rx_packet[280] __attribute__ ((aligned (4)));
static unsigned char  emi_ble_tx_packet [48]  __attribute__ ((aligned (4))) = {3,0,0,0,0,10};
static unsigned char  emi_zigbee_tx_packet[48]  __attribute__ ((aligned (4))) = {19,0,0,0,20,0,0};
static unsigned char  Private_TPLL_tx_packet[48] __attribute__ ((aligned (4))) = {3,0,0,0,0,10};
static unsigned int   s_emi_rx_cnt  __attribute__ ((aligned (4))) = 0;
static unsigned int   s_emi_rssibuf = 0;
static signed  char   s_emi_rssi = 0;
static unsigned int   s_state0,s_state1;

/**********************************************************************************************************************
 *                                          function implementation                                                   *
 *********************************************************************************************************************/

/**
 * @brief      This function serves to set singletone power
 * @param[in]  level - the power level.
 * @return     none
 */
void rf_set_power_level_index_singletone (rf_power_level_e level)
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
	reg_rf_ll_ctrl0 = 0x55;                                              // tx_en
	delay_us(150);

	value = (unsigned char)level&0x3f;
	reg_rf_lnm_pa_ow_ctrl_val |= BIT(6);							// TX_PA_PWR_OW  BIT6 set 1
	reg_rf_pa_ow_val = ((reg_rf_pa_ow_val&0x81)|(value<<1));		// TX_PA_PWR  BIT1 t0 BIT6 set value
}

extern unsigned char g_single_tong_freqoffset;

/**
 * @brief      This function serves to set the TX singletone power and channel
 * @param[in]  power_level - the power level.
 * @param[in]  rf_chn      - the channel.
 * @return     none
 */
void rf_emi_tx_single_tone(rf_power_level_e power_level,signed char rf_chn)
{
	g_single_tong_freqoffset = 1;
	rf_mode_init();
	rf_set_zigbee_250K_mode();
	rf_set_chn(rf_chn);
	rf_set_power_level_index_singletone(power_level);
	rf_set_txmode();
	g_single_tong_freqoffset = 0;

}

/**
 * @brief      This function serves to stop emi/(close RF)
 * @return     none
 */
void rf_emi_stop(void)
{
	reg_rf_lnm_pa_ow_ctrl_val = 0;
	reg_rf_pa_ow_val = 0; //TX_PA_PWRs
	rf_set_power_level_index (0);
	rf_set_tx_rx_off();
}

/**
 * @brief      This function serves to set the CD mode correlation register
 * @return     none
 */
void rf_emi_tx_continue_setup(void)
{
	reg_rf_tx_mode1 = 0x0a;
	reg_rf_access_code = 0x00;// access code
	reg_rf_tx_mode2 = 0x80;// kick tx controller to wait data

	s_state0 = EMI_STATE0;
	s_state1 = EMI_STATE1;
}

/**
 * @brief      This function serves to update the CD mode data
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  power_level - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @param[in]  pkt_type    - The type of data sent.
 * -#0:random
 * -#1:0xf0
 * -#2:0x55
 * @return     none
 */
void rf_emi_tx_continue_update_data(rf_mode_e rf_mode,rf_power_level_e power_level,signed char rf_chn,unsigned char pkt_type)
{
	rf_mode_init();
	switch(rf_mode)
	{
		case RF_MODE_BLE_1M_NO_PN:
			rf_set_ble_1M_NO_PN_mode();
			break;
		case RF_MODE_BLE_2M:
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

		default:break;
	}
	rf_pn_disable();
	rf_set_chn(rf_chn);
	reg_rf_ll_ctrl0 = 0x45;   // reset tx/rx state machine
	rf_set_power_level_index_singletone (power_level);
	rf_emi_tx_continue_setup();
	reg_rf_access_code = pkt_type; // 0:pbrs9 	1:0xf0	 2:0x55
}

/**
 * @brief      This function serves to generate random number.
 * @param[in]  state - the old random number.
 * @return     the new random number
 */
unsigned int emi_pn_gen(unsigned int state)
{
	unsigned int feed = 0;
	feed = (state&0x4000) >> 1;
	state ^= feed;
	state <<= 1;
	state = (state&0xfffe) + ((state&0x8000)>>15);
	return state;
}

/**
 * @brief      This function serves to continue to run the CD mode.The missing three in the program is because 
 *			   the original writing is problematic and will not be used.
 * @return     none
 */
void rf_continue_mode_run(void)
{
	if(1 == reg_rf_access_code){
		write_reg32(EMI_TX_FIFO_ADDR, 0x0f0f0f0f);
	}else if(2 == reg_rf_access_code){
		write_reg32(EMI_TX_FIFO_ADDR, 0x55555555);
	}else if(4 == reg_rf_access_code){
		write_reg32(EMI_TX_FIFO_ADDR, 0);
	}else if(5 == reg_rf_access_code){
		write_reg32(EMI_TX_FIFO_ADDR, 0xffffffff);
	}else{
		write_reg32(EMI_TX_FIFO_ADDR, (s_state0<<16)+s_state1);
		s_state0 = emi_pn_gen(s_state0);
		s_state1 = emi_pn_gen(s_state1);
	}

	while(read_reg8(EMI_TX_FIFO_ADDR) & 0x1){
	}
}

static unsigned char rxpara_flag = 1;
/**
 * @brief      This function serves to set rx mode and channel
 * @param[in]  mode   - mode of RF.
 * @param[in]  rf_chn - the rx channel.
 * @return     none
 */
void rf_emi_rx_setup(rf_mode_e mode,signed char rf_chn)
{
	rf_mode_init();
	switch(mode)
	{
		case RF_MODE_BLE_1M_NO_PN:
			rf_set_ble_1M_NO_PN_mode();
			break;
		case RF_MODE_BLE_2M:
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
		default:break;
	}
#if(0)
	rf_set_rx_maxlen(EMI_RX_MAX_PKT_PAYLOAD-2);//Rx mode in EMI is manual mode, and only one DMA fifo is used in manual mode.
										//If multiple DMA fifo are used, it should be noted that rx packet length cannot be greater than the depth of DMA fifo
#endif
	rf_set_rx_dma(emi_rx_packet,0,253);
	rf_pn_disable();
	rf_set_chn(rf_chn);//set freq
	if(mode != RF_MODE_ZIGBEE_250K)
		rf_access_code_comm(EMI_ACCESS_CODE); 	//access code
	rf_set_tx_rx_off();
	rf_set_rxmode();
	delay_us(150);
	if(rxpara_flag == 1)
	{
#if(0)
		rf_set_rxpara();
#endif
		rxpara_flag = 0;
	}

	if(rf_chn == 24 || rf_chn == 48 || rf_chn == 72)
	{
#if(0)
		rf_ldot_ldo_rxtxlf_bypass_en();
#endif
	}
	else
	{
#if(0)
		rf_ldot_ldo_rxtxlf_bypass_dis();
#endif
	}
	s_emi_rssi = 0;
	s_emi_rssibuf = 0;
	s_emi_rx_cnt = 0;
}

/**
 * @brief    This function serves to update the number of receiving packet and the RSSI
 * @return   none
 */
void rf_emi_rx_loop(void)
{
	if(rf_get_irq_status(FLD_RF_IRQ_RX))  // rx irq
	{
	   if((reg_rf_dec_err & 0xf0) == 0) // crc err
	   {
		   s_emi_rssibuf += reg_rf_agc_rssi_lat;
		   if(s_emi_rx_cnt)
		   {
			   if(s_emi_rssibuf != 0)
			   {
				   s_emi_rssibuf >>= 1;
			   }
		   }
		   s_emi_rssi = s_emi_rssibuf - 110;
		   s_emi_rx_cnt++;
	   }
	   rf_clr_irq_status(FLD_RF_IRQ_RX);    // clr rx irq
	   reg_rf_ll_cmd = 0x80;				// stop cmd
	}
}

/**
 * @brief    This function serves to get the number of packets received
 * @return   the number of packets received
 */
unsigned int rf_emi_get_rxpkt_cnt(void)
{
	return s_emi_rx_cnt;
}

/**
 * @brief    This function serves to get the RSSI of packets received
 * @return   the RSSI of packets received
 */
char rf_emi_get_rssi_avg(void)
{
	return s_emi_rssi;
}


/**
 * @brief      This function serves to generate random packets that need to be sent in burst mode
 * @param[in] *p - the address of random packets.
 * @param[in]  n - the number of random packets.
 * @return     none
 */
void rf_phy_test_prbs9 (unsigned char *p, int n)
{
	unsigned short x = 0x1ff;
	int i = 0;
	int j = 0;
	for ( i=0; i<n; i++)
	{
		unsigned char d = 0;
		for (j=0; j<8; j++)
		{
			if (x & 1)
			{
				d |= BIT(j);
			}
			x = (x >> 1) | (((x<<4) ^ (x<<8)) & 0x100);
		}
		*p++ = d;
	}
}

/**
 * @brief      This function serves to send packets in the burst mode
 * @param[in]  rf_mode  - mode of RF.
 * @param[in]  pkt_type - The type of data sent.
 * -#0:random
 * -#1:0xf0
 * -#2:0x55
 * @return     none
 */

void rf_emi_tx_burst_loop(rf_mode_e rf_mode,unsigned char pkt_type)
{
	unsigned char rf_data_len = EMI_TX_PKT_PAYLOAD+1;
	unsigned int rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
	reg_rf_ll_cmd = 0x80; // stop SM
	rf_set_txmode();
	if((rf_mode==RF_MODE_BLE_1M_NO_PN)||(rf_mode==RF_MODE_BLE_2M))//ble
	{
        rf_data_len = EMI_TX_PKT_PAYLOAD+2;
        rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[4]=0;
        emi_ble_tx_packet[5]=EMI_TX_PKT_PAYLOAD;
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len&0xff;
        rf_start_stx ((void *)emi_ble_tx_packet,stimer_get_tick() + 10);
		while(!(rf_get_irq_status(FLD_RF_IRQ_TX)));
		rf_clr_irq_status(FLD_RF_IRQ_TX);

	    delay_ms(2);
	    if(pkt_type == 0)
	    	rf_phy_test_prbs9(&emi_ble_tx_packet[6],37);
	}
	else if(rf_mode == RF_MODE_LR_S8_125K)
	{
        rf_data_len = EMI_TX_PKT_PAYLOAD+2;
        rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[4]=0;
        emi_ble_tx_packet[5]=EMI_TX_PKT_PAYLOAD;
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len&0xff;
        rf_start_stx ((void *)emi_ble_tx_packet,stimer_get_tick() + 10);
		while(!(rf_get_irq_status(FLD_RF_IRQ_TX)));
		rf_clr_irq_status(FLD_RF_IRQ_TX);


	    delay_ms(2);
	    if(pkt_type == 0)
	    	rf_phy_test_prbs9(&emi_ble_tx_packet[6],37);
	}
	else if(rf_mode == RF_MODE_LR_S2_500K)
	{
        rf_data_len = EMI_TX_PKT_PAYLOAD+2;
        rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
        emi_ble_tx_packet[4]=0;
        emi_ble_tx_packet[5]=EMI_TX_PKT_PAYLOAD;
        emi_ble_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
        emi_ble_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
        emi_ble_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
        emi_ble_tx_packet[0] = rf_tx_dma_len&0xff;
        rf_start_stx ((void *)emi_ble_tx_packet,stimer_get_tick() + 10);
		while(!(rf_get_irq_status(FLD_RF_IRQ_TX)));
		rf_clr_irq_status(FLD_RF_IRQ_TX);


	    delay_ms(2);
	    if(pkt_type == 0)
	    	rf_phy_test_prbs9(&emi_ble_tx_packet[6],37);
	}
	else if(rf_mode == RF_MODE_ZIGBEE_250K)
	{
		rf_data_len = EMI_TX_PKT_PAYLOAD+1;
		rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
		emi_zigbee_tx_packet[4]=EMI_TX_PKT_PAYLOAD+2;
		emi_zigbee_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
		emi_zigbee_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
		emi_zigbee_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
		emi_zigbee_tx_packet[0] = rf_tx_dma_len&0xff;
		rf_start_stx ((void *)emi_zigbee_tx_packet,stimer_get_tick() + 10);
		while(!(rf_get_irq_status(FLD_RF_IRQ_TX)));
		rf_clr_irq_status(FLD_RF_IRQ_TX);


	    delay_us(625*2);
	    if(pkt_type == 0)
	    	rf_phy_test_prbs9(&emi_zigbee_tx_packet[5],37);
	}
	else if((rf_mode == RF_MODE_PRIVATE_1M) || (rf_mode == RF_MODE_PRIVATE_2M))
		{
			rf_data_len = EMI_TX_PKT_PAYLOAD+1;
			Private_TPLL_tx_packet[4]=EMI_TX_PKT_PAYLOAD;
			rf_tx_dma_len = rf_tx_packet_dma_len(rf_data_len);
			Private_TPLL_tx_packet[3] = (rf_tx_dma_len >> 24)&0xff;
			Private_TPLL_tx_packet[2] = (rf_tx_dma_len >> 16)&0xff;
			Private_TPLL_tx_packet[1] = (rf_tx_dma_len >> 8)&0xff;
			Private_TPLL_tx_packet[0] = rf_tx_dma_len&0xff;
			rf_start_stx((void *)Private_TPLL_tx_packet, read_reg32(0x140200) + 10);
			while(!(rf_get_irq_status(FLD_RF_IRQ_TX)));
			rf_clr_irq_status(FLD_RF_IRQ_TX);


			delay_ms(1);
			if(pkt_type == 0)
				rf_phy_test_prbs9(&Private_TPLL_tx_packet[5],37);
		}
}


/**
 * @brief      This function serves to set the burst mode
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  power_level - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @param[in]  pkt_type    - The type of data sent.
 * -#0:random
 * -#1:0xf0
 * -#2:0x55
 * @return     none
 */
void rf_emi_tx_burst_setup(rf_mode_e rf_mode,rf_power_level_e power_level,signed char rf_chn,unsigned char pkt_type)
{
	unsigned char i = 0;
	unsigned char tx_data = 0;
//	write_reg8(0x100844,0x10); // print buffer size set
	rf_set_tx_dma(2,128);
	rf_set_chn(rf_chn);
	rf_mode_init();
	switch(rf_mode)
	{
		case RF_MODE_BLE_1M_NO_PN:
			rf_set_ble_1M_NO_PN_mode();
			break;
		case RF_MODE_BLE_2M:
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
		default:break;
	}
	if(rf_mode != RF_MODE_ZIGBEE_250K)
		rf_access_code_comm(EMI_ACCESS_CODE); 	//access code

	rf_pn_disable();
	rf_set_power_level (power_level);
	if(pkt_type == 1)
	{
		tx_data = 0x0f;
	}
	else if(pkt_type == 2)
	{
		tx_data = 0x55;
	}
	else if(pkt_type == 3)
	{
		tx_data = 0xaa;
	}
	else if(pkt_type==4){
		tx_data = 0xf0;
	}
	switch(rf_mode)
	{
	    case RF_MODE_LR_S2_500K:
	    case RF_MODE_LR_S8_125K:
	    case RF_MODE_BLE_1M_NO_PN:
	    case RF_MODE_BLE_2M:
	    	emi_ble_tx_packet[4] = pkt_type;//type
	    	for(i = 0;i < 37;i++)
	    	{
	    		emi_ble_tx_packet[6+i]=tx_data;
	    	}
	    	break;
	    case RF_MODE_ZIGBEE_250K:
	    	emi_zigbee_tx_packet[5] = pkt_type; // type
	    	for(i = 0;i < 37;i++)
	    	{
	    		emi_zigbee_tx_packet[5 + i] = tx_data;
	    	}
	    	break;
	    case RF_MODE_PRIVATE_1M:
	    case RF_MODE_PRIVATE_2M:
	    	Private_TPLL_tx_packet[5] = pkt_type; // type
			for(i = 0;i < 37;i++)
			{
				Private_TPLL_tx_packet[5 + i] = tx_data;
			}
	    	break;

	    default:
	    	break;
	}

}

/**
 * @brief      This function serves to reset baseband
 * @return     none
 */
void rf_emi_reset_baseband(void)
{
	reg_rst4 &= (~FLD_RST4_ZB);      		  // reset baseband
	reg_rst4 |= (FLD_RST4_ZB);				  // clr baseband
}

/**
 * @brief This function is used in RF current test configurations where only RF modules work
 * @param[in]	none.
 * @return		none.
 * @note  The complete test conditions for TX/RX current:
 *        (1) turn off the clock and power of the modules irrelevant to RF
 *        (2) hold the modules irrelevant to RF
 *        (3) disable PLL clock and RC_24M to make the Pad_24M is the only clock source for cclk
 *        (4) stall the MCU
 */
_attribute_ram_code_sec_noinline_ void rf_current_test_cfg(void)
{

/*        The following configurations are required for RF current testing:
 *        (1)During rf current test, hold digital modules except sws,algm,baseband,analog module to reduce dynamic power consumption
 *        (2)Turn off unnecessary module clocks and keep only swrire clk,analog spi clk,mcu clk,lm clk,zb pclk
 *        (3)Make the Pad_24M is the only clock source for cclk
 *        (4)Power down usb,npe
 *        (5)Power down of 24MHz RC oscillator
 *        (6)Power down bbpll ldo
 *        (7)stall the MCU
 *
 */
	//During rf current test, hold digital modules except sws/algm/baseband/analog module to reduce dynamic power consumption
	write_reg8(0x1401e0, 0x80);//reset swires,hold lspi/i2c/uart0/usb/pwm/uart1
	write_reg8(0x1401e1, 0x08);//reset algm,hold chg/stimer/dma/pke/gspi/spislv
	write_reg8(0x1401e2, 0x38);//enable power on reset to reset mcu (reset all),hold timer/audio/i2c1/trng/dpr
	write_reg8(0x1401e3, 0x81);//analog module reset,reset baseband,hold zb_mstclk/zb_lpclk/mspi/qdec/sar adc

	//Turn off unnecessary module clocks and keep only swrire clk/analog spi clk/mcu clk/lm clk/zb pclk
	write_reg8(0x1401e4, 0x80);//swrire clk_en,disable lspi/i2c/uart0/usb/pwm/dbgen/uart1 clock
	write_reg8(0x1401e5, 0x08);//analog spi clk_en,disable chg/stimer/dma/pke/machinetime/gspi/spislv clock
	write_reg8(0x1401e6, 0x30);//mcu_clk_en,lm_clk_en,disable timer/audio/i2c1/trng/dpr clock
	write_reg8(0x1401e7, 0x01);//zb_pclk_en,disable zb_mstclk/zb_lpclk/mspi/qdec clock

	write_reg8(0x1401e8,0x92);//clock source for cclk:PAD 24M
					          //This operation makes Pad_24M the only clock source for cclk.
						      //Before that, it is necessary to ensure that the default source clock is normal

	analog_write_reg8(0x7d, 0x86); //Keep baseband power on,<bit1>power down usb;<bit2>power down npe
	analog_write_reg8(0x05, analog_read_reg8(0x05)|BIT(2));//Power down of 24MHz RC oscillator
	analog_write_reg8(0x06,analog_read_reg8(0x06)|0x01); //Power down bbpll ldo

	__asm__ __volatile__ ("wfi");//mcu_stall
}


