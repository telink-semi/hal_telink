/********************************************************************************************************
 * @file    ext_rf.c
 *
 * @brief   This is the source file for BLE SDK
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
#include "tl_common.h"
#include "drivers.h"
#include "ext_lib.h"
#include "ext_rf.h"
#include "stack/ble/controller/ble_controller.h"


extern unsigned char        s_dcoc_software_cal_en;
extern unsigned short       g_rf_dcoc_iq_code;
extern void rf_rx_dcoc_cali_by_sw(void);                    //use extern to avoid modifying Driver rf.h
extern void rf_set_dcoc_iq_code(unsigned short iq_code);    //use extern to avoid modifying Driver rf.h
extern void rf_set_dcoc_iq_offset(signed short iq_offset);  //use extern to avoid modifying Driver rf.h

volatile unsigned int TXADDR = 0xc0013000;

#define   BLE_TXDMA_DATA        (0x170000 + 0x84)      //0x170084
#define   BLE_RXDMA_DATA        (0x170000 + 0x80)      //0x170080

_attribute_data_retention_sec_ signed char ble_txPowerLevel = 0; /* <<TX Power Level>>: -127 to +127 dBm */

//RF BLE Minimum TX Power LVL (unit: 1dBm)
const char  ble_rf_min_tx_pwr   = -23; /* -23dBm */
//RF BLE Maximum TX Power LVL (unit: 1dBm)
const char  ble_rf_max_tx_pwr   = 9;   /*  +9dBm */
//RF BLE Current TX Path Compensation (s16: -1280 ~ 1280, unit: 0.1 dB)
_attribute_data_retention_  signed short ble_rf_tx_path_comp = 0;
//RF BLE Current RX Path Compensation (s16: -1280 ~ 1280, unit: 0.1 dB)
_attribute_data_retention_  signed short ble_rf_rx_path_comp = 0;

//Current RF RX DMA buffer point for BLE
_attribute_data_retention_ unsigned char *ble_curr_rx_dma_buff = NULL;

_attribute_data_retention_ ext_rf_t blt_extRF;

void ble_rf_set_tx_dma(unsigned char fifo_dep,unsigned char size_div_16)//rf_set_tx_dma
{
    unsigned short fifo_byte_size = size_div_16<<4;
    rf_set_tx_dma_config();
    rf_set_tx_dma_fifo_num(fifo_dep);
    rf_set_tx_dma_fifo_size(fifo_byte_size);
}

_attribute_ram_code_
void ble_rf_set_rx_dma(unsigned char *buff, unsigned char size_div_16)//rf_set_rx_dma
{
    rf_set_rx_dma_config();
    unsigned short fifo_byte_size = size_div_16<<4;
    ble_curr_rx_dma_buff = buff;
    rf_set_rx_buffer(buff);
    reg_rf_rx_wptr_mask = 0;
    rf_set_rx_dma_fifo_size(fifo_byte_size);
}

void ble_rx_dma_config(void){
    //rf_set_rx_dma_config();
}


_attribute_ram_code_
void rf_mode_optimize_init(void) //rf_mode_init
{
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    reg_rst4 |= FLD_RST4_ZB;
    reg_clk_en4 |= FLD_CLK4_ZB_EN;

    reg_n22_rst    = 0xfffe; //reset dma_bb,zb_pon,zb,rstl_stimer,rst_modem,rstl_bb(bb:baseband)
    reg_n22_clk_en = 0xffff; //enable dma_bb,zb_hclk,clk_bb,clkzb32k_lp clock
    reg_bb_timer_ctrl |= FLD_BB_TIMER_TIMER_EN;

    //BLE SDK use
    //systime and bbtime are synchronized manually
    extern unsigned int  ext_BBTimerTick_BeforeSleep;  //in ext_pm.c
    extern unsigned int  ext_STimerTick_BeforeSleep; //in ext_pm.c
    reg_bb_timer_tick = (reg_system_tick - ext_STimerTick_BeforeSleep)/3 + ext_BBTimerTick_BeforeSleep;  //bbtime is 8M ,STIME is 24m

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
        extern void rf_tx_power_mode(rf_tx_power_e tx_power_mode);
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

#if BLC_PM_EN
_attribute_ram_code_
#endif
void rf_drv_ble_init(void)
{
    rf_mode_optimize_init();
    rf_ble_set_1m_phy();

    //rf_set_crc_config(&rf_crc_config[0]);
    rf_set_crc_init_value(0x555555);
    rf_set_crc_poly(0x0000065b);
    rf_set_crc_xor_out(0);
    rf_set_crc_byte_order(1);
    rf_set_crc_start_cal_byte_pos(0);
    rf_set_crc_len(3);
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   3. setting for BLE by BLE_Team
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //access code, can save
//  write_reg32(0x170008,0x00000000);   //default:0xf8118ac9;
    write_reg8(0x170030, 0x36);         //default:0x3c;disable tx timestamp en, add by LiBiao

//  write_reg8(0x80170206, 0x00);       //LL_RXWAIT, default 0x0009
//  write_reg8(0x8017020c, 0x50);       //LL_RXSTL   default 0x0095
//  write_reg8(0x8017020e, 0x00);       //LL_TXWAIT, default 0x0009
//  write_reg8(0x80170210, 0x00);       //LL_ARD,    default 0x0063


}

#if RF_THREE_CHANNEL_CALIBRATION
_attribute_data_retention_ unsigned char rf_channel_power[40];
_attribute_data_retention_ unsigned char channel_power_calibration_enable = 0;

_attribute_ram_code_ //must be RamCode
void ble_rf_set_chn_power(signed char  chn_num)
{
    unsigned char value = (unsigned char)(rf_channel_power[chn_num] & 0x3F);
    reg_rf_mode_cfg_txrx_0 = ((reg_rf_mode_cfg_txrx_0 & 0x7f) | ((value&0x01)<<7));
    reg_rf_mode_cfg_txrx_1 = ((reg_rf_mode_cfg_txrx_1 & 0xe0) | ((value>>1)&0x1f));
}

/**
 *  @brief      this function serve to set the TX power calibration.
 *  @param[in]  channel_power: channel power calibration of 40 channel.
 *  @return     none
*/
void rf_set_channel_power_calibration(unsigned char *channel_power)
{
    memcpy(rf_channel_power,channel_power,40);
}

/**
 *  @brief      this function serve to enable the rx timing sequence adjusted.
 *  @param[in]  enable: channel power calibration enable or disable.
 *  @return     none
*/
void rf_set_channel_power_enable(unsigned char enable)
{
    channel_power_calibration_enable = enable;
}
#endif

/**
 * @brief       This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return      none.
 */
_attribute_ram_code_ //must be RamCode
void rf_set_ble_channel (signed char chn_num)
{
    signed char ble_chn_num = 0;
    write_reg8 (0x170020, chn_num);

    if (chn_num < 11)
        ble_chn_num = chn_num + 2;

    else if (chn_num < 37)
        ble_chn_num = chn_num + 3;

    else if (chn_num == 37)
        ble_chn_num = 1;

    else if (chn_num == 38)
        ble_chn_num = 13;

    else if (chn_num == 39)
        ble_chn_num = 40;
#if RF_THREE_CHANNEL_CALIBRATION
    if(channel_power_calibration_enable)
    {
        ble_rf_set_chn_power(ble_chn_num - 1);
    }
#endif
    ble_chn_num = ble_chn_num << 1;
    rf_set_chn(ble_chn_num);

}

_attribute_ram_code_
void rf_start_fsm (fsm_mode_e mode, void* tx_addr, unsigned int tick)
{
    unsigned int r = core_interrupt_disable(); //prevent user IRQ priority 3 task destroying FSM setting

    unsigned int cur_stimer_tick = clock_time();
    unsigned int cur_bbtimer_tick = bb_clock_time();

    unsigned int diff_tick = tick - cur_stimer_tick;
    if((unsigned int)(diff_tick - 2*SYSTEM_TIMER_TICK_1US) < BIT(30)){
        cur_bbtimer_tick = (cur_bbtimer_tick + (diff_tick/3));
    }

    reg_rf_ll_cmd_schedule = cur_bbtimer_tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = mode;

    if(tx_addr){
        rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(tx_addr));
    }

    core_restore_interrupt(r);
}


// todo here need to optimize_Bool
_attribute_ram_code_
_Bool ll_resolvPrivateAddr(u8 *irk, u8 *addr, u8 irk_num)
{
    reg_cv_llbt_hash_status = FLD_CV_RPASE_STATUS_CLR; //clear flag
    reg_cv_llbt_rpase_cntl =(FLD_CV_PRASE_ENABLE) ; //disable RPA

    //set irk
    reg_cv_llbt_irk_ptr = (unsigned int)irk;


    //set prand and irk num = 1
    reg_cv_llbt_rpase_cntl |= (addr[3] | (addr[4]<<8) | (addr[5]<<16) | ((irk_num-1)<<24));

    reg_cv_llbt_hash_status |= (addr[0] | (addr[1]<<8) | (addr[2]<<16));


    DBG_CHN4_HIGH;
    reg_cv_llbt_rpase_cntl |= FLD_CV_RPASE_START;

    while(!((reg_cv_llbt_hash_status&0x60000000) == 0x40000000));

    DBG_CHN4_LOW;
    return (reg_cv_llbt_hash_status&FLD_CV_HASH_MATCH? 1:0);
}



// todo here need to optimize
u8 ll_getRpaAddr(u8 *irk, u8 prand[3], u8 rpa[6])
{
    reg_cv_llbt_hash_status |= FLD_CV_RPASE_STATUS_CLR; //clear flag
    reg_cv_llbt_rpase_cntl =(FLD_CV_PRASE_ENABLE | FLD_CV_GEN_RES ) ; //disable RPA

    //set irk
    reg_cv_llbt_irk_ptr = (unsigned int)irk;

    //set prand and irk num
    prand[2] = (prand[2] & 0x3F) | 0x40;
    reg_cv_llbt_rpase_cntl |= prand[0] | (prand[1]<<8) | (prand[2]<<16);

    //set RPA_GEN, enable and start RPA
    reg_cv_llbt_rpase_cntl |= FLD_CV_RPASE_START;


    while(!((reg_cv_llbt_hash_status&0x60000000) == 0x40000000));

    u32 hash = reg_cv_llbt_hash_status;

    rpa[0] = hash & 0xff;
    rpa[1] = (hash>>8)&0xff;
    rpa[2] = (hash>>16)&0xff;

    memcpy(&rpa[3], prand, 3);

    return 1;
}


#if FAST_SETTLE

_attribute_data_retention_ Fast_Settle fast_settle_1M;
_attribute_data_retention_ Fast_Settle fast_settle_2M;
_attribute_data_retention_ Fast_Settle fast_settle_S2;
_attribute_data_retention_ Fast_Settle fast_settle_S8;


#endif

typedef struct {
    s8                      tx_power_level;
    rf_power_level_index_e  tx_power_index;
}ble_txPowerTbl_t;

ble_txPowerTbl_t ext_rfPwrLvlIdx_buf[] = {
//         /*VBAT*/
//        // {8,      RF_POWER_INDEX_P8p58dBm     },/**<   dbm */
//         {8,        RF_POWER_INDEX_P7p96dBm     },/**<   dbm */
//        // {7,      RF_POWER_INDEX_P7p51dBm     },/**<   dbm */
//        // {7,      RF_POWER_INDEX_P7p25dBm     },/**<   dbm */
//         {7,        RF_POWER_INDEX_P7p00dBm     },/**<   dbm */
//         //{6,      RF_POWER_INDEX_P6p74dBm     },/**<   dbm */
//         {6,        RF_POWER_INDEX_P6p46dBm     },/**<   dbm */
//        // {5,      RF_POWER_INDEX_P5p90dBm     },/**<   dbm */
//        // {5,      RF_POWER_INDEX_P5p27dBm     },/**<   dbm */
//         {5,        RF_POWER_INDEX_P4p96dBm     },/**<   dbm */
//        // {4,      RF_POWER_INDEX_P4p60dBm     },/**<   dbm */
//         {4,        RF_POWER_INDEX_P4p24dBm     },/**<   dbm */
//        // {3,      RF_POWER_INDEX_P3p84dBm     },/**<   dbm */
//        // {3,      RF_POWER_INDEX_P3p42dBm     },/**<   dbm */
        {3,        RF_POWER_INDEX_P3p04dBm     },/**<   dbm */
//        // {2,      RF_POWER_INDEX_P2p52dBm     },/**<   dbm */
//        // { 2,     RF_POWER_INDEX_P2p16dBm     },  /**<   dbm */
//         { 2,       RF_POWER_INDEX_P2p03dBm     },  /**<   dbm */
//        // { 1,     RF_POWER_INDEX_P1p48dBm     },  /**<   dbm */
//         { 1,       RF_POWER_INDEX_P1p02dBm     },  /**<   dbm */
//        // { 0,     RF_POWER_INDEX_P0p67dBm     },  /**<   dbm */
//        // { 0,     RF_POWER_INDEX_P0p42dBm     },  /**<   dbm */
//        // { 0,     RF_POWER_INDEX_P0p17dBm     },  /**<   dbm */
         { 0,       RF_POWER_INDEX_P0p03dBm     },  /**<   dbm */
//        // {-0,     RF_POWER_INDEX_N0p24dBm     },  /**<   dbm */
//         //{-0,     RF_POWER_INDEX_N0p41dBm     },  /**<   dbm */
//         //{-0,     RF_POWER_INDEX_N0p73dBm     },  /**<   dbm */
//         {-1,       RF_POWER_INDEX_N1p09dBm     },  /**<   dbm */
//        // {-1,     RF_POWER_INDEX_N1p47dBm     },  /**<   dbm */
//         {-2,       RF_POWER_INDEX_N2p11dBm     },  /**<   dbm */
//        // {-2,     RF_POWER_INDEX_N2p58dBm     },  /**<   dbm */
//         {-3,       RF_POWER_INDEX_N3p10dBm     },  /**<   dbm */
//        // {-3,     RF_POWER_INDEX_N3p41dBm     },  /**<   dbm */
//         {-4,       RF_POWER_INDEX_N4p00dBm     },  /**<   dbm */
//        // {-4,     RF_POWER_INDEX_N4p64dBm     },  /**<   dbm */
//         {-5,       RF_POWER_INDEX_N5p71dBm     },  /**<   dbm */
//         {-6,       RF_POWER_INDEX_N6p60dBm     },  /**<   dbm */
//         {-7,       RF_POWER_INDEX_N7p61dBm     },  /**<   dbm */
//         {-8,       RF_POWER_INDEX_N8p79dBm     },  /**<   dbm */
//         {-10,      RF_POWER_INDEX_N10p26dBm    },   /**<   dbm */
//         {-12,      RF_POWER_INDEX_N12p08dBm    },   /**<   dbm */
//         {-16,      RF_POWER_INDEX_N16p29dBm    },   /**<   dbm */
//         {-18,      RF_POWER_INDEX_N18p30dBm    },   /**<   dbm */
//         {-21,      RF_POWER_INDEX_N21p35dBm    },   /**<   dbm */
//         {-25,      RF_POWER_INDEX_N25p77dBm    },   /**<   dbm */
//         {-39,      RF_POWER_INDEX_N39p23dBm    },   /**<   dbm */
};

unsigned char rf_ble_get_tx_pwr_idx(char rfTxPower)
{
    for(u8 i=0; i<(sizeof(ext_rfPwrLvlIdx_buf)>>1); i++)
    {
        if(rfTxPower == ext_rfPwrLvlIdx_buf[i].tx_power_level){
            return ext_rfPwrLvlIdx_buf[i].tx_power_index;
        }
    }
    return RF_POWER_INDEX_P0p03dBm;
}

char rf_ble_get_tx_pwr_level(rf_power_level_index_e rfPwrLvlIdx)
{

    for(u8 i=0; i<(sizeof(ext_rfPwrLvlIdx_buf)>>1); i++)
    {
        if(rfPwrLvlIdx == ext_rfPwrLvlIdx_buf[i].tx_power_index){
            return ext_rfPwrLvlIdx_buf[i].tx_power_level;
        }
    }

    return 0;
}
