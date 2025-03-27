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

extern dma_config_t rf_tx_dma_config;
extern dma_config_t rf_rx_dma_config;

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

void ble_rf_set_tx_dma(unsigned char fifo_dep,unsigned char size_div_16)
{
    unsigned short fifo_byte_size = size_div_16<<4;

    reg_rf_bb_auto_ctrl |= (FLD_RF_TX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK);
    reg_rf_bb_tx_chn_dep = fifo_dep;

    reg_rf_bb_tx_size   = fifo_byte_size&0xff;
    reg_rf_bb_tx_size_h = fifo_byte_size>>8;

    dma_config(DMA0,&rf_tx_dma_config);//solve dma_chn_dis(DMA0) cause read_num_en reset to 0
    dma_set_address(DMA0, TXADDR, BLE_TXDMA_DATA);   // TXADDR=0xc0013000;
}

_attribute_ram_code_
void ble_rf_set_rx_dma(unsigned char *buff, unsigned char size_div_16)
{

    unsigned short fifo_byte_size = size_div_16<<4;
    ble_curr_rx_dma_buff = buff;
    buff +=4;

    reg_rf_bb_auto_ctrl |= (FLD_RF_RX_MULTI_EN | FLD_RF_CH_0_RNUM_EN_BK);//ch0_rnum_en_bk,tx_multi_en,rx_multi_en

    //TODO: check with Qiangkai
    reg_rf_rx_wptr_mask = 0; //rx_wptr_real=rx_wptr & mask:After receiving 4 packets,the address returns to original address.mask value must in (0x01,0x03,0x07,0x0f)
    reg_rf_bb_rx_size = fifo_byte_size&0xff;//rx_idx_addr = {rx_wptr*bb_rx_size,4'h0}// in this setting the max data in one dma buffer is 0x20<<4.
    reg_rf_bb_rx_size_h = fifo_byte_size>>8;
    dma_set_address(DMA1, BLE_RXDMA_DATA, (u32)convert_ram_addr_cpu2bus(buff));
    reg_dma_size(DMA1)=0xffffffff;

}

void ble_rx_dma_config(void){
    dma_config(DMA1,&rf_rx_dma_config);
}

#if BLC_PM_EN
_attribute_ram_code_
#endif
void rf_drv_ble_init(void){
    rf_mode_optimize_init();
    rf_ble_set_1m_phy();

    //setting for BLE by BLE_Team
#if 1 //remove setting below to process potential register loss when reset bandband for multipe ICs sharing same BLE protocol code
      //and must correctly handle RF register setting in LinkLayer task modules which need them !!!

    /* <R-RFReg> Refactor RF related registers
     * Considering suspend/deepRet/deepSleep/reset_baseband may power down some modules, the registers status will lose,
     * The RF related Reg below should be configured when using.
     */

    //write_reg8(0x80170210, 0x00);         //LL_ARD,    default 0x0063    only involved in PTX/PRX, we can neglect it

    // <R-RFReg> This is only useful for Coded PHY, and it is already set in our code when using Coded PHY.
    //reg_rf_modem_mode_cfg_rx1_0 &= ~FLD_RF_LR_TRIG_MODE;      //coded phy accesscode trigger mode: manual mode


    /* no need set here, set in every linklayer task when need it */
    //write_reg8(0x8017020c, 0x50);         //LL_RXSTL   default 0x0095    should be set according to FSM.

    /* no need set here, set in every linklayer task when need it */
    //write_reg8(0x8017020e, 0x00);         //LL_TXWAIT, default 0x0009    only involved in BTX/BRX/RX2TX
    //write_reg8(0x80170206, 0x00);         //LL_RXWAIT, default 0x0009    only involved in BTX/BRX/TX2RX

    /* no need set here, make sure that set it in 1m/2m/coded PHY setting */
    //write_reg8(0x17044e, 31);             //default:0x1e;access code bit number match threshold
#endif

    // <R-RFReg> Still not decided how to refactor this reg. It's for rx timestamp utility, we must set this as is.
    write_reg8(0x170030, 0x36);         //default:0x3c;disable tx timestamp en, add by LiBiao
}

__INLINE void ble_rf_set_chn(signed char chn)
{
    extern rf_fast_settle_t g_fast_settle_cal_val;
    extern unsigned char g_rf_tx_fast_settle_chn_cal_flag;
    if(g_rf_tx_fast_settle_chn_cal_flag == 1)
    {
        extern void rf_set_hpmc_cal_val(unsigned short hpmc_gain);
        rf_set_hpmc_cal_val(g_fast_settle_cal_val.cal_tbl[chn]);
    }

    unsigned int freq;
    unsigned char ctrim;
    chn =  chn << 1;
    freq =  2402 + chn;
    if (freq >= 2550)
    {
        ctrim = 0;
    }
    else if (freq >= 2520)
    {
        ctrim = 1;
    }
    else if (freq >= 2461)
    {
        ctrim = 2;
    }
    else if (freq >= 2445)
    {
        ctrim = 3;
    }
    else if (freq >= 2425)
    {
        ctrim = 4;
    }
    else if (freq >= 2407)
    {
        ctrim = 5;
    }
    else if (freq >= 2380)
    {
        ctrim = 6;
    }
    else
    {
        ctrim = 7;
    }
    write_reg8(0x170644,  (read_reg8(0x170644) & 0xfe ));
    write_reg8(0x170629,(read_reg8(0x170629)&0x1f)|(ctrim<<5)|0x01);
    write_reg8(0x170628,  chn);
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
    write_reg8 (0x17000d, chn_num);
    if (chn_num < 11)
    {
        chn_num = chn_num+1;
    }
    else if(chn_num < 37)
    {
        chn_num = chn_num + 2;
    }
    else if (chn_num  == 37)
    {
        chn_num = 0;
    }
    else if(chn_num == 38)
    {
        chn_num = 12;
    }
    else if(chn_num == 39)
    {
        chn_num = 39;
    }

    ble_rf_set_chn(chn_num);
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

_attribute_ram_code_
void rf_start_fsm (fsm_mode_e mode, void* tx_addr, unsigned int tick)
{
    unsigned int r = core_interrupt_disable();
    write_reg32(0x80170218, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    reg_rf_ll_cmd = mode;

    if(tx_addr){
        dma_set_src_address(DMA0,(unsigned int)tx_addr);
    }


    core_restore_interrupt(r);
}

/*
 * this function is same as driver rf_emi_reset_baseband.
 * but rf_emi_reset_baseband is not ram code.
 */
_attribute_ram_code_ void ble_rf_reset_baseband(void)
{
    reg_rst3 &= (~FLD_RST3_ZB); // reset baseband
    reg_rst3 |= (FLD_RST3_ZB);  // clr baseband
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

#if (HADM_PHASE_CONTINUITY)
_attribute_data_retention_ rf_cs_tx_cali_t tx_cs_cali;
_attribute_data_retention_ rf_cs_rx_cali_t rx_cs_cali;
_attribute_data_retention_ unsigned char cs_phase_continuity_flag = 0;

/**
 * @brief       This function is mainly used to set the sequence related to Fast Settle in cs.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_phase_continuity_en(void)//CCLK_96M, consume 49us
{
    #if (FAST_SETTLE)
        //rf_rx_fast_settle_dis
        write_reg8(0x170629, read_reg8(0x170629)&0xf7);
        write_reg8(0x1706d0, read_reg8(0x1706d0)&0xfe);//dcoc
        write_reg8(0x1706ce, read_reg8(0x1706ce)&0xfe);//iq_code
        write_reg8(0x1706e2, read_reg8(0x1706e2)&0xfe);//ldo
        write_reg8(0x1706e4, read_reg8(0x1706e4)&0xfc);//ldo
        write_reg8(0x1706e6, read_reg8(0x1706e6)&0xfc);//ldo

        //rf_tx_fast_settle_dis
        write_reg8(0x170629, read_reg8(0x170629)&0xef);
        write_reg8(0x1706f6, read_reg8(0x1706f6)&0xfe);//hpmc
        write_reg8(0x1706e2, read_reg8(0x1706e2)&0xfe);//ldo
        write_reg8(0x1706e4, read_reg8(0x1706e4)&0xfc);//ldo
        write_reg8(0x1706e6, read_reg8(0x1706e6)&0xfc);//ldo
    #endif

    ble_rf_cs_set_rx_cali_value(&rx_cs_cali);
    ble_rf_cs_set_tx_cali_value(&tx_cs_cali);

#if (1)//2023-11-8_2
    //seq_ldo_pll_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(3));    //LDO_PLL_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(3));    //LDO_PLL_PUP_OW

    //seq_ldo_vco_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(4));    //LDO_VCO_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(4));    //LDO_VCO_PUP_OW

    //seq_ldo_pll_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(3))); //LDO_PLL_FC
    write_reg8(0x170761,read_reg8(0x170761)|BIT(3));    //LDO_PLL_FC_O

    //rf_seq_ldo_vco_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(4))); //LDO_VCO_FC
    write_reg8(0x170761,read_reg8(0x170761)|BIT(4));    //LDO_VCO_FC_OW

    //seq_pd_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(0));    //PD_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(0));    //PD_PUP_OW

    //seq_pd_en_fcal_bias_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2))); //PD_EN_FCAL_BIAS
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));    //PD_EN_FCAL_BIAS_OW

    //seq_xo_en_clk_ref_ow
    write_reg8(0x170770,read_reg8(0x170770)|BIT(3));    //XO_EN_CLK_REF
    write_reg8(0x170770,read_reg8(0x170770)|BIT(1));    //XO_EN_CLK_REF_OW

    //seq_vco_pup_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(0));    //VCO_PUP
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(0));    //VCO_PUP_OW

    //seq_lo_pup_vlo_fbk_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(6));    //LO_PUP_VLO_FBK
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(6));    //LO_PUP_VLO_FBK_OW

    //seq_fcal_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3))); //FCAL_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));    //FCAL_PUP_OW

    //_seq_fcal_set_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4))); //FCAL_SET
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));    //FCAL_SET_OW

    //seq_fcal_run_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5))); //FCAL_RUN
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));    //FCAL_RUN_OW

    //seq_divn_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(6));    //DIVN_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(6));    //DIVN_PUP_OW

    //seq_divn_openloop_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(7))); //DIVN_OPENLOOP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(7));    //DIVN_OPENLOOP_OW

    //ldo_rxtxhf_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(1));    //LDO_RXTXHF_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(1));    //LDO_RXTXHF_PUP_OW

    //ldo_lv_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(0));    //LDO_LV_PUP
    write_reg8(0x170760,read_reg8(0x170760)|BIT(0));    //LDO_LV_PUP_OW

    //bg_pup_ow
    write_reg8(0x170766,read_reg8(0x170766)|BIT(0));    //BG_PUP
    write_reg8(0x170764,read_reg8(0x170764)|BIT(0));    //BG_PUP_OW

    //rf_mixer_pup_ow
    write_reg8(0x17077b,read_reg8(0x17077b)|BIT(3));    //RX_MIX_PUP
    write_reg8(0x170778,read_reg8(0x170778)|BIT(4));    //RX_MIX_PUP_OW

    //dsm_run
    write_reg8(0x170682,read_reg8(0x170682)|BIT(0));    //DSM_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(0));    //DSM_RUN_OW

    write_reg8(0x170450,read_reg8(0x170450)&(~BIT(5))); //GFSK_AUTO
    write_reg8(0x170453,read_reg8(0x170453)|(BIT(1)));  //FREQ_COMP_EN
    write_reg8(0x170452,read_reg8(0x170452)|(BIT(5)));  //GFSK_EN

    write_reg8(0x170451,read_reg8(0x170451)|(BIT(1)));  //FREQ_COMP_AUTO

    //rf_hpm_cal_disable
    write_reg8(0x170688,read_reg8(0x170688)&(~BIT(3))); //TX_HPM_CAL_EN
    write_reg8(0x170686,read_reg8(0x170686)|BIT(3));    //TX_HPM_CAL_EN_OW

    //rf_seq_lo_pup_vlo_txfsk_ow
    write_reg8(0x170792,read_reg8(0x170792)|BIT(6));    //LO_PUP_VLO_TXFSK
    write_reg8(0x170790,read_reg8(0x170790)|BIT(6));    //LO_PUP_VLO_TXFSK_OW
    write_reg8(0x170792,read_reg8(0x170792)|BIT(7));    //LO_PUP_VLO_TXFSKDRV
    write_reg8(0x170790,read_reg8(0x170790)|BIT(7));    //LO_PUP_VLO_TXFSKDRV_OW

    //seq_lo_pup_vlo_rx_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(2));    //LO_PUP_VLO_RX
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(2));    //LO_PUP_VLO_RX_OW
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(3));    //LO_PUP_VLO_RXDRV
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(3));    //LO_PUP_VLO_RXDRV_OW
#elif (0)//2023-10-20
    //seq_ldo_pll_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(3));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(3));

    //seq_ldo_vco_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(4));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(4));

    //seq_ldo_pll_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(3)));
    write_reg8(0x170761,read_reg8(0x170761)|BIT(3));

    //rf_seq_ldo_vco_fc_ow
    write_reg8(0x170763,read_reg8(0x170763)&(~BIT(4)));
    write_reg8(0x170761,read_reg8(0x170761)|BIT(4));

    //seq_pd_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(0));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(0));

    //seq_pd_en_fcal_bias_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));

    //seq_xo_en_clk_ref_ow
    write_reg8(0x170770,read_reg8(0x170770)|BIT(3));
    write_reg8(0x170770,read_reg8(0x170770)|BIT(1));

    //seq_vco_pup_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(0));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(0));

    //seq_lo_pup_vlo_fbk_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(6));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(6));

    //seq_fcal_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));

    //_seq_fcal_set_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));

    //seq_fcal_run_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));

    //seq_divn_pup_ow
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(6));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(6));

    //seq_divn_openloop_ow
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(7)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(7));

    //ldo_rxtxhf_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(1));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(1));

    //ldo_lv_pup_ow
    write_reg8(0x170762,read_reg8(0x170762)|BIT(0));
    write_reg8(0x170760,read_reg8(0x170760)|BIT(0));

    //bg_pup_ow
    write_reg8(0x170766,read_reg8(0x170766)|BIT(0));
    write_reg8(0x170764,read_reg8(0x170764)|BIT(0));

    //rf_mixer_pup_ow
    write_reg8(0x17077b,read_reg8(0x17077b)|BIT(3));
    write_reg8(0x170778,read_reg8(0x170778)|BIT(4));

    //dsm_run
    write_reg8(0x170682,read_reg8(0x170682)|BIT(0));
    write_reg8(0x170680,read_reg8(0x170680)|BIT(0));

    //rf_rx_dig_mixer_en_ow
    write_reg8(0x170688,read_reg8(0x170688)|BIT(1));
    write_reg8(0x170686,read_reg8(0x170686)|BIT(1));

    //rf_hpm_cal_disable
    write_reg8(0x170688,read_reg8(0x170688)&(~BIT(3)));
    write_reg8(0x170686,read_reg8(0x170686)|BIT(3));

    //rf_seq_lo_pup_vlo_txfsk_ow
    write_reg8(0x170792,read_reg8(0x170792)|BIT(6));
    write_reg8(0x170790,read_reg8(0x170790)|BIT(6));
    write_reg8(0x170792,read_reg8(0x170792)|BIT(7));
    write_reg8(0x170790,read_reg8(0x170790)|BIT(7));

    //seq_lo_pup_vlo_rx_ow
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(2));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(2));
    write_reg8(0x17078e,read_reg8(0x17078e)|BIT(3));
    write_reg8(0x17078c,read_reg8(0x17078c)|BIT(3));
#endif

    ble_rf_cs_settle_sequence_mode(RF_HADM_SETTLE_SEQ_ON);

    cs_phase_continuity_flag = 1;
}

_attribute_ram_code_ void ble_rf_cs_phase_continuity_dis(unsigned char phase_en)
{
    ble_rf_cs_settle_sequence_mode(RF_HADM_SETTLE_SEQ_OFF);

    //ble_rf_cs_restore_cali_auto_run();
    ble_rf_cs_restore_cali_auto_run(phase_en);

#if (FAST_SETTLE)
    #if !SW_DCOC_EN
        ble_rf_set_dcoc_cal_val(fast_settle.dcoc_cal);
    #endif

    ble_rf_set_ldo_trim_val(fast_settle.ldo_trim);

    rf_ble_set_rx_settle(RX_SETTLE_US);
    rf_ble_set_tx_settle(TX_STL_TIFS_REAL_COMMON);

    //rf_rx_fast_settle_en
    write_reg8(0x170629,read_reg8(0x170629)|0x08);

    //rf_tx_fast_settle_en
    write_reg8(0x170629,read_reg8(0x170629)|0x10);
#endif

    cs_phase_continuity_flag = 0;
}

#if (1)
/**
 * @brief       This function is mainly used to enable the rx-related trim functions that are bypassed during channel sounding.
 * @param[in]   phase_en : Used to control whether the digital_MIX/GFSK/VCO is continuous or not. 1:Maintaining continuity;0:No longer continuous.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_restore_cali_auto_run(unsigned char phase_en)
{
#if (1)//2023-11-9
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));  //FCAL_DEBUG_RUN_OW

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS

    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//LDOT_DEBUG_RUN_OW

//  write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);      //HPMC_BYPASS

//  write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW

    write_reg8(0x1706ce,read_reg8(0x1706ce)&0xfe);  //DCOC_BYPASS_ADC

    write_reg8(0x1706d0,read_reg8(0x1706d0)&0xfe);  //DCOC_BYPASS_DAC

    write_reg8(0x1706c6,read_reg8(0x1706c6)&0xfe);      //RCCAL_DBG1_0-->BYPASS
    write_reg8(0x1706c6,(read_reg8(0x1706c6)&(~BIT(6))));//CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4))); //RXDCOC_RUN_OW

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0)));//RX_LNA_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(3))); //LDO_PLL_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(3))); //LDO_PLL_FC_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(4))); //LDO_VCO_FC_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(0))); //PD_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1))); //XO_EN_CLK_REF_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6))); //LO_PUP_VLO_FBK_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(6))); //DIVN_PUP_OW
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(7))); //DIVN_OPENLOOP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(1))); //LDO_RXTXHF_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW
//  write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

    if(phase_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW

        write_reg8(0x170450,read_reg8(0x170450)&(~BIT(5))); //GFSK_AUTO
        write_reg8(0x170453,read_reg8(0x170453)|(BIT(1)));  //FREQ_COMP_EN
        write_reg8(0x170452,read_reg8(0x170452)|(BIT(5)));  //GFSK_EN

        //no need, Default value BIT(1)
        //write_reg8(0x170451,read_reg8(0x170451)|(BIT(1)));    //FREQ_COMP_AUTO

        //VCO
        write_reg8(0x1706f6,read_reg8(0x1706f6)|BIT(0));    //HPMC_BYPASS

        write_reg8(0x170680,read_reg8(0x170680)|(BIT(5)));  //HPMC_RUN_OW
        write_reg8(0x170760,read_reg8(0x170760)|(BIT(4)));  //LDO_VCO_PUP_OW
        write_reg8(0x17078c,read_reg8(0x17078c)|(BIT(0)));  //VCO_PUP_OW
        write_reg8(0x170788,read_reg8(0x170788)|(BIT(6)));  //DIVN_PUP_OW
        write_reg8(0x170788,read_reg8(0x170788)|(BIT(7)));  //DIVN_OPENLOOP_OW

        write_reg8(0x170760,read_reg8(0x170760)|(BIT(0)));  //LDO_LV_PUP_OW
        write_reg8(0x170764,read_reg8(0x170764)|(BIT(0)));  //BG_PUP_OW

        write_reg8(0x170790,read_reg8(0x170790)|(BIT(6)));  //LO_PUP_VLO_TXFSK_OW

        write_reg8(0x17078c,read_reg8(0x17078c)|(BIT(2)));  //LO_PUP_VLO_RX_OW
    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW

        write_reg8(0x170450,read_reg8(0x170450)|(BIT(5)));  //GFSK_AUTO
        write_reg8(0x170453,read_reg8(0x170453)&(~BIT(1))); //FREQ_COMP_EN
        write_reg8(0x170452,read_reg8(0x170452)&(~BIT(5))); //GFSK_EN

        //can not be closed
        //write_reg8(0x170451,read_reg8(0x170451)&(~BIT(1)));   //FREQ_COMP_AUTO

        //VCO
        write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);      //HPMC_BYPASS

        write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW
        write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW
        write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW
        write_reg8(0x170788,read_reg8(0x170788)&(~BIT(6))); //DIVN_PUP_OW
        write_reg8(0x170788,read_reg8(0x170788)&(~BIT(7))); //DIVN_OPENLOOP_OW

        write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW
        write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

        write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

        write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW
    }

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(0))); //DSM_RUN_OW

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3))); //TX_HPM_CAL_EN_OW

//  write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7))); //LO_PUP_VLO_TXFSKDRV_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3))); //LO_PUP_VLO_RXDRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW
#elif (0)//2023-11-8_2
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));  //FCAL_DEBUG_RUN_OW

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal
    write_reg8(0x170788,0);

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS

    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//LDOT_DEBUG_RUN_OW

//  write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);      //HPMC_BYPASS

//  write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW

    write_reg8(0x1706ce,read_reg8(0x1706ce)&0xfe);  //DCOC_BYPASS_ADC

    write_reg8(0x1706d0,read_reg8(0x1706d0)&0xfe);  //DCOC_BYPASS_DAC

    write_reg8(0x1706c6,read_reg8(0x1706c6)&0xfe);      //RCCAL_DBG1_0-->BYPASS
    write_reg8(0x1706c6,(read_reg8(0x1706c6)&(~BIT(6))));//CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4))); //RXDCOC_RUN_OW

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0)));//RX_LNA_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(3))); //LDO_PLL_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(3))); //LDO_PLL_FC_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(4))); //LDO_VCO_FC_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(0))); //PD_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1))); //XO_EN_CLK_REF_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6))); //LO_PUP_VLO_FBK_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(6))); //DIVN_PUP_OW
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(7))); //DIVN_OPENLOOP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(1))); //LDO_RXTXHF_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW
//  write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

//  write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
    if(mix_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW
//      write_reg8(0x170686,read_reg8(0x170686)|(BIT(1)));  //RX_DIG_EN_OW
    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)&(~BIT(1))); //RX_DIG_EN_OW
    }

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(0))); //DSM_RUN_OW

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3))); //TX_HPM_CAL_EN_OW

//  write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7))); //LO_PUP_VLO_TXFSKDRV_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3))); //LO_PUP_VLO_RXDRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW
#elif (0)//2023-11-8_1
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));  //FCAL_DEBUG_RUN_OW

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal
    write_reg8(0x170788,0);

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS

    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//LDOT_DEBUG_RUN_OW

//  write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);      //HPMC_BYPASS

//  write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW

    write_reg8(0x1706ce,read_reg8(0x1706ce)&0xfe);  //DCOC_BYPASS_ADC

    write_reg8(0x1706d0,read_reg8(0x1706d0)&0xfe);  //DCOC_BYPASS_DAC

    write_reg8(0x1706c6,read_reg8(0x1706c6)&0xfe);      //RCCAL_DBG1_0-->BYPASS
    write_reg8(0x1706c6,(read_reg8(0x1706c6)&(~BIT(6))));//CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4))); //RXDCOC_RUN_OW

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0)));//RX_LNA_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(3))); //LDO_PLL_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(3))); //LDO_PLL_FC_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(4))); //LDO_VCO_FC_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(0))); //PD_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1))); //XO_EN_CLK_REF_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6))); //LO_PUP_VLO_FBK_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(6))); //DIVN_PUP_OW
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(7))); //DIVN_OPENLOOP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(1))); //LDO_RXTXHF_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW
//  write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

//  write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
    if(mix_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)|(BIT(1)));  //RX_DIG_EN_OW
    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)&(~BIT(1))); //RX_DIG_EN_OW
    }

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(0))); //DSM_RUN_OW

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3))); //TX_HPM_CAL_EN_OW

//  write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7))); //LO_PUP_VLO_TXFSKDRV_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3))); //LO_PUP_VLO_RXDRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW
#elif (0)//2023-11-7 unavailable scheme
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));  //FCAL_DEBUG_RUN_OW

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS

    write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW

    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//LDOT_DEBUG_RUN_OW

    write_reg8(0x1706d0,read_reg8(0x1706d0)&0xfe);  //DCOC_BYPASS_DAC

    write_reg8(0x1706ce,read_reg8(0x1706ce)&0xfe);  //DCOC_BYPASS_ADC

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));    //RCCAL_RUN_OW

    write_reg8(0x1706c6,read_reg8(0x1706c6)&0xfe);      //RCCAL_DBG1_0-->BYPASS
    write_reg8(0x1706c6,(read_reg8(0x1706c6)&(~BIT(6))));//CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4))); //RXDCOC_RUN_OW

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0)));//RX_LNA_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(3))); //LDO_PLL_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(3))); //LDO_PLL_FC_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(4))); //LDO_VCO_FC_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(0))); //PD_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1))); //XO_EN_CLK_REF_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6))); //LO_PUP_VLO_FBK_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(6))); //DIVN_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(7))); //DIVN_OPENLOOP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(1))); //LDO_RXTXHF_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW

    write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

//  write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
    if(mix_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)|(BIT(1)));  //RX_DIG_EN_OW
    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)&(~BIT(1))); //RX_DIG_EN_OW
    }

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(0))); //DSM_RUN_OW

//  write_reg8(0x170686,read_reg8(0x170686)&(~BIT(1))); //RX_DIG_EN_OW

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3))); //TX_HPM_CAL_EN_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7))); //LO_PUP_VLO_TXFSKDRV_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3))); //LO_PUP_VLO_RXDRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW
#elif (1)//2023-11-6
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));  //FCAL_DEBUG_RUN_OW

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal
    write_reg8(0x170788,0);

    write_reg8(0x1706e2 ,read_reg8(0x1706e2)&0xfe);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,read_reg8(0x1706e4)&0xfc);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS

    write_reg8(0x1706e6 ,read_reg8(0x1706e6)&0xfc);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS

    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//LDOT_DEBUG_RUN_OW

//  write_reg8(0x1706f6,read_reg8(0x1706f6)&0xfe);      //HPMC_BYPASS

//  write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5))); //HPMC_RUN_OW

    write_reg8(0x1706ce,read_reg8(0x1706ce)&0xfe);  //DCOC_BYPASS_ADC

    write_reg8(0x1706c6,read_reg8(0x1706c6)&0xfe);      //RCCAL_DBG1_0-->BYPASS
    write_reg8(0x1706c6,(read_reg8(0x1706c6)&(~BIT(6))));//CBPF_CCODE_BYPASS

    write_reg8(0x1706c7,(read_reg8(0x1706c7)&(~BIT(6))));//RCCAL_DBG1_1 --> COMP_POL

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4))); //RXDCOC_RUN_OW

    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(0)));//RX_LNA_PUP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(3))); //LDO_PLL_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(4))); //LDO_VCO_PUP_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(3))); //LDO_PLL_FC_OW

    write_reg8(0x170761,read_reg8(0x170761)&(~BIT(4))); //LDO_VCO_FC_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(0))); //PD_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170770,read_reg8(0x170770)&(~BIT(1))); //XO_EN_CLK_REF_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(0))); //VCO_PUP_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(6))); //LO_PUP_VLO_FBK_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(6))); //DIVN_PUP_OW
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(7))); //DIVN_OPENLOOP_OW

    write_reg8(0x170760,read_reg8(0x170760)&(~BIT(1))); //LDO_RXTXHF_PUP_OW

//  write_reg8(0x170760,read_reg8(0x170760)&(~BIT(0))); //LDO_LV_PUP_OW
//  write_reg8(0x170764,read_reg8(0x170764)&(~BIT(0))); //BG_PUP_OW

//  write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
    if(mix_en)
    {
        write_reg8(0x170778,read_reg8(0x170778)|BIT(4)); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)|(BIT(1)));  //RX_DIG_EN_OW
    }
    else
    {
        write_reg8(0x170778,read_reg8(0x170778)&(~BIT(4))); //RX_MIX_PUP_OW
        write_reg8(0x170686,read_reg8(0x170686)&(~BIT(1))); //RX_DIG_EN_OW
    }

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(0))); //DSM_RUN_OW

    write_reg8(0x170686,read_reg8(0x170686)&(~BIT(3))); //TX_HPM_CAL_EN_OW

//  write_reg8(0x170790,read_reg8(0x170790)&(~BIT(6))); //LO_PUP_VLO_TXFSK_OW

    write_reg8(0x170790,read_reg8(0x170790)&(~BIT(7))); //LO_PUP_VLO_TXFSKDRV_OW

//  write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(2))); //LO_PUP_VLO_RX_OW

    write_reg8(0x17078c,read_reg8(0x17078c)&(~BIT(3))); //LO_PUP_VLO_RXDRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2))); //PD_EN_FCAL_BIAS_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3))); //FCAL_PUP_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4))); //FCAL_SET_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5))); //FCAL_RUN_OW

    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1))); //PD_EN_PD_DRV_OW
#elif (0)//2023-11-2
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//ldo
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));//fcal
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//fcal
    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal

    write_reg8(0x170788,0);
    //write_reg16(0x170760,0);
    //write_reg8(0x17078c,0);
    //write_reg8(0x170764,0);
    if(mix_en)
    {
        write_reg8(0x170778,0x10);
    }
    else
    {
        write_reg8(0x170778,0);
    }
#else//2023-10-28
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//ldo
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));//fcal
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//fcal

    write_reg8(0x170788,0);
    write_reg16(0x170760,0);
    write_reg8(0x17078c,0);
    write_reg8(0x170764,0);
    if(mix_en)
    {
        write_reg8(0x170778,0x10);
    }
    else
    {
        write_reg8(0x170778,0);
    }
    //write_reg8(0x170778,0);//remove
    write_reg8(0x170680,0x40);
    write_reg8(0x170686,0x60);
    write_reg8(0x170790,0x00);
    write_reg8(0x17078c,0x00);

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal
    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4)));//dcoc
    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5)));//hpmc

    write_reg8(0x1706d0,read_reg8(0x1706d0)&(~BIT(0)));//dcoc
    write_reg8(0x1706ce,read_reg8(0x1706ce)&(~BIT(0)));//dcoc,adc  iq code
    write_reg8(0x1706c6,read_reg8(0x1706c6)&(~BIT(0)));//rccal
    write_reg8(0x1706f6,read_reg8(0x1706f6)&(~BIT(0)));//hpmc
    write_reg8(0x1706e2,read_reg8(0x1706e2)&(~BIT(0)));//ldo
    write_reg8(0x1706e4, read_reg8(0x1706e4)&0xfc);//ldo RXTXHF
    write_reg8(0x1706e6, read_reg8(0x1706e6)&0xfc);//ldo RXTXLF
#endif
}

#else
/**
 * @brief       This function is mainly used to enable the rx-related trim functions that are bypassed during channel sounding.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_restore_cali_auto_run(void)
{
    //2023-10-20
    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(2)));//ldo

    write_reg8(0x170681,read_reg8(0x170681)&(~BIT(3)));//fcal
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//fcal
    write_reg8(0x170788,read_reg8(0x170788)&0xc1);//fcal

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(2)));//rccal

    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(4)));//dcoc
//  write_reg8(0x170682,read_reg8(0x170682)|BIT(5));//hpmc
    write_reg8(0x170680,read_reg8(0x170680)&(~BIT(5)));//hpmc

    write_reg8(0x1706d0,read_reg8(0x1706d0)&(~BIT(0)));//dcoc
    write_reg8(0x1706ce,read_reg8(0x1706ce)&(~BIT(0)));//dcoc,adc  iq code
    write_reg8(0x1706c6,read_reg8(0x1706c6)&(~BIT(0)));//rccal
    write_reg8(0x1706f6,read_reg8(0x1706f6)&(~BIT(0)));//hpmc
    write_reg8(0x1706e2,read_reg8(0x1706e2)&(~BIT(0)));//ldo
    write_reg8(0x1706e4, read_reg8(0x1706e4)&0xfc);//ldo RXTXHF
    write_reg8(0x1706e6, read_reg8(0x1706e6)&0xfc);//ldo RXTXLF
}
#endif

/**
 * @brief       This function is mainly used to set the preparation and enable of manual fcal(frequency calibration).
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_manual_fcal_start(void)
{
//  rf_seq_pd_en_pd_drv_ow(0);
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(1));//PD_EN_PD_DRV
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(1)));//PD_EN_PD_DRV_OW

    write_reg8(0x170738,read_reg8(0x170738)|BIT(2));//BYPASS_CAL_CLK_GAT

//  rf_seq_pd_en_fcal_bias_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(2));//PD_EN_FCAL_BIAS
    write_reg8(0x170788,read_reg8(0x170788)|BIT(2));//PD_EN_FCAL_BIAS_OW

//  rf_seq_fcal_pup_ow1();
    write_reg8(0x17078a,read_reg8(0x17078a)|BIT(3));//FCAL_PUP
    write_reg8(0x170788,read_reg8(0x170788)|BIT(3));//FCAL_PUP_OW

//  rf_seq_fcal_set_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(4)));//FCAL_SET
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(4)));//FCAL_SET_OW

//  rf_seq_fcal_run_disow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(5)));//FCAL_RUN
    write_reg8(0x170788,read_reg8(0x170788)&(~BIT(5)));//FCAL_RUN_OW

    write_reg8(0x170683,read_reg8(0x170683)|BIT(3));//FCAL_DEBUG_RUN
}


/**
 * @brief       This function is mainly used to set the relevant value after manual fcal(frequency calibration).
 * @return      none.
 * @note        The function needs to be called after the rf_manual_fcal_start call 22us.
 */
_attribute_ram_code_ void ble_rf_manual_fcal_done(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//FCAL_DEBUG_RUN
    write_reg8(0x170738,read_reg8(0x170738)&(~BIT(2)));//BYPASS_CAL_CLK_GAT

//  rf_seq_pd_en_fcal_bias_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(2)));//PD_EN_FCAL_BIAS
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(2)));

//  rf_seq_fcal_pup_ow();
    write_reg8(0x17078a,read_reg8(0x17078a)&(~BIT(3)));//FCAL_PUP
//  write_reg8(0x170788,read_reg8(0x170788)&(~BIT(3)));

//  rf_seq_fcal_set_ow();
//  write_reg8(0x17078a,read_reg8(0x17078a)|(BIT(4)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(4));//FCAL_SET_OW

//  rf_seq_fcal_run_ow();
//  write_reg8(0x17078a,read_reg8(0x17078a)|(BIT(5)));
    write_reg8(0x170788,read_reg8(0x170788)|BIT(5));//FCAL_RUN_OW

//  rf_seq_pd_en_pd_drv_ow(1);
    write_reg8(0x170788,read_reg8(0x170788)|BIT(1));//PD_EN_PD_DRV_OW
}

/**
 * @brief       This function is mainly used to get the calibration value of the rx state that needs to be
 *              recorded in the cs function.
 * @param[out]  rx_cali -   Pointer to a structure that stores the value associated with the rx calibration.
 * @return      none.
 * @note        This function is usually called after a package has been received.
 */
_attribute_ram_code_ void ble_rf_cs_get_rx_cali_value(rf_cs_rx_cali_t *rx_cali)
{
    ble_rf_get_ldo_trim_val(&rx_cali->ldo_trim);
#if !SW_DCOC_EN
    ble_rf_get_dcoc_cal_val(&rx_cali->dcoc_cal);
#endif
    ble_rf_get_rccal_cal_val(&rx_cali->rccal_cal);
}

/**
 * @brief       This function is mainly used to get the calibration value of the tx state that needs to be
 *              recorded in the cs function.
 * @param[out]  rx_cali -   Pointer to a structure that stores the value associated with the tx calibration.
 * @return      none.
 * @note        This function is usually called after a package has been sent.
 */
_attribute_ram_code_ void ble_rf_cs_get_tx_cali_value(rf_cs_tx_cali_t *tx_cali)
{
    ble_rf_get_ldo_trim_val(&tx_cali->ldo_trim);
    extern unsigned short rf_get_hpmc_cal_val(void);
    tx_cali->tx_hpmc = rf_get_hpmc_cal_val();
}

/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_ void ble_rf_get_ldo_trim_val(rf_ldo_trim_t *ldo_trim)
{
    ldo_trim->LDO_CAL_TRIM = read_reg8(0x1706ea) & 0x3f;
    ldo_trim->LDO_RXTXHF_TRIM = read_reg8(0x1706ec) & 0x3f;
    ldo_trim->LDO_RXTXLF_TRIM = ((read_reg8(0x1706ed) & 0x0f) << 2) + ((read_reg8(0x1706ec) & 0xc0) >> 6);
    ldo_trim->LDO_PLL_TRIM = read_reg8(0x1706ee) & 0x3f;
    ldo_trim->LDO_VCO_TRIM = ((read_reg8(0x1706ef) & 0x0f) << 2) + ((read_reg8(0x1706ee) & 0xc0) >> 6);
}

#if !SW_DCOC_EN
/**
 *  @brief      This function is mainly used to get LDO Calibration-related values.
 *  @param[in]  dcoc_cal   - dcoc calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_ void ble_rf_get_dcoc_cal_val(rf_dcoc_cal_t *dcoc_cal)
{
    dcoc_cal->DCOC_IDAC = read_reg8(0x1706d8) & 0x3f;//DCOC_IDAC 0xd8[5:0]
    dcoc_cal->DCOC_QDAC = read_reg8(0x1706da) & 0x3f;//DCOC_QDAC 0xda[5:0]
    dcoc_cal->DCOC_IADC_OFFSET = read_reg8(0x1706dc) & 0x7f;//DCOC_IADC_OFFSET 0xdc[6:0]
    dcoc_cal->DCOC_QADC_OFFSET = (read_reg8(0x1706dc) & 0x80) >> 7 |(read_reg8(0x1706dd) & 0x3f) << 1;//DCOC_QADC_OFFSET 0xdc[7] 0xdd[5:0]
}

/**
 *  @brief      This function is mainly used to set dcoc Calibration-related values.
 *  @param[in]  dcoc_cal    - dcoc Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_ void ble_rf_set_dcoc_cal_val(rf_dcoc_cal_t dcoc_cal)
{
    write_reg8(0x1706d0,(dcoc_cal.DCOC_IDAC << 1) | 0x01);//DCOC_BYPASS_DAC
    write_reg8(0x1706d0,read_reg8(0x1706d0)|((dcoc_cal.DCOC_QDAC&0x01) << 7));
    write_reg8(0x1706d1,((dcoc_cal.DCOC_QDAC)&0x3e) >> 1);
    write_reg8(0x1706ce,(dcoc_cal.DCOC_IADC_OFFSET << 1) | 0x01);//DCOC_BYPASS_ADC
    write_reg8(0x1706cf,dcoc_cal.DCOC_QADC_OFFSET);
}

/**
 * @brief       This function is mainly used for the disable dcoc trim function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_dis_dcoc_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(4)));//RXDCOC_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(4));//RXDCOC_RUN_OW
}
#endif

/**
 *  @brief      This function is mainly used to get rccal Calibration-related values.
 *  @param[in]  rccal_cal   - rccal calibration value address pointer
 *  @return     none
*/
_attribute_ram_code_ void ble_rf_get_rccal_cal_val(rf_rccal_cal_t *rccal_cal)
{
    rccal_cal->RCCAL_CODE = read_reg8(0x1706ca)&0x1f;
    rccal_cal->CBPF_CCODE_L = read_reg8(0x1706ca)&0xe0 >> 5;
    rccal_cal->CBPF_CCODE_H = read_reg8(0x1706cb)&0x0f;
}

/**
 * @brief       This function is mainly used to enable LNA.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_lna_pup(void)
{
    write_reg8(0x17077a,read_reg8(0x17077a)|BIT(0));//RX_LNA_PUP
    write_reg8(0x170778,read_reg8(0x170778)|BIT(0));//RX_LNA_PUP_OW
}

/**
 * @brief       This function is mainly used to write the calibration value obtained through the rf_cs_get_rx_cali_vlue
 *              function to the corresponding register.
 * @param[in]   rx_cali     -   rx calibration value obtained by the rf_cs_get_rx_cali_vlue function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_set_rx_cali_value(rf_cs_rx_cali_t *pRx_cali)
{
    ble_rf_dis_fcal_trim();
    ble_rf_set_ldo_trim_val(pRx_cali->ldo_trim);
    ble_rf_dis_ldo_trim();
#if !SW_DCOC_EN
    ble_rf_set_dcoc_cal_val(pRx_cali->dcoc_cal);
#endif
    ble_rf_dis_rccal_trim();
    ble_rf_set_rccal_cal_val(pRx_cali->rccal_cal);
#if !SW_DCOC_EN
    ble_rf_dis_dcoc_trim();
#endif
    ble_rf_lna_pup();
}

/**
 * @brief       This function is used to write the tx calibration value obtained by rf_cs_get_tx_cali_vlue to the
 *              corresponding register.
 * @param[in]   tx_cali     -   tx calibration value obtained by the rf_cs_get_tx_cali_vlue function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_set_tx_cali_value(rf_cs_tx_cali_t *pTx_cali)
{
    ble_rf_dis_fcal_trim();
    ble_rf_set_ldo_trim_val(pTx_cali->ldo_trim);
    extern void rf_set_hpmc_cal_val(unsigned short hpmc_gain);
    rf_set_hpmc_cal_val(pTx_cali->tx_hpmc);
    ble_rf_dis_hpmc_trim();
}

/**
 * @brief       This function is mainly used for the disable fcal trim function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_dis_fcal_trim(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(3)));//FCAL_DEBUG_RUN
    write_reg8(0x170681,read_reg8(0x170681)|BIT(3));//FCAL_DEBUG_RUN_OW
}

/**
 *  @brief      This function is mainly used to set LDO Calibration-related values.
 *  @param[in]  ldo_trim   - ldo trim Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_ void ble_rf_set_ldo_trim_val(rf_ldo_trim_t ldo_trim)
{
    write_reg8(0x1706e2 ,(ldo_trim.LDO_CAL_TRIM << 1) | 0x01);//LDOT_LDO_CAL_BYPASS
    write_reg8(0x1706e4 ,(ldo_trim.LDO_RXTXHF_TRIM << 2) | 0x03);//LDOT_LDO_RXTXHF_BYPASS,LDOT_LDO_RXTXLF_BYPASS
    write_reg8(0x1706e5 , ldo_trim.LDO_RXTXLF_TRIM);
    write_reg8(0x1706e6 ,(ldo_trim.LDO_PLL_TRIM << 2) | 0x03);//LDOT_LDO_PLL_BYPASS,LDOT_LDO_VCO_BYPASS
    write_reg8(0x1706e7 , ldo_trim.LDO_VCO_TRIM);
}

/**
 * @brief       This function is mainly used for the disable ldo trim function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_dis_ldo_trim(void)
{
    write_reg8(0x170683,read_reg8(0x170683)&(~BIT(2)));//LDOT_DEBUG_RUN
    write_reg8(0x170681,read_reg8(0x170681)|BIT(2));//LDOT_DEBUG_RUN_OW
}

/**
 * @brief       This function is mainly used for the disable rccal trim function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_dis_rccal_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(2)));//RCCAL_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(2));//RCCAL_RUN_OW
}

/**
 *  @brief      This function is mainly used to set rccal Calibration-related values.
 *  @param[in]  rccal_cal    - rccal Calibration-related values.
 *  @return     none
*/
_attribute_ram_code_ void ble_rf_set_rccal_cal_val(rf_rccal_cal_t rccal_cal)
{
    write_reg8(0x1706c6,(rccal_cal.RCCAL_CODE << 1) | 0x01);//RCCAL_DBG1_0-->BYPASS
    write_reg8(0x1706c6,(rccal_cal.CBPF_CCODE_L & 0x01 ) << 7 | (read_reg8(0x1706c6)|BIT(6)));//CBPF_CCODE_BYPASS
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_L & 0x06) >> 1 | read_reg8(0x1706c7));
    write_reg8(0x1706c7,(rccal_cal.CBPF_CCODE_H << 2 | (read_reg8(0x1706c7)|BIT(6))));//RCCAL_DBG1_1 --> COMP_POL
}

/**
 * @brief       This function is mainly used for the disable hpmc trim function.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_dis_hpmc_trim(void)
{
    write_reg8(0x170682,read_reg8(0x170682)&(~BIT(5)));//HPMC_RUN
    write_reg8(0x170680,read_reg8(0x170680)|BIT(5));//HPMC_RUN_OW
}

/**
 * @brief       This function is used to enable or disable the corresponding sequence of shuttle in channel sounding mode; usually call
 *              this function before entering mode1/mode2 and pass the parameter RF_HADM_SETTLE_SEQ_ON to enable the corresponding sequence;
 *              and call the parameter RF_HADM_SETTLE_SEQ_OFF to disable the sequence after ending channel sounding.
 * @param[in]   on_off : Used to control whether to enable settle sequence in channel sounding RF_HADM_SETTLE_SEQ_OFF:off,RF_HADM_SETTLE_SEQ_ON:on
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_cs_settle_sequence_mode(rf_hadm_settle_seq_mode_e on_off)
{
    if (on_off == RF_HADM_SETTLE_SEQ_ON)
    {
        //close hpmc and ldo trim,close hpmc(53us), ldotrim(4.5us),save 58us
        //Default settle time:108.5us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x17068a,0x00);  //sub-sequence1 start time:0
        write_reg8(0x17068b,0x08);  //sub-sequence2 start time:8us
        write_reg8(0x17068c,0x30);  //sub-sequence3 start time:48us
        write_reg8(0x17068d,0x31);  //sub-sequence4 start time:48.5us
        write_reg8(0x17068e,0x33);  //sub-sequence5 start time:51us
        write_reg8(0x17068f,0x30);  //sub-sequence6 start time:48us

        //RX: rx_ldo_trim (4.5us), rx_dcoc(40us)
        //RX Default settle time:85us
        //Fast settle time = Default settle time - Settle time of the closed module
        write_reg8(0x170690,0x00);  //sub-sequence1 start time:0us
        write_reg8(0x170691,0x09);  //sub-sequence2 start time:9us
        write_reg8(0x170692,0x09);  //sub-sequence3 start time:9us
        write_reg8(0x170693,0x1b);  //sub-sequence4 start time:27us
        write_reg8(0x170694,0x2d);  //sub-sequence5 start time:45us
        write_reg8(0x170695,0x2d);  //sub-sequence6 start time:45us
    }
    else if(on_off == RF_HADM_SETTLE_SEQ_OFF)
    {
        //Default settle time:108.5us
        write_reg8(0x17068a,0x00);  //sub-sequence1 start time:0
        write_reg8(0x17068b,0x0d);  //sub-sequence2 start time:13us
        write_reg8(0x17068c,0x6a);  //sub-sequence3 start time:106us
        write_reg8(0x17068d,0x6b);  //sub-sequence4 start time:107us
        write_reg8(0x17068e,0x6e);  //sub-sequence5 start time:110us
        write_reg8(0x17068f,0x6a);  //sub-sequence6 start time:106us

        //RX Default settle time:85us
        write_reg8(0x170690,0x00);  //sub-sequence1 start time:0us
        write_reg8(0x170691,0x0d);  //sub-sequence2 start time:13us
        write_reg8(0x170692,0x0d);  //sub-sequence3 start time:13us
        write_reg8(0x170693,0x27);  //sub-sequence4 start time:43us
        write_reg8(0x170694,0x52);  //sub-sequence5 start time:82us
        write_reg8(0x170695,0x52);  //sub-sequence6 start time:82us
    }
}
#endif


_attribute_ram_code_ void ble_rf_channel_sounding_init(void)
{
    reg_rf_mode_ctrl0 |= FLD_RF_INFO_EXTENSION;
    reg_dma_ctr3(1) = ((reg_dma_ctr3(1) & 0xf8) | RF_QWORLD_WIDTH);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | RF_QWORLD_WIDTH);

    write_reg8(0x170030, 0x3e); //enable tx timestamp
}

_attribute_ram_code_ void ble_rf_channel_sounding_deinit(void)
{
    reg_rf_mode_ctrl0 &= (~FLD_RF_INFO_EXTENSION);
    reg_dma_ctr3(1) = ((reg_dma_ctr3(1) & 0xf8) | RF_WORLD_WIDTH);
    reg_rf_burst_size = ((reg_rf_burst_size & 0xfc) | RF_WORLD_WIDTH);

    reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;

    write_reg8(0x170030, 0x36); //disable tx timestamp

    ble_rf_agc_enable();
}

_attribute_ram_code_ void ble_rf_tx_channel_sounding_mode_en(void)
{
    BM_CLR(reg_rf_tx_mode1,FLD_RF_CRC_EN);
    BM_CLR(reg_rf_tx_mode2,FLD_RF_V_PN_EN);
    BM_SET(reg_rf_tx_mode2,FLD_RF_R_CUSTOM_MADE);

    BM_SET_MASK_VAL(reg_rf_preamble_trail, FLD_RF_TRAILER_LEN, MV(FLD_RF_TRAILER_LEN, 0));
}

_attribute_ram_code_ void ble_rf_tx_channel_sounding_mode_dis(void)
{
    BM_SET(reg_rf_tx_mode1,FLD_RF_CRC_EN);
    BM_SET(reg_rf_tx_mode2,FLD_RF_V_PN_EN);
    BM_CLR(reg_rf_tx_mode2,FLD_RF_R_CUSTOM_MADE);

    BM_SET_MASK_VAL(reg_rf_preamble_trail, FLD_RF_TRAILER_LEN, MV(FLD_RF_TRAILER_LEN, 2));
}

/**
 * @brief       This function is mainly used to enable channel sounding IQ sample mode.
 * @param[in]   interval    - The interval time between each IQ sampling is (interval + 1)*0.125us.
 * @param[in]   suppmode    - The length of each I or Q data.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_rx_channel_sounding_mode_en(unsigned char interval, rf_iq_data_mode_e suppmode)
{
    //sample_interval_time: (1 + 1)*0.125us ---> 0.25us ---> 4MHz
    reg_rf_mode_ctrl0 = ((reg_rf_mode_ctrl0 & (~FLD_RF_IQ_SAMP_INTERVAL)) | (interval << 4));//The max sample rate is 4Mhz.
    reg_rf_sof_offset = ((reg_rf_sof_offset & (~FLD_RF_SUPP_MODE)) | ((suppmode&0x07) << 4));
    reg_rf_mode_ctrl0 |= FLD_RF_IQ_SAMP_EN;
}

_attribute_ram_code_ void ble_rf_rx_channel_sounding_mode_dis(void)
{
    reg_rf_mode_ctrl0 &= (~FLD_RF_IQ_SAMP_EN);
}

/**
 * @brief       This function is mainly used to initialize some parameter settings of channel sounding IQ sample.
 * @param[in]   sample_num  - Number of groups to sample IQ data.Value range 0x01~0xffff.
 * @param[in]   start_point - Set the starting point of the sample.If it is rx_en mode, sampling starts
 *                            at 0.25us+start_point*0.125us after settle. If it is in sync mode, sampling
 *                            starts at (start_point + 1) * 0.125us after sync.
 *                            Value range 0x00~0xff.
 * @param[in]   sample_mode - IQ sampling starts after syncing packets or after the rx_en is pulled up.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_channel_sounding_iq_sample_config(unsigned short sample_num, unsigned char start_point, rf_hadm_iq_sample_mode_e sample_mode)
{
    reg_rf_iq_samp_num = sample_num;
    reg_rf_iq_samp_start = start_point;
    if(sample_mode == RF_HADM_IQ_SAMPLE_SYNC_MODE)
    {
        reg_rf_rxlatf |= FLD_RF_R_IQ_SAMP_MODE;
    }
    else
    {
        reg_rf_rxlatf &= (~FLD_RF_R_IQ_SAMP_MODE);
    }
}

_attribute_ram_code_ void ble_rf_set_manual_tx_mode(void)
{
    reg_rf_ll_ctrl0 = 0x45;// reset tx/rx state machine.
    reg_rf_ll_ctrl0 |= FLD_RF_R_TX_EN_MAN;
    reg_rf_rxmode &= (~FLD_RF_RX_ENABLE);
}


_attribute_ram_code_ void ble_rf_set_tx_modulation_index(rf_mi_value_e mi_value)//only support RF_MI_P0p00 and RF_MI_P0p50
{
    unsigned char modulation_index_low;
    unsigned char kvm_trim = 0;

    if(mi_value == RF_MI_P0p00){
        modulation_index_low = 0;
    }
    else if(mi_value == RF_MI_P0p50){
        modulation_index_low = 64;
    }
    else{
        return;
    }

    if(reg_rf_mode_cfg_tx1_0 & 0x01)
    {
        kvm_trim = 1;
    }

    reg_rf_radio_mode_cfg_rx2_0 = modulation_index_low;
    reg_rf_mode_cfg_tx1_0 = ((reg_rf_mode_cfg_tx1_0 & (~FLD_RF_VCO_TRIM_KVM))|(kvm_trim<<1));
}

_attribute_ram_code_ void ble_rf_set_power_level_singletone(rf_power_level_e level)
{
    unsigned char value = (unsigned char)level & 0x3f;

    if(level & BIT(7))
    {
        reg_rf_mode_cfg_tx3_0 |= FLD_RF_MODE_VANT_TX_BLE;// VANT
    }
    else
    {
        reg_rf_mode_cfg_tx3_0 &= ~FLD_RF_MODE_VANT_TX_BLE;// VBAT
    }

    reg_rf_lnm_pa_ow_ctrl_val |= BIT(6);                            // TX_PA_PWR_OW  BIT6 set 1
    reg_rf_pa_ow_val = ((reg_rf_pa_ow_val & 0x81) | (value << 1));  // TX_PA_PWR  BIT1 t0 BIT6 set value
}

/**
 * @brief       This function is mainly used to turn off the energy of the tone.
 * @return      none.
 * @note        After setting the tone energy with ble_rf_set_power_level_singletone, you need to call
 *              ble_rf_set_power_off_singletone to turn off the tone energy if you enter the send packet.
 */
_attribute_ram_code_ void ble_rf_set_power_off_singletone(void)
{
    reg_rf_pa_ow_val &= 0x81;               // TX_PA_PWR  BIT1 t0 BIT6 set 0
    reg_rf_lnm_pa_ow_ctrl_val &= ~BIT(6);   // TX_PA_PWR_OW  BIT6 set 0
}

/**
 * @brief       This function serves to set rf channel for CS.The actual channel set by this function is 2402 + chn.
 * @param[in]   chn   - That you want to set the channel as 2402 + chn.
 * @return      none.
 */
_attribute_ram_code_ void ble_rf_set_cs_channel(signed char chn)
{
    rf_set_chn(chn + 2);
}

/**
 * @brief       This function is mainly used for freeze AGC(Automatic Gain Control).
 * @return      none.
 * @note        This function needs to be called after ble_rf_agc_enable, otherwise there will be problems with rssi
 *              value exceptions.
 */
_attribute_ram_code_ void ble_rf_agc_disable()//TODO optimize execution time
{
    char gain_lat, lna_hgain, lna_lgain, lna_attn, cbpf_gain;
    reg_rf_radio_txrx_dbg1_0 |= FLD_RF_AGC_DISABLE;
    gain_lat = (read_reg8(0x170059)>>4)&0x07;
    write_reg8(0x170640,(read_reg8(0x170640)&0xe3)|((gain_lat&0x07)<<2));

    if(gain_lat == 0)
    {
        lna_hgain = 0;
        lna_lgain = 1;
        lna_attn  = 3;
        cbpf_gain = 0;
    }
    else if(gain_lat == 1)
    {
        lna_hgain = 0;
        lna_lgain = 3;
        lna_attn  = 2;
        cbpf_gain = 1;
    }
    else if(gain_lat == 2)
    {
        lna_hgain = 0;
        lna_lgain = 3;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 3)
    {
        lna_hgain = 3;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 4)
    {
        lna_hgain = 0xf;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 5)
    {
        lna_hgain = 0x3f;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else if(gain_lat == 6)
    {
        lna_hgain = 0;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 1;
    }
    else
    {
        lna_hgain = 0;
        lna_lgain = 0;
        lna_attn  = 0;
        cbpf_gain = 0;
    }

    write_reg8(0x17077a,(read_reg8(0x17077a)&0x81)|(lna_hgain<<1));
    write_reg8(0x170778,read_reg8(0x170778)|0x02);

    write_reg8(0x17077b,(read_reg8(0x17077b)&0xfe)|(lna_lgain>>1));
    write_reg8(0x17077a,(read_reg8(0x17077a)&0x7f)|(lna_lgain<<7));
    write_reg8(0x170778,read_reg8(0x170778)|0x04);

    write_reg8(0x17077b,(read_reg8(0x17077b)&0xf9)|((lna_attn&0x03)<<1));
    write_reg8(0x170778,read_reg8(0x170778)|0x08);

    write_reg8(0x170782,(read_reg8(0x170782)&0xfd)|(cbpf_gain&0x01)<<1);
    write_reg8(0x170780,read_reg8(0x170780)|0x02);
}

/**
 * @brief       This function is mainly used for agc auto run.
 * @return      none.
 * @note        Call this function to enable agc auto tuning if you want to receive different energy packets correctly
 *              after calling ble_rf_agc_disable to disable agc auto tuning.
 */
_attribute_ram_code_ void ble_rf_agc_enable(void)//TODO optimize execution time
{
    reg_rf_radio_txrx_dbg1_0 &= (~FLD_RF_AGC_DISABLE);
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(1)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(2)));
    write_reg8(0x170778,read_reg8(0x170778)&(~BIT(3)));
    write_reg8(0x170780,read_reg8(0x170780)&(~BIT(1)));
}
