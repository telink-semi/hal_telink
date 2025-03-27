/********************************************************************************************************
 * @file    hid_client_buf.h
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


struct blc_hid_client{
    gattc_sub_ccc_msg_t ntfInput;
    /* Characteristic value handle */
    u16 protocolModeHdl;                /* Protocol Mode */
    u16 bootKeyboardInputReportHdl;     /* Boot Keyboard Input Report */
    u16 bootKeyboardOutputReportHdl;    /* Boot Keyboard Output Report */
    u16 bootMouseInputReportHdl;        /* Boot Mouse Input Report */
    u16 HIDInformationHdl;              /* HID Information */
    u16 HIDControlPointHdl;             /* HID Control Point */
    u16 reportMapHdl;                   /* Report Map */

    u8 protocolModeProp;                /* Protocol Mode Properties */
    u8 bootKeyboardInputReportProp;     /* Boot Keyboard Input Report Properties */
    u8 bootKeyboardOutputReportProp;    /* Boot Keyboard Output Report Properties */
    u8 bootMouseInputReportProp;        /* Boot Mouse Input Report Properties */
    u8 HIDInformationProp;              /* HID Information Properties */
    u8 HIDControlPointProp;             /* HID Control Point Properties */
    u8 reportMapProp;                   /* Report Map Properties */

    struct __attribute__((packed)) {
        u16 attrHandle;
        u8 properties;
        u16 reportLen;
        u8 report[10];
        u16 cccHandle;
        u16 reportReferenceHandle;
        struct hid_reportReferenceValue reportReferenceValue;
    }reportCharInfo[HID_SUPPORT_REPORT_HANDLE_MAX];

    u8 protocolMode;
    hid_bootKeyboardInputValue_t bootKeyboardInputReport;
    u16 bootKeyboardOutputReport;
    hid_bootMouseInputValue_t bootMouseInputReport;
    hid_hidInformationVale_t HIDInformation;

    u16 reportMapLen;
    u8 reportMap[512];

    u8 reportCount;
    u8 reportFoundDescCount;

    u8 reserved;
}__attribute__((packed));

struct blc_hid_client_ctrl{
    blc_prf_proc_t process;
    struct blc_hid_client* pHidClient[STACK_PRF_ACL_CENTRAL_MAX_NUM];
}__attribute__((packed));


