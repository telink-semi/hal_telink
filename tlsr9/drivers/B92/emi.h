/********************************************************************************************************
 * @file    emi.h
 *
 * @brief   This is the header file for B92
 *
 * @author  Driver Group
 * @date    2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef  EMI_H_
#define  EMI_H_

#include "lib/include/rf.h"

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
#define EMI_ACCESS_ADDR                      0x140808
#define EMI_ACCESS_CODE                      0x29417671

/**********************************************************************************************************************
 *                                         function declaration                                                    *
 *********************************************************************************************************************/

/**
 * @brief      This function serves to set singletone power and channel
 * @param[in]  power_level - the power level.
 * @param[in]  rf_chn      - the channel.
 * @return     none
 */
void rf_emi_tx_single_tone(rf_power_level_e power_level,signed char rf_chn);

/**
 * @brief      This function serves to set rx mode and channel
 * @param[in]  mode   - mode of RF.
 * @param[in]  rf_chn - the rx channel.
 * @return     none
 */
void rf_emi_rx_setup(rf_mode_e mode,signed char rf_chn);

/**
 * @brief    This function serves to update the number of receiving packet and the RSSI
 * @return   none
 */
void rf_emi_rx_loop(void);

/**
 * @brief    This function serves to stop emi/(close RF)
 * @return   none
 */
void rf_emi_stop(void);

/**
 * @brief    This function serves to get the number of packets received.
 * @return   the number of packets received.
 */
unsigned int rf_emi_get_rxpkt_cnt(void);

/**
 * @brief    This function serves to get the RSSI of packets received
 * @return   the RSSI of packets received
 */
char rf_emi_get_rssi_avg(void);


/**
 * @brief      This function serves to set the CD mode correlation register
 * @return     none
 */
void rf_emi_tx_continue_setup(void);

/**
 * @brief      This function serves to update the CD mode data.
 * @param[in]  rf_mode     - mode of RF.
 * @param[in]  power_level - power level of RF.
 * @param[in]  rf_chn      - channel of RF.
 * @param[in]  pkt_type    - The type of data sent.
 * -#0:random
 * -#1:0xf0
 * -#2:0x55
 * @return     none
 */
void rf_emi_tx_continue_update_data(rf_mode_e rf_mode,rf_power_level_e power_level,signed char rf_chn,unsigned char pkt_type);

/**
 * @brief      This function serves to continue to run the CD mode
 * @return     none
 */
void rf_continue_mode_run(void);

/**
 * @brief      This function serves to send packets in the burst mode
 * @param[in]  rf_mode  - mode of RF.
 * @param[in]  pkt_type - The type of data sent.
 * -#0:random
 * -#1:0xf0
 * -#2:0x55
 * @return     none
 */
void rf_emi_tx_burst_loop(rf_mode_e rf_mode,unsigned char pkt_type);

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
void rf_emi_tx_burst_setup(rf_mode_e rf_mode,rf_power_level_e power_level,signed char rf_chn,unsigned char pkt_type);

/**
 * @brief      This function serves to generate random packets that need to be sent in burst mode
 * @param[in] *p - the address of random packets.
 * @param[in]  n - the number of random packets.
 * @return     none
 */
void rf_phy_test_prbs9 (unsigned char *p, int n);

/**
 * @brief      This function serves to reset baseband.
 * @return     none
 */
void rf_emi_reset_baseband(void);

/**
 * @brief      This function serves to generate random number.
 * @param[in]  state - the old random number.
 * @return     the new random number
 */
unsigned int emi_pn_gen(unsigned int state);

/**
 * @brief This function is used in RF current test configurations where only RF modules work
 * @param[in]	none.
 * @return		none.
 * @note  The complete test conditions for TX/RX current:
 *        (1) turn off the clock and power of the modules irrelevant to RF;
 *        (2) hold the modules irrelevant to RF;
 *        (3) disable PLL clock and RC_24M to make the Pad_24M is the only clock source for cclk;
 *        (4) stall the MCU.
 */
_attribute_ram_code_com_sec_noinline_ void rf_current_test_cfg(void);

/**
 * @brief      This function is mainly used to set the mode, channel and packet sending energy for sending pkt+tone mode.
 * @param[in]  rf_mode	   - Which mode to use to send packets.
 * @param[in]  power_level - the power level of packet.
 * @param[in]  rf_chn      - the channel.
 * @return     none
 */
void rf_emi_tx_pkt_tone_setup(rf_mode_e rf_mode,rf_power_level_e power_level,signed char rf_chn);

/**
 * @brief      This function is used to send PKT+TONE.
 * @param[in]  tone_power  - Which mode to use to send packets.
 * @param[in]  power_level - Set the energy of the sent tone. Usually the energy of this tone needs to be consistent with
 * 			   the energy of the pkt sent together.
 * @param[in]  tone_us      - Set the duration of sending tone.
 * @return     none
 */
void rf_emi_tx_pkt_tone_loop(rf_power_level_e tone_power,unsigned char tone_us);

#endif
