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

#if 1 //here is driver code
    rf_mode_init();
    rf_set_ble_1M_mode();
#else
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   1. merge from driver function "rf_mode_init"
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

    rf_clr_irq_mask(FLD_RF_IRQ_ALL);//The default interrupt mask in RF is open.
    //Close the interrupt mask in the initialization code and reopen it when in use

    reg_rf_ll_ctrl3 &= ~(FLD_RF_R_TX_EN_DLY_EN);//Turn off the extension tx_en function

    rf_modem_rate_mode(RF_24M_MODEM_RATE);

    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <4:2>:FE_RTRIM_RX          default:0x02->0x06  Front end matching resistor adjustment for RX.
    * This setting is used to improve RX sensitivity performance.(modified by chenxi.wang,confirmed by siming.leng 20250422.)
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_FE_RTRIM_RX)) | (0x06 << 2);
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <5:4>:LDOTRIM_TRIM_VREF          default:0x02->0x00(0.946V->0.901V)  Bump bits for the 900 mV LDOTRIM reference voltage.
    * This setting reduces RF power consumption by lowering the LDOTRIM reference voltage.
    * (modified by chenxi.wang,confirmed by siming.leng 20250422)
    */
    reg_rf_vco_ldotrim   &= (~FLD_RF_LDOTRIM_TRIM_VREF);
    /*
    *  bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <0:1>:LNA_ITRIM          default:0x00->0x03(4.4uA->6.2uA)  LNA PTAT biasing current trim
    * <3:2>:MIX_VBIAS          default:0x00->0x03(800mV->857mV)  Mixer bias voltage control
    * This setting is used to improve RX sensitivity performance.(modified by chenxi.wang,confirmed by siming.leng 20250422.)
    */
    reg_rf_lnm_pa_0 = (reg_rf_lnm_pa_0&(~(FLD_RF_MIX_VBIAS|FLD_RF_LNA_ITRIM)))|0x0f;
    /*
    *         bit                        default    value                note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <1:0>:cbpf_trim_i                default:0,->3(5.00uA->8.75uA)    Increasing the I-way trim current of cbpf to improve rx performance.
    * <3:2>:cbpf_trim_q                default:0,->3(5.00uA->8.75uA)    Increasing the Q-way trim current of cbpf to improve rx performance.
    * This setting is used to improve RX sensitivity performance.(modified by chenxi.wang,confirmed by siming.leng 20250422)
    */
    reg_rf_cbpf_adc_0 =(reg_rf_cbpf_adc_0&(~(FLD_RF_CBPF_TRIM_I|FLD_RF_CBPF_TRIM_Q)))|0X0f;
    /*
    *         bit                        default    value         note
    *                                                             note
    * ---------------------------------------------------------------------------
    * <31:0>:ADC_INVERT_CLK               default:1,->0    ADC gives its output on neg edge of the clock
    * This configuration is designed to address the issue of the RX PER floor not returning to zero under Aura path.
    * Attention: This configuration requires further testing to confirm the risk.
    * (modified by chenxi.wang,confirmed by xuqiang.zhang 20250422)
    */
    reg_rf_cbpf_adc_1  &=~FLD_RF_ADC_INVERT_CLK;

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   2. merge from driver function "rf_mode_init"
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //aura_1m
//    write_reg8(0x17063d,0x61);//ble:bw_code.
//    write_reg8(0x170620,0x10);//sc_code.
//    write_reg8(0x170621,0x0a);//if_freq,IF = 1Mhz,BW = 1Mhz.

    write_reg8(0x17063d, 0x01); //ble:bw_code.000 -> IF = 1MHz, BW = 667kHz (LIF, 1MBPS)
    write_reg8(0x170620, 0x30); //sc_code.11 = IF of 1000MHz (1MBPS mode)
    /*
    *       bit                 default     value                note
    *                                                            note
    * ---------------------------------------------------------------------------
    * <1>:MODE_VANT_RX           default:1           defines if RX is in vbat or vant mode. Default is LDO_ANT mode
    * <6:5>:IF_FREQ              default:0x00(IF:1MHz,BW:1MHz) Intermediate Frequency Selection.
    * This setting is used to set the RF different modes Intermediate Frequency.
    */
    reg_rf_mode_cfg_rx1_1 = (reg_rf_mode_cfg_rx1_1 & (~FLD_RF_IF_FREQ)) | FLD_RF_MODE_VANT_RX;

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
    write_reg8(0x1704c2,0x3e);//grx_0.
    write_reg8(0x1704c3,0x4b);//grx_1.
    write_reg8(0x1704c4,0x56);//grx_2.
    write_reg8(0x1704c5,0x63);//grx_3.
    write_reg8(0x1704c6,0x6e);//grx_4.
    write_reg8(0x1704c7,0x7a);//grx_5.
    write_reg8(0x1704c8,0x39);//default:0x00->0x39 Gain offset to compensate system error

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
    //write_reg8(0x1704bb,0x00);//disable 2 stage filter
    write_reg8(0x17043e,0x81);//BIT<7>:0 new ,1 old  pm2fm suppress more than pi/4

    //The following register configurations are configured in zigbee/hybee mode, which maintains register defaults
    write_reg8(0x170014,0x7a);//access code for hybee 500K.
    write_reg8(0x170015,0x35);//access code for hybee 500K.
    write_reg8(0x17043b,0x1c);//ZB_NUM_GEAR_H
    write_reg8(0x170132,0x01);//zigbee PHR field enable 1: phr field length embedded in data stream; 0: phr field length from reg ctrl as like private SB packet

    //The following registers are configured in BLE 125K and BLE 500K mode, which maintains the register defaults
    write_reg8(0x1704f0,0x1c);//defaults 0x1c. lr_s8_pdet synv_success threshold 0~32

    //for RF 48M
    reg_rf_hshp_ctrl_0 =  reg_rf_hshp_ctrl_0&(~FLD_RF_RXC_MODE_SEL);//48M RF demod rate sel (0:1mbps)
    reg_rf_hshp_ctrl_1 =  reg_rf_hshp_ctrl_1&(~FLD_RF_TXC_MODE_SEL);//48M RF demod rate sel (0:1mbps)

    //TODO:Only 4M 6M requires configuring the following registers, while other modes need to maintain default values.
    reg_rf_hshp_ctrl_2 &=(~FLD_RF_RXC_MODE_OW);
    write_reg8(0x17051a,0x1d);
    reg_rf_tx_hlen_mode &=(~FLD_RF_TX_VLD_EN);//r_tx_vld_en :tx vld output en
    write_reg8(0x170026,0x14);

    //private mdm bit-dly for BB crc calculate ACCESS;
    write_reg8(0x17051d,0x92);//bit<5> 1,bit dly en;,bit<6> 0,bit dly num_h 0;
    write_reg8(0x17051e,0x87);

    reg_rf_reg_sparelv1 &= (~FLD_RF_CBPF_HIGH_GBW);

    rf_set_crc_config(&rf_crc_config[0]);
#endif

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   3. setting for BLE by BLE_Team
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////
    rf_clr_irq_mask(FLD_RF_IRQ_ALL);//The default interrupt mask in RF is open.
    //Close the interrupt mask in the initialization code and reopen it when in use

    write_reg8(0x17044e,RF_ACCESS_CODE_DEFAULT_THRESHOLD);//ble sync threshold:To modem.

    write_reg8(0x170002,0x40| PRMBL_LENGTH_1M<<16);//preamble length

    //access code, can save
    //write_reg32(0x170008,0x00000000);   //default:0xf8118ac9;

    write_reg8(0x170030, 0x36);         //default:0x3c;disable tx timestamp en, add by LiBiao

    write_reg8(0x80170206, 0x00);       //LL_RXWAIT, default 0x0009
    write_reg8(0x8017020c, 0x50);       //LL_RXSTL   default 0x0095
    write_reg8(0x8017020e, 0x00);       //LL_TXWAIT, default 0x0009
    write_reg8(0x80170210, 0x00);       //LL_ARD,    default 0x0063

    reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;        //coded phy accesscode trigger mode: manual mode

    //rf_modem_hp_path(1);
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
#if (1)/* FPGA Board use */
    void rf_set_ble_channel (signed char chn)//void rf_fpga_set_chn(signed char chn)
    {
        write_reg8 (0x170020, chn);

        if (chn < 11)
            chn = chn + 1;

        else if (chn < 37)
            chn = chn + 2;

        else if (chn == 37)
            chn = 0;

        else if (chn == 38)
            chn = 12;

        else if (chn == 39)
            chn = 39;
        else{
            return;
        }

        unsigned char ctrim;
        unsigned int freq = 0;
        chn *= 2;
        freq = chn + 2402;

        if(freq >= 2550){
            ctrim = 2;
        }
        else if(freq >= 2520){
            ctrim = 2;
        }
        else if(freq >= 2495){
            ctrim = 2;
        }
        else if(freq >= 2460){
            ctrim = 2;
        }
        else if(freq >= 2440){
            ctrim = 3;
        }
        else if(freq >= 2416){
            ctrim = 4;
        }
        else if(freq >= 2380){
            ctrim = 5;
        }
        else{
            ctrim = 7;
        }

        write_reg8(0x170644,read_reg8(0x170644)&0xfe);
        write_reg8(0x170629,read_reg8(0x170629)|0x01);//Front end matching capacitor adjustment for TX/RX. This code should be sent on reg_rx_lna_ctrim_lv<3:0>.
        write_reg8(0x170628,chn);//CHNL Frequency = (2402+CHNL_NUM*2) MHz,chnl num can be from 0 to 39
        write_reg8(0x170629,(read_reg8(0x170629)&0x1f)|(ctrim<<5));
    }
#else
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
#endif

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

_attribute_data_retention_ Fast_Settle fast_settle_1M;
_attribute_data_retention_ Fast_Settle fast_settle_2M;
_attribute_data_retention_ Fast_Settle fast_settle_S2;
_attribute_data_retention_ Fast_Settle fast_settle_S8;

#endif


