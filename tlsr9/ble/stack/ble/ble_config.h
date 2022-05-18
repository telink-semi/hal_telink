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
#pragma once

//////////////////////////////////////////////////////////////////////////////
/**
 *  @brief  Definition for Device info
 */
#include "drivers.h"
#include "tl_common.h"

#define 	BQB_TEST_EN      										0
//BQB Test
#if BQB_TEST_EN
	#define BQB_TEST_PHY_EN         1
    #define BQB_TEST_CONN_UPD_EN    1
    #define BQB_TEST_CHN_MAP_EN     1
#else
    #define BQB_TEST_PHY_EN         0
    #define BQB_TEST_CONN_UPD_EN    0
    #define BQB_TEST_CHN_MAP_EN     0
#endif


///////////////////  Feature ////////////////////////////
#ifndef		LL_MULTI_SLAVE_MAC_ENABLE
#define		LL_MULTI_SLAVE_MAC_ENABLE				    			0
#endif

//////////////////////////////////////////////////////////////////////
//note both the following two macro XXX_PRIVATE_XXX and XXX_NORMAL_XXX
//CAN NOT be set 1 at the same time.One is private,another is standard.
#ifndef		LL_FEATURE_PRIVATE_BIS_SYNC_RECEIVER
#define 	LL_FEATURE_PRIVATE_BIS_SYNC_RECEIVER					0
#endif

#ifndef		LL_FEATURE_NORMAL_BIS_SYNC_RECEIVER
#define		LL_FEATURE_NORMAL_BIS_SYNC_RECEIVER						1
#endif
//////////////////////////////////////////////////////////////////////


#if(MCU_CORE_TYPE == MCU_CORE_825x)
	#define	FIX_HW_CRC24_EN											1
	#define HW_ECDH_EN                                     			0
#elif(MCU_CORE_TYPE == MCU_CORE_827x)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
#elif(MCU_CORE_TYPE == MCU_CORE_9518)
	#define	FIX_HW_CRC24_EN											0
	#define HW_ECDH_EN                                      		1
#else
	#error "unsupported mcu type !"
#endif

//conn param update/map update
#ifndef	BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
#define BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE					0  //TODO:
#endif


#ifndef LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN
#define LE_AUTHENTICATED_PAYLOAD_TIMEOUT_SUPPORT_EN					0
#endif


//Link layer feature enable flag default setting
#ifndef LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION
#define LL_FEATURE_SUPPORT_LE_DATA_LENGTH_EXTENSION					1
#endif

#ifndef LL_FEATURE_SUPPORT_LL_PRIVACY
#define LL_FEATURE_SUPPORT_LL_PRIVACY								0 //TODO: legAdv and slave role conn support now
#endif

#ifndef LL_FEATURE_SUPPORT_LE_2M_PHY
#define LL_FEATURE_SUPPORT_LE_2M_PHY								1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_CODED_PHY
#define LL_FEATURE_SUPPORT_LE_CODED_PHY								1
#endif

#ifndef LL_FEATURE_SUPPORT_LE_AOA_AOD
#define LL_FEATURE_SUPPORT_LE_AOA_AOD								0
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






//core_5.2 feature begin
#ifndef LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER
#define LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_MASTER		1
#endif

#ifndef LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE
#define LL_FEATURE_SUPPORT_CONNECTED_ISOCHRONOUS_STREAM_SLAVE		1
#endif

#ifndef LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER
#define LL_FEATURE_SUPPORT_ISOCHRONOUS_BROADCASTER					1
#endif

#ifndef LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER
#define LL_FEATURE_SUPPORT_SYNCHRONIZED_RECEIVER					1
#endif

#ifndef LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS
#define LL_FEATURE_SUPPORT_ISOCHRONOUS_CHANNELS						1
#endif

#ifndef	LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE
#define	LL_FEATURE_SUPPORT_ISOCHRONOUS_TEST_MODE					1
#endif
//core_5.2 feature end



#ifndef BQB_LOWER_TESTER_ENABLE
#define BQB_LOWER_TESTER_ENABLE										0
#endif



#ifndef HCI_NEW_FIFO_FEATURE_ENABLE
#define HCI_NEW_FIFO_FEATURE_ENABLE									1
#endif


#ifndef HCI_SEND_NUM_OF_CMP_AFT_ACK
#define HCI_SEND_NUM_OF_CMP_AFT_ACK									0
#endif


#ifndef L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE
#define L2CAP_DATA_2_HCI_DATA_BUFFER_ENABLE							0  //just for debug
#endif

#ifndef L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN
#define L2CAP_CREDIT_BASED_FLOW_CONTROL_MODE_EN     				0
#endif


#ifndef UPPER_TESTER_DBG_EN
#define UPPER_TESTER_DBG_EN											1
#endif

#ifndef UPPER_TESTER_HCI_LOG_EN
#define UPPER_TESTER_HCI_LOG_EN										1
#endif




