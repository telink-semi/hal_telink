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
#ifndef LL_STACK_H_
#define LL_STACK_H_

#include "stack/ble/ble_stack.h"
#include "stack/ble/ble_format.h"
#include "stack/ble/hci/hci_cmd.h"

#include "tl_common.h"
#include "drivers.h"

#define 		TASK_FIFO_REMOVE_LENGTH_SAVE_RAM					1
#define			ABANDONED_TASK_LINK_ENABLE							0
#define 		BS_SLOT_TIMIMG_ARCHITECTURE							1   //SIHUI BLE 5.2/multi_connecntion timing architecture



/******************************* conn_config start ******************************************************************/

#define			CONN_MAX_NUM_M0_S2									1
#define			CONN_MAX_NUM_M0_S4									2
#define			CONN_MAX_NUM_M1_S1									3
#define			CONN_MAX_NUM_M1_S2									4
#define			CONN_MAX_NUM_M1_S4									5
#define			CONN_MAX_NUM_M2_S0									6
#define			CONN_MAX_NUM_M2_S2									7
#define			CONN_MAX_NUM_M2_S4									8
#define			CONN_MAX_NUM_M4_S0									9
#define			CONN_MAX_NUM_M4_S2									10
#define			CONN_MAX_NUM_M4_S4									11


#ifndef			CONN_MAX_NUM_CONFIG
#define			CONN_MAX_NUM_CONFIG									CONN_MAX_NUM_M4_S4
#endif


#if (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M0_S2)
	#define			BLMS_MAX_CONN_NUM								2
	#define			BLMS_MAX_CONN_MASTER_NUM						0
	#define			BLMS_MAX_CONN_SLAVE_NUM							2

	#define 		BLMS_CONN_MASTER_EN								0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M0_S4)
	#define			BLMS_MAX_CONN_NUM								4
	#define			BLMS_MAX_CONN_MASTER_NUM						0
	#define			BLMS_MAX_CONN_SLAVE_NUM							4

	#define 		BLMS_CONN_MASTER_EN								0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M1_S1)
	#define			BLMS_MAX_CONN_NUM								2
	#define			BLMS_MAX_CONN_MASTER_NUM						1
	#define			BLMS_MAX_CONN_SLAVE_NUM							1
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M1_S2)
	#define			BLMS_MAX_CONN_NUM								3
	#define			BLMS_MAX_CONN_MASTER_NUM						1
	#define			BLMS_MAX_CONN_SLAVE_NUM							2
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M1_S4)
	#define			BLMS_MAX_CONN_NUM								3
	#define			BLMS_MAX_CONN_MASTER_NUM						1
	#define			BLMS_MAX_CONN_SLAVE_NUM							4
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M2_S0)
	#define			BLMS_MAX_CONN_NUM								2
	#define			BLMS_MAX_CONN_MASTER_NUM						2
	#define			BLMS_MAX_CONN_SLAVE_NUM							0

	#define			BLMS_CONN_SLAVE_EN								0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M2_S2)
	#define			BLMS_MAX_CONN_NUM								4
	#define			BLMS_MAX_CONN_MASTER_NUM						2
	#define			BLMS_MAX_CONN_SLAVE_NUM							2
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M2_S4)
	#define			BLMS_MAX_CONN_NUM								6
	#define			BLMS_MAX_CONN_MASTER_NUM						2
	#define			BLMS_MAX_CONN_SLAVE_NUM							4
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M4_S0)
	#define			BLMS_MAX_CONN_NUM								4
	#define			BLMS_MAX_CONN_MASTER_NUM						4
	#define			BLMS_MAX_CONN_SLAVE_NUM							0

	#define			BLMS_CONN_SLAVE_EN								0
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M4_S2)
	#define			BLMS_MAX_CONN_NUM								6
	#define			BLMS_MAX_CONN_MASTER_NUM						2
	#define			BLMS_MAX_CONN_SLAVE_NUM							4
#elif (CONN_MAX_NUM_CONFIG == CONN_MAX_NUM_M4_S4)
	#define			BLMS_MAX_CONN_NUM								8
	#define			BLMS_MAX_CONN_MASTER_NUM						4
	#define			BLMS_MAX_CONN_SLAVE_NUM							4
#endif



	#define			CONN_IDX_MASTER0								0
	#define			CONN_IDX_SLAVE0									BLMS_MAX_CONN_MASTER_NUM



#ifndef			BLMS_CONN_MASTER_EN
#define			BLMS_CONN_MASTER_EN									1
#endif

#ifndef			BLMS_CONN_SLAVE_EN
#define			BLMS_CONN_SLAVE_EN									1
#endif




//conn param update/map update
#ifndef	BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE
#define BLS_PROC_MASTER_UPDATE_REQ_IN_IRQ_ENABLE					0  //TODO:
#endif

/******************************* conn_config end ******************************************************************/



/******************************* ll_device start ******************************************************************/
#if (LL_MULTI_SLAVE_MAC_ENABLE)
	#define			SLAVE_DEV_MAX								3
#else
	#define			SLAVE_DEV_MAX								1
#endif
/******************************* ll_device end  ******************************************************************/



//TODO:  all debug MACRO should remove at last

#define			SCH_DEBUG_EN									0
#define			BLMS_DEBUG_EN									0

#if (BLMS_DEBUG_EN)
	int blt_ll_error_debug(u32 x);
	#define 		BLMS_ERR_DEBUG(en, x)						if(en){blt_ll_error_debug(x);}
#else
	#define			BLMS_DEBUG_EN								0
	#define 		BLMS_ERR_DEBUG(en, x)
#endif


#define			BLT_STRUCT_4B_ALIGN_CHECK_EN					0  //disable when release SDK, to clear warning


#define			SCAN_DEBUG_EN									0
#define			IAL_DEBUG_EN									0
#define			ACL_MASTER_SCHE_DEBUG							0
#define			ACL_MASTER_INITIATE								0
#define			SCHE_INSERT_DEBUG_EN							0  //sch_task_t structure changed, can not enable now
#define			SCHE_TIMING_IMPROVE_DBG_EN						0
#define			LEG_ADV_LINKLIST_DBG_EN							0


#define 		DBG_SLAVE_CONN_UPDATE							0
#define 		DBG_MASTER_CONN_UPDATE							0
#define 		DBG_CONN_UPDATE									(DBG_SLAVE_CONN_UPDATE || DBG_MASTER_CONN_UPDATE)

#define 		DBG_CIS_GPIO									0

#define 		DBG_HCI_FIFO									0

#define			DBG_PM_LOGIC									0
#define			DBG_PM_TIMIG									0


//Extended ADV debug
#define			DBG_EXTADV_LOGIC								0
#define			DBG_EXTADV_TIMING								0
#define			DBG_EXTADV_BUFFER								0

//Extended Scan debug
#define			DBG_EXTSCAN_LOGIC								0
#define			DBG_EXTSCAN_TIMING								0

#define			DBG_AUXSCAN_LOGIC_QW							0

//periodic ADV  ebug
#define			DBG_PRDADV_LOGIC								0
#define			DBG_PRDADV_TIMING								0

//periodic ADV Sync debug
#define			DBG_PDA_SYNC_LOGIC								0
#define			DBG_PDA_SYNC_TIMING								0


#define 		DBG_SLOT_CALCULTE_TIMING_EN						0


#define			DBG_BISNC_RX_PDU								0







#if(DBG_SLOT_CALCULTE_TIMING_EN)
	#define SLOTIME_EN_OTHER									1
	#define SLOTIME_EN_ADV										2
	#define SLOTIME_EN_ACL_CONN									3

	extern int	slot_timing_en;
	extern u32  slot_tick_begin;
#endif




#define			CRC_MATCH8(md,ms)	(md[0]==ms[0] && md[1]==ms[1] && md[2]==ms[2])


/******************************* ll start ***********************************************************************/

#define 		ADV_LEGACY_MASK									BIT(0)
#define 		ADV_EXTENDED_MASK								BIT(1)
#define 		SCAN_LEGACY_MASK								BIT(2)
#define 		SCAN_EXTENDED_MASK								BIT(3)
#define 		SET_RANDON_ADDR_CMD_MASK						BIT(4)

typedef	void (*hci_reset_callback_t)(void);
typedef	bool (*ll_adv_2_slave_callback_t)(rf_packet_connect_t *, bool);

typedef	struct {
	u8		num;	//not used now
	u8		mask;   //not used now
	u8		wptr;
	u8		rptr;
	u8*		p;
}scan_fifo_t;

typedef struct{
	//Private and DHkey use the same buffer, after obtaining dhkey, the private key content will be overwritten by dhkey
	u8 sc_sk_dhk_own[32];  //  own  private key[32]
	u8 sc_dhk_own[32];     //  own DHKey[32]
	u8 sc_pk_own[64];      //  own  public  key[64]
	u8 sc_pk_peer[64];     // peer  public  key[64]
}smp_sc_key_t;




extern _attribute_aligned_(4) smp_sc_key_t	smp_sc_key;
extern _attribute_aligned_(4) scan_fifo_t	scan_priRxFifo;
extern _attribute_aligned_(4) scan_fifo_t	scan_secRxFifo;

extern 	ll_adv_2_slave_callback_t 			ll_adv_2_slave_cb;


extern u8 glb_temp_rx_buff[];

/******************************* ll end *************************************************************************/




/******************************* BQB start ***********************************************************************/
#define			BQB_TEST_BIG_INTERVAL							0 //such as the interval = 4s

/******************************* BQB end *************************************************************************/


/******************************* ll_system start ***********************************************************************/


enum {
	FLAG_SCHEDULE_START					=	BIT(30),
	FLAG_SCHEDULE_DONE					=	BIT(29),
	FLAG_SCHEDULE_BUILD					=	BIT(28),

	FLAG_IRQ_RX							=	BIT(27),
	FLAG_IRQ_TX							=	BIT(26),


	FLAG_MODULE_MAINLOOP				= 	BIT(25),
	FLAG_MODULE_RESET					= 	BIT(24),
	FLAG_MODULE_SET_HOST_CHM			=	BIT(23),

	//ADV
	FLAG_SCHEDULE_SEND_EXTADV			= 	BIT(20),
	FLAG_SCHEDULE_SEND_AUXADV			= 	BIT(19),
	FLAG_SCHEDULE_EXTADV_INSERT			=	BIT(18),
	FLAG_SCHEDULE_LEGADV_INSERT			=	BIT(17),

	//SCAN
	FLAG_SCHEDULE_PRICHN_SCAN_INSERT	=	BIT(20),
	FLAG_SCHEDULE_SECCHN_SCAN_INSERT	=	BIT(19),
	FLAG_SCAN_DATA_REPORT				=	BIT(18),
	FLAG_EXT_SCAN_DATA_TRUNCATED		=	BIT(17),
	FLAG_SCHEDULE_EXTSCAN_DISABLE		=	BIT(16),



	//INIT
	FLAG_SCHEDULE_EXTINIT_CONN_PARAM_SET	=	BIT(20),
	FLAG_SCHEDULE_EXTINIT_CHECK_CONNRSP		=	BIT(19),


	//ACL
	FLAG_ACL_CONN_1						=	BIT(20),
	FLAG_ACL_CONN_2						=	BIT(19),
	FLAG_ACL_CONN_3						=	BIT(18),
	FLAG_ACL_CONN_4						=	BIT(17),
	FLAG_ACL_CONN_PARAM_UPDATE_CHECK	=	BIT(16),
	FLAG_ACL_SLAVE_CLEAR_SLEEP_LATENCY	=	BIT(15),
	FLAG_ACL_SLAVE_CHECK_UPDATE_CMD_DEC	=	BIT(14),


	//PERD_ADV & PERD_ADV_SYNC
	FLAG_SCHEDULE_AUX_SYNCINFO_UPDATE	=	BIT(20),
	FLAG_SCHEDULE_PRDADV_PARAM_UPDATE	=	BIT(19),
	FLAG_SCHEDULE_PRDADV_TASK_BEGIN		=	BIT(18),
	FLAG_AUXSCAN_PRDADV_SYNCINFO		=	BIT(17),
	FLAG_PRDADV_SYNC_RX					=	BIT(16),
	FLAG_PRDADV_DATA_REPORT				=	BIT(15),

	//CIG_MST
	FLAG_SCHEDULE_CIGMST_START				= 	BIT(20),
	FLAG_SCHEDULE_CISMST_START				= 	BIT(19),
	FLAG_SCHEDULE_CISMST_POST				= 	BIT(18),
	FLAG_SCHEDULE_CIGMST_INSERT				=	BIT(17),
	FLAG_SCHEDULE_CISMST_RX					=	BIT(16),
	FLAG_SCHEDULE_CISMST_ACL_TERM_CHECK		=   BIT(15),
	FLAG_SCHEDULE_CISMST_TX					=   BIT(14),


	//CIG_SLV
	FLAG_SCHEDULE_CIGSLV_START			= 	BIT(21),
	FLAG_SCHEDULE_CISSLV_START			= 	BIT(20),
	FLAG_SCHEDULE_CISSLV_POST			= 	BIT(19),
	FLAG_SCHEDULE_CIGSLV_INSERT			=	BIT(18),
	FLAG_SCHEDULE_CIGSLV_GET1ST_AP		=	BIT(17),
	FLAG_SCHEDULE_CISSLV_RX				=	BIT(16),
	FLAG_SCHEDULE_CISSLV_ACL_TERM_CHECK		=   BIT(15),
	FLAG_SCHEDULE_CISSLV_TX				=	BIT(14),


	//BIG_BCST
	FLAG_SCHEDULE_BIGBCST_START			= 	BIT(20),
	FLAG_SCHEDULE_BISBCST_START			= 	BIT(19),
	FLAG_SCHEDULE_BISBCST_POST			= 	BIT(18),
	FLAG_SCHEDULE_BIGBCST_INSERT		=	BIT(17),


	//BIG_SYNC
	FLAG_SCHEDULE_BIGSYNC_START			= 	BIT(20),
	FLAG_SCHEDULE_BISSYNC_START			= 	BIT(19),
	FLAG_SCHEDULE_BISSYNC_POST			= 	BIT(18),
	FLAG_SCHEDULE_BIGSYNC_INSERT		=	BIT(17),
	FLAG_SCHEDULE_BISSYNC_RX			=	BIT(16),
	FLAG_SCHEDULE_BISSYNC_SYNCINFOR		=   BIT(15),

	//ISOAL
	FLAG_ISOAL_MAINLOOP_CALLBACK		=   BIT(14),
	FLAG_ISOAL_RESET					=   BIT(13),
	FLAG_ISO_TEST_MAINLOOP_CALLBACK		=	BIT(12),
	FLAG_ISO_TEST_RESET					=	BIT(11),
	FLAG_ISO_TEST_TRANSMIT_CMD			=	BIT(10),
	FLAG_ISO_TEST_RECEIVE_CMD			=   BIT(9),
	FLAG_ISO_TEST_READ_COUNT_CMD		=   BIT(8),
	FLAG_ISO_TEST_END_CMD				=   BIT(7),



	FLAG_SCHEDULE_TASK_IDX_MASK			=	0x0F,  //0~15



};


void irq_system_timer(void);

u32  blt_ll_get_rx_packet_tick(u8 rf_len);
/******************************* ll_system end *************************************************************************/






/******************************* ll start **********************************************************************/

#define 	    BLMS_CONN_TIMING_EXTEND							1
#define			CONNECT_COMPLETE_CALLBACK_IN_MAINLOOP			1




#define			BLMS_STATE_NONE									0

#define			BLMS_STATE_SCHE_START							BIT(0)

#define			BLMS_STATE_ADV									BIT(1)
#define			BLMS_STATE_EXTADV_S								BIT(2)
#define			BLMS_STATE_EXTADV_E								BIT(3)

#define			BLMS_STATE_PRICHN_SCAN_S						BIT(4)	//primary channel scan start
#define			BLMS_STATE_PRICHN_SCAN_E						BIT(5)	//primary channel scan end
#define			BLMS_STATE_SECCHN_SCAN_S						BIT(6)	//secondary channel scan start
#define			BLMS_STATE_SECCHN_SCAN_E						BIT(7)	//secondary channel scan end
#define			BLMS_STATE_PDA_SYNC_S							BIT(8)	//periodic ADV sync start
#define			BLMS_STATE_PDA_SYNC_E							BIT(9)  //periodic ADV sync end

#define			BLMS_STATE_BTX_S								BIT(10)
#define			BLMS_STATE_BTX_E								BIT(11)
#define			BLMS_STATE_BRX_S								BIT(12)
#define			BLMS_STATE_BRX_E								BIT(13)


#define			BLMS_STATE_CIG_E								BIT(16)
#define			BLMS_STATE_CTX_S								BIT(17)  //CIS BTX Start
#define			BLMS_STATE_CTX_E								BIT(18)  //CIS BTX End
#define			BLMS_STATE_CRX_S								BIT(19)	 //CIS BRX Start
#define			BLMS_STATE_CRX_E								BIT(20)  //CIS BRX End


#define			BLMS_STATE_BIG_E								BIT(25)
#define			BLMS_STATE_BBCST_S								BIT(26)  //BIS BCST Start
#define			BLMS_STATE_BBCST_E								BIT(27)  //BIS BCST End
#define			BLMS_STATE_BSYNC_S								BIT(28)  //BIS SYNC Start
#define			BLMS_STATE_BSYNC_E								BIT(29)  //BIS SYNC End



#define 		BLMS_FLG_RF_CONN_DONE 		 					(FLD_RF_IRQ_CMD_DONE  | FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_RX_TIMEOUT | FLD_RF_IRQ_RX_CRC_2)


#define			SYS_IRQ_TRIG_NEW_TASK							0

#define			SYS_IRQ_TRIG_BTX_POST							BIT(0)
#define			SYS_IRQ_TRIG_BRX_POST							BIT(1)
#define			SYS_IRQ_TRIG_PRICHN_SCAN_POST					BIT(2)
#define			SYS_IRQ_TRIG_SECCHN_SCAN_POST					BIT(3)
#define			SYS_IRQ_TRIG_PDA_SYNC_POST						BIT(4)  //periodic ADV sync

#define			SYS_IRQ_TRIG_CIG_POST							BIT(6)
#define			SYS_IRQ_TRIG_CTX_START							BIT(7)
#define			SYS_IRQ_TRIG_CTX_POST							BIT(8)
#define			SYS_IRQ_TRIG_CRX_START							BIT(9)
#define			SYS_IRQ_TRIG_CRX_POST							BIT(10)

#define			SYS_IRQ_TRIG_EXTADV_SEND						BIT(12)

#define			SYS_IRQ_TRIG_BIG_POST							BIT(16)
#define			SYS_IRQ_TRIG_BIS_TX_START						BIT(17)
#define			SYS_IRQ_TRIG_BIS_TX_POST						BIT(18)
#define			SYS_IRQ_TRIG_BIS_RX_START						BIT(19)
#define			SYS_IRQ_TRIG_BIS_RX_POST						BIT(20)

#define			SYS_IRQ_TRIG_SCHE_START							BIT(24)


#if (MCU_CORE_TYPE == MCU_CORE_9518)

#else
	/* Different process for different MCU: ******************************************/
	extern u32 				Crc24Lookup[16];
	/*********************************************************************************/
#endif
_attribute_aligned_(4)
typedef struct {
	u8		macAddress_public[6];
	u8		macAddress_random[6];   //host may set this
}ll_mac_t;
extern _attribute_aligned_(4)  ll_mac_t  bltMac;




typedef union {
	struct{
		u8		leg_scan_en;	 //legacy scan
		u8		leg_init_en; 	 //legacy initiate
		u8		ext_scan_en;	 //extended scan
		u8		ext_init_en; 	 //extended initiate
	};
	struct{
		u16		leg_scan_init_en; //legacy scan or legacy initiate
		u16		ext_scan_init_en; //extended scan or extended initiate
	};
	u32 scn_init_en_pack;
}scn_init_en_t;

_attribute_aligned_(4)
typedef struct {

//0x00
	scn_init_en_t		scanInitEn_union;

	u8		leg_adv_en;      //legacy ADV
	u8		ext_adv_en;      //Extended ADV
	u8		state_chng;      //state change, set to 1 can only be execute in main_loop !!!
  	u8		state_flag;


	u8		pda_syncing_flg;
	u8		stimer_irq_process_en;
	u8		rf_fsm_busy;   //RF state machine busy
	u8		create_connection;



	u8		connUptCmd_pending;
	u8		connUpdate_busy;
	u8		connSync;				//if more than 8 connections, u8 -> u16
	u8		new_conn_forbidden;


//0x10
	u8		newConn_forbidden_master;
	u8		newConn_forbidden_slave;
	u8		sche_run_flag;
	u8		sdk_mainloop_flg;


	u8		max_master_num;
	u8		max_slave_num;
	u8		cur_master_num;
	u8		cur_slave_num;

	u8		connectEvt_mask;
	u8		disconnEvt_mask;
	u8		conupdtEvt_mask;  //conn_update
	u8		phyupdtEvt_mask;

	u8		getP256pubKeyEvtPending;
	u8		getP256pubKeystatus;
	u8		generateDHkeyEvtPending;
	u8		generateDHkeyStatus;

  	u8		cis_rx_dma_size;
  	u8		acl_rx_dma_size;
  	u8		perd_adv_sche_build_pending;
  	u8		iso_packet_length;  //7.8.2 LE Read Buffer Size command [v2]

//0x20
  	u8		controller_stack_param_check;
  	u8		u8_rsvd3;
  	u8		bis_rx_dma_size;
  	u8		big_sche_build_pending;

  	u8		maxRxOct;
  	u8		maxTxOct;
  	u8		maxTxOct_master;
  	u8		maxTxOct_slave;

  	u8		acl_master_en;
  	u8		acl_slave_en;
  	u8		acl_packet_length;  //7.8.2 LE Read Buffer Size command [v1]
  	u8		hci_cmd_mask;

  	u8		cig_mst_en;
  	u8      cig_slv_en;
  	u8		big_bcst_en;
  	u8		big_sync_en;

//0x30
  	u8		cig_slv_1st_sche_build_pending; //cig slave used only
  	u8		cis_create_pending;  //TODO: for cis master/slave, set and clear this flag
	u8		delay_clear_rf_status;
	u8		connUpdate_master_busy;

  	//TODO: remember one thing, dma bug found by Qiuwei/Lijing,  re_fix it
  	u32		cis_rx_dma_buff;   	//TODO: when CIS interrupt by other task, need modify
  	u32		acl_rx_dma_buff;

//0x40
  	u32		bis_rx_dma_buff;   	//TODO: when BIS interrupt by other task, need modify
  	u32     dly_start_tick_clr_rf_sts;

//0x50
  	u32 	bSlot_first_anchor;
	u32		connDleSendTimeUs;

} st_ll_para_t;

extern _attribute_aligned_(4)	st_ll_para_t  blmsParam;




_attribute_aligned_(4)
typedef struct {
	u32 rx_irq_tick;
	u32 rx_header_tick;
	u32 rx_timeStamp;
} rx_pkt_sts_t;

extern _attribute_aligned_(4)	rx_pkt_sts_t  bltRxPkt;




_attribute_aligned_(4)
typedef struct {
  	u32		ll_aclRxFifo_set	: 1;
  	u32		ll_aclTxMasFifo_set	: 1;
  	u32		ll_aclTxSlvFifo_set	: 1;
  	u32		hci_aclRxFifo_set	: 1;
  	u32		hci_isoRxFifo_set	: 1;
  	u32     ll_cisRxFifo_set	: 1;
  	u32		ll_cisTxFifo_set	: 1;
  	u32		ll_cisRxEvtFifo_set	: 1;
  	u32		ll_bisRxEvtFifo_set : 1;
  	u32		ll_bisTxFifo_set	: 1;
  	u32		ll_bisRxFifo_set	: 1;
  	u32     rfu					: 21;
}st_ll_temp_para_t;

extern _attribute_aligned_(4)	st_ll_temp_para_t  bltempParam;


extern	volatile	u32	blms_state;
extern	volatile    int blm_btxbrx_state;
extern				u8	blms_tx_empty_packet[];
extern	volatile	u32	systick_irq_trigger;

#if (MCU_CORE_TYPE == MCU_CORE_9518)

#else
	extern  volatile	u32 rftx_dma_conflic_flag;
	extern  volatile	u32 rftx_dma_conflic_tick;
#endif

typedef 	int (*ll_host_mainloop_callback_t)(void);
typedef 	int (*ll_enc_done_callback_t)(u16 connHandle, u8 status, u8 enc_enable);
typedef 	int (*ll_conn_complete_handler_t)(u16 conn, u8 *p);
typedef 	int (*ll_conn_terminate_handler_t)(u16 conn, u8 *p);
typedef 	int (*blc_main_loop_phyTest_callback_t)(void);
typedef		int	(*ll_iso_test_callback_t)(int flag, u16 handle, void *arg);

extern 	ll_host_mainloop_callback_t 			ll_host_main_loop_cb;
extern 	ll_enc_done_callback_t 	 				ll_encryption_done_cb;
extern 	ll_conn_complete_handler_t 				ll_connComplete_handler;
extern 	ll_conn_terminate_handler_t 			ll_connTerminate_handler;
extern 	blc_main_loop_phyTest_callback_t		blc_main_loop_phyTest_cb;


typedef 	int (*ll_fsm_op_s_callback_t)(int);
typedef 	int (*ll_fsm_op_p_callback_t)(void);

typedef 	int (*ll_task_callback_t)(int);
typedef 	int (*ll_task_callback_2_t)(int, void*p);

typedef		int (*ll_ext_scan_truncate_pend_t)(void);

extern	ll_fsm_op_s_callback_t					ll_btx_start_cb;
extern	ll_fsm_op_p_callback_t					ll_btx_post_cb;

extern	ll_task_callback_t						ll_acl_conn_irq_task_cb;
extern	ll_task_callback_t						ll_acl_conn_mlp_task_cb;

extern	ll_task_callback_t						ll_acl_slave_irq_task_cb;

extern	ll_task_callback_t						ll_cig_mst_irq_task_cb;
extern	ll_task_callback_2_t					ll_cig_mst_mlp_task_cb;

extern	ll_task_callback_t						ll_cig_slv_irq_task_cb;
extern	ll_task_callback_t						ll_cig_slv_mlp_task_cb;

extern	ll_task_callback_t						ll_leg_adv_irq_task_cb;
extern	ll_task_callback_t						ll_leg_adv_mlp_task_cb;
extern	ll_task_callback_t						ll_ext_adv_irq_task_cb;
extern	ll_task_callback_t						ll_ext_adv_mlp_task_cb;

extern	ll_task_callback_t						ll_leg_scan_irq_task_cb;
extern	ll_task_callback_t						ll_leg_scan_mlp_task_cb;
extern	ll_task_callback_t						ll_ext_scan_irq_task_cb;
extern	ll_task_callback_t						ll_ext_scan_mlp_task_cb;


extern	ll_task_callback_t						ll_init_mlp_task_cb;
extern	ll_task_callback_t						ll_ext_init_irq_task_cb;

extern	ll_task_callback_t						ll_prd_adv_irq_task_cb;
extern	ll_task_callback_2_t					ll_prd_adv_mlp_task_cb;
extern	ll_task_callback_2_t					ll_pda_sync_irq_task_cb;
extern	ll_task_callback_t						ll_pda_sync_mlp_task_cb;

extern	ll_task_callback_t						ll_big_bcst_irq_task_cb;
extern	ll_task_callback_2_t					ll_big_bcst_mlp_task_cb;

extern	ll_task_callback_2_t					ll_big_sync_irq_task_cb;
extern	ll_task_callback_t						ll_big_sync_mlp_task_cb;

extern	ll_task_callback_t						ll_secchn_scan_task_cb;

extern  ll_iso_test_callback_t 					ll_iso_test_main_loop_cb;


typedef 	int (*ll_rpa_tmo_mainloop_callback_t)(void);
typedef 	int (*ll_isoal_mainloop_callback_t)(int flag);

void 		blc_ll_registerRpaTmoMainloopCallback (ll_rpa_tmo_mainloop_callback_t cb);
void 		blt_ll_registerIsoalMainloopCallback (ll_isoal_mainloop_callback_t cb);
void 		blt_ll_registerIsoalTestMainloopCallback (ll_iso_test_callback_t cb);




typedef 	int (*ll_irq_rx_callback_t)(void);

extern		ll_irq_rx_callback_t			ll_irq_scan_rx_pri_chn_cb;
extern		ll_irq_rx_callback_t			ll_irq_scan_rx_sec_chn_cb;




void		smemset4(int * dest, int val, int len);



void 		blt_ll_registerHostMainloopCallback (ll_host_mainloop_callback_t cb);
void 		blt_ll_registerConnectionEncryptionDoneCallback(ll_enc_done_callback_t  cb);
void 		blt_ll_registerConnectionCompleteHandler(ll_conn_complete_handler_t  handler);
void 		blt_ll_registerConnectionTerminateHandler(ll_conn_terminate_handler_t  handler);



u8 			blt_ll_push_hold_data(u16 connHandle);
void  		blt_ll_setEncryptionBusy(u16 connHandle, u8 enc_busy);
int  		blt_ll_isEncryptionBusy(u16 connHandle);
u8 			blt_ll_pushTxfifoHold (u16 connHandle, u8 *p);
u8  		blt_ll_smpPushEncPkt (u16 connHandle, u8 type);
void 		blt_ll_smpSecurityProc(u16 connHandle);
void 		blt_ll_procDlePending(u16 connHandle);

u8			blt_ll_getCurrentState(void);
u8 			blt_ll_getOwnAddrType(u16 connHandle);
u8*			blt_ll_getOwnMacAddr(u16 connHandle, u8 addr_type);
void 		blt_ll_procGetP256pubKeyEvent (void);
void 		blt_ll_procGenDHkeyEvent (void);

ble_sts_t 	blc_ll_genRandomNumber(u8 *dest, u32 size);
ble_sts_t 	blc_ll_encryptedData(u8*key, u8*plaintextData, u8* encrypteTextData);
ble_sts_t   blc_ll_getP256pubKey(void);
ble_sts_t 	blc_ll_generateDHkey (u8* remote_public_key, bool use_dbg_key);

/******************************* ll end ************************************************************************/






/******************************* llms_slot start ******************************************************************/

#define			SLOT_UPDT_SLAVE_CONN_CREATE						BIT(0)
#define			SLOT_UPDT_SLAVE_CONN_UPDATE						BIT(1)
#define			SLOT_UPDT_SLAVE_SYNC_DONE						BIT(2)
#define			SLOT_UPDT_SLAVE_SSLOT_ADJUST					BIT(3)

#define			SLOT_UPDT_SLAVE_CONNUPDATE_FAIL					BIT(4)




#define			SLOT_UPDT_MASTER_CONN							BIT(6)
#define			SLOT_UPDT_CONN_TERMINATE						BIT(7)

#define			SLOT_UPDT_CIS_MASTER_CREATE						BIT(8)
#define			SLOT_UPDT_CIS_MASTER_CHANGE						BIT(9)   //CIS in CIG add or remove
#define			SLOT_UPDT_CIS_MASTER_REMOVE						BIT(10)
#define			SLOT_UPDT_CIS_SLAVE_CREATE						BIT(11)
#define			SLOT_UPDT_CIS_SLAVE_TERMINATE					BIT(12)

//#define			SLOT_UPDT_PERD_ADV_CREATE						BIT(13)
//#define			SLOT_UPDT_PERD_ADV_REMOVE						BIT(14)
#define			SLOT_UPDT_BIS_BCST_CREATE						BIT(15)
#define			SLOT_UPDT_BIS_BCST_REMOVE						BIT(16)
#define			SLOT_UPDT_BIS_BSYNC_CREATE						BIT(17)
#define			SLOT_UPDT_BIS_BSYNC_REMOVE						BIT(18)

#define			SLOT_UPDT_PRDADV_CREATE_SYNC					BIT(20)

#define			SLOT_UPDT_EXT_SCAN_DISABLE						BIT(25)
#define			SLOT_UPDT_ADV_SACN_STATE_CHANGE					BIT(26)



// 150us(T_ifs) + 352us(conn_req) = 502 us,  sub some margin 22(RX IRQ delay/irq_handler delay)
// real test data: 470 uS,  beginning is "u32 tick_now = rx_tick_now = clock_time();" in irq_acl_conn_rx
//						    ending is    "while( !(reg_rf_irq_status & FLD_RF_IRQ_TX));" in
//                          "irq_handler" to "u32 tick_now = rx_tick_now = clock_time();" is 4 uS
#define			PKT_INIT_AFTER_RX_TICK							( 480 *SYSTEM_TIMER_TICK_1US)

//scan_req(12B)+150us+scan_rsp(6+31) = (12+10)*8 + 150 + (37+10)*8;;;10 = preamble+accessCode+mic+crc
#define         ACKTIVE_SCAN_MAX_TICK                           (720*SYSTEM_TIMER_TICK_1US)//( 702*SYSTEM_TIMER_TICK_1US)

// master: 30    mS * 4 , slave:  8.75 mS/10   mS/18.75 mS  276 uS, 20190921
// master: 30    mS * 4 , slave:  7.5  mS/10   mS/18.75 mS  310 uS, 20190921
// master: 31.25 mS * 4 , slave:  7.5  mS/10   mS/18.75 mS  339 uS, 20190921
#define			SLOT_PROCESS_MAX_US								400
#define			SLOT_PROCESS_MAX_TICK							( 400 *SYSTEM_TIMER_TICK_1US)//( 400 *SYSTEM_TIMER_TICK_1US)
#define			SLOT_PROCESS_MAX_SSLOT_NUM						21 //400uS -> 20.48 sSlot
//TODO(SiHui): optimize later, not use a constant value, use a variable which relative with how many task alive now



// test data 88 uS 20191014 by SiHui, consider application layer flash read or flash write timing, need add some margin
#define			SCAN_BOUNDARY_MARGIN_COMMON_TICK				( 100 *SYSTEM_TIMER_TICK_1US + SLOT_PROCESS_MAX_TICK)
#define			SCAN_BOUNDARY_MARGIN_INIT_TICK					( PKT_INIT_AFTER_RX_TICK + SLOT_PROCESS_MAX_TICK)	  // initiate timing + slot_update_rebuild_allocate running potential biggest time




#define			BOUNDARY_RX_RELOAD_TICK							0  //new design, abandon all boundary RX(Eagle RX dma_len rewrite problem)



#define			SLOT_INDEX_INIT									0
#define			SLOT_INDEX_ALARM_LOW							BIT(31)     //15 Days
#define			SLOT_INDEX_ALARM_HIGH							0xFFFF0000  //30 Days        //(BIT(31)|BIT(30))   //23 Days






#define 		IRQ_BTX_DELAY_US								50

#define 		BTX_SEND_DELAY_US								130   // 1M: 87+40 =127; 2M: 117+16=133; Coded: 125

#define 		PAYLOAD_27B_1MPHY_US							328   // (31 + 10) * 8 = 328


#define 		PAYLOAD_27B_TIFS_27B_1MPHY_US					806   // (328*2 + 150) = 656 + 150 = 806
#define 		PAYLOAD_27B_TIFS_27B_1MPHY_SSLOT_NUM			41    // 806/19.53 = 41.27

#define 		PAYLOAD_27B_CODEDPHY_S8_US




/******************************* llms_slot end ********************************************************************/









/******************************* ll_schedule start ******************************************************************/

//#define			SLOT_SHIFT_NUM									3  		//BIT<7:3> is slot task type
#define			SCHTASK_IDX_MSK									0x03    //BIT<1:0> is slot task index

#define			SAMLL_SLOT_10US_EN								0  //1: 10uS;  0 : 20uS


#define 		MAX_CONFILICT_NUM								4



//task flag
//attention: can not be 0, 0 have other use
#define			TSKFLG_ACL_MASTER								1
#define			TSKFLG_ACL_SLAVE								2
#define			TSKFLG_CIG_MST									3
#define			TSKFLG_CIG_SLV									4
#define			TSKFLG_LEG_ADV									5
#define			TSKFLG_EXT_ADV									6
#define			TSKFLG_AUX_ADV									7
#define			TSKFLG_PERD_ADV									8
#define			TSKFLG_PRICHN_SCAN								9		//primary channel scan
#define			TSKFLG_SECCHN_SCAN								10		//secondary channel scan
#define			TSKFLG_PDA_SYNC									11
#define			TSKFLG_BIG_BCST									12
#define			TSKFLG_BIG_SYNC									13




#define			TSKFLG_VALID_MASK								0x7F
#define			TSKFLG_BSLOT_ALIGN								BIT(7)

/*******************************************************************************
	00 ~ 03 :  ACL master
	04 ~ 07 :  ACL slave
	08      :  CIG master
	09 ~ 10	:  CIG slave
	11      :  Leg ADV
	12 ~ 14 :  Ext_ADV      0x2000 0x1000 0x080
	15 ~ 17 :  Aux_ADV
	18 ~ 19 :  Periodic ADV
	20		:  Primary channel Scan(leg_scan & Ext_Scan)
	21 ~ 25 :  secondary channel Scan
	26 ~ 27 :  Periodic ADV Sync
	28 ~ 29 :  BIS Bcst
	30 ~ 31 :  BIS Sync
 ******************************************************************************/
//max task number
#define			TSKNUM_ACL_MASTER								BLMS_MAX_CONN_MASTER_NUM
#define			TSKNUM_ACL_SLAVE								BLMS_MAX_CONN_SLAVE_NUM

#define			TSKNUM_CIG_MST									1
#define			TSKNUM_CIG_SLV									2

#define			TSKNUM_LEG_ADV									1
#define			TSKNUM_EXT_ADV									4
#define			TSKNUM_AUX_ADV									4
#define			TSKNUM_PERD_ADV									2
#define			TSKNUM_PRICHN_SCAN								1

#define			TSKNUM_SECCHN_SCAN								3
#define			TSKNUM_PDA_SYNC									2	//PERIODIC_ADV_SYNC
#define			TSKNUM_BIG_BCST									2
#define			TSKNUM_BIG_SYNC									2


#define 		TSKNUM_MAX 										(TSKNUM_ACL_MASTER + TSKNUM_ACL_SLAVE + TSKNUM_CIG_MST + 					          \
																 TSKNUM_CIG_SLV + TSKNUM_LEG_ADV + TSKNUM_EXT_ADV + TSKNUM_AUX_ADV +        	  \
																 TSKNUM_PERD_ADV + TSKNUM_PRICHN_SCAN + TSKNUM_SECCHN_SCAN + TSKNUM_PDA_SYNC + \
																 TSKNUM_BIG_BCST + TSKNUM_BIG_SYNC)

//task offset
#define			TSKOFT_ACL_CONN									0
#define			TSKOFT_ACL_MASTER								0
#define			TSKOFT_ACL_SLAVE								( TSKNUM_ACL_MASTER )
#define			TSKOFT_CIG_MST									( TSKOFT_ACL_SLAVE  	+ TSKNUM_ACL_SLAVE )
#define			TSKOFT_CIG_SLV									( TSKOFT_CIG_MST 		+ TSKNUM_CIG_MST )
#define			TSKOFT_LEG_ADV									( TSKOFT_CIG_SLV  		+ TSKNUM_CIG_SLV )
#define			TSKOFT_EXT_ADV									( TSKOFT_LEG_ADV   		+ TSKNUM_LEG_ADV )
#define			TSKOFT_AUX_ADV									( TSKOFT_EXT_ADV    	+ TSKNUM_EXT_ADV )
#define			TSKOFT_PERD_ADV									( TSKOFT_AUX_ADV    	+ TSKNUM_AUX_ADV )
#define			TSKOFT_PRICHN_SCAN								( TSKOFT_PERD_ADV  		+ TSKNUM_PERD_ADV )
#define			TSKOFT_SECCHN_SCAN								( TSKOFT_PRICHN_SCAN    + TSKNUM_PRICHN_SCAN )
#define			TSKOFT_PDA_SYNC									( TSKOFT_SECCHN_SCAN  	+ TSKNUM_SECCHN_SCAN )
#define			TSKOFT_BIG_BCST									( TSKOFT_PDA_SYNC 	+ TSKNUM_PDA_SYNC )
#define			TSKOFT_BIG_SYNC									( TSKOFT_BIG_BCST      	+ TSKNUM_BIG_BCST )


//task mask begin
#define			TSKMSK_ACL_CONN_0								BIT(0)
#define			TSKMSK_ACL_MASTER_0								BIT(0)
#define			TSKMSK_ACL_SLAVE_0								BIT(BLMS_MAX_CONN_MASTER_NUM)
#define			TSKMSK_CIG_MASTER_0								BIT(TSKOFT_CIG_MST)
#define			TSKMSK_CIG_SLAVE_0								BIT(TSKOFT_CIG_SLV)
#define			TSKMSK_EXT_ADV_0								BIT(TSKOFT_EXT_ADV)
#define			TSKMSK_AUX_ADV_0								BIT(TSKOFT_AUX_ADV)
#define			TSKMSK_PERD_ADV_0								BIT(TSKOFT_PERD_ADV)
#define			TSKMSK_SECCHN_SCAN_0							BIT(TSKOFT_SECCHN_SCAN)
#define			TSKMSK_PDA_SYNC_0								BIT(TSKOFT_PDA_SYNC)
#define			TSKMSK_BIG_BCST_0								BIT(TSKOFT_BIG_BCST)
#define			TSKMSK_BIG_SYNC_0								BIT(TSKOFT_BIG_SYNC)


//task mask
#define			TSKMSK_ACL_CONN_ALL								((1<<BLMS_MAX_CONN_NUM) - 1)          //BIT_RNG(TSKOFT_ACL_CONN,   BLMS_MAX_CONN_NUM-1 )
#define			TSKMSK_ACL_MASTER								((1<<BLMS_MAX_CONN_MASTER_NUM) - 1)   //BIT_RNG(TSKOFT_ACL_MASTER, BLMS_MAX_CONN_MASTER_NUM-1 )
#define			TSKMSK_ACL_SLAVE								BIT_RNG(TSKOFT_ACL_SLAVE, 		TSKOFT_CIG_MST - 1)
#define			TSKMSK_CIG_MASTER								BIT_RNG(TSKOFT_CIG_MST,			TSKOFT_CIG_SLV - 1)
#define			TSKMSK_CIG_SLAVE								BIT_RNG(TSKOFT_CIG_SLV,			TSKOFT_LEG_ADV - 1)
#define			TSKMSK_LEG_ADV									BIT_RNG(TSKOFT_LEG_ADV,			TSKOFT_EXT_ADV - 1)
#define			TSKMSK_EXT_ADV									BIT_RNG(TSKOFT_EXT_ADV,  		TSKOFT_AUX_ADV - 1)
#define			TSKMSK_AUX_ADV									BIT_RNG(TSKOFT_AUX_ADV,  		TSKOFT_PERD_ADV - 1)
#define			TSKMSK_PERD_ADV									BIT_RNG(TSKOFT_PERD_ADV,		TSKOFT_PRICHN_SCAN - 1)
#define			TSKMSK_PRICHN_SCAN								BIT_RNG(TSKOFT_PRICHN_SCAN,		TSKOFT_SECCHN_SCAN - 1)
#define			TSKMSK_SECCHN_SCAN								BIT_RNG(TSKOFT_SECCHN_SCAN,		TSKOFT_PDA_SYNC - 1)
#define			TSKMSK_PDA_SYNC									BIT_RNG(TSKOFT_PDA_SYNC,		TSKOFT_BIG_BCST - 1)
#define			TSKMSK_BIG_BCST									BIT_RNG(TSKOFT_BIG_BCST, 	  	TSKOFT_BIG_SYNC - 1)
#define			TSKMSK_BIG_SYNC									BIT_RNG(TSKOFT_BIG_SYNC, 	  	TSKNUM_MAX - 1)





_attribute_aligned_(4)
typedef struct sch_task_t{
	//u16, 65536*20uS = 1.3S most;
	//u32, 65536*65536/2/625 = 3435973 unit most, 6871947*19.5uS=67 S most
	s32  begin; //right align
	s32  end;   //left align

#if (TASK_FIFO_REMOVE_LENGTH_SAVE_RAM)
	u16	 u16_rsvd;
#else
	u16	 length; //65536*19.5uS = 1.27 S most, 256*19.5 = 4.992 mS
#endif

	u16  event_index;


	u8	 scheTask_oft;  // 0~31, 5 bit enough
	u8	 scheTask_idx;  //max 4 now, 4 master/4 slave/4 CIS, consider future 16 master, 16 or 32 ?
	u8	 scheTask_flg;  //5 bit is enough
	u8	 taskFifo_idx;  //used in ADV now

	struct sch_task_t  *next;
} sch_task_t;

extern sch_task_t	bltSche_header;


/********************************************************************************************
 * 0.625 mS slot
 * 0.000625 S * 2^32 = 0.000625 S*65536*65536 = 2684354 S = 745 Hours = 31 Days
 *******************************************************************************************/

//big   slot: 625 uS
//small slot: 625 uS/32 = 312.5 tick, 19.5uS

_attribute_aligned_(4)
typedef struct{
//0x00
	u8	sSlot_idx_reset;
	u8	addTsk_idx;   	 //task index of add task
	u8	build_index;
	u8	bSlot_maxLen;

	u8	sSlot_diff_irq;
	u8	task_type_num;
	u8	sSlot_diff_next;
	u8	sche_process_en;

	u8	lklt_taskNum; //link list task number
	u8	abandon_taskNum;
	u8	insertTsk_flag;  //task flag of insert task
	u8	insertTsk_idx;   //task index of insert task

	u8	immediate_task;
	u8	u8_rsvd[3];


//0x10
	s32	sSlot_endIdx_dft;  	// right align
	u32	update;
	u32 task_en;
	u32 task_mask;

//0x20
	u32	bSlot_idx_start;
	u32	bSlot_idx_next;
	u32 bSlot_idx_irq_real;
	u32	bSlot_tick_start;

	u32	bSlot_tick_irq_real;
	s32	sSlot_idx_next;
	s32	sSlot_idx_past;
	s32 sSlot_idx_irq_real;

//0x40
	u32	sSlot_tick_start;
	u32	sSlot_tick_next;
	u32 sSlot_tick_irq;
	u32	sSlot_tick_irq_real;

//0x50
	u32	bSlot_endIdx_dft;  	// right align
	s32	sSlot_endIdx_maxPri;// right align
	u32	bSlot_endIdx_real;  // right align
	u32	sSlot_extend_num;

//0x60
	u32	system_irq_tick;
	u32 lklt_endTick;	  //  link_list end tick
	u32	task_end_tick;
	u32 lastTsk_endTick;  //not used now. end tick of last task on link_list

//0x70
	sch_task_t	*pTask_head;
	sch_task_t	*pTask_pre;  //now not used
	sch_task_t	*pTask_next;
	sch_task_t	*pTask_cur;


#if (ABANDONED_TASK_LINK_ENABLE)
	sch_task_t	*pTask_abandon_head;
	sch_task_t	*pTask_abandon_cur;
#endif


//	s32 sSlot_idx_irq;  //not used now
//	u32 bSlot_idx_irq;   //bug: never used, TODO SiHui
//	s32	sSlot_idx_start;  //it's always 0, so no need set
//  u32 bSlot_tick_next;  //not used now
//	u32 bSlot_tick_irq;       //not used now

}sch_man_t;

extern sch_man_t		bltSche;






#define	 FUTURE_TASK_MAX_NUM		 (TSKNUM_AUX_ADV + TSKNUM_SECCHN_SCAN)

_attribute_aligned_(4)
typedef struct {
	u8	task_flg;  //scheTask_flg
	u8	task_oft;
	u8	u8_rsvd[2];

	u32 tick_s;
	u32 tick_e;
}future_task_e;

_attribute_aligned_(4)
typedef struct {
	future_task_e task_tbl[FUTURE_TASK_MAX_NUM];

	u8	number;
}ll_future_task_t;
extern ll_future_task_t	bltFutTask;




#define PRI_TASK_NUM						TSKNUM_MAX

typedef signed short	pri_data_t;

typedef enum{
	TASK_PRIORITY_LOW			=	10,

	TASK_PRIORITY_MID			=	100,


	TASK_PRIORITY_PDA_SYNCED_FIRST = 200,


	TASK_PRIORITY_HIGH_THRES	=	220,

	TASK_PRIORITY_AUX_ADV		=	225,
	TASK_PRIORITY_AUX_SCAN_DFT	=	225,

	TASK_PRIORITY_PERD_ADV_DFT	=	228,



	TASK_PRIORITY_CONN_CREATE	=	230,

	TASK_PRIORITY_CONN_UPDATE	=	240,
	TASK_PRIORITY_PDA_SYNCING_DFT	=	240,

	TASK_PRIORITY_MAX			=	500,  // bigger than TASK_PRIORITY_CONN_UPDATE + CONN_INTERVAL_100MS
}task_pri_t;


typedef struct pri_mng_t{
	pri_data_t  pri_now[PRI_TASK_NUM];
	pri_data_t	pri_cal[PRI_TASK_NUM];
	pri_data_t	priMax_value;
	u8  step_final[PRI_TASK_NUM];
	u8  step_intvl[PRI_TASK_NUM];
//	u8  step_set[PRI_TASK_NUM];
	u8	priMax_index;  //now not used

} pri_mng_t;

extern pri_mng_t bltPri;




static inline void blt_ll_setSchedulerTaskPriority(u8 task_offset, pri_data_t pri)
{
	bltPri.pri_now[task_offset] = pri;
}
void blt_ll_incSchedulerTaskPriority(u8 task_offset, int inc);
void blt_ll_incSchedulerTaskCalPriority  (u8 task_offset, int inc);



static inline u8 blt_slot_getConnSlotPriority(u8 task_offset){
	return 0;
}







typedef int (*ll_sche_linklist_callback_t)(void);
extern		ll_sche_linklist_callback_t		ll_sche_linklist_acl_master_cb;


int		blt_ll_mainloop_startScheduler(void);
void 	blt_ll_irq_startScheduler(void);
int		blt_ll_updateScheduler(void);
void	blt_ll_reset_bSlot_idx(void);
void 	blt_ll_proc_bSlot_idx_overflow(void);



void 	blt_add_future_task(u8, u8, u32, u32);
bool 	blt_remove_future_task(u8);


void 	blt_ll_procStateChange(void);

void 	blt_ll_calculate_sSlot_next(u32 next_tick);

int 	blt_ll_addTask2ExistLinklist( sch_task_t *pStart_schTsk, int task_num_max);

int 	blt_ll_addTask2AbandonTaskLinklist( sch_task_t *pStart_schTsk, int task_num);

static inline void blt_sche_addTaskMask(u32 tskmsk){
	bltSche.task_mask |= (tskmsk);
}

static inline void blt_sche_removeTaskMask(u32 tskmsk){
	bltSche.task_mask &= ~(tskmsk);
}


static inline void blt_sche_enableTask(u32 tskmsk){
	bltSche.task_en |= (tskmsk);
}

static inline void blt_sche_disableTask(u32 tskmsk){
	bltSche.task_en &= ~(tskmsk);
}


static inline void blt_sche_addUpdate(u32 updt){
	bltSche.update |= (updt);
}


static inline void blt_sche_setSystemIrqTrigger(u32 trigger){
	systick_irq_trigger = trigger;
}



/******************************* ll_schedule end ********************************************************************/



















/******************************* ll_pm start ******************************************************************/
#ifndef			BLMS_PM_ENABLE
#define			BLMS_PM_ENABLE									1
#endif

#define 		ACL_SLAVE_PM_LATENCY_EN							1




#define			PPM_IDX_200PPM									2
#define			PPM_IDX_300PPM									3
#define			PPM_IDX_400PPM									4
#define			PPM_IDX_500PPM									5
#define			PPM_IDX_600PPM									6
#define			PPM_IDX_700PPM									7
#define			PPM_IDX_800PPM									8
#define			PPM_IDX_900PPM									9
#define			PPM_IDX_1000PPM									10
#define			PPM_IDX_1100PPM									11
#define			PPM_IDX_1200PPM									12
#define			PPM_IDX_1300PPM									13
#define			PPM_IDX_1400PPM									14
#define			PPM_IDX_1500PPM									15



#if (MCU_CORE_TYPE == MCU_CORE_9518)
	#define 		PPM_IDX_LONG_SLEEP_MIN							3
	#define 		PPM_IDX_SHORT_SLEEP_MIN							5 	//500 ppm
	#define 		PPM_IDX_MAX										10  //1000 ppm
#else  //825x/827x
	#define 		PPM_IDX_LONG_SLEEP_MIN							2
	#define 		PPM_IDX_SHORT_SLEEP_MIN							4
	#define 		PPM_IDX_MAX										8
#endif


#define 		WKPTASK_INVALID				0xFF

_attribute_aligned_(4)
typedef struct {
	u8		pm_inited;
	u8		sleep_allowed;
	u8 		deepRt_en;
	u8		deepRet_type;

	u8		wakeup_src;
	u8		gpio_early_wkp;
	u8		slave_no_sleep;
	u8		slave_idx_calib;

	u8      appWakeup_en;
	u8		appWakeup_flg;
	u8		min_tolerance_us;	// < 256uS
	u8		pm_entered;

	u8		wkpTsk_oft;
	u8		sys_ppm_index;
	u8		u8_rsvd1[2];

	u16		sleep_mask;
	u16 	user_latency;


	u32     deepRet_thresTick;
	u32     deepRet_earlyWakeupTick;
	u32		sleep_taskMask;
	u32		next_task_tick;
	u32		next_adv_tick;
	u32		wkpTsk_tick;
	u32     current_wakeup_tick; //The system wake-up tick of the actual transfer of the cpu_sleep_wakeup function.

	u32     appWakeup_tick;




	sch_task_t  *pTask_wakeup;

	sch_task_t	wkpTsk_fifo; //latency wake_up task fifo

}st_llms_pm_t;
extern st_llms_pm_t  blmsPm;




typedef 	int (*ll_module_pm_callback_t)(void);
extern ll_module_pm_callback_t  ll_module_pm_cb;


int blt_ll_sleep(void);
int blt_sleep_process(void);
/******************************* ll_pm end ********************************************************************/












/******************************* ll_misc start ******************************************************************/
u32 		blt_ll_connCalcAccessAddr_v2(void);

int 		blt_calBit1Number(u32 dat);
int 		blt_calBit1NumberV2(u64 num);

/******************************* ll_misc end ********************************************************************/





/******************************* ll_device start ******************************************************************/


typedef enum {
	MASTER_DEVICE_INDEX_0     = 0,

	SLAVE_DEVICE_INDEX_0      = 0,
	SLAVE_DEVICE_INDEX_1      = 1,
	SLAVE_DEVICE_INDEX_2      = 2
}local_dev_ind_t;   //local device index


/******************************* ll_device end ********************************************************************/





/******************************* HCI start ******************************************************************/
ble_sts_t  		blc_hci_reset(void);
ble_sts_t 		blc_hci_le_readBufferSize_cmd(u8 *pData);
ble_sts_t 		blc_hci_le_getLocalSupportedFeatures(u8 *features);

ble_sts_t 		blc_hci_readSuggestedDefaultTxDataLength (u8 *tx, u8 *txtime);
ble_sts_t 		blc_hci_writeSuggestedDefaultTxDataLength (u16 tx, u16 txtime);
ble_sts_t		blc_hci_readMaximumDataLength(hci_le_readMaxDataLengthCmd_retParam_t  *para);


ble_sts_t 		blc_hci_le_getRemoteSupportedFeatures(u16 connHandle);
ble_sts_t 		blc_hci_le_readChannelMap(u16 connHandle, u8 *returnChannelMap);

/******************************* HCI end ********************************************************************/

#if (MCU_CORE_TYPE == MCU_CORE_9518)
/****************************** (ble1m,2m,500k,125k)RF RX/TX packet format ********************************************
RF RX packet format:
  b0          b3    b4         b5       b6   b(5+w) b(6+w) b(8+w) b(9+w) b(12+w)  b(13+w)    b(14+w)  b(15+w)                      b(16+w)
*---------------*---------*-----------*------------*------------*---------------*-------------------*----------*--------------------------------------------------*
|  DMA_len(4B)  | type(1B)| Rf_len(1B)| payload(wB)|   CRC(3B)  | time_stamp(4B)|  Fre_offset(2B)   | Rssi(1B) |           pkt status indicator(1B)               |
| (b0,b1 valid) |        Header       |   Payload  |            |               |                   | rssi-110 |[0]:crc err;[1]:sfd err;[2]:ll err;[4]:pwr err;   |
|               |<--           PDU              -->|            |               |                   |          |[4]:long range 125k;[6:5]:N/A;[7]:nordic NACK ind |
*---------------*----------------------------------*------------*---------------*-------------------*----------*--------------------------------------------------*
|<--- 4byte --->|<------ 2 byte ----->|<- Rf_len ->|<- 3 byte ->|<----------------------------------- 8 byte ---------------------------------------------------->|
note:       b4       ->  type(1B): llid(2bit) nesn(1bit) sn(1bit) md(1bit).
we can see: DMA_len     =   rx[0] = w(Rf_len)+13 = rx[5]+13.
            CRC_OK      =   DMA_buffer[rx[0]+3] == 0x00 ? True : False.

******
RF TX packet format:
 b0          b3      b4         b5       b6   b(5+w)
*---------------*----------*-----------*------------*
|  DMA_len(4B)  | type(1B) | Rf_len(1B)| payload(wB)|
| (b0,b1 valid) |         Header       |   Payload  |
|               |<--               PDU           -->|
*---------------*-----------------------------------*
note:       b4      ->  type(1B): llid(2bit) nesn(1bit) sn(1bit) md(1bit).Here type only means that llid, other bit is automatically populated when sent by hardware
we can see: DMA_len = rx[0]= w(Rf_len) + 2.
**********************************************************************************************************************/



/***********************************(DLE and MTU buffer size formula)*************************************************
Note: DLE only contains the len of payload (maxTxOct/maxRxOct 251 bytes)
	1. ACL Tx Data buffer len = 4(DMA_len) + 2(BLE header) + maxTxOct + 4(MIC) = maxTxOct + 10
	2. ACL RX Data buffer len = 4(DMA_len) + 2(BLE header) + maxRxOct + 4(MIC) + 3(CRC) + 8(ExtraInfor)  = maxRxOct + 21

	MTU contains ATT exclusive L2cap_Length(2) and CID(2)
	1. MTU Tx buffer len = DMA(4) + Header(2)  + L2cap_Header(4) + MTU = MTU + 10
	2. MTU Rx buffer len = DMA(4) + Header(2) + + L2cap_Header(4) + MTU + MTU + 10
	//todo DMA and Header should not include in MTU buff, Just use DMA field to hold packed len
*********************************************************************************************************************/





/************************************** Link Layer pkt format *********************************************************
Link Layer pak format(BLE4.2 spec):
*-------------*-------------------*-------------------------------*-------------------*
| preamble(1B)| Access Address(4B)|          PDU(2~257B)          |      CRC(3B)      |
|             |                   |  Header(2B) | payload(0~255B) |                   |
*-------------*-------------------*-------------------------------*-------------------*
1.ADV Channel, payload:0~37bytes = 6bytes AdvAdd + [maximum 31bytes adv packet payload]
2.Data Channel, payload:0~255bytes = 0~251bytes + 4bytes MIC(may include MIC feild)[The payload in ble4.2 can reach 251 bytes].
  Protocol overhead: 10bytes(preamble\Access Address\Header\CRC) + L2CAP header 4bytes = 14bytes, all LL data contains 14 bytes of overhead,
  For att, opCode is also needed, 1bytes + handle 2bytes = 3bytes, 251-4-3=[final 247-3bytes available to users].
******
Link Layer pak format(BLE4.0\4.1 spec):
*-------------*-------------------*-------------------------------*-------------------*
| preamble(1B)| Access Address(4B)|          PDU(2~39B)           |      CRC(3B)      |
|             |                   |  Header(2B) | payload(0~37B)  |                   |
*-------------*-------------------*-------------------------------*-------------------*
1.ADV Channel, payload:0~37bytes = 6bytes AdvAdd + [maximum 31bytes adv packet payload]
2.Data Channel, payload:0~31bytes = 0~27bytes + 4bytes MIC(may include MIC feild)[The payload in ble4.0/4.1 is 27 bytes].
  Protocol overhead: 10bytes(preamble\Access Address\Header\CRC) + L2CAP header 4bytes = 14bytes,all LL data contains 14 bytes of overhead,
  For att, opCode is also needed, 1bytes + handle 2bytes = 3bytes, 27-4-3=[final 23-3bytes available to users] This is why the default mtu size is 23 in the ble4.0 protocol.
**********************************************************************************************************************/


/*********************************** Advertising channel PDU : Header *************************************************
Header(2B):[Advertising channel PDU Header](BLE4.0\4.1 spec):
*--------------*----------*------------*-------------*-------------*----------*
|PDU Type(4bit)| RFU(2bit)| TxAdd(1bit)| RxAdd(1bit) |Length(6bits)| RFU(2bit)|
*--------------*----------*------------*-------------*-------------*----------*
public (TxAdd = 0) or random (TxAdd = 1).
**********************************************************************************************************************/


/******************************************* Data channel PDU : Header ************************************************
Header(2B):[Data channel PDU Header](BLE4.2 spec):(BLE4.0\4.1 spec):
*----------*-----------*---------*----------*----------*-------------*----------*
|LLID(2bit)| NESN(1bit)| SN(1bit)| MD(1bit) | RFU(3bit)|Length(5bits)| RFU(3bit)|
*----------*-----------*---------*----------*----------*-------------*----------*
******
Header(2B):[Data channel PDU Header](BLE4.2 spec):
*----------*-----------*---------*----------*----------*------------------------*
|LLID(2bit)| NESN(1bit)| SN(1bit)| MD(1bit) | RFU(3bit)|       Length(8bits)    |
*----------*-----------*---------*----------*----------*------------------------*
start    pkt:  llid 2 -> 0x02
continue pkt:  llid 1 -> 0x01
control  pkt:  llid 3 -> 0x03
***********************************************************************************************************************/


/*********************************** DATA channel PDU ******************************************************************
*------------------------------------- ll data pkt -------------------------------------------*
|             |llid nesn sn md |  pdu-len   | l2cap_len(2B)| chanId(2B)|  opCode(1B)|data(xB) |
| DMA_len(4B) |   type(1B)     | rf_len(1B) |       L2CAP header       |       value          |
|             |          data_headr         |                        payload                  |
*-------------*-----------------------------*-------------------------------------------------*
*--------------------------------- ll control pkt ----------------------------*
| DMA_len(4B) |llid nesn sn md |  pdu-len   | LL Opcode(1B) |  CtrData(0~22B) |
|             |   type(1B)     | rf_len(1B) |               |      value      |
|             |          data_headr         |            payload              |
*-------------*-----------------------------*---------------------------------*
***********************************************************************************************************************/

#else

#endif





#endif /* LL_STACK_H_ */
