/********************************************************************************************************
 * @file    swire.c
 *
 * @brief   This is the source file for B91
 *
 * @author  Driver Group
 * @date    2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd.
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
#include "lib/include/swire.h"
#include "stimer.h"
#include "gpio.h"
#include "clock.h"
/*
 * the default value is 300000 stmer ticks, which is about 18.75ms.
 */
static unsigned int s_read_timeout_tick=300000;
/**
 * @brief     This function is to send a specific timing to reset the status of the slave device.
 *            When the master device connects the DP pin of the slave device, this function must be called first,
 *            because the DP pin of the slave device is USB function by default. Only after calling this function to send a specific timing,
 *            the slave device will be enabled as the swire function.
 *            When communication fails, you can also try to call this function to resume communication.
 * @param[in] gpio_swm - the swm pin.
 * @param[in] dp_through_swire_en - If the master is connected to the DP pin of the slave device, this parameter needs to be set to 1.
 * @return    none.
 */
void swire_sync (swm_pin_e gpio_swm,unsigned char dp_through_swire_en)
{
    swire_reset();
    //gpio output
    gpio_input_dis(gpio_swm);
    gpio_output_en(gpio_swm);
    gpio_function_en(gpio_swm);

    if (dp_through_swire_en) {

        gpio_set_low_level(gpio_swm);
        delay_ms (40);      //wait 40ms
        gpio_set_high_level(gpio_swm);
        delay_ms (80);      //wait 80ms
        gpio_set_low_level(gpio_swm);
        delay_ms (40);      //wait 40ms
        gpio_set_high_level(gpio_swm);
        delay_ms (20);      //wait 20ms
    }
    gpio_set_high_level(gpio_swm);
    delay_us (10);
    int i;
    for (i=0; i<12; i++) {
        gpio_set_low_level(gpio_swm);
        delay_us (4);
        gpio_set_high_level(gpio_swm);
        delay_us (1);
    }
    swire_set_swm_en(gpio_swm);
}

/**
 * @brief     This function is to set gpio as the swm function.
 * @param[in] gpio_swm - the swm pin needs to enable.
 * @return    none.
 */
void swire_set_swm_en(swm_pin_e gpio_swm)
{
    reg_gpio_func_mux(gpio_swm)= reg_gpio_func_mux(gpio_swm) & 0xfc;
    gpio_input_en(gpio_swm);
    gpio_function_dis(gpio_swm);


}

/**
 * @brief     This function is to set GPIO_PA7 as the sws function.
 * @return    none.
 */

void swire_set_sws_en(void)
{
    reg_gpio_func_mux(GPIO_SWS)= reg_gpio_func_mux(GPIO_SWS) & 0x3f;
    gpio_input_en(GPIO_SWS);
    gpio_function_dis(GPIO_SWS);

}

/**
 * @brief     This function is to set the clock of the swire module,
 *            the clock source of the swire module is HCLK, the clock of the actual swire module:  swire_clock = HCLK/div.
 *            swire_clock cannot be greater than HCLK.(div cannot be 0)
 *            Whether the chip as a master or slave, clock source are the following principles:
 *            the clock for sending data is swire_clock, and the clock for detecting received data is HCLK.
 * @param     swire_clock - swire clock
 * @return    none.
 */
void swire_set_clk(unsigned int swire_clock)
{
    reg_swire_ctl2 = ((sys_clk.hclk*1000*1000/swire_clock)& 0x7f);
}

/**
 * @brief     This function is used to set the ID of the slave.
 *            When the host accesses multiple slaves, the device can be distinguished by the ID of the slave.
 * @param[in] id    - slave id.The setting range is 0~0x1f.
 * @return    none.
 */
void swire_set_slave_id(unsigned char id)
{
    reg_swire_id = (reg_swire_id &(~FLD_SWIRE_ID_VALID))|(id & FLD_SWIRE_ID_VALID);

}
/**
 * @brief     This function is used by the master device to write data to the slave device.
 *            The swire protocol is to send the address first and then send the data.
 * @param[in] slave_id  - slave id.The setting range is 0~0x1f.
 *                        [6:5]: this function tells the slave to operate in byte/half-word/word.
 *                               the reason is that some addresses on the slave side can only be operated with 4 bytes.
 *                               10--32bit; 01--16bit ; 00--8bit
 *                        [4:0]: ID
 * @param[in] addr      - slave address(little endian format).
 * @param[in] addr_len  - slave address byte length,It is determined by the slave device, for example: B91s is four bytes, B85s is three bytes.
 * @param[in] data      - The data that needs to be written to the slave.
 * @param[in] data_len  - data length.
 * @return    none.
 */

void swire_master_write(unsigned char slave_id,unsigned char *addr, unsigned char addr_len,unsigned char *data,unsigned int data_len)
{
    unsigned int i;
    swire_reset();
    swire_master_write_cmd(0xff);
    swire_master_write_cmd(0x5a);
    for(i=0;i<addr_len;i++){
        swire_master_write_data(addr[addr_len-1-i]);
    }
    swire_master_write_data(slave_id & 0x1f);//bit7:0 for write;bit4~bit0 :slave id
    for(i=0;i<data_len;i++){
        swire_master_write_data(data[i]);//0 for write
    }
    swire_master_write_cmd(0xff);
}
/**
 * @brief      This function is used by the master device to read data to the slave device.
 *             The swire protocol is to send the address first and then wait to read the data returned by the slave.
 * @param[in] slave_id   - slave id.The setting range is 0~0x1f.
 *                           [6:5]: this function tells the slave to operate in byte/half-word/word.
 *                                  the reason is that some addresses on the slave side can only be operated with 4 bytes.
 *                                  10--32bit; 01--16bit ; 00--8bit
 *                           [4:0]: ID
 * @param[in]  addr      - slave address(little endian format).
 * @param[in]  addr_len  - slave address byte length,It is determined by the slave device, for example: B91s is four bytes, B85s is three bytes.
 * @param[out] data      - this is the data read from the slave.
 * @param[in]  data_len  - data length.
 * @return     0:read timeout  1:read success.
 */

unsigned char  swire_master_read (unsigned char slave_id,unsigned char *addr, unsigned char addr_len,unsigned char *data,unsigned int data_len)
{
    unsigned int i;
    swire_reset();
    swire_master_write_cmd(0xff);
    swire_master_write_cmd(0x5a);
    for(i=0;i<addr_len;i++){
        swire_master_write_data(addr[addr_len-1-i]);
    }
    swire_master_write_data(0x80 | (slave_id & 0x1f));//bit7: 1 for read;bit4~bit0 :slave id
    for(i=0;i<data_len;i++){
        unsigned char err_flag=1;
        reg_swire_ctl =  FLD_SWIRE_RD ;
        unsigned long t = stimer_get_tick ();
        while (stimer_get_tick () - t < s_read_timeout_tick){
/*
 *FLD_SWIRE_RD clearing mechanism: the clk of the slave responds to the master read operation. FLD_SWIRE_RD does not clear 0 until the slave sends the read operation.
 */
            if (!(reg_swire_ctl & FLD_SWIRE_RD)) {
                err_flag=0;
                data[i] = reg_swire_data;
                break;
            }
        }
/*
 * when the slave does not respond, FLD_SWIRE_RD is not cleared. In this case, if the FLD_SWIRE_WR write operation is configured and FLD_SWIRE_RD is not cleared,
 * the state machine in the swire module is abnormal and freezes. can restore the state machine by running swire_reset() or writing 0 to both FLD_SWIRE_WR and FLD_SWIRE_RD(changed by shuaixing,confirmed by jianzhi.20220805).
 */
        if(err_flag){
            swire_reset();
            swire_sync(GPIO_SWM,0);
            return 0;
        }
    }
    swire_master_write_cmd(0xff);
    return 1;
}

/*
 * @brief      This function is used to set swire read timeout tick.
 *             the swire read timing protocol is as follows: when it is a read operation, the master sends 1 unit of low level to the slave and then releases the bus.
 *             when the slave finds that the high level lasts longer than the previous low level time, it starts sending data. After sending 8 bits,
 *             it sends another unit of low level. The read operation time is the master sends one Bit and the slave sends nine bits.
 *             a bit allowance is added to the master and slave respectively. The final calculation formula is as follows:
 *             s_read_timeout_tick =   1000000*(1/master_clk*5*2 + 1/slave_clk*5*10)/(1/stimer_clk)
 *                                 =   stimer_clk*1000000*(10/master_clk+50/slave_clk)
 *             then two additional allowances have been added to prevent division problems:
 *             s_read_timeout_tick=s_stimer_tick+2;
 * @param[in]  master_clk_hz  - swire master clock,unit is HZ.
 * @param[in]  slave_clk_hz   - swire slave  clock,unit is HZ.
 * @return     none.
 */
void swire_read_set_timeout_tick(unsigned int master_clk_hz,unsigned int slave_clk_hz){
    s_read_timeout_tick = (SYSTEM_TIMER_TICK_1S*10)/master_clk_hz+(SYSTEM_TIMER_TICK_1S*50)/slave_clk_hz +2;

}

/*
 * @brief      This function is used to get swire read timeout tick.
 * @return     timeout tick.
*/
unsigned int swire_read_get_timeout_tick(){
    return s_read_timeout_tick;
}
