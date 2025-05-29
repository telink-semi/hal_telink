/********************************************************************************************************
 * @file    hid_server_buf.h
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
#if ((!defined(HOST_V2_ENABLE)))
struct blc_hid_server
{
    u16 protocolModeHdl;             /* Protocol Mode */
    u16 bootKeyboardInputReportHdl;  /* Boot Keyboard Input Report */
    u16 bootKeyboardOutputReportHdl; /* Boot Keyboard Output Report */
    u16 bootMouseInputReportHdl;     /* Boot Mouse Input Report */
    u16 HIDInformationHdl;           /* HID Information */
    u16 HIDControlPointHdl;          /* HID Control Point */
    u16 reportMapHdl;                /* Report Map */

    struct __attribute__((packed))
    {
        u16                             attrHandle;
        struct hid_reportReferenceValue reportReferenceValue;
    } reportCharInfo[HID_SUPPORT_REPORT_HANDLE_MAX];

    u16 test;
} __attribute__((packed));

struct blc_hid_server_ctrl
{
    blc_prf_proc_t        process;
    struct blc_hid_server hidServer;
} __attribute__((packed));
#else
struct blc_hid_server
{
    u16 protocolModeHdl;             /* Protocol Mode */
    u16 bootKeyboardInputReportHdl;  /* Boot Keyboard Input Report */
    u16 bootKeyboardOutputReportHdl; /* Boot Keyboard Output Report */
    u16 bootMouseInputReportHdl;     /* Boot Mouse Input Report */
    u16 HIDInformationHdl;           /* HID Information */
    u16 HIDControlPointHdl;          /* HID Control Point */
    u16 reportMapHdl;                /* Report Map */

    struct
    {
        u16                             attrHandle;
        struct hid_reportReferenceValue reportReferenceValue;
    } reportCharInfo[HID_SUPPORT_REPORT_HANDLE_MAX];

    // struct __attribute__((packed)) {
    //     u16 attrHandle;
    //     u16 isSubscribe;
    // }descCCCCharInfo[HID_SUPPORT_REPORT_HANDLE_MAX];
    u16 test;
};

struct blc_hid_server_ctrl
{
    struct blc_prf_process process;
    struct blc_hid_server  hidServer;
};
#endif
