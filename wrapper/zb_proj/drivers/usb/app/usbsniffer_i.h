/********************************************************************************************************
 * @file    usbsniffer_i.h
 *
 * @brief   This is the header file for usbsniffer_i
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
#pragma once

#include "../usbdesc.h"

#if (__PROJECT_TL_SNIFFER__)


#define SNIFFER_DEVICE_DESC     { \
    {sizeof(USB_Descriptor_Device_t), DTYPE_Device},    \
    0x0200,                                             \
    USB_CSCP_NoDeviceClass,                             \
    USB_CSCP_NoDeviceSubclass,                          \
    USB_CSCP_NoDeviceProtocol,                          \
    8,                                                  \
    ID_VENDOR,                                          \
    ID_PRODUCT,                                         \
    ID_VERSION,                                         \
    USB_STRING_VENDOR,                                  \
    USB_STRING_PRODUCT,                                 \
    0,                                                  \
    1                                                   \
}

#define SNIFFER_INTERFACE       { \
    {sizeof(USB_Descriptor_Interface_t), DTYPE_Interface},      \
    0,                                                          \
    0,                                                          \
    1,                                                          \
    CDC_CSCP_VendorSpecificProtocol,                            \
    CDC_CSCP_VendorSpecificProtocol,                            \
    CDC_CSCP_VendorSpecificProtocol,                            \
    NO_DESCRIPTOR                                               \
}

#define SNIFFER_ENDPOINT        { \
    {sizeof(USB_Descriptor_Endpoint_t), DTYPE_Endpoint},        \
    (ENDPOINT_DIR_IN | SNIFFER_TX_EPNUM),                       \
    EP_TYPE_BULK,                                               \
    SNIFFER_TX_EPSIZE,                                          \
    5                                                           \
}

#endif
