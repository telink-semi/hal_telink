/********************************************************************************************************
 * @file	ext_lib.h
 *
 * @brief	This is the header file for BLE SDK
 *
 * @author	BLE GROUP
 * @date	06,2022
 *
 * @par		Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *			All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#ifndef DRIVERS_TL321X_EXT_DRIVER_EXT_LIB_H_
#define DRIVERS_TL321X_EXT_DRIVER_EXT_LIB_H_


#include "types.h"
#include "../../uart.h"
#include "../../lib/include/pm.h"
#include "../../lib/include/rf.h"
#include <stdbool.h>
#include "../ext_pm.h"




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



#define PKE_OPERAND_MAX_WORD_LEN      (0x08)
#define PKE_OPERAND_MAX_BIT_LEN       (0x100)
#define ECC_MAX_WORD_LEN              PKE_OPERAND_MAX_WORD_LEN



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
	reg_irq_src0 |= BIT(IRQ_SYSTIMER);
	//plic_interrupt_enable(IRQ_SYSTIMER);
}

/**
 * @brief    This function serves to disable system timer interrupt.
 * @return  none
 */
static inline void systimer_irq_disable(void)
{
	reg_irq_src0 &= ~BIT(IRQ_SYSTIMER);
	//plic_interrupt_disable(IRQ_SYSTIMER);
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
 * @brief analog register below can store information when MCU in deepsleep mode
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
typedef struct __attribute__((packed))   pm_clock_drift
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
_attribute_ram_code_com_sec_noinline_ void pm_ble_cal_32k_rc_offset (int offset_tick, int rc32_cnt);

/**
 * @brief		This function reset calibrates the value
 * @param[in]	none
 * @return		none.
 */
void pm_ble_32k_rc_cal_reset(void);
#define PM_MIN_SLEEP_US			1500  //B92 todo

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








/******************************* mcu_security start ******************************************************************/
#define SECBOOT_DESC_SECTOR_NUM		2
#define SECBOOT_DESC_SIZE			0x2000  //8K for secure boot descriptor size

#define DESCRIPTOR_PUBKEY_OFFSET	0x1002

#define DESCRIPTOR_WATCHDOG_OFFSET	0x108A

typedef struct __attribute__((packed))  {
	unsigned char vendor_mark[4];
} sb_desc_1st_sector_t;

#define DESC_1ST_SECTOR_DATA_LEN      4
#define DESC_2ND_SECTOR_DATA_LEN	146  //16*9 + 2 = 144 + 2


typedef struct __attribute__((packed))  {
	unsigned short	multi_boot;
	unsigned char	public_key[64];
	unsigned char	signature[64];
	unsigned int	run_code_adr;   //4 byte
	unsigned int	run_code_size;
	unsigned char	watdog_v[4];
	unsigned char	smpi_lane[4];
} sb_desc_2nd_sector_t;


typedef struct __attribute__((packed))  {
	unsigned char	fw_enc_en;
	unsigned char	secboot_en;
	unsigned short  sb_desc_adr_k; //unit: 4KB
} mcu_secure_t;
extern mcu_secure_t  mcuSecur;

bool mcu_securuty_read_efuse(void);
bool mcu_securuty_read_idcode(void);
bool efuse_get_pubkey_hash(u8* pHash);
/******************************* mcu_security end ********************************************************************/

#endif /* DRIVERS_TL321X_EXT_DRIVER_EXT_LIB_H_ */
