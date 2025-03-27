/********************************************************************************************************
 * @file    gpio_simulate_uart.c
 *
 * @brief   This is the source file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "tl_common.h"
#if (FEATURE_TEST_MODE == TEST_PAWR_SYNC)

#include "gpio_simulate_uart.h"

#define SIMULATE_FIFO_SIZE              50
#define SIMULATE_FIFO_NUM               32

MYFIFO_INIT_IRAM(simuate_uart_fifo, SIMULATE_FIFO_SIZE, SIMULATE_FIFO_NUM);


#ifndef SIMULATE_GPIO_PIN
#define SIMULATE_GPIO_PIN               GPIO_PD4
#endif

#ifndef GSUART_BAUDRATE
#define GSUART_BAUDRATE                 1000000
#endif


_attribute_ble_data_retention_ my_fifo_t   *pSimulate_uart_fifo = NULL;


_attribute_ble_data_retention_ static uint32 sTlkApiBitIntv;


_attribute_ram_code_sec_noinline_
void simulate_gpio_putchar(uint08 byte)
{
    uint32 r = irq_disable();

    uint08 bits[14] = {0};
    uint08 out_level = reg_gpio_out(SIMULATE_GPIO_PIN);
    uint08 bit0 = out_level & ~(SIMULATE_GPIO_PIN);
    uint08 bit1 = out_level | SIMULATE_GPIO_PIN;


    bits[4] = bit0;
    for(int i=0; i<8; i++){
        if(byte & BIT(i)){
            bits[i+5] = bit1;
        }
        else{
            bits[i+5] = bit0;
        }
    }
    bits[13] = bit1;
    if(sys_clk.cclk <=32 && GSUART_BAUDRATE>115200)
    {
        bits[0] = bit1;
        bits[1] = bit1;
        bits[2] = bit1;
        bits[3] = bit1;
        unsigned char i,j = 0;
        /*
         * bit_nop is num of clock to transmit a bit use PRINT_BAUD_RATE.
         * 1 / sys_clk.cclk * (8 + 4 * bit_nop) = 1000000 / PRINT_BAUD_RATE.
         * 8 is clock of nop when bit_nop is 0.
         * 4 is clock of for(i) when bit_nop is not 0.
         */
        unsigned char bit_nop = sys_clk.cclk * 250000/ GSUART_BAUDRATE - 2;
        for(j = 0;j<14;j++)
        {
            for(i = 0;i<bit_nop;i++)//for:4 nop
            {
                __asm__("nop");
            }
            __asm__("nop");
            reg_gpio_out(SIMULATE_GPIO_PIN) = bits[j];
        }
    }
    else
    {
        uint32 time1 = clock_time();
        uint32 time2;
        for(int i=4; i<14; i++){
            time2 = time1;
            while(time1 - time2 < sTlkApiBitIntv){
                time1 = clock_time();
            }
            reg_gpio_out(SIMULATE_GPIO_PIN) = bits[i];
        }
    }
    irq_restore(r);
}


void app_gpio_simulate_uart_init(void)
{

    gpio_set_gpio_en(SIMULATE_GPIO_PIN);
    gpio_set_up_down_res(SIMULATE_GPIO_PIN, GPIO_PIN_PULLUP_1M);
    gpio_set_output_en(SIMULATE_GPIO_PIN, 1);
    gpio_write(SIMULATE_GPIO_PIN, 1);


    if (!pSimulate_uart_fifo){
        pSimulate_uart_fifo = &simuate_uart_fifo;
    }
    pSimulate_uart_fifo->wptr = pSimulate_uart_fifo->rptr = 0;

    sTlkApiBitIntv = SYSTEM_TIMER_TICK_1S/GSUART_BAUDRATE;
}


void app_gpio_simulate_uart_proc(void)
{
    uint08 *pData;
    if (pSimulate_uart_fifo->wptr != pSimulate_uart_fifo->rptr){
        pData =  pSimulate_uart_fifo->p + (pSimulate_uart_fifo->rptr++ & (pSimulate_uart_fifo->num - 1)) * pSimulate_uart_fifo->size;
    }
    else{
        return;
    }

    u8 dataLen = pData[0];
    for(int i=0; i<dataLen; i++){
        simulate_gpio_putchar(pData[i+1]);
    }
}

_attribute_ram_code_sec_noinline_
void simulateUart_send_str_data (u8 *pData, u8 data_len)
{
    if(!pSimulate_uart_fifo){
        return;
    }

    if(data_len > 50){
        return;
    }


    u32 r = irq_disable();

    u8 *pd =  pSimulate_uart_fifo->p + (pSimulate_uart_fifo->wptr & (pSimulate_uart_fifo->num - 1)) * pSimulate_uart_fifo->size;

    *pd++ = data_len;
    while (data_len--)
    {
        *pd++ = *pData++;
    }

    pSimulate_uart_fifo->wptr++;

    irq_restore(r);
}






#endif
