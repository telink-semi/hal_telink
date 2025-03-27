/********************************************************************************************************
 * @file    ext_lib.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
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
#ifndef DRIVERS_B91_EXT_DRIVER_EXT_LIB_H_
#define DRIVERS_B91_EXT_DRIVER_EXT_LIB_H_


#include "types.h"
#include "../../lib/include/pm.h"
#include "../../lib/include/rf.h"

/******************************* dbgport start ******************************************************************/
#define reg_bt_dbg_sel      REG_ADDR16(0x140354)
#define reg_bt_dbg_sel_l    REG_ADDR8(0x140354)
#define reg_bt_dbg_sel_h    REG_ADDR8(0x140355)

typedef enum{
    BT_DBG0_BB0_A0  = GPIO_PA0,
    BT_DBG0_BB1_A1  = GPIO_PA1,
    BT_DBG0_BB2_A2  = GPIO_PA2,
    BT_DBG0_BB3_A3  = GPIO_PA3,
    BT_DBG0_BB4_A4  = GPIO_PA4,
    BT_DBG0_BB5_B0  = GPIO_PB0,
    BT_DBG0_BB6_B1  = GPIO_PB1,
    BT_DBG0_BB7_B2  = GPIO_PB2,

    BT_DBG1_BB8_B3  = GPIO_PB3,
    BT_DBG1_BB9_B4  = GPIO_PB4,
    BT_DBG1_BB10_B5 = GPIO_PB5,
    BT_DBG1_BB11_B6 = GPIO_PB6,
    BT_DBG1_BB12_B7 = GPIO_PB7,
    BT_DBG1_BB13_C0 = GPIO_PC0,
    BT_DBG1_BB14_C1 = GPIO_PC1,
    BT_DBG1_BB15_C2 = GPIO_PC2,

    BT_DBG2_BB16_C3 = GPIO_PC3,
    BT_DBG2_BB17_C4 = GPIO_PC4,
    BT_DBG2_BB18_C5 = GPIO_PC5,
    BT_DBG2_BB19_C6 = GPIO_PC6,
    BT_DBG2_BB20_C7 = GPIO_PC7,
    BT_DBG2_BB21_D0 = GPIO_PD0,
    BT_DBG2_BB22_D1 = GPIO_PD1,
    BT_DBG2_BB23_D2 = GPIO_PD2,

    BT_DBG3_BB24_D3 = GPIO_PD3,
    BT_DBG3_BB25_D4 = GPIO_PD4,
    BT_DBG3_BB26_D5 = GPIO_PD5,
    BT_DBG3_BB27_D6 = GPIO_PD6,
    BT_DBG3_BB28_D7 = GPIO_PD7,
    BT_DBG3_BB29_E0 = GPIO_PE0,
    BT_DBG3_BB30_E1 = GPIO_PE1,
    BT_DBG3_BB31_E2 = GPIO_PE2,
}btdbg_pin_e;


void ble_dbg_port_init(int deg_sel0);
void bt_dbg_set_pin(btdbg_pin_e pin);
/******************************* dbgport end ********************************************************************/





/******************************* ext_aes start ******************************************************************/
extern unsigned int aes_data_buff[8];

void aes_encryption_le(u8* key, u8* plaintext, u8 *encrypted_data);
void aes_encryption_be(u8* key, u8* plaintext, u8 *encrypted_data);
bool aes_resolve_irk_rpa(u8 *key, u8 *addr);

/******************************* ext_aes end ********************************************************************/




/******************************* ext_audio start ******************************************************************/
void audio_set_codec_dac_gain(u8 state);
/******************************* ext_audio end ********************************************************************/




/******************************* ext_codec start ******************************************************************/
void iis_base_init(u8 *Ibuffer,u16 IbufferLen,u8 *Obuffer,u16 ObufferLen);

int codec_input_readData1(u8* pData,u16 pDataLen,u32 wptr);

u32 codec_get_InputWriteOffset(void);

u16 codec_get_InputBuffMaxlen(void);

void codec_set_InputReadOffset(u32 rptr);

u32 codec_get_InputReadOffset(void);

u16 codec_get_OutputBufferLen(void);
/******************************* ext_codec end ********************************************************************/




/******************************* ext_flash start ******************************************************************/

/******************************* ext_flash end ********************************************************************/




/******************************* ext_gpio start ******************************************************************/

/******************************* ext_gpio end ********************************************************************/




/******************************* ext_hci_uart start ******************************************************************/

/******************************* ext_hci_uart end ********************************************************************/





/******************************* ext_stimer start ******************************************************************/
#define SYSTICK_NUM_PER_US              16

#define SSLOT_TICK_NUM                  625/2     //attention: not use "()" for purpose !!!    625uS*16/32=625/2=312.5
#define SSLOT_TICK_REVERSE              2/625     //attention: not use "()" for purpose !!!

/**
 * @brief define system clock tick per us/ms/s.
 */
enum{
    SYSTEM_TIMER_TICK_125US     = 2000,   //125*16
};

typedef enum {
    STIMER_IRQ_MASK             =   BIT(0),
    STIMER_32K_CAL_IRQ_MASK     =   BIT(1),
}stimer_irq_mask_e;

typedef enum {
    STIMER_IRQ_CLR              =   BIT(0),
    STIMER_32K_CAL_IRQ_CLR      =   BIT(1),
}stimer_irq_clr_e;

typedef enum {
    FLD_IRQ_SYSTEM_TIMER            =   BIT(0),
}system_timer_irq_mask_e;

/**
 * @brief    This function serves to enable system timer interrupt.
 * @return  none
 */
__INLINE void systimer_irq_enable(void)
{
    reg_irq_src0 |= BIT(IRQ_SYSTIMER);
    //plic_interrupt_enable(IRQ_SYSTIMER);
}

/**
 * @brief    This function serves to disable system timer interrupt.
 * @return  none
 */
__INLINE void systimer_irq_disable(void)
{
    reg_irq_src0 &= ~BIT(IRQ_SYSTIMER);
    //plic_interrupt_disable(IRQ_SYSTIMER);
}

__INLINE void systimer_set_irq_mask(void)
{
    reg_system_irq_mask |= STIMER_IRQ_MASK;
}

__INLINE void systimer_clr_irq_mask(void)
{
    reg_system_irq_mask &= (~STIMER_IRQ_MASK);
}

__INLINE unsigned char systimer_get_irq_status(void)
{
    return reg_system_cal_irq & FLD_IRQ_SYSTEM_TIMER;
}

__INLINE void systimer_clr_irq_status(void)
{
    reg_system_cal_irq = STIMER_IRQ_CLR;
}

__INLINE void systimer_set_irq_capture(unsigned int tick)
{
    reg_system_irq_level = tick;
}

__INLINE unsigned int systimer_get_irq_capture(void)
{
    return reg_system_irq_level;
}

__INLINE int tick1_exceed_tick2(unsigned int tick1, unsigned int tick2)
{
    return (unsigned int)(tick1 - tick2) < BIT(30);
}


__INLINE int tick1_closed_to_tick2(unsigned int tick1, unsigned int tick2, unsigned int tick_distance)
{
    return (unsigned int)(tick1 + tick_distance - tick2) < (tick_distance<<1);
}

__INLINE int tick1_out_range_of_tick2(unsigned int tick1, unsigned int tick2, unsigned int tick_distance)
{
    return (unsigned int)(tick1 + tick_distance - tick2) > (tick_distance<<1);
}
/******************************* ext_stimer end ********************************************************************/










/******************************* ext_pm start ******************************************************************/
/**
 * There have been several customer complaints about DCDC_1P4_DCDC_1P8.
 * The problem that the driver has tested is that when the power supply voltage exceeds 3.8V,
 * the power supply voltage of flash will exceed the power supply range of flash,
 * but no relatively complete solution has been found.
 * The results of the meeting were (see jira: DRIV-1443 for details) :
 * 1. If 2.8V is used to power flash/codec, DCDC mode is not provided.
 * 2. If the flash is powered at 1.8V and no codec is used, the DCDC mode can be used.
 * (The theoretical analysis of chip colleagues is the case of 1.8V flash power supply,
 * because flash is a wide voltage flash, flash power supply voltage will not exceed the power supply range of flash.)
 * But this has not been verified internally.
 * According to the above situation, the current driver first removes this feature.
 * If there is a real demand in the future,
 * it is necessary to pull the internal chip colleagues to confirm the detailed use method and test related test items.
 * After all the tests have passed, open the feature.
 * changed by weihua.zhang, confirmed by yu.ling, at 20240319.
 */
#define DCDC_1P4_DCDC_1P8_EN                0


#ifndef PM_32k_RC_CALIBRATION_ALGORITHM_EN
#define PM_32k_RC_CALIBRATION_ALGORITHM_EN          1
#endif



#define SYS_NEED_REINIT_EXT32K              BIT(1)
#define WAKEUP_STATUS_TIMER_CORE            ( WAKEUP_STATUS_TIMER | WAKEUP_STATUS_CORE)
#define WAKEUP_STATUS_TIMER_PAD             ( WAKEUP_STATUS_TIMER | WAKEUP_STATUS_PAD)



/**
 * @brief analog register below can store information when MCU in deepsleep mode
 *        store your information in these ana_regs before deepsleep by calling analog_write function
 *        when MCU wakeup from deepsleep, read the information by by calling analog_read function
 *        Reset these analog registers only by power cycle
 */
#define DEEP_ANA_REG0                       0x39 //initial value =0x00
#define DEEP_ANA_REG1                       0x3a //initial value =0x00
#define DEEP_ANA_REG2                       0x3b //initial value =0x00
#define DEEP_ANA_REG3                       0x3c //initial value =0x00
#define DEEP_ANA_REG4                       0x3d //initial value =0x00
#define DEEP_ANA_REG5                       0x3e //initial value =0x00
#define DEEP_ANA_REG6                       0x3f //initial value =0x0f

/**
 * @brief these analog register can store data in deepsleep mode or deepsleep with SRAM retention mode.
 *        Reset these analog registers by watchdog, chip reset, RESET Pin, power cycle
 */

#define DEEP_ANA_REG7                       0x38 //initial value =0xff

//ana3e system used, user can not use
#define SYS_DEEP_ANA_REG                    PM_ANA_REG_POWER_ON_CLR_BUF0

extern  unsigned char           tl_24mrc_cal;
extern  unsigned int            g_pm_tick_32k_calib;
extern  unsigned int            g_pm_tick_cur;
extern  unsigned int            g_pm_tick_32k_cur;
extern  unsigned char           g_pm_long_suspend;
extern  unsigned int            g_pm_mspi_cfg;

extern  unsigned int            g_sleep_32k_rc_cnt;
extern  unsigned int            g_sleep_stimer_tick;

extern unsigned int ota_program_bootAddr;
extern unsigned int ota_firmware_max_size;
extern unsigned int ota_program_offset;


#define PM_MIN_SLEEP_US         1500  //eagle




/**
 * @brief   pm 32k rc calibration algorithm.
 */
typedef struct  pmb_clock_drift
{
    unsigned int    ref_tick;
    unsigned int    ref_tick_32k;
    int             offset;
    int             offset_dc;
//  int             offset_cur;         //BLE SDK use not
    unsigned int    offset_cal_tick;    //BLE SDK use
    int             tc;
    int             rc32;
    int             rc32_wakeup;
    int             rc32_rt;
    int             s0;
    unsigned char   calib;
    unsigned char   ref_no;             //BLE SDK use

} pmb_clock_drift_t;
extern pmb_clock_drift_t    pmbcd;

unsigned int pm_ble_get_32k_rc_calib (void);
void pm_ble_update_32k_rc_sleep_tick (unsigned int tick_32k, unsigned int tick);
void pm_ble_cal_32k_rc_offset (int offset_tick, int rc32_cnt);
void pm_ble_32k_rc_cal_reset(void);

__INLINE unsigned int pm_ble_get_latest_offset_cal_time(void)
{
    return pmbcd.offset_cal_tick;
}






typedef int (*suspend_handler_t)(void);
typedef void (*check_32k_clk_handler_t)(void);
typedef unsigned int (*pm_get_32k_clk_handler_t)(void);
typedef unsigned int (*pm_tim_recover_handler_t)(unsigned int);


extern  suspend_handler_t           func_before_suspend;
extern  pm_get_32k_clk_handler_t    pm_get_32k_tick;
extern  pm_tim_recover_handler_t    pm_tim_recover;
extern void check_32k_clk_stable(void);
extern unsigned int get_32k_tick(void);

void pm_sleep_start();


/**
 * @brief   internal oscillator or crystal calibration for environment change such as voltage, temperature
 *          to keep some critical PM or RF performance stable
 *          attention: this is a stack API, user can not call it
 * @param   none
 * @return  none
 */
void mcu_oscillator_crystal_calibration(void);


/**
 * @brief   This function serves to recover system timer from tick of internal 32k RC.
 * @param   none.
 * @return  none.
 */
unsigned int pm_tim_recover_32k_rc(unsigned int now_tick_32k);

/**
 * @brief   This function serves to recover system timer from tick of external 32k crystal.
 * @param   none.
 * @return  none.
 */
unsigned int pm_tim_recover_32k_xtal(unsigned int now_tick_32k);

/**
 * @brief   This function serves to get the 32k tick.
 * @param   none
 * @return  tick of 32k .
 */
extern unsigned int get_32k_tick(void);

unsigned int clock_get_digital_32k_tick(void);
/******************************* ext_pm end ********************************************************************/







/******************************* ext_uart start ******************************************************************/

/******************************* ext_uart end ********************************************************************/



#endif /* DRIVERS_B91_EXT_DRIVER_EXT_LIB_H_ */
