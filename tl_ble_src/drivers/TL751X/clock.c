/********************************************************************************************************
 * @file    clock.c
 *
 * @brief   This is the source file for TL751X
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
#include "lib/include/sys.h"
#include "clock.h"
#include "mspi.h"
#include "stimer.h"



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
sys_clk_t sys_clk = {
    .pll_clk = 192,
    .cclk_hclk = 24,//default 24M RC
    .pclk = 24,     //default 24M RC
    .mspi_clk = 24, //default 24M RC
    .dsp_clk = 24,  //default 24M RC
    .n22_clk = 48,  //default 48M xtal
    .wt_clk = 4,    //default 6 divisions from pclk(24M RC),i.e. 4M RC
};

sys_clk_config_t sys_clk_config = {
    .cclk_hclk_cfg = 0x01,
    .pclk_cfg = 0x00,
    .mspi_clk_cfg = 0x01,
    .dsp_clk_cfg = 0x01,
    .n22_clk_cfg = 0x01,
    .wt_clk_cfg = 0x06,
};

_attribute_data_retention_sec_ unsigned char tl_24mrc_cal;
clk_32k_type_e g_clk_32k_src;
unsigned char pll_vco_itrim = 0;
/**********************************************************************************************************************
 *                                              local variable                                                     *
 *********************************************************************************************************************/

/**********************************************************************************************************************
 *                                          local function prototype                                               *
 *********************************************************************************************************************/
static unsigned char clock_calculate_div_clk(sys_clock_src_e src, sys_clock_div_e div);

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/


/**
 * @brief       This function serves to set 32k clock source.
 * @param[in]   src - variable of 32k type.
 * @return      none.
 */
void clock_32k_init(clk_32k_type_e src)
{
    unsigned char sel_32k   = analog_read_reg8(areg_aon_0x4e & (~FLD_CLK32K_SEL));
    unsigned char power_32k = analog_read_reg8(areg_aon_0x05) & (~(FLD_32K_RC_PD | FLD_32K_XTAL_PD));
    analog_write_reg8(areg_aon_0x4e, sel_32k | (src << 7));
    if(src)
    {
        analog_write_reg8(areg_aon_0x05, power_32k | FLD_32K_RC_PD);//rc pwdn, sel 32k xtal
    }
    else
    {
        analog_write_reg8(areg_aon_0x05, power_32k | FLD_32K_XTAL_PD);//xtal pwdn, sel 32k rc
    }
    g_clk_32k_src = src;
}

#if 0 //The A0 version not support.
/**
 * @brief       This function serves to kick 32k xtal.
 * @param[in]   xtal_times - kick times.
 * @return      1 success, 0 error.
 */
unsigned char clock_kick_32k_xtal(unsigned char xtal_times)
{
    int last_32k_tick;
    int curr_32k_tick;
    for(unsigned char i = 0; i< xtal_times; i++)
    {
        if(0xff == g_chip_version)
        {
            delay_ms(1000);
        }
        else        //**Note that the clock is 24M crystal oscillator. PCLK is 24MHZ
        {
            //2.set PD0 as pwm output
            unsigned char pwm_clk = read_reg8(0x1401d8);//condition: PCLK is 24MHZ,PCLK = HCLK
            write_reg8(0x1401d8,((pwm_clk & 0xfc) | 0x01));//PCLK = 12M
            unsigned char reg_31e = read_reg8(0x14031e);    //PD0
            write_reg8(0x14031e,reg_31e & 0xfe);
            unsigned short reg_418 = read_reg16(0x140418);  //pwm1 cmp
            write_reg16(0x140418,0x01);
            unsigned short reg_41a = read_reg16(0x14041a);  //pwm1 max
            write_reg16(0x14041a,0x02);
            unsigned char reg_400 = read_reg8(0x140400);    //pwm en
            write_reg8(0x140400,0x02);
            write_reg8(0x140402,0xb6);                      //12M/(0xb6 + 1)/2 = 32k

            //3.wait for PWM wake up Xtal
            delay_ms(100);

            //4.Xtal 32k output
            analog_write_reg8(0x03, 0x4f); //<7:6>current select

            //5.Recover PD0 as Xtal pin
            write_reg8(0x1401d8,pwm_clk);
            write_reg8(0x14031e,reg_31e);
            write_reg16(0x140418,reg_418);
            write_reg16(0x14041a,reg_41a);
            write_reg8(0x140400,reg_400);
        }

        last_32k_tick = clock_get_32k_tick();   //clock_get_32k_tick()
        delay_us(305);      //for 32k tick accumulator, tick period: 30.5us, dly 10 ticks
        curr_32k_tick = clock_get_32k_tick();
        if(last_32k_tick != curr_32k_tick)      //clock_get_32k_tick()
        {
            return 1;       //pwm kick 32k pad success
        }
    }
    return 0;
}
#endif

/**
 * @brief     This function performs to select 24M as the system clock source.
 *            24M RC is inaccurate, and it is greatly affected by temperature, if need use it so real-time calibration is required
 *            The 24M RC needs to be calibrated before the pm_sleep_wakeup function,
 *            because this clock will be used to kick 24m xtal start after wake up,
 *            The more accurate this time, the faster the crystal will start.Calibration cycle depends on usage
 * @return    none.
 */
void clock_cal_24m_rc(void)
{
    analog_write_reg8(areg_0x148, 0x80);//wait 24m rc stable cycles

    analog_write_reg8(areg_aon_0x4f, analog_read_reg8(areg_aon_0x4f) | FLD_RC_24M_CAP_SEL);//sel cap from calibration module

    analog_write_reg8(areg_0x147, FLD_CAL_24M_RC_DISABLE);//disable 24m rc calibration
    analog_write_reg8(areg_0x147, FLD_CAL_24M_RC_ENABLE);//enable 24m rc calibration
    while((analog_read_reg8(areg_0x14f) & FLD_CAL_24M_DONE) == 0){};//wait cal_24m_done

    analog_write_reg8(areg_aon_0x52, analog_read_reg8(areg_0x14b));//write cal_rc_24m_cap to rc_24m_cap

    analog_write_reg8(areg_aon_0x4f, analog_read_reg8(areg_aon_0x4f) & (~FLD_RC_24M_CAP_SEL));//sel cap from pm_top

    analog_write_reg8(areg_0x147, FLD_CAL_24M_RC_DISABLE);//disable 24m rc calibration
    tl_24mrc_cal = analog_read_reg8(areg_aon_0x52);
}

/**
 * @brief     This function performs to select 32K as the system clock source.
 * @return    none.
 */
void clock_cal_32k_rc(void)
{
    analog_write_reg8(areg_aon_0x4f, analog_read_reg8(areg_aon_0x4f) | FLD_RC_32K_CAP_SEL);//sel cap from calibration module

    analog_write_reg8(areg_0x146, FLD_CAL_32K_RC_DISABLE);//disable 32k rc calibration
    analog_write_reg8(areg_0x146, FLD_CAL_32K_RC_ENABLE);//disable 32k rc calibration
    while((analog_read_reg8(areg_0x14f) & FLD_CAL_32K_DONE) == 0){};//wait cal_32k_done

    analog_write_reg8(areg_aon_0x51, analog_read_reg8(areg_0x149));//write cal_rc_32k_cap to rc_32k_cap[13:6]
    analog_write_reg8(areg_aon_0x4f, (analog_read_reg8(areg_aon_0x4f) & 0xc0) | analog_read_reg8(areg_0x14a));///write cal_rc_32k_res to rc_32k_cap[5:0]
    analog_write_reg8(areg_0x146, FLD_CAL_32K_RC_DISABLE);//disable 32k rc calibration
    analog_write_reg8(areg_aon_0x4f, analog_read_reg8(areg_aon_0x4f) & (~FLD_RC_32K_CAP_SEL));//sel cap from pm_top
}

/**
 * @brief       This function serves to set the 32k tick.
 * @param[in]   tick - the value of to be set to 32k.
 * @return      none.
 * @note        This function can only called when use 24M rc as clock source.
 */
_attribute_ram_code_sec_noinline_ void clock_set_32k_tick(unsigned int tick)
{
    reg_system_ctrl |= FLD_SYSTEM_32K_WR_EN;//r_32k_wr = 1;
    while(reg_system_st & FLD_SYSTEM_RD_BUSY);
    reg_system_timer_set_32k = tick;

    reg_system_st = FLD_SYSTEM_CMD_SYNC;//cmd_sync = 1,trig write
    /**
     * This delay time is about 1.38us under the calibrated 24M RC clock.
     * The minimum waiting time here is 3*pclk cycles+3*24M xtal cycles, a total of 0.25us,
     * wait 0.25us before you can use wr_busy signal for judgment, jianzhi suggested that this time to 1us is enough.
     * add by bingyu.li, confirmed by jianzhi.chen 20231115
     */
    core_cclk_delay_tick(sys_clk.cclk_hclk);//1us

    while(reg_system_st & FLD_SYSTEM_CMD_SYNC);//wait wr_busy = 0

}

/**
 * @brief       This function serves to get the 32k tick.
 * @return      32k tick.
 */
#if 0
/*
 * modify by yi.bao,confirmed by guangjun at 20210105
 * Use digital register way to get 32k tick may read error tick,cause the wakeup time is
 * incorrect with the setting time,the sleep time will very little or very big,will not wakeup on time.
 */
_attribute_ram_code_sec_noinline_ unsigned int clock_get_32k_tick(void)
{
    unsigned int timer_32k_tick;
    reg_system_st = FLD_SYSTEM_CLR_RD_DONE;//clr rd_done
    while((reg_system_st & FLD_SYSTEM_CLR_RD_DONE) != 0);//wait rd_done = 0;
    reg_system_ctrl &= ~FLD_SYSTEM_32K_WR_EN;   //1:32k write mode; 0:32k read mode
    while((reg_system_st & FLD_SYSTEM_CLR_RD_DONE) == 0);//wait rd_done = 1;
    timer_32k_tick = reg_system_timer_read_32k;
    reg_system_ctrl |= FLD_SYSTEM_32K_WR_EN;    //1:32k write mode; 0:32k read mode
    return timer_32k_tick;
}
#else
_attribute_ram_code_sec_noinline_ unsigned int clock_get_32k_tick(void)
{
    unsigned int t0 = 0;
    unsigned int t1 = 0;

    //In the system timer auto mode, when writing a tick value to the system tick, if the writing operation overlaps
    //with the 32k rising edge, the writing operation will be unsuccessful. When reading the 32k tick value,
    //first wait for the rising edge to pass to avoid overlap with the subsequent write tick value operation.
    //modify by weihua.zhang, confirmed by jianzhi at 20210126
    t0 = analog_read_reg32(0x60);
    while(1)
    {
        t1 = analog_read_reg32(0x60);
        if((t1-t0) == 1)
        {
            return t1;
        }
        else if(t1-t0)
        {
            t0 = t1;
        }
    }
}
#endif

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
        sys_clock_div_e wt_div)
{
    clock_mspi_clk_config(BASEBAND_PLL, (sys_clock_div_e)mspi_clk_div);
    clock_d25fclk_hclk_pclk_wtclk_config(BASEBAND_PLL, cclk_hclk_div, pclk_div, wt_div);
}

/**
 * @brief       This function use to configuer audio pll clk. 
 * @param[in]   clk - the audio pll clk
 * @return      none.
 * @note        The correct order to switch audio pll clock is 
 *              pm_audio_pll_power_down() -> clock_audio_pll_config() -> pm_audio_pll_power_on -> pm_wait_audio_pll_done().
 */
void clock_audio_pll_config(sys_audio_pll_clk_e clk)
{
    analog_write_reg8(areg_0x10d, analog_read_reg8(areg_0x10d) | 0x92);//calibrate xtal to 48MHz, different crystal oscillators may have inconsistent calibration values

    analog_write_reg8(areg_0x158, (analog_read_reg8(areg_0x158) & 0x80) | (clk & 0xff000000000000) >> 48);//post-divider, bit[6:0] 001100 divider by 24

    analog_write_reg8(areg_0x15d, (analog_read_reg8(areg_0x15d) & 0x00) | (clk & 0xff0000000000) >> 40);//int, bit[7:0]

    analog_write_reg8(areg_0x15a, (analog_read_reg8(areg_0x15a) & 0x00) | (clk & 0xff00000000) >> 32);//frac, bit[7:0] dsm_frac_overwrite_value<7:0>
    analog_write_reg8(areg_0x15b, (analog_read_reg8(areg_0x15b) & 0x00) | (clk & 0xff000000) >> 24);//frac, bit[7:0] dsm_frac_overwrite_value<15:8>
    analog_write_reg8(areg_0x15c, (analog_read_reg8(areg_0x15c) & 0xfc) | (clk & 0xff0000) >> 16);//frac, bit[1:0] dsm_frac_overwrite_value<17:16>

    analog_write_reg8(areg_0x162, (analog_read_reg8(areg_0x162) & 0x00) | (clk & 0xff00) >> 8);//fcal_target<7:0>
    analog_write_reg8(areg_0x163, (analog_read_reg8(areg_0x163) & 0xe0) | (clk & 0x0f));//fcal_target<12:8>
}

/**
 * @brief       This function use to configuer the d25fclk_hclk_pclk_wtclk.  
 * @param[in]   src - the d25fclk_hclk source
 * @param[in]   cclk_hclk_div - the cclk_hclk divider
 * @param[in]   pclk_div - the pclk divider
 * @param[in]   wt_div - the wt divider
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void clock_d25fclk_hclk_pclk_wtclk_config(sys_clock_src_e src, sys_clock_div_e cclk_hclk_div,
                                        sys_hclk_div_to_pclk_e pclk_div, sys_clock_div_e wt_div)
{
    //first set wt_divider to max value,and set cclk/hclk switch to 24rc to avoid the risk of hclk/pclk/wtclk exceeding its maximum configurable frequency for a short period of time
    //when switching different clock frequencies using this interface.
    write_reg8(0x140805, (read_reg8(0x140805) & 0xf0) | CLK_DIV15);//wtclk bit[3:0]
    write_reg8(0x140828, (read_reg8(0x140828) & 0xc0) | RC_24M | CLK_DIV1);//cclk/hclk to 24M rc clock

    write_reg8(0x140818, (read_reg8(0x140818) & 0xfc) | pclk_div);//pclk bit[1:0]
    write_reg8(0x140805, (read_reg8(0x140805) & 0xf0) | wt_div);//wtclk bit[3:0]
    write_reg8(0x140828, (read_reg8(0x140828) & 0xc0) | src | cclk_hclk_div);//d25fclk_hclk src:bit[5:4] div:bit[3:0]

    sys_clk.cclk_hclk = clock_calculate_div_clk(src, cclk_hclk_div);
    sys_clk.pclk = sys_clk.cclk_hclk / (1 << pclk_div);
    sys_clk.wt_clk = sys_clk.pclk / wt_div;
}

#if 0
/**
 * @brief       This function use to configure the mspi clock source. 
 * 
 * @param[in]   src - the mspi clk source
 * @param[in]   div - the mspi clk source divider
 * @return      none.
 */
_attribute_ram_code_sec_noinline_ void clock_mspi_clk_config_ram(sys_clock_src_e src, sys_clock_div_e div)
{
    //ensure mspi is not in busy status before change mspi clock
    mspi_stop_xip();

    //change mspi clock should be ram code.
    write_reg8(0x140800, (read_reg8(0x140800) & 0xc0) | src | div);//src:bit[5:4], div:bit[3:0]
    sys_clk.mspi_clk = clock_calculate_div_clk(src,div);
    CLOCK_DLY_5_CYC;
    mspi_set_xip_en();
}

/**
 * @brief       This function use to configure the mspi clock source.
 *
 * @param[in]   src - the mspi clk source
 * @param[in]   div - the mspi clk source divider
 * @return      none.
 */
_attribute_text_sec_ void clock_mspi_clk_config(sys_clock_src_e src, sys_clock_div_e div)
{

    DISABLE_BTB;
    clock_mspi_clk_config_ram(src,div);
    ENABLE_BTB;
}
#else
/**
 * @brief       This function use to configure the mspi clock source. 
 * @param[in]   src - the mspi clk source
 * @param[in]   div - mspi_clk can be divided from pll, rc and xtal. 
 *                    When selecting pll as the clock source, in order to not exceed the maximum frequency, the division coefficient can not less than 3.
 * @return      none.
 */
/*
    At present, Onca's design supports MSPI to dynamically switch its clock during runtime (fetching or interface reading data),
    so it does not need to be placed in SRAM for processing.
    This modification requires further pressure testing, modify by jilong.liu, confirmed by jianzhi at 20231221
*/
_attribute_ram_code_sec_noinline_ void clock_mspi_clk_config(sys_clock_src_e src, sys_clock_div_e div)
{
    write_reg8(0x140800, (read_reg8(0x140800) & 0xc0) | src | div);//src:bit[5:4], div:bit[3:0]
    sys_clk.mspi_clk = clock_calculate_div_clk(src, div);
}
#endif

/**
 * @brief       This function use to configure the dsp clock source. 
 * @param[in]   src - the dsp clk source
 * @param[in]   div - the dsp clk source divider. The maximum clock value of DSP is 96M Hz.
 * @return      none.   
 */
 _attribute_ram_code_sec_noinline_ void clock_dsp_clk_config(sys_clock_src_e src, sys_clock_div_e div)
{
    write_reg8(0x140855, (read_reg8(0x140855) & 0xc0) | src | div);//src:bit[5:4], div:bit[3:0]
    sys_clk.dsp_clk = clock_calculate_div_clk(src, div);
}

/**
 * @brief       This function use to configure the n22 clock source.  
 * @param[in]   div - the n22 clk source divider. The maximum clock value of N22 is 48M Hz.
 * @return      none.
 */
 _attribute_ram_code_sec_noinline_ void clock_n22_clk_config(sys_clock_div_e div)
{
    write_reg8(0x140856, (read_reg8(0x140856) & 0xf0) | div);//div:bit[3:0]
    sys_clk.n22_clk = 48 / div;
}

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
_attribute_ram_code_sec_noinline_ void clock_set_all_clock_to_default(void)
{
    clock_d25fclk_hclk_pclk_wtclk_config(RC_24M, CLK_DIV1, HCLK_DIV1_TO_PCLK, CLK_DIV6);

    clock_dsp_clk_config(RC_24M, CLK_DIV1);

    clock_mspi_clk_config(RC_24M, CLK_DIV1);

    clock_n22_clk_config(CLK_DIV1);
}

/**
 * @brief       This function use to save all clock configuration for the follow-up restore. 
 * @return      none.
 * @note        This function needs to be used in conjunction with clock_restore_clock_config().
 */
_attribute_ram_code_sec_noinline_ void clock_save_clock_config(void)
{
    sys_clk_config.cclk_hclk_cfg = read_reg8(0x140828);
    sys_clk_config.pclk_cfg = read_reg8(0x140818);
    sys_clk_config.wt_clk_cfg = read_reg8(0x140805);

    sys_clk_config.mspi_clk_cfg = read_reg8(0x140800);

    sys_clk_config.dsp_clk_cfg = read_reg8(0x140855);

    sys_clk_config.n22_clk_cfg = read_reg8(0x140856);
}

/**
 * @brief       This function use to restore all previously saved clock configurations.
 * @return      none.
 * @note        This function needs to be used in conjunction with clock_save_clock_config().
 */
_attribute_ram_code_sec_noinline_ void clock_restore_clock_config(void)
{
    clock_d25fclk_hclk_pclk_wtclk_config(sys_clk_config.cclk_hclk_cfg & BIT_RNG(4, 5),      //src
                                            sys_clk_config.cclk_hclk_cfg & BIT_RNG(0, 3),       //cclk_hclk_div
                                            sys_clk_config.pclk_cfg & BIT_RNG(0, 1),    //pclk_div 
                                            sys_clk_config.wt_clk_cfg & BIT_RNG(0, 3));     //wtclk_div

    clock_dsp_clk_config(sys_clk_config.dsp_clk_cfg & BIT_RNG(4, 5),        //src 
                            sys_clk_config.dsp_clk_cfg & BIT_RNG(0, 3));        //dspclk_div

    clock_mspi_clk_config(sys_clk_config.mspi_clk_cfg & BIT_RNG(4, 5),      //src
                            sys_clk_config.mspi_clk_cfg & BIT_RNG(0, 3));       //mspiclk_div

    clock_n22_clk_config(sys_clk_config.n22_clk_cfg & BIT_RNG(0, 3));       //n22 clk
}

/**********************************************************************************************************************
 *                                          local function implementation                                             *
 *********************************************************************************************************************/
/**
 * @brief       This function use to configuer baseband pll clk. 
 * @param[in]   clk - the baseband pll clk
 * @return      none.
 * @note        Due to the impact of switching PLL on many clocks, it can only set during initialization and does not allow to switch afterwards.
 *              The correct order to switch pll clock is power down pll -> clock_baseband_pll_config() -> power on pll -> pm_wait_bbpll_done().
 */
void clock_baseband_pll_config(sys_bbpll_clk_e clk)
{
    //power up pll, A0 version is not supported, A1 version will take effect. 
    analog_write_reg8(0x1e, analog_read_reg8(0x1e) & 0x7f);

    //pll clk
    analog_write_reg8(areg_0x105, (analog_read_reg8(areg_0x105) & 0x9f) | 0x20);//bpll_240M_refclk_sel
    analog_write_reg8(areg_0x106, (analog_read_reg8(areg_0x106) & 0xe0) | (clk & 0x1f));//bbpll_240M_div_ratio
    analog_write_reg8(areg_0x104, (analog_read_reg8(areg_0x104) & 0x1f) | (clk & 0xe0));//vco_itrim<2:0>
    sys_clk.pll_clk = (clk >> 8);
    pll_vco_itrim = (clk & 0xe0) >> 5;
}

/**
 * @brief       This function is used to calculate the clock after different clock sources, the unit is MHZ.
 * @param[in]   src - the clock source
 * @param[in]   div - the clock source divider
 * @return      clk.
 */
static inline unsigned char clock_calculate_div_clk(sys_clock_src_e src, sys_clock_div_e div)
{
    unsigned char clk = 0 ;
    if(RC_24M == src ){
        clk = 24 / div;
    }else if(XTAL_48M == src ){
        clk = 48 / div;
    }else{
        clk = sys_clk.pll_clk / div;
    }
    return clk;
}
