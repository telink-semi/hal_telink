/********************************************************************************************************
 * @file    clock.h
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
/** @page CLOCK
 *
 *  Introduction
 *  ===============
 *  TL751X clock setting.
 *
 *  API Reference
 *  ===============
 *  Header File: clock.h
 */

#ifndef CLOCK_H_
#define CLOCK_H_

#include "compiler.h"
#include "reg_include/register.h"

/**********************************************************************************************************************
 *                                         global constants                                                           *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                           global macro                                                             *
 *********************************************************************************************************************/
/**
 *  @note   1.The clock source for the following clock settings is BASEBAND_PLL (PLL_CLK_192M),
 *            if it is another clock source, you need to call other interfaces to set up by yourself.
 *          2.The following WT_CLK settings are set according to his maximum configurable frequency.
 */
#define     CCLK_192M_HCLK_192M_PCLK_48M_MSPI_48M   clock_init(CLK_DIV1, HCLK_DIV4_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV4)

#define     CCLK_96M_HCLK_96M_PCLK_48M_MSPI_48M     clock_init(CLK_DIV2, HCLK_DIV2_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV4)
#define     CCLK_96M_HCLK_96M_PCLK_24M_MSPI_48M     clock_init(CLK_DIV2, HCLK_DIV4_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV2)

#define     CCLK_64M_HCLK_64M_PCLK_16M_MSPI_48M     clock_init(CLK_DIV3, HCLK_DIV4_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV2)

#define     CCLK_48M_HCLK_48M_PCLK_12M_MSPI_48M     clock_init(CLK_DIV4, HCLK_DIV4_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV1)
#define     CCLK_48M_HCLK_48M_PCLK_24M_MSPI_48M     clock_init(CLK_DIV4, HCLK_DIV2_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV2)

#define     CCLK_32M_HCLK_32M_PCLK_16M_MSPI_48M     clock_init(CLK_DIV6, HCLK_DIV2_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV2)

#define     CCLK_24M_HCLK_24M_PCLK_6M_MSPI_48M      clock_init(CLK_DIV8, HCLK_DIV4_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV1)

#define     CCLK_16M_HCLK_16M_PCLK_16M_MSPI_48M     clock_init(CLK_DIV12, HCLK_DIV1_TO_PCLK, PLL_DIV4_TO_MSPI_CLK, CLK_DIV2)


/**********************************************************************************************************************
 *                                         global data type                                                           *
 *********************************************************************************************************************/

/**
 *  @brief  Define sys_clk struct.
 */
typedef struct {
    unsigned short pll_clk;     /**< pll clk */
    unsigned char cclk_hclk;    /**< cpu clk */
    unsigned char pclk;         /**< pclk */
    unsigned char mspi_clk;     /**< mspi_clk */
    unsigned char dsp_clk;      /**< dsp clk */
    unsigned char n22_clk;      /**< n22 clk */
    unsigned char wt_clk;       /**< wt clk */
}sys_clk_t;

/**
 *  @brief  Define sys_clk_config_t struct.
 */
typedef struct {
    unsigned char cclk_hclk_cfg;    /**< cpu clk cfg */
    unsigned char pclk_cfg;         /**< pclk cfg */
    unsigned char mspi_clk_cfg;     /**< mspi_clk cfg */
    unsigned char dsp_clk_cfg;      /**< dsp clk cfg */
    unsigned char n22_clk_cfg;      /**< n22 clk cfg */
    unsigned char wt_clk_cfg;       /**< wt clk cfg */
}sys_clk_config_t;

extern sys_clk_config_t sys_clk_config;

/**
 * @brief system clock type
 * |                                     |                                    |               |
 * | :-----------------------------------| :--------------------------------- | :------------ |
 * |             <4:0>                   |                 <7:5>              |    <15:8>     |
 * |analog_106<4:0> bbpll_240M_div_ratio |analog_104<7:5> bbpll_240M_vco_itrim|      clk      |
 * A0 chip, you can't power down when switching PLL clock, it's not very safe, so PLL_CLK_240M and PLL_CLK_288M are not open to public first.
 */
typedef enum{
    PLL_CLK_192M    = (0x10  | (7 << 5) | (192 << 8)),
    //PLL_CLK_240M  = (0x12  | (5 << 5) | (240 << 8)),
    //PLL_CLK_288M  = (0x14  | (1 << 5) | (288 << 8)),
}sys_bbpll_clk_e;

/**
 * @brief audio clock configuration
 * | ------- | ------- | ------- | ------- | ------- | ------- | ------- |
 * | <53:48> | <47:40> | <39:32> | <31:24> | <23:16> |  <15:8> |  <7:0>  |
 * |   divn  |   int   |   frac  |   frac  |   frac  |   fcal  |   fcal  |
 * | -----------------audio pll default freq: 36.864MHz----------------- |
 * |   0x0c  |   0x25  |   0xbc  |   0x74  |   0x03  |   0x4d  |   0x02  |
 */
typedef enum{
    AUDIO_PLL_CLK_33P8688M  = 0x0d25c9c3024b02,
    AUDIO_PLL_CLK_36P864M   = 0x0c25bc74034d02,
    AUDIO_PLL_CLK_147P456M  = 0x0431a59b001203,
    AUDIO_PLL_CLK_169P344M  = 0x032a105801a502,
}sys_audio_pll_clk_e;

/**
 * @brief system clock type.
 */
typedef enum{
    RC_24M           = 0x00,
    XTAL_48M         = 0x10,
    BASEBAND_PLL     = 0x20,
    // AUDIO_PLL         = 0x30,
}sys_clock_src_e;

/**
 * @brief 32K clock type.
 */
typedef enum{
    CLK_32K_RC   = 0,
    CLK_32K_XTAL = 1,
}clk_32k_type_e;

/**
 * @brief clock division type. clock division to cclk_hclk. clock division to clk_mspi. clock division to clk_lspi. pclk division to clk_wt. clock division to DSP. XTAL48M division to clk_n22.
 */
typedef enum{
    CLK_DIV1 = 1,   //Unavailable when PLL splits to clk_mspi.
    CLK_DIV2,       //Unavailable when PLL splits to clk_mspi.
    CLK_DIV3,
    CLK_DIV4,
    CLK_DIV5,
    CLK_DIV6,
    CLK_DIV7,
    CLK_DIV8,
    CLK_DIV9,
    CLK_DIV10,
    CLK_DIV11,
    CLK_DIV12,
    CLK_DIV13,
    CLK_DIV14,
    CLK_DIV15,
}sys_clock_div_e;

/**
 * @brief hclk div to pclk.
 */
typedef enum{
    HCLK_DIV1_TO_PCLK = 0,      //hclk:pclk = 1:1
    HCLK_DIV2_TO_PCLK = 1,      //hclk:pclk = 1:2
    HCLK_DIV4_TO_PCLK = 2,      //hclk:pclk = 1:4
}sys_hclk_div_to_pclk_e;

/**
 * @brief pll_div to mspi clk.
 */
typedef enum{
    PLL_DIV3_TO_MSPI_CLK    =    3,
    PLL_DIV4_TO_MSPI_CLK    =    4,
    PLL_DIV5_TO_MSPI_CLK    =    5,
    PLL_DIV6_TO_MSPI_CLK    =    6,
    PLL_DIV7_TO_MSPI_CLK    =    7,
    PLL_DIV8_TO_MSPI_CLK    =    8,
    PLL_DIV9_TO_MSPI_CLK    =    9,
    PLL_DIV10_TO_MSPI_CLK   =    10,
    PLL_DIV11_TO_MSPI_CLK   =    11,
    PLL_DIV12_TO_MSPI_CLK   =    12,
    PLL_DIV13_TO_MSPI_CLK   =    13,
    PLL_DIV14_TO_MSPI_CLK   =    14,
    PLL_DIV15_TO_MSPI_CLK   =    15,
}sys_pll_div_to_mspi_clk_e;

/**
 *  @brief  Define rc_24M_cal enable/disable.
 */
typedef enum {
    RC_24M_CAL_DISABLE=0,
    RC_24M_CAL_ENABLE,
}rc_24M_cal_e;


/**********************************************************************************************************************
 *                                     global variable declaration                                                    *
 *********************************************************************************************************************/
extern sys_clk_t sys_clk;
extern clk_32k_type_e g_clk_32k_src;

/**********************************************************************************************************************
 *                                      global function prototype                                                     *
 *********************************************************************************************************************/

/**
 * @brief       This function is used to set cclk /hclk/pclk/mspi_clk/wt_clk (BASEBAND_PLL as clock source).
 * @param[in]   cclk_hclk_div - the cclk same as the hclk is divided from pll. cclk/hclk max is 192M
 * @param[in]   pclk_div      - the pclk is divided from cclk/hclk. pclk max is 48M.
 * @param[in]   mspi_clk_div  - the mspi_clk is divided from 192M pll.
 *                              If it is built-in flash, the maximum speed of mspi is 64M.
 *                              If it is an external flash, the maximum speed of mspi needs to be based on the board test.
 *                              Because the maximum speed is related to the wiring of the board, and is also affected by temperature and GPIO voltage,
 *                              the maximum speed needs to be tested at the highest and lowest voltage of the board,
 *                              and the high and low temperature long-term stability test speed is no problem.
 * @param[in]   wt_div        - the wt_div is divided from pclk,The range of WT_CLK is from 32k to 12.288Mhz.
 * @return      none
 * @note        Do not switch the clock during the DMA sending and receiving process because during the clock switching process, 
 *              the system clock will be suspended for a period of time which may cause data loss.
 */
_attribute_ram_code_sec_noinline_
void clock_init(sys_clock_div_e cclk_hclk_div,
        sys_hclk_div_to_pclk_e pclk_div,
        sys_pll_div_to_mspi_clk_e mspi_clk_div,
        sys_clock_div_e wt_div);

/**
 * @brief       This function serves to set 32k clock source.
 * @param[in]   src - variable of 32k type.
 * @return      none.
 */
void clock_32k_init(clk_32k_type_e src);

//The A0 version not support.
/**
 * @brief       This function serves to kick 32k xtal.
 * @param[in]   xtal_times - kick times.
 * @return      1 success, 0 error.
 */
unsigned char clock_kick_32k_xtal(unsigned char xtal_times);

/**
 * @brief       This function performs to select 24M as the system clock source.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void clock_cal_24m_rc (void);

/**
 * @brief     This function performs to select 32K as the system clock source.
 * @return    none.
 */
void clock_cal_32k_rc (void);

/**
 * @brief       This function serves to get the 32k tick.
 * @return      32k tick.
 */
_attribute_ram_code_sec_noinline_  unsigned int clock_get_32k_tick (void);

/**
 * @brief       This function serves to set the 32k tick.
 * @param[in]   tick - the value of to be set to 32k.
 * @return      none.
 * @note        This function can only called when use 24M rc as clock source.
 */
_attribute_ram_code_sec_noinline_ void clock_set_32k_tick(unsigned int tick);

/**
 * @brief       This function use to configuer the d25fclk_hclk_pclk_wtclk.
 * @param[in]   src - the d25fclk_hclk source
 * @param[in]   cclk_hclk_div - the cclk_hclk divider
 * @param[in]   pclk_div - the pclk divider
 * @param[in]   wt_div - the wt divider
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void clock_d25fclk_hclk_pclk_wtclk_config(sys_clock_src_e src, sys_clock_div_e cclk_hclk_div,
                                        sys_hclk_div_to_pclk_e pclk_div, sys_clock_div_e wt_div);

/**
 * @brief       This function use to configuer audio pll clk. 
 * @param[in]   clk - the audio pll clk
 * @return      none.
 * @note        The correct order to switch audio pll clock is 
 *              pm_audio_pll_power_down() -> clock_audio_pll_config() -> pm_audio_pll_power_on -> pm_wait_audio_pll_done().
 */
void clock_audio_pll_config(sys_audio_pll_clk_e clk);

/**
 * @brief       This function use to configure the mspi clock source. 
 * @param[in]   src - the mspi clk source
 * @param[in]   div - mspi_clk can be divided from pll, rc and xtal. 
 *                    When selecting pll as the clock source, in order to not exceed the maximum frequency, the division coefficient can not less than 3.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void clock_mspi_clk_config(sys_clock_src_e src, sys_clock_div_e div);

/**
 * @brief       This function use to configure the dsp clock source. 
 * @param[in]   src - the dsp clk source
 * @param[in]   div - the dsp clk source divider. The maximum clock value of DSP is 96M Hz.
 * @return      none.   
 */
_attribute_ram_code_sec_noinline_ void clock_dsp_clk_config(sys_clock_src_e src, sys_clock_div_e div);

/**
 * @brief       This function use to configure the n22 clock source.  
 * @param[in]   div - the n22 clk source divider. The maximum clock value of N22 is 48M Hz.
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void clock_n22_clk_config(sys_clock_div_e div);

/**
 * @brief       This function use to set all clock to default. 
 * @return      none.
 * @note        After call this, the following clock will set to default source and value:
 *              -----------------------------------------------------------------------
 *              clock source |          clock
 *              -----------------------------------------------------------------------
 *              RC 24M       | HCLK 24M, PCLK 24M, WTCLK 4M, DSP CLK 24M, MSPI CLK 24M.
 *              XTAL 48M     | N22 CLK 48M
 *              -----------------------------------------------------------------------
 */
_attribute_ram_code_sec_noinline_ void clock_set_all_clock_to_default(void);

/**
 * @brief       This function use to save all clock configuration for the follow-up restore. 
 * @return      none.
 * @note        This function needs to be used in conjunction with clock_restore_clock_config().
 */
_attribute_ram_code_sec_noinline_ void clock_save_clock_config(void);

/**
 * @brief       This function use to restore all previously saved clock configurations.
 * @return      none.
 * @note        This function needs to be used in conjunction with clock_save_clock_config().
 */
_attribute_ram_code_sec_noinline_ void clock_restore_clock_config(void);

#endif

