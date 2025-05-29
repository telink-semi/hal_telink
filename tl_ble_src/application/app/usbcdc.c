/********************************************************************************************************
 * @file    usbcdc.c
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
#include "usbcdc.h"
#if (USB_CDC_ENABLE)
    #include "../usbstd/usb.h"

unsigned char            LineCoding[7] = {0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08};
static usb_cdc_read_cb_t rx_cb;

unsigned short usb_cdc_write(unsigned char *data, unsigned short length)
{
    unsigned short written = 0;

    if (usbhw_is_ep_busy(USB_EDP_CDC_IN)) {
        return written;
    }
    /*If the length of the data sent is equal to the wMaxPacketSize (CDC_TXRX_EPSIZE),
             the device must return a zero-length packet to indicate the end of the data stage,
             The following is the process of sending zero-length packet*/
    if (length > CDC_TXRX_EPSIZE - 1) {
        length = CDC_TXRX_EPSIZE - 1;
    }

    usbhw_reset_ep_ptr(USB_EDP_CDC_IN);

    for (; written < length; written++) {
        reg_usb_ep_dat(USB_EDP_CDC_IN) = (*data);
        ++data;
    }

    usbhw_data_ep_ack(USB_EDP_CDC_IN);

    return written;
}

void usb_cdc_read(usb_cdc_read_cb_t cb)
{
    rx_cb = cb;
}

void usb_cdc_rx_data_from_host(void)
{
    static unsigned char usb_cdc_data[CDC_TXRX_EPSIZE];
    unsigned short       usb_cdc_data_len;
    unsigned char        rx_from_usbhost_len = reg_usb_ep_ptr(USB_EDP_CDC_OUT);
    usbhw_reset_ep_ptr(USB_EDP_CDC_OUT);


    if (rx_from_usbhost_len > 0 && rx_from_usbhost_len <= CDC_TXRX_EPSIZE) {
        for (int i = 0; i < rx_from_usbhost_len; ++i) {
            usb_cdc_data[i] = reg_usb_ep_dat(USB_EDP_CDC_OUT);
        }
        usb_cdc_data_len = rx_from_usbhost_len & 0xff; //uart dma: first 4 bytes is the length of packet

        usb_cdc_read_cb_t cb = rx_cb;
        rx_cb                = NULL;
        cb(usb_cdc_data, usb_cdc_data_len);
    }
}

void usb_cdc_irq_data_process(void)
{
    unsigned char irq = usbhw_get_eps_irq();

    if (irq & FLD_USB_EDP4_IRQ) {
        usbhw_clr_eps_irq(FLD_USB_EDP4_IRQ);
        usbhw_reset_ep_ptr(USB_EDP_CDC_IN);
    }

    if (irq & FLD_USB_EDP5_IRQ) {
        if (rx_cb) {
            usbhw_clr_eps_irq(FLD_USB_EDP5_IRQ);
            usb_cdc_rx_data_from_host();
            usbhw_data_ep_ack(USB_EDP_CDC_OUT);
        }
    }
}

void usb_cdc_set_line_encoding(int data_request)
{
    if (USB_IRQ_DATA_REQ == data_request) {
        usbhw_reset_ctrl_ep_ptr();

        for (unsigned short i = 0; i < sizeof(LineCoding); i++) {
            LineCoding[i] = usbhw_read_ctrl_ep_data();
        }
    }
}

void usb_cdc_get_line_encoding(void)
{
    for (unsigned short i = 0; i < sizeof(LineCoding); i++) {
        usbhw_write_ctrl_ep_data(LineCoding[i]);
    }
}

#endif
