/********************************************************************************************************
 * @file    app_parse_char.c
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
#include "drivers.h"
#include <stdarg.h>
#include <strings.h>
#include "application/app/usbcdc.h"
#include "application/usbstd/usb.h"
#include "app_parse_char.h"

#if UI_CONTROL_ENABLE

typedef struct {
    u16 write_index;
    u16 read_index;
    u16 size;
    u8 *buffer;
} ring_buf_t;

u8 shellRecvCmdBuf[PARSE_CHAR_UART_BUFF_SIZE + 1];      //str + '\0'
u16 shellRecvCmdBufIdx = 0;
u8 uartRecvBuf[PARSE_CHAR_UART_BUFF_SIZE];
parse_fun_list_t* gParseList = NULL;
int gParseSize = 0;
static ring_buf_t appParseRingBuf;
static u8 ringBuf[PARSE_CHAR_UART_BUFF_SIZE * 2];

/**
 * @brief       ring buffer initial function.
 * @param[in]   ring_buf: ring buffer structure pointer.
 * @param[in]   size: ring buffer size.
 * @param[in]   buffer: ring buffer store data pointer.
 * @return      none.
 */
static void ring_buf_init(ring_buf_t *ring_buf, u16 size, u8 *buffer)
{
    ring_buf->size = size;
    ring_buf->buffer = buffer;
    ring_buf->write_index = 0;
    ring_buf->read_index = 0;
}

/**
 * @brief       free ring buffer space.
 * @param[in]   ring_buf: ring buffer structure pointer.
 * @return      none.
 */
static u16 ring_buf_free_space(ring_buf_t *ring_buf)
{
    if (ring_buf->read_index > ring_buf->write_index) {
        return ring_buf->read_index - ring_buf->write_index - 1;
    } else if (ring_buf->write_index > ring_buf->read_index) {
        return (ring_buf->read_index + ring_buf->size - ring_buf->write_index - 1);
    } else {
        // (ring_buf->write_index == ring_buf->read_index) means that buffer is empty
        return ring_buf->size - 1;
    }
}

/**
 * @brief       write data into ring buffer.
 * @param[in]   ring_buf: ring buffer structure pointer.
 * @param[in]   length: write buffer length.
 * @param[in]   buffer: write buffer pointer.
 * @return      number of bytes written.
 */
_attribute_ram_code_
static u16 ring_buf_write(ring_buf_t *ring_buf, u16 length, u8 *buffer)
{
    u16 free_space = ring_buf_free_space(ring_buf);
    u16 remaining = length;
    u16 len2;

    if (ring_buf->write_index >= ring_buf->read_index) {
        u16 len1 = free_space < (ring_buf->size - ring_buf->write_index) ? free_space : (ring_buf->size - ring_buf->write_index);
        if (len1 > remaining) {
            len1 = remaining;
        }

//      memcpy(&ring_buf->buffer[ring_buf->write_index], buffer, len1);
        //Interrupt code, must be placed in ramcode
        for(int i=0; i< len1; i++)
        {
            ring_buf->buffer[ring_buf->write_index + i] = buffer[i];
        }

        ring_buf->write_index = (ring_buf->write_index + len1) % ring_buf->size;
        remaining -= len1;
        buffer += len1;
        free_space -= len1;
    }

    len2 = remaining < free_space ? remaining : free_space;
//  memcpy(&ring_buf->buffer[ring_buf->write_index], buffer, len2);
    //Interrupt code, must be placed in ramcode
    for(int i=0; i< len2; i++)
    {
        ring_buf->buffer[ring_buf->write_index + i] = buffer[i];
    }
    remaining -= len2;
    ring_buf->write_index = (ring_buf->write_index + len2) % ring_buf->size;

    return length - remaining;
}

/**
 * @brief       read data into ring buffer.
 * @param[in]   ring_buf: ring buffer structure pointer.
 * @param[in]   length: want read buffer length.
 * @param[in]   buffer: read buffer pointer.
 * @return      number of bytes read.
 */
static u16 ring_buf_read(ring_buf_t *ring_buf, u16 length, u8 *buffer)
{
    u16 remaining = length;
    u16 available;

    if (ring_buf->read_index > ring_buf->write_index) {
        available = ring_buf->size - ring_buf->read_index;
        if (available > remaining) {
            available = remaining;
        }
        
        memcpy(buffer, &ring_buf->buffer[ring_buf->read_index], available);
        ring_buf->read_index = (ring_buf->read_index + available) % ring_buf->size;
        remaining -= available;
        buffer += available;
    }
    
    if (ring_buf->read_index < ring_buf->write_index) {
        available = ring_buf->write_index - ring_buf->read_index;
        if (available > remaining) {
            available = remaining;
        }
        
        memcpy(buffer, &ring_buf->buffer[ring_buf->read_index], available);
        ring_buf->read_index = (ring_buf->read_index + available) % ring_buf->size;
        remaining -= available;
        buffer += available;
    }
    
    return length - remaining;
}

/**
 * @brief       uart receive data irq handler.
 * @param[in]   none.
 * @return      none.
 */


_attribute_ram_code_ void uart_irq_handler(void)
{
#if(CHIP_TYPE == CHIP_TYPE_B91)
    if(uart_get_irq_status(PARSE_CHAR_UART_PORT,UART_TXDONE))
    {
        uart_clr_tx_done(PARSE_CHAR_UART_PORT);
    }
#elif(CHIP_TYPE == CHIP_TYPE_B92)
    if(uart_get_irq_status(PARSE_CHAR_UART_PORT,UART_TXDONE_IRQ_STATUS))
    {
        uart_clr_irq_status(PARSE_CHAR_UART_PORT, UART_TXDONE_IRQ_STATUS);
    }
#endif

#if(CHIP_TYPE == CHIP_TYPE_B91)
    if(uart_get_irq_status(PARSE_CHAR_UART_PORT,UART_RXDONE))
#elif(CHIP_TYPE == CHIP_TYPE_B92)
    if(uart_get_irq_status(PARSE_CHAR_UART_PORT,UART_RXDONE_IRQ_STATUS))
#endif
    {

        u32 rxLen;
        /* Get the length of Rx data */

        rxLen = uart_get_dma_rev_data_len(PARSE_CHAR_UART_PORT, PARSE_CHAR_UART_RX_DMA);
        // Currently don't care if there is enough room in ring buffer - data may be lost
        ring_buf_write(&appParseRingBuf, rxLen, uartRecvBuf);

        /* Clear RxDone state */
#if(CHIP_TYPE == CHIP_TYPE_B91)
        uart_clr_irq_status(PARSE_CHAR_UART_PORT, UART_CLR_RX);
#elif(CHIP_TYPE == CHIP_TYPE_B92)
        uart_clr_irq_status(PARSE_CHAR_UART_PORT, UART_RXDONE_IRQ_STATUS);
#endif
        uart_receive_dma(PARSE_CHAR_UART_PORT, uartRecvBuf, sizeof(uartRecvBuf));//[!!important - must]

        if((uart_get_irq_status(PARSE_CHAR_UART_PORT,UART_RX_ERR)))
        {
            #if(CHIP_TYPE == CHIP_TYPE_B91)
            uart_clr_irq_status(PARSE_CHAR_UART_PORT,UART_CLR_RX);
            #elif(CHIP_TYPE == CHIP_TYPE_B92)
            uart_clr_irq_status(PARSE_CHAR_UART_PORT,UART_RXDONE_IRQ_STATUS);
            #endif
        }
    }
}



_attribute_ram_code_ void uart1_irq_handler(void)
{
    uart_irq_handler();
}

PLIC_ISR_REGISTER(uart1_irq_handler, IRQ_UART1)
/**
 * @brief       parse initial interface(uart/usb-cdc) function.
 * @param[in]   none.
 * @return      none.
 */
static void init_interface(void)
{
    uart_reset(PARSE_CHAR_UART_PORT);

#if(CHIP_TYPE == CHIP_TYPE_B91)
    uart_set_pin(PARSE_CHAR_UART_TX_PIN, PARSE_CHAR_UART_RX_PIN);
#elif(CHIP_TYPE == CHIP_TYPE_B92)
    uart_set_pin(PARSE_CHAR_UART_PORT, PARSE_CHAR_B92_UART_TX_PIN, PARSE_CHAR_B92_UART_RX_PIN);
#endif
    unsigned short div;
    unsigned char bwpc;
    uart_cal_div_and_bwpc(PARSE_CHAR_UART_BAUDRATE, sys_clk.pclk*1000*1000, &div, &bwpc);
    uart_init(PARSE_CHAR_UART_PORT, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    uart_set_tx_dma_config(PARSE_CHAR_UART_PORT, PARSE_CHAR_UART_TX_DMA);
    uart_set_rx_dma_config(PARSE_CHAR_UART_PORT, PARSE_CHAR_UART_RX_DMA);

    uart_clr_irq_mask(PARSE_CHAR_UART_PORT, UART_RX_IRQ_MASK | UART_TX_IRQ_MASK | UART_TXDONE_MASK | UART_RXDONE_MASK);

#if(CHIP_TYPE == CHIP_TYPE_B91)
    uart_clr_tx_done(PARSE_CHAR_UART_PORT);
#elif(CHIP_TYPE == CHIP_TYPE_B92)
    uart_clr_irq_status(PARSE_CHAR_UART_PORT, UART_TXDONE_IRQ_STATUS);
#endif
    uart_set_rx_timeout(PARSE_CHAR_UART_PORT, bwpc, 12, UART_BW_MUL3);
    uart_set_irq_mask(PARSE_CHAR_UART_PORT, UART_RXDONE_MASK);

    plic_interrupt_enable(PARSE_CHAR_UART_PORT == UART0 ? IRQ_UART0:IRQ_UART1);
    plic_set_priority(PARSE_CHAR_UART_PORT == UART0 ? IRQ_UART0:IRQ_UART1, 1);

    uart_receive_dma(PARSE_CHAR_UART_PORT, uartRecvBuf, sizeof(uartRecvBuf));
}


/**
 * @brief       parse initial function.
 * @param[in]   parseList: parse command list.
 * @param[in]   size: list size.
 * @return      none.
 */
void app_parse_init(const parse_fun_list_t *parseList, int size)
{
    gParseList = (parse_fun_list_t *)(size_t)parseList;
    gParseSize = size;

    ring_buf_init(&appParseRingBuf, sizeof(ringBuf), ringBuf);

    init_interface();
}

/**
 * @brief       parse print log function.
 * @param[in]   Refer to printf parameter description.
 * @return      none.
 */
void app_parse_printf(const char *format, ...)
{
    u8 aclBuf[PARSE_CHAR_UART_BUFF_SIZE];
    va_list args;
    va_start( args, format );

    int ret = vsnprintf((char*)(aclBuf), PARSE_CHAR_UART_BUFF_SIZE, format, args);
    va_end( args );

    uart_send(PARSE_CHAR_UART_PORT, aclBuf, ret);
}

/**
 * @brief       Query the separator location.
 * @param[in]   str: input string, '\0' ending, ' ' or '\t' separator.
 * @return      next input parameter pointer.
 */
static char* tlk_strchr(char *str)
{
    bool stringFlag = false;

    while(*str == ' ' || *str == '\t')
    {
        str++;
    }

    if(*str == '"')
    {
        stringFlag = true;
        str++;
    }

    while(*str != '\0')
    {
        if(*str == '\\')    //Escape character
        {
            str++;
            if(*str == '"')
                str ++;
        }

        if(stringFlag)
        {
            if(*str == '"')
            {
                *str++ = '\0';
                break;
            }
        }
        else
        {
            if(*str == ' ' || *str == '\t')
            {
                *str++ = '\0';
                break;
            }
        }
        str++;
    }

    return str;
}

/**
 * @brief       parse string split input parameter.
 * @param[in]   str: input string, '\0' ending, ' ' or '\t' separator.
 * @param[out]  argv: split input parameter pointer.
 * @return      parameter size.
 */
static int tlk_split_argv(char *str, char *argv[])
{
    int argc = 0;

    if (!strlen(str))
    {
        return 0;
    }

    for(int i = strlen(str)-1; i>0; i--)
    {
        if(str[i] == '\r' || str[i] == '\n')
            str[i] = '\0';
        else
            break;
    }

    while (*str && (*str == ' ' || *str == '\t'))
    {
        str++;  //skip empty or tab
    }

    if (!*str)
    {
        return 0;
    }

    argv[argc++] = str;

    while ((str = tlk_strchr(str)))
    {
        while (*str && (*str == ' ' || *str == '\t'))
        {
            str++;
        }

        if (!*str)
        {
            break;
        }

        argv[argc++] = *str == '"'? str+1: str;

        if (argc == PARSE_CHAR_MAX_ARGV_SIZE)
        {
            app_parse_printf("Too many parameters (max %zu)\r\n", PARSE_CHAR_MAX_ARGV_SIZE);
            return 0;
        }
    }

    /* keep it POSIX style where argv[argc] is required to be NULL */
    argv[argc] = NULL;

    return argc;
}

/**
 * @brief       parse string loop.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_loop(void)
{
    while (true) {
        if (!ring_buf_read(&appParseRingBuf, 1, &shellRecvCmdBuf[shellRecvCmdBufIdx])) {
            return;
        }

        if (shellRecvCmdBuf[shellRecvCmdBufIdx] == '\n' || shellRecvCmdBuf[shellRecvCmdBufIdx] == '\r' ||
                shellRecvCmdBuf[shellRecvCmdBufIdx] == '\0' || shellRecvCmdBufIdx == (sizeof(shellRecvCmdBuf) - 1)) {
            shellRecvCmdBuf[shellRecvCmdBufIdx] = '\0';
            break;
        }

        // Continue reading characters
        shellRecvCmdBufIdx += 1;
    }

    // We have complete line
    if (shellRecvCmdBuf[shellRecvCmdBufIdx] == '\0') {
        char *argv[PARSE_CHAR_MAX_ARGV_SIZE];
        int argc = tlk_split_argv((char *)shellRecvCmdBuf, argv);

        for(int i = 0; i<gParseSize; i++)
        {
            if(strcasecmp(argv[0], gParseList[i].fun_name) == 0)
            {
                gParseList[i].fun(&argv[1], argc-1, gParseList[i].user_data);
            }
        }
        shellRecvCmdBufIdx = 0;
    }
}

/**
 * @brief       parse string to immediate value.
 * @param[in]   ps: value string, '\0' ending, supported -1, -0xAB, 1.
 * @return      immediate value.
 */
int app_parse_str2n (char * ps)
{
    int n = 0;
    int s = 1;
    int i = 0;
    int b = 10;

    while (ps[i]) {
        int c = ps[i];
        if (i==0 && c == '-') {
            s = -1;
        }
        else if (c == 'x' || c == 'X') {
            b = 16;
        }
        else {  //
            if (c>='A' && c<='F') {
                c = c - 'A' + 10;
            }
            else if (c>='a' && c<='f') {
                c = c - 'a' + 10;
            }
            else if (c>='0' && c<='9' ) {
                c -= '0';
            }
            else {
                c = 0;
            }
            n = n * b + c;

        }
        i++;
    }
    return s * n;
}

/**
 * @brief       parse string to immediate value.
 * @param[in]   ps: value string, '\0' ending, supported -1, -0xAB, 1.
 * @return      immediate value.
 */
int app_parse_str2xn (char * ps)
{
    int n = 0;
    int s = 1;
    int i = 0;
    int b = 16;

    while (ps[i]) {
        int c = ps[i];
        if (i==0 && c == '-') {
            s = -1;
        }
        else {  //
            if (c>='A' && c<='F') {
                c = c - 'A' + 10;
            }
            else if (c>='a' && c<='f') {
                c = c - 'a' + 10;
            }
            else if (c>='0' && c<='9' ) {
                c -= '0';
            }
            else {
                c = 0;
            }
            n = n * b + c;

        }
        i++;
    }
    return s * n;
}
#endif
