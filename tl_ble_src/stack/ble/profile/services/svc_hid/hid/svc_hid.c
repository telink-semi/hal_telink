/********************************************************************************************************
 * @file    svc_hid.c
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

#include "stack/ble/ble.h"

#define HID_START_HDL                       SERVICE_HUMAN_INTERFACE_DEVICE_HDL

#ifndef HID_INPUT_REPORT_1_ID
#define HID_INPUT_REPORT_1_ID               0x01
#endif

#ifndef HID_INPUT_REPORT_2_ID
#define HID_INPUT_REPORT_2_ID               0x02
#endif

#ifndef HID_INPUT_REPORT_2_ID
#define HID_INPUT_REPORT_2_ID               0x03
#endif

#ifndef HID_OUTPUT_REPORT_1_ID
#define HID_OUTPUT_REPORT_1_ID              0x11
#endif

#ifndef HID_OUTPUT_REPORT_2_ID
#define HID_OUTPUT_REPORT_2_ID              0x12
#endif

#ifndef HID_FEATURE_REPORT_1_ID
#define HID_FEATURE_REPORT_1_ID             0x21
#endif

#if HID_BOOT_PROTOCOL_MODE_ENABLE
//Protocol Mode
_attribute_ble_data_retention_
static u8 hidProtocolModeValue = HID_PROTOCOL_MODE_REPORT;
static const u16 hidProtocolModeValueLen = sizeof(hidProtocolModeValue);
#endif

#if HID_BOOT_KEYBOARD_INTPUT_ENABLE
// Boot Keyboard Input Report
_attribute_ble_data_retention_
static hid_bootKeyboardInputValue_t hidBootKeyInReportValue;
static const u16 hidBootKeyInReportValueLen = sizeof(hidBootKeyInReportValue);
#endif

#if HID_BOOT_KEYBOARD_OUTPUT_ENABLE
// Boot Keyboard Output Report
_attribute_ble_data_retention_
static u16 hidBootKeyOutReportValue = 0x0003;
static const u16 hidBootKeyOutReportValueLen = sizeof(hidBootKeyOutReportValue);
#endif

#if HID_BOOT_MOUSE_INTPUT_ENABLE
// Boot Mouse Input Report
_attribute_ble_data_retention_
static hid_bootMouseInputValue_t hidBootMouseInReportValue;
static const u16 hidBootMouseInReportValueLen = sizeof(hidBootMouseInReportValue);
#endif

extern const u8 hidReportMap[];
extern const u16 hidReportMapLen;
static const u8 hidReportMapExtServiceUuidVale[] = {0x00, 0x00};
static const u16 hidReportMapExtServiceUuidValeLen = sizeof(hidReportMapExtServiceUuidVale);

static const hid_hidInformationVale_t hidInformationValue = {
    .bcdHID = 0x0111,   // bcdHID (USB HID version)
    .bCountCode = 0x00, // bCountryCode
    .remoteWake = 0x01,
    .normallyConnectable = 0x00,
};
static const u16 hidInformationValueLen = sizeof(hidInformationValue);

#if HID_INPUT_REPORT_NUM > 0
    static const u8 hidReportInput1Value[] = {0x00};
    static const u16 hidReportInput1ValueLen = sizeof(hidReportInput1Value);
    static const u8 hidReportInput1RefValue[2] = {HID_INPUT_REPORT_1_ID, HID_REPORT_TYPE_INPUT};
    static const u16 hidReportInput1RefValueLen = sizeof(hidReportInput1RefValue);
#endif

#if HID_INPUT_REPORT_NUM > 1
    static const u8 hidReportInput2Value[] = {0x00};
    static const u16 hidReportInput2ValueLen = sizeof(hidReportInput2Value);
    static const u8 hidReportInput2RefValue[2] = {HID_INPUT_REPORT_2_ID, HID_REPORT_TYPE_INPUT};
    static const u16 hidReportInput2RefValueLen = sizeof(hidReportInput2RefValue);
#endif

#if HID_INPUT_REPORT_NUM > 2
    static const u8 hidReportInput3Value[] = {0x00};
    static const u16 hidReportInput3ValueLen = sizeof(hidReportInput3Value);
    static const u8 hidReportInput3RefValue[2] = {HID_INPUT_REPORT_3_ID, HID_REPORT_TYPE_INPUT};
    static const u16 hidReportInput3RefValueLen = sizeof(hidReportInput3RefValue);
#endif

#if HID_OUTPUT_REPORT_NUM > 0
    static const u8 hidReportOutput1Value[] = {0x00};
    static const u16 hidReportOutput1ValueLen = sizeof(hidReportOutput1Value);
    static const u8 hidReportOutput1RefValue[2] = {HID_OUTPUT_REPORT_1_ID, HID_REPORT_TYPE_OUTPUT};
    static const u16 hidReportOutput1RefValueLen = sizeof(hidReportOutput1RefValue);
#endif

#if HID_OUTPUT_REPORT_NUM > 1
    static const u8 hidReportOutput2Value[] = {0x00};
    static const u16 hidReportOutput2ValueLen = sizeof(hidReportOutput2Value);
    static const u8 hidReportOutput2RefValue[2] = {HID_OUTPUT_REPORT_2_ID, HID_REPORT_TYPE_OUTPUT};
    static const u16 hidReportOutput2RefValueLen = sizeof(hidReportOutput2RefValue);
#endif

#if HID_FEATURE_REPORT_NUM
    static const u8 hidReportFeatureValue[] = {0x00};
    static const u16 hidReportFeatureValueLen = sizeof(hidReportFeatureValue);
    static const u8 hidReportFeatureRefValue[2] = {HID_FEATURE_REPORT_1_ID, HID_REPORT_TYPE_FEATURE};
    static const u16 hidReportFeatureRefValueLen = sizeof(hidReportFeatureRefValue);
#endif

#define HID_DESCRIPTOR_REFERENCE(value)     \
    {ATT_PERMISSIONS_RDWR, ATT_16_UUID_LEN, (u8*)(size_t)descriptorReportReferenceUuid, (u16*)(size_t)&value##Len, sizeof(value), (u8*)(size_t)value, 0}

extern const u16 basIncludeVal[3];

/*
 * @brief the structure for default HID service List.
 */
static const atts_attribute_t hidList[] =
{
    ATTS_PRIMARY_SERVICE(serviceHumanInterfaceDeviceUuid),

    //include BAS
    ATTS_INCLUDE_DEFINE(&basIncludeVal[0]),

#if HID_BOOT_PROTOCOL_MODE_ENABLE
    //protocol mode
    ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteWithout, characteristicProtocolModeUuid, hidProtocolModeValue),
#endif

#if HID_BOOT_KEYBOARD_INTPUT_ENABLE
    //boot keyboard input report
    ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteNotify, characteristicBootKeyboardInputReportUuid, hidBootKeyInReportValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if HID_BOOT_KEYBOARD_OUTPUT_ENABLE
    //boot keyboard output report
    ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteWriteWithout, characteristicBootKeyboardOutputReportUuid, hidBootKeyOutReportValue),
#endif

#if HID_BOOT_MOUSE_INTPUT_ENABLE
    //boot mouse input report
    ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteNotify, characteristicBootMouseInputReportUuid, hidBootMouseInReportValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

    //hid map
    ATTS_CHAR_UUID_ENCR_READ_POINT_NOCB(charPropRead, characteristicReportMapUuid, hidReportMap),
    HID_DESCRIPTOR_REFERENCE(hidReportMapExtServiceUuidVale),

    //hid information
    ATTS_CHAR_UUID_READ_ENTITY_NOCB(charPropRead, characteristicHidInformationUuid, hidInformationValue),

    //hid control point
    ATTS_CHAR_UUID_WRITE_NULL(charPropWriteWithout, characteristicHidControlPointUuid),

#if HID_INPUT_REPORT_NUM > 0
    //report(input)
    ATTS_CHAR_UUID_RDWR_POINT_WCB(charPropReadWriteNotify, characteristicReportUuid, hidReportInput1Value),
    ATTS_COMMON_CCC_DEFINE,
    HID_DESCRIPTOR_REFERENCE(hidReportInput1RefValue),
#endif

#if HID_INPUT_REPORT_NUM > 1
    //report(input)
    ATTS_CHAR_UUID_RDWR_POINT_WCB(charPropReadWriteNotify, characteristicReportUuid, hidReportInput2Value),
    ATTS_COMMON_CCC_DEFINE,
    HID_DESCRIPTOR_REFERENCE(hidReportInput2RefValue),
#endif

#if HID_INPUT_REPORT_NUM > 2
    //report(input)
    ATTS_CHAR_UUID_RDWR_POINT_WCB(charPropReadWriteNotify, characteristicReportUuid, hidReportInput3Value),
    ATTS_COMMON_CCC_DEFINE,
    HID_DESCRIPTOR_REFERENCE(hidReportInput3RefValue),
#endif

#if HID_OUTPUT_REPORT_NUM > 0
    //report(output)
    ATTS_CHAR_UUID_RDWR_POINT_WCB(charPropReadWriteWriteWithout, characteristicReportUuid, hidReportOutput1Value),
    HID_DESCRIPTOR_REFERENCE(hidReportOutput1RefValue),
#endif

#if HID_OUTPUT_REPORT_NUM > 1
    //report(output)
    ATTS_CHAR_UUID_RDWR_POINT_WCB(charPropReadWriteWriteWithout, characteristicReportUuid, hidReportOutput2Value),
    HID_DESCRIPTOR_REFERENCE(hidReportOutput2RefValue),
#endif

#if HID_FEATURE_REPORT_NUM
    //report(feature)
    ATTS_CHAR_UUID_RDWR_POINT_WCB(charPropReadWrite, characteristicReportUuid, hidReportFeatureValue),
    HID_DESCRIPTOR_REFERENCE(hidReportFeatureRefValue),
#endif

};

/*
 * @brief the structure for default HID service group.
 */
_attribute_ble_data_retention_
static atts_group_t svcHidGroup =
{
    NULL,
    hidList,
    NULL,
    NULL,
    HID_START_HDL,
    0,
};

/**
 * @brief      for user add default HID service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_addHidGroup(void)
{
    svcHidGroup.endHandle = svcHidGroup.startHandle+ARRAY_SIZE(hidList)-1;
    blc_gatts_addAttributeServiceGroup(&svcHidGroup);
}

/**
 * @brief      for user remove default HID service in all GAP server.
 * @param[in]  none.
 * @return     none.
 */
void blc_svc_removeHidGroup(void)
{
    blc_gatts_removeAttributeServiceGroup(HID_START_HDL);
}

/**
 * @brief      for user register read or write attribute value callback function in HID service.
 * @param[in]  readCback: read attribute value callback function pointer.
 * @param[in]  writeCback: write attribute value callback function pointer.
 * @return     none.
 */
void blc_svc_hidCbackRegister(atts_r_cb_t readCback, atts_w_cb_t writeCback)
{
    svcHidGroup.readCback = readCback;
    svcHidGroup.writeCback = writeCback;
}
