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
#ifndef BLE_STACK_H_
#define BLE_STACK_H_

#include "stack/ble/ble_config.h"


/**
 *  @brief  Definition for Link Layer Feature Support
 */

#define LL_FEATURE_MASK_LL_ENCRYPTION                        			BIT(0) //core_4.0
#define LL_FEATURE_MASK_CONNECTION_PARA_REQUEST_PROCEDURE  	 			BIT(1) //core_4.1
#define LL_FEATURE_MASK_EXTENDED_REJECT_INDICATION           			BIT(2) //core_4.1
#define LL_FEATURE_MASK_SLAVE_INITIATED_FEATURES_EXCHANGE    			BIT(3) //core_4.1
#define LL_FEATURE_MASK_LE_PING                             	 		BIT(4) //core_4.1
#define LL_FEATURE_MASK_LE_DATA_LENGTH_EXTENSION             			BIT(5) //core_4.2
#define LL_FEATURE_MASK_LL_PRIVACY                           			BIT(6) //core_4.2
#define LL_FEATURE_MASK_EXTENDED_SCANNER_FILTER_POLICIES   	 			BIT(7) //core_4.2

#define LL_FEATURE_MASK_LE_2M_PHY         					 			BIT(8)  //core_5.0
#define LL_FEATURE_MASK_STABLE_MODULATION_INDEX_TX 			 			BIT(9)	//core_5.0
#define LL_FEATURE_MASK_STABLE_MODULATION_INDEX_RX 			 			BIT(10)	//core_5.0
#define LL_FEATURE_MASK_LE_CODED_PHY     					 			BIT(11)	//core_5.0
#define LL_FEATURE_MASK_LE_EXTENDED_ADVERTISING          	 			BIT(12)	//core_5.0
#define LL_FEATURE_MASK_LE_PERIODIC_ADVERTISING     		 			BIT(13)	//core_5.0
#define LL_FEATURE_MASK_CHANNEL_SELECTION_ALGORITHM2         			BIT(14)	//core_5.0
#define LL_FEATURE_MASK_LE_POWER_CLASS_1 					 			BIT(15)	//core_5.0
#define LL_FEATURE_MASK_MIN_USED_OF_USED_CHANNELS   	    	 		BIT(16)	//core_5.0

#define LL_FEATURE_MASK_CONNECTION_CTE_REQUEST  	    				BIT(17)	//core_5.1
#define LL_FEATURE_MASK_CONNECTION_CTE_RESPONSE  	    	 			BIT(18)	//core_5.1
#define LL_FEATURE_MASK_CONNECTIONLESS_CTE_TRANSMITTER  	 			BIT(19)	//core_5.1
#define LL_FEATURE_MASK_CONNECTIONLESS_CTE_RECEIVER  	     			BIT(20)	//core_5.1
#define LL_FEATURE_MASK_ANTENNA_SWITCHING_DURING_CTE_TRANSMISSION  	 	BIT(21)	//core_5.1
#define LL_FEATURE_MASK_ANTENNA_SWITCHING_DURING_CTE_RECEPTION  	 	BIT(22)	//core_5.1
#define LL_FEATURE_MASK_RECEIVING_CONSTANT_TONE_EXTENSIONS  	 		BIT(23)	//core_5.1
#define LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_SENDOR  	 	BIT(24)	//core_5.1
#define LL_FEATURE_MASK_PERIODIC_ADVERTISING_SYNC_TRANSFER_RECIPIENT  	BIT(25)	//core_5.1
#define LL_FEATURE_MASK_SLEEP_CLOCK_ACCURACY_UPDATES		   	 	    BIT(26)	//core_5.1
#define LL_FEATURE_MASK_REMOTE_PUBLIC_KEY_VALIDATION  					BIT(27)	//core_5.1

#define LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_MASTER  			BIT(28)	//core_5.2
#define LL_FEATURE_MASK_CONNECTED_ISOCHRONOUS_STREAM_SLAVE  			BIT(29)	//core_5.2
#define LL_FEATURE_MASK_ISOCHRONOUS_BROADCASTER  						BIT(30)	//core_5.2
#define LL_FEATURE_MASK_SYNCHRONIZED_RECEIVER	  						BIT(31)	//core_5.2
#define LL_FEATURE_MASK_ISOCHRONOUS_CHANNELS	  						BIT(0)	//core_5.2
#define LL_FEATURE_MASK_LE_POWER_CTRL_REQUEST	  						BIT(1)	//core_5.2
#define LL_FEATURE_MASK_LE_POWER_CHANGE_INDICATION	  					BIT(2)	//core_5.2
#define LL_FEATURE_MASK_LE_PATH_LOSS_MONITORING	  						BIT(3)	//core_5.2




/////////////////////////////////////////////////////////////////////////////

#define         VENDOR_ID                       0x0211
#define         VENDOR_ID_HI_B                  U16_HI(VENDOR_ID)
#define         VENDOR_ID_LO_B                  U16_LO(VENDOR_ID)

#define			BLUETOOTH_VER_4_0				6
#define			BLUETOOTH_VER_4_1				7
#define			BLUETOOTH_VER_4_2				8
#define			BLUETOOTH_VER_5_0				9
#define			BLUETOOTH_VER_5_1				10
#define			BLUETOOTH_VER_5_2				11

#ifndef 		BLUETOOTH_VER
#define			BLUETOOTH_VER					BLUETOOTH_VER_5_0
#endif


#if (BLUETOOTH_VER == BLUETOOTH_VER_4_2)
	#define			BLUETOOTH_VER_SUBVER			0x22BB
#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_0)
	#define			BLUETOOTH_VER_SUBVER			0x1C1C
#else
	#define			BLUETOOTH_VER_SUBVER			0x4103
#endif




#if (BLUETOOTH_VER == BLUETOOTH_VER_4_0)
	#define LL_FEATURE_ENABLE_LE_ENCRYPTION								1
	#define LL_CMD_MAX						   							LL_REJECT_IND

#elif (BLUETOOTH_VER == BLUETOOTH_VER_4_1)
	#define LL_FEATURE_ENABLE_LE_ENCRYPTION								1
	#define	LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION				1
	#define	LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE			1
	#define	LL_FEATURE_ENABLE_LE_PING									1

	#define LL_CMD_MAX						   							LL_PING_RSP

#elif (BLUETOOTH_VER == BLUETOOTH_VER_4_2)

	#define LL_FEATURE_ENABLE_LE_ENCRYPTION								1
	#define	LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION				1
	#define	LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE			1
	#define	LL_FEATURE_ENABLE_LE_PING									1
	#define	LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION					LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION
	#define	LL_FEATURE_ENABLE_LL_PRIVACY								LL_FEATURE_SUPPORT_LL_PRIVACY
	#define LL_CMD_MAX						  							LL_LENGTH_RSP

#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_0)

	#define LL_FEATURE_ENABLE_LE_ENCRYPTION								1
	#define	LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION				1
	#define	LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE			1
	#define	LL_FEATURE_ENABLE_LE_PING									1
	#define	LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION					LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION
	#define	LL_FEATURE_ENABLE_LL_PRIVACY								LL_FEATURE_SUPPORT_LL_PRIVACY
	#define	LL_FEATURE_ENABLE_LE_2M_PHY									LL_FEATURE_SUPPORT_LE_2M_PHY
	#define	LL_FEATURE_ENABLE_LE_CODED_PHY								LL_FEATURE_SUPPORT_LE_CODED_PHY
	#define	LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING					LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING
	#define	LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING					LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING
	#define	LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2				LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2

	#define LL_CMD_MAX						   							LL_MIN_USED_CHN_IND

	#define	LL_FEATURE_ENABLE_LE_EXTENDED_SCAN							LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING  //Vendor define
	#define	LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE						LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE  //Vendor define
	#define	LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING_SYNC				LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC

#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_1)

#elif (BLUETOOTH_VER == BLUETOOTH_VER_5_2)

	#define LL_FEATURE_ENABLE_LE_ENCRYPTION								1
	#define	LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION				1
	#define	LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE			1
	#define	LL_FEATURE_ENABLE_LE_PING									1
	#define	LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION					LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION
	#define	LL_FEATURE_ENABLE_LL_PRIVACY								LL_FEATURE_SUPPORT_LL_PRIVACY
	#define	LL_FEATURE_ENABLE_LE_2M_PHY									LL_FEATURE_SUPPORT_LE_2M_PHY
	#define	LL_FEATURE_ENABLE_LE_CODED_PHY								LL_FEATURE_SUPPORT_LE_CODED_PHY
	#define	LL_FEATURE_EANBLE_LE_AOA_AOD								LL_FEATURE_SUPPORT_LE_AOA_AOD
	#define	LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING					LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING
	#define	LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING					LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING
	#define	LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2				LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2
	#define	LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER		LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER
	#define	LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE		LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
	#define	LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER					LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER
	#define	LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER						LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER
	#define	LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS						LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS
	#define	LL_FEATURE_ENABLE_ISOCHRONOUS_TEST_MODE						LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE


	#define LL_FEATURE_ENABLE_CONNECTED_ISO								(     LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER \
																			| LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE     )

	#define LL_FEATURE_ENABLE_CONNECTIONLESS_ISO  						(	  LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER				\
																			| LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER )

	#define LL_FEATURE_ENABLE_ISO										(     LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER \
																			| LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE \
																			| LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER				\
																			| LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER )

	#define	LL_FEATURE_ENABLE_LE_EXTENDED_SCAN							LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING  //Vendor define
	#define	LL_FEATURE_ENABLE_LE_EXTENDED_INITIATE						LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE  //Vendor define

	#define LL_CMD_MAX						   							LL_POWER_CHANGE_IND

	#define LL_FEATURE_BIT_NUMBER_ISOCHRONOUS_CHANNELS					32

#else


#endif




#ifndef		 LL_FEATURE_ENABLE_LE_ENCRYPTION
#define		 LL_FEATURE_ENABLE_LE_ENCRYPTION							0
#endif

#ifndef		 LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE
#define		 LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE		0
#endif

#ifndef		 LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION
#define		 LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION				0
#endif

#ifndef		 LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE
#define		 LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE		0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_PING
#define		 LL_FEATURE_ENABLE_LE_PING									0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION
#define		 LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION					0
#endif

#ifndef		 LL_FEATURE_ENABLE_LL_PRIVACY
#define		 LL_FEATURE_ENABLE_LL_PRIVACY								0
#endif

#ifndef		 LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES
#define		 LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES			0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_2M_PHY
#define		 LL_FEATURE_ENABLE_LE_2M_PHY								0
#endif

#ifndef		 LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_TX
#define		 LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_TX				0
#endif

#ifndef		 LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_RX
#define		 LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_RX				0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_CODED_PHY
#define		 LL_FEATURE_ENABLE_LE_CODED_PHY								0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING
#define		 LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING					0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING
#define		 LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING					0
#endif

#ifndef		 LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2
#define		 LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2				0
#endif

#ifndef		 LL_FEATURE_ENABLE_LE_POWER_CLASS_1
#define		 LL_FEATURE_ENABLE_LE_POWER_CLASS_1							0
#endif

#ifndef		 LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS
#define		 LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS				0
#endif


#ifndef		 LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER
#define		 LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER		0
#endif

#ifndef		 LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
#define		 LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE		0
#endif

#ifndef		 LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER
#define		 LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER					0
#endif

#ifndef		 LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER
#define		 LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER					0
#endif

#ifndef		 LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS
#define		 LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS						0
#endif





//BIT<0:31>
// feature below is conFiged by application layer or HCI command
// <8>  : LL_FEATURE_ENABLE_LE_2M_PHY
// <11> : LL_FEATURE_ENABLE_LE_CODED_PHY
// <12> : LL_FEATURE_ENABLE_LE_EXTENDED_ADVERTISING
// <13> : LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING
// <14> : LL_FEATURE_ENABLE_CHANNEL_SELECTION_ALGORITHM2

// <28> : LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_MASTER
// <29> : LL_FEATURE_ENABLE_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
// <30> : LL_FEATURE_ENABLE_ISOCHRONOUS_BROADCASTER
// <31> : LL_FEATURE_ENABLE_SYNCHRONIZED_RECEIVER

#define LL_FEATURE_MASK_BASE0											(	LL_FEATURE_ENABLE_LE_ENCRYPTION 					<<0		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_CONNECTION_PARA_REQUEST_PROCEDURE <<1		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_EXTENDED_REJECT_INDICATION 	   	<<2		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_SLAVE_INITIATED_FEATURES_EXCHANGE <<3		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_LE_PING 						   	<<4		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_LE_DATA_LENGTH_EXTENSION 		   	<<5		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_LL_PRIVACY 					   	<<6		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_EXTENDED_SCANNER_FILTER_POLICIES  <<7		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_TX 	   	<<9		|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_STABLE_MODULATION_INDEX_RX  	   	<<10	|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_LE_PERIODIC_ADVERTISING 		   	<<13	|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_LE_POWER_CLASS_1 				   	<<15	|  \
		   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	   	LL_FEATURE_ENABLE_MIN_USED_OF_USED_CHANNELS 		<<16 	)


//BIT<32:63>
// feature below is conFiged by application layer or HCI command
// <32> :	LL_FEATURE_ENABLE_ISOCHRONOUS_CHANNELS
#define LL_FEATURE_MASK_BASE1											0



extern u32 LL_FEATURE_MASK_0;
extern u32 LL_FEATURE_MASK_1;



#define LL_FEATURE_BYTE_0												U32_BYTE0(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_1												U32_BYTE1(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_2												U32_BYTE2(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_3												U32_BYTE3(LL_FEATURE_MASK_0)
#define LL_FEATURE_BYTE_4												U32_BYTE0(LL_FEATURE_MASK_1)
#define LL_FEATURE_BYTE_5												U32_BYTE1(LL_FEATURE_MASK_1)
#define LL_FEATURE_BYTE_6												U32_BYTE2(LL_FEATURE_MASK_1)
#define LL_FEATURE_BYTE_7												U32_BYTE3(LL_FEATURE_MASK_1)










/******************************************** Link Layer **************************************************************/
#define 		BLE_T_IFS										150	//unit: uS
#define 		BLE_T_MSS										150	//unit: uS
#define			BLE_T_MAFS										300 //unit: uS


#define			SCA_MASTER_SLAVE_251_500_PPM					0
#define			SCA_MASTER_SLAVE_151_250_PPM					1
#define			SCA_MASTER_SLAVE_101_150_PPM					2
#define			SCA_MASTER_SLAVE_76_100_PPM						3
#define			SCA_MASTER_SLAVE_51_75_PPM						4
#define			SCA_MASTER_SLAVE_31_50_PPM						5
#define			SCA_MASTER_SLAVE_21_30_PPM						6
#define			SCA_MASTER_SLAVE_0_20_PPM						7


/**
 *  @brief  Definition for LL Control PDU Opcode
 */																		// rf_len without MIC
#define					LL_CONNECTION_UPDATE_REQ	0x00							// 12
#define					LL_CHANNEL_MAP_REQ			0x01							//	8
#define					LL_TERMINATE_IND			0x02							//	2

#define					LL_ENC_REQ					0x03	// encryption			// 23
#define					LL_ENC_RSP					0x04	// encryption			// 13
#define					LL_START_ENC_REQ			0x05	// encryption			//	1
#define					LL_START_ENC_RSP			0x06	// encryption			//	1

#define					LL_UNKNOWN_RSP				0x07							//	2
#define 				LL_FEATURE_REQ              0x08							//	9
#define 				LL_FEATURE_RSP              0x09							//	9

#define					LL_PAUSE_ENC_REQ			0x0A	// encryption			//	1
#define					LL_PAUSE_ENC_RSP			0x0B	// encryption			//	1

#define 				LL_VERSION_IND              0x0C							//	6
#define 				LL_REJECT_IND         		0x0D							//	2
#define 				LL_SLAVE_FEATURE_REQ        0x0E	//core_4.1				//	9
#define 				LL_CONNECTION_PARAM_REQ		0x0F	//core_4.1				// 24
#define 				LL_CONNECTION_PARAM_RSP		0x10	//core_4.1				// 24
#define					LL_REJECT_IND_EXT			0x11	//core_4.1				//	3
#define 				LL_PING_REQ					0x12    //core_4.1				//	1
#define					LL_PING_RSP					0x13    //core_4.1				//	1
#define 				LL_LENGTH_REQ				0x14    //core_4.2				//	9
#define					LL_LENGTH_RSP				0x15    //core_4.2				//	9
#define 				LL_PHY_REQ					0x16	//core_5.0				//	3
#define 				LL_PHY_RSP					0x17	//core_5.0				//	3
#define 				LL_PHY_UPDATE_IND			0x18	//core_5.0				//	5
#define 				LL_MIN_USED_CHN_IND			0x19	//core_5.0				//	3

#define 				LL_CTE_REQ					0x1A	//core_5.1				//	2
#define 				LL_CTE_RSP					0x1B	//core_5.1				//	2
#define 				LL_PERIODIC_SYNC_IND		0x1C	//core_5.1				// 35
#define 				LL_CLOCK_ACCURACY_REQ		0x1D	//core_5.1				//	2
#define 				LL_CLOCK_ACCURACY_RSP		0x1E	//core_5.1				//	2


#define 				LL_CIS_REQ					0x1F	//core_5.2				//	36
#define 				LL_CIS_RSP					0x20	//core_5.2				//	9
#define 				LL_CIS_IND					0x21	//core_5.2				//  16
#define 				LL_CIS_TERMINATE_IND		0x22	//core_5.2				//	4
#define 				LL_POWER_CONTROL_REQ		0x23	//core_5.2				//	4
#define 				LL_POWER_CONTROL_RSP		0x24	//core_5.2				//	5
#define 				LL_POWER_CHANGE_IND			0x25	//core_5.2				//	5









//Extended Header BIT
#define 		EXTHD_BIT_ADVA         							BIT(0)
#define 		EXTHD_BIT_TARGETA      							BIT(1)
#define			EXTHD_BIT_CTE_INFO         						BIT(2)
#define 		EXTHD_BIT_ADI		    						BIT(3)
#define 		EXTHD_BIT_AUX_PTR      							BIT(4)
#define 		EXTHD_BIT_SYNC_INFO	    						BIT(5)
#define 		EXTHD_BIT_TX_POWER	     						BIT(6)


//Extended Header Length
#define			EXTHD_LEN_6_ADVA								6
#define			EXTHD_LEN_6_TARGETA								6
#define			EXTHD_LEN_1_CTE									1
#define			EXTHD_LEN_2_ADI									2
#define			EXTHD_LEN_3_AUX_PTR								3
#define			EXTHD_LEN_18_SYNC_INFO							18
#define			EXTHD_LEN_1_TX_POWER							1


#define 		LL_EXTADV_MODE_NON_CONN_NON_SCAN    			(0x00)
#define 		LL_EXTADV_MODE_CONN        						(0x01)		//connectable, none_scannable
#define 		LL_EXTADV_MODE_SCAN        						(0x02)      //scannable,   none_connectable
#define 		LL_EXTADV_MODE_RFU        						(0x03)

#define			EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_30_US			0
#define			EXT_ADV_PDU_AUXPTR_OFFSET_UNITS_300_US			1

#define			EXT_ADV_PDU_AUXPTR_CA_51_500_PPM				0
#define			EXT_ADV_PDU_AUXPTR_CA_0_50_PPM					1

#define			EXT_ADV_PDU_SYINC_OFFSET_UNITS_30_US			0
#define			EXT_ADV_PDU_SYINC_OFFSET_UNITS_300_US			1


#define			BIG_PDU_BIG_OFFSET_UNITS_30_US					0
#define			BIG_PDU_BIG_OFFSET_UNITS_300_US					1

/**
 *  @brief  The Aux PHY field: the PHY used to transmit the auxiliary packet
 */
typedef enum{
	PHY_USED_AUXPTR_LE_1M		= 0x00,
	PHY_USED_AUXPTR_LE_2M		= 0x01,
	PHY_USED_AUXPTR_LE_CODED	= 0x02,
	PHY_USED_AUXPTR_RFU_BEGIN	= 0x03,  //0b011~0b111 reserved for future use
}aux_phy_field_t;




// Advertise channel PDU Type
typedef enum advChannelPDUType_e {
	LL_TYPE_ADV_IND 		 = 0x00,
	LL_TYPE_ADV_DIRECT_IND 	 = 0x01,
	LL_TYPE_ADV_NONCONN_IND  = 0x02,
	LL_TYPE_SCAN_REQ 		 = 0x03,		LL_TYPE_AUX_SCAN_REQ 	 = 0x03,
	LL_TYPE_SCAN_RSP 		 = 0x04,
	LL_TYPE_CONNNECT_REQ 	 = 0x05,		LL_TYPE_AUX_CONNNECT_REQ = 0x05,
	LL_TYPE_ADV_SCAN_IND 	 = 0x06,

	LL_TYPE_ADV_EXT_IND		 = 0x07,		LL_TYPE_AUX_ADV_IND 	 = 0x07,	LL_TYPE_AUX_SCAN_RSP = 0x07,	LL_TYPE_AUX_SYNC_IND = 0x07,	LL_TYPE_AUX_CHAIN_IND = 0x07,
	LL_TYPE_AUX_CONNNECT_RSP = 0x08,
} advChannelPDUType_t;



typedef enum {
	TYPE_MASK_ADV_IND 	 		= 	BIT(0),
	TYPE_MASK_ADV_DIRECT_IND	=	BIT(1),
	TYPE_MASK_ADV_NONCONN_IND 	= 	BIT(2),
	TYPE_MASK_SCAN_REQ  	 	= 	BIT(3),
	TYPE_MASK_SCAN_RSP 		 	= 	BIT(4),
	TYPE_MASK_CONNNECT_REQ 	 	= 	BIT(5),
	TYPE_MASK_ADV_SCAN_IND 		= 	BIT(6),
	TYPE_MASK_EXT_ADV 	 		= 	BIT(7),
} advTypeMask_t;








/**
 *  @brief  Definition for LLID of Connected Isochronous PDU header field
 */
typedef enum{
	ISO_LLID_UNFRAMED_PDU_END_FRAGMENT_SDU			= 0x00, //Unframed CIS Data PDU; end fragment of an SDU or a complete SDU
	ISO_LLID_UNFRAMED_PDU_START_CONTI_FRAGMENT_SDU  = 0x01,	//Unframed CIS Data PDU; start or continuation fragment of an SDU
	ISO_LLID_FRAMED_PDU_SEGMENT_SDU					= 0x02, //Framed CIS Data PDU; one or more segments of an SDU
	ISO_LLID_RESERVED 								= 0x03,
}ISO_llid_type;




/**
 *  @brief  Definition for BIG Control PDU Opcode
 */
typedef enum{
	BIG_CHANNEL_MAP_IND	= 0x00,
	BIG_TERMINATE_IND	= 0x01,
}big_ctrl_op;





/******************************************** L2CAP **************************************************************/

// l2cap pb flag type
#define L2CAP_FRIST_PKT_H2C              0x00
#define L2CAP_CONTINUING_PKT             0x01
#define L2CAP_FIRST_PKT_C2H              0x02


/**
 *  @brief  Definition for L2CAP CID name space for the LE-U
 */
typedef enum{
	L2CAP_CID_NULL				= 0x0000,
	L2CAP_CID_ATTR_PROTOCOL		= 0x0004,
	L2CAP_CID_SIG_CHANNEL		= 0x0005,
	L2CAP_CID_SMP				= 0x0006,
}l2cap_cid_type;


/**
 *  @brief  Definition for L2CAP signal packet formats
 */
typedef enum{
	L2CAP_COMMAND_REJECT_RSP           		= 0x01,
	L2CAP_CONNECTION_REQ                 	= 0x02,
	L2CAP_CONNECTION_RSP                 	= 0x03,
	L2CAP_CONFIGURATION_REQ                	= 0x04,
	L2CAP_CONFIGURATION_RSP           		= 0x05,
	L2CAP_DISCONNECTION_REQ           		= 0x06,
	L2CAP_DISCONNECTION_RSP           		= 0x07,
	L2CAP_ECHO_REQ          		 		= 0x08,
	L2CAP_ECHO_RSP           				= 0x09,
	L2CAP_INFORMATION_REQ           		= 0x0A,
	L2CAP_INFORMATION_RSP           		= 0x0B,
	L2CAP_CREATE_CHANNEL_REQ          		= 0x0C,
	L2CAP_CREATE_CHANNEL_RSP           		= 0x0D,
	L2CAP_MOVE_CHANNEL_REQ           		= 0x0E,
	L2CAP_MOVE_CHANNEL_RSP           		= 0x0F,
	L2CAP_MOVE_CHANNEL_CONFIRMATION_REQ		= 0x10,
	L2CAP_MOVE_CHANNEL_CONFIRMATION_RSP     = 0x11,
	L2CAP_CONNECTION_PARAMETER_UPDATE_REQ	= 0x12,		L2CAP_CMD_CONN_UPD_PARA_REQ		= 0x12,
	L2CAP_CONNECTION_PARAMETER_UPDATE_RSP	= 0x13,		L2CAP_CMD_CONN_UPD_PARA_RESP 	= 0x13,
	L2CAP_LE_CREDIT_BASED_CONNECTION_REQ 	= 0x14,
	L2CAP_LE_CREDIT_BASED_CONNECTION_RSP 	= 0x15,
	L2CAP_FLOW_CONTROL_CREDIT_IND 			= 0x16,
	L2CAP_CREDIT_BASED_CONNECTION_REQ 		= 0x17,	//core_5.2
	L2CAP_CREDIT_BASED_CONNECTION_RSP 		= 0x18,	//core_5.2
	L2CAP_CREDIT_BASED_RECONFIGURE_REQ 		= 0x19,	//core_5.2
	L2CAP_CREDIT_BASED_RECONFIGURE_RSP 		= 0x1A,	//core_5.2
}l2cap_sig_pkt_format;

/**
 *  @brief  Definition for Error Response of signal packet
 *  See the Core_v5.0(Vol 3/Part A/4.1) for more information.
 */
typedef enum{
	SIG_CMD_NOT_UNDERSTAND	= 0,
	SIG_MTU_EXCEEDED		= 1,
	SIG_INVALID_CID_REQUEST	= 2,
}l2cap_sig_cmd_reject_reason;

//Result values for the L2CAP_CREDIT_BASED_CONNECTION_RSP packet
typedef enum{
	L2CAP_ALL_CONN_SUCESSFULL=0,
	L2CAP_ALL_CONN_REFUSED_SPSM_NOT_SUPPORT=2,
	L2CAP_SOME_CONN_REFUSED_INSUFFICIENT_RESOURCES_AVAILABLE=4,
	L2CAP_ALL_CONN_REFUSED_INSUFFICIENT_AUTHENTICATION=5,
	L2CAP_ALL_CONN_REFUSED_INSUFFICIENT_AUTHORIZATION=6,
	L2CAP_ALL_CONN_REFUSED_INSUFFICIENT_ENCRYPTION_KEY_SIZE=7,
	L2CAP_ALL_CONN_REFUSED_INSUFFICIENT_ENCRYPTION=8,
	L2CAP_SOME_CONN_REFUSED_INVALID_SOURCE_CID=9,
	L2CAP_SOME_CONN_REFUSED_SOURCE_CID_ALREADY_ALLOCATED=0x0a,
	L2CAP_ALL_CONN_REFUSED_UNACCEPTABLE_PARAMETERS=0x0b,
	L2CAP_ALL_CONN_REFUSED_INVALID_PARAMETERS=0x0c,
}l2cap_credit_based_conn_rsp_result;
typedef enum{
	L2CAP_RECONFIG_SUCCESSFUL = 0,
	L2CAP_RECONFIG_FAIL_REDUCTION_MTU = 1,
	L2CAP_RECONFIG_FAIL_REDUCTION_MPS = 2,
	L2CAP_RECONFIG_FAIL_ONE_MORE_DCID_INVALID = 3,
	L2CAP_RECONFIG_FAIL_OTHER_UNACCEPTABLE_PARAMETERS = 4,
}l2cap_credit_based_reconfigure_rsp_result;







/**
 *  @brief  Definition for Error Response of ATTRIBUTE PROTOCOL PDUS
 *  See the Core_v5.0(Vol 3/Part F/3.4.1.1, "Error Response") for more information.
 */
typedef enum {

	ATT_SUCCESS = 0,

    ATT_ERR_INVALID_HANDLE,                              //!< The attribute handle given was not valid on this server
    ATT_ERR_READ_NOT_PERMITTED,                          //!< The attribute cannot be read
    ATT_ERR_WRITE_NOT_PERMITTED,                         //!< The attribute cannot be written
    ATT_ERR_INVALID_PDU,                                 //!< The attribute PDU was invalid
    ATT_ERR_INSUFFICIENT_AUTH,                           //!< The attribute requires authentication before it can be read or written
    ATT_ERR_REQ_NOT_SUPPORTED,                           //!< Attribute server does not support the request received from the client
    ATT_ERR_INVALID_OFFSET,                              //!< Offset specified was past the end of the attribute
    ATT_ERR_INSUFFICIENT_AUTHOR,                         //!< The attribute requires authorization before it can be read or written
    ATT_ERR_PREPARE_QUEUE_FULL,                          //!< Too many prepare writes have been queued
    ATT_ERR_ATTR_NOT_FOUND,                              //!< No attribute found within the given attribute handle range
    ATT_ERR_ATTR_NOT_LONG,                               //!< The attribute cannot be read or written using the Read Blob Request
    ATT_ERR_INSUFFICIENT_KEY_SIZE,                       //!< The Encryption Key Size used for encrypting this link is insufficient
    ATT_ERR_INVALID_ATTR_VALUE_LEN,                      //!< The attribute value length is invalid for the operation
    ATT_ERR_UNLIKELY_ERR,                                //!< The attribute request that was requested has encountered an error that was unlikely, and therefore could not be completed as requested
    ATT_ERR_INSUFFICIENT_ENCRYPT,                        //!< The attribute requires encryption before it can be read or written
    ATT_ERR_UNSUPPORTED_GRP_TYPE,                        //!< The attribute type is not a supported grouping attribute as defined by a higher layer specification
    ATT_ERR_INSUFFICIENT_RESOURCES,                      //!< Insufficient Resources to complete the request



}att_err_t;















/**
 *  @brief  AuxPtr field
 */
typedef struct{
	u8  chn_index    :6;
	u8  ca 		 	:1;
	u8  offset_unit	:1;
	u16 aux_offset  :13;
	u16	aux_phy		:3;
} aux_ptr_t;


/**
 *  @brief  SYncInfo field
 */
typedef struct{
	u16 syncPktOffset: 13;
	u16 offsetUnit:    1;
	u16 offsetAdjust:  1;
	u16 rfu: 		   1;

	u16 itvl;

	u8	chm[5];//[0:36]chm, : [37:39]sca
	u32 AA;
	u8 crcInit[3];

	u16 evtCounter;
}sync_info_t;









typedef struct{
	u32 dma_len;

	u8 type   :4;
	u8 rfu1   :1;
	u8 chan_sel:1;
	u8 txAddr :1;
	u8 rxAddr :1;

	u8  rf_len;				//LEN(6)_RFU(2)

	u8	advA[6];			//address
	u8	data[31];
}rf_packet_adv_t;



typedef struct{
	u32 dma_len;

	u8  type   :4;
	u8  rfu1   :1;
	u8  chan_sel:1;
	u8  txAddr :1;
	u8  rxAddr :1;

	u8  rf_len;				//LEN(6)_RFU(2)

	u8	scanA[6];			//
	u8	advA[6];			//
}rf_packet_scan_req_t;

typedef struct{
	u32 dma_len;

	u8  type   :4;
	u8  rfu1   :1;
	u8  chan_sel:1;
	u8  txAddr :1;
	u8  rxAddr :1;

	u8  rf_len;				//LEN(6)_RFU(2)

	u8	advA[6];			//address
	u8	data[31];			//0-31 byte
}rf_packet_scan_rsp_t;

typedef struct{
	u32 dma_len;

	u8  type   :4;
	u8  rfu1   :1;
	u8  chan_sel:1;
	u8  txAddr :1;
	u8  rxAddr :1;

	u8  rf_len;				//LEN(6)_RFU(2)
	u8	initA[6];			//scanA
	u8	advA[6];			//
	u8	accessCode[4];		// access code
	u8	crcinit[3];
	u8	winSize;
	u16	winOffset;
	u16 interval;
	u16 latency;
	u16 timeout;
	u8	chm[5];
	u8	hop;				//sca(3)_hop(5)
}rf_packet_connect_t;



typedef struct {
	u8 	type;
	u8  rf_len;
	u8 	opcode;
	u8 	winSize;
	u16 winOffset;
	u16 interval;
	u16 latency;
	u16 timeout;
	u16 instant;
}rf_packet_connect_upd_req_t;

typedef struct {
	u8 	type;
	u8  rf_len;
	u8 	opcode;
	u8 	chm[5];
	u16 instant;
} rf_packet_chm_upd_req_t;

typedef struct {
	u8 	type;
	u8  rf_len;
	u8 	opcode;
	u8 	rand[8];
	u16 ediv;
	u8	skdm[8];
	u8	ivm[4];
} rf_packet_ll_enc_req_t;

typedef struct {
	u8 	type;
	u8  rf_len;
	u8 	opcode;
	u8	skds[8];
	u8	ivs[4];
} rf_packet_ll_enc_rsp_t;

typedef struct {
	u8 	type;
	u8  rf_len;
	u8 	opcode;
	u8	unknownType;
} rf_packet_ll_unknown_rsp_t;

typedef struct {
	u8 	type;
	u8  rf_len;
	u8 	opcode;
	u8	featureSet[8];
} rf_packet_ll_feature_exg_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u8 	opcode;
	u8  versNr;
	u16 compId;
	u16 subVersNr;
}rf_packet_version_ind_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u8 	opcode;
	u8  errCode;
}rf_packet_ll_reject_ind_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u8 	opcode;
	u8	rejectOpcode;
	u8  errCode;
}rf_packet_ll_reject_ext_ind_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u8	opcode;
	u8	reason;
}rf_packet_ll_terminate_t;



typedef struct{
	u8	type;
	u8  rf_len;

	u8	opcode;				//
	u8	dat[1];				//
}rf_packet_ll_control_t;




















typedef struct{
    u8 llid   :2;
    u8 nesn   :1;
    u8 sn     :1;
    u8 md     :1;
    u8 rfu1   :3;

	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 data[1];
}rf_packet_l2cap_t;



typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 data[1];
}rf_packet_l2cap_req_t;




typedef struct{
	u8	llid;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8	id;
	u16 data_len;
	u16 min_interval;
	u16 max_interval;
	u16 latency;
	u16 timeout;
}rf_packet_l2cap_connParaUpReq_t;



typedef struct{
	u8	llid;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8	id;
	u16 data_len;
	u16 result;
}rf_packet_l2cap_connParaUpRsp_t;





typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  code;
	u8  id;
	u16 dataLen;
	u16  result;
}rf_pkt_l2cap_sig_connParaUpRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  code;
	u8  id;
	u16 length;
	u16 psm;
	u16 mtu;
	u16 mps;
	u16 init_credits;
	u16 scid[5];
}rf_pkt_l2cap_credit_based_connection_req_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  code;
	u8  id;
	u16 length;
	u16 mtu;
	u16 mps;
	u16 init_credits;
	u16 result;
	u16 dcid[5];
}rf_pkt_l2cap_credit_based_connection_rsp_t;


typedef struct{
	u8	llid;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8	id;
	u16 length;
	u16 spsm;
	u16 mtu;
	u16 mps;
	u16 init_credits;
	u16 scid[5];
}rf_packet_l2cap_credit_based_connection_req_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  code;
	u8  id;
	u16 length;
	u16 mtu;
	u16 mps;
	u16 dcid[5];
}rf_pkt_l2cap_credit_based_reconfigure_req_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  code;
	u8  id;
	u16 length;
	u16 result;
}rf_pkt_l2cap_credit_based_reconfigure_rsp_t;







typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 	errOpcode;
	u16 errHandle;
	u8  errReason;
}rf_packet_att_errRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;

	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16	startingHandle;
	u16	endingHandle;
	u8	attType[2];				//
}rf_packet_att_readByType_t;

typedef struct{
	u8	type;
	u8  rf_len;

	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16	startingHandle;
	u16	endingHandle;
	u8	attType[2];
	u8  attValue[2];
}rf_packet_att_findByTypeReq_t;

typedef struct{
	u8	type;
	u8  rf_len;

	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16 data[1];
}rf_packet_att_findByTypeRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16 handle;
}rf_packet_att_read_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16 handle;
	u16 offset;
}rf_packet_att_readBlob_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 	value[22];
}rf_packet_att_readRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8  datalen;
	u8  data[1];			// character_handle / property / value_handle / value
}rf_pkt_att_readByTypeRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8  datalen;
	u8  data[1];			// character_handle / property / value_handle / value
}rf_packet_att_readByTypeRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8  datalen;
	u8  data[1];			// character_handle / property / value_handle / value
}rf_packet_att_data_readByTypeRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8  datalen;
	u8  data[3];
}rf_packet_att_readByGroupTypeRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8  format;
	u8  data[1];			// character_handle / property / value_handle / value
}rf_packet_att_findInfoReq_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 flags;
}rf_packet_att_executeWriteReq_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u16 handle;
	u8 value;
}rf_packet_att_write_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 mtu[2];
}rf_packet_att_mtu_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 mtu[2];
}rf_packet_att_mtu_exchange_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
}rf_packet_att_writeRsp_t;

typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8  datalen;
	u8  data[1];			// character_handle / property / value_handle / value
}att_readByTypeRsp_t;


typedef struct{
	u8	type;
	u8  rf_len;
	u16	l2capLen;
	u16	chanId;
	u8  opcode;
	u8 	value[22];
}att_readRsp_t;














#define U16_SMALLER_THAN(d1, d2)     		  ((unsigned short)((d1) - (d2)) > BIT(15))
#define U16_GREATER_THAN_EQUAL_TO(d1, d2)     ((unsigned short)((d1) - (d2)) < BIT(15))

#define U32_SMALLER_THAN(d1, d2)     		  ((unsigned int)((d1) - (d2)) > BIT(31))
#define U32_GREATER_THAN_EQUAL_TO(d1, d2)     ((unsigned int)((d1) - (d2)) < BIT(31))




void smemset4(int * dest, int val, int len);
void smemset(void * dest, int val, int len);
void smemcpy(void *pd, void *ps, int len);
void smemcpy4(void *pd, void *ps, int len);
int smemcmp(void * m1, void * m2, int len);
int smemcmp4(unsigned int * m1, unsigned int * m2, register unsigned int len);
void *smemmove( void *dest , const void *src , size_t count);



#endif /* BLE_STACK_H_ */
