/********************************************************************************************************
 * @file    svc_hid.h
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

// HID: Human Interface Device Service

struct hid_hidInformationVale {
    uint16_t bcdHID;
    uint8_t  bCountCode;

    union {
        struct {
            uint8_t remoteWake : 1;
            uint8_t normallyConnectable : 1;
        };

        uint8_t Flags;
    };
}__attribute__((packed));

struct hid_bootKeyboardInputValue {
    union {
        struct {
            uint8_t rightGUI : 1;
            uint8_t rightAlt : 1;
            uint8_t rightShift : 1;
            uint8_t rightControl : 1;
            uint8_t leftGUI : 1;
            uint8_t leftAlt : 1;
            uint8_t leftShift : 1;
            uint8_t leftControl : 1;
        };

        uint8_t modifiers;
    };

    uint8_t vendorSpecific;
    uint8_t key[6];
};

struct hid_bootMouseInputValue {
    union {
        struct {
            uint8_t left : 1;
            uint8_t right : 1;
            uint8_t middle : 1;
        };

        uint8_t button;
    };

    int8_t x;
    int8_t y;
};

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


