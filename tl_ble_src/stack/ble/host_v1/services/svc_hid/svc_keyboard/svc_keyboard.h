/********************************************************************************************************
 * @file    svc_keyboard.h
 *
 * @brief   This is the header file for TLSR/TL
 *
 * @author  Bluetooth Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd.
 *          All rights reserved.
 *
 *          The information contained herein is confidential property of Telink
 *          Semiconductor (Shanghai) Co., Ltd. and is available under the terms
 *          of Commercial License Agreement between Telink Semiconductor (Shanghai)
 *          Co., Ltd. and the licensee or the terms described here-in. This heading
 *          MUST NOT be removed from this file.
 *
 *          Licensee shall not delete, modify or alter (or permit any third party to delete, modify, or
 *          alter) any information contained herein in whole or in part except as expressly authorized
 *          by Telink semiconductor (shanghai) Co., Ltd. Otherwise, licensee shall be solely responsible
 *          for any claim to the extent arising out of or relating to such deletion(s), modification(s)
 *          or alteration(s).
 *
 *          Licensees are granted free, non-transferable use of the information in this
 *          file under Mutual Non-Disclosure Agreement. NO WARRANTY of ANY KIND is provided.
 *
 *******************************************************************************************************/
#pragma once

#ifndef SVC_DEFAULT_KEYBOARD_ENABLE
#define SVC_DEFAULT_KEYBOARD_ENABLE     0
#endif

typedef struct {
    union {
        struct {
            unsigned char rightGUI : 1;
            unsigned char rightAlt : 1;
            unsigned char rightShift : 1;
            unsigned char rightControl : 1;
            unsigned char leftGUI : 1;
            unsigned char leftAlt : 1;
            unsigned char leftShift : 1;
            unsigned char leftControl : 1;
        };
        unsigned char modifiers;
    };

    unsigned char padding;
    unsigned char key[6];
} blc_defaultKeyboardData_t;

typedef struct {
    unsigned short data;
} blc_defaultConsumerControlData_t;

#if SVC_DEFAULT_KEYBOARD_ENABLE
#define HID_INPUT_REPORT_NUM            2
#define HID_OUTPUT_REPORT_NUM           1
#define HID_FEATURE_REPORT_NUM          0
#define HID_BOOT_PROTOCOL_MODE_ENABLE   1
#define HID_BOOT_KEYBOARD_INPUT_ENABLE  1
#define HID_BOOT_KEYBOARD_OUTPUT_ENABLE 1
#define HID_BOOT_MOUSE_INPUT_ENABLE     0

#define HID_INPUT_REPORT_1_ID           HID_REPORT_ID_KEYBOARD_INPUT
#define HID_INPUT_REPORT_2_ID           HID_REPORT_ID_CONSUME_CONTROL_INPUT
#define HID_OUTPUT_REPORT_1_ID          HID_REPORT_ID_KEYBOARD_INPUT
#endif
