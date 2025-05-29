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
#include "../../lib/include/rf/rf_common.h"
#include "ext_lib.h"
#include "../ext_misc.h"


 /******************************* efuse start *****************************************************************/
 /**
  * @brief     This function servers to get MAC address from EFUSE(byte [119:112]).
  * @return    The protection code value.
  */
 bool efuse_get_mac_address(u8* mac_read, int length)
 {
    unsigned char mac[8];
    efuse_read(112, mac, 8);

    u8 zero_8_byte[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    if(memcmp(mac, zero_8_byte, 8)){
        if(length > 8){
            length = 8;
        }
        memcpy(mac_read, (u8*)mac, length);

        return true;
    }
    else{
        return false;
    }
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
    gpio_function_dis(pin);
    reg_gpio_func_mux(pin) &= 0xC0;
    reg_gpio_func_mux(pin) |= (BIT(0) | BIT(3) | BIT(4));

}

void ble_dbg_port_init(int deg_sel0)
{
}



/**
 * @brief       This function is used to enable BaseBand debug function.
 * @param[in]   none.
 * @return      none.
 */
void rf_enable_bb_debug(void)
{
    unsigned int GPIO_BASE = 0x140C00;
    REG_ADDR8(GPIO_BASE+0x110) |= (BIT(1) | BIT(2)); // dbg_sel_bb_h/dbg_sel_bb_l = 1
    // PA[0]: TX_EN
    dbg_bb_set_pin(GPIO_PA0);
    // PA[1]: TX_ON
    dbg_bb_set_pin(GPIO_PA1);
    // PA[2]: RX_EN
    dbg_bb_set_pin(GPIO_PA2);
    // PA[3]: bb clk
    dbg_bb_set_pin(GPIO_PA3);
    // PA[4]: RX_HIT_SYNC
    dbg_bb_set_pin(GPIO_PA4);
    // PA[6]: TX_DATA
    dbg_bb_set_pin(GPIO_PA6);

    // PB[0]: RX_vld
    dbg_bb_set_pin(GPIO_PB0);
    // PB[1]: RX_DATA
    dbg_bb_set_pin(GPIO_PB1);
//  // PB5[5]: linklayer_tx_ss [0]
//  dbg_bb_set_pin(GPIO_PB5);
//  // PB6[6]: linklayer_tx_ss [1]
//  dbg_bb_set_pin(GPIO_PB6);
//  // PB7[7]: linklayer_tx_ss [2]
//  dbg_bb_set_pin(GPIO_PB7);

    // PD2[2]: linklayer_rx_ss [0]
    dbg_bb_set_pin(GPIO_PD2);
    // PD3[3]: linklayer_rx_ss [1]
    dbg_bb_set_pin(GPIO_PD3);
    // PD4[4]: linklayer_rx_ss [2]
    dbg_bb_set_pin(GPIO_PD4);
    // PD7[7]: DMA err
    dbg_bb_set_pin(GPIO_PD7);

    // PJ4 crystal
    dbg_bb_set_pin(GPIO_PJ4);
//  write_reg8(0x14081a,(read_reg8(0x14081a)&0xe0)|0x04);
//  write_reg8(0x140d0c,(read_reg8(0x140d0c)&0xe0)|0x10);
//  REG_ADDR8(GPIO_BASE+0x96)  &= ~BIT(4);

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


