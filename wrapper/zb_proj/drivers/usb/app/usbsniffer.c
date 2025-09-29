/********************************************************************************************************
 * @file    usbsniffer.c
 *
 * @brief   This is the source file for usbsniffer
 *
 * @author  Driver & Zigbee Group
 * @date    2021
 *
 * @par     Copyright (c) 2021, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
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
#include "usbsniffer.h"
#include "../usb.h"

#if (__PROJECT_TL_SNIFFER__)

#define	SNIFFER_HDR_LEN         8

u8 *g_sniffer_sendAddr = NULL;
u8 g_sniffer_sendRspData = 0;
u8 g_sniffer_remind_pktLen = 0;
u8 g_sniffer_rfChannel = 11;
bool g_sniffer_rfCaptureStarted = 0;


void usbSniffer_processControlRequest(u8 bmRequestType, u8 data_request, u8 bRequest, u16 wIndex)
{
    switch (bmRequestType) {
    case (REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE):
        if (USB_IRQ_SETUP_REQ == data_request) {
            if (0xc0 == bRequest) { // Get board version
                usbhw_reset_ctrl_ep_ptr();
                usbhw_write_ctrl_ep_data(0x31);
                usbhw_write_ctrl_ep_data(0x25);
                usbhw_write_ctrl_ep_data(0x31);
                usbhw_write_ctrl_ep_data(0x05);
                usbhw_write_ctrl_ep_data(0x02);
                usbhw_write_ctrl_ep_data(0x00);
                usbhw_write_ctrl_ep_data(0x01);
                usbhw_write_ctrl_ep_data(0x00);
            } else if (0xc6 == bRequest) {
                usbhw_reset_ctrl_ep_ptr();
                g_sniffer_sendRspData++;
                if (g_sniffer_sendRspData < 4) {
                    usbhw_write_ctrl_ep_data(0x01);
                } else {
                    usbhw_write_ctrl_ep_data(0x04);
                    g_sniffer_sendRspData = 0;
                }
            }
        }
        break;
    case (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE):
        if (USB_IRQ_SETUP_REQ == data_request) {
            if (0xd0 == bRequest) { // start capture
                g_sniffer_rfCaptureStarted = 1;
            } else if (0xd1 == bRequest) { // stop capture
                g_sniffer_rfCaptureStarted = 0;
            }
        } else {
            if (0xd2 == bRequest) {
                if (0 == wIndex) {// set channel
                    g_sniffer_rfChannel = usbhw_read_ctrl_ep_data();
                }
            }
        }
        break;
    default:
        break;
    }
}

_attribute_ram_code_ u8 usbSniffer_isTxBusy(void)
{
    return usbhw_is_ep_busy(SNIFFER_TX_EPNUM);
}

void usbSniffer_init(void)
{
    u16 usbID = 0;
    flash_read(CFG_TELINK_USB_ID, 2, (u8 *)&usbID);
    if (usbID == 0xffff) {
        do {
            usbID = (u16)drv_u32Rand();
        } while ((usbID == 0xffff) || (usbID == 0));

        flash_write(CFG_TELINK_USB_ID, 2, (u8 *)&usbID);
    }
    USB_Descriptor_Device_t *p = (USB_Descriptor_Device_t *)usbdesc_get_device();
    p->ReleaseNumber = usbID;
}

_attribute_ram_code_ void usbSniffer_sendRemainMsg(void)
{
    if (usbSniffer_isTxBusy() || !g_sniffer_remind_pktLen) {
        return;
    }

    u8 len = g_sniffer_remind_pktLen;
    u8 *pData = g_sniffer_sendAddr;
    if (g_sniffer_remind_pktLen > SNIFFER_TX_EPSIZE) {
        len = SNIFFER_TX_EPSIZE;
        g_sniffer_sendAddr += SNIFFER_TX_EPSIZE;
    }
    g_sniffer_remind_pktLen -= len;

    usbhw_reset_ep_ptr(SNIFFER_TX_EPNUM);

    for (u8 i = 0; i < len; i++) {
        usbhw_write_ep_data(SNIFFER_TX_EPNUM, pData[i]); // raw pkt
    }

    usbhw_data_ep_ack(SNIFFER_TX_EPNUM);
}

_attribute_ram_code_ void usbSniffer_sendMsg(u8 *pData, u8 dataLen, u32 t)
{
    if (!g_sniffer_rfCaptureStarted || g_sniffer_remind_pktLen ||
        !dataLen || !pData) {
        return;
    }

    usbhw_reset_ep_ptr(SNIFFER_TX_EPNUM);

    //HDR length 8
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, 0);           // pkt  status
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, dataLen + 5); // pkt len
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, 0);           // pkt  len
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, t);           // time stamp
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, (t >> 8));    // time stamp
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, (t >> 16));   // time stamp
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, (t >> 24));   // time stamp
    usbhw_write_ep_data(SNIFFER_TX_EPNUM, dataLen);     // payload len

    if (dataLen + SNIFFER_HDR_LEN > SNIFFER_TX_EPSIZE) {
        g_sniffer_remind_pktLen = dataLen + SNIFFER_HDR_LEN - SNIFFER_TX_EPSIZE;
        dataLen = SNIFFER_TX_EPSIZE - SNIFFER_HDR_LEN;
        g_sniffer_sendAddr = pData + dataLen;
    }

    for (u8 i = 0; i < dataLen; i++) {
        usbhw_write_ep_data(SNIFFER_TX_EPNUM, pData[i]); // raw pkt
    }

    usbhw_data_ep_ack(SNIFFER_TX_EPNUM);
}

#endif
