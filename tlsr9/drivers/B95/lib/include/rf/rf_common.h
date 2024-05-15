/********************************************************************************************************
 * @file    rf_common.h
 *
 * @brief   This is the header file for B95
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
#ifndef     RF_COMMON_H
#define     RF_COMMON_H

#include "lib/include/sys.h"
#include "lib/include/rf/rf_dma.h"
#include "lib/include/rf/rf_ble.h"
#include "lib/include/rf/rf_private.h"
#include "lib/include/rf/rf_zigbee.h"
#include "gpio.h"
#include "stimer.h"
/**********************************************************************************************************************
 *                                         RF  global macro                                                           *
 *********************************************************************************************************************/
#ifndef RF_BB_TIMER_AUTO_MODE
#define RF_BB_TIMER_AUTO_MODE     			1  //TODO:Need to update code after subsequent confirmation of usage
#endif

/**
 * @brief		This define for ble debug the effect of rx_dly.
 * 				when this function turn on the time of rx_dly will shorten 6.3us,
 */
#define 	RF_RX_SHORT_MODE_EN			1//In order to debug whether the problem is caused by rx_dly.
/**
 *  @brief This define serve to calculate the DMA length of packet.
 */
#define 	rf_tx_packet_dma_len(rf_data_len)			(((rf_data_len)+3)/4)|(((rf_data_len) % 4)<<22)

/**********************************************************************************************************************
 *                                       RF global data type                                                          *
 *********************************************************************************************************************/


/**
 *  @brief  Rx fast settle time
 *  @note Some notice for timing sequence.
 *  1:The timing sequence can be configured once during initialization.
 *  2:The timing sequence of tx and rx can be configured separately.
 *  3:Call the enable function rf_tx_fast_settle_en or rf_rx_fast_settle_en when using the configured timing sequence.
 *    To close it, call rf_tx_fast_settle_dis or rf_rx_fast_settle_dis.
 *  4:According to the different parameters, a normal calibration should be done regularly, such as parameter notes.
 *	  stop RF-related states, enable RX, wait for packet transmission to end -> rf_rx_fast_settle_update_cal_val.
 */
typedef enum{
	RX_SETTLE_TIME_15US		 = 0,/**<   reduce 74us of rx settle time.
	                                    Receive for a period of time and then do a normal calibration.
										note:
										1. Related to frequency points. When enabling fast settle on both TX and RX ends,
										   if 15us option is selected, both TX and RX ends must use the 15us option simultaneously.
										2. Refer to the following table to determine the range of frequency points used.
										   In addition to configuring the calibration values for the used frequency points,
										   corresponding channel values in the respective intervals need to be configured.
										   For example, if using the 2426 frequency point, channel should be configured as 26 and 24.
									   ----------------------------------------------------------------------------------
									   | Frequency     2400   2410   2420   2430   2440   2450   2460   2470   2480     |
									   |-----------------|------|------|------|------|------|------|------|------|------|
									   | channel Value   |   4  |  14  |  24  |  34  |  44  |  54  |  64  |  74  |  74  |
	                                    Receive for a period of time and then do a normal calibration.*/

	RX_SETTLE_TIME_37US		 = 1,/**<   reduce 44.5us of rx settle time.
	                                    Receive for a period of time and then do a normal calibration.*/

	RX_SETTLE_TIME_77US		 = 2,/**<  reduce 8.5us of rx settle time.
	                                    Do a normal calibration at the beginning.*/
	RX_FAST_SETTLE_NONE		 = 3

}rf_rx_fast_settle_time_e;

/**
 *  @brief  Tx fast settle time
 *  @note
 *  1:Call rf_fast_settle_config to configure timing during initialization.
 *  2:Call the enable function rf_tx_fast_settle_en when using the configured timing sequence.
 *    To close it, call rf_tx_fast_settle_dis.
 *  3:The deleted hardware calibration values are influenced by environmental temperature and require periodic recalibration.
 *	  Calibration method: Call rf_tx_fast_settle_dis, then for different frequency points:
 *	  stop RF-related states, enable TX, wait for packet transmission to end -> rf_tx_fast_settle_update_cal_val.
 */
typedef enum{
	TX_SETTLE_TIME_15US	 	= 0,/**<   reduce 87.5us of tx settle time.
	                                   After frequency hopping, a normal calibration must be done.
									   note:
									   1. Related to frequency points. When enabling fast settle on both TX and RX ends,
									   if 15us option is selected, both TX and RX ends must use the 15us option simultaneously.
									   2. Refer to the following table to determine the range of frequency points used.
										  Corresponding channel values in the respective intervals need to be configured.
										  For example, if using the 2426 frequency point, channel should be configured 24.
									   ----------------------------------------------------------------------------------
									   | Frequency     2400   2410   2420   2430   2440   2450   2460   2470   2480     |
									   |-----------------|------|------|------|------|------|------|------|------|------|
									   | channel Value   |   4  |  14  |  24  |  34  |  44  |  54  |  64  |  74  |  74  |
	                                   After frequency hopping, a normal calibration must be done.*/

	TX_SETTLE_TIME_51US	 	= 1,/**<  disable tx_ldo_trim function and tx_hpmc,reduce 61.5us of tx settle time.
	                                   After frequency hopping, a normal calibration must be done.*/

	TX_SETTLE_TIME_104US    = 2,/**<   reduce 8.5us of tx settle time.
	                                   Do a normal calibration at the beginning.*/
	TX_FAST_SETTLE_NONE		= 3

}rf_tx_fast_settle_time_e;


/**
 *  @brief  LDO trim calibration value
 */
typedef struct
{
	unsigned char LDO_CAL_TRIM;
	unsigned char LDO_RXTXHF_TRIM;
	unsigned char LDO_RXTXLF_TRIM;
	unsigned char LDO_PLL_TRIM;
	unsigned char LDO_VCO_TRIM;
}rf_ldo_trim_t;

/**
 *  @brief  DCOC calibration value
 */
typedef struct
{
	unsigned char DCOC_IDAC;
	unsigned char DCOC_QDAC;
	unsigned char DCOC_IADC_OFFSET;
	unsigned char DCOC_QADC_OFFSET;
}rf_dcoc_cal_t;

/**
 *  @brief  RCCAL calibration value
 */
typedef struct
{
	unsigned char RCCAL_CODE;
	unsigned char CBPF_CCODE_L;
	unsigned char CBPF_CCODE_H;
}rf_rccal_cal_t;

typedef struct
{
	unsigned short cal_tbl[81];
	rf_ldo_trim_t	ldo_trim;
	rf_dcoc_cal_t   dcoc_cal;
	rf_rccal_cal_t  rccal_cal;
}rf_fast_settle_t;


/**
 * @brief  	  	RF CRC config.
 * @note        init_value - The CRC init value to be set for RF.
 * 				poly - The CRC polynomial to be set for RF.
 * 				xor_out - XOR mask for CRC result (for no mask, should be 0)
 * 				byte_order - CRC output byte order (1:MSByte first,0:LSByte first)
 * 				start_cal_pos - CRC start byte position, 0 is the first byte
 * 				len - The CRC length to be set for RF
 */
typedef struct {
	unsigned int init_value;
	unsigned int poly;
	unsigned int xor_out;
	unsigned char byte_order;
	unsigned char start_cal_pos;
	unsigned char len;
} rf_crc_config_t;


/**
 *  @brief  select status of rf.
 */
typedef enum {
    RF_MODE_TX = 0,		/**<  Tx mode */
    RF_MODE_RX = 1,		/**<  Rx mode */
    RF_MODE_AUTO=2,		/**<  Auto mode */
	RF_MODE_OFF =3		/**<  TXRX OFF mode */
} rf_status_e;

/**
 *  @brief  Define power list of RF.
 *  @note	The energy value with the 1.8V keyword is the energy value when the GPIO output voltage is 1.8V
 *          (i.e. when the GPIO voltage in sys_init is configured to GPIO_VOLTAGE_1V8)
 *          Since the GPIO output voltage is set to 1.8V, the VDDO3 voltage supplying the RF is 1.8V, which will cause the energy to drop.
 *          Therefore, when the GPIO output voltage is set to 1.8V, the energy setting with the 1.8V keyword should be selected.
 *          Attention:
 *          (1)The power values in the table are for reference only, and the specific power values are subject to actual testing
 *          (2)For A0: At present, the maximum TX power can only reach 0dbm, and only the maximum gear(RF_POWER_P9p90dBm) is recommended.
 */
typedef enum {
	 /*VBAT*/
	 RF_POWER_P9p90dBm  = 63, /**<  9.9 dbm */
	 RF_POWER_P9p61dBm  = 59, /**<  9.6 dbm */
	 RF_POWER_P9p15dBm  = 54, /**<  9.1 dbm */
	 RF_POWER_P8p82dBm  = 51, /**<  8.8 dbm */
	 RF_POWER_P8p25dBm  = 48, /**<  8.2 dbm */
	 RF_POWER_P7p72dBm  = 44, /**<  7.7 dbm */
	 RF_POWER_P7p00dBm  = 40, /**<  7.0 dbm */
	 RF_POWER_P6p32dBm  = 36, /**<  6.3 dbm */
	 RF_POWER_P5p21dBm  = 32, /**<  5.2 dbm */

	 /*VANT*/
	 RF_POWER_P4p61dBm  = BIT(7) | 63,   /**<   4.6 dbm */
	 RF_POWER_P4p02dBm  = BIT(7) | 55,   /**<   4.0 dbm */
	 RF_POWER_P3p51dBm  = BIT(7) | 49,   /**<   3.5 dbm */
	 RF_POWER_P3p00dBm  = BIT(7) | 45,   /**<   3.0 dbm */
	 RF_POWER_P2p51dBm  = BIT(7) | 41,   /**<   2.5 dbm */
	 RF_POWER_P2p01dBm  = BIT(7) | 38,   /**<   2.0 dbm */
	 RF_POWER_P1p62dBm  = BIT(7) | 35,   /**<   1.6 dbm */
	 RF_POWER_P1p03dBm  = BIT(7) | 33,   /**<   1.0 dbm */
	 RF_POWER_P0p31dBm  = BIT(7) | 29,   /**<   0.3 dbm */
	 RF_POWER_P0p01dBm  = BIT(7) | 28,   /**<   0.0 dbm */
	 RF_POWER_N0p43dBm  = BIT(7) | 26,   /**<  -0.4 dbm */
	 RF_POWER_N1p15dBm  = BIT(7) | 24,   /**<  -1.0 dbm */
	 RF_POWER_N1p52dBm  = BIT(7) | 23,   /**<  -1.5 dbm */
	 RF_POWER_N2p51dBm  = BIT(7) | 20,   /**<  -2.5 dbm */
	 RF_POWER_N3p95dBm  = BIT(7) | 17,   /**<  -3.9 dbm */
	 RF_POWER_N5p94dBm  = BIT(7) | 13,   /**<  -5.9 dbm */
	 RF_POWER_N9p03dBm  = BIT(7) | 9,    /**<  -9.0 dbm */
	 RF_POWER_N13p42dBm = BIT(7) | 5,    /**<  -13.4 dbm */
	 RF_POWER_N22p53dBm = BIT(7) | 2,    /**<  -22.5 dbm */

	 RF_POWER_N30dBm    = 0xff,          /**<  -30 dbm */
	 RF_POWER_N50dBm    = BIT(7) | 0,    /**<  -50 dbm */

	 /*VBAT*/
	 RF_1V8_POWER_P6p32dBm = 63, /**<  6.3 dbm */
	 RF_1V8_POWER_P5p81dBm = 56, /**<  5.8 dbm */
	 RF_1V8_POWER_P5p00dBm = 50, /**<  5.0 dbm */
	 RF_1V8_POWER_P4p50dBm = 45, /**<  4.5 dbm */

	 /*VANT*/
	 RF_1V8_POWER_P4p23dBm  = BIT(7) | 63,   /**<   4.2 dbm */
	 RF_1V8_POWER_P3p91dBm  = BIT(7) | 55,   /**<   3.9 dbm */
	 RF_1V8_POWER_P3p43dBm  = BIT(7) | 50,   /**<   3.4 dbm */
	 RF_1V8_POWER_P2p94dBm  = BIT(7) | 45,   /**<   2.9 dbm */
	 RF_1V8_POWER_P2p42dBm  = BIT(7) | 41,   /**<   2.4 dbm */
	 RF_1V8_POWER_P1p95dBm  = BIT(7) | 38,   /**<   1.9 dbm */
	 RF_1V8_POWER_P1p21dBm  = BIT(7) | 34,   /**<   1.2 dbm */
	 RF_1V8_POWER_P0p82dBm  = BIT(7) | 31,   /**<   0.8 dbm */
	 RF_1V8_POWER_P0p21dBm  = BIT(7) | 29,   /**<   0.0 dbm */
	 RF_1V8_POWER_N0p34dBm  = BIT(7) | 27,   /**<  -0.3 dbm */
	 RF_1V8_POWER_N1p33dBm  = BIT(7) | 23,   /**<  -1.3 dbm */
	 RF_1V8_POWER_N1p80dBm  = BIT(7) | 22,   /**<  -1.8 dbm */
	 RF_1V8_POWER_N2p53dBm  = BIT(7) | 20,   /**<  -2.5 dbm */
	 RF_1V8_POWER_N3p31dBm  = BIT(7) | 18,   /**<  -3.3 dbm */
	 RF_1V8_POWER_N5p32dBm  = BIT(7) | 14,   /**<  -5.3 dbm */
	 RF_1V8_POWER_N7p32dBm  = BIT(7) | 11,   /**<  -7.3 dbm */
	 RF_1V8_POWER_N9p81dBm  = BIT(7) | 9,     /**<  -9.8 dbm */
	 RF_1V8_POWER_N12p42dBm = BIT(7) | 6,    /**<  -12.4 dbm */
	 RF_1V8_POWER_N16p01dBm = BIT(7) | 4,    /**<  -16.0 dbm */
	 RF_1V8_POWER_N27p51dBm = BIT(7) | 1,    /**<  -27.5 dbm */

	 RF_1V8_POWER_N30p00dBm    = 0xff,          /**<  -30 dbm */
	 RF_1V8_POWER_N50p00dBm    = BIT(7) | 0,    /**<  -50 dbm */
} rf_power_level_e;

/**
 *  @brief  Define power index list of RF.
 *  @note	The energy value with the 1.8V keyword is the energy value when the GPIO output voltage is 1.8V
 *          (i.e. when the GPIO voltage in sys_init is configured to GPIO_VOLTAGE_1V8)
 *          Since the GPIO output voltage is set to 1.8V, the VDDO3 voltage supplying the RF is 1.8V, which will cause the energy to drop.
 *          Therefore, when the GPIO output voltage is set to 1.8V, the energy setting with the 1.8V keyword should be selected.
 *          Attention:
 *          (1)The power values in the table are for reference only, and the specific power values are subject to actual testing
 *          (2)For A0: At present, the maximum TX power can only reach 0dbm, and only the maximum gear(RF_POWER_INDEX_P9p90dBm) is recommended.
 */

typedef enum {

	 /*VBAT*/
	 RF_POWER_INDEX_P9p90dBm,    /**<  power index of 9.9 dbm */
	 RF_POWER_INDEX_P9p61dBm,    /**<  power index of 9.6 dbm */
	 RF_POWER_INDEX_P9p15dBm,    /**<  power index of 9.1 dbm */
	 RF_POWER_INDEX_P8p82dBm,    /**<  power index of 8.8 dbm */
	 RF_POWER_INDEX_P8p25dBm,    /**<  power index of 8.2 dbm */
	 RF_POWER_INDEX_P7p72dBm,    /**<  power index of 7.7 dbm */
	 RF_POWER_INDEX_P7p00dBm,    /**<  power index of 7.0 dbm */
	 RF_POWER_INDEX_P6p32dBm,    /**<  power index of 6.3 dbm */
	 RF_POWER_INDEX_P5p21dBm,    /**<  power index of 5.2 dbm */

	 /*VANT*/
	 RF_POWER_INDEX_P4p61dBm,    /**<  power index of 4.6 dbm */
	 RF_POWER_INDEX_P4p02dBm,    /**<  power index of 4.0 dbm */
	 RF_POWER_INDEX_P3p51dBm,    /**<  power index of 3.5 dbm */
	 RF_POWER_INDEX_P3p00dBm,    /**<  power index of 3.0 dbm */
	 RF_POWER_INDEX_P2p51dBm,    /**<  power index of 2.5 dbm */
	 RF_POWER_INDEX_P2p01dBm,    /**<  power index of 2.0 dbm */
	 RF_POWER_INDEX_P1p62dBm,    /**<  power index of 1.6 dbm */
	 RF_POWER_INDEX_P1p03dBm,    /**<  power index of 1.0 dbm */
	 RF_POWER_INDEX_P0p31dBm,    /**<  power index of 0.3 dbm */
	 RF_POWER_INDEX_P0p01dBm,    /**<  power index of 0.0 dbm */
	 RF_POWER_INDEX_N0p43dBm,    /**<  power index of -0.4 dbm */
	 RF_POWER_INDEX_N1p15dBm,    /**<  power index of -1.0 dbm */
	 RF_POWER_INDEX_N1p52dBm,    /**<  power index of -1.5 dbm */
	 RF_POWER_INDEX_N2p51dBm,    /**<  power index of -2.5 dbm */
	 RF_POWER_INDEX_N3p95dBm,    /**<  power index of -3.9 dbm */
	 RF_POWER_INDEX_N5p94dBm,    /**<  power index of -5.9 dbm */
	 RF_POWER_INDEX_N9p03dBm,    /**<  power index of -9.0 dbm */
	 RF_POWER_INDEX_N13p42dBm,   /**<  power index of -13.4 dbm */
	 RF_POWER_INDEX_N22p53dBm,   /**<  power index of -22.5 dbm */

	 RF_POWER_INDEX_N30dBm,      /**<  power index of -30 dbm */
	 RF_POWER_INDEX_N50dBm,      /**<  power index of -50 dbm */

	 /*VBAT*/
	 RF_1V8_POWER_INDEX_P6p32dBm,    /**<  power index of 6.3 dbm */
	 RF_1V8_POWER_INDEX_P5p81dBm,    /**<  power index of 5.8 dbm */
	 RF_1V8_POWER_INDEX_P5p00dBm,    /**<  power index of 5.0 dbm */
	 RF_1V8_POWER_INDEX_P4p50dBm,    /**<  power index of 4.5 dbm */

	 /*VANT*/
	 RF_1V8_POWER_INDEX_P4p23dBm,    /**<  power index of 4.2 dbm */
	 RF_1V8_POWER_INDEX_P3p91dBm,    /**<  power index of 3.9 dbm */
	 RF_1V8_POWER_INDEX_P3p43dBm,    /**<  power index of 3.4 dbm */
	 RF_1V8_POWER_INDEX_P2p94dBm,    /**<  power index of 2.9 dbm */
	 RF_1V8_POWER_INDEX_P2p42dBm,    /**<  power index of 2.4 dbm */
	 RF_1V8_POWER_INDEX_P1p95dBm,    /**<  power index of 1.9 dbm */
	 RF_1V8_POWER_INDEX_P1p21dBm,    /**<  power index of 1.2 dbm */
	 RF_1V8_POWER_INDEX_P0p82dBm,    /**<  power index of 0.8 dbm */
	 RF_1V8_POWER_INDEX_P0p21dBm,    /**<  power index of 0.0 dbm */
	 RF_1V8_POWER_INDEX_N0p34dBm,    /**<  power index of -0.3 dbm */
	 RF_1V8_POWER_INDEX_N1p33dBm,    /**<  power index of -1.3 dbm */
	 RF_1V8_POWER_INDEX_N1p80dBm,    /**<  power index of -1.8 dbm */
	 RF_1V8_POWER_INDEX_N2p53dBm,    /**<  power index of -2.5 dbm */
	 RF_1V8_POWER_INDEX_N3p31dBm,    /**<  power index of -3.3 dbm */
	 RF_1V8_POWER_INDEX_N5p32dBm,    /**<  power index of -5.3 dbm */
	 RF_1V8_POWER_INDEX_N7p32dBm,    /**<  power index of -7.3 dbm */
	 RF_1V8_POWER_INDEX_N9p81dBm,    /**<  power index of -9.8 dbm */
	 RF_1V8_POWER_INDEX_N12p42dBm,   /**<  power index of -12.4 dbm */
	 RF_1V8_POWER_INDEX_N16p01dBm,   /**<  power index of -16.0 dbm */
	 RF_1V8_POWER_INDEX_N27p51dBm,   /**<  power index of -27.5 dbm */


	 RF_1V8_POWER_INDEX_N30p00dBm,   /**<  power index of -30 dbm */
	 RF_1V8_POWER_INDEX_N50p00dBm,   /**<  power index of -50 dbm */


} rf_power_level_index_e;



/**
 *  @brief  Define RF mode.
 */
typedef enum {
	RF_MODE_BLE_2M 		     =    BIT(0),		/**< ble 2m mode */
	RF_MODE_BLE_1M 		     =    BIT(1),		/**< ble 1M mode */
    RF_MODE_BLE_1M_NO_PN     =    BIT(2),		/**< ble 1M close pn mode */
	RF_MODE_ZIGBEE_250K      =    BIT(3),		/**< zigbee 250K mode */
    RF_MODE_LR_S2_500K       =    BIT(4),		/**< ble 500K mode */
    RF_MODE_LR_S8_125K       =    BIT(5),		/**< ble 125K mode */
    RF_MODE_PRIVATE_250K     =    BIT(6),		/**< private 250K mode */
    RF_MODE_PRIVATE_500K     =    BIT(7),		/**< private 500K mode */
    RF_MODE_PRIVATE_1M       =    BIT(8),		/**< private 1M mode */
    RF_MODE_PRIVATE_2M       =    BIT(9),		/**< private 2M mode */
    RF_MODE_BLE_2M_NO_PN     =    BIT(11),	    /**< ble 2M close pn mode */
	RF_MODE_PRI_GENERIC_250K =    BIT(17),	    /**< private generic 250K mode */
	RF_MODE_PRI_GENERIC_500K =    BIT(18),	    /**< private generic 500K mode */
	RF_MODE_PRI_GENERIC_1M   =    BIT(19),	    /**< private generic 1M mode */
	RF_MODE_PRI_GENERIC_2M   =    BIT(20),	    /**< private generic 2M mode */
#if(0)
	RF_MODE_ANT     	   =    BIT(10),	/**< ant mode */
    RF_MODE_HYBEE_1M   	   =    BIT(12),	/**< hybee 1M mode */
    RF_MODE_HYBEE_2M   	   =    BIT(13),	/**< hybee 2M mode */
    RF_MODE_HYBEE_500K     =    BIT(14),	/**< hybee 500K mode */
	RF_MODE_LR_S2_500K_NEW =    BIT(15),	/**< ble 500K mode new */
	RF_MODE_LR_S8_125K_NEW =    BIT(16),	/**< ble 125K mode new */
	RF_MODE_LOW_RATE_20K   =    BIT(19),    /**< low rate 20K mode */
	RF_MODE_LOW_RATE_25K   =    BIT(20),    /**< low rate 25K mode */
	RF_MODE_LOW_RATE_100K  =    BIT(21),    /**< low rate 100K mode */
	RF_MODE_HYBEE_1M_OLD   =    BIT(22),    /**< hybee 1M mode old*/
	RF_MODE_HYBEE_2M_OLD   =    BIT(23),    /**< hybee 2M mode old*/
	RF_MODE_HYBEE_500K_NEW =    BIT(24),    /**< hybee 500k mode new*/
	RF_MODE_HYBEE_1M_NEW   =    BIT(25),    /**< hybee 1M mode new*/
	RF_MODE_HYBEE_2M_NEW   =    BIT(26),    /**< hybee 2M mode new*/
	RF_MODE_HYBEE_500K_2BYTE_SFD    = BIT(27),     /**< hybee 500K 2byte sfd mode*/
	RF_MODE_HYBEE_1M_2BYTE_SFD      = BIT(28),     /**< hybee 1M 2byte sfd mode*/
	RF_MODE_HYBEE_2M_2BYTE_SFD      = BIT(29),     /**< hybee 2M 2byte sfd mode*/
	RF_MODE_HYBEE_2M_2BYTE_SFD_NEW  = BIT(30),     /**< hybee 2M 2byte sfd mode new*/
	RF_MODE_HR_2M                   = BIT(31),     /**< hr 2M mode*/
#endif

} rf_mode_e;

/**
 *  @brief  Define RF channel.
 */
typedef enum {
	 RF_CHANNEL_0   =    BIT(0),	/**< RF channel 0 */
	 RF_CHANNEL_1   =    BIT(1),	/**< RF channel 1 */
	 RF_CHANNEL_2   =    BIT(2),	/**< RF channel 2 */
	 RF_CHANNEL_3   =    BIT(3),	/**< RF channel 3 */
	 RF_CHANNEL_4   =    BIT(4),	/**< RF channel 4 */
	 RF_CHANNEL_5   =    BIT(5),	/**< RF channel 5 */
	 /**When using PTX,PRX + MultiCeiver function, chn6,7 is not available.**/
	 RF_CHANNEL_6   =    BIT(6),	/**< RF channel 6 */
	 RF_CHANNEL_7   =    BIT(7),	/**< RF channel 7 */
	 RF_CHANNEL_NONE =   0x00,		/**< none RF channel*/
	 RF_CHANNEL_ALL =    0xff,		/**< all RF channel */
} rf_channel_e;


/**
 * @brief define rf bb timer clock tick per us/ms/s.
 */
enum{
	BB_TIMER_TICK_1US		= 8,
	BB_TIMER_TICK_1MS		= 8000,
	BB_TIMER_TICK_1S		= 8000000,
};

/**********************************************************************************************************************
 *                                         RF global constants                                                        *
 *********************************************************************************************************************/
extern _attribute_data_retention_ volatile rf_power_level_e rf_power_Level_list[60]; //BLE SDK use: volatile to put into RAM
extern rf_mode_e   g_rfmode;
extern rf_crc_config_t rf_crc_config[3];

/**********************************************************************************************************************
 *                                         RF function declaration                                                    *
 *********************************************************************************************************************/

/**
 * @brief     This function performs to get rf baseband timer tick.
 * @return    rf baseband timer tick value.
 */
static inline unsigned int rf_bb_timer_get_tick(void)
{
	return reg_bb_timer_tick;
}

/**
 * @brief   	This function serves to judge the statue of  RF receive.
 * @return  	-#0:idle
 * 				-#1:rx_busy
 */
static inline unsigned char rf_receiving_flag(void)
{
	//if the value of [2:0] of the reg_0x170040 isn't 0 , it means that the RF is in the receiving packet phase.(confirmed by junwen).
	return ((read_reg8(0x170040)&0x07) > 1);
}

/**
 * @brief	  	This function serves to set the which irq enable.
 * @param[in]	mask 	- Options that need to be enabled.
 * @return	 	Yes: 1, NO: 0.
 */
static inline void rf_set_irq_mask(rf_irq_e mask)
{
	BM_SET(reg_rf_irq_mask,mask);
	BM_SET(reg_rf_ll_irq_mask_h ,(mask&0xff0000)>>16);
	BM_SET(reg_rf_ll_cmd,(mask&0x5000000)>>20);
	BM_SET(reg_rf_ll_irq_mask_h1,(mask&0x2000000)>>24);
}


/**
 * @brief	  	This function serves to clear the TX/RX irq mask.
 * @param[in]   mask 	- RX/TX irq value.
 * @return	 	none.
 */
static inline void rf_clr_irq_mask(rf_irq_e mask)
{
	BM_CLR(reg_rf_irq_mask ,mask);
	BM_CLR(reg_rf_ll_irq_mask_h ,(mask&0xff0000)>>16);
	BM_CLR(reg_rf_ll_cmd,(mask&0x5000000)>>20);
	BM_CLR(reg_rf_ll_irq_mask_h1,(mask&0x2000000)>>24);
}


/**
 *	@brief      This function serves to judge whether it is in a certain state.
 *	@param[in]  mask      - RX/TX irq status.
 * @retval      non-zero      -  the interrupt occurred.
 * @retval      zero  -  the interrupt did not occur.
 */
static inline unsigned int rf_get_irq_status(rf_irq_e status)
{
	return ((unsigned int )(BM_IS_SET(reg_rf_irq_status,status)| BM_IS_SET((reg_rf_irq_status_h<<16),status)|BM_IS_SET((reg_rf_irq_status_h1<<24),status) ));
}


/**
 *@brief	This function serves to clear the Tx/Rx finish flag bit.
 *			After all packet data are sent, corresponding Tx finish flag bit
 *			will be set as 1.By reading this flag bit, it can check whether
 *			packet transmission is finished. After the check, it is needed to
 *			manually clear this flag bit so as to avoid misjudgment.
 *@return	none.
 */
static inline void rf_clr_irq_status(rf_irq_e status)
{
	 reg_rf_irq_status = status;
	 reg_rf_irq_status_h = (status&0xff0000)>>16;
	 reg_rf_irq_status_h1 = (status&0x7000000)>>24;
}

/**
 * @brief   	This function serves to set RF access code.
 * @param[in]   acc   - the value of access code.
 * @return  	none.
 */
static inline void rf_access_code_comm (unsigned int acc)
{
	reg_rf_access_code = acc;
	//The following two lines of code are for trigger access code in S2,S8 mode.It has no effect on other modes.
	reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;
	write_reg8(0x170425,read_reg8(0x170425)|0x01);
}


/**
 * @brief		this function is to enable/disable each access_code channel for
 *				RF Rx terminal.
 * @param[in]	pipe  	- Bit0~bit7 correspond to channel 0~7, respectively.
 *					  	- #0:Disable.
 *					  	- #1:Enable.
 *						  If "enable" is set as 0xff (i.e. 11111111),
 *						  all access_code channels (0~7) are enabled.
 * @return	 	none
 */
static inline void rf_rx_acc_code_pipe_en(rf_channel_e pipe)
{
	reg_rf_modem_rx_ctrl_1 = pipe; //rx_access_code_chn_en
}


/**
 * @brief		this function is to select access_code channel for RF tx terminal.
 * @param[in]	pipe  	- tx access code channel,The pipe range is from 0 to 7
 *						  And only 1 channel can be selected every time.
 *						  If "enable" is set as 0x7 (i.e. 0111),the access_code channel (7) is enabled.
 * @return	 	none
 */
static inline void rf_tx_acc_code_pipe_en(rf_channel_e pipe)
{
    write_reg8(0x170215, ((read_reg8(0x170215)&0xf8) | pipe)|BIT(4)); //Tx_Channel_man[2:0]
}


/**
 * @brief 	  This function serves to reset RF Tx/Rx mode.
 * @return 	  none.
 */
static inline void rf_set_tx_rx_off(void)
{
	write_reg8 (0x80170216, 0x29);
	write_reg8 (0x80170028, 0x80);	// rx disable
	write_reg8 (0x80170202, 0x45);	// reset tx/rx state machine
}


/**
 * @brief    This function serves to turn off RF auto mode.
 * @return   none.
 * @note	 Attention: When forcibly stopping the state machine through this interface, it must be ensured
 * 			 that rx is not in the process of receiving packets.Otherwise, an error may be caused.To determine
 * 			 whether the packet is being received, you can use the function rf_receiving_flag.
 */
static inline void rf_set_tx_rx_off_auto_mode(void)
{
	reg_rf_ll_cmd = FLD_RF_R_CMD_TRIG;
}

/**
 * @brief		This function serve to rx dma fifo size.
 * @param[in]	fifo_byte_size - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return		none
 */
static inline void rf_set_rx_dma_fifo_size(unsigned short fifo_byte_size)
{
	reg_rf_bb_rx_size = fifo_byte_size&0xff;
	reg_rf_bb_rx_size_h = fifo_byte_size>>8;
}
/**
 * @brief		This function serve to set rx dma wptr.
 * @param[in]	fifo_num	-This parameter is used to set the mask value for the number of enabled FIFOs. The value of the mask must (0x00,0x01,0x03,0x07,0x0f,0x1f).
 * 						 	 The number of FIFOs enabled is the value of wptr_mask plus 1.(0x01,0x02,0x04,0x08,0x10,0x20)
 * @return 		none
 */
static inline void rf_set_rx_dma_fifo_num(unsigned char fifo_num)
{
	reg_rf_rx_wptr_mask = fifo_num;
}


/**
 * @brief	  	This function serves to DMA rxFIFO address
 *	            The function apply to the configuration of one rxFiFO when receiving packets,
 *	            In this case,the rxFiFo address can be changed every time a packet is received
 *	            Before setting, call the function "rf_set_rx_dma" to clear DMA fifo mask value(set 0)
 * @param[in]	rx_addr   - The address store receive packet.
 * @return	 	none
 * @note		rx_addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
static inline void rf_set_rx_buffer(unsigned char *rx_addr)
{
	rx_addr += 4;
	rf_dma_set_dst_address(RF_RX_DMA,(unsigned int)(rx_addr));
}
/**
 * @brief		This function serve to set the number of tx dma fifo.
 * @param[in]	fifo_dep - the number of dma fifo is 2 to the power of fifo_dep.
 * @return		none
 */
static inline void rf_set_tx_dma_fifo_num(unsigned char fifo_num)
{
	reg_rf_bb_tx_chn_dep = fifo_num;//tx_chn_dep = 2^2 =4 (have 4 fifo)
}

/**
 * @brief		This function serve to set the number of tx dma fifo.
 * @param[in]	fifo_byte_size - the size of each dma fifo.
 * @return		none
 */
static inline void rf_set_tx_dma_fifo_size(unsigned short fifo_byte_size)
{
	reg_rf_bb_tx_size	= fifo_byte_size&0xff;//tx_idx_addr = {tx_chn_adr*bb_tx_size,4'b0}// in this setting the max data in one dma buffer is 0x20<<4.And the The product of fifo_dep and bytesize cannot exceed 0xfff.
	reg_rf_bb_tx_size_h = fifo_byte_size>>8;
}
/**
 * @brief   This function serves to set RF tx settle time.
 * @tx_stl_us  tx settle time,the unit is us.The max value of this param is 0xfff;The default settling time value is 150us.
 * 			   The typical value is 113us (tx_settle time should not be less than this value).
 * @return  none.
 * @note	   Attention:It is not necessary to call this function to adjust the settling time in the normal sending state.
 */
static inline void rf_set_tx_settle_time(unsigned short tx_stl_us )
{
	tx_stl_us &= 0x0fff;
	write_reg16(0x170204, (read_reg16(0x170204)& 0xf000) |(tx_stl_us - 1));
}
/**
 * @brief   This function serves to set RF tx settle time and rx settle time.
 * @rx_stl_us  rx settle time,the unit is us.The max value of this param is 0xfff;The default settling time value is 150us.
 * 			   The typical value is 85us (rx_settle time should not be less than this value).
 * @return  none.
 * @note	   Attention:It is not necessary to call this function to adjust the settling time in the normal packet receiving state.
 */
static inline void rf_set_rx_settle_time( unsigned short rx_stl_us )
{
	 rx_stl_us &= 0x0fff;
	 write_reg16(0x17020c, (read_reg16(0x17020c)& 0xf000) |(rx_stl_us - 1));
}

/**
 * @brief  	  	This function serves to set CRC length for RF.
 * @param[in]  	len - The CRC length to be set for RF,the range is 0~4byte.
 * @return 		none.
 * @note	    Attention:when CRC length is 0,TX need to disable CRC.
 */
static inline void rf_set_crc_len(unsigned char len)
{
	reg_rf_crc_config2 = ((reg_rf_crc_config2&(~FLD_RF_CRC_LENGTH))|len);
}

/**
 * @brief  	  	This function serves to set CRC value for RF.
 * @param[in]  	init_value - The CRC init value to be set for RF.
 * @return 		none.
 */
static inline void rf_set_crc_init_value(unsigned int init_value)
{
	reg_rf_crc_init = init_value;
}

/**
 * @brief  	  	This function serves to set CRC polynomial for RF.
 * @param[in]  	poly - The CRC polynomial to be set for RF.
 *              For example, to set CRC polynomial(BLE CRC-24):
 *              crc_poly = x^24 + x^10 + x^9 + x^6 + x^4 + x^3 + x + 1
 *              the crc_poly value would be 0x65b.
 * @return 		none.
 */
static inline void rf_set_crc_poly(unsigned int poly)
{
	reg_rf_crc_poly = poly;
}

/**
 * @brief  	  	This function serves to set CRC start byte for RF.
 * @param[in]  	byte_pos - CRC start byte position, 0 is the first byte
 *           	ble:The starting position of the PDU header.
 *           	private:The starting position of the data packet
 *           	zigbee:The starting position is the first byte after SHR.
 * @return 		none.
 */
static inline void rf_set_crc_start_cal_byte_pos(unsigned char byte_pos)
{
	reg_rf_crc_config1 =(reg_rf_crc_config1&(~FLD_RF_CRC_START_BYTE))|byte_pos;
}

/**
 * @brief  	  	This function serves to set the XOR value of the CRC result for RF.
 * @param[in]  	xor_value - XOR mask for CRC result (for no mask, should be 0)
 * @return 		none.
 */
static inline void rf_set_crc_xor_out(unsigned int xor_value)
{
	reg_rf_crc_xorout = xor_value;
}

/**
 * @brief  	  	This function serves to set the CRC output byte order for RF.
 * @param[in]  	order - CRC output byte order (1:MSByte first,0:LSByte first)
 * @return 		none.
 */
static inline void rf_set_crc_byte_order(unsigned char order)
{
	reg_rf_crc_config2 = (reg_rf_crc_config2&0xf7)|(order<<3);
}

/**
 * @brief  	  	This function serves to set crc for RF.
 * @param[in]   config - crc config address pointer
 * @return 		none.
 */
static inline void rf_set_crc_config(const rf_crc_config_t *config)
{

	rf_set_crc_init_value(config->init_value);
	rf_set_crc_poly(config->poly);
	rf_set_crc_xor_out(config->xor_out);
	rf_set_crc_byte_order(config->byte_order);
	rf_set_crc_start_cal_byte_pos(config->start_cal_pos);
	rf_set_crc_len(config->len);
}

/**
 * @brief	This function serve to get tx wptr.
 * @param[in]	pipe_id	-	The number of tx fifo.0<= pipe_id <=5.
 * @return		The write pointer of the tx.
 */
static inline unsigned char rf_get_tx_wptr(unsigned char pipe_id)
{
	return reg_rf_dma_tx_wptr(pipe_id);
}

/**
 * @brief	This function serve to update the wptr of tx terminal.
 * @param[in]	pipe_id	-	The number of pipe which need to update wptr.
 * @param[in]	wptr	-	The pointer of write in tx terminal.
 * @return		none
 */
static inline void rf_set_tx_wptr(unsigned char pipe_id,unsigned char wptr)
{
	reg_rf_dma_tx_wptr(pipe_id) = wptr;
}


/**
 * @brief	This function serve to clear the writer pointer of tx terminal.
 * @param[in]	pipe_id	-	The number of tx DMA.0<= pipe_id <=5.
 * @return	none.
 */
static inline void rf_clr_tx_wptr(unsigned char pipe_id)
{
	reg_rf_dma_tx_wptr(pipe_id) = 0;
}

/**
 * @brief	This function serve to get ptx rptr.
 * @param[in]	pipe_id	-The number of tx pipe.0<= pipe_id <=5.
 * @return		The read pointer of the tx.
 */
static inline unsigned char rf_get_tx_rptr(unsigned char pipe_id)
{
	return reg_rf_dma_tx_rptr(pipe_id);
}


/**
 * @brief	This function serve to clear read pointer of tx terminal.
 * @param[in]	pipe_id	-	The number of tx DMA.0<= pipe_id <=5.
 * @return	none.
 */
static inline void rf_clr_tx_rptr(unsigned char pipe_id)
{
	reg_rf_dma_tx_rptr(pipe_id) = 0x80;
}

/**
 * @brief 	This function serve to get the pointer of read in rx terminal.
 * @return	wptr	-	The pointer of rx_rptr.
 */
static inline unsigned char rf_get_rx_rptr(void)
{
	return reg_rf_dma_rx_rptr;
}

/**
 * @brief	This function serve to clear read pointer of rx terminal.
 * @return	none.
 */
static inline void rf_clr_rx_rptr(void)
{
	write_reg8(0x1708f5, 0x80); //clear rptr
}


/**
 * @brief 	This function serve to get the pointer of write in rx terminal.
 * @return	wptr	-	The pointer of rx_wptr.
 */
static inline unsigned char rf_get_rx_wptr(void)
{
	return reg_rf_dma_rx_wptr;
}

/**
 * @brief      This function serves to initiate information of RF.
 * @return	   none.
 */
void rf_mode_init(void);

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] none
 * @return	  none.
 */
void rf_set_tx_dma_config(void);

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] fifo_depth 		- tx chn deep,fifo_depth range: 0~5,Number of fifo=2^fifo_depth.
 * @param[in] fifo_byte_size 	- The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return	  none.
 */
void rf_set_tx_dma(unsigned char fifo_depth,unsigned short fifo_byte_size);

/**
 * @brief      This function serves to rx dma setting.
 * @param[in]  buff - This parameter is the first address of the received data buffer, which must be 4 bytes aligned, otherwise the program will enter an exception.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *			   -# you need to leave 4bytes of space for the length information.
 *			   -# dma is transmitted in accordance with 4bytes, so the length of the buffer needs to be a multiple of 4. Otherwise, there may be an out-of-bounds problem
 *			   For example, the actual received data length is 5bytes, the minimum value of the actual buffer size that the user needs to set is 12bytes, and the calculation of 12bytes is explained as follows::
 *			   4bytes (length information) + 5bytes (data) + 3bytes (the number of additional bytes to prevent out-of-bounds)
 * @param[in]  wptr_mask  	   - This parameter is used to set the mask value for the number of enabled FIFOs. The value of the mask must (0x00,0x01,0x03,0x07,0x0f,0x1f).
 * 								 The number of FIFOs enabled is the value of wptr_mask plus 1.(0x01,0x02,0x04,0x08,0x10,0x20)
 * @param[in]  fifo_byte_size  - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return     none.
 */
void rf_set_rx_dma(unsigned char *buff,unsigned char wptr_mask,unsigned short fifo_byte_size);

/**
 * @brief		This function serve to rx dma config
 * @param[in]	none
 * @return		none
 */
void rf_set_rx_dma_config(void);

/**
 * @brief     This function serves to trigger srx on.
 * @param[in] tick  - Trigger rx receive packet after tick delay.
 * @return	  none.
 */
void rf_start_srx(unsigned int tick);


/**
 * @brief	  	This function serves to get rssi.
 * @return	 	rssi value.
 */
signed char rf_get_rssi(void);

/**
 * @brief  	 	This function serves to set RF Tx mode.
 * @return  	none.
 */
void rf_set_txmode(void);


/**
 * @brief	  	This function serves to set RF Tx packet address to DMA src_addr.
 * @param[in]	addr   - The packet address which to send.
 * @return	 	none.
 * @note		addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
_attribute_ram_code_sec_ void rf_tx_pkt(void* addr);


/**
 * @brief   	This function serves to set rf channel for all mode.The actual channel set by this function is 2400+chn.
 * @param[in]   chn   - That you want to set the channel as 2400+chn.
 * @return  	none.
 */
_attribute_ram_code_sec_noinline_ void rf_set_chn(signed char chn);


/**
 * @brief   	This function serves to disable pn of ble mode.
 * @return  	none.
 */
void rf_pn_disable(void);


/**
 * @brief   	This function serves to get the right fifo packet.
 * @param[in]   fifo_num   - The number of fifo set in dma.
 * @param[in]   fifo_dep   - deepth of each fifo set in dma.
 * @param[in]   addr       - address of rx packet.
 * @return  	the next rx_packet address.
 * @note		addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
unsigned char* rf_get_rx_packet_addr(int fifo_num,int fifo_dep,void* addr);


/**
 * @brief   	This function serves to set RF power level.
 * @param[in]   level 	 - The power level to set.
 * @return 		none.
 */
void rf_set_power_level (rf_power_level_e level);


/**
 * @brief   	This function serves to set RF power through select the level index.
 * @param[in]   idx 	 - The index of power level which you want to set.
 * @return  	none.
 */
void rf_set_power_level_index(rf_power_level_index_e idx);

/**
 * @brief	  	This function serves to get RF status.
 * @return	 	RF Rx/Tx status.
 */
rf_status_e rf_get_trx_state(void);

/**
 * @brief	  	This function serves to judge RF Tx/Rx state.
 * @param[in]	rf_status   - Tx/Rx status.
 * @param[in]	rf_channel  - This param serve to set frequency channel(2400+rf_channel) .
 * @return	 	Whether the setting is successful(-1:failed;else success).
 */
int rf_set_trx_state(rf_status_e rf_status, signed char rf_channel);

/**
 * @brief	  	This function serves to update the value of internal cap.
 * @param[in]  	value   - The value of internal cap which you want to set.
 * @return	 	none.
 */
void rf_update_internal_cap(unsigned char value);

/**
 * @brief	    This function serve to change the length of preamble.
 * @param[in]	len		-The value of preamble length.Set the register bit<0>~bit<4>.
 * @return		none
 */
void rf_set_preamble_len(unsigned char len);

/**
 * @brief	    This function serve to set the length of access code.
 * @param[in]	byte_len	-	The value of access code length,the range is 3~5byte.
 * @return		none
 */
void rf_set_access_code_len(unsigned char byte_len);

/**
 * @brief       This function serves to set RF rx timeout.
 * @param[in]	timeout_us	-	rx_timeout after timeout_us us,The maximum of this param is 0xfff.
 * @return      none.
 */
static inline void rf_set_rx_timeout(unsigned short timeout_us)
{
	reg_rf_rx_timeout = timeout_us - 1;
}

/**
 * @brief   This function serves to disable RF rx timeout.
 * @return  none.
 */
static inline void rf_dis_rx_timeout(void)
{
	reg_rf_ll_ctrl_1 &= ~(FLD_RF_RX_TIMEOUT_EN);
}

/**
 * @brief     	This function serves to RF trigger stx
 * @param[in] 	addr  	- DMA tx buffer.
 * @param[in] 	tick  	- Send after tick delay.
 * @return	   	none.
 * @note		addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
_attribute_ram_code_sec_noinline_ void rf_start_stx(void* addr, unsigned int tick);


/**
 * @brief     	This function serves to RF trigger stx2rx
 * @param[in] 	addr  	- DMA tx buffer.
 * @param[in] 	tick  	- Send after tick delay.
 * @return	    none.
 * @note		addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
_attribute_ram_code_sec_noinline_ void rf_start_stx2rx  (void* addr, unsigned int tick);

/**
 * @brief   	This function serves to set RF Rx manual on.
 * @return  	none.
 */
_attribute_ram_code_sec_noinline_ void rf_set_rxmode(void);

/**
 * @brief     	This function serves to RF trigger srx2rx.
 * @param[in] 	addr  - DMA tx buffer.
 * @param[in] 	tick  - Trigger rx receive packet after tick delay.
 * @return	    none.
 * @note		addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
_attribute_ram_code_sec_noinline_ void rf_start_srx2tx  (void* addr, unsigned int tick);

/**
 * @brief	  	This function is used to judge whether there is a CRC error in the received packet through hardware.
 * 				For the same packet, the value of this bit is consistent with the CRC flag bit in the packet.
 * @param[in]	none.
 * @return	 	none.
 */
unsigned char rf_get_crc_err(void);

/**
 * @brief  	   This function serves to set the max length of rx packet.Use byte_len to limit what DMA
 * 			   moves out will not exceed the buffer size we define.And old chip do this through dma size.
 * @param[in]  byte_len  - The longest of rx packet.
 * @return     none.
 */
static inline void rf_set_rx_maxlen(unsigned int byte_len)
{
	reg_rf_rxtmaxlen0 = byte_len&0xff;
	reg_rf_rxtmaxlen1 = (byte_len>>8)&0xff;
}

/**********************************************************************************************************************
 *  Fast settle related interfaces
 *  Attention:
 *  (1)This part of the function is only for the internal use of the driver, not open to customers to use,
 *  we will rewrite this part, and provide demo
 *  (2)When using B95 fastsettle, it should be noted that different settle times correspond to different calibration modules being turned off,
 *     so you need to manually configure the calibration values of these calibration modules before using fastsettle.
 *  (3)Calibration modules that require manual setting of calibration values:
 *     RX_SETTLE_TIME_11US: ldo trim; rx_fcal; dcoc;
 *     RX_SETTLE_TIME_37US: ldo trim; dcoc;
 *     RX_SETTLE_TIME_77US: ldo trim;
 *
 *     TX_SETTLE_TIME_25US: ldo trim; tx_fcal; hpmc;
 *     TX_SETTLE_TIME_51US: ldo trim; hpmc;
 *     TX_SETTLE_TIME_104US: ldo trim;
 *
 *********************************************************************************************************************/

/**
 *	@brief	  	This function serve to adjust rx settle timing sequence.
 *	@param[in]	rx_settle_us  	After adjusting the timing sequence, the time required for rx to settle.
 *	@return	 	none
 *	@note		RX_SETTLE_TIME_11US - disable Bandgap, rx_ldo_trim, PD_settle, rx_fcal,rx_rccal, rx_dcoc calibration,reduce 74us of rx settle time.
	                                  Receive for a period of time and then do a normal calibration
*/
signed char rf_fast_settle_config(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us);


/**
 *	@brief	  	This function serve to enable the tx timing sequence adjusted.
 *	@param[in]	none
 *	@return	 	none
*/
void rf_tx_fast_settle_en(void);

/**
 *	@brief	  	This function serve to disable the tx timing sequence adjusted.
 *	@param[in]	none
 *	@return	 	none
*/
void rf_tx_fast_settle_dis(void);

/**
 *	@brief	  	This function serve to enable the rx timing sequence adjusted.
 *	@param[in]	none
 *	@return	 	none
*/
void rf_rx_fast_settle_en(void);

/**
 *	@brief	  	This function serve to disable the rx timing sequence adjusted.
 *	@param[in]	none
 *	@return	 	none
*/
void rf_rx_fast_settle_dis(void);

/**
 *  @brief		This function is mainly used to get LDO Calibration-related values.
 *	@param[in]	ldo_trim   - ldo trim calibration value address pointer
 *	@return	 	none
*/
void rf_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal);

/**
 *  @brief		This function is mainly used to set LDO Calibration-related values.
 *	@param[in]  ldo_trim   - ldo trim Calibration-related values.
 *	@return	 	none
*/
void rf_set_rccal_cal_val(rf_rccal_cal_t rccal_cal);

/**
 *  @brief		This function is mainly used to get hpmc Calibration-related values.
 *	@param[in]	none
 *	@return	 	none
 *	@param[in]  value  - hpmc Calibration-related values.
 *	@return	 	none
*/
static inline void rf_set_tx_wait_time(unsigned short tx_wait_us )
{
	tx_wait_us &= 0x0fff;
	write_reg16(0x17020e, (read_reg16(0x17020e)& 0xf000) |(tx_wait_us - 1));
}

/**
 *  @brief		This function is mainly used to get LDO Calibration-related values.
 *	@param[in]	dcoc_cal   - dcoc calibration value address pointer
 *	@return	 	none
 *	@param[in]  dcoc_cal    - dcoc Calibration-related values.
 *	@return	 	none
*/
static inline void rf_set_rx_wait_time( unsigned short rx_wait_us )
{
	rx_wait_us &= 0x0fff;
	 write_reg16(0x170206, (read_reg16(0x170206)& 0xf000) |(rx_wait_us - 1));
}

/**
 *  @brief		This function is mainly used to get rccal Calibration-related values.
 *	@param[in]	rccal_cal  - rccal calibration value address pointer
 *	@return	 	none
*/
void rf_set_acc_match_threshold(unsigned char bit_len);

/**
 *  @brief		This function is mainly used to set rccal Calibration-related values.
 *	@param[in]	rccal_cal    - rccal Calibration-related values.
 *	@return	 	none
 *             It is used to clear RF related state machines, IRQ states, and digital internal logic states.
*/
void rf_clr_dig_logic_state(void);

/**
 *  @brief		This function is mainly used to get fcal Calibration-related values.
 *	@return	 	fcal dcap value
 *	@param[in]	fcal_tx_group   - tx fcal Address pointer for calibration value writing
 *	@param[in]	fcal_rx_group   - rx fcal Address pointer for calibration value writing
 *	@return	 	none
*/
void rf_reset_register_value(void);


/**
 * @brief      This function serves to set RF tx settle time.
 * @param[in]  tx_wait_us  tx settle time,the unit is us.The max value of this param is 0xfff; The default settling time value is 150us.
 * 			   The typical value is 113us (tx_settle time should not be less than this value).
 * @return     none.
 * @note	   Attention:It is not necessary to call this function to adjust the settling time in the normal sending state.
 */
void rf_tx_fast_settle_update_cal_val(rf_tx_fast_settle_time_e tx_settle_time,unsigned char chn);

/**
 * @brief      This function serves to set RF tx settle time and rx settle time.
 * @@param[in] rx_stl_us  rx settle time,the unit is us.The max value of this param is 0xfff;The default settling time value is 150us.
 * 			   The typical value is 85us (rx_settle time should not be less than this value).
 * @return     none.
 * @note	   Attention:It is not necessary to call this function to adjust the settling time in the normal packet receiving state.
 */
void rf_rx_fast_settle_update_cal_val(rf_rx_fast_settle_time_e rx_settle_time,unsigned char chn);

/**
 * @brief   This function do radio baseband reset
 * @param   none
 * @return  none
 * @note    Maintain RF register configuration; Reset RF status, buf of modem, and system_timer_bb.
 */
void rf_baseband_reset(void);


/**
 * @brief   This function reset baseband DMA, the DMA logic and registers will be reset.
 * @param   none
 * @return  none
 */
void rf_reset_dma(void);


/**
 * @brief   This function serves to restore baseband to default values.
 * @param   none
 * @return  none
 * @note    Restore all RF related registers to default values.
 */
void rf_radio_reset(void);

#endif
