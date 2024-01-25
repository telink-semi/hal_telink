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


#ifndef DRIVERS_B93_EXT_DRIVER_EXT_LIB_H_
#define DRIVERS_B93_EXT_DRIVER_EXT_LIB_H_


#include "types.h"
#include "../uart.h"
#include "../pm.h"
#include "../rf.h"
#include <stdbool.h>
#include "ext_pm.h"




/******************************* debug_start ******************************************************************/
void sub_wr_ana(unsigned int addr, unsigned char value, unsigned char e, unsigned char s);
void sub_wr(unsigned int addr, unsigned char value, unsigned char e, unsigned char s);
/******************************* debug_end ********************************************************************/


/******************************* dbgport start ******************************************************************/
#define reg_bb_dbg_sel      REG_ADDR16(0x140378)
#define reg_bb_dbg_sel_l    REG_ADDR8(0x140378)
#define reg_bb_dbg_sel_h    REG_ADDR8(0x140379)
#define	bt_dbg_set_pin		dbg_bb_set_pin

void ble_dbg_port_init(int deg_sel0);

void dbg_bb_set_pin(gpio_pin_e pin);

void rf_enable_bb_debug(void);
/******************************* dbgport end ********************************************************************/


/******************************* dbgErrorCode start ******************************************************************/
/* for debug (write ram)*/
#define	DBG_SRAM_ADDR					0x00014

#define PKE_OPERAND_MAX_WORD_LEN      (0x08)
#define PKE_OPERAND_MAX_BIT_LEN       (0x100)
#define ECC_MAX_WORD_LEN              PKE_OPERAND_MAX_WORD_LEN
/*
 * addr - only 0x00012 ~ 0x00021 can be used !!! */
#define write_dbg32(addr, value)   		write_sram32(addr, value)

#define write_log32(err_code)   		write_sram32(0x00014, err_code)
/******************************* dbgErrorCode end ********************************************************************/


/******************************* ext_aes start ******************************************************************/
#define HW_AES_CCM_ALG_EN										0  //TODO

extern unsigned int aes_data_buff[8];


#define	CV_LLBT_BASE				(0x160000)

#define reg_rwbtcntl				REG_ADDR32(CV_LLBT_BASE)

enum{

	FLD_NWINSIZE					= BIT_RNG(0,5),
	FLD_RWBT_RSVD6_7				= BIT_RNG(6,7),

	FLD_RWBTEN              =   BIT(8),
	FLD_CX_DNABORT      	=   BIT(9),
	FLD_CX_RXBSYENA     	=   BIT(10),
	FLD_CX_TXBSYENA			=   BIT(11),
	FLD_SEQNDSB				=   BIT(12),
	FLD_ARQNDSB     		=   BIT(13),
	FLD_FLOWDSB				=   BIT(14),
	FLD_HOPDSB				=   BIT(15),

	FLD_WHITDSB             =   BIT(16),
	FLD_CRCDSB      		=   BIT(17),
	FLD_CRYPTDSB     		=   BIT(18),
	FLD_LMPFLOWDSB			=   BIT(19),
	FLD_SNIFF_ABORT			=   BIT(20),
	FLD_PAGEINQ_ABORT    	=   BIT(21),
	FLD_RFTEST_ABORT		=   BIT(22),
	FLD_SCAN_ABORT			=   BIT(23),


	FLD_RWBT_RSVD24_25			=   BIT_RNG(24,25),
	FLD_CRYPT_SOFT_RST			=	BIT(26),   /**HW AES_CMM module reset*/
	FLD_SWINT_REQ               =   BIT(27),
	FLD_RADIOCNTL_SOFT_RST      =   BIT(28),
	FLD_REG_SOFT_RST     		=   BIT(29),
	FLD_MASTER_TGSOFT_RST		=   BIT(30),
	FLD_MASTER_SOFT_RST			=   BIT(31),

};


void aes_encryption_le(u8* key, u8* plaintext, u8 *encrypted_data);
void aes_encryption_be(u8* key, u8* plaintext, u8 *encrypted_data);

bool aes_resolve_irk_rpa(u8 *key, u8 *addr);

void blt_ll_setAesCcmPara(u8 role, u8 *sk, u8 *iv, u8 aad, u64 enc_pno, u64 dec_pno, u8 lastTxLenFlag);

/******************************* ext_aes end ********************************************************************/



/******************************* ext_aoa start ******************************************************************/

/******************************* ext_aoa end ********************************************************************/



/******************************* ext_audio start ******************************************************************/

/******************************* ext_audio end ********************************************************************/




/******************************* ext_codec start ******************************************************************/

/******************************* ext_codec end ********************************************************************/



/******************************* ext_flash start ******************************************************************/

/******************************* ext_flash end ********************************************************************/




/******************************* ext_gpio start ******************************************************************/

/******************************* ext_gpio end ********************************************************************/




/******************************* ext_hci_uart start ******************************************************************/

/******************************* ext_hci_uart end ********************************************************************/





/******************************* ext_stimer start ******************************************************************/
#define	SYSTICK_NUM_PER_US				24

#define	SSLOT_TICK_NUM					1875/4    //attention: not use "()" for purpose !!!    625uS*24/32=625*3/4=1875/4=468.75
#define	SSLOT_TICK_REVERSE				4/1875	  //attention: not use "()" for purpose !!!


typedef enum {
	STIMER_IRQ_MASK     		=   BIT(0),
	STIMER_32K_CAL_IRQ_MASK     =   BIT(1),
}stimer_irq_mask_e;

typedef enum {
	FLD_IRQ_SYSTEM_TIMER     		=   BIT(0),
}system_timer_irq_mask_e;


typedef enum {
	STIMER_IRQ_CLR	     		=   BIT(0),
	STIMER_32K_CAL_IRQ_CLR     	=   BIT(1),
}stimer_irq_clr_e;

/**
 * @brief define system clock tick per us/ms/s.
 */
enum{
	SYSTEM_TIMER_TICK_125US 	= 2000,   //125*16
};

/**
 * @brief    This function serves to enable system timer interrupt.
 * @return  none
 */
static inline void systimer_irq_enable(void)
{
	reg_irq_src0 |= BIT(IRQ1_SYSTIMER);
	//plic_interrupt_enable(IRQ1_SYSTIMER);
}

/**
 * @brief    This function serves to disable system timer interrupt.
 * @return  none
 */
static inline void systimer_irq_disable(void)
{
	reg_irq_src0 &= ~BIT(IRQ1_SYSTIMER);
	//plic_interrupt_disable(IRQ1_SYSTIMER);
}

static inline void systimer_set_irq_mask(void)
{
	reg_system_irq_mask |= STIMER_IRQ_MASK;
}

static inline void systimer_clr_irq_mask(void)
{
	reg_system_irq_mask &= (~STIMER_IRQ_MASK);
}

static inline unsigned char systimer_get_irq_status(void)
{
	return reg_system_cal_irq & FLD_IRQ_SYSTEM_TIMER;
}

static inline void systimer_clr_irq_status(void)
{
	reg_system_cal_irq = STIMER_IRQ_CLR;
}

static inline void systimer_set_irq_capture(unsigned int tick)
{
	reg_system_irq_level = tick;
}

static inline unsigned int systimer_get_irq_capture(void)
{
	return reg_system_irq_level;
}

static inline int tick1_exceed_tick2(u32 tick1, u32 tick2)
{
	return (u32)(tick1 - tick2) < BIT(30);
}


static inline int tick1_closed_to_tick2(unsigned int tick1, unsigned int tick2, unsigned int tick_distance)
{
	return (unsigned int)(tick1 + tick_distance - tick2) < (tick_distance<<1);
}

static inline int tick1_out_range_of_tick2(unsigned int tick1, unsigned int tick2, unsigned int tick_distance)
{
	return (unsigned int)(tick1 + tick_distance - tick2) > (tick_distance<<1);
}

/******************************* ext_stimer end ********************************************************************/



/******************************* ext_pm start ******************************************************************/
#ifndef	PM_32k_RC_CALIBRATION_ALGORITHM_EN
#define PM_32k_RC_CALIBRATION_ALGORITHM_EN				1
#endif

#define SYS_NEED_REINIT_EXT32K			    BIT(1)
#define WAKEUP_STATUS_TIMER_CORE     	    ( WAKEUP_STATUS_TIMER | WAKEUP_STATUS_CORE)
#define WAKEUP_STATUS_TIMER_PAD		        ( WAKEUP_STATUS_TIMER | WAKEUP_STATUS_PAD)

/**
 * @brief analog register below can store infomation when MCU in deepsleep mode
 * 	      store your information in these ana_regs before deepsleep by calling analog_write function
 * 	      when MCU wakeup from deepsleep, read the information by by calling analog_read function
 * 	      Reset these analog registers only by power cycle
 */
#define DEEP_ANA_REG0                       PM_ANA_REG_POWER_ON_CLR_BUF0 //initial value =0x00	[Bit0][Bit1] is already occupied. The customer cannot change!
#define DEEP_ANA_REG1                       PM_ANA_REG_POWER_ON_CLR_BUF1 //initial value =0x00
#define DEEP_ANA_REG2                       PM_ANA_REG_POWER_ON_CLR_BUF2 //initial value =0x00
#define DEEP_ANA_REG3                      	PM_ANA_REG_POWER_ON_CLR_BUF3 //initial value =0x00
#define DEEP_ANA_REG4                       PM_ANA_REG_POWER_ON_CLR_BUF4 //initial value =0x00
#define DEEP_ANA_REG5                       PM_ANA_REG_POWER_ON_CLR_BUF5 //initial value =0x00
#define DEEP_ANA_REG6                       PM_ANA_REG_POWER_ON_CLR_BUF6 //initial value =0x0f

/**
 * @brief these analog register can store data in deepsleep mode or deepsleep with SRAM retention mode.
 * 	      Reset these analog registers by watchdog, chip reset, RESET Pin, power cycle
 */

#define DEEP_ANA_REG7                       PM_ANA_REG_WD_CLR_BUF0 //initial value =0xff	[Bit0] is already occupied. The customer cannot change!

//ana39 system used, user can not use
#define SYS_DEEP_ANA_REG 					PM_ANA_REG_POWER_ON_CLR_BUF0





extern  unsigned char 		    tl_24mrc_cal;
extern 	unsigned int 			g_pm_tick_32k_calib;
extern  unsigned int 			g_pm_tick_cur;
extern  unsigned int 			g_pm_tick_32k_cur;
extern  unsigned char       	g_pm_long_suspend;
extern  unsigned int 			g_pm_mspi_cfg;

extern	unsigned int 			g_sleep_32k_rc_cnt;
extern	unsigned int 			g_sleep_stimer_tick;

extern unsigned int	ota_program_bootAddr;
extern unsigned int	ota_firmware_max_size;
extern unsigned int	ota_program_offset;

/**
 * @brief   pm 32k rc calibration algorithm.
 */
typedef struct  pm_clock_drift
{
	unsigned int	ref_tick;
	unsigned int	ref_tick_32k;
	int				offset;
	int				offset_dc;
//	int				offset_cur;
	unsigned int	offset_cal_tick;
	int				tc;
	int				rc32;
	int				rc32_wakeup;
	int				rc32_rt;
	int				s0;
	unsigned char	calib;
	unsigned char	ref_no;
} pm_clock_drift_t;


extern pm_clock_drift_t	pmbcd;

static inline unsigned int pm_ble_get_latest_offset_cal_time(void)
{
	return pmbcd.offset_cal_tick;
}
/**
 * @brief		Calculate the offset value based on the difference of 16M tick.
 * @param[in]	offset_tick	- the 16M tick difference between the standard clock and the expected clock.
 * @param[in]	rc32_cnt	- 32k count
 * @return		none.
 */
_attribute_ram_code_sec_noinline_ void pm_ble_cal_32k_rc_offset (int offset_tick, int rc32_cnt);

/**
 * @brief		This function reset calibrates the value
 * @param[in]	none
 * @return		none.
 */
void pm_ble_32k_rc_cal_reset(void);
#define PM_MIN_SLEEP_US			1500  //B93 todo

/**
 * @brief   internal oscillator or crystal calibration for environment change such as voltage, temperature
 * 			to keep some critical PM or RF performance stable
 * 			attention: this is a stack API, user can not call it
 * @param	none
 * @return	none
 */
void mcu_oscillator_crystal_calibration(void);

typedef int (*suspend_handler_t)(void);
typedef void (*check_32k_clk_handler_t)(void);
typedef unsigned int (*pm_get_32k_clk_handler_t)(void);
typedef unsigned int (*pm_tim_recover_handler_t)(unsigned int);

extern  suspend_handler_t 		 	func_before_suspend;
extern  check_32k_clk_handler_t  	pm_check_32k_clk_stable;
extern  pm_get_32k_clk_handler_t 	pm_get_32k_tick;
extern  pm_tim_recover_handler_t 	pm_tim_recover;


/******************************* ext_pm end ********************************************************************/



/******************************* ext_rf start ******************************************************************/

#ifndef FAST_SETTLE
#define FAST_SETTLE			1
#endif

enum{
	//BLE mode
	FLD_RF_BRX_SN_INIT	 		=   BIT(4),
	FLD_RF_BRX_NESN_INIT	 	=   BIT(5),
	FLD_RF_BTX_SN_INIT	 		=   BIT(6),
	FLD_RF_BTX_NESN_INIT	 	=   BIT(7),
};

//*********************Note: CRYPT*****************************/
enum{
	FLD_TLK_CRYPT_ENABLE		=   BIT(1),
	FLD_TLK_MST_SLV				=   BIT(2),
};

#define    reg_rf_tlk_sk0       REG_ADDR32(REG_BASEBAND_BASE_ADDR+0xa4)
#define    reg_rf_tlk_sk1       REG_ADDR32(REG_BASEBAND_BASE_ADDR+0xa8)
#define    reg_rf_tlk_sk2       REG_ADDR32(REG_BASEBAND_BASE_ADDR+0xac)
#define    reg_rf_tlk_sk3       REG_ADDR32(REG_BASEBAND_BASE_ADDR+0xb0)

#define    reg_tlk_sk(v)		REG_ADDR32(REG_BASEBAND_BASE_ADDR + 0xa4 + (v*4))



#define    reg_rf_tlk_iv0       REG_ADDR32(REG_BASEBAND_BASE_ADDR+0xb4)
#define    reg_rf_tlk_iv1       REG_ADDR32(REG_BASEBAND_BASE_ADDR+0xb8)

#define    reg_rf_tlk_aad       REG_ADDR8(REG_BASEBAND_BASE_ADDR+0xbc)

#define    reg_ccm_control				REG_ADDR8(REG_BB_LL_BASE_ADDR+0x3f)


enum{
	FLD_R_TXLEN_FLAG			= BIT(0),
};

//39 bits
#define		reg_rf_tx_ccm_pkt_cnt0_31		REG_ADDR32(REG_BB_LL_BASE_ADDR+0x40)
#define		reg_rf_tx_ccm_pkt_cnt32_38		REG_ADDR8(REG_BB_LL_BASE_ADDR+0x44)

#define		reg_rf_rx_ccm_pkt_cnt0_31		REG_ADDR32(REG_BB_LL_BASE_ADDR+0x48)
#define		reg_rf_rx_ccm_pkt_cnt32_38		REG_ADDR8(REG_BB_LL_BASE_ADDR+0x4c)

//*********************Note: must write and read one word*****************************/
#define REG_CV_LLBT_BASE_ADDR							(0x160000)

#define reg_cv_intcntl									REG_ADDR32(REG_CV_LLBT_BASE_ADDR +0x0c)

#define	reg_cv_llbt_rpase_cntl							REG_ADDR32(REG_CV_LLBT_BASE_ADDR+0x288)

enum
{
	FLD_CV_PRAND			= BIT_RNG(0,23),
	FLD_CV_IRK_NUM			= BIT_RNG(24,27),
	FLD_CV_GEN_RES			= BIT(29),
	FLD_CV_RPASE_START		= BIT(30),
	FLD_CV_PRASE_ENABLE		= BIT(31),
};

#define	reg_cv_llbt_hash_status							REG_ADDR32(REG_CV_LLBT_BASE_ADDR+0x28c)

enum
{
	FLD_CV_hash					= BIT_RNG(0,23),
	FLD_CV_IRK_CNT				= BIT_RNG(24,27),
	FLD_CV_RPASE_STATUS_CLR 	= BIT(28),
	FLD_CV_RPASE_STATUS			= BIT_RNG(29,30),
	FLD_CV_HASH_MATCH			= BIT(31),
};


#define	reg_cv_llbt_irk_ptr								REG_ADDR32(REG_CV_LLBT_BASE_ADDR+0x290)

#define			STOP_RF_STATE_MACHINE							( REG_ADDR8(0x80170200) = 0x80 )


#define DMA_RFRX_LEN_HW_INFO				0	// 826x: 8
#define DMA_RFRX_OFFSET_HEADER				4	// 826x: 12
#define DMA_RFRX_OFFSET_RFLEN				5   // 826x: 13
#define DMA_RFRX_OFFSET_DATA				6	// 826x: 14

#define RF_TX_PAKET_DMA_LEN(rf_data_len)		(((rf_data_len)+3)/4)|(((rf_data_len) % 4)<<22)
#define DMA_RFRX_OFFSET_CRC24(p)			(p[DMA_RFRX_OFFSET_RFLEN]+6)  //data len:3
#define DMA_RFRX_OFFSET_TIME_STAMP(p)		(p[DMA_RFRX_OFFSET_RFLEN]+9)  //data len:4
#define DMA_RFRX_OFFSET_FREQ_OFFSET(p)		(p[DMA_RFRX_OFFSET_RFLEN]+13) //data len:2
#define DMA_RFRX_OFFSET_RSSI(p)				(p[DMA_RFRX_OFFSET_RFLEN]+15) //data len:1, signed
#define DMA_RFRX_OFFSET_STATUS(p)			(p[DMA_RFRX_OFFSET_RFLEN]+16)

#define	RF_BLE_RF_PAYLOAD_LENGTH_OK(p)					( *((unsigned int*)p) == p[5]+13)    			//dma_len must 4 byte aligned
#define	RF_BLE_RF_PACKET_CRC_OK(p)						((p[(p[5]+5 + 11)] & 0x01) == 0x0)
#define	RF_BLE_PACKET_VALIDITY_CHECK(p)					(RF_BLE_RF_PAYLOAD_LENGTH_OK(p) && RF_BLE_RF_PACKET_CRC_OK(p))

#define	RF_BLE_RF_PACKET_CRC_OK_HW_ECC(p)						((p[p[5]+5+11-4] & 0x01) == 0x0)

#define rf_set_tx_packet_address(addr)		(dma_set_src_address(DMA0, convert_ram_addr_cpu2bus(addr)))


//RF BLE Minimum TX Power LVL (unit: 1dBm)
extern const char  ble_rf_min_tx_pwr;
//RF BLE Maximum TX Power LVL (unit: 1dBm)
extern const char  ble_rf_max_tx_pwr;
//RF BLE Current TX Path Compensation
extern	signed short ble_rf_tx_path_comp;
//RF BLE Current RX Path Compensation
extern	signed short ble_rf_rx_path_comp;
//Current RF RX DMA buffer point for BLE
extern  unsigned char *ble_curr_rx_dma_buff;


typedef enum {
	 RF_ACC_CODE_TRIGGER_AUTO   =    BIT(0),	/**< auto trigger */
	 RF_ACC_CODE_TRIGGER_MANU   =    BIT(1),	/**< manual trigger */
} rf_acc_trigger_mode;




extern signed char ble_txPowerLevel;

_attribute_ram_code_ void ble_rf_set_rx_dma(unsigned char *buff, unsigned char size_div_16);

_attribute_ram_code_ void ble_rf_set_tx_dma(unsigned char fifo_dep, unsigned char size_div_16);

_attribute_ram_code_ void ble_tx_dma_config(void);

_attribute_ram_code_ void ble_rx_dma_config(void);



/**
 * @brief   This function serves to settle adjust for RF Tx.This function for adjust the differ time
 * 			when rx_dly enable.
 * @param   txstl_us - adjust TX settle time.
 * @return  none
 */
static inline void 	rf_tx_settle_adjust(unsigned short txstl_us)
{
	REG_ADDR16(0x80170204) = txstl_us;
}

static inline unsigned short rf_read_tx_settle(void)
{
	return REG_ADDR16(0x80170204);
}


/* attention that not bigger than 4095 */
static inline void rf_ble_set_rx_settle( unsigned short rx_stl_us )
{
	 write_reg16(0x17020c, rx_stl_us);
}


/**
 * @brief	  	This function serves to update the value of internal cap.
 * @param[in]  	value   - The value of internal cap which you want to set.
 * @return	 	none.
 */
static inline void 	rf_update_internal_capacitance(unsigned char value)
{
	/*
	 * afe1v_reg10<5:0>		reg_xo_cdac_ana<5:0>		CDAC value (lowest cap to highest cap)
	 * afe1v_reg10<6>		reg_xo_mode_ana				mode control - 0 : AMP_OFF, 1 : AMP_ON.
	 * 													0 is to support dc coupling and 1 is to support ac coupling
	 * afe1v_reg10<7>		reg_xo_cap_off_ana			control of X1 and X2 capacitance values
														0 : cap follows CDAC, 1 : cap OFF
	 */
	analog_write_reg8(0x8A, (analog_read_reg8(0x8A) & 0x40)|(value & 0x3f));
}



/**
 * @brief   This function serves to set RF access code value.
 * @param[in]   ac - the address value.
 * @return  none
 */
static inline void rf_set_ble_access_code_value (unsigned int ac)
{
	write_reg32 (0x80170008, ac);
}

/**
 * @brief   This function serves to set RF access code.
 * @param[in]   p - the address to access.
 * @return  none
 */
static inline void rf_set_ble_access_code (unsigned char *p)
{
	write_reg32 (0x80170008, p[3] | (p[2]<<8) | (p[1]<<16) | (p[0]<<24));
}

/**
 * @brief   This function serves to reset function for RF.
 * @param   none
 * @return  none
 *******************need driver change
 */
static inline void reset_sn_nesn(void)
{
	REG_ADDR8(0x80170201) =  0x01;
}

/**
 * @brief   This function serves to set RF access code advantage.
 * @param   none.
 * @return  none.
 */
static inline void rf_set_ble_access_code_adv (void)
{
	write_reg32 (0x170008, 0xd6be898e);
}


/**
 * @brief   This function serves to triggle accesscode in coded Phy mode.
 * @param   none.
 * @return  none.
 */
static inline void rf_trigger_codedPhy_accesscode(void)
{
	write_reg8(0x170425,read_reg8(0x170425)|0x01);
}

/**
 * @brief     This function performs to enable RF Tx.
 * @param[in] none.
 * @return    none.
 */
static inline void rf_ble_tx_on ()
{
	write_reg8  (0x80170202, 0x45 | BIT(4));	// TX enable
}

/**
 * @brief     This function performs to done RF Tx.
 * @param[in] none.
 * @return    none.
 */
static inline void rf_ble_tx_done ()
{
	write_reg8  (0x80170202, 0x45);
}

/**
 * @brief   This function serves to set RX first timeout value.
 * @param[in]   tm - timeout, unit: uS.
 * @return  none.
 */
static inline void rf_set_1st_rx_timeout(unsigned int tm)
{
	reg_rf_ll_rx_fst_timeout = tm;
}


/**
 * @brief   This function serves to set RX timeout value.
 * @param[in]   tm - timeout, unit: uS, range: 0 ~ 0xfff
 * @return  none.
 */
static inline void rf_ble_set_rx_timeout(u16 tm)
{
	reg_rf_rx_timeout = tm;
}

/**
 * @brief	This function serve to set the length of preamble for BLE packet.
 * @param[in]	len		-The value of preamble length.Set the register bit<0>~bit<4>.
 * @return		none
 */
static inline void rf_ble_set_preamble_len(u8 len)
{
	write_reg8(0x170002,(read_reg8(0x170002)&0xe0)|(len&0x1f));
}

static inline int rf_ble_get_preamble_len(void)
{
	return (read_reg8(0x170002)&0x1f); //[4:0]: BLE preamble length
}

static inline void rf_set_dma_tx_addr(unsigned int src_addr)//Todo:need check by sunwei
{
	reg_dma_src_addr(DMA0)=convert_ram_addr_cpu2bus(src_addr);
}

typedef enum{
	FSM_BTX 	= 0x81,
	FSM_BRX 	= 0x82,
	FSM_PTX	= 0x83,
	FSM_PRX    = 0x84,
	FSM_STX 	= 0x85,
	FSM_SRX 	= 0x86,
	FSM_TX2RX	= 0x87,
	FSM_RX2TX	= 0x88,
}fsm_mode_e;


/**
 * @brief     	This function serves to RF trigger RF state machine.
 * @param[in] 	mode  - FSM mode.
 * @param[in] 	tx_addr  - DMA TX buffer, if not TX, must be "NULL"
 * @param[in] 	tick  - FAM Trigger tick.
 * @return	   	none.
 */
void rf_start_fsm(fsm_mode_e mode, void* tx_addr, unsigned int tick);

/**
 * @brief   	This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return  	none.
 */
void rf_set_ble_channel (signed char chn_num);

/**
 * @brief     This function performs to switch PHY test mode.
 * @param[in] mode - PHY mode
 * @return    none.
 */
void rf_switchPhyTestMode(rf_mode_e mode);


/**
 * @brief     This function performs to set RF Access Code Threshold.// use for BQB
 * @param[in] threshold   cans be 0-32bits
 * @return    none.
 */
void rf_set_accessCodeThreshold(u8 threshold);

/*
 * brief:If already know the DMA length value,this API can calculate the real RF length value that is easier for humans to understand.
 * param: dma_len -- the value calculated by this macro "rf_tx_packet_dma_len"
 * return: 0xFFFFFFFE --- error;
 *         other value--- success;
 */
u32 rf_cal_rfLenFromDmaLen(u32 dma_len);


//TODO: merge into driver
enum{
	FLD_RF_SN                     =	BIT(0),
};



/**
 * @brief    This function serves to enable zb_rt interrupt source.
 * @return  none
 */
static inline void zb_rt_irq_enable(void)
{
	plic_interrupt_enable(IRQ15_ZB_RT);
}


/*
 * SiHui & QingHua & SunWei sync with Xuqiang.Zhang & Zhiwei.Wang & Kaixin.Chen & Shujuan.chu
 * B91/B92/B93
 * TX settle recommend value by digital team: 108.5uS without fast settle;   108.5-58.5=50 with fast settle
 * we BLE use 110 without fast settle; 110-57=53 with fast settle, here 53 = real settle 45uS + extra 1 preamble 8uS(1M for example)
 *
 * RX settle recommend value by digital team: 108.5uS without fast settle;   85-40=45 with fast settle
 */

#if (FAST_SETTLE)
	#define RX_SETTLE_US					45
#else
	#define RX_SETTLE_US					85
#endif


#define PRMBL_LENGTH_1M						2	//preamble length for 1M PHY
#define PRMBL_LENGTH_2M						3   //preamble length for 2M PHY
#define PRMBL_LENGTH_Coded					10  //preamble length for Coded PHY, never change this value !!!

#define PRMBL_EXTRA_1M						(PRMBL_LENGTH_1M - 1)	// 1 byte for 1M PHY
#define PRMBL_EXTRA_2M						(PRMBL_LENGTH_2M - 2)	// 2 byte for 2M
#define PRMBL_EXTRA_Coded					0     					// 10byte for Coded, 80uS, no extra byte



#if RF_RX_SHORT_MODE_EN//open rx dly

	#if (FAST_SETTLE)
		#define 		TX_STL_LEGADV_SCANRSP_REAL					53	//can change, consider TX packet quality
	#else
		#define 		TX_STL_LEGADV_SCANRSP_REAL					110 //can change, consider TX packet quality
	#endif
	#define 		TX_STL_LEGADV_SCANRSP_SET						(TX_STL_LEGADV_SCANRSP_REAL - PRMBL_EXTRA_1M * 8)  //can not change !!!


	#if (FAST_SETTLE)
		#define			TX_STL_TIFS_REAL_COMMON						53	//can change, consider TX packet quality
	#else
		#define			TX_STL_TIFS_REAL_COMMON						110	//can change, consider TX packet quality
	#endif

	#define 		TX_STL_TIFS_REAL_1M								TX_STL_TIFS_REAL_COMMON  //can not change !!!
	#define 		TX_STL_TIFS_SET_1M								(TX_STL_TIFS_REAL_1M - PRMBL_EXTRA_1M * 8)  //can not change !!!

	#define 		TX_STL_TIFS_REAL_2M								TX_STL_TIFS_REAL_COMMON  //can not change !!!
	#define 		TX_STL_TIFS_SET_2M								(TX_STL_TIFS_REAL_2M - PRMBL_EXTRA_2M * 4)  //can not change !!!

	#define 		TX_STL_TIFS_REAL_CODED							TX_STL_TIFS_REAL_COMMON  //can not change !!!
	#define 		TX_STL_TIFS_SET_CODED							TX_STL_TIFS_REAL_CODED 	 //can not change !!!


	#if (FAST_SETTLE)
		#define			TX_STL_ADV_REAL_COMMON						53	//can change, consider TX packet quality
	#else
		#define			TX_STL_ADV_REAL_COMMON						110	//can change, consider TX packet quality
	#endif
	#define			TX_STL_ADV_REAL_1M								TX_STL_ADV_REAL_COMMON
	#define 		TX_STL_ADV_SET_1M								(TX_STL_ADV_REAL_1M - PRMBL_EXTRA_1M * 8)  //can not change !!!

	#define			TX_STL_ADV_REAL_2M								TX_STL_ADV_REAL_COMMON
	#define 		TX_STL_ADV_SET_2M								(TX_STL_ADV_REAL_2M - PRMBL_EXTRA_2M * 4)  //can not change !!!

	#define			TX_STL_ADV_REAL_CODED							TX_STL_ADV_REAL_COMMON
	#define 		TX_STL_ADV_SET_CODED							TX_STL_ADV_REAL_CODED  //can not change !!!


	#define 		TX_STL_AUTO_MODE_1M								(127 - PRMBL_EXTRA_1M * 8)
	#define			TX_STL_AUTO_MODE_2M								(133 - PRMBL_EXTRA_2M * 4)
	#define			TX_STL_AUTO_MODE_CODED							125


	#if (FAST_SETTLE)
		#define 	TX_STL_BTX_1ST_PKT_REAL							53 //110 - 57 = 53
	#else
		/* normal mode(no fast settle): for ACL Central, tx settle real 110uS or 107uS or even 105uS, not big difference,
		 * but for CIS Central, timing is very urgent considering T_MSS between two sub_event, so SiHui use 107, we keep this set
		 * fast settle mode:  */
		#define 	TX_STL_BTX_1ST_PKT_REAL							(110 - 3) //3 is total switch delay time
	#endif

	#define 		TX_STL_BTX_1ST_PKT_SET_1M						(TX_STL_BTX_1ST_PKT_REAL - PRMBL_EXTRA_1M * 8)
	#define			TX_STL_BTX_1ST_PKT_SET_2M						(TX_STL_BTX_1ST_PKT_REAL - PRMBL_EXTRA_2M * 4)
	#define			TX_STL_BTX_1ST_PKT_SET_CODED					TX_STL_BTX_1ST_PKT_REAL
#else
	#error "add code here, TX settle time"
#endif


/* AD convert delay : timing cost on RF analog to digital convert signal process:
 * 					Eagle	1M: 20uS	   2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
 *					Jaguar	1M: 20uS	   2M: 10uS;      500K(S2): 14uS    125K(S8):  14uS
 */
#define AD_CONVERT_DLY_1M											19	//before:20. Jaguar T_IFS need 32M + AD_Convert=19, tested by kai.jia at 2022-11-17
#define AD_CONVERT_DLY_2M											10
#define AD_CONVERT_DLY_CODED										14

#define OTHER_SWITCH_DELAY_1M										0
#define OTHER_SWITCH_DELAY_2M										0
#define OTHER_SWITCH_DELAY_CODED									0


#define HW_DELAY_1M													(AD_CONVERT_DLY_1M + OTHER_SWITCH_DELAY_1M)
#define HW_DELAY_2M													(AD_CONVERT_DLY_2M + OTHER_SWITCH_DELAY_2M)
#define HW_DELAY_CODED												(AD_CONVERT_DLY_CODED + OTHER_SWITCH_DELAY_CODED)

static inline void rf_ble_set_1m_phy(void)
{
	write_reg8(0x17063d,0x61);
	write_reg32(0x170620,0x23200a16);
	write_reg8(0x170420,0x8c);// script cc.BIT[3]continue mode.After syncing to the preamble, it will immediately enter
							  //the sync state again, reducing the probability of mis-syncing.modified by zhiwei,confirmed
							  //by qiangkai and xuqiang.20221205
	write_reg8(0x170422,0x00);
	write_reg8(0x17044d,0x01);
	write_reg8(0x17044e,0x1e);
	write_reg16(0x170436,0x0eb7);
	write_reg16(0x170438,0x71c4);
	write_reg8(0x170473,0x01);

	write_reg8(0x17049a,0x00);//tx_tp_align.
	write_reg16(0x1704c2,0x4b3a);
	write_reg32(0x1704c4,0x7a6e6356);
	write_reg8(0x1704c8,0x39);//bit[0:5]grx_fix
#if 1
	write_reg32(0x170000,0x4440081f | PRMBL_LENGTH_1M<<16);
#else
	write_reg32(0x170000,0x4446081f);
#endif

	write_reg16(0x170004,0x04f5);

	write_reg8(0x1704bb,0x50);//BIT[5]:rxc_chf_sel_ble;1M:0(default) 2M:1 open two stage filter to improve
							  //the sensitivity.modified by zhiwei,confirmed by wenfeng and xuqiang,20230106.
}


static inline void rf_ble_set_2m_phy(void)
{
	write_reg8(0x17063d,0x41);
	write_reg32(0x170620,0x26432a06);
	write_reg8(0x170420,0x8c);// script cc.BIT[3]continue mode.After syncing to the preamble, it will immediately enter
							  //the sync state again, reducing the probability of mis-syncing.modified by zhiwei,confirmed
							  //by qiangkai and xuqiang.20221205
	write_reg8(0x170422,0x01);
	write_reg8(0x17044d,0x01);
	write_reg8(0x17044e,0x1e);
	write_reg16(0x170436,0x0eb7);
	write_reg16(0x170438,0x71c4);
	write_reg8(0x170473,0x01);

	write_reg8(0x17049a,0x00);//tx_tp_align.

	write_reg16(0x1704c2,0x4c3b);
	write_reg32(0x1704c4,0x7b726359);
	write_reg8(0x1704c8,0x39);//bit[0:5]grx_fix
	#if 1
		write_reg32(0x170000,0x4440081f | PRMBL_LENGTH_2M<<16);
	#else
		write_reg32(0x170000,0x4446081f);
	#endif

	write_reg16(0x170004,0x04e5);

	write_reg8(0x1704bb,0x70);//BIT[5]:rxc_chf_sel_ble;1M:0(default) 2M:1 open two stage filter to improve
								  //the sensitivity.modified by zhiwei,confirmed by wenfeng and xuqiang,20230106.
}




static inline void rf_ble_set_coded_phy_common(void)
{
	write_reg8(0x17063d,0x61);
	write_reg32(0x170620,0x23200a16);
	write_reg8(0x170420,0xcd);// script cc.BIT[3]continue mode.After syncing to the preamble, it will immediately enter
							  //the sync state again, reducing the probability of mis-syncing.modified by zhiwei,confirmed
							  //by qiangkai and xuqiang.20221205
	write_reg8(0x170422,0x00);
	write_reg8(0x17044d,0x01);
	write_reg8(0x17044e,0xf0);
	write_reg16(0x170438,0x7dc8);
	write_reg8(0x170473,0xa1);

	write_reg8(0x17049a,0x00);//tx_tp_align.

	write_reg16(0x1704c2,0x4b3a);
	write_reg32(0x1704c4,0x7a6e6356);
	write_reg8(0x1704c8,0x39);//bit[0:5]grx_fix
	#if 1
		write_reg32(0x170000,0x4440081f | PRMBL_LENGTH_Coded<<16);
	#else
		write_reg32(0x170000,0x444a081f);
	#endif

	write_reg8(0x1704bb,0x50);//BIT[5]:rxc_chf_sel_ble;1M:0(default) 2M:1 open two stage filter to improve
							  //the sensitivity.modified by zhiwei,confirmed by wenfeng and xuqiang,20230106.
}


static inline void rf_ble_set_coded_phy_s2(void)
{
	write_reg16(0x170436,0x0cee);
	write_reg16(0x170004,0xa4f5);

}


static inline void rf_ble_set_coded_phy_s8(void)
{
	write_reg16(0x170436,0x0cf6);
	write_reg16(0x170004,0xb4f5);
}

//This is to be compatible in older versions. If you don't use them, you can delete them.
#define rf_trigle_codedPhy_accesscode rf_trigger_codedPhy_accesscode


#if FAST_SETTLE
	typedef struct
	{
		u8 LDO_CAL_TRIM;	//0xea[5:0]
		u8 LDO_RXTXHF_TRIM;	//0xee[5:0]
		u8 LDO_RXTXLF_TRIM;	//0xee[7:6]  0xed[3:0]
		u8 LDO_PLL_TRIM;	//0xee[5:0]
		u8 LDO_VCO_TRIM;	//0xee[7:6]  0xef[3:0]
		u8 rsvd;
	}Ldo_Trim;

	typedef struct
	{
		unsigned char tx_fast_en;
		unsigned char rx_fast_en;
		unsigned short cal_tbl[40];
		rf_ldo_trim_t	ldo_trim;
		rf_dcoc_cal_t   dcoc_cal;
	}Fast_Settle;
	extern Fast_Settle fast_settle;

	void ble_rf_tx_fast_settle(void);
	void ble_rf_rx_fast_settle(void);
	unsigned short get_rf_hpmc_cal_val(void);
	void set_rf_hpmc_cal_val(unsigned short value);
	unsigned char ble_is_rf_tx_fast_settle_en(void);
	unsigned char ble_is_rf_rx_fast_settle_en(void);
	void get_ldo_trim_val(u8* p);
	void set_ldo_trim_val(u8* p);
	void set_ldo_trim_on(void);

	/**
	 *	@brief	  	this function serve to enable the tx timing sequence adjusted.
	 *	@param[in]	none
	 *	@return	 	none
	*/
	void ble_rf_tx_fast_settle_en(void);

	/**
	 *	@brief	  	this function serve to disable the tx timing sequence adjusted.
	 *	@param[in]	none
	 *	@return	 	none
	*/
	void ble_rf_tx_fast_settle_dis(void);


	/**
	 *	@brief	  	this function serve to enable the rx timing sequence adjusted.
	 *	@param[in]	none
	 *	@return	 	none
	*/
	void ble_rf_rx_fast_settle_en(void);


	/**
	 *	@brief	  	this function serve to disable the rx timing sequence adjusted.
	 *	@param[in]	none
	 *	@return	 	none
	*/
	void ble_rf_rx_fast_settle_dis(void);



#endif

static inline u8 rf_ble_get_tx_pwr_idx(s8 rfTxPower)
{
    rf_power_level_index_e rfPwrLvlIdx;

    /*VBAT*/
    if      (rfTxPower >=   9)  {  rfPwrLvlIdx = RF_POWER_INDEX_P9p15dBm;  }
    else if (rfTxPower >=   8)  {  rfPwrLvlIdx = RF_POWER_INDEX_P8p25dBm;  }
    else if (rfTxPower >=   7)  {  rfPwrLvlIdx = RF_POWER_INDEX_P7p00dBm;  }
    else if (rfTxPower >=   6)  {  rfPwrLvlIdx = RF_POWER_INDEX_P6p32dBm;  }
    /*VANT*/
    else if (rfTxPower >=   5)  {  rfPwrLvlIdx = RF_POWER_INDEX_P5p21dBm;  }
    else if (rfTxPower >=   4)  {  rfPwrLvlIdx = RF_POWER_INDEX_P4p02dBm;  }
    else if (rfTxPower >=   3)  {  rfPwrLvlIdx = RF_POWER_INDEX_P3p00dBm;  }
    else if (rfTxPower >=   2)  {  rfPwrLvlIdx = RF_POWER_INDEX_P2p01dBm;  }
    else if (rfTxPower >=   1)  {  rfPwrLvlIdx = RF_POWER_INDEX_P1p03dBm;  }
    else if (rfTxPower >=   0)  {  rfPwrLvlIdx = RF_POWER_INDEX_P0p01dBm;  }
    else if (rfTxPower >=  -4)  {  rfPwrLvlIdx = RF_POWER_INDEX_N3p95dBm;  }
    else if (rfTxPower >=  -6)  {  rfPwrLvlIdx = RF_POWER_INDEX_N5p94dBm;  }
    else if (rfTxPower >= -10)  {  rfPwrLvlIdx = RF_POWER_INDEX_N9p03dBm; }
    else if (rfTxPower >= -14)  {  rfPwrLvlIdx = RF_POWER_INDEX_N13p42dBm; }
    else                        {  rfPwrLvlIdx = RF_POWER_INDEX_N22p53dBm; }

    return rfPwrLvlIdx;
}

static inline s8 rf_ble_get_tx_pwr_level(rf_power_level_index_e rfPwrLvlIdx)
{
    s8 rfTxPower;

    /*VBAT*/
    if      (rfPwrLvlIdx <= RF_POWER_INDEX_P9p15dBm)  {  rfTxPower =   9;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P8p25dBm)  {  rfTxPower =   8;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P7p00dBm)  {  rfTxPower =   7;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P6p32dBm)  {  rfTxPower =   6;  }
    /*VANT*/
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P5p21dBm)  {  rfTxPower =   5;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P4p02dBm)  {  rfTxPower =   4;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P3p00dBm)  {  rfTxPower =   3;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P2p01dBm)  {  rfTxPower =   2;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P1p03dBm)  {  rfTxPower =   1;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_P0p01dBm)  {  rfTxPower =   0;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N3p95dBm)  {  rfTxPower =  -4;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N5p94dBm)  {  rfTxPower =  -6;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N9p03dBm)  {  rfTxPower = -10;  }
    else if (rfPwrLvlIdx <= RF_POWER_INDEX_N13p42dBm) {  rfTxPower = -14;  }
    else                                              {  rfTxPower = -23;  }

    return rfTxPower;
}

void ble_rf_tx_channel_sounding_mode_en(void);

void ble_rf_tx_channel_sounding_mode_dis(void);

void ble_rf_rx_channel_sounding_mode_en(unsigned char interval, rf_iq_data_mode_e suppmode);

void ble_rf_rx_channel_sounding_mode_dis(void);

void ble_rf_channel_sounding_iq_sample_config(unsigned short sample_num, unsigned char start_point, rf_hadm_iq_sample_mode_e sample_mode);

/******************************* ext_rf end ********************************************************************/




/******************************* ext_uart start ******************************************************************/

/******************************* ext_uart end ********************************************************************/


/******************************* mcu_security start ******************************************************************/
#define SECBOOT_DESC_SECTOR_NUM		2
#define SECBOOT_DESC_SIZE			0x2000  //8K for secure boot descriptor size

#define DESCRIPTOR_PUBKEY_OFFSET	0x1002

#define DESCRIPTOR_WATCHDOG_OFFSET	0x108A

typedef struct {
	unsigned char vendor_mark[4];
} sb_desc_1st_sector_t;

#define DESC_1ST_SECTOR_DATA_LEN      4
#define DESC_2ND_SECTOR_DATA_LEN	146  //16*9 + 2 = 144 + 2


typedef struct {
	unsigned short	multi_boot;
	unsigned char	public_key[64];
	unsigned char	signature[64];
	unsigned int	run_code_adr;   //4 byte
	unsigned int	run_code_size;
	unsigned char	watdog_v[4];
	unsigned char	smpi_lane[4];
} sb_desc_2nd_sector_t;


typedef struct {
	unsigned char	fw_enc_en;
	unsigned char	secboot_en;
	unsigned short  sb_desc_adr_k; //unit: 4KB
} mcu_secure_t;
extern mcu_secure_t  mcuSecur;

bool mcu_securuty_read_efuse(void);
bool mcu_securuty_read_idcode(void);
bool efuse_get_pubkey_hash(u8* pHash);
/******************************* mcu_security end ********************************************************************/

#endif /* DRIVERS_B93_EXT_DRIVER_EXT_LIB_H_ */
