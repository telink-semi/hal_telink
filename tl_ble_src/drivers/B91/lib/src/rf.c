/********************************************************************************************************
 * @file    rf.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/rf.h"
#include "compiler.h"
#include "dma.h"
#include "clock.h"
#include "core.h"
#include "ext_driver/driver_internal/ext_rf.h" //add by BLE




/**********************************************************************************************************************
 *                                         RF global constants                                                        *
 *********************************************************************************************************************/
/**
 * @brief The table of rf power level.
 */
const rf_power_level_e rf_power_Level_list[30] =
    {
        /*VBAT*/
        RF_POWER_P9p11dBm,
        RF_POWER_P8p57dBm,
        RF_POWER_P8p05dBm,
        RF_POWER_P7p45dBm,
        RF_POWER_P6p98dBm,
        RF_POWER_P5p68dBm,
        /*VANT*/
        RF_POWER_P4p35dBm,
        RF_POWER_P3p83dBm,
        RF_POWER_P3p25dBm,
        RF_POWER_P2p79dBm,
        RF_POWER_P2p32dBm,
        RF_POWER_P1p72dBm,
        RF_POWER_P0p80dBm,
        RF_POWER_P0p01dBm,
        RF_POWER_N0p53dBm,
        RF_POWER_N1p37dBm,
        RF_POWER_N2p01dBm,
        RF_POWER_N3p37dBm,
        RF_POWER_N4p77dBm,
        RF_POWER_N6p54dBm,
        RF_POWER_N8p78dBm,
        RF_POWER_N12p06dBm,
        RF_POWER_N17p83dBm,
        RF_POWER_N23p54dBm,
};


static rf_status_e  s_rf_trxstate = RF_MODE_TX;
rf_mode_e           g_rfmode;
static unsigned int g_iq_data_len, g_sample_interval;

_attribute_data_retention_sec_ rf_fast_settle_t               *g_fast_settle_cal_val_ptr;
_attribute_data_retention_sec_ static unsigned char            g_rf_tx_fast_settle_chn_cal_flag = 0;
_attribute_data_retention_sec_ static rf_tx_fast_settle_time_e g_rf_tx_fast_settle_time         = TX_FAST_SETTLE_NONE;
_attribute_data_retention_sec_ static rf_rx_fast_settle_time_e g_rf_rx_fast_settle_time         = RX_FAST_SETTLE_NONE;
_attribute_data_retention_sec_ static unsigned char            s_dcoc_software_cal_en           = 1;
_attribute_data_retention_sec_ unsigned short                  g_rf_dcoc_iq_code                = 0;

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

/**********************************************************************************************************************
 *                                         global function implementation                                             *
 *********************************************************************************************************************/
#if (SW_DCOC_EN)
/**
 * @brief        This function is used to set whether or not to use the rx DCOC software calibration in rf_mode_init();
 * @param[in]     en:This value is used to set whether or not rx DCOC software calibration is performed.
 *                -#1:enable the DCOC software calibration;
 *                -#0:disable the DCOC software calibration;
 * @return         none.
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
 * @param[in]    calib_code - Value of iq_code after calibration.(The code is a combination value,you need to fill in the combined iq value)
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
 * @param[in]     iq_code:Value of iq_code after calibration.(The code is a combination value,you need to fill in the combined iq value)
 *                 <0> is used to control the switch of bypass dcoc calibration iq code, the value should be 1;
 *                 <6-1>:the value of I code, the range of value is 1~62;
 *                 <12-7>:the value of Q code, the range of value is 1~62.
 * @return         none.
 * @note
 */
_attribute_ram_code_sec_ void rf_set_dcoc_iq_code(unsigned short iq_code)
{
    write_reg16(0x140ed0, iq_code); //When writing iq values, you need to wait for the iq value to stabilise before enabling it.
    write_reg8(0x140ed0, read_reg8(0x140ed0) | BIT(0));
}

/**
 * @brief        This function is mainly used to set the overwrite value of iq offset and bypass dcoc calibration iq offset.
 * @param[in]     iq_offset:Value of iq_offset after calibration.(The code is a combination value,you need to fill in the combined iq offset value)
 *                 <0> is used to control the switch of bypass dcoc calibration iq offset, the value should be 1;
 *                 <7-1>:the value of I offset, the range of value is -64~63;
 *                 <14-8>:the value of Q offset, the range of value is -64~63.
 * @return         none.
 */
_attribute_ram_code_sec_ void rf_set_dcoc_iq_offset(signed short iq_offset)
{
    write_reg16(0x140ece, iq_offset); //When writing iq offset values, you need to wait for the iq offset value to stabilise before enabling it.
    write_reg8(0x140ece, read_reg8(0x140ece) | BIT(0));
}

/**
 * @brief        This function is mainly used to get the ADC IQ data for selection of optimum DCOC_IQ_CODE.
 * @param[out]  val    -    Address for storing ADC IQ.
 * @return        none.
 */
_attribute_ram_code_sec_ void rf_rd_iq_val(short *val)
{
    short temp_dat_i = 0;
    short temp_dat_q = 0;
    int   temp_dat_iq;
    short lim = 1024;                   //11bits ADC IQ value.1024: the threshold for signed or unsigned

    temp_dat_iq = read_reg32(0x140e6c); // ADC_I :11bits signed.ADC_Q :11bits signed.
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
 * @brief         This function is mainly used to bypass the ANA_DCOC_DAC_CODE function, and also overwrite dcoc_iq_code.
 * @param[in]     i_code    - I data that needs to be written to the ADC IQ value.Values range from 0 to 63
 * @param[in]     q_code    - Q data that needs to be written to the ADC IQ value.Values range from 0 to 63
 * @return         none.
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
                                                                //(Modified by chenxi,confirmed by xuqiang&yuya at 20231128.)
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
                                                                //(Modified by chenxi,confirmed by xuqiang&yuya at 20231128.)
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
                                                                    //(Modified by chenxi,confirmed by xuqiang&yuya at 20231128.)
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
 * @brief        This function is mainly used for dcoc calibration by the software.
 * @return       none.
 */
_attribute_ram_code_sec_noinline_ void rf_rx_dcoc_cali_by_sw(void)
{
    q_final.total           = 0xffffffff;
    unsigned char hw_dcoc_i = 0;
    unsigned char hw_dcoc_q = 0;

    write_reg8(0x140c4d, 0x00);             //rx_sync_chnl disable ,never get synced
                                            //<5:0>rx_chn_en        default:0x1,->0 rx_sync_chnl all closed
    write_reg8(0x140c20, 0xcc);             //MODEM_MODE_CFG_RX1_0 0xc4->0xcc
                                            //<3>:cont_mode,    default:0,->1 Enable continue mode.
    rf_set_rxmode();
    core_cclk_delay_tick((unsigned long long)sys_clk.cclk * 95);
    hw_dcoc_i = read_reg8(0x140ed8) & 0x3f; //DCOC_IDAC 0xd8[5:0]
    hw_dcoc_q = read_reg8(0x140eda) & 0x3f; //DCOC_QDAC 0xda[5:0]

    write_reg8(0x140f78, 0x0f);             //lnm_pa_ow_ctrl_val 0x00->0x0f
                                            //<0>:rx_lna_pup_ow      default:0,->1 lna overwrite en open, LNA-OFF
                                            //<1>:rx_lna_hgain_ow    default:0,->1 LNA-high-gain-overwrite en open
                                            //<2>:rx_lna_lgain_ow    default:0,->1 LNA-low-gain-overwrite en open
                                            //<3>:rx_lna_attn_ow     default:0,->1 LNA-capacitvie-attenuation-overwrite en open
                                            //LNA-high-gain path, number of slices
                                            //LNA-low-gain path,  number of slices and capacitive attenuation
    write_reg8(0x140f7a, 0x00);             //<0>:rx_lna_pup         default:0
                                            //<6:1>:rx_lna_hgain     default:0x3f, -> 0 LNA-high-gain-overwrite-slices 0
                                            //<7>:rx_lna_lgain       default:0
                                            //7b<0>:rx_lna_hgain     default:0
                                            //7b<2:1>:rx_lna_attn    default:0
    write_reg8(0x140e40, 0x16);             //RADIO_TXRX_DBG1_0  0x14->0x16  read ADC_IQ data only once
                                            //<2>:agc_diasable,      default:0,->1 Turn off the agc auto-adjustment function.
    rf_adc_iq                = rf_get_adc_iq();
    unsigned int avg_hw_i_sq = rf_adc_iq.adc_i * rf_adc_iq.adc_i;
    unsigned int avg_hw_q_sq = rf_adc_iq.adc_q * rf_adc_iq.adc_q;
    unsigned int hw_total    = avg_hw_i_sq + avg_hw_q_sq;

    rf_set_dcoc_iq_offset(0x0001);
    rf_dcoc_iq_search(Q_FIX_FIRST, &q_first);
    rf_dcoc_iq_search(I_FIX_FIRST, &i_first);
    unsigned int boundary_value = 8192; //boundary_value = 64*64+64*64;
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
    write_reg8(0x140c20, 0xc4); //MODEM_MODE_CFG_RX1_0    0xcc->0xc4
                                //<3>:cont_mode,          1,->0 disable continue mode.
    write_reg8(0x140f78, 0x00); //lnm_pa_ow_ctrl_val      0x01->0x00
                                //<0>:rx_lna_pup_ow       1,->0 release lna_overwrite_en
    write_reg8(0x140e40, 0x14); //RADIO_TXRX_DBG1_0       0x16->0x14
                                //<2>:agc_diasable,       1,->0 Turn on the agc auto-adjustment function.
    write_reg8(0x140f7a, 0x7e); //lna ow val  release lna_overwrite
    write_reg8(0x140c4d, 0x01); //rx_sync_chnl open ,can receive packet
}

//BLE SDK use for calling in ext_rf
_attribute_ram_code_sec_ void rf_sw_dcoc_cal(void)
{
    if(s_dcoc_software_cal_en == 1)
        {
            //Solve the problem of unstable rx sensitivity test of some chips by software dcoc calibration scheme. If the calibration value is
            //not lost after a calibration is completed, it can be used directly without recalibration. Since the _attribute_data_retention_sec_ type
            //variable is not lost in suspend and deep retention modes, it can be used to record the calibration value to avoid having to perform
            //software calibration again after returning from suspend and deep retention modes.(Modified by chenxi,confirmed by yuya at 20240407.)
            if(g_rf_dcoc_iq_code == 0)    //After calibration is completed, it is impossible for the value of g_rf_dcoc_iq_code to be 0.
            {
                rf_rx_dcoc_cali_by_sw();
            }
            else
            {
                rf_set_dcoc_iq_offset(0x0001);
                rf_set_dcoc_iq_code(g_rf_dcoc_iq_code);
            }
        }
}
#endif


/**
 * @brief      This function serves to initiate information of RF.
 * @return       none.
 * @note          Attention:
 *                 In order to solve the problem of poor receiver sensitivity performance of some chips with large DC offset:
 *                 1.Added DCOC software calibration scheme to the rf_mode_init() interface to get the smallest DC-offset for the chip.
 *                 2.Turn on the RX secondary filter in BLE S2 S8 modes to filter out DC offset and noise as much as possible,
 *                   in order to improve the chip's out of band anti-interference ability (including DC offset).
 *                But there are two things to note:
 *                (1)Using DCOC software calibration will increase the software execution time of rf_mode_init().
 *                (2)After turning on the RX secondary filter, the anti frequency offset range of the chip will be reduced to within ± 150kHz.
 */
void rf_mode_init(void)
{
    if (s_dcoc_software_cal_en == 1) {
        //Solve the problem of unstable rx sensitivity test of some chips by software dcoc calibration scheme. If the calibration value is
        //not lost after a calibration is completed, it can be used directly without recalibration. Since the _attribute_data_retention_sec_ type
        //variable is not lost in suspend and deep retention modes, it can be used to record the calibration value to avoid having to perform
        //software calibration again after returning from suspend and deep retention modes.(Modified by chenxi,confirmed by yuya at 20240407.)
        if (g_rf_dcoc_iq_code == 0) //After calibration is completed, it is impossible for the value of g_rf_dcoc_iq_code to be 0.
        {
            rf_rx_dcoc_cali_by_sw();
        } else {
            rf_set_dcoc_iq_offset(0x0001);
            rf_set_dcoc_iq_code(g_rf_dcoc_iq_code);
        }
    }

    write_reg8(0x140ed2, 0x9b); //DCOC_SFIIP DCOC_SFQQP
    write_reg8(0x140ed3, 0x19); //DCOC_SFQQ
#if RF_RX_SHORT_MODE_EN
    write_reg8(0x140c7b, 0x0e); //BLANK_WINDOW
    write_reg8(0x140c79, 0x38); //BIT[3] RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix
                                //per floor issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#else
    write_reg8(0x140c7b, 0xfe); //BLANK_WINDOW
    write_reg8(0x140c79, 0x08); //RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix per floor
                                //issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#endif

    write_reg8(0x140e4a, 0x0e);    //POW_000_001
    write_reg8(0x140e4b, 0x09);    //POW_001_010_H
    write_reg8(0x140e4e, 0x09);    //POW_100_101 //POW_101_100_L
    write_reg8(0x140e4f, 0x0f);    //POW_101_100_H
    write_reg8(0x140e54, 0x0e);    //POW_001_010_L
    write_reg8(0x140e55, 0x09);    //POW_001_010_H
    write_reg8(0x140e56, 0x0c);    //POW_011_100_L
    write_reg8(0x140e57, 0x08);    //POW_011_100_H
    write_reg8(0x140e58, 0x09);    //POW_101_100_L
    write_reg8(0x140e59, 0x0f);    //POW_101_100_H

    write_reg8(0x140c76, 0x50);    //FREQ_CORR_CFG2_0
    write_reg8(0x140c77, 0x73);    //FREQ_CORR_CFG2_1
#if RF_RX_SHORT_MODE_EN
    write_reg8(0x14083a, 0x86);    //rx_ant_offset  rx_dly(0x140c7b,0x140c79,0x14083a,0x14083b)
    write_reg8(0x14083b, 0x65);    //samp_offset
#endif
    analog_write_reg8(0x8b, 0x04); //FREQ_CORR_CFG2_1
}

/**
 * @brief     This function serves to  set ble_1M  mode of RF.
 * @return    none.
 */
void rf_set_ble_1M_mode(void)
{
    write_reg8(0x140e3d, 0x61); //ble:bw_code.
    write_reg8(0x140e20, 0x16); //sc_code.
    write_reg8(0x140e21, 0x0a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e); //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01); //TOT_DEV_RST.

    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x39); //grx_0.
    write_reg8(0x140cc3, 0x4b); //grx_1.
    write_reg8(0x140cc4, 0x56); //grx_2.
    write_reg8(0x140cc5, 0x62); //grx_3.
    write_reg8(0x140cc6, 0x6e); //grx_4.
    write_reg8(0x140cc7, 0x79); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode.
    write_reg8(0x140801, 0x08); //PN.
    write_reg8(0x140802, 0x46); //preamble len 0x46 for ble confirmed by biao.li.20200828.
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xf5); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif


    g_rfmode = RF_MODE_BLE_1M;
}

/**
 * @brief     This function serves to  set ble_1M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_1M_NO_PN_mode(void)
{
    write_reg8(0x140e3d, 0x61); //ble:bw_code.
    write_reg8(0x140e20, 0x16); //sc_code.
    write_reg8(0x140e21, 0x0a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e); //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01); //TOT_DEV_RST.

    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x39); //grx_0.
    write_reg8(0x140cc3, 0x4b); //grx_1.
    write_reg8(0x140cc4, 0x56); //grx_2.
    write_reg8(0x140cc5, 0x62); //grx_3.
    write_reg8(0x140cc6, 0x6e); //grx_4.
    write_reg8(0x140cc7, 0x79); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode.
    write_reg8(0x140801, 0x00); //PN.
    write_reg8(0x140802, 0x42); //preamble len 0x46 for ble confirmed by biao.li.20200828.
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xf5); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //RX:acc_len modem.

#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_BLE_1M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_2M  mode of RF.
 * @return    none.
 */
void rf_set_ble_2M_mode(void)
{
    write_reg8(0x140e3d, 0x41); //ble:bw_code.
    write_reg8(0x140e20, 0x06); //sc_code.
    write_reg8(0x140e21, 0x2a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e); //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01); //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x3b); //grx_0.
    write_reg8(0x140cc3, 0x4c); //grx_1.
    write_reg8(0x140cc4, 0x58); //grx_2.
    write_reg8(0x140cc5, 0x64); //grx_3.
    write_reg8(0x140cc6, 0x70); //grx_4.
    write_reg8(0x140cc7, 0x7a); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode.
    write_reg8(0x140801, 0x08); //PN.
    write_reg8(0x140802, 0x46); //preamble len 0x46 for ble confirmed by biao.li.20200828.
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xe5); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_BLE_2M;
}

/**
 * @brief     This function serves to  set ble_2M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_2M_NO_PN_mode(void)
{
    write_reg8(0x140e3d, 0x41); //ble:bw_code.
    write_reg8(0x140e20, 0x06); //sc_code.
    write_reg8(0x140e21, 0x2a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e); //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01); //TOT_DEV_RST.

    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x3b); //grx_0.
    write_reg8(0x140cc3, 0x4c); //grx_1.
    write_reg8(0x140cc4, 0x58); //grx_2.
    write_reg8(0x140cc5, 0x64); //grx_3.
    write_reg8(0x140cc6, 0x70); //grx_4.
    write_reg8(0x140cc7, 0x7a); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode.
    write_reg8(0x140801, 0x00); //PN.
    write_reg8(0x140802, 0x43); //preamble len.
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xe5); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //RX:acc_len modem.

#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif
    g_rfmode = RF_MODE_BLE_2M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_500K  mode of RF.
 * @return    none.
 */
void rf_set_ble_500K_mode(void)
{
    write_reg8(0x140e3d, 0x61); //ble:bw_code.
    write_reg8(0x140e20, 0x16); //sc_code.
    write_reg8(0x140e21, 0x0a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8d); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0xf0); //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xee); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0c); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc8); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x7d); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x21); //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x36); //grx_0.
    write_reg8(0x140cc3, 0x48); //grx_1.
    write_reg8(0x140cc4, 0x54); //grx_2.
    write_reg8(0x140cc5, 0x62); //grx_3.
    write_reg8(0x140cc6, 0x6e); //grx_4.
    write_reg8(0x140cc7, 0x79); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode.
    write_reg8(0x140801, 0x08); //PN.
    write_reg8(0x140802, 0x4a); //preamble len.
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xf5); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0xa4); //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //But this will lead to a narrowing of the RX packet receiving bandwidth and a decrease in frequency offset performance.(Modified by chenxi,confirmed by yuya at 20240407)
    write_reg8(0x140e7a, read_reg8(0x140e7a) & 0xdf); //bit<5>:BYPASS_RRC_BLE    default 1,->0 Turn on the secondary filter
#endif

    g_rfmode = RF_MODE_LR_S2_500K;
}

/**
 * @brief     This function serves to  set zigbee_125K  mode of RF.
 * @return    none.
 */
void rf_set_ble_125K_mode(void)
{
    write_reg8(0x140e3d, 0x61); //ble:bw_code.
    write_reg8(0x140e20, 0x16); //sc_code.
    write_reg8(0x140e21, 0x0a); //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20); //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23); //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8d); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00); //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0xf0); //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xf6); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0c); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc8); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x7d); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x21); //TOT_DEV_RST.

    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x36); //grx_0.
    write_reg8(0x140cc3, 0x48); //grx_1.
    write_reg8(0x140cc4, 0x54); //grx_2.
    write_reg8(0x140cc5, 0x62); //grx_3.
    write_reg8(0x140cc6, 0x6e); //grx_4.
    write_reg8(0x140cc7, 0x79); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode.
    write_reg8(0x140801, 0x08); //PN.
    write_reg8(0x140802, 0x4a); //preamble len.
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xf5); //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0xb4); //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //RX:acc_len modem.

#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //But this will lead to a narrowing of the RX packet receiving bandwidth and a decrease in frequency offset performance.(Modified by chenxi,confirmed by yuya at 20240407)
    write_reg8(0x140e7a, read_reg8(0x140e7a) & 0xdf); //bit<5>:BYPASS_RRC_BLE    default 1,->0 Turn on the secondary filter
#endif
    g_rfmode = RF_MODE_LR_S8_125K;
}

/**
 * @brief     This function serves to  set zigbee_250K  mode of RF.
 * @return    none.
 */
void rf_set_zigbee_250K_mode(void)
{
    write_reg8(0x140e3d, 0x41);        //ble:bw_code.
    write_reg8(0x140e20, 0x06);        //sc_code.
    write_reg8(0x140e21, 0x2a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x0f);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x01);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x80);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x02);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x01);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x39);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.

    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x13);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x42);        //preamble len.
    write_reg8(0x140803, 0x44);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0x000000a7); //access code for zigbee 250K.
    write_reg32(0x140810, 0x000000d1); //access code for hybee 1m.
    write_reg8(0x140818, 0x95);        //access code for hybee 2m.
    write_reg8(0x140819, 0x0c);        //access code for hybee 500K.

    write_reg8(0x140821, 0x23);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE   default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_ZIGBEE_250K;
}

/**
 * @brief     This function serves to  set pri_250K  mode of RF.
 * @return    none.
 */
void rf_set_pri_250K_mode(void)
{
    write_reg8(0x140e3d, 0x61);        //ble:bw_code.
    write_reg8(0x140e20, 0x16);        //sc_code.
    write_reg8(0x140e21, 0x0a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x12);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x00);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x1f);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x41);        //preamble len.
    write_reg8(0x140803, 0x45);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xfb);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0x000000a7); //access code for zigbee 250K.
    write_reg32(0x140810, 0x000000d1); //access code for hybee 1m.
    write_reg8(0x140818, 0x95);        //access code for hybee 2m.
    write_reg8(0x140819, 0x0c);        //access code for hybee 500K.

    write_reg8(0x140821, 0xa1);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.

#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif
    g_rfmode = RF_MODE_PRIVATE_250K;
}

/**
 * @brief     This function serves to  set pri_500K  mode of RF.
 * @return    none.
 */
void rf_set_pri_500K_mode(void)
{
    write_reg8(0x140e3d, 0x61);        //ble:bw_code.
    write_reg8(0x140e20, 0x16);        //sc_code.
    write_reg8(0x140e21, 0x0a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x0e);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x00);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x1f);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x41);        //preamble len.
    write_reg8(0x140803, 0x47);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xfb);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0xf8118ac9); //access code for zigbee 250K.
    write_reg32(0x140810, 0xd3f03577); //access code for hybee 1m.
    write_reg8(0x140818, 0x03);        //access code for hybee 2m.
    write_reg8(0x140819, 0x0c);        //access code for hybee 500K.

    write_reg8(0x140821, 0xa1);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.

#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_PRIVATE_500K;
}

/**
 * @brief     This function serves to  set pri_1M  mode of RF.
 * @return    none.
 */
void rf_set_pri_1M_mode(void)
{
    write_reg8(0x140e3d, 0x61);        //ble:bw_code.
    write_reg8(0x140e20, 0x16);        //sc_code.
    write_reg8(0x140e21, 0x0a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x20);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x23);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x00);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x00);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x1f);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x42);        //preamble len.
    write_reg8(0x140803, 0x44);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xfa);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0xf8118ac9); //access code for zigbee 250K.
    write_reg32(0x140810, 0xd3f03577); //access code for hybee 1m.
    write_reg8(0x140818, 0x03);        //access code for hybee 2m.
    write_reg8(0x140819, 0x0c);        //access code for hybee 500K.

    write_reg8(0x140821, 0xa1);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_PRIVATE_1M;
}

/**
 * @brief     This function serves to  set pri_2M  mode of RF.
 * @return    none.
 */
void rf_set_pri_2M_mode(void)
{
    write_reg8(0x140e3d, 0x41);        //ble:bw_code.
    write_reg8(0x140e20, 0x06);        //sc_code.
    write_reg8(0x140e21, 0x2a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x1e);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x01);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x00);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.

    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x1f);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x43);        //preamble len.
    write_reg8(0x140803, 0x44);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xea);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.
    write_reg32(0x140808, 0xf8118ac9); //access code for zigbee 250K.
    write_reg32(0x140810, 0xd3f03577); //access code for hybee 1m.
    write_reg8(0x140818, 0x03);        //access code for hybee 2m.
    write_reg8(0x140819, 0x0c);        //access code for hybee 500K.

    write_reg8(0x140821, 0xa1);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif


    g_rfmode = RF_MODE_PRIVATE_2M;
}

/**
 * @brief     This function serves to  set hybee_500K  mode of RF.
 * @return    none.
 */
void rf_set_hybee_500K_mode(void)
{
    write_reg8(0x140e3d, 0x41);        //ble:bw_code.
    write_reg8(0x140e20, 0x06);        //sc_code.
    write_reg8(0x140e21, 0x2a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x0f);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x01);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x80);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x02);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x01);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x39);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x13);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x42);        //preamble len.
    write_reg8(0x140803, 0x54);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0x000000a7); //access code for zigbee 250K.
    write_reg32(0x140810, 0x000000d1); //access code for hybee 1m.
    write_reg8(0x140818, 0x95);        //access code for hybee 2m.
    write_reg8(0x140819, 0x2f);        //access code for hybee 500K.

    write_reg8(0x140821, 0x23);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.

#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_HYBEE_500K;
}

/**
 * @brief     This function serves to  set hybee_1M  mode of RF.
 * @return    none.
 */
void rf_set_hybee_1M_mode(void)
{
    write_reg8(0x140e3d, 0x41);        //ble:bw_code.
    write_reg8(0x140e20, 0x06);        //sc_code.
    write_reg8(0x140e21, 0x2a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x0f);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x01);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x80);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x02);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x01);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x39);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x17);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x42);        //preamble len.
    write_reg8(0x140803, 0x44);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0x000000a7); //access code for zigbee 250K.
    write_reg32(0x140810, 0x000000d1); //access code for hybee 1m.
    write_reg8(0x140818, 0x95);        //access code for hybee 2m.
    write_reg8(0x140819, 0x2f);        //access code for hybee 500K.

    write_reg8(0x140821, 0x23);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_HYBEE_1M;
}

/**
 * @brief     This function serves to  set hybee_2M  mode of RF.
 * @return    none.
 */
void rf_set_hybee_2M_mode(void)
{
    write_reg8(0x140e3d, 0x41);        //ble:bw_code.
    write_reg8(0x140e20, 0x06);        //sc_code.
    write_reg8(0x140e21, 0x2a);        //if_freq,IF = 1Mhz,BW = 1Mhz.
    write_reg8(0x140e22, 0x43);        //HPMC_EXP_DIFF_COUNT_L.
    write_reg8(0x140e23, 0x26);        //HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x140e3f, 0x00);        //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00);        //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c);        // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                       //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                       //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
    write_reg8(0x140c22, 0x01);        //modem:BLE_MODE_TX,2MBPS.
    write_reg8(0x140c4e, 0x18);        //ble sync threshold:To modem.
    write_reg8(0x140c4d, 0x0f);        //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x01);        //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x80);        //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x02);        //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10);        //modem:disable MSK.
    write_reg8(0x140c3d, 0x01);        //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x39);        //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7);        //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e);        //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4);        //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71);        //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01);        //TOT_DEV_RST.


    write_reg8(0x140c9a, 0x00);        //tx_tp_align.
    write_reg8(0x140cc2, 0x36);        //grx_0.
    write_reg8(0x140cc3, 0x48);        //grx_1.
    write_reg8(0x140cc4, 0x54);        //grx_2.
    write_reg8(0x140cc5, 0x62);        //grx_3.
    write_reg8(0x140cc6, 0x6e);        //grx_4.
    write_reg8(0x140cc7, 0x79);        //grx_5.

    write_reg8(0x140800, 0x1b);        //tx_mode.
    write_reg8(0x140801, 0x00);        //PN.
    write_reg8(0x140802, 0x42);        //preamble len.
    write_reg8(0x140803, 0x44);        //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xe0);        //bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x140805, 0x04);        //lr mode bit<4:5> 0:off,3:125k,2:500k.

    write_reg32(0x140808, 0x000000a7); //access code for zigbee 250K.
    write_reg32(0x140810, 0x000000d1); //access code for hybee 1m.
    write_reg8(0x140818, 0x95);        //access code for hybee 1m.
    write_reg8(0x140819, 0x2f);        //access code for hybee 500K.

    write_reg8(0x140821, 0x23);        //rx packet len 0 enable.
    write_reg8(0x140822, 0x00);        //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c);        //RX:acc_len modem.
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_HYBEE_2M;
}

void rf_set_ant_mode(void)
{
    write_reg8(0x140e3d, 0x61); //ble:bw_code
    write_reg8(0x140e20, 0x16); //sc_code
    write_reg8(0x140e21, 0x0a); //if_freq,IF = 1Mhz,BW = 1Mhz
    write_reg8(0x140e22, 0x20); //HPMC_EXP_DIFF_COUNT_L
    write_reg8(0x140e23, 0x23); //HPMC_EXP_DIFF_COUNT_H

    write_reg8(0x140e3f, 0x00); //250k modulation index:telink add rx for 250k/500k.
    write_reg8(0x140c3f, 0x00); //LOW_RATE_EN bit<1>:1 enable bit<2>:0 250k.
    write_reg8(0x140c20, 0x8c); // script cc disable long rang trigger.BIT[3]continue mode.After syncing to the preamble,
                                //it will immediately enter the sync state again, reducing the probability of mis-syncing.
                                //modified by zhiwei,confirmed by qiangkai and xuqiang.20221205

    write_reg8(0x140c22, 0x00); //modem:BLE_MODE_TX,1MBPS
    write_reg8(0x140c4e, 0x20); //sync threshold:TO MODEM  access_code threshold

    write_reg8(0x140c4d, 0x01); //r_rxchn_en_i:To modem.
    write_reg8(0x140c21, 0x00); //modem:ZIGBEE_MODE:01.
    write_reg8(0x140c23, 0x00); //modem:ZIGBEE_MODE_TX.
    write_reg8(0x140c26, 0x00); //modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x140c2a, 0x10); //modem:disable MSK.
    write_reg8(0x140c3d, 0x00); //modem:zb_sfd_frm_ll.
    write_reg8(0x140c2c, 0x38); //modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x140c36, 0xb7); //LR_NUM_GEAR_L.
    write_reg8(0x140c37, 0x0e); //LR_NUM_GEAR_H.
    write_reg8(0x140c38, 0xc4); //LR_TIM_EDGE_DEV.
    write_reg8(0x140c39, 0x71); //LR_TIM_REC_CFG_1.
    write_reg8(0x140c73, 0x01); //TOT_DEV_RST.

    //write_reg8(0x140c79,0x08);//RX_DIS_PDET_BLANK


    write_reg8(0x140c9a, 0x00); //tx_tp_align.
    write_reg8(0x140cc2, 0x36); //grx_0.
    write_reg8(0x140cc3, 0x48); //grx_1.
    write_reg8(0x140cc4, 0x54); //grx_2.
    write_reg8(0x140cc5, 0x62); //grx_3.
    write_reg8(0x140cc6, 0x6e); //grx_4.
    write_reg8(0x140cc7, 0x79); //grx_5.

    write_reg8(0x140800, 0x1f); //tx_mode
    write_reg8(0x140801, 0x00); //PN.
    write_reg8(0x140802, 0x42); //preamble length
    write_reg8(0x140803, 0x44); //bit<0:1>private mode control. bit<2:3> tx mode.
    write_reg8(0x140804, 0xfb); //bit<4>mode:1->1m;bit<0:1>:head mode;bit<2:3>:crc_mode.
    write_reg8(0x140805, 0x04); //lr mode bit<4:5> 0:off,3:125k,2:500k.bit<0:2> TX:acc_len

    write_reg8(0x140821, 0xa1); //rx packet len 0 enable.
    write_reg8(0x140822, 0x00); //rxchn_man_en.
    write_reg8(0x140c4c, 0x4c); //bit<0:2> RX:acc_len modem
#if (SW_DCOC_EN)
    //B91 only S2/S8 modes turn on the secondary filter to improve sensitivity performance.
    //Restore the secondary filter to initial state (turn off)
    write_reg8(0x140e7a, 0x20); //bit<5>:BYPASS_RRC_BLE    default 1; Turn off the secondary filter
#endif

    g_rfmode = RF_MODE_ANT;
}

/**
 * @brief      This setting serve to set the configuration of Tx DMA.
 */
_attribute_data_sec_    //BLE USED: in IRQ
dma_config_t rf_tx_dma_config = {
    .dst_req_sel    = DMA_REQ_ZB_TX,      //tx req.
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
void rf_set_tx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK); //u_pd_mcu.u_dmac.atcdmac100_ahbslv.tx_multi_en,rx_multi_en,ch_0_rnum_en_bk.
    dma_config(DMA0,(dma_config_t *)&rf_tx_dma_config);     //BLE SDK use: to mute warning
    dma_set_dst_address(DMA0, reg_rf_txdma_adr);
}

/**
 * @brief     This function serves to set RF tx DMA setting.
 * @param[in] fifo_depth        - tx chn deep,fifo_depth range: 0~5,Number of fifo=2^fifo_depth.
 * @param[in] fifo_byte_size    - The length of one dma fifo,the range is 0x10~0xff0(the corresponding number of fifo bytes is fifo_byte_size;and must be a multiple of 16).
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
 * @note       In this struct write_num_en must be 0;This setting will cause the conflict of DMA.
 */
_attribute_data_sec_    //BLE USED: in IRQ
dma_config_t rf_rx_dma_config = {
    .dst_req_sel    = 0,                  //tx req.
    .src_req_sel    = DMA_REQ_ZB_RX,
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
    .write_num_en   = 0, //must 0.
    .auto_en        = 1, //must 1.
};

/**
 * @brief       This function serve to rx dma config
 * @param[in]   none
 * @return      none
 */
void rf_set_rx_dma_config(void)
{
    reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK); //ch0_rnum_en_bk,tx_multi_en,rx_multi_en.
    dma_config(DMA1, &rf_rx_dma_config);
    dma_set_src_address(DMA1, reg_rf_rxdma_adr);
    reg_dma_size(1) = 0xffffffff;
}

/**
 * @brief      This function serves to rx dma setting.
 * @param[in]  buff - This parameter is the first address of the received data buffer, which must be 4 bytes aligned, otherwise the program will enter an exception.
 * @attention  The first four bytes in the buffer of the received data are the length of the received data.
 *             The actual buffer size that the user needs to set needs to be noted on two points:
 *             -# you need to leave 4bytes of space,the dma transfers start from the fourth byte of the Buff.
 *             -# dma is transmitted in accordance with 4bytes, so the length of the buffer needs to be a multiple of 4. Otherwise, there may be an out-of-bounds problem.
 * @param[in]  wptr_mask       - This parameter is used to set the mask value for the number of enabled FIFOs. The value of the mask must (0x00,0x01,0x03,0x07,0x0f,0x1f).
 *                               The number of FIFOs enabled is the value of wptr_mask plus 1.(0x01,0x02,0x04,0x08,0x10,0x20)
 * @param[in]  fifo_byte_size  - The length of one dma fifo,the range is 0x10~0xff0(the corresponding number of fifo bytes is fifo_byte_size;and must be a multiple of 16).
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
 * @brief       This function serves to RF trigger stx.
 * @param[in]   addr  - DMA tx buffer.
 * @param[in]   tick  - Trigger tx after tick delay.
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_stx(void *addr, unsigned int tick)
{
    dma_set_src_address(DMA0, (unsigned int)addr);
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x85;
}

/**
 * @brief     This function serves to trigger srx on.
 * @param[in] tick  - Trigger rx receive packet after tick delay.
 * @return    none.
 */
void rf_start_srx(unsigned int tick)
{
    write_reg32(0x140a28, 0x0fffffff);           // first timeout.
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    write_reg8(0x140a00, 0x86);
}

/**
 * @brief       This function serves to start Rx of auto mode. In this mode,
 *              RF module stays in Rx status until a packet is received or it fails to receive packet when timeout expires.
 *              Timeout duration is set by the parameter "tick".
 *              The address to store received data is set by the function "addr".
 * @param[in]   addr   - The address to store received data.
 * @param[in]   tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
 * @return      none
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_brx(void *addr, unsigned int tick)
{
    write_reg32(0x80140a28, 0x0fffffff);
    write_reg32(0x80140a18, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    dma_set_src_address(DMA0, (unsigned int)addr);
    write_reg8(0x80140a00, 0x82);                // ble rx.
}

/**
 * @brief       This function serves to start tx of auto mode. In this mode,
 *              RF module stays in tx status until a packet is sent or it fails to sent packet when timeout expires.
 *              Timeout duration is set by the parameter "tick".
 *              The address to store send data is set by the function "addr".
 * @param[in]   addr   - The address to store send data.
 * @param[in]   tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
 * @return      none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_btx(void *addr, unsigned int tick)
{
    write_reg32(0x80140a18, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    dma_set_src_address(DMA0, (unsigned int)addr);
    write_reg8(0x80140a00, 0x81);                // ble tx.
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
    dma_set_src_address(DMA0, (unsigned int)addr);
    write_reg32(0x80140a18, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;     // Enable cmd_schedule mode.
    write_reg8(0x80140a00, 0x87);                    // single tx2rx.
}

volatile unsigned char g_single_tong_freqoffset = 0; //for eliminate single carrier frequency offset.

/**
 * @brief       This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return      none.
 */


void rf_set_ble_chn(signed char chn_num)
{
    signed char ble_chn_num = 0;
    write_reg8(0x14080d, chn_num);
    if (chn_num < 11) {
        ble_chn_num = chn_num + 2;
    }

    else if (chn_num < 37) {
        ble_chn_num = chn_num + 3;
    }

    else if (chn_num == 37) {
        ble_chn_num = 1;
    }

    else if (chn_num == 38) {
        ble_chn_num = 13;
    }

    else if (chn_num == 39) {
        ble_chn_num = 40;
    }

    else if (chn_num < 51) {
        ble_chn_num = chn_num;
    }

    else if (chn_num <= 61) {
        ble_chn_num = -61 + chn_num;
    }

    ble_chn_num = ble_chn_num << 1;
    rf_set_chn(ble_chn_num);
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

    unsigned short tmp = read_reg16(0x140ef6);
    tmp                = (tmp & 0xf001) | hpmc_gain; //bit<1:11> 1111 0000 0000 0001
    write_reg16(0x140ef6, tmp);
}

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

    write_reg8(0x140e44, (read_reg8(0x140e44) | 0x01));
    write_reg8(0x140e44, (read_reg8(0x140e44) & 0x01) | freq_low << 1);
    write_reg8(0x140e45, (read_reg8(0x140e45) & 0xc0) | freq_high);
    write_reg8(0x140e29, (read_reg8(0x140e29) & 0x01f) | (ctrim << 5)); //FE_CTRIM
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
 * @param[in]   fifo_num   - the number of fifo set in dma.
 * @param[in]   fifo_dep   - deepth of each fifo set in dma.
 * @param[in]   addr       - address of rx packet.
 * @return      the next rx_packet address.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
unsigned char *rf_get_rx_packet_addr(int fifo_num, int fifo_dep, void *addr)
{
    unsigned char rptr;
    rptr                   = read_reg8(0x1004f5);
    unsigned char *raw_pkt = (unsigned char *)((unsigned char *)addr + (rptr & (fifo_num - 1)) * (fifo_dep));
    write_reg8(0x1004f5, 0x40);
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
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception..
 */
void rf_tx_pkt(void *addr)
{
    dma_set_src_address(DMA0, (unsigned int)addr);
    reg_dma_ctr0(0) |= 0x01;
}

/**
 * @brief       This function serves to set pri sb mode enable.
 * @return      none.
 */
void rf_private_sb_en(void)
{
    reg_rf_format |= FLD_RF_HEAD_MODE;
}

/**
 * @brief       This function serves to set pri sb mode payload length.
 * @param[in]   pay_len  - In private sb mode packet payload length.
 * @return      none.
 */
void rf_set_private_sb_len(int pay_len)
{
    reg_rf_sblen = ((reg_rf_sblen & 0x00) | pay_len);
}

/**
 * @brief       This function serves to disable pn of ble mode.
 * @return      none.
 */
void rf_pn_disable(void)
{
    reg_rf_tx_mode2 &= (~FLD_RF_V_PN_EN);
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
 * @brief       This function serves to set RF power level.
 * @param[in]   level    - The power level to set.
 * @return      none.
 */
_attribute_ram_code_sec_    //added by BLE
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



/* add by BLE:
 * 1. for future power control feature, must be RamCode
 * 2. if this function will be called in IRQ, IRQ protection is needed if user can use it in mainloop
 * 3. use "blt_extRF.txPower_index" to record user power setting
 */
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
        
        blt_extRF.txPower_level = level;                /*!< added by BLE, important */
        blt_extRF.txPower_index = (unsigned char)idx;   /*!< added by BLE, important */
    }
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

/**
 * @brief       This function serves to set pin for RFFE of RF.
 * @param[in]   tx_pin   - select pin as rffe to send.
 * @param[in]   rx_pin   - select pin as rffe to receive.
 * @return      none.
 */
void rf_set_rffe_pin(rf_pa_tx_pin_e tx_pin, rf_lna_rx_pin_e rx_pin)
{
    unsigned char val  = 0;
    unsigned char mask = 0xff;

    switch (tx_pin) {
    case RF_RFFE_TX_PB0:
        val  = BIT(0);
        mask = (unsigned char)~(BIT(1) | BIT(0));
        break;

    case RF_RFFE_TX_PB6:
        val  = 0;
        mask = (unsigned char)~(BIT(5) | BIT(4));
        reg_gpio_pad_mul_sel |= BIT(2);
        break;

    case RF_RFFE_TX_PD7:
        val  = BIT(7);
        mask = (unsigned char)~(BIT(7) | BIT(6));
        reg_gpio_pad_mul_sel |= BIT(0);
        break;

    case RF_RFFE_TX_PE5:
        val  = BIT(2);
        mask = (unsigned char)~(BIT(3) | BIT(2));
        break;

    default:
        val  = 0;
        mask = 0xff;
        break;
    }

    reg_gpio_func_mux(tx_pin) = (reg_gpio_func_mux(tx_pin) & mask) | val;

    switch (rx_pin) {
    case RF_RFFE_RX_PB1:
        val  = BIT(2);
        mask = (unsigned char)~(BIT(3) | BIT(2));
        break;

    case RF_RFFE_RX_PD6:
        val  = BIT(5);
        mask = (unsigned char)~(BIT(5) | BIT(4));
        reg_gpio_pad_mul_sel |= BIT(0);
        break;

    case RF_RFFE_RX_PE4:
        val  = BIT(0);
        mask = (unsigned char)~(BIT(1) | BIT(0));
        break;

    default:
        val  = 0;
        mask = 0xff;
        break;
    }
    reg_gpio_func_mux(rx_pin) = (reg_gpio_func_mux(rx_pin) & mask) | val;
    BM_CLR(reg_gpio_func(tx_pin), tx_pin & 0xff);
    BM_CLR(reg_gpio_func(rx_pin), rx_pin & 0xff);
}

/**
 * @brief       This function serves to close internal cap;
 * @return      none.
 */
void rf_turn_off_internal_cap(void)
{
    analog_write_reg8(0x8a, analog_read_reg8(0x8a) | 0x80);
}

/**
 * @brief       This function serves to update the value of internal cap.
 * @param[in]   value   - The value of internal cap which you want to set.
 * @return      none.
 */
void rf_update_internal_cap(unsigned char value)
{
    analog_write_reg8(0x8a, (analog_read_reg8(0x8a) & 0xc0) | (value & 0x3f));
}

/**
 * @brief       This function serves to get RF status.
 * @return      RF Rx/Tx status.
 */
rf_status_e rf_get_trx_state(void)
{
    return s_rf_trxstate;
}

/**
 * @brief   This function serve to change the length of preamble.
 * @param[in]   len     -The value of preamble length.Set the register bit<0>~bit<4>.
 * @return      none
 */
void rf_set_preamble_len(unsigned char len)
{
    len = len & 0x1f;
    write_reg8(0x140802, (read_reg8(0x140802) & 0xe0) | len);
}

/**
 * @brief   This function serve to set the private ack enable,mainly used in prx/ptx.
 * @param[in]   rf_mode     -   Must be one of the private mode.
 * @return      none
 */
void rf_set_pri_tx_ack_en(rf_mode_e rf_mode)
{
    if (rf_mode == RF_MODE_PRIVATE_1M) {
        write_reg8(0x140804, 0x9a); //1m 9a //enable  ack flag
    } else if (rf_mode == RF_MODE_PRIVATE_2M) {
        write_reg8(0x140804, 0x8a); //2m,8a
    } else if (rf_mode == RF_MODE_PRIVATE_500K || rf_mode == RF_MODE_PRIVATE_250K) {
        write_reg8(0x140804, read_reg8(0x140804) & 0xbf);
    }
}

/**
 * @brief   This function serve to set the length of access code.
 * @param[in]   byte_len    -   The value of access code length.
 * @return      none
 */
void rf_set_access_code_len(unsigned char byte_len)
{
    unsigned char temp;
    temp = byte_len & 0x07;
    write_reg8(0x140805, (read_reg8(0x140805) & 0xf8) | temp);
    write_reg8(0x140c4c, (read_reg8(0x140c4c) & 0xf8) | temp);
}

/**
 * @brief   This function serve to set access code.This function will first get the length of access code from register 0x140805
 *          and then set access code in addr.
 * @param[in]   pipe_id -The number of pipe.0<= pipe_id <=5.
 * @param[in]   acc -The value access code
 * @note        For compatibility with previous versions the access code should be bit transformed by bit_swap();
 */
void rf_set_pipe_access_code(unsigned int pipe_id, unsigned char *addr)
{
    unsigned char i = 0;

    unsigned char acc_len = read_reg8(0x140805) & 0x07;
    switch (pipe_id) {
    case 0:
    case 1:
        for (i = 0; i < acc_len; i++) {
            write_reg8(reg_rf_access_code_base_pipe0 + i + (pipe_id * 8), addr[i]);
        }
        break;
    case 2:
    case 3:
    case 4:
    case 5:
        for (i = 0; i < acc_len; i++) {
            write_reg8(reg_rf_access_code_base_pipe0 + i + 8, addr[i]);
        }
        write_reg8((reg_rf_access_code_base_pipe2 + (pipe_id - 2)), addr[0]);
        break;
    default:
        break;
    }
    //The following two lines of code are for trigger access code in S2,S8 mode.It has no effect on other modes.
    reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;
    write_reg8(0x140c25, read_reg8(0x140c25) | 0x01);
}

/**
 * @brief   This function serve to initial the ptx setting.
 * @return  none.
 */
void rf_ptx_config(void)
{
    write_reg8(0x140a02, read_reg8(0x140a02) & 0xfe); //md_en
    write_reg8(0x140a03, read_reg8(0x140a03) & 0xf7); //crc_en
    write_reg8(0x140a15, 0xd0);                       //chn tx_manual off
}

/**
 * @brief   This function serve to initial the prx setting.
 * @return  none.
 */
void rf_prx_config(void)
{
    write_reg8(0x140a03, 0x30); //rx timeout off
    write_reg8(0x140a01, 0x3f); //reset pid
    write_reg8(0x140a15, 0xc0); //chn tx_manual off
}

/**
 * @brief   This function serves to set RF ptx trigger.
 * @param[in]   addr    -   The address of tx_packet.
 * @param[in]   tick    -   Trigger ptx after (tick-current tick),If the difference is less than 0, trigger immediately.
 * @return  none.
 * @note        addr:must be aligned by word (4 bytes), otherwise the program will enter an exception.
 */
void rf_start_ptx(void *addr, unsigned int tick)
{
    dma_set_src_address(DMA0, (unsigned int)addr);
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    reg_rf_ll_cmd = 0x83;
}

/**
 * @brief   This function serves to set RF prx trigger.
 * @param[in]   tick    -   Trigger prx after (tick-current tick),If the difference is less than 0, trigger immediately.
 * @return  none.
 */
void rf_start_prx(unsigned int tick)
{
    write_reg32(0x140a28, 0x0fffffff);           // first timeout.
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    write_reg8(0x140a00, 0x84);
}

/**
 * @brief   This function serves to judge whether the FIFO is empty.
 * @param pipe_id specify the pipe.
 * @return TX FIFO empty bit.
 *          -#0 TX FIFO NOT empty.
 *          -#1 TX FIFO empty.
 */
unsigned char rf_is_rx_fifo_empty(unsigned char pipe_id)
{
    return (reg_rf_dma_tx_wptr(pipe_id)) == (reg_rf_dma_tx_rptr(pipe_id));
}

/**
 * @brief   This function to set retransmit and retransmit delay.
 * @param[in]   retry_times - Number of retransmit, 0: retransmit OFF
 * @param[in]   retry_delay - Retransmit delay time.
 * @return      none.
 */
void rf_set_ptx_retry(unsigned char retry_times, unsigned short retry_delay)
{
    retry_times &= 0x0f; //accommodate with private chips
    write_reg8(0x140a14, retry_times);

    retry_delay &= 0x0fff;
    unsigned short tmp = read_reg16(0x140a10);
    tmp &= 0xf000;
    tmp |= retry_delay;
    write_reg16(0x140a10, tmp);
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
    write_reg32(0x140a28, 0x0fffffff);           // first timeout
    write_reg32(0x140a18, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN; // Enable cmd_schedule mode.
    dma_set_src_address(DMA0, (unsigned int)(addr));
    write_reg8(0x80140a00, 0x88);                // single rx2tx
}

/**
 * @brief       This function is used to  set the modulation index of the receiver.
 *              This function is common to all modes,the order of use requirement:configure mode first,then set the the modulation index,default is 0.5 in drive,
 *              both sides need to be consistent otherwise performance will suffer,if don't specifically request,don't need to call this function.
 * @param[in]   mi  - the value of modulation_index*100.
 * @return      none.
 */
void rf_set_rx_modulation_index(rf_mi_value_e mi_value)
{
    unsigned char modulation_index_high;
    unsigned char modulation_index_low;
    unsigned char kvm_trim;
    mi_value             = (unsigned int)(mi_value * 1.28);
    modulation_index_low = mi_value % 256;

    modulation_index_high = (mi_value % 512) >> 8;

    reg_rf_modem_rxc_mi_flex_ble_0 = modulation_index_low;
    reg_rf_modem_rxc_mi_flex_ble_1 |= modulation_index_high;


    if (reg_rf_mode_cfg_tx1_0 & 0x01) //2MBPS mode
    {
        if ((mi_value >= 75) && (mi_value <= 100)) {
            kvm_trim = 3;
        } else if (mi_value > 100) {
            kvm_trim = 7;
        } else {
            kvm_trim = 1;
        }
    } else {
        if ((mi_value >= 75) && (mi_value <= 100)) {
            kvm_trim = 1;
        } else if ((mi_value > 100) && (mi_value <= 150)) {
            kvm_trim = 3;
        } else if (mi_value > 150) {
            kvm_trim = 7;
        } else {
            kvm_trim = 0;
        }
    }
    reg_rf_mode_cfg_tx1_0 = (reg_rf_mode_cfg_tx1_0 & ~(FLD_RF_VCO_TRIM_KVM)) | (kvm_trim << 1);
}

/**
 * @brief       This function is used to set the modulation index of the sender.
 *              This function is common to all modes,the order of use requirement:configure mode first,then set the the modulation index,default is 0.5 in drive,
 *              both sides need to be consistent otherwise performance will suffer,if don't specifically request,don't need to call this function.
 * @param[in]   mi  - the value of modulation_index*100.
 * @return      none.
 */
void rf_set_tx_modulation_index(rf_mi_value_e mi_value)
{
    unsigned char modulation_index_high;
    unsigned char modulation_index_low;
    unsigned char kvm_trim;
    mi_value             = (unsigned int)(mi_value * 1.28);
    modulation_index_low = mi_value % 256;

    modulation_index_high = (mi_value % 512) >> 8;
    reg_rf_mode_cfg_rx2_0 = modulation_index_low;
    reg_rf_mode_cfg_rx2_1 |= modulation_index_high;

    if (reg_rf_mode_cfg_tx1_0 & 0x01) //2MBPS mode
    {
        if ((mi_value >= 75) && (mi_value <= 100)) {
            kvm_trim = 3;
        } else if (mi_value > 100) {
            kvm_trim = 7;
        } else {
            kvm_trim = 1;
        }
    } else {
        if ((mi_value >= 75) && (mi_value <= 100)) {
            kvm_trim = 1;
        } else if ((mi_value > 100) && (mi_value <= 150)) {
            kvm_trim = 3;
        } else if (mi_value > 150) {
            kvm_trim = 7;
        } else {
            kvm_trim = 0;
        }
    }
    reg_rf_mode_cfg_tx1_0 = (reg_rf_mode_cfg_tx1_0 & ~(FLD_RF_VCO_TRIM_KVM)) | (kvm_trim << 1);
}

/**
 *  @brief      This function serve to adjust tx/rx settle timing sequence.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @return     none
*/
_attribute_ram_code_
void rf_fast_settle_config(rf_tx_fast_settle_time_e tx_settle_us, rf_rx_fast_settle_time_e rx_settle_us)
{
    g_rf_tx_fast_settle_time = tx_settle_us;
    g_rf_rx_fast_settle_time = rx_settle_us;
    //tx
    if (tx_settle_us == TX_SETTLE_TIME_50US) {
        //close hpmc and ldo trim,close hpmc(53us), ldotrim(4.5us),save 58us
        //Default settle time:108.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x140e84, (read_reg8(0x140e84) & 0xf0) | 0x0a); //1010

        write_reg8(0x140e96, 0x00);                                //sub-sequence1 start time:0
        write_reg8(0x140e97, 0x08);                                //sub-sequence2 start time:8us
        write_reg8(0x140e98, 0x30);                                //sub-sequence3 start time:48us
        write_reg8(0x140e99, 0x31);                                //sub-sequence4 start time:48.5us
        write_reg8(0x140e9a, 0x33);                                //sub-sequence5 start time:51us
        write_reg8(0x140e9b, 0x30);                                //sub-sequence6 start time:48us
    } else if (tx_settle_us == TX_SETTLE_TIME_104US) {
        // only close ldo trim(4.5us)
        //Default settle time:108.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x140e84, (read_reg8(0x140e84) & 0xf8) | 0x0e); //1110

        write_reg8(0x140e96, 0x00);                                //sub-sequence1 start time:0us
        write_reg8(0x140e97, 0x09);                                //sub-sequence2 start time:9us
        write_reg8(0x140e98, 0x65);                                //sub-sequence3 start time:101.5us
        write_reg8(0x140e99, 0x66);                                //sub-sequence4 start time:102us
        write_reg8(0x140e9a, 0x69);                                //sub-sequence5 start time:105us
        write_reg8(0x140e9b, 0x65);                                //sub-sequence6 start time:101.5us
    }

    //rx
    //RX: rx_ldo_trim (4.5us), rx_dcoc(40us)
    //RX Default settle time:85us
    //Fast settle time = Default settle time - Settle time of the closed module
    if (rx_settle_us == RX_SETTLE_TIME_45US) {
        //disable ldo trim(4.5us),rx dcoc(40us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x140e84, (read_reg8(0x140e84) & 0x0f) | 0x60); //0110

        write_reg8(0x140e9c, 0x00);                                //sub-sequence1 start time:0us
        write_reg8(0x140e9d, 0x09);                                //sub-sequence2 start time:9us
        write_reg8(0x140e9e, 0x09);                                //sub-sequence3 start time:9us
        write_reg8(0x140e9f, 0x1b);                                //sub-sequence4 start time:27us
        write_reg8(0x140ea0, 0x2d);                                //sub-sequence5 start time:45us
        write_reg8(0x140ea1, 0x2d);                                //sub-sequence6 start time:45us
    } else if (rx_settle_us == RX_SETTLE_TIME_80US) {
        //disable ldo trim(4.5us)
        //Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x140e84, (read_reg8(0x140e84) & 0x0f) | 0xe0); //1110
        write_reg8(0x140e9c, 0x00);                                //sub-sequence1 start time:0us
        write_reg8(0x140e9d, 0x09);                                //sub-sequence2 start time:9us
        write_reg8(0x140e9e, 0x09);                                //sub-sequence3 start time:9us
        write_reg8(0x140e9f, 0x22);                                //sub-sequence4 start time:34us
        write_reg8(0x140ea0, 0x4d);                                //sub-sequence5 start time:77us
        write_reg8(0x140ea1, 0x4d);                                //sub-sequence6 start time:77us
    }
}

/**
 *  @brief      This function serve to enable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
_attribute_ram_code_  //BLE SDK use
void rf_tx_fast_settle_en(void)
{
    if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_50US) {
        g_rf_tx_fast_settle_chn_cal_flag = 1;
        write_reg8(0x140ef6, read_reg8(0x140ef6) | 0x01); //hpmc bypass enable
        write_reg8(0x140ee2, read_reg8(0x140ee2) | 0x01); //ldo cal bypass enable
        write_reg8(0x140ee4, read_reg8(0x140ee4) | 0x03); //ldo RXTXHF bypass enable
        write_reg8(0x140ee6, read_reg8(0x140ee6) | 0x03); //ldo RXTXLF bypass enable
    } else if (g_rf_tx_fast_settle_time == TX_SETTLE_TIME_104US) {
        write_reg8(0x140ee2, read_reg8(0x140ee2) | 0x01); //ldo cal bypass enable
        write_reg8(0x140ee4, read_reg8(0x140ee4) | 0x03); //ldo RXTXHF bypass enable
        write_reg8(0x140ee6, read_reg8(0x140ee6) | 0x03); //ldo RXTXLF bypass enable
    }
    write_reg8(0x140e29, read_reg8(0x140e29) | 0x10);
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
    write_reg8(0x140ef6, read_reg8(0x140ef6) & 0xfe); //hpmc bypass disable
    write_reg8(0x140ee2, read_reg8(0x140ee2) & 0xfe); //ldo cal bypass disable
    write_reg8(0x140ee4, read_reg8(0x140ee4) & 0xfc); //ldo RXTXHF bypass disable
    write_reg8(0x140ee6, read_reg8(0x140ee6) & 0xfc); //ldo RXTXLF bypass disable
    write_reg8(0x140e29, read_reg8(0x140e29) & 0xef);
}

/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim calibration value address pointer
 *  @return     none
*/

static void rf_get_ldo_trim_val(rf_ldo_trim_t *ldo_cla)
{
    ldo_cla->LDO_CAL_TRIM    = read_reg8(0x140eea) & 0x3f;
    ldo_cla->LDO_RXTXHF_TRIM = read_reg8(0x140eec) & 0x3f;
    ldo_cla->LDO_RXTXLF_TRIM = ((read_reg8(0x140eed) & 0x0f) << 2) + ((read_reg8(0x140eec) & 0xc0) >> 6);
    ldo_cla->LDO_PLL_TRIM    = read_reg8(0x140eee) & 0x3f;
    ldo_cla->LDO_VCO_TRIM    = ((read_reg8(0x140eef) & 0x0f) << 2) + ((read_reg8(0x140eee) & 0xc0) >> 6);
}

/**
 *  @brief      This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_
static void rf_set_ldo_trim_val(rf_ldo_trim_t ldo_trim)
{
    write_reg8(0x140ee2, (ldo_trim.LDO_CAL_TRIM << 1));
    write_reg8(0x140ee4, (ldo_trim.LDO_RXTXHF_TRIM << 2));
    write_reg8(0x140ee5, ldo_trim.LDO_RXTXLF_TRIM);
    write_reg8(0x140ee6, (ldo_trim.LDO_PLL_TRIM << 2));
    write_reg8(0x140ee7, ldo_trim.LDO_VCO_TRIM);
}

#if (0)
/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
static void rf_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal)
{
    dcoc_cal->DCOC_IDAC        = read_reg8(0x140ed8) & 0x3f;                                            //DCOC_IDAC 0xd8[5:0]
    dcoc_cal->DCOC_QDAC        = read_reg8(0x140eda) & 0x3f;                                            //DCOC_QDAC 0xda[5:0]
    dcoc_cal->DCOC_IADC_OFFSET = read_reg8(0x140edc) & 0x7f;                                            //DCOC_IADC_OFFSET 0xdc[6:0]
    dcoc_cal->DCOC_QADC_OFFSET = (read_reg8(0x140edc) & 0x80) >> 7 | (read_reg8(0x140edd) & 0x3f) << 1; //DCOC_QADC_OFFSET 0xdc[7] 0xdd[5:0]
}

/**
 *  @brief      This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return     none
*/
static void rf_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal)
{
    write_reg8(0x140ed0, (dcoc_cal.DCOC_IDAC << 1));
    write_reg8(0x140ed0, read_reg8(0x140ed0) | ((dcoc_cal.DCOC_QDAC & 0x01) << 7));
    write_reg8(0x140ed1, ((dcoc_cal.DCOC_QDAC) & 0x3e) >> 1);
    write_reg8(0x140ece, (dcoc_cal.DCOC_IADC_OFFSET << 1));
    write_reg8(0x140ecf, dcoc_cal.DCOC_QADC_OFFSET);
}


#endif
/**
 *  @brief      This function serve to enable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
_attribute_ram_code_  ////BLE SDK use
void rf_rx_fast_settle_en(void)
{
    write_reg8(0x140ee2, read_reg8(0x140ee2) | 0x01); //ldo cal bypass enable
    write_reg8(0x140ee4, read_reg8(0x140ee4) | 0x03); //ldo RXTXHF bypass enable
    write_reg8(0x140ee6, read_reg8(0x140ee6) | 0x03); //ldo RXTXLF bypass enable
    write_reg8(0x140e29, read_reg8(0x140e29) | 0x08);
}

/**
 *  @brief      This function serve to disable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void rf_rx_fast_settle_dis(void)
{
    g_rf_rx_fast_settle_time = RX_FAST_SETTLE_NONE;
    write_reg8(0x140ee2, read_reg8(0x140ee2) & 0xfe); //ldo cal bypass disable
    write_reg8(0x140ee4, read_reg8(0x140ee4) & 0xfc); //ldo RXTXHF bypass disable
    write_reg8(0x140ee6, read_reg8(0x140ee6) & 0xfc); //ldo RXTXLF bypass disable
    write_reg8(0x140e29, read_reg8(0x140e29) & 0xf7);
}

/**
 *  @brief      This function is used to set the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80.
                                Reserved for future functionality. Currently, this parameter has no effect.
 *  @return     none
 *  @note       RX_SETTLE_TIME_45US - disable rx_ldo_trim and rx_dcoc calibration,reduce 44.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_80US - disable rx_ldo_trim calibration,reduce 4.5us of rx settle time. Do a normal calibration at the beginning.
*/
void rf_rx_fast_settle_update_cal_val(rf_rx_fast_settle_time_e rx_settle_time, unsigned char chn)
{
    (void)rx_settle_time;
    (void)chn;
    rf_fast_settle_t fs_cv;
    rf_rx_fast_settle_get_cal_val(rx_settle_time, chn, &fs_cv);
    rf_rx_fast_settle_set_cal_val(rx_settle_time, chn, &fs_cv);
}

/**
 *  @brief      This function is used to get the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80.
 *  @param[in]  fs_cv           Fast settle calibration value address pointer.
                Reserved for future functionality. Currently, this parameter has no effect.
 *  @return     none
 *  @note       RX_SETTLE_TIME_45US - disable rx_ldo_trim and rx_dcoc calibration,reduce 44.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_80US - disable rx_ldo_trim calibration,reduce 4.5us of rx settle time. Do a normal calibration at the beginning.
*/

void rf_rx_fast_settle_get_cal_val(rf_rx_fast_settle_time_e rx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    (void)rx_settle_time;
    (void)chn;
    rf_get_ldo_trim_val(&(fs_cv->ldo_trim));
}

/**
 *  @brief      This function is used to set the rx fast_settle calibration value.
 *  @param[in]  rx_settle_us    After adjusting the timing sequence, the time required for rx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80.
                                Reserved for future functionality. Currently, this parameter has no effect.
 *  @param[in]  fs_cv           Fast settle calibration value address pointer.
 *  @return     none
 *  @note       RX_SETTLE_TIME_45US - disable rx_ldo_trim and rx_dcoc calibration,reduce 44.5us of rx settle time.Receive for a period of time and then do a normal calibration.
 *              RX_SETTLE_TIME_80US - disable rx_ldo_trim calibration,reduce 4.5us of rx settle time. Do a normal calibration at the beginning.
*/
_attribute_ram_code_
void rf_rx_fast_settle_set_cal_val(rf_rx_fast_settle_time_e rx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    (void)rx_settle_time;
    (void)chn;
    rf_set_ldo_trim_val(fs_cv->ldo_trim);
}

/**
 *  @brief      This function is mainly used to get hpmc Calibration-related values.
 *  @param[in]  none
 *  @return     Returns the hpmc_gain value
*/
_attribute_ram_code_sec_noinline_ static unsigned short rf_get_hpmc_cal_val()
{
    unsigned short cali;
    unsigned short hpmc_gain;
    cali      = read_reg16(0x140efe);
    hpmc_gain = (cali << 1) & 0x0ffe;
    return hpmc_gain;
}

/**
 *  @brief      This function is used to set the tx fast_settle calibration value.
 *  @param[in]  tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]  chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Only applicable to TX_SETTLE_TIME_50US, other parameters are invalid.
 *                              (When tx_settle_us is 50us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @return     none
 *  @note       TX_SETTLE_TIME_50US  - disable tx_ldo_trim function and tx_hpmc,reduce 58us of tx settle time.After frequency hopping, a normal calibration must be done.
 *              TX_SETTLE_TIME_104US - disable tx_ldo_trim function,reduce 4.5us of tx settle time. Do a normal calibration at the beginning.
*/
void rf_tx_fast_settle_update_cal_val(rf_tx_fast_settle_time_e tx_settle_time, unsigned char chn)
{
    rf_fast_settle_t fs_cv;
    rf_tx_fast_settle_get_cal_val(tx_settle_time, chn, &fs_cv);
    rf_tx_fast_settle_set_cal_val(tx_settle_time, chn, &fs_cv);
}

/**
 *  @brief        This function is used to get the tx fast_settle calibration value.
 *  @param[in]    tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]    chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Only applicable to TX_SETTLE_TIME_50US, other parameters are invalid.
 *                                (When tx_settle_us is 50us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @param[in]    fs_cv           Fast settle calibration value address pointer.
 *  @return       none
 *  @note         TX_SETTLE_TIME_50US  - disable tx_ldo_trim function and tx_hpmc,reduce 58us of tx settle time.After frequency hopping, a normal calibration must be done.
 *                TX_SETTLE_TIME_104US - disable tx_ldo_trim function,reduce 4.5us of tx settle time. Do a normal calibration at the beginning.
*/

void rf_tx_fast_settle_get_cal_val(rf_tx_fast_settle_time_e tx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    if (tx_settle_time == TX_SETTLE_TIME_50US) {
        if (chn <= 80) {
            fs_cv->cal_tbl[chn] = rf_get_hpmc_cal_val();
        }
    }
    rf_get_ldo_trim_val(&(fs_cv->ldo_trim));
}

/**
 *  @brief        This function is used to set the tx fast_settle calibration value.
 *  @param[in]    tx_settle_us    After adjusting the timing sequence, the time required for tx to settle.
 *  @param[in]    chn             Calibrates the frequency (2400 + chn). Range: 0 to 80. Only applicable to TX_SETTLE_TIME_50US, other parameters are invalid.
 *                                (When tx_settle_us is 50us, the modules to be calibrated are frequency-dependent, so all used frequency points need to be calibrated.)
 *  @param[in]    fs_cv           Fast settle calibration value address pointer.
 *  @return       none
 *  @note         TX_SETTLE_TIME_50US  - disable tx_ldo_trim function and tx_hpmc,reduce 58us of tx settle time.After frequency hopping, a normal calibration must be done.
 *                TX_SETTLE_TIME_104US - disable tx_ldo_trim function,reduce 4.5us of tx settle time. Do a normal calibration at the beginning.
*/
_attribute_ram_code_
void rf_tx_fast_settle_set_cal_val(rf_tx_fast_settle_time_e tx_settle_time, unsigned char chn, rf_fast_settle_t *fs_cv)
{
    (void)tx_settle_time;
    (void)chn;
    rf_set_ldo_trim_val(fs_cv->ldo_trim);
}

/**
 * @brief      This function serves to init the 2-wire-PTA.
 * @param[in]  ble_priority_pin - the pin of ble_priority.
 * @param[in]  wlan_active_pin  - the pin of wlan_active.
 * @param[in]  ble_priority_mode- the mode of ble_priority pin.
 *             when the mode is PTA_BLE_PRIORITY_TX,the pin of ble_priority will be high if tx.
 *             when the mode is PTA_BLE_PRIORITY_RX,the pin of ble_priority will be high if rx.
 *             when the mode is PTA_BLE_PRIORITY_TRX,the pin of ble_priority will be high if tx and rx.
 * @return     none
 */
void rf_2wire_pta_init(pta_bleprio_pin_e ble_priority_pin, gpio_pin_e wlan_active_pin, pta_2wire_mode_e ble_priority_mode)
{
    //wifi_2wire_sel
    reg_gpio_bb_mux_dbug0 &= (~(FLD_GPIO_DBG_SEL_BT | FLD_GPIO_WIFI_CO_SEL)); //wifi_io_sel
    reg_gpio_bb_mux_dbug0 |= (FLD_GPIO_WIFI_SEL2W);                           //  reg_gpio_irq_ctrl|=BIT(0);
    //ble_activity
    if (ble_priority_mode == PTA_BLE_PRIORITY_TX) {
        reg_rf_coex_enable &= (~FLD_RF_TRX_PRIO);
        reg_rf_coex_enable &= (~FLD_RF_RX_PRIO);
        reg_rf_coex_enable |= (FLD_RF_TX_PRIO);
    } else if (ble_priority_mode == PTA_BLE_PRIORITY_RX) {
        reg_rf_coex_enable &= (~FLD_RF_TRX_PRIO);
        reg_rf_coex_enable &= (~FLD_RF_TX_PRIO);
        reg_rf_coex_enable |= (FLD_RF_RX_PRIO);
    } else if (ble_priority_mode == PTA_BLE_PRIORITY_TRX) {
        reg_rf_coex_enable &= (~FLD_RF_RX_PRIO);
        reg_rf_coex_enable &= (~FLD_RF_TX_PRIO);
        reg_rf_coex_enable |= (FLD_RF_TRX_PRIO);
    }
    gpio_output_dis(wlan_active_pin);
    gpio_input_en(wlan_active_pin);
    reg_gpio_func_mux(ble_priority_pin) = reg_gpio_func_mux(ble_priority_pin) | (0xc0);
    gpio_function_dis(ble_priority_pin);
    gpio_function_en(wlan_active_pin);
}

/**
 * @brief      This function serves to init the 3-wire-PTA.
 * @param[in]  ble_active_pin - the pin of ble_active.
 * @param[in]  ble_status_pin - the pin of ble_status.
 * @param[in]  wlan_deny_pin  - the pin of wlan_deny.
 * @param[in]  ble_status_mode  - the mode of ble_status.
               when the mode is PTA_BLE_STATUS_TX,the ble_status pin will be high if stx.
               when the mode is PTA_BLE_STATUS_RX,the ble_status pin will be high if srx.
 * @return     none
 * @note       Attention:In the three-wire PTA mode, there will be a period of time t1 to
 *             detect wlan_active and a time t2 to switch the ble_status state before the
 *             action is triggered.The actual start time of the corresponding RF action will
 *             shift backward by the larger of the two.These two periods of time can be set
 *             by function rf_set_pta_t1_time and function rf_set_pta_t2_time respectively.
 */
void rf_3wire_pta_init(pta_bleactive_pin_e ble_active_pin, pta_blestatus_pin_e ble_status_pin, pta_wlandeny_pin_e wlan_deny_pin, pta_3wire_mode_e ble_status_mode)
{
    reg_rf_coex_enable |= FLD_RF_COEX_EN;                                                                                    //r_coex_en
    reg_rf_coex_enable |= FLD_RF_COEX_STATUS;                                                                                //r_coex_status
    (ble_status_mode == 0) ? (reg_rf_coex_enable &= (~FLD_RF_COEX_TRX_POL)) : (reg_rf_coex_enable |= (FLD_RF_COEX_TRX_POL)); //PTA 3-wire mode choose

    reg_gpio_bb_mux_dbug0 &= (~(FLD_GPIO_DBG_SEL_BT | FLD_GPIO_WIFI_CO_SEL));                                                //wifi_io_sel
    reg_gpio_bb_mux_dbug0 &= (~FLD_GPIO_WIFI_SEL2W);                                                                         //wifi_io_sel

    //wifi_co_activity pb[3]
    reg_gpio_func_mux(ble_active_pin) = reg_gpio_func_mux(ble_active_pin) | (0xc0);

    //wifi_co_status pe[4]
    reg_gpio_func_mux(ble_status_pin) = reg_gpio_func_mux(ble_status_pin) | (0x03);

    //wifi_co_activity pe[5]
    reg_gpio_func_mux(wlan_deny_pin) = reg_gpio_func_mux(wlan_deny_pin) | (0x0c);

    gpio_function_dis(ble_active_pin);
    gpio_function_dis(ble_status_pin);
    gpio_function_dis(wlan_deny_pin);
}

/**
 * @brief       This function is used to set the antenna switching sequence table. The content in the table is the
 *              antenna sequence number that needs to be switched to when the position is found by the look-up table.
 *              Since determining the antenna switching sequence needs to determine the order of the table lookup and
 *              the setting of the table content, this function is usually used in conjunction with the function
 *              rf_aoa_aod_ant_pattern.
 * @param[in]   dat      - Antenna serial number written into the antenna switching sequence table.The value in the table
 *                       corresponds to the antenna number that needs to be switched to when it is found in the table.The
 *                       value range is 0 to 7.
 * @return      none.
 */
void rf_aoa_aod_ant_lut(unsigned char *dat)
{
    write_reg8(0x140868, ((read_reg8(0x140868) & 0xf8) | dat[0]));
    write_reg8(0x140868, ((read_reg8(0x140868) & 0x8f) | (dat[1] << 4)));
    write_reg8(0x140869, ((read_reg8(0x140869) & 0xf8) | dat[2]));
    write_reg8(0x140869, ((read_reg8(0x140869) & 0x8f) | (dat[3] << 4)));
    write_reg8(0x14086a, ((read_reg8(0x14086a) & 0xf8) | dat[4]));
    write_reg8(0x14086a, ((read_reg8(0x14086a) & 0x8f) | (dat[5] << 4)));
    write_reg8(0x14086b, ((read_reg8(0x14086b) & 0xf8) | dat[6]));
    write_reg8(0x14086b, ((read_reg8(0x14086b) & 0x8f) | (dat[7] << 4)));
    write_reg8(0x14086c, ((read_reg8(0x14086c) & 0xf8) | dat[8]));
    write_reg8(0x14086c, ((read_reg8(0x14086c) & 0x8f) | (dat[9] << 4)));

    write_reg8(0x14086d, ((read_reg8(0x14086d) & 0xf8) | dat[10]));
    write_reg8(0x14086d, ((read_reg8(0x14086d) & 0x8f) | (dat[11] << 4)));
    write_reg8(0x14086e, ((read_reg8(0x14086e) & 0xf8) | dat[12]));
    write_reg8(0x14086e, ((read_reg8(0x14086e) & 0x8f) | (dat[13] << 4)));
    write_reg8(0x14086f, ((read_reg8(0x14086f) & 0xf8) | dat[14]));
    write_reg8(0x14086f, ((read_reg8(0x14086f) & 0x8f) | (dat[15] << 4)));
}

/**
 * @brief       This function is mainly used to set the antenna switching mode. Vulture support three different
 *              table lookup sequences.The setting here is just the order of the table lookup, and the content
 *              in the table is the number of the antenna to be switched to.The switching sequence of the antenna
 *              needs to be determined by the combination of the table look-up sequence and the antenna number in
 *              the table,so this function is usually used together with the rf_aoa_aod_ant_lut function.
 * @param[in]   pattern     - Enumeration of several different look-up table order modes.Refer to the corresponding
 *                            enumeration annotation for the meaning of the mode.
 * @return      none.
 */
void rf_aoa_aod_ant_pattern(rf_ant_pattern_e pattern)
{
    reg_rf_man_ant_slot = ((reg_rf_man_ant_slot & (~FLD_RF_ANT_PAT)) | pattern);
}

/**
 * @brief       This function is mainly used to set the number of antennas enabled by the multi-antenna board in the
 *              AOA/AOD function;the vulture series chips currently support up to 8 antennas for switching.By default,
 *              it is set to 8 antennas. After configuring the RF-related settings, you can set the number of enabled
 *              antennas, and this setting needs to be completed before sending and receiving packets.
 * @param[in]   ant_num     - The number of antennas, the value ranges from 1 to 8.
 * @return      none.
 */
void rf_aoa_aod_set_ant_num(unsigned char ant_num)
{
    ant_num       = (((ant_num - 1) & 0x07) << 4);
    reg_rf_rxsupp = ((reg_rf_rxsupp & (~FLD_RF_ANT_NUM)) | ant_num);
}

/**
 * @brief       This function is mainly used to set the IQ data sample interval time. In normal mode, the sampling interval of AOA is 4us, and AOD will judge whether
 *              the sampling interval is 4us or 2us according to CTE info.The 4us/2us sampling interval corresponds to the 2us/1us slot mode stipulated in the protocol.
 *              Due to the current antenna hardware switching only supporting 4us/2us intervals, setting the sampling interval to 1us or less will result in sampling at
 *              one antenna switching interval. Therefore, the sampling data needs to be processed by the upper layer as needed. At present, it is mainly used for
 *              debugging processes.After configuring RF, you can call this function to configure slot time.
 * @param[in]   time_us - AOA or AOD slot time mode.
 * @return      none.
 * @note        Attention:(1)When the time is 0.25us, it cannot be used with the 20bit iq data type, which will cause the sampling data to overflow.
 *                        (2)Since only the antenna switching interval of 4us/2us is supported, the sampling interval of 1us and shorter time intervals
 *                            will be sampled multiple times in one antenna switching interval. Suggestions can be used according to specific needs.
 */
void rf_aoa_aod_sample_interval_time(rf_aoa_aod_sample_interval_time_e sample_time)
{
    if ((sample_time & 0xff) <= 3) {
        reg_rf_man_ant_slot = ((reg_rf_man_ant_slot & 0xcf) | ((sample_time << 4) & 0xff));
        reg_rf_mode_ctrl0 &= 0xfc;
    } else {
        reg_rf_man_ant_slot &= 0xcf;
        reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & 0xfc) | ((sample_time - 3) & 0xff));
    }
    g_sample_interval = sample_time;
}

/**
 * @brief       This function is mainly used to initialize the parameters related to AOA/AOD antennas, including the
 *              number of antennas, the pins for controlling the antennas,the look-up mode of antenna switching, and
 *              the content of the antenna switching sequence table.
 * @param[in]   ant_num         - The number of antennas, the value ranges from 1 to 8.
 * @param[in]   ant_pin_config: - Control antenna pin selection and configuration.The parameter setting needs to be
 *                                set according to the number and position of the control antenna.For example,if you
 *                                need to control four antennas, it is best to use Antsel0 and Antsel2.
 * @param[in]   pattern         - Enumeration of several different look-up table order modes.
 * @param[in]   dat             - The antenna value written into the antenna switching sequence table ranges from 0 to 7.
 * @return      none.
 */
void rf_aoa_aod_ant_init(unsigned char num, rf_ant_pin_sel_t *ant_pin_config, rf_ant_pattern_e pattern, unsigned char *dat)
{
    unsigned char val  = 0;
    unsigned char mask = 0xff;
    rf_aoa_aod_set_ant_num(num);

    if (ant_pin_config->antsel0_pin == RF_ANT_SEL0_PB5) {
        mask = (unsigned char)~(BIT(3) | BIT(2));
        val  = 0;
    } else if (ant_pin_config->antsel0_pin == RF_ANT_SEL0_PC1) {
        mask = (unsigned char)~(BIT(3) | BIT(2));
        val  = BIT(2);
    }
    reg_gpio_func_mux(ant_pin_config->antsel0_pin) = (reg_gpio_func_mux(ant_pin_config->antsel0_pin) & mask) | val;

    if (ant_pin_config->antsel1_pin == RF_ANT_SEL1_PC2) {
        mask = (unsigned char)~(BIT(5) | BIT(4));
        val  = BIT(4);
    }
    reg_gpio_func_mux(ant_pin_config->antsel1_pin) = (reg_gpio_func_mux(ant_pin_config->antsel1_pin) & mask) | val;

    if (ant_pin_config->antsel2_pin == RF_ANT_SEL2_PC3) {
        mask = (unsigned char)~(BIT(6) | BIT(7));
        val  = BIT(6);
    }
    reg_gpio_func_mux(ant_pin_config->antsel1_pin) = (reg_gpio_func_mux(ant_pin_config->antsel1_pin) & mask) | val;

    gpio_function_dis(ant_pin_config->antsel0_pin);
    gpio_function_dis(ant_pin_config->antsel1_pin);
    gpio_function_dis(ant_pin_config->antsel2_pin);

    rf_aoa_aod_ant_pattern(pattern);

    rf_aoa_aod_ant_lut(dat);
}

/**
 * @brief       This function is mainly used to set the parameters related to AOA/AOD sampling, including the length
 *              of IQ data, sampling interval, and sampling offset.
 * @param[in]   iq_data             - The length of each I or Q data.
 * @param[in]   sample_interval     - AOA or AOD sampling interval time.
 * @param[in]   sample_point_offset - The parameter range is -45 to 210.If the parameter is negative,the position of
 *                                    the sampling point moves forward.The absolute value of the parameter is multiplied
 *                                    by 0.125us.If the parameter is positive, the position of the sampling point moves
 *                                    backward. The parameter is multiplied by 0.125us.
 * @return      none.
 */
void rf_aoa_aod_sample_init(rf_aoa_aod_iq_data_mode_e iq_data, rf_aoa_aod_sample_interval_time_e sample_interval, char sample_point_offset)
{
    rf_aoa_aod_iq_data_mode(iq_data);
    rf_aoa_aod_sample_interval_time(sample_interval);
    rf_aoa_aod_sample_point_adjust(sample_point_offset);
}

/**
 * @brief       This function is used to calculate the number of IQ groups in the received AOA/AOD packet.
 * @param[in]   p               - Received packet address pointer.
 * @return      Returns the number of groups of iq in the package.
 */
unsigned int rf_aoa_aod_iq_group_number(unsigned char *p)
{
    unsigned char y = 0;
    y               = ((g_sample_interval >> 8) & 0xff);
    if (SAMPLE_AOA_4US_AOD_CTEINFO_INTERVAL == g_sample_interval) {
        if ((p[6] & 0xc0) == 0x40) {
            y = 8;
        }
    }
    return ((((p[6] & 0x1f) << 3) - 12) / (y >> 2) + 8);
}

/**
 * @brief       This function is mainly used to obtain the offset of header information in the packet data received
 *              in AOA/AOD mode.
 * @param[in]   p               - Received packet address pointer.
 * @return      The return value is the offset of header information in the packet.
 */
unsigned int rf_aoa_aod_hdinfo_offset(unsigned char *p)
{
    unsigned char x = 0;
    x               = ((g_iq_data_len >> 8) & 0xff);

    return (rf_aoa_aod_iq_group_number(p) * x + p[5] + 10);
}

/**
 * @brief       This function is mainly used to detect whether the DMA length of the received packet is correct in
 *              the AOA/AOD mode.
 * @param[in]   p               - Received packet address pointer.
 * @return      Return length to judge whether it is correct, 1: ok, 0: false
 */
unsigned char rf_aoa_aod_is_rx_pkt_len_ok(unsigned char *p)
{
    unsigned char x = 0;
    x               = ((g_iq_data_len >> 8) & 0xff);

    return (p[0] == ((rf_aoa_aod_iq_group_number(p) * x + p[5] + 14) & 0xff)) ? 1 : 0;
}

/**
 * @brief       This function is used to obtain the timestamp information in the AOA/AOD package.
 * @param[in]   p               - Received packet address pointer.
 * @return      Returns the timestamp value in the packet.
 */
unsigned int rf_aoa_aod_get_pkt_timestamp(unsigned char *p)
{
    unsigned char x = 0;
    x               = ((g_iq_data_len >> 8) & 0xff);
    return (p[rf_aoa_aod_iq_group_number(p) * x + p[5] + 13] << 24 | p[rf_aoa_aod_iq_group_number(p) * x + p[5] + 12] << 16 | p[rf_aoa_aod_iq_group_number(p) * x + p[5] + 11] << 8 | p[rf_aoa_aod_iq_group_number(p) * x + p[5] + 10]);
}

/**
 * @brief       This function is mainly used to obtain the CRC value in the AOA/AOD packet.
 * @param[in]   p               - Received packet address pointer.
 * @return      The return value is the rssi value in headerinformation.
 */
signed char rf_aoa_aod_get_pkt_rssi(unsigned char *p)
{
    unsigned char x = 0;
    x               = ((g_iq_data_len >> 8) & 0xff);

    return (p[rf_aoa_aod_iq_group_number(p) * x + p[5] + 16] - 110);
}

/**
 * @brief       This function is mainly used to set the type of AOA/AOD iq data. The default data type is 8bit.This
 *              configuration can be done before starting to receive the package.
 * @param[in]   mode    - The length of each I or Q data.
 * @return      none.
 * @note        Attention :When the iq data is 20bit, it cannot be used with the 0.25us mode, which will cause the
 *                         sampling data to overflow.
 */
void rf_aoa_aod_iq_data_mode(rf_aoa_aod_iq_data_mode_e mode)
{
    reg_rf_sof_offset = ((reg_rf_sof_offset & (~FLD_RF_SUPP_MODE)) | ((mode << 4) & 0xff));
    g_iq_data_len     = mode;
}
