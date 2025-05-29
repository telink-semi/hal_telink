/********************************************************************************************************
 * @file    svc_hid.c
 *
 * @brief   This is the source file for TLSR/TL
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
#include <stddef.h>

#include "common/types.h"
#include "common/utility.h"
#include "common/compiler.h"

#include "../../../l2cap/att/inc/ble_att_uuid.h"
#include "../../../l2cap/att/inc/ble_att_service.h"

#include "../../../l2cap/att/inc/uuid16bit.h"

#include "../../inc/svc.h"
#include "../../inc/svc_format.h"

#include "../svc_hid.h"

#include "stack/ble/service/hids.h"

#include "svc_hid.h"

#include "vendor/common/user_config.h"
#include "../svc_keyboard/svc_keyboard.h"
#include "../svc_mouse/svc_mouse.h"
#include "../svc_km/svc_km.h"

#define HID_START_HDL                   SERVICE_HUMAN_INTERFACE_DEVICE_HDL

#ifndef HID_INPUT_REPORT_NUM
#define HID_INPUT_REPORT_NUM 1
#endif

#ifndef HID_OUTPUT_REPORT_NUM
#define HID_OUTPUT_REPORT_NUM 0
#endif

#ifndef HID_FEATURE_REPORT_NUM
#define HID_FEATURE_REPORT_NUM 0
#endif

#ifndef HID_BOOT_PROTOCOL_MODE_ENABLE
#define HID_BOOT_PROTOCOL_MODE_ENABLE 0
#endif

#ifndef HID_BOOT_KEYBOARD_INPUT_ENABLE
#define HID_BOOT_KEYBOARD_INPUT_ENABLE 0
#endif

#ifndef HID_BOOT_KEYBOARD_OUTPUT_ENABLE
#define HID_BOOT_KEYBOARD_OUTPUT_ENABLE 0
#endif

#ifndef HID_BOOT_MOUSE_INPUT_ENABLE
#define HID_BOOT_MOUSE_INPUT_ENABLE 0
#endif

#ifndef HID_INPUT_REPORT_1_ID
#define HID_INPUT_REPORT_1_ID           0x01
#endif

#ifndef HID_INPUT_REPORT_2_ID
#define HID_INPUT_REPORT_2_ID           0x02
#endif

#ifndef HID_INPUT_REPORT_2_ID
#define HID_INPUT_REPORT_2_ID           0x03
#endif

#ifndef HID_OUTPUT_REPORT_1_ID
#define HID_OUTPUT_REPORT_1_ID          0x11
#endif

#ifndef HID_OUTPUT_REPORT_2_ID
#define HID_OUTPUT_REPORT_2_ID          0x12
#endif

#ifndef HID_FEATURE_REPORT_1_ID
#define HID_FEATURE_REPORT_1_ID         0x21
#endif

#if HID_BOOT_PROTOCOL_MODE_ENABLE
//Protocol Mode
_attribute_ble_data_retention_
static uint8_t hidProtocolModeValue = HID_PROTOCOL_MODE_REPORT;
static const uint16_t hidProtocolModeValueLen = sizeof(hidProtocolModeValue);
#endif

#if HID_BOOT_KEYBOARD_INPUT_ENABLE
// Boot Keyboard Input Report
_attribute_ble_data_retention_
static struct hid_bootKeyboardInputValue hidBootKeyInReportValue;
static const uint16_t hidBootKeyInReportValueLen = sizeof(hidBootKeyInReportValue);
#endif

#if HID_BOOT_KEYBOARD_OUTPUT_ENABLE
// Boot Keyboard Output Report
_attribute_ble_data_retention_
static uint16_t hidBootKeyOutReportValue = 0x0003;
static const uint16_t hidBootKeyOutReportValueLen = sizeof(hidBootKeyOutReportValue);
#endif

#if HID_BOOT_MOUSE_INPUT_ENABLE
// Boot Mouse Input Report
_attribute_ble_data_retention_
static struct hid_bootMouseInputValue hidBootMouseInReportValue;
static const uint16_t hidBootMouseInReportValueLen = sizeof(hidBootMouseInReportValue);
#endif

extern const uint8_t  hidReportMap[];
extern const uint16_t hidReportMapLen;
static const uint8_t  hidReportMapExtServiceUuidVale[] = { 0x00, 0x00 };
static const uint16_t hidReportMapExtServiceUuidValeLen = sizeof(hidReportMapExtServiceUuidVale);

static const struct hid_hidInformationVale hidInformationValue = {
    .bcdHID = 0x0111, // bcdHID (USB HID version)
    .bCountCode = 0x00,   // bCountryCode
    .remoteWake = 0x01,
    .normallyConnectable = 0x00,
};
static const uint16_t hidInformationValueLen = sizeof(hidInformationValue);

#if HID_INPUT_REPORT_NUM > 0
static const uint8_t  hidReportInput1Value[] = { 0x00 };
static const uint16_t hidReportInput1ValueLen = sizeof(hidReportInput1Value);
static const uint8_t  hidReportInput1RefValue[2] = { HID_INPUT_REPORT_1_ID, HID_REPORT_TYPE_INPUT };
static const uint16_t hidReportInput1RefValueLen = sizeof(hidReportInput1RefValue);
#endif

#if HID_INPUT_REPORT_NUM > 1
static const uint8_t  hidReportInput2Value[] = { 0x00 };
static const uint16_t hidReportInput2ValueLen = sizeof(hidReportInput2Value);
static const uint8_t  hidReportInput2RefValue[2] = { HID_INPUT_REPORT_2_ID, HID_REPORT_TYPE_INPUT };
static const uint16_t hidReportInput2RefValueLen = sizeof(hidReportInput2RefValue);
#endif

#if HID_INPUT_REPORT_NUM > 2
static const uint8_t  hidReportInput3Value[] = { 0x00 };
static const uint16_t hidReportInput3ValueLen = sizeof(hidReportInput3Value);
static const uint8_t  hidReportInput3RefValue[2] = { HID_INPUT_REPORT_3_ID, HID_REPORT_TYPE_INPUT };
static const uint16_t hidReportInput3RefValueLen = sizeof(hidReportInput3RefValue);
#endif

#if HID_OUTPUT_REPORT_NUM > 0
static const uint8_t  hidReportOutput1Value[] = { 0x00 };
static const uint16_t hidReportOutput1ValueLen = sizeof(hidReportOutput1Value);
static const uint8_t  hidReportOutput1RefValue[2] = { HID_OUTPUT_REPORT_1_ID, HID_REPORT_TYPE_OUTPUT };
static const uint16_t hidReportOutput1RefValueLen = sizeof(hidReportOutput1RefValue);
#endif

#if HID_OUTPUT_REPORT_NUM > 1
static const uint8_t  hidReportOutput2Value[] = { 0x00 };
static const uint16_t hidReportOutput2ValueLen = sizeof(hidReportOutput2Value);
static const uint8_t  hidReportOutput2RefValue[2] = { HID_OUTPUT_REPORT_2_ID, HID_REPORT_TYPE_OUTPUT };
static const uint16_t hidReportOutput2RefValueLen = sizeof(hidReportOutput2RefValue);
#endif

#if HID_FEATURE_REPORT_NUM
static const uint8_t  hidReportFeatureValue[] = { 0x00 };
static const uint16_t hidReportFeatureValueLen = sizeof(hidReportFeatureValue);
static const uint8_t  hidReportFeatureRefValue[2] = { HID_FEATURE_REPORT_1_ID, HID_REPORT_TYPE_FEATURE };
static const uint16_t hidReportFeatureRefValueLen = sizeof(hidReportFeatureRefValue);
#endif

#define HID_DESCRIPTOR_REFERENCE(value) \
    {ATT_PERMISSIONS_RDWR, ATT_16_UUID_LEN, (uint8_t *)(size_t)descriptorReportReferenceUuid, (uint16_t *)(size_t) & value##Len, sizeof(value), (uint8_t *)(size_t)value, 0}

extern const uint16_t basIncludeVal[3];

/*
 * @brief the structure for default HID service List.
 */
static const struct atts_attribute hidList[] =
{
    ATTS_PRIMARY_SERVICE(serviceHumanInterfaceDeviceUuid),

    //include BAS
    ATTS_INCLUDE_DEFINE(&basIncludeVal[0]),

#if HID_BOOT_PROTOCOL_MODE_ENABLE
//protocol mode
ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteWithout, characteristicProtocolModeUuid, hidProtocolModeValue),
#endif

#if HID_BOOT_KEYBOARD_INPUT_ENABLE
    //boot keyboard input report
    ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteNotify, characteristicBootKeyboardInputReportUuid, hidBootKeyInReportValue),
    ATTS_COMMON_CCC_DEFINE,
#endif

#if HID_BOOT_KEYBOARD_OUTPUT_ENABLE
    //boot keyboard output report
    ATTS_CHAR_UUID_RDWR_ENTITY_WCB(charPropReadWriteWriteWithout, characteristicBootKeyboardOutputReportUuid, hidBootKeyOutReportValue),
#endif

#if HID_BOOT_MOUSE_INPUT_ENABLE
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
static struct atts_group svcHidGroup =
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
    svcHidGroup.endHandle = svcHidGroup.startHandle + ARRAY_SIZE(hidList) - 1;
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
    svcHidGroup.readCallback = readCback;
    svcHidGroup.writeCallback = writeCback;
}

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

#if (HID_BOOT_PROTOCOL_MODE_ENABLE == 0) && (HID_BOOT_KEYBOARD_INPUT_ENABLE || HID_BOOT_KEYBOARD_OUTPUT_ENABLE || HID_BOOT_MOUSE_INPUT_ENABLE)
#error "ERR: if want support Boot Mode, must enable Boot Protocol Mode"
#endif
