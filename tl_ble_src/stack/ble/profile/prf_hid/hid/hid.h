/********************************************************************************************************
 * @file    hid.h
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

// HID: Human Interface Device Service
// HIDC: Human Interface Device Service Client.
// HIDS: Human Interface Device Service

/******************************* HID Common Start **********************************************************************/
//Protocol Mode Characteristic Value
#define BOOT_PROTOCOL_MODE                  0x00
#define REPORT_PROTOCOL_MODE                0x01
#define CHECK_PROTOCOL_MODE(mode)           ((mode) == BOOT_PROTOCOL_MODE || (mode) == REPORT_PROTOCOL_MODE)

#define HID_SUPPORT_REPORT_HANDLE_MAX       8       //cannot change.

struct hid_reportReferenceValue{
    u8 reportId;
    u8 reportType;
};
/******************************* HID Common End **********************************************************************/

/******************************* HID Client Start **********************************************************************/
//HID Client Event ID
enum{
    HID_EVT_HIDC_START = HID_EVT_TYPE_HID_CLIENT,
    HIDC_EVT_RECV_BOOT_KEYBOARD_INPUT_REPORT,       //refer to struct blc_hidc_recvBootKeyboardInputReportEvt.
    HIDC_EVT_RECV_BOOT_MOUSE_INPUT_REPORT,          //refer to struct blc_hidc_recvBootMouseInputReportEvt.
    HIDC_EVT_RECV_INPUT_REPORT_DATA,                //refer to struct blc_hidc_recvInputReportData.
};

struct blc_hidc_regParam{

};

//refer to HIDC_EVT_RECV_BOOT_KEYBOARD_INPUT_REPORT,
struct blc_hidc_recvBootKeyboardInputReportEvt{
    u8* value;
    u16 len;
}__attribute__((packed));

//refer to HIDC_EVT_RECV_BOOT_MOUSE_INPUT_REPORT.
struct blc_hidc_recvBootMouseInputReportEvt{
    u8* value;
    u16 len;
}__attribute__((packed));

//refer to HIDC_EVT_RECV_INPUT_REPORT_DATA.
struct blc_hidc_recvInputReportData{
    u8 reportId;
    u8* value;
    u16 len;
}__attribute__((packed));

/**
 * @brief       for user to register human interface device service control client module.
 * @param[in]   param - currently not used, fixed NULL.
 * @return      none.
 */
void blc_hid_registerHIDControlClient(const struct blc_hidc_regParam *param);

//HID Client Read Characteristic Value Operation API
int blc_hidc_readProtocolMode(u16 connHandle, prf_read_cb_t readCb);
int blc_hidc_readReportMap(u16 connHandle, prf_read_cb_t readCb);
int blc_hidc_readBootKeyboardInputReport(u16 connHandle, prf_read_cb_t readCb);
int blc_hidc_readBootKeyboardOutputReport(u16 connHandle, prf_read_cb_t readCb);;
int blc_hidc_readBootMouseInputReport(u16 connHandle, prf_read_cb_t readCb);
int blc_hidc_readHIDInformation(u16 connHandle, prf_read_cb_t readCb);
int blc_hidc_readReport(u16 connHandle, struct hid_reportReferenceValue *reference, prf_read_cb_t readCb);
int blc_hidc_readReportInput(u16 connHandle, u8 reportId, prf_read_cb_t readCb);
int blc_hidc_readReportOutput(u16 connHandle, u8 reportId, prf_read_cb_t readCb);
int blc_hidc_readReportFeature(u16 connHandle, u8 reportId, prf_read_cb_t readCb);

//HID Client Write Characteristic Value Operation API
int blc_hidc_writeProtocolMode(u16 connHandle, u8 protocolMode);
int blc_hidc_writeBootProtocolMode(u16 connHandle);
int blc_hidc_writeReportProtocolMode(u16 connHandle);
int blc_hidc_writeBootKeyboardInput(u16 connHandle, hid_bootKeyboardInputValue_t *pBootKeyboardInputReport, prf_write_cb_t writeCb);
int blc_hidc_writeBootKeyboardIOutut(u16 connHandle, u16 bootKeyboardOutputReport, prf_write_cb_t writeCb);
int blc_hidc_writeBootKeyboardIOututWithout(u16 connHandle, u16 bootKeyboardOutputReport);
int blc_hidc_writeBootMouseInput(u16 connHandle, hid_bootMouseInputValue_t *pBootMouseInputReport, prf_write_cb_t writeCb);
int blc_hidc_writeHIDControlPointWithout(u16 connHandle, u8 HIDControlPoint);
int blc_hidc_writeEnterSuspend(u16 connHandle);
int blc_hidc_writeExitSuspend(u16 connHandle);
int blc_hidc_writeReport(u16 connHandle, struct hid_reportReferenceValue *reference, u8* value, u16 valueLen, prf_write_cb_t writeCb);
int blc_hidc_writeReportWithout(u16 connHandle, struct hid_reportReferenceValue *reference, u8* value, u16 valueLen);
int blc_hidc_writeReportOutput(u16 connHandle, u8 reportId, u8* value, u16 valueLen, prf_write_cb_t writeCb);
int blc_hidc_writeReportOutputWithout(u16 connHandle, u8 reportId, u8* value, u16 valueLen);
int blc_hidc_writeReportFeature(u16 connHandle, u8 reportId, u8* value, u16 valueLen, prf_write_cb_t writeCb);

//HID Client Get Characteristic Value Operation API
int blc_hidc_getProtocolMode(u16 connHandle, u8 *protocolMode);
int blc_hidc_getReportMap(u16 connHandle, u8 *reportMap, u16 *reportMapLen);
int blc_hidc_getBootKeyboardInputReport(u16 connHandle, hid_bootKeyboardInputValue_t *bootKeyboardInputReport);
int blc_hidc_getBootKeyboardOutputReport(u16 connHandle, u16 *bootKeyboardOutputReport);
int blc_hidc_getBootMouseInputReport(u16 connHandle, hid_bootMouseInputValue_t *bootMouseInputReport);
int blc_hidc_getHIDInformation(u16 connHandle, hid_hidInformationVale_t *HIDInformation);
int blc_hidc_getReport(u16 connHandle, struct hid_reportReferenceValue *reference, u8 *report, u16 *reportLen);
int blc_hidc_getReportInput(u16 connHandle, u8 reportId, u8 *report, u16 *reportLen);
int blc_hidc_getReportOutput(u16 connHandle, u8 reportId, u8 *report, u16 *reportLen);
int blc_hidc_getReportFeature(u16 connHandle, u8 reportId, u8 *report, u16 *reportLen);

int blc_usb_setProtocol(u16 connHandle, u8 protocolMode);
int blc_usb_setBootProtocolMode(u16 connHandle);
int blc_usb_setReportProtocolMode(u16 connHandle);
int blc_usb_getProtocol(u16 connHandle, prf_read_cb_t readCb);
int blc_usb_getReport(u16 connHandle, struct hid_reportReferenceValue *reference, prf_read_cb_t readCb);
int blc_usb_getReportInput(u16 connHandle, u8 reportId, prf_read_cb_t readCb);
int blc_usb_getReportOutput(u16 connHandle, u8 reportId, prf_read_cb_t readCb);
int blc_usb_getReportFeature(u16 connHandle, u8 reportId, prf_read_cb_t readCb);
int blc_usb_setReport(u16 connHandle, struct hid_reportReferenceValue *reference, u8* value, u16 valueLen, prf_write_cb_t writeCb);
int blc_usb_setReportOutput(u16 connHandle, u8 reportId, u8* value, u16 valueLen, prf_write_cb_t writeCb);
int blc_usb_dataOut(u16 connHandle, u8 reportId, u8* value, u16 valueLen);
int blc_usb_setReportFeature(u16 connHandle, u8 reportId, u8* value, u16 valueLen, prf_write_cb_t writeCb);
/******************************* HID Client End **********************************************************************/


/******************************* HID Server Start **********************************************************************/
//HID Server Event ID
enum{
    HID_EVT_HIDS_START = HID_EVT_TYPE_HID_SERVER,
    HIDS_EVT_PROTOCOL_MODE_CHANGE,              // refer to struct blc_hids_protocolModeChangeEvt.
    HIDS_EVT_RECV_BOOT_KEYBOARD_INPUT_REPORT,   //refer to struct blc_hids_recvBootKeyboardInputReportEvt.
    HIDS_EVT_RECV_BOOT_KEYBOARD_OUTPUT_REPORT,  //refer to struct blc_hids_recvBootKeyboardOutputReportEvt.
    HIDS_EVT_RECV_BOOT_MOUSE_INPUT_REPORT,      //refer to struct blc_hids_recvBootMouseInputReportEvt.
    HIDS_EVT_ENTER_SUSPEND_STATE,               //refer to None.
    HIDS_EVT_EXIT_SUSPEND_STATE,                //refer to None.
    HIDS_EVT_RECV_REPORT,                       //refer to struct blc_hids_recvReportEvt.
};

struct blc_hids_regParam{

};

//refer to HIDS_EVT_PROTOCOL_MODE_CHANGE.
struct blc_hids_protocolModeChangeEvt{
    u8 protocolMode;
};

//refer to HIDS_EVT_RECV_BOOT_KEYBOARD_INPUT_REPORT,
struct blc_hids_recvBootKeyboardInputReportEvt{
    u8* value;
    u16 len;
}__attribute__((packed));

//refer to HIDS_EVT_RECV_BOOT_KEYBOARD_OUTPUT_REPORT,
struct blc_hids_recvBootKeyboardOutputReportEvt{
    u8* value;
    u16 len;
}__attribute__((packed));

//refer to HIDS_EVT_RECV_BOOT_MOUSE_INPUT_REPORT,
struct blc_hids_recvBootMouseInputReportEvt{
    u8* value;
    u16 len;
}__attribute__((packed));

//refer to HIDS_EVT_RECV_REPORT.
struct blc_hids_recvReportEvt{
    u8 reportId;
    u8 reportType;
    u8* value;
    u16 len;
}__attribute__((packed));

/**
 * @brief       for user to register human interface device service control server module.
 * @param[in]   param - currently not used, fixed NULL.
 * @return      none.
 * note:if you want to modify the value, directly modify the svc_hid.c file.
 */
void blc_hid_registerHIDControlServer(const struct blc_hids_regParam *param);

void blc_hids_setBootKeyboardInput(hid_bootKeyboardInputValue_t* value);
void blc_hids_setBootMouseInput(hid_bootMouseInputValue_t* value);
int blc_hids_notifyBootKeyboardInput(u16 connHandle, hid_bootKeyboardInputValue_t* value);
int blc_hids_notifyBootMouseInput(u16 connHandle, hid_bootMouseInputValue_t* value);
int blc_hids_notifyInputReport(u16 connHandle, u8 reportID, u8* value, u16 valueLen);
/******************************* DIS Server End **********************************************************************/
