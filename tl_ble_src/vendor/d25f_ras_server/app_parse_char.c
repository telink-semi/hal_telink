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

typedef struct __attribute__((packed))
{
    u16 write_index;
    u16 read_index;
    u16 size;
    u8 *buffer;
} ring_buf_t;

u8                shellRecvCmdBuf[PARSE_CHAR_UART_BUFF_SIZE + 1]; //str + '\0'
u16               shellRecvCmdBufIdx = 0;
u8                uartRecvBuf[PARSE_CHAR_UART_BUFF_SIZE];
parse_fun_list_t *gParseList = NULL;
int               gParseSize = 0;
static ring_buf_t appParseRingBuf;
static u8         ringBuf[PARSE_CHAR_UART_BUFF_SIZE * 2];

// for DMA print
volatile u8 is_app_parse_dma_busy = 0;
static u8   parse_char_dma_buffer[PARSE_CHAR_UART_BUFF_SIZE];
/**
 * @brief       ring buffer initial function.
 * @param[in]   ring_buf: ring buffer structure pointer.
 * @param[in]   size: ring buffer size.
 * @param[in]   buffer: ring buffer store data pointer.
 * @return      none.
 */
static void ring_buf_init(ring_buf_t *ring_buf, u16 size, u8 *buffer)
{
    ring_buf->size        = size;
    ring_buf->buffer      = buffer;
    ring_buf->write_index = 0;
    ring_buf->read_index  = 0;
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
_attribute_ram_code_ static u16 ring_buf_write(ring_buf_t *ring_buf, u16 length, u8 *buffer)
{
    u16 free_space = ring_buf_free_space(ring_buf);
    u16 remaining  = length;
    u16 len2;

    if (ring_buf->write_index >= ring_buf->read_index) {
        u16 len1 = free_space < (ring_buf->size - ring_buf->write_index) ? free_space : (ring_buf->size - ring_buf->write_index);
        if (len1 > remaining) {
            len1 = remaining;
        }

        //      memcpy(&ring_buf->buffer[ring_buf->write_index], buffer, len1);
        //Interrupt code, must be placed in ramcode
        for (int i = 0; i < len1; i++) {
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
    for (int i = 0; i < len2; i++) {
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



_attribute_ram_code_sec_ void hci_uart_irq_handler(void)
{
     if (uart_get_irq_status(UART_MODULE_SEL,UART_TXDONE_IRQ_STATUS)) {
         uart_clr_irq_status(UART_MODULE_SEL,UART_TXDONE_IRQ_STATUS);
         is_app_parse_dma_busy = 0;
     }
}
PLIC_ISR_REGISTER(hci_uart_irq_handler, UART_MODULE_IRQ);


_attribute_ram_code_ void hci_uart_dma_irq_handler(void)
{

    if (dma_get_tc_irq_status( BIT(PARSE_CHAR_UART_RX_DMA))) {

        ring_buf_write(&appParseRingBuf, uartRecvBuf[0], uartRecvBuf+4);

        if ((uart_get_irq_status(UART_MODULE_SEL,UART_RX_ERR))) {
            uart_clr_irq_status(UART_MODULE_SEL,UART_RXBUF_IRQ_STATUS);
        }
        dma_clr_tc_irq_status( BIT(PARSE_CHAR_UART_RX_DMA));

        uart_receive_dma(UART_MODULE_SEL, uartRecvBuf + 4, sizeof(uartRecvBuf)-4);
    }
}
PLIC_ISR_REGISTER(hci_uart_dma_irq_handler, IRQ_DMA)



/**
 * @brief       parse initial interface(uart/usb-cdc) function.
 * @param[in]   none.
 * @return      none.
 */
static void init_interface(void)
{

    unsigned short div;
    unsigned char bwpc;

    uart_hw_fsm_reset(UART_MODULE_SEL);
    uart_set_pin(UART_MODULE_SEL, PARSE_CHAR_UART_TX_PIN, PARSE_CHAR_UART_RX_PIN);
    uart_cal_div_and_bwpc(1000000, sys_clk.pclk*1000*1000, &div, &bwpc);
    uart_set_rx_timeout_with_exp(UART_MODULE_SEL, bwpc, 12, UART_BW_MUL2,0);
    uart_init(UART_MODULE_SEL, div, bwpc, UART_PARITY_NONE, UART_STOP_BIT_ONE);

    uart_set_tx_dma_config(UART_MODULE_SEL,PARSE_CHAR_UART_TX_DMA);
    uart_set_rx_dma_config(UART_MODULE_SEL,PARSE_CHAR_UART_RX_DMA);

    uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
    uart_set_irq_mask(UART_MODULE_SEL, UART_TXDONE_MASK);
    plic_interrupt_enable(UART_MODULE_IRQ);
    plic_set_priority(UART_MODULE_IRQ, 1);

    dma_set_irq_mask(PARSE_CHAR_UART_RX_DMA, TC_MASK);
    plic_interrupt_enable(IRQ_DMA);

    uart_receive_dma(UART_MODULE_SEL, uartRecvBuf+4, sizeof(uartRecvBuf-4)); //[!!important - must]

    uart_clr_irq_status(UART_MODULE_SEL, UART_TXDONE_IRQ_STATUS);
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
    va_list args;
    va_start(args, format);

    while(is_app_parse_dma_busy);
    int ret = vsnprintf((char *)(parse_char_dma_buffer), PARSE_CHAR_UART_BUFF_SIZE, format, args);
    va_end(args);
    uart_send_dma(UART_MODULE_SEL,parse_char_dma_buffer, ret);
    is_app_parse_dma_busy = 1;
}

/**
 * @brief       Query the separator location.
 * @param[in]   str: input string, '\0' ending, ' ' or '\t' separator.
 * @return      next input parameter pointer.
 */
static char *tlk_strchr(char *str)
{
    bool stringFlag = false;

    while (*str == ' ' || *str == '\t') {
        str++;
    }

    if (*str == '"') {
        stringFlag = true;
        str++;
    }

    while (*str != '\0') {
        if (*str == '\\') //Escape character
        {
            str++;
            if (*str == '"') {
                str++;
            }
        }

        if (stringFlag) {
            if (*str == '"') {
                *str++ = '\0';
                break;
            }
        } else {
            if (*str == ' ' || *str == '\t') {
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

    if (!strlen(str)) {
        return 0;
    }

    for (int i = strlen(str) - 1; i > 0; i--) {
        if (str[i] == '\r' || str[i] == '\n') {
            str[i] = '\0';
        } else {
            break;
        }
    }

    while (*str && (*str == ' ' || *str == '\t')) {
        str++; //skip empty or tab
    }

    if (!*str) {
        return 0;
    }

    argv[argc++] = str;

    while ((str = tlk_strchr(str))) {
        while (*str && (*str == ' ' || *str == '\t')) {
            str++;
        }

        if (!*str) {
            break;
        }

        argv[argc++] = *str == '"' ? str + 1 : str;

        if (argc == PARSE_CHAR_MAX_ARGV_SIZE) {
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

        if (shellRecvCmdBuf[shellRecvCmdBufIdx] == '\n' || shellRecvCmdBuf[shellRecvCmdBufIdx] == '\r' || shellRecvCmdBuf[shellRecvCmdBufIdx] == '\0' ||
            shellRecvCmdBufIdx == (sizeof(shellRecvCmdBuf) - 1)) {
            shellRecvCmdBuf[shellRecvCmdBufIdx] = '\0';
            break;
        }

        // Continue reading characters
        shellRecvCmdBufIdx += 1;
    }

    // We have complete line
    if (shellRecvCmdBuf[shellRecvCmdBufIdx] == '\0') {
        char *argv[PARSE_CHAR_MAX_ARGV_SIZE];
        int   argc = tlk_split_argv((char *)shellRecvCmdBuf, argv);

        for (int i = 0; i < gParseSize; i++) {
            if (strcasecmp(argv[0], gParseList[i].fun_name) == 0) {
                gParseList[i].fun(&argv[1], argc - 1, gParseList[i].user_data);
            }
        }
        memset(shellRecvCmdBuf, 0, shellRecvCmdBufIdx + 1);
        shellRecvCmdBufIdx = 0;
    }
}

/**
 * @brief       parse string to immediate value.
 * @param[in]   ps: value string, '\0' ending, supported -1, -0xAB, 1.
 * @return      immediate value.
 */
int app_parse_str2n(char *ps)
{
    int n = 0;
    int s = 1;
    int i = 0;
    int b = 10;

    while (ps[i]) {
        int c = ps[i];
        if (i == 0 && c == '-') {
            s = -1;
        } else if (c == 'x' || c == 'X') {
            b = 16;
        } else { //
            if (c >= 'A' && c <= 'F') {
                c = c - 'A' + 10;
            } else if (c >= 'a' && c <= 'f') {
                c = c - 'a' + 10;
            } else if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
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
int app_parse_str2xn(char *ps)
{
    int n = 0;
    int s = 1;
    int i = 0;
    int b = 16;

    while (ps[i]) {
        int c = ps[i];
        if (i == 0 && c == '-') {
            s = -1;
        } else { //
            if (c >= 'A' && c <= 'F') {
                c = c - 'A' + 10;
            } else if (c >= 'a' && c <= 'f') {
                c = c - 'a' + 10;
            } else if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                c = 0;
            }
            n = n * b + c;
        }
        i++;
    }
    return s * n;
}
#endif
