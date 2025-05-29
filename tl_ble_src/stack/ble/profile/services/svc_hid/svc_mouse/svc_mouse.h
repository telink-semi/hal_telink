/********************************************************************************************************
 * @file    svc_mouse.h
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

#ifndef SVC_DEFAULT_MOUSE_ENABLE
    #define SVC_DEFAULT_MOUSE_ENABLE 0
#endif

typedef struct
{
    union
    {
        struct
        {
            unsigned char left   : 1;
            unsigned char right  : 1;
            unsigned char middle : 1;
            unsigned char btn4   : 1;
            unsigned char btn5   : 1;
        };

        unsigned char button;
    };

    char x;
    char y;
    char wheel;
} blc_defaultMouseData_t;

#if SVC_DEFAULT_MOUSE_ENABLE
    #define HID_INPUT_REPORT_NUM            1
    #define HID_OUTPUT_REPORT_NUM           0
    #define HID_FEATURE_REPORT_NUM          0
    #define HID_BOOT_PROTOCOL_MODE_ENABLE   1
    #define HID_BOOT_KEYBOARD_INTPUT_ENABLE 0
    #define HID_BOOT_KEYBOARD_OUTPUT_ENABLE 0
    #define HID_BOOT_MOUSE_INTPUT_ENABLE    1

    #define HID_INPUT_REPORT_1_ID           HID_REPORT_ID_MOUSE_INPUT
#endif
