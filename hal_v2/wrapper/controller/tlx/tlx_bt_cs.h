/**************************************************************************************************
 * @file    tlx_bt_cs.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
 **************************************************************************************************/
#ifndef _APP_CS_H_
#define _APP_CS_H_


extern rf_cs_ant_switch_ctrl ant_switch_ctrl[];
extern cs_ant_switch_config_t ant_cfg;

/**
 * @brief      BLE channel sounding initialize.
 * @param      None
 * @return     None
 */
void app_channel_sounding_init(void);

#define CS_OPTIMIZE_T_IP_FCS    1

/**
 *  @brief  Antenna Switch Configuration
 */
#ifndef ANTENNA_SWITCHING_AUTO_EN
	#define ANTENNA_SWITCHING_AUTO_EN 0
#endif

#ifndef NUM_ANT_SUPPORT
	#define NUM_ANT_SUPPORT 0x01
#endif

#ifndef MAX_ANT_PATHS_SUPPORT
	#define MAX_ANT_PATHS_SUPPORT 0X01
#endif

/**
 *  @brief  Antenna switch configuration for channel sounding
 */
#define ANTENNA_SWITCHING_SEL_0_PIN GPIO_PB3
#define ANTENNA_SWITCHING_CTRL_BASE     (0)

#define APP_CS_CONFIG_NUM 1
#define CS_RX_FIFO_NUM    4

#ifndef CS_ALIGN_16
#define CS_ALIGN_16(len)                 ((((len) + 15) >> 4) << 4)
#endif

#ifndef CS_RX_MODE0_FIFO_SIZE_MAX
#define CS_RX_MODE0_FIFO_SIZE_MAX        CS_ALIGN_16(80 * 20 + 80 + 4)
#endif

#ifndef CS_RX_MODE1_FIFO_SIZE_MAX
#define CS_RX_MODE1_FIFO_SIZE_MAX        CS_ALIGN_16((5 + 44 + 128 + 15) * 20 + 80 + 4)
#endif

#ifndef CS_RX_MODE2_FIFO_SIZE_MAX
#define CS_RX_MODE2_FIFO_SIZE_MAX(AP, PM, SW) \
	CS_ALIGN_16((5 + (AP + 1) * (PM + SW) - SW) * 20 + 80 + 4)
#endif

#ifndef max3
#define max3(a, b, c) \
	(((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c)))
#endif

#ifndef CS_RX_FIFO_SIZE
#define CS_RX_FIFO_SIZE \
	max3(CS_RX_MODE0_FIFO_SIZE_MAX, CS_RX_MODE1_FIFO_SIZE_MAX, \
		CS_RX_MODE2_FIFO_SIZE_MAX(MAX_ANT_PATHS_SUPPORT, 40, 10))
#endif

#define CS_PCT_DATA_SIZE \
	((MAX_ANT_PATHS_SUPPORT + 1) * STEP_NUM_PER_SUBEVENT * (PHASE_ITEM_LEN + AMP_ITEM_LEN))

#define CS_CALIB_OFFSET_CALI_TABLE_HEADER_INFO 0x300
/* RF_POWER_P6p71dBm = 27 for TL721X RF_TX_POWER_A3, matching cs_reflector_demo */
#define CS_USE_TX_POWER_LEVEL            27

#ifndef CS_SUPPORT_PRIVATE_CONTROL_PDU
	#define CS_SUPPORT_PRIVATE_CONTROL_PDU 1
#endif

extern u8 app_CsConfigParam[];
extern u8 hciCsSubeventRxFifo[];
extern u8 pct_raw_data[];

#endif /* _APP_CS_H_ */
