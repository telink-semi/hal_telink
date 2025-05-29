/********************************************************************************************************
 * @file    ext_hadm_rf.h
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
#ifndef DRIVERS_B92_EXT_DRIVER_DRIVER_INTERNAL_EXT_HADM_RF_H_
#define DRIVERS_B92_EXT_DRIVER_DRIVER_INTERNAL_EXT_HADM_RF_H_
#include "../../lib/include/rf.h"
#include "common/types.h"

//3us TxLLDL
//2us TxPathDLY (tx on -> rx sync 64us,64 -40 -8 -14 = 2
//1M extra preamble length * 8us
//settle : TX_FAST_SETTLE_TIME + 1
#include "ext_rf.h"
#define     CS_TX_PATH_DELAY_1M             2
#define     CS_TX_PATH_DELAY_2M             1
#define     CS_TX_LL_DEALY_1M               3
#define     CS_TX_LL_DEALY_2M               2

#define     CS_TX_STL_BTX_1ST_PKT_REAL_1M   (CS_TX_LL_DEALY_1M + (TX_FAST_SETTLE_TIME+1) + CS_TX_PATH_DELAY_1M + 8*PRMBL_EXTRA_1M)//(110 - 3) //3 is total switch delay time
#define     CS_TX_STL_BTX_1ST_PKT_REAL_2M   (CS_TX_LL_DEALY_2M + (TX_FAST_SETTLE_TIME+1) + CS_TX_PATH_DELAY_2M + 8*PRMBL_EXTRA_1M)//(110 - 3) //3 is total switch delay time

#define     CS_AD_CONVERT_DLY_1M            (RF_RX_SHORT_MODE_EN ? 12:19)  //before:20. Jaguar T_IFS need 32M + AD_Convert=19, tested by kai.jia at 2022-11-17
#define     CS_AD_CONVERT_DLY_2M            (RF_RX_SHORT_MODE_EN ? 5 : 13)

#if  (SW_DCOC_EN)
#define     CS_OTHER_SWITCH_DELAY_1M        2
#define     CS_OTHER_SWITCH_DELAY_2M        0
#else
#define     CS_OTHER_SWITCH_DELAY_1M        0
#define     CS_OTHER_SWITCH_DELAY_2M        0
#endif
#define     CS_HW_DELAY_1M                  (CS_AD_CONVERT_DLY_1M + CS_OTHER_SWITCH_DELAY_1M)
#define     CS_HW_DELAY_2M                  (CS_AD_CONVERT_DLY_2M + CS_OTHER_SWITCH_DELAY_2M)

#define     CS_HW_DELAY_CS_1M               (CS_HW_DELAY_1M)
#define     CS_HW_DELAY_CS_2M               (CS_HW_DELAY_2M)


#define     reg_rf_ll_irq_list_h        REG_ADDR8(REG_BB_LL_BASE_ADDR+0x4d)

#ifndef HADM_PHASE_CONTINUITY
    #define HADM_PHASE_CONTINUITY           1
#endif
/**
 * @brief    Enumerated variables for CS's settle sequence mode on or off.
 */
typedef enum
{
    RF_CS_SETTLE_SEQ_OFF,//Turns off the settle timing in channel sounding mode and reverts to the normal settle sequence.
    RF_CS_SETTLE_SEQ_ON  //Enable settle sequence for use in channel sounding mode.
}rf_cs_settle_seq_mode_e;

/**
 * @brief   Select how you want to start IQ sampling.
 */
typedef enum
{
    RF_CS_IQ_SAMPLE_SYNC_MODE,
    RF_CS_IQ_SAMPLE_RXEN_MODE
}rf_cs_iq_sample_mode_e;

/**
 * @brief       This function is used to get the value of the agc gain latch.
 * @return      Returns the value of gain latch.
 */
__INLINE unsigned char rf_get_gain_lat_value(void)
{
    return (reg_rf_modem_gain_lat0 & 0x07);
}

/**
 *  @brief  tx related calibration value in hadm function.
 */
typedef struct __attribute__((packed))  {
    unsigned short    tx_hpmc;
    rf_ldo_trim_t ldo_trim;
}rf_cs_tx_cali_t;

/**
 *  @brief  rx related calibration value in hadm function.
 */
typedef struct __attribute__((packed)) {
    rf_ldo_trim_t   ldo_trim;
#if (!SW_DCOC_EN)
    rf_dcoc_cal_t   dcoc_cal;
#endif
    rf_rccal_cal_t  rccal_cal;
}rf_cs_rx_cali_t;

/**
 * @brief   Define function to set tx channel or rx channel.
 */
typedef enum
{
    TX_CHANNEL      = 0,
    RX_CHANNEL      = 1,
}rf_trx_chn_e;



#if (HADM_PHASE_CONTINUITY)
    extern _attribute_data_retention_ rf_cs_tx_cali_t tx_cs_cali;
    extern _attribute_data_retention_ rf_cs_rx_cali_t rx_cs_cali;
    extern _attribute_data_retention_ unsigned char cs_phase_continuity_flag;

    void ble_rf_cs_phase_continuity_en(void);
    void ble_rf_cs_phase_continuity_dis(unsigned char phase_en);
    void rf_cs_restore_cali_auto_run(unsigned char phase_en);
    void rf_manual_fcal_start(void);
    void rf_manual_fcal_done(void);
    void rf_cs_get_rx_cali_vlue(rf_cs_rx_cali_t *rx_cali);
    void rf_cs_get_tx_cali_vlue(rf_cs_tx_cali_t *tx_cali);
    void rf_cs_get_ldo_trim_val(rf_ldo_trim_t *ldo_trim);
#if (!SW_DCOC_EN)
    void rf_cs_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal);
    void rf_cs_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal);
#endif
    void rf_cs_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal);
    void rf_lna_pup(void);
    void rf_cs_set_rx_cali_vlue(rf_cs_rx_cali_t rx_cali);
    void rf_cs_set_tx_cali_vlue(rf_cs_tx_cali_t tx_cali);
    void rf_cs_set_ldo_trim_val(rf_ldo_trim_t ldo_trim);
    void ble_rf_dis_rccal_trim(void);
    void rf_cs_set_rccal_cal_val(rf_rccal_cal_t rccal_cal);
    void rf_cs_settle_sequence_mode(rf_cs_settle_seq_mode_e on_off);
#endif

void ble_rf_channel_sounding_init(void);

void ble_rf_channel_sounding_deinit(void);

void ble_rf_channel_sounding_iq_sample_config(unsigned short sample_num, unsigned char start_point, rf_cs_iq_sample_mode_e sample_mode);

void ble_rf_set_manual_tx_mode(void);

void ble_rf_set_tx_modulation_index(rf_mi_value_e mi_value);

void ble_rf_set_cs_channel(signed char chn);

void rf_agc_disable(void);

void rf_agc_enable(void);

void ble_rf_set_accessCodeThreshold(u8 threshold);

/**
 * @brief   Initialize the structure used to control the antenna IO.
 */
typedef struct{
    gpio_func_pin_e antsel0_pin;
    gpio_func_pin_e antsel1_pin;
    gpio_func_pin_e antsel2_pin;
}rf_ant_pin_sel_t;

/**
 * @brief   Take 4 antennas as an example to illustrate the antenna switching sequence.
 *          SWITCH_SEQ_MODE0    - antenna index switch sequence 01230123
 *          SWITCH_SEQ_MODE1    - antenna index switch sequence 0123210
 *          SWITCH_SEQ_MODE2    - antenna index switch sequence 001000200030
 */
typedef enum{
    SWITCH_SEQ_MODE0         = 0,
    SWITCH_SEQ_MODE1         = BIT(6),
    SWITCH_SEQ_MODE2         = BIT(7)
}rf_ant_pattern_e;

/**
 * @brief   cs tx antenna switch mode.
 */
typedef enum
{
    RF_CS_TX_ANT_SWITCH_TXON = 0x01,
    RF_CS_TX_ANT_SWITCH_PAPUP = 0x02
}rf_cs_tx_ant_mode_e;

/**
 * @brief   Enumerated variables for cs antenna clock mode.
 */
typedef enum
{
    RF_CS_ANT_CLK_ALWAYS_ON_MODE = 0,//The clock keeps working, the clk_en is set by register 0x2a[0].
    RF_CS_ANT_CLK_SWITCH_ON_MODE = 1 //Only during antenna switch over clock on, ant_switch_clk auto open in aoa/d mode.
}rf_cs_ant_clk_mode_e;

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
void rf_aoa_aod_ant_pattern(rf_ant_pattern_e pattern);

/**
 * @brief       This function is mainly used to set the number of antennas enabled by the multi-antenna board in the
 *              AOA/AOD function;the vulture series chips currently support up to 8 antennas for switching.By default,
 *              it is set to 8 antennas. After configuring the RF-related settings, you can set the number of enabled
 *              antennas, and this setting needs to be completed before sending and receiving packets.
 * @param[in]   ant_num     - The number of antennas, the value ranges from 1 to 8.
 * @return      none.
 */
void rf_aoa_aod_set_ant_num(unsigned char ant_num);

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
void rf_aoa_aod_ant_lut(unsigned char *dat);

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
void ble_rf_cs_ant_lut(unsigned int dat);

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
void rf_aoa_aod_ant_init(unsigned char num, rf_ant_pin_sel_t * ant_pin_config, rf_ant_pattern_e pattern, unsigned int dat);

void rf_cs_txant_switch_mode(rf_cs_tx_ant_mode_e mode);

void rf_cs_rxant_switch_on(void);

void rf_cs_rxant_switch_off(void);

void rf_cs_ant_switch_auto(void);

void rf_cs_ant_switch_manual(void);


/**
 * @brief       This function is mainly used to set the energy when sending a single carrier.
 * @param[in]   level       - The slice corresponding to the energy value.
 * @return      none.
 */
void rf_cs_set_power_level_singletone(rf_power_level_e level);
/**
 * @brief       This function is mainly used to turn off the energy of the tone.
 * @return      none.
 * @note        After setting the tone energy with rf_set_power_level_singletone, you need to call
 *              rf_set_power_off_singletone to turn off the tone energy if you enter the send packet.
 */
void rf_cs_set_power_off_singletone(void);


/**
 * @brief       This function is mainly used for the disable hpmc trim function.
 * @return      none.
 */
void rf_cs_dis_hpmc_trim(void);

/**
 * @brief       This function is mainly used for the disable ldo trim function.
 * @return      none.
 */
void rf_cs_dis_ldo_trim(void);

/**
 * @brief       This function is mainly used for the disable dcoc trim function.
 * @return      none.
 */
void rf_cs_dis_dcoc_trim(void);

/**
 * @brief       This function is mainly used for the disable rccal trim function.
 * @return      none.
 */
void rf_cs_dis_rccal_trim(void);

/**
 * @brief       This function is mainly used for the disable fcal trim function.
 * @return      none.
 */
void rf_cs_dis_fcal_trim(void);

/**
 * @brief       This function is mainly used to initialize the parameters related to CS antennas.
 * @param[in]   clk_mode    - Set whether the antenna-related clock is always on or only when switching antennas.
 * @param[in]   ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.
 * @param[in]   ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @param[in]   ant_txoffset- Adjust the switching start point of the tx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @return      none.
 */
void rf_cs_ant_init(rf_cs_ant_clk_mode_e clk_mode,unsigned char ant_interval,unsigned char ant_rxoffset,unsigned char ant_txoffset);

/**
 * @brief       This function is mainly used to set the antenna switching interval.
 * @param[in]   ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.Value range 0x00~0x1ff.
 * @return      none.
 */
void rf_cs_set_ant_interval(unsigned short ant_interval);

/**
 * @brief       This function is mainly used to set the starting position of the antenna switching at the rx-side.
 * @param[in]   ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 *              Value range 0x00~0x1ff.
 * @return      none.
 */
void rf_cs_set_rx_ant_offset(unsigned short ant_rxoffset);

/**
 * @brief       This function is mainly used to set the starting position of the antenna switching at the tx-side.
 * @param[in]   ant_txoffset- Adjust the switching start point of the rx-side antenna,(ant_txoffset + 1)*0.125us.
 *              Value range 0x00~0x1ff.
 * @return      none.
 */
void rf_cs_set_tx_ant_offset(unsigned short ant_txoffset);

/**
 * @brief       This function is mainly used to set the clock working mode of the antenna.
 * @param[in]   clk_mode    - Open all the time or only when switching antennas.
 * @return      none.
 */
void rf_cs_ant_clk_mode(rf_cs_ant_clk_mode_e clk_mode);

void blc_set_default_antenna(u8 ant_index);

/**
 * @brief        This function is mainly used to get the timestamp information in the process of sending
 *                 and receiving packets; in the packet receiving stage, this register stores the sync moment
 *                 timestamp, and this information remains unchanged until the next sending and receiving packets.
 *                 In the send packet stage, the register stores the timestamp value of the tx_on moment, which
 *                 remains unchanged until the next send/receive packet.
 * @return        TX:timestamp value of the tx_on moment.
 *                 RX:timestamp value of the sync moment.
 */
static inline unsigned int rf_cs_get_timestamp(void)
{
    return reg_rf_timestamp;
}


/**
 * @brief        This function is mainly used to get the timestamp of the moment when tx_en is pulled up from the registers.
 * @return        The timestamp of the moment when tx_en is pulled up.
 */
static inline unsigned int rf_cs_get_tx_pos_timestamp(void)
{
    return reg_rf_tr_turnaround_pos_time;
}

/**
 * @brief        This function is mainly used to get the timestamp of the moment when tx_en is pulled down from the registers.
 * @return        The timestamp of the moment when tx_en is pulled down.
 */
static inline unsigned int rf_cs_get_tx_neg_timestamp(void)
{
    return reg_rf_tr_turnaround_neg_time;
}

/**
 * @brief        This function is mainly used to return the timestamp of the IQ sampling start point through the register.
 * @return        The timestamp of IQ sampling start point.
 */
static inline unsigned int rf_cs_get_iq_start_timestamp(void)
{
    return reg_rf_iqstart_tstamp;
}
#endif /* DRIVERS_B92_EXT_DRIVER_DRIVER_INTERNAL_EXT_HADM_RF_H_ */
