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
#elif (MCU_CORE_TYPE == MCU_CORE_TL321X)
	#define	CERTIFICATION_MARK			4
	#define	SOFT_STRUCTURE				1
	#define	MAJOR_VERSION				0
	#define	MINOR_VERSION				0
	#define	PATCH						0
	#define	RSVD						0xff
#endif




/* Different process for different MCU: ****************************/
#if(MCU_CORE_TYPE == MCU_CORE_B91)
	#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION			1
#elif(MCU_CORE_TYPE == MCU_CORE_B92)
	#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION			1
#elif(MCU_CORE_TYPE == MCU_CORE_TL321X)
	#define STACK_IRQ_CODE_IN_SRAM_DUE_TO_FLASH_OPERATION			1
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

#ifndef STACK_SUPPORT_FLASH_PROTECTION_ENABLE
#define STACK_SUPPORT_FLASH_PROTECTION_ENABLE				1
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


#ifndef HCI_SEND_NUM_OF_CMP_AFT_ACK
#define HCI_SEND_NUM_OF_CMP_AFT_ACK									0
#endif

#ifndef HCI_TX_FIFO_OPTIMIZE_EN
#define HCI_TX_FIFO_OPTIMIZE_EN                                     0
#endif

#ifndef HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN
#define HCI_CONTROLLER_TO_HOST_FLOW_CTRL_EN							0
#endif


/* 1: multiple connection SDK; 0 : single connection SDK */
#define	BLE_MULTIPLE_CONNECTION_ENABLE								1




