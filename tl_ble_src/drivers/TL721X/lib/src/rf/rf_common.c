/********************************************************************************************************
 * @file    rf_common.c
 *
 * @brief   This is the source file for TL721X
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
#include "lib/include/rf/rf_common.h"
#include "lib/include/pm/pm.h"
#include "compiler.h"

#include "ext_driver/driver_internal/ext_rf.h" //add by BLE

/*********************************************************************************************************************
 *                                         RF global constants                                                       *
 *********************************************************************************************************************/
/**
 *  @brief   Define power list of RF.
 *  @note    (1)The energy meter is averaged over 5 chips at room temperature and 3.3V supply voltage.
 *           (2)Transmit energy in VBAT mode decreases as the supply voltage drops.
 *           (3)There will be some differences in the energy values tested between different chips.
 *           (4)At present, power levels above 9dbm cannot be used.There is spectrum leakage when TX sends energy to levels above 9dbm.(A2 has fixed this issue)
 *           (5)TX Power levels above 10dbm are internal test versions and are not open to users.
 */
const rf_power_level_e rf_power_Level_list[70] =
    {
#if RF_TX_POWER_A2
/*VBAT*/
    #ifdef GREATER_TX_POWER_EN
        RF_POWER_P12p11dBm, /**<  12.1 dbm */
        RF_POWER_P11p86dBm, /**<  11.8 dbm */
        RF_POWER_P11p49dBm, /**<  11.5 dbm */
        RF_POWER_P11p01dBm, /**<  11.0 dbm */
        RF_POWER_P10p50dBm, /**<  10.5 dbm */
    #endif
        RF_POWER_P10p00dBm, /**<  10.0 dbm */
        RF_POWER_P9p45dBm,  /**<  9.5 dbm */
        RF_POWER_P9p10dBm,  /**<  9.1 dbm */
        RF_POWER_P8p55dBm,  /**<  8.4 dbm */
        RF_POWER_P8p06dBm,  /**<  8.0 dbm */
        RF_POWER_P7p52dBm,  /**<  7.5 dbm */
        RF_POWER_P6p71dBm,  /**<  6.7 dbm */
        RF_POWER_P6p08dBm,  /**<  6.0 dbm */
        RF_POWER_P5p53dBm,  /**<  5.5 dbm */
        RF_POWER_P4p79dBm,  /**<  4.8 dbm */
        RF_POWER_P4p09dBm,  /**<  4.1 dbm */


        /*VANT*/
        RF_POWER_P3p82dBm,           /**<   3.8 dbm */
        RF_POWER_P3p48dBm,           /**<   3.5 dbm */
        RF_POWER_P3p32dBm,           /**<   3.3 dbm */
        RF_POWER_P3p04dBm,           /**<   3.0 dbm */
        RF_POWER_P2p73dBm,           /**<   2.7 dbm */
        RF_POWER_P2p43dBm,           /**<   2.4 dbm */
        RF_POWER_P2p00dBm,           /**<   2.0 dbm */
        RF_POWER_P1p65dBm,           /**<   1.6 dbm */
        RF_POWER_P0p98dBm,           /**<   1.0 dbm */
        RF_POWER_P0p66dBm,           /**<   0.7 dbm */
        RF_POWER_P0p03dBm,           /**<   0.0 dbm */
        RF_POWER_N0p66dBm,           /**<   -0.7 dbm */
        RF_POWER_N1p08dBm,           /**<   -1.0 dbm */
        RF_POWER_N1p67dBm,           /**<  -1.7 dbm */
        RF_POWER_N2p30dBm,           /**<  -2.3 dbm */
        RF_POWER_N3p08dBm,           /**<  -3.0 dbm */
        RF_POWER_N3p89dBm,           /**<  -3.9 dbm */
        RF_POWER_N4p36dBm,           /**<  -4.4 dbm */
        RF_POWER_N4p90dBm,           /**<  -5.0 dbm */
        RF_POWER_N5p38dBm,           /**<  -5.4 dbm */
        RF_POWER_N6p03dBm,           /**<  -6.0 dbm */
        RF_POWER_N6p61dBm,           /**<  -6.6 dbm */
        RF_POWER_N7p39dBm,           /**<  -7.4 dbm */
        RF_POWER_N8p14dBm,           /**<  -8.1 dbm */
        RF_POWER_N9p14dBm,           /**<  -9.1 dbm */
        RF_POWER_N10p09dBm,          /**<  -10.1 dbm */
        RF_POWER_N11p42dBm,          /**<  -11.4 dbm */
        RF_POWER_N12p72dBm,          /**<  -12.7 dbm */
        RF_POWER_N14p64dBm,          /**<  -14.6 dbm */
        RF_POWER_N16p82dBm,          /**<  -16.8 dbm */
        RF_POWER_N20p22dBm,          /**<  -20.2 dbm */
        RF_POWER_N25p19dBm,          /**<  -25.2 dbm */
        RF_POWER_N41p80dBm,          /**<  -41.8 dbm */
                                     //  (1)Normal energy levels(described above) can be used for typical applications.The following energy levels reduce
                                     //     TX power consumption but require LDO and DCDC voltage to be raised from 0.94V to 1.05V before TX.To offset
                                     //     increased base power, the voltage should be restored to 0.94V after RX.You can refer to app_ble_mode.c in RF_Demo.
                                     //    (2)The default power-up configuration is 1.05V gears There is no need to switch back and forth, but note that the
                                     //       overall tx power and power consumption will increase.
        RF_1P05_VANT_POWER_P5p11dBm, /**<   5.1 dbm */
        RF_1P05_VANT_POWER_P5p00dBm, /**<   5.0 dbm */
        RF_1P05_VANT_POWER_P4p60dBm, /**<   4.6 dbm */
        RF_1P05_VANT_POWER_P4p00dBm, /**<   4.0 dbm */
        RF_1P05_VANT_POWER_P3p52dBm, /**<   3.5 dbm */
#else
/*VBAT*/
    #ifdef GREATER_TX_POWER_EN
        /**TODO:The A1 version of the chip cannot be used with power levels above 9dbm.
     * Spectrum leakage occurs when the transmitter transmits more than 9dbm.
     * This issue will be fixed in the A2 version of the chip*/
        RF_POWER_P10p97dBm, /**<  11.0 dbm */
        RF_POWER_P10p47dBm, /**<  10.5 dbm */
        RF_POWER_P10p00dBm, /**<  10.0 dbm */
        RF_POWER_P9p48dBm,  /**<  9.5 dbm */
    #endif
        RF_POWER_P9p06dBm,  /**<  9.0 dbm */
        RF_POWER_P8p49dBm,  /**<  8.5 dbm */
        RF_POWER_P7p96dBm,  /**<  8.0 dbm */
        RF_POWER_P7p51dBm,  /**<  7.5 dbm */
        RF_POWER_P7p25dBm,  /**<  7.3 dbm */
        RF_POWER_P7p00dBm,  /**<  7.0 dbm */
        RF_POWER_P6p74dBm,  /**<  6.7 dbm */
        RF_POWER_P6p46dBm,  /**<  6.5 dbm */
        RF_POWER_P5p90dBm,  /**<  5.9 dbm */
        RF_POWER_P5p27dBm,  /**<  5.3 dbm */
        RF_POWER_P4p96dBm,  /**<  5.0 dbm */
        RF_POWER_P4p60dBm,  /**<  4.6 dbm */
        RF_POWER_P4p29dBm,  /**<  4.3 dbm */
        RF_POWER_P3p93dBm,  /**<  3.9 dbm */
        RF_POWER_P3p57dBm,  /**<  3.5 dbm */
        RF_POWER_P2p98dBm,  /**<  3.1 dbm */
        RF_POWER_P2p75dBm,  /**<  2.7 dbm */
        RF_POWER_P2p42dBm,  /**<  2.4 dbm */
        RF_POWER_P1p91dBm,  /**<  1.9 dbm */

        /*VANT*/
        RF_POWER_P1p46dBm,  /**<   1.5 dbm */
        RF_POWER_P1p30dBm,  /**<   1.3 dbm */
        RF_POWER_P1p13dBm,  /**<   1.1 dbm */
        RF_POWER_P0p84dBm,  /**<   0.8 dbm */
        RF_POWER_P0p64dBm,  /**<   0.6 dbm */
        RF_POWER_P0p36dBm,  /**<   0.4 dbm */
        RF_POWER_P0p14dBm,  /**<   0.1 dbm */
        RF_POWER_P0p05dBm,  /**<   0.0 dbm */
        RF_POWER_N0p10dBm,  /**<  -0.1 dbm */
        RF_POWER_N0p35dBm,  /**<  -0.3 dbm */
        RF_POWER_N0p73dBm,  /**<  -0.7 dbm */
        RF_POWER_N1p09dBm,  /**<  -1.0 dbm */
        RF_POWER_N1p55dBm,  /**<  -1.5 dbm */
        RF_POWER_N2p11dBm,  /**<  -2.1 dbm */
        RF_POWER_N2p62dBm,  /**<  -2.6 dbm */
        RF_POWER_N3p19dBm,  /**<  -3.2 dbm */
        RF_POWER_N3p56dBm,  /**<  -3.6 dbm */
        RF_POWER_N4p10dBm,  /**<  -4.1 dbm */
        RF_POWER_N4p67dBm,  /**<  -4.7 dbm */
        RF_POWER_N5p71dBm,  /**<  -5.7 dbm */
        RF_POWER_N6p60dBm,  /**<  -6.6 dbm */
        RF_POWER_N7p53dBm,  /**<  -7.5 dbm */
        RF_POWER_N8p94dBm,  /**<  -8.9 dbm */
        RF_POWER_N10p42dBm, /**<  -10.4 dbm */
        RF_POWER_N12p08dBm, /**<  -12.0 dbm */
        RF_POWER_N16p44dBm, /**<  -16.5 dbm */
        RF_POWER_N18p52dBm, /**<  -18.5 dbm */
        RF_POWER_N21p35dBm, /**<  -20.6 dbm */
        RF_POWER_N24p04dBm, /**<  -24.0 dbm */
        RF_POWER_N28p86dBm, /**<  -28.9 dbm */
        RF_POWER_N44p08dBm, /**<  -44.0 dbm */
#endif
};

/**
 * @brief Define TX energy modes RF_TX_NORMAL_POWER and RF_TX_HIGH_POWER
 * @note  Defaults to RF_TX_NORMAL_POWER for A2.
 *        RF_TX_HIGH_POWER mode can increase TX power.
 */
static rf_status_e s_rf_trxstate = RF_MODE_TX;
rf_mode_e          g_rfmode;
rf_crc_config_t    rf_crc_config[3] =
    {
        {0x555555,   0x0000065b, 0, 1, 0, 3}, //ble
        {0xffffffff, 0x00001021, 0, 1, 0, 2}, //private
        {0x00000000, 0x00001021, 0, 1, 1, 2}, //zigbee,hybee
};

_attribute_data_retention_sec_ rf_fast_settle_t               *g_fast_settle_cal_val_ptr;
_attribute_data_retention_sec_ static unsigned char            g_rf_tx_fast_settle_chn_cal_flag = 0;
_attribute_data_retention_sec_ static rf_tx_fast_settle_time_e g_rf_tx_fast_settle_time         = TX_FAST_SETTLE_NONE;
_attribute_data_retention_sec_ static rf_rx_fast_settle_time_e g_rf_rx_fast_settle_time         = RX_FAST_SETTLE_NONE;

/*
 *This macro definition is used to revert to the use of a hardware DCOC in the event of a problem with the debug software DCOC.
 *Note:Depending on the given scheme of the design, the software DCOC must be turned on, and this macro definition is only used
 *for internal debugging.Set this macro to 0 when the hardware DCOC needs to be restored(Modified by zhiwei,confirmed by kaixin,20250108)
 */
_attribute_data_retention_sec_ unsigned char        s_dcoc_software_cal_en = 1; //BLE not use static
_attribute_data_retention_sec_ unsigned short       g_rf_dcoc_iq_code      = 0;

typedef enum
{
    I_FIX_FIRST = 0,
    Q_FIX_FIRST = 1
} rf_dcoc_iq_search_e;

typedef struct
{
    short adc_i;
    short adc_q;
} rf_dcoc_adc_iq_t;

rf_dcoc_adc_iq_t rf_adc_iq;

typedef struct
{
    unsigned char i;
    unsigned char q;
    unsigned int  total;
} rf_dcoc_iq_t;

rf_dcoc_iq_t q_first;
rf_dcoc_iq_t i_first;
rf_dcoc_iq_t q_final;

/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

/**
 * @brief        This function is used to set whether or not to use the rx DCOC software calibration in rf_mode_init();
 * @param[in]    en:This value is used to set whether or not rx DCOC software calibration is performed.
 *                -#1:enable the DCOC software calibration;
 *                -#0:disable the DCOC software calibration;
 * @return        none.
 * @note        Attention:
 *                 1.Driver default enable to solve the problem of poor receiver sensitivity performance of some chips with large DC offset
 *                 2.The following conditions should be noted when using this function:
 *                   If you use the RX function, it must be enabled, otherwise it will result in a decrease in RX sensitivity.
 *                   If you only use tx and not rx, and want to save code execution time for rf_mode_init(), you can disable it
 */
void rf_set_rx_dcoc_cali_by_sw(unsigned char en)
{
    s_dcoc_software_cal_en = en;
}

/**
 * @brief        This function is used to update the rx DCOC calibration value.
 * @param[in]   calib_code - Value of iq_code after calibration.(The code is a combination value,you need to fill in the combined iq value)
 *                 <0> is used to control the switch of bypass dcoc calibration iq code, the value should be 1;
 *                 <6-1>:the value of I code, the range of value is 1~62;
 *                 <12-7>:the value of Q code, the range of value is 1~62.
 * @return         none.
 */
void rf_update_rx_dcoc_calib_code(unsigned short calib_code)
{
    g_rf_dcoc_iq_code = calib_code;
}

/**
 * @brief        This function is mainly used to set the overwrite value of iq code and bypass dcoc calibration iq code.
 * @param[in]    iq_code:Value of iq_code after calibration.(The code is a combination value,you need to fill in the combined iq value)
 *                 <0> is used to control the switch of bypass dcoc calibration iq code, the value should be 1;
 *                 <6-1>:the value of I code, the range of value is 1~62;
 *                 <12-7>:the value of Q code, the range of value is 1~62.
 * @return       none.
 * @note
 */
_attribute_ram_code_sec_ void rf_set_dcoc_iq_code(unsigned short iq_code)
{
    write_reg16(0x1706d0, iq_code); //When writing iq values, you need to wait for the iq value to stabilise before enabling it.
    write_reg8(0x1706d0, read_reg8(0x1706d0) | BIT(0));
}

/**
 * @brief        This function is mainly used to set the overwrite value of iq offset and bypass dcoc calibration iq offset.
 * @param[in]    iq_offset:Value of iq_offset after calibration.(The code is a combination value,you need to fill in the combined iq offset value)
 *                 <0> is used to control the switch of bypass dcoc calibration iq offset, the value should be 1;
 *                 <7-1>:the value of I offset, the range of value is -64~63;
 *                 <14-8>:the value of Q offset, the range of value is -64~63.
 * @return       none.
 */
_attribute_ram_code_sec_ void rf_set_dcoc_iq_offset(signed short iq_offset)
{
    write_reg16(0x1706ce, iq_offset); //When writing iq offset values, you need to wait for the iq offset value to stabilise before enabling it.
    write_reg8(0x1706ce, read_reg8(0x1706ce) | BIT(0));
}

/**
 * @brief      This function is mainly used to get the ADC IQ data for selection of optimum DCOC_IQ_CODE.
 * @param[out] val    -    Address for storing ADC IQ.
 * @return     none.
 */
_attribute_ram_code_sec_ void rf_rd_iq_val(short *val)
{
    short temp_dat_i = 0;
    short temp_dat_q = 0;
    int   temp_dat_iq;
    short lim = 1024;                   //11bits ADC IQ value.1024: the threshold for signed or unsigned

    temp_dat_iq = read_reg32(0x17066c); //ADC_I :11bits signed.ADC_Q :11bits signed.
                                        //<7-0>:ADC_RDBK_I_L
                                        //<10-8>:ADC_RDBK_I_H
                                        //<15-11>:Reserved
                                        //<23-16>:ADC_RDBK_Q_L
                                        //<26-24>:ADC_RDBK_Q_H
                                        //<31-27>:Reserved
    temp_dat_i = temp_dat_iq & 0x07ff;
    temp_dat_q = (temp_dat_iq >> 16) & 0x07ff;

    //convert 11bits signed to 32bits signed data
    if (temp_dat_i >= lim) {
        temp_dat_i = 0xf800 | temp_dat_i;
    }
    if (temp_dat_q >= lim) {
        temp_dat_q = 0xf800 | temp_dat_q;
    }
    val[0] = temp_dat_i;
    val[1] = temp_dat_q;
}

/**
 * @brief        This function is mainly used to bypass the ANA_DCOC_DAC_CODE function, and also overwrite dcoc_iq_code.
 * @param[in]    i_code    - I data that needs to be written to the ADC IQ value.Values range from 1 to 62
 * @param[in]    q_code    - Q data that needs to be written to the ADC IQ value.Values range from 1 to 62
 * @return       none.
 */
_attribute_ram_code_sec_ void rf_dcoc_iq_bypass(unsigned char i_code, unsigned char q_code)
{
    unsigned char  i_c;
    unsigned char  q_c_h, q_c_l;
    unsigned short wt_val;

    q_c_h             = q_code >> 1;
    q_c_l             = q_code & 0x1;
    i_c               = (i_code << 1) + (q_c_l << 7) + 1;
    wt_val            = (q_c_h << 8) + i_c;
    g_rf_dcoc_iq_code = wt_val;
    rf_set_dcoc_iq_code(wt_val);
}

/**
 * @brief    This function serves to get the rf adc iq value
 * @return   RF DCOC ADC IQ structure containing the ADC I and Q values.
*/
rf_dcoc_adc_iq_t rf_get_adc_iq(void)
{
    rf_dcoc_adc_iq_t rf_adc_iq_temp;
    short            iq_values[9][2];
    for (int i = 0; i < 9; i++) {
        rf_rd_iq_val(iq_values[i]);
    }
    for (int i = 0; i < 9 - 1; i++) {
        for (int j = 0; j < 9 - i - 1; j++) {
            if (iq_values[j][0] < iq_values[j + 1][0]) {
                int temp_i          = iq_values[j][0];
                iq_values[j][0]     = iq_values[j + 1][0];
                iq_values[j + 1][0] = temp_i;
            }
            if (iq_values[j][1] < iq_values[j + 1][1]) {
                int temp_q          = iq_values[j][1];
                iq_values[j][1]     = iq_values[j + 1][1];
                iq_values[j + 1][1] = temp_q;
            }
        }
    }
    rf_adc_iq_temp.adc_i = iq_values[4][0];
    rf_adc_iq_temp.adc_q = iq_values[4][1];
    return rf_adc_iq_temp;
}

/**
 * @brief      This function serves to find the optimal dcoc calibration value
 * @param[in]  mode - dcoc search method
 * @param[out] iq_value - dcoc calibration related data
 * @return     none
*/
_attribute_ram_code_sec_noinline_ void rf_dcoc_iq_search(rf_dcoc_iq_search_e mode, rf_dcoc_iq_t *iq_value)
{
    short         value[2] = {0, 0};
    unsigned int  stotal_t = 0;
    unsigned char smin_i   = 32;
    unsigned char smin_q   = 32;
    for (unsigned char i = 1; i < 63; i = i + 4) {
        if (mode == Q_FIX_FIRST) {
            rf_dcoc_iq_bypass(i, 32);
        } else {
            rf_dcoc_iq_bypass(32, i);
        }
        core_cclk_delay_tick((unsigned long long)sys_clk.cclk); //A 1us delay time is required to ensure that the ADC valve remains stable in the event of a change in DC-offset
                                                                //(Modified by zhiwei,confirmed by xuqiang&yuya at 20241210.)
        rf_adc_iq             = rf_get_adc_iq();
        unsigned int avg_i_sq = rf_adc_iq.adc_i * rf_adc_iq.adc_i;
        unsigned int avg_q_sq = rf_adc_iq.adc_q * rf_adc_iq.adc_q;
        unsigned int total    = avg_q_sq + avg_i_sq;

        if (i == 1) {
            stotal_t = total;
        }
        if (total <= stotal_t) {
            stotal_t = total;
            if (mode == Q_FIX_FIRST) {
                smin_i = i;
            } else {
                smin_q = i;
            }
        }
    }

    for (unsigned char i = 1; i < 63; i = i + 2) {
        if (mode == Q_FIX_FIRST) {
            rf_dcoc_iq_bypass(smin_i, i);
        } else {
            rf_dcoc_iq_bypass(i, smin_i);
        }

        core_cclk_delay_tick((unsigned long long)sys_clk.cclk); //A 1us delay time is required to ensure that the ADC valve remains stable in the event of a change in DC-offset
                                                                //(Modified by zhiwei,confirmed by xuqiang&yuya at 20241210.)
        rf_rd_iq_val(value);

        unsigned int avg_i_sq = value[0] * value[0];
        unsigned int avg_q_sq = value[1] * value[1];
        unsigned int total    = avg_q_sq + avg_i_sq;

        if (total <= stotal_t) {
            if (mode == Q_FIX_FIRST) {
                smin_q = i;
            } else {
                smin_i = i;
            }
            stotal_t = total;
        }
    }

    unsigned char i_st = 0;
    unsigned char i_ed = 0;
    unsigned char q_st = 0;
    unsigned char q_ed = 0;
    if (smin_i < 2) {
        i_st = 1;
        i_ed = smin_i + 2;
    } else if (smin_i > 59) {
        i_ed = 63;
        i_st = smin_i - 2;
    } else {
        i_st = smin_i - 2;
        i_ed = smin_i + 2;
    }

    if (smin_q < 2) {
        q_st = 1;
        q_ed = smin_q + 2;
    } else if (smin_q > 59) {
        q_ed = 63;
        q_st = smin_q - 2;
    } else {
        q_st = smin_q - 2;
        q_ed = smin_q + 2;
    }


    unsigned char fmin_i = smin_i;
    unsigned char fmin_q = smin_q;
    for (unsigned char i = i_st; i <= i_ed; i = i + 1) {
        for (unsigned char j = q_st; j <= q_ed; j = j + 1) {
            if (i == smin_i && j == smin_q) {
                continue;
            }
            rf_dcoc_iq_bypass(i, j);
            core_cclk_delay_tick((unsigned long long)sys_clk.cclk); //A 1us delay time is required to ensure that the ADC valve remains stable in the event of a change in DC-offset
                                                                    //(Modified by zhiwei,confirmed by xuqiang&yuya at 20241210.)
            rf_rd_iq_val(value);

            unsigned int avg_i_sq = value[0] * value[0];
            unsigned int avg_q_sq = value[1] * value[1];
            unsigned int total    = avg_q_sq + avg_i_sq;

            if (total <= stotal_t) {
                fmin_i   = i;
                fmin_q   = j;
                stotal_t = total;
            }
        }
    }
    iq_value->i     = fmin_i;
    iq_value->q     = fmin_q;
    iq_value->total = stotal_t;
}

/**
 * @brief    This function is mainly used for dcoc calibration by the software.
 * @return   none.
 */
_attribute_ram_code_sec_noinline_ void rf_rx_dcoc_cali_by_sw(void)
{
    q_final.total           = 0xffffffff;
    unsigned char hw_dcoc_i = 0;
    unsigned char hw_dcoc_q = 0;

    write_reg8(0x17044d, 0x00);                                  //rx_sync_chnl disable ,never get synced
                                                                 //<5:0>rx_chn_en        default:0x1,->0 rx_sync_chnl all closed
    write_reg8(0x170420, 0xcc);                                  //MODEM_MODE_CFG_RX1_0 0xc4->0xcc
                                                                 //<3>:cont_mode,        default:0,->1 Enable continue mode.
    rf_set_rxmode();
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk * 95); //delay is required, must insure dcoc_calibration finished
                                                                 //Different chips have different delay time, TL721X typical value is 81,other chips need to be reconfirmed,
                                                                 //for insurance here the delay time is 95us.(modified by zhiwei,confirmed by yuya,20250109)
    hw_dcoc_i = read_reg8(0x1706d8) & 0x3f;                      //DCOC_IDAC 0xd8[5:0]
    hw_dcoc_q = read_reg8(0x1706da) & 0x3f;                      //DCOC_QDAC 0xda[5:0]

    write_reg8(0x170778, 0x0f);                                  //lnm_pa_ow_ctrl_val 0x00->0x0f
                                                                 //<0>:rx_lna_pup_ow      default:0,->1 lna overwrite en open, LNA-OFF
                                                                 //<1>:rx_lna_hgain_ow    default:0,->1 LNA-high-gain-overwrite en open
                                                                 //<2>:rx_lna_lgain_ow    default:0,->1 LNA-low-gain-overwrite en open
                                                                 //<3>:rx_lna_attn_ow     default:0,->1 LNA-capacitvie-attenuation-overwrite en open
                                                                 //LNA-high-gain path, number of slices
                                                                 //LNA-low-gain path, number of slices and capacitive attenuation
    write_reg8(0x17077a, 0x00);                                  //<0>:rx_lna_pup        default:0
                                                                 //<6:1>:rx_lna_hgain    default:0x3f, -> 0 LNA-high-gain-overwrite-slices 0
                                                                 //<7>:rx_lna_lgain      default:0
                                                                 //7b<0>:rx_lna_hgain    default:0
                                                                 //7b<2:1>:rx_lna_attn   default:0

    write_reg8(0x170640, 0x16);                                  //RADIO_TXRX_DBG1_0     0x14->0x16 read ADC_IQ data only once
                                                                 //<2>:agc_diasable,     default:0,->1 Turn off the agc auto-adjustment function.

    rf_adc_iq                = rf_get_adc_iq();
    unsigned int avg_hw_i_sq = rf_adc_iq.adc_i * rf_adc_iq.adc_i;
    unsigned int avg_hw_q_sq = rf_adc_iq.adc_q * rf_adc_iq.adc_q;
    unsigned int hw_total    = avg_hw_i_sq + avg_hw_q_sq;

    rf_set_dcoc_iq_offset(0x0001);
    rf_dcoc_iq_search(Q_FIX_FIRST, &q_first);
    rf_dcoc_iq_search(I_FIX_FIRST, &i_first);
    unsigned int boundary_value = 12800; //boundary_value = 80*80+80*80;
    if ((q_first.total >= boundary_value) || (i_first.total >= boundary_value) || (hw_total >= boundary_value)) {
        //re-calibration q_final
        rf_dcoc_iq_search(Q_FIX_FIRST, &q_final);
    }
    unsigned int min_total = (q_first.total <= i_first.total && q_first.total <= q_final.total && q_first.total <= hw_total) ? q_first.total :
                             (i_first.total <= q_final.total && i_first.total <= hw_total)                                   ? i_first.total :
                             (q_final.total <= hw_total)                                                                     ? q_final.total :
                                                                                                                               hw_total;

    if (min_total == q_first.total) {
        rf_dcoc_iq_bypass(q_first.i, q_first.q);
    } else if (min_total == i_first.total) {
        rf_dcoc_iq_bypass(i_first.i, i_first.q);
    } else if (min_total == q_final.total) {
        rf_dcoc_iq_bypass(q_final.i, q_final.q);
    } else if (min_total == hw_total) {
        rf_dcoc_iq_bypass(hw_dcoc_i, hw_dcoc_q);
    } else {
        rf_dcoc_iq_bypass(q_first.i, q_first.q);
    }


    rf_set_tx_rx_off();
    write_reg8(0x170420, 0xc4); //MODEM_MODE_CFG_RX1_0  0xcc->0xc4
                                //<3>:cont_mode,        1,->0 disable continue mode.
    write_reg8(0x170778, 0x00); //lnm_pa_ow_ctrl_val    0x01->0x00
                                //<0>:rx_lna_pup_ow     1,->0 release lna_overwrite_en
    write_reg8(0x170640, 0x14); //RADIO_TXRX_DBG1_0     0x16->0x14
                                //<2>:agc_diasable,     1,->0 Turn on the agc auto-adjustment function.
    write_reg8(0x17077a, 0x7e); //lna ow val  release lna_overwrite
    write_reg8(0x17044d, 0x01); //rx_sync_chnl open ,can receive packet
}

/**
 * @brief     This function is used to select the A2 version tx power mode.
 * @param[in] tx_power_mode - tx power mode.
 * @return    none.
 * @note      (1)There are two types of tx power modes, RF_TX_NORMAL_POWER and RF_TX_HIGH_POWER
 *            (2)The default is RF_TX_NORMAL_POWER, RF_TX_HIGH_POWER mode can increase tx power.
 *            (3)In version A2, if the transmission power exceeds 10 dBm, it will cause the 2M transmission drift test to fail.
 *               To resolve this, it is necessary to increase the preamble length to 7 byte in 2M mode.
 *            (4)This interface is only for internal testing purposes.
 */
void rf_tx_power_mode(rf_tx_power_e tx_power_mode)
{
    if (tx_power_mode == RF_TX_NORMAL_POWER) {
        reg_rf_radio_extral_1 = (reg_rf_radio_extral_1 & (~FLD_RF_PA_CAS_BIAS_DIG));
    } else {
        reg_rf_radio_extral_1 = (reg_rf_radio_extral_1 & (~FLD_RF_PA_CAS_BIAS_DIG)) | 0x03;
    }
}

/**
 * @brief     This function is used to select the A2 version rx performance mode.
 * @param[in] rx_performance - rx performance mode.
 * @return    none.
 * @note      There are two types of RX performance modes, RF_RX_NORMAL_PERFORMANCE and RX_HIGH_PERFORMANCE
 *            The default is RF_RX_NORMAL_PERFORMANCE, This mode is the default performance configuration.
 *            RX_HIGH_PERFORMANCE mode can improve performance, but the rx power consumption will increase
 */
void rf_rx_performance_mode(rf_rx_performance_e rx_performance)
{
    if (rx_performance == RF_RX_NORMAL_PERFORMANCE) {
        reg_rf_cbpf_adc_0 = (reg_rf_cbpf_adc_0 & (~FLD_RF_CBPF_TRIM_I) & (~FLD_RF_CBPF_TRIM_Q));
    } else {
        reg_rf_cbpf_adc_0 = (reg_rf_cbpf_adc_0 & (~FLD_RF_CBPF_TRIM_I) & (~FLD_RF_CBPF_TRIM_Q)) | 0x0a;
    }
}

/**
 * @brief      This function serves to initiate information of RF.
 * @return       none.
 * @return       none.
 * @note          Attention:
 *                 In order to solve the problem of poor receiver sensitivity performance of some chips with large DC offset:
 *                 1.Added DCOC software calibration scheme to the rf_mode_init() interface to get the smallest DC-offset for the chip.
 *                But there is thing to note:
 *                (1)Using DCOC software calibration will increase the software execution time of rf_mode_init().
 */
void rf_mode_init(void)
{
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    reg_rst4 |= FLD_RST4_ZB;
    reg_clk_en4 |= FLD_CLK4_ZB_EN;

    reg_n22_rst    = 0xfffe; //reset dma_bb,zb_pon,zb,rstl_stimer,rst_modem,rstl_bb(bb:baseband)
    reg_n22_clk_en = 0xffff; //enable dma_bb,zb_hclk,clk_bb,clkzb32k_lp clock
    reg_bb_timer_ctrl |= FLD_BB_TIMER_TIMER_EN;
    //one_time_setup
    write_reg8(0x1706d2, 0x9b); //DCOC_SFIIP:bit<4> DCOC_SFQQP:bit<5> DCOC_SFII_L:bit<6-7>
    write_reg8(0x1706d3, 0x19); //DCOC_SFII_H:bit<0-1> DCOC_SFQQ:bit<2-5>
#if RF_RX_SHORT_MODE_EN
    write_reg8(0x17047b, 0x0e); //BLANK_WINDOW
    write_reg8(0x170479, 0x38); //BIT[3] RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix
                                //per floor issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#else
    write_reg8(0x17047b, 0xfe); //BLANK_WINDOW
    write_reg8(0x170479, 0x08); //RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix per floor
                                //issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#endif

    //To set AGC thresholds
    write_reg8(0x17064a, 0x0e); //POW_000_001:bit<0-6> POW_001_010_L:bit<7>
    write_reg8(0x17064b, 0x09); //POW_001_010_H:bit<0-5>
    write_reg8(0x17064e, 0x09); //POW_100_101:bit<0-6> POW_101_100_L:bit<7>
    write_reg8(0x17064f, 0x0f); //POW_101_100_H:bit<0-5>
    write_reg8(0x170654, 0x0e); //POW_000_001:bit<0-6> POW_001_010_L:bit<7>
    write_reg8(0x170655, 0x09); //POW_001_010_H:bit<0-5>
    write_reg8(0x170656, 0x0c); //POW_010_011:bit<0-6> POW_011_100_L:bit<7>
    write_reg8(0x170657, 0x08); //POW_011_100_H:bit<0-5>
    write_reg8(0x170658, 0x09); //POW_100_101:bit<0-6> POW_101_100_L:bit<7>
    write_reg8(0x170659, 0x0f); //POW_101_100_H:bit<0-5>

    //For optimum preamble detection
    write_reg8(0x170476, 0x50);                  //RX_PE_DET_MIN_LO_THRESH
    write_reg8(0x170477, 0x73);                  //RX_PE_DET_MIN_HI_THRESH
    rf_clr_irq_mask(FLD_RF_IRQ_ALL);             //The default interrupt mask in RF is open.
    reg_rf_ll_ctrl3 &= ~(FLD_RF_R_TX_EN_DLY_EN); //Turn off the extension tx_en function
    //Close the interrupt mask in the initialization code and reopen it when in use

    if ((g_chip_version == CHIP_VERSION_A0) || (g_chip_version == CHIP_VERSION_A1)) {
        /*
        *       bit                 default value               note
        *                                                       note
        * ---------------------------------------------------------------------------
        * <5:4>:pa_vbias           default:01,->00,->11(515mv->535mv->465mv)
        * Reduce pa_vbais voltage to optimise TX transmit power consumption.modified by chenxi.wang,confirmed
        * by wenfeng.lou 24020826.
        */
        write_reg8(0x17074c, 0x30);
        /*
        *       bit                 default value               note
        *                                                       note
        * ---------------------------------------------------------------------------
        * <7:5>:TX_BUF_TRIM_DIG           default:0x04,->0x06 Tx lo buffer vbias trim to adjust the duty cycle.
        * Adjust the duty cycle of the Tx Lo buffer vbias trim to reduce TX power consumption.modified by chenxi.wang,confirmed
        * by wenfeng.lou 24020826.
        */
        write_reg8(0x170638, 0xc8);
        /*
        *       bit                 default value               note
        *                                                       note
        * ---------------------------------------------------------------------------
        * <1:0>:PD_TRIM_FCAL_BIAS           default:0x00,->0x02(0.361-->0.457) Trim for the fcal bias block. Vctrl node is biased to this voltage when PLL is in open loop.
        * <2>:  PD_EN_VPD_PULLUP            default:0x00 enable pullup of vpd.
        * <3>:  PD_EN_VPD_PULLDN            default:0x00 enable pulldn of vpd.
        * <5:4>:DAC_TRIM_IBIAS              default:0x00 Not used inside the DAC.
        * <7:6>:DAC_TRIM_RLOAD              default:0x00 Trim DAC output resistor.
        * Adjust the voltage of the fcal bias block to improve fdev performance.Due to the revamped design optimization this setting on the A2 chip can be restored to use
        * the default values.modified by zhiwei.wang,confirmed by wenfeng.lou 20241012.
        */
        write_reg8(0x170752, 0x02);
        /*
         *       bit                 default value               note
         *                                                       note
         * ---------------------------------------------------------------------------
         * <2:0>:VCO_TRIM_KVT                default:0x07,->0x00(80MHz/V-->50MHz/V) Adjustment of Kv of vctrl path depending upon reference frequency.
         *                                                                        Default should change depending upon if the reference frequency is 24MHz or 32MHz.
         * <3>:VCO_EN_PKDET                  default:0x00 enable peak detector operation.
         * <5:4>:LDOTRIM_TRIM_VREF           default:0x02 Bump bits for the 900 mV LDOTRIM reference voltage..
         * Adjust the Kv of vctrl path depending upon reference frequency to improve fdev performance.modified by zhiwei.wang,confirmed by wenfeng.lou 20241012.
         */
        write_reg8(0x170754, 0x20);
    }
    /*
     *       bit                 default value               note
     *                                                       note
     * ---------------------------------------------------------------------------
     * <2:0>:DAC_TRIM_VCM                default:0x01,->0x07(0.3105-->0.5675) Trim DAC output common mode.
     * <4:3>:DAC_TRIM_RFBK               default:0x00 Bump bits for the gain of the output opamp in the DAC.
     * <5>:DAC_INVERT_CLK                default:0x00 Invert the clock edge on which incoming data from digital is latched. By default falling edge is used.
     * Adjust the DAC_TRIM_VCM gear to improve fdev performance.modified by zhiwei.wang,confirmed by wenfeng.lou 20241012.
     */
    write_reg8(0x170753, 0x27);
    /*
     *         bit                        default    value                note
     * ---------------------------------------------------------------------------
     * <1:0>:PA_RAMP_MODE           default:01,->03 Increment PA slices to programmed value from 0 using delay of 24M between each step.
     *                              (1 - 2 - 4 - 8 -16 - 32 - 48-63)
     * <4:2>:EXT_PA_EN_ASSERT_DLY   default:02 delay of ext_pa_en signal going high.0 to 3.5us in steps of 0.5us.
     * <7:5>:EXT_PA_EN_DEASSERT_DLY default:02 delay of ext_pa_en signal going low.0 to 3.5us in steps of 0.5us.
     * This setting is set to 0x4b to improve the bandedge characteristics.modified by chenxi.wang,confirmed by wenfeng.lou 20241218.
     */
    write_reg8(0x170624, 0x4b);
    if (g_chip_version == CHIP_VERSION_A2) {
        /*
        *       bit                 default value               note
        *                                                       note
        * ---------------------------------------------------------------------------
        * <1:0>:lna_itrim          default:00,->11(4.4u->6.2u)
        * <5:4>:pa_vbias           default:01,->00,->11(515mv->535mv->465mv)
        * Reduce pa_vbais voltage to optimise TX transmit power consumption and Adjusting lna_itrim to optimize RX performance.
        * modified by chenxi.wang,confirmed by wenfeng.lou 20241205.
        */
        write_reg8(0x17074c, 0x33);
        /*
         *       bit                 default value               note
         *                                                       note
         * ---------------------------------------------------------------------------
         * <2:0>:VCO_TRIM_KVT                default:0x07,->0x03(80MHz/V-->70MHz/V) Adjustment of Kv of vctrl path depending upon reference frequency.
         *                                                                        Default should change depending upon if the reference frequency is 24MHz or 32MHz.
         * <3>:VCO_EN_PKDET                  default:0x00 enable peak detector operation.
         * <5:4>:LDOTRIM_TRIM_VREF           default:0x02,->0x03 Bump bits for the 900 mV LDOTRIM reference voltage..
         * Adjust the Kv of vctrl path depending upon reference frequency to improve fdev performance.modified by zhiwei.wang,confirmed by wenfeng.lou 20241012.
         */
        write_reg8(0x170754, 0x33);
        /*
         * This configuration is used for A2 to improve the performance of rf rx sensitivity.
         * Defaults to RF_RX_NORMAL_PERFORMANCE for A2.
         * If you need higher performance, you need to call rf_rx_performance_mode() after rf_mode_init;
         * Select the RF_RX_HIGH_PERFORMANCE mode, in which the RX sensitivity will increase, but the receiving power consumption will increase
         * (modified by chenxi.wang,confirmed by wenfeng.lou 20241205.)
         */
        rf_rx_performance_mode(RF_RX_HIGH_PERFORMANCE);
        /*
         * This configuration is used for A2 to improve the tx power.
         * Defaults to RF_TX_NORMAL_POWER for A2.
         * If you need higher power, you need to call rf_tx_power_mode() after rf_mode_init;
         * RF_TX_HIGH_POWER mode can increase tx power energy.
         * Note:
         * (1)In version A2, if the transmission power exceeds 10 dBm, it will cause the 2M transmission drift test to fail.
         *    To resolve this, it is necessary to increase the preamble length to 7 byte in 2M mode.
         * (2)This interface is only for internal testing purposes.
         * (modified by chenxi.wang,confirmed by wenfeng.lou 20241205.)
         */
        rf_tx_power_mode(RF_TX_NORMAL_POWER);

        /*
         *   reg            bit                 default value               note
         *                                                       note
         * ---------------------------------------------------------------------------
         * LDO1_1(0x170741)       <5>:VCO_TRIM_KVT          default:0,-> 1 Bypass the LDO output to Vline
         * REG_SPARELV1(0x17075c) <3>:                      default:0,-> 1 Bypass the LDO output to Vline
         * The following two configurations increase the output power in ANT mode by using ANT_LDO bypass. modified by chenxi.wang,confirmed by wenfeng.lou 20241210.
         */
        write_reg8(0x170741, 0x20);
        write_reg8(0x17075c, 0x09);
    }

#if(RF_RX_DCOC_SOFTWARE_CAL_EN)
    if (s_dcoc_software_cal_en == 1) {
        /*Solve the problem of unstable rx sensitivity test of some chips by software dcoc calibration scheme. If the calibration value is
         *not lost after a calibration is completed, it can be used directly without recalibration. Since the _attribute_data_retention_sec_ type
         *variable is not lost in suspend and deep retention modes, it can be used to record the calibration value to avoid having to perform
         *software calibration again after returning from suspend and deep retention modes.(Modified by zhiwei,confirmed by xuqiang and yuya at 20250102.)
         *After calibration is completed, it is impossible for the value of g_rf_dcoc_iq_code to be 0.
         *@Note:
         *(1)According to B92's setup the solution is to put it at the beginning of rf_mode_init;However, on the TL721x, I found that the dcoc calibration is
         *not good where it is placed after reg_bb_timer_ctrl |= FLD_BB_TIMER_TIMER_EN;.Confirmed with yuya that it needs to be delayed(typical value:5us) for
         *a while after power up so put the solution at the end of rf_mode_init on the TL721X.(Modified by zhiwei,confirmed by xuqiang,jianzhi and yuya at 20250102.)
         *(2)Once the software DCOC is calibrated, there is no need to retrieve the calibration value and set it back during the fast settle process.However, the sequence of
         * fast settle is only related to the setsettle time, so this part does not need to be changed accordingly to the software dcoc.(Modified by zhiwei,confirmed by yuya at 20250109.)
         *(3)The calibration of the software DCOC needs to be done after the call to the rf_rx_performance_mode function, as this function changes the current resulting
         *in different DCOC values.(Modified by zhiwei,confirmed by wenfeng at 20250109.)
         */
        if (g_rf_dcoc_iq_code == 0) {
            rf_rx_dcoc_cali_by_sw();
        } else {
            rf_set_dcoc_iq_offset(0x0001);
            rf_set_dcoc_iq_code(g_rf_dcoc_iq_code);
        }
    }
#endif
    /*
     *         bit                        default    value                note
     * ---------------------------------------------------------------------------
     * <7>:PA_RAMP_TSEQ_OR_TX_ON_SEL      default:0,->1 bit to select between tx on or pa ramp from timing sequence
     * (1)This setting advances the PA ramp start time to the end of the timing sequence,
     *    and after configuration, tx performs PA ramp up before preamble carrier.
     * (2)Due to the PA ramp up performed by tx before preamble transmission, the settling time of tx will increase by 8us.
     *    To adapt to this TX method, the preamble length will be reduced
     *  Modified by chenxi.wang,confirmed by xuqiang.zhang 20250114.
     */
     reg_rf_lnm_pa_ow_ctrl_val |=FLD_RF_PA_RAMP_TSEQ_OR_TX_ON;
}

/**
 * @brief      This setting serve to set the configuration of Tx DMA.
 */
// _attribute_data_sec_    //BLE USED: in IRQ
rf_dma_config_t rf_tx_dma_config = {
    .dst_req_sel    = 8,                  //tx req.(must 8)
    .src_req_sel    = 0,
    .dst_addr_ctrl  = DMA_ADDR_FIX,
    .src_addr_ctrl  = DMA_ADDR_INCREMENT, //increment.
    .dstmode        = DMA_HANDSHAKE_MODE, //handshake.
    .srcmode        = DMA_NORMAL_MODE,
    .dstwidth       = DMA_CTR_WORD_WIDTH, //must word.
    .srcwidth       = DMA_CTR_WORD_WIDTH, //must word.
    .src_burst_size = 0,                  //must 0.
    .vacant_bit     = 0,
    .read_num_en    = 1,
    .priority       = 0,
    .write_num_en   = 0,
    .auto_en        = 1, //must 1.
};

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] none
 * @return    none.
 */
_attribute_ram_code_  //BLE SDK use
void rf_set_tx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK); //u_pd_mcu.u_dmac.atcdmac100_ahbslv.tx_multi_en,rx_multi_en,ch_0_rnum_en_bk.
    rf_dma_config(RF_TX_DMA, &rf_tx_dma_config);
    rf_dma_set_dst_address(RF_TX_DMA, reg_rf_txdma_adr);
}

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] fifo_depth        - tx chn deep,fifo_depth range: 0~5,Number of fifo=2^fifo_depth.
 * @param[in] fifo_byte_size    - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return    none.
 */
void rf_set_tx_dma(unsigned char fifo_dep, unsigned short fifo_byte_size)
{
    rf_set_tx_dma_config();
    rf_set_tx_dma_fifo_num(fifo_dep);
    rf_set_tx_dma_fifo_size(fifo_byte_size);
}

/**
 * @brief      This setting serve to set the configuration of Rx DMA.
 */
// _attribute_data_sec_    //BLE USED: in IRQ
rf_dma_config_t rf_rx_dma_config = {
    .dst_req_sel    = 0,                  //tx req.
    .src_req_sel    = 9,                  //must 9
    .dst_addr_ctrl  = 0,
    .src_addr_ctrl  = DMA_ADDR_FIX,       //increment.
    .dstmode        = DMA_NORMAL_MODE,
    .srcmode        = DMA_HANDSHAKE_MODE, //handshake.
    .dstwidth       = DMA_CTR_WORD_WIDTH, //must word.
    .srcwidth       = DMA_CTR_WORD_WIDTH, //must word.
    .src_burst_size = 0,                  //must 0.
    .vacant_bit     = 0,
    .read_num_en    = 0,
    .priority       = 0,
    .write_num_en   = 1,
    .auto_en        = 1, //must 1.
};

/**
 * @brief       This function serve to rx dma config
 * @param[in]   none
 * @return      none
 */
_attribute_ram_code_  //BLE SDK use
void rf_set_rx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK); //ch0_rnum_en_bk,tx_multi_en,rx_multi_en.
    rf_dma_config(RF_RX_DMA, &rf_rx_dma_config);
    rf_dma_set_src_address(RF_RX_DMA, reg_rf_rxdma_adr);
    rf_dma_set_size(RF_RX_DMA, 0xFFFFFC, RF_DMA_WORD_WIDTH);
}

/**
 * @brief      This function serves to rx dma setting.
 * @param[in]  buff - This parameter is the first address of the received data buffer, which must be 4 bytes aligned, otherwise the program will enter an exception.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *             -# you need to leave 4bytes of space for the length information.
 *             -# dma is transmitted in accordance with 4bytes, so the length of the buffer needs to be a multiple of 4. Otherwise, there may be an out-of-bounds problem
 *             For example, the actual received data length is 5bytes, the minimum value of the actual buffer size that the user needs to set is 12bytes, and the calculation of 12bytes is explained as follows::
 *             4bytes (length information) + 5bytes (data) + 3bytes (the number of additional bytes to prevent out-of-bounds)
 * @param[in]  wptr_mask       - This parameter is used to set the mask value for the number of enabled FIFOs. The value of the mask must (0x00,0x01,0x03,0x07,0x0f,0x1f).
 *                               The number of FIFOs enabled is the value of wptr_mask plus 1.(0x01,0x02,0x04,0x08,0x10,0x20)
 * @param[in]  fifo_byte_size  - The length of one dma fifo,the range is 1~0xffff(the corresponding number of fifo bytes is fifo_byte_size).
 * @return     none.
 */
void rf_set_rx_dma(unsigned char *buff, unsigned char wptr_mask, unsigned short fifo_byte_size)
{
    rf_set_rx_dma_config();
    rf_set_rx_buffer(buff);
    rf_set_rx_dma_fifo_num(wptr_mask);
    rf_set_rx_dma_fifo_size(fifo_byte_size);
}

/**
 * @brief       This function serves to RF trigger stx
 * @param[in]   addr    - DMA tx buffer.
 * @param[in]   tick    - Send after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_stx(void *addr, unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x85;                        //stx
}

/**
 * @brief     This function serves to trigger srx on.
 * @param[in] tick  - Trigger rx receive packet after tick delay.
 * @return    none.
 */
void rf_start_srx(unsigned int tick)
{
    reg_rf_ll_rx_fst_timeout = 0x0fffffff;       // first timeout.
    reg_rf_ll_cmd_schedule   = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x86;
}

/**
 * @brief       This function serves to RF trigger stx2rx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger tx send packet after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_stx2rx(void *addr, unsigned int tick)
{
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(addr));
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x87;                        // single tx2rx.
}

/**
 *  @brief      This function is mainly used to set hpmc Calibration-related values.
 *  @param[in]  hpmc_gain  - hpmc Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_sec_noinline_ static void rf_set_hpmc_cal_val(unsigned short hpmc_gain)
{
    //The calibration value of hpmc is different at different frequency points,
    //So you need to reset it every time you switch channels.

    unsigned short tmp = read_reg16(0x1706f6);
    tmp                = (tmp & 0xf001) | hpmc_gain; //bit<1:11> 1111 0000 0000 0000
    write_reg16(0x1706f6, tmp);
}

volatile unsigned char g_single_tong_freqoffset = 0; //for eliminate single carrier frequency offset.

/**
 * @brief       This function serves to set rf channel for all mode.The actual channel set by this function is 2400+chn.
 * @param[in]   chn   - That you want to set the channel as 2400+chn.
 * @return      none.
 */
void rf_set_chn(signed char chn)
{
    if (g_rf_tx_fast_settle_chn_cal_flag == 1) {
        rf_set_hpmc_cal_val(g_fast_settle_cal_val_ptr->cal_tbl[chn]);
    }
    unsigned int  freq_low;
    unsigned int  freq_high;
    unsigned int  chnl_freq;
    unsigned char ctrim;
    unsigned int  freq;

    freq = 2400 + chn;
    if (freq >= 2550) {
        ctrim = 0;
    } else if (freq >= 2520) {
        ctrim = 1;
    } else if (freq >= 2495) {
        ctrim = 2;
    } else if (freq >= 2465) {
        ctrim = 3;
    } else if (freq >= 2435) {
        ctrim = 4;
    } else if (freq >= 2405) {
        ctrim = 5;
    } else if (freq >= 2380) {
        ctrim = 6;
    } else {
        ctrim = 7;
    }

    chnl_freq = freq * 2 + g_single_tong_freqoffset;
    freq_low  = (chnl_freq & 0x7f);
    freq_high = ((chnl_freq >> 7) & 0x3f);

    write_reg8(0x170644, (read_reg8(0x170644) | 0x01));
    write_reg8(0x170644, (read_reg8(0x170644) & 0x01) | freq_low << 1);
    write_reg8(0x170645, (read_reg8(0x170645) & 0xc0) | freq_high);
    write_reg8(0x170629, (read_reg8(0x170629) & 0x1f) | (ctrim << 5)); //FE_CTRIM
}

/**
 * @brief       This function serves to get rssi.
 * @return      rssi value.
 */
signed char rf_get_rssi(void)
{
    return (((signed char)(read_reg8(REG_TL_MODEM_BASE_ADDR + 0x5d))) - 110); //this function can not tested on fpga
}

/**
 * @brief       This function serves to set RF Rx manual on.
 * @return      none.
 */
void rf_set_rxmode(void)
{
    reg_rf_ll_ctrl0 = 0x45;                          // reset tx/rx state machine.
    reg_rf_modem_mode_cfg_rx1_0 |= FLD_RF_CONT_MODE; //set continue mode.
    reg_rf_ll_ctrl0 |= FLD_RF_R_RX_EN_MAN;           //rx enable.
    reg_rf_rxmode |= FLD_RF_RX_ENABLE;               //bb rx enable.
}

/**
 * @brief       This function serves to get the right fifo packet.
 * @param[in]   fifo_num   - The number of fifo set in dma.
 * @param[in]   fifo_dep   - deepth of each fifo set in dma.
 * @param[in]   addr       - address of rx packet.
 * @return      the next rx_packet address.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
unsigned char *rf_get_rx_packet_addr(int fifo_num, int fifo_dep, void *addr)
{
    unsigned char rptr;
    rptr                   = reg_rf_dma_rx_rptr;
    unsigned char *raw_pkt = (unsigned char *)((unsigned char *)addr + (rptr & (fifo_num - 1)) * (fifo_dep));
    reg_rf_dma_rx_rptr     = 0x40;
    return raw_pkt;
}

/**
 * @brief       This function serves to set RF Tx mode.
 * @return      none.
 */
void rf_set_txmode(void)
{
    reg_rf_ll_ctrl0 = 0x45; // reset tx/rx state machine.
    reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
    reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}

/**
 * @brief       This function serves to set RF Tx packet address to DMA src_addr.
 * @param[in]   addr   - The packet address which to send.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_tx_pkt(void *addr)
{
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(addr));
    reg_bb_dma_ctr0(0) |= 0x01;
}

/**
 * @brief       This function serves to disable pn of rf mode.
 * @return      none.
 */
void rf_pn_disable(void)
{
    reg_rf_tx_mode2 &= (~FLD_RF_ZB_PN_EN);
    reg_rf_tx_mode2 &= (~FLD_RF_V_PN_EN);
}

/**
 * @brief       This function serves to set RF power level.
 * @param[in]   level    - The power level to set.
 * @return      none.
 */
_attribute_ram_code_sec_        /*!< added by BLE */
void rf_set_power_level(rf_power_level_e level)
{
    unsigned char value;
    if (level & BIT(7)) {
        reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;
    } else {
        reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
    }

    value                  = (unsigned char)(level & 0x3F);
    reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value & 0x01) << 7));
    reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value >> 1) & 0x1f));
    blt_extRF.txPower_level = level;                /*!< added by BLE, important */
}

/**
 * @brief       This function serves to set RF power through select the level index.
 * @param[in]   idx      - The index of power level which you want to set.
 * @return      none.
 */
void rf_set_power_level_index(rf_power_level_index_e idx)
{
    unsigned char value;
    unsigned char level = 0;

    if (idx < sizeof(rf_power_Level_list) / sizeof(rf_power_Level_list[0])) {
        level = rf_power_Level_list[idx];
        if (level & BIT(7)) {
            reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;
        } else {
            reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
        }

        value = (unsigned char)(level & 0x3F);

        reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value & 0x01) << 7));
        reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value >> 1) & 0x1f));

        /* add by BLE, important */
        blt_extRF.txPower_level = level;
        blt_extRF.txPower_index = (unsigned char)idx;
    }
}

/**
 * @brief     This function serves to get RF status.
 * @return    RF Rx/Tx status.
 */
rf_status_e rf_get_trx_state(void)
{
    return s_rf_trxstate;
}

/**
 * @brief       This function serves to judge RF Tx/Rx state.
 * @param[in]   rf_status   - Tx/Rx status.
 * @param[in]   rf_channel  - This param serve to set frequency channel(2400+rf_channel) .
 * @return      Whether the setting is successful(-1:failed;else success).
 */
int rf_set_trx_state(rf_status_e rf_status, signed char rf_channel)
{
    int err = 0;

    reg_rf_ll_ctrl0 = 0x45; // reset tx/rx state machine.
    rf_set_chn(rf_channel);

    if (rf_status == RF_MODE_TX) {
        rf_set_txmode();
        s_rf_trxstate = RF_MODE_TX;
    } else if (rf_status == RF_MODE_RX) {
        rf_set_rxmode();
        s_rf_trxstate = RF_MODE_RX;
    } else if (rf_status == RF_MODE_OFF) {
        rf_set_tx_rx_off();
        s_rf_trxstate = RF_MODE_OFF;
    } else if (rf_status == RF_MODE_AUTO) {
        reg_rf_ll_cmd   = 0x80;               //stop cmd.
        reg_rf_ll_ctrl3 = 0x29;               // reg0x140a16 pll_en_man and tx_en_dly_en  enable.
        reg_rf_rxmode |= (~FLD_RF_RX_ENABLE); //rx disable.
        reg_rf_ll_ctrl0 &= 0xce;              //reg0x140a02 disable rx_en_man and tx_en_man.
        s_rf_trxstate = RF_MODE_AUTO;
    } else {
        err = -1;
    }
    return err;
}

/**
 * @brief       This function serve to change the length of preamble.
 * @param[in]   len     -The value of preamble length.Set the register bit<0>~bit<4>.
 * @return      none
 */
void rf_set_preamble_len(unsigned char len)
{
    len                   = len & 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail & (~FLD_RF_PREAMBLE_LEN)) | len;
}

/**
 * @brief       This function serve to set the length of access code.
 * @param[in]   byte_len    -   The value of access code length,the range is 3~5byte.
 * @return      none
 */
void rf_set_access_code_len(unsigned char byte_len)
{
    unsigned char temp;
    temp                   = byte_len & 0x07;
    reg_rf_acclen          = (reg_rf_acclen & (~FLD_RF_ACC_LEN)) | temp;
    reg_rf_modem_rx_ctrl_0 = (reg_rf_modem_rx_ctrl_0 & (~FLD_RF_RX_ACC_LNE)) | temp;
}

/**
 * @brief       This function serves to RF trigger srx2rx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger rx receive packet after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_srx2tx(void *addr, unsigned int tick)
{
    reg_rf_ll_rx_fst_timeout = 0x0fffffff;       // first timeout
    reg_rf_ll_cmd_schedule   = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    rf_dma_set_src_address(RF_TX_DMA, (unsigned int)(addr));
    reg_rf_ll_rest_pid = 0x3f;
    reg_rf_ll_cmd      = 0x88;                   // single rx2tx
}

/**
 * @brief       This function is used to judge whether there is a CRC error in the received packet through hardware.
 *              For the same packet, the value of this bit is consistent with the CRC flag bit in the packet.
 * @param[in]   none.
 * @return      none.
 */
unsigned char rf_get_crc_err(void)
{
    return (reg_rf_dec_err & 0x10);
}

/**********************************************************************************************************************
 *  Fast settle related interfaces
 *  Attention:
 *  (1)This part of the function is only for the internal use of the driver, not open to customers to use,
 *  we will rewrite this part, and provide demo
 *  (2)When using TL721X fast settle, it should be noted that different settle times correspond to different calibration modules being turned off,
 *     so you need to manually configure the calibration values of these calibration modules before using fast settle.
 *  (3)Calibration modules that require manual setting of calibration values:
 *     RX_SETTLE_TIME_15US: ldo trim; rx_fcal; dcoc;
 *     RX_SETTLE_TIME_37US: ldo trim; dcoc;
 *     RX_SETTLE_TIME_77US: ldo trim;
 *
 *     TX_SETTLE_TIME_23US: ldo trim; tx_fcal; hpmc;
 *     TX_SETTLE_TIME_59US: ldo trim; hpmc;
 *     TX_SETTLE_TIME_112US: ldo trim;
 *
 *********************************************************************************************************************/

/**
 *  @brief      This function serve to adjust tx/rx settle timing sequence.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @return      0                   -  Correct configuration.
 *              -1                   -  Incorrect configuration.
*/
_attribute_ram_code_
signed char rf_fast_settle_config(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us)
{
    if ((tx_settle_us == TX_SETTLE_TIME_23US && rx_settle_us != RX_SETTLE_TIME_15US) ||
        (rx_settle_us == RX_SETTLE_TIME_15US && tx_settle_us != TX_SETTLE_TIME_23US)) {
        return -1;
    }

    g_rf_tx_fast_settle_time = tx_settle_us;
    g_rf_rx_fast_settle_time = rx_settle_us;
    //tx
    if (tx_settle_us == TX_SETTLE_TIME_23US) {
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_0 = 0x00; //sub-sequence1 start time:0
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_1 = 0x00; //sub-sequence2 start time:0us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb0  = 0x0c; //sub-sequence3 start time:12us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb1  = 0x0d; //sub-sequence4 start time:13us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_0 = 0x0f; //sub-sequence5 start time:15us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_1 = 0x0c; //sub-sequence6 start time:12us

        //disable Bandgap(8us),tx_ldo_trim(8.5us),PD_settle(5.5us),tx_fcal(12.5us),tx_hpmc(53us),save 25us
        //Default settle time:120.5us(tx seq end time is 112.5us with a fixed increase of 8us PA ramp time)
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl &= ~(FLD_RF_LDOT_TX_RUN_CB | FLD_RF_FCAL_TX_RUN_CB | FLD_RF_HPMC_RUN_CB); //0000
        reg_rf_txrx_en_dbg_ow_ctrl1 = (reg_rf_txrx_en_dbg_ow_ctrl1 & 0xfc);                               //1100
    } else if (tx_settle_us == TX_SETTLE_TIME_59US) {
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_0 = 0x00;                                                       //sub-sequence1 start time:0
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_1 = 0x08;                                                       //sub-sequence2 start time:8us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb0  = 0x30;                                                       //sub-sequence3 start time:47.5us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb1  = 0x31;                                                       //sub-sequence4 start time:48us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_0 = 0x33;                                                       //sub-sequence5 start time:51us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_1 = 0x30;                                                       //sub-sequence6 start time:48us

        //close hpmc and ldo trim,close hpmc(53us), ldotrim(8.5us),save 51us
        //Default settle time:120.5us(tx seq end time is 112.5us with a fixed increase of 8us PA ramp time)
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl & 0xf0) | 0x0a; //1010
    } else if (tx_settle_us == TX_SETTLE_TIME_112US) {
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_0 = 0x00;                        //sub-sequence1 start time:0
        reg_rf_idle_txfsk_ss1_ss2_strt_cb_1 = 0x08;                        //sub-sequence2 start time:0us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb0  = 0x65;                        //sub-sequence3 start time:101us
        reg_rf_idle_txfsk_ss3_ss4_strt_cb1  = 0x66;                        //sub-sequence4 start time:102us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_0 = 0x69;                        //sub-sequence5 start time:105us
        reg_rf_idle_txfsk_ss6_ss7_strt_cb_1 = 0x65;                        //sub-sequence6 start time:102us

        // only close ldo trim(8.5us)
        //Default settle time:120.5us(tx seq end time is 112.5us with a fixed increase of 8us PA ramp time)
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl & 0xf8) | 0x0e; //1110
    }

    //rx
    if (rx_settle_us == RX_SETTLE_TIME_15US) {
        reg_rf_idle_rx_ss1_ss2_strt_cb_0 = 0x00; //sub-sequence1 start time:0us
        reg_rf_idle_rx_ss1_ss2_strt_cb_1 = 0x00; //sub-sequence2 start time:0us
        reg_rf_idle_rx_ss3_ss4_strt_cb_0 = 0x00; //sub-sequence3 start time:0us
        reg_rf_idle_rx_ss3_ss4_strt_cb_1 = 0x08; //sub-sequence4 start time:8us
        reg_rf_idle_rx_ss5_ss6_strt_cb_0 = 0x0f; //sub-sequence5 start time:11us
        reg_rf_idle_rx_ss5_ss6_strt_cb_1 = 0x0f; //sub-sequence6 start time:11us

        //disable Bandgap(8us), rx_ldo_trim(8.5us), PD_settle(5.5us), rx_fcal(12.5us),rx_rccal(9us), rx_dcoc(40us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl &= ~(FLD_RF_RXDCOC_RUN_CB | FLD_RF_RCCAL_RUN_CB | FLD_RF_FCAL_RX_RUN_CB | FLD_RF_LDOT_RX_RUN_CB); //0000
        reg_rf_txrx_en_dbg_ow_ctrl1 = (reg_rf_txrx_en_dbg_ow_ctrl1 & 0xf3);                                                       //0011
    } else if (rx_settle_us == RX_SETTLE_TIME_37US) {
        reg_rf_idle_rx_ss1_ss2_strt_cb_0 = 0x00;                                                                                  //sub-sequence1 start time:0us
        reg_rf_idle_rx_ss1_ss2_strt_cb_1 = 0x08;                                                                                  //sub-sequence2 start time:8us
        reg_rf_idle_rx_ss3_ss4_strt_cb_0 = 0x08;                                                                                  //sub-sequence3 start time:8us
        reg_rf_idle_rx_ss3_ss4_strt_cb_1 = 0x22;                                                                                  //sub-sequence4 start time:34us
        reg_rf_idle_rx_ss5_ss6_strt_cb_0 = 0x25;                                                                                  //sub-sequence5 start time:37us
        reg_rf_idle_rx_ss5_ss6_strt_cb_1 = 0x25;                                                                                  //sub-sequence6 start time:37us

        //disable ldo trim(8.5us),rx dcoc(40us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl & 0x0F) | 0x60; //0110
    } else if (rx_settle_us == RX_SETTLE_TIME_77US) {
        reg_rf_idle_rx_ss1_ss2_strt_cb_0 = 0x00;                           //sub-sequence1 start time:0us
        reg_rf_idle_rx_ss1_ss2_strt_cb_1 = 0x08;                           //sub-sequence2 start time:9us
        reg_rf_idle_rx_ss3_ss4_strt_cb_0 = 0x08;                           //sub-sequence3 start time:9us
        reg_rf_idle_rx_ss3_ss4_strt_cb_1 = 0x22;                           //sub-sequence4 start time:34us
        reg_rf_idle_rx_ss5_ss6_strt_cb_0 = 0x4d;                           //sub-sequence5 start time:77us
        reg_rf_idle_rx_ss5_ss6_strt_cb_1 = 0x4d;                           //sub-sequence6 start time:77us

        //disable ldo trim(8.5us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        reg_rf_txrx_cb_cal_ctrl = (reg_rf_txrx_cb_cal_ctrl & 0x0f) | 0xe0; //1110
    }
    return 0;
}

/**
 *  @brief      This function serve to enable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
_attribute_ram_code_
void rf_tx_fast_settle_en(void)
{
    if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_23US) {
        g_rf_tx_fast_settle_chn_cal_flag = 1;
        reg_rf_hpmc_debug_0 |= FLD_RF_HPMC_BYPASS;                                             //hpmc bypass enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass enable.
        reg_rf_tx_frac_ctrl0 |= FLD_RF_FCAL_STL_DCAP_EN;                                       //FCAL settle enable
    } else if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_59US) {
        g_rf_tx_fast_settle_chn_cal_flag = 1;
        reg_rf_hpmc_debug_0 |= FLD_RF_HPMC_BYPASS;                                             //hpmc bypass enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass enable.
    } else if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_112US) {
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass enable.
    }
    reg_rf_burst_cfg_txrx_1 |= FLD_RF_TX_TIM_SRQ_SEL_TESQ;
}

/**
 *  @brief      This function serve to disable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_tx_fast_settle_dis(void)
{
    g_rf_tx_fast_settle_chn_cal_flag = 0;
    g_rf_tx_fast_settle_time         = TX_FAST_SETTLE_NONE;
    reg_rf_hpmc_debug_0 &= ~FLD_RF_HPMC_BYPASS;                                             //hpmc bypass disable
    reg_rf_ldot_dbg1 &= ~FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass disable
    reg_rf_ldot_dbg2_0 &= ~(FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass disable
    reg_rf_ldot_dbg3_0 &= ~(FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass disable.
    reg_rf_tx_frac_ctrl0 &= ~FLD_RF_FCAL_STL_DCAP_EN;                                       //FCAL settle disable

    reg_rf_burst_cfg_txrx_1 &= ~FLD_RF_TX_TIM_SRQ_SEL_TESQ;                                 //tx fast settle disable
}

/**
 *  @brief      This function serve to enable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
_attribute_ram_code_
void rf_rx_fast_settle_en(void)
{
    if (g_rf_rx_fast_settle_time == RX_SETTLE_TIME_15US) {
#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
            reg_rf_dcoc_bypass_dac_0 |= FLD_RF_DCOC_BYPASS_DAC;                                //dcoc bypass dac enable
#endif
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass enable.
        reg_rf_tx_frac_ctrl0 |= FLD_RF_FCAL_STL_DCAP_EN;                                       //FCAL settle enable
        reg_rf_rccal_dbg1_0 |= FLD_RF_CBPF_CCODE_BYPASS;                                       //CBPF_CCODE_BYPASS enable
    } else if (g_rf_rx_fast_settle_time == RX_SETTLE_TIME_37US) {
        reg_rf_dcoc_bypass_dac_0 |= FLD_RF_DCOC_BYPASS_DAC;                                    //dcoc bypass dac enable
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass enable.
    } else if (g_rf_rx_fast_settle_time == RX_SETTLE_TIME_77US) {
        reg_rf_ldot_dbg1 |= FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass enable
        reg_rf_ldot_dbg2_0 |= (FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass enable
        reg_rf_ldot_dbg3_0 |= (FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass enable.
    }
    reg_rf_burst_cfg_txrx_1 |= FLD_RF_RX_TIM_SRQ_SEL_TESQ;
}

/**
 *  @brief      This function serve to disable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_rx_fast_settle_dis(void)
{
    g_rf_rx_fast_settle_time = RX_FAST_SETTLE_NONE;

#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
        reg_rf_dcoc_bypass_dac_0 &= ~FLD_RF_DCOC_BYPASS_DAC;                                //dcoc bypass dac disable
#endif
    reg_rf_ldot_dbg1 &= ~FLD_RF_LDOT_LDO_CAL_BYPASS;                                        //ldo cal bypass disable
    reg_rf_ldot_dbg2_0 &= ~(FLD_RF_LDOT_LDO_RXTXHF_BYPASS | FLD_RF_LDOT_LDO_RXTXLF_BYPASS); //ldo RXTXHF/RXTXLF bypass disable
    reg_rf_ldot_dbg3_0 &= ~(FLD_RF_LDOT_LDO_PLL_BYPASS | FLD_RF_LDOT_LDO_VCO_BYPASS);       //ldo PLL/VCO bypass disable.
    reg_rf_tx_frac_ctrl0 &= ~FLD_RF_FCAL_STL_DCAP_EN;                                       //FCAL settle disable
    reg_rf_rccal_dbg1_0 &= ~FLD_RF_CBPF_CCODE_BYPASS;                                       //CBPF_CCODE_BYPASS disable

    reg_rf_burst_cfg_txrx_1 &= ~FLD_RF_RX_TIM_SRQ_SEL_TESQ;                                 //rx fast settle disable
}

/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim calibration value address pointer
 *  @return     none
*/

static void rf_get_ldo_trim_val(rf_ldo_trim_t *ldo_cla)
{
    ldo_cla->LDO_CAL_TRIM    = reg_rf_ldot_rdbk1 & FLD_RF_LDOT_LDO_CAL_TRIM;
    ldo_cla->LDO_RXTXHF_TRIM = reg_rf_ldot_rdbk2_0 & FLD_RF_LDOT_LDO_RXTXHF_TRIM;
    ldo_cla->LDO_RXTXLF_TRIM = ((reg_rf_ldot_rdbk2_1 & FLD_RF_LDOT_LDO_RXTXLF_TRIM_H) << 2) + ((reg_rf_ldot_rdbk2_0 & FLD_RF_LDOT_LDO_RXTXLF_TRIM_L) >> 6);
    ldo_cla->LDO_PLL_TRIM    = reg_rf_ldot_rdbk3_0 & FLD_RF_LDOT_LDO_PLL_TRIM;
    ldo_cla->LDO_VCO_TRIM    = ((reg_rf_ldot_rdbk3_1 & FLD_RF_LDOT_LDO_VCO_TRIM_H) << 2) + ((reg_rf_ldot_rdbk3_0 & FLD_RF_LDOT_LDO_VCO_TRIM_L) >> 6);
}

/**
 *  @brief      This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_
static void rf_set_ldo_trim_val(rf_ldo_trim_t ldo_trim)
{
    reg_rf_ldot_dbg1   = (ldo_trim.LDO_CAL_TRIM << 1);
    reg_rf_ldot_dbg2_0 = (ldo_trim.LDO_RXTXHF_TRIM << 2);
    reg_rf_ldot_dbg2_1 = ldo_trim.LDO_RXTXLF_TRIM & FLD_RF_LDOT_LDO_RXTXLF_TRIM_OVERWRITE;
    reg_rf_ldot_dbg3_0 = (ldo_trim.LDO_PLL_TRIM << 2);
    reg_rf_ldot_dbg3_1 = ldo_trim.LDO_VCO_TRIM & FLD_RF_LDOT_LDO_VCO_TRIM_OVERWRITE;
}

/**
 *  @brief      This function is mainly used to get hpmc Calibration-related values.
 *  @param[in]  none
 *  @return     Returns the hpmc_gain value
*/
_attribute_ram_code_sec_noinline_ static unsigned short rf_get_hpmc_cal_val(void)
{
    unsigned short cali;
    unsigned short hpmc_gain;
    cali      = read_reg16(0x1706fe);
    hpmc_gain = (cali << 1) & 0x0ffe;
    return hpmc_gain;
}

#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_
static void rf_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal)
{
    dcoc_cal->DCOC_IDAC        = reg_rf_dcoc_rdbk1_0 & FLD_RF_DCOC_IDAC_CODE;                                                                     //DCOC_IDAC 0xd8[5:0]
    dcoc_cal->DCOC_QDAC        = reg_rf_dcoc_rdbk2 & FLD_RF_DCOC_QDAC_CODE;                                                                       //DCOC_QDAC 0xda[5:0]
    dcoc_cal->DCOC_IADC_OFFSET = reg_rf_dcoc_rdbk3_0 & FLD_RF_DCOC_IADC_OFFSET;                                                                   //DCOC_IADC_OFFSET 0xdc[6:0]
    dcoc_cal->DCOC_QADC_OFFSET = (reg_rf_dcoc_rdbk3_0 & FLD_RF_DCOC_QADC_OFFSET_L) >> 7 | (reg_rf_dcoc_rdbk3_1 & FLD_RF_DCOC_QADC_OFFSET_H) << 1; //DCOC_QADC_OFFSET 0xdc[7] 0xdd[5:0]
}

/**
 *  @brief      This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_
static void rf_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal)
{
    reg_rf_dcoc_bypass_dac_0 = (dcoc_cal.DCOC_IDAC << 1);
    reg_rf_dcoc_bypass_dac_0 = reg_rf_dcoc_bypass_dac_0 | ((dcoc_cal.DCOC_QDAC & 0x01) << 7);
    reg_rf_dcoc_bypass_dac_1 = ((dcoc_cal.DCOC_QDAC) & 0x3e) >> 1;
    reg_rf_dcoc_bypass_adc_0 = (dcoc_cal.DCOC_IADC_OFFSET << 1) | 0x01;
    reg_rf_dcoc_bypass_adc_1 = dcoc_cal.DCOC_QADC_OFFSET;
}
#endif

/**
 *  @brief      This function is mainly used to get fcal Calibration-related values.
 *  @return     fcal dcap value
*/
_attribute_ram_code_sec_noinline_ static unsigned char rf_get_fcal_cal_val(void)
{
    return reg_rf_fcal_rdbk;
}

/**
 *  @brief      This function serves to set the range of chn group corresponding to the process of obtaining fcal calibration values at different frequency points
 *  @param[in]  *fcal_chn_range  - chn group range pointer(fcal_chn_range[0]<=chn_num <= fcal_chn_range[1])
 *  @return     none
 *  @note       If the frequency point is set using the rf_set_chn() interface when obtaining FCAL calibration values for different chn,
 *              this interface needs to be used to set the range of chn.
*/
static void rf_set_fcal_chn_group_range_ctf(unsigned short *fcal_chn_range)
{
    for (int i = 0; i < 8; i++) {
        fcal_chn_range[i] *= 2;
        reg_rf_fcal_chn_range_ctf_low(i)  = fcal_chn_range[i] & 0xff;
        reg_rf_fcal_chn_range_ctf_high(i) = (fcal_chn_range[i] >> 8) & 0x1f;
    }
    reg_rf_txrx_dbg3_0 |= FLD_RF_CHNL_FREQ_DIRECT; //CHNL Frequency decided by TXRX_DBG.CHNL_FREQ
}

/**
 *  @brief      This function is mainly used to get rccal Calibration-related values.
 *  @param[in]  rccal_cal   - rccal calibration value address pointer
 *  @return     none
*/
void rf_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal)
{
    rccal_cal->RCCAL_CODE   = reg_rf_rccal_rdbk_0 & FLD_RF_RCCAL_CODE;
    rccal_cal->CBPF_CCODE_L = (reg_rf_rccal_rdbk_0 & FLD_RF_CBPF_CCODE_L) >> 6;
    rccal_cal->CBPF_CCODE_H = reg_rf_rccal_rdbk_1 & FLD_RF_CBPF_CCODE_H;
}

/**
 *  @brief      This function is mainly used to set rccal Calibration-related values.
 *  @param[in]  rccal_cal    - rccal Calibration-related values.
 *  @return     none
*/
void rf_set_rccal_cal_val(rf_rccal_cal_t rccal_cal)
{
    reg_rf_rccal_dbg1_0 = (reg_rf_rccal_dbg1_0 & (~FLD_RF_RCCAL_OVERWRITE)) | (rccal_cal.RCCAL_CODE);
    reg_rf_rccal_dbg1_0 = (reg_rf_rccal_dbg1_0 & (~FLD_RF_CBPF_CCODE_OVERWRITE_L)) | ((rccal_cal.CBPF_CCODE_L & 0x01) << 7);
    reg_rf_rccal_dbg1_1 = (reg_rf_rccal_dbg1_1 & (~FLD_RF_CBPF_CCODE_OVERWRITE_H)) | ((rccal_cal.CBPF_CCODE_L & 0x02) >> 1) | ((rccal_cal.CBPF_CCODE_H) << 1);
}

/**
 *  @brief      This function is used to set the tx fast_settle calibration value.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to TX_SETTLE_TIME_23US and TX_SETTLE_TIME_59US, other parameters are invalid.
 *                              (When tx_settle_us is 23us or 59us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @return     none
 *  @note       TX_SETTLE_TIME_23US  - disable Bandgap,tx_ldo_trim,PD_settle,tx_fcal,tx_hpmc,reduce 87.5us of tx settle time.
                                       After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_59US  - disable tx_ldo_trim function and tx_hpmc,reduce 61.5us of tx settle time.After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_112US - disable tx_ldo_trim function,reduce 8.5us of tx settle time. Do a normal calibration at the beginning.
*/
void rf_tx_fast_settle_update_cal_val(rf_tx_fast_settle_time_e tx_settle_time, unsigned char chn)
{
    rf_fast_settle_t fs_cv;
    rf_tx_fast_settle_get_cal_val(tx_settle_time, chn, &fs_cv);
    rf_tx_fast_settle_set_cal_val(tx_settle_time, chn, &fs_cv);
}

/**
 *  @brief      This function is used to set the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to RX_SETTLE_TIME_15US, other parameters are invalid.
 *                              (When rx_settle_us is 15us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @return     none
 *  @note       RX_SETTLE_TIME_15US  - disable Bandgap, rx_ldo_trim, PD_settle, rx_fcal,rx_rccal, rx_dcoc calibration,reduce 74us of rx settle time.
                                       Receive for a period of time and then do a normal calibration
 *              RX_SETTLE_TIME_37US  - disable rx_ldo_trim and rx_dcoc calibration,reduce 48.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_77US  - disable rx_ldo_trim calibration,reduce 8.5us of rx settle time. Do a normal calibration at the beginning.
*/
void rf_rx_fast_settle_update_cal_val(rf_rx_fast_settle_time_e rx_settle_time, unsigned char chn)
{
    rf_fast_settle_t fs_cv;
    rf_rx_fast_settle_get_cal_val(rx_settle_time, chn, &fs_cv);
    rf_rx_fast_settle_set_cal_val(rx_settle_time, chn, &fs_cv);
}

/**
 *  @brief      This function is used to get the tx fast_settle calibration value.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to TX_SETTLE_TIME_23US and TX_SETTLE_TIME_59US, other parameters are invalid.
 *                              (When tx_settle_us is 23us or 59us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @param[in]  fs_cv           Fast settle calibration value address pointer.
 *  @return     none
 *  @note       Calibration values must be obtained with fast settle mode disabled.
                TX_SETTLE_TIME_23US  - disable Bandgap,tx_ldo_trim,PD_settle,tx_fcal,tx_hpmc,reduce 87.5us of tx settle time.
                                       After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_59US  - disable tx_ldo_trim function and tx_hpmc,reduce 61.5us of tx settle time.After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_112US - disable tx_ldo_trim function,reduce 8.5us of tx settle time. Do a normal calibration at the beginning.
*/

void rf_tx_fast_settle_get_cal_val(rf_tx_fast_settle_time_e tx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    unsigned short rf_fcal_range[8] = {2400, 2410, 2420, 2430, 2440, 2450, 2460, 2470};
    if (tx_settle_time == TX_SETTLE_TIME_23US) {
        if (chn <= 80) {
            fs_cv->cal_tbl[chn] = rf_get_hpmc_cal_val();
            if (chn % 10 == 4) {
                rf_set_fcal_chn_group_range_ctf(rf_fcal_range);
                fs_cv->tx_fcal[chn / 10] = rf_get_fcal_cal_val();
            }
        }
    } else if (tx_settle_time == TX_SETTLE_TIME_59US) {
        if (chn <= 80) {
            fs_cv->cal_tbl[chn] = rf_get_hpmc_cal_val();
        }
    }
    rf_get_ldo_trim_val(&(fs_cv->ldo_trim));
}

/**
 *  @brief      This function is used to set the tx fast_settle calibration value.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to TX_SETTLE_TIME_23US and TX_SETTLE_TIME_59US, other parameters are invalid.
 *                              (When tx_settle_us is 23us or 59us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @param[in]  fs_cv           Fast settle calibration value address pointer.
 *  @return     none
 *  @note       TX_SETTLE_TIME_23US  - disable Bandgap,tx_ldo_trim,PD_settle,tx_fcal,tx_hpmc,reduce 87.5us of tx settle time.
                                       After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_59US  - disable tx_ldo_trim function and tx_hpmc,reduce 61.5us of tx settle time.After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_112US - disable tx_ldo_trim function,reduce 8.5us of tx settle time. Do a normal calibration at the beginning.
*/
_attribute_ram_code_
void rf_tx_fast_settle_set_cal_val(rf_tx_fast_settle_time_e tx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    unsigned short rf_fcal_range[8] = {2400, 2410, 2420, 2430, 2440, 2450, 2460, 2470};
    if (tx_settle_time == TX_SETTLE_TIME_23US) {
        if (chn <= 80) {
            if (chn % 10 == 4) {
                rf_set_fcal_chn_group_range_ctf(rf_fcal_range);
                reg_rf_fcal_ctrl_tx(chn / 10) = fs_cv->tx_fcal[chn / 10];
            }
        }
    }
    rf_set_ldo_trim_val(fs_cv->ldo_trim);
}

/**
 *  @brief      This function is used to get the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to RX_SETTLE_TIME_15US, other parameters are invalid.
 *                              (When rx_settle_us is 15us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @param[in]  fs_cv           Fast settle calibration value address pointer.
 *  @return     none
 *  @note       RX_SETTLE_TIME_15US  - disable Bandgap, rx_ldo_trim, PD_settle, rx_fcal,rx_rccal, rx_dcoc calibration,reduce 74us of rx settle time.
                                       Receive for a period of time and then do a normal calibration
 *              RX_SETTLE_TIME_37US  - disable rx_ldo_trim and rx_dcoc calibration,reduce 48.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_77US  - disable rx_ldo_trim calibration,reduce 8.5us of rx settle time. Do a normal calibration at the beginning.
*/

void rf_rx_fast_settle_get_cal_val(rf_rx_fast_settle_time_e rx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    unsigned short rf_fcal_range[8] = {2400, 2410, 2420, 2430, 2440, 2450, 2460, 2470};
    if (rx_settle_time == RX_SETTLE_TIME_15US) {
        if (chn <= 80) {
            if (chn % 10 == 4) {
                rf_set_fcal_chn_group_range_ctf(rf_fcal_range);
                fs_cv->rx_fcal[chn / 10] = rf_get_fcal_cal_val();
            }
        }
#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
            rf_get_dcoc_cal_val(&(fs_cv->dcoc_cal));
#endif

        rf_get_rccal_cal_val(&(fs_cv->rccal_cal));
    } else if (rx_settle_time == RX_SETTLE_TIME_37US) {
#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
            rf_get_dcoc_cal_val(&(fs_cv->dcoc_cal));
#endif
    }
    rf_get_ldo_trim_val(&(fs_cv->ldo_trim));
}

/**
 *  @brief      This function is used to set the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Applies to RX_SETTLE_TIME_15US, other parameters are invalid.
 *                              (When rx_settle_us is 15us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @param[in]  fs_cv           Fast settle calibration value address pointer.
 *  @return     none
 *  @note       RX_SETTLE_TIME_15US  - disable Bandgap, rx_ldo_trim, PD_settle, rx_fcal,rx_rccal, rx_dcoc calibration,reduce 74us of rx settle time.
                                       Receive for a period of time and then do a normal calibration
 *              RX_SETTLE_TIME_37US  - disable rx_ldo_trim and rx_dcoc calibration,reduce 48.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_77US  - disable rx_ldo_trim calibration,reduce 8.5us of rx settle time. Do a normal calibration at the beginning.
*/
_attribute_ram_code_
void rf_rx_fast_settle_set_cal_val(rf_rx_fast_settle_time_e rx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    unsigned short rf_fcal_range[8] = {2400, 2410, 2420, 2430, 2440, 2450, 2460, 2470};
    if (rx_settle_time == RX_SETTLE_TIME_15US) {
        if (chn <= 80) {
            if (chn % 10 == 4) {
                rf_set_fcal_chn_group_range_ctf(rf_fcal_range);
                reg_rf_fcal_ctrl_rx(chn / 10) = fs_cv->rx_fcal[chn / 10];
            }
        }
#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
            rf_set_dcoc_cal_val(fs_cv->dcoc_cal);
#endif
        rf_set_rccal_cal_val(fs_cv->rccal_cal);
    } else if (rx_settle_time == RX_SETTLE_TIME_37US) {
#if(!RF_RX_DCOC_SOFTWARE_CAL_EN)
            rf_set_dcoc_cal_val(fs_cv->dcoc_cal);
#endif
    }
    rf_set_ldo_trim_val(fs_cv->ldo_trim);
}

/**
 * @brief      This function serves to reset RF digital logic states.
 * @return     none
 * @note       This function requires setting reset zb, rstl_bb, and rst_mdm.
 *             It is used to clear RF related state machines, IRQ states, and digital internal logic states.
 */
_attribute_ram_code_sec_noinline_ void rf_clr_dig_logic_state(void)
{
    reg_n22_rst &= ~((FLD_RST0_ZB) | ((FLD_RST1_RSTL_BB | FLD_RST1_RST_MDM) << 8));
    reg_n22_rst |= ((FLD_RST0_ZB) | ((FLD_RST1_RSTL_BB | FLD_RST1_RST_MDM) << 8));
}

/**
 * @brief      This function is used to restore the rf related registers to their default values.
 * @return     none
 * @note       (1)After calling this interface, all configured interfaces of rf need to be called again.
 *             (2)After calling this interface, the tick of bb timer will be reset to zero.
 *             (3)After calling this interface, RF DMA configurations need to be reconfigured.
 */
_attribute_ram_code_sec_noinline_ void rf_reset_register_value(void)
{
    reg_n22_rst0 &= ~FLD_RST0_ZB_PON;
    reg_n22_rst0 |= FLD_RST0_ZB_PON;
}

/**
 * @brief          This function is mainly used to set the energy when sending a single carrier.
 * @param[in]    level        - The slice corresponding to the energy value.
 * @return         none.
 */
void rf_set_power_level_singletone(rf_power_level_e level)
{
    unsigned char value = 0;

    if (level & BIT(7)) {
        reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE; // VANT
    } else {
        reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;
    }
    value = (unsigned char)level & 0x3f;
    reg_rf_lnm_pa_ow_ctrl_val |= BIT(6);                           // TX_PA_PWR_OW  BIT6 set 1
    reg_rf_pa_ow_val = ((reg_rf_pa_ow_val & 0x81) | (value << 1)); // TX_PA_PWR  BIT1 t0 BIT6 set value
}
