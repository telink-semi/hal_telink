/********************************************************************************************************
 * @file    svc_hid.h
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

#include "vendor/common/user_config.h"
#include "../svc_keyboard/svc_keyboard.h"
#include "../svc_mouse/svc_mouse.h"
#include "../svc_km/svc_km.h"

// HID: Human Interface Device Service

#ifndef HID_INPUT_REPORT_NUM
#define HID_INPUT_REPORT_NUM                1
#endif

#ifndef HID_OUTPUT_REPORT_NUM
#define HID_OUTPUT_REPORT_NUM               0
#endif

#ifndef HID_FEATURE_REPORT_NUM
#define HID_FEATURE_REPORT_NUM              0
#endif

#ifndef HID_BOOT_PROTOCOL_MODE_ENABLE
#define HID_BOOT_PROTOCOL_MODE_ENABLE       0
#endif

#ifndef HID_BOOT_KEYBOARD_INTPUT_ENABLE
#define HID_BOOT_KEYBOARD_INTPUT_ENABLE     0
#endif

#ifndef HID_BOOT_KEYBOARD_OUTPUT_ENABLE
#define HID_BOOT_KEYBOARD_OUTPUT_ENABLE     0
#endif

#ifndef HID_BOOT_MOUSE_INTPUT_ENABLE
#define HID_BOOT_MOUSE_INTPUT_ENABLE        0
#endif


typedef struct __attribute__((packed)){
    u16 bcdHID;
    u8 bCountCode;
    union{
        struct{
            u8 remoteWake:1;
            u8 normallyConnectable:1;
        };
        u8 Flags;
    };
} hid_hidInformationVale_t;

typedef struct{
    union{
        struct{
            u8 rightGUI:1;
            u8 rightAlt:1;
            u8 rightShift:1;
            u8 rightControl:1;
            u8 leftGUI:1;
            u8 leftAlt:1;
            u8 leftShift:1;
            u8 leftControl:1;
        };
        u8 modifiers;
    };
    u8 vendorSpecific;
    u8 key[6];
} hid_bootKeyboardInputValue_t;

typedef struct{
    union{
        struct{
            u8 left:1;
            u8 right:1;
            u8 middle:1;
        };
        u8 button;
    };
    s8 x;
    s8 y;
} hid_bootMouseInputValue_t;

/**
 * @brief      for user add default HID service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addHidGroup(void);

/**
 * @brief      for user remove default HID service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeHidGroup(void);

/**
 * @brief      for user register read or write attribute value callback function in HID service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_hidCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback);

#if (HID_INPUT_REPORT_NUM + HID_OUTPUT_REPORT_NUM + HID_FEATURE_REPORT_NUM) == 0
#error "ERR: Mandatory to support at least one Report Type!!!"
#endif

#if HID_INPUT_REPORT_NUM > 3
#error "ERR: Maximum Input Report Type Count is 3"
#endif

#if HID_OUTPUT_REPORT_NUM > 2
#error "ERR: Maximum Output Report Type Count is 2"
#endif

#if HID_FEATURE_REPORT_NUM > 1
#error "ERR: Maximum Feature Report Type Count is 1"
#endif

#if (HID_BOOT_PROTOCOL_MODE_ENABLE == 0) && (HID_BOOT_KEYBOARD_INTPUT_ENABLE || HID_BOOT_KEYBOARD_OUTPUT_ENABLE || HID_BOOT_MOUSE_INTPUT_ENABLE)
#error "ERR: if want support Boot Mode, must enable Boot Protocol Mode"
#endif
