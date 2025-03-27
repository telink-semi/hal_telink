/********************************************************************************************************
 * @file    svc_keyboard.c
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
#include "svc_mouse.h"
#include "stack/ble/service/hids.h"

#if SVC_DEFAULT_MOUSE_ENABLE
const unsigned char hidReportMap[] = {
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x02, // USAGE (Mouse)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, HID_REPORT_ID_MOUSE_INPUT, //report ID 01
    0x09, 0x01, //   USAGE (Pointer)
    0xa1, 0x00, //   COLLECTION (Physical)
    0x05, 0x09, //     USAGE_PAGE (Button)
    // 1 is mouse left button,2 is mouse right button,3 is central button
    0x19, 0x01, //     USAGE_MINIMUM (Button 1)
    0x29, 0x05, //     USAGE_MAXIMUM (Button 5)
    0x15, 0x00, //     LOGICAL_MINIMUM (0)
    0x25, 0x01, //     LOGICAL_MAXIMUM (1)
    0x95, 0x05, //     REPORT_COUNT (3)
    0x75, 0x01, //     REPORT_SIZE (1)
    0x81, 0x02, //     INPUT (Data,Var,Abs)
    0x95, 0x01, //     REPORT_COUNT (1)
    0x75, 0x03, //     REPORT_SIZE (3)
    0x81, 0x01, //     INPUT (Cnst,Var,Abs)
    0x05, 0x01, //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30, //     USAGE (X)
    0x09, 0x31, //     USAGE (Y)
    0x15, 0x81, //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //     LOGICAL_MAXIMUM (127)
    0x75, 0x08, //     REPORT_SIZE (16)
    0x95, 0x02, //     REPORT_COUNT (2)
    0x81, 0x06, //     INPUT (Data,Var,Rel)
    0x09, 0x38, //     USAGE (Wheel)
    0x15, 0x81, //LOGICAL_MINIMUM (-127)
    0x25, 0x7f, //LOGICAL_MAXIMUM (127)
    0x75, 0x08, //REPORT_SIZE (16)
    0x95, 0x01, //REPORT_COUNT (1)
    0x81, 0x06, //INPUT (Data,Var,Rel)
    0xc0, //   END_COLLECTION
    0xc0, // END_COLLECTION
};

const unsigned short hidReportMapLen = sizeof(hidReportMap);
#endif
