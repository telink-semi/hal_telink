/********************************************************************************************************
 * @file    ext_lib.c
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
#include <string.h>
#include "../../compatibility_pack/cmpt.h"
#include "../../gpio.h"
#include "../../lib/include/pm/pm.h"
#include "lib/include/rf/rf_common.h"
#include "ext_lib.h"
#include "../ext_misc.h"


 /******************************* efuse start *****************************************************************/
void efuse_read(unsigned char addr, unsigned char* buff, unsigned char len)
{
    (void)addr;
    (void)buff;
    (void)len;
    //todo
}
 /**
  * @brief     This function servers to get MAC address from EFUSE(byte [119:112]).
  * @return    The protection code value.
  */
 bool efuse_get_mac_address(u8* mac_read, int length)
 {
#if 1
    (void)mac_read;
    (void)length;
     return FALSE;  //todo
#else
    unsigned char mac[8] = {0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF, 0XFF};
    efuse_read(112, mac, 8);

    u8 zero_8_byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    if(memcmp(mac, zero_8_byte, 8)){
        if(length > 8){
            length = 8;
        }
        memcpy(mac_read, (u8*)mac, length);

        return TRUE;
    }
    else{
        return FALSE;
    }
#endif
 }
 /******************************* efuse end *******************************************************************/


 /******************************* debug_start ***************************************************************/
/**
 * @brief      This function writes a byte data to analog register
 * @param[in]  addr - the address of the analog register needs to write
 * @param[in]  value  - the data will be written to the analog register
 * @param[in]  e - the end address of value
 * @param[in]  s - the start address of the value
 * @return     none
 */
void sub_wr_ana(unsigned int addr, unsigned char value, unsigned char e, unsigned char s)
{
    unsigned char v, mask, tmp1, target, tmp2;

    v = analog_read(addr);
    mask = BIT_MASK_LEN(e - s + 1);
    tmp1 = value & mask;

    tmp2 = v & (~BIT_RNG(s,e));

    target = (tmp1 << s) | tmp2;
    analog_write(addr, target);
}

/**
 * @brief      This function writes a byte data to a specified analog register
 * @param[in]  addr - the address of the analog register needs to write
 * @param[in]  value  - the data will be written to the analog register
 * @param[in]  e - the end address of value
 * @param[in]  s - the start address of the value
 * @return     none
 */
void sub_wr(unsigned int addr, unsigned char value, unsigned char e, unsigned char s)
{
    unsigned char v, mask, tmp1, target, tmp2;

    v = read_reg8(addr);
    mask = BIT_MASK_LEN(e - s + 1);
    tmp1 = value & mask;

    tmp2 = v & (~BIT_RNG(s,e));

    target = (tmp1 << s) | tmp2;
    write_reg8(addr, target);
}

 /******************************* debug_end ****************************************************************/


/******************************* dbgport start ******************************************************************/
void dbg_bb_set_pin(gpio_pin_e pin)
{
    u8 mask = 0x3F;//bit5~bit0
    u8 val = 0x39;//function select DBG_BB

    //note:  setting pad the function  must before  setting no_gpio function, cause it will lead to uart transmit extra one byte data at begin.(confirmed by minghai&sunpeng)
    reg_gpio_func_mux(pin) = (reg_gpio_func_mux(pin) & mask) | val;

    gpio_function_dis(pin);
}

void ble_dbg_port_init(int deg_sel0)
{
    /* 1. r_dbg_sel0: 0x80170003[5],  debug port switch
     * 2. r_dbg_sel1: 0x80170003[7],  debug port switch
     *
     *          r_dbg_sel0 = 1          r_dbg_sel0 = 0          r_dbg_sel1 = 1
     *
     *  PA0:    tx_en                   tx_en
     *  PA1:    tx_on                   tx_on
     *  PA2:    rx_en                   rx_en
     *  PA3:    clk_bb                  clk_bb
     *  PA4:    hit_sync                hit_sync
     *  PB0:    sclk                    sclk
     *  PB1:    tx_data                 tx_data
     *  PB2:    rx_data_vld             rx_data_vld             rx_symb_vld
     *  PB3:    rx_data                 rx_data                 rx_symb0(for zigbee)
     *  PB4:                                                    rx_symb1(for zigbee)
     *  PB5:                                                    rx_symb2(for zigbee)
     *  PB6:                                                    rx_symb3(for zigbee)
     *  PB7:    ll_ss[0]                ll_ss[0]
     *  PC0:    ll_ss[1]                ll_ss[1]
     *  PC1:    ll_ss[2]                ll_ss[2]
     *  PC2:    rx_ss[0]                rx_ss[0]
     *  PC3:    rx_ss[1]                rx_ss[1]
     *  PC4:    rx_ss[2]                rx_ss[2]
     *  PC5:    dma_ack_tx              dma_ack_rx
     *  PC6:    reg_wr                  reg_rd
     *  PC7:    dma_eof                 dma_err
     *  PD0:    dma_sof                 dma_cyc
     *  PD1:    dma_rdy_tx              dma_rdy_rx
     *  PD2:    reg_cs                  reg_cs
     */

    /* ll_ss    state
        0       IDLE
        1       ACTIVE
        2       TXSTL
        3       TX
        4       RXWAIT
        5       RX
        6       TXWAIT

       rx_ss    state
        0       IDLE
        1       SYNC
        2       DEC
        3       HD
        4       FOOT
    */

    /*
     * sub_wr(0x80170003, 1, 5, 5)  //r_dbg_sel0 = 1                0x80140803[5]:  dbg_sel0
     */
    if(deg_sel0){
        REG_ADDR8(0x170003) |= BIT(5);//default 0x0
    }

    /*
     *  sub_wr(0x80140379, 0, 7, 4) //dbg_sel_bt1-4 = 0             0x80140379[7:4] = 0        0000xxxx
        sub_wr(0x80140378, 1, 1, 1) //dbg_sel_bb_l = 1              0x80140378[1] = 1          xxxxxx1x      enable dbg_sel_bb_l
        sub_wr(0x80140378, 1, 2, 2) //dbg_sel_bb_h = 1              0x80140378[2] = 1          xxxxx1xx      enable dbg_sel_bb_h
        sub_wr(0x80140378, 0 ,5, 5) //dbg_axon_bb_sel = 0           0x80140378[5] = 0          xx0xx11x      disable dbg_axon_bb_sel
     */
    reg_bb_dbg_sel_h &= 0x0F;       // 140379[7:4] = 0
    reg_bb_dbg_sel_h |= BIT(0);     // 140379[0] = 1; if not set, B2/B3 not work.
    reg_bb_dbg_sel_l |= BIT(1);     // 140378[1] = 1
    reg_bb_dbg_sel_l |= BIT(2);     // 140378[2] = 1
    reg_bb_dbg_sel_l &= ~BIT(5);    // 140378[5] = 0
}

/**
 * @brief       This function is used to enable BaseBand debug function.
 * @param[in]   none.
 * @return      none.
 */
void rf_enable_bb_debug(void)
{
    ble_dbg_port_init(0);//dma_rx
//  ble_dbg_port_init(1);//dma_tx

#if (1)
    dbg_bb_set_pin(GPIO_PA0); //tx_en
    dbg_bb_set_pin(GPIO_PA1); //tx_on
    dbg_bb_set_pin(GPIO_PA2); //rx_en
#endif

#if (0)
    dbg_bb_set_pin(GPIO_PA3); //clk_bb
    dbg_bb_set_pin(GPIO_PA4); //hit_sync
    dbg_bb_set_pin(GPIO_PB0); //sclk
    dbg_bb_set_pin(GPIO_PB1); //tx_data
    dbg_bb_set_pin(GPIO_PB2); //rx_data_vld
    dbg_bb_set_pin(GPIO_PB3); //rx_data
#elif (0)//tx
    dbg_bb_set_pin(GPIO_PA3); //clk_bb
    dbg_bb_set_pin(GPIO_PB1); //tx_data
#elif (0)//rx
    dbg_bb_set_pin(GPIO_PA3); //clk_bb
    dbg_bb_set_pin(GPIO_PB2); //rx_data_vld
    dbg_bb_set_pin(GPIO_PB3); //rx_data
#endif

    /* ll_ss    state
        0       IDLE
        1       ACTIVE
        2       TXSTL
        3       TX
        4       RXWAIT
        5       RX
        6       TXWAIT
    */
#if (1)
    dbg_bb_set_pin(GPIO_PB7); //ll_ss[0]
    dbg_bb_set_pin(GPIO_PC0); //ll_ss[1]
    dbg_bb_set_pin(GPIO_PC1); //ll_ss[2]
#endif

    /* rx_ss    state
        0       IDLE
        1       SYNC
        2       DEC
        3       HD
        4       FOOT
    */
#if (1)
    dbg_bb_set_pin(GPIO_PC2); //rx_ss[0]
    dbg_bb_set_pin(GPIO_PC3); //rx_ss[1]
    dbg_bb_set_pin(GPIO_PC4); //rx_ss[2]
#endif

#if (0)
    dbg_bb_set_pin(GPIO_PC5); //dma_ack_rx(dbg_sel0=0)
                              //dma_ack_tx(dbg_sel0=1)

    dbg_bb_set_pin(GPIO_PC6); //reg_rd(dbg_sel0=0)
                              //reg_wr(dbg_sel0=1)

    dbg_bb_set_pin(GPIO_PC7); //dma_err(dbg_sel0=0)
                              //dma_eof(dbg_sel0=1)

    dbg_bb_set_pin(GPIO_PD0); //dma_cyc(dbg_sel0=0)
                              //dma_sof(dbg_sel0=1)

    dbg_bb_set_pin(GPIO_PD1); //dma_rdy_rx(dbg_sel0=0)
                              //dma_rdy_tx(dbg_sel0=1)

    dbg_bb_set_pin(GPIO_PD2); //reg_cs
#endif
}
/******************************* dbgport end ********************************************************************/


/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] gpio - the pin needs to set its pull-up/down resistor
 * @param[in] up_down - the type of the pull-up/down resistor
 * @return    none
 */
void gpio_setup_up_down_resistor(gpio_pin_e gpio, gpio_pull_type up_down)
{
    ///////////////////////////////////////////////////////////
    //        PA[3:0]           PA[7:4]         PB[3:0]         PB[7:4]     PC[3:0]         PC[7:4]
    // sel: ana_0x17<7:0>    ana_0x18<7:0>  ana_0x19<7:0>  ana_0x1a<7:0>  ana_0x1b<7:0>  ana_0x1c<7:0>
    //        PD[3:0]           PD[7:4]         PE[3:0]         PE[7:4]     PF[3:0]         PF[7:4]
    // sel: ana_0x1d<7:0>    ana_0x1e<7:0>  ana_0x1f<7:0>  ana_0x20<7:0>  ana_0x21<7:0>  ana_0x22<7:0>
    unsigned char r_val = up_down & 0x03;

    unsigned char base_ana_reg = 0;
    if((gpio>>8)<6)//A-E
    {
         base_ana_reg = 0x17 + ((gpio >> 8) << 1) + ((gpio & 0xf0) ? 1 : 0 );
    }
    else{
        return;
    }
    unsigned char shift_num, mask_not;

    if(gpio & 0x11){
        shift_num = 0;
        mask_not = 0xfc;
    }
    else if(gpio & 0x22){
        shift_num = 2;
        mask_not = 0xf3;
    }
    else if(gpio & 0x44){
        shift_num = 4;
        mask_not = 0xcf;
    }
    else if(gpio & 0x88){
        shift_num = 6;
        mask_not = 0x3f;
    }
    else{
        return;
    }
    analog_write_reg8(base_ana_reg, (analog_read_reg8(base_ana_reg) & mask_not) | (r_val << shift_num));
}



/******************************* trng_start ******************************************************************/
/**
 * @brief     This function performs to generate a series of random numbers
 * @param[in]  len - data length
 * @param[out] data - data pointer
 * @return    none
 **/
_attribute_no_inline_
void generateRandomNum(int len, unsigned char *data)
{
    int i;
    unsigned int randNums = 0;
    /* if len is odd */
    for (i=0; i<len; i++ ) {
        if( (i & 3) == 0 ){
            randNums = rand();
        }

        data[i] = randNums & 0xff;
        randNums >>=8;
    }
}
/******************************* trng_end ********************************************************************/


