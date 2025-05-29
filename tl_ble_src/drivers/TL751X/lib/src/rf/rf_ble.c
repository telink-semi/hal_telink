/********************************************************************************************************
 * @file    rf_ble.c
 *
 * @brief   This is the source file for TL751X
 *
 * @author  Driver Group
 * @date    2023
 *
 * @par     Copyright (c) 2023, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/rf/rf_ble.h"
#include "compiler.h"


/*********************************************************************************************************************
 *                                         global function implementation                                            *
 *********************************************************************************************************************/

/**
 * @brief     This function serves to  set ble_1M  mode of RF.
 * @return    none.
 */
void rf_set_ble_1M_mode(void)
{
    //ble1m_set_up
    reg_rf_tx_mode1 = 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail&0xe0)|0x02;//rf preamble len
    reg_rf_format = (reg_rf_format&0xe0)|0x15;
    reg_rf_acc_len = (reg_rf_acc_len&0x87)|FLD_RF_RX_BYTE_ORDER;//lr off,rx_crc4_order,tx_byte_order,rx byte order
    reg_rf_tx_mode2 |=FLD_RF_V_PN_EN;

    //ble_rate
    reg_rf_trx_rate &= 0xfc;

    g_rfmode = RF_MODE_BLE_1M;
}

/**
 * @brief     This function serves to  set ble_1M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_1M_NO_PN_mode(void)
{
    //ble1m_set_up
    reg_rf_tx_mode1 = 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail&0xe0)|0x02;//rf preamble len
    reg_rf_format = (reg_rf_format&0xe0)|0x15;
    reg_rf_acc_len = (reg_rf_acc_len&0x87)|FLD_RF_RX_BYTE_ORDER;//lr off,rx_crc4_order,tx_byte_order,rx byte order

    //ble_rate
    reg_rf_trx_rate &= 0xfc;

    g_rfmode = RF_MODE_BLE_1M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_2M  mode of RF.
 * @return    none.
 */
void rf_set_ble_2M_mode(void)
{
    reg_rf_tx_mode1 = 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail&0xe0)|0x03;//rf preamble len
    reg_rf_format = (reg_rf_format&0xe0)|0x05;
    reg_rf_acc_len = (reg_rf_acc_len&0x87)|FLD_RF_RX_BYTE_ORDER;//lr off,rx_crc4_order,tx_byte_order,rx byte order
    reg_rf_tx_mode2 |=FLD_RF_V_PN_EN;

    //ble_rate
    reg_rf_trx_rate = (reg_rf_trx_rate &0xfc)|0x01;

    g_rfmode = RF_MODE_BLE_2M;
}

/**
 * @brief     This function serves to  set ble_2M_NO_PN  mode of RF.
 * @return    none.
 */
void rf_set_ble_2M_NO_PN_mode(void)
{
    //ble1m_set_up
    reg_rf_tx_mode1 = 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail&0xe0)|0x03;//rf preamble len
    reg_rf_format = (reg_rf_format&0xe0)|0x05;
    reg_rf_acc_len = (reg_rf_acc_len&0x87)|FLD_RF_RX_BYTE_ORDER;//lr off,rx_crc4_order,tx_byte_order,rx byte order

    //ble_rate
    reg_rf_trx_rate = (reg_rf_trx_rate &0xfc)|0x01;


    g_rfmode = RF_MODE_BLE_2M_NO_PN;
}

/**
 * @brief     This function serves to  set ble_500K  mode of RF.
 * @return    none.
 */
void rf_set_ble_500K_mode(void)
{
    //ble_500K_set_up
    reg_rf_tx_mode1 = 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail&0xe0)|0x0a;//rf preamble len
    reg_rf_format = (reg_rf_format&0xe0)|0x15;
    reg_rf_acc_len = (reg_rf_acc_len&0x87)|0xa0;

    //ble_rate
    reg_rf_trx_rate = (reg_rf_trx_rate &0xfc)|0x03;


    g_rfmode = RF_MODE_LR_S2_500K;
}

/**
 * @brief     This function serves to  set zigbee_125K  mode of RF.
 * @return    none.
 */
void rf_set_ble_125K_mode(void)
{
    //ble_125K_set_up
    reg_rf_tx_mode1 = 0x1f;
    reg_rf_preamble_trail = (reg_rf_preamble_trail&0xe0)|0x0a;//rf preamble len
    reg_rf_format = (reg_rf_format&0xe0)|0x15;
    reg_rf_acc_len = (reg_rf_acc_len&0x87)|0xa0;

    //ble_rate
    reg_rf_trx_rate = (reg_rf_trx_rate &0xfc)|0x02;

    g_rfmode = RF_MODE_LR_S8_125K;
}

/**
 * @brief       This function serves to start Rx of auto mode. In this mode,
 *              RF module stays in Rx status until a packet is received or it fails to receive packet when timeout expires.
 *              Timeout duration is set by the parameter "tick".
 *              The address to store received data is set by the function "addr".
 * @param[in]   addr   - The address to store received data.
 * @param[in]   tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
 * @return      none
 */
void rf_start_brx  (void* addr, unsigned int tick)
{
    write_reg32 (0xd4170228, 0x0fffffff);
    write_reg32(0xd4170218, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    write_reg8 (0xd4170200, 0x82);// ble rx.
}


/**
 * @brief       This function serves to start tx of auto mode. In this mode,
 *              RF module stays in tx status until a packet is sent or it fails to sent packet when timeout expires.
 *              Timeout duration is set by the parameter "tick".
 *              The address to store send data is set by the function "addr".
 * @param[in]   addr   - The address to store send data.
 * @param[in]   tick   - It indicates timeout duration in Rx status.Max value: 0xffffff (16777215).
 * @return      none.
 */
void rf_start_btx (void* addr, unsigned int tick)
{
    write_reg32(0xd4170218, tick);
    reg_rf_ll_ctrl3 |= FLD_RF_R_CMD_SCHEDULE_EN;    // Enable cmd_schedule mode.
    rf_dma_set_src_address(RF_TX_DMA,(unsigned int)(addr));
    write_reg8 (0xd4170200, 0x81);                      // ble tx.
}

/**
 * @brief       This function serves to set RF baseband channel.This function is suitable for ble open PN mode.
 * @param[in]   chn_num  - Bluetooth channel set according to Bluetooth protocol standard.
 * @return      none.
 */
void rf_set_ble_chn (signed char chn_num)
{
//  signed char ble_chn_num = 0;
    write_reg8 (0xd417000d, chn_num);
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
        chn_num = chn_num<<1;
        rf_set_chn(chn_num);

}
