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
#ifndef STACK_BLE_DEBUG_DEBUG_CFG_H_
#define STACK_BLE_DEBUG_DEBUG_CFG_H_






/*****************************************************************************************************************

												Dump message

*****************************************************************************************************************/

#define			ERR_IGNORE									0
#define			ERR_TRIGGER_CODE_STUCK						1
#define			ERR_LOG_ON_SRAM								2

#define			BLT_ERR_PROCESS								ERR_LOG_ON_SRAM


#if (BLT_ERR_PROCESS == ERR_TRIGGER_CODE_STUCK || BLT_ERR_PROCESS == ERR_LOG_ON_SRAM)
	void blt_ll_error_debug(u32 x);
	#define 		BLMS_ERR_DEBUG(en, x)						if(en){blt_ll_error_debug(x);}
#else
	#define 		BLMS_ERR_DEBUG(en, x)
#endif

#define			BLMS_DEBUG_EN								0


/*****************************************************************************************************************

												Dump message

*****************************************************************************************************************/
#ifndef UPPER_TESTER_DBG_EN
#define UPPER_TESTER_DBG_EN											0
#endif

#ifndef UPPER_TESTER_HCI_LOG_EN
#define UPPER_TESTER_HCI_LOG_EN										0
#endif


#ifndef STACK_USB_LOG_EN
#define STACK_USB_LOG_EN											0
#endif

#ifndef STACK_DUMP_EN
#define STACK_DUMP_EN												0
#endif

#ifndef IUT_HCI_LOG_EN
#define IUT_HCI_LOG_EN												0
#endif

#ifndef HOST_HCI_ERR_LOG_EN
#define HOST_HCI_ERR_LOG_EN											0
#endif

#ifndef BLC_LL_LOG_EN
#define BLC_LL_LOG_EN												0
#endif


#ifndef CIS_FLOW_LOG_EN
#define CIS_FLOW_LOG_EN												0
#endif

#ifndef LL_CTRL_LOG_EN
#define LL_CTRL_LOG_EN												0
#endif


/* BLE smp trans.. log enable */
#ifndef	SMP_DBG_EN
#define SMP_DBG_EN												0
#endif

#ifndef	TX_PUSH_DATA_LOG
#define TX_PUSH_DATA_LOG										0
#endif

#ifndef	RX_L2CAP_DATA_LOG
#define RX_L2CAP_DATA_LOG										0
#endif

#ifndef DBG_HOST_LOG
#define DBG_HOST_LOG											0
#endif

#ifndef DBG_L2CAP_BTSNOOP_LOG
#define DBG_L2CAP_BTSNOOP_LOG									0
#endif

#ifndef DBG_GATT_LOG
#define DBG_GATT_LOG                                            0
#endif

#ifndef DBG_GAPC_LOG
#define DBG_GAPC_LOG											0
#endif

#ifndef	DBG_BOUNDARY_RX
#define DBG_BOUNDARY_RX											0
#endif

#ifndef	DBG_LL_CTRL_LOG_EN
#define DBG_LL_CTRL_LOG_EN									    0
#endif


#ifndef	DBG_IAL_EN
#define DBG_IAL_EN									   		 	0
#endif


#ifndef	DBG_PRVC_RL_EN
#define DBG_PRVC_RL_EN										0
#endif

#ifndef	DBG_PRVC_LEGADV_EN
#define DBG_PRVC_LEGADV_EN									0
#endif

#ifndef	DBG_PRVC_LEGSCAN_EN
#define DBG_PRVC_LEGSCAN_EN									0
#endif

#ifndef	DBG_PRVC_INIT_EN
#define DBG_PRVC_INIT_EN									0
#endif

#ifndef	DBG_PRVC_CONN_EN
#define DBG_PRVC_CONN_EN									0
#endif

#ifndef	DBG_PRVC_EXTADV_EN
#define DBG_PRVC_EXTADV_EN									0
#endif

#ifndef	DBG_PRVC_EXTSCAN_EN
#define DBG_PRVC_EXTSCAN_EN									0
#endif

#ifndef	DBG_EXTSCAN_REPORT
#define DBG_EXTSCAN_REPORT										0
#endif

#ifndef	DBG_EXTSCAN_ERR_PKT_EN
#define DBG_EXTSCAN_ERR_PKT_EN									0
#endif


#ifndef	DEB_CIG_MST_EN
#define DEB_CIG_MST_EN											0
#endif

#ifndef	DEB_CIG_SLV_EN
#define DEB_CIG_SLV_EN											0
#endif

#ifndef	DBG_CIS_DISCONN_EN
#define DBG_CIS_DISCONN_EN										0
#endif

#ifndef	DEB_BIG_BCST_EN
#define DEB_BIG_BCST_EN											0
#endif

#ifndef	DEB_BIG_SYNC_EN
#define DEB_BIG_SYNC_EN											0
#endif

#ifndef	DEB_BIG_SYNC_LL_DATA_EN
#define DEB_BIG_SYNC_LL_DATA_EN									0
#endif

#ifndef DBG_ISO_TEST_EN
#define	DBG_ISO_TEST_EN											0
#endif



#ifndef	DBG_CIS_TERMINATE
#define DBG_CIS_TERMINATE									   	0
#endif


#ifndef	DBG_CIS_1ST_AP_TIMING_EN
#define DBG_CIS_1ST_AP_TIMING_EN								0
#endif


#ifndef	DBG_NUM_COM_PKT
#define DBG_NUM_COM_PKT											0
#endif


#ifndef	DBG_CIS_PARAM
#define DBG_CIS_PARAM											0
#endif

#ifndef	DBG_CIS_CENTRAL_PARAM
#define DBG_CIS_CENTRAL_PARAM									0
#endif

#ifndef	DBG_SET_CIG_PARAMS
#define DBG_SET_CIG_PARAMS										0
#endif

#ifndef DBG_CIS_TX_DATA
#define	DBG_CIS_TX_DATA											0
#endif

#ifndef DBG_CIS_RX_DATA
#define	DBG_CIS_RX_DATA											0
#endif

#ifndef DBG_CIS_RX_DATA_FLOW_EN
#define	DBG_CIS_RX_DATA_FLOW_EN									0
#endif

#ifndef DBG_CIS_TX_DATA_FLOW_EN
#define	DBG_CIS_TX_DATA_FLOW_EN									0
#endif


#ifndef DBG_HCI_CIS_TEST
#define	DBG_HCI_CIS_TEST										0
#endif


#ifndef	DBG_SUBRATE_EN
#define DBG_SUBRATE_EN											0
#endif



#ifndef DBG_LL_PAST_EN
#define DBG_LL_PAST_EN                                          0
#endif

#ifndef DBG_LL_PCL_EN
#define DBG_LL_PCL_EN                                           0
#endif

#ifndef DBG_LL_CC_EN
#define DBG_LL_CC_EN                                            0
#endif


#ifndef DBG_CUSTOM_ACLC_TIMING
#define DBG_CUSTOM_ACLC_TIMING                                 	0
#endif



#ifndef DEB_BIG_SYNC_TIMESTAM_EN
#define DEB_BIG_SYNC_TIMESTAM_EN                                 	0
#endif





/*****************************************************************************************************************

												VCD

*****************************************************************************************************************/
#define	VCD_DEFINE_DEFAULT				1

#define	VCD_DEFINE_EXTENDED				5
#define	VCD_DEFINE_BIS_SYNC				6

#define VCD_DEFINE_CIS					10
#define VCD_DEFINE_CIS_PER				11
#define VCD_DEFINE_CIS_CEN				12
#define	VCD_FANQH_DEFINE_CIS			13

#define VCD_DEFINE_BIS					20


#define VCD_DEFINE_SIHUI				30



#ifndef VCD_DEFINE_SELECT
#define	VCD_DEFINE_SELECT				VCD_DEFINE_BIS_SYNC
#endif



/* BLE rf irq timing && log enable */
#ifndef SL_STACK_IRQ_TIMING_EN
#define	SL_STACK_IRQ_TIMING_EN				1
#endif

#ifndef SL_STACK_SCHE_TIMING_EN
#define	SL_STACK_SCHE_TIMING_EN				1
#endif

#ifndef SL_STACK_FSM_TIMING_EN
#define	SL_STACK_FSM_TIMING_EN				0
#endif


#ifndef SL_STACK_EXT_PRD_BASE_TIMING_EN
#define	SL_STACK_EXT_PRD_BASE_TIMING_EN		1
#endif


#ifndef SL_STACK_BIG_BCST_TIMING_EN
#define	SL_STACK_BIG_BCST_TIMING_EN			0
#endif


#ifndef SL_STACK_EXTSCAN_BASIC_TIMING_EN
#define	SL_STACK_EXTSCAN_BASIC_TIMING_EN	1
#endif


/* ACL connection */
#ifndef SL_STACK_ACL_BASIC_TIMING_EN
#define	SL_STACK_ACL_BASIC_TIMING_EN		0
#endif

#ifndef SL_STACK_ACL_TX_FIFO_EN
#define	SL_STACK_ACL_TX_FIFO_EN				0
#endif



#ifndef SL_STACK_CIS_BASIC_TIMING_EN
#define	SL_STACK_CIS_BASIC_TIMING_EN		1
#endif


#ifndef SL_STACK_CIS_RX_DATA_EN
#define	SL_STACK_CIS_RX_DATA_EN				1
#endif


#ifndef SL_STACK_CIS_TX_DATA_EN
#define	SL_STACK_CIS_TX_DATA_EN				1
#endif


#ifndef SL_STACK_BIS_SOURCE_TIMING_EN
#define SL_STACK_BIS_SOURCE_TIMING_EN		0
#endif


#ifndef SL_STACK_BIS_SINK_TIMING_EN
#define SL_STACK_BIS_SINK_TIMING_EN			1
#endif


#ifndef SL_STACK_BIS_RX_DATA_EN
#define SL_STACK_BIS_RX_DATA_EN				0
#endif





/* controller,BQB,IUT, critical command & event */
#define	SL_STACK_IUT_CMD_EVT				1




#define	SL_STACK_ISO_DATA_EN				1











#endif /* STACK_BLE_DEBUG_DEBUG_CFG_H_ */
