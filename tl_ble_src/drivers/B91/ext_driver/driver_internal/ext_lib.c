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

#include "../../lib/include/pm.h"
#include "../../lib/include/rf.h"
#include "ext_lib.h"



 /******************************* ADC_start ********************************************************************/


 _attribute_data_retention_sec_
 adc_vref_ctr_t adc_vref_cfg = {
    .adc_vref        = 1175, //default ADC ref voltage (unit:mV)
    .adc_vref_offset = 0,    //ADC calibration value voltage offset (unit:mV).
    .adc_calib_en    = 1,    //default enable
 };

 /**
  * @brief       This function enable adc reference voltage calibration
  * @param[in] en - 1 enable  0 disable
  * @return     none.
  */
 void   adc_calib_vref_enable(unsigned char en)
 {
    adc_vref_cfg.adc_calib_en = en;
 }


 /******************************* ADC_end***************************************************************/





/******************************* dbgport start ******************************************************************/
void bt_dbg_set_pin(btdbg_pin_e pin)
{
    u8 val,v=0;
    u8 mask= 0;
    u8 n=0;
    if(pin&0xf)
    {
        v = pin&0xf;
        do{
            n++;
            v= v>>1;
        }while(v);

        mask = ((unsigned char)~(3<<((n-1)*2)));
        val = ((3<<((n-1)*2)));
    }
    else if(pin&0xf0)
    {
        v = ((pin>>4)&0xf);
        do{
            n++;
            v= v>>1;
        }while(v);

        mask = ((unsigned char)~(3<<((n-1)*2)));
        val = ((3<<((n-1)*2)));
    }
    else
    {
        while(1);
    }

    //note:  setting pad the function  must before  setting no_gpio function, cause it will lead to uart transmit extra one byte data at begin.(confirmed by minghai&sunpeng)
    reg_gpio_func_mux(pin)=(reg_gpio_func_mux(pin)& mask)|val;

    gpio_function_dis(pin);
}


void ble_dbg_port_init(int deg_sel0)
{

    /* 1. dbg_sel0: 0x80140803[5],  debug port switch
     * 2. dbg_sel1: 0x80140803[7],  debug port switch
     *
     *         dbg_sel0 = 1            dbg_sel0 = 0                dbg_sel1 = 1
     *
     *  PA0:    tx_en                   tx_en
     *  PA1:    tx_on                   tx_on
     *  PA2:    rx_en                   rx_en
     *  PA3:    clk_bb                  clk_bb
     *  PA4:    hit_sync                hit_sync
     *  PB0:    sclk                    sclk
     *  PB1:    tx_data                 tx_data
     *  PB2:    rx_data_vld             rx_data_vld
     *  PB3:    rx_data0                rx_data0                rx_symb0(for zigbee)
     *  PB4:                                                    rx_symb1(for zigbee)
     *  PB5:                                                    rx_symb2(for zigbee)
     *  PB6:                                                    rx_symb3(for zigbee)
     *  PB7:    ll_ss[0]                ll_ss[0]
     *  PC0:    ll_ss[1]                ll_ss[1]
     *  PC1:    ll_ss[2]                ll_ss[2]
     *  PC2:    ss[0]                   ss[0]
     *  PC3:    ss[1]                   ss[1]
     *  PC4:    ss[2]                   ss[2]
     *  PC5:    dma_ack_tx              dma_ack_rx
     *  PC6:    reg_wr                  reg_rd
     *  PC7:    dma_eof                 dma_err
     *  PD0:    dma_sof                 dma_cyc
     *  PD1:    dma_rdy_tx              dma_rdy_rx
     */

    /* ll_ss    state
        0       IDLE
        1       ACTIVE
        2       TXSTL
        3       TX
        4       RXWAIT
        5       RX
        6       TXWAIT

       bb_ss    state
        0       IDLE
        1       SYNC
        2       DEC
        3       HD
        4       FOOT
    */

    /* 0x80140803[5]:  dbg_sel0 */
    if(deg_sel0 == 1){
        REG_ADDR8(0x140803) |= BIT(5);
    }




    /*
     *  sub_wr(GPIO_BASE+0x55, 0, 7, 4) //dbg_sel_bt1-4 = 0             140355[7:4] = 0          0000xxxx
        sub_wr(GPIO_BASE+0x54, 3, 2, 1) //dbg_sel_bb_h/l = 1            140354[2:1] = 3          xxxxx11x      enable dbg_sel_bb_h/bb_l
        sub_wr(GPIO_BASE+0x54, 0 ,5, 5) //dbg_axon_bb_sel = 0           140354[5] =  0           xx0xx11x      disable dbg_axon_bb_sel
     */

    reg_bt_dbg_sel_h &= 0x0F;               // 140355[7:4] = 0
    reg_bt_dbg_sel_h |= BIT(0);             // 140355[0] = 1; if not set, B2/B3 not work.
    reg_bt_dbg_sel_l |= (BIT(2) | BIT(1));  // 140354[2:1] = 3
    reg_bt_dbg_sel_l &= ~BIT(5);            // 140354[5] =  0
}

/**
 * @brief     This function set a pin's pull-up/down resistor.
 * @param[in] gpio - the pin needs to set its pull-up/down resistor
 * @param[in] up_down - the type of the pull-up/down resistor
 * @return    none
 */
void gpio_setup_up_down_resistor(gpio_pin_e gpio, gpio_pull_type up_down)
{
    unsigned char r_val = up_down & 0x03;

    unsigned char base_ana_reg = 0x0e + ((gpio >> 8) << 1) + ( (gpio & 0xf0) ? 1 : 0 );  //group = gpio>>8;
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

    if(GPIO_DP == gpio){
        //usb_dp_pullup_en (0);
    }

    analog_write_reg8(base_ana_reg, (analog_read_reg8(base_ana_reg) & mask_not) | (r_val << shift_num));
}

/******************************* dbgport end ********************************************************************/


/******************************* trng_start ******************************************************************/
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
