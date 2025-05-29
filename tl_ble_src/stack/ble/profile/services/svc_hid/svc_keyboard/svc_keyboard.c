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

#include "svc_keyboard.h"
#include "stack/ble/service/hids.h"

#if SVC_DEFAULT_KEYBOARD_ENABLE

const unsigned char hidReportMap[] =
    {
        //keyboard report in
        0x05,
        0x01,                         // Usage Pg (Generic Desktop)
        0x09,
        0x06,                         // Usage (Keyboard)
        0xA1,
        0x01,                         // Collection: (Application)
        0x85,
        HID_REPORT_ID_KEYBOARD_INPUT, // Report Id (keyboard)
        0x05,
        0x07,                         // Usage Pg (Key Codes)
        0x19,
        0xE0,                         // Usage Min (224)  VK_CTRL:0xe0
        0x29,
        0xE7,                         // Usage Max (231)  VK_RWIN:0xe7
        0x15,
        0x00,                         // Log Min (0)
        0x25,
        0x01,                         // Log Max (1)
                                      // Modifier byte
        0x75,
        0x01,                         // Report Size (1)   1 bit * 8
        0x95,
        0x08,                         // Report Count (8)
        0x81,
        0x02,                         // Input: (Data, Variable, Absolute)
                                      // Reserved byte
        0x95,
        0x01,                         // Report Count (1)
        0x75,
        0x08,                         // Report Size (8)
        0x81,
        0x01,                         // Input: (static constant)

        //keyboard output
        //5 bit led ctrl: NumLock CapsLock ScrollLock Compose kana
        0x95,
        0x05, //Report Count (5)
        0x75,
        0x01, //Report Size (1)
        0x05,
        0x08, //Usage Pg (LEDs )
        0x19,
        0x01, //Usage Min
        0x29,
        0x05, //Usage Max
        0x91,
        0x02, //Output (Data, Variable, Absolute)
        //3 bit reserved
        0x95,
        0x01, //Report Count (1)
        0x75,
        0x03, //Report Size (3)
        0x91,
        0x01, //Output (static constant)

        // Key arrays (6 bytes)
        0x95,
        0x06, // Report Count (6)
        0x75,
        0x08, // Report Size (8)
        0x15,
        0x00, // Log Min (0)
        0x25,
        0xF1, // Log Max (241)
        0x05,
        0x07, // Usage Pg (Key Codes)
        0x19,
        0x00, // Usage Min (0)
        0x29,
        0xf1, // Usage Max (241)
        0x81,
        0x00, // Input: (Data, Array)
        0xC0, // End Collection

        //consumer report in
        0x05,
        0x0C,                                // Usage Page (Consumer)
        0x09,
        0x01,                                // Usage (Consumer Control)
        0xA1,
        0x01,                                // Collection (Application)
        0x85,
        HID_REPORT_ID_CONSUME_CONTROL_INPUT, //     Report Id
        0x75,
        0x10,                                //global, report size 16 bits
        0x95,
        0x01,                                //global, report count 1
        0x15,
        0x01,                                //global, min  0x01
        0x26,
        0x8c,
        0x02,                                //global, max  0x28c
        0x19,
        0x01,                                //local, min   0x01
        0x2a,
        0x8c,
        0x02,                                //local, max    0x28c
        0x81,
        0x00,                                //main,  input data variable, absolute
        0xc0,                                //main, end collection
};

const unsigned short hidReportMapLen = sizeof(hidReportMap);
#endif
