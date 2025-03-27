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
#include "stack/ble/controller/ble_controller.h"

volatile unsigned int TXADDR = 0xc0013000;

#define   BLE_TXDMA_DATA        (0x170000 + 0x84)      //0x170084
#define   BLE_RXDMA_DATA        (0x170000 + 0x80)      //0x170080

//extern dma_config_t rf_tx_dma_config;
//extern dma_config_t rf_rx_dma_config;

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

_attribute_data_retention_sec_ ext_rf_t blt_extRF;

void ble_rf_set_tx_dma(unsigned char fifo_dep,unsigned char size_div_16)
{
    unsigned short fifo_byte_size = size_div_16<<4;
//
//  reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK);
//  reg_rf_bb_tx_chn_dep = fifo_dep;
//
//  reg_rf_bb_tx_size   = fifo_byte_size&0xff;
//  reg_rf_bb_tx_size_h = fifo_byte_size>>8;
//
//  dma_config(DMA0,&rf_tx_dma_config);//solve dma_chn_dis(DMA0) cause read_num_en reset to 0
//  dma_set_address(DMA0, TXADDR, BLE_TXDMA_DATA);   // TXADDR=0xc0013000;
    rf_set_tx_dma_config();
    rf_set_tx_dma_fifo_num(fifo_dep);
    rf_set_tx_dma_fifo_size(fifo_byte_size);
}

_attribute_ram_code_
void ble_rf_set_rx_dma(unsigned char *buff, unsigned char size_div_16)
{

//  unsigned short fifo_byte_size = size_div_16<<4;
//  ble_curr_rx_dma_buff = buff;
//  buff +=4;
//
//  reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK);//ch0_rnum_en_bk,tx_multi_en,rx_multi_en
//
//  //TODO: check with Qiangkai
//  reg_rf_rx_wptr_mask = 0; //rx_wptr_real=rx_wptr & mask:After receiving 4 packets,the address returns to original address.mask value must in (0x01,0x03,0x07,0x0f)
//  reg_rf_bb_rx_size = fifo_byte_size&0xff;//rx_idx_addr = {rx_wptr*bb_rx_size,4'h0}// in this setting the max data in one dma buffer is 0x20<<4.
//  reg_rf_bb_rx_size_h = fifo_byte_size>>8;
//  dma_set_address(DMA1, BLE_RXDMA_DATA, (u32)convert_ram_addr_cpu2bus(buff));
//  reg_dma_size(DMA1)=0xffffffff;

    unsigned short fifo_byte_size = size_div_16<<4;
    rf_set_rx_dma_config();
    ble_curr_rx_dma_buff = buff;
    rf_set_rx_buffer(buff);
    reg_rf_rx_wptr_mask = 0;
    rf_set_rx_dma_fifo_size(fifo_byte_size);
}

void ble_rx_dma_config(void){
//  dma_config(DMA1,&rf_rx_dma_config);
    rf_set_rx_dma_config();
}

#if BLC_PM_EN
_attribute_ram_code_
#endif
void rf_drv_ble_init(void){

#if (PRMBL_LENGTH_1M < 1 || PRMBL_LENGTH_1M > 15)
    #error "1M PHY pream_ble error!!!"
#endif


#if (PRMBL_LENGTH_2M < 2 || PRMBL_LENGTH_1M > 15)
    #error "2M PHY pream_ble error!!!"
#endif

#if 0 //here is driver code
    rf_mode_init();
    rf_set_ble_1M_mode();
#else
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   2. merge from driver function "rf_mode_init"
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    pm_set_dig_module_power_switch(FLD_PD_ZB_EN, PM_POWER_UP);

    reg_rst4    |=  FLD_RST4_ZB;
    reg_clk_en4 |=  FLD_CLK4_ZB_EN;

    reg_n22_rst = 0xffc;//reset dma_bb,zb,rst_modem,rstl_bb(bb:baseband)
    reg_n22_clk_en0 = 0xff;//enable dma_bb,zb_hclk
    reg_rf_tstimp_ctrl |= FLD_RF_R_STIMER_REVERT_EN; //Switching RF clock to stimer.
    //one_time_setup
    write_reg8(0x1706d2,0x9b);//DCOC_SFIIP:bit<4> DCOC_SFQQP:bit<5> DCOC_SFII_L:bit<6-7>
    write_reg8(0x1706d3,0x19);//DCOC_SFII_H:bit<0-1> DCOC_SFQQ:bit<2-5>
#if RF_RX_SHORT_MODE_EN
    write_reg8(0x17047b,0x0e);//BLANK_WINDOW
    write_reg8(0x170479,0x38);//BIT[3] RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix
                              //per floor issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#else
    write_reg8(0x17047b,0xfe);//BLANK_WINDOW
    write_reg8(0x170479,0x08);//RX_DIS_PDET_BLANK.BIT_RNG[4,5]SHORT MODE all mode open pdet blank to fix per floor
                              //issue.modified by zhiwei,confirmed by qiangkai and xuqiang.20221205
#endif

    //To set AGC thresholds
    write_reg8(0x17064a,0x0e);//POW_000_001:bit<0-6> POW_001_010_L:bit<7>
    write_reg8(0x17064b,0x09);//POW_001_010_H:bit<0-5>
    write_reg8(0x17064e,0x09);//POW_100_101:bit<0-6> POW_101_100_L:bit<7>
    write_reg8(0x17064f,0x0f);//POW_101_100_H:bit<0-5>
    write_reg8(0x170654,0x0e);//POW_000_001:bit<0-6> POW_001_010_L:bit<7>
    write_reg8(0x170655,0x09);//POW_001_010_H:bit<0-5>
    write_reg8(0x170656,0x0c);//POW_010_011:bit<0-6> POW_011_100_L:bit<7>
    write_reg8(0x170657,0x08);//POW_011_100_H:bit<0-5>
    write_reg8(0x170658,0x09);//POW_100_101:bit<0-6> POW_101_100_L:bit<7>
    write_reg8(0x170659,0x0f);//POW_101_100_H:bit<0-5>

    //For optimum preamble detection
    write_reg8(0x170476,0x50);//RX_PE_DET_MIN_LO_THRESH
    write_reg8(0x170477,0x73);//RX_PE_DET_MIN_HI_THRESH

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1:0>:cbpf_trim_i                default:0,->3(5.00u->8.75u)    Increasing the I-way trim current of cbpf to improve rx performance.
    * <3:2>:cbpf_trim_q                   default:0,->3(5.00u->8.75u)    Increasing the Q-way trim current of cbpf to improve rx performance.
    * <6:5>:cbpf_trim_short_dcbias    default:2,->1(460mv->490mv)    Increase cbpf_trim_short_dcbias voltage to boost RX performance.
    * <7:7>:cbpf_vcm_trim_l            default:1,->0(490mv->520mv) Increase cbpf_vcm_trim_l voltage to boost RX performance.
    * This setting is used for A0 to improve performance, pending A1 hardware to fix performance issues; this setting will be canceled
    * out to reduce power consumption. modified by zhiwei.wang,confirmed by wenfeng.lou 24020531.
    */
    write_reg8(0x17074e,0x4f);

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <2:0>:VCO_TRIM_KVT                default:0x7  Adjustment of Kv of vctrl path depending upon reference frequency. Default should change depending upon if the reference frequency is 24MHz or 32MHz
    * <3>  :VCO_EN_PKDET                default:0    Enable peak detector operation
    * <5:4>:LDOTRIM_TRIM_VREF           default:2,->0(0.946V->0.901V) Bump bits for the 900 mV LDOTRIM reference voltage.
    * This setting is used to change the LDO trimming reference voltage from 0.946 to 0.901 and then LDO output to 1.05v.
    * modified by chenxi.wang,confirmed by wenfeng.lou 20240820.
    */
    write_reg8(0x170754,0x07);
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <0>:LDO_PLL_BOOST           default:0 To boost the LDO output voltage to close to 1.5V with trim code being maximum
    * <1>:LDO_VCO_BYPASS          default:0 Bypass the LDO output to Vline
    * <2>:LDO_VCO_BOOST           default:0 To boost the LDO output voltage to close to 1.5V with trim code being maximum
    * <3>:LDO_CAL_BYPASS          default:0 Bypass the LDO output to Vline
    * <4>:LDO_CAL_BOOST           default:0 To boost the LDO output voltage to close to 1.5V with trim code being maximum
    * <5>:LDO_ANT_BYPASS          default:0,->1 Bypass the LDO output to Vline
    * This setting is used for A1 to obtain higher output power under vant mode.modified by chenxi.wang,confirmed by wenfeng.lou 20240820.
    */
    write_reg8(0x170741,0x20);

    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <0>:LNA_HP_DIG                 default:1 LNA high power high performance control: 0:normal mode;1:high power low noise mode
    * <3:1>:TX_ATTN_TRIM_DIG         default:4 Tx lo attenuation mode trim
    * <4>:TX_ATTN_SEL_DIG            default:0 Tx lo attenuation enable:0(default): tx lo normal mode; 1:tx lo attenuation mode
    * <7:5>:TX_BUF_TRIM_DIG          default:4,->0 Tx lo buffer vbias trim to adjust the duty cycle
    * This setting is used for A1 to optimise power consumption.modified by chenxi.wang,confirmed by wenfeng.lou 20240820.
    */
    write_reg8(0x170638,0x09);

    rf_clr_irq_mask(FLD_RF_IRQ_ALL);//The default interrupt mask in RF is open.
    //Close the interrupt mask in the initialization code and reopen it when in use

    reg_rf_ll_ctrl3 &= ~(FLD_RF_R_TX_EN_DLY_EN);//Turn off the extension tx_en function

    //aura_1m
    write_reg8(0x17063d,0x61);//ble:bw_code.
    write_reg8(0x170620,0x10);//sc_code.
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <4:2>:FE_RTRIM_RX          default:0x02->0x03  Front end matching resistor adjustment for RX. (Configured by the rf_rx_performance_mode interface)
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 &(~FLD_RF_IF_FREQ))|FLD_RF_MODE_VANT_RX;
    write_reg8(0x170622,0x20);//RADIO BLE_MODE_TX,1MBPS:bit<0>;VCO_TRIM_KV:bit<1-3>;HPMC_EXP_DIFF_COUNT_L:bit<4-7>.
    write_reg8(0x170623,0x23);//HPMC_EXP_DIFF_COUNT_H.
    write_reg8(0x170422,0x00);//modem:BLE_MODE_TX,1MBPS.
    write_reg8(0x17044e,RF_ACCESS_CODE_DEFAULT_THRESHOLD);//ble sync threshold:To modem.


    write_reg8(0x17063f,0x00);//250k modulation index:telink add rx for 250k/500k.

    //rx_cont_mode
    write_reg8(0x170420,0xc8);// script cc. rx continue mode on:bit<3>


    write_reg8(0x17044d,0x01);//r_rxchn_en_i:To modem.
    write_reg8(0x170421,0x00);//modem:ZIGBEE_MODE:01
    write_reg8(0x170423,0x00);//modem:ZIGBEE_MODE_TX.
    write_reg8(0x170426,0x00);//modem:sync rst sel,for zigbee access code sync.
    write_reg8(0x17042a,0x10);//modem:disable MSK.
    write_reg8(0x17043d,0x00);//modem:zb_sfd_frm_ll.
    write_reg8(0x17042c,0x38);//modem:zb_dis_rst_pdet_isfd.
    write_reg8(0x170436,0xb7);//LR_NUM_GEAR_L.
    write_reg8(0x170437,0x0e);//LR_NUM_GEAR_H.
    write_reg8(0x170438,0xb6);//LR_TIM_EDGE_DEV.0xc4->0xb6
    write_reg8(0x170439,0x71);//LR_TIM_REC_CFG_1.
    write_reg8(0x170473,0x01);//TOT_DEV_RST.
    write_reg8(0x17049a,0x00);//tx_tp_align.

    //agc_table_1m
    write_reg8(0x1704c2,0x3a);//grx_0.
    write_reg8(0x1704c3,0x4b);//grx_1.
    write_reg8(0x1704c4,0x56);//grx_2.
    write_reg8(0x1704c5,0x63);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x7a);//grx_5.


    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0,0x1c);//defaults 0x1c. lr_s8_pdet synv_success threshold 0~32
    write_reg8(0x1704f2,0xa4);//defaults 0xa4. bit<4-6>:0x02 lr_s8_demod pidx adjust threshold
    write_reg8(0x1704f3,0x15);//defaults 0x15. bit<0-2>:0x05 LR S8 sync_success moment to mIdx delay taps [-1~-4]

    //ble1m_setup
    write_reg32(0x170000,0x5440080f|PRMBL_LENGTH_1M<<16);//tx_mode.
//  write_reg8(0x170001,0x08);//PN.
//  write_reg8(0x170002,0x40| PRMBL_LENGTH_1M<<16);//preamble length
//  write_reg8(0x170003,0x54);//bit<0:1>private mode control.
    write_reg8(0x170004,0xf1);//bit<4>mode:1->1m;bit<0:3>:ble head.
    write_reg8(0x170005,0x04);//lr mode bit<4:5>

    write_reg8(0x170021,0xa1);//rx packet len 0 enable.
                              //bit<5>:write packet length filed into sram

    write_reg8(0x170022,0x00);//rxchn_man_en.
    write_reg8(0x17044c,0x0c);//RX:acc_len modem.0x4c->0x0c

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   3. setting for BLE by BLE_Team
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //access code, can save
    write_reg32(0x170008,0x00000000);   //default:0xf8118ac9;

    write_reg8(0x170030, 0x36);         //default:0x3c;disable tx timestamp en, add by LiBiao

    write_reg8(0x80170206, 0x00);       //LL_RXWAIT, default 0x0009
    write_reg8(0x8017020c, 0x50);       //LL_RXSTL   default 0x0095
    write_reg8(0x8017020e, 0x00);       //LL_TXWAIT, default 0x0009
    write_reg8(0x80170210, 0x00);       //LL_ARD,    default 0x0063
    /*
    * This configuration is used for A1 to improve the performance of rf rx sensitivity.
    * Defaults to RX_LOW_POWER for A1.
    * If you need higher performance, you need to call rf_rx_performance_mode() after rf_mode_init;
    * Select the RX_HIGH_PERFORMING mode, in which the RX sensitivity is increased by 1dBm, but the receiving power consumption will increase
    * (modified by chenxi.wang,confirmed by wenfeng.lou 20240826.)
    */
    rf_rx_performance_mode(RF_RX_LOW_POWER);

    reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;        //coded phy accesscode trigger mode: manual mode
#endif
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
#if FAST_SETTLE
    unsigned char ble_chn = chn_num;
#endif
    write_reg8 (0x170020, chn_num);
    if (chn_num < 11)
        chn_num += 2;
    else if (chn_num < 37)
        chn_num += 3;
    else if (chn_num == 37)
        chn_num = 1;
    else if (chn_num == 38)
        chn_num = 13;
    else if (chn_num == 39)
        chn_num = 40;

    chn_num = chn_num << 1;
    rf_set_chn(chn_num);
#if RF_THREE_CHANNEL_CALIBRATION
    if(channel_power_calibration_enable)
    {
        ble_rf_set_chn_power(chn_num);
    }
#endif
#if FAST_SETTLE
    if(fast_settle.tx_fast_en){
        set_rf_hpmc_cal_val(fast_settle.cal_tbl[ble_chn]);
    }
#endif
}

#if (SCHEDULE_USE_BB_TIMER)
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
#else
_attribute_ram_code_
void rf_start_fsm (fsm_mode_e mode, void* tx_addr, unsigned int tick)
{
    unsigned int r = core_interrupt_disable();
//  write_reg32(0x80170218, tick);
    reg_rf_ll_cmd_schedule = tick;
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = mode;

    if(tx_addr){
        rf_dma_set_src_address(RF_TX_DMA,(unsigned int)tx_addr);
    }

//  if(mode != FSM_SRX){ //BLE SDK never use PRX mode
//      rf_fsm_tx_trigger_flag = 1;
//  }


    core_restore_interrupt(r);
}
#endif
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

/**
 * this function is same as rf_clr_dig_logic_state. just because rf_clr_dig_logic_state is driver code and is flash code.
 * so here rewrite the API and define the API as ram code.
 */
_attribute_ram_code_
void rf_ble_clr_dig_logic_state(void)
{
    reg_n22_rst &= ~((FLD_RST0_ZB)|((FLD_RST1_RSTL_BB|FLD_RST1_RST_MDM)<<8));
    reg_n22_rst |= ((FLD_RST0_ZB)|((FLD_RST1_RSTL_BB|FLD_RST1_RST_MDM)<<8));
}

#if FAST_SETTLE

#define RADIOADDR 0x170600

_attribute_data_retention_ Fast_Settle fast_settle;

/* close hpmc(53us), ldotrim(4.5us),save 58us
 * 0x140e84:[0] tx ldo trim
 *          [1] tx fcal
 *          [2] tx hpmc
 *          [3] tx dcoc
 */
_attribute_ram_code_
void ble_rf_tx_fast_settle()
{
    //close hpmc and ldo trim
    write_reg8(RADIOADDR+0x84,(read_reg8(RADIOADDR+0x84)&0xf0)|0x0a); //1010

    write_reg8(RADIOADDR+0x96,0x00);    //0
    write_reg8(RADIOADDR+0x97,0x08);    //8us
    write_reg8(RADIOADDR+0x98,0x30);    //48us
    write_reg8(RADIOADDR+0x99,0x31);    //48.5us
    write_reg8(RADIOADDR+0x9a,0x33);    //51us
    write_reg8(RADIOADDR+0x9b,0x30);    //0x6a


    // only close hpmc
//  write_reg8(RADIOADDR+0x84,(read_reg8(RADIOADDR+0x84)&0xf8)|0x0b); //1011
//
//  write_reg8(RADIOADDR+0x96,0x00);    //0
//  write_reg8(RADIOADDR+0x97,0x0d);    //13us
//  write_reg8(RADIOADDR+0x98,0x35);    //53us
//  write_reg8(RADIOADDR+0x99,0x36);    //53.5us
//  write_reg8(RADIOADDR+0x9a,0x38);    //55.5us
//  write_reg8(RADIOADDR+0x9b,0x35);    //53us

    // only ldo trim
//  write_reg8(RADIOADDR+0x84,(read_reg8(RADIOADDR+0x84)&0xf8)|0x0e); //1110
//
//  write_reg8(RADIOADDR+0x96,0x00);    //0
//  write_reg8(RADIOADDR+0x97,0x08);    //8us
//  write_reg8(RADIOADDR+0x98,0x65);    //48us
//  write_reg8(RADIOADDR+0x99,0x66);    //48.5us
//  write_reg8(RADIOADDR+0x9a,0x68);    //51us
//  write_reg8(RADIOADDR+0x9b,0x65);    //0x6a

    // all open,
//  write_reg8(RADIOADDR+0x84,(read_reg8(RADIOADDR+0x84)&0xf8)|0x0f); //1111
//
//  write_reg8(RADIOADDR+0x96,0x00);    //0
//  write_reg8(RADIOADDR+0x97,0x0d);    //8us
//  write_reg8(RADIOADDR+0x98,0x6a);    //48us
//  write_reg8(RADIOADDR+0x99,0x6b);    //48.5us
//  write_reg8(RADIOADDR+0x9a,0x6d);    //51us
//  write_reg8(RADIOADDR+0x9b,0x6a);    //0x6a


}

/* close dcoc(40us), ldotrim(4.5us),save 45us
 * 0x140e84:[4] rx ldo trim
 *          [5] rx fcal
 *          [6] rx rccal
 *          [7] rx dcoc
 */

_attribute_ram_code_
void ble_rf_rx_fast_settle()
{
    write_reg8(RADIOADDR+0x84,(read_reg8(RADIOADDR+0x84)&0x0f)|0x60);

    write_reg8(RADIOADDR+0x9c,0x00);    //0us
    write_reg8(RADIOADDR+0x9d,0x08);    //8us
    write_reg8(RADIOADDR+0x9e,0x08);    //8us
    write_reg8(RADIOADDR+0x9f,0x1b);    //34us
    write_reg8(RADIOADDR+0xa0,0x25);    //37us
    write_reg8(RADIOADDR+0xa1,0x25);    //37us
}

_attribute_ram_code_
unsigned short get_rf_hpmc_cal_val()
{
    unsigned short cali;
    unsigned short r;
    cali = read_reg16(RADIOADDR+0xfe);  //140efe<0:10>
    r = (cali<<1)& 0x0ffe;      //to 140ef6 <1:11>  0000 1111 1111 1110
    return r;
}


_attribute_ram_code_
void set_rf_hpmc_cal_val(unsigned short value)
{
    unsigned short tmp = read_reg16(RADIOADDR+0xf6);
    tmp = (tmp & 0xf001) | value | 0x0001;  //bit<1:11> 1111 0000 0000 0001
    write_reg16(RADIOADDR+0xf6,tmp);
}

/**
 *  @brief      this function serve to enable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void ble_rf_tx_fast_settle_en(void)
{
    fast_settle.tx_fast_en = 1;
    write_reg8(RADIOADDR+0x29,read_reg8(RADIOADDR+0x29)|0x10);  //140e29 <4>
}

/**
 *  @brief      this function serve to disable the tx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void ble_rf_tx_fast_settle_dis(void)
{
    fast_settle.tx_fast_en = 0;
    write_reg8(RADIOADDR+0x29,read_reg8(RADIOADDR+0x29)&0xef);  //140e29 <4>
}

/**
 *  @brief      this function serve to enable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void ble_rf_rx_fast_settle_en(void)
{
    fast_settle.rx_fast_en = 1;
    write_reg8(RADIOADDR+0x29,read_reg8(RADIOADDR+0x29)|0x08);  //140e29 <3>
}

/**
 *  @brief      this function serve to disable the rx timing sequence adjusted.
 *  @param[in]  none
 *  @return     none
*/
void ble_rf_rx_fast_settle_dis(void)
{
    fast_settle.rx_fast_en = 0;
    write_reg8(RADIOADDR+0x29,read_reg8(RADIOADDR+0x29)&0xf7);  //140e29 <3>
}

u8 ble_is_rf_tx_fast_settle_en()
{
    return fast_settle.tx_fast_en;
}

u8 ble_is_rf_rx_fast_settle_en()
{
    return fast_settle.rx_fast_en;
}
#if 0  //B92 use drivers
/*
 *  LDOT_RDBK1      0xea        0x00
 *                  LDOT_LDO_CAL_TRIM   [5:0]   0x00
 *  LDOT_RDBK2_0    0xec        0xc0
 *                  LDOT_LDO_RXTXHF_TRIM    [5:0]   0x00
 *                  LDOT_LDO_RXTXLF_TRIM_L  [7:6]   0x3
 *  LDOT_RDBK2_1    0xed        0x05
 *                  LDOT_LDO_RXTXLF_TRIM_H  [3:0]   0x5
 *  LDOT_RDBK3_0    0xee        0x00
 *                  LDOT_LDO_PLL_TRIM   [5:0]   0x00
 *                  LDOT_LDO_VCO_TRIM_L [7:6]   0x0
 *  LDOT_RDBK3_1    0xef        0x00
 *                  LDOT_LDO_VCO_TRIM_H [3:0]   0x0
 */
void get_ldo_trim_val(u8* p)
{
    u8  tmp_val;
    *p++ = read_reg8(RADIOADDR+0xea) & 0x3f;                        //LDO_CAL_TRIM 0xea[5:0]
    tmp_val = read_reg8(RADIOADDR+0xec);
    *p++ = tmp_val & 0x3f;                                          //LDO_RXTXHF_TRIM 0xec[5:0]
    *p++ = (tmp_val & 0xc0)>>6 | (read_reg8(RADIOADDR+0xed)&0x0f)<<2 ;  //LDO_RXTXLF_TRIM 0xec[7:6]  0xed[3:0]
    tmp_val = read_reg8(RADIOADDR+0xee);
    *p++ = tmp_val & 0x3f;                                          //LDO_PLL_TRIM 0xee[5:0]
    *p++ = (tmp_val & 0xc0)>>6 | (read_reg8(RADIOADDR+0xef)&0x0f)<<2;   //LDO_VCO_TRIM 0xee[7:6]  0xef[3:0]
}

/*
 *  LDOT_DBG1   0xe2        0x40
 *              LDOT_LDO_CAL_BYPASS [0] 0x0
 *              LDOT_LDO_CAL_TRIM_OVERWRITE [6:1]   0x20
 *  LDOT_DBG2_0 0xe4        0x80
 *              LDOT_LDO_RXTXHF_BYPASS  [0] 0x0
 *              LDOT_LDO_RXTXLF_BYPASS  [1] 0x0
 *              LDOT_LDO_RXTXHF_TRIM_OVERWRITE  [7:2]   0x20
 *  LDOT_DBG2_1 0xe5        0x20
 *              LDOT_LDO_RXTXLF_TRIM_OVERWRITE  [5:0]   0x20
 *  LDOT_DBG3_0 0xe6        0x80
 *              LDOT_LDO_PLL_BYPASS [0] 0x0
 *              LDOT_LDO_VCO_BYPASS [1] 0x0
 *              LDOT_LDO_PLL_TRIM_OVERWRITE [7:2]   0x20
 *  LDOT_DBG3_1 0xe7        0x20
 *              LDOT_LDO_VCO_TRIM_OVERWRITE [5:0]   0x20
 */
void set_ldo_trim_val(u8* p)
{
    write_reg8(RADIOADDR+0xe2 ,(*p++ << 1) | 0x01);
    write_reg8(RADIOADDR+0xe4 ,(*p++ << 2) | 0x03);
    write_reg8(RADIOADDR+0xe5 , *p++);
    write_reg8(RADIOADDR+0xe6 ,(*p++ << 2) | 0x03);
    write_reg8(RADIOADDR+0xe7 , *p);
}
#endif
/*need to use :
 * PA0,PA1,PA2
 * PB1,PB7,
 * PC0,PC1,PC2,PC3,PC4
 */

void bb_dbg_setting(void)
{
    unsigned int GPIO_BASE = 0x140300;

    sub_wr(GPIO_BASE+0x55, 0, 7, 4); //dbg_sel_bt1-4 = 0
    sub_wr(GPIO_BASE+0x54, 3, 2, 1); //dbg_sel_bb_h/l = 1
    sub_wr(GPIO_BASE+0x54, 0 ,5, 5); //dbg_axon_bb_sel = 0

    sub_wr(GPIO_BASE+0x0e, 0, 1, 1); //pb_io[1]
    sub_wr(GPIO_BASE+0x32, 3, 3, 2); //pb[1] tx_data_o
    sub_wr(GPIO_BASE+0x06, 0, 2, 2); //pa_io[2]
    sub_wr(GPIO_BASE+0x30, 3, 5, 4); //pa[2] rx_en_o
    sub_wr(GPIO_BASE+0x06, 0, 1, 1); //pa_io[1]
    sub_wr(GPIO_BASE+0x30, 3, 3, 2); //pa[1] tx_on_o
    sub_wr(GPIO_BASE+0x06, 0, 0, 0); //pa_io[0]
    sub_wr(GPIO_BASE+0x30, 3, 1, 0); //pa[0] tx_en_o
}


#endif


