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
#include "../bis_source_config.h"

#if (PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER)

#pragma once


#ifndef PARSE_CHAR_UART_PORT
#define PARSE_CHAR_UART_PORT                    UART0
#endif

#ifndef PARSE_CHAR_UART_TX_DMA
#define PARSE_CHAR_UART_TX_DMA                  DMA5
#endif

#ifndef PARSE_CHAR_UART_RX_DMA
#define PARSE_CHAR_UART_RX_DMA                  DMA6
#endif

#ifndef PARSE_CHAR_UART_TX_PIN
#define PARSE_CHAR_UART_TX_PIN                  UART0_TX_PA3
#endif

#ifndef PARSE_CHAR_UART_RX_PIN
#define PARSE_CHAR_UART_RX_PIN                  UART0_RX_PA4
#ifndef PARSE_CHAR_B92_UART_TX_PIN
#define PARSE_CHAR_B92_UART_TX_PIN              GPIO_FC_PA0
#endif

#ifndef PARSE_CHAR_B92_UART_RX_PIN
#define PARSE_CHAR_B92_UART_RX_PIN              GPIO_FC_PA1
#endif

#ifndef PARSE_CHAR_UART_BAUDRATE
#define PARSE_CHAR_UART_BAUDRATE                1000000
#endif

#ifndef PARSE_CHAR_UART_BUFF_SIZE
#define PARSE_CHAR_UART_BUFF_SIZE               128
#endif

#ifndef PARSE_CHAR_MAX_ARGV_SIZE
#define PARSE_CHAR_MAX_ARGV_SIZE                16
#endif

typedef struct{
    char *fun_name;
    void (*fun)(char *argv[], int argc, void *user_data);
    void *user_data;
} parse_fun_list_t;

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
 * @param[in]   ps: value string, '\0' ending,
 *              hexadecimal: start with 0x or -0x.
 *              Octal number: start with 0 or -0.
 *
 * @return      immediate value.
 */
int app_parse_str2n(char * ps);

/**
 * @brief       parse print log function.
 * @param[in]   Refer to printf parameter description.
 * @return      none.
 */
void app_parse_printf(const char *format, ...);

#endif  //PRODUCT_BIS_SOURCE_SELECT == PRODUCT_SIG_AURACAST_TRANSMITTER
