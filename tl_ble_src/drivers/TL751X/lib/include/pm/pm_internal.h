/********************************************************************************************************
 * @file    pm_internal.h
 *
 * @brief   This is the header file for TL751X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#pragma once

#include "reg_include/register.h"
#include "compiler.h"
#include "analog.h"

/********************************************************************************************************
 *                                          internal
 *******************************************************************************************************/

/********************************************************************************************************
 *              This is currently included in the H file for compatibility with other SDKs.
 *******************************************************************************************************/

//When the watchdog comes back, the Eagle chip does not clear 0x7f[0]. To avoid this problem, this macro definition is added.
#ifndef WDT_REBOOT_RESET_ANA7F_WORK_AROUND
#define WDT_REBOOT_RESET_ANA7F_WORK_AROUND  1
#endif

/********************************************************************************************************
 *              This is just for internal debug purpose, users are prohibited from calling.
 *******************************************************************************************************/

/*
 * @note    This is for internal stability debugging use only.
 */
#define PM_DEBUG                        0

/**
 * @brief   active mode AVDD1 output trim definition
 * @note    The voltage values of the following gears are all theoretical values, and there may be deviations between the actual and theoretical values.
 */
typedef enum {
    PM_AVDD1_VOLTAGE_1V050  = 0x00, /**< AVDD1 output 1.050V */
    PM_AVDD1_VOLTAGE_1V075  = 0x01, /**< AVDD1 output 1.075V */
    PM_AVDD1_VOLTAGE_1V100  = 0x02, /**< AVDD1 output 1.100V */
    PM_AVDD1_VOLTAGE_1V125  = 0x03, /**< AVDD1 output 1.125V */
    PM_AVDD1_VOLTAGE_1V150  = 0x04, /**< AVDD1 output 1.150V */
    PM_AVDD1_VOLTAGE_1V175  = 0x05, /**< AVDD1 output 1.175V */
    PM_AVDD1_VOLTAGE_1V200  = 0x06, /**< AVDD1 output 1.200V */
    PM_AVDD1_VOLTAGE_1V225  = 0x07, /**< AVDD1 output 1.225V */
}pm_avdd1_voltage_e;

/**
 * @brief   active mode DVDD2 output trim definition
 * @note    The voltage values of the following gears are all theoretical values, and there may be deviations between the actual and theoretical values.
 */
typedef enum {
    PM_DVDD2_VOLTAGE_0V750  = 0x00, /**< DVDD2 output 0.750V */
    PM_DVDD2_VOLTAGE_0V775  = 0x01, /**< DVDD2 output 0.775V */
    PM_DVDD2_VOLTAGE_0V800  = 0x02, /**< DVDD2 output 0.800V */
    PM_DVDD2_VOLTAGE_0V825  = 0x03, /**< DVDD2 output 0.825V */
    PM_DVDD2_VOLTAGE_0V850  = 0x04, /**< DVDD2 output 0.850V */
    PM_DVDD2_VOLTAGE_0V875  = 0x05, /**< DVDD2 output 0.875V */
    PM_DVDD2_VOLTAGE_0V900  = 0x06, /**< DVDD2 output 0.900V */
    PM_DVDD2_VOLTAGE_0V925  = 0x07, /**< DVDD2 output 0.925V */
}pm_dvdd2_voltage_e;

/**
 * @brief   active mode AVDD2 output trim definition
 * @note    The voltage values of the following gears are all theoretical values, and there may be deviations between the actual and theoretical values.
 */
typedef enum {
    PM_AVDD2_VOLTAGE_2V346  = 0x00, /**< AVDD2 output 2.346V */
    PM_AVDD2_VOLTAGE_2V382  = 0x01, /**< AVDD2 output 2.382V */
    PM_AVDD2_VOLTAGE_2V420  = 0x02, /**< AVDD2 output 2.420V */
    PM_AVDD2_VOLTAGE_2V459  = 0x03, /**< AVDD2 output 2.459V */
    PM_AVDD2_VOLTAGE_2V500  = 0x04, /**< AVDD2 output 2.500V */
    PM_AVDD2_VOLTAGE_2V541  = 0x05, /**< AVDD2 output 2.541V */
    PM_AVDD2_VOLTAGE_2V584  = 0x06, /**< AVDD2 output 2.584V */
    PM_AVDD2_VOLTAGE_2V629  = 0x07, /**< AVDD2 output 2.629V */
}pm_avdd2_voltage_e;

/**
 * @brief   active mode DVDD1 output trim definition
 * @note    The voltage values of the following gears are all theoretical values, and there may be deviations between the actual and theoretical values.
 */
typedef enum {
    PM_DVDD1_VOLTAGE_0V725  = 0x00, /**< DVDD1 output 0.725V */
    PM_DVDD1_VOLTAGE_0V750  = 0x01, /**< DVDD1 output 0.750V */
    PM_DVDD1_VOLTAGE_0V775  = 0x02, /**< DVDD1 output 0.775V */
    PM_DVDD1_VOLTAGE_0V800  = 0x03, /**< DVDD1 output 0.800V */
    PM_DVDD1_VOLTAGE_0V825  = 0x04, /**< DVDD1 output 0.825V */
    PM_DVDD1_VOLTAGE_0V850  = 0x05, /**< DVDD1 output 0.850V */
    PM_DVDD1_VOLTAGE_0V875  = 0x06, /**< DVDD1 output 0.875V */
    PM_DVDD1_VOLTAGE_0V900  = 0x07, /**< DVDD1 output 0.900V */
}pm_dvdd1_voltage_e;

/**
 * @brief   active mode retention LDO output trim definition
 */
typedef enum {
    PM_RETLDO_VOLTAGE_0V55  = 0x00, /**< retention LDO output 0.725V */
    PM_RETLDO_VOLTAGE_0V60  = 0x01, /**< retention LDO output 0.750V */
    PM_RETLDO_VOLTAGE_0V65  = 0x02, /**< retention LDO output 0.775V */
    PM_RETLDO_VOLTAGE_0V70  = 0x03, /**< retention LDO output 0.800V */
    PM_RETLDO_VOLTAGE_0V75  = 0x04, /**< retention LDO output 0.825V */
    PM_RETLDO_VOLTAGE_0V80  = 0x05, /**< retention LDO output 0.850V */
    PM_RETLDO_VOLTAGE_0V85  = 0x06, /**< retention LDO output 0.875V */
    PM_RETLDO_VOLTAGE_0V90  = 0x07, /**< retention LDO output 0.900V */
}pm_retldo_voltage_e;


/**
 * @brief       This function serves to reboot system.
 * @return      none
 */
_always_inline void sys_reset_all(void)
{
    reg_pwdn_en = 0x20;
}

/**
 * @brief       this function servers to wait bbpll clock lock.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void pm_wait_bbpll_done(void);

/**
 * @brief       This function is used to determine the stability of the crystal oscillator.
 *              To judge the stability of the crystal oscillator, xo_ready_ana is invalid, and use an alternative solution to judge.
 *              Alternative principle: Because the clock source of the stimer is the crystal oscillator,
 *              if the crystal oscillator does not vibrate, the tick value of the stimer does not increase or increases very slowly (when there is interference).
 *              So first use 24M RC to run the program and wait for a fixed time, calculate the number of ticks that the stimer should increase during this time,
 *              and then read the tick value actually increased by the stimer.
 *              When it reaches 50% of the calculated value, it proves that the crystal oscillator has started.
 *              If it is not reached for a long time, the system will reboot.
 * @param[in]   all_ramcode_en  - Whether all processing in this function is required to be ram code. If this parameter is set to 1, it requires that:
 *              before calling this function, you have done the disable BTB, disable interrupt, mspi_stop_xip and other operations as the corresponding function configured to 0.
 * @attention   This function can only be called with the 24M clock configuration
 * @return      none.
 */
_attribute_ram_code_sec_ void pm_wait_xtal_ready(unsigned char all_ramcode_en);

/**
 * @brief       This function serves to set AVDD1.
 * @param[in]   voltage - avdd1 setting gear.
 * @return      none.
 */
static inline void pm_set_avdd1(pm_avdd1_voltage_e voltage)
{
    analog_write_reg8(0x04, (analog_read_reg8(0x04) & 0xf8) | voltage);
}

/**
 * @brief       This function serves to set AVDD2.
 * @param[in]   voltage - avdd2 setting gear, can be set from 0 to 7.
 * @return      none.
 */
static inline void pm_set_avdd2(pm_avdd2_voltage_e voltage)
{
    analog_write_reg8(0x22, (analog_read_reg8(0x22) & 0xf8) | voltage);
}

/**
 * @brief       This function serves to set DVDD1.
 * @param[in]   voltage - dvdd1 setting gear, can be set from 0 to 7.
 * @return      none.
 */
static inline void pm_set_dvdd1(pm_dvdd1_voltage_e voltage)
{
    analog_write_reg8(0x22, (analog_read_reg8(0x22) & 0x8f) | (voltage << 4));
}

/**
 * @brief       This function serves to set DVDD2.
 * @param[in]   voltage - dvdd2 setting gear, can be set from 0 to 7.
 * @return      none.
 */
static inline void pm_set_dvdd2(pm_dvdd2_voltage_e voltage)
{
    analog_write_reg8(0x21, (analog_read_reg8(0x21) & 0xf8) | voltage);
}

/**
 * @brief       This function serves to set low power DVDD1.
 * @param[in]   voltage - dvdd1 setting gear, can be set from 0 to 7.
 * @return      none.
 */
static inline void pm_set_lc_dvdd1(pm_dvdd1_voltage_e voltage)
{
    analog_write_reg8(0x0b, (analog_read_reg8(0x0b) & 0xf8) | voltage);
}

/**
 * @brief       This function serves to set low power DVDD2.
 * @param[in]   voltage - dvdd2 setting gear, can be set from 0 to 7.
 * @return      none.
 */
static inline void pm_set_lc_dvdd2(pm_dvdd2_voltage_e voltage)
{
    analog_write_reg8(0x0b, (analog_read_reg8(0x0b) & 0x8f) | (voltage << 4));
}

/**
 * @brief       This function serves to set retention LDO.
 * @param[in]   voltage - retention LDO setting gear, can be set from 0 to 7.
 * @return      none.
 */
static inline void pm_set_ret_ldo(pm_retldo_voltage_e voltage)
{
    analog_write_reg8(0x10, (analog_read_reg8(0x10) & 0xf8) | voltage);
}


