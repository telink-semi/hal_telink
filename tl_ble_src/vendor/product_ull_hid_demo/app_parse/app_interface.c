#include "tl_common.h"
#include "drivers.h"
#include "app_parse_cfg.h"
#include "app_ringbuffer.h"
#include "../ull_hid_config.h"
u8 uartRecvBuf[PARSE_CHAR_UART_BUFF_SIZE];
extern ring_buf_t appParseRingBuf;

#if (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_USB_CDC)
#include "application/app/usbcdc.h"
#include "application/usbstd/usb.h"
u8 cdcBuf[CDC_TXRX_EPSIZE];

#define RING_BUFF_SIZE                    2048

u8 cdcRingBuf[RING_BUFF_SIZE];
int cdcRingWptr = 0;
int cdcRingRptr = 0;

void app_cdc_loop(void)
{

    /* The busy check is repeated in the usb_cdc_tx_data_to_host() function, but it can */
    /* be used separately as well. This is to avoid unnecessary buffer preparation.        */
    if(usbhw_is_ep_busy(USB_EDP_CDC_IN))
    {
        return;
    }

    int size = (cdcRingWptr-cdcRingRptr)&(RING_BUFF_SIZE-1);
    int resSize = min(size, CDC_TXRX_EPSIZE-1);

    if(resSize != 0)
    {
//        tlkapi_printf(1, "get data size is %d %d\n", resSize, size);
        for(int i=0; i<resSize; i++)
        {
            cdcBuf[i] = cdcRingBuf[cdcRingRptr++];
            cdcRingRptr &= (RING_BUFF_SIZE-1);
        }

        // tlkapi_printf(1, "cdcBuf: %s \n", cdcBuf);

        usb_cdc_write(cdcBuf, resSize);

    }
}

void app_cdc_send_value(unsigned char * data_ptr, unsigned short data_len)
{
    for(int i=0; i < data_len; i++)
    {
        cdcRingBuf[cdcRingWptr++] = data_ptr[i];
        cdcRingWptr &= (RING_BUFF_SIZE-1);
    }
}

#endif

#if (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_UART)
/**
 * @brief        uart receive data irq handler.
 * @param[in]    none.
 * @return        none.
 */


_attribute_ram_code_ void uart_irq_handler1(void)
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
        ring_buf_write(&appParseRingBuf, 1, '\0');

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


_attribute_ram_code_ void uart0_irq_handler(void)
{
    uart_irq_handler1();
}
PLIC_ISR_REGISTER(uart0_irq_handler, IRQ_UART0)

#else

/**
 * @brief        usb-cdc receive data callback.
 * @param[in]    data: usb receive data pointer.
 * @param[in]    length: data length.
 * @return        none.
 */
static void usb_cdc_read_cb(unsigned char * data, unsigned short length)
{
    ring_buf_write(&appParseRingBuf, length, data);
    usb_cdc_read(usb_cdc_read_cb);
}
#endif

/**
 * @brief        parse initial interface(uart/usb-cdc) function.
 * @param[in]    none.
 * @return        none.
 */
void init_interface(void)
{
#if (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_UART)
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

    plic_interrupt_enable(PARSE_CHAR_UART_PORT == UART0 ? IRQ19_UART0:IRQ18_UART1);
    plic_set_priority(PARSE_CHAR_UART_PORT == UART0 ? IRQ19_UART0:IRQ18_UART1, 2);

    uart_receive_dma(PARSE_CHAR_UART_PORT, uartRecvBuf, sizeof(uartRecvBuf));
#elif (APP_AUDIO_UI_IFACE == APP_AUDIO_UI_USB_CDC)
    reg_usb_ep1_buf_addr = 0x00;
    reg_usb_ep3_buf_addr = 0x00;
    reg_usb_ep6_buf_addr = 0x00;
    reg_usb_ep7_buf_addr = 0xc0;
    reg_usb_ep8_buf_addr = 0xc0;
    reg_usb_ep5_buf_addr = 0xc0;
    reg_usb_ep4_buf_addr = 0xe0;
    reg_usb_ep2_buf_addr = 0x00;

    usb_set_pin_en();
    usb_init();
    usbhw_data_ep_ack(USB_EDP_CDC_OUT);
    core_interrupt_enable();
    usbhw_set_irq_mask(USB_IRQ_RESET_MASK|USB_IRQ_SUSPEND_MASK);
    usb_cdc_read(usb_cdc_read_cb);
#endif
}

