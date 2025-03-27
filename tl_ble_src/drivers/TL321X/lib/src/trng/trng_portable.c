/********************************************************************************************************
 * @file    trng_portable.c
 *
 * @brief   This is the source file for TL321X
 *
 * @author  Driver Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/trng/trng_portable.h"
#include "compiler.h"
#include "lib/include/core.h"
/**********************************************************************************************************************
 *                                            local constants                                                       *
 *********************************************************************************************************************/


/**********************************************************************************************************************
 *                                              local macro                                                        *
 *********************************************************************************************************************/
/*This mode is to use the random number module, the randomness of obtaining random numbers will be better,
  but the time and power consumption will be large, and there will be compatibility effects with the former.
*/
#define TRUE_RANDOM_MODE                                          1

/*This mode is to obtain a random number seed to be processed so as to obtain a random number,
  the performance of the random number obtained in this mode is poor (compared to the true random number module to obtain random numbers),
  but this mode will greatly reduce the power consumption.
*/
#define PSEUDO_RANDOM_MODE                                        2

#define  TRNG_MODE           PSEUDO_RANDOM_MODE

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
unsigned int g_trng_error_timeout_us  = 0xffffffff;

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
void trng_set_error_timeout(unsigned int timeout_us){
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

#if(TRNG_MODE == PSEUDO_RANDOM_MODE)
/**
 * @brief     This function serves to check whether the trng is ready.
 * @return    0:ready   1:no ready.
 */
static bool trng_is_ready(void){
    return !(rTRNG_SR & BIT(1));
}
#endif

#define TRNG_WAIT()                         wait_condition_fails_or_timeout(trng_is_ready,g_trng_error_timeout_us,trng_timeout_handler,(unsigned int)0)


/**
 * @brief Initialize TRNG-related generic configurations.
 * @note        Only after calling this function can other TRNG related functions be called.
 *              Otherwise, other TRNG function settings will not take effect.
 * @return None.
 * @verbatim
      -# 1.The trng module is mounted on the AHB bus, and the internal configuration clock that controls the trng
           registers is derived from hclk, and the trng sample clock source is roclk.

   @endverbatim
 */
void trng_dig_en(void)
{
    trng_reset();
    trng_clk_en();
}
/**
 * @brief     Disable TRNG module.
 * @return    none
 */
void trng_dig_dis(void)
{
    reg_rst2 &= ~FLD_RST2_TRNG;
    reg_clk_en2 &= ~FLD_CLK2_TRNG_EN;
}

/**
 * @brief     This function initialize trng.
 * @return    none
 * @note
  @verbatim
      -# 1. The power consumption current of trng is about 1ma.
  @endverbatim
 */
drv_api_status_e trng_init(void)
{
#if(TRNG_MODE == TRUE_RANDOM_MODE)
#if 0
    /**
     *1: The default register configuration of the chip's random number module is set up
     *   according to the best possible randomness, so some of the initialization setup
     *   code is commented out to reduce code size and execution time.
     *2: Currently, trng has two entropy sources: RO and TERO, and we have confirmed with OSR
     *   that the randomness will be better if we choose RO as the entropy source.
     *3: The lower the sampling clock of the RO mode, the better the randomization performance,
     *   currently configurable values 1/4, 1/8, 1/16, 1/32 (the sampling clock frequency is the dividing factor of the input clock).
     *4: The random number performance of trng is better when all the entropy sources are turned on, currently there are four entropy
     *   sources (with more entropy sources turned on, the more power trng consumes).
     */

    trng_dig_en();
    /***close ro rng***/
    trng_disable();
    /***set random number mode to true random number mode***/
    trng_set_mode(0);
    /***turn on the four-way ro entropy source***/
    trng_ro_entropy_config(0x0F);
    /***configure the 16-way entropy source subconfiguration for the serial number 0 entropy source***/
    trng_ro_sub_entropy_config(0, 0xFFFF);
    /***configure the 16-way entropy source subconfiguration for the serial number 1 entropy source***/
    trng_ro_sub_entropy_config(1, 0xFFFF);
    /***configure the 16-way entropy source subconfiguration for the serial number 2 entropy source***/
    trng_ro_sub_entropy_config(2, 0xFFFF);
    /***configure the 16-way entropy source subconfiguration for the serial number 3 entropy source***/
    trng_ro_sub_entropy_config(3, 0xFFFF);
    /***setting the ro rng sampling clock frequency***/
    trng_set_freq(3);
    /***enable ro rng***/
    trng_enable();
#endif
#elif(TRNG_MODE == PSEUDO_RANDOM_MODE)
    //TRNG module Reset clear
    reg_rst2 |= FLD_RST2_TRNG;
    //turn on TRNG clock
    reg_clk_en2 |= FLD_CLK2_TRNG_EN;

    rTRNG_CR &= ~BIT(0); //disable
    rTRNG_MSEL = 0x00;               //TCR_MSEL
    rTRNG_CR |= BIT(0); //enable
    trng_reseed();
    if(TRNG_WAIT()){
        return DRV_API_TIMEOUT;
    }
    g_rnd_m_w = rTRNG_DR;   //get the random number
    if(TRNG_WAIT()){
        return DRV_API_TIMEOUT;
    }
    g_rnd_m_z = rTRNG_DR;

    //Reset TRNG module
    reg_rst2 &= (~FLD_RST2_TRNG);
    //turn off TRNG module clock
    reg_clk_en2 &= ~(FLD_CLK2_TRNG_EN);
#endif
    return DRV_API_SUCCESS;

}

/**
 * @brief     This function performs to get one random number.
 * @return    the value of one random number.
 */
int trng_rand(void)
{
#if(TRNG_MODE == TRUE_RANDOM_MODE)
    unsigned char i;
    unsigned char status;
    unsigned char trng_buf[4];
    /**
     *1: The get_rand() he amount of data obtained from this function exceeds 32 bytes, and the random number seed is updated every 32 bytes,
     *   Each time less than 32 bytes of data are obtained, the pseudo-random seed is updated each time this function is called.
     *2: Due to the high power consumption of the trng module, it is recommended to shut down the trng module once it has finished fetching random numbers.
     *3: When trng and pm modules are used together, the trng module needs to be shut down before going to sleep, otherwise the data from the trng module will be an outlier.
     *4: With the CPU running at 120Mhz and the TRNG module operating at 60Mhz, when the function is switched from the true random mode to the pseudo-random mode,
     *   it takes 235us to run the function in memory to get 4 bytes of data, and 269us to run the function in flash memory to get 4 bytes of data.
     */
    trng_dig_en();
    for(i=0;i<6;i++)
    {
        status = get_rand(trng_buf, 4);
        if(status == TRNG_SUCCESS)
        {
            break;
        }
    }
    trng_dig_dis();
    return *(int*)trng_buf;
#elif(TRNG_MODE == PSEUDO_RANDOM_MODE)
    g_rnd_m_w = 18000 * (g_rnd_m_w & 0xffff) + (g_rnd_m_w >> 16);
    g_rnd_m_z = 36969 * (g_rnd_m_z & 0xffff) + (g_rnd_m_z >> 16);
    unsigned int result = (g_rnd_m_z << 16) + g_rnd_m_w;
    return (int)( result  ^ stimer_get_tick() );
#endif
}
