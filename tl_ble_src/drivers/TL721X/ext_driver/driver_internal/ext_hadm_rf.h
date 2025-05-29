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

#include "../../lib/include/rf/rf_common.h"
#include "common/types.h"

#define     SW_DCOC_EN                  RF_RX_DCOC_SOFTWARE_CAL_EN

//3us Tx ll delay
//2us tx path delay
//settle :TX_FAST_SETTLE_TIME + 1
#include "ext_rf.h"

#define     CS_TX_PATH_DELAY_1M             2
#define     CS_TX_PATH_DELAY_2M             1
#define     CS_TX_LL_DEALY_1M               3
#define     CS_TX_LL_DEALY_2M               2

#define     CS_TX_STL_BTX_1ST_PKT_REAL_1M   (CS_TX_LL_DEALY_1M + (TX_FAST_SETTLE_TIME+1) + CS_TX_PATH_DELAY_1M + 8*PRMBL_EXTRA_1M)//(110 - 3) //3 is total switch delay time
#define     CS_TX_STL_BTX_1ST_PKT_REAL_2M   (CS_TX_LL_DEALY_2M + (TX_FAST_SETTLE_TIME+1) + CS_TX_PATH_DELAY_2M + 8*PRMBL_EXTRA_1M)//(110 - 3) //3 is total switch delay time

#define     CS_AD_CONVERT_DLY_1M            (RF_RX_SHORT_MODE_EN ? 12:19)  //before:20. Jaguar T_IFS need 32M + AD_Convert=19, tested by kai.jia at 2022-11-17
#define     CS_AD_CONVERT_DLY_2M            10//todo?

#if     (SW_DCOC_EN)
#define     CS_OTHER_SWITCH_DELAY_1M        0
#define     CS_OTHER_SWITCH_DELAY_2M        3
#else
#define     CS_OTHER_SWITCH_DELAY_1M        0
#define     CS_OTHER_SWITCH_DELAY_2M        0
#endif
#define     CS_HW_DELAY_1M                  (CS_AD_CONVERT_DLY_1M + CS_OTHER_SWITCH_DELAY_1M)
#define     CS_HW_DELAY_2M                  (CS_AD_CONVERT_DLY_2M + CS_OTHER_SWITCH_DELAY_2M)

#define     CS_HW_DELAY_CS_1M               (CS_HW_DELAY_1M - 1)
#define     CS_HW_DELAY_CS_2M               (CS_HW_DELAY_2M)

////////////////////////////////////rf_cs.h end/////////////////////////////
#include "reg_include/rf_reg.h"
#include "driver.h"
#define    rf_aoa_aod_iq_data_offset(p)                    (p[5]+10)

/**
 * @brief    Take 4 antennas as an example to illustrate the antenna switching sequence.
 *             SWITCH_SEQ_MODE0    - antenna index switch sequence 01230123
 *             SWITCH_SEQ_MODE1    - antenna index switch sequence 0123210
 *             SWITCH_SEQ_MODE2    - antenna index switch sequence 001000200030
 */
typedef enum{
    SWITCH_SEQ_MODE0         = 0,
    SWITCH_SEQ_MODE1         = BIT(6),
    SWITCH_SEQ_MODE2         = BIT(7)
}rf_ant_pattern_e;

/**
 * @brief    Enumerated variables for CS's settle sequence mode on or off.
 */
typedef enum
{
    RF_CS_SETTLE_SEQ_OFF,//Turns off the settle timing in channel sounding mode and reverts to the normal settle sequence.
    RF_CS_SETTLE_SEQ_ON  //Enable settle sequence for use in channel sounding mode.
}rf_cs_settle_seq_mode_e;


/**
 * @brief    Enumerated variables for CS's IQ sampling start method.
 */
typedef enum
{
    RF_CS_IQ_SAMPLE_SYNC_MODE,//IQ sampling starts when the packet is synchronised.
    RF_CS_IQ_SAMPLE_RXEN_MODE //IQ sampling starts when rx_en is pulled up.
}rf_cs_iq_sample_mode_e;

typedef enum
{
    RF_CS_TX_ANT_SWITCH_TXON = 0x01,
    RF_CS_TX_ANT_SWITCH_PAPUP = 0x02
}rf_cs_tx_ant_mode_e;

/**
 * @brief    Initialize the structure used to control the antenna IO.
 */
typedef struct{
    gpio_func_pin_e        antsel0_pin;
    gpio_func_pin_e        antsel1_pin;
    gpio_func_pin_e     antsel2_pin;
}rf_ant_pin_sel_t;

/**
 * @brief    Enumerated variables for cs antenna clock mode.
 */
typedef enum
{
    RF_CS_ANT_CLK_ALWAYS_ON_MODE,//The clock keeps working while the cs is working.
    RF_CS_ANT_CLK_SWITCH_ON_MODE //Only during antenna switchover clock on.
}rf_cs_ant_clk_mode_e;


//typedef enum{
//    RF_RX_ACL_AOA_AOD_EN  = BIT(0),
//    RF_RX_ADV_AOA_AOD_EN  = BIT(1),
//    RF_TX_ACL_AOA_AOD_EN  = BIT(2),
//    RF_TX_ADV_AOA_AOD_EN  = BIT(3),
//    RF_AOA_AOD_OFF        = 0
//}rf_aoa_aod_mode_e;

/*
 * @brief   The chip can receive AOA/AOD packets in ADV or ACL format.
 * @note    Only one of the modes can be enabled.In the AOA/AOD mode, the packet format is different from the normal mode,
 *             so use rf_is_rx_right and rf_aoa_aod_is_rx_pkt_len_ok for crc and length verification.
 */
typedef enum{
    RF_RX_ACL_AOA_AOD_EN  = BIT(0),
    RF_RX_ADV_AOA_AOD_EN  = BIT(1),
    RF_AOA_AOD_RX_OFF     = 0
}rf_aoa_aod_rx_mode_e;

/*
 * @brief   The chip can send AOA/AOD packets in ADV or ACL format.
 * @note    Only one of the modes can be enabled.
 */
typedef enum{
    RF_TX_ACL_AOA_AOD_EN  = BIT(2),
    RF_TX_ADV_AOA_AOD_EN  = BIT(3),
    RF_AOA_AOD_TX_OFF     = 0
}rf_aoa_aod_tx_mode_e;

/*
 * @brief  Data length type of AOA/AOD sampling.
 * |                 |                         |
 * | :-------------- | :---------------------- |
 * |        <15:8>      |          <7:0>          |
 * |   IQ byte len   |   iq data bit num mode  |
 */
typedef enum{
    IQ_8_BIT_MODE           = 0x0200,
    IQ_16_BIT_MODE          = 0x0401,
    IQ_16_BIT_LOW_MODE      = 0x0402,
    IQ_16_BIT_HIGH_MODE     = 0x0403,
    IQ_20_BIT_MODE          = 0x0504
}rf_iq_data_mode_e;



/*
 * @brief  AOA/AOD sample interval time type enumeration.
 * @note   Attention:When the time is 0.25us, it cannot be used with the 20bit iq data type, which will cause the sampling data to overflow.
 *           In normal mode, the sampling interval of AOA is 4us, and AOD will judge whether the sampling interval is 4us or 2us according to
 *           CTE info.
 */
typedef enum{
    SAMPLE_NORMAL_INTERVAL       = 0,//In this case sample interval of aoa is 4us, and aod will judge sample interval is 4us or 2us according to CTE info.
    SAMPLE_2US_INTERVAL          = 3,
    SAMPLE_1US_INTERVAL          = 4,
    SAMPLE_0P5US_INTERVAL        = 5,
    SAMPLE_0P25US_INTERVAL       = 6
}rf_aoa_aod_sample_interval_time_e;

/**
 *  @brief  LDO trim calibration value
 */
typedef struct
{
    unsigned char LDO_CAL_TRIM;
    unsigned char LDO_RXTXHF_TRIM;
    unsigned char LDO_RXTXLF_TRIM;
    unsigned char LDO_PLL_TRIM;
    unsigned char LDO_VCO_TRIM;
}rf_cs_ldo_trim_t;

/**
 *  @brief  DCOC calibration value
 */
typedef struct
{
    unsigned char DCOC_IDAC;
    unsigned char DCOC_QDAC;
    unsigned char DCOC_IADC_OFFSET;
    unsigned char DCOC_QADC_OFFSET;
}rf_cs_dcoc_cal_t;

/**
 *  @brief  RCCAL calibration value
 */
typedef struct
{
    unsigned char RCCAL_CODE;
    unsigned char CBPF_CCODE_L;
    unsigned char CBPF_CCODE_H;
}rf_cs_rccal_cal_t;

typedef struct
{
    unsigned short cal_tbl[40];
    rf_ldo_trim_t    ldo_trim;
    rf_dcoc_cal_t   dcoc_cal;
    rf_rccal_cal_t  rccal_cal;
}rf_cs_fast_settle_t;

/**
 *  @brief  tx related calibration value in cs function.
 */
typedef struct {
    unsigned short      tx_hpmc;
    rf_ldo_trim_t ldo_trim;
} rf_cs_tx_cali_t;

/**
 *  @brief  rx related calibration value in cs function.
 */
typedef struct{
    rf_ldo_trim_t    ldo_trim;
#if (!SW_DCOC_EN)
    rf_dcoc_cal_t   dcoc_cal;
#endif
    rf_rccal_cal_t  rccal_cal;
}rf_cs_rx_cali_t;



/**
 * @brief    Define function to set tx channel or rx channel.
 */
typedef enum
{
    TX_CHANNEL        = 0,
    RX_CHANNEL        = 1,
}rf_trx_chn_e;

/**
 *  @brief        This function is mainly used to get LDO Calibration-related values.
 *  @param[in]    ldo_trim   - ldo trim calibration value address pointer
 *  @return         none
*/
void rf_cs_get_ldo_trim_val(rf_ldo_trim_t *ldo_trim);

/**
 *  @brief        This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return         none
*/
void rf_cs_set_ldo_trim_val(rf_ldo_trim_t ldo_trim);

/**
 *  @brief        This function is mainly used to get hpmc Calibration-related values.
 *  @param[in]    none
 *  @return         none
*/
_attribute_ram_code_sec_noinline_ unsigned short rf_cs_get_hpmc_cal_val(void);

/**
 *  @brief        This function is mainly used to set hpmc Calibration-related values.
 *  @param[in]  value  - hpmc Calibration-related values.
 *  @return         none
*/
_attribute_ram_code_sec_noinline_ void rf_cs_set_hpmc_cal_val(unsigned short value);

/**
 *  @brief        This function is mainly used to get LDO Calibration-related values.
 *  @param[in]    dcoc_cal   - dcoc calibration value address pointer
 *  @return         none
*/
void rf_cs_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal);

/**
 *  @brief        This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return         none
*/
void rf_cs_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal);


/**
 *  @brief        This function is mainly used to get rccal Calibration-related values.
 *  @param[in]    rccal_cal  - rccal calibration value address pointer
 *  @return         none
*/
void rf_cs_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal);

/**
 *  @brief        This function is mainly used to set rccal Calibration-related values.
 *  @param[in]    rccal_cal    - rccal Calibration-related values.
 *  @return         none
*/
void rf_cs_set_rccal_cal_val(rf_rccal_cal_t rccal_cal);

/**
 * @brief        This function is mainly used for the disable hpmc trim function.
 * @return        none.
 */
void rf_cs_dis_hpmc_trim(void);

/**
 * @brief        This function is mainly used for the disable ldo trim function.
 * @return        none.
 */
void rf_cs_dis_ldo_trim(void);

/**
 * @brief        This function is mainly used for the disable dcoc trim function.
 * @return        none.
 */
void rf_cs_dis_dcoc_trim(void);

/**
 * @brief        This function is mainly used for the disable rccal trim function.
 * @return        none.
 */
void rf_cs_dis_rccal_trim(void);

/**
 * @brief        This function is mainly used for the disable fcal trim function.
 * @return        none.
 */
void rf_cs_dis_fcal_trim(void);

/****************************************************************************************************************************************
 *                                         RF : AOA/AOD related functions                                                                     *
 ****************************************************************************************************************************************/

/**
 * @brief        This function is used to calibrate AOA, AOD sampling frequency offset.This function is mainly used to set the position
 *                 of iq data sampling point in sampleslot to optimize the sampling data result. By default, sampling is performed at the
 *                 middle position of iqsampleslot, and the sampling point is 0.125us ahead of time for each decrease of 1 code.
 *                 Each additional code will move the sampling point back by 0.125us
 * @param[in]    samp_locate:Compare the parameter with the default value, reduce one code to advance 0.125us, increase or decrease 1 to move
 *                             back 0.125us.
 * @return        none.
 */
static inline void rf_aoa_aod_sample_point_adjust(unsigned char samp_locate)
{
    reg_rf_samp_offset = samp_locate;
}


/**
 * @brief        This function is used to set the position of the first antenna switch after the reference.The default is in the middle of the
 *                 first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code.
 *                 Each additional code will move the switch point back by 0.125us
 * @param[in]    swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
 *                             back 0.125us.
 * @return        none.
 */
void rf_aoa_rx_ant_switch_point_adjust(unsigned short swt_offset);


/**
 * @brief        This function is used to set the position of the first antenna switch after the AOD transmitter reference.The default is in the middle of the
 *                 first switch_slot; and the switch point is 0.125us ahead of time for each decrease of 1 code. Each additional code will move
 *                 the switch point back by 0.125us
 * @param[in]    swt_offset : Compare the parameter with the default value, reduce 1 to advance 0.125us, increase or decrease 1 to move
 *                             back 0.125us.
 * @return        none.
 */
void rf_aod_tx_ant_switch_point_adjust(unsigned short swt_offset);


/**
 * @brief        This function is mainly used to set the IQ data sample interval time. In normal mode, the sampling interval of AOA is 4us, and AOD will judge whether
 *                 the sampling interval is 4us or 2us according to CTE info.The 4us/2us sampling interval corresponds to the 2us/1us slot mode stipulated in the protocol.
 *                 Since the current hardware only supports the antenna switching interval of 4us/2us, setting the sampling interval to 1us or less will cause multiple
 *                 sampling at the interval of one antenna switching. Therefore, the sampling data needs to be processed by the upper layer according to the needs, and
 *                 currently it is mostly used Used in the debug process.
 *                 After configuring RF, you can call this function to configure slot time.
 * @param[in]    time_us    - AOA or AOD slot time mode.
 * @return        none.
 * @note        Attention:(1)When the time is 0.25us, it cannot be used with the 20bit iq data type, which will cause the sampling data to overflow.
 *                           (2)Since only the antenna switching interval of 4us/2us is supported, the sampling interval of 1us and shorter time intervals
 *                               will be sampled multiple times in one antenna switching interval. Suggestions can be used according to specific needs.
 */
void rf_aoa_aod_sample_interval_time(rf_aoa_aod_sample_interval_time_e time_us);


/**
 * @brief        This function is mainly used to set the type of AOA/AODiq data. The default data type is 8bit. This configuration can be done before starting to receive
 *                 the package.
 * @param[in]    mode    - The length of each I or Q data.
 * @return        none.
 */
void rf_aoa_aod_iq_data_mode(rf_iq_data_mode_e mode);

/**
 * @brief        This function is mainly used to set the antenna switching mode. Vulture support three different
 *                 table lookup sequences.The setting here is just the order of the table lookup, and the content
 *                 in the table is the number of the antenna to be switched to.The switching sequence of the antenna
 *                 needs to be determined by the combination of the table look-up sequence and the antenna number in
 *                 the table,so this function is usually used together with the rf_aoa_aod_ant_lut function.
 * @param[in]    pattern     - Enumeration of several different look-up table order modes.Refer to the corresponding
 *                               enumeration annotation for the meaning of the mode.
 * @return        none.
 */
void rf_aoa_aod_ant_pattern(rf_ant_pattern_e pattern);

/**
 * @brief        This function is mainly used to set the number of antennas enabled by the multi-antenna board in the
 *                 AOA/AOD function;the vulture series chips currently support up to 8 antennas for switching.By default,
 *                 it is set to 8 antennas. After configuring the RF-related settings, you can set the number of enabled
 *                 antennas, and this setting needs to be completed before sending and receiving packets.
 * @param[in]    ant_num     - The number of antennas, the value ranges from 1 to 8.
 * @return        none.
 */
void rf_aoa_aod_set_ant_num(unsigned char ant_num);


/**
 * @brief        This function is mainly used to initialize the parameters related to AOA/AOD antennas, including the
 *                 number of antennas, the pins for controlling the antennas,the look-up mode of antenna switching, and
 *                 the content of the antenna switching sequence table.
 * @param[in]    ant_num            - The number of antennas, the value ranges from 1 to 8.
 * @param[in]    ant_pin_config:    - Control antenna pin selection and configuration.The parameter setting needs to be
 *                                   set according to the number and position of the control antenna.For example,if you
 *                                   need to control four antennas, it is best to use Antsel0 and Antsel2.
 * @param[in]    pattern            - Enumeration of several different look-up table order modes.
 * @param[in]    dat             - The antenna value written into the antenna switching sequence table ranges from 0 to 7.
 * @return        none.
 */
void rf_aoa_aod_ant_init(unsigned char num,rf_ant_pin_sel_t * ant_pin_config,rf_ant_pattern_e pattern,unsigned int dat);


/****************************************************************************************************************************************
 *                                         RF : Channel Sounding related functions                                                                     *
 ****************************************************************************************************************************************/

/**
 * @brief        This function is mainly used to initialize some parameter settings of the CS IQ sample.
 * @param[in]    samp_num    - Number of groups to sample IQ data.Value range 0x00~0xffff.
 * @param[in]    interval    - The interval time between each IQ sampling is (interval + 1)*0.125us.
 *                               Value range 0x01~0x0f.
 * @param[in]    start_point    - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                               at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                               starts at (start_point + 1) * 0.125us after sync.Value range 0x00~0xff.
 * @param[in]    suppmode    - The length of each I or Q data.
 * @param[in]    sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return        none.
 */
void rf_cs_iq_sample_init(unsigned short samp_num,unsigned char interval,unsigned char start_point,rf_iq_data_mode_e suppmode,rf_cs_iq_sample_mode_e sample_mode);


/**
 * @brief        This function is mainly used to set the IQ sample interval.
 * @param[in]    interval     - Set the interval for IQ sample, (interval + 1)*0.125us.Value range 0x01~0x0f.
 * @return        none.
 * @note         Sampling frequency = 1/sampling interval, so the maximum sampling frequency is 4MHz.
 */
void rf_cs_sample_interval_time(unsigned char interval);

/**
 * @brief        This function is mainly used to initialize the parameters related to CS antennas.
 * @param[in]    clk_mode    - Set whether the antenna-related clock is always on or only when switching antennas.
 * @param[in]    ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.
 * @param[in]    ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @param[in]    ant_txoffset- Adjust the switching start point of the tx-side antenna,(ant_rxoffset + 1)*0.125us.
 * @return        none.
 */
void rf_cs_ant_init(rf_cs_ant_clk_mode_e clk_mode,unsigned char ant_interval,unsigned char ant_rxoffset,unsigned char ant_txoffset);

/**
 * @brief        This function is mainly used to set the antenna switching interval.
 * @param[in]    ant_interval- Set the interval for antenna switching, (interval + 1)*0.125us.Value range 0x00~0x1ff.
 * @return        none.
 */
void rf_cs_set_ant_interval(unsigned short ant_interval);

/**
 * @brief        This function is mainly used to set the starting position of the antenna switching at the rx-side.
 * @param[in]    ant_rxoffset- Adjust the switching start point of the rx-side antenna,(ant_rxoffset + 1)*0.125us.
 *                 Value range 0x00~0x1ff.
 * @return        none.
 */
void rf_cs_set_rx_ant_offset(unsigned short ant_rxoffset);

/**
 * @brief        This function is mainly used to set the starting position of the antenna switching at the tx-side.
 * @param[in]    ant_txoffset- Adjust the switching start point of the rx-side antenna,(ant_txoffset + 1)*0.125us.
 *                 Value range 0x00~0x1ff.
 * @return        none.
 */
void rf_cs_set_tx_ant_offset(unsigned short ant_txoffset);

/**
 * @brief        This function is mainly used to set the clock working mode of the antenna.
 * @param[in]    clk_mode    - Open all the time or only when switching antennas.
 * @return        none.
 */
void rf_cs_ant_clk_mode(rf_cs_ant_clk_mode_e clk_mode);

/**
 * @brief        This function is mainly used to set the way IQ sampling starts.
 * @param[in]    sample_mode    - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return        none.
 */
void rf_cs_iq_sample_mode(rf_cs_iq_sample_mode_e sample_mode);

/**
 * @brief        This function is mainly used to set the starting position of IQ sampling.
 * @param[in]    start_point  - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                               at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                               starts at (start_point + 1) * 0.125us after sync.value range is 0x00~0xff.
 *                               The rx_en mode and sync mode can be configured with the function rf_cs_iq_sample_mode.
 * @return        none.
 */
void rf_cs_iq_start_point(unsigned char pos);

/**
 * @brief        This function is mainly used to set the number of IQ samples in groups.
 * @param[in]    samp_num    - Number of groups to sample IQ data.Value range 0x00~0xffff.
 * @return        none.
 */
void rf_cs_iq_sample_number(unsigned short samp_num);


/**
 * @brief        This function is mainly used to enable the IQ sampling function.
 * @return        none.
 */
void rf_cs_iq_sample_enable(void);

/**
 * @brief        This function is mainly used to disable the IQ sampling function.
 * @return        none.
 */
void rf_cs_iq_sample_disable(void);

/**
 * @brief        This function is mainly used to obtain the sync flag bit from the packet, which is
 *                 used to identify whether the packet is data received after passing sync.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the Sync flag information in the packet.
 */
unsigned char rf_cs_get_pkt_sync_flag(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to obtain the packet quality indicator from the packet,which is
 *                 used to identify the quality of the received packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the packet quality information in the packet.0:very good;1:good;2:bad;
 */
unsigned char rf_cs_get_packet_quality_indicator(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to get the timestamp of the sync-to-packet moment from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the Sync timestamp information in the packet.
 */
unsigned int rf_cs_get_pkt_rx_sync_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to get the timestamp of the moment tx_en is pulled up from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment tx_en is pulled up in the packet.
 */
unsigned int rf_cs_get_pkt_tx_pos_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to get the timestamp of the moment tx_en is pulled down from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information  of the moment tx_en is pulled down in the packet.
 */
unsigned int rf_cs_get_pkt_tx_neg_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to get the timestamp of the moment when the IQ data collection starts from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment when the IQ data collection starts in the packet.
 */
unsigned int rf_cs_get_pkt_iq_start_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to get the timestamp of the moment when the IQ data collection starts from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment when the IQ data collection starts in the packet.
 */
unsigned int rf_cs_get_pkt_tx_frac_pos_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to get the timestamp of the moment when the IQ data collection starts from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the timestamp information of the moment when the IQ data collection starts in the packet.
 */
unsigned int rf_cs_get_pkt_tx_frac_neg_timestamp(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief        This function is mainly used to obtain the rssi information from the packet.
 * @param[in]    p            - The packet address.
 * @param[in]    sample_num    - The number of sample points that the packet contains.
 * @param[in]    data_len    - The data length of the sample point in the packet.
 * @return        Returns the rssi information in the packet.
 */
signed char rf_cs_get_pkt_rssi_value(unsigned char *p,unsigned short sample_num,rf_iq_data_mode_e data_len);

/**
 * @brief       This function serves to set RF's channel.The step of this function is in KHz.
 *                The frequency set by this function is (chn+2400) MHz+chn_k KHz.
 * @param[in]   chn_m - RF channel. The unit of this parameter is MHz, and its set frequency
 *                          point is (2400+chn)MHz.
 * @param[in]   chn_k - The unit of this parameter is KHz, which means to shift chn_k KHz to
 *                         the right on the basis of chn.Its value ranges from 0 to 999.
 * @param[in]    trx_mode - Defines the frequency point setting of tx mode or rx mode.
 * @return      none.
 */
_attribute_ram_code_sec_ void rf_set_channel_k_step(signed char chn_m,unsigned int chn_k,rf_trx_chn_e trx_mode);//general


/**
 * @brief        This function is mainly used to switch off the agc auto-adjustment function and keep the current
 *                 agc gain value.
 * @return        none.
 * @note        This function should be called after a wanted signal has been received and followed by an input
 *                 signal energy that is consistent with the earlier wanted signal.
 */
void rf_agc_disable(void);

/**
 * @brief        This function is mainly used for agc auto run.
 * @return        none.
 * @note        Call this function to enable agc auto tuning if you want to receive different energy packets correctly
 *                 after calling rf_agc_disable to disable agc auto tuning..
 */
void rf_agc_enable(void);

/**
 * @brief        This function is mainly used to set the sequence related to Fast Settle in CS.
 * @return        none.
 * @note        This function needs to be called after rf_agc_disable, otherwise there will be problems with rssi
 *                 value exceptions.
 */
void rf_cs_set_phase_continuous(void);

/**
 * @brief        This function is mainly used to turn off the energy of the tone.
 * @return        none.
 * @note        After setting the tone energy with rf_set_power_level_singletone, you need to call
 *                 rf_cs_set_power_off_singletone to turn off the tone energy if you enter the send packet.
 */
void rf_cs_set_power_off_singletone(void);

/**
 * @brief        This function is mainly used to enable the CS extension function of the hd_info.
 * @return        none.
 */
static inline void rf_cs_hd_info_enable(void)
{
    reg_rf_mode_ctrl0 |= FLD_RF_INFO_EXTENSION;
}

/**
 * @brief        This function is mainly used to disable the CS extension function of the hd_info.
 * @return        none.
 */
static inline void rf_cs_hd_info_disable(void)
{
    reg_rf_mode_ctrl0 &= (~FLD_RF_INFO_EXTENSION);
}

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

/**
 * @brief        This function is mainly used to get the timestamp of the moment when tx_en is pulled up from the registers.
 * @return        The timestamp of the moment when tx_en is pulled up.
 */
static inline unsigned int rf_cs_get_tx_frac_pos_timestamp(void)
{
    return reg_rf_tx_frac_time_pose_time;
}

static inline void rf_cs_set_guard_time(unsigned char guard_time)
{
    write_reg8(0x170140,(read_reg8(0x170140)&0x0f)|((guard_time&0x0f)<<4));
}

/**
 * @brief        This function is mainly used to set the preparation and enable of manual fcal(frequency calibration).
 * @return        none.
 */
void rf_manual_fcal_start(void);

/**
 * @brief        This function is mainly used to set the relevant value after manual fcal(frequency calibration).
 * @return        none.
 * @note        The function needs to be called after the rf_manual_fcal_start call 22us.
 */
void rf_manual_fcal_done(void);

/**
 * @brief        This function is mainly used to get the calibration value of the rx state that needs to be
 *                 recorded in the cs function.
 * @param[out]    rx_cali    -    Pointer to a structure that stores the value associated with the rx calibration.
 * @return        none.
 * @note        This function is usually called after a package has been received.
 */
void rf_cs_get_rx_cali_vlue(rf_cs_rx_cali_t *rx_cali);

/**
 * @brief        This function is mainly used to get the calibration value of the tx state that needs to be
 *                 recorded in the cs function.
 * @param[out]    rx_cali    -    Pointer to a structure that stores the value associated with the tx calibration.
 * @return        none.
 * @note        This function is usually called after a package has been sent.
 */
void rf_cs_get_tx_cali_vlue(rf_cs_tx_cali_t *tx_cali);

/**
 * @brief        This function is mainly used to write the calibration value obtained through the rf_cs_get_rx_cali_vlue
 *                 function to the corresponding register.
 * @param[in]    rx_cali        -    rx calibration value obtained by the rf_cs_get_rx_cali_vlue function.
 * @return        none.
 */
void rf_cs_set_rx_cali_vlue(rf_cs_rx_cali_t rx_cali);

/**
 * @brief        This function is used to write the tx calibration value obtained by rf_cs_get_tx_cali_vlue to the
 *                 corresponding register.
 * @param[in]    tx_cali        -    tx calibration value obtained by the rf_cs_get_tx_cali_vlue function.
 * @return        none.
 */
void rf_cs_set_tx_cali_vlue(rf_cs_tx_cali_t tx_cali);

/**
 * @brief        This function is mainly used to enable the rx-related trim functions that are bypassed during channel sounding.
 * @param[in]    phase_en : Used to control whether the digital IF is continuous or not.1:Maintaining continuity;0:No longer continuous.
 * @return        none.
 */
void rf_cs_restore_cali_auto_run(unsigned char phase_en);

/**
 * @brief        This function is used to enable or disable the corresponding sequence of shuttle in channel sounding mode; usually call
 *                 this function before entering mode1/mode2 and pass the parameter RF_CS_SETTLE_SEQ_ON to enable the corresponding sequence;
 *                 and call the parameter RF_CS_SETTLE_SEQ_OFF to disable the sequence after ending channel sounding.
 * @param[in]    on_off : Used to control whether to enable settle sequence in channel sounding mode.RF_CS_SETTLE_SEQ_OFF:off,RF_CS_SETTLE_SEQ_ON:on
 * @return        none.
 */
void rf_cs_settle_sequence_mode(rf_cs_settle_seq_mode_e on_off);

//void rf_cs_send_pkt_tone(void* addr,  unsigned int tick, unsigned int time_pkt_to_tone);

void rf_cs_send_tone_pkt(void* addr,  unsigned int tick, unsigned int time_pkt_to_tone);

void rf_cs_send_pkt_tone(void* addr,  unsigned int tick, unsigned int time_pkt_to_tone);

void rf_cs_txant_switch_mode(rf_cs_tx_ant_mode_e mode);

void rf_cs_rxant_switch_on(void);

void rf_cs_rxant_switch_off(void);

void rf_cs_ant_switch_auto(void);

void rf_cs_ant_switch_manual(void);

/**
 * @brief        This function is used to set the position of the first switch after the reference in aoa mode.The default
 *                 is in the middle of the first sample_slot; The starting position of sampling can be fine-tuned through
 *                 parameter setting to select the best sampling point.
 * @param[in]    switch_point_offset - sample_point_offset:The parameter range is -45 to 210.If the parameter is negative,
 *                 the position of the sampling point moves forward. The absolute value of the parameter is multiplied by
 *                 0.125us.If the parameter is positive, the position of the sampling point moves backward. The parameter
 *                 is multiplied by 0.125us.
 * @return        none.
 */
static inline void rf_aoa_ant_switch_point_adjust(char switch_point_offset)
{
    reg_rf_rx_antoffset = 0x86 + switch_point_offset;
}

/**
 * @brief        This function is used to set the position of the first switch after the reference in aod mode.The default
 *                 is in the middle of the first sample_slot; The starting position of sampling can be fine-tuned through
 *                 parameter setting to select the best sampling point.
 * @param[in]    switch_point_offset - sample_point_offset:The parameter range is -116 to 139.If the parameter is negative,
 *                 the position of the sampling point moves forward. The absolute value of the parameter is multiplied by
 *                 0.125us.If the parameter is positive, the position of the sampling point moves backward. The parameter
 *                 is multiplied by 0.125us.
 * @return        none.
 */
static inline void rf_aod_ant_switch_point_adjust(char switch_point_offset)
{
    reg_rf_tx_antoffset = 0x74 + switch_point_offset;
}

/**
 * @brief        This function enables the receiving functions of AOA/AOD in ordinary format packets or ADV format
 *                 packets.After configuring the RF function, if you want to receive a packet with AOA/AOD information,
 *                 you can call this function to make the chip enter the corresponding mode to receive the packet. The
 *                 default state is a normal package without AOA/AOD information.
 * @param[in]    mode - AOA/AOD broadcast package or normal package rx mode.When the parameter is RF_AOA_AOD_RX_OFF,
 *                 the normal packet without AOA/D is received.
 * @return        none.
 */
static inline void rf_aoa_aod_set_rx_mode(rf_aoa_aod_rx_mode_e mode)
{
    reg_rf_rxsupp = ((reg_rf_rxsupp & 0xfc)|mode);
}

/**
 * @brief        This function enables the sending functions of AOA/AOD in ordinary format packets or ADV format
 *                 packets.After configuring the RF function, if you want to send  a packet with AOA/AOD information,
 *                 you can call this function to make the chip enter the corresponding mode to send the packet. The
 *                 default state is a normal package without AOA/AOD information.
 * @param[in]    mode - AOA/AOD broadcast package or normal package tx mode.When the parameter is RF_AOA_AOD_RX_OFF,
 *                 the normal packet without AOA/D is sending.
 * @return        none.
 */
static inline void rf_aoa_aod_set_tx_mode(rf_aoa_aod_tx_mode_e mode)
{
    reg_rf_rxsupp = ((reg_rf_rxsupp & 0xf3)|mode);
}

/**
 * @brief        This function is used to calculate the number of IQ groups in the received AOA/AOD packet.
 * @param[in]    p                - Received packet address pointer.
 * @return        Returns the number of groups of iq in the package.
 */
unsigned int rf_aoa_aod_iq_group_number(unsigned char *p);

/**
 * @brief        This function is mainly used to obtain the CRC value in the AOA/AOD packet.
 * @param[in]    p                - Received packet address pointer.
 * @return        The return value is the rssi value in headerinformation.
 */
signed char rf_aoa_aod_get_pkt_rssi(unsigned char *p);
/**
 * @brief          This function is mainly used to set the energy when sending a single carrier.
 * @param[in]    level        - The slice corresponding to the energy value.
 * @return         none.
 */
void rf_cs_set_power_level_singletone(rf_power_level_e level);

////////////////////////////////////rf_cs.h end/////////////////////////////

#ifndef HADM_PHASE_CONTINUITY
    #define HADM_PHASE_CONTINUITY       1
#endif

#if (HADM_PHASE_CONTINUITY)
    extern _attribute_data_retention_ rf_cs_tx_cali_t tx_cs_cali;
    extern _attribute_data_retention_ rf_cs_rx_cali_t rx_cs_cali;
    extern _attribute_data_retention_ unsigned char cs_phase_continuity_flag;

    void ble_rf_cs_phase_continuity_en(void);
    void ble_rf_cs_phase_continuity_dis(unsigned char phase_en);
    void ble_rf_cs_set_tx_cali_value(rf_cs_tx_cali_t *tx_cali);
#endif

#define     reg_rf_ll_irq_list_h        REG_ADDR8(REG_BB_LL_BASE_ADDR+0x4d)

/**
 *  @brief  set the modulation index.
 */
typedef enum {
    RF_MI_P0p00 = 0,          /**< MI = 0 */      ///< RF_MI_P0p00
    RF_MI_P0p076 = 76,          /**< MI = 0.076 *////< RF_MI_P0p076
    RF_MI_P0p32 = 320,          /**< MI = 0.32 */ ///< RF_MI_P0p32
    RF_MI_P0p50 = 500,          /**< MI = 0.5 */  ///< RF_MI_P0p50
    RF_MI_P0p60 = 600,          /**< MI = 0.6 */  ///< RF_MI_P0p60
    RF_MI_P0p70 = 700,          /**< MI = 0.7 */  ///< RF_MI_P0p70
    RF_MI_P0p80 = 800,          /**< MI = 0.8 */  ///< RF_MI_P0p80
    RF_MI_P0p90 = 900,          /**< MI = 0.9 */  ///< RF_MI_P0p90
    RF_MI_P1p20 = 1200,     /**< MI = 1.2 */      ///< RF_MI_P1p20
    RF_MI_P1p30 = 1300,     /**< MI = 1.3 */      ///< RF_MI_P1p30
    RF_MI_P1p40 = 1400,     /**< MI = 1.4 */      ///< RF_MI_P1p40
}rf_mi_value_e;

/**
 * @brief   select baseband transmission unit
 */
typedef enum{
    RF_WORLD_WIDTH  = 0,
    RF_DWORLD_WIDTH = 1,
    RF_QWORLD_WIDTH = 2,
}rf_trans_unit_e;

/**
 * @brief       This function is used to get the value of the agc gain latch.
 * @return      Returns the value of gain latch.
 */
__INLINE unsigned char rf_get_gain_lat_value(void)
{
    return (reg_rf_modem_gain_lat0 & 0x07);
}


void ble_rf_channel_sounding_init(void);

void ble_rf_channel_sounding_deinit(void);

void ble_rf_channel_sounding_iq_sample_config(unsigned short sample_num, unsigned char start_point, rf_cs_iq_sample_mode_e sample_mode);

void ble_rf_set_manual_tx_mode(void);

void ble_rf_set_tx_modulation_index(rf_mi_value_e mi_value);

void ble_rf_set_cs_channel(signed char chn);

void ble_rf_set_accessCodeThreshold(u8 threshold);


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

void blc_set_default_antenna(u8 ant_index);

#endif /* DRIVERS_B92_EXT_DRIVER_DRIVER_INTERNAL_EXT_HADM_RF_H_ */
