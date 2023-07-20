/******************************************************************************
 * Copyright (c) 2023 Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#pragma once

//////////////////////////////////////////////////////////////////////////////
/**
 *  @brief  Definition for Device info
 */
#include "drivers.h"
#include "tl_common.h"


// Version : CERTIFICATION_MARK.SOFT_STRUCTURE.Major.Minor Patch
#if (MCU_CORE_TYPE == MCU_CORE_B91)
	#define	CERTIFICATION_MARK			4
	#define	SOFT_STRUCTURE				1
	#define	MAJOR_VERSION				0
	#define	MINOR_VERSION				0
	#define	PATCH						0
	#define	RSVD						0xff
#elif (MCU_CORE_TYPE == MCU_CORE_B92)
	#define	CERTIFICATION_MARK			4
	#define	SOFT_STRUCTURE				1
	#define	MAJOR_VERSION				0
	#define	MINOR_VERSION				0
	#define	PATCH						0
	#define	RSVD						0xff
#endif


///////////////////  Feature ////////////////////////////
#ifndef		MULTIPLE_LOCAL_DEVICE_ENABLE
#define		MULTIPLE_LOCAL_DEVICE_ENABLE				    		0
#endif

#ifndef     LL_EXT_ADV_DURATION_OPTIMIZE_EN
#define     LL_EXT_ADV_DURATION_OPTIMIZE_EN                         0
#endif
//////////////////////////////////////////////////////////////////////



/* Different process for different MCU: ****************************/
#if(MCU_CORE_TYPE == MCU_CORE_825x)
	#define	FIX_HW_CRC24_EN											1
	#define HW_ECDH_EN                                     			0
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				1
#elif(MCU_CORE_TYPE == MCU_CORE_827x)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				1
#elif(MCU_CORE_TYPE == MCU_CORE_B91)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				0
	#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION			1
#elif(MCU_CORE_TYPE == MCU_CORE_B92)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
	#define ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN				0
	#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION			1
	#define HW_AES_CCM_ALG_EN										0
#else
	#error "unsupported mcu type !"
#endif



/* OS Support: default open RTOS */
#ifndef OS_SUP_EN
#define OS_SUP_EN     1
#endif



/*	This code in RF irq and system irq put in RAM by force
 * Because of the flash resource contention problem, when the
 * flash access is interrupted by a higher priority interrupt,
 * the interrupt processing function cannot operate the flash(For Eagle)
*/
#ifndef STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION
#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION				0
#endif



//Link layer feature enable flag default setting
#ifndef LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION
#define LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION					1
#endif

#ifndef LL_FEATURE_SUPPORT_PRIVACY
#define LL_FEATURE_SUPPORT_PRIVACY									1   //must be enable, because we need process peer device RPA
#endif

#ifndef LL_FEATURE_SUPPORT_EXTENDED_SCANNER_FILTER_POLICIES
#define LL_FEATURE_SUPPORT_EXTENDED_SCANNER_FILTER_POLICIES         0
#endif

#ifndef LL_FEATURE_SUPPORT_LE_2M_PHY
#define LL_FEATURE_SUPPORT_LE_2M_PHY								1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_CODED_PHY
#define LL_FEATURE_SUPPORT_LE_CODED_PHY								1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_PAST_SENDER
#define LL_FEATURE_SUPPORT_LE_PAST_SENDER							1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_PAST_RECIPIENT
#define LL_FEATURE_SUPPORT_LE_PAST_RECIPIENT						1
#endif

#ifndef LL_FEATURE_SUPPORT_SLEEP_CLK_ACCURACY_UPDATE
#define LL_FEATURE_SUPPORT_SLEEP_CLK_ACCURACY_UPDATE			    0
#endif

#ifndef LL_FEATURE_SUPPORT_REMOTE_PUBLIC_KEY_VALIDATION
#define LL_FEATURE_SUPPORT_REMOTE_PUBLIC_KEY_VALIDATION             1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING
#define LL_FEATURE_SUPPORT_LE_EXTENDED_ADVERTISING					1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING
#define LL_FEATURE_SUPPORT_LE_EXTENDED_SCANNING						1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE
#define LL_FEATURE_SUPPORT_LE_EXTENDED_INITIATE						1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING
#define LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING					1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC
#define LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC				1
#endif

#ifndef LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2
#define LL_FEATURE_SUPPORT_CHANNEL_SELECTION_ALGORITHM2				1
#endif

#ifndef LL_FEATURE_SUPPORT_MIN_USED_OF_USED_CHANNELS
#define LL_FEATURE_SUPPORT_MIN_USED_OF_USED_CHANNELS                0
#endif

//core_5.1 feature begin
#ifndef LL_FEATURE_SUPPORT_LE_AOA_AOD
#define LL_FEATURE_SUPPORT_LE_AOA_AOD								0
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTION_CTE_REQUEST
#define LL_FEATURE_SUPPORT_CONNECTION_CTE_REQUEST					0
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTION_CTE_RESPONSE
#define LL_FEATURE_SUPPORT_CONNECTION_CTE_RESPONSE					0
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_TRANSMITTER
#define LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_TRANSMITTER			0
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_RECEIVER
#define LL_FEATURE_SUPPORT_CONNECTIONLESS_CTE_RECEIVER				0
#endif

#ifndef LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD
#define LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_TRANSMISSION_AOD	0
#endif

#ifndef LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_RECEPTION_AOA
#define LL_FEATURE_SUPPORT_ANTENNA_SWITCHING_CTE_RECEPTION_AOA		0
#endif

#ifndef LL_FEATURE_SUPPORT_RECEIVING_CONSTANT_TONE_EXTENSIONS
#define LL_FEATURE_SUPPORT_RECEIVING_CONSTANT_TONE_EXTENSIONS		0
#endif
//core_5.1 feature end


//core_5.2 feature begin
#ifndef LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER
#define LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER		1
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
#define LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE		1
#endif

#ifndef LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER
#define LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER					1 //broadcast sender
#endif

#ifndef LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER
#define LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER					1 //broadcast receiver
#endif

#ifndef LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS
#define LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS						1
#endif

#ifndef	LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE
#define	LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE					1
#endif

#ifndef	LL_FEATURE_SUPPORT_POWER_CONTROL_REQUEST
#define	LL_FEATURE_SUPPORT_POWER_CONTROL_REQUEST					0
#endif

#ifndef	LL_FEATURE_SUPPORT_POWER_LOSS_MONITORING
#define	LL_FEATURE_SUPPORT_POWER_LOSS_MONITORING					0
#endif
//core_5.2 feature end


//core_5.3 feature begin
#ifndef	LL_FEATURE_SUPPORT_PERIODIC_ADV_ADI_SUPPORT
#define	LL_FEATURE_SUPPORT_PERIODIC_ADV_ADI_SUPPORT					0
#endif

#ifndef	LL_FEATURE_SUPPORT_CONNECTION_SUBRATING
#define	LL_FEATURE_SUPPORT_CONNECTION_SUBRATING						0
#endif

#ifndef	LL_FEATURE_SUPPORT_CONNECTION_SUBRATING_HOST
#define	LL_FEATURE_SUPPORT_CONNECTION_SUBRATING_HOST				1
#endif

#ifndef	LL_FEATURE_SUPPORT_CHANNEL_CLASSIFICATION
#define	LL_FEATURE_SUPPORT_CHANNEL_CLASSIFICATION					1
#endif



//core_5.4 begin
#ifndef	LL_FEATURE_SUPPORT_ADVERTISING_CODING_SELECTION
#define	LL_FEATURE_SUPPORT_ADVERTISING_CODING_SELECTION						0
#endif

#ifndef	LL_FEATURE_SUPPORT_ADVERTISING_CODING_SELECT_HOST_SUPPORT
#define	LL_FEATURE_SUPPORT_ADVERTISING_CODING_SELECT_HOST_SUPPORT			0
#endif

#ifndef	LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER
#define	LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_ADVERTISER	0
#endif

#ifndef	LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER
#define	LL_FEATURE_SUPPORT_PERIODIC_ADVERTISING_WITH_RESPONSES_SCANNER		0
#endif
//core_5.4 end

//core_5.4+  begin
#ifndef	LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_INITIATOR
#define	LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_INITIATOR						1
#endif

#ifndef	LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR
#define	LL_FEATURE_SUPPORT_CHANNEL_SOUNDING_REFLECTOR						1
#endif
//core_5.4+  end




#if LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER
	//BIS sync, no matter either by extended scanning or by PAST procedure, both require periodic sync.
	#if (!LL_FEATURE_SUPPORT_LE_PERIODIC_ADVERTISING_SYNC)
		#error "must support periodic sync"
	#endif
#endif

//core_5.3 feature end


/* 2 CIS slave allocate on same one ACL slave, BQB 5.2 need  */
#ifndef	ONE_ACL_SLAVE_MATCH_2_CIS_SLAVE_ENABLE
#define	ONE_ACL_SLAVE_MATCH_2_CIS_SLAVE_ENABLE						0
#endif

#ifndef	CIS_WINDOW_WIDENING_FOR_BIG_PPM
#define	CIS_WINDOW_WIDENING_FOR_BIG_PPM								0
#endif


#ifndef HCI_SEND_NUM_OF_CMP_AFT_ACK
#define HCI_SEND_NUM_OF_CMP_AFT_ACK									0
#endif

#ifndef HCI_TX_FIFO_OPTIMIZE_EN
#define HCI_TX_FIFO_OPTIMIZE_EN                                     0
#endif

#ifndef HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
#define HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN							0
#endif

#ifndef L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE
#define L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE							0  //SiHui: just for debug
#endif

#ifndef L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
#define L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN     				1
#endif



//gen p256, dhkey supported by controller
#ifndef	CONTROLLER_GEN_P256KEY_ENABLE
#define	CONTROLLER_GEN_P256KEY_ENABLE								0
#endif

#ifndef SMP_SEC_LEVEL_CHECK_EN
#define	SMP_SEC_LEVEL_CHECK_EN										0
#endif



#ifndef ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN
#define	ZBIT_FLASH_WRITE_TIME_LONG_WORKAROUND_EN					0
#endif

/* MCU without IRQ Priority, but need support sonos Flash */
#ifndef SONOS_FLASH_WRITE_TIME_LONG_WORKAROUND_EN
#define	SONOS_FLASH_WRITE_TIME_LONG_WORKAROUND_EN					0
#endif

#ifndef  LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN
#define  LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN                0
#endif






#if (LL_FEATURE_SUPPORT_PRIVACY)
	/* privacy relative */
	#ifndef LL_FEATURE_SUPPORT_LOCAL_RPA
	#define LL_FEATURE_SUPPORT_LOCAL_RPA								1
	#endif

	//if network privacy mode ignore IDA
	#ifndef NETWORK_PRIVACY_IGNORE_IDA_CHECK
	#define NETWORK_PRIVACY_IGNORE_IDA_CHECK							0   //BQB need enable, can be disable in SDK
	#endif


	#ifndef	LL_FEATURE_SUPPORT_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE
	#define LL_FEATURE_SUPPORT_RPA_ADV_DATA_RELATED_ADDRESS_CHANGE		0   //BQB need enable, can be disable in SDK
	#endif


	#ifndef	EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER
	#define EXTADV_DATA_CHANGE_MANUAL_DATA_BUFFER						0	//BQB need enable, can be disable in SDK
	#endif


	#ifndef	SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY
	#define SMP_LOCAL_IRK_MATCH_CONTROLLER_NEW_PRIVACY					0
	#endif
#endif



/* 1: multiple connection SDK; 0 : single connection SDK */
#define	BLE_MULTIPLE_CONNECTION_ENABLE								1

// fanqh start
#ifndef		FIX_CIS_EVT_OVERFLOW
#define		FIX_CIS_EVT_OVERFLOW									0
#endif



#ifndef	CIS_ADD_CIE
#define	CIS_ADD_CIE													1
#endif

#ifndef	BIS_BRD_SET_PARAM
#define BIS_BRD_SET_PARAM											0
#endif


#ifndef CIS_TSK_JUMP_ADD_PDU
#define	CIS_TSK_JUMP_ADD_PDU										1
#endif


#ifndef BQB_LOWER_TESTER_ENABLE
#define BQB_LOWER_TESTER_ENABLE										0
#endif

#ifndef BQB_UPPER_TESTER_ENABLE
#define BQB_UPPER_TESTER_ENABLE										0
#endif


#ifndef	CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE
#define	CUSTOM_CONNECTION_ESTABLISH_EVT_ENABLE						0
#endif

#ifndef	FANQH_OPTIMIZE_BIS_API
#define FANQH_OPTIMIZE_BIS_API										1
#endif


