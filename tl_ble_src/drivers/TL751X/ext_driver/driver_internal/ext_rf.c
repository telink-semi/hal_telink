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

_attribute_ram_code_
void ble_rf_set_tx_dma(unsigned char fifo_dep,unsigned char size_div_16)
{
    rf_set_tx_dma(fifo_dep, size_div_16*16);
}

_attribute_ram_code_
void ble_rf_set_rx_dma(unsigned char *buff, unsigned char size_div_16)
{
    ble_curr_rx_dma_buff = buff;
    rf_set_rx_dma(buff, 0, size_div_16*16);
}

void ble_rx_dma_config(void){
    rf_set_rx_dma_config();
}

#if BLC_PM_EN
_attribute_ram_code_
#endif
void rf_drv_ble_init(void)
{
    rf_mode_init();
    rf_set_ble_1M_mode();
}

#if RF_THREE_CHANNEL_CALIBRATION
_attribute_data_retention_ unsigned char rf_channel_power[40];
_attribute_data_retention_ unsigned char channel_power_calibration_enable = 0;

_attribute_ram_code_ //must be RamCode
void ble_rf_set_chn_power(signed char chn_num)
{

}

/**
 *  @brief      this function serve to set the TX power calibration.
 *  @param[in]  channel_power: channel power calibration of 40 channel.
 *  @return     none
*/
void rf_set_channel_power_calibration(unsigned char *channel_power)
{
    memcpy(rf_channel_power, channel_power, 40);
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
    rf_set_ble_chn(chn_num);
#if RF_THREE_CHANNEL_CALIBRATION
    if(channel_power_calibration_enable)
    {
        ble_rf_set_chn_power(chn_num);
    }
#endif
}

_attribute_ram_code_
void rf_start_fsm (fsm_mode_e mode, void* tx_addr, unsigned int tick)
{
    tx_addr = tx_addr == NULL ? reg_bb_dma_src_addr(RF_TX_DMA) :tx_addr;
    switch(mode)
    {
    case FSM_BTX:
        rf_start_btx(tx_addr, tick);
        break;
    case FSM_BRX:
        rf_start_brx(tx_addr, tick);
        break;
    case FSM_STX:
        rf_start_stx(tx_addr, tick);
        break;
    case FSM_SRX:
        rf_start_srx(tick);
        break;
    case FSM_TX2RX:
        rf_start_stx2rx(tx_addr, tick);
        break;
    case FSM_RX2TX:
        rf_start_srx2tx(tx_addr, tick);
        break;
    default:
        rf_start_btx(tx_addr, tick);
        break;
    }
}



// todo here need to optimize_Bool
_attribute_ram_code_
_Bool ll_resolvPrivateAddr(u8 *irk, u8 *addr, u8 irk_num)
{
    return true;
}



// todo here need to optimize
u8 ll_getRpaAddr(u8 *irk, u8 prand[3], u8 rpa[6])
{
    return 1;
}






_attribute_ram_code_ void rf_ble_set_tx_rx_settle_time(unsigned char tx_rx_stl_us)
{
    unsigned short tx_rx_stl_us_add=0;
    unsigned char settle_1=0x82;
    unsigned char settle_2=0x8f;
#if 1 //optimize, to save running time
    write_reg8(0xd4170204, tx_rx_stl_us-1);
    write_reg8(0xd417020c, tx_rx_stl_us-1);
#else
    tx_rx_stl_us &= 0x0fff;
    write_reg16(0xd4170204, (read_reg16(0xd4170204)& 0xf000) |(tx_rx_stl_us-1));
    write_reg16(0xd417020c, (read_reg16(0xd417020c)& 0xf000) |(tx_rx_stl_us-1));
#endif

    if(tx_rx_stl_us>53)
    {
#if 0 //optimize, to save running time
        if(tx_rx_stl_us>178)
        {
            tx_rx_stl_us_add = tx_rx_stl_us-53;
            settle_1 +=125;
            settle_2 +=  (tx_rx_stl_us_add-125);
        }
        else
#endif
        {
            tx_rx_stl_us_add = tx_rx_stl_us-53;//tx settle time- 53
            settle_1 += tx_rx_stl_us_add;
        }
    }
    else if(tx_rx_stl_us<53)
    {
        tx_rx_stl_us_add = 53 - tx_rx_stl_us;
        settle_2 -= tx_rx_stl_us_add;
    }

    write_reg8(CSEMDIGADDR+ 0x9C0+0x5e,settle_1);//default value:0x82(bit7 non-data bit),target value:0x02+tx_stl_us_add
    write_reg8(CSEMDIGADDR+ 0x9C0+0x5a,settle_2);
}




_attribute_data_retention_  unsigned char ble_rf_tx_settle_value = 0;


//adjust_tx_in_fsm: FSM is working, adjust TX settle value by tx wait, can not set TX register, MCU will die
_attribute_ram_code_ void rf_ble_csem_set_tx_rx_settle(int adjust_tx_in_fsm, unsigned char tx_stl_us, unsigned char rx_stl_us)
{
#if 1

    if(adjust_tx_in_fsm)  //FSM is working, can not change TX & RX settle value, change tx wait only
    {
        reg_rf_ll_txwait_l = (tx_stl_us + RF_TX_WAIT_MIN_VALUE - ble_rf_tx_settle_value);
        //reg_rf_ll_txwait_h = 0;
    }
    else
    {
        if(tx_stl_us)
        {
            if(tx_stl_us > TX_SETTLE_MIN_ALIGN_1MPHY) //only 1 situation:  tx_stl_auto_mode[PHY]
            {
                ble_rf_tx_settle_value = TX_SETTLE_REG_MIN; //43
                rf_ble_set_tx_rx_settle_time(TX_SETTLE_REG_MIN);
                reg_rf_ll_txwait_l = RF_TX_WAIT_MIN_VALUE + (tx_stl_us  - TX_SETTLE_REG_MIN);
                //reg_rf_ll_txwait_h = 0;
            }
            else{
                ble_rf_tx_settle_value = tx_stl_us;
                rf_ble_set_tx_rx_settle_time(tx_stl_us);

                reg_rf_ll_txwait_l = RF_TX_WAIT_MIN_VALUE;
                //reg_rf_ll_txwait_h = 0;
            }
        }


        if(rx_stl_us)
        {
            if(tx_stl_us){ //TX settle set already, use TX settle value
                if(rx_stl_us > TX_SETTLE_MIN_ALIGN_1MPHY){ //only 1 situation: RXSET_OPTM_ANTI_INTRF
                    //e.g. TX settle value is 43, RX settle value is 100, rx wait should be 100 - 9 - 43 = 48
                    reg_rf_ll_rxwait_l = RF_RX_WAIT_MIN_VALUE + (rx_stl_us - ble_rf_tx_settle_value);
                    //reg_rf_ll_rxwait_h = 0;
                }
            }
            else{ //if RX settle value only, set it directly
                rf_ble_set_tx_rx_settle_time(rx_stl_us);

                reg_rf_ll_rxwait_l = RF_RX_WAIT_MIN_VALUE;
                //reg_rf_ll_rxwait_h = 0;
            }
        }
    }
#endif
}


