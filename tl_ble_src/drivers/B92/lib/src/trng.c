/********************************************************************************************************
 * @file    trng.c
 *
 * @brief   This is the source file for B92
 *
 * @author  Driver Group
 * @date    2020
 *
 * @par     Copyright (c) 2020, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/trng.h"
#include "compiler.h"
#include "core.h"
/**********************************************************************************************************************
 *                                            local constants                                                       *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              local macro                                                        *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                             local data type                                                     *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              global variable                                                       *
 *********************************************************************************************************************/

_attribute_data_retention_sec_ unsigned int g_rnd_m_w = 0;
_attribute_data_retention_sec_ unsigned int g_rnd_m_z = 0;

/**
 * trng error timeout(us),a large value is set by default,can set it by trng_set_error_timeout().
 */
unsigned int g_trng_error_timeout_us = 0xffffffff;

/**********************************************************************************************************************
 *                                              local variable                                                     *
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *                                          local function prototype                                               *
 *********************************************************************************************************************/
/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/

/**
 * @brief     This function serves to trng hardware reset(the hardware device is reset to restore it to its initial state (restart the device), the initialization needs to be reconfigured).
 * @return    none.
 */
void trng_hw_reset(void)
{
    reg_rst2 &= (~FLD_RST2_TRNG);
    reg_rst2 |= FLD_RST2_TRNG;
}

/**
 * @brief     This function serves to set the trng timeout(us).
 * @param[in] timeout_us - the timeout(us).
 * @return    none.
 * @note      The default timeout (g_trng_error_timeout_us) is the larger value.If the timeout exceeds the feed dog time and triggers a watchdog restart,
 *            g_trng_error_timeout_us can be changed to a smaller value via this interface, depending on the application.
 *            g_trng_error_timeout_us the minimum time must meet the following conditions:
 *            1. at least 1ms;
 *            2. maximum interrupt processing time;
 */
void trng_set_error_timeout(unsigned int timeout_us)
{
    g_trng_error_timeout_us = timeout_us;
}

/**
 * @brief     This function serves to record the api status.
 * @param[in] trng_error_timeout_code - trng_api_error_code_e.
 * @return    none.
 * @note      This function can be rewritten according to the application scenario,The parameters of the interface are useless(only one reason for an error,do not need to use enumeration to distinguish it),
 *            if record the details of the reason, can implement it by yourself,trng_hw_reset must be called.
 */
__attribute__((weak)) void trng_timeout_handler(unsigned int trng_error_timeout_code)
{
    (void)trng_error_timeout_code;
    trng_hw_reset();
}

/**
 * @brief     This function serves to check whether the trng is ready.
 * @return    0:ready   1:no ready.
 */
static bool trng_is_ready(void)
{
    return !(reg_rbg_sr & FLD_RBG_SR_DRDY);
}

#define TRNG_WAIT() wait_condition_fails_or_timeout(trng_is_ready, g_trng_error_timeout_us, trng_timeout_handler, (unsigned int)0)

/**
 * @brief     This function performs to get one random number.If chip in suspend TRNG module should be close.
 *            else its current will be larger.
 * @return    DRV_API_SUCCESS: operation successful;
 *            DRV_API_TIMEOUT: timeout exit(g_trng_error_timeout_us refer to the note for trng_set_error_timeout,the solution processing is already done in trng_timeout_handler, so just re-invoke the interface);
 */
drv_api_status_e trng_init(void)
{
    //TRNG module Reset clear
    reg_rst2 |= FLD_RST2_TRNG;
    //turn on TRNG clock
    reg_clk_en2 |= FLD_CLK2_TRNG_EN;

    reg_trng_cr0 &= ~(FLD_TRNG_CR0_RBGEN); //disable
    reg_trng_rtcr = 0x00;                  //TCR_MSEL
    reg_trng_cr0 |= (FLD_TRNG_CR0_RBGEN);  //enable

    if (TRNG_WAIT()) {
        return DRV_API_TIMEOUT;
    }
    g_rnd_m_w = reg_rbg_dr; //get the random number
    if (TRNG_WAIT()) {
        return DRV_API_TIMEOUT;
    }
    g_rnd_m_z = reg_rbg_dr;

    //Reset TRNG module
    reg_rst2 &= (~FLD_RST2_TRNG);
    //turn off TRNG module clock
    reg_clk_en2 &= ~(FLD_CLK2_TRNG_EN);

    reg_trng_cr0 &= ~(FLD_TRNG_CR0_RBGEN | FLD_TRNG_CR0_ROSEN0 | FLD_TRNG_CR0_ROSEN1 | FLD_TRNG_CR0_ROSEN2 | FLD_TRNG_CR0_ROSEN3);

    return DRV_API_SUCCESS;
}

/**
 * @brief     This function performs to get one random number.
 * @return    the value of one random number.
 */
_attribute_ram_code_sec_noinline_ unsigned int trng_rand(void) //16M clock, code in flash 23us, code in sram 4us
{
    g_rnd_m_w           = 18000 * (g_rnd_m_w & 0xffff) + (g_rnd_m_w >> 16);
    g_rnd_m_z           = 36969 * (g_rnd_m_z & 0xffff) + (g_rnd_m_z >> 16);
    unsigned int result = (g_rnd_m_z << 16) + g_rnd_m_w;

    return (unsigned int)(result ^ stimer_get_tick());
}

/**********************************************************************************************************************
  *                                         local function implementation                                             *
  *********************************************************************************************************************/
