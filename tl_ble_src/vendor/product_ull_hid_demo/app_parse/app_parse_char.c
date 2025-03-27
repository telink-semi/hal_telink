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
#include "application/app/usbcdc.h"
#include "application/usbstd/usb.h"
#include "app_parse_char.h"
#include "app_parse_cfg.h"
#include "app_ringbuffer.h"

static u8 ringBuf[PARSE_CHAR_UART_BUFF_SIZE * 2];
u8 shellRecvCmdBuf[PARSE_CHAR_UART_BUFF_SIZE + 1];        //str + '\0'
u16 shellRecvCmdBufIdx = 0;
int gParseSize = 0;
ring_buf_t appParseRingBuf;
parse_fun_list_t* gParseList = NULL;
extern void init_interface(void);
/**
 * @brief        parse initial function.
 * @param[in]    parseList: parse command list.
 * @param[in]    size: list size.
 * @return        none.
 */
void app_parse_init(const parse_fun_list_t *parseList, int size)
{
    gParseList = (parse_fun_list_t *)parseList;
    gParseSize = size;

    ring_buf_init(&appParseRingBuf, sizeof(ringBuf), ringBuf);

    init_interface();
}

/**
 * @brief        parse print log function.
 * @param[in]    Refer to printf parameter description.
 * @return        none.
 */
void app_parse_printf(const char *format, ...)
{
    u8 aclBuf[PARSE_CHAR_UART_BUFF_SIZE];
    va_list args;
    va_start( args, format );

    int ret = vsnprintf((char*)(aclBuf), PARSE_CHAR_UART_BUFF_SIZE, format, args);
    va_end( args );

#if (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_UART)
    uart_send(PARSE_CHAR_UART_PORT, aclBuf, ret);
#elif (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_USB_CDC)
    app_cdc_send_value(&aclBuf[0], ret);
#endif
}

/**
 * @brief        Query the separator location.
 * @param[in]    str: input string, '\0' ending, ' ' or '\t' separator.
 * @return        next input parameter pointer.
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
 * @brief        parse string split input parameter.
 * @param[in]    str: input string, '\0' ending, ' ' or '\t' separator.
 * @param[out]    argv: split input parameter pointer.
 * @return        parameter size.
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
        str++;    //skip empty or tab
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
 * @brief        parse string loop.
 * @param[in]    none.
 * @return        none.
 */
void app_parse_loop(void)
{
#if (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_USB_CDC)
    usb_handle_irq();
    app_cdc_loop();
#endif
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
 * @brief        parse string to immediate value.
 * @param[in]    ps: value string, '\0' ending, supported -1, -0xAB, 1.
 * @return        immediate value.
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
        else {    //
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

