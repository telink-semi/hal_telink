/********************************************************************************************************
 * @file    app_parse_char.h
 *
 * @brief   This is the header file for BLE SDK
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

#pragma once
#include "app_config.h"
#if (INTER_TEST_MODE == TEST_HDT_SENDER)
#if (BOARD_SELECT == BOARD_952X_EVK_C1T266A20)
#define PARSE_CHAR_UART_TX_PIN GPIO_FC_PC7
#define PARSE_CHAR_UART_RX_PIN GPIO_FC_PC6
#elif (BOARD_SELECT == BOARD_952X_EVK_C1T266A102)
#define PARSE_CHAR_UART_TX_PIN GPIO_FC_PB7
#define PARSE_CHAR_UART_RX_PIN GPIO_FC_PB6
#elif (BOARD_SELECT == BOARD_721X_EVK_C1T315A20)
#define PARSE_CHAR_UART_TX_PIN GPIO_FC_PC7
#define PARSE_CHAR_UART_RX_PIN GPIO_FC_PC6
#elif (BOARD_SELECT == BOARD_721X_EVK_C1T315A102)
#define PARSE_CHAR_UART_TX_PIN GPIO_FC_PF7
#define PARSE_CHAR_UART_RX_PIN GPIO_FC_PF6
#endif

#ifndef PARSE_CHAR_UART_PORT
#define PARSE_CHAR_UART_PORT UART1
#endif

#ifndef PARSE_CHAR_UART_TX_DMA
#define PARSE_CHAR_UART_TX_DMA DMA5
#endif

#ifndef PARSE_CHAR_UART_RX_DMA
#define PARSE_CHAR_UART_RX_DMA DMA6
#endif

#ifndef PARSE_CHAR_UART_TX_PIN
#define PARSE_CHAR_UART_TX_PIN GPIO_FC_PC7
#endif

#ifndef PARSE_CHAR_UART_RX_PIN
#define PARSE_CHAR_UART_RX_PIN GPIO_FC_PC6
#endif

#ifndef PARSE_CHAR_UART_BAUDRATE
#define PARSE_CHAR_UART_BAUDRATE 1000000
#endif

#ifndef PARSE_CHAR_UART_BUFF_SIZE
#define PARSE_CHAR_UART_BUFF_SIZE 256
#endif

#ifndef PARSE_CHAR_MAX_ARGV_SIZE
#define PARSE_CHAR_MAX_ARGV_SIZE 40
#endif

typedef struct __attribute__((packed))
{
    char fun_name[16];
    void (*fun)(char *argv[], int argc, void *user_data);
    void *user_data;
} parse_fun_list_t;

typedef struct __attribute__((packed))
{
    char  param_name[16];
    void *param_ptr;
    char  type;    //0:string, other: Numerical size
    char  maxSize; //
} set_param_list_t;

/**
 * @brief       parse initial function.
 * @param[in]   parseList: parse command list.
 * @param[in]   size: list size.
 * @return      none.
 */
void app_parse_init(const parse_fun_list_t *parseList, int size);

/**
 * @brief       parse string loop.
 * @param[in]   none.
 * @return      none.
 */
void app_parse_loop(void);

/**
 * @brief       parse string to immediate value.
 * @param[in]   ps: value string, '\0' ending, supported -1, -0xAB, 1.
 * @return      immediate value.
 */
int app_parse_str2n(char *ps);

/**
 * @brief       parse string to immediate value.
 * @param[in]   ps: value string in hex, '\0' ending, supported -1, -0xAB, 1.
 * @return      immediate value.
 */
int app_parse_str2xn(char *ps);

/**
 * @brief       parse print log function.
 * @param[in]   Refer to printf parameter description.
 * @return      none.
 */
void app_parse_printf(const char *format, ...);
#endif
