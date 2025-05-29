#pragma once

#ifndef PARSE_CHAR_UART_PORT
    #define PARSE_CHAR_UART_PORT UART0
#endif

#ifndef PARSE_CHAR_UART_TX_DMA
    #define PARSE_CHAR_UART_TX_DMA DMA5
#endif

#ifndef PARSE_CHAR_UART_RX_DMA
    #define PARSE_CHAR_UART_RX_DMA DMA6
#endif

#ifndef PARSE_CHAR_UART_TX_PIN
    #define PARSE_CHAR_UART_TX_PIN UART0_TX_PD2
#endif

#ifndef PARSE_CHAR_UART_RX_PIN
    #define PARSE_CHAR_UART_RX_PIN UART0_RX_PD3
#endif

#ifndef PARSE_CHAR_B92_UART_TX_PIN
    #define PARSE_CHAR_B92_UART_TX_PIN GPIO_FC_PA0
#endif

#ifndef PARSE_CHAR_B92_UART_RX_PIN
    #define PARSE_CHAR_B92_UART_RX_PIN GPIO_FC_PA1
#endif

#ifndef PARSE_CHAR_UART_BAUDRATE
    #define PARSE_CHAR_UART_BAUDRATE 1000000
#endif

#ifndef PARSE_CHAR_UART_BUFF_SIZE
    #define PARSE_CHAR_UART_BUFF_SIZE 256
#endif

#ifndef PARSE_CHAR_MAX_ARGV_SIZE
    #define PARSE_CHAR_MAX_ARGV_SIZE 16
#endif
