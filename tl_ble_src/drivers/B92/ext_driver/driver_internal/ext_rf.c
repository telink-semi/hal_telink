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
    write_reg8 (0x17000d, chn_num);

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
_attribute_data_retention_ Fast_Settle fast_settle_1M;
_attribute_data_retention_ Fast_Settle fast_settle_2M;
_attribute_data_retention_ Fast_Settle fast_settle_S2;
_attribute_data_retention_ Fast_Settle fast_settle_S8;
#endif
