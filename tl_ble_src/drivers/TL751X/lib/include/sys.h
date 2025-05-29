/********************************************************************************************************
 * @file    sys.h
 *
 * @brief   This is the header file for TL751X
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
/** @page SYS
 *
 *  Introduction
 *  ===============
 *  Clock init and system timer delay.
 *
 *  API Reference
 *  ===============
 *  Header File: sys.h
 */

#ifndef SYS_H_
#define SYS_H_
#include "reg_include/stimer_reg.h"
#include "clock.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
/**
 * brief instruction delay
 */

#define _ASM_NOP_                   __asm__ __volatile__("nop")

#define CLOCK_DLY_1_CYC             _ASM_NOP_
#define CLOCK_DLY_2_CYC             _ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_3_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_4_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_5_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_6_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_7_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_8_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_9_CYC             _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_10_CYC            _ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_;_ASM_NOP_
#define CLOCK_DLY_24_CYC            CLOCK_DLY_6_CYC;CLOCK_DLY_6_CYC;CLOCK_DLY_6_CYC;CLOCK_DLY_6_CYC
#define CLOCK_DLY_64_CYC            CLOCK_DLY_10_CYC;CLOCK_DLY_10_CYC;CLOCK_DLY_10_CYC;CLOCK_DLY_10_CYC;CLOCK_DLY_10_CYC;CLOCK_DLY_10_CYC;CLOCK_DLY_4_CYC

/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/


/**
 * @brief   The maximum voltage that the chip can withstand is 3.6V.
 *          This setting is quite important, please make sure to check how to use it clearly.
 *          In A0 version, if not set up correctly may cause an increase in current.
 *          If need to change vbat type after sys_init(), call the sys_set_vbat_type().
*/
typedef enum{
    VBAT_MAX_VALUE_GREATER_THAN_3V6 = 0x00,     /* 
                                                    When the VBAT is greater than 3.6V, the vbat_v need to set as VBAT_MAX_VALUE_LESS_THAN_3V6 mode, 
                                                    then the bypass is closed and the vbat voltage passes through an LDO to supply power to the chip.
                                                    The voltage of the GPIO pin (VOH) is the voltage after VBAT passes through the LDO (V_ldo),
                                                    and the maximum value is about 3.3V floating 10% (V_ldoh).
                                                    When VBAT > V_ldoh, <p>VOH = V_ldo = V_ldoh.
                                                    When VBAT < V_ldoh, <p>VOH = V_ldo = VBAT.
                                                */
    VBAT_MAX_VALUE_LESS_THAN_3V6    = BIT(3),   /*
                                                    When the VBAT is less than 3.6V, the vbat_v need to set as VBAT_MAX_VALUE_LESS_THAN_3V6 mode, 
                                                    then the bypass is turned on and the vbat voltage directly supplies power to the chip.
                                                    VOH(the output voltage of GPIO)= VBAT.
                                                */
}vbat_type_e;

/**
 * @brief   chip version.
 * @note    this value should confirm when chip reversion.
 */
typedef enum{
    CHIP_VERSION_A0 = 0x00,
    CHIP_VERSION_A1 = 0x10,
}sys_chip_version_e;



typedef enum{
   D25F = BIT(0),
   N22  = BIT(1),
   DSP  = BIT(2),
}sys_core_e;

/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/

extern unsigned int g_chip_version;

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/
/**
 * @brief      This function reboot mcu.
 * @return     none
 */
_attribute_text_sec_ void sys_reboot(void);

/**
 * @brief       This function serves to initialize system.
 * @param[in]   vbat_v  - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 *                      - Please refer to vbat_type_e for specific usage precautions.
 * @return      none
 * @note        For crystal oscillator with slow start-up or poor quality, after calling this function, 
 *              a reboot will be triggered(whether a reboot has occurred can be judged by using PM_ANA_REG_POWER_ON_CLR_BUF0[bit1]).
 *              For the case where the crystal oscillator used is very slow to start-up, you can call the pm_set_xtal_stable_timer_param interface 
 *              to adjust the waiting time for the crystal oscillator to start before calling the sys_init interface.
 *              When this time is adjusted to meet the crystal oscillator requirements, it will not reboot.
 */
void sys_init(vbat_type_e vbat_v);

/**
 * @brief       This function serves to set vbat type. 
 * @param[in]   vbat_v  - This parameter is used to determine whether the VBAT voltage can be greater than 3.6V.
 *                      - Please refer to vbat_type_e for specific usage precautions.
 * @return      none
 */
void sys_set_vbat_type(vbat_type_e vbat_v);

/**
 * @brief       This function serves to initialize dsp core system.
 * @param[in]  addr - start up address
 * @return      none
 * @note        Only after calling this function can other DSP related functions be called. 
 *              Otherwise, other DSP function settings will not take effect.
 */
void sys_dsp_init(unsigned int addr);

/**
 * @brief       This function serves to start dsp core system.
 * @return      none
 */
void sys_dsp_start(void);

/**
 * @brief      This function serves to initialize n22 core system.
 * @param[in]  addr - start up address
 * @return     none
 * @note        Only after calling this function can other N22 related functions be called. 
 *              Otherwise, other N22 function settings will not take effect.
 */
void sys_n22_init(unsigned int addr);

/**
 * @brief       This function serves to start n22 core system.
 * @return      none
 */
void sys_n22_start(void);

/**
 * @brief       This function serves to stall n22 by dis n22 core/ram clock.
 * @return      none
 */
void sys_n22_clk_dis(void);

/**
 * @brief       This function serves to start n22.
 * @return      none
 */
void sys_n22_clk_en(void);

/**
 * @brief       This function serves to stall dsp by dis dsp clock.
 * @return      none
 */
void sys_dsp_clk_dis(void);

/**
 * @brief       This function serves to start dsp.
 * @return      none
 */
void sys_dsp_clk_en(void);

/**
 * @brief       This function serves to save n22 clk reg.
 * @return      none
 */
void sys_n22_clk_reg_save(void);

/**
 * @brief       This function serves to restore n22 clk reg.
 * @return      none
 */
void sys_n22_clk_reg_restore(void);

/**
 * @brief       This function serves to save dsp clk reg.
 * @return      none
 */
void sys_dsp_clk_reg_save(void);

/**
 * @brief       This function serves to restore dsp clk reg.
 * @return      none
 */
void sys_dsp_clk_reg_restore(void);

/**
 * @brief       This function serves to judge whether the module power on.
 * @module      - pm_pd_module_e
 * @return      1: power down   0:power on.
 */
_Bool pm_dig_module_is_power_on(pm_pd_module_e module);

/**
 * @brief       This function serves to judge whether n22 reg write or read.
 * @return      none
 */
unsigned char sys_n22_is_reg_rw_permitted(void);

/**
 * @brief       This function serves to judge whether dsp reg write or read.
 * @return      none
 */
unsigned char sys_dsp_is_reg_rw_permitted(void);

/**
 * @brief       This function serves to judge whether dsp/n22 is init.
 * @param[in]   core  - sys_core_e(n22/dsp)
 * @return      0: is init   1/2: no init
 */
unsigned int sys_core_is_initialized(sys_core_e core);
#endif
